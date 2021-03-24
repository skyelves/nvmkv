//
// Created by 王柯 on 2021-03-24.
//

#include <stdio.h>
#include <cstdlib>

#ifndef NVMKV_FASTALLOC_H
#define NVMKV_FASTALLOC_H

#define block_size ((size_t)4<<30) // 4GB

#define likely(x)   (__builtin_expect(!!(x), 1))
#define unlikely(x) (__builtin_expect(!!(x), 0))

class fastalloc {
    char *dram[100];
    char *dram_curr = NULL;
    uint64_t dram_total = 0;
    uint64_t dram_left = 0;
    uint64_t dram_cnt = 0;

    fastalloc();

    void init();

    void *alloc(uint64_t size, bool _on_nvm = false);

    void free();
};


#endif //NVMKV_FASTALLOC_H
