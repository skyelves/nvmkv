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

#define GET_SEG_POS(currentNode,dir_index) (((uint64_t)(currentNode) + sizeof(VarLengthHashTreeNode) + dir_index*sizeof(HashTreeSegment*)))

#define GET_SUBKEY(key, start, length) ( (key>>(64 - start - length) & (((uint64_t)1<<length)-1)))

#define REMOVE_NODE_FLAG(key) (key & (((uint64_t)1<<56)-1) )
#define PUT_KEY_VALUE_FLAG(key) (key | ((uint64_t)1<<56))
#define GET_NODE_FLAG(key) (key>>56)

#define HT_INIT_GLOBAL_DEPTH 0
#define HT_BUCKET_SIZE 4
#define HT_BUCKET_MASK_LEN 2
#define HT_MAX_BUCKET_NUM (1<<HT_BUCKET_MASK_LEN)

#define SIZE_OF_CHAR 8
#define HT_NODE_LENGTH 32
#define HT_NODE_PREFIX_MAX_BYTES 6
#define HT_NODE_PREFIX_MAX_BITS 48
#define HT_KEY_LENGTH 64

#define VLHT_PROFILE

#ifdef VLHT_PROFILE
extern uint64_t split_cnt;
extern uint64_t double_cnt;
#endif

//#define NEW_ERT_PROFILE_TIME 1

#ifdef NEW_ERT_PROFILE_TIME
extern timeval start_time, end_time;
extern uint64_t _grow, _update, _travelsal, _decompression;
#endif

//#define ERT_SCAN_PROFILE_TIME 1

#ifdef ERT_SCAN_PROFILE_TIME
extern timeval start_time, end_time;
extern uint64_t _random, _sequential;
#endif

// in fact, it's better to use two kv pair in internal nodes and leaf nodes
class HashTreeKeyValue {
public:
    bool type = 1;
    unsigned int len;
    uint64_t value = 0;
    unsigned char* key;// indeed only need uint8 or uint16

    HashTreeKeyValue(){
        key = NULL;
        len = 0;
    }

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

        BucketKeyValue(){
            subkey = 0;
            value = 0;
        }
};

class HashTreeBucket {
public:
   
    BucketKeyValue counter[HT_BUCKET_SIZE];
//    int64_t lock_meta = 0;

    HashTreeBucket();

    uint64_t get(uint64_t key);

    int find_place(uint64_t _key, uint64_t _key_len, uint64_t _depth);

    bool read_lock();

    bool write_lock();

    void free_read_lock();

    void free_write_lock();

    bool is_write_locked();

    bool is_read_locked();
};

HashTreeBucket *new_vlht_bucket(int _depth = 0);

class HashTreeSegment {
public:
    uint64_t depth = 0;
    HashTreeBucket *bucket;
    // HashTreeBucket bucket[HT_MAX_BUCKET_NUM];
    int64_t lock_meta = 0;


    HashTreeSegment();

    ~HashTreeSegment();

    void init(uint64_t _depth);

    bool read_lock();

    bool write_lock();

    void free_read_lock();

    void free_write_lock();

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
    int64_t lock_meta = 0;
    VarLengthHashTreeHeader header;
    HashTreeKeyValue** treeNodeValues;
    // used to represent the elements in the treenode prefix, but not in CCEH

    VarLengthHashTreeNode();

    ~VarLengthHashTreeNode();

    void init(int prefixLen, unsigned char headerDepth = 0, unsigned char global_depth = 0);

    void init(VarLengthHashTreeNode* oldNode);

    void put(uint64_t subKey, uint64_t value, uint64_t beforeAddress);//value is limited to non-zero number, zero is used to define empty counter in bucket

    void put(uint64_t subkey, uint64_t value, HashTreeSegment* tmp_seg, HashTreeBucket* tmp_bucket, uint64_t dir_index, uint64_t seg_index, uint64_t beforeAddress);

    bool put_with_read_lock(uint64_t subkey, uint64_t value, HashTreeSegment* tmp_seg, HashTreeBucket* tmp_bucket, uint64_t dir_index, uint64_t seg_index, uint64_t beforeAddress);
    
    void node_put(int pos, HashTreeKeyValue* kv);
    
    uint64_t get(uint64_t key);

    bool read_lock();

    bool write_lock();

    void free_read_lock();

    void free_write_lock();

    bool is_write_locked();

    bool is_read_locked();
};

VarLengthHashTreeNode *new_varlengthhashtree_node(int prefixLen, unsigned char headerDepth = 0, unsigned char globalDepth = HT_INIT_GLOBAL_DEPTH);

class Length64HashTreeKeyValue {
public:
    uint64_t key = 0;// indeed only need uint8 or uint16
    uint64_t value = 0;

    void operator =(Length64HashTreeKeyValue a){
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

    uint64_t get(uint64_t key, bool& keyValueFlag);

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
    Length64HashTreeHeader header;
    unsigned char global_depth = 0;
    uint32_t dir_size = 1;
    Length64HashTreeKeyValue* treeNodeValues;
    // used to represent the elements in the treenode prefix, but not in CCEH
    
    Length64HashTreeNode();

    ~Length64HashTreeNode();

    void init( unsigned char headerDepth = 0, unsigned char global_depth = 0);

    void put(uint64_t subkey, uint64_t value, uint64_t beforeAddress);

    void put(uint64_t subkey, uint64_t value, Length64HashTreeSegment* tmp_seg, Length64HashTreeBucket* tmp_bucket, uint64_t dir_index, uint64_t seg_index, uint64_t beforeAddress);

    void node_put(int pos, Length64HashTreeKeyValue* kv);
    
    uint64_t get(uint64_t subkey, bool& keyValueFlag);

};

Length64HashTreeNode *new_length64hashtree_node(int _key_len, unsigned char headerDepth = 1, unsigned char globalDepth = HT_INIT_GLOBAL_DEPTH);


#endif //NVMKV_VARLENGTH_HASHTREE_NODE_H
