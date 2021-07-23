//
// Created by 杨冠群 on 2021-07-08.
//

#include <stdio.h>
#include "varLengthHashTree_node.h"

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

HashTreeKeyValue *new_vlht_key_value(unsigned char* key, unsigned int len ,uint64_t value) {
    HashTreeKeyValue *_new_key_value = static_cast<HashTreeKeyValue *>(fast_alloc(sizeof(HashTreeKeyValue)));
    _new_key_value->type = 1;
    _new_key_value->key = key;
    _new_key_value->len = len;
    _new_key_value->value = value;
    return _new_key_value;
}

uint64_t HashTreeBucket::get(uint64_t key) {
    for (int i = 0; i < HT_BUCKET_SIZE; ++i) {
        if (key == counter[i].subkey) {
            return counter[i].value;
        }
    }
    return 0;
}

int HashTreeBucket::find_place(uint64_t _key, uint64_t _key_len, uint64_t _depth) {
    // full: return -1
    // exists or not full: return index or empty counter
    int res = -1;
    for (int i = 0; i < HT_BUCKET_SIZE; ++i) {
        if (_key == counter[i].subkey) {
            return i;
        } else if ((res == -1) && counter[i].subkey == 0 && counter[i].value == 0) {
            res = i;
        } else if ((res == -1) &&
                   (GET_SEG_NUM(_key, _key_len, _depth) != GET_SEG_NUM(counter[i].subkey, _key_len, _depth))) {
            res = i;
        }
    }
    return res;
}

HashTreeBucket *new_vlht_bucket(int _depth) {
    HashTreeBucket *_new_bucket = static_cast<HashTreeBucket *>(fast_alloc(sizeof(HashTreeBucket)));
    return _new_bucket;
}

HashTreeSegment::HashTreeSegment() {
    depth = 0;
    bucket = static_cast<HashTreeBucket *>(fast_alloc(sizeof(HashTreeBucket) * HT_MAX_BUCKET_NUM));
}

HashTreeSegment::~HashTreeSegment() {}

void HashTreeSegment::init(uint64_t _depth) {
    depth = _depth;
    bucket = static_cast<HashTreeBucket *>(fast_alloc(sizeof(HashTreeBucket) * HT_MAX_BUCKET_NUM));
#ifdef HT_PROFILE_LOAD_FACTOR
    ht_seg_num++;
#endif
}

HashTreeSegment *new_vlht_segment(uint64_t _depth) {
    HashTreeSegment *_new_ht_segment = static_cast<HashTreeSegment *>(fast_alloc(sizeof(HashTreeSegment)));
    _new_ht_segment->init(_depth);
    return _new_ht_segment;
}

VarLengthHashTreeNode::VarLengthHashTreeNode() {
    type = 0;
    global_depth = 0;
    dir_size = pow(2, global_depth);
    // dir = static_cast<HashTreeSegment **>(fast_alloc(sizeof(HashTreeSegment *) * dir_size));
    for (int i = 0; i < dir_size; ++i) {
        *(HashTreeSegment **)GET_SEG_POS(this,i) = new_vlht_segment();
    }
}

VarLengthHashTreeNode::~VarLengthHashTreeNode() {

}

void VarLengthHashTreeNode::init(unsigned char headerDepth, unsigned char global_depth) {
    type = 0;
    this->global_depth = global_depth;
    this->dir_size = pow(2, global_depth);
    header.depth = headerDepth;
    treeNodeValues = static_cast<HashTreeKeyValue*>(fast_alloc(sizeof(HashTreeKeyValue ) * (1+HT_NODE_PREFIX_MAX_BITS/HT_NODE_LENGTH)));
    // dir = static_cast<HashTreeSegment *>(fast_alloc(sizeof(HashTreeSegment *) * dir_size));
    for (int i = 0; i < this->dir_size; ++i) {
        *(HashTreeSegment **)GET_SEG_POS(this,i) = new_vlht_segment(global_depth);
    }
}
void VarLengthHashTreeNode::put(uint64_t subkey, uint64_t value, uint64_t beforeAddress){
    uint64_t dir_index = GET_SEG_NUM(subkey, HT_NODE_LENGTH, global_depth);
    HashTreeSegment *tmp_seg = *(HashTreeSegment **)GET_SEG_POS(this,dir_index);
    uint64_t seg_index = GET_BUCKET_NUM(subkey, HT_BUCKET_MASK_LEN);
    HashTreeBucket *tmp_bucket = &(tmp_seg->bucket[seg_index]);
    put(subkey,value,tmp_seg,tmp_bucket,dir_index,seg_index,beforeAddress);
}

