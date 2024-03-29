#include "cacheline_concious_extendible_hash.h"

using namespace std;

#define GET_SEG_NUM(key, key_len, depth)  ((key>>(key_len-depth))&(((uint64_t)1<<depth)-1))
#define GET_BUCKET_NUM(key, bucket_mask_len) ((key)&(((uint64_t)1<<bucket_mask_len)-1))

#ifdef CCEH_PROFILE_TIME
timeval start_time, end_time;
uint64_t t1 = 0, t2 = 0, t3 = 0;
#endif

#ifdef CCEH_PROFILE_LOAD_FACTOR
uint64_t cceh_seg_num = 0;
#endif

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

int cceh_bucket::find_place(Key_t key, uint64_t depth, uint64_t key_len) {
    int res = -1;
    for (int i = 0; i < CCEH_BUCKET_SIZE; ++i) {
        if (key == kv[i].key) {
            return i;
        } else if ((res == -1) && kv[i].key == 0 && kv[i].value == 0) {
            res = i;
        } else if ((res == -1) && ((GET_SEG_NUM(key, key_len, depth)) != (GET_SEG_NUM(kv[i].key, key_len, depth)))) {
            res = i;
        }
    }
    return res;
}

Value_t cceh_bucket::get(Key_t key) {
    for (int i = 0; i < CCEH_BUCKET_SIZE; ++i) {
        if (key == kv[i].key) {
            return kv[i].value;
        }
    }
    return 0;
}

cceh_segment::cceh_segment() {
    depth = 0;
    bucket = static_cast<cceh_bucket *>(fast_alloc(sizeof(cceh_bucket) * CCEH_MAX_BUCKET_NUM));
}

cceh_segment::~cceh_segment() {

}

void cceh_segment::init(uint64_t _depth) {
    depth = _depth;
    bucket = static_cast<cceh_bucket *>(fast_alloc(sizeof(cceh_bucket) * CCEH_MAX_BUCKET_NUM));
}

cceh_segment *new_cceh_segment(uint64_t _depth) {
    cceh_segment *_new_cceh_segment = static_cast<cceh_segment *>(fast_alloc(sizeof(cceh_segment)));
    _new_cceh_segment->init(_depth);
    return _new_cceh_segment;
}


cacheline_concious_extendible_hash::cacheline_concious_extendible_hash() {
    global_depth = 0;
    dir_size = pow(2, global_depth);
    key_len = 64;
    dir = static_cast<cceh_segment **>(fast_alloc(sizeof(cceh_segment *) * dir_size));
    for (int i = 0; i < dir_size; ++i) {
        dir[i] = new_cceh_segment();
    }
#ifdef CCEH_PROFILE_LOAD_FACTOR
    cceh_seg_num = dir_size;
#endif
}

cacheline_concious_extendible_hash::~cacheline_concious_extendible_hash() {
}

void cacheline_concious_extendible_hash::init(uint64_t _global_depth, uint64_t _key_len) {
    global_depth = _global_depth;
    dir_size = pow(2, global_depth);
    key_len = _key_len;
    dir = static_cast<cceh_segment **>(fast_alloc(sizeof(cceh_segment *) * dir_size));
    for (int i = 0; i < dir_size; ++i) {
        dir[i] = new_cceh_segment(_global_depth);
    }
#ifdef CCEH_PROFILE_LOAD_FACTOR
    cceh_seg_num = dir_size;
#endif
}

