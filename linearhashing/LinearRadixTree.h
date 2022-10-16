//
// Created by menghe on 10/15/22.
//
#ifndef NVMKV_LINEARRADIXTREE_H
#define NVMKV_LINEARRADIXTREE_H

#include "linearhashing.h"

#define NODE_LENGTH 32

#ifdef LHT_PROFILE
uint64_t overflow_cnt = 0;
#endif

using namespace std;

class LinearRadixTree {
public:
    LinearRadixTree() {
        init();
    }

    void init() {
        head = static_cast<linearHashing *>(fast_alloc(sizeof(linearHashing)));
        head->init();
    }

    void put(KVPair kvPair) {
        auto &cur = head->get(kvPair.key_);
        auto node = head;

        if (&cur == &not_found) {
            head->insert(kvPair);
            return;
        } else {
            if (!cur.flag) {
                auto next = (linearHashing *) (cur.value_);
                next->insert(kvPair);
                return;
            } else {
                auto newLinearHashing = static_cast<linearHashing *>(fast_alloc(sizeof(linearHashing)));
                newLinearHashing->init(node->depth + 1);
                lhCflush((char *) newLinearHashing, sizeof(linearHashing));

                newLinearHashing->insert(cur);
                newLinearHashing->insert(kvPair);

                cur.value_ = (uint64_t) newLinearHashing;
                cur.flag = false;
                lhCflush((char *) &cur, sizeof(cur));
                return;
            }
        }

    }

    KVPair& get(uint64_t key) {
        auto &cur = head->get(key);
        if (&cur == &not_found) {
            return cur;
        } else {
            if (!cur.flag) {
                auto next = (linearHashing *) (cur.value_);
                return next->get(key);
            } else {
                return cur;
            }
        }
    }

private:
    linearHashing *head;
};

LinearRadixTree *new_LinearRadixTree() {
    auto pointer = static_cast<LinearRadixTree *>(fast_alloc(sizeof(LinearRadixTree)));
    pointer->init();
    return pointer;
}

#endif //NVMKV_LINEARRADIXTREE_H
