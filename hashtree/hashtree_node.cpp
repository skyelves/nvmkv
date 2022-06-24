//
// Created by 王柯 on 2021-03-04.
//

#include <stdio.h>
#include "hashtree_node.h"

#ifdef HT_PROFILE_TIME
timeval start_time, end_time;
uint64_t t1, t2, t3;
#endif

#ifdef HT_PROFILE_LOAD_FACTOR
uint64_t ht_seg_num = 0;
uint64_t ht_dir_num = 0;
#endif

#define GET_SEG_NUM(key, key_len, depth)  ((key>>(key_len-depth))&((0x1ull<<depth)-1))
#define GET_BUCKET_NUM(key, bucket_mask_len) ((key)&((0x1ull<<bucket_mask_len)-1))

inline void mfence(void) {
    asm volatile("mfence":: :"memory");
}

inline void clflush(char *data, size_t len) {
    volatile char *ptr = (char *) ((unsigned long) data & (~(CACHELINESIZE - 1)));
    mfence();
    for (; ptr < data + len; ptr += CACHELINESIZE) {
        asm volatile("clflush %0" : "+m" (*(volatile char *) ptr));
    }
    mfence();
}

ht_key_value *new_ht_key_value(uint64_t key, uint64_t value) {
    ht_key_value *_new_key_value = static_cast<ht_key_value *>(fast_alloc(sizeof(ht_key_value)));
    _new_key_value->type = 1;
    _new_key_value->key = key;
    _new_key_value->value = value;
    return _new_key_value;
}

uint64_t ht_bucket::get(uint64_t key) {
    for (int i = 0; i < HT_BUCKET_SIZE; ++i) {
        if (key == counter[i].key) {
            return counter[i].value;
        }
    }
    return 0;
}

int ht_bucket::find_place(uint64_t _key, uint64_t _key_len, uint64_t _depth) {
    // full: return -1
    // exists or not full: return index or empty counter
    int res = -1;
    for (int i = 0; i < HT_BUCKET_SIZE; ++i) {
        if (_key == counter[i].key) {
            return i;
        } else if ((res == -1) && counter[i].key == 0 && counter[i].value == 0) {
            res = i;
        } else if ((res == -1) &&
                   (GET_SEG_NUM(_key, _key_len, _depth) != GET_SEG_NUM(counter[i].key, _key_len, _depth))) {
            res = i;
        }
    }
    return res;
}

ht_bucket *new_ht_bucket(int _depth) {
    ht_bucket *_new_bucket = static_cast<ht_bucket *>(fast_alloc(sizeof(ht_bucket)));
    return _new_bucket;
}

ht_segment::ht_segment() {
    depth = 0;
    bucket = static_cast<ht_bucket *>(fast_alloc(sizeof(ht_bucket) * HT_MAX_BUCKET_NUM));
}

ht_segment::~ht_segment() {}

void ht_segment::init(uint64_t _depth) {
    depth = _depth;
    bucket = static_cast<ht_bucket *>(fast_alloc(sizeof(ht_bucket) * HT_MAX_BUCKET_NUM));
#ifdef HT_PROFILE_LOAD_FACTOR
    ht_seg_num++;
#endif
}

ht_segment *new_ht_segment(uint64_t _depth) {
    ht_segment *_new_ht_segment = static_cast<ht_segment *>(fast_alloc(sizeof(ht_segment)));
    _new_ht_segment->init(_depth);
    return _new_ht_segment;
}

hashtree_node::hashtree_node() {
    type = 0;
    global_depth = 0;
    key_len = 16;
    dir_size = pow(2, global_depth);
    dir = static_cast<ht_segment **>(fast_alloc(sizeof(ht_segment *) * dir_size));
    for (int i = 0; i < dir_size; ++i) {
        dir[i] = new_ht_segment();
    }
}

hashtree_node::~hashtree_node() {

}

