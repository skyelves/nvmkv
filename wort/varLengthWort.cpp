#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <emmintrin.h>
#include <assert.h>
#include <x86intrin.h>
#include <math.h>
#include "varLengthWort.h"

/**
 * Macros to manipulate pointer tags
 */
#define IS_LEAF(x) (((uintptr_t)x & 1))
#define SET_LEAF(x) ((void*)((uintptr_t)x | 1))
#define LEAF_RAW(x) ((var_length_wort_leaf*)((void*)((uintptr_t)x & ~1)))

#define GET_VAR_POS(key,pos) ((unsigned char)((pos%2==0) ? (((unsigned char)(key[pos/2]))>>4) : ((uint64_t)(key[pos/2]) & ((0x1UL<<4)-1))))


#define CACHE_LINE_SIZE 64

static inline void mfence() {
    asm volatile("mfence":: : "memory");
}

static void flush_buffer(void *buf, unsigned long len, bool fence) {
    unsigned long i;
    len = len + ((unsigned long) (buf) & (CACHE_LINE_SIZE - 1));
    if (fence) {
        mfence();
        for (i = 0; i < len; i += CACHE_LINE_SIZE) {
            asm volatile ("clflush %0\n" : "+m" (*((char *) buf + i)));
        }
        mfence();
    } else {
        for (i = 0; i < len; i += CACHE_LINE_SIZE) {
            asm volatile ("clflush %0\n" : "+m" (*((char *) buf + i)));
        }
    }
}

static int get_index(unsigned long key, int depth) {
    int index;

    index = ((key >> ((WORT_MAX_DEPTH - depth) * WORT_NODE_BITS)) & WORT_LOW_BIT_MASK);
    return index;
}

/**
 * Allocates a node of the given type,
 * initializes to zero and sets the type.
 */
static var_length_wort_node *var_length_alloc_node() {
    var_length_wort_node *n;
    void *ret;
    ret = fast_alloc(sizeof(var_length_wort_node16));
//    posix_memalign(&ret, 64, sizeof(wort_node16));
    n = static_cast<var_length_wort_node *>(ret);
    memset(n, 0, sizeof(var_length_wort_node16));
    return n;
}

/**
 * Initializes an wort tree
 * @return 0 on success.
 */
int var_length_wort_tree_init(var_length_wort_tree *t) {
    t->root = NULL;
    t->size = 0;
    return 0;
}

var_length_wort_tree *new_var_length_wort_tree() {
    var_length_wort_tree *_new_wort_tree = static_cast<var_length_wort_tree *>(fast_alloc(sizeof(var_length_wort_tree)));
    var_length_wort_tree_init(_new_wort_tree);
    return _new_wort_tree;
}

static var_length_wort_node **find_child(var_length_wort_node *n, unsigned char c) {
    var_length_wort_node16 *p;

    p = (var_length_wort_node16 *) n;
    if (p->children[c])
        return &p->children[c];

    return NULL;
}

// Simple inlined if
static inline int min(int a, int b) {
    return (a < b) ? a : b;
}

/**
 * Returns the number of prefix characters shared between
 * the key and node.
 */
static int check_prefix(const var_length_wort_node *n, char* key, int key_len, int depth) {
//	int max_cmp = min(min(n->pwortial_len, WORT_MAX_PREFIX_LEN), (key_len * INDEX_BITS) - depth);
    int max_cmp = min(min(n->pwortial_len, WORT_MAX_PREFIX_LEN), WORT_MAX_HEIGHT - depth);
    int idx;
    for (idx = 0; idx < max_cmp; idx++) {
        if (n->pwortial[idx] != GET_VAR_POS(key, (depth + idx)))
            return idx;
    }
    return idx;
}

/**
 * Checks if a leaf matches
 * @return 0 on success.
 */
static int leaf_matches(const var_length_wort_leaf *n, char* key, int key_len, int depth) {
    (void) depth;
    // Fail if the key lengths are different
    if (n->key_len != (uint32_t) key_len) return 1;

    // Compare the keys stworting at the depth
	return !(memcmp(n->key, key, key_len)==0);
    // return !(n->key == key);
}

