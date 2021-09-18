//
// Created by 杨冠群 on 7/27/21.
//

#ifndef NVMKV_VARLENGTHFASTFAIR_H
#define NVMKV_VARLENGTHFASTFAIR_H

#include "../fastalloc/fastalloc.h"

#include <cassert>
#include <climits>
#include <fstream>
#include <future>
#include <iostream>
#include <math.h>
#include <mutex>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <vector>

#define PAGESIZE 512

#define CACHE_LINE_SIZE 64

#define IS_FORWARD(c) (c % 2 == 0)

using namespace std;







class varLengthKey{
public:
    char* key;
    int len;

    void init(char* key, int len){
        this->key = key;
        this->len = len;
    }
};

class varlength_ff_key_value {
public:
    uint64_t key;
    uint64_t value;

    varlength_ff_key_value();

    varlength_ff_key_value(uint64_t _key, uint64_t _value);
};

class varlength_page;

class varlength_fastfair {
private:
    int height;
    char *root;

public:
    varlength_fastfair();

    void init();

    void setNewRoot(char *new_root);

    void put(char * key, int key_len, char *value, int value_len = 8);

    void fastfair_insert_internal(char *left, char* key, int key_len, char *value,
                                  uint32_t level);

    void fastfair_delete(uint64_t key);

    void fastfair_delete_internal(uint64_t key, char *ptr, uint32_t level,
                                  uint64_t *deleted_key,
                                  bool *is_leftmost_node, varlength_page **left_sibling);

    char *get(char* key, int key_len);

    // void _scan(uint64_t min, uint64_t max, vector<varlength_ff_key_value> &buf);

    // vector<varlength_ff_key_value> scan(uint64_t min, uint64_t max);

    void printAll();

    friend class varlength_page;
};

varlength_fastfair *new_varlengthfastfair();

class varlength_header {
private:
    varlength_page *leftmost_ptr;     // 8 bytes
    varlength_page *sibling_ptr;      // 8 bytes
    uint32_t level;         // 4 bytes
    uint8_t switch_counter; // 1 bytes
    uint8_t is_deleted;     // 1 bytes
    int16_t last_index;     // 2 bytes
    char dummy[8];          // 8 bytes

    friend class varlength_page;

    friend class varlength_fastfair;

public:
    varlength_header() {
        leftmost_ptr = NULL;
        sibling_ptr = NULL;
        switch_counter = 0;
        last_index = -1;
        is_deleted = false;
    }

    ~varlength_header() {}
};

class varlength_entry {
private:
    uint64_t key; // 8 bytes
    char *ptr;       // 8 bytes
public:
    varlength_entry() {
        key = LONG_MAX;
        ptr = NULL;
    }

    friend class varlength_page;

    friend class varlength_fastfair;
};

const int varlength_cardinality = (PAGESIZE - sizeof(varlength_header)) / sizeof(varlength_entry);
const int varlength_count_in_line = CACHE_LINE_SIZE / sizeof(varlength_entry);

class varlength_page {
private:
    varlength_header hdr;                 // header in persistent memory, 16 bytes
    varlength_entry records[varlength_cardinality]; // slots in persistent memory, 16 bytes * n

public:
    friend class varlength_fastfair;

    varlength_page(uint32_t level = 0) {
        hdr.level = level;
        records[0].ptr = NULL;
    }

    void init(uint32_t level = 0) {
        hdr.level = level;
        records[0].ptr = NULL;
    }

    // this is called when tree grows
    varlength_page(varlength_page *left, uint64_t key, varlength_page *right, uint32_t level = 0);

    void grow_init(varlength_page *left, uint64_t key, varlength_page *right, uint32_t level = 0);

    void *operator new(size_t size) {
        void *ret = fast_alloc(size);
//        posix_memalign(&ret, 64, size);
        return ret;
    }

    inline int count() {
        uint8_t previous_switch_counter;
        int count = 0;
        do {
            previous_switch_counter = hdr.switch_counter;
            count = hdr.last_index + 1;

            while (count >= 0 && records[count].ptr != NULL) {
                if (IS_FORWARD(previous_switch_counter))
                    ++count;
                else
                    --count;
            }

            if (count < 0) {
                count = 0;
                while (records[count].ptr != NULL) {
                    ++count;
                }
            }

        } while (previous_switch_counter != hdr.switch_counter);

        return count;
    }

