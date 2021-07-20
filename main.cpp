#include<atomic>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <stdio.h>
#include <map>
#include <set>
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
        cout << name << ", " << throughPut << endl;                                             \
    }

#define Time_BODY1(condition, name, func)                                                        \
    if(condition) {                                                                             \
        sleep(1);                                                                               \
        timeval start, ends;                                                                    \
        gettimeofday(&start, NULL);                                                             \
        for (int i = 0; i < 100000; ++i) {                                                     \
            func                                                                                \
        }                                                                                       \
        gettimeofday(&ends, NULL);                                                              \
        double timeCost = (ends.tv_sec - start.tv_sec) * 1000000 + ends.tv_usec - start.tv_usec;\
        double throughPut = (double) 100000 / timeCost;                                        \
        cout << name << ", " << throughPut << endl;                                             \
    }

int testNum = 100000;

int numThread = 1;

int test_algorithms_num = 10;
bool test_case[10] = {1, // ht
                      0, // art
                      0, // wort
                      0, // woart
                      0, // cacheline_concious_extendible_hash
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

rng r;

thread **threads;

uint64_t *mykey;
char **mykey_str;
//uint64_t *negative_key;

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

void generate_workflow(uint64_t type) {
    // 0: dense
    // 1: sparse
    // 2: variable length: 4 bytes - 128 bytes

    mykey = new uint64_t[testNum];
    rng_init(&r, 1, 2);
    if (type == 0) {
        for (int i = 0; i < testNum; ++i) {
            mykey[i] = rng_next(&r) % testNum;
        }
    } else if (type == 1) {
        for (int i = 0; i < testNum; ++i) {
            mykey[i] = rng_next(&r);
        }
    } else if (type == 2) {
        mykey_str = new char*[testNum];
        uint64_t key_len[10] = {4, 8, 12, 16, 20, 32, 64, 128};
        for (int i = 0; i < testNum; ++i) {
            uint64_t tmp = rng_next(&r);
            uint64_t len = key_len[tmp % 8];
            mykey_str[i]=new char[len+2];
            for (int j = 0; j < len; j += 8) {
                *((uint64_t *) (mykey_str[i] + j)) = tmp;
                tmp = rng_next(&r);
            }
        }
    }

//    out.close();

//    negative_key = new uint64_t[testNum];
//    for (int i = 0; i < testNum; ++i) {
//        uint64_t tmp = rng_next(&r);
//        while (s.find(tmp) != s.end()) {
//            tmp = rng_next(&r);
//        }
//        negative_key[i] = tmp;
//    }
}

void speedTest() {
//    out.open("/home/wangke/nvmkv/res.txt", ios::app);
    generate_workflow(1);
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
//    cout << "hash tree put" << testNum << " kv pais in " << timeCost / 1000000 << " s" << endl;
//    cout << "hash tree Put ThroughPut: " << throughPut << " Mops" << endl;
//    cout << throughPut << endl;

    // insert speed for ht
    Time_BODY(test_case[0], "ET put", { ht->crash_consistent_put(NULL, mykey[i], 1, 0); })

    // insert speed for art
    Time_BODY(test_case[1], "art tree put", { art_put(art, (unsigned char *) &(mykey[i]), 8, &value); })

    // insert speed for wort
    Time_BODY(test_case[2], "wort put", { wort_put(wort, mykey[i], 8, &value); })

    // insert speed for woart
    Time_BODY(test_case[3], "woart put", { woart_put(woart, mykey[i], 8, &value); })

    // insert speed for cceh
    Time_BODY(test_case[4], "cceh put", { cceh->put(mykey[i], value); })

    // insert speed for fast&fair
    Time_BODY(test_case[5], "fast&fair put", { ff->put(mykey[i], (char *) &value); })

    // insert speed for fast&fair
    Time_BODY(test_case[6], "roart put", { roart->put(mykey[i], value); })

    // query speed for ht
    Time_BODY(test_case[0], "ET positive_get", { ht->get(mykey[i]); })

    // query speed for art
    Time_BODY(test_case[1], "arttree positive_get", { art_get(art, (unsigned char *) &(mykey[i]), 8); })

    // query speed for wort
    Time_BODY(test_case[2], "wort positive_get", { wort_get(wort, mykey[i], 8); })

    // query speed for woart
    Time_BODY(test_case[3], "woart positive_get", { woart_get(woart, mykey[i], 8); })

    // query speed for cceh
    Time_BODY(test_case[4], "cceh positive_get", { cceh->get(mykey[i]); })

    // query speed for fast&fair
    Time_BODY(test_case[5], "fast&fair positive_get", { ff->get(mykey[i]); })

    // query speed for roart
    Time_BODY(test_case[6], "roart positive_get", { roart->get(mykey[i]); })

//    // query speed for ht
//    Time_BODY(test_case[0], "ET negative_get", { ht->get(negative_key[i]); })
//
//    // query speed for art
//    Time_BODY(test_case[1], "arttree negative_get", { art_get(art, (unsigned char *) &(negative_key[i]), 8); })
//
//    // query speed for wort
//    Time_BODY(test_case[2], "wort negative_get", { wort_get(wort, negative_key[i], 8); })
//
//    // query speed for woart
//    Time_BODY(test_case[3], "woart negative_get", { woart_get(woart, negative_key[i], 8); })
//
//    // query speed for cceh
//    Time_BODY(test_case[4], "cceh negative_get", { cceh->get(negative_key[i]); })
//
//    // query speed for fast&fair
//    Time_BODY(test_case[5], "fast&fair negative_get", { ff->get(negative_key[i]); })
//
//    // query speed for fast&fair
//    Time_BODY(test_case[6], "roart negative_get", { roart->get(negative_key[i]); })
//
//    value = 2;
//    // insert speed for ht
//    Time_BODY(test_case[0], "ET update", { ht->crash_consistent_put(NULL, mykey[i], 2, 0); })
//
//    // insert speed for art
//    Time_BODY(test_case[1], "art tree update", { art_put(art, (unsigned char *) &(mykey[i]), 8, &value); })
//
//    // insert speed for wort
//    Time_BODY(test_case[2], "wort update", { wort_put(wort, mykey[i], 8, &value); })
//
//    // insert speed for woart
//    Time_BODY(test_case[3], "woart update", { woart_put(woart, mykey[i], 8, &value); })
//
//    // insert speed for cceh
//    Time_BODY(test_case[4], "cceh update", { cceh->put(mykey[i], value); })
//
//    // insert speed for fast&fair
//    Time_BODY(test_case[5], "fast&fair update", { ff->put(mykey[i], (char *) &value); })
//
//    // insert speed for fast&fair
//    Time_BODY(test_case[6], "roart update", { roart->put(mykey[i], value); })

    //range query speed for ht
    Time_BODY1(range_query_test_case[0], "ET range query ", { ht->scan(mykey[i], mykey[i] + 1000); })

    //range query speed for wort
    Time_BODY1(range_query_test_case[1], "wort range query ", { wort_scan(wort, mykey[i], mykey[i] + 1000); })

    //range query speed for woart
    Time_BODY1(range_query_test_case[2], "woart range query ", { woart_scan(woart, mykey[i], mykey[i] + 1000); })

    //range query speed for fast&fair
    Time_BODY1(range_query_test_case[3], "fast&fair range query ", { ff->scan(mykey[i], mykey[i] + 1000); })

    out.close();
}

void correctnessTest() {
    map<uint64_t, uint64_t> mm;
    mykey = new uint64_t[testNum];
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


void *concurrency_put_with_thread(int threadNum, concurrencyhashtree *cht) {

    try {
        timeval start, ends;
        gettimeofday(&start, NULL);

        // mtx.try_lock();  
        // cout<<threadNum<<endl<<(uint64_t)(cht->root)<<endl;
        // mtx.unlock();
        for (int i = 0; i < testNum; ++i) {
            cht->crash_consistent_put(NULL, 900, 1, 0);
        }

        gettimeofday(&ends, NULL);
        double timeCost = (ends.tv_sec - start.tv_sec) * 1000000 + ends.tv_usec - start.tv_usec;
        double throughPut = (double) testNum / timeCost;

        mtx.try_lock();
        cout << "concurrnecy hash tree put" + to_string(threadNum) << " " << testNum << " kv pais in "
             << timeCost / 1000000 << " s" << endl;
        cout << "concurrnecy hash tree put" + to_string(threadNum) << " " << "ThroughPut: " << throughPut << " Mops"
             << endl;
        mtx.unlock();
    } catch (const char *msg) {
        cout << msg << endl;
    }

}

void concurrencyTest(concurrencyhashtree *cht) {
    // mykey = new uint64_t[testNum];
    // rng r;
    // rng_init(&r, 1, 2);
    // for (int i = 0; i < testNum; ++i) {
    //     mykey[i] = rng_next(&r);
    // }

    try {
        // concurrency_put_with_thread(1);
        for (int i = 0; i < numThread; i++) {
            threads[i] = new std::thread(concurrency_put_with_thread, i, cht);
        }
        // concurrency_put_with_thread((void*)1);
    } catch (const char *error) {
        cout << error << endl;
    }
    // out.close();

}


void test_key_len() {

    for (int key_len = 12; key_len <= 64; key_len += 2) {
        cceh = new_cceh(0, key_len);
        //generate skewed data set
        mykey = new uint64_t[testNum];
        rng_init(&r, 1, 2);
        for (int i = 0; i < testNum; ++i) {
            mykey[i] = (rng_next(&r) % testNum) % (0x1ull << (key_len - 1));
//            mykey[i] = (rng_next(&r)) % (0x1ull << (key_len - 1));
        }
        uint64_t value = 1;

        timeval start, ends;
        gettimeofday(&start, NULL);
        for (int i = 0; i < testNum; ++i) {
            cceh->put(mykey[i], value);
        }
        gettimeofday(&ends, NULL);
        double timeCost = (ends.tv_sec - start.tv_sec) * 1000000 + ends.tv_usec - start.tv_usec;
        double throughPut = (double) testNum / timeCost;
        cout << key_len << " ," << throughPut << endl;
    }
}


void recoveryTest() {
    mykey = new uint64_t[testNum];
    rng r;
    rng_init(&r, 1, 2);
    for (int i = 0; i < testNum; ++i) {
        mykey[i] = rng_next(&r);
    }

    for (int i = 0; i < testNum; ++i) {
        ht->crash_consistent_put(NULL, mykey[i], 1, 0);
    }


    timeval start, ends;
    gettimeofday(&start, NULL);
    recovery(ht->getRoot());
    gettimeofday(&ends, NULL);
    double timeCost = (ends.tv_sec - start.tv_sec) * 1000000 + ends.tv_usec - start.tv_usec;

    cout << "ht recovery test" << testNum << " kv pais in " << timeCost / 1000000 << " s" << endl;


}

int main(int argc, char *argv[]) {
    sscanf(argv[1], "%d", &numThread);
    sscanf(argv[2], "%d", &testNum);
    init_fast_allocator();
    ht = new_hashtree(64, 4);
    art = new_art_tree();
    wort = new_wort_tree();
    woart = new_woart_tree();
    cceh = new_cceh();
    ff = new_fastfair();
    roart = new_roart();
//    mt = new_mass_tree();
//    bt = new_blink_tree(numThread);
//    concurrencyhashtree *cht = new_concurrency_hashtree(64, 0);


//  test_key_len();
//    correctnessTest();
    speedTest();


//    recoveryTest();
//    profile();
//    range_query_correctness_test();
//    cout << ht->node_cnt << endl;
//    cout << ht->get_access << endl;
    fast_free();
    return 0;
}