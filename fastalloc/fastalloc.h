//
// Created by 王柯 on 2021-03-24.
//

#include <iostream>
#include <stdio.h>
#include <cstdlib>
#include <fcntl.h>
#include <string>
#include <atomic>
#include <thread>
#include <sys/mman.h>

#ifndef NVMKV_FASTALLOC_H
#define NVMKV_FASTALLOC_H

#define ALLOC_SIZE ((size_t)4<<30) // 4GB
#define CONCURRENCY_ALLOC_SIZE ((size_t)4<<27)  
#define CACHELINESIZE (64)


#define ALLOCATORNUM 64

#define MAP_SYNC 0x080000 /* perform synchronous page faults for the mapping */
#define MAP_SHARED_VALIDATE 0x03    /* share + validate extension flags */

#define likely(x)   (__builtin_expect(!!(x), 1))
#define unlikely(x) (__builtin_expect(!!(x), 0))
#define cas(_p, _u, _v)  (__atomic_compare_exchange_n (_p, _u, _v, false, __ATOMIC_ACQUIRE, __ATOMIC_ACQUIRE))

using namespace std;

class fastalloc {

public:

    bool is_used = false;
    char *dram[100];
    char *dram_curr = NULL;
    uint64_t dram_left = 0;
    uint64_t dram_cnt = 0;

    char *nvm[100];
    char *nvm_curr = NULL;
    uint64_t nvm_left = 0;
    uint64_t nvm_cnt = 0;

    fastalloc();

    virtual void init();

    virtual void *alloc(uint64_t size, bool _on_nvm = true);

    virtual void free();
};


class concurrency_fastalloc :public fastalloc {

    public:

        atomic<bool> mtx = false;

        void init();

        void * alloc(uint64_t size, bool _on_nvm = true);

        void lock();
        
        void free_lock();

};

void init_fast_allocator(bool isMultiThread);

void concurrency_init_fast_allocator();

void *fast_alloc(uint64_t size, bool _on_nvm = true);

void *concurrency_fast_alloc(uint64_t size, bool _on_nvm = true);

void fast_free();


#endif //NVMKV_FASTALLOC_H
