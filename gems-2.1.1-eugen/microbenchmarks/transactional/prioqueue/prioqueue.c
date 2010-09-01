#include "transaction.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>


//Globals...
#define HEAPSIZE 256
#define NUM_ENTRIES 2048
#define NOPE HEAPSIZE+1 

int num_operations;
int num_threads;

int** key_access_table;

typedef struct{
  int element;
  int lookup_index;
  int pad[30];
}heapEntry;
           

struct {
  int pad[32];      
  int             maxSize;      // Maximum size of the heap
  int pad1[32];
  int             currentSize;  // Number of elements in heap
  int pad2[32];
  heapEntry       *array; // Heap - Min priority Queue 
  int pad3[32];
  int             *lookup_array;  // Index into the heap
}gm;  

int  op_array[5];
int insertHeap( int x, int key);
void percolateUp( int hole, int * new_index );
void percolateDown( int hole, int * new_index );
void deleteHeapMin( int * min );


//************************************************************
// ************** HEAP CLASS ***************************
/**
 * Construct the binary heap.
 * capacity is the capacity of the binary heap.
 */
void initializeHeap( int capacity )
{
  int i, temp;
  gm.currentSize = 0;
  gm.array = (heapEntry *) malloc( (capacity+1)*sizeof(heapEntry));
  gm.lookup_array = (int *) malloc ( (NUM_ENTRIES+1) * sizeof(int));
  gm.maxSize = capacity;
  printf("Heap Capacity = %d\n", capacity);
  for(i=0; i < HEAPSIZE; ++i){
    //temp = random() % HEAPSIZE;
    temp = 0;   /* All keys have frequency counts of zero */
    insertHeap( temp, NUM_ENTRIES + 1 );
  }
  for (i=0; i < NUM_ENTRIES + 1; i++){
    gm.lookup_array[i] = NOPE;  /* Initially no elements in heap. */ 
  }           
  for(i =0; i < 5; ++i){
    op_array[i] = 0;
  }
}

void deleteHeap(){
  if(gm.array){
    free(gm.array);
  }
}

/**
 * Insert item x into the priority queue, maintaining heap order.
 * Duplicates are allowed.
 * Throw Overflow if container is full.
 */
int insertHeap( int  x , int key)
{
  if( isFull( ) ){
    //  cout << "Tree is FULL numElements = " << gm.currentSize << endl;
    return;
  }
  
  // Percolate up
  int hole = ++gm.currentSize;
  int new_index = 0;
  //temporarily assign the new element to hole location
  gm.array[hole].element = x;
  gm.array[hole].lookup_index = key;
  gm.lookup_array[key] = hole;
  //get element in correct index
  percolateUp( hole, &new_index );
  return new_index;
}

/*
 * Given an index of the item of interest, increase that key's value by delta, then
 * re-heapify
 */
void increaseHeapKey( int index, int delta, int * new_index ){
  if( !isEmpty()){
    LOGTM_ASSERT( 1 <= index && index <= gm.currentSize );
    gm.array[index].element += delta;
    //need to percolate down
    percolateDown(index, new_index);
  }
}

/*
 * Given an index of the item of interest, decrease that key's value by delta, then
 * re-heapify
 */
void decreaseHeapKey( int index, int delta, int * new_index ){
  if( !isEmpty()){
    LOGTM_ASSERT( 1 <= index && index <= gm.currentSize );
    gm.array[index].element -= delta;
    // need to percolate up
    percolateUp(index, new_index);
  }
}

/*
 * Given an index of the item needed to be removed, do the following:
 *    1) Perform decreaseKey() until it reaches the correct position
 *    2) Call deleteHeapMin()
 */
void removeFromHeap( int index ){
  if( !isEmpty() ){
    LOGTM_ASSERT( 1 <= index && index <= gm.currentSize );
    int min_element = findMin();
    int new_index = 0;
    // squash this value, and propagate MIN up
    gm.array[index].element = min_element;
    percolateUp(index, &new_index );
    LOGTM_ASSERT( new_index != 0 );
    
    //now we should be the minimum element
    deleteHeapMin( &min_element );
  }
}

/**
 * Find the smallest item in the priority queue.
 * Return the smallest item, or throw Underflow if empty.
 */
int  findMin( )
{
  if( isEmpty( ) ){
    return;
  }

  return gm.array[ 1 ].element;
}

/**
 * Remove the smallest item from the priority queue
 * and place it in minItem. Throw Underflow if empty.
 */
