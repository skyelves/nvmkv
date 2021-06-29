//
// Created by 杨冠群 on 2021-06-26.
//

#ifndef CONCURRENCY_CCEH_H
#define CONCURRENCY_CCEH_H

#include <iostream>
#include <stdio.h>
#include <sys/time.h>
#include <cstring>
#include <cmath>
#include <vector>
#include <thread> 
#include <iostream>
#include "../fastalloc/fastalloc.h"

#define CAS(_p, _u, _v)  (__atomic_compare_exchange_n (_p, _u, _v, false, __ATOMIC_ACQUIRE, __ATOMIC_ACQUIRE))
#define CCEH_BUCKET_SIZE 8
#define CCEH_BUCKET_MASK_LEN 8
#define CCEH_MAX_BUCKET_NUM (1<<CCEH_BUCKET_MASK_LEN)

//#define CCEH_PROFILE_TIME 1
//#define CCEH_PROFILE_LOAD_FACTOR 1

#ifdef CCEH_PROFILE_TIME
extern timeval start_time, end_time;
extern uint64_t t1, t2, t3;
#endif

#ifdef CCEH_PROFILE_LOAD_FACTOR
extern uint64_t cceh_seg_num;
#endif

typedef uint64_t Key_t;
typedef uint64_t Value_t;

class concurrency_cceh_key_value {
public:
    uint64_t key = 0;
    uint64_t value = 0;
};

class concurrency_cceh_bucket {
public:
    int64_t lock_meta = 0;
    concurrency_cceh_key_value kv[CCEH_BUCKET_SIZE];

    int find_place(Key_t key, uint64_t depth);

    Value_t get(Key_t key);

    bool read_lock();

    bool write_lock();

    void free_read_lock();

    void free_write_lock();
};

class concurrency_cceh_segment {
public:
    uint64_t depth = 0;
    uint64_t cnt = 0;
    int64_t lock_meta = 0;
    concurrency_cceh_bucket *bucket;

    concurrency_cceh_segment();

    ~concurrency_cceh_segment();

    void init(uint64_t _depth);

    bool read_lock();

    bool write_lock();

    void free_read_lock();

    void free_write_lock();
};

concurrency_cceh_segment *new_concurrency_cceh_segment(uint64_t _depth = 0);

class concurrency_cceh {
public:

    class Directory{
        public:
            concurrency_cceh_segment **dir = NULL;
            uint64_t global_depth = 0;
            uint64_t dir_size = 1;
    };

    Directory * directory = NULL;
    uint64_t key_len = 64;
    int64_t lock_meta = 0;

    concurrency_cceh();

    ~concurrency_cceh();

    void init(uint64_t _global_depth, uint64_t _key_len);

    void put(Key_t key, Value_t value);

    Value_t get(Key_t key);

    bool read_lock();

    bool write_lock();

    void free_read_lock();

    void free_write_lock();
};

concurrency_cceh *new_concurrency_cceh(uint64_t _global_depth = 0, uint64_t _key_len = 64);




#endif //NVMKV_CACHELINE_CONCIOUS_EXTENDIBLE_HASH_H
