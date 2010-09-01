#include <strings.h>
#include <pthread.h>
#include <transaction.h>
#include "btree_c.h"
#include "btree_c_util.h"

BTreeNode* BTreeNode_findChild(BTreeNode* node, key_type_t key){
  int index;
  char * child_str;
  /*assert(node->children[0] != NULL);*/
  index = BTreeNode_findIndex(node, key);
  
  //if(!node->isLeaf){
  //  SIMICS_ASSERT(index >= 0);
  //}

  if(index < 0){
    return NULL;
  } else {
    return node->children[index];
  }
}

int BTreeNode_findIndex(BTreeNode* node, key_type_t key){
  int insert_at, i;

  /*assert(node != NULL);*/
  insert_at = -1;
  if(node->key_count == 0){
    /*assert(node->isLeaf);*/
    insert_at = 0;
    /*} else if (node->key_count == 1){
      if(key < node->keys[0])
      insert_at = 0;
      else
      insert_at = 1;*/
  } else {
    if(node->isLeaf){
      for(i=0; i<node->key_count && insert_at<0; ++i){
        if(key <= node->keys[i])
          insert_at = i;
      }
    } else {
      for(i=0; i<node->key_count && insert_at<0; ++i){
        if(key < node->keys[i])
          insert_at = i;
      }
    }
    if(insert_at < 0){
      /*printf("node->key_count=%d\n", node->key_count); */
      /*printf("key: %d should be greater than last key: %d\n", key, node->keys[node->key_count-1]); */
      if(node->key_count != 0)
        /*assert(key >= node->keys[node->key_count-1]);*/
      insert_at = node->key_count;       
    }
  }
  
  return insert_at;
}

int BTreeNode_isFull(BTreeNode* node){
  return (node->key_count == NODE_NUM_PAIRS);
}

int BTreeNode_local_insert(BTreeNode *node, int insert_at, key_type_t key, BTreeNode* ptr){
  int i;
  /*assert(node != NULL);*/
  
  if(!node->isLeaf){
    node->key_count++;
    /*node->children[node->key_count] = node->children[node->key_count-1]; shouldn't need this*/
    for(i=node->key_count; i>=insert_at+1; --i){ 
      if(i != NODE_NUM_PAIRS) /* don't insert key for last child */
        node->keys[i] = node->keys[i-1];
      node->children[i] = node->children[i-1];
      //cout << "just moved(" << i << ", " << keys[i] << ", " << (char*) children[i] << ")" << endl;
    }
    node->keys[insert_at] = key;
    node->children[insert_at + 1] = (BTreeNode *) ptr;
  } else {
    //shift keys & children
    node->key_count++;
    node->children[node->key_count] = node->children[node->key_count-1];
    for(i=node->key_count; i>=insert_at+1; --i){
      if(i < NODE_NUM_PAIRS) /* don't insert key for last child */
        node->keys[i] = node->keys[i-1];
      node->children[i] = node->children[i-1];
      //cout << "just moved(" << i << ", " << keys[i] << ", " << (char*) children[i] << ")" << endl;
    }
    if(insert_at < NODE_NUM_PAIRS) /* don't insert key for last child */
      node->keys[insert_at] = key;
    node->children[insert_at] = (BTreeNode *) ptr;
  }
    //print();
  
  /*printf("node after\n");
    BTreeNode_print(node);*/
  /*if(!node->isLeaf)
    assert(node->children[node->key_count] != NULL);
  */

  return(node->key_count == NODE_NUM_PAIRS);
}

key_ptr_pair BTreeNode_split(BTreeNode *node, BTree* tree, int tid){
  int median, i;
  BTreeNode* left;
  BTreeNode* newNode;
  key_ptr_pair return_pair;
  thread_data * t_data;

  /*assert(node != NULL);*/
  /*  assert(node->key_count == NODE_NUM_PAIRS);*/
  median = (NODE_NUM_PAIRS/2)+1;
  /*self = pthread_self();*/
  /*  printf("pthread self is %d\n", self);*/
  /*t_data = (thread_data *) pthread_getspecific(self);*/
  /*id = t_data->id;*/
  /*printf("pthread self %d, id = %d\n", self, id);*/
  /*SIMICS_ASSERT(id >= 0 && id < 32);*/

  BEGIN_CLOSED_TRANSACTION(2);
  if(node->isLeaf){    
    /*printf("splitting leaf node 0x%x\n", node); BTreeNode_print(node); */
    /*newNode = BTree_create_leaf_node(tree, node->keys[median]%(NUM_POOLS));*/
    
    newNode = BTree_create_leaf_node(tree, (tid % NUM_POOLS));

    for(i=0; (i + median) < (NODE_NUM_PAIRS); ++i){
      BTreeNode_local_insert(newNode, i, node->keys[median+i], node->children[median+i]);
    }
    newNode->children[newNode->key_count] = node->children[NODE_NUM_PAIRS];
    node->children[median] = newNode;
  } else {
    /*printf("splitting non-leaf, median is: (%d, 0x%x)\n", median, node->children[median]);*/

    /*assert(node->children[NODE_NUM_PAIRS] != NULL);*/
    left = (BTreeNode*) node->children[median];
    /*newNode = BTree_create_internal_node(tree, left, node->keys[median]%NUM_POOLS);*/
    newNode = BTree_create_internal_node(tree, left, (tid % NUM_POOLS));

    /*assert(median < NODE_NUM_PAIRS);*/
    for(i=0; i+median < NODE_NUM_PAIRS; ++i){
      /*BTreeNode_local_insert(newNode, i, node->keys[median+i], node->children[median+i]);*/
      /*assert(node->keys[median+i] != 0);*/
      newNode->keys[i] = node->keys[median+i];
      newNode->children[i+1] = node->children[median+i+1];
      newNode->key_count++;
    }
  }
  return_pair.key = node->keys[median];
  return_pair.ptr = newNode;
  
  node->key_count = median;
  COMMIT_CLOSED_TRANSACTION(2);

  return return_pair;
}

