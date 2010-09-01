#ifndef TRANSACTION_H_
#define TRANSACTION_H_

#include <pthread.h>
#include <assert.h>

#define NEW_RUBY_MAGIC_CALL( service )                      \
  __asm__ __volatile__                                      \
    ( "sethi %1, %%g0  !magic service %2\n\t"               \
          : /* no outputs */                                \
      : "r" (0), "i" (service), "i" (service)         \
          : "l0", "memory" /* clobber register */                     \
        );                                                        
// This allows open id's of 0-1 and closed id's of 0-17

#define BEGIN_CLOSED_TRANSACTION(id)  NEW_RUBY_MAGIC_CALL(id + 1024)
#define COMMIT_CLOSED_TRANSACTION(id) NEW_RUBY_MAGIC_CALL(id + 2048)
#define BEGIN_OPEN_TRANSACTION(id)    NEW_RUBY_MAGIC_CALL(id + 3072)
#define COMMIT_OPEN_TRANSACTION(id)   NEW_RUBY_MAGIC_CALL(id + 4096)

#define SYSTEM_CALL(id)               NEW_RUBY_MAGIC_CALL(id + 5120)
#define ABORT_TRANSACTION(id)         NEW_RUBY_MAGIC_CALL(id + 6144)

#define BEGIN_WORKLOAD_TRANSACTION    NEW_RUBY_MAGIC_CALL(3)
#define END_WORKLOAD_TRANSACTION      NEW_RUBY_MAGIC_CALL(5)

#define BEGIN_EXPOSED NEW_RUBY_MAGIC_CALL(6)
#define END_EXPOSED NEW_RUBY_MAGIC_CALL(7)

#define BEGIN_ESCAPE NEW_RUBY_MAGIC_CALL(6)
#define END_ESCAPE NEW_RUBY_MAGIC_CALL(7)

#define SET_LOG_BASE NEW_RUBY_MAGIC_CALL(8)
#define SET_HANDLER_ADDRESS NEW_RUBY_MAGIC_CALL(9)  
#define RELEASE_ISOLATION NEW_RUBY_MAGIC_CALL(10)
#define HANDLER_RESTART NEW_RUBY_MAGIC_CALL(11)
#define HANDLER_CONTINUE NEW_RUBY_MAGIC_CALL(28)

#ifdef SIMICS
#define SIMICS_ASSERT(X) if (!(X)) { NEW_RUBY_MAGIC_CALL(15);}
#else
#define SIMICS_ASSERT(X) assert(X)
#endif

#define LOGTM_ASSERT(X) if (!(X)) { NEW_RUBY_MAGIC_CALL(15) }

#define SIMICS_BREAK_EXECUTION NEW_RUBY_MAGIC_CALL(0x40000)

#define RUBY_WATCH NEW_RUBY_MAGIC_CALL(19)

#define REGISTER_COMMIT_ACTION_HANDLER NEW_RUBY_MAGIC_CALL(21)
#define REGISTER_COMPENSATION_ACTION_HANDLER NEW_RUBY_MAGIC_CALL(22)
#define REGISTER_COMPENSATING_ACTION NEW_RUBY_MAGIC_CALL(23)
#define REGISTER_COMMIT_ACTION NEW_RUBY_MAGIC_CALL(24)
#define EXECUTE_COMPENSATING_ACTION NEW_RUBY_MAGIC_CALL(25)
#define EXECUTE_COMMIT_ACTION NEW_RUBY_MAGIC_CALL(26)

#define XMALLOC NEW_RUBY_MAGIC_CALL(27)
#define SIMICS_BEGIN_BARRIER NEW_RUBY_MAGIC_CALL(29)
#define SIMICS_END_BARRIER NEW_RUBY_MAGIC_CALL(30)

#define BEGIN_TIMER NEW_RUBY_MAGIC_CALL(31)
#define END_TIMER NEW_RUBY_MAGIC_CALL(32)

//#define CHECK_REGS NEW_RUBY_MAGIC_CALL(31)

#define REGISTER_THREAD_WITH_HYPERVISOR NEW_RUBY_MAGIC_CALL(40)

#define SUMMARY_SIGNATURE_SET          NEW_RUBY_MAGIC_CALL(50)
#define SUMMARY_SIGNATURE_UNSET        NEW_RUBY_MAGIC_CALL(51)
#define SUMMARY_SIGNATURE_ADD_ADDR     NEW_RUBY_MAGIC_CALL(52)
#define SUMMARY_SIGNATURE_REMOVE_ADDR  NEW_RUBY_MAGIC_CALL(53)
#define SUMMARY_SIGNATURE_GET_INDEX    NEW_RUBY_MAGIC_CALL(54)
#define SUMMARY_SIGNATURE_IGNORE       NEW_RUBY_MAGIC_CALL(55)

#define SET_RESTORE_PC       NEW_RUBY_MAGIC_CALL(60)
#define GET_LOG_SIZE         NEW_RUBY_MAGIC_CALL(61)
#define GET_THREAD_ID        NEW_RUBY_MAGIC_CALL(62)

#define MAX_THREADS 128
#define MAX_LOG_SIZE 200000

