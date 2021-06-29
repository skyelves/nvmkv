#include<atomic>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <stdio.h>
#include <map>
#include <string.h>
#include <sys/time.h>
#include <functional>
#include <unordered_map>
#include "blink/blink_tree.h"
#include "mass/mass_tree.h"
#include "hashtree/hashtree.h"
#include "rng/rng.h"
#include "art/art.h"
#include "wort/wort.h"
#include "woart/woart.h"
#include "cceh/cacheline_concious_extendible_hash.h"
#include "fastfair/fastfair.h"
#include "roart/roart.h"
#include "concurrencyhashtree/concurrency_hashtree.h"

using namespace std;

ofstream out;

#define Time_BODY_LOAD(condition, name, func)                                                        \
    if(condition) {                                                                             \
        sleep(1);                                                                               \
        timeval start, ends;                                                                    \
        gettimeofday(&start, NULL);                                                             \
        for (int i = 0; i < nLoadOp; ++i) {                                                     \
            func                                                                                \
        }                                                                                       \
        gettimeofday(&ends, NULL);                                                              \
        double timeCost = (ends.tv_sec - start.tv_sec) * 1000000 + ends.tv_usec - start.tv_usec;\
        double throughPut = (double) nLoadOp / timeCost;                                        \
        cout << name << "ThroughPut: " << throughPut << " Mops" << endl;                        \
    }

enum YCSBRunOp {
    Get,
    Scan,
    Update,
};

#define Time_BODY_RUN(condition, name, isupd,                                              \
getfunc, updfunc, scanfunc)                                                                     \
    if(condition) {                                                                             \
        sleep(1);                                                                               \
        timeval start, ends;                                                                    \
        gettimeofday(&start, NULL);                                                             \
        for (int i = 0; i < nRunOp; ++i) {                                                         \
            if (isupd == YCSBRunOp::Update) {                                                   \
                { updfunc }                                                                     \
            } else if (isupd == YCSBRunOp::Get) {                                               \
                { getfunc }                                                                     \
            } else if (isupd == YCSBRunOp::Scan) {                                              \
                { scanfunc }                                                                    \
            }                                                                                   \
        }                                                                                       \
        gettimeofday(&ends, NULL);                                                              \
        double timeCost = (ends.tv_sec - start.tv_sec) * 1000000 + ends.tv_usec - start.tv_usec;\
        double throughPut = (double) nRunOp / timeCost;                                        \
        cout << name << "ThroughPut: " << throughPut << " Mops" << endl;                        \
    }


uint64_t nLoadOp, nRunOp;

int numThread = 1;

int test_algorithms_num = 10;
bool test_case[10] = {1, // ht
                      0, // art
                      1, // wort
                      1, // woart
                      1, // cacheline_concious_extendible_hash
                      0, // fast&fair
                      0, // roart
                      0};

bool range_query_test_case[10] = {
        0, // ht
        0, // wort
        0, // woart
        0, // fast&fair
        0, // roart
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

// ycsb workload related
std::vector<uint64_t> allRunKeys;
std::vector<uint64_t> allRunValues;
std::vector<uint64_t> allLoadKeys;
std::vector<uint64_t> allLoadValues;

std::vector<YCSBRunOp> allRunOp;
std::unordered_map<uint64_t, uint64_t>* oracleMap;

thread** threads;

std::mutex mtx;

#define info(args...) \
{ \
    printf("INFO: "); \
    printf(args); \
    printf("\n"); \
    fflush(stdout); \
}

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
    info("Finish parse run file");

    if (correctCheck) {
        for (uint64_t hashKey : allRunKeys) {
            assert(oracleMap->count(hashKey));
            // info("hash: %lx", hashKey);
            uint64_t htValue = ht->get(hashKey);
            assert(htValue);
            uint64_t mapValue = oracleMap->at(hashKey);
            if (htValue != mapValue) {
                info("Incorrect key %lx v1 %lx v2 %lx", hashKey, htValue, mapValue);
                assert(false);
            }
        }
        info("Pass correctness check!");
    }
}

void parseYCSBLoadFile(std::string wlName, bool correctCheck) {
    std::string rawStr;
    uint64_t opCnt = 0;
    uint32_t rangeNum = 0;
    uint64_t hashKey, hashValue;
    std::ifstream loadFile(wlName + ".load");
    assert(loadFile.is_open());
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
    info("Finish parse load file");

    nLoadOp = allLoadKeys.size();
}

