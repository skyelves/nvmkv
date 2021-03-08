//
// Created by 王柯 on 2021-03-04.
//

#include "hashtree_node.h"

hashtree_node::hashtree_node() {
    array = new kv[size];
//    array = new bucket[size];
}

hashtree_node::hashtree_node(int _size) {
    size = _size;
    array = new kv[size];
//    array = new bucket[size];
}

hashtree_node::~hashtree_node() {
    delete[]array;
}

hashtree_node *new_node(int _size) {
    hashtree_node *_new_node = new hashtree_node(_size);
    _new_node->hash = new BOBHash32(_size);
    return _new_node;
}