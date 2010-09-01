#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include "transaction.h"

typedef struct{
  unsigned int element;
  unsigned int pad[31];
}array;

struct{
        unsigned int      num_threads;
        unsigned int      num_ops;
        unsigned int      big_array_size;
        unsigned int      small_array_size;
        unsigned int      transaction_size;
        unsigned int      access_array[3];
        double            access_distribution[3];
        double            read_percentage[3];
        double            duty_cycle;
        unsigned int pad[1024];
        array             *small_array;
        unsigned int pad2[1024];
        array             *big_array;
        unsigned int pad3[1024];
        array             *dummy_array;
}gm;
        
void init_arrays(){
		
		int i;
        double sum = 0;
		for(i = 0; i < 3; i++){
          sum += gm.access_distribution[i];
        }
        if (sum != 1.0){
          printf("ERROR! SUM OF DISTRIBUTIONS NOT EQUAL TO 1.0\n");
          SIMICS_ASSERT(0);
        }
        for (i = 0; i < 3; i++){
          if (!((gm.read_percentage[i] >= 0.0) & (gm.read_percentage[i] <= 1.0))){
            printf("ERROR! READ_PERCENTAGE[%d] NOT BETWEEN 0.0 AND 1.0: %lf \n", i, gm.read_percentage[i]);
            SIMICS_ASSERT(0);
          }
          if (gm.access_array[i] != 0 && gm.access_array[i] != 1){
            printf("ERROR! ACCESS_ARRAY[%d] NOT 0 or 1: %d\n", i, gm.access_array[i]);
            SIMICS_ASSERT(0);
          }          
        }                    
        SIMICS_ASSERT(gm.big_array_size > 0);
        SIMICS_ASSERT(gm.small_array_size > 0 && gm.small_array_size <= gm.big_array_size);
        SIMICS_ASSERT(gm.transaction_size > 0);                      
        SIMICS_ASSERT(gm.duty_cycle > 0.0 && gm.duty_cycle < 1.0);
        SIMICS_ASSERT(gm.num_ops > 0);

        gm.small_array   = (array*) calloc(gm.small_array_size, sizeof(array));
        gm.big_array     = (array*) calloc(gm.big_array_size, sizeof(array));
        gm.dummy_array   = (array*) calloc(2 * ((1 - gm.duty_cycle)/gm.duty_cycle) * gm.transaction_size, sizeof(array));

}	


void do_phase(int phase, int* random_array_index,double* random_array){
    int j;    
    for (j = 0; j < (gm.access_distribution[phase] * gm.transaction_size); j++){
             int access_type = (random_array[(*random_array_index)++] > gm.read_percentage[phase]);
             int access_index;
             *random_array_index = (*random_array_index >= 512) ? 0 : *random_array_index;
             switch(gm.access_array[phase]){
                case 0:
                        access_index = random_array[(*random_array_index)++] * gm.small_array_size;
                        if (access_type == 0){
                          volatile int dummy_read = gm.small_array[access_index].element;
                        } else {
                          gm.small_array[access_index].element = 0;
                        }
                        break;                  
                case 1:                
                        access_index = random_array[(*random_array_index)++] * gm.big_array_size;
                        if (access_type == 0){
                          volatile int dummy_read = gm.big_array[access_index].element;
                        } else {
                          gm.big_array[access_index].element = 0;
                        }
                        break;
                default:
                        SIMICS_ASSERT(0);
             }                                        
             *random_array_index = (*random_array_index >= 512) ? 0 : *random_array_index;
    }
}              

void* slaveStart(void *id){
		int myid;
		long int r,p;
		int i,j;
    int k;
    int local_sense = 0;
    volatile int internal_count = 0;
    double *random_array;
    int     random_array_index = 0;

    myid = *((int*)id);
    tm_bind_to_cabinet(myid + 1);
    
    random_array = (double*) calloc( 512, sizeof(double));
    for (i = 0 ; i < 512; i++){
      random_array[i] = (double) rand() / RAND_MAX;
    }

    Barrier_breaking(&local_sense, myid, gm.num_threads);
    set_transaction_registers(myid);

    /* GET RANDOM ARRAY INTO CACHE */
    for (i = 0 ; i < 512; i++){
      volatile int dummy_read = random_array[i];
    }
        
    for (i = 0 ; i < gm.num_ops; i++){
      BEGIN_WORKLOAD_TRANSACTION
      
      BEGIN_TRANSACTION(0);
      do_phase(0, &random_array_index, random_array); 
      do_phase(1, &random_array_index, random_array); 
      do_phase(2, &random_array_index, random_array); 

      COMMIT_TRANSACTION(0);

      for (j = 0; j < (random_array[random_array_index] * 2 * ((1 - gm.duty_cycle) / gm.duty_cycle) * gm.transaction_size); j++)
        gm.dummy_array[j].element = 0;        

      END_WORKLOAD_TRANSACTION
		}
    Barrier_breaking(&local_sense, myid, gm.num_threads);
}		
				
int main(int argc, char **argv){
		int n,i;
		pthread_t *threads;
    pthread_attr_t pthread_custom_attr;
		int	*ids;
		
    hrtime_t  StartTime;
		hrtime_t  EndTime;  

		if (argc != 16)
		{
			printf ("Usage: %s p n\n  where p # threads, n #ops\n",argv[0]);
			exit(1);
		}

        
		gm.num_threads = atoi(argv[1]);
		gm.num_ops = atoi(argv[2]) / gm.num_threads;
    gm.small_array_size = atoi(argv[3]);
    gm.big_array_size   = atoi(argv[4]);
    gm.transaction_size = atoi(argv[5]);
    gm.access_array[0]  = atoi(argv[6]);
    gm.access_array[1]  = atoi(argv[7]);
    gm.access_array[2]  = atoi(argv[8]);
    gm.access_distribution[0] = atof(argv[9]);
    gm.access_distribution[1] = atof(argv[10]);
    gm.access_distribution[2] = atof(argv[11]);
    gm.read_percentage[0]     = atof(argv[12]);
    gm.read_percentage[1]     = atof(argv[13]);
    gm.read_percentage[2]     = atof(argv[14]);
    gm.duty_cycle             = atof(argv[15]);
		srandom(gm.num_ops);		
		
		init_arrays();
    
    init_transaction_state(gm.num_threads);
      
		StartTime = gethrtime(); 	
    threads=(pthread_t *)malloc(gm.num_threads*sizeof(pthread_t));
		ids = (int*)calloc(gm.num_threads, sizeof(int));
		pthread_attr_init(&pthread_custom_attr);
		for (i=0; i< (gm.num_threads)  ; i++)
		{
			ids[i] = i;	
			pthread_create(&threads[i], &pthread_custom_attr, slaveStart, (void *)(ids+i));
		}

		for (i=0; i< (gm.num_threads) ; i++)
		{
			pthread_join(threads[i],NULL);
		}
		EndTime = gethrtime();
		
		printf("Time = %lld nanoseconds\n", EndTime - StartTime);
		
		return 0;

}		

												
								
							
