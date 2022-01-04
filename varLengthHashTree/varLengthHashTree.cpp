//
// Created by 杨冠群 on 2021-07-08.
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "varLengthHashTree.h"

/*
 *         begin  len
 * key [______|___________|____________]
 */
#define GET_SUB_KEY(key, begin, len)  (((key)>>(64-(begin)-(len)))&(((uint64_t)1<<(len))-1))
#define GET_SEG_NUM(key, key_len, depth)  ((key>>(key_len-depth))&(((uint64_t)1<<depth)-1))
#define GET_BUCKET_NUM(key, bucket_mask_len) ((key)&(((uint64_t)1<<bucket_mask_len)-1))

#define GET_SEG_POS(currentNode,dir_index) (((uint64_t)(currentNode) + sizeof(VarLengthHashTreeNode) + dir_index*sizeof(HashTreeSegment*)))

#define GET_32BITS(pointer,pos) (*((uint32_t *)(pointer+pos)))
// #define GET_32BITS(pointer,pos) ((*(pointer+pos))<<24 | (*(pointer+pos+1))<< 16 | (*(pointer+pos+2))<<8 | *(pointer+pos+3))
#define GET_16BITS(pointer,pos) ((*(pointer+pos))<<8 | (*(pointer+pos+1)))

#define _16_BITS_OF_BYTES 2
#define _32_BITS_OF_BYTES 4


bool keyIsSame(unsigned char* key1, unsigned int length1, unsigned char* key2, unsigned int length2){
    if(length1!=length2){
        return false;
    }
    return strncmp((char *)key1,(char *)key2,length1)==0;
    for(int i=0;i<length1;i++){
        if(*(key1+i)!=*(key2+i)){
            return false;
        }
    }
    return true;
}
bool keyIsSame(unsigned char* key1, unsigned char* key2, unsigned int length){
//    return keyIsSame(key1,length,key2,length);
    return strncmp((char *)key1,(char *)key2,length)==0;
}

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

bool isSame(unsigned char* key1, uint64_t key2, int pos, int length){
    uint64_t subkey = GET_SUBKEY(key2,pos,length);
    subkey <<= (64 - length);
    for(int i = 0;i<length/SIZE_OF_CHAR;i++){
        if((subkey>>56)!=(uint64_t)key1[i]){
            return false;
        }
        subkey<<=SIZE_OF_CHAR;
    }
    return true;
}

VarLengthHashTree::VarLengthHashTree() {
    root = new_varlengthhashtree_node(HT_NODE_LENGTH);
}

VarLengthHashTree::VarLengthHashTree(int _span, int _init_depth) {
    init_depth = _init_depth;
    root = new_varlengthhashtree_node(HT_NODE_LENGTH);
}

VarLengthHashTree::~VarLengthHashTree() {
    delete root;
}

void VarLengthHashTree::init(int prefixLen) {
    root = new_varlengthhashtree_node(prefixLen);
}

VarLengthHashTree *new_varLengthHashtree() {
    VarLengthHashTree *_new_hash_tree = static_cast<VarLengthHashTree *>(concurrency_fast_alloc(sizeof(VarLengthHashTree)));
    _new_hash_tree->init(0);
    return _new_hash_tree;
}


void VarLengthHashTree:: crash_consistent_put(VarLengthHashTreeNode *_node, int length, unsigned char* key, uint64_t value, int pos){
    crash_consistent_put(_node,length,key,value,(uint64_t)&root,pos);
}


