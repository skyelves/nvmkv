//
// Created by 杨冠群 on 7/27/21.
//

#include "varlengthfastfair.h"


void mfence() { asm volatile("mfence":: : "memory"); }



void clflush(char *data, int len) {
    volatile char *ptr = (char *) ((unsigned long) data & ~(CACHE_LINE_SIZE - 1));
    mfence();
    for (; ptr < data + len; ptr += CACHE_LINE_SIZE) {
        asm volatile("clflush %0" : "+m"(*(volatile char *) ptr));
        //++clflush_cnt;
    }
    mfence();
}


varlength_page::varlength_page(varlength_page *left, uint64_t key, varlength_page *right, uint32_t level ) {
    hdr.leftmost_ptr = left;
    hdr.level = level;
    records[0].key = key;
    records[0].ptr = (char *) right;
    records[1].ptr = NULL;

    hdr.last_index = 0;

    clflush((char *) this, sizeof(varlength_page));
}

inline void varlength_page::insert_key(char* key, int key_len, char *ptr, int *num_entries,
                        bool flush, bool update_last_index) {
    varLengthKey* _key = static_cast<varLengthKey *>(fast_alloc(sizeof(varLengthKey)));
    _key->init(key,key_len);

    if (flush) {
        clflush((char *) _key, sizeof(varLengthKey));
    }
    insert_key((uint64_t)_key,ptr,num_entries,flush,update_last_index);
}

inline void varlength_page::insert_key(uint64_t key, char *ptr, int *num_entries, bool flush, bool update_last_index) {
    // update switch_counter
    if (!IS_FORWARD(hdr.switch_counter))
        ++hdr.switch_counter;

    // FAST
    if (*num_entries == 0) { // this page is empty
        varlength_entry *new_entry = (varlength_entry *) &records[0];
        varlength_entry *array_end = (varlength_entry *) &records[1];

        new_entry->key = key;
        new_entry->ptr = (char *) ptr;

        array_end->ptr = (char *) NULL;

        if (flush) {
            clflush((char *) this, CACHE_LINE_SIZE);
        }
    } else {
        int i = *num_entries - 1, inserted = 0, to_flush_cnt = 0;
        records[*num_entries + 1].ptr = records[*num_entries].ptr;
        if (flush) {
            if ((uint64_t) &(records[*num_entries + 1].ptr) % CACHE_LINE_SIZE == 0)
                clflush((char *) &(records[*num_entries + 1].ptr), sizeof(char *));
        }

        // FAST
        for (i = *num_entries - 1; i >= 0; i--) {
            if (greater(records[i].key,key)){
                records[i + 1].ptr = records[i].ptr;
                records[i + 1].key = records[i].key;

                if (flush) {
                    uint64_t records_ptr = (uint64_t) (&records[i + 1]);

                    int remainder = records_ptr % CACHE_LINE_SIZE;
                    bool do_flush =
                            (remainder == 0) ||
                            ((((int) (remainder + sizeof(varlength_entry)) / CACHE_LINE_SIZE) == 1) &&
                                ((remainder + sizeof(varlength_entry)) % CACHE_LINE_SIZE) != 0);
                    if (do_flush) {
                        clflush((char *) records_ptr, CACHE_LINE_SIZE);
                        to_flush_cnt = 0;
                    } else
                        ++to_flush_cnt;
                }
            } else {
                records[i + 1].ptr = records[i].ptr;
                records[i + 1].key = key;
                records[i + 1].ptr = ptr;

                if (flush)
                    clflush((char *) &records[i + 1], sizeof(varlength_entry));
                inserted = 1;
                break;
            }
        }
        if (inserted == 0) {
            records[0].ptr = (char *) hdr.leftmost_ptr;
            records[0].key = key;
            records[0].ptr = ptr;
            if (flush)
                clflush((char *) &records[0], sizeof(varlength_entry));
        }
    }

    if (update_last_index) {
        hdr.last_index = *num_entries;
    }
    ++(*num_entries);
}

varlength_page* varlength_page::store(varlength_fastfair *bt, char *left, char* key, int key_len, char *value, bool flush,
                varlength_page *invalid_sibling) {
        // If this node has a sibling node,
        if (hdr.sibling_ptr && (hdr.sibling_ptr != invalid_sibling)) {
            // Compare this key with the first key of the sibling
            if (greater(key,key_len,hdr.sibling_ptr->records[0].key)){
                return hdr.sibling_ptr->store(bt, NULL, key, key_len, value, true,
                                              invalid_sibling);
            }
        }

        int num_entries = count();

        // FAST
        if (num_entries < varlength_cardinality - 1) {
            insert_key(key, key_len, value, &num_entries, flush);
            return this;
        } else { // FAIR
            // overflow
            // create a new node
            varlength_page *sibling = new varlength_page(hdr.level);
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
                sibling->hdr.leftmost_ptr = (varlength_page *) records[m].ptr;
            }

            sibling->hdr.sibling_ptr = hdr.sibling_ptr;
            clflush((char *) sibling, sizeof(varlength_page));

            hdr.sibling_ptr = sibling;
            clflush((char *) &hdr, sizeof(hdr));

            // set to NULL
            if (IS_FORWARD(hdr.switch_counter))
                hdr.switch_counter += 2;
            else
                ++hdr.switch_counter;
            records[m].ptr = NULL;
            clflush((char *) &records[m], sizeof(varlength_entry));

            hdr.last_index = m - 1;
            clflush((char *) &(hdr.last_index), sizeof(int16_t));

            num_entries = hdr.last_index + 1;

            varlength_page *ret;

            // insert the key
            if (greater(split_key,key,key_len)) {
                insert_key(key, key_len, value, &num_entries);
                ret = this;
            } else {
                sibling->insert_key(key, key_len, value, &sibling_cnt);
                ret = sibling;
            }

            // Set a new root or insert the split key to the parent
            if (bt->root == (char *) this) { // only one node can update the root ptr
                varlength_page *new_root =
                        new varlength_page((varlength_page *) this, split_key, sibling, hdr.level + 1);
                bt->setNewRoot((char *) new_root);
            } else {
                bt->fastfair_insert_internal(NULL, ((varLengthKey*)split_key)->key,((varLengthKey*)split_key)->len, (char *) sibling,
                                             hdr.level + 1);
            }

            return ret;
        }
    }

