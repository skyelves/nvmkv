//
// Created by menghe on 1/9/22.
//
#include "varLengthLbtree.h"




inline static void mfence() {
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

static int last_slot_in_line[LEAF_KEY_NUM];


int greaterNotEqual(unsigned char* key1, uint64_t len1, unsigned char* key2, uint64_t len2){
    if(len1!=len2){
        return len1>len2?1:-1;
    }
    for(int i=0;i<len1;i++){
        if(key1[i]==key2[i]){
            continue;
        }else if(key1[i]>key2[i]){
            return 1;
        }else{
            return -1;
        }
    }
    return 0;
}

bool _greater(unsigned char* key1, uint64_t len1, unsigned char* key2, uint64_t len2){
    return greaterNotEqual(key1,len1,key2,len2)>=0;
}

void varLengthLbtree::qsortBleaf(varLengthbleaf *p, int start, int end, int pos[]) {
    if (start >= end) return;

    int pos_start = pos[start];
    _key_type key = p->k(pos_start);
    uint64_t keyLength = p->kLength(pos_start);// pivot
    int l, r;

    l = start;
    r = end;
    while (l < r) {
        while ((l < r) && (greaterNotEqual( p->k(pos[r]),p->kLength(pos[r]),key,keyLength)>0)) r--;
        if (l < r) {
            pos[l] = pos[r];
            l++;
        }
        while ((l < r) && (greaterNotEqual( p->k(pos[l]),p->kLength(pos[l]),key,keyLength)<=0)) l++;
        if (l < r) {
            pos[r] = pos[l];
            r--;
        }
    }
    pos[l] = pos_start;
    qsortBleaf(p, start, l - 1, pos);
    qsortBleaf(p, l + 1, end, pos);
}

void varLengthLbtree::insert(_key_type _key, uint64_t keyLength, void *_ptr){
    void *ptr = (void *)(fast_alloc(sizeof(uint64_t)));
    *(uint64_t *)ptr = *(uint64_t *)_ptr;
    _key_type key = ( _key_type)(fast_alloc(keyLength));
    memcpy(key, _key, keyLength);
    clflush((char *) key, keyLength);
    _Pointer8B parray[32] = {};  // 0 .. root_level will be used
    short ppos[32] = {};    // 1 .. root_level will be used
    bool isfull[32] = {};  // 0 .. root_level will be used

    unsigned char key_hash = hashcode1B(key);
    volatile long long sum;

    /* Part 1. get the positions to insert the key */
    {
        varLengthbnode *p;
        varLengthbleaf *lp;
        int i, t, m, b;

        Again2:

        // 2. search nonleaf nodes
        p = tree_meta->tree_root;

        for (i = tree_meta->root_level; i > 0; i--) {

            // prefetch the entire node
            NODE_PREF(p);

            parray[i] = p;
            isfull[i] = (p->num() == NON_LEAF_KEY_NUM);

            // binary search to narrow down to at most 8 entries
            b = 1;
            t = p->num();
            while (b + 7 <= t) {
                m = (b + t) >> 1;
                int comp = greaterNotEqual(key,keyLength,p->k(m),p->kLength(m));
                if (comp>0) b = m + 1;
                else if (comp<0) t = m - 1;
                else {
                    p = p->ch(m);
                    ppos[i] = m;
                    goto inner_done;
                }
            }

            // sequential search (which is slightly faster now)
            for (; b <= t; b++)
                if (!_greater(key,keyLength,p->k(b),p->kLength(b))) break;
            p = p->ch(b - 1);
            ppos[i] = b - 1;

            inner_done:;
        }

        // 3. search leaf node
        lp = (varLengthbleaf *) p;

        // prefetch the entire node
        LEAF_PREF(lp);

        // if the lock bit is set, abort

        parray[0] = lp;

        // SIMD comparison
        // a. set every byte to key_hash in a 16B register
        __m128i key_16B = _mm_set1_epi8((char) key_hash);

        // b. load meta into another 16B register
        __m128i fgpt_16B = _mm_load_si128((const __m128i *) lp);

        // c. compare them
        __m128i cmp_res = _mm_cmpeq_epi8(key_16B, fgpt_16B);

        // d. generate a mask
        unsigned int mask = (unsigned int)
                _mm_movemask_epi8(cmp_res);  // 1: same; 0: diff

        // remove the lower 2 bits then AND bitmap
        mask = (mask >> 2) & ((unsigned int) (lp->bitmap));

        // search every matching candidate
        while (mask) {
            int jj = bitScan(mask) - 1;  // next candidate

            if (greaterNotEqual( lp->k(jj),lp->kLength(jj),key,keyLength)==0) { // found: do nothing, return
                return;
            }

            mask &= ~(0x1 << jj);  // remove this bit
        } // end while



        isfull[0] = lp->isFull();
        if (isfull[0]) {
            for (i = 1; i <= tree_meta->root_level; i++) {
                p = parray[i];
                p->lock() = 1;
                if (!isfull[i]) break;
            }
        }


    } // end of Part 1

    /* Part 2. leaf node */
    {
        varLengthbleaf *lp = parray[0];
        varLengthbleafMeta meta = *((varLengthbleafMeta *) lp);

        /* 1. leaf is not full */
        if (!isfull[0]) {

            meta.v.lock = 0;  // clear lock in temp meta

            // 1.1 get first empty slot
            uint16_t bitmap = meta.v.bitmap;
            int slot = bitScan(~bitmap) - 1;

            // 1.2 set leaf.entry[slot]= (k, v);
            // set fgpt, bitmap in meta
            lp->k(slot) = key;
            lp->ch(slot) = ptr;
            lp->kLength(slot) = keyLength;
            meta.v.fgpt[slot] = key_hash;
            bitmap |= (1 << slot);

            // 1.3 line 0: 0-2; line 1: 3-6; line 2: 7-10; line 3: 11-13
            // in line 0?
            if (slot < 3) {
                // 1.3.1 write word 0
                meta.v.bitmap = bitmap;
                lp->setWord0(&meta);

                // 1.3.2 flush
                clflush((char *) lp, 8);
                return;
            }

                // 1.4 line 1--3
            else {
                int last_slot = last_slot_in_line[slot];
                int from = 0;
                for (int to = slot + 1; to <= last_slot; to++) {
                    if ((bitmap & (1 << to)) == 0) {
                        // 1.4.1 for each empty slot in the line
                        // copy an entry from line 0
                        lp->ent[to] = lp->ent[from];
                        meta.v.fgpt[to] = meta.v.fgpt[from];
                        bitmap |= (1 << to);
                        bitmap &= ~(1 << from);
                        from++;
                    }
                }

                // 1.4.2 flush the line containing slot
                clflush((char *) &(lp->k(slot)), 16);

                // 1.4.3 change meta and flush line 0
                meta.v.bitmap = bitmap;
                lp->setBothWords(&meta);
                clflush((char *) lp, 8);
                return;
            }
        } // end of not full
        // 2.1 get sorted positions
        int sorted_pos[LEAF_KEY_NUM];
        for (int i = 0; i < LEAF_KEY_NUM; i++) sorted_pos[i] = i;
        qsortBleaf(lp, 0, LEAF_KEY_NUM - 1, sorted_pos);

        // 2.2 split point is the middle point
        int split = (LEAF_KEY_NUM / 2);  // [0,..split-1] [split,LEAF_KEY_NUM-1]
        _key_type split_key = lp->k(sorted_pos[split]);
        uint64_t split_length = lp->kLength(sorted_pos[split]);

        // 2.3 create new node
        varLengthbleaf *newp = (varLengthbleaf *) fast_alloc(sizeof(varLengthbleaf));

        // 2.4 move entries sorted_pos[split .. LEAF_KEY_NUM-1]
        uint16_t freed_slots = 0;
        for (int i = split; i < LEAF_KEY_NUM; i++) {
            newp->ent[i] = lp->ent[sorted_pos[i]];
            newp->fgpt[i] = lp->fgpt[sorted_pos[i]];

            // add to freed slots bitmap
            freed_slots |= (1 << sorted_pos[i]);
        }
        newp->bitmap = (((1 << (LEAF_KEY_NUM - split)) - 1) << split);
        newp->lock = 0;
        newp->alt = 0;

        // remove freed slots from temp bitmap
        meta.v.bitmap &= ~freed_slots;

        newp->next[0] = lp->next[lp->alt];
        lp->next[1 - lp->alt] = newp;

        // set alt in temp bitmap
        meta.v.alt = 1 - lp->alt;

        // 2.5 key > split_key: insert key into new node
        if (greaterNotEqual(key,keyLength,split_key,split_length)>0) {
            newp->k(split - 1) = key;
            newp->ch(split - 1) = ptr;
            newp->kLength(split - 1) = keyLength;
            newp->fgpt[split - 1] = key_hash;
            newp->bitmap |= 1 << (split - 1);

            if (tree_meta->root_level > 0) meta.v.lock = 0;  // do not clear lock of root
        }

        // 2.6 clwb newp, clwb lp line[3] and sfence
        clflush((char *) newp, sizeof(varLengthbleaf));
        clflush((char *) &(lp->next[0]), 8);

        // 2.7 clwb lp and flush: NVM atomic write to switch alt and set bitmap
        lp->setBothWords(&meta);
        clflush((char *) lp, 8);

        // 2.8 key < split_key: insert key into old node
        if (greaterNotEqual(key,keyLength,split_key,split_length)<=0) {

            // note: lock bit is still set
            if (tree_meta->root_level > 0) meta.v.lock = 0;  // do not clear lock of root

            // get first empty slot
            uint16_t bitmap = meta.v.bitmap;
            int slot = bitScan(~bitmap) - 1;

            // set leaf.entry[slot]= (k, v);
            // set fgpt, bitmap in meta
            lp->k(slot) = key;
            lp->kLength(slot) = keyLength;
            lp->ch(slot) = ptr;
            meta.v.fgpt[slot] = key_hash;
            bitmap |= (1 << slot);

            // line 0: 0-2; line 1: 3-6; line 2: 7-10; line 3: 11-13
            // in line 0?
            if (slot < 3) {
                // write word 0
                meta.v.bitmap = bitmap;
                lp->setWord0(&meta);
                // flush
                clflush((char *) lp, 8);
            }
                // line 1--3
            else {
                int last_slot = last_slot_in_line[slot];
                int from = 0;
                for (int to = slot + 1; to <= last_slot; to++) {
                    if ((bitmap & (1 << to)) == 0) {
                        // for each empty slot in the line
                        // copy an entry from line 0
                        lp->ent[to] = lp->ent[from];
                        meta.v.fgpt[to] = meta.v.fgpt[from];
                        bitmap |= (1 << to);
                        bitmap &= ~(1 << from);
                        from++;
                    }
                }

                // flush the line containing slot
                clflush((char *) &(lp->k(slot)), 8);

                // change meta and flush line 0
                meta.v.bitmap = bitmap;
                lp->setBothWords(&meta);
                clflush((char *) lp, 8);
            }
        }

        key = split_key;
        keyLength = split_length;
        ptr = newp;
        /* (key, ptr) to be inserted in the parent non-leaf */

    } // end of Part 2

    /* Part 3. nonleaf node */
    {
        varLengthbnode *p, *newp;
        int n, i, pos, r, lev, total_level;

#define   LEFT_KEY_NUM        ((NON_LEAF_KEY_NUM)/2)
#define   RIGHT_KEY_NUM        ((NON_LEAF_KEY_NUM) - LEFT_KEY_NUM)

        total_level = tree_meta->root_level;
        lev = 1;

        while (lev <= total_level) {

            p = parray[lev];
            n = p->num();
            pos = ppos[lev] + 1;  // the new child is ppos[lev]+1 >= 1

            /* if the non-leaf is not full, simply insert key ptr */

            if (n < NON_LEAF_KEY_NUM) {
                for (i = n; i >= pos; i--) p->ent[i + 1] = p->ent[i];

                p->k(pos) = key;
                p->ch(pos) = ptr;
                p->kLength(pos) = keyLength;
                p->num() = n + 1;
                mfence();

                // unlock after all changes are globally visible
                return;
            }

            /* otherwise allocate a new non-leaf and redistribute the keys */
            newp = (varLengthbnode *) fast_alloc(sizeof(varLengthbnode));

            /* if key should be in the left node */
            if (pos <= LEFT_KEY_NUM) {
                for (r = RIGHT_KEY_NUM, i = NON_LEAF_KEY_NUM; r >= 0; r--, i--) {
                    newp->ent[r] = p->ent[i];
                }
                /* newp->key[0] actually is the key to be pushed up !!! */
                for (i = LEFT_KEY_NUM - 1; i >= pos; i--) p->ent[i + 1] = p->ent[i];

                p->k(pos) = key;
                p->kLength(pos) = keyLength;
                p->ch(pos) = ptr;
            }
                /* if key should be in the right node */
            else {
                for (r = RIGHT_KEY_NUM, i = NON_LEAF_KEY_NUM; i >= pos; i--, r--) {
                    newp->ent[r] = p->ent[i];
                }
                newp->k(r) = key;
                newp->kLength(r) = keyLength;
                newp->ch(r) = ptr;
                r--;
                for (; r >= 0; r--, i--) {
                    newp->ent[r] = p->ent[i];
                }
            } /* end of else */

            key = newp->k(0);
            keyLength = newp->kLength(0);
            ptr = newp;

            p->num() = LEFT_KEY_NUM;
            if (lev < total_level) p->lock() = 0; // do not clear lock bit of root
            newp->num() = RIGHT_KEY_NUM;
            newp->lock() = 0;

            lev++;
        } /* end of while loop */

        /* root was splitted !! add another level */
        newp = (varLengthbnode *) fast_alloc(sizeof (varLengthbnode));

        newp->num() = 1;
        newp->lock() = 1;
        newp->ch(0) = tree_meta->tree_root;
        newp->ch(1) = ptr;
        newp->kLength(1) = keyLength;
        newp->k(1) = key;
        mfence();  // ensure new node is consistent

        void *old_root = tree_meta->tree_root;
        tree_meta->root_level = lev;
        tree_meta->tree_root = newp;
        mfence();   // tree root change is globablly visible
        // old root and new root are both locked

        // unlock old root
        if (total_level > 0) { // previous root is a nonleaf
            ((varLengthbnode *) old_root)->lock() = 0;
        } else { // previous root is a leaf
            ((varLengthbleaf *) old_root)->lock = 0;
        }

        // unlock new root
        newp->lock() = 0;
        return;

#undef RIGHT_KEY_NUM
#undef LEFT_KEY_NUM
    }
}


void *varLengthLbtree::lookup(_key_type key, uint64_t keyLength, int *pos){
    varLengthbnode *p;
    varLengthbleaf *lp;
    int i, t, m, b;

    unsigned char key_hash = hashcode1B(key);
    int ret_pos;

    Again1:
    // 1. RTM begin
//    if (_xbegin() != _XBEGIN_STARTED) goto Again1;

    // 2. search nonleaf nodes
    p = tree_meta->tree_root;

    for (i = tree_meta->root_level; i > 0; i--) {

        // prefetch the entire node
        NODE_PREF(p);


        // binary search to narrow down to at most 8 entries
        b = 1;
        t = p->num();
        while (b + 7 <= t) {
            m = (b + t) >> 1;
            int comp = greaterNotEqual(key,keyLength,p->k(m),p->kLength(m));
            if (comp>0) b = m + 1;
            else if (comp<0) t = m - 1;
            else {
                p = p->ch(m);
                goto inner_done;
            }
        }

        // sequential search (which is slightly faster now)
        for (; b <= t; b++)
            if (!_greater(key,keyLength,p->k(b),p->kLength(b))) break;
        p = p->ch(b - 1);

        inner_done:;
    }

    // 3. search leaf node
    lp = (varLengthbleaf *) p;

    // prefetch the entire node
    LEAF_PREF(lp);


    // SIMD comparison
    // a. set every byte to key_hash in a 16B register
    __m128i key_16B = _mm_set1_epi8((char) key_hash);

    // b. load meta into another 16B register
    __m128i fgpt_16B = _mm_load_si128((const __m128i *) lp);

    // c. compare them
    __m128i cmp_res = _mm_cmpeq_epi8(key_16B, fgpt_16B);

    // d. generate a mask
    unsigned int mask = (unsigned int)
            _mm_movemask_epi8(cmp_res);  // 1: same; 0: diff

    // remove the lower 2 bits then AND bitmap
    mask = (mask >> 2) & ((unsigned int) (lp->bitmap));

    // search every matching candidate
    ret_pos = -1;
    while (mask) {
        int jj = bitScan(mask) - 1;  // next candidate

        if (greaterNotEqual( lp->k(jj),lp->kLength(jj),key,keyLength)==0) { // found: do nothing, return
            ret_pos = jj;
            break;
        }

        mask &= ~(0x1 << jj);  // remove this bit
    } // end while

    if(pos!=0)
        *pos = ret_pos;
    return ret_pos==-1?nullptr:(void *) lp->ch(ret_pos);
}


typedef struct BldThArgs {

    _key_type start_key; // input
    _key_type num_key;   // input

    int top_level;  // output
    int n_nodes[32];  // output
    _Pointer8B pfirst[32];  // output

} BldThArgs;


void varLengthtreeMeta::setFirstLeaf(varLengthbleaf *leaf) {
    *first_leaf = leaf;
    clflush((char *) first_leaf, 8);
}

void varLengthLbtree::init() {
    this->tree_meta = static_cast<varLengthtreeMeta *>(fast_alloc(sizeof(varLengthtreeMeta)));
    auto nvm_address = fast_alloc(4 * KB);
    tree_meta->init(nvm_address);
}


int varLengthLbtree::bulkloadSubtree(_key_type input, int start_key, int num_key, float bfill, int target_level, _Pointer8B pfirst[], int n_nodes[]) {
    int ncur[32];
    int top_level;

    // 1. compute leaf and nonleaf number of keys
    int leaf_fill_num = (int) ((float) LEAF_KEY_NUM * bfill);
    leaf_fill_num = max(leaf_fill_num, 1);

    int nonleaf_fill_num = (int) ((float) NON_LEAF_KEY_NUM * bfill);
    nonleaf_fill_num = max(nonleaf_fill_num, 1);


    // 2. compute number of nodes
    n_nodes[0] = ceiling(num_key, leaf_fill_num);
    top_level = 0;

    for (int i = 1; n_nodes[i - 1] > 1 && i <= target_level; i++) {
        n_nodes[i] = ceiling(n_nodes[i - 1], nonleaf_fill_num + 1);
        top_level = i;
    } // end of for each nonleaf level


    // 3. allocate nodes
    pfirst[0] = fast_alloc(sizeof(varLengthbleaf) * n_nodes[0]);
    for (int i = 1; i <= top_level; i++) {
        pfirst[i] = fast_alloc(sizeof(varLengthbnode) * n_nodes[i]);
    }

    // 4. populate nodes
    for (int ll = 1; ll <= top_level; ll++) {
        ncur[ll] = 0;
        varLengthbnode *np = (varLengthbnode *) (pfirst[ll]);
        np->lock() = 0;
        np->num() = -1;
    }

    varLengthbleaf *leaf = pfirst[0];
    int nodenum = n_nodes[0];

    varLengthbleafMeta leaf_meta;
    leaf_meta.v.bitmap = (((1 << leaf_fill_num) - 1)
            << (LEAF_KEY_NUM - leaf_fill_num));
    leaf_meta.v.lock = 0;
    leaf_meta.v.alt = 0;

    int key_id = start_key;
    for (int i = 0; i < nodenum; i++) {
        varLengthbleaf *lp = &(leaf[i]);

        // compute number of keys in this leaf node
        int fillnum = leaf_fill_num; // in most cases
        if (i == nodenum - 1) {
            fillnum = num_key - (nodenum - 1) * leaf_fill_num;
//            assert(fillnum >= 1 && fillnum <= leaf_fill_num);

            leaf_meta.v.bitmap = (((1 << fillnum) - 1)
                    << (LEAF_KEY_NUM - fillnum));
        }

        // lbtree tends to leave the first line empty
        for (int j = LEAF_KEY_NUM - fillnum; j < LEAF_KEY_NUM; j++) {

            // get key from input
            _key_type mykey = input;
            key_id++;

            // entry
            lp->k(j) = mykey;
            lp->kLength(j) = 1;
            lp->ch(j) = (void *) mykey;

            // hash
            leaf_meta.v.fgpt[j] = hashcode1B(mykey);

        } // for each key in this leaf node

        // sibling pointer
        lp->next[0] = ((i < nodenum - 1) ? &(leaf[i + 1]) : NULL);
        lp->next[1] = NULL;

        // 2x8B meta
        lp->setBothWords(&leaf_meta);


        // populate nonleaf node
        _Pointer8B child = lp;
        _key_type left_key = lp->k(LEAF_KEY_NUM - fillnum);

        // append (left_key, child) to level ll node
        // child is the level ll-1 node to be appended.
        // left_key is the left-most key in the subtree of child.
        for (int ll = 1; ll <= top_level; ll++) {
            varLengthbnode *np = ((varLengthbnode *) (pfirst[ll])) + ncur[ll];

            // if the node has >=1 child
            if (np->num() >= 0) {
                int kk = np->num() + 1;
                np->ch(kk) = child;
                np->k(kk) = left_key;
                np->num() = kk;

                if ((kk == nonleaf_fill_num) && (ncur[ll] < n_nodes[ll] - 1)) {
                    ncur[ll]++;
                    np++;
                    np->lock() = 0;
                    np->num() = -1;
                }
                break;
            }

            // new node
            np->ch(0) = child;
            np->num() = 0;

            child = np;

        }

    }
    return top_level;
}


int varLengthLbtree::bulkload(int keynum, _key_type input, float bfill) {

    BldThArgs bta;
    bta.top_level = bulkloadSubtree(
            input, 0, keynum, bfill, 31,
            bta.pfirst, bta.n_nodes);
    tree_meta->root_level = bta.top_level;
    tree_meta->tree_root = bta.pfirst[tree_meta->root_level];
    tree_meta->setFirstLeaf(bta.pfirst[0]);
    return tree_meta->root_level;
}

varLengthLbtree *new_varLengthlbtree() {
    varLengthLbtree *mytree = static_cast<varLengthLbtree *>(fast_alloc(sizeof(varLengthLbtree)));
    mytree->init();
    _key_type first = (_key_type)fast_alloc(1);
    mytree->bulkload(1, first);
    return mytree;
}