void VarLengthHashTreeNode::put(uint64_t subkey, uint64_t value, HashTreeSegment* tmp_seg, HashTreeBucket* tmp_bucket, uint64_t dir_index, uint64_t seg_index, uint64_t beforeAddress){
    int bucket_index = tmp_bucket->find_place(subkey, HT_NODE_LENGTH, tmp_seg->depth);
    if (bucket_index == -1) {
        //condition: full
        if (likely(tmp_seg->depth < global_depth)) {
            HashTreeSegment *new_seg = new_vlht_segment(tmp_seg->depth + 1);
            int64_t stride = pow(2, global_depth - tmp_seg->depth);
            int64_t left = dir_index - dir_index % stride;
            int64_t mid = left + stride / 2, right = left + stride;

            //migrate previous data to the new bucket
            for (int i = 0; i < HT_MAX_BUCKET_NUM; ++i) {
                uint64_t bucket_cnt = 0;
                for (int j = 0; j < HT_BUCKET_SIZE; ++j) {
                    uint64_t tmp_key = tmp_seg->bucket[i].counter[j].subkey;
                    uint64_t tmp_value = tmp_seg->bucket[i].counter[j].value;
                    dir_index = GET_SEG_NUM(tmp_key, HT_NODE_LENGTH, global_depth);
                    if (dir_index >= mid) {
                        HashTreeSegment *dst_seg = new_seg;
                        seg_index = i;
                        HashTreeBucket *dst_bucket = &(dst_seg->bucket[seg_index]);
                        dst_bucket->counter[bucket_cnt].value = tmp_value;
                        dst_bucket->counter[bucket_cnt].subkey = tmp_key;
                        bucket_cnt++;
                    }
                }
            }
            clflush((char *) new_seg, sizeof(HashTreeSegment));

            // set dir[mid, right) to the new bucket
            for (int i = right - 1; i >= mid; --i) {
                *(HashTreeSegment **)GET_SEG_POS(this,i) = new_seg;
                clflush((char *) GET_SEG_POS(this,i), sizeof(HashTreeSegment *));
            }
            tmp_seg->depth = tmp_seg->depth + 1;
            clflush((char *) &(tmp_seg->depth), sizeof(tmp_seg->depth));
            this->put(subkey, value, beforeAddress);
            return;
        } else {
            //condition: tmp_bucket->depth == global_depth
            VarLengthHashTreeNode *newNode = static_cast<VarLengthHashTreeNode *>(fast_alloc(sizeof(VarLengthHashTreeNode)+sizeof(HashTreeSegment *)*dir_size*2));
            newNode->global_depth = global_depth+1;
            newNode->dir_size = dir_size*2;
            newNode->header.init(&this->header,this->header.len,this->header.depth);
            //set dir
            for (int i = 0; i < newNode->dir_size; ++i) {
                *(HashTreeSegment **)GET_SEG_POS(newNode,i) = *(HashTreeSegment **) GET_SEG_POS(this,(i / 2));
            }
            clflush((char *) newNode, sizeof(VarLengthHashTreeNode)+sizeof(HashTreeSegment *)*newNode->dir_size);
            *(VarLengthHashTreeNode**)beforeAddress = newNode;
            clflush((char *) beforeAddress, sizeof(VarLengthHashTreeNode*));
            newNode->put(subkey, value, beforeAddress);
            return;
        }
    } else {
        if (unlikely(tmp_bucket->counter[bucket_index].subkey == subkey)) {
            //key exists
            tmp_bucket->counter[bucket_index].value = value;
            clflush((char *) &(tmp_bucket->counter[bucket_index].value), 8);
        } else {
            // there is a place to insert
            tmp_bucket->counter[bucket_index].value = value;
            mfence();
            tmp_bucket->counter[bucket_index].subkey = subkey;
            // Here we clflush 16bytes rather than two 8 bytes because all counter are set to 0.
            // If crash after key flushed, then the value is 0. When we return the value, we would find that the key is not inserted.
            clflush((char *) &(tmp_bucket->counter[bucket_index].subkey), 16);
        }
    }
    return;
}

uint64_t VarLengthHashTreeNode::get(uint64_t subkey) {
    uint64_t dir_index = GET_SEG_NUM(subkey, HT_NODE_LENGTH, global_depth);
    HashTreeSegment *tmp_seg = *(HashTreeSegment **)GET_SEG_POS(this,dir_index);
    uint64_t seg_index = GET_BUCKET_NUM(subkey, HT_BUCKET_MASK_LEN);
    HashTreeBucket *tmp_bucket = &(tmp_seg->bucket[seg_index]);
    return tmp_bucket->get(subkey);
}

VarLengthHashTreeNode *new_varlengthhashtree_node( int _key_len, unsigned char headerDepth, unsigned char globalDepth) {
    VarLengthHashTreeNode *_new_node = static_cast<VarLengthHashTreeNode *>(fast_alloc(sizeof(VarLengthHashTreeNode)+sizeof(HashTreeSegment *)*pow(2, globalDepth)));
    _new_node->init(headerDepth,globalDepth);
    return _new_node;
}

void VarLengthHashTreeNode::node_put(int pos, HashTreeKeyValue* kv){
    treeNodeValues[header.len - pos] = *kv;
}

void VarLengthHashTreeHeader::init(VarLengthHashTreeHeader* oldHeader, unsigned char length, unsigned char depth){
    assign(oldHeader->array,length);
    this->depth = depth;
    this->len = length;
}

// make sure return n HT_NODE_LENGTH
int VarLengthHashTreeHeader::computePrefix(unsigned char* key, int len, unsigned int pos){
    int matchedLength = 0; 
    for(int i=0;i<this->len;i++){
        if(pos+i>len||key[pos+i]!=this->array[i]){
            return matchedLength;
        }
        matchedLength++;
    }
    return matchedLength - matchedLength%HT_NODE_LENGTH;
}


void VarLengthHashTreeHeader::assign(unsigned char* key, unsigned char assignedLength){
    for(int i=0;i<assignedLength;i++){
        array[i] = key[i];
    }
}
