//
// Created by 王柯 on 2021-03-04.
//

#ifndef NVMKV_HASHTREE_NODE_H
#define NVMKV_HASHTREE_NODE_H

#include <cstdint>
#include <sys/time.h>
#include <vector>
#include <map>
#include <math.h>
#include <cstdint>
#include "../fastalloc/fastalloc.h"

#define likely(x)   (__builtin_expect(!!(x), 1))
#define unlikely(x) (__builtin_expect(!!(x), 0))

#define HT_BUCKET_SIZE 8
#define HT_BUCKET_MASK_LEN 8
#define HT_MAX_BUCKET_NUM (1<<HT_BUCKET_MASK_LEN)

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
class ht_key_value {
public:
    bool type = 1;
    uint64_t key = 0;// indeed only need uint8 or uint16
    uint64_t value = 0;
};

ht_key_value *new_ht_key_value(uint64_t key = 0, uint64_t value = 0);

class ht_bucket {
public:
    ht_key_value counter[HT_BUCKET_SIZE];

    uint64_t get(uint64_t key);

    int find_place(uint64_t _key, uint64_t _key_len, uint64_t _depth);
};

ht_bucket *new_ht_bucket(int _depth = 0);

class ht_segment {
public:
    uint64_t depth = 0;
    ht_bucket *bucket;
//    ht_bucket bucket[HT_MAX_BUCKET_NUM];

    ht_segment();

    ~ht_segment();

    void init(uint64_t _depth);
};

ht_segment *new_ht_segment(uint64_t _depth = 0);

class hashtree_node {
public:
    bool type = 0;
    int key_len = 16;
    uint64_t global_depth = 0;
    uint64_t dir_size = 1;
    ht_segment **dir = NULL;

    hashtree_node();

    ~hashtree_node();

    void init(uint32_t _global_depth, int _key_len);

    void put(uint64_t key,
             uint64_t value);//value is limited to non-zero number, zero is used to define empty counter in bucket

    uint64_t get(uint64_t key);
};

hashtree_node *new_hashtree_node(int _depth = 4, int _key_len = 8);


#endif //NVMKV_HASHTREE_NODE_H
