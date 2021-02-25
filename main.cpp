#include <iostream>
#include <unistd.h>
#include <stdio.h>
#include <map>
#include <sys/time.h>
#include <pthread.h>
#include "blink/blink_tree.h"
#include "art/art.h"
#include "mass/mass_tree.h"
#include "rng/rng.h"

using namespace std;

int testNum = 10000000;

int numThread = 1;

union {
    blink_tree *bt;
    mass_tree *mt;
    adaptive_radix_tree *art;
} mytree;

//blink_tree *mytree;
//adaptive_radix_tree *mytree;
//mass_tree *mytree;

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
//        *(uint64_t *)(key + 1)= rand();
//        adaptive_radix_tree_put(mytree, (const void *)(key + 1), 8);
        int x = rng_next(&r);
        int y = x + 1;
//        blink_tree_write(mytree, &x, sizeof(int), &x);
        mass_tree_put(mytree.mt, &x, 8, &y, sizeof(y));
//        int *res = static_cast<int *>(mass_tree_get(mytree.mt, &x, 8));
//        cout << x << ": " << *(res+1) << endl;
    }
}

void speedTest() {
    srand(time(0));
    timeval start, ends;
    gettimeofday(&start, NULL);
//    put();
    pthread_t *tids = new pthread_t[numThread];
    for (int i = 0; i < numThread; ++i) {
        int ret = pthread_create(&tids[i], NULL, &putFunc, NULL);
        if (ret != 0) {
            cout << "pthread_create error: error_code=" << ret << endl;
        }
    }
    for (int j = 0; j < numThread; ++j) {
        pthread_join(tids[j], NULL);
    }
    gettimeofday(&ends, NULL);
    double timeCost = (ends.tv_sec - start.tv_sec) * 1000000 + ends.tv_usec - start.tv_usec;
    double throughPut = (double) testNum / timeCost;
//    cout << "Total Put " << testNum << " kv pais in " << timeCost / 1000000 << " s" << endl;
//    cout << "Total Put ThroughPut: " << throughPut << " Mops" << endl;
    cout << throughPut << endl;
}


int main(int argc, char *argv[]) {
    sscanf(argv[1], "%d", &numThread);
    sscanf(argv[2], "%d", &testNum);
    mytree.mt = new_mass_tree();
//    mytree.art = new_adaptive_radix_tree();
//    mytree.bt = new_blink_tree(numThread);
//    cout << testPut() << endl;
//    cout << testUpdate() << endl;
//    cout << testDel() << endl;
    speedTest();
    return 0;
}