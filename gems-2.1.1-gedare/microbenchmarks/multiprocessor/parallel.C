
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <synch.h>
#include <assert.h>
#include <sys/time.h>
#include <sys/errno.h>
#include <sys/procset.h>
#include <sys/processor.h>

const int   BUFF_SIZE = 12800;  // 6400 bytes = 100 blocks * 64 bytes/block
#define   MAX_NUMP    16

/* Processor IDs are found, on a Sun SMP, through psrinfo */
//int       ProcessorIds[MAX_NUMP] = {0, 1, 4, 5, 8 , 9, 12, 13, 32, 33, 36, 37, 40, 41, 44, 45};
int       ProcessorIds[MAX_NUMP] = {0, 1, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17};

int       numProcs;
volatile int syncCount;
volatile int syncThreadStart;

hrtime_t  startTime;
hrtime_t  endTime;  
int       Count;
char buffer[BUFF_SIZE];
int iterations;

/* -- define magic call for breakpoint */

#define RUBY_MAGIC_CALL( service )                              \
  do                                                            \
  {                                                             \
    asm volatile (                                              \
                   "sethi %0, %%g0"                             \
                   : /* -- no outputs */                        \
                   : "g" ( (((service) & 0xff) << 16) | 0 )     \
                 );                                             \
  } while( 0 );


/* The function which is called once the thread is allocated */
void* ThreadLoop(void* tmp){  
    int threadId = (int) tmp;  
    int ret3;
    int startTime, endTime;
    
    //    printf("Creating thread %d\n", threadId);

    /* ********************** Thread Initialization *********************** */
    /* Bind the thread to a processor.  This will make sure that each of
     * threads are on a different processor.  ProcessorIds[threadId]
     * specifies the processor ID which the thread is binding to.   */ 

    //    fprintf(stderr, "thread id = %d, proc id = %d\n", threadId, ProcessorIds[threadId]);
    ret3 = processor_bind(P_LWPID, P_MYID, ProcessorIds[threadId], NULL);
    assert(ret3 == 0);
    /* ********************** Thread Synchronization *********************** */

    for(int j=0; j<iterations; j++){
      // each processor acts in turn
      while(syncCount != threadId){
      }    

      if(threadId == 0 && (j==1 || j==2)){
	RUBY_MAGIC_CALL( 4 ); 
      }
      
      // p0 -- write the buffer
      if(threadId == 0){

	for(int i=0; i<BUFF_SIZE; i++){
	  buffer[i] = i;
	}
	syncCount++;

      } 
      // other procs
      else {
	char tmp = '\0';
	for(int i= 0; i<BUFF_SIZE; ++i){
	  //tmp += buffer[i];
	  buffer[i] = threadId;
	}
	if(syncCount < numProcs-1){
	  syncCount++;
	}
	else if (syncCount == numProcs-1){
	  syncCount = 0;
	}
	else{
	  fprintf(stderr, "Error syncCount > numProcs");
	}
      }      

    }// end for j


}

main(int argc, char** argv){
    pthread_t* threads;  
    int ret, ret2; 
    int dx; 

    fprintf(stderr, "starting...\n");

    if(argc != 3){
      fprintf(stderr, "usage: sharing-microbenchmark-read <num procs> <num iterations>\n");
      exit(1);
    }
    numProcs = atoi(argv[1]);  
    iterations = atoi(argv[2]);
    assert(numProcs <= MAX_NUMP);

    /* Initialize array of thread structures */
    threads = (pthread_t *) malloc(sizeof(pthread_t) * numProcs);
    assert(threads != NULL);  /* Initialize mutexs */
    
    // Initialize sync variables
    syncCount = 0;              
    syncThreadStart = 0;

    pthread_attr_t attr;
    pthread_attr_init(&attr); /* initialize attr with default attributes */
    pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);  /* system-wide contention */

    for(dx=0; dx < numProcs; dx++) {
      //      ret = thr_create(NULL, 0, ThreadLoop, (void *) dx, THR_BOUND, &threads[dx]);    
      ret = pthread_create(&threads[dx], &attr, ThreadLoop, (void *) dx);
      assert(ret == 0); 
    }
  
    /* Wait for each of the threads to terminate */
    for(dx=0; dx < numProcs; dx++) {   
      // ret2 = thr_join(threads[dx], NULL, NULL);
      ret2 = pthread_join(threads[dx], (void **)NULL);
      assert(ret2 == 0);  
    }  

    RUBY_MAGIC_CALL( 4 ); 

}