void VarLengthHashTree:: crash_consistent_put(VarLengthHashTreeNode *_node, int length, unsigned char* key, uint64_t value, uint64_t beforeAddress, int pos){
    // HashTreeBucket* lock_bucket = NULL;
    HashTreeSegment* lock_segment = NULL;

    while(length>=pos){
        PUT_RETRY:
            VarLengthHashTreeNode *currentNode = *(VarLengthHashTreeNode**)beforeAddress;
            unsigned char headerDepth = currentNode->header.depth;

            if(!currentNode->read_lock()){
                std::this_thread::yield();
                goto PUT_RETRY;
            }

            if(*(VarLengthHashTreeNode**)beforeAddress!=currentNode){
                currentNode->free_read_lock();
                std::this_thread::yield();
                goto PUT_RETRY;
            }

            int matchedPrefixLen = 0;
            // init a number bigger than HT_NODE_PREFIX_MAX_LEN to represent there is no value
            if(currentNode->header.len>HT_NODE_PREFIX_MAX_BYTES){
                currentNode->free_read_lock();
                if(!currentNode->write_lock()){
                    std::this_thread::yield();
                    goto PUT_RETRY;
                }

                // double check
                if((*(VarLengthHashTreeNode**)beforeAddress!=currentNode)||currentNode->header.len<=HT_NODE_PREFIX_MAX_BYTES){
                    currentNode->free_write_lock();
                    std::this_thread::yield();
                    goto PUT_RETRY;
                }

                int maxSize = HT_NODE_PREFIX_MAX_BYTES - HT_NODE_PREFIX_MAX_BYTES%(HT_NODE_LENGTH/SIZE_OF_CHAR);
                currentNode->header.len = min((length - pos),maxSize);
                currentNode->header.assign(key+pos,currentNode->header.len);

                currentNode->free_write_lock();
                goto PUT_RETRY;
            }else{
                // compute this prefix
                matchedPrefixLen = currentNode->header.computePrefix(key,length,pos);
            }

            if(pos + matchedPrefixLen == length){
                HashTreeKeyValue *kv = new_vlht_key_value(key, length, value);
                clflush((char *) kv, sizeof(HashTreeKeyValue));
                currentNode->node_put(matchedPrefixLen,kv);
                if(lock_segment!=NULL){
                    // lock_bucket->free_read_lock();
                    lock_segment->free_read_lock();
                }
                currentNode->free_read_lock();
                return;
            }

            if(matchedPrefixLen == currentNode->header.len){
                // if prefix is match
                // move the pos

                pos += currentNode->header.len;
                currentNode->free_read_lock();

                // compute subkey (16bits as default)
                // uint64_t subkey = GET_16BITS(key,pos);

                CCEH_RETRY:
                    uint64_t subkey = GET_32BITS(key,pos);

                    // use the subkey to search in a cceh node
                    uint64_t next = 0;

                    uint64_t dir_index = GET_SEG_NUM(subkey, HT_NODE_LENGTH, (*(VarLengthHashTreeNode**)beforeAddress)->global_depth);
                    HashTreeSegment *tmp_seg = *(HashTreeSegment **)GET_SEG_POS((*(VarLengthHashTreeNode**)beforeAddress),dir_index);

                    // read lock segment
                    if(!tmp_seg->read_lock()){
                        std::this_thread::yield();
                        goto CCEH_RETRY;
                    }

                    uint64_t old_dir_index = GET_SEG_NUM(subkey, HT_NODE_LENGTH, (*(VarLengthHashTreeNode**)beforeAddress)->global_depth);
                    HashTreeSegment * old_seg = *(HashTreeSegment **)GET_SEG_POS((*(VarLengthHashTreeNode**)beforeAddress),old_dir_index);

                    if(old_seg!=tmp_seg||old_seg->depth != tmp_seg->depth){
                        tmp_seg->free_read_lock();
                        std::this_thread::yield();
                        goto CCEH_RETRY;
                    }

                    uint64_t seg_index = GET_BUCKET_NUM(subkey, HT_BUCKET_MASK_LEN);
                    HashTreeBucket *tmp_bucket = &(tmp_seg->bucket[seg_index]);

                    if(!tmp_bucket->read_lock()){
                        tmp_seg->free_read_lock();
                        std::this_thread::yield();
                        goto CCEH_RETRY;
                    }

                    int i;
                    uint64_t beforeA;
                    for (i = 0; i < HT_BUCKET_SIZE; ++i) {
                        if (subkey == tmp_bucket->counter[i].subkey) {
                            next =  tmp_bucket->counter[i].value;
                            beforeA = (uint64_t)&tmp_bucket->counter[i].value;
                            break;
                        }
                    }

                    if (next == 0) {
                        //not exists
                        HashTreeKeyValue *kv = new_vlht_key_value(key, length, value);
                        clflush((char *) kv, sizeof(HashTreeKeyValue));
                        if(!(*(VarLengthHashTreeNode**)beforeAddress)->put_with_read_lock(subkey, (uint64_t) kv, tmp_seg, tmp_bucket, dir_index, seg_index, beforeAddress)){
                            goto CCEH_RETRY;
                        }
                        if(lock_segment!=NULL){
                            // lock_bucket->free_read_lock();
                            lock_segment->free_read_lock();
                        }
                        return;
                    } else {

                        if (((bool *) next)[0]) {
                            // next is key value pair, which means collides

                            tmp_bucket->free_read_lock();

                            if(!tmp_bucket->write_lock()){
                                tmp_seg->free_read_lock();
                                std::this_thread::yield();
                                goto CCEH_RETRY;
                            }

                            if(tmp_bucket->counter[i].value!=next){
                                tmp_bucket->free_write_lock();
                                tmp_seg->free_read_lock();
                                std::this_thread::yield();
                                goto CCEH_RETRY;
                            }

                            unsigned char* preKey = ((HashTreeKeyValue *) next)->key;
                            unsigned int preLength = ((HashTreeKeyValue *) next)->len;
                            uint64_t preValue = ((HashTreeKeyValue *) next)->value;

                            if (unlikely(keyIsSame(key,length,preKey,preLength))) {
                                //same key, update the value
                                ((HashTreeKeyValue *) next)->value = value;
                                clflush((char *) &(((HashTreeKeyValue *) next)->value), 8);
                            } else {
                                //not same key: needs to create a new node
                                VarLengthHashTreeNode *newNode = new_varlengthhashtree_node(HT_NODE_PREFIX_MAX_BYTES+1,headerDepth+1);

                                // put pre kv
                                // uint64_t preSubkey = GET_16BITS(preKey,pos);
                                crash_consistent_put_without_lock(newNode,preLength,preKey,preValue,(uint64_t)&newNode,pos + HT_NODE_LENGTH/SIZE_OF_CHAR);
                                // put new kv
                                crash_consistent_put_without_lock(newNode,length,key,value,(uint64_t)&newNode,pos + HT_NODE_LENGTH/SIZE_OF_CHAR);
                                clflush((char *) newNode, sizeof(VarLengthHashTreeNode));

                                tmp_bucket->counter[i].value = (uint64_t)newNode;
                                clflush((char*)&tmp_bucket->counter[i].value,8);
                            }
                                // TODO comfirm the order of clflush and unlock
                            tmp_bucket->free_write_lock();
                            tmp_seg->free_read_lock();

                            if(lock_segment!=NULL){
                                // lock_bucket->free_read_lock();
                                lock_segment->free_read_lock();
                            }
                            return;
                        } else {
                            // next is next extendible hash

                            pos += HT_NODE_LENGTH/SIZE_OF_CHAR;

                            tmp_bucket->free_read_lock();
                            if(lock_segment!=NULL){
                                // lock_bucket->free_read_lock();
                                lock_segment->free_read_lock();
                            }

                            // lock_bucket = tmp_bucket;
                            lock_segment = tmp_seg;

                            currentNode = (VarLengthHashTreeNode *) next;
                            beforeAddress =  beforeA;
                            headerDepth = currentNode->header.depth;
                        }
                    }
            }else{
                // if prefix is not match (shorter)
                // split a new tree node and insert

                currentNode->free_read_lock();
                if(!currentNode->write_lock()){
                    std::this_thread::yield();
                    goto PUT_RETRY;
                }

                // double check
                if(*(VarLengthHashTreeNode**)beforeAddress!=currentNode){
                    currentNode->free_write_lock();
                    std::this_thread::yield();
                    goto PUT_RETRY;
                }

                // build new tree node
                VarLengthHashTreeNode *newNode = new_varlengthhashtree_node(0,headerDepth);
                newNode->header.init(&currentNode->header,matchedPrefixLen,currentNode->header.depth);
                for(int j=currentNode->header.len - matchedPrefixLen;j<=currentNode->header.len;j++){
                    newNode->treeNodeValues[j - currentNode->header.len + matchedPrefixLen] = currentNode->treeNodeValues[j];
                }

                // uint64_t subkey = GET_16BITS(key,matchedPrefixLen);
                uint64_t subkey = GET_32BITS(key,pos+matchedPrefixLen);
                HashTreeKeyValue *kv = new_vlht_key_value(key, length, value);
                clflush((char *) kv, sizeof(HashTreeKeyValue));

                newNode->put(subkey,(uint64_t)kv,(uint64_t)&newNode);
                // newNode->put(GET_16BITS(currentNode->header.array,matchedPrefixLen),(uint64_t)currentNode);
                newNode->put(GET_32BITS(currentNode->header.array,matchedPrefixLen),(uint64_t)currentNode,(uint64_t)&newNode);

                // modify currentNode
                matchedPrefixLen += 4;
                for(int i=0;i<HT_NODE_PREFIX_MAX_BYTES-matchedPrefixLen;i++){
                    currentNode->header.array[i] = currentNode->header.array[i+matchedPrefixLen];
                }
                currentNode->header.depth -= matchedPrefixLen;
                currentNode->header.len -= matchedPrefixLen;

                clflush((char*)&(currentNode->header),8);
                clflush((char *) newNode, sizeof(VarLengthHashTreeNode));

                // modify the successor
                *(VarLengthHashTreeNode**)beforeAddress = newNode;

                currentNode->free_write_lock();
                if(lock_segment!=NULL){
                    // lock_bucket->free_read_lock();
                    lock_segment->free_read_lock();
                }
                return;
            }
    }
}


