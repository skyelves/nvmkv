//
// Created by 杨冠群 on 2021-06-20.
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "concurrency_hashtree.h"

/*
 *         begin  len
 * key [______|___________|____________]
 */
#define GET_SUB_KEY(key, begin, len)  (((key)>>(64-(begin)-(len)))&(((uint64_t)1<<(len))-1))
#define GET_SEG_NUM(key, key_len, depth)  ((key>>(key_len-depth))&(((uint64_t)1<<depth)-1))
#define GET_BUCKET_NUM(key, bucket_mask_len) ((key)&(((uint64_t)1<<bucket_mask_len)-1))


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

concurrencyhashtree::concurrencyhashtree() {
    root = new_concurrency_hashtree_node(init_depth, span);
//    root = new_extendible_hash(init_depth, span);
//    node_cnt++;
}

concurrencyhashtree::concurrencyhashtree(int _span, int _init_depth) {
    span = _span;
    init_depth = _init_depth;
    root = new_concurrency_hashtree_node(init_depth, _span);
//    root = new_extendible_hash(init_depth, span);
//    node_cnt++;
}

concurrencyhashtree::~concurrencyhashtree() {
    delete root;
}

void concurrencyhashtree::init(int _span, int _init_depth) {
    span = _span;
    span_test[0] = 32;
    span_test[1] = 16;
    span_test[2] = 16;
    span_test[3] = 8;
    init_depth = _init_depth;
    root = new_concurrency_hashtree_node(init_depth, span_test[0]);
//    root = new_extendible_hash(init_depth, span);
//    node_cnt++;
}

concurrencyhashtree *new_concurrency_hashtree(int _span, int _init_depth) {
//    hashtree *_new_hash_tree = new hashtree(_span, _init_depth);
    concurrencyhashtree *_new_hash_tree = static_cast<concurrencyhashtree *>(fast_alloc(sizeof(concurrencyhashtree)));
    _new_hash_tree->init(_span, _init_depth);
    return _new_hash_tree;
}

void concurrencyhashtree::crash_consistent_put(concurrency_hashtree_node *_node, uint64_t k, uint64_t v, uint64_t layer) {
    concurrency_hashtree_node *tmp = _node;
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
            concurrency_ht_key_value *kv = new_concurrency_ht_key_value(k, v);
            clflush((char *) kv, sizeof(concurrency_ht_key_value));
            tmp->put(sub_key, (uint64_t) kv);//crash consistency operations
            return;
        } else {
            if (((bool *) next)[0]) {
                // next is key value pair, which means collides
                uint64_t pre_k = ((concurrency_ht_key_value *) next)->key;
                if (unlikely(k == pre_k)) {
                    //same key, update the value
                    ((concurrency_ht_key_value *) next)->value = v;
                    clflush((char *) &(((concurrency_ht_key_value *) next)->value), 8);
                    return;
                } else {
                    //not same key: needs to create a new eh
                    concurrency_hashtree_node *new_node = new_concurrency_hashtree_node(init_depth, span_test[j + 1]);
                    uint64_t pre_k_next_sub_key = GET_SUB_KEY(pre_k, i + span_test[j], span_test[j + 1]);
                    new_node->put(pre_k_next_sub_key, (uint64_t) next);
                    crash_consistent_put(new_node, k, v, j + 1);
                    tmp->put(sub_key, (uint64_t) new_node);
//                    node_cnt++;
                    return;
                }
            } else {
                // next is next extendible hash
                tmp = (concurrency_hashtree_node *) next;
            }
        }
    }
}

