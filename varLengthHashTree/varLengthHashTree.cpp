//
// Created by 杨冠群 on 2021-07-08.
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "varLengthHashTree.h"

//#ifdef VLHT_PROFILE_TIME
//timeval start_time, end_time;
//uint64_t t1, t2, t3, t4;
//// t1: insertion
//// t2: segment split
//// t3: directory double
//// t4: decompression
//#endif

/*
 *         begin  len
 * key [______|___________|____________]
 */
#define GET_SUB_KEY(key, begin, len)  (((key)>>(64-(begin)-(len)))&(((uint64_t)1<<(len))-1))
#define GET_SEG_NUM(key, key_len, depth)  ((key>>(key_len-depth))&(((uint64_t)1<<depth)-1))
#define GET_BUCKET_NUM(key, bucket_mask_len) ((key)&(((uint64_t)1<<bucket_mask_len)-1))

#define GET_SEG_POS(currentNode, dir_index) (((uint64_t)(currentNode) + sizeof(VarLengthHashTreeNode) + dir_index*sizeof(HashTreeSegment*)))

#define GET_32BITS(pointer, pos) (*((uint32_t *)(pointer+pos)))
// #define GET_32BITS(pointer,pos) ((*(pointer+pos))<<24 | (*(pointer+pos+1))<< 16 | (*(pointer+pos+2))<<8 | *(pointer+pos+3))
#define GET_16BITS(pointer, pos) ((*(pointer+pos))<<8 | (*(pointer+pos+1)))

#define _16_BITS_OF_BYTES 2
#define _32_BITS_OF_BYTES 4

#ifdef VLHT_PROFILE
uint64_t vlht_visited_node;
#endif


bool keyIsSame(unsigned char *key1, unsigned int length1, unsigned char *key2, unsigned int length2) {
    if (length1 != length2) {
        return false;
    }
    return strncmp((char *) key1, (char *) key2, length1) == 0;
}

