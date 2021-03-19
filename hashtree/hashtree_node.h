//
// Created by 王柯 on 2021-03-04.
//

#ifndef NVMKV_HASHTREE_NODE_H
#define NVMKV_HASHTREE_NODE_H

#include <cstdint>
#include "extendible_hash.h"

class hashtree_node {
public:
    extendible_hash *eht;

    hashtree_node();

    hashtree_node(int _depth, int _key_len);

    ~hashtree_node();

};

hashtree_node *new_hashtree_node(int _depth = 4, int _key_len = 8);


#endif //NVMKV_HASHTREE_NODE_H
