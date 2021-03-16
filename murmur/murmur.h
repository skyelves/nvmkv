//
// Created by 王柯 on 2021-03-16.
//

#ifndef NVMKV_MURMUR_H
#define NVMKV_MURMUR_H


#include <cstdint>

#define MAX_PRIME32 1229

class murmur {
private:
    uint32_t seed;
public:
    murmur():seed(3){}
    murmur(uint32_t _seed):seed(_seed){}
    void init(uint32_t _seed);
    uint32_t run(const void *key, int len);

};


#endif //NVMKV_MURMUR_H
