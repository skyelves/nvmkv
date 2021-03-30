//
// Created by 王柯 on 2021-03-04.
//

#include <stdio.h>
#include "hashtree_node.h"


#define GET_DIR_NUM(key, key_len, depth)  ((key>>(key_len-depth))&(((uint64_t)1<<key_len)-1))
//#define GET_DIR_NUM(key, key_len, depth)  ((key>>(key_len-depth))&0xffff)

key_value *new_key_value(uint64_t key, uint64_t value) {
    key_value *_new_key_value = static_cast<key_value *>(fast_alloc(sizeof(key_value)));
    _new_key_value->type = 1;
    _new_key_value->key = key;
    _new_key_value->value = value;
    return _new_key_value;
}

bucket::bucket() {}

bucket::bucket(int _depth) {
    depth = _depth;
}

inline void bucket::init(int _depth) {
    depth = _depth;
}

void bucket::set_depth(int _depth) {
    depth = _depth;
}

int64_t bucket::get(uint64_t key) {
    for (int i = 0; i < cnt; ++i) {
        if (key == counter[i].key) {
            if (counter[i].value != 0)
                return counter[i].value;
        }
    }
    return -1;
}

int bucket::find_place(uint64_t key) {
    // full: return -1
    // exists or not full: return index or empty counter
    for (int i = 0; i < cnt; ++i) {
        if (key == counter[i].key) {
            return i;
        }
    }
    if (likely(cnt < BUCKET_SIZE))
        return cnt;
    else
        return -1;
}

bucket *new_bucket(int _depth) {
    bucket *_new_bucket = static_cast<bucket *>(fast_alloc(sizeof(bucket)));
    _new_bucket->init(_depth);
    return _new_bucket;
}

hashtree_node::hashtree_node() {
    dir = static_cast<bucket **>(fast_alloc(sizeof(bucket *) * dir_size));
    for (int i = 0; i < dir_size; ++i) {
        dir[i] = new_bucket();
    }
}

hashtree_node::hashtree_node(int _global_depth, int _key_len) {
    global_depth = _global_depth;
    key_len = _key_len;
    dir_size = pow(2, global_depth);
    dir = static_cast<bucket **>(fast_alloc(sizeof(bucket *) * dir_size));
    for (int i = 0; i < dir_size; ++i) {
        dir[i] = new_bucket(_global_depth);
    }
}

hashtree_node::~hashtree_node() {
}

void hashtree_node::init(uint32_t _global_depth, int _key_len) {
    type = 0;
    global_depth = _global_depth;
    key_len = _key_len;
    dir_size = pow(2, global_depth);
//    dir = new bucket *[dir_size];
    dir = static_cast<bucket **>(fast_alloc(sizeof(bucket *) * dir_size));
    for (int i = 0; i < dir_size; ++i) {
//        dir[i] = new bucket(_global_depth);
        dir[i] = new_bucket(_global_depth);
    }
}

void hashtree_node::put(uint64_t key, uint64_t value) {
    uint64_t index = GET_DIR_NUM(key, key_len, global_depth);
    bucket *tmp_bucket = dir[index];
    int bucket_index = tmp_bucket->find_place(key);
    if (bucket_index == -1) {
        //condition: full
        if (likely(tmp_bucket->depth < global_depth)) {
            bucket *new_bucket1 = new_bucket(tmp_bucket->depth + 1);
            bucket *new_bucket2 = new_bucket(tmp_bucket->depth + 1);
            //set dir [left,right)
            uint64_t left = index, mid = index, right = index + 1;
            for (int i = index + 1; i < dir_size; ++i) {
                if (likely(dir[i] != tmp_bucket)) {
                    right = i;
                    break;
                } else if (unlikely(i == (dir_size - 1))) {
                    right = dir_size;
                    break;
                }
            }
            for (int i = index - 1; i >= 0; --i) {
                if (likely(dir[i] != tmp_bucket)) {
                    left = i + 1;
                    break;
                } else if (unlikely(i == 0)) {
                    left = 0;
                    break;
                }
            }
            mid = (left + right) / 2;
            for (int i = left; i < mid; ++i) {
                dir[i] = new_bucket1;
            }
            for (int i = mid; i < right; ++i) {
                dir[i] = new_bucket2;
            }
            //migrate previous data
            for (int i = 0; i < BUCKET_SIZE; ++i) {
                uint64_t tmp_key = tmp_bucket->counter[i].key;
                uint64_t tmp_value = tmp_bucket->counter[i].value;
                index = GET_DIR_NUM(tmp_key, key_len, global_depth);
                bucket *dst_bucket = dir[index];
                dst_bucket->counter[dst_bucket->cnt].key = tmp_key;
                dst_bucket->counter[dst_bucket->cnt].value = tmp_value;
                dst_bucket->cnt++;
            }
            //todo delete previous bucket
            put(key, value);
            return;
        } else {
            //condition: tmp_bucket->depth == global_depth
            global_depth += 1;
            dir_size *= 2;
            //set dir
            bucket **new_dir = static_cast<bucket **>(fast_alloc(sizeof(bucket *) * dir_size));
            for (int i = 0; i < dir_size; ++i) {
                new_dir[i] = dir[i / 2];
            }
            bucket *new_bucket1 = new_bucket(global_depth);
            bucket *new_bucket2 = new_bucket(global_depth);
            new_dir[index * 2] = new_bucket1;
            new_dir[index * 2 + 1] = new_bucket2;
            //migrate previous data
            for (int i = 0; i < BUCKET_SIZE; ++i) {
                uint64_t tmp_key = tmp_bucket->counter[i].key;
                int64_t tmp_value = tmp_bucket->counter[i].value;
                index = GET_DIR_NUM(tmp_key, key_len, global_depth);
                bucket *dst_bucket = new_dir[index];
                dst_bucket->counter[dst_bucket->cnt].key = tmp_key;
                dst_bucket->counter[dst_bucket->cnt].value = tmp_value;
                dst_bucket->cnt++;
            }
            //todo delete previous dir
            dir = new_dir;
            put(key, value);
            return;
        }
    } else {
        if (unlikely(
                (tmp_bucket->counter[bucket_index].key == key) &&
                (tmp_bucket->counter[bucket_index].value != 0))) {
            //key exists
            tmp_bucket->counter[bucket_index].value = value;
        } else {
            // there is a place to insert
            tmp_bucket->counter[bucket_index].key = key;
            tmp_bucket->counter[bucket_index].value = value;
            tmp_bucket->cnt++;
        }
    }
    return;
}

int64_t hashtree_node::get(uint64_t key) {
    uint64_t index = GET_DIR_NUM(key, key_len, global_depth);
    return dir[index]->get(key);
}

hashtree_node *new_hashtree_node(int _global_depth, int _key_len) {
    hashtree_node *_new_node = static_cast<hashtree_node *>(fast_alloc(sizeof(hashtree_node)));
    _new_node->init(_global_depth, _key_len);
    return _new_node;
}