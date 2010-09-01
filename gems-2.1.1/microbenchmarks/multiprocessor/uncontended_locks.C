#include <stdlib.h>
#include <stdio.h>
#include <sys/atomic.h>
#include <iostream.h>
#include <assert.h>
#include <sys/time.h>
#include <sys/errno.h>
#include <sys/procset.h>
#include <sys/processor.h>
#include <sys/types.h>
#include <unistd.h>
#include <thread.h>
#include <synch.h>


// Locking Mechanism
// returns old value of the lock (0 to 255, a la ldstub) 
// (implemented in tryLock.s)
extern "C" int tryLock(void *addr);


////////////////////////////
// lock structure
////////////////////////////

struct Lock {
  volatile int lock_var;
  char pad[60]; // padded to 64 bytes so it fills a cache line
};

struct Counter {
  volatile int count_var;
  char pad[60]; // padded to 64 bytes so it fills a cache line
};

////////////////////////////
// Global variables
////////////////////////////

int * processorIds;
int numProcs;
int numLocks;
int acquires; 

Lock * lock_array;
Counter * counter_array;
int ** access_order;


const int small_spin_length = 5000;

///////////////////////////
// Magic Services
///////////////////////////

typedef enum 
{ 
  Do_Breakpoint         =  4,
  Enable_Memory_System  = 10,
  Disable_Memory_System = 11,
  Clear_Profile_Stats   = 12,
  Dump_Profile_Stats    = 13,
  Enable_STC            = 14,
  Disable_STC           = 15,
  Flush_STC             = 16,
  Set_CPU_Switch        = 17,
  Dump_Cache            = 18,
  Invalidate_STC        = 19
} Ruby_Magic_Service;

#define RUBY_MAGIC_CALL( service )                              \
  do                                                            \
  {                                                             \
    asm volatile (                                              \
                   "sethi %0, %%g0"                             \
                   : /* -- no outputs */                        \
                   : "g" ( (((service) & 0xff) << 16) | 0 )     \
                 );                                             \
  } while( 0 );


///////////////////////////
// Functions
///////////////////////////

inline extern void acquire_lock(Lock * lock_ptr, int threadId, Counter * counter)
{
  int val;
  bool got_lock = false;
  while(!got_lock) {
   
#ifdef DEBUG
    printf("thread %d stuck in acquire_lock loop on lock %x, counter = %d\n", 
 	   threadId, lock_ptr, counter->count_var);
#endif
    
    // Test
    while(lock_ptr->lock_var != 0) {
      // Nothing
#ifdef DEBUG
      printf("thread %d stuck in test loop on lock %x, counter = %d\n", threadId, lock_ptr, counter->count_var);
#endif
    }
    
    // Atomic Test & Set
    val = tryLock((void *) &(lock_ptr->lock_var));
    
    // got lock
    if(val == 0){
      got_lock = true;
#ifdef DEBUG
      printf("Got the lock, and lock_var is %x\n", lock_ptr->lock_var);
#endif

    }
  }
}

// release a held lock
inline extern void release_lock(Lock * lock_ptr){
  lock_ptr->lock_var = 0;

#ifdef DEBUG
  printf("release test: lock_var = %d\n", lock_ptr->lock_var);
#endif

}


// Do nothing for a set number of cycles
inline extern void noop(){
  asm volatile ("srl %g0, 0, %g0");
}

void delay(int cycles)
{
  for(int i=0; i<cycles; ++i) {
    noop();
  }
}


void* thread_loop(void* tmp)
{
  int threadId = (int) tmp;  
  int ret;
  
  /* Bind the thread to a processor.  This will make sure that each of
   * threads are on a different processor.  processorIds[threadId]
   * specifies the processor ID which the thread is binding to.   */ 
  ret = processor_bind(P_LWPID, P_MYID, processorIds[threadId], NULL);
  assert(ret == 0);
  
#ifdef DEBUG
  printf("thread id = %d, proc id = %d\n", threadId, processorIds[threadId]);
#endif
  
  // Each thread accesses locks in random order and increments the counter
  for(int i=0; i<acquires; ++i){
    int lock_index = access_order[threadId][i];

#ifdef DEBUG
    printf("Thread %d trying lock %d\n", threadId, lock_index);
#endif

    acquire_lock(&(lock_array[lock_index]), threadId, &(counter_array[lock_index]));
    
    // update counter
    counter_array[lock_index].count_var = threadId;

#ifdef DEBUG
    printf("Thread %d releasing lock %d\n", threadId, lock_index);
#endif
    release_lock(&(lock_array[lock_index]));

    // wait a small number of cycles
    delay(small_spin_length);
  }
}

main(int argc, char** argv)
{
  if(argc != 4){
    fprintf(stderr, "usage: uncontended_locks <num procs> <num locks> <acquires>\n");
    exit(0);
  }

#ifdef DEBUG
  printf("starting uncontended locks microbenchmark\n");
#endif

  thread_t* threads;  
  
  // parse num procs, stride and iterations
  numProcs = atoi(argv[1]);
  numLocks = atoi(argv[2]);
  acquires = atoi(argv[3]);

  int maxNumProcs = sysconf(_SC_NPROCESSORS_ONLN);
  processorIds = new int[maxNumProcs];
  
  assert(numProcs <= maxNumProcs);
  
  // Initialize thread to processor bindings
  int procId = 0;
  for(int i=0; i<numProcs; ++i){
    int cont = 1;
    for(; cont; ++procId){
      int status = p_online(procId, P_STATUS);
      if (status == P_ONLINE){
	cont = 0;
	processorIds[i] = procId; 
      }
    }
  }

  // Initialize array of thread structures
  threads = (thread_t *) malloc(sizeof(thread_t) * numProcs);
  assert(threads != NULL);
    
  // Initialize random lock access order array
  access_order = new int*[numProcs];
  for(int i=0; i<numProcs; ++i){
    access_order[i] = new int[acquires];
    for(int j=0; j<acquires; ++j){
      int tmp = rand();
      access_order[i][j] = tmp % numLocks;
    }
  }

  // Initialize Lock and Counter arrays
  lock_array = new Lock[numLocks];
  counter_array = new Counter[numLocks];
  
  for(int i=0; i<numLocks; ++i){
    lock_array[i].lock_var = 0;
    counter_array[i].count_var = 0;
  }

#ifdef DEBUG
  printf("About to create threads\n");
#endif

  // create the threads
  int ret;
  for(int dx=0; dx < numProcs-1; dx++) {
    ret = thr_create(NULL, 0, thread_loop, (void *) dx, THR_BOUND,
		     &threads[dx]);    
    assert(ret == 0); 
  }
  
  // magic break to clear ruby stats
  RUBY_MAGIC_CALL(Do_Breakpoint);
  thread_loop((void*) (numProcs-1));

  // Wait for each of the threads to terminate 
  for(int dx=0; dx < numProcs-1; dx++) {    
    ret = thr_join(threads[dx], NULL, NULL);
    assert(ret == 0);  
  }  


}



  