#define CACHE_BLOCK_SIZE 64
#define ADDRESS_MASK 0xffffffc0       // Last 6 bits give the entry type
#define ENTRY_MASK   0x0000003f

#define MAX_NUM_SUMMARY_SIGNATURE_CONFLICT_HANDLERS 16

#ifndef GLOBAL_LOCK
#define BEGIN_TRANSACTION(id) BEGIN_CLOSED_TRANSACTION(id)
#else
#define BEGIN_TRANSACTION(id) \
{                             \
 int myid = pthread_self();  \
 if (nesting_depth[myid] == 0){  \
    pthread_mutex_lock(&global_lock);   \
    nesting_depth[myid]++;       \
 } else {                        \
   nesting_depth[myid]++;        \
 }                               \
}
#endif

#ifndef GLOBAL_LOCK
#define COMMIT_TRANSACTION(id) COMMIT_CLOSED_TRANSACTION(id)
#else
#define COMMIT_TRANSACTION(id)  \
{                             \
 int myid = pthread_self();  \
 nesting_depth[myid]--;      \
 if (nesting_depth[myid] == 0)  \
    pthread_mutex_unlock(&global_lock);   \
}
#endif

//WARNING: Modifications to this structure affects functionality of trap transfer
//code in ruby. See ruby/log_tm/TransactionManager.C trapToHandler()
typedef struct{
        unsigned int pad1[1024];
        unsigned int lowestConflictLevel;
        unsigned int transaction_frame_pointer;
        unsigned int transactionLevel;
        unsigned int threadID;
        unsigned int transaction_log_size;
        unsigned int num_retries;
        unsigned int trap_address;
        unsigned int trap_type;
        unsigned int transaction_log[MAX_LOG_SIZE];
        unsigned int pad2[1024];
}thread_transContext_t;               

typedef struct summary_signature_conflict_handlers_s {
	int num_handlers;
	void (*handlers_array[MAX_NUM_SUMMARY_SIGNATURE_CONFLICT_HANDLERS])(int);
} summary_signature_conflict_handlers_t;

/******************************************************************************

Organization of Transaction Log 

Types of Entries -
1) Active Transaction Header
2) Garbage Transaction Header
3) Register Checkpoint
4) Log Unroll entry
5) Compensation entry
6) Commit entry
7) Xmalloc entry

Active Transaction Header: 
  Size - 68 bytes
  ***************************************************************
  *   PARENT_FP   *           XACT_DATA     * XXXXXX  * 00 0001 *
  ***************************************************************
  < ---- 4B -----> <---------- N * 4B -----> <--26b--> <-- 6b -->
Garbage Transaction Header: Similar to Active Transaction Header except for type field
Register Checkpoint: 
Log Unroll entry:
  Size - 68 bytes
  ***************************************************************
  *              OLD DATA             * BLOCK ADDRESS * 00 0100 *
  ***************************************************************
  < ------------ 64B ----------------> <--- 26b -----> <-- 6b -->
Compensation entry:
  Size - 68x bytes
  ***************************************************************
  * PAD *  INPUT PARAMS   *  NUM_INP  *  PC    * XXXX * 00 0101 *
  ***************************************************************
        <- NUM_INP * 4B --><-- 4B ---><-- 4B -->< 26b ><-- 6b --> 
Commit entry:
  Size - 68x bytes
  ***************************************************************
  * NEXT * PAD  * INP PARAMS * NUM_INP  *  PC  * XXXX * 00 0110 *
  ***************************************************************
  <- 4B ->      <- N_I * 4B ><-- 4B ---><- 4B ->< 26b ><-- 6b --> 
xmalloc entry:
  Size - 68 bytes
  ***************************************************************
  *             PAD             *  ALLOC_SIZE  * XXXX * 00 0111 *
  ***************************************************************
  <----------   60B   ---------><----- 4B ----->< 26b ><-- 6b --> 
******************************************************************************/


extern thread_transContext_t **threadTransContext;
extern pthread_mutex_t        global_lock;
extern int                    nesting_depth[MAX_THREADS];

void init_transaction_state(int num_threads);
void init_transaction_logs(int num_threads);
void set_log_base(int threadid);
void set_handler_address();
void ruby_watch(unsigned int address);
void seg_func();

/*********************http://www.ciphersbyritter.com***************************/ 

#define UL unsigned long


#define znew  ((g_rand.z=36969*(g_rand.z&65535)+(g_rand.z>>16))<<16)
#define wnew  ((g_rand.w=18000*(g_rand.w&65535)+(g_rand.w>>16))&65535)
#define MWC   (znew+wnew)
#define SHR3  (g_rand.jsr=(g_rand.jsr=(g_rand.jsr=g_rand.jsr^(g_rand.jsr<<17))^(g_rand.jsr>>13))^(g_rand.jsr<<5))
#define CONG  (g_rand.jcong=69069*g_rand.jcong+1234567)
#define KISS  ((MWC^CONG)+SHR3)
/*  Global static variables: */
static struct{
char pad[64];        
UL z, w, jsr, jcong;
char pad2[64];        
}g_rand;

long xact_rand();
		
#endif /* TRANSACTION_H_ */