bool keyIsSame(unsigned char *key1, unsigned char *key2, unsigned int length) {
    return strncmp((char *) key1, (char *) key2, length) == 0;
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

bool isSame(unsigned char *key1, uint64_t key2, int pos, int length) {
    uint64_t subkey = GET_SUBKEY(key2, pos, length);
    subkey <<= (64 - length);
    for (int i = 0; i < length / SIZE_OF_CHAR; i++) {
        if ((subkey >> 56) != (uint64_t) key1[i]) {
            return false;
        }
        subkey <<= SIZE_OF_CHAR;
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

void VarLengthHashTree::init() {
    root = new_varlengthhashtree_node(HT_NODE_LENGTH);
#ifdef VLHT_PROFILE
    vlht_visited_node = 0;
#endif
}

VarLengthHashTree *new_varLengthHashtree() {
    VarLengthHashTree *_new_hash_tree = static_cast<VarLengthHashTree *>(fast_alloc(sizeof(VarLengthHashTree)));
    _new_hash_tree->init();
    return _new_hash_tree;
}


void
VarLengthHashTree::crash_consistent_put(VarLengthHashTreeNode *_node, int length, unsigned char *key, uint64_t value,
                                        int pos) {
    crash_consistent_put(_node, length, key, value, (uint64_t) &root, pos);
}


void
VarLengthHashTree::crash_consistent_put(VarLengthHashTreeNode *_node, int length, unsigned char *key, uint64_t value,
                                        uint64_t beforeAddress, int pos) {
    VarLengthHashTreeNode *currentNode = _node;
    if (_node == NULL) {
        currentNode = root;
    }
    unsigned char headerDepth = currentNode->header.depth;
    while (length > pos) {
        int matchedPrefixLen;

//#ifdef VLHT_PROFILE
//        vlht_visited_node++;
//#endif

        // init a number bigger than HT_NODE_PREFIX_MAX_LEN to represent there is no value
        if (currentNode->header.len > HT_NODE_PREFIX_MAX_BYTES) {
            currentNode->header.len =
                    (length - (pos + 1) * SIZE_OF_CHAR) > HT_NODE_PREFIX_MAX_BITS ? HT_NODE_PREFIX_MAX_BITS : (length -
                                                                                                               (pos +
                                                                                                                1) *
                                                                                                               SIZE_OF_CHAR);
            currentNode->header.assign(key + pos);
            pos += HT_NODE_PREFIX_MAX_BYTES;
            continue;
        } else {
            // compute this prefix
            matchedPrefixLen = currentNode->header.computePrefix(key, length, pos);
        }
        if (pos + matchedPrefixLen == length) {
#ifdef VLHT_PROFILE_TIME
            gettimeofday(&start_time, NULL);
#endif
            HashTreeKeyValue *kv = new_vlht_key_value(key, length, value);
            clflush((char *) kv, sizeof(HashTreeKeyValue));
            currentNode->node_put(matchedPrefixLen, kv);
#ifdef VLHT_PROFILE_TIME
            gettimeofday(&end_time, NULL);
            t1 += (end_time.tv_sec - start_time.tv_sec) * 1000000 + end_time.tv_usec - start_time.tv_usec;
#endif
            return;
        }
        if (matchedPrefixLen == currentNode->header.len) {
            // if prefix is match 
            // move the pos
            pos += currentNode->header.len;

            // compute subkey (16bits as default)
            // uint64_t subkey = GET_16BITS(key,pos);
            uint64_t subkey = GET_32BITS(key, pos);

            // use the subkey to search in a cceh node
            uint64_t next = 0;

            uint64_t dir_index = GET_SEG_NUM(subkey, HT_NODE_LENGTH, currentNode->global_depth);
            HashTreeSegment *tmp_seg = *(HashTreeSegment **) GET_SEG_POS(currentNode, dir_index);
            uint64_t seg_index = GET_BUCKET_NUM(subkey, HT_BUCKET_MASK_LEN);
            HashTreeBucket *tmp_bucket = &(tmp_seg->bucket[seg_index]);
            int i;
            uint64_t beforeA;
            for (i = 0; i < HT_BUCKET_SIZE; ++i) {
                if (subkey == tmp_bucket->counter[i].subkey) {
                    next = tmp_bucket->counter[i].value;
                    beforeA = (uint64_t) &tmp_bucket->counter[i].value;
                    break;
                }
            }

            pos += HT_NODE_LENGTH / SIZE_OF_CHAR;
            if (next == 0) {
                //not exists
#ifdef VLHT_PROFILE_TIME
                gettimeofday(&start_time, NULL);
#endif
                HashTreeKeyValue *kv = new_vlht_key_value(key, length, value);
                clflush((char *) kv, sizeof(HashTreeKeyValue));
                currentNode->put(subkey, (uint64_t) kv, tmp_seg, tmp_bucket, dir_index, seg_index, beforeAddress);
#ifdef VLHT_PROFILE_TIME
                gettimeofday(&end_time, NULL);
                t1 += (end_time.tv_sec - start_time.tv_sec) * 1000000 + end_time.tv_usec - start_time.tv_usec;
#endif
                return;
            } else {
                if (((bool *) next)[0]) {
                    // next is key value pair, which means collides
#ifdef VLHT_PROFILE_TIME
                    gettimeofday(&start_time, NULL);
#endif
                    unsigned char *preKey = ((HashTreeKeyValue *) next)->key;
                    unsigned int preLength = ((HashTreeKeyValue *) next)->len;
                    uint64_t preValue = ((HashTreeKeyValue *) next)->value;
                    if (unlikely(keyIsSame(key, length, preKey, preLength))) {
                        //same key, update the value
                        ((HashTreeKeyValue *) next)->value = value;
                        clflush((char *) &(((HashTreeKeyValue *) next)->value), 8);
#ifdef VLHT_PROFILE_TIME
                        gettimeofday(&end_time, NULL);
                        t1 += (end_time.tv_sec - start_time.tv_sec) * 1000000 + end_time.tv_usec - start_time.tv_usec;
#endif
                        return;
                    } else {
                        //not same key: needs to create a new node
                        VarLengthHashTreeNode *newNode = new_varlengthhashtree_node(HT_NODE_LENGTH, headerDepth + 1);

                        // put pre kv
                        // uint64_t preSubkey = GET_16BITS(preKey,pos);
                        crash_consistent_put(newNode, preLength, preKey, preValue,
                                             (uint64_t) &tmp_bucket->counter[i].value, pos);

                        // put new kv
                        crash_consistent_put(newNode, length, key, value, (uint64_t) &tmp_bucket->counter[i].value,
                                             pos);

                        clflush((char *) newNode, sizeof(VarLengthHashTreeNode));

                        tmp_bucket->counter[i].value = (uint64_t) newNode;
                        clflush((char *) &tmp_bucket->counter[i].value, 8);
#ifdef VLHT_PROFILE_TIME
                        gettimeofday(&end_time, NULL);
                        t1 += (end_time.tv_sec - start_time.tv_sec) * 1000000 + end_time.tv_usec - start_time.tv_usec;
#endif
                        return;
                    }
                } else {
                    // next is next extendible hash
                    currentNode = (VarLengthHashTreeNode *) next;
                    beforeAddress = beforeA;
                    headerDepth = currentNode->header.depth;
                }
            }
        } else {
            // if prefix is not match (shorter)
            // split a new tree node and insert

//#ifdef VLHT_PROFILE
//            vlht_visited_node++;
//#endif
#ifdef VLHT_PROFILE_TIME
            gettimeofday(&start_time, NULL);
#endif
            // build new tree node
            VarLengthHashTreeNode *newNode = new_varlengthhashtree_node(HT_NODE_LENGTH, headerDepth);
            newNode->header.init(&currentNode->header, matchedPrefixLen, currentNode->header.depth);
            for (int j = 0; j < matchedPrefixLen; j++) {
                newNode->node_put(j, &currentNode->treeNodeValues[currentNode->header.len - j]);
            }

            // uint64_t subkey = GET_16BITS(key,matchedPrefixLen);
            uint64_t subkey = GET_32BITS(key, matchedPrefixLen);
            HashTreeKeyValue *kv = new_vlht_key_value(key, length, value);
            clflush((char *) kv, sizeof(HashTreeKeyValue));
            newNode->put(subkey, (uint64_t) kv, (uint64_t) &newNode);
            // newNode->put(GET_16BITS(currentNode->header.array,matchedPrefixLen),(uint64_t)currentNode);
            newNode->put(GET_32BITS(currentNode->header.array, matchedPrefixLen), (uint64_t) currentNode,
                         (uint64_t) &newNode);

            // modify currentNode 
            currentNode->header.depth -= matchedPrefixLen * SIZE_OF_CHAR / HT_NODE_LENGTH;
            currentNode->header.len -= matchedPrefixLen;
            for (int i = 0; i < HT_NODE_PREFIX_MAX_BYTES - matchedPrefixLen; i++) {
                currentNode->header.array[i] = currentNode->header.array[i + matchedPrefixLen];
            }
            clflush((char *) &(currentNode->header), 8);
            clflush((char *) newNode, sizeof(VarLengthHashTreeNode));

            // modify the successor 
            *(VarLengthHashTreeNode **) beforeAddress = newNode;
#ifdef VLHT_PROFILE_TIME
            gettimeofday(&end_time, NULL);
            t4 += (end_time.tv_sec - start_time.tv_sec) * 1000000 + end_time.tv_usec - start_time.tv_usec;
#endif
            return;
        }
    }
}

uint64_t VarLengthHashTree::get(int length, unsigned char *key) {
    auto currentNode = root;
    if (currentNode == NULL) {
        return 0;
    }
    int pos = 0;
    while (pos != length) {
#ifdef VLHT_PROFILE
        vlht_visited_node++;
#endif
        if (length - pos <= currentNode->header.len) {
            if (!strncmp((char *) key + pos,
                         (char *) currentNode->treeNodeValues[currentNode->header.len - length + pos].key + pos,
                         length - pos)) {
                return (uint64_t) currentNode->treeNodeValues[currentNode->header.len - length + pos].value;
            } else {
                return 0;
            }
        }
        if (strncmp((char *) key + pos, (char *) currentNode->header.array, currentNode->header.len)) {
            return 0;
        }
        pos += currentNode->header.len;
        // uint64_t subkey = GET_16BITS(key,pos);
        uint64_t subkey = GET_32BITS(key, pos);

        auto next = currentNode->get(subkey);
        // pos+=_16_BITS_OF_BYTES;
        pos += _32_BITS_OF_BYTES;
        if (next == 0) {
            return 0;
        }
        if ((((bool *) next)[0])) {
            // is value
            if (pos == length) {
                return ((HashTreeKeyValue *) next)->value;
            } else {
                return 0;
            }
        } else {
            currentNode = (VarLengthHashTreeNode *) next;
        }
    }
    return 0;
}

double VarLengthHashTree::profile() {
#ifdef VLHT_PROFILE_TIME
    cout << "ET, " << t1 << ", " << t2 << ", " << t3 << ", " << t4 << endl;
#endif
#ifdef VLHT_PROFILE
    return vlht_visited_node;
#else
    return 0;
#endif
}

void Length64HashTree::crash_consistent_put(Length64HashTreeNode *_node, uint64_t key, uint64_t value, int len) {
    // 0-index of current bytes

    if (key == 6552766278671465630) { ;
    }
    Length64HashTreeNode *currentNode = _node;
    if (_node == NULL) {
        currentNode = root;
    }
    unsigned char headerDepth = currentNode->header.depth;
    uint64_t beforeAddress = (uint64_t) &root;

    while (len < HT_KEY_LENGTH / SIZE_OF_CHAR) {
        int matchedPrefixLen;

#ifdef VLHT_PROFILE
        vlht_visited_node++;
#endif

        // init a number larger than HT_NODE_PREFIX_MAX_LEN to represent there is no value
        if (currentNode->header.len > HT_NODE_PREFIX_MAX_BYTES) {
            int size = HT_NODE_PREFIX_MAX_BITS / HT_NODE_LENGTH * HT_NODE_LENGTH;

            currentNode->header.len =
                    (HT_KEY_LENGTH - len) <= size ? (HT_KEY_LENGTH - len) / SIZE_OF_CHAR : size / SIZE_OF_CHAR;
            currentNode->header.assign(key, len);
            len += size / SIZE_OF_CHAR;
            continue;
        } else {
            // compute this prefix
            matchedPrefixLen = currentNode->header.computePrefix(key, len * SIZE_OF_CHAR);
        }
        if (len + matchedPrefixLen == (HT_KEY_LENGTH / SIZE_OF_CHAR)) {
            Length64HashTreeKeyValue *kv = new_l64ht_key_value(key, value);
            clflush((char *) kv, sizeof(Length64HashTreeKeyValue));
            currentNode->node_put(matchedPrefixLen, kv);
            return;
        }
        if (matchedPrefixLen == currentNode->header.len) {
            // if prefix is match 
            // move the pos
            len += currentNode->header.len;

            // compute subkey (16bits as default)
            // uint64_t subkey = GET_16BITS(key,pos);
            uint64_t subkey = GET_SUBKEY(key, len * SIZE_OF_CHAR, HT_NODE_LENGTH);

            // use the subkey to search in a cceh node
            uint64_t next = 0;

            uint64_t dir_index = GET_SEG_NUM(subkey, HT_NODE_LENGTH, currentNode->global_depth);
            Length64HashTreeSegment *tmp_seg = *(Length64HashTreeSegment **) GET_SEG_POS(currentNode, dir_index);
            uint64_t seg_index = GET_BUCKET_NUM(subkey, HT_BUCKET_MASK_LEN);
            Length64HashTreeBucket *tmp_bucket = &(tmp_seg->bucket[seg_index]);
            int i;
            uint64_t beforeA;
            for (i = 0; i < HT_BUCKET_SIZE; ++i) {
                if (subkey == tmp_bucket->counter[i].subkey) {
                    next = tmp_bucket->counter[i].value;
                    beforeA = (uint64_t) &tmp_bucket->counter[i].value;
                    break;
                }
            }
            len += HT_NODE_LENGTH / SIZE_OF_CHAR;
            if (next == 0) {
                //not exists
                Length64HashTreeKeyValue *kv = new_l64ht_key_value(key, value);
                clflush((char *) kv, sizeof(Length64HashTreeKeyValue));
                currentNode->put(subkey, (uint64_t) kv, tmp_seg, tmp_bucket, dir_index, seg_index, beforeAddress);
                return;
            } else {
                if (((bool *) next)[0]) {
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
        } else {
            // if prefix is not match (shorter)
            // split a new tree node and insert 

            // build new tree node
            Length64HashTreeNode *newNode = new_length64hashtree_node(HT_NODE_LENGTH, headerDepth);
            newNode->header.init(&currentNode->header, matchedPrefixLen, currentNode->header.depth);
            for (int j = 0; j < matchedPrefixLen; j++) {
                newNode->node_put(j, &currentNode->treeNodeValues[currentNode->header.len - j]);
            }

            uint64_t subkey = GET_SUBKEY(key, (len + matchedPrefixLen) * SIZE_OF_CHAR, HT_NODE_LENGTH);

            Length64HashTreeKeyValue *kv = new_l64ht_key_value(key, value);
            clflush((char *) kv, sizeof(Length64HashTreeKeyValue));

            newNode->put(subkey, (uint64_t) kv, (uint64_t) &newNode);
            // newNode->put(GET_16BITS(currentNode->header.array,matchedPrefixLen),(uint64_t)currentNode);
            newNode->put(GET_32BITS(currentNode->header.array, matchedPrefixLen), (uint64_t) currentNode,
                         (uint64_t) &newNode);

            // modify currentNode 
            currentNode->header.depth -= matchedPrefixLen * SIZE_OF_CHAR / HT_NODE_LENGTH;
            currentNode->header.len -= matchedPrefixLen;
            for (int i = 0; i < HT_NODE_PREFIX_MAX_BYTES - matchedPrefixLen; i++) {
                currentNode->header.array[i] = currentNode->header.array[i + matchedPrefixLen];
            }
            clflush((char *) &(currentNode->header), 8);
            clflush((char *) newNode, sizeof(VarLengthHashTreeNode));

            // modify the successor 
            *(Length64HashTreeNode **) beforeAddress = newNode;
            return;
        }
    }
}

uint64_t Length64HashTree::get(uint64_t key) {
    auto currentNode = root;
    if (currentNode == NULL) {
        return 0;
    }
    int pos = 0;
    while (pos < HT_KEY_LENGTH / SIZE_OF_CHAR) {
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
        // uint64_t subkey = GET_16BITS(key,pos);
        uint64_t subkey = GET_SUBKEY(key, pos * SIZE_OF_CHAR, HT_NODE_LENGTH);
        auto next = currentNode->get(subkey);
        // pos+=_16_BITS_OF_BYTES;
        pos += _32_BITS_OF_BYTES;
        if (next == 0) {
            return 0;
        }
        if ((((bool *) next)[0])) {
            // is value
            if (key == ((Length64HashTreeKeyValue *) next)->key) {
                return ((Length64HashTreeKeyValue *) next)->value;
            } else {
                return 0;
            }
        } else {
            currentNode = (Length64HashTreeNode *) next;
        }
    }
    return 0;
}


Length64HashTree *new_length64HashTree() {
    Length64HashTree *_new_hash_tree = static_cast<Length64HashTree *>(fast_alloc(sizeof(Length64HashTree)));
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
