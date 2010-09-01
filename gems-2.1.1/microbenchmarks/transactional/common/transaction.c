#include "transaction.h" 
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/pset.h>
#include <stdlib.h>
#include <signal.h>

volatile int pad1[64];
thread_transContext_t **threadTransContext;

volatile int sense = 0;
volatile int count = 0;

pthread_mutex_t bar_lock;
pthread_mutex_t global_lock;

int   nesting_depth[MAX_THREADS];

summary_signature_conflict_handlers_t *summary_signature_conflict_handlers;
volatile int pad2[64];

void seg_func(){
  LOGTM_ASSERT(0);
  exit(1);
}  

void Barrier_init(){
    sense = 0;
    count = 0;
    pthread_mutex_init(&bar_lock, NULL);
}           

void init_nesting_depth(){

       int i;
       for ( i = 0; i < MAX_THREADS; i++){
               nesting_depth[i] = 0;
       }
       pthread_mutex_init(&global_lock, NULL);
}

void Barrier_breaking(int* local_sense,int id, int num_thr) {
   volatile int ret;
   
   if ((*local_sense) == 0) 
		  (*local_sense) = 1;
	else
		  (*local_sense) = 0;
	
    pthread_mutex_lock(&bar_lock);
    count++;
    ret = (count == num_thr);
    pthread_mutex_unlock(&bar_lock);
    SIMICS_BEGIN_BARRIER;
       
    if(ret) {
      SIMICS_BREAK_EXECUTION
	    count = 0;
    	sense = (*local_sense);
    } else {
	  while (sense != (*local_sense)){
        //usleep(1);     // For non-simulator runs
       }  
    }
    SIMICS_END_BARRIER;
}

void Barrier(int *local_sense, int id, int num_thr) {
  Barrier_breaking(local_sense, id, num_thr);
}          

void Barrier_non_breaking(int* local_sense,int id, int num_thr) {
   volatile int ret;
   
   if ((*local_sense) == 0) 
		  (*local_sense) = 1;
	else
		  (*local_sense) = 0;
	
    pthread_mutex_lock(&bar_lock);
    count++;
    ret = (count == num_thr);
    pthread_mutex_unlock(&bar_lock);
    SIMICS_BEGIN_BARRIER;
       
    if(ret) {
	    count = 0;
    	sense = (*local_sense);
    } else {
	  while (sense != (*local_sense)){
        //usleep(1);     // For non-simulator runs
       }  
    }
    SIMICS_END_BARRIER;
}

/*
 * cabinet_nr = 1, 2, ...
 */
void tm_bind_to_cabinet(int cabinet_nr)
{
	  printf("binding to cabinet %d\n", cabinet_nr); 
	  if (pset_bind(cabinet_nr, P_LWPID, P_MYID, NULL) == -1) {
	    perror("pset_bind");
	    // exit(2);
	  }
}

/*****************************************************************************/ 

void compensation_handler(){
        volatile int dummy = 1;
        while(dummy){
          EXECUTE_COMPENSATING_ACTION
        }
}

void commit_handler(){
        volatile int dummy = 1;
        while(dummy){
          EXECUTE_COMMIT_ACTION
        }
}
          

void register_compensation_handler(){
    void *address = &compensation_handler;    
          asm volatile (                                           
			    "mov %0,%%g2"								
				::"r"(address)
                :"%g2"
				);                                            
    REGISTER_COMPENSATION_ACTION_HANDLER        
}        

void register_commit_handler(){
    void *address = &commit_handler;    
          asm volatile (                                           
			    "mov %0,%%g2"								
				::"r"(address)
                :"%g2"
				);                                            
    REGISTER_COMMIT_ACTION_HANDLER        
}        

void register_compensating_action(void *address, int num_inputs, unsigned int *input_array){
          asm volatile (                                           
			    "mov %0,%%g2\n"								
			    "mov %1,%%g3\n"
			    "mov %2,%%o0\n"
			    "mov %3,%%o1\n"
			    "mov %4,%%o2\n"
			    "mov %5,%%o3\n"
			    "mov %6,%%o4\n"
			    "mov %7,%%o5\n"
				::"r"(address), "r"(num_inputs), "r"(input_array[0]), "r"(input_array[1]), "r"(input_array[2]), "r"(input_array[3]), "r"(input_array[4]), "r"(input_array[5]) 						
                :"%g2", "%g3", "%o0", "%o1", "%o2", "%o3", "%o4", "%o5" 
				);                                            
    REGISTER_COMPENSATING_ACTION        
}        

