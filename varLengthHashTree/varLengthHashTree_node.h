//
// Created by 杨冠群 on 2021-07-08.
//

#ifndef NVMKV_VARLENGTH_HASHTREE_NODE_H
#define NVMKV_VARLENGTH_HASHTREE_NODE_H

#include <cstdint>
#include <sys/time.h>
#include <vector>
#include <map>
#include <math.h>
#include <cstdint>
#include "../fastalloc/fastalloc.h"

#define likely(x)   (__builtin_expect(!!(x), 1))
#define unlikely(x) (__builtin_expect(!!(x), 0))


#define GET_SUB_KEY(key, begin, len)  (((key)>>(64-(begin)-(len)))&(((uint64_t)1<<(len))-1))
#define GET_SEG_NUM(key, key_len, depth)  ((key>>(key_len-depth))&(((uint64_t)1<<depth)-1))
#define GET_BUCKET_NUM(key, bucket_mask_len) ((key)&(((uint64_t)1<<bucket_mask_len)-1))


#define HT_BUCKET_SIZE 4
#define HT_BUCKET_MASK_LEN 8
#define HT_MAX_BUCKET_NUM (1<<HT_BUCKET_MASK_LEN)

#define SIZE_OF_CHAR 8
#define HT_NODE_LENGTH 32
#define HT_NODE_PREFIX_MAX_BYTES 6
#define HT_NODE_PREFIX_MAX_BITS 48
//#define HT_PROFILE_TIME 1
//#define HT_PROFILE_LOAD_FACTOR 1

#ifdef HT_PROFILE_TIME
extern timeval start_time, end_time;
extern uint64_t t1, t2, t3;
#endif

#ifdef HT_PROFILE_LOAD_FACTOR
extern uint64_t ht_seg_num;
extern uint64_t ht_dir_num;
#endif

// in fact, it's better to use two kv pair in internal nodes and leaf nodes
class HashTreeKeyValue {
public:
    bool type = 1;
    unsigned char* key;// indeed only need uint8 or uint16
    unsigned int len;
    uint64_t value = 0;

    void operator =(HashTreeKeyValue a){
        this->type = a.type;
        this->key = a.key;
        this->len = a.len;
        this->value = a.value;
    };
};

HashTreeKeyValue *new_vlht_key_value(unsigned char* key, unsigned int len ,uint64_t value = 0);

class BucketKeyValue{
    public:
        uint64_t subkey = 0;
        uint64_t value = 0;
};

class HashTreeBucket {
public:
   
    BucketKeyValue counter[HT_BUCKET_SIZE];

    uint64_t get(uint64_t key);

    int find_place(uint64_t _key, uint64_t _key_len, uint64_t _depth);
};

HashTreeBucket *new_vlht_bucket(int _depth = 0);

class HashTreeSegment {
public:
    uint64_t depth = 0;
    HashTreeBucket *bucket;
//    ht_bucket bucket[HT_MAX_BUCKET_NUM];

    HashTreeSegment();

    ~HashTreeSegment();

    void init(uint64_t _depth);
};

HashTreeSegment *new_vlht_segment(uint64_t _depth = 0);


class VarLengthHashTreeHeader{
    public:
        unsigned char len = 7;
        unsigned char depth;
        unsigned char array[6]; 

        void init(VarLengthHashTreeHeader* oldHeader, unsigned char length, unsigned char depth);

        int computePrefix(unsigned char* key, int len, unsigned int pos);

        void assign(unsigned char* key, unsigned char assignedLength = HT_NODE_PREFIX_MAX_BYTES);

};

class VarLengthHashTreeNode {
public:
    bool type = 0;
    int key_len = 16;
    VarLengthHashTreeHeader header;
    uint64_t global_depth = 0;
    uint64_t dir_size = 1;
    HashTreeKeyValue* treeNodeValues;
    // used to represent the elements in the treenode prefix, but not in CCEH
    HashTreeSegment **dir = NULL;
    
    VarLengthHashTreeNode();

    ~VarLengthHashTreeNode();

    void init( int _key_len = HT_NODE_LENGTH, unsigned char headerDepth = 0);

    void put(uint64_t subKey, uint64_t value);//value is limited to non-zero number, zero is used to define empty counter in bucket

    void node_put(int pos, HashTreeKeyValue* kv);
    
    uint64_t get(uint64_t key);

};



VarLengthHashTreeNode *new_varlengthhashtree_node( int _key_len, unsigned char headerDepth = 1);


#endif //NVMKV_VARLENGTH_HASHTREE_NODE_H
