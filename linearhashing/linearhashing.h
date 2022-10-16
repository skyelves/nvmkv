//
// Created by menghe on 10/12/22.
//
#ifndef NVMKV_LINEARHASHING_H
#define NVMKV_LINEARHASHING_H

#include "../fastalloc/fastalloc.h"

#define BLOCK_SIZE 16
#define BUFFER_SIZE 32


inline void lhMfence() { asm volatile("mfence":: : "memory"); }

inline void lhCflush(char *data, int len) {
    volatile char *ptr = (char *) ((unsigned long) data & ~(CACHE_LINE_SIZE - 1));
    lhMfence();
    for (; ptr < data + len; ptr += CACHE_LINE_SIZE) {
        asm volatile("clflush %0" : "+m"(*(volatile char *) ptr));
        //++clflush_cnt;
    }
    lhMfence();
}

struct KVPair {
public:
    KVPair() = default;

    KVPair(uint64_t key, uint64_t value) : key_(key), value_(value) {

    }

    uint64_t key_;
    uint64_t value_;
};

class Block {
    KVPair *records;
    Block *overflow;
    uint64_t records_num;

public:
    Block() {
        init();
    }

    void init() {
        overflow = nullptr;
        records = (KVPair *) (fast_alloc((BLOCK_SIZE) * sizeof(KVPair)));;
        records_num = 0;
    }

    bool isPresent(uint64_t x) {
        Block *node = this;
        while (node) {
            for (uint64_t i = 0; i < records_num; i++) {
                if (node->records[i].key_ == x) {
                    return true;
                }
            }
            node = node->overflow;
        }
        return false;
    }

    KVPair get(uint64_t x) {
        Block *node = this;
        while (node) {
            for (uint64_t i = 0; i < node->records_num; i++) {
                if (node->records[i].key_ == x) {
                    return node->records[i];
                }
            }
            node = node->overflow;
        }
        return KVPair(0, 0);
    }

    void add(KVPair x) {
        if (records_num < BLOCK_SIZE) {
            records[records_num] = x;
            records_num++;
            lhCflush((char *) &records[records_num - 1], sizeof(KVPair));
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
        records_num = 0;
    }

};

class linearHashing {
private:
    uint64_t numRecords, numBits;
    Block **buckets = nullptr;
    uint64_t bucketsNum = 0;
    KVPair *newArray;
    uint64_t arraySize;
    uint64_t capacity;

public:
    linearHashing() {
        init();
    }

    void init() {
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
    }

    unsigned int hash(uint64_t key) {
        unsigned int mod = (1 << numBits);
        return (unsigned int) (key % mod + mod) % mod;
    }

    // todo: ratio = 1.0 * numRecords / (bucketsNum * BLOCK_SIZE)?;
    // 2 bucket, 16 kvs/bucket => 32 kvs
    // 16 vs
    int occupancy() {
        double ratio = 1.0 * numRecords / (bucketsNum * BLOCK_SIZE);
        return (int) (100 * ratio);
//        return (int) (100 * (ratio / (BUFFER_SIZE / 4)));
    }

    bool isPresent(uint64_t key) {
        unsigned int k = hash(key);
        if (k >= bucketsNum) {
            k -= (1 << (numBits - 1));
        }
        if (buckets[k]->isPresent(key)) {
            return true;
        }
        return false;
    }

    void insert(KVPair kvPair) {
        unsigned int k = hash(kvPair.key_);
        if (k >= bucketsNum) {
            k -= (1 << (numBits - 1));
        }
        buckets[k]->add(kvPair);
        numRecords++;
        while (occupancy() >= 75) {
            if(bucketsNum >= capacity){
                capacity *= 2;
                auto newBuckets = static_cast<Block **>(fast_alloc(sizeof(Block *) * (capacity)));
                memcpy(newBuckets, buckets, sizeof(Block *) * bucketsNum);
                lhCflush((char *) newBuckets, sizeof(Block *) * capacity);
                buckets = newBuckets;
                lhCflush((char *) (&buckets), sizeof(buckets));
            }


            buckets[bucketsNum] = static_cast<Block *>(fast_alloc(sizeof(Block)));
            buckets[bucketsNum]->init();
            lhCflush((char *) (buckets[bucketsNum]), sizeof(Block));
            lhCflush((char *) (&(buckets[bucketsNum])), sizeof(Block*));
            bucketsNum++;

            numBits = ceil(log2((double) bucketsNum));
            k = bucketsNum - 1 - (1 << (numBits - 1));

            uint64_t pos = 0;
            buckets[k]->cleanAll(&newArray, arraySize, pos);

            for (unsigned int i = 0; i < pos; i++) {
                auto k = hash((newArray)[i].key_);
                buckets[k]->add((newArray)[i]);
            }
        }
    }

    KVPair get(uint64_t key) {
        unsigned int k = hash(key);
        if (k >= bucketsNum) {
            k -= (1 << (numBits - 1));
        }
        return buckets[k]->get(key);
    }
};

static linearHashing *new_linearhash() {
    auto pointer = static_cast<linearHashing *>(fast_alloc(sizeof(linearHashing)));
    pointer->init();
    return pointer;
}


#endif //NVMKV_LINEARHASHING_H
