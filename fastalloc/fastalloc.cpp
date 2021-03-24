//
// Created by 王柯 on 2021-03-24.
//

#include "fastalloc.h"

fastalloc::fastalloc() {}

void fastalloc::init() {
    dram[dram_cnt] = new char[block_size];
    dram_curr = dram[dram_cnt];
    dram_total = block_size;
    dram_left = block_size;
    dram_cnt++;
}

void *fastalloc::alloc(uint64_t size, bool _on_nvm) {
    if (unlikely(size > dram_left)) {
        dram[dram_cnt] = new char[block_size];
        dram_curr = dram[dram_cnt];
        dram_total = block_size;
        dram_left = block_size;
        dram_cnt++;
        dram_left -= size;
        void *tmp = dram_curr;
        dram_curr = dram_curr + size;
        return tmp;
    } else {
        dram_left -= size;
        void *tmp = dram_curr;
        dram_curr = dram_curr + size;
        return tmp;
    }
}

void fastalloc::free() {
    if (dram != NULL) {
        dram_total = 0;
        dram_left = 0;
        for (int i = 0; i < dram_cnt; ++i) {
            delete[]dram[i];
        }
        dram_curr = NULL;
    }
}