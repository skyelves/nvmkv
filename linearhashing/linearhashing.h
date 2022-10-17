//
// Created by menghe on 10/12/22.
//
#ifndef NVMKV_LINEARHASHING_H
#define NVMKV_LINEARHASHING_H

#include "../fastalloc/fastalloc.h"

#define BLOCK_SIZE 16
#define BUFFER_SIZE 32

#define NODE_LENGTH 32

#define LHT_PROFILE 1

#ifdef LHT_PROFILE
extern uint64_t overflow_cnt;
#endif

#define GET_KEY(key, depth) ((key << (depth * NODE_LENGTH)) >> (64 - NODE_LENGTH))

inline void lhMfence() { asm volatile("mfence":: : "memory"); }

inline void lhCflush(char *data, int len) {
    volatile char *ptr = (char *) ((unsigned long) data & ~(CACHE_LINE_SIZE - 1));
    lhMfence();
    for (; ptr < data + len; ptr += CACHE_LINE_SIZE) {
        asm volatile("clflush %0" : "+m"(*(volatile char *) ptr));
    }
    lhMfence();
}

struct KVPair {
public:
    KVPair() = default;

    KVPair(uint64_t key, uint64_t value) : key_(key), value_(value) {}

    bool flag = true;
    uint64_t key_;
    uint64_t value_;
};

static uint64_t ref_count = 0;
static KVPair not_found(0, 0);

class Block {
public:
    KVPair *records;
    Block *overflow;
    uint64_t records_num;

    Block() {
        init();
    }

    void init() {
        overflow = nullptr;
        records = (KVPair *) (fast_alloc((BLOCK_SIZE) * sizeof(KVPair)));;
        records_num = 0;
    }

    bool isPresent(uint64_t x, int depth = 0) {
        Block *node = this;
        while (node) {
            for (uint64_t i = 0; i < records_num; i++) {
                if (GET_KEY(node->records[i].key_, depth) == x) {
                    return true;
                }
            }
            node = node->overflow;
        }
        return false;
    }

    KVPair &get(uint64_t x, int depth = 0) {
        Block *node = this;
        while (node) {
            for (uint64_t i = 0; i < node->records_num; i++) {
                if (GET_KEY((node->records[i].key_), depth) == x) {
                    return node->records[i];
                }
            }
#ifdef LHT_PROFILE
            ++overflow_cnt;
#endif
            node = node->overflow;
            ref_count++;
        }
        return not_found;
    }

    void add(KVPair x) {
        if (records_num < BLOCK_SIZE) {
            records[records_num] = x;
            lhCflush((char *) &records[records_num], sizeof(KVPair));
            ++records_num;
            lhCflush((char *) &records_num, sizeof(records_num));
        } else {
            if (overflow == nullptr) {
                overflow = static_cast<Block *>(fast_alloc(sizeof(Block)));
                overflow->init();
                lhCflush((char *) overflow, sizeof(Block *));
            }
            overflow->add(x);
        }
    }

    /**
     * get all KVPairs and put into the array.
     * apply for new array if the size of array is not enough
     *
     * @param array
     * @param size
     * @param pos
     */
    void cleanAll(KVPair **array, uint64_t &size, uint64_t &pos) {
        Block *node = this;
        while (node) {
            for (uint64_t i = 0; i < node->records_num; i++) {
                if (pos == size) {
                    auto newArray = static_cast<KVPair *>(fast_alloc(sizeof(KVPair) * size * 2));
                    memcpy(newArray, *array, sizeof(KVPair) * size);
                    *array = newArray;
                    size *= 2;
                }
                (*array)[pos] = node->records[i];
                pos++;
            }
            node->records_num = 0;
            node = node->overflow;
        }
    }

};

class linearHashing {
public:
    uint64_t numRecords, numBits;
    Block **buckets = nullptr;
    uint64_t bucketsNum = 0;
    KVPair *newArray;
    uint64_t arraySize;
    uint64_t capacity;


    linearHashing() {
        init();
    }

    void init(int depth_ = 0) {
        buckets = static_cast<Block **>(fast_alloc(sizeof(Block *) * 2));
        buckets[0] = static_cast<Block *>(fast_alloc(sizeof(Block)));
        buckets[0]->init();
        buckets[1] = static_cast<Block *>(fast_alloc(sizeof(Block)));
        buckets[1]->init();
        bucketsNum = 2;
        numRecords = 0;
        capacity = 2;
        numBits = 1;
        newArray = static_cast<KVPair *>(fast_alloc(sizeof(KVPair) * 1024));
        arraySize = 1024;
        depth = depth_;
    }

    unsigned int hash(uint64_t key) {
        unsigned int mod = (1 << numBits);
        return (unsigned int) (key % mod + mod) % mod;
    }

    int occupancy() {
        double ratio = 1.0 * numRecords / (bucketsNum * BLOCK_SIZE);
        return (int) (100 * ratio);}

    bool isPresent(uint64_t key) {
        unsigned int k = hash(GET_KEY(key, depth));
        if (k >= bucketsNum) {
            k -= (1 << (numBits - 1));
        }
        if (buckets[k]->isPresent(key, depth)) {
            return true;
        }
        return false;
    }

    void insert(KVPair kvPair) {
        unsigned int k = hash(GET_KEY((kvPair.key_), depth));
        if (k >= bucketsNum) {
            k -= (1 << (numBits - 1));
        }
        auto& buc = buckets[k]->get(kvPair.key_,depth);
        if(&buc == &not_found){
            buckets[k]->add(kvPair);
            numRecords++;
        }else{
            buc.value_ = kvPair.value_;
            lhCflush((char*)&buc.value_,sizeof(buc.value_));
        }
        while (occupancy() > 75) {
            if (bucketsNum >= capacity) {
                auto newBuckets = static_cast<Block **>(fast_alloc(sizeof(Block *) * (capacity * 2)));
                memcpy(newBuckets, buckets, sizeof(Block *) * bucketsNum);
                lhCflush((char *) newBuckets, sizeof(Block *) * capacity);
                buckets = newBuckets;
                lhCflush((char *) (&buckets), sizeof(buckets));
                capacity *= 2;
                lhCflush((char *) (&capacity), sizeof(capacity));
            }


            buckets[bucketsNum] = static_cast<Block *>(fast_alloc(sizeof(Block)));
            buckets[bucketsNum]->init();
            lhCflush((char *) (buckets[bucketsNum]), sizeof(Block));
            bucketsNum++;
            lhCflush((char *) (&bucketsNum), sizeof(bucketsNum));

            numBits = ceil(log2((double) bucketsNum));
            k = bucketsNum - 1 - (1 << (numBits - 1));

            uint64_t pos = 0;
            buckets[k]->cleanAll(&newArray, arraySize, pos);

            for (unsigned int i = 0; i < pos; i++) {
                auto k = hash(GET_KEY((newArray[i].key_), depth));
                buckets[k]->add((newArray)[i]);
            }
        }
    }

    KVPair &get(uint64_t key) {
        key = GET_KEY(key, depth);
        unsigned int k = hash(key);
        if (k >= bucketsNum) {
            k -= (1 << (numBits - 1));
        }
        return buckets[k]->get(key, depth);
    }

    int depth;
};

static linearHashing *new_linearhash() {
    auto pointer = static_cast<linearHashing *>(fast_alloc(sizeof(linearHashing)));
    pointer->init();
    return pointer;
}


#endif //NVMKV_LINEARHASHING_H