void register_commit_action(void *address, int num_inputs, unsigned int* input_array){
          asm volatile (                                           
			    "mov %0,%%g2\n"								
			    "mov %1,%%g3\n"								
			    "mov %2,%%o0\n"
			    "mov %3,%%o1\n"
			    "mov %4,%%o2\n"
			    "mov %5,%%o3\n"
			    "mov %6,%%o4\n"
			    "mov %7,%%o5\n"
				::"r"(address), "r"(num_inputs), "r"(input_array[0]), "r"(input_array[1]), "r"(input_array[2]), "r"(input_array[3]), "r"(input_array[4]), "r"(input_array[5]) 						
                :"%g2", "%g3", "%o0", "%o1", "%o2", "%o3", "%o4", "%o5" 
				);                                            
    REGISTER_COMMIT_ACTION        
}

void* xmalloc(unsigned long size){
  int log_ptr;
     asm volatile (
          "mov %0,%%o0\n"
        ::"r"(size)
                :"%o0"
        );
  XMALLOC
     asm volatile (
          "mov %%o0, %0\n"
        :"=r"(log_ptr)
        );
  return (void *) log_ptr;
}
    
void tm_unroll_log(int, int);

unsigned int power_of_2(unsigned int a){
  unsigned int val;
  val = 1;
  val <<= a;
  return val;
}

unsigned int compute_backoff(unsigned int num_retries, int threadID){
    unsigned int backoff = 0;
    unsigned int max_backoff; 
    
     if (num_retries > 16)
       max_backoff = 64 * 1024 + (num_retries - 16);  
     else
      max_backoff = power_of_2(num_retries);            
    
    backoff = max_backoff;
    
    return backoff; 
}

void randomized_backoff(unsigned int num_retries, int threadID){
  volatile int a[32];
  volatile int b;
  int j;
  int backoff = (unsigned int) (CONG) % compute_backoff(num_retries, threadID);
  for (j = 0; j < backoff; j++){
      b += a[j % 32];    
  }                   

}  

void delay(unsigned int period){
  volatile int a[32];
  volatile int b;
  int j;
  for (j = 0; j < period; j++){
      b += a[j % 32];    
  }                   

}  

void tm_unroll_log_entry(unsigned int* entry){
  int k;
  
  unsigned int *address = (unsigned int *) (*(entry+16) & ADDRESS_MASK);
  
  for (k = 0; k < 16; k++){
    unsigned int data = *(entry + k);
    *address = data;
    address++;
  }                   
}

void tm_execute_compensation(unsigned int *entry){
      int i;  
      unsigned int input_vector[6];  
      unsigned int pc = *(entry - 2);
      unsigned int num_inputs = *(entry - 3);
      for (i = 0; i < num_inputs; i++){
        input_vector[i] = *(entry - 4 - i);
      }
      asm volatile(   \
              "mov %1, %%o0\n" \
              "call %0\n"        \
              "mov %2, %%o1\n" \
              ::"r"(pc),"r"(num_inputs),"r"(input_vector)
              :"%o0","%o1","%o7"
              );

}

void tm_execute_commit_action(unsigned int *entry){
      int i;  
      unsigned int input_vector[6];  
      unsigned int pc = *(entry - 2);
      unsigned int num_inputs = *(entry - 3);
      for (i = 0; i < num_inputs; i++){
        input_vector[i] = *(entry - 4 - i);
      }
      asm volatile(   \
              "mov %1, %%o0\n" \
              "call %0\n"        \
              "mov %2, %%o1\n" \
              ::"r"(pc),"r"(num_inputs),"r"(input_vector)
              :"%o0","%o1","%o7"
              );
}

void tm_execute_commit_actions(int threadID){
    unsigned int *next_entry = (unsigned int*) threadTransContext[threadID]->trap_address;
    unsigned int *curr_entry; 
    
    do {
        curr_entry = next_entry;     
        tm_execute_commit_action(curr_entry + 17);
        next_entry = (unsigned int*)(*curr_entry);      
    } while (next_entry != curr_entry);
}    

void tm_release_isolation(int xact_level){
          asm volatile (                                           
			    "mov %0,%%g2"								
				::"r"(xact_level)						
                :"%g2"
				);                                            
          RELEASE_ISOLATION
}
        
void tm_execute_summary_signature_conflict_handlers(threadID) {
	int i;
	for (i = summary_signature_conflict_handlers->num_handlers-1; i >= 0; i--) {
		summary_signature_conflict_handlers->handlers_array[i](threadID);
	}
}

