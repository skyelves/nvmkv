//
// Created by 杨冠群 on 2021-06-20.
//

#ifndef NVMKV_CONCURRENCY_HASHTREE_NODE_H
#define NVMKV_CONCURRENCY_HASHTREE_NODE_H

#include <cstdint>
#include <sys/time.h>
#include <vector>
#include <map>
#include <math.h>
#include <cstdint>
#include <thread>
#include "../fastalloc/fastalloc.h"

#define likely(x)   (__builtin_expect(!!(x), 1))
#define unlikely(x) (__builtin_expect(!!(x), 0))

#define HT_BUCKET_SIZE 8
#define HT_BUCKET_MASK_LEN 8
#define HT_MAX_BUCKET_NUM (1<<HT_BUCKET_MASK_LEN)

#define cas(_p, _u, _v)  (__atomic_compare_exchange_n (_p, _u, _v, false, __ATOMIC_ACQUIRE, __ATOMIC_ACQUIRE))
//#define HT_PROFILE_TIME 1
//#define HT_PROFILE_LOAD_FACTOR 1

#ifdef HT_PROFILE_TIME
extern timeval start_time, end_time;
extern uint64_t t1, t2, t3;
#endif

#ifdef HT_PROFILE_LOAD_FACTOR
extern uint64_t ht_seg_num;
extern uint64_t ht_dir_num;
#endif

// in fact, it's better to use two kv pair in internal nodes and leaf nodes
class concurrency_ht_key_value {
public:
    bool type = 1;
    uint64_t key = 0;// indeed only need uint8 or uint16
    uint64_t value = 0;
};

concurrency_ht_key_value *new_concurrency_ht_key_value(uint64_t key = 0, uint64_t value = 0);

class concurrency_ht_bucket {
public:
    concurrency_ht_key_value counter[HT_BUCKET_SIZE];
    int64_t lock_meta;

    uint64_t get(uint64_t key);

    int find_place(uint64_t _key, uint64_t _key_len, uint64_t _depth);

    bool read_lock();

    bool write_lock();

    void free_read_lock();

    void free_write_lock();

    bool is_write_locked();

    bool is_read_locked();
};

concurrency_ht_bucket *new_concurrency_ht_bucket(int _depth = 0);

class concurrency_ht_segment {
public:
    int64_t depth = 0;
    concurrency_ht_bucket *bucket;
    int64_t lock_meta = 0;
//    ht_bucket bucket[HT_MAX_BUCKET_NUM];

    concurrency_ht_segment();

    ~concurrency_ht_segment();

    void init(uint64_t _depth);

    bool read_lock();

    bool write_lock();

    void free_read_lock();

    void free_write_lock();

    bool is_write_locked();

    bool is_read_locked();

};

concurrency_ht_segment *new_concurrency_ht_segment(uint64_t _depth = 0);

class concurrency_hashtree_node {
public:
    bool type = 0;
    uint64_t global_depth = 0;
    uint64_t dir_size = 1;
    int key_len = 16;
    concurrency_ht_segment **dir = NULL;
    int64_t lock_meta = 0;

    concurrency_hashtree_node();

    ~concurrency_hashtree_node();

    void init(uint32_t _global_depth, int _key_len);

    void put(uint64_t key,
             uint64_t value);//value is limited to non-zero number, zero is used to define empty counter in bucket

    uint64_t get(uint64_t key);

    bool read_lock();

    bool write_lock();

    void free_read_lock();

    void free_write_lock();

    bool is_write_locked();

    bool is_read_locked();
};

concurrency_hashtree_node *new_concurrency_hashtree_node(int _depth = 4, int _key_len = 8);


#endif
