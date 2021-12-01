#include <atomic>
#include <iostream>
#include <unistd.h>
#include <stdio.h>
#include <map>
#include <unordered_map>
#include <vector>
#include <sys/time.h>
#include <pthread.h>
#include "blink/blink_tree.h"
#include "mass/mass_tree.h"
#include "hashtree/hashtree.h"
#include "rng/rng.h"
#include "art/art.h"
#include "wort/wort.h"
#include "woart/woart.h"
#include "cceh/cacheline_concious_extendible_hash.h"
#include "cceh/concurrency_cceh.h"
#include "fastfair/fastfair.h"
#include "fastfair/concurrency_fastfair.h"
#include "roart/roart.h"
#include "concurrencyhashtree/concurrency_hashtree.h"
#include "woart/conWoart.h"
#include "varLengthHashTree/varLengthHashTree.h"
#include "fastfair/varlengthfastfair.h"
#include "woart/varLengthWoart.h"
#include "wort/varLengthWort.h"
#include "lbtree/lbtree.h"


using namespace std;

ofstream out;

#define CON_Time_BODY(name, func)                                                          \
 {                                                                                          \
    sleep(1);                                                                               \
    timeval start, ends;                                                                    \
    gettimeofday(&start, NULL);                                                             \
    for(int j = 0; j < i; j++){                                                             \
        auto fun = []( int const & j)  {                                                             \
            init_fast_allocator(true);                                                      \
                for (int k = j * (testNum / numThread); k < (j + 1) * (testNum / numThread); ++k) { \
                    func                                                                        \
                }                                                                           \
        };                                                                                    \
        threads[j]  = new std::thread(fun, move(j));                                                 \
    }                                                                                       \
    for (int j = 0; j < i; j++) {                                                           \
        threads[j]->join();                                                                 \
    }                                                                                           \
                                                                                            \
    gettimeofday(&ends, NULL);                                                              \
    double timeCost = (ends.tv_sec - start.tv_sec) * 1000000 + ends.tv_usec - start.tv_usec;\
    double throughPut = (double) nLoadOp / timeCost;                                        \
    cout << name << throughPut <<  endl;                        \
    }

#define Time_BODY(condition, name, func)                                                        \
    if(condition) {                                                                             \
        sleep(1);                                                                               \
        timeval start, ends;                                                                    \
        gettimeofday(&start, NULL);                                                             \
        for (int i = 0; i < testNum; ++i) {                                                     \
            func                                                                                \
        }                                                                                       \
        gettimeofday(&ends, NULL);                                                              \
        double timeCost = (ends.tv_sec - start.tv_sec) * 1000000 + ends.tv_usec - start.tv_usec;\
        double throughPut = (double) testNum / timeCost;                                        \
        cout << name << testNum << " kv pais in " << timeCost / 1000000 << " s" << endl;        \
        cout << name << "ThroughPut: " << throughPut << " Mops" << endl;                        \
    }

#define Scan_Time_BODY(condition, name, func)                                                        \
    if(condition) {                                                                             \
        sleep(1);                                                                               \
        timeval start, ends;                                                                    \
        gettimeofday(&start, NULL);                                                             \
        for (int i = 0; i < 500; i++) {                                                     \
            func                                                                                \
        }                                                                                       \
        gettimeofday(&ends, NULL);                                                              \
        double timeCost = (ends.tv_sec - start.tv_sec) * 1000000 + ends.tv_usec - start.tv_usec;\
        double throughPut = (double) 500 / timeCost;                                        \
        cout << name << testNum << " kv pais in " << timeCost / 1000000 << " s" << endl;        \
        cout << name << "ThroughPut: " << throughPut << " Mops" << endl;                        \
    }

#define CONCURRENCY_Time_BODY(name, func){                                                        \
        sleep(1);                                                                               \
        timeval _start, _ends;                                                                    \
        gettimeofday(&_start, NULL);                                                             \
        func                                                                                    \
        gettimeofday(&_ends, NULL);                                                              \
        double timeCost = (_ends.tv_sec - _start.tv_sec) * 1000000 + _ends.tv_usec - _start.tv_usec;\
        double throughPut = (double) testNum / timeCost;                                        \
        cout << name << testNum << " kv pais in " << timeCost / 1000000 << " s" << endl;        \
        cout << name << "ThroughPut: " << throughPut << " Mops" << endl;                        \
}                        \


#define MEMORY_BODY_EXPE(condition, name, func)                                                        \
    if(condition) {                                                                             \
        sleep(1);                                                                               \
        timeval start, ends;                                                                    \
        cout<<name<<", ";\
        uint64_t memory=0;                                                             \
        int interval = testNum/10;                                                              \
        for (int i = 0; i < testNum; ++i) {                                                     \
            func                                                                                \
            if (i%interval==interval-1){\
                memory=concurrency_fastalloc_profile();                                                               \
                cout << memory << ", ";\
            }\
        }\
        cout<<endl;\
    }

#define KEYS_Time_BODY(condition, name, func)                                                        \
    if(condition) {                                                                             \
        sleep(1);                                                                               \
        timeval start, ends;                                                                    \
        cout<<name<<", ";\
        int interval = testNum/10;                                                              \
        double timeCost, throughPut; \
        gettimeofday(&start, NULL);                                                             \
        for (int i = 0; i < testNum; ++i) {                                                     \
            func                                                                                \
            if (i%interval==interval-1){\
                gettimeofday(&ends, NULL);                                                              \
        timeCost = (ends.tv_sec - start.tv_sec) * 1000000 + ends.tv_usec - start.tv_usec;\
        throughPut = (double) interval / timeCost;                                        \
        cout << throughPut << ", ";                        \
        gettimeofday(&start, NULL);                                                             \
            }\
        }                                                                                       \
        cout<<endl;\
    }


#define MULTITHREAD true

int testNum = 100000;

int numThread = 1;

int test_algorithms_num = 10;
bool test_case[10] = {0, // ht
                      0, // art
                      1, // wort
                      1, // woart
                      0, // cacheline_concious_extendible_hash
                      1, // fast&fair
                      0, // roart
                      1, // ert
                      1, // lb+tree
                      0};

bool range_query_test_case[10] = {
        0, // ht
        1, // wort
        1, // woart
        1, // fast&fair
        0, // roart
        1, // ert
        1, // lb+tree
        0
};

