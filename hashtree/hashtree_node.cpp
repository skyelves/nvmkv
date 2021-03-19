//
// Created by 王柯 on 2021-03-04.
//

#include "hashtree_node.h"

hashtree_node::hashtree_node() {
    eht = new extendible_hash(4);
}

hashtree_node::hashtree_node(int _depth, int _key_len) {
    eht = new extendible_hash(_depth, _key_len);
}

hashtree_node::~hashtree_node() {
    delete eht;
}

hashtree_node *new_hashtree_node(int _depth, int _key_len) {
    hashtree_node *_new_node = new hashtree_node(_depth, _key_len);
    return _new_node;
}