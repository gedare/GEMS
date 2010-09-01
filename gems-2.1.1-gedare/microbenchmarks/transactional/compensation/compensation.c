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

struct entry_t open_counter;

void compensating_action();

void* slaveLoop(void *id){

        int myid = *((int*)id);
        int i,j;
        int local_sense = 0;
        volatile int dummy = 0;
        volatile int rand_v = 0;
        unsigned int input_array[6];

        tm_bind_to_cabinet(myid+1);
        Barrier(&local_sense, myid, num_threads);
        set_transaction_registers(myid);

        for (i = 0 ; i < num_ops; i++){
          
          for (j = 0; j < 8; j++)
            dummy += random() % num_ops;
            
          BEGIN_CLOSED_TRANSACTION(1)         
            
            BEGIN_ESCAPE
            rand_v = random() % 10;
            open_counter.element++;
            register_compensating_action(&compensating_action, 0, input_array);
            END_ESCAPE
              
            /*
            BEGIN_OPEN_TRANSACTION(1)
              open_counter.element++;
            COMMIT_OPEN_TRANSACTION(1)  
            */

            if (rand_v == 0) 
              ABORT_TRANSACTION(0)

            closed_counter.element++;
          
          COMMIT_CLOSED_TRANSACTION(1);         
          
       }
}          
                 
void compensating_action(){
  
  /* 
  BEGIN_OPEN_TRANSACTION(2)
  open_counter.element--;
  COMMIT_OPEN_TRANSACTION(2)
  */
  BEGIN_ESCAPE
  open_counter.element--;
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
    open_counter.element = 0;
    
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
        
    if (closed_counter.element != open_counter.element){
      printf("COMPENSATION FAILED: CLOSED COUNTER: %d OPEN_COUNTER: %d\n", closed_counter.element, open_counter.element);
      LOGTM_ASSERT(0);
    }
    else {
      printf("COMPENSATION SUCCESSFULL\n");
    }
        
    SIMICS_BREAK_EXECUTION;

		EndTime = gethrtime();
		
		printf("Time = %lld nanoseconds\n", EndTime - StartTime);
		
		return 0;
}		

												
								
							
