//
// Created by 王柯 on 2021-03-04.
//
#include <vector>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "hashtree.h"

/*
 *         begin  len
 * key [______|___________|____________]
 */
#define GET_SUB_KEY(key, begin, len)  (((key)>>(64-(begin)-(len)))&((0x1ull<<(len))-1))
#define GET_SEG_NUM(key, key_len, depth)  ((key>>(key_len-depth))&((0x1ull<<depth)-1))
#define GET_BUCKET_NUM(key, bucket_mask_len) ((key)&((0x1ull<<bucket_mask_len)-1))


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
//    node_cnt++;
}

hashtree::hashtree(int _span, int _init_depth) {
    span = _span;
    init_depth = _init_depth;
    root = new_hashtree_node(init_depth, _span);
//    root = new_extendible_hash(init_depth, span);
//    node_cnt++;
}

hashtree::~hashtree() {
    delete root;
}

void hashtree::init(int _span, int _init_depth) {
    span = _span;
    span_test[0] = 32;
    span_test[1] = 16;
    span_test[2] = 16;
    span_test[3] = 16;
    init_depth = _init_depth;
    root = new_hashtree_node(init_depth, span_test[0]);
//    root = new_extendible_hash(init_depth, span);
//    node_cnt++;
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
            ht_key_value *kv = new_ht_key_value(k, v);
            clflush((char *) kv, sizeof(ht_key_value));
            tmp->put(sub_key, (uint64_t) kv);//crash consistency operations
            return;
        } else {
            if (((bool *) next)[0]) {
                // next is key value pair, which means collides
                uint64_t pre_k = ((ht_key_value *) next)->key;
                if (unlikely(k == pre_k)) {
                    //same key, update the value
                    ((ht_key_value *) next)->value = v;
                    clflush((char *) &(((ht_key_value *) next)->value), 8);
                    return;
                } else {
                    //not same key: needs to create a new eh
                    hashtree_node *new_node = new_hashtree_node(init_depth, span_test[j + 1]);
                    uint64_t pre_k_next_sub_key = GET_SUB_KEY(pre_k, i + span_test[j], span_test[j + 1]);
                    new_node->put(pre_k_next_sub_key, (uint64_t) next);
                    crash_consistent_put(new_node, k, v, j + 1);
                    tmp->put(sub_key, (uint64_t) new_node);
//                    node_cnt++;
                    return;
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
                    clflush((char *) &(((ht_key_value *) next)->value), 8);
                    return;
                } else {
                    //not same key: needs to create a new eh
                    hashtree_node *new_node = new_hashtree_node(init_depth, span_test[j + 1]);
                    uint64_t pre_k_next_sub_key = GET_SUB_KEY(pre_k, i + span_test[j], span_test[j + 1]);
                    new_node->put(pre_k_next_sub_key, (uint64_t) next);
                    tmp->put(sub_key, (uint64_t) new_node);
                    tmp = new_node;
//                    node_cnt++;
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
//        get_access++;
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

void hashtree::all_subtree_kv(hashtree_node *tmp, vector<ht_key_value> &res) {
    //bfs may perform better
    for (int i = 0; i < tmp->dir_size; ++i) {
        ht_segment *tmp_seg = tmp->dir[i];
        for (int j = 0; j < HT_MAX_BUCKET_NUM; ++j) {
            ht_bucket *tmp_bucket = &(tmp_seg->bucket[j]);
            for (int k = 0; k < HT_BUCKET_SIZE; ++k) {
                if (((bool *) tmp_bucket->counter[k].value)[0] == 1) {
                    res.push_back(*((ht_key_value *) tmp_bucket->counter[k].value));
                } else {
                    all_subtree_kv((hashtree_node *) tmp_bucket->counter[k].value, res);
                }
            }
        }
    }
}

void hashtree::node_scan(hashtree_node *tmp, uint64_t left, uint64_t right, uint64_t layer, vector<ht_key_value> &res) {
    int i = 0, j = 0;
    for (j = 0; j < layer; ++j) {
        i += span_test[j];
    }

    uint64_t sub_left = GET_SUB_KEY(left, i, span_test[j]);
    uint64_t sub_right = GET_SUB_KEY(right, i, span_test[j]);
    uint64_t left_index = GET_SEG_NUM(sub_left, span_test[j], tmp->global_depth);
    uint64_t right_index = GET_SEG_NUM(sub_right, span_test[j], tmp->global_depth);
    ht_segment *left_seg = tmp->dir[left_index];
    ht_segment *right_seg = tmp->dir[right_index];
    if (left_seg != right_seg) {
        ht_segment *tmp_seg;
        //add somes keys in left seg
        tmp_seg = left_seg;
        for (int k = 0; k < HT_MAX_BUCKET_NUM; ++k) {
            ht_bucket *tmp_bucket = &(tmp_seg->bucket[k]);
            for (int l = 0; l < HT_BUCKET_SIZE; ++l) {
                if (tmp_bucket->counter[l].key == sub_left && tmp_bucket->counter[l].value != 0) {
                    if (((bool *) tmp_bucket->counter[l].value)[0] == 1) {
                        res.push_back(*((ht_key_value *) tmp_bucket->counter[l].value));
                    } else {
                        node_scan((hashtree_node *) tmp_bucket->counter[l].value, left, 0xffffffffffffffff, j + 1, res);
                    }
                } else if (tmp_bucket->counter[l].key > sub_left) {
                    if (((bool *) tmp_bucket->counter[l].value)[0] == 1) {
                        res.push_back(*((ht_key_value *) tmp_bucket->counter[l].value));
                    } else {
                        all_subtree_kv((hashtree_node *) tmp_bucket->counter[l].value, res);
                    }
                }
            }
        }

        //add interal segs all keys
        for (int k = left_index + 1; k < right_index; ++k) {
            if (tmp->dir[k] != tmp_seg)
                tmp_seg = tmp->dir[k];
            else
                continue;
            if (tmp_seg != left_seg && tmp_seg != right_seg) {
                for (int l = 0; l < HT_MAX_BUCKET_NUM; ++l) {
                    ht_bucket *tmp_bucket = &(tmp_seg->bucket[l]);
                    for (int m = 0; m < HT_BUCKET_SIZE; ++m) {
                        if (tmp_bucket->counter[m].key != 0 && tmp_bucket->counter[m].value != 0) {
                            if (((bool *) tmp_bucket->counter[m].value)[0] == 1) {
                                res.push_back(*((ht_key_value *) tmp_bucket->counter[m].value));
                            } else {
                                all_subtree_kv((hashtree_node *) tmp_bucket->counter[m].value, res);
                            }
                        }
                    }
                }
            }
        }

        //add some keys in right seg
        tmp_seg = right_seg;
        for (int k = 0; k < HT_MAX_BUCKET_NUM; ++k) {
            ht_bucket *tmp_bucket = &(tmp_seg->bucket[k]);
            for (int l = 0; l < HT_BUCKET_SIZE; ++l) {
                if (tmp_bucket->counter[l].key == sub_right && tmp_bucket->counter[l].value != 0) {
                    if (((bool *) tmp_bucket->counter[l].value)[0] == 1) {
                        res.push_back(*((ht_key_value *) tmp_bucket->counter[l].value));
                    } else {
                        node_scan((hashtree_node *) tmp_bucket->counter[l].value, 0, right, j + 1, res);
                    }
                } else if (tmp_bucket->counter[l].key < sub_right && tmp_bucket->counter[l].value != 0) {
                    if (((bool *) tmp_bucket->counter[l].value)[0] == 1) {
                        res.push_back(*((ht_key_value *) tmp_bucket->counter[l].value));
                    } else {
                        all_subtree_kv((hashtree_node *) tmp_bucket->counter[l].value, res);
                    }
                }
            }
        }

    } else {
        //left segment == right segment
        ht_segment *tmp_seg = left_seg;
        if (sub_left == sub_right) {
            for (int k = 0; k < HT_MAX_BUCKET_NUM; ++k) {
                ht_bucket *tmp_bucket = &(tmp_seg->bucket[k]);
                for (int l = 0; l < HT_BUCKET_SIZE; ++l) {
                    if (tmp_bucket->counter[l].key == sub_left && tmp_bucket->counter[l].value != 0) {
                        if (((bool *) tmp_bucket->counter[l].value)[0] == 1) {
                            res.push_back(*((ht_key_value *) tmp_bucket->counter[l].value));
                        } else {
                            node_scan((hashtree_node *) tmp_bucket->counter[l].value, left, right, j + 1, res);
                        }
                    }
                }
            }
        } else {
            for (int k = 0; k < HT_MAX_BUCKET_NUM; ++k) {
                ht_bucket *tmp_bucket = &(tmp_seg->bucket[k]);
                for (int l = 0; l < HT_BUCKET_SIZE; ++l) {
                    if (tmp_bucket->counter[l].key == sub_left && tmp_bucket->counter[l].value != 0) {
                        if (((bool *) tmp_bucket->counter[l].value)[0] == 1) {
                            res.push_back(*((ht_key_value *) tmp_bucket->counter[l].value));
                        } else {
                            node_scan((hashtree_node *) tmp_bucket->counter[l].value, left, 0xffffffffffffffff, j + 1,
                                      res);
                        }
                    } else if (tmp_bucket->counter[l].key == sub_right && tmp_bucket->counter[l].value != 0) {
                        if (((bool *) tmp_bucket->counter[l].value)[0] == 1) {
                            res.push_back(*((ht_key_value *) tmp_bucket->counter[l].value));
                        } else {
                            node_scan((hashtree_node *) tmp_bucket->counter[l].value, 0, right, j + 1, res);
                        }
                    } else if (tmp_bucket->counter[l].key > sub_left && tmp_bucket->counter[l].key < sub_right) {
                        if (((bool *) tmp_bucket->counter[l].value)[0] == 1) {
                            res.push_back(*((ht_key_value *) tmp_bucket->counter[l].value));
                        } else {
                            all_subtree_kv((hashtree_node *) tmp_bucket->counter[l].value, res);
                        }
                    }
                }
            }
        }
    }
}

vector<ht_key_value> hashtree::scan(uint64_t left, uint64_t right) {
    vector<ht_key_value> res;
    node_scan(root, left, right, 0, res);
    return res;
}

void hashtree::update(uint64_t k, uint64_t v) {
    hashtree_node *tmp = root;

    uint64_t sub_key;

    for (int i = 0, j = 0; i < 64; i += span_test[j], j++) {
        sub_key = GET_SUB_KEY(k, i, span_test[j]);
        uint64_t next = tmp->get(sub_key);
        if (next == 0) {
            //not exists
            return;
        } else {
            if (((bool *) next)[0]) {
                uint64_t pre_k = ((ht_key_value *) next)->key;
                if (k == pre_k) {
                    //same key, update the value
                    ((ht_key_value *) next)->value = v;
                    clflush((char *) &(((ht_key_value *) next)->value), 8);
                    return;
                }
            } else {
                // next is next extendible hash
                tmp = (hashtree_node *) next;
            }
        }
    }
}

uint64_t hashtree::del(uint64_t k) {
    hashtree_node *tmp = root;

    uint64_t sub_key;

    for (int i = 0, j = 0; i < 64; i += span_test[j], j++) {
        sub_key = GET_SUB_KEY(k, i, span_test[j]);
        uint64_t next = tmp->get(sub_key);
        if (next == 0) {
            //not exists
            return 0;
        } else {
            if (((bool *) next)[0]) {
                uint64_t pre_k = ((ht_key_value *) next)->key;
                if (k == pre_k) {
                    //same key, update the value
                    uint64_t res = ((ht_key_value *) next)->value;
                    ((ht_key_value *) next)->value = 0;
                    clflush((char *) &(((ht_key_value *) next)->value), 8);
                    return res;
                }
            } else {
                // next is next extendible hash
                tmp = (hashtree_node *) next;
            }
        }
    }
}

void recovery(hashtree_node* root){
    if(!root){
        return;
    }
    for(int i=0;i<root->dir_size;i++){
        ht_segment* current_seg = root->dir[i];
        int64_t stride = pow(2, root->global_depth - current_seg->depth);
        int64_t left = i - i % stride;
        int64_t mid = left + stride / 2, right = left + stride;
        for(int j =0;j<HT_MAX_BUCKET_NUM;j++){
            for(int k=0;k<HT_BUCKET_SIZE;k++){
                if(current_seg->bucket[j].counter[k].key!=0){
                    if(!((bool *)current_seg->bucket[j].counter[k].value)[0]){
                        recovery((hashtree_node*)current_seg->bucket[j].counter[k].value);
                    }
                }
            }
        }
        int64_t before = i;
        int64_t flag  = -1;
        for(int j = left;j<right;j++){
            if(root->dir[j]->depth!=root->dir[before]->depth){
                if(root->dir[before]->depth>root->dir[j]->depth){
                    flag = before;
                    before = j;
                    break;
                }else{
                    flag = j;
                    break;
                }
            }
        }
        if(flag!=-1){
            int64_t need_recovery_stride = pow(2, root->global_depth - root->dir[flag]->depth);
            int64_t need_recovery_left = flag - flag % need_recovery_stride;
            int64_t need_recovery_mid = need_recovery_left + need_recovery_stride / 2, need_recovery_right = need_recovery_left + need_recovery_stride;
            for(int j = need_recovery_left;j<need_recovery_right;j++){
                if(root->dir[j]->depth!=root->dir[flag]->depth){
                    root->dir[j] =  root->dir[flag];
                    clflush((char *) root->dir[j], sizeof(ht_segment *));
                }
                root->dir[before]->depth +=1;
                clflush((char *) &(root->dir[before]->depth), sizeof(root->dir[before]->depth));
            }
        }
        i = right-1;
    }
}