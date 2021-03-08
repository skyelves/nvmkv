//
// Created by 王柯 on 2021-03-04.
//

#ifndef NVMKV_HASHTREE_NODE_H
#define NVMKV_HASHTREE_NODE_H

#include <cstdint>
#include "bobhash32.h"

class hashtree_node {
public:
    BOBHash32 *hash;
    int size = 1024;

    hashtree_node();

    hashtree_node(int _size);

    ~hashtree_node();

    class kv {
    public:
        uint64_t k = 0;
        uint64_t v = 0;
    };

//    class bucket {
//    public:
//        kv _kv[16];
//    };

    kv *array;
//    bucket *array;
};

hashtree_node *new_node(int _size = 1024);


#endif //NVMKV_HASHTREE_NODE_H