    // inline bool remove_key(uint64_t key) {
    //     // Set the switch_counter
    //     if (IS_FORWARD(hdr.switch_counter))
    //         ++hdr.switch_counter;

    //     bool shift = false;
    //     int i;
    //     for (i = 0; records[i].ptr != NULL; ++i) {
    //         if (!shift && records[i].key == key) {
    //             records[i].ptr =
    //                     (i == 0) ? (char *) hdr.leftmost_ptr : records[i - 1].ptr;
    //             shift = true;
    //         }

    //         if (shift) {
    //             records[i].key = records[i + 1].key;
    //             records[i].ptr = records[i + 1].ptr;

    //             // flush
    //             uint64_t records_ptr = (uint64_t) (&records[i]);
    //             int remainder = records_ptr % CACHE_LINE_SIZE;
    //             bool do_flush =
    //                     (remainder == 0) ||
    //                     ((((int) (remainder + sizeof(varlength_entry)) / CACHE_LINE_SIZE) == 1) &&
    //                      ((remainder + sizeof(varlength_entry)) % CACHE_LINE_SIZE) != 0);
    //             if (do_flush) {
    //                 clflush((char *) records_ptr, CACHE_LINE_SIZE);
    //             }
    //         }
    //     }

    //     if (shift) {
    //         --hdr.last_index;
    //     }
    //     return shift;
    // }

    // bool remove(varlength_fastfair *bt, uint64_t key, bool only_rebalance = false,
    //             bool with_lock = true) {
    //     if (!only_rebalance) {
    //         int num_entries_before = count();

    //         // This node is root
    //         if (this == (varlength_page *) bt->root) {
    //             if (hdr.level > 0) {
    //                 if (num_entries_before == 1 && !hdr.sibling_ptr) {
    //                     bt->root = (char *) hdr.leftmost_ptr;
    //                     clflush((char *) &(bt->root), sizeof(char *));

    //                     hdr.is_deleted = 1;
    //                 }
    //             }

    //             // Remove the key from this node
    //             bool ret = remove_key(key);
    //             return true;
    //         }

    //         bool should_rebalance = true;
    //         // check the node utilization
    //         if (num_entries_before - 1 >= (int) ((cardinality - 1) * 0.5)) {
    //             should_rebalance = false;
    //         }

    //         // Remove the key from this node
    //         bool ret = remove_key(key);

    //         if (!should_rebalance) {
    //             return (hdr.leftmost_ptr == NULL) ? ret : true;
    //         }
    //     }

    //     // Remove a key from the parent node
    //     uint64_t deleted_key_from_parent = 0;
    //     bool is_leftmost_node = false;
    //     varlength_page *left_sibling;
    //     bt->fastfair_delete_internal(key, (char *) this, hdr.level + 1,
    //                                  &deleted_key_from_parent, &is_leftmost_node,
    //                                  &left_sibling);

    //     if (is_leftmost_node) {
    //         hdr.sibling_ptr->remove(bt, hdr.sibling_ptr->records[0].key, true,
    //                                 with_lock);
    //         return true;
    //     }

    //     int num_entries = count();
    //     int left_num_entries = left_sibling->count();

    //     // Merge or Redistribution
    //     int total_num_entries = num_entries + left_num_entries;
    //     if (hdr.leftmost_ptr)
    //         ++total_num_entries;

    //     uint64_t parent_key;

    //     if (total_num_entries > cardinality - 1) { // Redistribution
    //         int m = (int) ceil(total_num_entries / 2);

    //         if (num_entries < left_num_entries) { // left -> right
    //             if (hdr.leftmost_ptr == nullptr) {
    //                 for (int i = left_num_entries - 1; i >= m; i--) {
    //                     insert_key(left_sibling->records[i].key,
    //                                left_sibling->records[i].ptr, &num_entries);
    //                 }

    //                 left_sibling->records[m].ptr = nullptr;
    //                 clflush((char *) &(left_sibling->records[m].ptr), sizeof(char *));

    //                 left_sibling->hdr.last_index = m - 1;
    //                 clflush((char *) &(left_sibling->hdr.last_index), sizeof(int16_t));

