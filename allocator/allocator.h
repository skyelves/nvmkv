/**
 *    author:     UncP
 *    date:    2018-11-23
 *    license:    BSD-3
**/

#ifndef _allocator_h_
#define _allocator_h_

// this allocator is used to allocate small size memory(typically <=4kb) in a fast way,
// an allocator is obtained by one thread only so no thread synchronization is needed,
// the memory allocated is always vaild until the allocator itself is destroyed

#define meta_block_size ((size_t)4 << 10) // 4kb
// TODO: linux huge page advise?
#define block_size      ((size_t)4 << 20) // 4mb

#define likely(x)   (__builtin_expect(!!(x), 1))
#define unlikely(x) (__builtin_expect(!!(x), 0))

typedef struct block {
    void *buffer;
    size_t now;
    size_t tot;

    struct block *next;
} block;

typedef struct allocator {
    block *meta_curr = nullptr;
    block *curr = nullptr;
//    block *small_curr = nullptr;
    block *meta_curr_nvm = nullptr;
    block *curr_nvm = nullptr;
//    block *small_curr_nvm = nullptr;
    size_t meta_num = 0;
    size_t block_num = 0;
    size_t meta_num_nvm = 0;
    size_t block_num_nvm = 0;
    size_t meta_file_num = 0;
    size_t meta_nvm_file_num = 0;
    size_t block_file_num = 0;
    size_t block_nvm_file_num = 0;
} allocator;

void init_allocator();

void *allocator_alloc(size_t size, bool _on_nvm = false);

//void *allocator_alloc_small(size_t size);

void allocator_free(void *ptr);

#endif /* _allocator_h_ */