void tm_register_summary_signature_conflict_handler(void (*handler)(int)) {
	assert(summary_signature_conflict_handlers->num_handlers < MAX_NUM_SUMMARY_SIGNATURE_CONFLICT_HANDLERS);
	summary_signature_conflict_handlers->handlers_array[summary_signature_conflict_handlers->num_handlers] = handler; 
	summary_signature_conflict_handlers->num_handlers++;
}

void tm_init_summary_signature_conflict_handlers() {
	summary_signature_conflict_handlers = (summary_signature_conflict_handlers_t*) malloc(sizeof(summary_signature_conflict_handlers_t));
	summary_signature_conflict_handlers->num_handlers = 0;
}
	 
void tm_log_unroll(int threadID){                  
       unsigned int *log_base = threadTransContext[threadID]->transaction_log;
       unsigned int log_size = threadTransContext[threadID]->transaction_log_size;
       unsigned int *log_ptr  = log_base + (log_size / 4);
       unsigned int  p = threadTransContext[threadID]->num_retries;     
       unsigned int xact_level = threadTransContext[threadID]->transactionLevel;
       unsigned int lowest_conflict_level = threadTransContext[threadID]->lowestConflictLevel;
       unsigned int *new_frame_pointer = 0x0;
       int alloc_size;
    
       while (log_size > 0){
       //while (xact_level >= lowest_conflict_level){                  // PARTIAL ROLLBACK
            unsigned int entry_type = (*(log_ptr - 1)) & ENTRY_MASK;
            switch (entry_type){
              case 0x00000004:  // LOG UNROLL ENTRY
                            tm_unroll_log_entry(log_ptr - 17);        
                            log_ptr -= 17;
                            log_size -= 68;
                            break;
              case 0x00000005:  // COMPENSATION ENTRY
                            tm_execute_compensation(log_ptr);
                            log_ptr -= 17;
                            log_size -= 68;
                            break; 
              case 0x00000006:  // COMMIT ENTRY
                            log_ptr -= 17;
                            log_size -= 68;
                            break;
              case 0x00000007:  // XMALLOC ENTRY
            								alloc_size = *(log_ptr - 2);
                            log_ptr -= (17 + alloc_size/4);
                            log_size -= (68 + alloc_size);
                            break;
              case 0x00000001:  // ACTIVE TRANSACTION HEADER
                            tm_release_isolation(xact_level);
                            log_ptr -= 17;
                            log_size -= 68;
                            xact_level--;
                            new_frame_pointer = (unsigned int*) *log_ptr;
                            break;
              case 0x00000002:  // GARBAGE TRANSACTION HEADER
              case 0x00000003:  // REGISTER CHECKPOINT
                            log_ptr -= 17;
                            log_size -= 68;
                            break;
              default:
                            SIMICS_ASSERT(0);
                            break;
            }                   
       }                       
       
       if (p > 0)     
        randomized_backoff(p, threadID);
       
       threadTransContext[threadID]->transaction_frame_pointer = (unsigned int) new_frame_pointer;
       threadTransContext[threadID]->transactionLevel = xact_level;
       threadTransContext[threadID]->transaction_log_size = log_size;
}       
        
void tm_trap_handler(int threadID){

       int j;
       int p;
       volatile int dummy;
       int occupy_stack[2048];

       
       int num_retries = threadTransContext[threadID]->num_retries; 
       if (num_retries == -1){
        /* Dummy call to get the trap handler code mapped in. 
           Dont want page faults  */       
        return;        
       } 
        
       unsigned int trap_type = threadTransContext[threadID]->trap_type;
       switch (trap_type){
          case 1: // Unroll log
             tm_log_unroll(threadID);                  
             asm volatile(   \
                "mov 1, %%g3\n" \
                ::"r"(p)
                :"%g3"
              );   
              break;
          case 2: // TLB miss
              dummy = *(int *)(threadTransContext[threadID]->trap_address);
              asm volatile(   \
                "mov 0, %%g3\n" \
                ::"r"(p)
                :"%g3"
              );   
              break;
          case 3: // Commit Actions 
              tm_execute_commit_actions(threadID);
              asm volatile(   \
                "mov 0, %%g3\n" \
                ::"r"(p)
                :"%g3"
              );   
              break;
          case 4: // Summary Signature Conflict 
							tm_execute_summary_signature_conflict_handlers(threadID);	 
              asm volatile(   \
                "mov 0, %%g3\n" \
                ::"r"(p)
                :"%g3"
              );   
              break;
          case 5: // Pending interrupt
              asm volatile(   \
                "mov 0, %%g3\n" \
                ::"r"(p)
                :"%g3"
              );   
              break;
          default:
              break;
       }
}        

