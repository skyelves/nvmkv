//
// Created by 王柯 on 5/12/21.
//

#ifndef NVMKV_CONCURRENCY_FASTFAIR_H
#define NVMKV_CONCURRENCY_FASTFAIR_H

#include "../fastalloc/fastalloc.h"
#include <cassert>
#include <climits>
#include <fstream>
#include <future>
#include <iostream>
#include <math.h>
#include <mutex>
#include <shared_mutex>
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

inline void concurrency_mfence() { asm volatile("mfence":: : "memory"); }

inline void concurrency_clflush(char *data, int len) {
    volatile char *ptr = (char *) ((unsigned long) data & ~(CACHE_LINE_SIZE - 1));
    concurrency_mfence();
    for (; ptr < data + len; ptr += CACHE_LINE_SIZE) {
        asm volatile("clflush %0" : "+m"(*(volatile char *) ptr));
        //++clflush_cnt;
    }
    concurrency_mfence();
}

class concurrency_ff_key_value {
public:
    uint64_t key;
    uint64_t value;

    concurrency_ff_key_value();

    concurrency_ff_key_value(uint64_t _key, uint64_t _value);
};

class concurrency_page;

class concurrency_fastfair {
private:
    int height;
    char *root;

public:
    concurrency_fastfair();

    void init();

    void setNewRoot(char *new_root);

    void put(uint64_t key, char *value, int value_len = 8);

    void concurrency_fastfair_insert_internal(char *left, uint64_t key, char *value,
                                  uint32_t level);

    void concurrency_fastfair_delete(uint64_t key);

    void concurrency_fastfair_delete_internal(uint64_t key, char *ptr, uint32_t level,
                                  uint64_t *deleted_key,
                                  bool *is_leftmost_node, concurrency_page **left_sibling);

    char *get(uint64_t key);

    void _scan(uint64_t min, uint64_t max, vector<concurrency_ff_key_value> &buf);

    vector<concurrency_ff_key_value> scan(uint64_t min, uint64_t max);

    void printAll();

    friend class concurrency_page;
};

concurrency_fastfair *new_concurrency_fastfair();

class concurrency_header {
private:
    concurrency_page *leftmost_ptr;     // 8 bytes
    concurrency_page *sibling_ptr;      // 8 bytes
    uint32_t level;         // 4 bytes
    uint8_t switch_counter; // 1 bytes
    uint8_t is_deleted;     // 1 bytes
    int16_t last_index;     // 2 bytes
    char dummy[8];          // 8 bytes
    shared_mutex *mtx;

    friend class concurrency_page;

    friend class concurrency_fastfair;

public:
    concurrency_header() {
        leftmost_ptr = NULL;
        sibling_ptr = NULL;
        switch_counter = 0;
        last_index = -1;
        is_deleted = false;
        mtx = new shared_mutex();
    }

    void *operator new(size_t size) {
        return concurrency_fast_alloc(size);
    }

    ~concurrency_header() {}
};

class concurrency_entry {
private:
    uint64_t key; // 8 bytes
    char *ptr;       // 8 bytes
public:
    concurrency_entry() {
        key = LONG_MAX;
        ptr = NULL;
    }

    friend class concurrency_page;

    friend class concurrency_fastfair;
};

const int concurrency_cardinality = (PAGESIZE - sizeof(concurrency_header)) / sizeof(concurrency_entry);
const int concurrency_count_in_line = CACHE_LINE_SIZE / sizeof(concurrency_entry);

class concurrency_page {
private:
    concurrency_header hdr;                 // header in persistent memory, 16 bytes
    concurrency_entry records[concurrency_cardinality]; // slots in persistent memory, 16 bytes * n

public:
    friend class concurrency_fastfair;

    concurrency_page(uint32_t level = 0) {
        hdr.level = level;
        records[0].ptr = NULL;
    }

    // this is called when tree grows
    concurrency_page(concurrency_page *left, uint64_t key, concurrency_page *right, uint32_t level = 0) {
        hdr.leftmost_ptr = left;
        hdr.level = level;
        records[0].key = key;
        records[0].ptr = (char *) right;
        records[1].ptr = NULL;

        hdr.last_index = 0;

        concurrency_clflush((char *) this, sizeof(concurrency_page));
    }

