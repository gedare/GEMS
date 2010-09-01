#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/processor.h>
#include <sys/types.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sched.h>
#include "transaction.h"
#include <sys/pset.h>
#include <strings.h>

/* NOTE: This microbenchmark has been tested with OpenSolaris b.33 */

void printDebugMsg(unsigned int val1, unsigned int val2, unsigned int val3, unsigned int tobreak)
{
  asm(".volatile");
  asm("sethi 8001, %g0");
  asm(".nonvolatile");
}

pthread_t *threads;
pthread_attr_t pthread_custom_attr;
int num_ops;
int num_threads;

struct {
	char pad[32];
	unsigned long long value;
	char pad2[30];
}	global_counter;

volatile int dummy;

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
slaveLoop(void *id)
{
	int myid = *((int*)id);
  unsigned int i, j, ret;
  int local_sense = 0;
	char localbuf[1024];
	unsigned int input_array[6];
	int mybinding;
	unsigned long long cycle;

	tm_bind_to_cabinet((myid%(num_threads/2))+1);
	Barrier(&local_sense, myid, num_threads);

	if (myid > num_threads/2-1) {
		BEGIN_ESCAPE
			while(1) {	
				thr_yield();
			}
		END_ESCAPE
	} else {
		set_handler_address();
		set_log_base(myid);

		while (1) {
			BEGIN_WORKLOAD_TRANSACTION
			BEGIN_CLOSED_TRANSACTION(0)
				global_counter.value = 0;
				for (i=0;i<10;i++) { 
					global_counter.value++;
					BEGIN_ESCAPE
						for (j=0;j<128;j++);
						thr_yield();
					END_ESCAPE
				}
				SIMICS_ASSERT(global_counter.value == 10);
			COMMIT_CLOSED_TRANSACTION(0)
			END_WORKLOAD_TRANSACTION
			for (i=0;i<1024;i++);
		}
	}
}

int main(int argc, char **argv){
		int n,i,j;
		int	*ids;
		hrtime_t  StartTime;
		hrtime_t  EndTime;  
		
		if (argc != 3)
		{
			printf ("Usage: %s n d\n  where n is no. of threads, d no. of ops\n",argv[0]);
			exit(1);
		}
		
		num_threads = atoi(argv[1]);
		num_ops = atoi(argv[2]);
		srandom(num_ops);
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
