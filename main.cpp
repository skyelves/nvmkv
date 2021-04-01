#include <iostream>
#include <unistd.h>
#include <stdio.h>
#include <map>
#include <string.h>
#include <sys/time.h>
#include <pthread.h>
#include "blink/blink_tree.h"
#include "cart/cart.h"
#include "mass/mass_tree.h"
#include "hashtree/hashtree.h"
#include "extendibleHash/extendible_hash.h"
#include "rng/rng.h"
#include "art/art.h"
#include "wort/wort.h"

using namespace std;

#define Time_BODY(condition ,name, func)                                                        \
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

int testNum = 100000;

int numThread = 1;

int test_algorithms_num = 5;
bool test_case[10] = {0, // ht
                      0, // art
                      0, // cart
                      0, // eh
                      1, // wort
                      0};

blink_tree *bt;
mass_tree *mt;
concurrent_adaptive_radix_tree *cart;
hashtree *ht;
extendible_hash *eh;
art_tree *art;
wort_tree *wort;


uint64_t *mykey;

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
        mykey[i] = rng_next(&r);
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
    Time_BODY(test_case[0], "hash tree put ", { ht->put(mykey[i], 1); })

    // insert speed for art
    Time_BODY(test_case[1], "art tree put ", { art_put(art, (unsigned char *) &(mykey[i]), 8, &value); })

    // insert speed for cart
    Time_BODY(test_case[2], "cart tree put ", { concurrent_adaptive_radix_tree_put(cart, &(mykey[i]), 8, &value, 8); })

    // insert speed for extendible hash
    Time_BODY(test_case[3], "eh put ", { eh->put(mykey[i], 1); })

    // insert speed for wort
    Time_BODY(test_case[4], "wort put ", { wort_put(wort, mykey[i], 8, &value); })

    // query speed for ht
    Time_BODY(test_case[0], "hash tree get ", { ht->get(mykey[i]); })

    // query speed for art
    Time_BODY(test_case[1], "art tree get ", { art_get(art, (unsigned char *) &(mykey[i]), 8); })

    // query speed for art
    Time_BODY(test_case[2], "cart tree get ", { concurrent_adaptive_radix_tree_get(cart, &(mykey[i]), 8); })

    // query speed for extendible hash
    Time_BODY(test_case[3], "eh get ", { eh->get(mykey[i]); })

    // query speed for wort
    Time_BODY(test_case[4], "wort get ", { wort_get(wort, mykey[i], 8); })

}

void correctnessTest() {
    map<uint64_t, uint64_t> mm;
    mykey = new uint64_t[testNum];
    rng r;
    rng_init(&r, 1, 2);
    for (int i = 0; i < testNum; ++i) {
        mykey[i] = rng_next(&r);
        mm[mykey[i]] = i + 1;
    }

    for (int i = 0; i < testNum; ++i) {
        ht->put(mykey[i], i + 1);
    }

    int64_t res = 0;
    for (int i = 0; i < testNum; ++i) {
        res = ht->get(mykey[i]);
        if (res != mm[mykey[i]]) {
            cout << i << ", " << mykey[i] << ", " << res << endl;
//            return;
        }
    }
//    cout << 1 << endl;
    return;

}


int main(int argc, char *argv[]) {
    sscanf(argv[1], "%d", &numThread);
    sscanf(argv[2], "%d", &testNum);
    init_fast_allocator();
    ht = new_hashtree(32, 0);
    eh = new_extendible_hash(0, 64);
    art = new_art_tree();
    cart = new_concurrent_adaptive_radix_tree();
    wort = new_wort_tree();
//    mt = new_mass_tree();
//    bt = new_blink_tree(numThread);
    speedTest();
//    correctnessTest();
    cout << ht->node_cnt << endl;
    cout << ht->get_access << endl;
    fast_free();
    return 0;
}