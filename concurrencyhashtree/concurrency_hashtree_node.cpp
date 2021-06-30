//
// Created by 杨冠群 on 2021-06-20.
//

#include <stdio.h>
#include "concurrency_hashtree_node.h"

#ifdef HT_PROFILE_TIME
timeval start_time, end_time;
uint64_t t1, t2, t3;
#endif

#ifdef HT_PROFILE_LOAD_FACTOR
uint64_t ht_seg_num = 0;
uint64_t ht_dir_num = 0;
#endif

#define GET_SEG_NUM(key, key_len, depth)  ((key>>(key_len-depth))&(((uint64_t)1<<depth)-1))
#define GET_BUCKET_NUM(key, bucket_mask_len) ((key)&(((uint64_t)1<<bucket_mask_len)-1))

inline void mfence(void) {
    asm volatile("mfence":: :"memory");
}

inline void clflush(char *data, size_t len) {
    volatile char *ptr = (char *) ((unsigned long) data & (~(CACHELINESIZE - 1)));
    mfence();
    for (; ptr < data + len; ptr += CACHELINESIZE) {
        asm volatile("clflush %0" : "+m" (*(volatile char *) ptr));
    }
    mfence();
}

concurrency_ht_key_value *new_concurrency_ht_key_value(uint64_t key, uint64_t value) {
    concurrency_ht_key_value *_new_key_value = static_cast<concurrency_ht_key_value *>(concurrency_fast_alloc(sizeof(concurrency_ht_key_value)));
    _new_key_value->type = 1;
    _new_key_value->key = key;
    _new_key_value->value = value;
    return _new_key_value;
}

uint64_t concurrency_ht_bucket::get(uint64_t key) {
    for (int i = 0; i < HT_BUCKET_SIZE; ++i) {
        if (key == counter[i].key) {
            return counter[i].value;
        }
    }
    return 0;
}

int concurrency_ht_bucket::find_place(uint64_t _key, uint64_t _key_len, uint64_t _depth) {
    // full: return -1
    // exists or not full: return index or empty counter
    int res = -1;
    for (int i = 0; i < HT_BUCKET_SIZE; ++i) {
        if (_key == counter[i].key) {
            return i;
        } else if ((res == -1) && counter[i].key == 0 && counter[i].value == 0) {
            res = i;
        } else if ((res == -1) &&
                   (GET_SEG_NUM(_key, _key_len, _depth) != GET_SEG_NUM(counter[i].key, _key_len, _depth))) {
            res = i;
        }
    }
    return res;
}

concurrency_ht_bucket *new_concurrency_ht_bucket(int _depth) {
    concurrency_ht_bucket *_new_bucket = static_cast<concurrency_ht_bucket *>(concurrency_fast_alloc(sizeof(concurrency_ht_bucket)));
    return _new_bucket;
}

bool concurrency_ht_bucket::read_lock(){
    int64_t val = lock_meta;
    while(val>-1){
        if(cas(&lock_meta, &val, val+1)){
            return true;
        }
        val = lock_meta;
    }
    return false;
}

bool concurrency_ht_bucket::write_lock(){
    int64_t val = lock_meta;
    if(val<0){
        return false;
    }

    while(!cas(&lock_meta, &val, -1)){
        val = lock_meta;
        if(val<0){
            return false;
        }
    }

    while(val && lock_meta != 0-val-1){
        asm("nop");
    }

    return true;

}

void concurrency_ht_bucket::free_read_lock(){
    int64_t val = lock_meta;
    while(!cas(&lock_meta, &val, val-1)){
        val = lock_meta;
    }
}

void concurrency_ht_bucket::free_write_lock(){
    lock_meta = 0;
}

concurrency_ht_segment::concurrency_ht_segment() {
    depth = 0;
    bucket = static_cast<concurrency_ht_bucket *>(concurrency_fast_alloc(sizeof(concurrency_ht_bucket) * HT_MAX_BUCKET_NUM));
}

concurrency_ht_segment::~concurrency_ht_segment() {}

void concurrency_ht_segment::init(uint64_t _depth) {
    depth = _depth;
    bucket = static_cast<concurrency_ht_bucket *>(concurrency_fast_alloc(sizeof(concurrency_ht_bucket) * HT_MAX_BUCKET_NUM));
#ifdef HT_PROFILE_LOAD_FACTOR
    ht_seg_num++;
#endif
}