// Find the minimum leaf under a node
static var_length_wort_leaf *minimum(const var_length_wort_node *n) {
    // Handle base cases
    if (!n) return NULL;
    if (IS_LEAF(n)) return LEAF_RAW(n);

    int idx = 0;

    while (!((var_length_wort_node16 *) n)->children[idx]) idx++;
    return minimum(((var_length_wort_node16 *) n)->children[idx]);
}

static int longest_common_prefix(var_length_wort_leaf *l1, var_length_wort_leaf *l2, int depth) {
//	int idx, max_cmp = (min(l1->key_len, l2->key_len) * INDEX_BITS) - depth;
    int idx, max_cmp = WORT_MAX_HEIGHT - depth;

    for (idx = 0; idx < max_cmp; idx++) {
        if (GET_VAR_POS(l1->key, (depth + idx)) != GET_VAR_POS(l2->key, (depth + idx)))
            return idx;
    }
    return idx;
}

/**
 * Searches for a value in the wort tree
 * @arg t The tree
 * @arg key The key
 * @arg key_len The length of the key
 * @return NULL if the item was not found, otherwise
 * the value pointer is returned.
 */
void *var_length_wort_get(const var_length_wort_tree *t, char* key, int key_len) {
    var_length_wort_node **child;
    var_length_wort_node *n = t->root;
    int prefix_len, depth = 0;

    while (n) {
        // Might be a leaf
        if (IS_LEAF(n)) {
            n = (var_length_wort_node *) LEAF_RAW(n);
            // Check if the expanded path matches
            if (!leaf_matches((var_length_wort_leaf *) n, key, key_len, depth)) {
                return ((var_length_wort_leaf *) n)->value;
            }
            return NULL;
        }

        if (n->depth == depth) {
            // Bail if the prefix does not match
            if (n->pwortial_len) {
                prefix_len = check_prefix(n, key, key_len, depth);
                if (prefix_len != min(WORT_MAX_PREFIX_LEN, n->pwortial_len))
                    return NULL;
                depth = depth + n->pwortial_len;
            }
        } else {
            var_length_wort_leaf *leaf[2];
            int cnt, pos, i;

            for (pos = 0, cnt = 0; pos < 16; pos++) {
                if (((var_length_wort_node16 *) n)->children[pos]) {
                    leaf[cnt] = minimum(((var_length_wort_node16 *) n)->children[pos]);
                    cnt++;
                    if (cnt == 2)
                        break;
                }
            }

            int prefix_diff = longest_common_prefix(leaf[0], leaf[1], depth);
            var_length_wort_node old_path;
            old_path.pwortial_len = prefix_diff;
            for (i = 0; i < min(WORT_MAX_PREFIX_LEN, prefix_diff); i++)
                old_path.pwortial[i] = GET_VAR_POS(leaf[1]->key, (depth + i));

            prefix_len = check_prefix(&old_path, key, key_len, depth);
            if (prefix_len != min(WORT_MAX_PREFIX_LEN, old_path.pwortial_len))
                return NULL;
            depth = depth + old_path.pwortial_len;
        }

        // Recursively search
        child = find_child(n, GET_VAR_POS(key, depth));
        n = (child) ? *child : NULL;
        depth++;
    }
    return NULL;
}

static var_length_wort_leaf *make_leaf(char* key, int key_len, void *value, bool flush) {
    //wort_leaf *l = (wort_leaf*)malloc(sizeof(wort_leaf));
    var_length_wort_leaf *l;
    void *ret;
    ret = fast_alloc(sizeof(var_length_wort_leaf));
//    posix_memalign(&ret, 64, sizeof(wort_leaf));
    l = static_cast<var_length_wort_leaf *>(ret);
    l->value = value;
    l->key_len = key_len;
    l->key = key;

    if (flush == true)
        flush_buffer(l, sizeof(var_length_wort_leaf), true);
    return l;
}

