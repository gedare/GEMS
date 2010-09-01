#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <transaction.h>
#include <sys/processor.h>
#include <assert.h>
#include <sched.h>

int *work_space;
int *nest_space;
int *run_start;
int *run_end;

int array_length;
int num_threads;
int num_processors;
int nest_depth;

pthread_attr_t pthread_custom_attr;

void* slaveStart(void *id){
		int myid;
		long int r,p;
		int i,j,k;
    int success = 1;
    int a = array_length; 
    int t = random() % 2;
    int local_sense = 0; 

		myid = *((int*)id);

    tm_bind_to_cabinet(myid+1);
    Barrier_breaking(&local_sense, myid, num_threads);
    set_transaction_registers(myid);    

    //printf("Thread %d scanning %d\n", myid, t);
    begin_transaction(0);
        
    for (k = 0; k < nest_depth ; k++){
      begin_transaction(k+1);
      if (t == 0){
        for (i = run_start[myid] ; i <= run_end[myid]; i = i + 16){
          if (k > 0){
            if (work_space[(i+k)%a] != myid || nest_space[(i+k)%a] != (k-1)){
               success = 0;
               SIMICS_ASSERT(0);
               break;
            }   
          }                       
          work_space[(i+k)%a] = myid;
          nest_space[(i+k)%a] = k;
          work_space[(i+k+1)%a] = myid;
          nest_space[(i+k+1)%a] = k;
        }
      } else {
        for (i = run_end[myid] ; i >= run_start[myid]; i = i - 16){
          if (k > 0){
            if (work_space[(i+k)%a] != myid || nest_space[(i+k)%a] != (k-1)){
               success = 0;
               SIMICS_ASSERT(0);
               break;
            }   
          }                       
          work_space[(i+k)%a] = myid;
          nest_space[(i+k)%a] = k;
          work_space[(i+k+1)%a] = myid;
          nest_space[(i+k+1)%a] = k;
        }
     }
    }    
                                            
    for (k = nest_depth-1 ; k >= 0; k--){
      for (i = run_start[myid] ; i <= run_end[myid]; i = i + 16){
        if(work_space[(i+k)%a] != myid || nest_space[(i+k)%a] != k){
          SIMICS_ASSERT(0);
          success = 0;
        }       
      } 
      end_transaction(k+1);
    } 
    end_transaction(0);
                
    if (success) {
      printf("%d Transaction success\n",myid);
    } else {
      printf("%d Transaction failed\n", myid);
    }
}		

int main(int argc, char **argv){
		pthread_t *threads;
		pthread_attr_t pthread_custom_attr;
		int	*ids;
    int i,j;
    int *rand_perm;
        
		if (argc != 4)
		{
			printf ("Usage: %s p n l\n  where p is no. of processors, n array length (multiple of 16), l nesting depth\n",argv[0]);
			exit(1);
		}

		num_processors = atoi(argv[1]);
		array_length = atoi(argv[2]);
    nest_depth = atoi(argv[3]);
    assert(nest_depth < 16);
		srandom(array_length);		
    num_threads = 1 * num_processors;
        
    work_space = (int*)calloc(array_length, sizeof(int));
    nest_space = (int*)calloc(array_length, sizeof(int));
    run_start = (int*)calloc(num_threads, sizeof(int));
    run_end = (int*)calloc(num_threads, sizeof(int));
    //printf("work_space: %x nest_space: %x\n", work_space, nest_space);
        
    for (i = 0; i < array_length; i++){
      work_space[i] = -1;
      nest_space[i] = -1;
    }  
                   
    rand_perm = (int*) calloc(num_threads, sizeof(int));
    for (i = 0; i < num_threads; i++){
        rand_perm[i] = i;
        int done = 0;    
        int k = random() % num_threads;
        while (!done){
            for (j = 0 ; j < i; j++)
              if (rand_perm[j] == k){
                done = 0;
                break;
              }
            if (j == i){
               done = 1;
               rand_perm[i] = k;
            } else {
               k = random() % num_threads;
            }
        }
    }                   
        
    for (i = 0; i < num_threads; i++){
         run_start[i] = rand_perm[i];
         run_end[i] = array_length - 16 + run_start[i];    
         printf("%d: start: %d end: %d\n", i, run_start[i], run_end[i]); 
		}

    init_transaction_state(num_threads);

		threads=(pthread_t *)malloc(num_threads*sizeof(pthread_t *));
		ids = (int*)calloc(num_threads, sizeof(int));
		pthread_attr_init(&pthread_custom_attr);
		for (i=0; i<num_threads-1; i++)
		{
			ids[i] = i;	
			pthread_create(&threads[i], &pthread_custom_attr, slaveStart, (void *)(ids+i));
		}
    ids[i] = i;
    slaveStart(ids+i);

		/* Synchronize the completion of each thread. */
		for (i=0; i<num_threads-1; i++)
		{
			pthread_join(threads[i],NULL);
		}
		END_WORKLOAD_TRANSACTION

		return 0;

}		

												
								
							