blink_tree *bt;
mass_tree *mt;
hashtree *ht;
art_tree *art;
wort_tree *wort;
woart_tree *woart;
cacheline_concious_extendible_hash *cceh;
fastfair *ff;
ROART *roart;
VarLengthHashTree *vlht;
Length64HashTree *l64ht;
varlength_fastfair *vlff;
var_length_woart_tree *vlwt;
var_length_wort_tree *vlwot;
lbtree *lbt;

conwoart_tree *conwoart;
concurrencyhashtree *cht;
concurrency_cceh *con_cceh;
concurrency_fastfair *con_fastfair;


uint64_t nLoadOp, nRunOp;
thread **threads;

uint64_t *mykey;

std::mutex mtx;

enum YCSBRunOp {
    Get,
    Scan,
    Update,
};

std::vector<uint64_t> allRunKeys;
std::vector<uint64_t> allRunValues;
std::vector<uint64_t> allLoadKeys;
std::vector<uint64_t> allLoadValues;

std::vector<YCSBRunOp> allRunOp;
std::unordered_map<uint64_t, uint64_t> *oracleMap;

int value = 1;

void parseLine(std::string rawStr, uint64_t &hashKey, uint64_t &hashValue, uint32_t &rangeNum, bool isScan) {
    uint32_t t = rawStr.find(" ");
    uint32_t t1 = rawStr.find(" ", t + 1);
    std::string tableName = rawStr.substr(t, t1 - t);
    uint32_t t2 = rawStr.find(" ", t1 + 1);
    uint32_t t3 = rawStr.find(" ", t2 + 1);
    std::string keyName = rawStr.substr(t1, t2 - t1);
    // std::string fullKey = tableName + keyName; // key option1: table+key concat
    // std::string fullKey = keyName; // key option2: only key
    std::string fullKey = keyName.substr(5); // key option3: remove 'user' prefix, out of range
    if (fullKey.size() > 19) // key option4: key's tail, in range uint64_t
        fullKey = fullKey.substr(fullKey.size() - 19); // tail
    std::string fullValue = rawStr.substr(t2);
    std::string rangeNumStr;
    if (isScan) {
        rangeNumStr = rawStr.substr(t2, t3 - t2);
        fullValue = rawStr.substr(t3);
    }
    static std::hash<std::string> hash_str;
    // info("%s", fullKey.c_str());
    // hashKey = hash_str(fullKey); // use hash, or
    hashKey = std::stoll(fullKey); // just convert to int
    hashValue = hash_str(fullValue);
    rangeNum = 0;
    if (isScan) {
        rangeNum = std::stoi(rangeNumStr);
    }
    // info("key:%s, hashed_key: %lx, hashed_value: %lx, rangeNum: %d", fullKey.c_str(), hashKey, hashValue, rangeNum);
    // info("hashed_key: %lx, hashed_value: %lx", hashKey, hashValue);
}

void parseYCSBRunFile(std::string wlName, bool correctCheck = false) {
    std::string rawStr;
    uint64_t hashKey, hashValue;

    std::ifstream runFile(wlName + ".run");
    assert(runFile.is_open());
    uint64_t opCnt = 0;
    uint32_t rangeNum = 0;
    while (std::getline(runFile, rawStr)) {
        assert(rawStr.size() > 4);
        std::string opStr = rawStr.substr(0, 4);
        if (opStr == "READ") {
            // info("%s", rawStr.c_str());
            parseLine(rawStr, hashKey, hashValue, rangeNum, false);
            allRunKeys.push_back(hashKey); // buffer this query
            allRunValues.push_back(0);
            allRunOp.push_back(YCSBRunOp::Get);
            opCnt++;
        } else if (opStr == "INSE" || opStr == "UPDA") {
            // info("%s", rawStr.c_str());
            parseLine(rawStr, hashKey, hashValue, rangeNum, false);
            allRunKeys.push_back(hashKey);
            allRunValues.push_back(hashValue);
            allRunOp.push_back(YCSBRunOp::Update);
            if (correctCheck) (*oracleMap)[hashKey] = hashValue;
            opCnt++;
        } else if (opStr == "SCAN") {
            parseLine(rawStr, hashKey, hashValue, rangeNum, true);
            allRunKeys.push_back(hashKey);
            // allRunValues.push_back(1024);
            allRunValues.push_back(rangeNum);
            allRunOp.push_back(YCSBRunOp::Scan);
            opCnt++;
        } else {
            // other info
        }
    }
    runFile.close();
    nRunOp = opCnt;
    // info("Finish parse run file");
    cout << "run" << endl;
    if (correctCheck) {
        for (uint64_t hashKey : allRunKeys) {
            assert(oracleMap->count(hashKey));
            // info("hash: %lx", hashKey);
            uint64_t htValue = ht->get(hashKey);
            assert(htValue);
            uint64_t mapValue = oracleMap->at(hashKey);
            if (htValue != mapValue) {
                // info("Incorrect key %lx v1 %lx v2 %lx", hashKey, htValue, mapValue);
                assert(false);
            }
        }
        // info("Pass correctness check!");
    }
}

void parseYCSBLoadFile(std::string wlName, bool correctCheck) {
    std::string rawStr;
    uint64_t opCnt = 0;
    uint32_t rangeNum = 0;
    uint64_t hashKey, hashValue;
    std::ifstream loadFile(wlName + ".load");
    assert(loadFile.is_open());
    cout << "ok" << endl;
    while (std::getline(loadFile, rawStr)) {
        assert(rawStr.size() > 4);
        std::string opStr = rawStr.substr(0, 4);
        if (opStr == "INSE" || opStr == "UPDA") {
            // info("%s", rawStr.c_str());
            parseLine(rawStr, hashKey, hashValue, rangeNum, false);
            allLoadKeys.push_back(hashKey);
            allLoadValues.push_back(hashValue);
            if (correctCheck) (*oracleMap)[hashKey] = hashValue;
            opCnt++;
        } else {
            // other info
        }
    }
    loadFile.close();
    // info("Finish parse load file");
    cout << "loaded" << endl;
    nLoadOp = allLoadKeys.size();
}

void *putFunc(void *arg) {
    rng r;
#ifdef __linux__
    uint64_t tid = static_cast<uint64_t>(pthread_self()) % 10000;
#else
    uint64_t tid;
    pthread_threadid_np(0, &tid);
    tid = tid % 10000;
#endif
    rng_init(&r, tid, tid + numThread);
    for (int i = 0; i < testNum / numThread; ++i) {
//        char *key = new char[9];
//        key[0] = 8;
//        *(uint64_t *) (key + 1) = rand();
//        concurrent_adaptive_radix_tree_put(cart, (const void *) (key + 1), 8, value, VALUE_LEN);
        uint64_t x = rng_next(&r);
//        uint64_t x = i;
//        ht->put(x, x);
//        blink_tree_write(mytree, &x, sizeof(int), &x);
//        mass_tree_put(mt, &x, 8, value, strlen(value));
//        int *res = static_cast<int *>(mass_tree_get(mt, &x, 8));
//        cout << x << ": " << *(res+1) << endl;
    }
}

