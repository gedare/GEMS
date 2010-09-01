
#ifndef _BTREE_H
#define _BTREE_H

#include <stdlib.h>
#include <assert.h>

//#define NUM_POOLS 1
#ifndef USE_MALLOC
#define MAX_NODES 600000
#endif
//#define NODE_NUM_PAIRS 5 /* 64 B */
//#define NODE_NUM_PAIRS 1022 /* 8 kB */

//#define NULL 0

/************************************/
/* BTreeNode Class                  */
/************************************/

typedef unsigned long long key_type_t;

typedef struct __BTreeNode {
  key_type_t keys[NODE_NUM_PAIRS];
  void * children[NODE_NUM_PAIRS + 1];
  int key_count;
  int isLeaf;
  int pad1;
  int pad2;
} BTreeNode;

typedef struct __BTreeNodeList {
  int pad1[32];
  BTreeNode* nodes;
  int nodeCount;
  int pad2[29];
} BTreeNodeList;

/************************************/
/* BTree Class                      */
/************************************/

typedef struct __BTree {
  int pad0[32];
  BTreeNode* m_root;
  int pad1[32];
  int m_nodesPerPool;
  int pad4[32];
  BTreeNodeList m_freeLists[NUM_POOLS];
} BTree;

typedef struct __key_ptr_pair {
  key_type_t key;
  void* ptr;
} key_ptr_pair;

int BTreeNode_insert(BTreeNode* node, key_type_t key, BTreeNode * ptr, BTree * tree);

int BTreeNode_findIndex(BTreeNode* node, key_type_t key);

int BTreeNode_local_insert(BTreeNode *node, int insert_at, key_type_t key, BTreeNode* ptr);

key_ptr_pair BTreeNode_split(BTreeNode *node, BTree * tree, int tid);

BTreeNode* BTreeNode_allocate(BTree* tree, int pool);

void BTreeNode_print(BTreeNode* node);

/************************************/
/* BTree Class fns                  */
/************************************/


BTree* BTree_create();

// Construct a new Leaf or Internal Node
BTreeNode* BTree_create_leaf_node(BTree* tree, int pool);

// Construct a new Internal Node
BTreeNode* BTree_create_internal_node(BTree * tree, BTreeNode* left, int pool);

// Construct a new Root Node
BTreeNode* BTree_create_root_node(BTree* tree, BTreeNode* left, key_type_t key, BTreeNode* right);

void BTree_insert(BTree* tree, key_type_t key, char* value, int tid);

char* BTree_lookup(BTree* tree, key_type_t key);

void BTree_print(BTree* tree);


// Top-Level Functions
void insert(BTree* tree, int tid, key_type_t key, char * string);
char * lookup(BTree* tree, int tid, key_type_t key);

#endif