void VarLengthHashTree:: crash_consistent_put_without_lock(VarLengthHashTreeNode *_node, int length, unsigned char* key, uint64_t value, uint64_t beforeAddress, int pos){
    while(length>=pos){
            VarLengthHashTreeNode *currentNode = *(VarLengthHashTreeNode**)beforeAddress;


            int matchedPrefixLen = 0;
            // init a number bigger than HT_NODE_PREFIX_MAX_LEN to represent there is no value
            if(currentNode->header.len>HT_NODE_PREFIX_MAX_BYTES){
                int maxSize = HT_NODE_PREFIX_MAX_BYTES - HT_NODE_PREFIX_MAX_BYTES%(HT_NODE_LENGTH/SIZE_OF_CHAR);
                currentNode->header.len = min((length - pos),maxSize);
                currentNode->header.assign(key+pos,currentNode->header.len);
                // pos += HT_NODE_PREFIX_MAX_BYTES;
                continue;
            }else{
                // compute this prefix
                matchedPrefixLen = currentNode->header.computePrefix(key,length,pos);
            }
            if(pos + matchedPrefixLen == length){
                HashTreeKeyValue *kv = new_vlht_key_value(key, length, value);
                clflush((char *) kv, sizeof(HashTreeKeyValue));
                currentNode->node_put(matchedPrefixLen,kv);
                return;
            }
            if(matchedPrefixLen == currentNode->header.len){
                // if prefix is match
                // move the pos

            pos += currentNode->header.len;
            // compute subkey (16bits as default)
            // uint64_t subkey = GET_16BITS(key,pos);

            uint64_t subkey = GET_32BITS(key,pos);

            // use the subkey to search in a cceh node
            uint64_t next = 0;

            uint64_t dir_index = GET_SEG_NUM(subkey, HT_NODE_LENGTH, currentNode->global_depth);
            HashTreeSegment *tmp_seg = *(HashTreeSegment **)GET_SEG_POS(currentNode,dir_index);
            uint64_t seg_index = GET_BUCKET_NUM(subkey, HT_BUCKET_MASK_LEN);
            HashTreeBucket *tmp_bucket = &(tmp_seg->bucket[seg_index]);

            int i;
            uint64_t beforeA;
            for (i = 0; i < HT_BUCKET_SIZE; ++i) {
                if (subkey == tmp_bucket->counter[i].subkey) {
                    next =  tmp_bucket->counter[i].value;
                    beforeA = (uint64_t)&tmp_bucket->counter[i].value;
                    break;
                }
            }
            if (next == 0) {
                //not exists
                HashTreeKeyValue *kv = new_vlht_key_value(key, length, value);
                clflush((char *) kv, sizeof(HashTreeKeyValue));
                currentNode->put(subkey, (uint64_t) kv, tmp_seg, tmp_bucket, dir_index, seg_index, beforeAddress);
                return;
            } else {
                if (((bool *) next)[0]) {
                    // next is key value pair, which means collides
                    unsigned char* preKey = ((HashTreeKeyValue *) next)->key;
                    unsigned int preLength = ((HashTreeKeyValue *) next)->len;
                    uint64_t preValue = ((HashTreeKeyValue *) next)->value;
                    if (unlikely(keyIsSame(key,length,preKey,preLength))) {
                        //same key, update the value
                        ((HashTreeKeyValue *) next)->value = value;
                        clflush((char *) &(((HashTreeKeyValue *) next)->value), 8);
                    } else {
                        //not same key: needs to create a new node
                        VarLengthHashTreeNode *newNode = new_varlengthhashtree_node(HT_NODE_PREFIX_MAX_BYTES+1,currentNode->header.depth+matchedPrefixLen*SIZE_OF_CHAR/HT_NODE_LENGTH+1);

                        // put pre kv
                        // uint64_t preSubkey = GET_16BITS(preKey,pos);
                        crash_consistent_put_without_lock(newNode,preLength,preKey,preValue,(uint64_t)&newNode,pos + HT_NODE_LENGTH/SIZE_OF_CHAR);
                        // put new kv
                        crash_consistent_put_without_lock(newNode,length,key,value,(uint64_t)&newNode,pos + HT_NODE_LENGTH/SIZE_OF_CHAR);
                        clflush((char *) newNode, sizeof(VarLengthHashTreeNode));

                        tmp_bucket->counter[i].value = (uint64_t)newNode;
                        clflush((char*)&tmp_bucket->counter[i].value,8);
                    }
                    return;
                } else {
                    // next is next extendible hash
                    pos += HT_NODE_LENGTH/SIZE_OF_CHAR;
                    currentNode = (VarLengthHashTreeNode *) next;
                    beforeAddress =  beforeA;
                }
            }
        }else{
            // if prefix is not match (shorter)
            // split a new tree node and insert 

            // build new tree node
            VarLengthHashTreeNode *newNode = new_varlengthhashtree_node(0,currentNode->header.depth);
            newNode->header.init(&currentNode->header,matchedPrefixLen,currentNode->header.depth);
            for(int j=currentNode->header.len - matchedPrefixLen;j<=currentNode->header.len;j++){
                newNode->treeNodeValues[j - currentNode->header.len + matchedPrefixLen] = currentNode->treeNodeValues[j];
            }

            // uint64_t subkey = GET_16BITS(key,matchedPrefixLen);
            uint64_t subkey = GET_32BITS(key,(pos+matchedPrefixLen));
            HashTreeKeyValue *kv = new_vlht_key_value(key, length, value);
            clflush((char *) kv, sizeof(HashTreeKeyValue));

            newNode->put(subkey,(uint64_t)kv,(uint64_t)&newNode);
            // newNode->put(GET_16BITS(currentNode->header.array,matchedPrefixLen),(uint64_t)currentNode);

            newNode->put(GET_32BITS(currentNode->header.array,matchedPrefixLen),(uint64_t)currentNode,(uint64_t)&newNode);

            matchedPrefixLen += HT_NODE_LENGTH/SIZE_OF_CHAR;

            // modify currentNode 
            unsigned char tmpHeader[HT_NODE_PREFIX_MAX_BYTES];
            for(int i=0;i<matchedPrefixLen;i++){
                tmpHeader[i] = currentNode->header.array[i];
            }
            for(int i=0;i<HT_NODE_PREFIX_MAX_BYTES-matchedPrefixLen ;i++){
                currentNode->header.array[i] = currentNode->header.array[i+matchedPrefixLen];
            }
            for(int i=HT_NODE_PREFIX_MAX_BYTES-matchedPrefixLen;i<HT_NODE_PREFIX_MAX_BYTES;i++){
                currentNode->header.array[i] = tmpHeader[i-HT_NODE_PREFIX_MAX_BYTES+matchedPrefixLen];
            }
            currentNode->header.depth += matchedPrefixLen*SIZE_OF_CHAR/HT_NODE_LENGTH;
            currentNode->header.len -= matchedPrefixLen;

            clflush((char*)&(currentNode->header),8);
            clflush((char *) newNode, sizeof(VarLengthHashTreeNode));

            // modify the successor 
            *(VarLengthHashTreeNode**)beforeAddress = newNode;
            return;
        }
    }
}