void speedTest() {
    mykey = new uint64_t[testNum];
    rng r;
    rng_init(&r, 1, 2);
    for (int i = 0; i < testNum; ++i) {
//        mykey[i] = rng_next(&r);
//        mykey[i] = rng_next(&r) & 0xffffffff00000000;
//        mykey[i] = rng_next(&r) % testNum;
//        mykey[i] = rng_next(&r) % 100000000;
        mykey[i] = i;
    }
    uint64_t value = 1;
    vector<Length64HashTreeKeyValue> res;
//    timeval start, ends;
//    gettimeofday(&start, NULL);
//    put();
//    pthread_t *tids = new pthread_t[numThread];
//    for (int i = 0; i < numThread; ++i) {
//        int ret = pthread_create(&tids[i], NULL, &putFunc, NULL);
//        if (ret != 0) {
//            cout << "pthread_create error: error_code=" << ret << endl;
//        }
//    }
//    for (int j = 0; j < numThread; ++j) {
//        pthread_join(tids[j], NULL);
//    }
//    gettimeofday(&ends, NULL);
//    double timeCost = (ends.tv_sec - start.tv_sec) * 1000000 + ends.tv_usec - start.tv_usec;
//    double throughPut = (double) testNum / timeCost;
//    cout << "hash tree Put " << testNum << " kv pais in " << timeCost / 1000000 << " s" << endl;
//    cout << "hash tree Put ThroughPut: " << throughPut << " Mops" << endl;
//    cout << throughPut << endl;

    // insert speed for ht
    Time_BODY(test_case[0], "hash tree put ", { ht->crash_consistent_put(NULL, mykey[i], 1, 0); })

    // insert speed for art
    Time_BODY(test_case[1], "art tree put ", { art_put(art, (unsigned char *) &(mykey[i]), 8, &value); })

    // insert speed for wort
    Time_BODY(test_case[2], "wort put ", { wort_put(wort, mykey[i], 8, &value); })

    // insert speed for woart
    Time_BODY(test_case[3], "woart put ", { woart_put(woart, mykey[i], 8, &value); })

    // insert speed for cceh
    Time_BODY(test_case[4], "cceh put ", { cceh->put(mykey[i], value); })

    // insert speed for fast&fair
    Time_BODY(test_case[5], "fast&fair put ", { ff->put(mykey[i], (char *) &value); })

    // insert speed for fast&fair
    Time_BODY(test_case[6], "roart put ", { roart->put(mykey[i], value); })

    // insert speed for ert
    Time_BODY(test_case[7], "ert put  ", { l64ht->crash_consistent_put(NULL, mykey[i], 1); })

    // insert speed for lbt
    Time_BODY(test_case[8], "lbtree put  ", { lbt->insert(mykey[i], &value); })

    // query speed for ht
    Time_BODY(test_case[0], "hash tree get ", { ht->get(mykey[i]); })

    // query speed for art
    Time_BODY(test_case[1], "art tree get ", { art_get(art, (unsigned char *) &(mykey[i]), 8); })

    // query speed for wort
    Time_BODY(test_case[2], "wort get ", { wort_get(wort, mykey[i], 8); })

    // query speed for woart
    Time_BODY(test_case[3], "woart get ", { woart_get(woart, mykey[i], 8); })

    // query speed for cceh
    Time_BODY(test_case[4], "cceh get ", { cceh->get(mykey[i]); })

    // query speed for fast&fair
    Time_BODY(test_case[5], "fast&fair get ", { ff->get(mykey[i]); })

    // query speed for fast&fair
    Time_BODY(test_case[6], "roart get ", { roart->get(mykey[i]); })

    Time_BODY(test_case[7], "ert get ", { l64ht->get(mykey[i]); })

    int pos;
    Time_BODY(test_case[8], "lbt get ", { lbt->lookup(mykey[i], &pos); })

    //range query speed for ht
    Scan_Time_BODY(range_query_test_case[0], "hash tree range query ",
                   { ht->scan(mykey[i], mykey[i] + testNum * 0.001); })

    //range query speed for wort
    Scan_Time_BODY(range_query_test_case[1], "wort range query ",
                   { wort_scan(wort, mykey[i], mykey[i] + testNum * 0.001); })

    //range query speed for woart
    Scan_Time_BODY(range_query_test_case[2], "woart tree range query ",
                   { woart_scan(woart, mykey[i], mykey[i] + testNum * 0.001); })

    //range query speed for fast&fair
    Scan_Time_BODY(range_query_test_case[3], "fast&fair range query ",
                   { ff->scan(mykey[i], mykey[i] + testNum * 0.001); })

    //range query speed for roart
    Scan_Time_BODY(range_query_test_case[4], "roart tree range query ",
                   { roart->scan(mykey[i], mykey[i] + testNum * 0.001); })

    //range query speed for ert
    Scan_Time_BODY(range_query_test_case[5], "ert range query ",
                   { l64ht->scan(mykey[i], mykey[i] + testNum * 0.001); })

    //range query speed for lb+tree
    Scan_Time_BODY(range_query_test_case[6], "lb+tree range query ",
                   { lbt->rangeQuery(mykey[i], mykey[i] + testNum * 0.001); })

    //range query speed for ht
    Scan_Time_BODY(range_query_test_case[0], "hash tree range query ",
                   { ht->scan(mykey[i], mykey[i] + testNum * 0.005); })

    //range query speed for wort
    Scan_Time_BODY(range_query_test_case[1], "wort range query ",
                   { wort_scan(wort, mykey[i], mykey[i] + testNum * 0.005); })

    //range query speed for woart
    Scan_Time_BODY(range_query_test_case[2], "woart tree range query ",
                   { woart_scan(woart, mykey[i], mykey[i] + testNum * 0.005); })

    //range query speed for fast&fair
    Scan_Time_BODY(range_query_test_case[3], "fast&fair range query ",
                   { ff->scan(mykey[i], mykey[i] + testNum * 0.005); })

    //range query speed for roart
    Scan_Time_BODY(range_query_test_case[4], "roart tree range query ",
                   { roart->scan(mykey[i], mykey[i] + testNum * 0.005); })

    //range query speed for ert
    Scan_Time_BODY(range_query_test_case[5], "ert range query ",
                   { l64ht->scan(mykey[i], mykey[i] + testNum * 0.005); })

    //range query speed for lb+tree
    Scan_Time_BODY(range_query_test_case[6], "lb+tree range query ",
                   { lbt->rangeQuery(mykey[i], mykey[i] + testNum * 0.005); })

    //range query speed for ht
    Scan_Time_BODY(range_query_test_case[0], "hash tree range query ",
                   { ht->scan(mykey[i], mykey[i] + testNum * 0.01); })

    //range query speed for wort
    Scan_Time_BODY(range_query_test_case[1], "wort range query ",
                   { wort_scan(wort, mykey[i], mykey[i] + testNum * 0.01); })

    //range query speed for woart
    Scan_Time_BODY(range_query_test_case[2], "woart tree range query ",
                   { woart_scan(woart, mykey[i], mykey[i] + testNum * 0.01); })

    //range query speed for fast&fair
    Scan_Time_BODY(range_query_test_case[3], "fast&fair range query ",
                   { ff->scan(mykey[i], mykey[i] + testNum * 0.01); })

    //range query speed for roart
    Scan_Time_BODY(range_query_test_case[4], "roart tree range query ",
                   { roart->scan(mykey[i], mykey[i] + testNum * 0.01); })

    //range query speed for ert
    Scan_Time_BODY(range_query_test_case[5], "ert range query ",
                   { l64ht->scan(mykey[i], mykey[i] + testNum * 0.01); })

    //range query speed for lb+tree
    Scan_Time_BODY(range_query_test_case[6], "lb+tree range query ",
                   { lbt->rangeQuery(mykey[i], mykey[i] + testNum * 0.01); })

    //range query speed for ht
    Scan_Time_BODY(range_query_test_case[0], "hash tree range query ", { ht->scan(mykey[i], mykey[i] + 31); })

    //range query speed for wort
    Scan_Time_BODY(range_query_test_case[1], "wort range query ", { wort_scan(wort, mykey[i], mykey[i] + 31); })

    //range query speed for woart
    Scan_Time_BODY(range_query_test_case[2], "woart tree range query ",
                   { woart_scan(woart, mykey[i], mykey[i] + 31); })

    //range query speed for fast&fair
    Scan_Time_BODY(range_query_test_case[3], "fast&fair range query ", { ff->scan(mykey[i], mykey[i] + 31); })

    //range query speed for roart
    Scan_Time_BODY(range_query_test_case[4], "roart tree range query ", { roart->scan(mykey[i], mykey[i] + 31); })

    //range query speed for ert
    Scan_Time_BODY(range_query_test_case[5], "ert range query ",
                   { l64ht->scan(mykey[i], mykey[i] + 31); })

    //range query speed for lb+tree
    Scan_Time_BODY(range_query_test_case[6], "lb+tree range query ",
                   { lbt->rangeQuery(mykey[i], mykey[i] + 31); })

//    cout << concurrency_fastalloc_profile()<<endl;
    out.close();
}

