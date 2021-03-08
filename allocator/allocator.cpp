/**
 *    author:     UncP
 *    date:    2018-11-23
 *    license:    BSD-3
**/

#define _POSIX_C_SOURCE 200112L

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <pthread.h>
#include <fcntl.h>
#include <thread>
#include <errno.h>
#include <unistd.h> // for close


#include "allocator.h"

#define MAP_SYNC 0x080000 /* perform synchronous page faults for the mapping */
#define MAP_SHARED_VALIDATE 0x03    /* share + validate extension flags */
#define MAP_ANONYMOUS        0x20

uint64_t FILE_SIZE = 1UL << 24;

static pthread_key_t key;
static int initialized = 0;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static void *block_alloc(block *b, size_t size, int *success) {
    if (likely((b->now + size) <= b->tot)) {
        *success = 1;
        void *ptr = (char *) b->buffer + b->now;
        b->now += size;
        return ptr;
    } else {
        *success = 0;
        return 0;
    }
}

static block *new_block(allocator *a, bool _on_nvm = false) {
    size_t s = (sizeof(block) + 63) & (~((size_t) 63));
    int success;
    block *b;
    if (_on_nvm) {
        b = static_cast<block *>(block_alloc(a->meta_curr_nvm, s, &success));
    } else {
        b = static_cast<block *>(block_alloc(a->meta_curr, s, &success));
    }
    if (likely(success)) {
#ifdef __linux__
        uint64_t tid = static_cast<uint64_t>(pthread_self()) % 10000;
#else
        uint64_t tid;
        pthread_threadid_np(0, &tid);
        tid = tid % 10000;
//        b->buffer = mmap(0, block_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
#endif
        char tid_str[10] = {0};
        sprintf(tid_str, "%lu", tid);
        if (_on_nvm) {
            char storage_filename[50] = "/aepmount/block/";
            char file_suffix[4] = {0};
            uint64_t offset = block_size * a->block_num_nvm;
            if (offset + block_size > FILE_SIZE) {
                a->block_num_nvm = 0;
                offset = 0;
                a->block_nvm_file_num++;
            }
            sprintf(file_suffix, "%lu", a->block_nvm_file_num);
            strcat(storage_filename, tid_str);
            strcat(storage_filename, "-");
            strcat(storage_filename, file_suffix);
            int fd;
            if (access(storage_filename, 0) == 0) {
                fd = open(storage_filename, O_CREAT | O_RDWR, 0644);
            } else {
                //文件不存在，就创建一个FILE SIZE大小的文件
                fd = open(storage_filename, O_CREAT | O_RDWR, 0644);
//            printf("%s,%d\n", storage_filename, fd);
                lseek(fd, (FILE_SIZE) - 1, SEEK_SET);
                write(fd, "1", 1);
            }
            b->buffer = (char *) mmap(NULL, block_size, PROT_READ | PROT_WRITE, MAP_SYNC | MAP_SHARED_VALIDATE, fd,
                                      offset);// MAP_SYNC only valid for PM
            a->block_num_nvm++;
            close(fd);
        } else {
//#ifdef __linux__
//            char storage_filename[50] = "/home/wangke/nvmkv/block/";
//#else
//            char storage_filename[50] = "/Users/wangke/CLionProjects/nvmkv/block/";
//#endif
//            char file_suffix[4] = {0};
//            uint64_t offset = block_size * a->block_num;
//            if (offset + block_size > FILE_SIZE) {
//                a->block_num = 0;
//                offset = 0;
//                a->block_file_num++;
//            }
//            sprintf(file_suffix, "%lu", a->block_file_num);
//            strcat(storage_filename, tid_str);
//            strcat(storage_filename, "-");
//            strcat(storage_filename, file_suffix);
////            printf("%s\n", storage_filename);
//            int fd = open(storage_filename, O_CREAT | O_RDWR, 0644);
//            lseek(fd, (FILE_SIZE) - 1, SEEK_SET);
//            write(fd, "1", 1);
//            b->buffer = (char *) mmap(NULL, block_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd,
//                                      offset);
//            a->block_num++;
//            close(fd);
            b->buffer = malloc(block_size);
        }
        assert(b->buffer != MAP_FAILED);
        b->now = 0;
        b->tot = block_size;
        b->next = 0;
        return b;
    }
    return 0;
}

