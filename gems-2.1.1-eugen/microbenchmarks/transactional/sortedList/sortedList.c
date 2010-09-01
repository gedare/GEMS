#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include "transaction.h"

typedef struct _listNode{
  int key;
  struct _listNode *next;
  struct _listNode *prev;
}ListNode;            

typedef ListNode* Nodeptr;

typedef struct _globalNode{
  int cachepad0[32];      
  volatile int      global_search_count;        
  int cachepad1[32];
  volatile int      global_op_count;
  int cachepad2[32];
  volatile int      global_mutate_count;
  int cachepad3[32];
  Nodeptr  list_head;
  int cachepad4[64];
  volatile int      sense;
  volatile int      count;
}Global; 

int num_operations;
int num_threads;
int sleep_time;

Nodeptr init_nodes;

Global gm;

int listInsert(Nodeptr *head, int key){
    Nodeptr currNode;
    Nodeptr prevNode;
    Nodeptr newNode;
    int result = 1; 
    newNode = (Nodeptr)malloc(sizeof(ListNode));      
    
    BEGIN_TRANSACTION(0);     
    currNode = *head;
    prevNode = NULL;
    while (currNode != NULL){
      if (currNode->key < key){
          prevNode = currNode;
          currNode = currNode->next;
      } else if (currNode->key == key){
          result = 0;
          break;
      } else {
          result = 1;
          break;
      }            
    }

    if (result == 1){
      newNode->key = key;
      newNode->prev = prevNode;
      newNode->next = currNode;
      if (prevNode != NULL)
        prevNode -> next = newNode;
      if (currNode != NULL)
        currNode -> prev = newNode;
      if (currNode == *head)
          *head = newNode;
    }

    BEGIN_TRANSACTION(3);
    gm.global_op_count++;
    gm.global_mutate_count++;
    COMMIT_TRANSACTION(3);
    
    COMMIT_TRANSACTION(0);     
   
    if (result == 0)
      free(newNode);
            
    return result;
}    
         
int listSearch(Nodeptr *head, int key){
    Nodeptr currNode;
    int found = 0;
    volatile int k;
    int j;

    BEGIN_TRANSACTION(1);     
    
    currNode = *head;
    while (currNode != NULL && found == 0){
      if (currNode -> key == key)
        found = 1;
      else
        currNode = currNode->next;
      for (j = 0; j < 275; j++){
        k += gm.global_search_count;
      }  
    }
                                  
    BEGIN_TRANSACTION(14);
    for (j = 0; j < 200; j++){
      k += gm.global_op_count;
    }  
    gm.global_op_count++;
    COMMIT_TRANSACTION(14);
    
    COMMIT_TRANSACTION(1);     
     
    return found;
}

int listDelete(Nodeptr *head, int key){
    Nodeptr currNode;
    int found = 0;

    BEGIN_TRANSACTION(2);     
    
    currNode = *head;
    while(currNode != NULL && found == 0){
      if (currNode -> key == key)
        found = 1;
      else
        currNode = currNode->next;
    } 
    if (found == 1){
       if (currNode->prev != NULL)     
         currNode->prev->next = currNode->next;
       if (currNode->next != NULL)
         currNode->next->prev = currNode->prev;
       if (currNode == *head)
         *head = currNode->next;
    }                                                       
    
    BEGIN_TRANSACTION(3);
    gm.global_op_count++;
    gm.global_mutate_count++;
    COMMIT_TRANSACTION(3);
    
    COMMIT_TRANSACTION(2);     
    
    if (found == 1){
      free(currNode);  
    }  
    
    return found;
}

void printList(Nodeptr *head){
    Nodeptr currNode;
    
    currNode = *head;
    printf("Sorted List: ");  
    while (currNode != NULL){
      printf("%d ", currNode->key);
      currNode = currNode->next;
    }
    printf("\n");
}

void* slaveStart(void *id){
		int myid;
		int i,j;
    int result;
    int local_sense = 0;
    Nodeptr listHead,currNode;
	  
    myid = *((int*)id);
    listHead = gm.list_head;
        
    tm_bind_to_cabinet(myid + 1);
    Barrier_breaking(&local_sense, myid, num_threads);
    set_transaction_registers(myid);

    /* warm-up caches */
    currNode = listHead;
    while (currNode != NULL){
      currNode = currNode->next;
    }

    for (i = 0; i < num_operations; i++){
      BEGIN_WORKLOAD_TRANSACTION     
      int key = 64 + ((int)random()) & (0x0000001f) ;
      listSearch(&listHead, key);
      END_WORKLOAD_TRANSACTION                 
                  
    }
    Barrier_breaking(&local_sense, myid, num_threads);
}        

int main(int argc, char **argv){
        
  int i,j;
  int init_length;
  pthread_t *threads;
  pthread_attr_t pthread_custom_attr;
  int	*ids;
  hrtime_t  StartTime;
  hrtime_t  EndTime; 
        
  float p; 

  if (argc != 4)
	{
			printf ("Usage: %s p n init_len \n  where p is no. of processors, n num_operations (multiple of 16)\n",argv[0]);
			exit(1);
  }
  num_threads = atoi(argv[1]);
  num_operations = atoi(argv[2]) / num_threads ;
  init_length = atoi(argv[3]);
        
  srandom(num_operations);
  gm.list_head = NULL;
  gm.count  = 0;
  gm.sense = 0; 

  init_nodes = (Nodeptr)calloc(init_length, sizeof(ListNode));
  for (i = 0; i < init_length; i++){
    init_nodes[i].key = 2 * i;
    if (i != (init_length - 1))
      init_nodes[i].next = (Nodeptr)(init_nodes+i+1);         
    else  
      init_nodes[i].next = NULL;                   
    if (i != 0)  
      init_nodes[i].prev = (Nodeptr)(init_nodes+i-1);
    else
      init_nodes[i].prev = NULL;         
  }
  gm.list_head = init_nodes;
        
  init_transaction_state(num_threads); 	
  
  StartTime = gethrtime(); 
  threads=(pthread_t *)malloc(num_threads*sizeof(pthread_t *));
  ids = (int*)calloc(num_threads, sizeof(int));
  pthread_attr_init(&pthread_custom_attr);
  for (i=0; i<num_threads; i++)
	{
			ids[i] = i;	
			pthread_create(&threads[i], &pthread_custom_attr, slaveStart, (void *)(ids+i));
	}
  
  for (i=0; i<num_threads; i++)
	{
		pthread_join(threads[i],NULL);
	}
  
  EndTime = gethrtime();
  printf("Time = %lld nanoseconds\n", EndTime - StartTime);
  printList(&gm.list_head);         
  
  return 0;
}                
               
              
            
             
                                  
                  
                     
            
                    
