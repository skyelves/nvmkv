//
// Created by 王柯 on 2021-03-16.
//

#ifndef NVMKV_EXTENDIBLE_HASH_H
#define NVMKV_EXTENDIBLE_HASH_H

#include <math.h>
#include <cstdint>

#define likely(x)   (__builtin_expect(!!(x), 1))
#define unlikely(x) (__builtin_expect(!!(x), 0))

#define BUCKET_SIZE 16

class key_pointer {
public:
    uint64_t key;
    uint64_t value;
};

class bucket {
public:
    int depth=0;
    int cnt = 0;
    key_pointer counter[BUCKET_SIZE];
    bucket();
    bucket(int _depth);
    void set_depth(int _depth);

    int get(uint64_t key);
    int find_place(uint64_t key);
};

class extendible_hash {
private:
    uint64_t global_depth = 0;
    uint64_t dir_size = 1;
    bucket **dir = NULL;
public:
    extendible_hash();

    extendible_hash(uint32_t _global_depth);

    void put(uint64_t key, uint64_t value);

    int get(uint64_t key);
};


#endif //NVMKV_EXTENDIBLE_HASH_H
