#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/processor.h>
#include "transaction.h"

struct entry_t{
  int element;
  int pad[31];
};
       
struct entry_t closed_counter;

pthread_t *threads;
pthread_attr_t pthread_custom_attr;
int num_ops;
int num_threads;

struct entry_t escape_counter;

struct entry_t escape_sentinel;

void commit_action(unsigned int val);

void* slaveLoop(void *id){

        int myid = *((int*)id);
        int i,j;
        int local_sense = 0;
        volatile int dummy = 0;
        unsigned int input_array[6];
        
        tm_bind_to_cabinet(myid+1);
        Barrier(&local_sense, myid, num_threads);
        set_transaction_registers(myid);
        
        for (i = 0 ; i < num_ops; i++){
          
          for (j = 0; j < 2; j++)
            dummy += random() % num_ops;
            
          BEGIN_CLOSED_TRANSACTION(1)         
            
            //while (escape_sentinel.element){};
            //escape_sentinel.element = 1;
            //input_array[0] = escape_counter.element+1;
            register_commit_action(&commit_action, 0, input_array);
            closed_counter.element++;
          COMMIT_CLOSED_TRANSACTION(1);         
          
       }
}          
                 
void commit_action(unsigned int val){
  
  /*      
  BEGIN_ESCAPE
  escape_counter.element = val;
  escape_sentinel.element = 0;
  END_ESCAPE
  */
  BEGIN_ESCAPE
    escape_counter.element++;
  END_ESCAPE

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
    
    
    closed_counter.element = 0;
    escape_counter.element = 0;
    escape_sentinel.element = 0;
    
    init_transaction_state(num_threads);
    
		StartTime = gethrtime(); 		
    printf("Creating threads...\n");
    
		threads=(pthread_t *)malloc(num_threads*sizeof(*threads));
		ids = (int*)calloc(num_threads, sizeof(int));
		pthread_attr_init(&pthread_custom_attr);
    pthread_attr_setscope(&pthread_custom_attr, PTHREAD_SCOPE_SYSTEM);
    for (i=0; i< num_threads - 1; i++){
			ids[i] = i;	
			pthread_create(&threads[i], &pthread_custom_attr, slaveLoop, (void *)(ids+i));
		}
        
    ids[i] = num_threads - 1;
    slaveLoop(ids+i);
		/* Synchronize the completion of each thread. */

		for (i=0; i<num_threads - 1; i++){
			pthread_join(threads[i],NULL);
		}

    if (closed_counter.element != escape_counter.element){
      printf("COMMIT ACTION FAILED: CLOSED COUNTER: %d ESCAPE_COUNTER: %d\n", closed_counter.element, escape_counter.element);
      LOGTM_ASSERT(0);
    }
    else{
      printf("COMMIT ACTION SUCCESFULL: CLOSED COUNTER: %d ESCAPE_COUNTER: %d\n", closed_counter.element, escape_counter.element);
    }
    
    SIMICS_BREAK_EXECUTION;

		EndTime = gethrtime();
		
		printf("Time = %lld nanoseconds\n", EndTime - StartTime);
		
		return 0;
}		

												
								
							