    void *operator new(size_t size) {
        void *ret = concurrency_fast_alloc(size);
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

    inline bool remove_key(uint64_t key) {
        // Set the switch_counter
        if (IS_FORWARD(hdr.switch_counter))
            ++hdr.switch_counter;

        bool shift = false;
        int i;
        for (i = 0; records[i].ptr != NULL; ++i) {
            if (!shift && records[i].key == key) {
                records[i].ptr =
                        (i == 0) ? (char *) hdr.leftmost_ptr : records[i - 1].ptr;
                shift = true;
            }

            if (shift) {
                records[i].key = records[i + 1].key;
                records[i].ptr = records[i + 1].ptr;

                // flush
                uint64_t records_ptr = (uint64_t) (&records[i]);
                int remainder = records_ptr % CACHE_LINE_SIZE;
                bool do_flush =
                        (remainder == 0) ||
                        ((((int) (remainder + sizeof(concurrency_entry)) / CACHE_LINE_SIZE) == 1) &&
                         ((remainder + sizeof(concurrency_entry)) % CACHE_LINE_SIZE) != 0);
                if (do_flush) {
                    concurrency_clflush((char *) records_ptr, CACHE_LINE_SIZE);
                }
            }
        }

        if (shift) {
            --hdr.last_index;
        }
        return shift;
    }

    bool remove(concurrency_fastfair *bt, uint64_t key, bool only_rebalance = false,
                bool with_lock = true) {
        if (!only_rebalance) {
            int num_entries_before = count();

            // This node is root
            if (this == (concurrency_page *) bt->root) {
                if (hdr.level > 0) {
                    if (num_entries_before == 1 && !hdr.sibling_ptr) {
                        bt->root = (char *) hdr.leftmost_ptr;
                        concurrency_clflush((char *) &(bt->root), sizeof(char *));

                        hdr.is_deleted = 1;
                    }
                }

                // Remove the key from this node
                bool ret = remove_key(key);
                return true;
            }

            bool should_rebalance = true;
            // check the node utilization
            if (num_entries_before - 1 >= (int) ((concurrency_cardinality - 1) * 0.5)) {
                should_rebalance = false;
            }

            // Remove the key from this node
            bool ret = remove_key(key);

            if (!should_rebalance) {
                return (hdr.leftmost_ptr == NULL) ? ret : true;
            }
        }

        // Remove a key from the parent node
        uint64_t deleted_key_from_parent = 0;
        bool is_leftmost_node = false;
        concurrency_page *left_sibling;
        bt->concurrency_fastfair_delete_internal(key, (char *) this, hdr.level + 1,
                                     &deleted_key_from_parent, &is_leftmost_node,
                                     &left_sibling);

        if (is_leftmost_node) {
            hdr.sibling_ptr->remove(bt, hdr.sibling_ptr->records[0].key, true,
                                    with_lock);
            return true;
        }

        int num_entries = count();
        int left_num_entries = left_sibling->count();

        // Merge or Redistribution
        int total_num_entries = num_entries + left_num_entries;
        if (hdr.leftmost_ptr)
            ++total_num_entries;

        uint64_t parent_key;

        if (total_num_entries > concurrency_cardinality - 1) { // Redistribution
            int m = (int) ceil(total_num_entries / 2);

            if (num_entries < left_num_entries) { // left -> right
                if (hdr.leftmost_ptr == nullptr) {
                    for (int i = left_num_entries - 1; i >= m; i--) {
                        insert_key(left_sibling->records[i].key,
                                   left_sibling->records[i].ptr, &num_entries);
                    }

                    left_sibling->records[m].ptr = nullptr;
                    concurrency_clflush((char *) &(left_sibling->records[m].ptr), sizeof(char *));

                    left_sibling->hdr.last_index = m - 1;
                    concurrency_clflush((char *) &(left_sibling->hdr.last_index), sizeof(int16_t));

                    parent_key = records[0].key;
                } else {
                    insert_key(deleted_key_from_parent, (char *) hdr.leftmost_ptr,
                               &num_entries);

                    for (int i = left_num_entries - 1; i > m; i--) {
                        insert_key(left_sibling->records[i].key,
                                   left_sibling->records[i].ptr, &num_entries);
                    }

                    parent_key = left_sibling->records[m].key;

                    hdr.leftmost_ptr = (concurrency_page *) left_sibling->records[m].ptr;
                    concurrency_clflush((char *) &(hdr.leftmost_ptr), sizeof(concurrency_page *));

                    left_sibling->records[m].ptr = nullptr;
                    concurrency_clflush((char *) &(left_sibling->records[m].ptr), sizeof(char *));

                    left_sibling->hdr.last_index = m - 1;
                    concurrency_clflush((char *) &(left_sibling->hdr.last_index), sizeof(int16_t));
                }

                if (left_sibling == ((concurrency_page *) bt->root)) {
                    concurrency_page *new_root =
                            new concurrency_page(left_sibling, parent_key, this, hdr.level + 1);
                    bt->setNewRoot((char *) new_root);
                } else {
                    bt->concurrency_fastfair_insert_internal((char *) left_sibling, parent_key,
                                                 (char *) this, hdr.level + 1);
                }
            } else { // from leftmost case
                hdr.is_deleted = 1;
                concurrency_clflush((char *) &(hdr.is_deleted), sizeof(uint8_t));

                concurrency_page *new_sibling = new concurrency_page(hdr.level);
                new_sibling->hdr.sibling_ptr = hdr.sibling_ptr;

                int num_dist_entries = num_entries - m;
                int new_sibling_cnt = 0;

                if (hdr.leftmost_ptr == nullptr) {
                    for (int i = 0; i < num_dist_entries; i++) {
                        left_sibling->insert_key(records[i].key, records[i].ptr,
                                                 &left_num_entries);
                    }

                    for (int i = num_dist_entries; records[i].ptr != NULL; i++) {
                        new_sibling->insert_key(records[i].key, records[i].ptr,
                                                &new_sibling_cnt, false);
                    }

                    concurrency_clflush((char *) (new_sibling), sizeof(concurrency_page));

                    left_sibling->hdr.sibling_ptr = new_sibling;
                    concurrency_clflush((char *) &(left_sibling->hdr.sibling_ptr), sizeof(concurrency_page *));

                    parent_key = new_sibling->records[0].key;
                } else {
                    left_sibling->insert_key(deleted_key_from_parent,
                                             (char *) hdr.leftmost_ptr, &left_num_entries);

                    for (int i = 0; i < num_dist_entries - 1; i++) {
                        left_sibling->insert_key(records[i].key, records[i].ptr,
                                                 &left_num_entries);
                    }

                    parent_key = records[num_dist_entries - 1].key;

                    new_sibling->hdr.leftmost_ptr =
                            (concurrency_page *) records[num_dist_entries - 1].ptr;
                    for (int i = num_dist_entries; records[i].ptr != NULL; i++) {
                        new_sibling->insert_key(records[i].key, records[i].ptr,
                                                &new_sibling_cnt, false);
                    }
                    concurrency_clflush((char *) (new_sibling), sizeof(concurrency_page));

                    left_sibling->hdr.sibling_ptr = new_sibling;
                    concurrency_clflush((char *) &(left_sibling->hdr.sibling_ptr), sizeof(concurrency_page *));
                }

                if (left_sibling == ((concurrency_page *) bt->root)) {
                    concurrency_page *new_root =
                            new concurrency_page(left_sibling, parent_key, new_sibling, hdr.level + 1);
                    bt->setNewRoot((char *) new_root);
                } else {
                    bt->concurrency_fastfair_insert_internal((char *) left_sibling, parent_key,
                                                 (char *) new_sibling, hdr.level + 1);
                }
            }
        } else {
            hdr.is_deleted = 1;
            concurrency_clflush((char *) &(hdr.is_deleted), sizeof(uint8_t));
            if (hdr.leftmost_ptr)
                left_sibling->insert_key(deleted_key_from_parent,
                                         (char *) hdr.leftmost_ptr, &left_num_entries);

            for (int i = 0; records[i].ptr != NULL; ++i) {
                left_sibling->insert_key(records[i].key, records[i].ptr,
                                         &left_num_entries);
            }

            left_sibling->hdr.sibling_ptr = hdr.sibling_ptr;
            concurrency_clflush((char *) &(left_sibling->hdr.sibling_ptr), sizeof(concurrency_page *));
        }

        return true;
    }

    inline void insert_key(uint64_t key, char *ptr, int *num_entries,
                           bool flush = true, bool update_last_index = true) {
        // update switch_counter
        if (!IS_FORWARD(hdr.switch_counter))
            ++hdr.switch_counter;

        // FAST
        if (*num_entries == 0) { // this page is empty
            concurrency_entry *new_concurrency_entry = (concurrency_entry *) &records[0];
            concurrency_entry *array_end = (concurrency_entry *) &records[1];
            new_concurrency_entry->key = (uint64_t) key;
            new_concurrency_entry->ptr = (char *) ptr;

            array_end->ptr = (char *) NULL;

            if (flush) {
                concurrency_clflush((char *) this, CACHE_LINE_SIZE);
            }
        } else {
            int i = *num_entries - 1, inserted = 0, to_flush_cnt = 0;
            records[*num_entries + 1].ptr = records[*num_entries].ptr;
            if (flush) {
                if ((uint64_t) &(records[*num_entries + 1].ptr) % CACHE_LINE_SIZE == 0)
                    concurrency_clflush((char *) &(records[*num_entries + 1].ptr), sizeof(char *));
            }

            // FAST
            for (i = *num_entries - 1; i >= 0; i--) {
                if (key < records[i].key) {
                    records[i + 1].ptr = records[i].ptr;
                    records[i + 1].key = records[i].key;

                    if (flush) {
                        uint64_t records_ptr = (uint64_t) (&records[i + 1]);

                        int remainder = records_ptr % CACHE_LINE_SIZE;
                        bool do_flush =
                                (remainder == 0) ||
                                ((((int) (remainder + sizeof(concurrency_entry)) / CACHE_LINE_SIZE) == 1) &&
                                 ((remainder + sizeof(concurrency_entry)) % CACHE_LINE_SIZE) != 0);
                        if (do_flush) {
                            concurrency_clflush((char *) records_ptr, CACHE_LINE_SIZE);
                            to_flush_cnt = 0;
                        } else
                            ++to_flush_cnt;
                    }
                } else {
                    records[i + 1].ptr = records[i].ptr;
                    records[i + 1].key = key;
                    records[i + 1].ptr = ptr;

                    if (flush)
                        concurrency_clflush((char *) &records[i + 1], sizeof(concurrency_entry));
                    inserted = 1;
                    break;
                }
            }
            if (inserted == 0) {
                records[0].ptr = (char *) hdr.leftmost_ptr;
                records[0].key = key;
                records[0].ptr = ptr;
                if (flush)
                    concurrency_clflush((char *) &records[0], sizeof(concurrency_entry));
            }
        }

        if (update_last_index) {
            hdr.last_index = *num_entries;
        }
        ++(*num_entries);
    }

    // Insert a new key - FAST and FAIR
    concurrency_page *store(concurrency_fastfair *bt, char *left, uint64_t key, char *value, bool flush,
                concurrency_page *invalid_sibling = NULL) {

        hdr.mtx->lock();
        if(hdr.is_deleted){
            hdr.mtx->unlock();
            return NULL;
        }
        
        // If this node has a sibling node,
        if (hdr.sibling_ptr && (hdr.sibling_ptr != invalid_sibling)) {
            // Compare this key with the first key of the sibling
            if (key > hdr.sibling_ptr->records[0].key) {
                hdr.mtx->unlock();
                return hdr.sibling_ptr->store(bt, NULL, key, value, true,
                                              invalid_sibling);
            }
        }

        int num_entries = count();

        // FAST
        if (num_entries < concurrency_cardinality - 1) {
            insert_key(key, value, &num_entries, flush);
            hdr.mtx->unlock();
            return this;
        } else { // FAIR
            // overflow
            // create a new node
            concurrency_page *sibling = new concurrency_page(hdr.level);
            int m = (int) ceil(num_entries / 2);
            uint64_t split_key = records[m].key;

            // migrate half of keys into the sibling
            int sibling_cnt = 0;
            if (hdr.leftmost_ptr == NULL) { // leaf node
                for (int i = m; i < num_entries; ++i) {
                    sibling->insert_key(records[i].key, records[i].ptr, &sibling_cnt,
                                        false);
                }
            } else { // internal node
                for (int i = m + 1; i < num_entries; ++i) {
                    sibling->insert_key(records[i].key, records[i].ptr, &sibling_cnt,
                                        false);
                }
                sibling->hdr.leftmost_ptr = (concurrency_page *) records[m].ptr;
            }

            sibling->hdr.sibling_ptr = hdr.sibling_ptr;
            concurrency_clflush((char *) sibling, sizeof(concurrency_page));

            // if(hdr.leftmost_ptr == nullptr){
            //     sibling->hdr.mtx->lock();
            // }
            hdr.sibling_ptr = sibling;
            concurrency_clflush((char *) &hdr, sizeof(hdr));

            // set to NULL
            if (IS_FORWARD(hdr.switch_counter))
                hdr.switch_counter += 2;
            else
                ++hdr.switch_counter;
            records[m].ptr = NULL;
            concurrency_clflush((char *) &records[m], sizeof(concurrency_entry));

            hdr.last_index = m - 1;
            concurrency_clflush((char *) &(hdr.last_index), sizeof(int16_t));

            num_entries = hdr.last_index + 1;

            concurrency_page *ret;

            // insert the key
            if (key < split_key) {
                insert_key(key, value, &num_entries);
                ret = this;
            } else {
                sibling->insert_key(key, value, &sibling_cnt);
                ret = sibling;
            }

            // Set a new root or insert the split key to the parent
            if (bt->root == (char *) this) { // only one node can update the root ptr
                concurrency_page *new_root =
                        new concurrency_page((concurrency_page *) this, split_key, sibling, hdr.level + 1);
                bt->setNewRoot((char *) new_root);

                // if(hdr.leftmost_ptr!=nullptr){
                //     hdr.mtx->unlock();
                // }
            } else {
                // if(hdr.leftmost_ptr!=nullptr){
                //     hdr.mtx->unlock();
                // }
                bt->concurrency_fastfair_insert_internal(NULL, split_key, (char *) sibling,
                                             hdr.level + 1);
            }

            hdr.mtx->unlock();
            // sibling->hdr.mtx->unlock();
            return ret;
        }
    }

    // Search keys with linear search
    void linear_search_range(uint64_t min, uint64_t max,
                             vector<concurrency_ff_key_value> &buf) {
        int i, off = 0;
        uint8_t previous_switch_counter;
        concurrency_page *current = this;

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
                                        concurrency_ff_key_value tmp(tmp_key, *(uint64_t *) tmp_ptr);
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
                                            concurrency_ff_key_value tmp(tmp_key, *(uint64_t *) tmp_ptr);
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
                                            concurrency_ff_key_value tmp(tmp_key, *(uint64_t *) tmp_ptr);
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
                                        concurrency_ff_key_value tmp(tmp_key, *(uint64_t *) tmp_ptr);
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

    char *linear_search(uint64_t key) {
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
                    if ((k = records[0].key) == key) {
                        if ((t = records[0].ptr) != NULL) {
                            if (k == records[0].key) {
                                ret = t;
                                continue;
                            }
                        }
                    }

                    for (i = 1; records[i].ptr != NULL; ++i) {
                        if ((k = records[i].key) == key) {
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
                        if ((k = records[i].key) == key) {
                            if (records[i - 1].ptr != (t = records[i].ptr) && t) {
                                if (k == records[i].key) {
                                    ret = t;
                                    break;
                                }
                            }
                        }
                    }

                    if (!ret) {
                        if ((k = records[0].key) == key) {
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

            if ((t = (char *) hdr.sibling_ptr) && key >= ((concurrency_page *) t)->records[0].key)
                return t;

            return NULL;
        } else { // internal node
            do {
                previous_switch_counter = hdr.switch_counter;
                ret = NULL;

                if (IS_FORWARD(previous_switch_counter)) {
                    if (key < (k = records[0].key)) {
                        if ((t = (char *) hdr.leftmost_ptr) != records[0].ptr) {
                            ret = t;
                            continue;
                        }
                    }

                    for (i = 1; records[i].ptr != NULL; ++i) {
                        if (key < (k = records[i].key)) {
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
                        if (key >= (k = records[i].key)) {
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
                if (key >= ((concurrency_page *) t)->records[0].key)
                    return t;
            }

            if (ret) {
                return ret;
            } else
                return (char *) hdr.leftmost_ptr;
        }

        return NULL;
    }

    // print a node
    void print() {
        if (hdr.leftmost_ptr == NULL)
            printf("[%d] leaf %x \n", this->hdr.level, this);
        else
            printf("[%d] internal %x \n", this->hdr.level, this);
        printf("last_index: %d\n", hdr.last_index);
        printf("switch_counter: %d\n", hdr.switch_counter);
        printf("search direction: ");
        if (IS_FORWARD(hdr.switch_counter))
            printf("->\n");
        else
            printf("<-\n");

        if (hdr.leftmost_ptr != NULL)
            printf("%x ", hdr.leftmost_ptr);

        for (int i = 0; records[i].ptr != NULL; ++i)
            printf("%ld,%x ", records[i].key, records[i].ptr);

        printf("%x ", hdr.sibling_ptr);

        printf("\n");
    }

    void printAll() {
        if (hdr.leftmost_ptr == NULL) {
            printf("printing leaf node: ");
            print();
        } else {
            printf("printing internal node: ");
            print();
            ((concurrency_page *) hdr.leftmost_ptr)->printAll();
            for (int i = 0; records[i].ptr != NULL; ++i) {
                ((concurrency_page *) records[i].ptr)->printAll();
            }
        }
    }
};

#endif //NVMKV_FASTFAIR_H
