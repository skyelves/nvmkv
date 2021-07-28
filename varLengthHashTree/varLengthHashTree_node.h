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


#define GET_SUB_KEY(key, begin, len)  (((key)>>(64-(begin)-(len)))&((0x1ull<<(len))-1))
#define GET_SEG_NUM(key, key_len, depth)  ((key>>(key_len-depth))&((0x1ull<<depth)-1))
#define GET_BUCKET_NUM(key, bucket_mask_len) ((key)&((0x1ull<<bucket_mask_len)-1))

#define GET_SEG_POS(currentNode,dir_index) (((uint64_t)(currentNode) + sizeof(VarLengthHashTreeNode) + dir_index*sizeof(HashTreeSegment*)))

#define GET_SUBKEY(key, start, length) ( (key>>(64 - start - length) & ((0x1ull<<length)-1)))

#define HT_INIT_GLOBAL_DEPTH 0
#define HT_BUCKET_SIZE 4
#define HT_BUCKET_MASK_LEN 8
#define HT_MAX_BUCKET_NUM (1<<HT_BUCKET_MASK_LEN)

#define SIZE_OF_CHAR 8
#define HT_NODE_LENGTH 32
#define HT_NODE_PREFIX_MAX_BYTES 6
#define HT_NODE_PREFIX_MAX_BITS 48
#define HT_KEY_LENGTH 64

#define VLHT_PROFILE_TIME
#ifdef VLHT_PROFILE_TIME
extern timeval start_time, end_time;
extern uint64_t t1, t2, t3, t4;
// t1: insertion
// t2: segment split
// t3: directory double
// t4: decompression
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
    // HashTreeBucket bucket[HT_MAX_BUCKET_NUM];

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
    unsigned char global_depth = 0;
    uint32_t dir_size = 1;
    VarLengthHashTreeHeader header;
    HashTreeKeyValue* treeNodeValues;
    // used to represent the elements in the treenode prefix, but not in CCEH

    VarLengthHashTreeNode();

    ~VarLengthHashTreeNode();

    void init(unsigned char headerDepth = 0, unsigned char global_depth = 0);

    void put(uint64_t subKey, uint64_t value, uint64_t beforeAddress);//value is limited to non-zero number, zero is used to define empty counter in bucket

    void put(uint64_t subkey, uint64_t value, HashTreeSegment* tmp_seg, HashTreeBucket* tmp_bucket, uint64_t dir_index, uint64_t seg_index, uint64_t beforeAddress);

    void node_put(int pos, HashTreeKeyValue* kv);
    
    uint64_t get(uint64_t key);

};

VarLengthHashTreeNode *new_varlengthhashtree_node( int _key_len, unsigned char headerDepth = 1, unsigned char globalDepth = HT_INIT_GLOBAL_DEPTH);

class Length64HashTreeKeyValue {
public:
    bool type = 1;
    uint64_t key = 0;// indeed only need uint8 or uint16
    uint64_t value = 0;

    void operator =(Length64HashTreeKeyValue a){
        this->type = a.type;
        this->key = a.key;
        this->value = a.value;
    };
};

Length64HashTreeKeyValue *new_l64ht_key_value(uint64_t key ,uint64_t value = 0);

class Length64BucketKeyValue{
    public:
        uint64_t subkey = 0;
        uint64_t value = 0;
};

class Length64HashTreeBucket {
public:
   
    Length64BucketKeyValue counter[HT_BUCKET_SIZE];

    uint64_t get(uint64_t key);

    int find_place(uint64_t _key, uint64_t _key_len, uint64_t _depth);
};

Length64HashTreeBucket *new_l64ht_bucket(int _depth = 0);

class Length64HashTreeSegment {
public:
    uint64_t depth = 0;
    Length64HashTreeBucket *bucket;
//    ht_bucket bucket[HT_MAX_BUCKET_NUM];

    Length64HashTreeSegment();

    ~Length64HashTreeSegment();

    void init(uint64_t _depth);
};

Length64HashTreeSegment *new_l64ht_segment(uint64_t _depth = 0);

class Length64HashTreeHeader{
    public:
        unsigned char len = 7;
        unsigned char depth;
        unsigned char array[6];

        void init(Length64HashTreeHeader* oldHeader, unsigned char length, unsigned char depth);

        int computePrefix(uint64_t key, int pos);

        void assign(uint64_t key, int startPos);

        void assign(unsigned char* key, unsigned char assignedLength = HT_NODE_PREFIX_MAX_BYTES);
};

class Length64HashTreeNode {
public:
    bool type = 0;
    unsigned char global_depth = 0;
    uint32_t dir_size = 1;
    Length64HashTreeHeader header;
    Length64HashTreeKeyValue* treeNodeValues;
    // used to represent the elements in the treenode prefix, but not in CCEH
    
    Length64HashTreeNode();

    ~Length64HashTreeNode();

    void init( unsigned char headerDepth = 0, unsigned char global_depth = 0);

    void put(uint64_t subkey, uint64_t value, uint64_t beforeAddress);

    void put(uint64_t subkey, uint64_t value, Length64HashTreeSegment* tmp_seg, Length64HashTreeBucket* tmp_bucket, uint64_t dir_index, uint64_t seg_index, uint64_t beforeAddress);

    void node_put(int pos, Length64HashTreeKeyValue* kv);
    
    uint64_t get(uint64_t subkey);

};

Length64HashTreeNode *new_length64hashtree_node(int _key_len, unsigned char headerDepth = 1, unsigned char globalDepth = HT_INIT_GLOBAL_DEPTH);


#endif //NVMKV_VARLENGTH_HASHTREE_NODE_H
