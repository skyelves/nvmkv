//
// Created by 王柯 on 2021-03-04.
//

#include <stdio.h>
#include <string.h>
#include <vector>
#include <stdlib.h>
#include "hashtree.h"

/*
 *         begin  len
 * key [______|___________|____________]
 */
#define GET_SUB_KEY(key, begin, len)  (((key)>>(64-(begin)-(len)))&(((uint64_t)1<<(len))-1))
//len should less than 32
//#define GET_SUB_KEY(key, begin, len)  ((key>>(64-begin-len))&(0xffff))
//#define GET_SUB_KEY(key, begin, len)  ((key>>(64-begin-len))&(0xff))

inline void mfence(void) {
    asm volatile("mfence":: :"memory");
}

inline void clflush(char *data, size_t len) {
    volatile char *ptr = (char *) ((unsigned long) data & (~(CACHELINESIZE - 1)));
    mfence();
    for (; ptr < data + len; ptr += CACHELINESIZE) {
        asm volatile("clflush %0" : "+m" (*(volatile char *) ptr));
    }
    mfence();
}

hashtree::hashtree() {
    root = new_hashtree_node(init_depth, span);
//    root = new_extendible_hash(init_depth, span);
    node_cnt++;
}

hashtree::hashtree(int _span, int _init_depth) {
    span = _span;
    init_depth = _init_depth;
    root = new_hashtree_node(init_depth, _span);
//    root = new_extendible_hash(init_depth, span);
    node_cnt++;
}

hashtree::~hashtree() {
    delete root;
}

void hashtree::init(int _span, int _init_depth) {
    span = _span;
    init_depth = _init_depth;
    root = new_hashtree_node(init_depth, _span);
//    root = new_extendible_hash(init_depth, span);
    span_test[0] = 32;
    span_test[1] = 16;
    span_test[2] = 8;
    span_test[3] = 8;
    node_cnt++;
}

hashtree *new_hashtree(int _span, int _init_depth) {
//    hashtree *_new_hash_tree = new hashtree(_span, _init_depth);
    hashtree *_new_hash_tree = static_cast<hashtree *>(fast_alloc(sizeof(hashtree)));
    _new_hash_tree->init(_span, _init_depth);
    return _new_hash_tree;
}

void hashtree::crash_consistent_put(hashtree_node *_node, uint64_t k, uint64_t v, uint64_t layer) {
    hashtree_node *tmp = _node;
    if (_node == NULL)
        tmp = root;

    ht_key_value *kv = new_ht_key_value(k, v);
    mfence();
    clflush((char *) kv, sizeof(ht_key_value));

    uint64_t sub_key;
    int i = 0, j = 0;
    for (j = 0; j < layer; j++) {
        i += span_test[j];
    }
    for (; i < 64; i += span_test[j], j++) {
        sub_key = GET_SUB_KEY(k, i, span_test[j]);
        uint64_t next = tmp->get(sub_key);
        if (next == 0) {
            //not exists
            tmp->put(sub_key, (uint64_t) kv);//crash consistency operations
            return;
        } else {
            if (((bool *) next)[0]) {
                // next is key value pair, which means collides
                uint64_t pre_k = ((ht_key_value *) next)->key;
                if (unlikely(k == pre_k)) {
                    //same key, update the value
                    ((ht_key_value *) next)->value = v;
                    mfence();
                    clflush((char *) &(((ht_key_value *) next)->value), 8);
                    return;
                } else {
                    //not same key: needs to create a new eh
                    hashtree_node *new_node = new_hashtree_node(init_depth, span_test[j + 1]);
                    uint64_t pre_k_next_sub_key = GET_SUB_KEY(pre_k, i + span_test[j], span_test[j + 1]);
                    new_node->put(pre_k_next_sub_key, (uint64_t) next);
                    crash_consistent_put(new_node, k, v, j + 1);
                    tmp->put(sub_key, (uint64_t) new_node);
                    node_cnt++;
                    return;
                    /* todo: for crash consistency,
                     * tmp->put(sub_key, (uint64_t) new_eh);
                     * should be the last operation
                     */
                }
            } else {
                // next is next extendible hash
                tmp = (hashtree_node *) next;
            }
        }
    }
}

void hashtree::put(uint64_t k, uint64_t v) {
    hashtree_node *tmp = root;

    ht_key_value *kv = new_ht_key_value(k, v);
    mfence();
    clflush((char *) kv, sizeof(ht_key_value));

    uint64_t sub_key;
    for (int i = 0, j = 0; i < 64; i += span_test[j], j++) {
        sub_key = GET_SUB_KEY(k, i, span_test[j]);
        uint64_t next = tmp->get(sub_key);
        if (next == 0) {
            //not exists
            tmp->put(sub_key, (uint64_t) kv);//crash consistency operations
            return;
        } else {
            if (((bool *) next)[0]) {
                // next is key value pair, which means collides
                uint64_t pre_k = ((ht_key_value *) next)->key;
                if (unlikely(k == pre_k)) {
                    //same key, update the value
                    ((ht_key_value *) next)->value = v;
                    mfence();
                    clflush((char *) &(((ht_key_value *) next)->value), 8);
                    return;
                } else {
                    //not same key: needs to create a new eh
                    hashtree_node *new_node = new_hashtree_node(init_depth, span_test[j + 1]);
                    uint64_t pre_k_next_sub_key = GET_SUB_KEY(pre_k, i + span_test[j], span_test[j + 1]);
                    new_node->put(pre_k_next_sub_key, (uint64_t) next);
                    tmp->put(sub_key, (uint64_t) new_node);
                    tmp = new_node;
                    node_cnt++;
                    /* todo: for crash consistency,
                     * tmp->put(sub_key, (uint64_t) new_eh);
                     * should be the last operation
                     */
                }
            } else {
                // next is next extendible hash
                tmp = (hashtree_node *) next;
            }
        }
    }
}

uint64_t hashtree::get(uint64_t k) {
    hashtree_node *tmp = root;
    int64_t next;
    for (int i = 0, j = 0; i < 64; i += span_test[j], j++) {
        uint64_t sub_key = GET_SUB_KEY(k, i, span_test[j]);
        next = tmp->get(sub_key);
        get_access++;
        if (next == 0) {
            //not exists
            return 0;
        } else {
            if (((bool *) next)[0]) {
                // is key value pair
                break;
            } else {
                // is next extendible hash
                tmp = (hashtree_node *) next;
            }
        }
    }
    return ((ht_key_value *) next)->value;
}