static void add_child(var_length_wort_node16 *n, var_length_wort_node **ref, unsigned char c, void *child) {
    (void) ref;
    n->children[c] = (var_length_wort_node *) child;
}

/**
 * Calculates the index at which the prefixes mismatch
 */
static int prefix_mismatch(const var_length_wort_node *n, char* key, int key_len, int depth, var_length_wort_leaf **l) {
    int max_cmp = min(min(WORT_MAX_PREFIX_LEN, n->pwortial_len), WORT_MAX_HEIGHT - depth);
    int idx;
    for (idx = 0; idx < max_cmp; idx++) {
        if (n->pwortial[idx] != GET_VAR_POS(key, (depth + idx)))
            return idx;
    }

    // If the prefix is short we can avoid finding a leaf
    if (n->pwortial_len > WORT_MAX_PREFIX_LEN) {
        // Prefix is longer than what we've checked, find a leaf
        *l = minimum(n);
        max_cmp = WORT_MAX_HEIGHT - depth;
        for (; idx < max_cmp; idx++) {
            if (GET_VAR_POS((*l)->key, (idx + depth)) != GET_VAR_POS(key, (depth + idx)))
                return idx;
        }
    }
    return idx;
}

void recovery_prefix(var_length_wort_node *n, int depth) {
    var_length_wort_leaf *leaf[2];
    int cnt, pos, i, j;

    for (pos = 0, cnt = 0; pos < 16; pos++) {
        if (((var_length_wort_node16 *) n)->children[pos]) {
            leaf[cnt] = minimum(((var_length_wort_node16 *) n)->children[pos]);
            cnt++;
            if (cnt == 2)
                break;
        }
    }

    int prefix_diff = longest_common_prefix(leaf[0], leaf[1], depth);
    var_length_wort_node old_path;
    old_path.pwortial_len = prefix_diff;
    for (i = 0; i < min(WORT_MAX_PREFIX_LEN, prefix_diff); i++)
        old_path.pwortial[i] = GET_VAR_POS(leaf[1]->key, (depth + i));
    old_path.depth = depth;
    *((uint64_t *) n) = *((uint64_t *) &old_path);
    flush_buffer(n, sizeof(var_length_wort_node), true);
}

