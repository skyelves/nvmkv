//
// Created by 王柯 on 2021-03-16.
//

#ifndef NVMKV_EXTENDIBLE_HASH_H
#define NVMKV_EXTENDIBLE_HASH_H

#include <math.h>
#include <cstdint>

#define likely(x)   (__builtin_expect(!!(x), 1))
#define unlikely(x) (__builtin_expect(!!(x), 0))

#define BUCKET_SIZE 4

class key_pointer {
public:
    uint64_t key = 0;
    uint64_t value = 0;
};

class bucket {
public:
    int depth = 0;
    int cnt = 0;
    key_pointer counter[BUCKET_SIZE];

    bucket();

    bucket(int _depth);

    void set_depth(int _depth);

    int64_t get(uint64_t key);

    int find_place(uint64_t key);
};

class extendible_hash {
private:
    uint64_t global_depth = 0;
    uint64_t dir_size = 1;
    int key_len = 16;
    bucket **dir = NULL;
public:
    extendible_hash();

    extendible_hash(uint32_t _global_depth);

    extendible_hash(uint32_t _global_depth, int _key_len);

    void set_key_len(int _key_len);

    void put(uint64_t key,
             uint64_t value);//value is limited to non-zero number, zero is used to define empty counter in bucket

    int64_t get(uint64_t key);
};

extendible_hash* new_extendible_hash(uint32_t _global_depth, int _key_len);


#endif //NVMKV_EXTENDIBLE_HASH_H