void correctnessTest() {
    map<uint64_t, uint64_t> mm;
    mykey = new uint64_t[testNum];
    rng r;
    rng_init(&r, 1, 2);

    uint64_t value = 1;
    for (int i = 0; i < testNum; ++i) {
        mykey[i] = rng_next(&r);
//        mykey[i] = i + 1;
        mm[mykey[i]] = i + 1;
//        roart->put(mykey[i], i + 1);
//        ff->put(mykey[i], (char *) &mm[mykey[i]]);
//        wort_put(wort, mykey[i], 8, &mm[mykey[i]]);
//        cceh->put(mykey[i], i + 1);
        ht->crash_consistent_put(NULL, mykey[i], i + 1, 0);
//        for (int j = 0; j < testNum; ++j) {
//            int64_t res = cceh->get(mykey[j]);
//            if (res != mm[mykey[j]]) {
//                cout << i << endl;
//                cout << j << ", " << mykey[j] << ", " << res << ", " << mm[mykey[j]] << endl;
//                return;
//            }
//        }
    }

    int64_t res = 0;
//    for (int i = 0; i < testNum; ++i) {
//        mm[mykey[i]] = i + 2;
//        ht->update(mykey[i], i + 2);
//        res = ht->del(mykey[i]);
//        if (res != mm[mykey[i]]) {
//            cout << i << ", " << mykey[i] << ", " << res << ", " << mm[mykey[i]] << endl;
//            return;
//        }
//    }

    for (int i = 0; i < testNum; ++i) {
//        res = roart->get(mykey[i]);
//        res = *(int64_t *) ff->get(mykey[i]);
//        res = *(int64_t *) wort_get(wort, mykey[i], 8);
        res = ht->get(mykey[i]);
        if (res != mm[mykey[i]]) {
            cout << i << ", " << mykey[i] << ", " << res << ", " << mm[mykey[i]] << endl;
//            return;
        }
    }
//    cout << 1 << endl;
    return;
}

void profile() {
    out.open("/Users/wangke/Desktop/ht_profile.csv");
    mykey = new uint64_t[testNum];
    rng r;
    rng_init(&r, 1, 2);
    for (int i = 0; i < testNum; ++i) {
        mykey[i] = rng_next(&r);
    }
    uint64_t value = 1;
    timeval start, ends;
    gettimeofday(&start, NULL);
    for (int i = 0; i < testNum; ++i) {
//        cceh->put(mykey[i], value);
        ht->crash_consistent_put(NULL, mykey[i], i + 1, 0);
        if (i % 10000 == 0) {
//            out << i << ", " << cceh->dir_size << ", "
//                << (double) i / (cceh_seg_num * CCEH_BUCKET_SIZE * CCEH_MAX_BUCKET_NUM) << endl;
//            out << i << ", " << ht_dir_num << ", " << ht_seg_num << ", "
//                << (double) i / (ht_seg_num * HT_MAX_BUCKET_NUM * HT_BUCKET_SIZE) << endl;
        }
    }
    gettimeofday(&ends, NULL);
    double timeCost = (ends.tv_sec - start.tv_sec) * 1000000 + ends.tv_usec - start.tv_usec;
//    out << t1 << endl << t2 << endl << t3 << endl;
//    out << timeCost << endl;
    out.close();
}

