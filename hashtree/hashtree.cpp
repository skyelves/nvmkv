//
// Created by 王柯 on 2021-03-04.
//

#include <stdio.h>
#include <string.h>
#include <vector>
#include <stdlib.h>
#include "hashtree.h"

hashtree::hashtree() {
    hash = new BOBHash32(19);
    root = new_node(1024);
}

hashtree::~hashtree() {
    return;
}

bool hashtree::put(uint64_t k, uint64_t v) {
//    char k_char[66] = {0};
//    sprintf(k_char, "%llu", k);
//    int k_len = strlen(k_char);
//    int index = root->hash->run(k_char, k_len) % root->size;
//    root->array[index].k = k;
//    root->array[index].v = v;
    hashtree_node *tmp = root;
    char k_char[66] = {0};
    sprintf(k_char, "%llu", k);
    int k_len = strlen(k_char);
    uint value = hash->run(k_char, k_len);
    uint index = 0;
    while (1) {
        index = value % tmp->size;
//        index = tmp->hash->run(k_char, k_len) % tmp->size;
        if (tmp->array[index].k == 0) {
            //slot empty
            tmp->array[index].k = k;
            tmp->array[index].v = v;
            break;
        } else {
            if (tmp->array[index].k == k) {
                tmp->array[index].v = v;
                break;
            } else if (tmp->array[index].v == 0) {
                //not empty
                //go to the deeper layer hash
                tmp = (hashtree_node *) tmp->array[index].k;
            } else {
                //collision
                //create a deeper layer hash table and insert two kv
                uint64_t k2 = tmp->array[index].k;
                uint64_t v2 = tmp->array[index].v;
                char k2_char[66] = {0};
                sprintf(k2_char, "%llu", k2);
                int k2_len = strlen(k2_char);
                uint value2 = hash->run(k2_char, k2_len);
                hashtree_node *first_new_node = new_node(tmp->size * 2);
                hashtree_node *_new_node = first_new_node;
                while (1) {
                    int index1 = value % _new_node->size;
                    int index2 = value2 % _new_node->size;
//                    int index1 = _new_node->hash->run(k_char, k_len) % _new_node->size;
//                    int index2 = _new_node->hash->run(k2_char, k2_len) % _new_node->size;
                    if (index1 != index2) {
                        _new_node->array[index1].k = k;
                        _new_node->array[index1].v = v;
                        _new_node->array[index2].k = k2;
                        _new_node->array[index2].v = v2;
                        break;
                    } else {
                        //todo collision again
                        hashtree_node *deeper_node = new_node(_new_node->size * 2);
                        _new_node->array[index1].k = (uint64_t) deeper_node;
                        _new_node = deeper_node;
                    }
                }
                //todo check the slot is still valid and compress the following operations to a 8 byte operation
                tmp->array[index].k = (uint64_t) first_new_node;
                tmp->array[index].v = 0;
                break;
            }
        }
    }
    return true;
}

int hashtree::get(int k) {
    return 0;
}

