#include "concurrency_cceh.h"

using namespace std;

#define GET_SEG_NUM(key, key_len, depth)  ((key>>(key_len-depth))&(((uint64_t)1<<depth)-1))
#define GET_BUCKET_NUM(key, bucket_mask_len) ((key)&(((uint64_t)1<<bucket_mask_len)-1))

#ifdef CCEH_PROFILE_TIME
timeval start_time, end_time;
uint64_t t1 = 0, t2 = 0, t3 = 0;
#endif

#ifdef CCEH_PROFILE_LOAD_FACTOR
uint64_t cceh_seg_num = 0;
#endif

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

int concurrency_cceh_bucket::find_place(Key_t key, uint64_t depth) {
    int res = -1;
    for (int i = 0; i < CCEH_BUCKET_SIZE; ++i) {
        if (key == kv[i].key) {
            return i;
        } else if ((res == -1) && kv[i].key == 0 && kv[i].value == 0) {
            res = i;
        } else if ((res == -1) && ((GET_SEG_NUM(key, 64, depth)) != (GET_SEG_NUM(kv[i].key, 64, depth)))) {
            res = i;
        }
    }
    return res;
}

Value_t concurrency_cceh_bucket::get(Key_t key) {
    for (int i = 0; i < CCEH_BUCKET_SIZE; ++i) {
        if (key == kv[i].key) {
            return kv[i].value;
        }
    }
    return 0;
}

bool concurrency_cceh_bucket::read_lock(){
    int64_t val = lock_meta;
    while(val>-1){
        if(cas(&lock_meta, &val, val+1)){
            return true;
        }
        val = lock_meta;
    }
    return false;
}