void range_query_correctness_test() {
    testNum = 1000000;
    mykey = new uint64_t[testNum];
    rng r;
    rng_init(&r, 1, 2);
    uint64_t mmax = 0;
    uint64_t mmin = INT64_MAX;
    for (int i = 0; i < testNum; ++i) {
        mykey[i] = rng_next(&r);
        if (mykey[i] > mmax) {
            mmax = mykey[i];
        }
        if (mykey[i] < mmin) {
            mmin = mykey[i];
        }
    }
    // vector<concurrency_ff_key_value> res;
//    vector<ht_key_value> res;
//    vector<woart_key_value> res;
//    vector<wort_key_value> res;

//    vector<Length64HashTreeKeyValue> res;

    vector<void *> res;

//    uint64_t value = 1;
//    for (int i = 0; i < testNum; i += 1) {
//        l64ht->crash_consistent_put(NULL, mykey[i], 1);
//        // roart->put(i + 1, i + 1);
////        ff->put(i + 1, (char *) &value);
////        woart_put(woart, i + 1, 8, &value);
////        wort_put(wort, i + 1, 8, &value);
////        ht->crash_consistent_put(NULL, i+1, 1, 0);
//        if (l64ht->get(mykey[i]) != 1) {
//            cout << "wrong" << endl;
//        }
//    }


//    for (int i = 0; i < testNum; ++i) {
////        res = ht->scan(mykey[i], mykey[i] + 10000);
//        res = wort_scan(wort, mykey[i], mykey[i] + 10000);
//        cout << res.size() << endl;
//    }
//    res = wort_scan(wort, 1, 10000);
//    res =  ht->scan(1, 10000);
//    res = woart_scan(woart, 1, 10000);
//    res = ff->scan(1, 10000);
//
//    for (int i = 0; i < res.size(); ++i) {
//        cout << res[i].key << ", " << res[i].value << endl;
//    }
//    cout << res.size() << endl;
    // roart->scan(1, 10000);

//    timeval start, ends;
//    gettimeofday(&start, NULL);
//    for (int i = 0; i < 10000; ++i) {
////        l64ht->node_scan(NULL, mykey[i], mykey[i] + 1000, res, 0);                                                                                \
//
//    }
//    gettimeofday(&ends, NULL);
//    double timeCost = (ends.tv_sec - start.tv_sec) * 1000000 + ends.tv_usec - start.tv_usec;
//    double throughPut = (double) 10000 / timeCost;
//    cout << "HT_SCAN " << testNum << " kv pais in " << timeCost / 1000000 << " s" << endl;
//    cout << "HT_SCAN " << "ThroughPut: " << throughPut << " Mops" << endl;

    // Time_BODY(1,"HT_SCAN ",{
    //      l64ht ->node_scan(NULL,mykey[i],mykey[i]+10000,res,0);
    // })


    for (int i = 0; i < testNum; ++i) {
        lbt->insert(mykey[i], &value);
    }
    timeval start, ends;
    gettimeofday(&start, NULL);
    lbt->rangeQuery(mmin, mmax);
    gettimeofday(&ends, NULL);
    double timeCost = (ends.tv_sec - start.tv_sec) * 1000000 + ends.tv_usec - start.tv_usec;
    double throughPut = (double) 10000 / timeCost;
    cout << "lbtree range query " << testNum << " kv pais in " << timeCost / 1000000 << " s" << endl;
    cout << "lbtree range query " << "ThroughPut: " << throughPut << " Mops" << endl;
    cout << res.size() << endl;
}


void *concurrency_cht_put(int threadNum) {
    init_fast_allocator(true);
    for (int i = threadNum * (testNum / numThread); i < (threadNum + 1) * (testNum / numThread); ++i) {
        cht->crash_consistent_put(NULL, allLoadKeys[i], allLoadValues[i], 0);
    }
    // fast_free();
}

void *concurrency_cht_run(int threadNum) {
    init_fast_allocator(true);
    for (int i = threadNum * (testNum / numThread); i < (threadNum + 1) * (testNum / numThread); ++i) {
        if (allRunOp[i] == YCSBRunOp::Update) {
            {
                cht->crash_consistent_put(NULL, allLoadKeys[i], allLoadValues[i], 0);
            }
        } else if (allRunOp[i] == YCSBRunOp::Get) {
            {
                cht->get(allLoadKeys[i]);
            }
        } else if (allRunOp[i] == YCSBRunOp::Scan) {
            { ; }
        }
    }
}

void *concurrency_cceh_put(int threadNum) {
    init_fast_allocator(true);
    for (int i = threadNum * (testNum / numThread); i < (threadNum + 1) * (testNum / numThread); ++i) {
        con_cceh->put(mykey[i], 1);
    }
}

void *concurrency_fastfair_put(int threadNum) {
    init_fast_allocator(true);
    for (int i = threadNum * (testNum / numThread); i < (threadNum + 1) * (testNum / numThread); ++i) {
        con_fastfair->put(mykey[i], (char *) &value);
    }
}

void *concurrency_vlht_put(int threadNum) {
    init_fast_allocator(true);
    for (int i = threadNum * (testNum / numThread); i < (threadNum + 1) * (testNum / numThread); ++i) {
        vlht->crash_consistent_put(NULL, 8, (unsigned char *) &mykey[i], 1);
//        vlht->crash_consistent_put(NULL, 8, (unsigned char *) &allLoadKeys[i], allLoadValues[i]);
        // vlht->crash_consistent_put_without_lock(NULL,8,(unsigned char*)&mykey[i],1,(uint64_t)&vlht->root);
    }
}

void *concurrency_lbr_put(int threadNum) {
    init_fast_allocator(true);
    for (int i = threadNum * (testNum / numThread); i < (threadNum + 1) * (testNum / numThread); ++i) {
//        lbt->insert(mykey[i], &value);
        lbt->insert(allLoadKeys[i], &allLoadValues[i]);
    }
}

void *concurrency_lbr_run(int threadNum) {
    init_fast_allocator(true);
    for (int i = threadNum * (testNum / numThread); i < (threadNum + 1) * (testNum / numThread); ++i) {
        if (allRunOp[i] == YCSBRunOp::Update) {
            {
                lbt->insert(allLoadKeys[i], &allLoadValues[i]);
            }
        } else if (allRunOp[i] == YCSBRunOp::Get) {
            {
                lbt->lookup(allLoadKeys[i]);
            }
        } else if (allRunOp[i] == YCSBRunOp::Scan) {
            { ; }
        }
    }
}


