//
// Created by 王柯 on 2021-04-01.
//

#ifndef NVMKV_CACHELINE_CONCIOUS_EXTENDIBLE_HASH_H
#define NVMKV_CACHELINE_CONCIOUS_EXTENDIBLE_HASH_H

#include <iostream>
#include <stdio.h>
#include <sys/time.h>
#include <cstring>
#include <cmath>
#include <vector>
#include <pthread.h>
#include <iostream>
#include "../fastalloc/fastalloc.h"

#define CAS(_p, _u, _v)  (__atomic_compare_exchange_n (_p, _u, _v, false, __ATOMIC_ACQUIRE, __ATOMIC_ACQUIRE))
#define CCEH_BUCKET_SIZE 8
#define CCEH_BUCKET_MASK_LEN 8
#define CCEH_MAX_BUCKET_NUM (2<<CCEH_BUCKET_MASK_LEN)

#define GET_SEG_NUM(key, key_len, depth)  ((key>>(key_len-depth))&(((uint64_t)1<<depth)-1))
#define GET_BUCKET_NUM(key, bucket_mask_len) ((key)&(((uint64_t)1<<bucket_mask_len)-1))

//#define CCEH_PROFILE_TIME 1
//#define CCEH_PROF ILE_LOAD_FACTOR 1

#ifdef CCEH_PROFILE_TIME
extern timeval start_time, end_time;
extern uint64_t t1, t2, t3;
#endif

#ifdef CCEH_PROFILE_LOAD_FACTOR
extern uint64_t cceh_seg_num;
#endif

typedef uint64_t Key_t;
typedef uint64_t Value_t;

class cceh_key_value {
public:
    uint64_t key = 0;
    uint64_t value = 0;
};

class cceh_bucket {
public:
    cceh_key_value kv[CCEH_BUCKET_SIZE];

    int find_place(Key_t key, uint64_t depth);

    Value_t get(Key_t key);
};

class cceh_segment {
public:
    uint64_t depth = 0;
    uint64_t cnt = 0;
    cceh_bucket *bucket;

    cceh_segment();

    ~cceh_segment();

    void init(uint64_t _depth);
};

cceh_segment *new_cceh_segment(uint64_t _depth = 0);

class cacheline_concious_extendible_hash {
public:
    cceh_segment **dir = NULL;
    uint64_t global_depth = 0;
    uint64_t dir_size = 1;
    uint64_t key_len = 64;

    cacheline_concious_extendible_hash();

    ~cacheline_concious_extendible_hash();

    void init(uint64_t _global_depth, uint64_t _key_len);

    void put(Key_t key, Value_t value);

    Value_t get(Key_t key);
};

cacheline_concious_extendible_hash *new_cceh(uint64_t _global_depth = 0, uint64_t _key_len = 64);


#endif //NVMKV_CACHELINE_CONCIOUS_EXTENDIBLE_HASH_H