bool concurrency_cceh_bucket::write_lock(){
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

void concurrency_cceh_bucket::free_read_lock(){
    int64_t val = lock_meta;
    while(!cas(&lock_meta, &val, val-1)){
        val = lock_meta;
    }
}

void concurrency_cceh_bucket::free_write_lock(){
    lock_meta = 0;
}


concurrency_cceh_segment::concurrency_cceh_segment() {
    depth = 0;
    bucket = static_cast<concurrency_cceh_bucket *>(concurrency_fast_alloc(sizeof(concurrency_cceh_bucket) * CCEH_MAX_BUCKET_NUM));
}

concurrency_cceh_segment::~concurrency_cceh_segment() {

}

void concurrency_cceh_segment::init(uint64_t _depth) {
    depth = _depth;
    bucket = static_cast<concurrency_cceh_bucket *>(concurrency_fast_alloc(sizeof(concurrency_cceh_bucket) * CCEH_MAX_BUCKET_NUM));
}

concurrency_cceh_segment *new_concurrency_cceh_segment(uint64_t _depth) {
    concurrency_cceh_segment *_new_cceh_segment = static_cast<concurrency_cceh_segment *>(concurrency_fast_alloc(sizeof(concurrency_cceh_segment)));
    _new_cceh_segment->init(_depth);
    return _new_cceh_segment;
}

bool concurrency_cceh_segment::read_lock(){
    int64_t val = lock_meta;
    while(val>-1){
        if(cas(&lock_meta, &val, val+1)){
            return true;
        }
        val = lock_meta;
    }
    return false;
}

bool concurrency_cceh_segment::write_lock(){
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

void concurrency_cceh_segment::free_read_lock(){
    int64_t val = lock_meta;
    while(!cas(&lock_meta, &val, val-1)){
        val = lock_meta;
    }
}

void concurrency_cceh_segment::free_write_lock(){
    lock_meta = 0;
}



concurrency_cceh::concurrency_cceh() {
    global_depth = 0;
    dir_size = pow(2, global_depth);
    key_len = 64;
    dir = static_cast<concurrency_cceh_segment **>(concurrency_fast_alloc(sizeof(concurrency_cceh_segment *) * dir_size));
    for (int i = 0; i < dir_size; ++i) {
        dir[i] = new_concurrency_cceh_segment();
    }
#ifdef CCEH_PROFILE_LOAD_FACTOR
    cceh_seg_num = dir_size;
#endif
}

concurrency_cceh::~concurrency_cceh() {
}

void concurrency_cceh::init(uint64_t _global_depth, uint64_t _key_len) {
    global_depth = _global_depth;
    dir_size = pow(2, global_depth);
    key_len = _key_len;
    dir = static_cast<concurrency_cceh_segment **>(concurrency_fast_alloc(sizeof(concurrency_cceh_segment *) * dir_size));
    for (int i = 0; i < dir_size; ++i) {
        dir[i] = new_concurrency_cceh_segment(_global_depth);
    }
#ifdef CCEH_PROFILE_LOAD_FACTOR
    cceh_seg_num = dir_size;
#endif
}

void concurrency_cceh::put(Key_t key, Value_t value) {

    RETRY:
        if(!read_lock()){
            std::this_thread::yield();
            goto RETRY;
        }
        // value should not be 0
        uint64_t dir_index = GET_SEG_NUM(key, key_len, global_depth);
        concurrency_cceh_segment *tmp_seg = dir[dir_index];

        // get read lock
        if(!tmp_seg->read_lock()){
            free_read_lock();
            std::this_thread::yield();
            goto RETRY;
        }

        uint64_t seg_index = GET_BUCKET_NUM(key, CCEH_BUCKET_MASK_LEN);
        concurrency_cceh_bucket *tmp_bucket = &(tmp_seg->bucket[seg_index]);

        if(!tmp_bucket->read_lock()){
            tmp_seg->free_read_lock();
            free_read_lock();
            std::this_thread::yield();
            goto RETRY;
        }
    
        int bucket_index = tmp_bucket->find_place(key, tmp_seg->depth);
        if (bucket_index == -1) {
            //condition: full
            if (likely(tmp_seg->depth < global_depth)) {
    #ifdef CCEH_PROFILE_TIME
                gettimeofday(&start_time, NULL);
    #endif
    #ifdef CCEH_PROFILE_LOAD_FACTOR
                cceh_seg_num++;
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

                concurrency_cceh_segment *new_seg = new_concurrency_cceh_segment(tmp_seg->depth + 1);
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
                //migrate previous data to the new segment
                for (int i = 0; i < CCEH_MAX_BUCKET_NUM; ++i) {
                    uint64_t bucket_cnt = 0;
                    for (int j = 0; j < CCEH_BUCKET_SIZE; ++j) {
                        uint64_t tmp_key = tmp_seg->bucket[i].kv[j].key;
                        uint64_t tmp_value = tmp_seg->bucket[i].kv[j].value;
                        dir_index = GET_SEG_NUM(tmp_key, key_len, global_depth);
                        if (dir_index >= mid) {
                            concurrency_cceh_segment *dst_seg = new_seg;
                            seg_index = i;
                            concurrency_cceh_bucket *dst_bucket = &(dst_seg->bucket[seg_index]);
                            dst_bucket->kv[bucket_cnt].value = tmp_value;
                            dst_bucket->kv[bucket_cnt].key = tmp_key;
                            bucket_cnt++;
                        }
                    }
                }
                clflush((char *) new_seg, sizeof(concurrency_cceh_segment));


                //set dir[mid, right) to the new segment
                for (int i = right - 1; i >= mid; --i) {
                    dir[i] = new_seg;

                    clflush((char *) dir[i], sizeof(concurrency_cceh_segment *));

                }

                tmp_seg->depth = tmp_seg->depth + 1;

                clflush((char *) &(tmp_seg->depth), sizeof(tmp_seg->depth));

                tmp_bucket->free_read_lock();
                tmp_seg->free_write_lock();
                free_read_lock();

    #ifdef CCEH_PROFILE_TIME
                gettimeofday(&end_time, NULL);
                t2+=(end_time.tv_sec - start_time.tv_sec) * 1000000 + end_time.tv_usec - start_time.tv_usec;
    #endif
                put(key, value);
                return;
            } else {
                //condition: tmp_seg->depth == global_depth
    #ifdef CCEH_PROFILE_TIME
                gettimeofday(&start_time, NULL);
    #endif
    #ifdef CCEH_PROFILE_LOAD_FACTOR
                cceh_seg_num++;
    #endif

                concurrency_cceh_segment ** old_dir = dir;
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
                concurrency_cceh_segment **new_dir = static_cast<concurrency_cceh_segment **>(concurrency_fast_alloc(sizeof(concurrency_cceh_segment *) * dir_size));
                for (int i = 0; i < dir_size; ++i) {
                    new_dir[i] = dir[i / 2];
                }
                concurrency_cceh_segment *new_seg = new_concurrency_cceh_segment(global_depth);
                new_dir[dir_index * 2 + 1] = new_seg;
                //migrate previous data
                for (int i = 0; i < CCEH_MAX_BUCKET_NUM; ++i) {
                    uint64_t bucket_cnt = 0;
                    uint64_t new_dir_index = 0;
                    for (int j = 0; j < CCEH_BUCKET_SIZE; ++j) {
                        uint64_t tmp_key = tmp_seg->bucket[i].kv[j].key;
                        uint64_t tmp_value = tmp_seg->bucket[i].kv[j].value;
                        new_dir_index = GET_SEG_NUM(tmp_key, key_len, global_depth);
                        if ((dir_index * 2 + 1) == new_dir_index) {
                            concurrency_cceh_segment *dst_seg = new_seg;
                            seg_index = i;
                            concurrency_cceh_bucket *dst_bucket = &(dst_seg->bucket[seg_index]);
                            dst_bucket->kv[bucket_cnt].key = tmp_key;
                            dst_bucket->kv[bucket_cnt].value = tmp_value;
                            bucket_cnt++;
                        }
                    }
                }

                clflush((char *) new_seg, sizeof(concurrency_cceh_bucket));
                clflush((char *) new_dir, sizeof(concurrency_cceh_bucket *) * dir_size);

                dir = new_dir;
                clflush((char *) dir, sizeof(dir));

                // tmp_seg->depth = tmp_seg->depth + 1;
                clflush((char *) &(tmp_seg->depth), sizeof(tmp_seg->depth));
    #ifdef CCEH_PROFILE_TIME
                gettimeofday(&end_time, NULL);
                t3+=(end_time.tv_sec - start_time.tv_sec) * 1000000 + end_time.tv_usec - start_time.tv_usec;
    #endif

                tmp_bucket->free_read_lock();
                tmp_seg->free_read_lock();
                free_write_lock();

                put(key, value);
                return;
            }
        } else {
    #ifdef CCEH_PROFILE_TIME
            gettimeofday(&start_time, NULL);
    #endif

            uint64_t old_value = tmp_bucket->kv[bucket_index].value;
            // free bucket read lock
            tmp_bucket->free_read_lock();
    
            if(!tmp_bucket->write_lock()){
                tmp_seg->free_read_lock();
                free_read_lock();
                std::this_thread::yield();
                goto RETRY;
            }

            if(tmp_bucket->kv[bucket_index].value!=old_value){
                tmp_bucket->free_write_lock();
                tmp_seg->free_read_lock();
                free_read_lock();
                std::this_thread::yield();
                goto RETRY;
            }

            if (unlikely(tmp_bucket->kv[bucket_index].key == key)) {
                //key exists
                tmp_bucket->kv[bucket_index].value = value;
                clflush((char *) &(tmp_bucket->kv[bucket_index].value), 8);

            } else {
                // there is a place to insert
                tmp_bucket->kv[bucket_index].value = value;
                mfence();
                tmp_bucket->kv[bucket_index].key = key;
                clflush((char *) &(tmp_bucket->kv[bucket_index].key), 16);
            }
            tmp_bucket->free_write_lock();
            tmp_seg->free_read_lock();
            free_read_lock();
    #ifdef CCEH_PROFILE_TIME
            gettimeofday(&end_time, NULL);
            t1+=(end_time.tv_sec - start_time.tv_sec) * 1000000 + end_time.tv_usec - start_time.tv_usec;
    #endif
        }
}

Value_t concurrency_cceh::get(Key_t key) {
    uint64_t dir_index = GET_SEG_NUM(key, key_len, global_depth);
    concurrency_cceh_segment *tmp_seg = dir[dir_index];
    uint64_t seg_index = GET_BUCKET_NUM(key, CCEH_BUCKET_MASK_LEN);
    concurrency_cceh_bucket *tmp_bucket = &(tmp_seg->bucket[seg_index]);
    return tmp_bucket->get(key);
}

concurrency_cceh *new_concurrency_cceh(uint64_t _global_depth, uint64_t _key_len) {
    concurrency_cceh *_new_cceh = static_cast<concurrency_cceh *>(concurrency_fast_alloc(
            sizeof(concurrency_cceh)));
    _new_cceh->init(_global_depth, _key_len);
    return _new_cceh;
}

bool concurrency_cceh::read_lock(){
    int64_t val = lock_meta;
    while(val>-1){
        if(cas(&lock_meta, &val, val+1)){
            return true;
        }
        val = lock_meta;
    }
    return false;
}

bool concurrency_cceh::write_lock(){
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

void concurrency_cceh::free_read_lock(){
    int64_t val = lock_meta;
    while(!cas(&lock_meta, &val, val-1)){
        val = lock_meta;
    }
}

void concurrency_cceh::free_write_lock(){
    lock_meta = 0;
}