void transaction_manager_stub(int dummy){

  int restart;
  // DUMMY CALL to avoid TLB miss
  if(dummy){
    return;
  }
        BEGIN_ESCAPE
        asm volatile(   \
              "call %1\n"        \
              "mov %%g2, %%O0\n" \
              "mov %%g3, %0\n"   \
              :"=r"(restart)  
              :"r"(&tm_trap_handler) 
              :"%o0", "%o7"
              );
        
        if (restart){
          END_ESCAPE       
          HANDLER_RESTART
        } else {
          END_ESCAPE       
          HANDLER_CONTINUE
        } 
}

void touch_log(int threadid){
  threadTransContext[threadid]->transaction_log[0] = 0;
}                             

void walk_log(int threadid){
  int i;
  for (i = 0; i < MAX_LOG_SIZE; i+=1024)
    threadTransContext[threadid]->transaction_log[i] = 0;
}  


void init_log(int log_size, int num_threads){
        int i,j;
        
        threadTransContext = (thread_transContext_t**) calloc(num_threads, sizeof(thread_transContext_t*));        
        for (i = 0 ; i < num_threads; i++){
          threadTransContext[i] = (thread_transContext_t*) memalign ( sysconf(_SC_PAGESIZE) * 2, sizeof(thread_transContext_t));
          threadTransContext[i]->pad1[0] = 0;
          threadTransContext[i]->transaction_frame_pointer = (unsigned int) threadTransContext[i]->transaction_log;
          threadTransContext[i]->transaction_log_size= 0;
          walk_log(i);
          //for(j=0; j < MAX_LOG_SIZE; ++j){
          //  threadTransContext[i]->transaction_log[j] = 0;
          //}
          //printf("init log: %d %x %x\n", i, threadTransContext[i], threadTransContext[i]->transaction_log);
        }
        
}          

void init_g_rand(){g_rand.z=362436069; g_rand.w=521288629; g_rand.jsr=123456789; g_rand.jcong=380116160;}

void init_transaction_logs(int num_threads){
    int i;
    init_g_rand();
    init_log(MAX_LOG_SIZE, num_threads);           // allocate log and write dummy log entries. Avoid page faults later on.

}

void init_transaction_state(int num_threads){
  init_transaction_logs(num_threads);
  Barrier_init();
  init_nesting_depth();
  signal(SIGSEGV, &seg_func);
}  
          
void set_log_base(int threadid){
          delay(1024);
        printf("Setting log: %d %x %x\n", threadid, threadTransContext[threadid]->pad1, threadTransContext[threadid]->transaction_log);
        /* DUMMY CALL */
        threadTransContext[threadid]->num_retries = -1;
        tm_trap_handler(threadid);
        transaction_manager_stub(1);

        threadTransContext[threadid]->threadID = threadid;
        threadTransContext[threadid]->num_retries = 0;
        threadTransContext[threadid]->transaction_log[0] = 0;
        asm volatile (   \
			    "mov %0,%%g3\n" \
			    "mov %1,%%g4\n" \
			    "mov %2,%%g5\n" \
				::"r"(threadTransContext[threadid]->pad1), "r"(threadTransContext[threadid]->transaction_log), "i"(MAX_LOG_SIZE * sizeof(unsigned int)/ 1024)						
                :"%g3", "%g4", "%g5"
				);                                            
        SET_LOG_BASE
}

void set_handler_address(){
          printf("Setting handler address %x\n", &transaction_manager_stub+16);
          delay(1024);
          /* DUMMY CALL */
          threadTransContext[0]->num_retries = -1;
          tm_trap_handler(0);
          transaction_manager_stub(1);

          void *handler_address = &transaction_manager_stub + 16;

          asm volatile (                                           
			    "mov %0,%%g2"								
				::"r"(handler_address)						
                :"%g2"
				);                                           
					REGISTER_THREAD_WITH_HYPERVISOR 
          SET_HANDLER_ADDRESS
}     