    //                 parent_key = records[0].key;
    //             } else {
    //                 insert_key(deleted_key_from_parent, (char *) hdr.leftmost_ptr,
    //                            &num_entries);

    //                 for (int i = left_num_entries - 1; i > m; i--) {
    //                     insert_key(left_sibling->records[i].key,
    //                                left_sibling->records[i].ptr, &num_entries);
    //                 }

    //                 parent_key = left_sibling->records[m].key;

    //                 hdr.leftmost_ptr = (varlength_page *) left_sibling->records[m].ptr;
    //                 clflush((char *) &(hdr.leftmost_ptr), sizeof(varlength_page *));

    //                 left_sibling->records[m].ptr = nullptr;
    //                 clflush((char *) &(left_sibling->records[m].ptr), sizeof(char *));

    //                 left_sibling->hdr.last_index = m - 1;
    //                 clflush((char *) &(left_sibling->hdr.last_index), sizeof(int16_t));
    //             }

    //             if (left_sibling == ((varlength_page *) bt->root)) {
    //                 varlength_page *new_root =
    //                         new varlength_page(left_sibling, parent_key, this, hdr.level + 1);
    //                 bt->setNewRoot((char *) new_root);
    //             } else {
    //                 bt->fastfair_insert_internal((char *) left_sibling, ((varLengthKey*)parent_key)->key,((varLengthKey*)parent_key)->len,
    //                                              (char *) this, hdr.level + 1);
    //             }
    //         } else { // from leftmost case
    //             hdr.is_deleted = 1;
    //             clflush((char *) &(hdr.is_deleted), sizeof(uint8_t));

    //             varlength_page *new_sibling = new varlength_page(hdr.level);
    //             new_sibling->hdr.sibling_ptr = hdr.sibling_ptr;

    //             int num_dist_entries = num_entries - m;
    //             int new_sibling_cnt = 0;

    //             if (hdr.leftmost_ptr == nullptr) {
    //                 for (int i = 0; i < num_dist_entries; i++) {
    //                     left_sibling->insert_key(records[i].key, records[i].ptr,
    //                                              &left_num_entries);
    //                 }

    //                 for (int i = num_dist_entries; records[i].ptr != NULL; i++) {
    //                     new_sibling->insert_key(records[i].key, records[i].ptr,
    //                                             &new_sibling_cnt, false);
    //                 }

    //                 clflush((char *) (new_sibling), sizeof(varlength_page));

    //                 left_sibling->hdr.sibling_ptr = new_sibling;
    //                 clflush((char *) &(left_sibling->hdr.sibling_ptr), sizeof(varlength_page *));

    //                 parent_key = new_sibling->records[0].key;
    //             } else {
    //                 left_sibling->insert_key(deleted_key_from_parent,
    //                                          (char *) hdr.leftmost_ptr, &left_num_entries);

    //                 for (int i = 0; i < num_dist_entries - 1; i++) {
    //                     left_sibling->insert_key(records[i].key, records[i].ptr,
    //                                              &left_num_entries);
    //                 }

    //                 parent_key = records[num_dist_entries - 1].key;

    //                 new_sibling->hdr.leftmost_ptr =
    //                         (varlength_page *) records[num_dist_entries - 1].ptr;
    //                 for (int i = num_dist_entries; records[i].ptr != NULL; i++) {
    //                     new_sibling->insert_key(records[i].key, records[i].ptr,
    //                                             &new_sibling_cnt, false);
    //                 }
    //                 clflush((char *) (new_sibling), sizeof(varlength_page));

    //                 left_sibling->hdr.sibling_ptr = new_sibling;
    //                 clflush((char *) &(left_sibling->hdr.sibling_ptr), sizeof(varlength_page *));
    //             }

    //             if (left_sibling == ((varlength_page *) bt->root)) {
    //                 varlength_page *new_root =
    //                         new varlength_page(left_sibling, parent_key, new_sibling, hdr.level + 1);
    //                 bt->setNewRoot((char *) new_root);
    //             } else {
    //                 bt->fastfair_insert_internal((char *) left_sibling, parent_key,
    //                                              (char *) new_sibling, hdr.level + 1);
    //             }
    //         }
    //     } else {
    //         hdr.is_deleted = 1;
    //         clflush((char *) &(hdr.is_deleted), sizeof(uint8_t));
    //         if (hdr.leftmost_ptr)
    //             left_sibling->insert_key(deleted_key_from_parent,
    //                                      (char *) hdr.leftmost_ptr, &left_num_entries);

