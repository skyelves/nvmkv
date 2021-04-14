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
uint64_t ht_bucket_num = 0;
#endif

#define GET_DIR_NUM(key, key_len, depth)  ((key>>(key_len-depth))&(((uint64_t)1<<depth)-1))
//#define GET_DIR_NUM(key, key_len, depth)  ((key>>(key_len-depth))&0xffff)

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

ht_bucket::ht_bucket() {}

ht_bucket::ht_bucket(int _depth) {
    depth = _depth;
}

inline void ht_bucket::init(int _depth) {
    depth = _depth;
}

void ht_bucket::set_depth(int _depth) {
    depth = _depth;
}

uint64_t ht_bucket::get(uint64_t key) {
    for (int i = 0; i < BUCKET_SIZE; ++i) {
        if (key == counter[i].key) {
            return counter[i].value;
        }
    }
    return 0;
}

int ht_bucket::find_place(uint64_t _key, uint64_t _key_len) {
    // full: return -1
    // exists or not full: return index or empty counter
    int res = -1;
    for (int i = 0; i < BUCKET_SIZE; ++i) {
        if (_key == counter[i].key) {
            return i;
        } else if ((res == -1) && counter[i].key == 0 && counter[i].value == 0) {
            res = i;
        } else if ((res == -1) &&
                   (GET_DIR_NUM(_key, _key_len, depth) != GET_DIR_NUM(counter[i].key, _key_len, depth))) {
            res = i;
        }
    }
    return res;
}

ht_bucket *new_ht_bucket(int _depth) {
#ifdef HT_PROFILE_LOAD_FACTOR
    ht_bucket_num++;
#endif
    ht_bucket *_new_bucket = static_cast<ht_bucket *>(fast_alloc(sizeof(ht_bucket)));
    _new_bucket->init(_depth);
    return _new_bucket;
}

hashtree_node::hashtree_node() {
    dir = static_cast<ht_bucket **>(fast_alloc(sizeof(ht_bucket *) * dir_size));
    for (int i = 0; i < dir_size; ++i) {
        dir[i] = new_ht_bucket();
    }
}

hashtree_node::hashtree_node(int _global_depth, int _key_len) {
    global_depth = _global_depth;
    key_len = _key_len;
    dir_size = pow(2, global_depth);
    dir = static_cast<ht_bucket **>(fast_alloc(sizeof(ht_bucket *) * dir_size));
    for (int i = 0; i < dir_size; ++i) {
        dir[i] = new_ht_bucket(_global_depth);
    }
}

hashtree_node::~hashtree_node() {
}

void hashtree_node::init(uint32_t _global_depth, int _key_len) {
    type = 0;
    global_depth = _global_depth;
    key_len = _key_len;
    dir_size = pow(2, global_depth);
//    dir = new bucket *[dir_size];
    dir = static_cast<ht_bucket **>(fast_alloc(sizeof(ht_bucket *) * dir_size));
    for (int i = 0; i < dir_size; ++i) {
//        dir[i] = new bucket(_global_depth);
        dir[i] = new_ht_bucket(_global_depth);
    }
}

