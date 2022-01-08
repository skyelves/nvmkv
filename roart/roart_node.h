//
// Created by 王柯 on 5/25/21.
//

#ifndef NVMKV_ROART_NODE_H
#define NVMKV_ROART_NODE_H

#include "roart_key.h"
#include <emmintrin.h>
#include <atomic>
#include <fstream>
#include <set>
#include <stdint.h>
#include <string.h>
#include <algorithm>
#include <assert.h>
#include <iostream>
#include <bitset>
#include <functional>
#include <vector>
#include <map>
#include <strings.h>

using namespace std;

#define ZENTRY

const size_t LeafArrayLength = 64;
const size_t FingerPrintShift = 48;

enum class NTypes : uint8_t {
    N4 = 1,
    N16 = 2,
    N48 = 3,
    N256 = 4,
    ROART_Leaf = 5,
    LeafArray = 6
};

size_t get_node_size(NTypes type);

class LeafArray;

class ROART_BASENODE {
public:
    NTypes type;

    ROART_BASENODE(NTypes type_) : type(type_) {}

    virtual ~ROART_BASENODE() {}
};

class ROART_Leaf : public ROART_BASENODE {
public:
    size_t key_len;
    size_t val_len;
//    uint64_t key;
// variable key
#ifdef KEY_INLINE
    char kv[0]; // append key and value
#else
    uint8_t *fkey;
    char *value;
#endif

public:
    ROART_Leaf(const ROART_KEY *k);

    ROART_Leaf(uint8_t *key_, size_t key_len_, char *value_,
         size_t val_len_); // used for update
    // use for test
    ROART_Leaf() : ROART_BASENODE(NTypes::ROART_Leaf) {}

    virtual ~ROART_Leaf() {}

    bool checkKey(const ROART_KEY *k) const;

    size_t getKeyLen() const;

    char *GetKey();

    char *GetValue();

    uint16_t getFingerPrint();

    void graphviz_debug(ofstream &f);

} __attribute__((aligned(64)));

static constexpr uint32_t maxStoredPrefixLength = 4;
struct Prefix {
    uint32_t prefixCount = 0;
    uint8_t prefix[maxStoredPrefixLength];
};
static_assert(sizeof(Prefix) == 8, "Prefix should be 64 bit long");

class N : public ROART_BASENODE {
public:
    N();

    N(NTypes type, uint32_t level, const uint8_t *prefix, uint32_t prefixLength);

    N(NTypes type, uint32_t level, const Prefix &prefi);

    N(const N &) = delete;

    N(N &&) = delete;

    virtual ~N() {}

    // 3b type 59b version 1b lock 1b obsolete
    // obsolete means this node has been deleted
    atomic<uint64_t> *type_version_lock_obsolete;

    alignas(64) atomic<Prefix> prefix;
    const uint32_t level;
    uint16_t count = 0;
    uint16_t compactCount = 0;

    uint64_t generation_version = 0;
    atomic<uint64_t> recovery_latch;

    static const uint64_t dirty_bit = ((uint64_t) 1 << 60);

    void setType(NTypes type);

    static uint64_t convertTypeToVersion(NTypes type);

    N *setDirty(N *val);

    N *clearDirty(N *val);

    bool isDirty(N *val);

    void helpFlush(atomic<N *> *n);

//    void set_generation();
//
//    uint64_t get_generation();
//
//    void check_generation();

    NTypes getType() const;

    uint32_t getLevel() const;

    static uint32_t getCount(N *node);

    void setCount(uint16_t count_, uint16_t compactCount_);

    bool isLocked(uint64_t version) const;

    void writeLockOrRestart(bool &needRestart);

    void lockVersionOrRestart(uint64_t &version, bool &needRestart);

    void writeUnlock();

    uint64_t getVersion() const;

    /**
     * returns true if node hasn't been changed in between
     */
    bool checkOrRestart(uint64_t startRead) const;

    bool readVersionOrRestart(uint64_t startRead) const;

    static bool isObsolete(uint64_t version);

    /**
     * can only be called when node is locked
     */
    void writeUnlockObsolete() { type_version_lock_obsolete->fetch_add(0b11); }

    static N *getChild(const uint8_t k, N *node);

    static void insertAndUnlock(N *node, N *parentNode, uint8_t keyParent,
                         uint8_t key, N *val, bool &needRestart);