void set_transaction_registers(int threadid){  
          delay(1024);
          void *handler_address = &transaction_manager_stub + 16;
          threadTransContext[threadid]->threadID = threadid;
          threadTransContext[threadid]->num_retries = -1;
          threadTransContext[threadid]->transaction_log[0] = 0;
          /* DUMMY CALLS */
          tm_trap_handler(threadid);
          transaction_manager_stub(1);

          asm volatile (                                           
			    "mov %0,%%g2"								
				::"r"(handler_address)						
                :"%g2"
				);                                            
          SET_HANDLER_ADDRESS
          asm volatile (   \
			    "mov %0,%%g3\n" \
			    "mov %1,%%g4\n" \
			    "mov %2,%%g5\n" \
				::"r"(threadTransContext[threadid]->pad1), "r"(threadTransContext[threadid]->transaction_log), "i"(MAX_LOG_SIZE * sizeof(unsigned int)/ 1024)						
                :"%g3", "%g4", "%g5"
				);                                            
          SET_LOG_BASE
}          

void ruby_watch(volatile unsigned int address){
          asm volatile (                                           
			    "mov %0,%%g1"								
				::"r"(address)						
                :"%g1"
				);                                            
          RUBY_WATCH
}       

int get_thread_id(){
        //return 0;
        int thread_id;
        GET_THREAD_ID
        asm volatile(
              "mov %%g2, %0"
              :"=r"(thread_id)
              ::"%g2"
              );
        return thread_id;
}
        
int begin_transaction(int id){
		switch (id){
			case 0:	
				BEGIN_TRANSACTION(0)
				break;
			case 1:
				BEGIN_TRANSACTION(1)
				break;	
			case 2:	
				BEGIN_TRANSACTION(2)
				break;
			case 3:	
				BEGIN_TRANSACTION(3)
				break;
			case 4:	
				BEGIN_TRANSACTION(4)
				break;
			case 5:	
				BEGIN_TRANSACTION(5)
				break;
			case 6:	
				BEGIN_TRANSACTION(6)
				break;
			case 7:	
				BEGIN_TRANSACTION(7)
				break;
			case 8:	
				BEGIN_TRANSACTION(8)
				break;
			case 9:	
				BEGIN_TRANSACTION(9)
				break;
			case 10:	
				BEGIN_TRANSACTION(10)
				break;
			case 11:	
				BEGIN_TRANSACTION(11)
				break;
			case 12:	
				BEGIN_TRANSACTION(12)
				break;
			case 13:	
				BEGIN_TRANSACTION(13)
				break;
			case 14:	
				BEGIN_TRANSACTION(14)
				break;
			case 15:	
				BEGIN_TRANSACTION(15)
				break;
			case 16:	
				BEGIN_TRANSACTION(16)
				break;
			case 17:	
				BEGIN_TRANSACTION(17)
				break;
			case 18:	
				BEGIN_TRANSACTION(18)
				break;
			case 19:	
				BEGIN_TRANSACTION(19)
				break;
			default:	
				BEGIN_TRANSACTION(0)
				break;
		}		
		return 0;

}

int end_transaction(int id){

		switch (id){
			case 0:	
				COMMIT_TRANSACTION(0)
				break;
			case 1:
				COMMIT_TRANSACTION(1)
				break;	
			case 2:	
				COMMIT_TRANSACTION(2)
				break;
			case 3:	
				COMMIT_TRANSACTION(3)
				break;
			case 4:	
				COMMIT_TRANSACTION(4)
				break;
			case 5:	
				COMMIT_TRANSACTION(5)
				break;
			case 6:	
				COMMIT_TRANSACTION(6)
				break;
			case 7:	
				COMMIT_TRANSACTION(7)
				break;
			case 8:	
				COMMIT_TRANSACTION(8)
				break;
			case 9:	
				COMMIT_TRANSACTION(9)
				break;
			case 10:	
				COMMIT_TRANSACTION(10)
				break;
			case 11:	
				COMMIT_TRANSACTION(11)
				break;
			case 12:	
				COMMIT_TRANSACTION(12)
				break;
			case 13:	
				COMMIT_TRANSACTION(13)
				break;
			case 14:	
				COMMIT_TRANSACTION(14)
				break;
			case 15:	
				COMMIT_TRANSACTION(15)
				break;
			case 16:	
				COMMIT_TRANSACTION(16)
				break;
			case 17:	
				COMMIT_TRANSACTION(17)
				break;
			case 18:	
				COMMIT_TRANSACTION(18)
				break;
			case 19:	
				COMMIT_TRANSACTION(19)
				break;
			default:	
				COMMIT_TRANSACTION(0)
				break;
		}		
		return 0;
}		

long xact_rand(){
  long retval;
  BEGIN_ESCAPE;
  retval = rand();
  END_ESCAPE;
  return retval;
}