static block *new_meta_block(allocator *a, bool _on_nvm = false) {
    void *buf;
#ifdef __linux__
    uint64_t tid = static_cast<uint64_t>(pthread_self()) % 10000;
#else
    uint64_t tid;
    pthread_threadid_np(0, &tid);
    tid = tid % 10000;
#endif
    char tid_str[10] = {0};
    sprintf(tid_str, "%lu", tid);
    if (_on_nvm) {
        char storage_filename[50] = "/aepmount/meta/";
        char file_suffix[4] = {0};
        uint64_t offset = meta_block_size * a->meta_num_nvm;
        if (offset + meta_block_size > FILE_SIZE) {
            a->meta_num_nvm = 0;
            offset = 0;
            a->meta_nvm_file_num++;
        }
        sprintf(file_suffix, "%lu", a->meta_nvm_file_num);
        strcat(storage_filename, file_suffix);
        strcat(storage_filename, "-");
        strcat(storage_filename, tid_str);
        int fd = open(storage_filename, O_CREAT | O_RDWR, 0644);
        lseek(fd, (FILE_SIZE) - 1, SEEK_SET);
        write(fd, "1", 1);
        buf = (char *) mmap(NULL, meta_block_size, PROT_READ | PROT_WRITE, MAP_SYNC | MAP_SHARED_VALIDATE, fd,
                            offset);
        a->meta_num_nvm++;
        close(fd);
    } else {
//#ifdef __linux__
//        char storage_filename[50] = "/home/wangke/nvmkv/meta/";
//#else
//        char storage_filename[50] = "/Users/wangke/CLionProjects/nvmkv/meta/";
//#endif
//        char file_suffix[4] = {0};
//        uint64_t offset = meta_block_size * a->meta_num;
//        if (offset + meta_block_size > FILE_SIZE) {
//            a->meta_num = 0;
//            offset = 0;
//            a->meta_file_num++;
//        }
//        sprintf(file_suffix, "%lu", a->meta_file_num);
//        strcat(storage_filename, file_suffix);
//        strcat(storage_filename, "-");
//        strcat(storage_filename, tid_str);
//        int fd = open(storage_filename, O_CREAT | O_RDWR, 0644);
//        lseek(fd, (FILE_SIZE) - 1, SEEK_SET);
//        write(fd, "1", 1);
//        buf = (char *) mmap(NULL, meta_block_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd,
//                            offset);
//        a->meta_num++;
//        close(fd);
        buf = malloc(meta_block_size);
    }
////    char *buf = mmap(NULL, meta_block_size, PROT_READ | PROT_WRITE, MAP_SYNC | MAP_SHARED_VALIDATE, nvmFd, 0);
//    printf("\n\nerrno=%d\n\n", errno);
    assert(buf != MAP_FAILED);

    block *m = (block *) buf;
    m->buffer = (void *) buf;
    m->now = (sizeof(block) + 63) & (~((size_t) 63));
    m->tot = meta_block_size;
    m->next = 0;
    return m;
}

static void free_block(block *b) {
    munmap(b->buffer, b->tot);
}

void destroy_allocator(void *arg) {
    // for test reason don't dealloc memory when this thread exit,
    // memory will be munmaped when process exit
    return;
//
//    allocator *a = (allocator *) arg;
//
//    block *curr = a->curr;
//    while (curr) {
//        block *next = curr->next;
//        free_block(curr);
//        curr = next;
//    }
//
//    block *small_curr = a->small_curr;
//    while (small_curr) {
//        block *next = small_curr->next;
//        free_block(small_curr);
//        small_curr = next;
//    }
//
//    curr = a->meta_curr;
//    while (curr) {
//        block *next = curr->next;
//        free_block(curr);
//        curr = next;
//    }
//
//    free((void *) a);
}

void init_allocator() {
    pthread_mutex_lock(&mutex);
    if (initialized == 0) {
        assert(pthread_key_create(&key, destroy_allocator) == 0);
        initialized = 1;
    }
    pthread_mutex_unlock(&mutex);
}

