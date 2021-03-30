//
// Created by 王柯 on 2021-03-04.
//

#ifndef NVMKV_HASHTREE_H
#define NVMKV_HASHTREE_H

#include "hashtree_node.h"

class hashtree {
private:
    int64_t span = 8; //equals to the key_len in the extendible hash
    int init_depth = 4; //represent extendible hash initial global depth
    hashtree_node *root = NULL;
//    extendible_hash *root = NULL;
public:
    uint64_t span_test[4] = {32, 16, 8, 8};
    int node_cnt = 0;
    uint64_t get_access = 0;

    hashtree();

    hashtree(int _span, int _init_depth);

    ~hashtree();

    void init(int _span = 8, int _init_depth = 4);

    void put(uint64_t k, uint64_t v);

    int64_t get(uint64_t k);
};

hashtree *new_hashtree(int _span = 8, int _init_depth = 4);


#endif //NVMKV_HASHTREE_H
