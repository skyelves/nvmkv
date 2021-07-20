//
// Created by 杨冠群 on 2021-07-08.
//

#ifndef NVMKV_VARLENGTH_HASHTREE_H
#define NVMKV_VARLENGTH_HASHTREE_H

#include "varLengthHashTree_node.h"


class VarLengthHashTree {
private:
    int init_depth = 0; //represent extendible hash initial global depth
    VarLengthHashTreeNode *root = NULL;
//    extendible_hash *root = NULL;
public:

    VarLengthHashTree();

    VarLengthHashTree(int _span, int _init_depth);

    ~VarLengthHashTree();

    void init();

    //support variable values, for convenience, we set v to 8 byte
    void crash_consistent_put(VarLengthHashTreeNode *_node, int length, unsigned char* key, uint64_t value );
    
    void put(uint64_t k, uint64_t v);

    uint64_t get(uint64_t k);

    void all_subtree_kv(VarLengthHashTreeNode *tmp, vector<HashTreeKeyValue> &res);

    void node_scan(VarLengthHashTreeNode *tmp, uint64_t left, uint64_t right, uint64_t layer, vector<HashTreeKeyValue> &res);

    vector<HashTreeKeyValue> scan(uint64_t left, uint64_t right);

    void update(uint64_t k, uint64_t v);

    uint64_t del(uint64_t k);
};

VarLengthHashTree *new_varLengthHashtree();


#endif //NVMKV_HASHTREE_H
