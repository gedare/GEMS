#include "spillpq.h"
#include "freelist.h"
#include "gabdebug.h"

static Chain_Control queues[10];
static size_t queue_size[10];
static Freelist_Control free_nodes[10];

/* split pq: the heap */
typedef struct {
  Chain_Node Node;
  uint32_t key;
  uint32_t val;
  uint32_t heap_index;
} pq_node;

static pq_node **the_heap[10]; // FIXME: need multiple heaps
static int heap_current_size[10];

/* heap implementation ... */
#define HEAP_PARENT(i) (i>>1)
#define HEAP_FIRST (1)
#define HEAP_LEFT(i) (i<<1)
#define HEAP_RIGHT(i) (HEAP_LEFT(i)+1)

#define HEAP_NODE_TO_KV(n) ((((uint64_t)n->key) << 32UL) | (uint64_t)n->val)
#define kv_value(kv) ((uint32_t)kv)
#define kv_key(kv)   (kv>>32)

static inline
int sparc64_splitheappq_heap_allocate( int qid, size_t max_pq_size )
{
  the_heap[qid] = _Workspace_Allocate(max_pq_size * sizeof(pq_node*));
  if ( ! the_heap[qid] ) {
    printk("Unable to allocate heap of size: \n",max_pq_size*sizeof(pq_node*));
    while (1);
  }
  heap_current_size[qid] = 0;

  return 0;
}

static inline
void swap_entries(int qid, int a, int b) {
  pq_node *tmp = the_heap[qid][a];
  int tmpIndex = the_heap[qid][a]->heap_index;
  the_heap[qid][a]->heap_index = the_heap[qid][b]->heap_index;
  the_heap[qid][b]->heap_index = tmpIndex;
  the_heap[qid][a] = the_heap[qid][b];
  the_heap[qid][b] = tmp;
}

static inline
void bubble_up( int qid, int i )
{
  while ( i > 1 && the_heap[qid][i]->key < the_heap[qid][HEAP_PARENT(i)]->key ) {
    swap_entries (qid, i, HEAP_PARENT(i));
    i = HEAP_PARENT(i);
  }
}

static inline
void bubble_down( int qid, int i ) {
  int j = 0;

  do {
    j = i;
    if ( HEAP_LEFT(j) <= heap_current_size[qid] ) {
      if (the_heap[qid][HEAP_LEFT(j)]->key < the_heap[qid][i]->key)
        i = HEAP_LEFT(j);
    }
    if ( HEAP_RIGHT(j) <= heap_current_size[qid] ) {
      if (the_heap[qid][HEAP_RIGHT(j)]->key < the_heap[qid][i]->key) 
        i = HEAP_RIGHT(j);
    }
    swap_entries(qid, i,j);
  } while (i != j);
}

static inline
void heap_insert( int qid, uint64_t kv ) {
  pq_node *n = freelist_get_node(&free_nodes[qid]);
  if (!n) {
    printk("Unable to allocate new node while spilling\n");
    while (1);
  }

  n->key = kv_key(kv);
  n->val = kv_value(kv);
  ++heap_current_size[qid];
  the_heap[qid][heap_current_size[qid]] = n;
  n->heap_index = heap_current_size[qid];
  bubble_up(qid, heap_current_size[qid]);
}

static inline
void heap_remove( int qid, int i ) {
  swap_entries(qid, i, heap_current_size[qid]);
  freelist_put_node(&free_nodes[qid], the_heap[qid][heap_current_size[qid]]);
  --heap_current_size[qid];
  bubble_down(qid, i);
}

/*
void heap_change_key( int i, int k ) {
  if (the_heap[qid][i]->key < k) {
    heap_increase_key(i,k);
  } else if (the_heap[qid][i]->key > k) {
    heap_decrease_key(i,k);
  }
}

void heap_decrease_key( int i, int k ) {
  the_heap[qid][i]->key = k;
  bubble_up(i);
}

void heap_increase_key( int i, int k ) {
  the_heap[qid][i]->key = k;
  bubble_down(i);
}
*/

