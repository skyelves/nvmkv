//
// Created by 王柯 on 2021-03-24.
//

#include "fastalloc.h"

fastalloc *myallocator;
thread_local concurrency_fastalloc* concurrency_myallocator;

fastalloc::fastalloc() {}

void fastalloc::init() {
    dram[dram_cnt] = new char[ALLOC_SIZE];
    dram_curr = dram[dram_cnt];
    dram_left = ALLOC_SIZE;
    dram_cnt++;

#ifdef __linux__
    string nvm_filename = "/mnt/aep1/test";
    nvm_filename = nvm_filename + to_string(nvm_cnt);
    int nvm_fd = open(nvm_filename.c_str(), O_CREAT | O_RDWR, 0644);
    if (posix_fallocate(nvm_fd, 0, ALLOC_SIZE) < 0)
        puts("fallocate fail\n");
    nvm[nvm_cnt] = (char *) mmap(NULL, ALLOC_SIZE, PROT_READ | PROT_WRITE, MAP_SYNC | MAP_SHARED_VALIDATE, nvm_fd, 0);
#else
    nvm[nvm_cnt] = new char[ALLOC_SIZE];
#endif
    nvm_curr = nvm[nvm_cnt];
    nvm_left = ALLOC_SIZE;
    nvm_cnt++;
}

void concurrency_fastalloc::init() {
    dram[dram_cnt] = new char[CONCURRENCY_ALLOC_SIZE];
    dram_curr = dram[dram_cnt];
    dram_left = CONCURRENCY_ALLOC_SIZE;
    dram_cnt++;

#ifdef __linux__
    std::thread::id this_id = std::this_thread::get_id();
    unsigned int t = *(unsigned int*)&this_id;// threadid 转成 unsigned int
    string nvm_filename = "/mnt/aep1/test"+to_string(t);
    nvm_filename = nvm_filename + to_string(nvm_cnt);
    int nvm_fd = open(nvm_filename.c_str(), O_CREAT | O_RDWR, 0644);
    if (posix_fallocate(nvm_fd, 0, CONCURRENCY_ALLOC_SIZE) < 0)
        puts("fallocate fail\n");
    nvm[nvm_cnt] = (char *) mmap(NULL, CONCURRENCY_ALLOC_SIZE, PROT_READ | PROT_WRITE, MAP_SYNC | MAP_SHARED_VALIDATE, nvm_fd, 0);
#else
    nvm[nvm_cnt] = new char[CONCURRENCY_ALLOC_SIZE];
#endif
    nvm_curr = nvm[nvm_cnt];
    nvm_left = CONCURRENCY_ALLOC_SIZE;
    nvm_cnt++;
}

void *fastalloc::alloc(uint64_t size, bool _on_nvm) {
    size = size / 64 * 64 + (!!(size % 64)) * 64;
    if (_on_nvm) {
        if (unlikely(size > nvm_left)) {
#ifdef __linux__
            string nvm_filename = "/mnt/aep1/test";
            nvm_filename = nvm_filename + to_string(nvm_cnt);
            int nvm_fd = open(nvm_filename.c_str(), O_CREAT | O_RDWR, 0644);
            if (posix_fallocate(nvm_fd, 0, ALLOC_SIZE) < 0)
                puts("fallocate fail\n");
            nvm[nvm_cnt] = (char *) mmap(NULL, ALLOC_SIZE, PROT_READ | PROT_WRITE, MAP_SYNC | MAP_SHARED_VALIDATE, nvm_fd, 0);
#else
            nvm[nvm_cnt] = new char[ALLOC_SIZE];
#endif
            nvm_curr = nvm[nvm_cnt];
            nvm_left = ALLOC_SIZE;
            nvm_cnt++;
            nvm_left -= size;
            void *tmp = nvm_curr;
            nvm_curr = nvm_curr + size;
            return tmp;
        } else {
            nvm_left -= size;
            void *tmp = nvm_curr;
            nvm_curr = nvm_curr + size;
            return tmp;
        }
    } else {
        if (unlikely(size > dram_left)) {
            dram[dram_cnt] = new char[ALLOC_SIZE];
            dram_curr = dram[dram_cnt];
            dram_left = ALLOC_SIZE;
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
}


void fastalloc::free() {
    if (dram != NULL) {
        dram_left = 0;
        for (int i = 0; i < dram_cnt; ++i) {
            delete[]dram[i];
        }
        dram_curr = NULL;
    }
}

uint64_t fastalloc::profile(bool _on_nvm) {
    if (_on_nvm)
        return nvm_cnt * ALLOC_SIZE - nvm_left;
    else
        return dram_cnt * ALLOC_SIZE - dram_left;
}

void init_fast_allocator(bool isMultiThread) {
    if(isMultiThread){
        concurrency_myallocator = new concurrency_fastalloc;
        concurrency_myallocator->init();
    }else{
        myallocator = new fastalloc;
        myallocator->init();
    }
}

void *fast_alloc(uint64_t size, bool _on_nvm) {
    return concurrency_myallocator->alloc(size, _on_nvm);
}

void *concurrency_fast_alloc(uint64_t size, bool _on_nvm){
    return concurrency_myallocator->alloc(size, _on_nvm);
}

void fast_free() {
    if(myallocator!=NULL){
        myallocator->free();
        delete myallocator;
    }
    
    if(concurrency_myallocator!=NULL){
        concurrency_myallocator->free();
        delete concurrency_myallocator;
    }
#ifdef __linux__
    system("rm /mnt/aep1/test*");
#endif
}


uint64_t fastalloc_profile() {
    if (myallocator != NULL)
        return myallocator->profile();
}

uint64_t concurrency_fastalloc_profile() {
    if (concurrency_myallocator != NULL)
        return concurrency_myallocator->profile();
    else
        return -1;
}