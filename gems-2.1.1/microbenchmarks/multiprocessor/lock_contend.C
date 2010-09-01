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

////////////////////////////
// Global variables
////////////////////////////

int * processorIds;
int numProcs;
int acquires;


// lock variable (0 == not locked) initially locked.
int pad0[16];
volatile int lock_var = 1;
int pad1[16];

struct Counter {
  int pad0[16];
  int counter;
  int pad1[16];
};

Counter protected_counters[32];
Counter protected_counter;

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

extern "C" int tryLock(void *addr); /* it's in a .s file... */
/* returns old value of the lock (0 to 255, a la ldstub) */

#if 0 
// in file tryLock.s
.seg "text"
	.align 4

	.global tryLock
	.type tryLock, #function
	
tryLock:
	/* %o0 contains addr of the lock; word-aligned so we add 3 bytes before
	doing our ldstub, so we only write to the least significant byte */
ldstub [%o0+3], %o0 /* %o0 serves as both the input param & the result */
retl
nop
#endif

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

// Tries to get the lock.
// Returns true if the lock was
// aquired. 
/*
inline extern bool aquire_lock(){
  int val = lock_var;
  bool retval = false;
  
  if(lock_var == 0){
    val = tryLock((void *) &lock_var);
    
    // got lock
    if(val == 0){
      retval = true;
    }
  }

  return retval;
}
*/

inline extern void acquire_lock()
{
  int val;
  while(1) {
    // Test
    while(lock_var != 0) {
      // Nothing
    }
    
    // Atomic Test & Set
    val = tryLock((void *) &lock_var);
    
    // got lock
    if(val == 0){
      return;
    }
  }
}

#define RUBY_MAGIC_CALL( service )                              \
  do                                                            \
  {                                                             \
    asm volatile (                                              \
                   "sethi %0, %%g0"                             \
                   : /* -- no outputs */                        \
                   : "g" ( (((service) & 0xff) << 16) | 0 )     \
                 );                                             \
  } while( 0 );


void* getLock(void* tmp)
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
  
  // All threads race for the lock 
  // as soon as it is released
  while (1) {
    // Lock
    acquire_lock();
    //    delay(small_spin_length);
  
    protected_counters[threadId].counter++;
    protected_counter.counter++;

    if (protected_counter.counter > acquires) {
      RUBY_MAGIC_CALL(Do_Breakpoint);
      for(int i=0; i < numProcs; i++) {
	printf("Thread id = %d, proc id = %d, count = %d, percent = %d\n", i, processorIds[i],
	       protected_counters[i].counter, 
	       (100*protected_counters[i].counter)/protected_counter.counter);
      }
      exit(0);
    }

    //    delay(small_spin_length);
    // Unlock
    lock_var = 0;
    //    delay(3*small_spin_length);
    delay(200);
  }
}

main(int argc, char** argv)
{
  if(argc != 4){
    fprintf(stderr, "usage: lock_contend <num procs> <#cycles to hold lock> <acquires>\n");
    exit(0);
  }

  thread_t* threads;  
    
  // parse num procs, stride and iterations
  numProcs = atoi(argv[1]);
  int spin_length = atoi(argv[2]);
  acquires = atoi(argv[3]);

  int maxNumProcs = sysconf(_SC_NPROCESSORS_ONLN);
  processorIds = new int[maxNumProcs];
  
  assert(numProcs <= maxNumProcs);
  
  // Initialize thread to processor bindings
  int procId = -1;
  for(int i=0; i<numProcs; ++i){
    while(1) {
      procId++;
      int status = p_online(procId, P_STATUS);
      if (status == P_ONLINE){
	processorIds[i] = procId;
	//	printf("Thread id = %d, proc id = %d\n", i, processorIds[i]);
	break;
      }
    }
  }

  /* Initialize array of thread structures */
  threads = (thread_t *) malloc(sizeof(thread_t) * numProcs);
  assert(threads != NULL);  /* Initialize mutexs */
    
  // Initialize counters
  assert(numProcs <= 16);
  //  protected_counters = new int[numProcs];
  protected_counter.counter = 0;
  for(int i=0; i < numProcs; i++) {
    protected_counters[i].counter = 0;
  }

  // create the threads
  int ret;
  for(int dx=0; dx < numProcs; dx++) {
    ret = thr_create(NULL, 0, getLock, (void *) dx, THR_BOUND,
		     &threads[dx]);    
    assert(ret == 0); 
  }
  
  // something's wrong if the lock is free
  assert(lock_var != 0);
  
  // wait for a bit to be sure that all 
  // processes are spinning on the lock

  delay(spin_length);

#ifdef DEBUG
  printf("p0 releasing the lock");
#endif      
  
  // magic break to clear ruby stats
  RUBY_MAGIC_CALL(Do_Breakpoint);
  
  delay(spin_length);

  lock_var = 0; // release the lock
  
  // Wait forever
  while(1){
    noop();
  }

}



  
