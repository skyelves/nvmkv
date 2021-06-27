// created by 杨冠群 2021-6-18

#ifndef NVMKV_CONCURRENCY_HASHTREE_H
#define NVMKV_CONCURRENCY_HASHTREE_H

#include "concurrency_hashtree_node.h"

class concurrencyhashtree {
private:
    int64_t span = 8; //equals to the key_len in the extendible hash
    int init_depth = 4; //represent extendible hash initial global depth
//    extendible_hash *root = NULL;
public:
    uint64_t span_test[4] = {32, 16, 8, 8};
    int node_cnt = 0;
    uint64_t get_access = 0;
    concurrency_hashtree_node *root = NULL;

    concurrencyhashtree();

    concurrencyhashtree(int _span, int _init_depth);

    ~concurrencyhashtree();

    void init(int _span = 8, int _init_depth = 4);

    //support variable values, for convenience, we set v to 8 byte
    void crash_consistent_put(concurrency_hashtree_node *_node, uint64_t k, uint64_t v, uint64_t layer);

    void put(uint64_t k, uint64_t v);

    uint64_t get(uint64_t k);

    void all_subtree_kv(concurrency_hashtree_node *tmp, vector<concurrency_ht_key_value> &res);

    void node_scan(concurrency_hashtree_node *tmp, uint64_t left, uint64_t right, uint64_t layer, vector<concurrency_ht_key_value> &res);

    vector<concurrency_ht_key_value> scan(uint64_t left, uint64_t right);

    void update(uint64_t k, uint64_t v);

    uint64_t del(uint64_t k);
};

concurrencyhashtree *new_concurrency_hashtree(int _span = 8, int _init_depth = 4);


#endif 