#ifdef USE_MALLOC

BTreeNode* BTreeNode_allocate(BTree* tree, int pool){
  BTreeNode* newNode;
  
  //SIMICS_ASSERT(pool < NUM_POOLS);
  BEGIN_CLOSED_TRANSACTION(10);
  newNode = (BTreeNode*) malloc(sizeof(BTreeNode));
  COMMIT_CLOSED_TRANSACTION(10);
  tree->m_freeLists[pool].nodeCount++;
  return newNode;
}

#else

BTreeNode* BTreeNode_allocate(BTree* tree, int pool){
  BTreeNode* newNode;

  //SIMICS_ASSERT(pool < NUM_POOLS);
  BEGIN_CLOSED_TRANSACTION(10);
  //SIMICS_ASSERT(tree->m_freeLists[pool].nodeCount < tree->m_nodesPerPool);
  newNode = &tree->m_freeLists[pool].nodes[tree->m_freeLists[pool].nodeCount++];
  COMMIT_CLOSED_TRANSACTION(10);
  return newNode;
}

#endif

void BTreeNode_print(BTreeNode* node){
  int i;
  char* string;
  BTree* child;

  if (node == NULL) {
    printf("NULL node");
    return;
  }

  // print own addr, child count, and isLeaf
  printf("0x%x:%d:%d ", node, node->key_count, node->isLeaf);

  // print contents
  if (node->isLeaf) {
    for (i = 0; i < node->key_count; ++i) {
      string = (char *) node->children[i];
      if (string != NULL)
        printf("%d:%s %llu ", i, string, node->keys[i]);
      else
        printf("%d:null %llu ", i, node->keys[i]);
    }
    /*
      string = (char *) node->children[i];
      if (string != NULL)
      printf("%s ", string);
      else
      printf("null ");
    */
    printf("\n");

  } else {
    for (i = 0; i < node->key_count ; ++i) {
      child = node->children[i];
      if (child != NULL){
        printf("0x%x %llu ", child, node->keys[i]);
        /*assert(node->keys[i] >= 0);*/
      } else {
        printf("null %llu ", node->keys[i]);
      }
    }
    child = node->children[i];
    if (child != NULL)
      printf("0x%x ", child);
    else
      printf("null ");
    printf("\n");
    // recursively print children
    for (i = 0; i <= node->key_count; ++i) {
      BTreeNode_print((BTreeNode*) node->children[i]);
    }
  }
}

/************************************/
/* BTree Class                      */
/************************************/

BTree* BTree_create(){
  BTree* newTree;
  int i;
  int size;

  newTree = (BTree*) malloc(sizeof(BTree));

#ifndef USE_MALLOC
  printf("MAX_NODES = %d\n", MAX_NODES);
  printf("NUM_POOLS = %d\n", NUM_POOLS);
  printf("nodesPerPool = %d\n", MAX_NODES/NUM_POOLS + 1);
  printf("BTree node is %d bytes\n", sizeof(BTreeNode));

  newTree->m_nodesPerPool = MAX_NODES/NUM_POOLS + 1;
  assert(sizeof(BTreeNode) == 128);
  for(i=0; i<NUM_POOLS; ++i){
    size = newTree->m_nodesPerPool*sizeof(BTreeNode);
    printf("allocating size %d\n", size);
    //SIMICS_ASSERT(size > 0);
    newTree->m_freeLists[i].nodes = (BTreeNode *) memalign(128, size);
    //SIMICS_ASSERT(newTree->m_freeLists[i].nodes != NULL);
    bzero(newTree->m_freeLists[i].nodes, size);
  }
#endif

  newTree->m_root = BTree_create_leaf_node(newTree, 0);
  return newTree;
}

