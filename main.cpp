#include <atomic>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <stdio.h>
#include <map>
#include <string.h>
#include <sys/time.h>
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
#include "varLengthHashTree/varLengthHashTree.h"

using namespace std;

ofstream out;

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
        cout << throughPut << ", " << endl;                        \
    }


#define Time_BODY_EXPE(condition, name, func)                                                        \
    if(condition) {                                                                             \
        sleep(1);                                                                               \
        timeval start, ends;                                                                    \
        cout<<name<<", ";\
        gettimeofday(&start, NULL);                                                             \
        int interval = testNum/10;                                                              \
        for (int i = 0; i < testNum; ++i) {                                                     \
            func                                                                                \
            if (i%interval==interval-1){\
                gettimeofday(&ends, NULL);                                                              \
                double timeCost = (ends.tv_sec - start.tv_sec) * 1000000 + ends.tv_usec - start.tv_usec;\
                double throughPut = (double) interval / timeCost;                                        \
                cout << throughPut << ", ";\
                gettimeofday(&start, NULL);       \
            }\
        }\
        cout<<endl;\
    }

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
                memory=fastalloc_profile();                                                               \
                cout << memory << ", ";\
            }\
        }\
        cout<<endl;\
    }

int testNum = 100000;

int numThread = 1;

int test_algorithms_num = 10;
bool test_case[10] = {0, // ht
                      0, // art
                      0, // wort
                      0, // woart
                      0, // cacheline_concious_extendible_hash
                      0, // fast&fair
                      0, // roart
                      1, // vlht
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
VarLengthHashTree *vlht;
Length64HashTree *l64ht;

concurrencyhashtree *cht;
concurrency_cceh *con_cceh;
concurrency_fastfair *con_fastfair;

thread **threads;

uint64_t *mykey;

std::mutex mtx;

int value = 1;

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
        mykey[i] = rng_next(&r) % testNum;
    }
    uint64_t value = 1;
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

    // insert speed for vlht
    Time_BODY(test_case[7], "vlht put  ", { vlht->crash_consistent_put(NULL, 8, (unsigned char *) &mykey[i], 1); })

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

    // query speed for vlht
    Time_BODY(test_case[7], "vlht get ", { vlht->get(8, (unsigned char *) &(mykey[i])); })

    //range query speed for ht
    Time_BODY(range_query_test_case[0], "hash tree range query ", { ht->scan(mykey[i], mykey[i] + 1024); })

    //range query speed for wort
    Time_BODY(range_query_test_case[1], "wort range query ", { wort_scan(wort, mykey[i], mykey[i] + 1024); })

    //range query speed for woart
    Time_BODY(range_query_test_case[2], "woart tree range query ", { woart_scan(woart, mykey[i], mykey[i] + 1024); })

    //range query speed for fast&fair
    Time_BODY(range_query_test_case[3], "fast&fair range query ", { ff->scan(mykey[i], mykey[i] + 1024); })

    out.close();
}

