//
// Created by 王柯 on 2021-03-24.
//

#include <iostream>
#include <stdio.h>
#include <cstdlib>
#include <fcntl.h>
#include <string>
#include <sys/mman.h>

#ifndef NVMKV_FASTALLOC_H
#define NVMKV_FASTALLOC_H

#define ALLOC_SIZE ((size_t)4<<30) // 4GB
#define CACHELINESIZE (64)

#define MAP_SYNC 0x080000 /* perform synchronous page faults for the mapping */
#define MAP_SHARED_VALIDATE 0x03    /* share + validate extension flags */

#define likely(x)   (__builtin_expect(!!(x), 1))
#define unlikely(x) (__builtin_expect(!!(x), 0))

using namespace std;

class fastalloc {
    char *dram[100];
    char *dram_curr = NULL;
    uint64_t dram_left = 0;
    uint64_t dram_cnt = 0;

    char *nvm[100];
    char *nvm_curr = NULL;
    uint64_t nvm_left = 0;
    uint64_t nvm_cnt = 0;

public:
    fastalloc();

    void init();

    void *alloc(uint64_t size, bool _on_nvm = true);

    void free();

    uint64_t profile(bool _on_nvm= true);
};

void init_fast_allocator();

void *fast_alloc(uint64_t size, bool _on_nvm = true);

void fast_free();

uint64_t fastalloc_profile();


#endif //NVMKV_FASTALLOC_H