// Construct a new Leaf or Internal Node
BTreeNode* BTree_create_leaf_node(BTree* tree, int pool){
  BTreeNode* newNode;
  int i;

  newNode = BTreeNode_allocate(tree, pool);

  for (i=0; i<NODE_NUM_PAIRS; ++i){
    newNode->children[i] = NULL;
    newNode->keys[i] = 5;
  }
  newNode->children[NODE_NUM_PAIRS] = NULL;
  newNode->isLeaf = 1;
  newNode->key_count = 0;

  return newNode;
}

// Construct a new Internal Node
BTreeNode* BTree_create_internal_node(BTree* tree, BTreeNode* left, int pool){
  BTreeNode* newNode;
  
  newNode = BTreeNode_allocate(tree, pool);

  newNode->children[0] = left;
  newNode->keys[0] = 5;
  newNode->children[1] = NULL;
  newNode->key_count = 0;
  newNode->isLeaf = 0;

  return newNode;
}

// Construct a new Root Node
BTreeNode* BTree_create_root_node(BTree* tree, BTreeNode* left, key_type_t key, BTreeNode* right){
  BTreeNode* newNode;

  newNode = BTreeNode_allocate(tree, 0);

  newNode->children[0] = left;
  newNode->keys[0] = key;
  newNode->children[1] = right;
  newNode->key_count = 1;
  newNode->isLeaf = 0;

  /*BTreeNode_print(newNode);*/
  return newNode;
}

void BTree_insert(BTree* tree, key_type_t key, char* value, int tid){
  int full, insert_at;
  BTreeNode* newRoot;
  BTreeNode* node;
  BTreeNode* child;
  key_ptr_pair median;

  /*assert(tree != NULL);*/
  /*assert(tree->m_root != NULL);*/

  if(BTreeNode_isFull(tree->m_root)){
    /*printf("\n\nABOUT TO SPLIT ROOT!\n");*/
    /*BTree_print(tree);*/

    /*printf("new root is = 0x%x\n", newRoot);*/
    BEGIN_CLOSED_TRANSACTION(3);
    median = BTreeNode_split(tree->m_root, tree, tid);
    newRoot = BTree_create_root_node(tree, tree->m_root, median.key, median.ptr);
    /*printf("new root is: 0x%x, median = (%llu, 0x%x)\n", newRoot, median.key, median.ptr);*/
    tree->m_root = newRoot;
    COMMIT_CLOSED_TRANSACTION(3);
    /*printf("JUST SPLIT ROOT!\n");*/
    /*BTree_print(tree);*/
    /*insert_at = BTreeNode_findIndex(newRoot, median.key);*/
    /*printf("insert_at = %d\n", insert_at);*/
    /*BTreeNode_local_insert(newRoot, 1, median.key, median.ptr);*/
  /*BTreeNode_insert(newRoot, median.key, median.ptr, tree);*/
    /*printf("FINISHED INSERT\n");*/
    /*BTree_print(tree);*/
    /*assert(newRoot->isLeaf == 0);*/
    /*assert(tree->m_root->isLeaf == 0);*/
  }
  
  node = tree->m_root;
  while(!node->isLeaf){
    child = BTreeNode_findChild(node, key);
    if (BTreeNode_isFull(child)){
      median = BTreeNode_split(child, tree, tid);
      /*printf("median = (%llu, 0x%x)\n", median.key, median.ptr);*/
      /*BTreeNode_print(node);*/
      insert_at = BTreeNode_findIndex(node, median.key);
      /*printf("insert_at = %d\n", insert_at);*/
      BTreeNode_local_insert(node, insert_at, median.key, median.ptr);

      /*printf("after split\n");*/
      /*BTree_print(tree);*/
      if(key < median.key)
        node = child;
      else
        node = median.ptr;
    } else {
      node = child;
    }
  }
  insert_at = BTreeNode_findIndex(node, key);
  /*printf("for leaf, node=0x%x, key=%llu, insert_at = %d\n", node, key, insert_at);*/
  BTreeNode_local_insert(node, insert_at, key, (BTreeNode*) value);
}

char * BTree_lookup(BTree* tree, key_type_t key){
  char * result;
  BTreeNode* node;
  key_ptr_pair median;

  /*assert(tree != NULL);*/
  /*assert(tree->m_root != NULL);*/

  node = tree->m_root;
  while(!node->isLeaf){
    /*printf("checking node: 0x%x\n", node);*/
    node = BTreeNode_findChild(node, key);
  }
  /*printf("found leaf: 0x%x\n", node);*/
  /*BTreeNode_print(node);*/
  /*assert(node->children[0] != NULL);*/
  /*printf("leaf child[0]: %s\n", (char *) node->children[0]);  */
  result = (char *) BTreeNode_findChild(node, key);

  /* assert(result != NULL); not found? */
  /*printf("found string: %s\n", result);*/
  return result;
}

void BTree_print(BTree* tree){
  BTreeNode_print(tree->m_root);
}