void profile() {
//    out.open("/Users/wangke/Desktop/ht_profile.csv");
    mykey = new uint64_t[testNum];
    rng r;
    rng_init(&r, 1, 2);
    for (int i = 0; i < testNum; ++i) {
        mykey[i] = rng_next(&r);
    }
    uint64_t value = 1;

    int span[] = {4, 8, 16, 32, 64, 128};
    unsigned char **keys = new unsigned char *[testNum];
    int *lengths = new int[testNum];

    for (int i = 0; i < testNum; i++) {
//        lengths[i] = span[0];
        lengths[i] = span[rng_next(&r) % 6];
        keys[i] = static_cast<unsigned char *>(malloc(lengths[i]));
        for (int j = 0; j < lengths[i]; j++) {
            keys[i][j] = rng_next(&r);
        }
    }

    timeval start, ends;
//    gettimeofday(&start, NULL);
    for (int i = 0; i < testNum; ++i) {
//        cceh->put(mykey[i], value);
//        ff->put(mykey[i], (char *) &value);
//        roart->put(mykey[i], value);
//        woart_put(woart, mykey[i], 8, &value);
//        wort_put(wort, mykey[i], 8, &value);
//        vlht->crash_consistent_put(NULL, lengths[i], keys[i], 1);
        vlht->crash_consistent_put(NULL, 8, (unsigned char *)&mykey[i], 1);
//        ht->crash_consistent_put(NULL, mykey[i], i + 1, 0);
//        if (i % 10000 == 0) {
//            out << i << ", " << cceh->dir_size << ", "
//                << (double) i / (cceh_seg_num * CCEH_BUCKET_SIZE * CCEH_MAX_BUCKET_NUM) << endl;
//            out << i << ", " << ht_dir_num << ", " << ht_seg_num << ", "
//                << (double) i / (ht_seg_num * HT_MAX_BUCKET_NUM * HT_BUCKET_SIZE) << endl;
//        }
    }
//    gettimeofday(&ends, NULL);
//    double timeCost = (ends.tv_sec - start.tv_sec) * 1000000 + ends.tv_usec - start.tv_usec;
//    cout << timeCost << endl;
//    woart_profile();
    vlht->profile();
//    cceh->profile();
//    gettimeofday(&start, NULL);
//    for (int i = 0; i < testNum; ++i) {
//        woart_get(woart, mykey[i], 8);
//        wort_get(wort, mykey[i], 8);
//        vlht->get(lengths[i], keys[i]);
//    }
//    gettimeofday(&ends, NULL);
//    double timeCost = (ends.tv_sec - start.tv_sec) * 1000000 + ends.tv_usec - start.tv_usec;
//    double throughPut = (double) testNum / timeCost;
//    double avg_visited_node = vlht->profile() / testNum;
//    double avg_time = timeCost / avg_visited_node;
//    cout << avg_visited_node << ", " << avg_time;
//    cout << "varLengthHashTree get " << testNum << " kv pais in " << timeCost / 1000000 << " s" << endl;
//    cout << "varLengthHashTree get " << "ThroughPut: " << throughPut << " Mops" << endl;
    //    out << t1 << endl << t2 << endl << t3 << endl;
//    out << timeCost << endl;
//    out.close();
}

void varLengthTest() {
    int span[] = {8};
    unsigned char **keys = new unsigned char *[testNum];
    int *lengths = new int[testNum];
    rng r;
    rng_init(&r, 1, 2);

    for (int i = 0; i < testNum; i++) {
        lengths[i] = span[0];
        // lengths[i] = span[rng_next(&r)%7];
        keys[i] = static_cast<unsigned char *>(malloc(lengths[i]));
        for (int j = 0; j < lengths[i]; j++) {
            keys[i][j] = rng_next(&r) % testNum;
        }
    }
    timeval start, ends;
    gettimeofday(&start, NULL);
    for (int i = 0; i < testNum; i++) {
        vlht->crash_consistent_put(NULL, lengths[i], keys[i], 1);
    }
    gettimeofday(&ends, NULL);
    double timeCost = (ends.tv_sec - start.tv_sec) * 1000000 + ends.tv_usec - start.tv_usec;
    double throughPut = (double) testNum / timeCost;
    cout << "varLengthHashTree put " << testNum << " kv pais in " << timeCost / 1000000 << " s" << endl;
    cout << "varLengthHashTree put " << "ThroughPut: " << throughPut << " Mops" << endl;

    int wrong = 0;
    gettimeofday(&start, NULL);
    for (int i = 0; i < testNum; i++) {
        uint64_t res = vlht->get(lengths[i], keys[i]);
        // if(res!=1){
        //     wrong++;
        // }                                                  
    }
    gettimeofday(&ends, NULL);
//    cout<< " wrong! "<<wrong/testNum<<endl;
    timeCost = (ends.tv_sec - start.tv_sec) * 1000000 + ends.tv_usec - start.tv_usec;
    throughPut = (double) testNum / timeCost;
    cout << "varLengthHashTree get " << testNum << " kv pais in " << timeCost / 1000000 << " s" << endl;
    cout << "varLengthHashTree get " << "ThroughPut: " << throughPut << " Mops" << endl;


    fast_free();
}