uint64_t VarLengthHashTree::get(int length, unsigned char* key){
    auto currentNode = root;
    if(currentNode==NULL){
        return 0;
    }
    int  pos  = 0;
    while(pos<=length){
        if(length-pos <= currentNode->header.len){
            if(currentNode->treeNodeValues[currentNode->header.len - length+pos] == NULL){
                return 0;
            }
            if(keyIsSame(key,length,(currentNode->treeNodeValues[currentNode->header.len - length+pos]->key),currentNode->treeNodeValues[currentNode->header.len - length+pos]->len)){
                return currentNode->treeNodeValues[currentNode->header.len - length+pos]->value;
            }else{
                return 0;
            }
        }
        if(strncmp((char *)key+pos,(char *)currentNode->header.array,currentNode->header.len)){
            return 0;
        }
        pos += currentNode->header.len;
        // uint64_t subkey = GET_16BITS(key,pos);
        uint64_t subkey = GET_32BITS(key,pos);

        auto next = currentNode->get(subkey);
        // pos+=_16_BITS_OF_BYTES;
        pos+=_32_BITS_OF_BYTES;
        if(next==0){
            return 0;
        }
        if((((bool *) next)[0])){
            // is value
            // if(pos==length){
            return ((HashTreeKeyValue*)next)->value;
            // }else{
            //     return 0;
            // }
        }else{
            currentNode = (VarLengthHashTreeNode*)next;
        }
    }
    return 0;
}

