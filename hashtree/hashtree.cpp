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
#define GET_SUB_KEY(key, begin, len)  ((key>>(64-begin-len))&((1<<len)-1))
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
    hashtree *_new_hash_tree = new hashtree(_span, _init_depth);
    return _new_hash_tree;
}

void hashtree::put(uint64_t k, uint64_t v) {
//    hashtree_node *tmp = root;
    extendible_hash *tmp = root;
    uint64_t sub_key;
    for (int i = 0; i < 64 - span; i += span) {
        sub_key = GET_SUB_KEY(k, i, span);
        int64_t next = tmp->get(sub_key);
        if (next == -1) {
            //not exists
//            next = (int64_t) new_hashtree_node(init_depth, span);
            next = (int64_t) new_extendible_hash(init_depth, span);
            node_cnt++;
            tmp->put(sub_key, next);
        }
        tmp = (extendible_hash *) next;
    }
    sub_key = GET_SUB_KEY(k, (sizeof(k) * 8 - span), span);
    tmp->put(sub_key, v);
    return;
}

int64_t hashtree::get(uint64_t k) {
    extendible_hash *tmp = root;
    int64_t next;
    for (int i = 0; i < 64; i += span) {
        uint16_t sub_key = GET_SUB_KEY(k, i, span);
        next = tmp->get(sub_key);
        if (next == -1) {
            //not exists
            break;
        } else {
            tmp = (extendible_hash *) next;
        }
    }
    return next;
}

