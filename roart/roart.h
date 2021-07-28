//
// Created by 王柯 on 5/25/21.
//

#ifndef NVMKV_ROART_H
#define NVMKV_ROART_H

#include <set>
#include "roart_key.h"
#include "roart_node.h"

#define LEAF_ARRAY

#define PROFILE

#ifdef PROFILE
    extern uint64_t roart_visited_node;
#endif

class ROART {
public:
    N *root;

    bool checkKey(const ROART_KEY *ret, const ROART_KEY *k) const;

    enum class CheckPrefixResult : uint8_t { Match, NoMatch, OptimisticMatch };

    enum class CheckPrefixPessimisticResult : uint8_t {
        Match,
        NoMatch,
        SkippedLevel
    };

    enum class PCCompareResults : uint8_t {
        Smaller,
        Equal,
        Bigger,
        SkippedLevel
    };
    enum class PCEqualsResults : uint8_t {
        BothMatch,
        Contained,
        NoMatch,
        SkippedLevel
    };
    enum class OperationResults : uint8_t {
        Success,
        NotFound, // remove
        Existed,  // insert
        UnSuccess
    };
    static CheckPrefixResult checkPrefix(N *n, const ROART_KEY *k, uint32_t &level);

    static CheckPrefixPessimisticResult
    checkPrefixPessimistic(N *n, const ROART_KEY *k, uint32_t &level,
                           uint8_t &nonMatchingKey, Prefix &nonMatchingPrefix);

    static PCCompareResults checkPrefixCompare(const N *n, const ROART_KEY *k,
                                               uint32_t &level);

    static PCEqualsResults checkPrefixEquals(const N *n, uint32_t &level,
                                             const ROART_KEY *start, const ROART_KEY *end);

public:
    ROART();

    ROART(const ROART &) = delete;

    ~ROART();

    void rebuild(std::vector<std::pair<uint64_t, size_t>> &rs,
                 uint64_t start_addr, uint64_t end_addr, int thread_id);

    uint64_t get(uint64_t key);

    OperationResults update(const ROART_KEY *k) const;

    bool lookupRange(const ROART_KEY *start, const ROART_KEY *end, ROART_KEY *continueKey,
                     ROART_Leaf *result[], std::size_t resultLen,
                     std::size_t &resultCount) const;

    vector<ROART_KEY> scan(uint64_t min, uint64_t max);

    OperationResults put(uint64_t key, uint64_t value);

    OperationResults remove(const ROART_KEY *k);

    ROART_Leaf *allocLeaf(const ROART_KEY *k) const;

    double profile();

    void graphviz_debug();
} __attribute__((aligned(64)));

ROART *new_roart();

#ifdef ARTPMDK
void *allocate_size(size_t size);

#endif

#ifdef COUNT_ALLOC
double getalloctime();
#endif

#ifdef CHECK_COUNT
int get_count();
#endif

#endif //NVMKV_ROART_H
