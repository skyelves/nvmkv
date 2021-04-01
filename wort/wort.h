//
// Created by 王柯 on 2021-03-31.
//

#ifndef NVMKV_WORT_H
#define NVMKV_WORT_H

#include <stdint.h>
#include <stdbool.h>

/* If you want to change the number of entries,
 * change the values of NODE_BITS & MAX_DEPTH */
#define NODE_BITS			4
#define MAX_DEPTH			15
#define NUM_NODE_ENTRIES 	(0x1UL << NODE_BITS)
#define LOW_BIT_MASK		((0x1UL << NODE_BITS) - 1)

#define MAX_PREFIX_LEN		6
#define MAX_HEIGHT			(MAX_DEPTH + 1)

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

typedef int(*wort_callback)(void *data, const unsigned char *key, uint32_t key_len, void *value);

/**
 * This struct is included as pwort
 * of all the various node sizes
 */
typedef struct {
    unsigned char depth;
    unsigned char pwortial_len;
    unsigned char pwortial[MAX_PREFIX_LEN];
} wort_node;

/**
 * Full node with 16 children
 */
typedef struct {
    wort_node n;
    wort_node *children[NUM_NODE_ENTRIES];
} wort_node16;

/**
 * Represents a leaf. These are
 * of arbitrary size, as they include the key.
 */
typedef struct {
    void *value;
    uint32_t key_len;
    unsigned long key;
} wort_leaf;

/**
 * Main struct, points to root.
 */
typedef struct {
    wort_node *root;
    uint64_t size;
} wort_tree;

/**
 * Initializes an wort tree
 * @return 0 on success.
 */
int wort_tree_init(wort_tree *t);

wort_tree *new_wort_tree();

/**
 * Inserts a new value into the wort tree
 * @arg t The tree
 * @arg key The key
 * @arg key_len The length of the key
 * @arg value Opaque value.
 * @return NULL if the item was newly inserted, otherwise
 * the old value pointer is returned.
 */
void* wort_put(wort_tree *t, const unsigned long key, int key_len, void *value);

/**
 * Searches for a value in the wort tree
 * @arg t The tree
 * @arg key The key
 * @arg key_len The length of the key
 * @return NULL if the item was not found, otherwise
 * the value pointer is returned.
 */
void* wort_get(const wort_tree *t, const unsigned long key, int key_len);


#endif //NVMKV_WORT_H
