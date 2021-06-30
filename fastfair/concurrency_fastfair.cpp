//
// Created by 王柯 on 5/12/21.
//

#include "concurrency_fastfair.h"

concurrency_ff_key_value::concurrency_ff_key_value() {

}

concurrency_ff_key_value::concurrency_ff_key_value(uint64_t _key, uint64_t _value) : key(_key), value(_value) {

}

concurrency_fastfair *new_concurrency_fastfair() {
    concurrency_fastfair *new_ff = static_cast<concurrency_fastfair *>(concurrency_fast_alloc(sizeof(concurrency_fastfair)));
    new_ff->init();
    return new_ff;
}

/*
 *  class concurrency_fastfair
 */
concurrency_fastfair::concurrency_fastfair() {
    root = (char *) new concurrency_page();
    height = 1;
}

void concurrency_fastfair::init() {
    root = (char *) new concurrency_page();
    height = 1;
}

void concurrency_fastfair::setNewRoot(char *new_root) {
    this->root = (char *) new_root;
    concurrency_clflush((char *) &(this->root), sizeof(char *));
    ++height;
}

char *concurrency_fastfair::get(uint64_t key) {
    concurrency_page *p = (concurrency_page *) root;

    while (p->hdr.leftmost_ptr != NULL) {
        p = (concurrency_page *) p->linear_search(key);
    }

    concurrency_page *t;
    while ((t = (concurrency_page *) p->linear_search(key)) == p->hdr.sibling_ptr) {
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
void concurrency_fastfair::put(uint64_t key, char *value, int value_len) { // need to be string
    char *value_allocated = static_cast<char *>(concurrency_fast_alloc(value_len));
    memcpy(value_allocated, value, value_len);
    concurrency_clflush(value_allocated, value_len);
    concurrency_page *p = (concurrency_page *) root;

    while (p->hdr.leftmost_ptr != NULL) {
        p = (concurrency_page *) p->linear_search(key);
    }

    if (!p->store(this, NULL, key, value_allocated, true)) { // store
        put(key, value_allocated);
    }
}

// store the key into the node at the given level
void concurrency_fastfair::concurrency_fastfair_insert_internal(char *left, uint64_t key, char *value,
                                        uint32_t level) {
    if (level > ((concurrency_page *) root)->hdr.level)
        return;

    concurrency_page *p = (concurrency_page *) this->root;

    while (p->hdr.level > level)
        p = (concurrency_page *) p->linear_search(key);

    if (!p->store(this, NULL, key, value, true)) {
        concurrency_fastfair_insert_internal(left, key, value, level);
    }
}

void concurrency_fastfair::concurrency_fastfair_delete(uint64_t key) {
    concurrency_page *p = (concurrency_page *) root;

    while (p->hdr.leftmost_ptr != NULL) {
        p = (concurrency_page *) p->linear_search(key);
    }

    concurrency_page *t;
    while ((t = (concurrency_page *) p->linear_search(key)) == p->hdr.sibling_ptr) {
        p = t;
        if (!p)
            break;
    }

    if (p) {
        if (!p->remove(this, key)) {
            concurrency_fastfair_delete(key);
        }
    } else {
        printf("not found the key to delete %lu\n", key);
    }
}

void concurrency_fastfair::concurrency_fastfair_delete_internal(uint64_t key, char *ptr, uint32_t level,
                                        uint64_t *deleted_key,
                                        bool *is_leftmost_node, concurrency_page **left_sibling) {
    if (level > ((concurrency_page *) this->root)->hdr.level)
        return;

    concurrency_page *p = (concurrency_page *) this->root;

    while (p->hdr.level > level) {
        p = (concurrency_page *) p->linear_search(key);
    }

    if ((char *) p->hdr.leftmost_ptr == ptr) {
        *is_leftmost_node = true;
        return;
    }

    *is_leftmost_node = false;

    for (int i = 0; p->records[i].ptr != NULL; ++i) {
        if (p->records[i].ptr == ptr) {
            if (i == 0) {
                if ((char *) p->hdr.leftmost_ptr != p->records[i].ptr) {
                    *deleted_key = p->records[i].key;
                    *left_sibling = p->hdr.leftmost_ptr;
                    p->remove(this, *deleted_key, false, false);
                    break;
                }
            } else {
                if (p->records[i - 1].ptr != p->records[i].ptr) {
                    *deleted_key = p->records[i].key;
                    *left_sibling = (concurrency_page *) p->records[i - 1].ptr;
                    p->remove(this, *deleted_key, false, false);
                    break;
                }
            }
        }
    }
}

void concurrency_fastfair::_scan(uint64_t min, uint64_t max,
                     vector<concurrency_ff_key_value> &buf) {
    concurrency_page *p = (concurrency_page *) root;

    while (p) {
        if (p->hdr.leftmost_ptr != NULL) {
            // The current concurrency_page is internal
            p = (concurrency_page *) p->linear_search(min);
        } else {
            // Found a leaf
            p->linear_search_range(min, max, buf);

            break;
        }
    }
}

// Function to search keys from "min" to "max"
vector<concurrency_ff_key_value> concurrency_fastfair::scan(uint64_t min, uint64_t max) {
    vector<concurrency_ff_key_value> res;

    _scan(min, max, res);
    return res;
}

void concurrency_fastfair::printAll() {
    int total_keys = 0;
    concurrency_page *leftmost = (concurrency_page *) root;
    printf("root: %x\n", root);
    if (root) {
        do {
            concurrency_page *sibling = leftmost;
            while (sibling) {
                if (sibling->hdr.level == 0) {
                    total_keys += sibling->hdr.last_index + 1;
                }
                sibling->print();
                sibling = sibling->hdr.sibling_ptr;
            }
            printf("-----------------------------------------\n");
            leftmost = leftmost->hdr.leftmost_ptr;
        } while (leftmost);
    }

    printf("total number of keys: %d\n", total_keys);
}