void *concurrency_woart_put(int threadNum) {
    init_fast_allocator(true);
    for (int i = threadNum * (testNum / numThread); i < (threadNum + 1) * (testNum / numThread); ++i) {
        conwoart_put(conwoart, mykey[i], 8, &value);
        conwoart_get(conwoart, mykey[i], 8);
    }
}


void concurrencyTest() {
//    testNum = 1000000;
    threads = new thread *[64];
    mykey = new uint64_t[testNum];
    rng r;
    rng_init(&r, 1, 2);
    for (int i = 0; i < testNum; ++i) {
        mykey[i] = rng_next(&r);
    }

    string wlName = "/scorpio/home/shared/ycsb_benchmark/heavyload_a";
//    string wlName = "/Users/wangke/CLionProjects/nvmkv/ycsb_demo";
    parseYCSBLoadFile(wlName, false);
    parseYCSBRunFile(wlName, false);

    for (int i = 1; i <= 32; i *= 2) {
        init_fast_allocator(true);
        numThread = i;
        // vlht = new_varLengthHashtree();

         cht = new_concurrency_hashtree(64, 0);
        // con_cceh = new_concurrency_cceh();
        // con_fastfair = new_concurrency_fastfair();
//        conwoart = new_conwoart_tree();
        lbt = new_lbtree();
//        CONCURRENCY_Time_BODY("concurrency lbtree  " + to_string(i) + " threads ", {
            for (int i = 0; i < numThread; i++) {
                threads[i] = new std::thread(concurrency_lbr_put, i);
            }
            for (int i = 0; i < numThread; i++) {
                threads[i]->join();
            }
//        })

        CONCURRENCY_Time_BODY("concurrency lbtree  " + to_string(i) + " threads ", {
            for (int i = 0; i < numThread; i++) {
                threads[i] = new std::thread(concurrency_lbr_run, i);
            }
            for (int i = 0; i < numThread; i++) {
                threads[i]->join();
            }
        })

////         CONCURRENCY_Time_BODY("concurrency varLength hash tree " + to_string(i) + " threads ", {
//             for(int i=0;i<numThread;i++){
//                 threads[i]  = new std::thread(concurrency_cht_put,i);
//             }
//
//             for (int i = 0; i < numThread; i++) {
//                 threads[i]->join();
//             }
////         })
//        CONCURRENCY_Time_BODY("concurrency varLength hash tree " + to_string(i) + " threads ", {
//            for(int i=0;i<numThread;i++){
//                threads[i]  = new std::thread(concurrency_cht_run,i);
//            }
//
//            for (int i = 0; i < numThread; i++) {
//                threads[i]->join();
//            }
//        })

        // CONCURRENCY_Time_BODY("concurrency woart  " + to_string(i) + " threads ", {
        //     for(int i=0;i<numThread;i++){
        //         threads[i]  = new std::thread(concurrency_woart_put,i);
        //     }

        //     for (int i = 0; i < numThread; i++) {
        //         threads[i]->join();
        //     }
        // })

        // concurrency_vlht_put(0);

        // int failed = 0;
        // vector<uint64_t> failed_key;
        // for(int i=0;i<testNum;i++){
        //     int res = vlht->get(8,(unsigned char*)&mykey[i]);
        //     if(res!=1){
        //         failed++;
        // cout<<"failed : "<<i<< " key : "<< mykey[i]<<" value: "<< res <<endl;
        // vlht->crash_consistent_put_without_lock(NULL,8,(unsigned char*)&mykey[i],1,(uint64_t)&vlht->root);
        // if(vlht->get(8,(unsigned char*)&mykey[i])!=1){
        //     cout<<"still wrong!"<<endl;
        // }else{
        //     cout<<"fixed"<<endl;
        // }
        //     }
        // }

        // cout<<"failed : "<<failed<<endl;
        // CONCURRENCY_Time_BODY("concurrency fast fair ", {
        //     for(int i=0;i<numThread;i++){
        //         threads[i]  = new std::thread(concurrency_fastfair_put,i);
        //     }

        //     for(int i=0;i<numThread;i++){
        //         threads[i]->join();
        //     }
        // })

        // int failed = 0;
        // vector<uint64_t> failed_key;
        // for(int i=0;i<testNum;i++){
        //     int res = *(int*)con_fastfair->get(mykey[i]);
        //     if(res!=1){
        //         failed++;
        //         cout<<"failed : "<<i<< " key : "<< mykey[i]<<" value: "<< res <<endl;
        //         con_fastfair->put(mykey[i],(char *) &value);
        //         if((*(int*)con_fastfair->get(mykey[i]))!=1){
        //             cout<<"still wrong!"<<endl;
        //         }else{
        //             cout<<"fixed"<<endl;
        //         }
        //     }
        // }

        // timeval start, ends;
        // gettimeofday(&start, NULL);
        // CONCURRENCY_Time_BODY("concurrency_cceh ", { 
        //     for(int i=0;i<numThread;i++){
        //         threads[i]  = new std::thread(concurrency_put_with_thread,i);
        //     }

        //     for(int i=0;i<numThread;i++){
        //         threads[i]->join();
        //     }
        // })

        // gettimeofday(&ends, NULL);
        // double timeCost = (ends.tv_sec - start.tv_sec) * 1000000 + ends.tv_usec - start.tv_usec;
        // double throughPut = (double) testNum / timeCost;
        // cout << "concurrency hash tree put " << testNum << " kv pais with "<<numThread<<" threads in " << timeCost / 1000000 << " s" << endl;
        // cout << "concurrency hash tree" << "ThroughPut: " << throughPut << " Mops" << endl;

        // int failed = 0;
        // vector<uint64_t> failed_key;
        // for(int i=0;i<testNum;i++){
        //     int res = cht->get(mykey[i]);
        //     if(res!=1){
        //         failed++;
        // cout<<"failed : "<<i<< " key : "<< mykey[i]<<" value: "<< res <<endl;

//         cht->crash_consistent_put(NULL,mykey[i],1,0);
        // if(cht->get(mykey[i])!=1){
        //     cout<<"still wrong!"<<endl;
        // }else{
        //     cout<<"fixed"<<endl;
        // }
        //     }
        // }

        // CONCURRENCY_Time_BODY("concurrency_cceh ", { 
        //         for(int i=0;i<numThread;i++){
        //         threads[i]  = new std::thread(concurrency_cceh_put,i);
        //         }

        //         for(int i=0;i<numThread;i++){
        //         threads[i]->join();
        //         }
        //     }
        // )




        // gettimeofday(&ends, NULL);
        // double timeCost = (ends.tv_sec - start.tv_sec) * 1000000 + ends.tv_usec - start.tv_usec;
        // double throughPut = (double) testNum / timeCost;
        // cout << "concurrency CCEH put " << testNum << " kv pais with "<<numThread<<" threads in " << timeCost / 1000000 << " s" << endl;
        // cout << "concurrency CCEH" << "ThroughPut: " << throughPut << " Mops" << endl;

//        int failed = 0;
//        for(int i=0;i<testNum;i++){
        // int res = con_cceh->get(mykey[i]);
        // int res = cht->get(mykey[i]);
        // int res = *(int*)con_fastfair->get(mykey[i]);
        // int res = vlht->get(8,(unsigned char*)&mykey[i]);
//            auto res = conwoart_get(conwoart, mykey[i], 8);
//            int pos;
//            void * p = lbt->lookup(mykey[i],&pos);
//            uint64_t res = ((bleaf*)p)->ch(pos).value;
//            try{
//                if( res==1 || *(int*)res!=1){
//        //            if(res==NULL||*(int*)res!=1){
//
//                    // if(res!=1){
//                        failed++;
//                        // cout<<"failed : "<<i<< " key : "<< mykey[i]<<" value: "<< res <<endl;
//                        // con_cceh->put(mykey[i],1);
//                        // if(con_cceh->get(mykey[i])!=1){
//                        //     cout<<"still wrong!"<<endl;
//                        // }else{
//                        //     cout<<"fixed"<<endl;
//                        // }
//                    }
//            }catch( Exception msg){
//                failed++;
//            }

    }

//        cout<<failed<<endl;
//        fast_free();
//    }
}