varlength_ff_key_value::varlength_ff_key_value() {

}

varlength_ff_key_value::varlength_ff_key_value(uint64_t _key, uint64_t _value) : key(_key), value(_value) {

}

varlength_fastfair *new_varlengthfastfair() {
    varlength_fastfair *new_ff = static_cast<varlength_fastfair *>(fast_alloc(sizeof(varlength_fastfair)));
    new_ff->init();
    return new_ff;
}

/*
 *  class fastfair
 */
varlength_fastfair::varlength_fastfair() {
    root = (char *) new varlength_page();
    height = 1;
}

void varlength_fastfair::init() {
    root = (char *) new varlength_page();
    height = 1;
}

void varlength_fastfair::setNewRoot(char *new_root) {
    this->root = (char *) new_root;
    clflush((char *) &(this->root), sizeof(char *));
    ++height;
}

char *varlength_fastfair::get(char* key, int len) {
    varlength_page *p = (varlength_page *) root;

    while (p->hdr.leftmost_ptr != NULL) {
        p = (varlength_page *) p->linear_search(key, len);
    }

   varlength_page *t;
    while ((t = (varlength_page *) p->linear_search(key, len)) == p->hdr.sibling_ptr) {
        p = t;
        if (!p) {
            break;
        }
    }

    if (!t) {
        printf("NOT FOUND %lu, t = %x\n", key, t);
        return NULL;
    }

    return (char *) t;
}

// insert the key in the leaf node
void varlength_fastfair::put(char* key, int key_len, char *value, int value_len) { // need to be string
    char *value_allocated = static_cast<char *>(fast_alloc(value_len));
    memcpy(value_allocated, value, value_len);
    clflush(value_allocated, value_len);
    varlength_page *p = (varlength_page *) root;

    while (p->hdr.leftmost_ptr != NULL) {
        p = (varlength_page *) p->linear_search(key, key_len);
    }

    if (!p->store(this, NULL, key, key_len, value_allocated, true)) { // store
        put(key, key_len, value_allocated);
    }
}

// store the key into the node at the given level
void varlength_fastfair::fastfair_insert_internal(char *left, char* key, int key_len, char *value,
                                        uint32_t level) {
    if (level > ((varlength_page *) root)->hdr.level)
        return;

    varlength_page *p = (varlength_page *) this->root;

    while (p->hdr.level > level)
        p = (varlength_page *) p->linear_search(key,key_len);

    if (!p->store(this, NULL, key, key_len, value, true)) {
        fastfair_insert_internal(left, key, key_len, value, level);
    }
}

// void varlength_fastfair::fastfair_delete(uint64_t key) {
//     varlength_page *p = (varlength_page *) root;

//     while (p->hdr.leftmost_ptr != NULL) {
//         p = (varlength_page *) p->linear_search(key);
//     }

//     varlength_page *t;
//     while ((t = (varlength_page *) p->linear_search(key)) == p->hdr.sibling_ptr) {
//         p = t;
//         if (!p)
//             break;
//     }

//     if (p) {
//         if (!p->remove(this, key)) {
//             fastfair_delete(key);
//         }
//     } else {
//         printf("not found the key to delete %lu\n", key);
//     }
// }

// void varlength_fastfair::fastfair_delete_internal(uint64_t key, char *ptr, uint32_t level,
//                                         uint64_t *deleted_key,
//                                         bool *is_leftmost_node, varlength_page **left_sibling) {
//     if (level > ((varlength_page *) this->root)->hdr.level)
//         return;

//     varlength_page *p = (varlength_page *) this->root;

//     while (p->hdr.level > level) {
//         p = (varlength_page *) p->linear_search(key);
//     }

//     if ((char *) p->hdr.leftmost_ptr == ptr) {
//         *is_leftmost_node = true;
//         return;
//     }

//     *is_leftmost_node = false;

//     for (int i = 0; p->records[i].ptr != NULL; ++i) {
//         if (p->records[i].ptr == ptr) {
//             if (i == 0) {
//                 if ((char *) p->hdr.leftmost_ptr != p->records[i].ptr) {
//                     *deleted_key = p->records[i].key;
//                     *left_sibling = p->hdr.leftmost_ptr;
//                     p->remove(this, *deleted_key, false, false);
//                     break;
//                 }
//             } else {
//                 if (p->records[i - 1].ptr != p->records[i].ptr) {
//                     *deleted_key = p->records[i].key;
//                     *left_sibling = (varlength_page *) p->records[i - 1].ptr;
//                     p->remove(this, *deleted_key, false, false);
//                     break;
//                 }
//             }
//         }
//     }
// }



// Function to search keys from "min" to "max"
// vector<varlength_ff_key_value> varlength_fastfair::scan(uint64_t min, uint64_t max) {
//     vector<varlength_ff_key_value> res;

//     _scan(min, max, res);
//     return res;
// }

