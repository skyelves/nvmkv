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
        if (&cur == &not_found) {
            head->insert(kvPair);
            return;
        } else {
            if (!cur.flag) {
                auto next = (linearHashing *) (cur.value_);
                next->insert(kvPair);
                return;
            } else {
                if(cur.key_ == kvPair.key_){
                    cur.value_ = kvPair.value_;
                    lhCflush((char *) (&cur.value_), sizeof(cur.value_));
                    return;
                }
                auto newLinearHashing = static_cast<linearHashing *>(fast_alloc(sizeof(linearHashing)));
                newLinearHashing->init(head->depth + 1);
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
        if (!cur.flag) {
            auto next = (linearHashing *) (cur.value_);
            ref_count ++;
            return next->get(key);
        } else {
            return cur;
        }
    }

    void rangeQuery(uint64_t start, uint64_t end, vector<KVPair>& res){
        rangeQuery(head,start,end,res);
    }

    void rangeQuery(linearHashing* head, uint64_t start, uint64_t end, vector<KVPair>& res){
        for(uint64_t i=0;i<head->bucketsNum;i++){
            auto node = head->buckets[i];
            while(node && node->records_num > 0){
                for(uint64_t j=0;j<node->records_num;j++){
                    if(node->records[j].flag == true){
                        if(node->records[j].key_ >= start && node->records[j].key_ <= end){
                            res.push_back(node->records[j]);
                        }
                    }else{
                        rangeQuery((linearHashing*)(node->records[j].value_), start, end, res);
                    }
                }
                node = node->overflow;
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