void varLengthTest() {
    int span[] = {1024};
    testNum = 1000000;
    unsigned char **keys = new unsigned char *[testNum];
    int *lengths = new int[testNum];
    rng r;
    rng_init(&r, 1, 2);

    for (int i = 0; i < testNum; i++) {
         lengths[i] = span[0];
//        lengths[i] = span[rng_next(&r) % 6];
        keys[i] = static_cast<unsigned char *>( malloc(lengths[i]));

        for (int j = 0; j < lengths[i]; j++) {
            keys[i][j] = (char) rng_next(&r);
        }
    }

    // mykey = new uint64_t[testNum];
    // rng r;
    // rng_init(&r, 1, 2);
    // for (int i = 0; i < testNum; ++i) {
    //     mykey[i] = rng_next(&r);
    // }


     Time_BODY(1,"varLengthHashTree put " , { vlht->crash_consistent_put(NULL,lengths[i],keys[i],1); })
     Time_BODY(1,"varLengthHashTree get " , { vlht->get(lengths[i],keys[i]); })


     Time_BODY(1,"varLengthFast&Fair put " , { vlff->put((char*)keys[i],lengths[i],(char *) &value); })
     Time_BODY(1,"varLengthFast&Fair get " , {
         if(*(char*)(vlff->get((char*)keys[i],lengths[i]))!=1){
             cout<<*(uint64_t*)keys[i]<<endl;
         };
     })

//    Time_BODY(1, "varLengthWoart put ",
//              { var_length_woart_put(vlwt, (char *) keys[i], lengths[i] * 8, (char *) &value); })
//     Time_BODY(1,"varLengthWoart get " , {
//         if(*(char*)(var_length_woart_get(vlwt,(char*)keys[i],lengths[i]*8))!=1){
//             cout<<*(uint64_t*)&mykey[i]<<endl;
//         };
//     })


    Time_BODY(1, "varLengthWort put ", { var_length_wort_put(vlwot, (char *) keys[i], lengths[i], (char *) &value); })
    Time_BODY(1, "varLengthWort get ", {
        if (*(char *) (var_length_wort_get(vlwot, (char *) keys[i], lengths[i])) != 1) {
            cout << *(uint64_t *) &mykey[i] << endl;
        };
    })
//    cout << concurrency_fastalloc_profile() << endl;
    fast_free();
}

void ycsb_test() {
//    string wlName = "/scorpio/home/shared/ycsb_benchmark/heavyload_a";
    string wlName = "/Users/wangke/CLionProjects/nvmkv/ycsb_demo";
    parseYCSBLoadFile(wlName, false);
    parseYCSBRunFile(wlName, false);


    for (int i = 1; i <= 36; i > 4 ? i += 4 : i *= 2) {
        init_fast_allocator(true);
        numThread = i;
//        cht = new_concurrency_hashtree(64, 0);
//        con_cceh = new_concurrency_cceh();
//        con_fastfair = new_concurrency_fastfair();
//        conwoart = new_conwoart_tree();

//        testNum = nLoadOp;
//        CON_Time_BODY( "concurrency hash tree put "+ to_string(i)+" thread ", {
//            cht->crash_consistent_put(NULL, allLoadKeys[k], allLoadValues[k], 0);
//        })
//
//        testNum = nRunOp;
//        CON_Time_BODY("concurrent hash tree get "+ to_string(i)+" thread ",{
//            if (allRunOp[k] == YCSBRunOp::Update) {
//                { cht->crash_consistent_put(NULL, allRunKeys[k], allRunValues[k], 0); }                                                                     \
//            } else if ( allRunOp[k]== YCSBRunOp::Get) {
//                {
//                     cht->get(allRunKeys[k]);  }
//            } else if ( allRunOp[k]== YCSBRunOp::Scan) {
//                { ; }
//            }
//        } )

//        testNum = nLoadOp;
//        CON_Time_BODY( "concurrent CCEH put "+ to_string(i)+" thread ", {
//            con_cceh->put(allLoadKeys[k],  allLoadValues[k]);
//        })
//
//        testNum = nRunOp;
//        CON_Time_BODY("concurrent CCEH get "+ to_string(i)+" thread ",{
//            if (allRunOp[k] == YCSBRunOp::Update) {
//                { con_cceh->put(allRunKeys[k],  allRunValues[k]); }                                                                     \
//            } else if ( allRunOp[k]== YCSBRunOp::Get) {
//                {
//                    con_cceh->get(allRunKeys[k]);
//                }
//            } else if ( allRunOp[k]== YCSBRunOp::Scan) {
//                { ; }
//            }
//        } )
//
//        testNum = nLoadOp;
//        CON_Time_BODY( "concurrent Fast&Fair put "+ to_string(i)+" thread ", {
//            con_fastfair->put(allLoadKeys[k], (char *)&allLoadValues[k]);
//        })
//
//        testNum = nRunOp;
//        CON_Time_BODY("concurrent Fast&Fair get "+ to_string(i)+" thread ",{
//            if (allRunOp[k] == YCSBRunOp::Update) {
//                { con_fastfair->put(allRunKeys[k], (char *)& allRunValues[k]); }                                                                     \
//            } else if ( allRunOp[k]== YCSBRunOp::Get) {
//                {
//                    con_fastfair->get(allRunKeys[k]);
//                }
//            } else if ( allRunOp[k]== YCSBRunOp::Scan) {
//                { ; }
//            }
//        })

//
//        testNum = nLoadOp;
//        CON_Time_BODY("concurrent Woart put " + to_string(i) + " thread ", {
//            conwoart_put(conwoart, allLoadKeys[k], 8, &allLoadValues[k]);
//        })
//
//        testNum = nRunOp;
//        CON_Time_BODY("concurrent Woart get " + to_string(i) + " thread ", {
//            if (allRunOp[k] == YCSBRunOp::Update) {
//                {
//                    conwoart_put(conwoart, allRunKeys[k], 8, &allRunValues[k]);
//                }
//            } else if (allRunOp[k] == YCSBRunOp::Get) {
//                {
//                    conwoart_get(conwoart, allRunKeys[k], 8);
//                }
//            } else if (allRunOp[k] == YCSBRunOp::Scan) {
//                { ; }
//            }
//        })


        testNum = nLoadOp;
        CON_Time_BODY(to_string(i) + ", ", {
            lbt->insert(allLoadKeys[k], &allLoadValues[k]);
        })
        testNum = nRunOp;
        CON_Time_BODY("concurrent lb+trees get " + to_string(i) + " thread ", {
            if (allRunOp[k] == YCSBRunOp::Update) {
                {
                    lbt->insert(allLoadKeys[k], &allLoadValues[k]);
                }
            } else if (allRunOp[k] == YCSBRunOp::Get) {
                {
                    lbt->lookup(allLoadKeys[k]);
                }
            } else if (allRunOp[k] == YCSBRunOp::Scan) {
                { ; }
            }
        })

//        fast_free();
    }



    //vlht = new_varLengthHashtree();


}