    static void change(N *node, uint8_t key, N *val);

    static void removeAndUnlock(N *node, uint8_t key, N *parentNode,
                         uint8_t keyParent, bool &needRestart);

    Prefix getPrefi() const;

    void setPrefix(const uint8_t *prefix, uint32_t length, bool flush);

    void addPrefixBefore(N *node, uint8_t key);

    static ROART_Leaf *getLeaf(const N *n);

    static bool isLeaf(const N *n);

    static N *setLeaf(const ROART_Leaf *k);

    static LeafArray *getLeafArray(const N *n);

    static bool isLeafArray(const N *n);

    static N *setLeafArray(const LeafArray *la);

    static N *getAnyChild(N *n);

    static ROART_Leaf *getAnyChildTid(const N *n);

    static void deleteChildren(N *node);

    static void deleteNode(N *node);

    static tuple<N *, uint8_t> getSecondChild(N *node, const uint8_t k);

    template<typename curN, typename biggerN>
    static void tryInsertOrGrowAndUnlock(curN *n, N *parentNode,
                                         uint8_t keyParent, uint8_t key, N *val,
                                         NTypes type, bool &needRestart);

    template<typename curN>
    static void compactAndInsertAndUnlock(curN *n, N *parentNode,
                                          uint8_t keyParent, uint8_t key,
                                          N *val, NTypes type,
                                          bool &needRestart);

    template<typename curN, typename smallerN>
    static void removeAndShrink(curN *n, N *parentNode, uint8_t keyParent,
                                uint8_t key, NTypes type, bool &needRestart);

    static void getChildren(N *node, uint8_t start, uint8_t end,
                            tuple<uint8_t, N *> children[],
                            uint32_t &childrenCount);

    static void rebuild_node(N *node,
                             vector<pair<uint64_t, size_t>> &rs,
                             uint64_t start_addr, uint64_t end_addr,
                             int thread_id);

    static void graphviz_debug(ofstream &f, N *node);

    // do insert without checking anything
    static void unchecked_insert(N *node, uint8_t roart_key_byte, N *child, bool flush);

    static bool key_keylen_lt(char *a, const int alen, char *b, const int blen,
                              const int compare_level);

    static bool leaf_lt(ROART_Leaf *a, ROART_Leaf *b, int compare_level);

    static bool leaf_key_lt(ROART_Leaf *a, const ROART_KEY *b, const int compare_level);

    static bool key_leaf_lt(const ROART_KEY *a, ROART_Leaf *b, const int compare_level);

    static bool key_key_lt(const ROART_KEY *a, const ROART_KEY *b);

    static const int ZentryKeyShift = 48;

    static uint8_t getZentryKey(uintptr_t zentry);

    static N *getZentryPtr(uintptr_t zentry);

    static pair<uint8_t, N *> getZentryKeyPtr(uintptr_t zentry);

    static uintptr_t makeZentry(uint8_t key, N *node);

} __attribute__((aligned(64)));


class N4 : public N {
public:
#ifdef ZENTRY
    atomic<uintptr_t> zens[4];
#else
    atomic<uint8_t> keys[4];
    atomic<N *> children[4];
#endif
public:
    N4(uint32_t level, const uint8_t *prefix, uint32_t prefixLength);

    N4(uint32_t level, const Prefix &prefi);

    virtual ~N4() {}

    bool insert(uint8_t key, N *n, bool flush);

    template<class NODE>
    void copyTo(NODE *n) const;

    void change(uint8_t key, N *val);

    N *getChild(const uint8_t k);

    bool remove(uint8_t k, bool force, bool flush);

    N *getAnyChild() const;

    tuple<N *, uint8_t> getSecondChild(const uint8_t key) const;

    void deleteChildren();

    void getChildren(uint8_t start, uint8_t end,
                     tuple<uint8_t, N *> children[],
                     uint32_t &childrenCount);

    uint32_t getCount() const;

    void graphviz_debug(ofstream &f);
} __attribute__((aligned(64)));


class N16 : public N {
public:
#ifdef ZENTRY
    atomic<uintptr_t> zens[16];
#else
    atomic<uint8_t> keys[16];
    atomic<N *> children[16];
#endif

    static uint8_t flipSign(uint8_t keyByte);

