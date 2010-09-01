#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include "transaction.h"
#include <sys/processor.h>

#define LEFT_NULL	1 
#define RIGHT_NULL  2

#define SUCCESS 1
#define FAIL    0

#define MAX_DEQUE_SIZE 16384

#define QUEUE_WORK 8 
#define NON_QUEUE_WORK 64

struct{ 
  int non_queue_work;
  int num_ops;
  int deque_size;
  int num_threads;                             
  int empty_array[8];
  int pad[32];
  volatile int right_null_pos;
  int pad1[32];
  volatile int deque[MAX_DEQUE_SIZE+2];
  int pad2[32];
  volatile int left_null_pos;
  int pad3[32];
  volatile int global_op_count;
  int pad4[32];
}gm;  
  
pthread_attr_t pthread_custom_attr;

int do_junk(int duration){
    int bl = 0;
    int i;
    
    for(i = 0;i < duration; i++){
         int k = i % 8;   
         bl += gm.empty_array[k];
    }     
    //if (duration > 0)     
    //  usleep(random()%2);    
    return bl;
}     
    
/* Non concurrent operation */
void init_deque(){
		
		int i;
		
		gm.left_null_pos = 0;
		gm.right_null_pos = 1;
		
		gm.deque_size = 0;
		
		gm.deque[0] =  LEFT_NULL;
		for (i = 1; i < MAX_DEQUE_SIZE + 2; i++)
			gm.deque[i] = RIGHT_NULL;			
}	

int enqueue_right(int key){
		int k;
		/* Find index of leftmost RN */
    k = gm.right_null_pos;
		if (k == (MAX_DEQUE_SIZE + 1)) 
			return 0;	/* DEQUE FULL */
		gm.deque[k] = key;
		gm.right_null_pos = k + 1;
		return 1;
}											
						
int dequeue_right(){
		int k,curr;

    k = gm.right_null_pos;
		curr = gm.deque[k-1];

		if(curr == LEFT_NULL)
			return 0;	/* Empty */
		gm.deque[k-1] = RIGHT_NULL;
		gm.right_null_pos = k-1;
		return curr;
}

int enqueue_left(int key){
		int k;
		/* Find index of rightmost LN */
    k = gm.left_null_pos;
		if (k == 0) 
			return 0;	/* DEQUE FULL */
		gm.deque[k] = key;
		gm.left_null_pos = k - 1;
		return 1;
}											

int dequeue_left(){
		int k,curr;

    k = gm.left_null_pos;
		curr = gm.deque[k+1];

		if(curr == RIGHT_NULL)
			return 0;	/* Empty */
		gm.deque[k+1] = LEFT_NULL;
		gm.left_null_pos = k+1;
		return curr;
						
}

void print_deque(){

		int i;
		printf("Deque: ");
		for (i = 0; i < MAX_DEQUE_SIZE + 2; i++){
			printf("%x ",gm.deque[i]);
		}
		printf("\n");
}

void* slaveDeque(void *id){
		int myid;
		long int r,p;
		int i,j;
    int k;
    int local_sense = 0;
		
    myid = *((int*)id);
    
    tm_bind_to_cabinet(myid + 1);
    Barrier_breaking(&local_sense, myid, gm.num_threads);
    set_transaction_registers(myid);

		for (i = 0 ; i < gm.num_ops; i++){
			r = random() % 4;
      p = random() % QUEUE_WORK; 

      BEGIN_WORKLOAD_TRANSACTION
            
      BEGIN_TRANSACTION(0)
      BEGIN_TRANSACTION(1)
      gm.global_op_count++;
      COMMIT_TRANSACTION(1)
      BEGIN_TRANSACTION(2)
			switch (r)
			{
				case 0: 
					enqueue_right(8);
					break;
				case 1: 
					dequeue_right();
					break;
				case 2: 
					enqueue_left(8);	
					break;
				case 3: 
					dequeue_left();
					break;
			}
      do_junk(p);
      COMMIT_TRANSACTION(2)
      COMMIT_TRANSACTION(0)
      k = do_junk(random() % gm.non_queue_work);
      
      END_WORKLOAD_TRANSACTION
		}

    Barrier_breaking(&local_sense, myid, gm.num_threads);
}		
				
int main(int argc, char **argv){
		int n,i;
		pthread_t *threads;
		int	*ids;
		
    hrtime_t  StartTime;
		hrtime_t  EndTime;  

		if (argc != 4)
		{
			printf ("Usage: %s p n\n  where p # threads, n #ops\n",argv[0]);
			exit(1);
		}

		gm.num_threads = atoi(argv[1]);
		gm.num_ops = atoi(argv[2]) / gm.num_threads;
    gm.non_queue_work = atoi(argv[3]);
		srandom(gm.num_ops);		
		
		init_deque();
    gm.global_op_count = 0;
		for (i = 0 ; i < 128; i++)
			enqueue_right(8);

    init_transaction_state(gm.num_threads);    
		
    StartTime = gethrtime(); 	
    threads=(pthread_t *)malloc(gm.num_threads*sizeof(pthread_t));
		ids = (int*)calloc(gm.num_threads, sizeof(int));
		pthread_attr_init(&pthread_custom_attr);
		for (i=0; i < gm.num_threads; i++)
		{
			ids[i] = i;	
			pthread_create(&threads[i], &pthread_custom_attr, slaveDeque, (void *)(ids+i));
		}

		for (i=0; i< gm.num_threads; i++)
		{
			pthread_join(threads[i],NULL);
		}
		EndTime = gethrtime();
		
		printf("Time = %lld nanoseconds\n", EndTime - StartTime);
		
		//print_deque();

		return 0;

}		

												
								
							