static void init_thread_allocator() {
    allocator *a = new allocator;
    assert(posix_memalign((void **) &a, 64, sizeof(allocator)) == 0);
    a->meta_num = 0;
    a->block_num = 0;
    a->meta_num_nvm = 0;
    a->block_num_nvm = 0;
    a->meta_file_num = 0;
    a->meta_nvm_file_num = 0;
    a->block_file_num = 0;
    a->block_nvm_file_num = 0;
    block *meta = new_meta_block(a, false);
    a->meta_curr = meta;

    block *curr = new_block(a, false);
    assert(curr);
    a->curr = curr;

#ifdef __linux__
    block *meta_nvm = new_meta_block(a, true);
    a->meta_curr_nvm = meta_nvm;

    block *curr_nvm = new_block(a, true);
    assert(curr_nvm);
    a->curr_nvm = curr_nvm;
#endif

//    block *small_curr = new_block(a);
//    assert(small_curr);
//    a->small_curr = small_curr;

    assert(pthread_setspecific(key, (void *) a) == 0);
}

static allocator *get_thread_allocator() {
    allocator *a;

#ifdef __linux__
    a = (allocator *)pthread_getspecific(key);
    if (unlikely(a == 0)) {
      init_thread_allocator();
      a = (allocator *)pthread_getspecific(key);
      assert(a);
    }
#else
    a = static_cast<allocator *>(pthread_getspecific(key));
    if (unlikely(a == 0)) {
        init_thread_allocator();
        assert((a = static_cast<allocator *>(pthread_getspecific(key))));
    }
#endif

    return a;
}

void *allocator_alloc(size_t size, bool _on_nvm) {
    allocator *a = get_thread_allocator();

    // cache line alignment
    if (_on_nvm) {
        size = (size + 255) & (~((size_t) 255));
        int success;
        void *ptr = block_alloc(a->curr_nvm, size, &success);
        if (unlikely(success == 0)) {
            block *_new_block = new_block(a, _on_nvm);
            if (unlikely(_new_block == 0)) {
                block *meta = new_meta_block(a, _on_nvm);
                assert(meta);
                meta->next = a->meta_curr_nvm;
                a->meta_curr_nvm = meta;
                _new_block = new_block(a, _on_nvm);
//                if (_new_block == 0)
//                    printf("%d, %d\n", a->block_nvm_file_num, a->meta_num_nvm);
                assert(_new_block);
            }
            _new_block->next = a->curr_nvm;
            a->curr_nvm = _new_block;
            ptr = block_alloc(a->curr_nvm, size, &success);
        }
        assert(success);
        return ptr;
    } else {
        size = (size + 63) & (~((size_t) 63));
        int success;
        void *ptr = block_alloc(a->curr, size, &success);
        if (unlikely(success == 0)) {
            block *_new_block = new_block(a, _on_nvm);
            if (unlikely(_new_block == 0)) {
                block *meta = new_meta_block(a, _on_nvm);
                meta->next = a->meta_curr;
                a->meta_curr = meta;
                _new_block = new_block(a, _on_nvm);
                assert(_new_block);
            }
            _new_block->next = a->curr;
            a->curr = _new_block;
            ptr = block_alloc(a->curr, size, &success);
        }
        assert(success);
        return ptr;
    }
}

//void *allocator_alloc_small(size_t size) {
//    allocator *a = get_thread_allocator();
//
//    // 8 bytes alignment
//    size = (size + 8) & (~((size_t) 8));
//
//    int success;
//    void *ptr = block_alloc(a->small_curr, size, &success);
//    if (unlikely(success == 0)) {
//        block *_new_block = new_block(a);
//        if (unlikely(_new_block == 0)) {
//            block *meta = new_meta_block(a);
//            meta->next = a->meta_curr;
//            a->meta_curr = meta;
//            _new_block = new_block(a);
//            assert(_new_block);
//        }
//        _new_block->next = a->small_curr;
//        a->small_curr = _new_block;
//        ptr = block_alloc(a->small_curr, size, &success);
//    }
//    assert(success);
//    return ptr;
//}

void allocator_free(void *ptr) {
    // do nothing
    (void) ptr;
}