void recoveryTest() {
    mykey = new uint64_t[testNum];
    rng r;
    rng_init(&r, 1, 2);
    for (int i = 0; i < testNum; ++i) {
        mykey[i] = rng_next(&r);
    }

    for (int j = 1000000; j <= 10000000; j += 2000000) {
        vlht = new_varLengthHashtree();
        testNum = j;
        for (int i = 0; i < testNum; ++i) {
            vlht->crash_consistent_put_without_lock(NULL, 8, (unsigned char *) &mykey[i], 1, (uint64_t) &vlht->root);
        }

        timeval start, ends;
        gettimeofday(&start, NULL);
        recovery(vlht->root, 0);
        gettimeofday(&ends, NULL);
        double timeCost = (ends.tv_sec - start.tv_sec) * 1000000 + ends.tv_usec - start.tv_usec;

        cout << "vlht recovery test" << testNum << " kv pais in " << timeCost / 1000000 << " s" << endl;

    }


}

void effect2() {
    int span[] = {4, 8, 16, 32, 64, 128};
//    testNum = 30000000;
    unsigned char **keys = new unsigned char *[testNum];
    int *lengths = new int[testNum];
    rng r;
    rng_init(&r, 1, 2);

    for (int i = 0; i < testNum; i++) {
//         lengths[i] = span[1];
        lengths[i] = span[rng_next(&r) % 6];
        keys[i] = static_cast<unsigned char *>( malloc(lengths[i]));

        for (int j = 0; j < lengths[i]; j++) {
            keys[i][j] = (char) rng_next(&r);
        }
    }

    Time_BODY(1, "varLengthHashTree put ", { vlht->crash_consistent_put(NULL, lengths[i], keys[i], 1); })
//    Time_BODY(1, "varLengthHashTree get ", { vlht->get(lengths[i], keys[i]); })


//    Time_BODY(1, "varLengthFast&Fair put ", { vlff->put((char *) keys[i], lengths[i], (char *) &value); })
//    Time_BODY(1, "varLengthFast&Fair get ", {
//        if (*(char *) (vlff->get((char *) keys[i], lengths[i])) != 1) {
//            cout << *(uint64_t *) keys[i] << endl;
//        };
//    })
//
//    Time_BODY(1, "varLengthWoart put ", { var_length_woart_put(vlwt, (char *) keys[i], lengths[i] * 8, (char *) &value); })
//    Time_BODY(1, "varLengthWoart get ", {
//        if (*(char *) (var_length_woart_get(vlwt, (char *) keys[i], lengths[i] * 8)) != 1) {
//            cout << *(uint64_t *) &mykey[i] << endl;
//        };
//    })
//
//
//    Time_BODY(1, "varLengthWort put ", { var_length_wort_put(vlwot, (char *) keys[i], lengths[i], (char *) &value); })
//    Time_BODY(1, "varLengthWort get ", {
//        if (*(char *) (var_length_wort_get(vlwot, (char *) keys[i], lengths[i])) != 1) {
//            cout << *(uint64_t *) &mykey[i] << endl;
//        };
//    })

    cout << concurrency_fastalloc_profile() << endl;
}

int main(int argc, char *argv[]) {
    sscanf(argv[1], "%d", &numThread);
    sscanf(argv[2], "%d", &testNum);
    init_fast_allocator(true);
    ht = new_hashtree(64, 0);
    art = new_art_tree();
    wort = new_wort_tree();
    woart = new_woart_tree();
    cceh = new_cceh();
    ff = new_fastfair();
    roart = new_roart();
    l64ht = new_length64HashTree();
    lbt = new_lbtree();
    vlht = new_varLengthHashtree();
    vlwt = new_var_length_woart_tree();
    vlwot = new_var_length_wort_tree();
// //    mt = new_mass_tree();

    vlff = new_varlengthfastfair();
//    bt = new_blink_tree(numThread);
    // correctnessTest();

//     speedTest();

    varLengthTest();

//    effect2();

// build for cocurrencyTest

//    try {

//        recoveryTest();
//     concurrencyTest();

//        ycsb_test();

//    } catch (void *) {
//        fast_free();
//    }
//    profile();
//    range_query_correctness_test();
//    cout << ht->node_cnt << endl;
//    cout << ht->get_access << endl;
    fast_free();
    return 0;
}
