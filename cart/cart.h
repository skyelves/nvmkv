/**
 *    author:     UncP
 *    date:    2019-02-16
 *    license:    BSD-3
**/

#ifndef _adapitve_radix_tree_h_
#define _adaptive_radix_tree_h_

#include <stddef.h>

typedef struct concurrent_adaptive_radix_tree adaptive_radix_tree;

concurrent_adaptive_radix_tree* new_concurrent_adaptive_radix_tree();
void free_concurrent_adaptive_radix_tree(concurrent_adaptive_radix_tree *art);
int concurrent_adaptive_radix_tree_put(concurrent_adaptive_radix_tree *art, const void *_key, size_t len,
                                       const void *value, size_t value_len);
void* concurrent_adaptive_radix_tree_get(concurrent_adaptive_radix_tree *art, const void *key, size_t len);

#endif /* _adaptive_radix_tree_h_ */