void concurrencyhashtree::put(uint64_t k, uint64_t v) {
    concurrency_hashtree_node *tmp = root;

    concurrency_ht_key_value *kv = new_concurrency_ht_key_value(k, v);
    clflush((char *) kv, sizeof(concurrency_ht_key_value));

    uint64_t sub_key;
    for (int i = 0, j = 0; i < 64; i += span_test[j], j++) {
        sub_key = GET_SUB_KEY(k, i, span_test[j]);

        GET_RETRY:
            if(!tmp->read_lock()){
                std::this_thread::yield();
                goto GET_RETRY;
            }

            uint64_t dir_index = GET_SEG_NUM(sub_key, tmp->key_len, tmp->global_depth);
            concurrency_ht_segment *tmp_seg = tmp->dir[dir_index];
            
            // read lock segment
            if(!tmp_seg->read_lock()){
                tmp->free_read_lock();
                std::this_thread::yield();
                goto GET_RETRY;
            }

            // double check 
            uint64_t check_dir_index = GET_SEG_NUM(sub_key, tmp->key_len, tmp->global_depth);
            if(tmp_seg!=tmp->dir[check_dir_index]){
                tmp_seg->free_read_lock();
                tmp->free_read_lock();
                std::this_thread::yield();
                goto GET_RETRY;
            }

            uint64_t seg_index = GET_BUCKET_NUM(sub_key, HT_BUCKET_MASK_LEN);
            concurrency_ht_bucket *tmp_bucket = &(tmp_seg->bucket[seg_index]);
            tmp_bucket->read_lock();

            uint64_t next = 0;
            uint64_t pos = 0;
            for (int i = 0; i < HT_BUCKET_SIZE; ++i) {
                if (sub_key == tmp_bucket->counter[i].key) {
                    next = tmp_bucket->counter[i].value;
                    pos = i;
                    break;
                }
            }

            // tmp_bucket->free_read_lock();
            // tmp_seg->free_read_lock();

        if (next == 0) {
            //not exists

            tmp_bucket->free_read_lock();
            tmp_bucket->write_lock();

            if(tmp_bucket->counter[pos].value!=0){
                tmp_bucket->free_write_lock();
                tmp_seg->free_read_lock();
                tmp->free_read_lock();
                std::this_thread::yield();
                goto GET_RETRY;
            }
            
            // there is a place to insert
            tmp_bucket->counter[pos].value = v;
            mfence();
            tmp_bucket->counter[pos].key = k;

            // Here we clflush 16bytes rather than two 8 bytes because all counter are set to 0.
            // If crash after key flushed, then the value is 0. When we return the value, we would find that the key is not inserted.
            clflush((char *) &(tmp_bucket->counter[pos].key), 16);
            
            tmp_bucket->free_write_lock();
            tmp_seg->free_read_lock();
            tmp->free_read_lock();

        
            return;
        } else {
            if (((bool *) next)[0]) {
                // next is key value pair, which means collides
                uint64_t pre_k = ((concurrency_ht_key_value *) next)->key;

                tmp_bucket->free_read_lock();
                tmp_bucket->write_lock();

                if(tmp_bucket->counter[pos].value!=next){
                    tmp_bucket->free_write_lock();
                    tmp_seg->free_read_lock();
                    tmp->free_read_lock();  
                    std::this_thread::yield();
                    goto GET_RETRY;
                }   
            
                if (unlikely(k == pre_k)) {
                    //same key, update the value
                    ((concurrency_ht_key_value *) next)->value = v;
                    clflush((char *) &(((concurrency_ht_key_value *) next)->value), 8);
                    return;
                } else {
                    //not same key: needs to create a new eh
                    concurrency_hashtree_node *new_node = new_concurrency_hashtree_node(init_depth, span_test[j + 1]);
                    uint64_t pre_k_next_sub_key = GET_SUB_KEY(pre_k, i + span_test[j], span_test[j + 1]);
                    new_node->put(pre_k_next_sub_key, (uint64_t) next);
                    tmp_bucket->counter[pos].value =  (uint64_t) new_node;
                    tmp_bucket->free_write_lock();
                    tmp_seg->free_read_lock();
                    tmp = new_node;
                }
            } else {
                // next is next extendible hash
                tmp_bucket->free_read_lock();
                tmp_seg->free_read_lock();

                tmp = (concurrency_hashtree_node *) next;
            }
        }
    }
}

uint64_t concurrencyhashtree::get(uint64_t k) {
    concurrency_hashtree_node *tmp = root;
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
                tmp = (concurrency_hashtree_node *) next;
            }
        }
    }
    return ((concurrency_ht_key_value *) next)->value;
}

void concurrencyhashtree::all_subtree_kv(concurrency_hashtree_node *tmp, vector<concurrency_ht_key_value> &res) {
    //bfs may perform better
    for (int i = 0; i < tmp->dir_size; ++i) {
        concurrency_ht_segment *tmp_seg = tmp->dir[i];
        for (int j = 0; j < HT_MAX_BUCKET_NUM; ++j) {
            concurrency_ht_bucket *tmp_bucket = &(tmp_seg->bucket[j]);
            for (int k = 0; k < HT_BUCKET_SIZE; ++k) {
                if (((bool *) tmp_bucket->counter[k].value)[0] == 1) {
                    res.push_back(*((concurrency_ht_key_value *) tmp_bucket->counter[k].value));
                } else {
                    all_subtree_kv((concurrency_hashtree_node *) tmp_bucket->counter[k].value, res);
                }
            }
        }
    }
}

