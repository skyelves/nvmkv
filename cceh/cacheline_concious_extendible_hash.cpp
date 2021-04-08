#include <iostream>
#include <stdio.h>
#include "cacheline_concious_extendible_hash.h"

using namespace std;

inline void mfence(void) {
    asm volatile("mfence":: :"memory");
}

inline void clflush(char *data, size_t len) {
    volatile char *ptr = (char *) ((unsigned long) data & (~(CacheLineSize - 1)));
    mfence();
    for (; ptr < data + len; ptr += CacheLineSize) {
        asm volatile("clflush %0" : "+m" (*(volatile char *) ptr));
    }
    mfence();
}

int cceh_bucket::find_place(Key_t key, uint64_t depth) {
    int res = -1;
    for (int i = 0; i < CCEH_BUCKET_SIZE; ++i) {
        if (key == kv[i].key) {
            return i;
        } else if ((res == -1) && kv[i].key == 0) {
            res = i;
        } else if ((res == -1) && ((GET_SEG_NUM(key, 64, depth)) != (GET_SEG_NUM(kv[i].key, 64, depth)))) {
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
    bucket_mask_len = 8;
    max_num = pow(2, bucket_mask_len);
    depth = 0;
    cnt = 0;
    bucket = static_cast<cceh_bucket *>(fast_alloc(sizeof(cceh_bucket) * max_num));
}

cceh_segment::~cceh_segment() {

}

void cceh_segment::init(uint64_t _depth, uint64_t _bucket_mask_len) {
    bucket_mask_len = _bucket_mask_len;
    max_num = pow(2, bucket_mask_len);
    depth = _depth;
    cnt = 0;
    bucket = static_cast<cceh_bucket *>(fast_alloc(sizeof(cceh_bucket) * max_num));
}

cceh_segment *new_cceh_segment(uint64_t _depth, uint64_t _bucket_mask_len) {
    cceh_segment *_new_cceh_segment = static_cast<cceh_segment *>(fast_alloc(sizeof(cceh_segment)));
    _new_cceh_segment->init(_depth, _bucket_mask_len);
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
}

void cacheline_concious_extendible_hash::put(Key_t key, Value_t value) {
    // value should not be 0
    uint64_t dir_index = GET_SEG_NUM(key, key_len, global_depth);
    cceh_segment *tmp_seg = dir[dir_index];
    uint64_t seg_index = GET_BUCKET_NUM(key, tmp_seg->bucket_mask_len);
    cceh_bucket *tmp_bucket = &(tmp_seg->bucket[seg_index]);
    int bucket_index = tmp_bucket->find_place(key, tmp_seg->depth);
    if (bucket_index == -1) {
        //condition: full
        if (likely(tmp_seg->depth < global_depth)) {
            cceh_segment *new_seg1 = new_cceh_segment(tmp_seg->depth + 1);
            cceh_segment *new_seg2 = new_cceh_segment(tmp_seg->depth + 1);
            //set dir [left,right)
            uint64_t left = dir_index, mid = dir_index, right = dir_index + 1;
            for (int i = dir_index + 1; i < dir_size; ++i) {
                if (likely(dir[i] != tmp_seg)) {
                    right = i;
                    break;
                } else if (unlikely(i == (dir_size - 1))) {
                    right = dir_size;
                    break;
                }
            }
            for (int i = dir_index - 1; i >= 0; --i) {
                if (likely(dir[i] != tmp_seg)) {
                    left = i + 1;
                    break;
                } else if (unlikely(i == 0)) {
                    left = 0;
                    break;
                }
            }
            mid = (left + right) / 2;
            for (int i = left; i < mid; ++i) {
                dir[i] = new_seg1;
            }
            for (int i = mid; i < right; ++i) {
                dir[i] = new_seg2;
            }
            //migrate previous data
            for (int i = 0; i < tmp_seg->max_num; ++i) {
                for (int j = 0; j < tmp_seg->bucket[i].cnt; ++j) {
                    uint64_t tmp_key = tmp_seg->bucket[i].kv[j].key;
                    uint64_t tmp_value = tmp_seg->bucket[i].kv[j].value;
                    dir_index = GET_SEG_NUM(tmp_key, key_len, global_depth);
                    cceh_segment *dst_seg = dir[dir_index];
//                    seg_index = GET_BUCKET_NUM(tmp_key, dst_seg->bucket_mask_len);
                    seg_index = i;
                    cceh_bucket *dst_bucket = &(dst_seg->bucket[seg_index]);
                    dst_bucket->kv[dst_bucket->cnt].key = tmp_key;
                    dst_bucket->kv[dst_bucket->cnt].value = tmp_value;
                    dst_bucket->cnt++;
                }
            }
            //todo delete previous bucket
            put(key, value);
            return;
        } else {
            //condition: tmp_seg->depth == global_depth
            global_depth += 1;
            dir_size *= 2;
            //set dir
            cceh_segment **new_dir = static_cast<cceh_segment **>(fast_alloc(sizeof(cceh_segment *) * dir_size));
            for (int i = 0; i < dir_size; ++i) {
                new_dir[i] = dir[i / 2];
            }
            cceh_segment *new_seg1 = new_cceh_segment(global_depth);
            cceh_segment *new_seg2 = new_cceh_segment(global_depth);
            new_dir[dir_index * 2] = new_seg1;
            new_dir[dir_index * 2 + 1] = new_seg2;
            //migrate previous data
            for (int i = 0; i < tmp_seg->max_num; ++i) {
                for (int j = 0; j < tmp_seg->bucket[i].cnt; ++j) {
                    uint64_t tmp_key = tmp_seg->bucket[i].kv[j].key;
                    uint64_t tmp_value = tmp_seg->bucket[i].kv[j].value;
                    dir_index = GET_SEG_NUM(tmp_key, key_len, global_depth);
                    cceh_segment *dst_seg = new_dir[dir_index];
//                    seg_index = GET_BUCKET_NUM(tmp_key, dst_seg->bucket_mask_len);
                    seg_index = i;
                    cceh_bucket *dst_bucket = &(dst_seg->bucket[seg_index]);
                    dst_bucket->kv[dst_bucket->cnt].key = tmp_key;
                    dst_bucket->kv[dst_bucket->cnt].value = tmp_value;
                    dst_bucket->cnt++;
                }
            }
            //todo delete previous dir
            dir = new_dir;
            put(key, value);
            return;
        }
    } else {
        if (unlikely(tmp_bucket->kv[bucket_index].key == key)) {
            //key exists
            tmp_bucket->kv[bucket_index].value = value;
        } else {
            // there is a place to insert
            tmp_bucket->kv[bucket_index].value = value;
            mfence();
            tmp_bucket->kv[bucket_index].key = key;
//            clflush((char *) tmp_bucket->kv[bucket_index].value, 8);

            tmp_bucket->cnt++;//not needed
        }
    }
}

Value_t cacheline_concious_extendible_hash::get(Key_t key) {
    uint64_t dir_index = GET_SEG_NUM(key, key_len, global_depth);
    cceh_segment *tmp_seg = dir[dir_index];
    uint64_t seg_index = GET_BUCKET_NUM(key, tmp_seg->bucket_mask_len);
    cceh_bucket *tmp_bucket = &(tmp_seg->bucket[seg_index]);
    return tmp_bucket->get(key);
}

cacheline_concious_extendible_hash *new_cceh(uint64_t _global_depth, uint64_t _key_len) {
    cacheline_concious_extendible_hash *_new_cceh = static_cast<cacheline_concious_extendible_hash *>(fast_alloc(
            sizeof(cacheline_concious_extendible_hash)));
    _new_cceh->init(_global_depth, _key_len);
    return _new_cceh;
}
