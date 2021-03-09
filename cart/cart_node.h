/**
 *    author:     UncP
 *    date:    2019-02-05
 *    license:    BSD-3
**/

#ifndef _art_node_h_
#define _art_node_h_

#include <stddef.h>

#ifdef Debug
#include <assert.h>
#define debug_assert(v) assert(v)
#else
#define debug_assert(v)
#endif // Debug

#define fuck printf("fuck\n");

#define likely(x)   (__builtin_expect(!!(x), 1))
#define unlikely(x) (__builtin_expect(!!(x), 0))

typedef struct cart_node art_node;

#define is_leaf(ptr) ((uintptr_t)(ptr) & 1)
#define make_leaf(ptr) ((uintptr_t)((const char *)(ptr) - 1) | 1)
#define get_leaf_key(ptr) (((const char *)((uintptr_t)(ptr) & (~(uintptr_t)1))) + 1)
#define get_leaf_len(ptr) ((size_t)*(char *)((uintptr_t)(ptr) & (~(uintptr_t)1)))

cart_node* new_node_art_node();
void free_art_node(cart_node *an);
cart_node** art_node_add_child(cart_node *an, unsigned char byte, cart_node *child, cart_node **new_node);
cart_node** art_node_find_child(cart_node *an, uint64_t version, unsigned char byte);
int art_node_is_full(cart_node *an);
void art_node_set_prefix(cart_node *an, const void *key, size_t off, int prefix_len);
const char* art_node_get_prefix(cart_node *an);
int art_node_prefix_compare(cart_node *an, uint64_t version, const void *key, size_t len, size_t off);
unsigned char art_node_truncate_prefix(cart_node *an, int off);
uint64_t art_node_get_version(cart_node *an);
uint64_t art_node_get_version_unsafe(cart_node *an);
uint64_t art_node_get_stable_expand_version(cart_node *an);
// uint64_t art_node_get_stable_insert_version(cart_node *an);
int art_node_version_get_prefix_len(uint64_t version);
int art_node_version_compare_expand(uint64_t version1, uint64_t version2);
// int art_node_version_compare_insert(uint64_t version1, uint64_t version2);
int art_node_lock(cart_node *an);
cart_node* art_node_get_locked_parent(cart_node *an);
void art_node_set_parent_unsafe(cart_node *an, cart_node *parent);
void art_node_unlock(cart_node *an);
int art_node_version_is_old(uint64_t version);
cart_node* art_node_replace_leaf_child(cart_node *an, const void *key, size_t len, size_t off);
void art_node_replace_child(cart_node *parent, unsigned char byte, cart_node *old, cart_node *new_node);
cart_node* art_node_expand_and_insert(cart_node *an, const void *key, size_t len, size_t off, int common);
size_t art_node_version_get_offset(uint64_t version);

#ifdef Debug
void art_node_print(cart_node *an);
void print_key(const void *key, size_t len);
#endif

#endif /* _art_node_h_ */
