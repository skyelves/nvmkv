//
// Created by 杨冠群 on 2021-07-28.
//

#ifndef NVMKV_VARLENGTHWOART_H
#define NVMKV_VARLENGTHWOART_H


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

#define SIZE_OF_CHAR 8
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



typedef int(*woart_callback)(void *data, const unsigned char *key, uint32_t key_len, void *value);


struct var_length_woart_key_value {
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
} var_length_path_comp;

/**
 * This struct is included as pwoart
 * of all the various node sizes
 */
typedef struct {
    uint8_t type;
    var_length_path_comp path;
} var_length_woart_node;

typedef struct {
    unsigned char key;
    char i_ptr;
} var_length_slot_array;

/**
 * Small node with only 4 children, but
 * 8byte slot array field.
 */
typedef struct {
    var_length_woart_node n;
    var_length_slot_array slot[4];
    var_length_woart_node *children[4];
} var_length_woart_node4;

/**
 * Node with 16 keys and 16 children, and
 * a 8byte bitmap field
 */
typedef struct {
    var_length_woart_node n;
    unsigned long bitmap;
    unsigned char keys[16];
    var_length_woart_node *children[16];
} var_length_woart_node16;

/**
 * Node with 48 children and a full 256 byte field,
 */
typedef struct {
    var_length_woart_node n;
    unsigned char keys[256];
    var_length_woart_node *children[48];
} var_length_woart_node48;

/**
 * Full node with 256 children
 */
typedef struct {
    var_length_woart_node n;
    var_length_woart_node *children[256];
} var_length_woart_node256;

/**
 * Represents a leaf. These are
 * of arbitrary size, as they include the key.
 */
typedef struct {
    void *value;
    unsigned char* key;
    uint32_t key_len;
} var_length_woart_leaf;

/**
 * Main struct, points to root.
 */
typedef struct {
    var_length_woart_node *root;
    uint64_t size;
} var_length_woart_tree;

/*
 * For range lookup in NODE16
 */
typedef struct {
    unsigned char key;
    var_length_woart_node *child;
} var_length_key_pos;

/**
 * Initializes an woart tree
 * @return 0 on success.
 */
int var_length_woart_tree_init(var_length_woart_tree *t);

var_length_woart_tree *new_var_length_woart_tree();

/**
 * DEPRECATED
 * Initializes an woart tree
 * @return 0 on success.
 */
#define init_var_length_woart_tree(...) var_length_woart_tree_init(__VA_ARGS__)

/**
 * Inserts a new value into the woart tree
 * @arg t The tree
 * @arg key The key
 * @arg key_len The length of the key
 * @arg value Opaque value.
 * @return NULL if the item was newly inserted, otherwise
 * the old value pointer is returned.
 */
void *var_length_woart_put(var_length_woart_tree *t, unsigned char* key, int key_len, void *value, int value_len = 8);

/**
 * Searches for a value in the woart tree
 * @arg t The tree
 * @arg key The key
 * @arg key_len The length of the key
 * @return NULL if the item was not found, otherwise
 * the value pointer is returned.
 */
void *var_length_woart_get(const var_length_woart_tree *t, unsigned char* key, int key_len);

void var_length_woart_all_subtree_kv(var_length_woart_node *n, vector<var_length_woart_key_value> &res);

void var_length_woart_node_scan(var_length_woart_node *n, uint64_t left, uint64_t right, uint64_t depth, vector<var_length_woart_key_value> &res,
                     int key_len = 8);


vector<var_length_woart_key_value> var_length_woart_scan(const var_length_woart_tree *t, uint64_t left, uint64_t right, int key_len = 8);


#endif //NVMKV_VARLENGTHWOART_H
