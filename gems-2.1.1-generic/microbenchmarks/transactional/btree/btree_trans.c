
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <assert.h>
#include "transaction.h"
#include "btree_c.h"

void  insert(BTree* tree, int tid, key_type_t key, char * string){
  if (string == NULL){
    printf("trying to insert NULL string\n");
    return;
  }
  
  BEGIN_CLOSED_TRANSACTION(0);
  
  BTree_insert(tree, key, string, tid);
  
  COMMIT_CLOSED_TRANSACTION(0);
}

char * lookup(BTree *tree, int tid, key_type_t key){
  char * result;

  BEGIN_CLOSED_TRANSACTION(1);
  
  result = BTree_lookup(tree, key);
  
  COMMIT_CLOSED_TRANSACTION(1);

  return result;
}
