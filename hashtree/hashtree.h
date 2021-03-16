//
// Created by 王柯 on 2021-03-04.
//

#ifndef NVMKV_HASHTREE_H
#define NVMKV_HASHTREE_H

#include "hashtree_node.h"
#include "bobhash32.h"
#include "../murmur/murmur.h"

class hashtree {
private:
//    BOBHash32 *run;
    murmur *hash;
    hashtree_node *root;
public:
    hashtree();

    ~hashtree();

    bool put(uint64_t k, uint64_t v);

    int get(int k);
};


#endif //NVMKV_HASHTREE_H