void deleteHeapMin( int * minItem )
{
  if( isEmpty( ) )
    {
      return;
    }
  
  int new_index = 0;
  *minItem = gm.array[ 1 ].element;
  gm.lookup_array[ gm.array[ 1 ].lookup_index] = NOPE; 
  gm.array[ 1 ].element = gm.array[ gm.currentSize ].element;
  gm.array[ 1 ].lookup_index = gm.array[ gm.currentSize] .lookup_index;
  gm.lookup_array[ gm.array[ 1 ].lookup_index] = 1; 
  gm.currentSize--;
  percolateDown( 1, &new_index );
}

/**
 * Establish heap order property from an arbitrary
 * arrangement of items. Runs in linear time.
 */
void buildHeap( )
{
  int i, new_index;
  for(i = gm.currentSize / 2; i > 0; i-- )
    percolateDown( i, &new_index );
}

/**
 * Test if the priority queue is logically empty.
 * Return true if empty, false otherwise.
 */
int isEmpty( )
{
  return (gm.currentSize == 0);
}

/**
 * Test if the priority queue is logically full.
 * Return true if full, false otherwise.
 */
int isFull( )
{
  return (gm.currentSize == gm.maxSize);
}

/**
 * Make the priority queue logically empty.
 */
void makeEmpty( )
{
  gm.currentSize = 0;
}

/**
 * Internal method to percolate UP in the heap
 * hole is the index at which percolate begins, tmp is the node value
 * you want to percolate.  
 *  New location for item is at new_index
 */
void percolateUp( int hole, int * new_index ){
  int tmp = gm.array[hole].element;
  int tmp_l = gm.array[hole].lookup_index;
  for( ; hole > 1 && tmp < gm.array[ hole / 2 ].element; hole /= 2 ){
    gm.array[ hole ].element = gm.array[ hole / 2 ].element;
    gm.array[ hole ].lookup_index = gm.array[ hole / 2 ].lookup_index;
    gm.lookup_array[ gm.array[ hole / 2].lookup_index ] = hole;
  }  
  gm.array[ hole ].element = tmp;
  gm.array[ hole ].lookup_index = tmp_l;
  gm.lookup_array[ tmp_l ] = hole;
  *new_index = hole;
}

/**
 * Internal method to percolate down in the heap.
 * hole is the index at which the percolate begins.
 * New location for item is at new_index
 */
void percolateDown( int hole, int * new_index )
{
  /* 1*/      int child;
  /* 2*/      int tmp = gm.array[ hole ].element;
  /* 2*/      int tmp_l = gm.array[ hole ].lookup_index;
  
  /* 3*/      for( ; hole * 2 <= gm.currentSize; hole = child )
    {
      /* 4*/          child = hole * 2;
      /* 5*/          if( child != gm.currentSize && gm.array[ child + 1 ].element < gm.array[ child ].element )
        /* 6*/              child++;
      /* 7*/          if( gm.array[ child ].element < tmp ){
        /* 8*/              gm.array[ hole ].element = gm.array[ child ].element;
        /* 8*/              gm.array[ hole ].lookup_index = gm.array[ child ].lookup_index;
        /* 8*/              gm.lookup_array[ gm.array[ child ].lookup_index] = hole;
                      } 
                      else
        /* 9*/              break;
    }
  /*10*/      gm.array[ hole ].element = tmp;
  /*10*/      gm.array[ hole ].lookup_index = tmp_l;
              if (gm.lookup_array[tmp_l] != hole)
                gm.lookup_array[ tmp_l ] = hole; 
  *new_index = hole;
}

//*********************************
// prints the current tree
void printHeap(){
  int i;
  printf("PrintHeap: gm.currentSize = %d\n", gm.currentSize);
  printf("[ ");
  for(i=0; i <= gm.currentSize; ++i){
    if (gm.array[i].lookup_index == (NUM_ENTRIES + 1))
      printf("(%d, - ) ", gm.array[i].element);
    else
      printf("(%d, %d) ", gm.array[i].element, gm.array[i].lookup_index);
  }
  printf("]\n");
}

void printLookupArray(){
  int i;
  printf("Lookup Array: \n");
  printf("[ ");
  for(i=0; i <= NUM_ENTRIES; ++i){
    if (gm.lookup_array[i] != NOPE)      
      printf("(%d, %d) ", i, gm.lookup_array[i]);
  }
  printf("]\n");
}  
          

//************** END HEAP CLASS *********************
//***********************************************************
//************************************************************
void insert(int id, int key){
  printf("Insert thread %d key %d\n", id, key);

  BEGIN_TRANSACTION(16);

  //op_array[0]++;
  insertHeap(1, key);

  COMMIT_TRANSACTION(16);

}

