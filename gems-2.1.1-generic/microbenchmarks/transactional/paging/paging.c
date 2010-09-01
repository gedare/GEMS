#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/processor.h>
#include <sys/types.h>
#include <assert.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sched.h>
#include "transaction.h"
#include <sys/pset.h>
#include <sys/mman.h>

/* NOTE: This microbenchmark has been tested with OpenSolaris b.33 */

// code to catch segmentation faults
void my_seg_func(){
        printf("Segmentation fault \n");
        NEW_RUBY_MAGIC_CALL(15);
        NEW_RUBY_MAGIC_CALL(0x40000);
        SIMICS_ASSERT(0);
        exit(0);
}

void printDebugMsg(unsigned int val1, unsigned int val2, unsigned int val3, unsigned int tobreak )
{
  asm(".volatile");
  asm("sethi 8001, %g0");
  asm(".nonvolatile");
}

pthread_mutex_t clean_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_t *threads;
pthread_attr_t pthread_custom_attr;
int num_ops;
int num_threads;
int cleanme_flag[32];
int cleanus_counter;

char padding1[1024]; 

volatile int dummy;

typedef struct emptyPage_s {
	char dummy[8192]; 
}	emptyPage_t;

emptyPage_t** GlobalPagesVector;
unsigned long long nopages;

void MAGIC_CALL_resolve_summary_conflict(unsigned int pid, unsigned int tid, unsigned int addr, unsigned int type) {
  asm(".volatile");
  asm("sethi 42, %g0");
  asm(".nonvolatile");
}

void MAGIC_CALL_get_summary_conflict_info(unsigned int *addr, unsigned int *type)
{
  asm(".volatile");
  asm(".register %g2,#scratch");
  asm(".register %g3,#scratch");
  asm("sethi 41, %g0");
  asm("st %g2, [%i0]");
  asm("st %g3, [%i1]");
  asm(".nonvolatile");
}

void handle_summary_conflict(int threadID) 
{
	unsigned int type, addr;
	MAGIC_CALL_get_summary_conflict_info(&addr, &type);
	MAGIC_CALL_resolve_summary_conflict(0, 0, addr, type);
}

void 
slaveLoop(void *id) {
	int myid = *((int*)id);
  int i, j, ret;
  int local_sense = 0;
  int total_iter=0;
  long long int temp;
	char localbuf[1024];
	unsigned int input_array[6];
	int mybinding;
	unsigned long long c;

	emptyPage_t** pagesVector;

	tm_bind_to_cabinet(myid+1);
  Barrier(&local_sense, myid, num_threads);

	if (myid == num_threads-1) { 	
  	set_handler_address();
	  set_log_base(myid);

		int k = 0;
		BEGIN_ESCAPE
		while (1) {
			while (!cleanus_counter);
			for (c=0; c<num_threads+1; c++) {
				memcntl(GlobalPagesVector[c], 8192, MC_SYNC, (caddr_t) (MS_SYNC|MS_INVALIDATE), 0, 0);
			}
			pthread_mutex_lock(&clean_mutex);
			cleanus_counter = 0;
			pthread_mutex_unlock(&clean_mutex);
		}
		END_ESCAPE
	} else {
  	set_handler_address();
	  set_log_base(myid);

		for (i=0; i<num_ops; i++) {
  	  BEGIN_CLOSED_TRANSACTION(1);
  	 		GlobalPagesVector[num_threads]->dummy[0] = 0;
 	  	 	GlobalPagesVector[myid]->dummy[0] = 0;
				for (j = 0; j < 3; j++) {
 	  	 		GlobalPagesVector[num_threads]->dummy[0]++;
	 	  	 	GlobalPagesVector[myid]->dummy[0]++;
					BEGIN_ESCAPE
						pthread_mutex_lock(&clean_mutex);
						cleanus_counter = 1;
						pthread_mutex_unlock(&clean_mutex);
						while(cleanus_counter);
					END_ESCAPE
				}
				SIMICS_ASSERT(GlobalPagesVector[num_threads]->dummy[0]==3);
				SIMICS_ASSERT(GlobalPagesVector[myid]->dummy[0]==3);
  		COMMIT_CLOSED_TRANSACTION(1);
		}
	}
}

int main(int argc, char **argv){
		int n,i,j,c;
		int	*ids;
		hrtime_t  StartTime;
		hrtime_t  EndTime;  

    signal(SIGSEGV, &my_seg_func); 
		nopages = 128;
		
		if (argc != 3)
		{
			printf ("Usage: %s n d\n  where n is no. of threads, d no. of ops\n",argv[0]);
			exit(1);
		}
		
		num_threads = atoi(argv[1]);
		num_ops = atoi(argv[2]);
		srandom(num_ops);
		for (i=0; i<num_threads-1; i++) {
			cleanme_flag[i] = 0;
		}
		cleanus_counter = 0;
    GlobalPagesVector = (emptyPage_t**) calloc(nopages, sizeof(emptyPage_t*));        
    if (!GlobalPagesVector) {	
      printf("ERROR callco failed\n");
      assert(0);
    }
    printf("GlobalPagesVector %lld pages\n", nopages);
    for (c = 0 ; c < nopages; c++) {
      GlobalPagesVector[c] = NULL;
      GlobalPagesVector[c] = (emptyPage_t*) memalign ( 8192, sizeof(emptyPage_t));
      if(!GlobalPagesVector[c]){
        printf("ERROR Global memalign failed\n");
				printf("Cannot allocate page %d\n", c);
        assert(0);
      }
			GlobalPagesVector[c]->dummy[1024]=c; 
    }
    printf("GlobalPagesVector allocation completed\n");
		threads = (pthread_t *) malloc(num_threads*sizeof(*threads));
		ids = (int*)calloc(num_threads, sizeof(int));
		pthread_attr_init(&pthread_custom_attr);
    pthread_attr_setscope(&pthread_custom_attr, PTHREAD_SCOPE_SYSTEM);

    Barrier_init();
    init_transaction_logs(num_threads);
		tm_init_summary_signature_conflict_handlers();
		tm_register_summary_signature_conflict_handler(handle_summary_conflict);
          
		StartTime = gethrtime(); 		
    printf("Creating threads...\n");
        
    for (i=0; i< num_threads - 1; i++)
		{
			ids[i] = i;	
			pthread_create(&threads[i], &pthread_custom_attr, slaveLoop, (void *)(ids+i));
		}
        
    ids[i] = num_threads - 1;
    slaveLoop(ids+i);

		/* Synchronize the completion of each thread. */
		for (i=0; i<num_threads - 1; i++){
			pthread_join(threads[i],NULL);
		}
		EndTime = gethrtime();
		
		printf("Time = %lld nanoseconds\n", EndTime - StartTime);
		return 0;
}		

												
								
							
