//
// Created by 杨冠群 on 2021-07-08.
//

#include <stdio.h>
#include "varLengthHashTree_node.h"


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
    HashTreeKeyValue *_new_key_value = static_cast<HashTreeKeyValue *>(concurrency_fast_alloc(sizeof(HashTreeKeyValue)));
    _new_key_value->type = 1;
    _new_key_value->key = key;
    _new_key_value->len = len;
    _new_key_value->value = value;
    return _new_key_value;
}

HashTreeBucket::HashTreeBucket(){
    lock_meta = 0;
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
    HashTreeBucket *_new_bucket = static_cast<HashTreeBucket *>(concurrency_fast_alloc(sizeof(HashTreeBucket)));
    return _new_bucket;
}


bool HashTreeBucket::read_lock(){
    int64_t val = lock_meta;
    while(val>-1){
        if(cas(&lock_meta, &val, val+1)){
            return true;
        }
        val = lock_meta;
    }
    return false;
}

bool HashTreeBucket::write_lock(){
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

void HashTreeBucket::free_read_lock(){
    int64_t val = lock_meta;
    while(!cas(&lock_meta, &val, val-1)){
        val = lock_meta;
    }
}

void HashTreeBucket::free_write_lock(){
    lock_meta = 0;
}


HashTreeSegment::HashTreeSegment() {
    depth = 0;
    lock_meta = 0;
    bucket = static_cast<HashTreeBucket *>(concurrency_fast_alloc(sizeof(HashTreeBucket) * HT_MAX_BUCKET_NUM));
}

HashTreeSegment::~HashTreeSegment() {}

void HashTreeSegment::init(uint64_t _depth) {
    depth = _depth;
    lock_meta = 0;
    bucket = static_cast<HashTreeBucket *>(concurrency_fast_alloc(sizeof(HashTreeBucket) * HT_MAX_BUCKET_NUM));
}

HashTreeSegment *new_vlht_segment(uint64_t _depth) {
    HashTreeSegment *_new_ht_segment = static_cast<HashTreeSegment *>(concurrency_fast_alloc(sizeof(HashTreeSegment)));
    _new_ht_segment->init(_depth);
    return _new_ht_segment;
}


bool HashTreeSegment::read_lock(){
    int64_t val = lock_meta;
    while(val>-1){
        if(cas(&lock_meta, &val, val+1)){
            return true;
        }
        val = lock_meta;
    }
    return false;
}

bool HashTreeSegment::write_lock(){
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

void HashTreeSegment::free_read_lock(){
    int64_t val = lock_meta;
    while(!cas(&lock_meta, &val, val-1)){
        val = lock_meta;
    }
}

void HashTreeSegment::free_write_lock(){
   lock_meta = 0;
}

VarLengthHashTreeNode::VarLengthHashTreeNode() {
    type = 0;
    global_depth = 0;
    dir_size = pow(2, global_depth);
    header.depth = 1;
    lock_meta = 0;
    header.len = 0;
    // dir = static_cast<HashTreeSegment **>(concurrency_fast_alloc(sizeof(HashTreeSegment *) * dir_size));
    treeNodeValues = static_cast<HashTreeKeyValue**>(concurrency_fast_alloc(sizeof(HashTreeKeyValue* ) * (1+HT_NODE_PREFIX_MAX_BITS/HT_NODE_LENGTH)));

    for (int i = 0; i < dir_size; ++i) {
        *(HashTreeSegment **)GET_SEG_POS(this,i) = new_vlht_segment();
    }
}

VarLengthHashTreeNode::~VarLengthHashTreeNode() {

}

void VarLengthHashTreeNode::init(int prefixLen, unsigned char headerDepth, unsigned char global_depth) {
    
    type = 0;
    this->global_depth = global_depth;
    this->dir_size = pow(2, global_depth);
    header.depth = headerDepth;
    header.len = prefixLen;
    lock_meta = 0;
    treeNodeValues = static_cast<HashTreeKeyValue**>(concurrency_fast_alloc(sizeof(HashTreeKeyValue* ) * (1+HT_NODE_PREFIX_MAX_BITS/HT_NODE_LENGTH)));

    // dir = static_cast<HashTreeSegment *>(concurrency_fast_alloc(sizeof(HashTreeSegment *) * dir_size));
    for (int i = 0; i < this->dir_size; ++i) {
        *(HashTreeSegment **)GET_SEG_POS(this,i) = new_vlht_segment(global_depth);
    }
}
void VarLengthHashTreeNode::init(VarLengthHashTreeNode* oldNode){
    type = 0;
    this->global_depth = oldNode->global_depth+1;
    this->dir_size = oldNode->dir_size*2;
    this->header.init(&oldNode->header,oldNode->header.len,oldNode->header.depth);
    treeNodeValues = static_cast<HashTreeKeyValue**>(concurrency_fast_alloc(sizeof(HashTreeKeyValue* ) * (1+HT_NODE_PREFIX_MAX_BITS/HT_NODE_LENGTH)));
    for(int i=0;i<1+HT_NODE_PREFIX_MAX_BITS/HT_NODE_LENGTH;i++){
        this->treeNodeValues[i] =  oldNode->treeNodeValues[i];
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
            }
            clflush((char *) GET_SEG_POS(this,right-1), sizeof(HashTreeSegment *)*(right-mid));

            tmp_seg->depth = tmp_seg->depth + 1;
            clflush((char *) &(tmp_seg->depth), sizeof(tmp_seg->depth));
            this->put(subkey, value, beforeAddress);
            return;
        } else {
            //condition: tmp_bucket->depth == global_depth
            VarLengthHashTreeNode *newNode = static_cast<VarLengthHashTreeNode *>(concurrency_fast_alloc(sizeof(VarLengthHashTreeNode)+sizeof(HashTreeSegment *)*dir_size*2));
            newNode->init(this);

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


bool VarLengthHashTreeNode::put_with_read_lock(uint64_t subkey, uint64_t value, HashTreeSegment* tmp_seg, HashTreeBucket* tmp_bucket, uint64_t dir_index, uint64_t seg_index, uint64_t beforeAddress){
    int bucket_index = tmp_bucket->find_place(subkey, HT_NODE_LENGTH, tmp_seg->depth);
    if (bucket_index == -1) {
        //condition: full
        if (likely(tmp_seg->depth < global_depth)) {
            
            if(!read_lock()){
                tmp_bucket->free_read_lock();
                tmp_seg->free_read_lock();
                std::this_thread::yield();
                return false;
            }
            if(this!=(*(VarLengthHashTreeNode**)beforeAddress)){
                tmp_bucket->free_read_lock();
                tmp_seg->free_read_lock();
                free_read_lock();
                std::this_thread::yield();
                return false;
            }

            // uint64_t dir_index = GET_SEG_NUM(subkey, HT_NODE_LENGTH, global_depth);
            // HashTreeSegment* old_seg = *(HashTreeSegment **)GET_SEG_POS((*(VarLengthHashTreeNode**)beforeAddress),dir_index);

            // if(old_seg!=tmp_seg||old_seg->depth!=tmp_seg->depth){
            //     tmp_bucket->free_read_lock();
            //     tmp_seg->free_read_lock();
            //     free_read_lock();
            //     std::this_thread::yield();
            //     return false;
            // }

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
            }
            clflush((char *) GET_SEG_POS(this,right-1), sizeof(HashTreeSegment *)*(right-mid));

            tmp_seg->depth = tmp_seg->depth + 1;
            clflush((char *) &(tmp_seg->depth), sizeof(tmp_seg->depth));
            // this->put(subkey, value, beforeAddress);

            tmp_bucket->free_read_lock();
            tmp_seg->free_write_lock();
            free_read_lock();

            return false;
        } else {
            //condition: tmp_bucket->depth == global_depth

            if(!write_lock()){
                tmp_bucket->free_read_lock();
                tmp_seg->free_read_lock();
                std::this_thread::yield();
                return false;
            }

            if((*(VarLengthHashTreeNode**)beforeAddress)!=this){
                tmp_bucket->free_read_lock();
                tmp_seg->free_read_lock();
                free_write_lock();
                std::this_thread::yield();
                return false;
            }

            VarLengthHashTreeNode *newNode = static_cast<VarLengthHashTreeNode *>(concurrency_fast_alloc(sizeof(VarLengthHashTreeNode)+sizeof(HashTreeSegment *)*dir_size*2));
            newNode->init(this);
            //set dir
            for (int i = 0; i < newNode->dir_size; ++i) {
                *(HashTreeSegment **)GET_SEG_POS(newNode,i) = *(HashTreeSegment **) GET_SEG_POS(this,(i / 2));
            }
            clflush((char *) newNode, sizeof(VarLengthHashTreeNode)+sizeof(HashTreeSegment *)*newNode->dir_size);
            *(VarLengthHashTreeNode**)beforeAddress = newNode;
            clflush((char *) beforeAddress, sizeof(VarLengthHashTreeNode*));

            tmp_bucket->free_read_lock();
            tmp_seg->free_read_lock();
            free_write_lock();
            // newNode->put(subkey, value, beforeAddress);
            return false;
        }
    } else {
        uint64_t old_value = tmp_bucket->counter[bucket_index].value;
        // free bucket read lock
        tmp_bucket->free_read_lock();
  
        if(!tmp_bucket->write_lock()){
            tmp_seg->free_read_lock();
            std::this_thread::yield();
            return false;
        }

        if(tmp_bucket->counter[bucket_index].value!=old_value){
            tmp_bucket->free_write_lock();
            tmp_seg->free_read_lock();
            std::this_thread::yield();
            return false;
        }

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
        tmp_bucket->free_write_lock();
        tmp_seg->free_read_lock();
    }
    return true;
}


uint64_t VarLengthHashTreeNode::get(uint64_t subkey) {
    uint64_t dir_index = GET_SEG_NUM(subkey, HT_NODE_LENGTH, global_depth);
    HashTreeSegment *tmp_seg = *(HashTreeSegment **)GET_SEG_POS(this,dir_index);
    uint64_t seg_index = GET_BUCKET_NUM(subkey, HT_BUCKET_MASK_LEN);
    HashTreeBucket *tmp_bucket = &(tmp_seg->bucket[seg_index]);
    return tmp_bucket->get(subkey);
}

VarLengthHashTreeNode *new_varlengthhashtree_node( int prefixLen, unsigned char headerDepth, unsigned char globalDepth) {
    VarLengthHashTreeNode *_new_node = static_cast<VarLengthHashTreeNode *>(concurrency_fast_alloc(sizeof(VarLengthHashTreeNode)+sizeof(HashTreeSegment *)*pow(2, globalDepth)));
    _new_node->init(prefixLen,headerDepth,globalDepth);
    return _new_node;
}

void VarLengthHashTreeNode::node_put(int pos, HashTreeKeyValue* kv){
    treeNodeValues[header.len - pos] = kv;
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
        if(pos+i>=len||key[pos+i]!=this->array[i]){
            return matchedLength - matchedLength%(HT_NODE_LENGTH/SIZE_OF_CHAR);
        }
        matchedLength++;
    }
    return matchedLength - matchedLength%(HT_NODE_LENGTH/SIZE_OF_CHAR);
}


void VarLengthHashTreeHeader::assign(unsigned char* key, unsigned char assignedLength){
    for(int i=0;i<assignedLength;i++){
        array[i] = key[i];
    }
}


bool VarLengthHashTreeNode::read_lock(){
    int64_t val = lock_meta;
    while(val>-1){
        if(cas(&lock_meta, &val, val+1)){
            return true;
        }
        val = lock_meta;
    }
    return false;
}

bool VarLengthHashTreeNode::write_lock(){
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

void VarLengthHashTreeNode::free_read_lock(){
    int64_t val = lock_meta;
    while(!cas(&lock_meta, &val, val-1)){
        val = lock_meta;
    }
}

void VarLengthHashTreeNode::free_write_lock(){
   lock_meta = 0;
}

Length64HashTreeKeyValue *new_l64ht_key_value(uint64_t key ,uint64_t value ){
    Length64HashTreeKeyValue *_new_key_value = static_cast<Length64HashTreeKeyValue *>(concurrency_fast_alloc(sizeof(Length64HashTreeKeyValue)));
    _new_key_value->key = key;
    _new_key_value->value = value;
    return _new_key_value;
}


uint64_t Length64HashTreeBucket::get(uint64_t key, bool& keyValueFlag){
    for (int i = 0; i < HT_BUCKET_SIZE; ++i) {
        if (key == REMOVE_NODE_FLAG(counter[i].subkey)) {
            keyValueFlag = GET_NODE_FLAG(counter[i].subkey);
            return counter[i].value;
        }
    }
    return 0;
}

int Length64HashTreeBucket::find_place(uint64_t _key, uint64_t _key_len, uint64_t _depth){
    // full: return -1
    // exists or not full: return index or empty counter
    int res = -1;
    for (int i = 0; i < HT_BUCKET_SIZE; ++i) {
        uint64_t removedFlagKey = REMOVE_NODE_FLAG(counter[i].subkey);
        if (_key == removedFlagKey) {
            return i;
        } else if ((res == -1) && removedFlagKey == 0 && counter[i].value == 0) {
            res = i;
        } else if ((res == -1) &&
                   (GET_SEG_NUM(_key, _key_len, _depth) != GET_SEG_NUM(removedFlagKey, _key_len, _depth))) { // todo: wrong logic
            res = i;
        }
    }
    return res;
}


Length64HashTreeBucket *new_l64ht_bucket(int _depth){
    Length64HashTreeBucket *_new_bucket = static_cast<Length64HashTreeBucket *>(concurrency_fast_alloc(sizeof(Length64HashTreeBucket)));
    return _new_bucket;
}

Length64HashTreeSegment::Length64HashTreeSegment(){
    depth = 0;
    bucket = static_cast<Length64HashTreeBucket *>(concurrency_fast_alloc(sizeof(Length64HashTreeBucket) * HT_MAX_BUCKET_NUM));
}

Length64HashTreeSegment::~Length64HashTreeSegment(){}

void Length64HashTreeSegment::init(uint64_t _depth){
    depth = _depth;
    bucket = static_cast<Length64HashTreeBucket *>(concurrency_fast_alloc(sizeof(Length64HashTreeBucket) * HT_MAX_BUCKET_NUM));
}


Length64HashTreeSegment *new_l64ht_segment(uint64_t _depth ){
    Length64HashTreeSegment *_new_ht_segment = static_cast<Length64HashTreeSegment *>(concurrency_fast_alloc(sizeof(Length64HashTreeSegment)));
    _new_ht_segment->init(_depth);
    return _new_ht_segment;
}

void Length64HashTreeHeader::init(Length64HashTreeHeader* oldHeader, unsigned char length, unsigned char depth){
    assign(oldHeader->array,length);
    this->depth = depth;
    this->len = length;
}
 
int Length64HashTreeHeader::computePrefix(uint64_t key, int startPos){
    if(this->len==0){
        return 0;
    }
    uint64_t subkey = GET_SUBKEY(key,startPos,(this->len*SIZE_OF_CHAR));
    subkey <<= (64 - this->len*SIZE_OF_CHAR);
    int res = 0;
    for(int i=0;i<this->len;i++){
        if((subkey>>56) != ((uint64_t)array[i])){
            break;
        }
        res++;
    }
    return res; 
}

void Length64HashTreeHeader::assign(uint64_t key, int startPos){
    uint64_t subkey = GET_SUBKEY(key,startPos,(this->len*SIZE_OF_CHAR));
    subkey <<= (HT_NODE_PREFIX_MAX_BITS - this->len*SIZE_OF_CHAR);
    for(int i=HT_NODE_PREFIX_MAX_BYTES-1;i>=0;i--){
        array[i] = (char)subkey & (((uint64_t)1<<8)-1);
        subkey >>= 8;
    }
}

void Length64HashTreeHeader::assign(unsigned char* key, unsigned char assignedLength){
    for(int i=0;i<assignedLength;i++){
        array[i] = key[i];
    }
}

Length64HashTreeNode::Length64HashTreeNode(){
    global_depth = 0;
    dir_size = pow(2, global_depth);
    // dir = static_cast<HashTreeSegment **>(concurrency_fast_alloc(sizeof(HashTreeSegment *) * dir_size));
    for (int i = 0; i < dir_size; ++i) {
        *(Length64HashTreeSegment **)GET_SEG_POS(this,i) = new_l64ht_segment();
    }
}

Length64HashTreeNode::~Length64HashTreeNode(){}

void Length64HashTreeNode::init(unsigned char headerDepth, unsigned char global_depth){
    this->global_depth = global_depth;
    this->dir_size = pow(2, global_depth);
    header.depth = headerDepth;
    treeNodeValues = static_cast<Length64HashTreeKeyValue*>(concurrency_fast_alloc(sizeof(Length64HashTreeKeyValue ) * (1+HT_NODE_PREFIX_MAX_BITS/HT_NODE_LENGTH)));
    // dir = static_cast<HashTreeSegment *>(fast_alloc(sizeof(HashTreeSegment *) * dir_size));
    for (int i = 0; i < this->dir_size; ++i) {
        *(Length64HashTreeSegment **)GET_SEG_POS(this,i) = new_l64ht_segment(global_depth);
    }
}

void Length64HashTreeNode::put(uint64_t subkey, uint64_t value, uint64_t beforeAddress){
    uint64_t dir_index = GET_SEG_NUM(subkey, HT_NODE_LENGTH, global_depth);
    Length64HashTreeSegment *tmp_seg = *(Length64HashTreeSegment **)GET_SEG_POS(this,dir_index);
    uint64_t seg_index = GET_BUCKET_NUM(subkey, HT_BUCKET_MASK_LEN);
    Length64HashTreeBucket *tmp_bucket = &(tmp_seg->bucket[seg_index]);
    put(subkey,value,tmp_seg,tmp_bucket,dir_index,seg_index,beforeAddress);
}

void Length64HashTreeNode::put(uint64_t subkey, uint64_t value, Length64HashTreeSegment* tmp_seg, Length64HashTreeBucket* tmp_bucket, uint64_t dir_index, uint64_t seg_index, uint64_t beforeAddress){
    int bucket_index = tmp_bucket->find_place(subkey, HT_NODE_LENGTH, tmp_seg->depth);
    if (bucket_index == -1) {
        //condition: full
        if (likely(tmp_seg->depth < global_depth)) {
            Length64HashTreeSegment *new_seg = new_l64ht_segment(tmp_seg->depth + 1);
            int64_t stride = pow(2, global_depth - tmp_seg->depth);
            int64_t left = dir_index - dir_index % stride;
            int64_t mid = left + stride / 2, right = left + stride;

            //migrate previous data to the new bucket
            for (int i = 0; i < HT_MAX_BUCKET_NUM; ++i) {
                uint64_t bucket_cnt = 0;
                for (int j = 0; j < HT_BUCKET_SIZE; ++j) {
                    uint64_t tmp_key = REMOVE_NODE_FLAG(tmp_seg->bucket[i].counter[j].subkey);
                    uint64_t tmp_value = tmp_seg->bucket[i].counter[j].value;
                    dir_index = GET_SEG_NUM(tmp_key, HT_NODE_LENGTH, global_depth);
                    if (dir_index >= mid) {
                        Length64HashTreeSegment *dst_seg = new_seg;
                        seg_index = i;
                        Length64HashTreeBucket *dst_bucket = &(dst_seg->bucket[seg_index]);
                        dst_bucket->counter[bucket_cnt].value = tmp_value;
                        dst_bucket->counter[bucket_cnt].subkey = tmp_seg->bucket[i].counter[j].subkey;
                        bucket_cnt++;
                    }
                }
            }
            clflush((char *) new_seg, sizeof(Length64HashTreeSegment));

            // set dir[mid, right) to the new bucket
            for (int i = right - 1; i >= mid; --i) {
                *(Length64HashTreeSegment **)GET_SEG_POS(this,i) = new_seg;
            }
            clflush((char *) GET_SEG_POS(this,right-1), sizeof(Length64HashTreeSegment *)*(right-mid));

            tmp_seg->depth = tmp_seg->depth + 1;
            clflush((char *) &(tmp_seg->depth), sizeof(tmp_seg->depth));
            this->put(subkey, value, beforeAddress);
            return;
        } else {
            //condition: tmp_bucket->depth == global_depth
            Length64HashTreeNode *newNode = static_cast<Length64HashTreeNode *>(concurrency_fast_alloc(sizeof(Length64HashTreeNode)+sizeof(HashTreeSegment *)*dir_size*2));
            newNode->global_depth = global_depth+1;
            newNode->dir_size = dir_size*2;
            newNode->header.init(&this->header,this->header.len,this->header.depth);
            //set dir
            for (int i = 0; i < newNode->dir_size; ++i) {
                *(Length64HashTreeSegment **)GET_SEG_POS(newNode,i) = *(Length64HashTreeSegment **) GET_SEG_POS(this,(i / 2));
            }
            clflush((char *) newNode, sizeof(Length64HashTreeNode)+sizeof(Length64HashTreeSegment *)*newNode->dir_size);
            *(Length64HashTreeNode**)beforeAddress = newNode;
            clflush((char *) beforeAddress, sizeof(Length64HashTreeNode*));
            newNode->put(subkey, value, beforeAddress);
            return;
        }
    } else {
        if (unlikely(tmp_bucket->counter[bucket_index].subkey == subkey) && subkey != 0) {
            //key exists
            tmp_bucket->counter[bucket_index].value = value;
            clflush((char *) &(tmp_bucket->counter[bucket_index].value), 8);
        } else {
            // there is a place to insert
            tmp_bucket->counter[bucket_index].value = value;
            mfence();
            tmp_bucket->counter[bucket_index].subkey = PUT_KEY_VALUE_FLAG(subkey);
            // Here we clflush 16bytes rather than two 8 bytes because all counter are set to 0.
            // If crash after key flushed, then the value is 0. When we return the value, we would find that the key is not inserted.
            clflush((char *) &(tmp_bucket->counter[bucket_index].subkey), 16);
        }
    }
    return;
}

void Length64HashTreeNode::node_put(int pos, Length64HashTreeKeyValue* kv){
    treeNodeValues[header.len - pos] = *kv;
}

uint64_t Length64HashTreeNode::get(uint64_t subkey, bool& keyValueFlag){
    uint64_t dir_index = GET_SEG_NUM(subkey, HT_NODE_LENGTH, global_depth);
    Length64HashTreeSegment *tmp_seg = *(Length64HashTreeSegment **)GET_SEG_POS(this,dir_index);
    uint64_t seg_index = GET_BUCKET_NUM(subkey, HT_BUCKET_MASK_LEN);
    Length64HashTreeBucket *tmp_bucket = &(tmp_seg->bucket[seg_index]);
    return tmp_bucket->get(subkey, keyValueFlag);
}


Length64HashTreeNode *new_length64hashtree_node(int _key_len, unsigned char headerDepth, unsigned char globalDepth){
    Length64HashTreeNode *_new_node = static_cast<Length64HashTreeNode *>(concurrency_fast_alloc(sizeof(Length64HashTreeNode)+sizeof(Length64HashTreeSegment *)*pow(2, globalDepth)));
    _new_node->init(headerDepth,globalDepth);
    return _new_node;
}