    static unsigned ctz(uint16_t x);

    int getChildPos(const uint8_t k);

    N16(uint32_t level, const uint8_t *prefix, uint32_t prefixLength);

    N16(uint32_t level, const Prefix &prefi);

    virtual ~N16() {}

    bool insert(uint8_t key, N *n, bool flush);

    template<class NODE>
    void copyTo(NODE *n) const;

    void change(uint8_t key, N *val);

    N *getChild(const uint8_t k);

    bool remove(uint8_t k, bool force, bool flush);

    N *getAnyChild() const;

    void deleteChildren();

    void getChildren(uint8_t start, uint8_t end,
                     tuple<uint8_t, N *> children[],
                     uint32_t &childrenCount);

    uint32_t getCount() const;

    void graphviz_debug(ofstream &f);
} __attribute__((aligned(64)));

class N48 : public N {
public:
    atomic<uint8_t> childIndex[256];
#ifdef ZENTRY
    atomic<uintptr_t> zens[48];
#else
    atomic<N *> children[48];
#endif
    static const uint8_t emptyMarker = 48;

    N48(uint32_t level, const uint8_t *prefix, uint32_t prefixLength);

    N48(uint32_t level, const Prefix &prefi);

    virtual ~N48() {}

    bool insert(uint8_t key, N *n, bool flush);

    template<class NODE>
    void copyTo(NODE *n) const;

    void change(uint8_t key, N *val);

    N *getChild(const uint8_t k);

    bool remove(uint8_t k, bool force, bool flush);

    N *getAnyChild() const;

    void deleteChildren();

    void getChildren(uint8_t start, uint8_t end,
                     tuple<uint8_t, N *> children[],
                     uint32_t &childrenCount);

    uint32_t getCount() const;

    void graphviz_debug(ofstream &f);
} __attribute__((aligned(64)));

class N256 : public N {
public:
    atomic<N *> children[256];

    N256(uint32_t level, const uint8_t *prefix, uint32_t prefixLength);

    N256(uint32_t level, const Prefix &prefi);

    virtual ~N256() {}

    bool insert(uint8_t key, N *val, bool flush);

    template<class NODE>
    void copyTo(NODE *n) const;

    void change(uint8_t key, N *n);

    N *getChild(const uint8_t k);

    bool remove(uint8_t k, bool force, bool flush);

    N *getAnyChild() const;

    void deleteChildren();

    void getChildren(uint8_t start, uint8_t end,
                     tuple<uint8_t, N *> children[],
                     uint32_t &childrenCount);

    uint32_t getCount() const;

    void graphviz_debug(ofstream &f);
} __attribute__((aligned(64)));

class LeafArray : public N {
public:
    std::atomic<uint64_t> leaf[LeafArrayLength];
    std::atomic<std::bitset<LeafArrayLength>>
            bitmap; // 0 means used slot; 1 means empty slot

public:
    LeafArray(uint32_t level = -1) : N(NTypes::LeafArray, level, {}, 0) {
        bitmap.store(std::bitset<LeafArrayLength>{}.reset());
        memset(leaf, 0, sizeof(leaf));
    }

    virtual ~LeafArray() {}

    size_t getRightmostSetBit() const;

    void setBit(size_t bit_pos, bool to = true);

    uint16_t getFingerPrint(size_t pos) const;

    ROART_Leaf *getLeafAt(size_t pos) const;

    N *getAnyChild() const;

    static uintptr_t fingerPrintLeaf(uint16_t fingerPrint, ROART_Leaf *l);

    ROART_Leaf *lookup(const ROART_KEY *k) const;

    bool update(const ROART_KEY *k, ROART_Leaf *l);

    bool insert(ROART_Leaf *l, bool flush);

    bool remove(const ROART_KEY *k);

    void reload();

    uint32_t getCount() const;

    bool isFull() const;

    void splitAndUnlock(N *parentNode, uint8_t parentROART_KEY, bool &need_restart);

    std::vector<ROART_Leaf *> getSortedLeaf(const ROART_KEY *start, const ROART_KEY *end,
                                            int start_level, bool compare_start,
                                            bool compare_end);

    void graphviz_debug(std::ofstream &f);

} __attribute__((aligned(64)));

#endif //NVMKV_ROART_NODE_H