concurrency_ht_segment *new_concurrency_ht_segment(uint64_t _depth) {
    concurrency_ht_segment *_new_ht_segment = static_cast<concurrency_ht_segment *>(concurrency_fast_alloc(sizeof(concurrency_ht_segment)));
    _new_ht_segment->init(_depth);
    return _new_ht_segment;
}

bool concurrency_ht_segment::read_lock(){
    int64_t val = lock_meta;
    while(val>-1){
        if(cas(&lock_meta, &val, val+1)){
            return true;
        }
        val = lock_meta;
    }
    return false;
}

bool concurrency_ht_segment::write_lock(){
    int64_t val = lock_meta;
    if(val<0){
        return false;
    }

    while(!cas(&lock_meta, &val, -1)){
        val = lock_meta;
        if(val<0){
            return false;
        }
    }

    while(val && lock_meta != 0-val-1){
        asm("nop");
    }

    return true;
}

void concurrency_ht_segment::free_read_lock(){
    int64_t val = lock_meta;
    while(!cas(&lock_meta, &val, val-1)){
        val = lock_meta;
    }
}

void concurrency_ht_segment::free_write_lock(){
   lock_meta = 0;
}

concurrency_hashtree_node::concurrency_hashtree_node() {
    type = 0;
    global_depth = 0;
    key_len = 16;
    dir_size = pow(2, global_depth);
    dir = static_cast<concurrency_ht_segment **>(concurrency_fast_alloc(sizeof(concurrency_ht_segment *) * dir_size));
    for (int i = 0; i < dir_size; ++i) {
        dir[i] = new_concurrency_ht_segment();
    }
}

concurrency_hashtree_node::~concurrency_hashtree_node() {

}

void concurrency_hashtree_node::init(uint32_t _global_depth, int _key_len) {
    type = 0;
    global_depth = _global_depth;
    key_len = _key_len;
    dir_size = pow(2, global_depth);
    dir = static_cast<concurrency_ht_segment **>(concurrency_fast_alloc(sizeof(concurrency_ht_segment *) * dir_size));
    for (int i = 0; i < dir_size; ++i) {
        dir[i] = new_concurrency_ht_segment(_global_depth);
    }
#ifdef HT_PROFILE_LOAD_FACTOR
    ht_dir_num += dir_size;
#endif
}