bool VarLengthHashTree::read_lock(){
    int64_t val = lock_meta;
    while(val>-1){
        if(cas(&lock_meta, &val, val+1)){
            return true;
        }
        val = lock_meta;
    }
    return false;
}

bool VarLengthHashTree::write_lock(){
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

void VarLengthHashTree::free_read_lock(){
    int64_t val = lock_meta;
    while(!cas(&lock_meta, &val, val-1)){
        val = lock_meta;
    }
}

void VarLengthHashTree::free_write_lock(){
    lock_meta = 0;
}


void recovery(VarLengthHashTreeNode* root, int depth){
    if(!root){
        return;
    }
    if(root->header.depth!=depth){
        int matchedPrefixLen = (depth - root->header.depth)*HT_NODE_LENGTH/SIZE_OF_CHAR;
        unsigned char tmpHeader[HT_NODE_PREFIX_MAX_BYTES];
        for(int i=0;i<HT_NODE_PREFIX_MAX_BYTES - matchedPrefixLen;i++){
            tmpHeader[i] = root->header.array[i];
        }
        for(int i=HT_NODE_PREFIX_MAX_BYTES-matchedPrefixLen;i<HT_NODE_PREFIX_MAX_BYTES ;i++){
            root->header.array[i] = root->header.array[i-HT_NODE_PREFIX_MAX_BYTES+matchedPrefixLen];
        }
        for(int i=0;i<matchedPrefixLen;i++){
            root->header.array[i] = tmpHeader[i];
        }
        root->header.depth = depth;
    }

    for(int i=0;i<root->dir_size;i++){
        HashTreeSegment* current_seg = *(HashTreeSegment **)GET_SEG_POS(root,i);
        int64_t stride = pow(2, root->global_depth - current_seg->depth);
        int64_t left = i - i % stride;
        int64_t mid = left + stride / 2, right = left + stride;
        for(int j =0;j<HT_MAX_BUCKET_NUM;j++){
            for(int k=0;k<HT_BUCKET_SIZE;k++){
                if(current_seg->bucket[j].counter[k].subkey!=0){
                    if(!((bool *)current_seg->bucket[j].counter[k].value)[0]){
                        recovery((VarLengthHashTreeNode*)current_seg->bucket[j].counter[k].value,depth+root->header.len*SIZE_OF_CHAR/HT_NODE_LENGTH+1);
                    }
                }
            }
        }
        int64_t before = i;
        int64_t flag  = -1;
        for(int j = left;j<right;j++){
            if((*(HashTreeSegment **)GET_SEG_POS(root,j))->depth!=(*(HashTreeSegment **)GET_SEG_POS(root,before))->depth){
                if((*(HashTreeSegment **)GET_SEG_POS(root,before))->depth>(*(HashTreeSegment **)GET_SEG_POS(root,j))->depth){
                    flag = before;
                    before = j;
                    break;
                }else{
                    flag = j;
                    break;
                }
            }
        }
        if(flag!=-1){
            int64_t need_recovery_stride = pow(2, root->global_depth - (*(HashTreeSegment **)GET_SEG_POS(root,flag))->depth);
            int64_t need_recovery_left = flag - flag % need_recovery_stride;
            int64_t need_recovery_mid = need_recovery_left + need_recovery_stride / 2, need_recovery_right = need_recovery_left + need_recovery_stride;
            for(int j = need_recovery_left;j<need_recovery_right;j++){
                if((*(HashTreeSegment **)GET_SEG_POS(root,j))->depth!=(*(HashTreeSegment **)GET_SEG_POS(root,flag))->depth){
                    (*(HashTreeSegment **)GET_SEG_POS(root,j)) =  (*(HashTreeSegment **)GET_SEG_POS(root,flag));
                    clflush((char *) (*(HashTreeSegment **)GET_SEG_POS(root,j)), sizeof(HashTreeSegment *));
                }
                (*(HashTreeSegment **)GET_SEG_POS(root,before))->depth +=1;
                clflush((char *) &((*(HashTreeSegment **)GET_SEG_POS(root,before))->depth), sizeof((*(HashTreeSegment **)GET_SEG_POS(root,before))->depth));
            }
        }
        i = right-1;
    }
}

void Length64HashTree:: crash_consistent_put(Length64HashTreeNode *_node, uint64_t key, uint64_t value, int len){
    // 0-index of current bytes

    Length64HashTreeNode *currentNode = _node;
    if (_node == NULL){
        currentNode = root;
    }
    unsigned char headerDepth = currentNode->header.depth;
    uint64_t beforeAddress = (uint64_t)&root;

    while(len < HT_KEY_LENGTH / SIZE_OF_CHAR){
        int matchedPrefixLen;

        // init a number larger than HT_NODE_PREFIX_MAX_LEN to represent there is no value
        if(currentNode->header.len>HT_NODE_PREFIX_MAX_BYTES){
            int size = HT_NODE_PREFIX_MAX_BITS/HT_NODE_LENGTH * HT_NODE_LENGTH;

            currentNode->header.len = (HT_KEY_LENGTH - len) <= size ? (HT_KEY_LENGTH - len)/SIZE_OF_CHAR : size/SIZE_OF_CHAR;
            currentNode->header.assign(key,len);
            len += size/SIZE_OF_CHAR;
            continue;
        }else{
            // compute this prefix
            matchedPrefixLen = currentNode->header.computePrefix(key,len*SIZE_OF_CHAR);
        }
        if(len + matchedPrefixLen == (HT_KEY_LENGTH / SIZE_OF_CHAR)){
            Length64HashTreeKeyValue *kv = new_l64ht_key_value(key, value);
            clflush((char *) kv, sizeof(Length64HashTreeKeyValue));
            currentNode->node_put(matchedPrefixLen,kv);
            return;
        }
        if(matchedPrefixLen == currentNode->header.len){
            // if prefix is match 
            // move the pos
            len += currentNode->header.len;

            // compute subkey (16bits as default)
            // uint64_t subkey = GET_16BITS(key,pos);
            uint64_t subkey = GET_SUBKEY(key,len*SIZE_OF_CHAR,HT_NODE_LENGTH);

            // use the subkey to search in a cceh node
            uint64_t next = 0;

            uint64_t dir_index = GET_SEG_NUM(subkey, HT_NODE_LENGTH, currentNode->global_depth);
            Length64HashTreeSegment *tmp_seg = *(Length64HashTreeSegment **)GET_SEG_POS(currentNode,dir_index);
            uint64_t seg_index = GET_BUCKET_NUM(subkey, HT_BUCKET_MASK_LEN);
            Length64HashTreeBucket *tmp_bucket = &(tmp_seg->bucket[seg_index]);
            int i;
            bool keyValueFlag = false;
            uint64_t beforeA;
            for (i = 0; i < HT_BUCKET_SIZE; ++i) {
                if (subkey == REMOVE_NODE_FLAG(tmp_bucket->counter[i].subkey)) {
                    next =  tmp_bucket->counter[i].value;
                    keyValueFlag = GET_NODE_FLAG(tmp_bucket->counter[i].subkey);
                    beforeA = (uint64_t)&tmp_bucket->counter[i].value;
                    break;
                }
            }
            len += HT_NODE_LENGTH / SIZE_OF_CHAR;
            if (len == 8) {
                if (next == 0) {
                    currentNode->put(subkey, (uint64_t) value, tmp_seg, tmp_bucket, dir_index, seg_index, beforeAddress);
                    return;
                } else {
                    tmp_bucket->counter[i].value = value;
                    clflush((char *) &tmp_bucket->counter[i].value, 8);
                    return;
                }
            } else {
                if (next == 0) {
                    //not exists
                    Length64HashTreeKeyValue *kv = new_l64ht_key_value(key, value);
                    clflush((char *) kv, sizeof(Length64HashTreeKeyValue));
                    currentNode->put(subkey, (uint64_t) kv, tmp_seg, tmp_bucket, dir_index, seg_index, beforeAddress);
                    return;
                } else {
                    if (keyValueFlag) {
                        // next is key value pair, which means collides
                        uint64_t prekey = ((Length64HashTreeKeyValue *) next)->key;
                        uint64_t prevalue = ((Length64HashTreeKeyValue *) next)->value;
                        if (unlikely(key == prekey)) {
                            //same key, update the value
                            ((Length64HashTreeKeyValue *) next)->value = value;
                            clflush((char *) &(((Length64HashTreeKeyValue *) next)->value), 8);
                            return;
                        } else {
                            //not same key: needs to create a new node
                            Length64HashTreeNode *newNode = new_length64hashtree_node(HT_NODE_LENGTH, headerDepth + 1);

                            // put pre kv
                            crash_consistent_put(newNode, prekey, prevalue, len);

                            // put new kv
                            crash_consistent_put(newNode, key, value, len);

                            clflush((char *) newNode, sizeof(Length64HashTreeNode));

                            // todo: think of crash consistency
                            tmp_bucket->counter[i].subkey = REMOVE_NODE_FLAG(tmp_bucket->counter[i].subkey);
                            tmp_bucket->counter[i].value = (uint64_t) newNode;
                            clflush((char *) &tmp_bucket->counter[i].value, 8);
                            return;
                        }
                    } else {
                        // next is next extendible hash
                        currentNode = (Length64HashTreeNode *) next;
                        beforeAddress = beforeA;
                        headerDepth = currentNode->header.depth;
                    }
                }
            }
        }else{
            // if prefix is not match (shorter)
            // split a new tree node and insert 

            // build new tree node
            Length64HashTreeNode *newNode = new_length64hashtree_node(HT_NODE_LENGTH,headerDepth);
            newNode->header.init(&currentNode->header,matchedPrefixLen,currentNode->header.depth);

            for(int j=currentNode->header.len - matchedPrefixLen;j<=currentNode->header.len;j++){
                newNode->treeNodeValues[j - currentNode->header.len + matchedPrefixLen] = currentNode->treeNodeValues[j];
            }

            uint64_t subkey = GET_SUBKEY(key,(len+matchedPrefixLen)*SIZE_OF_CHAR,HT_NODE_LENGTH);

            Length64HashTreeKeyValue *kv = new_l64ht_key_value(key, value);
            clflush((char *) kv, sizeof(Length64HashTreeKeyValue));

            newNode->put(subkey,(uint64_t)kv,(uint64_t)&newNode);
            // newNode->put(GET_16BITS(currentNode->header.array,matchedPrefixLen),(uint64_t)currentNode);
            newNode->put(GET_32BITS(currentNode->header.array,matchedPrefixLen),(uint64_t)currentNode,(uint64_t)&newNode);

            // modify currentNode 
            currentNode->header.depth -= matchedPrefixLen*SIZE_OF_CHAR/HT_NODE_LENGTH;
            currentNode->header.len -= matchedPrefixLen;
            for(int i=0;i<HT_NODE_PREFIX_MAX_BYTES-matchedPrefixLen;i++){
                currentNode->header.array[i] = currentNode->header.array[i+matchedPrefixLen];
            }
            clflush((char*)&(currentNode->header),8);
            clflush((char *) newNode, sizeof(VarLengthHashTreeNode));

            // modify the successor 
            *(Length64HashTreeNode**)beforeAddress = newNode;
            return;
        }
    }
}

uint64_t Length64HashTree::get(uint64_t key){
    auto currentNode = root;
    if(currentNode==NULL){
        return 0;
    }
    int pos = 0;
    while(pos<HT_KEY_LENGTH/SIZE_OF_CHAR){
        if(currentNode->header.len) {
            if (HT_KEY_LENGTH / SIZE_OF_CHAR - pos <= currentNode->header.len) {
                if (currentNode->treeNodeValues[currentNode->header.len - HT_KEY_LENGTH + pos].key == key) {
                    return (uint64_t) currentNode->treeNodeValues[currentNode->header.len - HT_KEY_LENGTH + pos].value;
                } else {
                    return 0;
                }
            }
            if (!isSame((unsigned char *) currentNode->header.array, key, pos * SIZE_OF_CHAR,
                        currentNode->header.len * SIZE_OF_CHAR)) {
                return 0;
            }
            pos += currentNode->header.len;
        }
        // uint64_t subkey = GET_16BITS(key,pos);
        uint64_t subkey = GET_SUBKEY(key,pos*SIZE_OF_CHAR,HT_NODE_LENGTH);
        bool keyValueFlag = false;
        auto next = currentNode->get(subkey, keyValueFlag);
        // pos+=_16_BITS_OF_BYTES;
        pos+=_32_BITS_OF_BYTES;
        if(next==0){
            return 0;
        }
        if(keyValueFlag){
            // is value
            if (pos == 8) {
                return next;
            } else {
                if (key == ((Length64HashTreeKeyValue *) next)->key) {
                    return ((Length64HashTreeKeyValue *) next)->value;
                } else {
                    return 0;
                }
            }
        }else{
            currentNode = (Length64HashTreeNode*)next;
        }
    }
    return 0;
}

void Length64HashTree::scan(uint64_t left, uint64_t right){
    vector<Length64HashTreeKeyValue> res;
    node_scan(root,left,right,res,0);
//    cout << res.size() << endl;
}



void Length64HashTree::node_scan(Length64HashTreeNode *tmp, uint64_t left, uint64_t right, vector<Length64HashTreeKeyValue> &res, int pos, uint64_t prefix){
    if(unlikely(tmp == NULL)){
        tmp = root;
    }
    uint64_t leftPos = UINT64_MAX, rightPos = UINT64_MAX;

    for(int i=0;i<tmp->header.len;i++){
        uint64_t subkey = GET_SUBKEY(left,pos+i,8);
        if(subkey==(uint64_t)tmp->header.array[i]){
            continue;
        }else{
            if(subkey>(uint64_t)tmp->header.array[i]){
                return;
            }else{
                leftPos = 0;
                break;
            }
        }
    }

    for(int i=0;i<tmp->header.len;i++){
        uint64_t subkey = GET_SUBKEY(right,pos+i,8);
        if(subkey==(uint64_t)tmp->header.array[i]){
            continue;
        }else{
            if(subkey>(uint64_t)tmp->header.array[i]){
                rightPos = tmp->dir_size-1;
                break;
            }else{
                return;
            }
        }
    }

    if(tmp->header.len > 0) {
        prefix = (prefix << tmp->header.len * SIZE_OF_CHAR);
        for (int i = 0; i < tmp->header.len; ++i) {
            prefix += tmp->header.array[i] << (tmp->header.len-i);
        }
        pos += tmp->header.len * SIZE_OF_CHAR;
    }
    uint64_t leftSubkey = UINT64_MAX, rightSubkey = UINT64_MAX;
    if(leftPos == UINT64_MAX){
        leftSubkey = GET_SUBKEY(left,pos,HT_NODE_LENGTH);
        leftPos = GET_SEG_NUM(leftSubkey, HT_NODE_LENGTH, tmp->global_depth);
    }
    if(rightPos == UINT64_MAX){
        rightSubkey = GET_SUBKEY(right,pos,HT_NODE_LENGTH);
        rightPos = GET_SEG_NUM(rightSubkey, HT_NODE_LENGTH, tmp->global_depth);
    }
    prefix = (prefix << HT_NODE_LENGTH);
    pos += HT_NODE_LENGTH;
    Length64HashTreeSegment *last_seg = NULL;
    for(uint32_t i=leftPos;i<=rightPos;i++){
        Length64HashTreeSegment *tmp_seg = *(Length64HashTreeSegment **)GET_SEG_POS(tmp,i);
        if (tmp_seg == last_seg)
            continue;
        else
            last_seg = tmp_seg;
        //todo if leftsubkey == rightsubkey, there is no need to scan all the segment.
        for(auto j=0;j<HT_MAX_BUCKET_NUM;j++){
            for(auto k=0;k<HT_BUCKET_SIZE;k++){
                bool keyValueFlag = GET_NODE_FLAG(tmp_seg->bucket[j].counter[k].subkey);
                uint64_t curSubkey = REMOVE_NODE_FLAG(tmp_seg->bucket[j].counter[k].subkey);
                uint64_t value = tmp_seg->bucket[j].counter[k].value;
                if((value==0&&curSubkey==0) || (tmp_seg != *(Length64HashTreeSegment **)GET_SEG_POS(tmp, GET_SEG_NUM(curSubkey, HT_NODE_LENGTH, tmp_seg->depth)))){
                    continue;
                }
                if((leftSubkey==UINT64_MAX||curSubkey>leftSubkey)&&(rightSubkey==UINT64_MAX||curSubkey<rightSubkey)){
                    if (pos == 64){
                        Length64HashTreeKeyValue tmp;
                        tmp.key = curSubkey + prefix;
                        tmp.value = value;
                        res.push_back(tmp);
                    } else {
                        if (keyValueFlag) {
                            res.push_back(*(Length64HashTreeKeyValue *) value);
                        } else {
                            getAllNodes((Length64HashTreeNode *) value, res, prefix + curSubkey);
                        }
                    }
                } else if(curSubkey==leftSubkey||curSubkey==rightSubkey){
                    if (pos == 64){
                        Length64HashTreeKeyValue tmp;
                        tmp.key = curSubkey + prefix;
                        tmp.value = value;
                        res.push_back(tmp);
                    } else {
                        if (pos == HT_KEY_LENGTH || keyValueFlag) {
                            res.push_back(*(Length64HashTreeKeyValue *) value);
                        } else {
                            node_scan((Length64HashTreeNode *) value, left, right, res, pos, prefix + curSubkey);
                        }
                    }
                }
            }
        }
    }


}

void Length64HashTree::getAllNodes(Length64HashTreeNode *tmp, vector<Length64HashTreeKeyValue> &res, int pos, uint64_t prefix){
    if(tmp==NULL){
        return;
    }
    if(tmp->header.len > 0) {
        prefix = (prefix << tmp->header.len * SIZE_OF_CHAR);
        for (int i = 0; i < tmp->header.len; ++i) {
            prefix += tmp->header.array[i] << (tmp->header.len-i);
        }
        pos += tmp->header.len * SIZE_OF_CHAR;
    }
    prefix = (prefix << HT_NODE_LENGTH);
    pos += HT_NODE_LENGTH;
    Length64HashTreeSegment *last_seg = NULL;
    for(int i=0;i<tmp->dir_size;i++){
        Length64HashTreeSegment *tmp_seg = *(Length64HashTreeSegment **)GET_SEG_POS(tmp,i);
        if (tmp_seg == last_seg)
            continue;
        else
            last_seg = tmp_seg;
        for(auto j=0;j<HT_MAX_BUCKET_NUM;j++){
            for(auto k=0;k<HT_BUCKET_SIZE;k++){
                bool keyValueFlag = GET_NODE_FLAG(tmp_seg->bucket[j].counter[k].subkey);
                uint64_t curSubkey = REMOVE_NODE_FLAG(tmp_seg->bucket[j].counter[k].subkey);
                uint64_t value = tmp_seg->bucket[j].counter[k].value;
                if(pos == 64){
                    Length64HashTreeKeyValue tmp;
                    tmp.key = curSubkey + prefix;
                    tmp.value = value;
                    res.push_back(tmp);
                } else {
                    if(curSubkey==0 || (tmp_seg != *(Length64HashTreeSegment **)GET_SEG_POS(tmp, GET_SEG_NUM(curSubkey, HT_NODE_LENGTH, tmp_seg->depth)))){
                        continue;
                    }
                    if(keyValueFlag){
                        res.push_back(*(Length64HashTreeKeyValue*)value);
                    }else{
                        getAllNodes((Length64HashTreeNode*)value, res, pos, prefix + curSubkey);
                    }
                }
            }
        }
    }
}

Length64HashTree *new_length64HashTree(){
    Length64HashTree *_new_hash_tree = static_cast<Length64HashTree *>(concurrency_fast_alloc(sizeof(Length64HashTree)));
    _new_hash_tree->init();
    return _new_hash_tree;
}


Length64HashTree::Length64HashTree() {
    root = new_length64hashtree_node(HT_NODE_LENGTH);
}

Length64HashTree::Length64HashTree(int _span, int _init_depth) {
    init_depth = _init_depth;
    root = new_length64hashtree_node(HT_NODE_LENGTH);
}

Length64HashTree::~Length64HashTree() {
    delete root;
}

void Length64HashTree::init() {
    root = new_length64hashtree_node(HT_NODE_LENGTH);
}