static void *recursive_insert(var_length_wort_node *n, var_length_wort_node **ref, char* key,
                              int key_len, void *value, int depth, int *old) {
    // If we are at a NULL node, inject a leaf
    if (!n) {
        *ref = (var_length_wort_node *) SET_LEAF(make_leaf(key, key_len, value, true));
        flush_buffer(ref, sizeof(uintptr_t), true);
        return NULL;
    }

    // If we are at a leaf, we need to replace it with a node
    if (IS_LEAF(n)) {
        var_length_wort_leaf *l = LEAF_RAW(n);

        // Check if we are updating an existing value
        if (!leaf_matches(l, key, key_len, depth)) {
            *old = 1;
            void *old_val = l->value;
            l->value = value;
            flush_buffer(&l->value, sizeof(uintptr_t), true);
            return old_val;
        }

        // New value, we must split the leaf into a node4
        var_length_wort_node16 *new_node = (var_length_wort_node16 *) var_length_alloc_node();
        new_node->n.depth = depth;

        // Create a new leaf
        var_length_wort_leaf *l2 = make_leaf(key, key_len, value, false);

        // Determine longest prefix
        int i, longest_prefix = longest_common_prefix(l, l2, depth);
        new_node->n.pwortial_len = longest_prefix;
        for (i = 0; i < min(WORT_MAX_PREFIX_LEN, longest_prefix); i++)
            new_node->n.pwortial[i] = GET_VAR_POS(key, (depth + i));

        // Add the leafs to the new node4
        add_child(new_node, ref, GET_VAR_POS(l->key, (depth + longest_prefix)), SET_LEAF(l));
        add_child(new_node, ref, GET_VAR_POS(l2->key, (depth + longest_prefix)), SET_LEAF(l2));

        mfence();
        flush_buffer(new_node, sizeof(var_length_wort_node16), false);
        flush_buffer(l2, sizeof(var_length_wort_leaf), false);
        mfence();

        *ref = (var_length_wort_node *) new_node;
        flush_buffer(ref, 8, true);
        return NULL;
    }

    if (n->depth != depth) {
        recovery_prefix(n, depth);
    }

    // Check if given node has a prefix
    if (n->pwortial_len) {
        // Determine if the prefixes differ, since we need to split
        var_length_wort_leaf *l = NULL;
        int prefix_diff = prefix_mismatch(n, key, key_len, depth, &l);
        if ((uint32_t) prefix_diff >= n->pwortial_len) {
            depth += n->pwortial_len;
            goto RECURSE_SEARCH;
        }

        // Create a new node
        var_length_wort_node16 *new_node = (var_length_wort_node16 *) var_length_alloc_node();
        new_node->n.depth = depth;
        new_node->n.pwortial_len = prefix_diff;
        memcpy(new_node->n.pwortial, n->pwortial, min(WORT_MAX_PREFIX_LEN, prefix_diff));

        // Adjust the prefix of the old node
        var_length_wort_node temp_path;
        if (n->pwortial_len <= WORT_MAX_PREFIX_LEN) {
            add_child(new_node, ref, n->pwortial[prefix_diff], n);
            temp_path.pwortial_len = n->pwortial_len - (prefix_diff + 1);
            temp_path.depth = (depth + prefix_diff + 1);
            memcpy(temp_path.pwortial, n->pwortial + prefix_diff + 1,
                   min(WORT_MAX_PREFIX_LEN, temp_path.pwortial_len));
        } else {
            int i;
            if (l == NULL)
                l = minimum(n);
            add_child(new_node, ref, GET_VAR_POS(l->key, (depth + prefix_diff)), n);
            temp_path.pwortial_len = n->pwortial_len - (prefix_diff + 1);
            for (i = 0; i < min(WORT_MAX_PREFIX_LEN, temp_path.pwortial_len); i++)
                temp_path.pwortial[i] = GET_VAR_POS(l->key, (depth + prefix_diff + 1 + i));
            temp_path.depth = (depth + prefix_diff + 1);
        }

        // Insert the new leaf
        l = make_leaf(key, key_len, value, false);
        add_child(new_node, ref, GET_VAR_POS(key, (depth + prefix_diff)), SET_LEAF(l));

        mfence();
        flush_buffer(new_node, sizeof(var_length_wort_node16), false);
        flush_buffer(l, sizeof(var_length_wort_leaf), false);
        mfence();

        *ref = (var_length_wort_node *) new_node;
        *((uint64_t *) n) = *((uint64_t *) &temp_path);

        mfence();
        flush_buffer(n, sizeof(var_length_wort_node), false);
        flush_buffer(ref, sizeof(uintptr_t), false);
        mfence();

        return NULL;
    }

    RECURSE_SEARCH:;

    // Find a child to recurse to
    var_length_wort_node **child = find_child(n, GET_VAR_POS(key, depth));
    if (child) {
        return recursive_insert(*child, child, key, key_len, value, depth + 1, old);
    }

    // No child, node goes within us
    var_length_wort_leaf *l = make_leaf(key, key_len, value, true);

    add_child((var_length_wort_node16 *) n, ref, GET_VAR_POS(key, depth), SET_LEAF(l));
    flush_buffer(&((var_length_wort_node16 *) n)->children[GET_VAR_POS(key, depth)], sizeof(uintptr_t), true);
    return NULL;
}

/**
 * Inserts a new value into the wort tree
 * @arg t The tree
 * @arg key The key
 * @arg key_len The length of the key
 * @arg value Opaque value.
 * @return NULL if the item was newly inserted, otherwise
 * the old value pointer is returned.
 */