void deleteMin(int id, int *key){
  //returns the minimum value in key
  
            
  BEGIN_TRANSACTION(17);
            

  //op_array[1]++;
  deleteHeapMin(key);

            
  COMMIT_TRANSACTION(17);
            
}

void increaseKey(int delta){
  int index, new_index;
  int rnd = random();

            
  BEGIN_TRANSACTION(2);
            

  //get a random index
  index = rnd % (gm.currentSize+1);
  if(index == 0){
    index = (gm.currentSize+1)/2;
  }
  //op_array[2]++;
  increaseHeapKey(index, delta, &new_index);

            
  COMMIT_TRANSACTION(2);
            
}

void decreaseKey(int delta){
  int index, new_index;
  int rnd = random();

            
  BEGIN_TRANSACTION(3);
            

  //get a random index
  index = rnd % (gm.currentSize+1);
  if(index == 0){
    index = (gm.currentSize+1)/2;
  }
  //op_array[3]++;
  decreaseHeapKey(index, delta, &new_index);

            
  COMMIT_TRANSACTION(3);
            
}

void removeKey(){
  int index;
  int rnd = random();

            
  BEGIN_TRANSACTION(4);
            
  //get a random index
  index = rnd % (gm.currentSize+1);
  if(index == 0){
    index = (gm.currentSize+1)/2;
  }
  //op_array[4]++;
  removeFromHeap(index);

            
  COMMIT_TRANSACTION(4);
            
}

void accessKey(int myid, int key){
    int delta; 
    
            
    BEGIN_TRANSACTION(18);
            
      if (gm.lookup_array[key] == NOPE){
        /* Element not present in LFU Cache */
        int min, new_index;
        deleteMin(myid, &min);
        new_index = insertHeap(1, key);
        //printHeap();
        //printLookupArray();
      } else {
        int new_index;
        int index = gm.lookup_array[ key ];
        increaseHeapKey(index, 1, &new_index);      
        //printHeap();
        //printLookupArray();
      }  
            
      COMMIT_TRANSACTION(18);
            

}        

void * slaveStart(void * id) {
  int myid;
  int i,j;
  int result;
  int local_sense = 0;
  int key_access_count = 0;
	  
  myid = *((int*)id);

  tm_bind_to_cabinet(myid + 1);
  Barrier_breaking(&local_sense, myid, num_threads);
  set_transaction_registers(myid);

  for (i = 0; i < num_operations; i++){
    int op = 5;
    int delta = random() % 101;
    int key = random() % NUM_ENTRIES;

    BEGIN_WORKLOAD_TRANSACTION
      
    switch (op){
    case 0:
      insert(myid, key);
      break;                
    case 1:
      key = 0;
      deleteMin(myid, &key);
      break;  
    case 2:
      increaseKey(delta);
      break;
    case 3:
      decreaseKey(delta);
      break;
    case 4:
      removeKey();
      break;
    case 5:
      key = key_access_table[myid][key_access_count++];
      accessKey(myid, key);
    default:
      break;
    }
      
    END_WORKLOAD_TRANSACTION
    //printf("%d Operation %d Key %d \n", result, op, key); 
  }
  Barrier_breaking(&local_sense, myid, num_threads);
}        


int main(int argc, char **argv){
  int i,j, temp;
  pthread_t *threads;
  pthread_attr_t pthread_custom_attr;
  int	*ids;
  hrtime_t  StartTime;
  hrtime_t  EndTime;  
  
  if (argc != 3)
    {
      printf ("Usage: %s p n\n  where p is no. of processors, n num_operations (multiple of 16)\n",argv[0]);
      exit(1);
    }
  num_threads = atoi(argv[1]);
  num_operations = atoi(argv[2]) / num_threads;
        
  srandom(num_operations);

  //initialize the tree...fill up all of the tree!
  gm.array = NULL;
  initializeHeap(HEAPSIZE);

  key_access_table = (int**) calloc(num_threads, sizeof(int*));
  for (i = 0; i < num_threads; i++)
    key_access_table[i] = (int*) calloc(2 * num_operations, sizeof(int));

  for (i = 0; i < num_threads; i++){
    for ( j = 0; j < 2 * num_operations; j++)      
      scanf("%d\n", &key_access_table[i][j]);
  }          

  for (i = 0; i < num_operations / 2 ; i++){
    accessKey(0, key_access_table[0][i]);
  }

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
        
  return 0;
}                
   
