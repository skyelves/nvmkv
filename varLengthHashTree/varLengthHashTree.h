//
// Created by 杨冠群 on 2021-07-08.
//

#ifndef NVMKV_VARLENGTH_HASHTREE_H
#define NVMKV_VARLENGTH_HASHTREE_H

#include "varLengthHashTree_node.h"

#define VLHT_PROFILE

#ifdef VLHT_PROFILE
extern uint64_t vlht_visited_node;
#endif

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
    void crash_consistent_put(VarLengthHashTreeNode *_node, int length, unsigned char* key, uint64_t value, uint64_t beforeAddress, int pos = 0);
    
    void crash_consistent_put(VarLengthHashTreeNode *_node, int length, unsigned char* key, uint64_t value, int pos = 0);

    uint64_t get(int length, unsigned char* key);

    void all_subtree_kv(VarLengthHashTreeNode *tmp, vector<HashTreeKeyValue> &res);

    void node_scan(VarLengthHashTreeNode *tmp, uint64_t left, uint64_t right, uint64_t layer, vector<HashTreeKeyValue> &res);

    vector<HashTreeKeyValue> scan(uint64_t left, uint64_t right);

    void update(uint64_t k, uint64_t v);

    uint64_t del(uint64_t k);

    double profile();
};

VarLengthHashTree *new_varLengthHashtree();

class Length64HashTree {
private:
    int init_depth = 0; //represent extendible hash initial global depth
    Length64HashTreeNode *root = NULL;
//    extendible_hash *root = NULL;
public:

    Length64HashTree();

    Length64HashTree(int _span, int _init_depth);

    ~Length64HashTree();

    void init();

    //support variable values, for convenience, we set v to 8 byte
    void crash_consistent_put(Length64HashTreeNode *_node, uint64_t key, uint64_t value, int len = 0);
    
    uint64_t get(uint64_t key);

};

Length64HashTree *new_length64HashTree();

#endif
