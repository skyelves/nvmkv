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

hashtree::hashtree() {
//    root = new_hashtree_node(init_depth, span);
    root = new_extendible_hash(init_depth, span);
    node_cnt++;
}

hashtree::hashtree(int _span, int _init_depth) {
    span = _span;
    init_depth = _init_depth;
//    root = new_hashtree_node(init_depth, _span);
    root = new_extendible_hash(init_depth, span);
    node_cnt++;
}

hashtree::~hashtree() {
    delete root;
}

void hashtree::init(int _span, int _init_depth) {
    span = _span;
    init_depth = _init_depth;
//    root = new_hashtree_node(init_depth, _span);
    root = new_extendible_hash(init_depth, span);
    node_cnt++;
}

hashtree *new_hashtree(int _span, int _init_depth) {
//    hashtree *_new_hash_tree = new hashtree(_span, _init_depth);
    hashtree *_new_hash_tree = static_cast<hashtree *>(fast_alloc(sizeof(hashtree)));
    _new_hash_tree->init(_span, _init_depth);
    return _new_hash_tree;
}

void hashtree::put(uint64_t k, uint64_t v) {
    extendible_hash *tmp = root;
    key_value *kv = new_key_value(k, v);
    uint64_t sub_key;
    for (int i = 0; i < 64; i += span) {
        sub_key = GET_SUB_KEY(k, i, span);
        int64_t next = tmp->get(sub_key);
        if (next == -1) {
            //not exists
            tmp->put(sub_key, (uint64_t) kv);
            return;
        } else {
            if (((bool *) next)[0]) {
                // next is key value pair, which means collides
                uint64_t pre_k = ((key_value *) next)->key;
                if (unlikely(k == pre_k)) {
                    //same key, update the value
                    ((key_value *) next)->value = v;
                    return;
                } else {
                    //not same key: needs to create a new eh
                    extendible_hash *new_eh = new_extendible_hash(init_depth, span);
                    uint64_t pre_k_next_sub_key = GET_SUB_KEY(pre_k, i + span, span);
                    new_eh->put(pre_k_next_sub_key, (uint64_t) next);
                    tmp->put(sub_key, (uint64_t) new_eh);
                    tmp = new_eh;
                    node_cnt++;
                    /* todo: for crash consistency,
                     * tmp->put(sub_key, (uint64_t) new_eh);
                     * should be the last operation
                     */
                }
            } else {
                // next is next extendible hash
                tmp = (extendible_hash *) next;
            }
        }
    }
    return;
}

int64_t hashtree::get(uint64_t k) {
    extendible_hash *tmp = root;
    int64_t next;
    for (int i = 0; i < 64; i += span) {
        uint64_t sub_key = GET_SUB_KEY(k, i, span);
        next = tmp->get(sub_key);
        if (next == -1) {
            //not exists
            return -1;
        } else {
            if (((bool *) next)[0]) {
                // is key value pair
                break;
            } else {
                // is next extendible hash
                tmp = (extendible_hash *) next;
            }
        }
    }
    return ((key_value *) next)->value;
}