static inline
uint64_t heap_min( int qid ) {
  if (heap_current_size[qid]) {
    return (HEAP_NODE_TO_KV(the_heap[qid][1]));
  }
  return (uint64_t)-1; // FIXME: error handling
}

static inline
uint64_t heap_pop_min( int qid ) {
  uint64_t kv;
  kv = heap_min(qid);
  if ( kv != (uint64_t)-1 )
    heap_remove(qid, 1);
  return kv;
}

static inline 
int sparc64_splitheappq_initialize( int qid, size_t max_pq_size )
{
  int i;
  uint64_t reg = 0;
  freelist_initialize(&free_nodes[qid], sizeof(pq_node), max_pq_size);
  sparc64_splitheappq_heap_allocate(qid, max_pq_size);

  _Chain_Initialize_empty(&queues[qid]);
  HWDS_GET_SIZE_LIMIT(qid,reg);
  queue_size[qid] = reg;

  return 0;
}

static inline 
int sparc64_splitheappq_spill_node(int qid)
{
  uint64_t kv;

  HWDS_SPILL(qid, kv);
  if (!kv) {
    DPRINTK("%d\tNothing to spill!\n", qid);
    return kv;
  }

  heap_insert(qid, kv);

  return 0;
}

static inline 
int sparc64_splitheappq_fill_node(int qid)
{
  uint32_t exception;
  uint64_t kv;

  kv = heap_pop_min(qid);

  // add node to hwpq
  HWDS_FILL(qid, kv_key(kv), kv_value(kv), exception); 

  if (exception) {
    DPRINTK("Spilling (%d,%X) while filling\n");
    return sparc64_splitheappq_spill_node(qid);
  }

  return 0;
}

static inline 
int sparc64_splitheappq_handle_spill( int qid )
{
  int i = 0;

  // pop elements off tail of hwpq, merge into software pq
  while ( i < queue_size[qid]/2 ) { // FIXME
    i++;
    sparc64_splitheappq_spill_node(qid);
  }

  return 0;
}

/*
 * Current algorithm pulls nodes from the head of the sorted sw pq
 * and fills them into the hw pq.
 */
static inline 
int sparc64_splitheappq_handle_fill(int qid)
{
 int            i = 0;

  // FIXME: figure out what threshold to use (right now just half the queue)
  while (heap_current_size[qid] > 0 && i < queue_size[qid]/2) {
    i++;
    sparc64_splitheappq_fill_node(qid);
  }

  return 0;
}

static inline 
int sparc64_splitheappq_handle_extract(int qid)
{
  uint64_t kv;
  uint32_t key;
  uint32_t val;
  int i;

  HWDS_GET_PAYLOAD(kv);
  key = kv_key(kv);
  val = kv_value(kv);

  DPRINTK("software extract: queue: %d\tnode: %x\tprio: %d\n",
      qid,val,key);

  // linear search, ugh
  for ( i = 0; i < heap_current_size[qid]; i++ ) {
    if ( the_heap[qid][i]->key == key ) {
      heap_remove(qid,i);
      i--;
      break;
    }
  }

  if ( i == heap_current_size[qid]) {
    DPRINTK("Failed software extract: %d\t%X\n", key, val);
    return -1;
  }

  return 0;
}

static inline 
int sparc64_splitheappq_drain( int qid )
{
  heap_current_size[qid] = 0;
  return 0;
}

static inline 
int sparc64_splitheappq_context_switch( int qid )
{
  int i = 0;

  // pop elements off tail of hwpq, merge into software pq
  while ( sparc64_splitheappq_spill_node(qid) ); // FIXME: pass heap pointer

  // no need to refill, will happen later.

  return 0;
}

sparc64_spillpq_operations sparc64_splitheappq_ops = {
  sparc64_splitheappq_initialize,
  sparc64_splitheappq_handle_spill,
  sparc64_splitheappq_handle_fill,
  sparc64_splitheappq_handle_extract,
  sparc64_splitheappq_drain,
  sparc64_splitheappq_context_switch
};