void *var_length_wort_put(var_length_wort_tree *t, char* key, int key_len, void *value, int value_len) {
    int old_val = 0;
    void *value_allocated = fast_alloc(value_len);
    memcpy(value_allocated, value, value_len);
    char *key_allocated = (char *)fast_alloc(key_len);
    memcpy(key_allocated, key, key_len);
    flush_buffer(value_allocated, value_len, true);
    flush_buffer(key_allocated, key_len, true);
    void *old = recursive_insert(t->root, &t->root, key_allocated, key_len, value_allocated, 0, &old_val);
    if (!old_val) t->size++;
    return old;
}

// void wort_all_subtree_kv(wort_node *n, vector<wort_key_value> &res) {
//     if (n == NULL)
//         return;
//     wort_node *tmp = n;
//     wort_node **child;
//     if (IS_LEAF(tmp)) {
//         tmp = (wort_node *) LEAF_RAW(tmp);
//         wort_key_value tmp_kv;
//         tmp_kv.key = ((wort_leaf *) tmp)->key;
//         tmp_kv.value = *(uint64_t *) (((wort_leaf *) tmp)->value);
//         res.push_back(tmp_kv);
//     } else {
//         // Recursively search
//         for (int i = 0; i < 16; ++i) {
//             child = find_child(tmp, i);
//             wort_node *next = (child) ? *child : NULL;
//             wort_all_subtree_kv(next, res);
//         }
//     }
// }

// void
// wort_node_scan(wort_node *n, uint64_t left, uint64_t right, uint64_t depth, vector<wort_key_value> &res, int key_len) {
//     //depth first search
//     if (n == NULL)
//         return;
//     wort_node *tmp = n;
//     wort_node **child;
//     if (IS_LEAF(tmp)) {
//         tmp = (wort_node *) LEAF_RAW(tmp);
//         // Check if the expanded path matches
//         uint64_t tmp_key = ((wort_leaf *) tmp)->key;
//         if (tmp_key >= left && tmp_key <= right) {
//             wort_key_value tmp_kv;
//             tmp_kv.key = tmp_key;
//             tmp_kv.value = *(uint64_t *) (((wort_leaf *) tmp)->value);
//             res.push_back(tmp_kv);
//         }
//     } else {
//         if (tmp->pwortial_len) {
//             int max_cmp = min(min(tmp->pwortial_len, WORT_MAX_PREFIX_LEN), WORT_MAX_HEIGHT - depth);
//             for (int idx = 0; idx < max_cmp; idx++) {
//                 if (tmp->pwortial[idx] > get_index(left, depth + idx)) {
//                     break;
//                 } else if (tmp->pwortial[idx] < get_index(left, depth + idx)) {
//                     return;
//                 }
//             }
//             for (int idx = 0; idx < max_cmp; idx++) {
//                 if (tmp->pwortial[idx] < get_index(right, depth + idx)) {
//                     break;
//                 } else if (tmp->pwortial[idx] > get_index(left, depth + idx)) {
//                     return;
//                 }
//             }
//             depth = depth + tmp->pwortial_len;
//         }
//         // Recursively search
//         unsigned char left_index = get_index(left, depth);
//         unsigned char right_index = get_index(right, depth);

//         if (left_index != right_index) {
//             child = find_child(tmp, left_index);
//             wort_node *next = (child) ? *child : NULL;
//             wort_node_scan(next, left, 0xffffffffffffffff, depth + 1, res);
//             child = find_child(tmp, right_index);
//             next = (child) ? *child : NULL;
//             wort_node_scan(next, 0, right, depth + 1, res);
//         } else {
//             child = find_child(tmp, left_index);
//             wort_node *next = (child) ? *child : NULL;
//             wort_node_scan(next, left, right, depth + 1, res);
//         }

//         for (int i = left_index + 1; i < right_index; ++i) {
//             child = find_child(tmp, i);
//             wort_node *next = (child) ? *child : NULL;
//             wort_all_subtree_kv(next, res);
//         }
//     }
// }

// vector<wort_key_value> wort_scan(const wort_tree *t, uint64_t left, uint64_t right, int key_len) {
//     vector<wort_key_value> res;
//     wort_node_scan(t->root, left, right, 0, res);
//     return res;
// }
