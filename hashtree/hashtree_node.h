//
// Created by 王柯 on 2021-03-04.
//

#ifndef NVMKV_HASHTREE_NODE_H
#define NVMKV_HASHTREE_NODE_H

#include <cstdint>

#include <math.h>
#include <cstdint>
#include "../fastalloc/fastalloc.h"

#define likely(x)   (__builtin_expect(!!(x), 1))
#define unlikely(x) (__builtin_expect(!!(x), 0))

#define BUCKET_SIZE 8

class ht_key_value {
public:
    bool type = 1;
    uint64_t key = 0;// indeed only need uint8 or uint16
    uint64_t value = 0;
};

ht_key_value *new_ht_key_value(uint64_t key = 0, uint64_t value = 0);

class ht_bucket {
public:
    int depth = 0;
    int cnt = 0;
    ht_key_value counter[BUCKET_SIZE];

    ht_bucket();

    ht_bucket(int _depth);

    void init(int _depth);

    void set_depth(int _depth);

    uint64_t get(uint64_t key);

    int find_place(uint64_t _key, uint64_t _key_len);
};

ht_bucket *new_ht_bucket(int _depth = 0);

class hashtree_node {
public:
    bool type = 0;
    uint64_t global_depth = 0;
    uint64_t dir_size = 1;
    int key_len = 16;
    ht_bucket **dir = NULL;

    hashtree_node();

    hashtree_node(int _global_depth, int _key_len);

    ~hashtree_node();

    void init(uint32_t _global_depth, int _key_len);


    void put(uint64_t key,
             uint64_t value);//value is limited to non-zero number, zero is used to define empty counter in bucket

    uint64_t get(uint64_t key);
};

hashtree_node *new_hashtree_node(int _depth = 4, int _key_len = 8);


#endif //NVMKV_HASHTREE_NODE_H