void speedTest() {
    // out.open("/home/wangke/nvmkv/res.txt", ios::app);
    // mykey = new uint64_t[testNum];
    // rng r;
    // rng_init(&r, 1, 2);
    // for (int i = 0; i < testNum; ++i) {
    //     mykey[i] = rng_next(&r);
    // }
    // uint64_t value = 1;
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
    Time_BODY_LOAD(test_case[0], "hash tree put ", { ht->crash_consistent_put(NULL, allLoadKeys[i], allLoadValues[i], 0); })

    // insert speed for art
    Time_BODY_LOAD(test_case[1], "art tree put ", { art_put(art, (unsigned char *) &(allLoadKeys[i]), 8, &allLoadValues[i]); })

    // insert speed for wort
    Time_BODY_LOAD(test_case[2], "wort put ", { wort_put(wort, allLoadKeys[i], 8, &allLoadValues[i]); })

    // insert speed for woart
    Time_BODY_LOAD(test_case[3], "woart put ", { woart_put(woart, allLoadKeys[i], 8, &allLoadValues[i]); })

    // insert speed for cceh
    Time_BODY_LOAD(test_case[4], "cceh put ", { cceh->put(allLoadKeys[i], allLoadValues[i]); })

    // insert speed for fast&fair
    Time_BODY_LOAD(test_case[5], "fast&fair put ", { ff->put(allLoadKeys[i], (char *) &allLoadValues[i]); })

    // insert speed for fast&fair
    Time_BODY_LOAD(test_case[6], "roart put ", { roart->put(allLoadKeys[i], allLoadValues[i]); })

    printf("\n");

    // query speed for ht
    // Note: the range query is looked up in a hashed range, therefore the valid items are limited.
    Time_BODY_RUN(test_case[0], "hash tree get ", allRunOp[i], { ht->get(allRunKeys[i]); }, { ht->crash_consistent_put(NULL, allRunKeys[i], allRunValues[i], 0); }, { ht->scan(allRunKeys[i], allRunKeys[i] + allRunValues[i]); })

    // query speed for art
    Time_BODY_RUN(test_case[1], "art tree get ", allRunOp[i], { art_get(art, (unsigned char *) &(allRunKeys[i]), 8); }, { art_put(art, (unsigned char *) &(allRunKeys[i]), 8, &allRunValues[i]); }, { /* assert(false); */ })

    // query speed for wort
    Time_BODY_RUN(test_case[2], "wort get ", allRunOp[i], { wort_get(wort, allRunKeys[i], 8); }, { wort_put(wort, allRunKeys[i], 8, &allRunValues[i]); }, { wort_scan(wort, allRunKeys[i], allRunKeys[i] + allRunValues[i]); })

    // query speed for woart
    Time_BODY_RUN(test_case[3], "woart get ", allRunOp[i], { woart_get(woart, allRunKeys[i], 8); }, { woart_put(woart, allRunKeys[i], 8, &allRunValues[i]); }, { woart_scan(woart, allRunKeys[i], allRunKeys[i] + allRunValues[i]); })

    //  query speed for cceh
    Time_BODY_RUN(test_case[4], "cceh get ", allRunOp[i], { cceh->get(allRunKeys[i]); }, { cceh->put(allRunKeys[i], allRunValues[i]); }, { /* assert(false); */ })

    // query speed for fast&fair
    Time_BODY_RUN(test_case[5], "fast&fair get ", allRunOp[i], { ff->get(allRunKeys[i]); }, { ff->put(allRunKeys[i], (char *) &allRunValues[i]); }, { ff->scan(allRunKeys[i], allRunKeys[i] + allRunValues[i]); })

    // out.close();
}

int main(int argc, char *argv[]) {
    // sscanf(argv[1], "%d", &numThread);
    // sscanf(argv[2], "%d", &testNum);
    if (argc < 2) {
        info("Usage ./nvmkv /scorpio/home/shared/ycsb_benchmark/workloada");
        assert(false);
    }
    std::string wlName = argv[1];
    init_fast_allocator();
    ht = new_hashtree(64, 4);


    parseYCSBLoadFile(wlName, false);
    parseYCSBRunFile(wlName, false);
    art = new_art_tree();
    wort = new_wort_tree();
    woart = new_woart_tree();
    cceh = new_cceh();
    ff = new_fastfair();
    roart = new_roart();
//    mt = new_mass_tree();
//    bt = new_blink_tree(numThread);
//    concurrencyhashtree *cht = new_concurrency_hashtree(64, 0);

    // test_key_len();
//    correctnessTest();
    speedTest();


// build for cocurrencyTest
//    threads = new thread* [numThread];
//    try{
//        concurrencyTest(cht);
//    }catch(void*){
//        fast_free();
//    }
//    profile();
//    range_query_correctness_test();
//    cout << ht->node_cnt << endl;
//    cout << ht->get_access << endl;
    fast_free();
    return 0;
}