void concurrencyhashtree::node_scan(concurrency_hashtree_node *tmp, uint64_t left, uint64_t right, uint64_t layer, vector<concurrency_ht_key_value> &res) {
    int i = 0, j = 0;
    for (j = 0; j < layer; ++j) {
        i += span_test[j];
    }

    uint64_t sub_left = GET_SUB_KEY(left, i, span_test[j]);
    uint64_t sub_right = GET_SUB_KEY(right, i, span_test[j]);
    uint64_t left_index = GET_SEG_NUM(sub_left, span_test[j], tmp->global_depth);
    uint64_t right_index = GET_SEG_NUM(sub_right, span_test[j], tmp->global_depth);
    concurrency_ht_segment *left_seg = tmp->dir[left_index];
    concurrency_ht_segment *right_seg = tmp->dir[right_index];
    if (left_seg != right_seg) {
        concurrency_ht_segment *tmp_seg;
        //add somes keys in left seg
        tmp_seg = left_seg;
        for (int k = 0; k < HT_MAX_BUCKET_NUM; ++k) {
            concurrency_ht_bucket *tmp_bucket = &(tmp_seg->bucket[k]);
            for (int l = 0; l < HT_BUCKET_SIZE; ++l) {
                if (tmp_bucket->counter[l].key == sub_left && tmp_bucket->counter[l].value != 0) {
                    if (((bool *) tmp_bucket->counter[l].value)[0] == 1) {
                        res.push_back(*((concurrency_ht_key_value *) tmp_bucket->counter[l].value));
                    } else {
                        node_scan((concurrency_hashtree_node *) tmp_bucket->counter[l].value, left, 0xffffffffffffffff, j + 1, res);
                    }
                } else if (tmp_bucket->counter[l].key > sub_left) {
                    if (((bool *) tmp_bucket->counter[l].value)[0] == 1) {
                        res.push_back(*((concurrency_ht_key_value *) tmp_bucket->counter[l].value));
                    } else {
                        all_subtree_kv((concurrency_hashtree_node *) tmp_bucket->counter[l].value, res);
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
                    concurrency_ht_bucket *tmp_bucket = &(tmp_seg->bucket[l]);
                    for (int m = 0; m < HT_BUCKET_SIZE; ++m) {
                        if (tmp_bucket->counter[m].key != 0 && tmp_bucket->counter[m].value != 0) {
                            if (((bool *) tmp_bucket->counter[m].value)[0] == 1) {
                                res.push_back(*((concurrency_ht_key_value *) tmp_bucket->counter[m].value));
                            } else {
                                all_subtree_kv((concurrency_hashtree_node *) tmp_bucket->counter[m].value, res);
                            }
                        }
                    }
                }
            }
        }

        //add some keys in right seg
        tmp_seg = right_seg;
        for (int k = 0; k < HT_MAX_BUCKET_NUM; ++k) {
            concurrency_ht_bucket *tmp_bucket = &(tmp_seg->bucket[k]);
            for (int l = 0; l < HT_BUCKET_SIZE; ++l) {
                if (tmp_bucket->counter[l].key == sub_right && tmp_bucket->counter[l].value != 0) {
                    if (((bool *) tmp_bucket->counter[l].value)[0] == 1) {
                        res.push_back(*((concurrency_ht_key_value *) tmp_bucket->counter[l].value));
                    } else {
                        node_scan((concurrency_hashtree_node *) tmp_bucket->counter[l].value, 0, right, j + 1, res);
                    }
                } else if (tmp_bucket->counter[l].key < sub_right && tmp_bucket->counter[l].value != 0) {
                    if (((bool *) tmp_bucket->counter[l].value)[0] == 1) {
                        res.push_back(*((concurrency_ht_key_value *) tmp_bucket->counter[l].value));
                    } else {
                        all_subtree_kv((concurrency_hashtree_node *) tmp_bucket->counter[l].value, res);
                    }
                }
            }
        }

    } else {
        //left segment == right segment
        concurrency_ht_segment *tmp_seg = left_seg;
        if (sub_left == sub_right) {
            for (int k = 0; k < HT_MAX_BUCKET_NUM; ++k) {
                concurrency_ht_bucket *tmp_bucket = &(tmp_seg->bucket[k]);
                for (int l = 0; l < HT_BUCKET_SIZE; ++l) {
                    if (tmp_bucket->counter[l].key == sub_left && tmp_bucket->counter[l].value != 0) {
                        if (((bool *) tmp_bucket->counter[l].value)[0] == 1) {
                            res.push_back(*((concurrency_ht_key_value *) tmp_bucket->counter[l].value));
                        } else {
                            node_scan((concurrency_hashtree_node *) tmp_bucket->counter[l].value, left, right, j + 1, res);
                        }
                    }
                }
            }
        } else {
            for (int k = 0; k < HT_MAX_BUCKET_NUM; ++k) {
                concurrency_ht_bucket *tmp_bucket = &(tmp_seg->bucket[k]);
                for (int l = 0; l < HT_BUCKET_SIZE; ++l) {
                    if (tmp_bucket->counter[l].key == sub_left && tmp_bucket->counter[l].value != 0) {
                        if (((bool *) tmp_bucket->counter[l].value)[0] == 1) {
                            res.push_back(*((concurrency_ht_key_value *) tmp_bucket->counter[l].value));
                        } else {
                            node_scan((concurrency_hashtree_node *) tmp_bucket->counter[l].value, left, 0xffffffffffffffff, j + 1,
                                      res);
                        }
                    } else if (tmp_bucket->counter[l].key == sub_right && tmp_bucket->counter[l].value != 0) {
                        if (((bool *) tmp_bucket->counter[l].value)[0] == 1) {
                            res.push_back(*((concurrency_ht_key_value *) tmp_bucket->counter[l].value));
                        } else {
                            node_scan((concurrency_hashtree_node *) tmp_bucket->counter[l].value, 0, right, j + 1, res);
                        }
                    } else if (tmp_bucket->counter[l].key > sub_left && tmp_bucket->counter[l].key < sub_right) {
                        if (((bool *) tmp_bucket->counter[l].value)[0] == 1) {
                            res.push_back(*((concurrency_ht_key_value *) tmp_bucket->counter[l].value));
                        } else {
                            all_subtree_kv((concurrency_hashtree_node *) tmp_bucket->counter[l].value, res);
                        }
                    }
                }
            }
        }
    }
}

vector<concurrency_ht_key_value> concurrencyhashtree::scan(uint64_t left, uint64_t right) {
    vector<concurrency_ht_key_value> res;
    node_scan(root, left, right, 0, res);
    return res;
}

void concurrencyhashtree::update(uint64_t k, uint64_t v) {
    concurrency_hashtree_node *tmp = root;

    uint64_t sub_key;

    for (int i = 0, j = 0; i < 64; i += span_test[j], j++) {
        sub_key = GET_SUB_KEY(k, i, span_test[j]);
        uint64_t next = tmp->get(sub_key);
        if (next == 0) {
            //not exists
            return;
        } else {
            if (((bool *) next)[0]) {
                uint64_t pre_k = ((concurrency_ht_key_value *) next)->key;
                if (k == pre_k) {
                    //same key, update the value
                    ((concurrency_ht_key_value *) next)->value = v;
                    clflush((char *) &(((concurrency_ht_key_value *) next)->value), 8);
                    return;
                }
            } else {
                // next is next extendible hash
                tmp = (concurrency_hashtree_node *) next;
            }
        }
    }
}

uint64_t concurrencyhashtree::del(uint64_t k) {
    concurrency_hashtree_node *tmp = root;

    uint64_t sub_key;

    for (int i = 0, j = 0; i < 64; i += span_test[j], j++) {
        sub_key = GET_SUB_KEY(k, i, span_test[j]);
        uint64_t next = tmp->get(sub_key);
        if (next == 0) {
            //not exists
            return 0;
        } else {
            if (((bool *) next)[0]) {
                uint64_t pre_k = ((concurrency_ht_key_value *) next)->key;
                if (k == pre_k) {
                    //same key, update the value
                    uint64_t res = ((concurrency_ht_key_value *) next)->value;
                    ((concurrency_ht_key_value *) next)->value = 0;
                    clflush((char *) &(((concurrency_ht_key_value *) next)->value), 8);
                    return res;
                }
            } else {
                // next is next extendible hash
                tmp = (concurrency_hashtree_node *) next;
            }
        }
    }
}