void hashtree_node::init(uint32_t _global_depth, int _key_len) {
    type = 0;
    global_depth = _global_depth;
    key_len = _key_len;
    dir_size = pow(2, global_depth);
    dir = static_cast<ht_segment **>(fast_alloc(sizeof(ht_segment *) * dir_size));
    for (int i = 0; i < dir_size; ++i) {
        dir[i] = new_ht_segment(_global_depth);
    }
#ifdef HT_PROFILE_LOAD_FACTOR
    ht_dir_num += dir_size;
#endif
}

void hashtree_node::put(uint64_t key, uint64_t value) {
    // value should not be 0
    uint64_t dir_index = GET_SEG_NUM(key, key_len, global_depth);
    ht_segment *tmp_seg = dir[dir_index];
    uint64_t seg_index = GET_BUCKET_NUM(key, HT_BUCKET_MASK_LEN);
    ht_bucket *tmp_bucket = &(tmp_seg->bucket[seg_index]);
    int bucket_index = tmp_bucket->find_place(key, key_len, tmp_seg->depth);
    if (bucket_index == -1) {
        //condition: full
        if (likely(tmp_seg->depth < global_depth)) {
#ifdef HT_PROFILE_TIME
            gettimeofday(&start_time, NULL);
#endif
            ht_segment *new_seg = new_ht_segment(tmp_seg->depth + 1);
            //set dir [left,right)
//            int64_t left = dir_index, mid = dir_index, right = dir_index + 1;
//            for (int i = dir_index + 1; i < dir_size; ++i) {
//                if (likely(dir[i] != tmp_seg)) {
//                    right = i;
//                    break;
//                } else if (unlikely(i == (dir_size - 1))) {
//                    right = dir_size;
//                    break;
//                }
//            }
//            for (int i = dir_index - 1; i >= 0; --i) {
//                if (likely(dir[i] != tmp_seg)) {
//                    left = i + 1;
//                    break;
//                } else if (unlikely(i == 0)) {
//                    left = 0;
//                    break;
//                }
//            }
//            mid = (left + right) / 2;

            int64_t stride = pow(2, global_depth - tmp_seg->depth);
            int64_t left = dir_index - dir_index % stride;
            int64_t mid = left + stride / 2, right = left + stride;


            //migrate previous data to the new bucket
            for (int i = 0; i < HT_MAX_BUCKET_NUM; ++i) {
                uint64_t bucket_cnt = 0;
                for (int j = 0; j < HT_BUCKET_SIZE; ++j) {
//                    bool tmp_type = tmp_seg->bucket[i].counter[j].type;
                    uint64_t tmp_key = tmp_seg->bucket[i].counter[j].key;
                    uint64_t tmp_value = tmp_seg->bucket[i].counter[j].value;
                    dir_index = GET_SEG_NUM(tmp_key, key_len, global_depth);
                    if (dir_index >= mid) {
                        ht_segment *dst_seg = new_seg;
                        seg_index = i;
                        ht_bucket *dst_bucket = &(dst_seg->bucket[seg_index]);
                        dst_bucket->counter[bucket_cnt].value = tmp_value;
                        dst_bucket->counter[bucket_cnt].key = tmp_key;
//                        dst_bucket->counter[bucket_cnt].type = tmp_type;
                        bucket_cnt++;
                    }
                }
            }
            clflush((char *) new_seg, sizeof(ht_segment));

            // set dir[mid, right) to the new bucket
            for (int i = right - 1; i >= mid; --i) {
                dir[i] = new_seg;
//                clflush((char *) dir[i], sizeof(ht_segment *));
            }
            clflush((char *) dir[right - 1], (stride / 2) * sizeof(ht_segment *));

            tmp_seg->depth = tmp_seg->depth + 1;
            clflush((char *) &(tmp_seg->depth), sizeof(tmp_seg->depth));
#ifdef HT_PROFILE_TIME
            gettimeofday(&end_time, NULL);
            t2 += (end_time.tv_sec - start_time.tv_sec) * 1000000 + end_time.tv_usec - start_time.tv_usec;
#endif
            put(key, value);
            return;
        } else {
            //condition: tmp_bucket->depth == global_depth
#ifdef HT_PROFILE_TIME
            gettimeofday(&start_time, NULL);
#endif
#ifdef HT_PROFILE_LOAD_FACTOR
            ht_dir_num += dir_size;
#endif
            global_depth += 1;
            dir_size *= 2;
            //set dir
            ht_segment **new_dir = static_cast<ht_segment **>(fast_alloc(sizeof(ht_segment *) * dir_size));
            for (int i = 0; i < dir_size; ++i) {
                new_dir[i] = dir[i / 2];
            }
            ht_segment *new_seg = new_ht_segment(global_depth);
            new_dir[dir_index * 2 + 1] = new_seg;
            //migrate previous data
            for (int i = 0; i < HT_MAX_BUCKET_NUM; ++i) {
                uint64_t bucket_cnt = 0;
                uint64_t new_dir_index = 0;
                for (int j = 0; j < HT_BUCKET_SIZE; ++j) {
//                    bool tmp_type = tmp_seg->bucket[i].counter[j].type;
                    uint64_t tmp_key = tmp_seg->bucket[i].counter[j].key;
                    uint64_t tmp_value = tmp_seg->bucket[i].counter[j].value;
                    new_dir_index = GET_SEG_NUM(tmp_key, key_len, global_depth);
                    if ((dir_index * 2 + 1) == new_dir_index) {
                        ht_segment *dst_seg = new_seg;
                        seg_index = i;
                        ht_bucket *dst_bucket = &(dst_seg->bucket[seg_index]);
                        dst_bucket->counter[bucket_cnt].key = tmp_key;
                        dst_bucket->counter[bucket_cnt].value = tmp_value;
//                        dst_bucket->counter[bucket_cnt].type = tmp_type;
                        bucket_cnt++;
                    }
                }
            }

            clflush((char *) new_seg, sizeof(ht_segment));
            clflush((char *) new_dir, sizeof(ht_segment *) * dir_size);

            dir = new_dir;
            clflush((char *) dir, sizeof(dir));

            tmp_seg->depth = tmp_seg->depth + 1;
            clflush((char *) &(tmp_seg->depth), sizeof(tmp_seg->depth));
#ifdef HT_PROFILE_TIME
            gettimeofday(&end_time, NULL);
            t3 += (end_time.tv_sec - start_time.tv_sec) * 1000000 + end_time.tv_usec - start_time.tv_usec;
#endif
            put(key, value);
            return;
        }
    } else {
#ifdef HT_PROFILE_TIME
        gettimeofday(&start_time, NULL);
#endif
        if (unlikely(tmp_bucket->counter[bucket_index].key == key)) {
            //key exists
            tmp_bucket->counter[bucket_index].value = value;
            clflush((char *) &(tmp_bucket->counter[bucket_index].value), 8);
        } else {
            // there is a place to insert
            tmp_bucket->counter[bucket_index].value = value;
            mfence();
            tmp_bucket->counter[bucket_index].key = key;
            // Here we clflush 16bytes rather than two 8 bytes because all counter are set to 0.
            // If crash after key flushed, then the value is 0. When we return the value, we would find that the key is not inserted.
            clflush((char *) &(tmp_bucket->counter[bucket_index].key), 16);
        }
#ifdef HT_PROFILE_TIME
        gettimeofday(&end_time, NULL);
        t1 += (end_time.tv_sec - start_time.tv_sec) * 1000000 + end_time.tv_usec - start_time.tv_usec;
#endif
    }
    return;
}

uint64_t hashtree_node::get(uint64_t key) {
    uint64_t dir_index = GET_SEG_NUM(key, key_len, global_depth);
    ht_segment *tmp_seg = dir[dir_index];
    uint64_t seg_index = GET_BUCKET_NUM(key, HT_BUCKET_MASK_LEN);
    ht_bucket *tmp_bucket = &(tmp_seg->bucket[seg_index]);
    return tmp_bucket->get(key);
}

hashtree_node *new_hashtree_node(int _global_depth, int _key_len) {
    hashtree_node *_new_node = static_cast<hashtree_node *>(fast_alloc(sizeof(hashtree_node)));
    _new_node->init(_global_depth, _key_len);
    return _new_node;
}