void concurrency_hashtree_node::put(uint64_t key, uint64_t value) {

    RETRY:
    if(!read_lock()){
        std::this_thread::yield();
        goto RETRY;
    }

    // value should not be 0
    uint64_t dir_index = GET_SEG_NUM(key, key_len, global_depth);
    concurrency_ht_segment *tmp_seg = dir[dir_index];


    // get read lock
    if(!tmp_seg->read_lock()){
        free_read_lock();
        std::this_thread::yield();
        goto RETRY;
    }


    uint64_t seg_index = GET_BUCKET_NUM(key, HT_BUCKET_MASK_LEN);
    concurrency_ht_bucket *tmp_bucket = &(tmp_seg->bucket[seg_index]);

    if(!tmp_bucket->read_lock()){
        tmp_seg->free_read_lock();
        free_read_lock();
        std::this_thread::yield();
        goto RETRY;
    }

    int bucket_index = tmp_bucket->find_place(key, key_len, tmp_seg->depth);
    if (bucket_index == -1) {
        //condition: full
        if (likely(tmp_seg->depth < global_depth)) {
#ifdef HT_PROFILE_TIME
            gettimeofday(&start_time, NULL);
#endif
            uint64_t old_depth = tmp_seg->depth;
            // free seg read lock
            tmp_seg->free_read_lock();

            // get seg write lock
            if(!tmp_seg->write_lock()){
                tmp_bucket->free_read_lock();
                free_read_lock();
                std::this_thread::yield();
                goto RETRY;
            }

            if(tmp_seg->depth!=old_depth){
                tmp_bucket->free_read_lock();
                tmp_seg->free_write_lock();
                free_read_lock();
                std::this_thread::yield();
                goto RETRY;
            }


            concurrency_ht_segment *new_seg = new_concurrency_ht_segment(tmp_seg->depth + 1);
            //set dir [left,right)
            int64_t left = dir_index, mid = dir_index, right = dir_index + 1;
            for (int i = dir_index + 1; i < dir_size; ++i) {
                if (likely(dir[i] != tmp_seg)) {
                    right = i;
                    break;
                } else if (unlikely(i == (dir_size - 1))) {
                    right = dir_size;
                    break;
                }
            }
            for (int i = dir_index - 1; i >= 0; --i) {
                if (likely(dir[i] != tmp_seg)) {
                    left = i + 1;
                    break;
                } else if (unlikely(i == 0)) {
                    left = 0;
                    break;
                }
            }
            mid = (left + right) / 2;

            //migrate previous data to the new bucket
            for (int i = 0; i < HT_MAX_BUCKET_NUM; ++i) {
                uint64_t bucket_cnt = 0;
                for (int j = 0; j < HT_BUCKET_SIZE; ++j) {
                    bool tmp_type = tmp_seg->bucket[i].counter[j].type;
                    uint64_t tmp_key = tmp_seg->bucket[i].counter[j].key;
                    uint64_t tmp_value = tmp_seg->bucket[i].counter[j].value;
                    dir_index = GET_SEG_NUM(tmp_key, key_len, global_depth);
                    if (dir_index >= mid) {
                        concurrency_ht_segment *dst_seg = new_seg;
                        seg_index = i;
                        concurrency_ht_bucket *dst_bucket = &(dst_seg->bucket[seg_index]);
                        dst_bucket->counter[bucket_cnt].value = tmp_value;
                        dst_bucket->counter[bucket_cnt].key = tmp_key;
                        dst_bucket->counter[bucket_cnt].type = tmp_type;
                        bucket_cnt++;
                    }
                }
            }
            clflush((char *) new_seg, sizeof(concurrency_ht_segment));
          
            // set dir[mid, right) to the new bucket
            for (int i = right - 1; i >= mid; --i) {
                dir[i] = new_seg;
                clflush((char *) dir[i], sizeof(concurrency_ht_segment *));
            }

            tmp_seg->depth = tmp_seg->depth + 1;
            clflush((char *) &(tmp_seg->depth), sizeof(tmp_seg->depth));

            tmp_bucket->free_read_lock();
            tmp_seg->free_write_lock();
            free_read_lock();

#ifdef HT_PROFILE_TIME
            gettimeofday(&end_time, NULL);
            t2 += (end_time.tv_sec - start_time.tv_sec) * 1000000 + end_time.tv_usec - start_time.tv_usec;
#endif
            put(key, value);
            return;
        } else {
            //condition: tmp_bucket->depth == global_depth
#ifdef HT_PROFILE_TIME
            gettimeofday(&start_time, NULL);
#endif
#ifdef HT_PROFILE_LOAD_FACTOR
            ht_dir_num += dir_size;
#endif
            concurrency_ht_segment ** old_dir = dir;
            free_read_lock();

            if(!write_lock()){
                tmp_bucket->free_read_lock();
                tmp_seg->free_read_lock();
                std::this_thread::yield();
                goto RETRY;
            }

            if(old_dir!=dir){
                tmp_bucket->free_read_lock();
                tmp_seg->free_read_lock();
                free_write_lock();
                goto RETRY;
            }
   
            global_depth += 1;
            dir_size *= 2;
            //set dir
            concurrency_ht_segment **new_dir = static_cast<concurrency_ht_segment **>(concurrency_fast_alloc(sizeof(concurrency_ht_segment *) * dir_size));
            for (int i = 0; i < dir_size; ++i) {
                new_dir[i] = dir[i / 2];
            }
            concurrency_ht_segment *new_seg = new_concurrency_ht_segment(global_depth);
            new_dir[dir_index * 2 + 1] = new_seg;
            //migrate previous data
            for (int i = 0; i < HT_MAX_BUCKET_NUM; ++i) {
                uint64_t bucket_cnt = 0;
                uint64_t new_dir_index = 0;
                for (int j = 0; j < HT_BUCKET_SIZE; ++j) {
                    bool tmp_type = tmp_seg->bucket[i].counter[j].type;
                    uint64_t tmp_key = tmp_seg->bucket[i].counter[j].key;
                    uint64_t tmp_value = tmp_seg->bucket[i].counter[j].value;
                    new_dir_index = GET_SEG_NUM(tmp_key, key_len, global_depth);
                    if ((dir_index * 2 + 1) == new_dir_index) {
                        concurrency_ht_segment *dst_seg = new_seg;
                        seg_index = i;
                        concurrency_ht_bucket *dst_bucket = &(dst_seg->bucket[seg_index]);
                        dst_bucket->counter[bucket_cnt].key = tmp_key;
                        dst_bucket->counter[bucket_cnt].value = tmp_value;
                        dst_bucket->counter[bucket_cnt].type = tmp_type;
                        bucket_cnt++;
                    }
                }
            }

            clflush((char *) new_seg, sizeof(concurrency_ht_segment));
            clflush((char *) new_dir, sizeof(concurrency_ht_segment *) * dir_size);

            dir = new_dir;
            clflush((char *) dir, sizeof(dir));

            tmp_bucket->free_read_lock();
            tmp_seg->free_read_lock();
            free_write_lock();

#ifdef HT_PROFILE_TIME
            gettimeofday(&end_time, NULL);
            t3 += (end_time.tv_sec - start_time.tv_sec) * 1000000 + end_time.tv_usec - start_time.tv_usec;
#endif
            put(key, value);
            return;
        }
    } else {
#ifdef HT_PROFILE_TIME
        gettimeofday(&start_time, NULL);
#endif

        uint64_t old_value = tmp_bucket->counter[bucket_index].value;
        // free bucket read lock
        tmp_bucket->free_read_lock();
  
        if(!tmp_bucket->write_lock()){
            tmp_seg->free_read_lock();
            free_read_lock();
            std::this_thread::yield();
            goto RETRY;
        }

        if(tmp_bucket->counter[bucket_index].value!=old_value){
            tmp_bucket->free_write_lock();
            tmp_seg->free_read_lock();
            free_read_lock();
            std::this_thread::yield();
            goto RETRY;
        }
       
        if (unlikely(tmp_bucket->counter[bucket_index].key == key)) {
            //key exists
            tmp_bucket->counter[bucket_index].value = value;

            clflush((char *) &(tmp_bucket->counter[bucket_index].value), 8);

            tmp_bucket->free_write_lock();
        } else {
            // there is a place to insert
            tmp_bucket->counter[bucket_index].value = value;
            tmp_bucket->counter[bucket_index].key = key;
            // Here we clflush 16bytes rather than two 8 bytes because all counter are set to 0.
            // If crash after key flushed, then the value is 0. When we return the value, we would find that the key is not inserted.
            clflush((char *) &(tmp_bucket->counter[bucket_index].key), 16);
            
            tmp_bucket->free_write_lock();
        }

        tmp_seg->free_read_lock();
        free_read_lock();

#ifdef HT_PROFILE_TIME
        gettimeofday(&end_time, NULL);
        t1 += (end_time.tv_sec - start_time.tv_sec) * 1000000 + end_time.tv_usec - start_time.tv_usec;
#endif
    }
    return;
}

