//
// Created by 杨冠群 on 2021-07-08.
//

#ifndef NVMKV_VARLENGTH_HASHTREE_H
#define NVMKV_VARLENGTH_HASHTREE_H

#include "varLengthHashTree_node.h"

//#define ERT_PROFILE

class VarLengthHashTree {

public:
#ifdef VLHT_PROFILE
    int bucket_cnt[1024] = {0};
    int decompression_cnt = 0;
#endif
    int init_depth = 0; //represent extendible hash initial global depth
    VarLengthHashTreeNode *root = NULL;
    int64_t lock_meta = 0;

    VarLengthHashTree();

    VarLengthHashTree(int _span, int _init_depth);

    ~VarLengthHashTree();

    void init(int prefixLen);

    //support variable values, for convenience, we set v to 8 byte
    void crash_consistent_put(VarLengthHashTreeNode *_node, int length, unsigned char* key, uint64_t value, uint64_t beforeAddress, int pos = 0);
    
    void crash_consistent_put(VarLengthHashTreeNode *_node, int length, unsigned char* key, uint64_t value, int pos = 0);

    void crash_consistent_put_without_lock(VarLengthHashTreeNode *_node, int length, unsigned char* key, uint64_t value, uint64_t beforeAddress, int pos = 0);

    uint64_t get(int length, unsigned char* key);

    void all_subtree_kv(VarLengthHashTreeNode *tmp, vector<HashTreeKeyValue> &res);

    vector<HashTreeKeyValue> scan(uint64_t left, uint64_t right);

    void update(uint64_t k, uint64_t v);

    uint64_t del(uint64_t k);

    bool read_lock();

    bool write_lock();

    void free_read_lock();

    void free_write_lock();
};

VarLengthHashTree *new_varLengthHashtree();

class Length64HashTree {
private:
    int init_depth = 0; //represent extendible hash initial global depth
    Length64HashTreeNode *root = NULL;
//    extendible_hash *root = NULL;
public:
//    double memory_header = 0;
//    double memory_seg = 0;
//    double memory_kv = 0;
#ifdef ERT_PROFILE
    int scan_node_num = 0;
    int scan_cnt = 0;
#endif

    Length64HashTree();

    Length64HashTree(int _span, int _init_depth);

    ~Length64HashTree();

    void init();

    //support variable values, for convenience, we set v to 8 byte
    void crash_consistent_put(Length64HashTreeNode *_node, uint64_t key, uint64_t value, int len = 0);
    
    uint64_t get(uint64_t key);

    void scan(uint64_t left, uint64_t right);

    void node_scan(Length64HashTreeNode *tmp, uint64_t left, uint64_t right, vector<Length64HashTreeKeyValue> &res, int pos=0, uint64_t prefix = 0);

    void getAllNodes(Length64HashTreeNode *tmp, vector<Length64HashTreeKeyValue> &res, int pos = 0, uint64_t prefix = 0);

    uint64_t memory_profile(Length64HashTreeNode *tmp, int pos = 0);
};

Length64HashTree *new_length64HashTree();
void recovery(VarLengthHashTreeNode* root, int depth);
#endif