void cacheline_concious_extendible_hash::put(Key_t key, Value_t value) {
    // value should not be 0
    uint64_t dir_index = GET_SEG_NUM(key, key_len, global_depth);
    cceh_segment *tmp_seg = dir[dir_index];
    uint64_t seg_index = GET_BUCKET_NUM(key, CCEH_BUCKET_MASK_LEN);
    cceh_bucket *tmp_bucket = &(tmp_seg->bucket[seg_index]);
    int bucket_index = tmp_bucket->find_place(key, tmp_seg->depth, key_len);
    if (bucket_index == -1) {
        //condition: full
        if (likely(tmp_seg->depth < global_depth)) {
#ifdef CCEH_PROFILE_TIME
            gettimeofday(&start_time, NULL);
#endif
#ifdef CCEH_PROFILE_LOAD_FACTOR
            cceh_seg_num++;
#endif
            cceh_segment *new_seg = new_cceh_segment(tmp_seg->depth + 1);
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

            //migrate previous data to the new segment
            for (int i = 0; i < CCEH_MAX_BUCKET_NUM; ++i) {
                uint64_t bucket_cnt = 0;
                for (int j = 0; j < CCEH_BUCKET_SIZE; ++j) {
                    uint64_t tmp_key = tmp_seg->bucket[i].kv[j].key;
                    uint64_t tmp_value = tmp_seg->bucket[i].kv[j].value;
                    dir_index = GET_SEG_NUM(tmp_key, key_len, global_depth);
                    if (dir_index >= mid) {
                        cceh_segment *dst_seg = new_seg;
                        seg_index = i;
                        cceh_bucket *dst_bucket = &(dst_seg->bucket[seg_index]);
                        dst_bucket->kv[bucket_cnt].value = tmp_value;
                        dst_bucket->kv[bucket_cnt].key = tmp_key;
                        bucket_cnt++;
                    }
                }
            }
            clflush((char *) new_seg, sizeof(cceh_segment));


            //set dir[mid, right) to the new segment
            for (int i = right - 1; i >= mid; --i) {
                dir[i] = new_seg;
//                clflush((char *) dir[i], sizeof(cceh_segment *));

            }
            clflush((char *) dir[right - 1], (stride / 2) * sizeof(cceh_segment *));

            tmp_seg->depth = tmp_seg->depth + 1;
            clflush((char *) &(tmp_seg->depth), sizeof(tmp_seg->depth));
#ifdef CCEH_PROFILE_TIME
            gettimeofday(&end_time, NULL);
            t2+=(end_time.tv_sec - start_time.tv_sec) * 1000000 + end_time.tv_usec - start_time.tv_usec;
#endif
            put(key, value);
            return;
        } else {
            //condition: tmp_seg->depth == global_depth
#ifdef CCEH_PROFILE_TIME
            gettimeofday(&start_time, NULL);
#endif
#ifdef CCEH_PROFILE_LOAD_FACTOR
            cceh_seg_num++;
#endif
            global_depth += 1;
            dir_size *= 2;
            //set dir
            cceh_segment **new_dir = static_cast<cceh_segment **>(fast_alloc(sizeof(cceh_segment *) * dir_size));
            for (int i = 0; i < dir_size; ++i) {
                new_dir[i] = dir[i / 2];
            }
            cceh_segment *new_seg = new_cceh_segment(global_depth);
            new_dir[dir_index * 2 + 1] = new_seg;
            //migrate previous data
            for (int i = 0; i < CCEH_MAX_BUCKET_NUM; ++i) {
                uint64_t bucket_cnt = 0;
                uint64_t new_dir_index = 0;
                for (int j = 0; j < CCEH_BUCKET_SIZE; ++j) {
                    uint64_t tmp_key = tmp_seg->bucket[i].kv[j].key;
                    uint64_t tmp_value = tmp_seg->bucket[i].kv[j].value;
                    new_dir_index = GET_SEG_NUM(tmp_key, key_len, global_depth);
                    if ((dir_index * 2 + 1) == new_dir_index) {
                        cceh_segment *dst_seg = new_seg;
                        seg_index = i;
                        cceh_bucket *dst_bucket = &(dst_seg->bucket[seg_index]);
                        dst_bucket->kv[bucket_cnt].key = tmp_key;
                        dst_bucket->kv[bucket_cnt].value = tmp_value;
                        bucket_cnt++;
                    }
                }
            }

            clflush((char *) new_seg, sizeof(cceh_segment));
            clflush((char *) new_dir, sizeof(cceh_segment *) * dir_size);

            dir = new_dir;
            clflush((char *) dir, sizeof(dir));

            tmp_seg->depth = tmp_seg->depth + 1;
            clflush((char *) &(tmp_seg->depth), sizeof(tmp_seg->depth));
#ifdef CCEH_PROFILE_TIME
            gettimeofday(&end_time, NULL);
            t3+=(end_time.tv_sec - start_time.tv_sec) * 1000000 + end_time.tv_usec - start_time.tv_usec;
#endif

            put(key, value);
            return;
        }
    } else {
#ifdef CCEH_PROFILE_TIME
        gettimeofday(&start_time, NULL);
#endif
        if (unlikely(tmp_bucket->kv[bucket_index].key == key)) {
            //key exists
            tmp_bucket->kv[bucket_index].value = value;
            clflush((char *) &(tmp_bucket->kv[bucket_index].value), 8);

        } else {
            // there is a place to insert
            tmp_bucket->kv[bucket_index].value = value;
            mfence();
            tmp_bucket->kv[bucket_index].key = key;
            clflush((char *) &(tmp_bucket->kv[bucket_index].key), 16);
        }
#ifdef CCEH_PROFILE_TIME
        gettimeofday(&end_time, NULL);
        t1+=(end_time.tv_sec - start_time.tv_sec) * 1000000 + end_time.tv_usec - start_time.tv_usec;
#endif
    }
}

Value_t cacheline_concious_extendible_hash::get(Key_t key) {
    uint64_t dir_index = GET_SEG_NUM(key, key_len, global_depth);
    cceh_segment *tmp_seg = dir[dir_index];
    uint64_t seg_index = GET_BUCKET_NUM(key, CCEH_BUCKET_MASK_LEN);
    cceh_bucket *tmp_bucket = &(tmp_seg->bucket[seg_index]);
    return tmp_bucket->get(key);
}

cacheline_concious_extendible_hash *new_cceh(uint64_t _global_depth, uint64_t _key_len) {
    cacheline_concious_extendible_hash *_new_cceh = static_cast<cacheline_concious_extendible_hash *>(fast_alloc(
            sizeof(cacheline_concious_extendible_hash)));
    _new_cceh->init(_global_depth, _key_len);
    return _new_cceh;
}
