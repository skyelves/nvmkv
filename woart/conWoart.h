//
// Created by 杨冠群 on 2021-08-01.
//

#ifndef NVMKV_CONWOwoart_H
#define NVMKV_CONWOwoart_H



#include <shared_mutex>
#include <stdint.h>
#include <stdbool.h>
#include <vector>
#include "../fastalloc/fastalloc.h"

#ifdef __linux__
#include <byteswap.h>
#endif

#define NODE4        1
#define NODE16        2
#define NODE48        3
#define NODE256        4

#define WOART_BITS_PER_LONG        64
#define CACHE_LINE_SIZE    64

/* If you want to change the number of entries,
 * change the values of WOART_NODE_BITS & WOART_MAX_DEPTH */
#define WOART_NODE_BITS            8
#define WOART_MAX_DEPTH            7
#define WOART_NUM_NODE_ENTRIES    (0x1UL << WOART_NODE_BITS)
#define WOART_LOW_BIT_MASK        ((0x1UL << WOART_NODE_BITS) - 1)

#define WOART_MAX_PREFIX_LEN        6
#define WOART_MAX_HEIGHT            (WOART_MAX_DEPTH + 1)

#if defined(__GNUC__) && !defined(__clang__)
# if __STDC_VERSION__ >= 199901L && 402 == (__GNUC__ * 100 + __GNUC_MINOR__)
/*
 * GCC 4.2.2's C99 inline keyword support is pretty broken; avoid. Introduced in
 * GCC 4.2.something, fixed in 4.3.0. So checking for specific major.minor of
 * 4.2 is fine.
 */
#  define BROKEN_GCC_C99_INLINE
# endif
#endif


typedef int(*conwoart_callback)(void *data, const unsigned char *key, uint32_t key_len, void *value);


struct conwoart_key_value {
    uint64_t key;
    uint64_t value;
};

/**
 * path compression
 * pwoartial_len: Optimistic
 * pwoartial: Pessimistic
 */
typedef struct {
    unsigned char depth;
    unsigned char pwoartial_len;
    unsigned char pwoartial[WOART_MAX_PREFIX_LEN];
} conpath_comp;

/**
 * This struct is included as pwoart
 * of all the various node sizes
 */
typedef struct {
    uint8_t type;
    conpath_comp path;
    shared_mutex *mtx;
} conwoart_node;

typedef struct {
    unsigned char key;
    char i_ptr;
} conslot_array;

/**
 * Small node with only 4 children, but
 * 8byte slot array field.
 */
typedef struct {
    conwoart_node n;
    conslot_array slot[4];
    shared_mutex *mtx;
    conwoart_node *children[4];

} conwoart_node4;

/**
 * Node with 16 keys and 16 children, and
 * a 8byte bitmap field
 */
typedef struct {
    conwoart_node n;
    unsigned long bitmap;
    unsigned char keys[16];
    shared_mutex *mtx;
    conwoart_node *children[16];

} conwoart_node16;

/**
 * Node with 48 children and a full 256 byte field,
 */
typedef struct {
    conwoart_node n;
    unsigned char keys[256];
    shared_mutex *mtx;
    conwoart_node *children[48];
} conwoart_node48;

/**
 * Full node with 256 children
 */
typedef struct {
    conwoart_node n;
    shared_mutex *mtx;
    conwoart_node *children[256];
} conwoart_node256;

/**
 * Represents a leaf. These are
 * of arbitrary size, as they include the key.
 */
typedef struct {
    void *value;
    uint32_t key_len;
    unsigned long key;
} conwoart_leaf;

/**
 * Main struct, points to root.
 */
typedef struct {
    conwoart_node *root;
    uint64_t size;
    shared_mutex *mtx;
} conwoart_tree;

/*
 * For range lookup in NODE16
 */
typedef struct {
    unsigned char key;
    conwoart_node *child;
} conkey_pos;

/**
 * Initializes an conwoart tree
 * @return 0 on success.
 */
int conwoart_tree_init(conwoart_tree *t);

conwoart_tree *new_conwoart_tree();

/**
 * DEPRECATED
 * Initializes an conwoart tree
 * @return 0 on success.
 */
#define init_conwoart_tree(...) conwoart_tree_init(__VA_ARGS__)

/**
 * Inserts a new value into the conwoart tree
 * @arg t The tree
 * @arg key The key
 * @arg key_len The length of the key
 * @arg value Opaque value.
 * @return NULL if the item was newly inserted, otherwise
 * the old value pointer is returned.
 */
void *conwoart_put(conwoart_tree *t, const unsigned long key, int key_len, void *value, int value_len = 8);

/**
 * Searches for a value in the conwoart tree
 * @arg t The tree
 * @arg key The key
 * @arg key_len The length of the key
 * @return NULL if the item was not found, otherwise
 * the value pointer is returned.
 */
void *conwoart_get(const conwoart_tree *t, const unsigned long key, int key_len);

void conwoart_all_subtree_kv(conwoart_node *n, vector<conwoart_key_value> &res);

void conwoart_node_scan(conwoart_node *n, uint64_t left, uint64_t right, uint64_t depth, vector<conwoart_key_value> &res,
                     int key_len = 8);


vector<conwoart_key_value> conwoart_scan(const conwoart_tree *t, uint64_t left, uint64_t right, int key_len = 8);


#endif //NVMKV_WOwoart_H
