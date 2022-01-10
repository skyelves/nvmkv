//
// Created by menghe on 1/9/22.
//
#ifndef NVMKV_VARLENGTHLBTREE_H
#define NVMKV_VARLENGTHLBTREE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <thread>
#include <atomic>
#include <vector>
#include <map>
#include <immintrin.h>
#include "nodeprof.h"
#include "../fastalloc/fastalloc.h"

#ifndef KB
#define KB      (1024)
#endif

#define NONLEAF_LINE_NUM        4    // 256B
#define LEAF_LINE_NUM           4    // 256B
#define CACHE_LINE_SIZE    64

#ifndef PREFETCH_NUM_AHEAD
#define PREFETCH_NUM_AHEAD    3
#endif

#define NON_LEAF_KEY_NUM    (NONLEAF_SIZE/(KEY_SIZE+POINTER_SIZE)-1)

#define NONLEAF_SIZE    (CACHE_LINE_SIZE * NONLEAF_LINE_NUM)
#define LEAF_SIZE       (CACHE_LINE_SIZE * LEAF_LINE_NUM)

#define LEAF_KEY_NUM        (14)

typedef unsigned char * _key_type;
#define KEY_SIZE             8   /* size of a key in tree node */
#define POINTER_SIZE         8   /* size of a pointer/value in node */

#define bitScan(x)  __builtin_ffs(x)
#define countBit(x) __builtin_popcount(x)
#define ceiling(x, y)  (((x) + (y) - 1) / (y))
//#define max(x, y) ((x)<=(y) ? (y) : (x))
#define CAS(_p, _u, _v)  (__atomic_compare_exchange_n (_p, _u, _v, false, __ATOMIC_ACQUIRE, __ATOMIC_ACQUIRE))

static inline unsigned char hashcode1B(_key_type x) {
//    x ^= x >> 32;
//    x ^= x >> 16;
//    x ^= x >> 8;
//    return (unsigned char) (x & 0x0ffULL);
    return x[0];
}


class _Pointer8B {
public:
    unsigned long long value;  /* 8B to contain a pointer */

public:
    _Pointer8B() {}

    _Pointer8B(const void *ptr) { value = (unsigned long long) ptr; }

    _Pointer8B(const _Pointer8B &p) { value = p.value; }

    _Pointer8B &operator=(const void *ptr) {
        value = (unsigned long long) ptr;
        return *this;
    }

    _Pointer8B &operator=(const _Pointer8B &p) {
        value = p.value;
        return *this;
    }

    bool operator==(const void *ptr) {
        bool result = (value == (unsigned long long) ptr);
        return result;
    }

    bool operator==(const _Pointer8B &p) {
        bool result = (value == p.value);
        return result;
    }


    operator void *() { return (void *) value; }

    operator char *() { return (char *) value; }

    operator struct varLengthbnode *() { return (struct varLengthbnode *) value; }

    operator struct varLengthbleaf *() { return (struct varLengthbleaf *) value; }

    operator unsigned long long() { return value; }

    bool isNull(void) { return (value == 0); }

    void print(void) { printf("%llx\n", value); }

};

/**
 *  An IdxEntry consists of a key and a pointer.
 */
typedef struct varLengthIdxEntry {
    _key_type k = nullptr;
    uint64_t kLength = 0;
    _Pointer8B ch;
} varLengthIdxEntry;


typedef struct varLengthbnodeMeta {
    int lock;    /* lock bit for concurrency control */
    int num;     /* number of keys */
} varLengthbnodeMeta;

class varLengthbnode {
public:
    varLengthIdxEntry ent[NON_LEAF_KEY_NUM + 1];
public:
    _key_type &k(int idx) { return ent[idx].k; }

    _Pointer8B &ch(int idx) { return ent[idx].ch; }

    uint64_t &kLength(int idx) {return ent[idx].kLength; }

    int &lock(void) { return ((varLengthbnodeMeta *) &(ent[0].k))->lock; }

    char *chEndAddr(int idx) {
        return (char *) &(ent[idx].ch) + sizeof(_Pointer8B) - 1;
    }

    int &num(void) { return ((varLengthbnodeMeta *) &(ent[0].k))->num; }

}; // bnode

typedef union varLengthbleafMeta {
    unsigned long long word8B[2];
    struct {
        uint16_t bitmap: 14;
        uint16_t lock: 1;
        uint16_t alt: 1;
        unsigned char fgpt[LEAF_KEY_NUM]; /* fingerprints */
    } v;
} varLengthbleafMeta;


class varLengthbleaf {
public:
    uint16_t bitmap: 14;
    uint16_t lock: 1;
    uint16_t alt: 1; // 2 bytes
    unsigned char fgpt[LEAF_KEY_NUM]; /* fingerprints */ // 16 bytes
    varLengthIdxEntry ent[LEAF_KEY_NUM]; // 352 bytes
    varLengthbleaf *next[2]; // 368 bytes

public:
    _key_type &k(int idx) { return ent[idx].k; }

    _Pointer8B &ch(int idx) { return ent[idx].ch; }

    uint64_t &kLength(int idx) {return ent[idx].kLength; }

    int num() { return countBit(bitmap); }

    varLengthbleaf *nextSibling() { return next[alt]; }

    bool isFull(void) { return (bitmap == 0x3fff); }

    void setBothWords(varLengthbleafMeta *m) {
        varLengthbleafMeta *my_meta = (varLengthbleafMeta *) this;
        my_meta->word8B[1] = m->word8B[1];
        my_meta->word8B[0] = m->word8B[0];
    }

    void setWord0(varLengthbleafMeta *m) {
        varLengthbleafMeta *my_meta = (varLengthbleafMeta *) this;
        my_meta->word8B[0] = m->word8B[0];
    }

}; // bleaf

class varLengthtreeMeta {
public:
    int root_level; // leaf: level 0, parent of leaf: level 1
    _Pointer8B tree_root;
    varLengthbleaf **first_leaf; // on NVM

public:
    varLengthtreeMeta(void *nvm_address, bool recover = false) {
        root_level = 0;
        tree_root = NULL;
        first_leaf = (varLengthbleaf **) nvm_address;

        if (!recover) setFirstLeaf(NULL);
    }

    void init(void *nvm_address) {
        root_level = 0;
        tree_root = NULL;
        first_leaf = (varLengthbleaf **) nvm_address;
    }

    void setFirstLeaf(varLengthbleaf *leaf);
};


class varLengthLbtree {

public:
    varLengthtreeMeta *tree_meta;

public:

    uint64_t memory_usage = 0;

    class kv{
    public:
        _key_type k;
        uint64_t v;
    };

    int bulkload(int keynum, _key_type input, float bfill = 1.0);

    int bulkloadSubtree(_key_type input, int start_key, int num_key, float bfill, int target_level, _Pointer8B pfirst[], int n_nodes[]);

    void init();

    void *lookup(_key_type key, uint64_t keyLength, int* pos = 0);

    void *get_recptr(void *p, int pos) {
        return ((varLengthbleaf *) p)->ch(pos);
    }

    // insert (key, ptr)
    void insert(_key_type key, uint64_t keyLength, void *_ptr);

    // delete key
    void del(_key_type key);

    int level() { return tree_meta->root_level; }

    vector<kv> rangeQuery(_key_type key , _key_type end);

    uint64_t memory_profile();

private:

    void qsortBleaf(varLengthbleaf *p, int start, int end, int pos[]);
};

varLengthLbtree *new_varLengthlbtree();


#endif //NVMKV_VARLENGTHLBTREE_H