    //         for (int i = 0; records[i].ptr != NULL; ++i) {
    //             left_sibling->insert_key(records[i].key, records[i].ptr,
    //                                      &left_num_entries);
    //         }

    //         left_sibling->hdr.sibling_ptr = hdr.sibling_ptr;
    //         clflush((char *) &(left_sibling->hdr.sibling_ptr), sizeof(varlength_page *));
    //     }

    //     return true;
    // }

    inline void insert_key(char* key, int key_len, char *ptr, int *num_entries,
                           bool flush = true, bool update_last_index = true);

    inline void insert_key(uint64_t key, char *ptr, int *num_entries, bool flush = true, bool update_last_index = true);

    // Insert a new key - FAST and FAIR
    varlength_page *store(varlength_fastfair *bt, char *left, char* key, int key_len, char *value, bool flush,
                varlength_page *invalid_sibling = NULL) ;

    // Search keys with linear search
    void linear_search_range(uint64_t min, uint64_t max,
                             vector<varlength_ff_key_value> &buf) {
        int i, off = 0;
        uint8_t previous_switch_counter;
        varlength_page *current = this;

        while (current) {
            int old_off = off;
            do {
                previous_switch_counter = current->hdr.switch_counter;
                off = old_off;

                uint64_t tmp_key;
                char *tmp_ptr;

                if (IS_FORWARD(previous_switch_counter)) {
                    if ((tmp_key = current->records[0].key) > min) {
                        if (tmp_key < max) {
                            if ((tmp_ptr = current->records[0].ptr) != NULL) {
                                if (tmp_key == current->records[0].key) {
                                    if (tmp_ptr) {
//                                        buf[off++] = (unsigned long) tmp_ptr;
                                        varlength_ff_key_value tmp(tmp_key, *(uint64_t *) tmp_ptr);
                                        buf.push_back(tmp);
                                    }
                                }
                            }
                        } else
                            return;
                    }

                    for (i = 1; current->records[i].ptr != NULL; ++i) {
                        if ((tmp_key = current->records[i].key) > min) {
                            if (tmp_key < max) {
                                if ((tmp_ptr = current->records[i].ptr) !=
                                    current->records[i - 1].ptr) {
                                    if (tmp_key == current->records[i].key) {
                                        if (tmp_ptr) {
                                            varlength_ff_key_value tmp(tmp_key, *(uint64_t *) tmp_ptr);
                                            buf.push_back(tmp);
                                        }
                                    }
                                }
                            } else
                                return;
                        }
                    }
                } else {
                    for (i = count() - 1; i > 0; --i) {
                        if ((tmp_key = current->records[i].key) > min) {
                            if (tmp_key < max) {
                                if ((tmp_ptr = current->records[i].ptr) !=
                                    current->records[i - 1].ptr) {
                                    if (tmp_key == current->records[i].key) {
                                        if (tmp_ptr) {
                                            varlength_ff_key_value tmp(tmp_key, *(uint64_t *) tmp_ptr);
                                            buf.push_back(tmp);
                                        }
                                    }
                                }
                            } else
                                return;
                        }
                    }

                    if ((tmp_key = current->records[0].key) > min) {
                        if (tmp_key < max) {
                            if ((tmp_ptr = current->records[0].ptr) != NULL) {
                                if (tmp_key == current->records[0].key) {
                                    if (tmp_ptr) {
                                        varlength_ff_key_value tmp(tmp_key, *(uint64_t *) tmp_ptr);
                                        buf.push_back(tmp);
                                    }
                                }
                            }
                        } else
                            return;
                    }
                }
            } while (previous_switch_counter != current->hdr.switch_counter);

            current = current->hdr.sibling_ptr;
        }
    }