void effect1(){
    mykey = new uint64_t[testNum];
    rng r;
    rng_init(&r, 1, 2);
    for (int i = 0; i < testNum; ++i) {
        mykey[i] = rng_next(&r);
//        mykey[i] = rng_next(&r) & 0xffffffff00000000;
//        mykey[i] = rng_next(&r) % testNum;
    }
    uint64_t value = 1;

    Time_BODY(1, "vlht put  ", { vlht->crash_consistent_put(NULL, 8, (unsigned char *) &mykey[i], 1); })

    Time_BODY(1, "vlht get ", { vlht->get(8, (unsigned char *) &(mykey[i])); })

    cout<<fastalloc_profile()<<endl;
}

void effect2() {
    mykey = new uint64_t[testNum];
    rng r;
    rng_init(&r, 1, 2);
    for (int i = 0; i < testNum; ++i) {
//        mykey[i] = rng_next(&r);
//        mykey[i] = rng_next(&r) & 0xffffffff00000000;
        mykey[i] = rng_next(&r) % testNum;
    }
    uint64_t value = 1;

    // insert speed for ht
    MEMORY_BODY_EXPE(test_case[0], "hash tree put ", { ht->crash_consistent_put(NULL, mykey[i], 1, 0); })

    // insert speed for art
    MEMORY_BODY_EXPE(test_case[1], "art tree put ", { art_put(art, (unsigned char *) &(mykey[i]), 8, &value); })

    // insert speed for wort
    MEMORY_BODY_EXPE(test_case[2], "wort put ", { wort_put(wort, mykey[i], 8, &value); })

    // insert speed for woart
    MEMORY_BODY_EXPE(test_case[3], "woart put ", { woart_put(woart, mykey[i], 8, &value); })

    // insert speed for cceh
    MEMORY_BODY_EXPE(test_case[4], "cceh put ", { cceh->put(mykey[i], value); })

    // insert speed for fast&fair
    MEMORY_BODY_EXPE(test_case[5], "fast&fair put ", { ff->put(mykey[i], (char *) &value); })

    // insert speed for fast&fair
    MEMORY_BODY_EXPE(test_case[6], "roart put ", { roart->put(mykey[i], value); })

    // insert speed for vlht
    MEMORY_BODY_EXPE(test_case[7], "vlht put  ", { vlht->crash_consistent_put(NULL, 8, (unsigned char *) &mykey[i], 1); })

//    // query speed for ht
//    MEMORY_BODY_EXPE(test_case[0], "hash tree get ", { ht->get(mykey[i]); })
//
//    // query speed for art
//    MEMORY_BODY_EXPE(test_case[1], "art tree get ", { art_get(art, (unsigned char *) &(mykey[i]), 8); })
//
//    // query speed for wort
//    MEMORY_BODY_EXPE(test_case[2], "wort get ", { wort_get(wort, mykey[i], 8); })
//
//    // query speed for woart
//    MEMORY_BODY_EXPE(test_case[3], "woart get ", { woart_get(woart, mykey[i], 8); })
//
//    // query speed for cceh
//    MEMORY_BODY_EXPE(test_case[4], "cceh get ", { cceh->get(mykey[i]); })
//
//    // query speed for fast&fair
//    MEMORY_BODY_EXPE(test_case[5], "fast&fair get ", { ff->get(mykey[i]); })
//
//    // query speed for fast&fair
//    MEMORY_BODY_EXPE(test_case[6], "roart get ", { roart->get(mykey[i]); })
//
//    // query speed for vlht
//    MEMORY_BODY_EXPE(test_case[7], "vlht get ", { vlht->get(8, (unsigned char *) &(mykey[i])); })
}

int main(int argc, char *argv[]) {
    sscanf(argv[1], "%d", &numThread);
    sscanf(argv[2], "%d", &testNum);
    init_fast_allocator(false);
    ht = new_hashtree(64, 0);
    // art = new_art_tree();
    wort = new_wort_tree();
    woart = new_woart_tree();
    cceh = new_cceh();
    ff = new_fastfair();
    roart = new_roart();
    l64ht = new_length64HashTree();

//    mt = new_mass_tree();
    vlht = new_varLengthHashtree();
//    bt = new_blink_tree(numThread);
//    speedTest();
//
//    varLengthTest();
//    profile();
    effect1();
//    effect2();
//    range_query_correctness_test();
//    cout << ht->node_cnt << endl;
//    cout << ht->get_access << endl;
    // fast_free();
    return 0;
}