bool concurrency_hashtree_node::put_with_read_lock(uint64_t key, uint64_t value, concurrency_ht_segment *tmp_seg,  concurrency_ht_bucket * tmp_bucket) {

    
    int bucket_index = tmp_bucket->find_place(key, key_len, tmp_seg->depth);
    if (bucket_index == -1) {

        //condition: full
        if (likely(tmp_seg->depth < global_depth)) {
#ifdef HT_PROFILE_TIME
            gettimeofday(&start_time, NULL);
#endif
            if(!read_lock()){
                tmp_bucket->free_read_lock();
                tmp_seg->free_read_lock();
                std::this_thread::yield();
                return false;
            }

            uint64_t dir_index = GET_SEG_NUM(key, key_len, global_depth);
            uint64_t seg_index = GET_BUCKET_NUM(key, HT_BUCKET_MASK_LEN);

            if(dir[dir_index]!=tmp_seg){
                tmp_bucket->free_read_lock();
                tmp_seg->free_read_lock();
                free_read_lock();
                std::this_thread::yield();
                return false;
            }

            uint64_t old_depth = tmp_seg->depth;

            tmp_seg->free_read_lock();

            if(!tmp_seg->write_lock()){
                tmp_bucket->free_read_lock();
                free_read_lock();
                std::this_thread::yield();
                return false;
            }
            
            if(tmp_seg->depth!=old_depth){
                tmp_bucket->free_read_lock();
                tmp_seg->free_write_lock();
                free_read_lock();
                std::this_thread::yield();
               return false;
            }

            concurrency_ht_segment *new_seg = new_concurrency_ht_segment(tmp_seg->depth + 1);
            //set dir [left,right)
            int64_t stride = pow(2,  global_depth - tmp_seg->depth);
            int64_t left = dir_index - dir_index % stride;
            int64_t mid = left + stride / 2, right = left + stride;

            //migrate previous data to the new bucket
            for (int i = 0; i < HT_MAX_BUCKET_NUM; ++i) {
                uint64_t bucket_cnt = 0;
                for (int j = 0; j < HT_BUCKET_SIZE; ++j) {
                    bool tmp_type = tmp_seg->bucket[i].counter[j].type;
                    uint64_t tmp_key = tmp_seg->bucket[i].counter[j].key;
                    uint64_t tmp_value = tmp_seg->bucket[i].counter[j].value;
                    dir_index = GET_SEG_NUM(tmp_key, key_len, global_depth);
                    if (dir_index >= mid) {
                        concurrency_ht_segment *dst_seg = new_seg;
                        seg_index = i;
                        concurrency_ht_bucket *dst_bucket = &(dst_seg->bucket[seg_index]);
                        dst_bucket->counter[bucket_cnt].value = tmp_value;
                        dst_bucket->counter[bucket_cnt].key = tmp_key;
                        dst_bucket->counter[bucket_cnt].type = tmp_type;
                        bucket_cnt++;
                    }
                }
            }
            clflush((char *) new_seg, sizeof(concurrency_ht_segment));

            // set dir[mid, right) to the new bucket
            for (int i = right - 1; i >= mid; --i) {
                dir[i] = new_seg;
                clflush((char *) dir[i], sizeof(concurrency_ht_segment *));
            }

            tmp_seg->depth = tmp_seg->depth + 1;
            clflush((char *) &(tmp_seg->depth), sizeof(tmp_seg->depth));

            tmp_bucket->free_read_lock();
            tmp_seg->free_write_lock();
            free_read_lock();

#ifdef HT_PROFILE_TIME
            gettimeofday(&end_time, NULL);
            t2 += (end_time.tv_sec - start_time.tv_sec) * 1000000 + end_time.tv_usec - start_time.tv_usec;
#endif
            return false;
        } else {
            //condition: tmp_bucket->depth == global_depth
#ifdef HT_PROFILE_TIME
            gettimeofday(&start_time, NULL);
#endif
#ifdef HT_PROFILE_LOAD_FACTOR
            ht_dir_num += dir_size;
#endif

            concurrency_ht_segment** old_dir = dir;
            if(!write_lock()){
                tmp_bucket->free_read_lock();
                tmp_seg->free_read_lock();
                std::this_thread::yield();
                return false;
            }

            if(old_dir!=dir){
                tmp_bucket->free_read_lock();
                tmp_seg->free_read_lock();
                free_write_lock();
                std::this_thread::yield();
                return false;
            }

            uint64_t new_global_depth = global_depth + 1;
            uint64_t new_dir_size = dir_size * 2;

            //set dir
            concurrency_ht_segment **new_dir = static_cast<concurrency_ht_segment **>(concurrency_fast_alloc(sizeof(concurrency_ht_segment *) * new_dir_size));
            for (int i = 0; i < new_dir_size; ++i) {
                new_dir[i] = dir[i / 2];
            }
            // concurrency_ht_segment *new_seg = new_concurrency_ht_segment(new_global_depth);
            // new_dir[dir_index * 2 + 1] = new_seg;
            // //migrate previous data
            // for (int i = 0; i < HT_MAX_BUCKET_NUM; ++i) {
            //     uint64_t bucket_cnt = 0;
            //     uint64_t new_dir_index = 0;
            //     for (int j = 0; j < HT_BUCKET_SIZE; ++j) {
            //         bool tmp_type = tmp_seg->bucket[i].counter[j].type;
            //         uint64_t tmp_key = tmp_seg->bucket[i].counter[j].key;
            //         uint64_t tmp_value = tmp_seg->bucket[i].counter[j].value;
            //         new_dir_index = GET_SEG_NUM(tmp_key, key_len, new_global_depth);
            //         if ((dir_index * 2 + 1) == new_dir_index) {
            //             concurrency_ht_segment *dst_seg = new_seg;
            //             seg_index = i;
            //             concurrency_ht_bucket *dst_bucket = &(dst_seg->bucket[seg_index]);
            //             dst_bucket->counter[bucket_cnt].key = tmp_key;
            //             dst_bucket->counter[bucket_cnt].value = tmp_value;
            //             dst_bucket->counter[bucket_cnt].type = tmp_type;
            //             bucket_cnt++;
            //         }
            //     }
            // }

            // clflush((char *) new_seg, sizeof(concurrency_ht_segment));
            clflush((char *) new_dir, sizeof(concurrency_ht_segment *) * new_dir_size);

            global_depth  = new_global_depth;
            dir_size  = new_dir_size;
            dir = new_dir;
          
            clflush((char *) dir, sizeof(dir));

            tmp_bucket->free_read_lock();
            tmp_seg->free_read_lock();
            free_write_lock();

#ifdef HT_PROFILE_TIME
            gettimeofday(&end_time, NULL);
            t3 += (end_time.tv_sec - start_time.tv_sec) * 1000000 + end_time.tv_usec - start_time.tv_usec;
#endif
            return false;
        }
    } else {
#ifdef HT_PROFILE_TIME
        gettimeofday(&start_time, NULL);
#endif
        uint64_t old_value = tmp_bucket->counter[bucket_index].value;
        // free bucket read lock
        tmp_bucket->free_read_lock();
  
        if(!tmp_bucket->write_lock()){
            tmp_seg->free_read_lock();
            // free_read_lock();
            std::this_thread::yield();
            return false;
        }

        if(tmp_bucket->counter[bucket_index].value!=old_value){
            tmp_bucket->free_write_lock();
            tmp_seg->free_read_lock();
            // free_read_lock();
            std::this_thread::yield();
            return false;
        }
       
        if (unlikely(tmp_bucket->counter[bucket_index].key == key)) {
            //key exists
            tmp_bucket->counter[bucket_index].value = value;

            clflush((char *) &(tmp_bucket->counter[bucket_index].value), 8);

            tmp_bucket->free_write_lock();
        } else {
            // there is a place to insert
            tmp_bucket->counter[bucket_index].value = value;
            tmp_bucket->counter[bucket_index].key = key;
            // Here we clflush 16bytes rather than two 8 bytes because all counter are set to 0.
            // If crash after key flushed, then the value is 0. When we return the value, we would find that the key is not inserted.
            clflush((char *) &(tmp_bucket->counter[bucket_index].key), 16);
            
            tmp_bucket->free_write_lock();
        }

        tmp_seg->free_read_lock();
        // free_read_lock();

#ifdef HT_PROFILE_TIME
        gettimeofday(&end_time, NULL);
        t1 += (end_time.tv_sec - start_time.tv_sec) * 1000000 + end_time.tv_usec - start_time.tv_usec;
#endif
    }
    return true;
}