    char *linear_search(char* key, int len) {
        int i = 1;
        uint8_t previous_switch_counter;
        char *ret = NULL;
        char *t;
        uint64_t k;

        if (hdr.leftmost_ptr == NULL) { // Search a leaf node
            do {
                previous_switch_counter = hdr.switch_counter;
                ret = NULL;

                // search from left ro right
                if (IS_FORWARD(previous_switch_counter)) {
                    k = records[0].key;
                    if (keyIsSame(k,key,len)) {
                        if ((t = records[0].ptr) != NULL) {
                            if (k == records[0].key) {
                                ret = t;
                                continue;
                            }
                        }
                    }

                    for (i = 1; records[i].ptr != NULL; ++i) {
                        k = records[i].key;
                        if (keyIsSame(k,key,len)) {
                            if (records[i - 1].ptr != (t = records[i].ptr)) {
                                if (k == records[i].key) {
                                    ret = t;
                                    break;
                                }
                            }
                        }
                    }
                } else { // search from right to left
                    for (i = count() - 1; i > 0; --i) {
                        k = records[i].key;
                        if (keyIsSame(k,key,len)) {
                            if (records[i - 1].ptr != (t = records[i].ptr) && t) {
                                if (k == records[i].key) {
                                    ret = t;
                                    break;
                                }
                            }
                        }
                    }

                    if (!ret) {
                        k = records[0].key;
                        if ( keyIsSame(k,key,len)) {
                            if (NULL != (t = records[0].ptr) && t) {
                                if (k == records[0].key) {
                                    ret = t;
                                    continue;
                                }
                            }
                        }
                    }
                }
            } while (hdr.switch_counter != previous_switch_counter);

            if (ret) {
                return ret;
            }

            if ((t = (char *) hdr.sibling_ptr) && !greater((((varlength_page *) t)->records[0].key),key,len));
                return t;

            return NULL;
        } else { // internal node
            do {
                previous_switch_counter = hdr.switch_counter;
                ret = NULL;

                if (IS_FORWARD(previous_switch_counter)) {
                    k = records[0].key;
                    if (greater(k,key,len)) {
                        if ((t = (char *) hdr.leftmost_ptr) != records[0].ptr) {
                            ret = t;
                            continue;
                        }
                    }

                    for (i = 1; records[i].ptr != NULL; ++i) {
                        k = records[i].key;
                        if (greater(k,key,len)) {
                            if ((t = records[i - 1].ptr) != records[i].ptr) {
                                ret = t;
                                break;
                            }
                        }
                    }

                    if (!ret) {
                        ret = records[i - 1].ptr;
                        continue;
                    }
                } else { // search from right to left
                    for (i = count() - 1; i >= 0; --i) {
                        k = records[i].key;
                        if (!greater(k,key,len)) {
                            if (i == 0) {
                                if ((char *) hdr.leftmost_ptr != (t = records[i].ptr)) {
                                    ret = t;
                                    break;
                                }
                            } else {
                                if (records[i - 1].ptr != (t = records[i].ptr)) {
                                    ret = t;
                                    break;
                                }
                            }
                        }
                    }
                }
            } while (hdr.switch_counter != previous_switch_counter);

            if ((t = (char *) hdr.sibling_ptr) != NULL) {
                if (!greater(((varlength_page *) t)->records[0].key,key,len))
                    return t;
            }

            if (ret) {
                return ret;
            } else
                return (char *) hdr.leftmost_ptr;
        }

        return NULL;
    }


    bool keyIsSame(uint64_t key, char* key2, int len2){
        return keyIsSame(((varLengthKey*)key)->key,((varLengthKey*)key)->len,key2,len2);
    }

    bool keyIsSame(char* key1, int len1, char* key2, int len2){
        if(len1!=len2){
            return false;
        }
        return strncmp((char *)key1,(char *)key2,len1)==0;
    }

    bool greater(uint64_t key1, uint64_t key2){
        return greater(((varLengthKey*)key1)->key,((varLengthKey*)key1)->len,((varLengthKey*)key2)->key,((varLengthKey*)key2)->len);
    }

    bool greater(uint64_t key1, char* key2, int len2){
        return greater(((varLengthKey*)key1)->key,((varLengthKey*)key1)->len,key2,len2);
    }
    bool greater(char* key2, int len2, uint64_t key1){
        return greater(key2,len2,((varLengthKey*)key1)->key,((varLengthKey*)key1)->len);
    }
    bool greater(char* key1, int len1, char* key2, int len2){
        if(len1!=len2){
            return len1>len2; 
        }
        return strncmp((char *)key1,(char *)key2,len1)>0;
    }
};

#endif //NVMKV_VARLENGTHFASTFAIR_H
