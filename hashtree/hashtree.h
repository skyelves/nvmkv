//
// Created by 王柯 on 2021-03-04.
//

#ifndef NVMKV_HASHTREE_H
#define NVMKV_HASHTREE_H

#include "hashtree_node.h"

class hashtree {
private:
    int64_t span = 8; //equals to the key_len in the extendible hash
    int init_depth = 0; //represent extendible hash initial global depth
    hashtree_node *root = NULL;
//    extendible_hash *root = NULL;
public:
    uint64_t span_test[4] = {32, 16, 8, 8};
    int node_cnt = 0;
    uint64_t get_access = 0;

    hashtree();

    hashtree(int _span, int _init_depth);

    ~hashtree();

    hashtree_node * getRoot(){
        return root;
    }
    int getInitDepth(){
        return this->init_depth;
    }

    int64_t getSpan(){
        return span;
    }

    void init(int _span = 8, int _init_depth = 4);

    //support variable values, for convenience, we set v to 8 byte
    void crash_consistent_put(hashtree_node *_node, uint64_t k, uint64_t v, uint64_t layer);

    void put(uint64_t k, uint64_t v);

    uint64_t get(uint64_t k);

    void all_subtree_kv(hashtree_node *tmp, vector<ht_key_value> &res);

    void node_scan(hashtree_node *tmp, uint64_t left, uint64_t right, uint64_t layer, vector<ht_key_value> &res);

    vector<ht_key_value> scan(uint64_t left, uint64_t right);

    void update(uint64_t k, uint64_t v);

    uint64_t del(uint64_t k);
};

hashtree *new_hashtree(int _span = 8, int _init_depth = 4);


void recovery(hashtree_node* head);

#endif //NVMKV_HASHTREE_H