uint64_t concurrency_hashtree_node::get(uint64_t key) {


    GET_RETRY:

    if(!read_lock()){
        std::this_thread::yield();
        goto GET_RETRY;
    }

    uint64_t dir_index = GET_SEG_NUM(key, key_len, global_depth);
    concurrency_ht_segment *tmp_seg = dir[dir_index];
    
    // read lock segment
    if(!tmp_seg->read_lock()){
        free_read_lock();
        std::this_thread::yield();
        goto GET_RETRY;
    }

    // double check 
    uint64_t check_dir_index = GET_SEG_NUM(key, key_len, global_depth);
    if(tmp_seg!=dir[check_dir_index]){
        tmp_seg->free_read_lock();
        free_read_lock();
        std::this_thread::yield();
        goto GET_RETRY;
    }

    uint64_t seg_index = GET_BUCKET_NUM(key, HT_BUCKET_MASK_LEN);
    concurrency_ht_bucket *tmp_bucket = &(tmp_seg->bucket[seg_index]);
    if(!tmp_bucket->read_lock()){
        tmp_seg->free_read_lock();
        free_read_lock();
        std::this_thread::yield();
        goto GET_RETRY;
    }
    uint64_t value  = tmp_bucket->get(key);
    tmp_bucket->free_read_lock();
    tmp_seg->free_read_lock();
    free_read_lock();
    return value;
}


bool concurrency_hashtree_node::read_lock(){
    int64_t val = lock_meta;
    while(val>-1){
        if(cas(&lock_meta, &val, val+1)){
            return true;
        }
        val = lock_meta;
    }
    return false;
}

bool concurrency_hashtree_node::write_lock(){
    int64_t val = lock_meta;
    if(val<0){
        return false;
    }

    while(!cas(&lock_meta, &val, -1)){
        val = lock_meta;
        if(val<0){
            return false;
        }
    }

    while(val && lock_meta != 0-val-1){
        asm("nop");
    }

    return true;

}

void concurrency_hashtree_node::free_read_lock(){
    int64_t val = lock_meta;
    while(!cas(&lock_meta, &val, val-1)){
        val = lock_meta;
    }
}

void concurrency_hashtree_node::free_write_lock(){
   lock_meta = 0;
}

concurrency_hashtree_node *new_concurrency_hashtree_node(int _global_depth, int _key_len) {
    concurrency_hashtree_node *_new_node = static_cast<concurrency_hashtree_node *>(concurrency_fast_alloc(sizeof(concurrency_hashtree_node)));
    _new_node->init(_global_depth, _key_len);
    return _new_node;
}