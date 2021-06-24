#include<atomic>
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
#include "fastfair/fastfair.h"
#include "roart/roart.h"
#include "concurrencyhashtree/concurrency_hashtree.h"

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
        cout << name << testNum << " kv pais in " << timeCost / 1000000 << " s" << endl;        \
        cout << name << "ThroughPut: " << throughPut << " Mops" << endl;                        \
    }

#define MULTITHREAD true

int testNum = 100000;

int numThread = 1;

int test_algorithms_num = 10;
bool test_case[10] = {1, // ht
                      0, // art
                      1, // wort
                      1, // woart
                      1, // cacheline_concious_extendible_hash
                      1, // fast&fair
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

concurrencyhashtree * cht;


thread** threads;

uint64_t *mykey;

std::mutex mtx;

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
    out.open("/home/wangke/nvmkv/res.txt", ios::app);
    mykey = new uint64_t[testNum];
    rng r;
    rng_init(&r, 1, 2);
    for (int i = 0; i < testNum; ++i) {
        mykey[i] = rng_next(&r);
//        mykey[i] = rng_next(&r) & 0xffffffff00000000;
//        mykey[i] = rng_next(&r) % testNum;
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
    mykey = new uint64_t[testNum];
    rng r;
    rng_init(&r, 1, 2);
    for (int i = 0; i < testNum; ++i) {
        mykey[i] = rng_next(&r);
    }
    vector<ff_key_value> res;
//    vector<ht_key_value> res;
//    vector<woart_key_value> res;
//    vector<wort_key_value> res;
    uint64_t value = 1;
    for (int i = 0; i < testNum; i += 1) {
        roart->put(i + 1, i + 1);
//        ff->put(i + 1, (char *) &value);
//        woart_put(woart, i + 1, 8, &value);
//        wort_put(wort, i + 1, 8, &value);
//        ht->crash_consistent_put(NULL, i+1, 1, 0);
    }
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
    roart->scan(1, 10000);
}



void *concurrency_put_with_thread(int threadNum){
        for (int i = threadNum*(testNum/numThread); i < (threadNum+1)*(testNum/numThread); ++i) {                                                     
              cht->crash_consistent_put(NULL,mykey[i], 1, 0);
        }
}

void concurrencyTest(){
     mykey = new uint64_t[testNum];
        rng r;
        rng_init(&r, 1, 2);
        for (int i = 0; i < testNum; ++i) {
            mykey[i] = rng_next(&r);
     }

    for( int i =1;i<=8;i*=2){
        numThread = i;
        init_fast_allocator(true);
        cht = new_concurrency_hashtree(64, 0);

        timeval start, ends;                                                                    
        gettimeofday(&start, NULL);    

        for(int i=0;i<numThread;i++){
            threads[i]  = new std::thread(concurrency_put_with_thread,i);     
        }

        for(int i=0;i<numThread;i++){
            threads[i]->join();
        }

        gettimeofday(&ends, NULL);                                                              
        double timeCost = (ends.tv_sec - start.tv_sec) * 1000000 + ends.tv_usec - start.tv_usec;
        double throughPut = (double) testNum / timeCost;  
        cout << "concurrency hash tree put " << testNum << " kv pais with "<<numThread<<" threads in " << timeCost / 1000000 << " s" << endl;        
        cout << "concurrency hash tree" << "ThroughPut: " << throughPut << " Mops" << endl;   
        fast_free();
    }
}

int main(int argc, char *argv[]) {
    sscanf(argv[1], "%d", &numThread);
    sscanf(argv[2], "%d", &testNum);
    init_fast_allocator(true);
    // ht = new_hashtree(64, 0);
    // art = new_art_tree();
    // wort = new_wort_tree();
    // woart = new_woart_tree();
    // cceh = new_cceh();
    // ff = new_fastfair();
    // roart = new_roart();
    

//    mt = new_mass_tree();
//    bt = new_blink_tree(numThread);
    // correctnessTest();
//    speedTest();


// build for cocurrencyTest
    threads = new thread* [64];
    // try{

    concurrencyTest();

    // }catch(void*){
    //     fast_free();
    // }
//    profile();
//    range_query_correctness_test();
//    cout << ht->node_cnt << endl;
//    cout << ht->get_access << endl;
    // fast_free();
    return 0;
}