void hashtree_node::put(uint64_t key, uint64_t value) {
    // value should not be 0
    uint64_t index = GET_DIR_NUM(key, key_len, global_depth);
    ht_bucket *tmp_bucket = dir[index];
    int bucket_index = tmp_bucket->find_place(key, key_len);
    if (bucket_index == -1) {
        //condition: full
        if (likely(tmp_bucket->depth < global_depth)) {
#ifdef HT_PROFILE_TIME
            gettimeofday(&start_time, NULL);
#endif
            ht_bucket *new_bucket = new_ht_bucket(tmp_bucket->depth + 1);
            //set dir [left,right)
            int64_t left = index, mid = index, right = index + 1;
            for (int i = index + 1; i < dir_size; ++i) {
                if (likely(dir[i] != tmp_bucket)) {
                    right = i;
                    break;
                } else if (unlikely(i == (dir_size - 1))) {
                    right = dir_size;
                    break;
                }
            }
            for (int i = index - 1; i >= 0; --i) {
                if (likely(dir[i] != tmp_bucket)) {
                    left = i + 1;
                    break;
                } else if (unlikely(i == 0)) {
                    left = 0;
                    break;
                }
            }
            mid = (left + right) / 2;

            //migrate previous data to the new bucket
            uint64_t bucket_cnt = 0;
            for (int i = 0; i < BUCKET_SIZE; ++i) {
                uint64_t tmp_key = tmp_bucket->counter[i].key;
                uint64_t tmp_value = tmp_bucket->counter[i].value;
                index = GET_DIR_NUM(tmp_key, key_len, global_depth);
                if (index >= mid) {
                    ht_bucket *dst_bucket = new_bucket;
                    dst_bucket->counter[bucket_cnt].key = tmp_key;
                    dst_bucket->counter[bucket_cnt].value = tmp_value;
                    bucket_cnt++;
                }
            }
            mfence();
            clflush((char *) new_bucket, sizeof(ht_bucket));

            // set dir[mid, right) to the new bucket
            for (int i = right - 1; i >= mid; --i) {
                dir[i] = new_bucket;
                mfence();
                clflush((char *) dir[i], sizeof(ht_bucket *));
            }

            tmp_bucket->depth = tmp_bucket->depth + 1;
            mfence();
            clflush((char *) &(tmp_bucket->depth), sizeof(tmp_bucket->depth));
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
            global_depth += 1;
            dir_size *= 2;
            //set dir
            ht_bucket **new_dir = static_cast<ht_bucket **>(fast_alloc(sizeof(ht_bucket *) * dir_size));
            for (int i = 0; i < dir_size; ++i) {
                new_dir[i] = dir[i / 2];
            }
            clflush((char *) new_dir, sizeof(ht_bucket *) * dir_size);
            ht_bucket *new_bucket = new_ht_bucket(global_depth);
            new_dir[index * 2 + 1] = new_bucket;
            //migrate previous data
            uint64_t bucket_cnt = 0;
            uint64_t new_index = 0;
            for (int i = 0; i < BUCKET_SIZE; ++i) {
                uint64_t tmp_key = tmp_bucket->counter[i].key;
                int64_t tmp_value = tmp_bucket->counter[i].value;
                new_index = GET_DIR_NUM(tmp_key, key_len, global_depth);
                if ((index * 2 + 1) == new_index) {
                    ht_bucket *dst_bucket = new_bucket;
                    dst_bucket->counter[bucket_cnt].key = tmp_key;
                    dst_bucket->counter[bucket_cnt].value = tmp_value;
                    bucket_cnt++;
                }
            }
            mfence();
            clflush((char *) new_bucket, sizeof(ht_bucket));

            dir = new_dir;
            mfence();
            clflush((char *) dir, sizeof(dir));
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
            mfence();
            clflush((char *) &(tmp_bucket->counter[bucket_index].value), 8);
        } else {
            // there is a place to insert
            tmp_bucket->counter[bucket_index].value = value;
            mfence();
            tmp_bucket->counter[bucket_index].key = key;
            mfence();
            clflush((char *) &(tmp_bucket->counter[bucket_index].key), 16);
        }
    }
#ifdef HT_PROFILE_TIME
    gettimeofday(&end_time, NULL);
    t1 += (end_time.tv_sec - start_time.tv_sec) * 1000000 + end_time.tv_usec - start_time.tv_usec;
#endif
    return;
}

uint64_t hashtree_node::get(uint64_t key) {
    uint64_t index = GET_DIR_NUM(key, key_len, global_depth);
    return dir[index]->get(key);
}

hashtree_node *new_hashtree_node(int _global_depth, int _key_len) {
    hashtree_node *_new_node = static_cast<hashtree_node *>(fast_alloc(sizeof(hashtree_node)));
    _new_node->init(_global_depth, _key_len);
    return _new_node;
}