#include "splitpq.h"
#include "freelist.h"
#include "gabdebug.h"

static Chain_Control queues[10];
static size_t queue_size[10];

/* split pq: the heap */
static pq_node **the_heap;
static int heap_current_size;

/* heap implementation ... */
#define HEAP_PARENT(i) (i>>1)
#define HEAP_FIRST (1)
#define HEAP_LEFT(i) (i<<1)
#define HEAP_RIGHT(i) (HEAP_LEFT(i)+1)

#define HEAP_NODE_TO_KV(n) ((((uint64_t)n->key) << 32UL) | (uint64_t)n->val)
#define kv_value(kv) ((uint32_t)kv)
#define kv_key(kv)   (kv>>32)

static inline
void sparc64_splitpq_heap_allocate( size_t max_pq_size )
{
  the_heap = _Workspace_Allocate(max_pq_size * sizeof(pq_node*));
  if ( ! the_heap ) {
    printk("Unable to allocate heap of size: \n",max_pq_size*sizeof(pq_node*));
    while (1);
  }
  heap_current_size = 0;
}

static inline
void swap_entries(int a, int b) {
  pq_node *tmp = the_heap[a];
  int tmpIndex = the_heap[a]->heap_index;
  the_heap[a]->heap_index = the_heap[b]->heap_index;
  the_heap[b]->heap_index = tmpIndex;
  the_heap[a] = the_heap[b];
  the_heap[b] = tmp;
}

static inline
void bubble_up( int i )
{
  while ( i > 1 && the_heap[i]->key < the_heap[HEAP_PARENT(i)]->key ) {
    swap_entries (i, HEAP_PARENT(i));
    i = HEAP_PARENT(i);
  }
}

static inline
void bubble_down( int i ) {
  int j = 0;

  do {
    j = i;
    if ( HEAP_LEFT(j) <= heap_current_size ) {
      if (the_heap[HEAP_LEFT(j)]->key < the_heap[i]->key)
        i = HEAP_LEFT(j);
    }
    if ( HEAP_RIGHT(j) <= heap_current_size ) {
      if (the_heap[HEAP_RIGHT(j)]->key < the_heap[i]->key) 
        i = HEAP_RIGHT(j);
    }
    swap_entries(i,j);
  } while (i != j);
}

static inline
heap_insert( uint64_t kv ) {
  pq_node *n = sparc64_alloc_node();
  if (!n) {
    printk("Unable to allocate new node while spilling\n");
    while (1);
  }

  n->key = kv_key(kv);
  n->val = kv_value(kv);
  ++heap_current_size;
  the_heap[heap_current_size] = n;
  n->heap_index = heap_current_size;
  bubble_up(heap_current_size);
}

static inline
heap_remove( int i ) {
  swap_entries(i, heap_current_size);
  sparc64_free_node(the_heap[heap_current_size]);
  --heap_current_size;
  bubble_down(i);
}

/*
void heap_change_key( int i, int k ) {
  if (the_heap[i]->key < k) {
    heap_increase_key(i,k);
  } else if (the_heap[i]->key > k) {
    heap_decrease_key(i,k);
  }
}

void heap_decrease_key( int i, int k ) {
  the_heap[i]->key = k;
  bubble_up(i);
}

void heap_increase_key( int i, int k ) {
  the_heap[i]->key = k;
  bubble_down(i);
}
*/

static inline
uint64_t heap_min( ) {
  if (heap_current_size) {
    return (HEAP_NODE_TO_KV(the_heap[1]));
  }
  return (uint64_t)-1; // FIXME: error handling
}

static inline
uint64_t heap_pop_min( ) {
  uint64_t kv;
  kv = heap_min();
  if ( kv != (uint64_t)-1 )
    heap_remove(1);
  return kv;
}

static inline 
void sparc64_splitpq_initialize( size_t max_pq_size )
{
  int i;
  uint64_t reg = 0;
  sparc64_hwpq_allocate_freelist(max_pq_size, sizeof(pq_node));
  sparc64_splitpq_heap_allocate(max_pq_size);

  for (i = 0; i < 10; i++) {
    _Chain_Initialize_empty(&queues[i]);
    HWDS_GET_SIZE_LIMIT(i,reg);
    queue_size[i] = reg;
  }
}

static inline 
void sparc64_splitpq_spill_node(int queue_idx)
{
  uint64_t kv;

  HWDS_SPILL(queue_idx, kv);
  if (!kv) {
    printk("Nothing to spill!\n");
    while(1);
  }

  heap_insert(kv);
}

static inline 
void sparc64_splitpq_fill_node(int queue_idx)
{
  uint32_t exception;
  uint64_t kv;

  kv = heap_pop_min();

  // add node to hwpq
  HWDS_FILL(queue_idx, kv_key(kv), kv_value(kv), exception); 

  if (exception) {
    DPRINTK("Spilling (%d,%X) while filling\n");
    sparc64_splitpq_spill_node(queue_idx);
  }
}

static inline 
void sparc64_splitpq_handle_spill( int queue_idx )
{
  int i = 0;

  // pop elements off tail of hwpq, merge into software pq
  while ( i < queue_size[queue_idx]/2 ) { // FIXME
    i++;
    sparc64_splitpq_spill_node(queue_idx);
  }
}

/*
 * Current algorithm pulls nodes from the head of the sorted sw pq
 * and fills them into the hw pq.
 */
static inline 
void sparc64_splitpq_handle_fill(int queue_idx)
{
 int            i = 0;

  // FIXME: figure out what threshold to use (right now just half the queue)
  while (heap_current_size > 0 && i < queue_size[queue_idx]/2) {
    i++;
    sparc64_splitpq_fill_node(queue_idx);
  }
}

static inline 
void sparc64_splitpq_handle_extract(int queue_idx)
{
  uint64_t kv;
  uint32_t key;
  uint32_t val;
  int i;

  HWDS_GET_PAYLOAD(kv);
  key = kv_key(kv);
  val = kv_value(kv);

  DPRINTK("software extract: queue: %d\tnode: %x\tprio: %d\n",
      queue_idx,val,key);

  // linear search, ugh
  for ( i = 0; i < heap_current_size; i++ ) {
    if ( the_heap[i]->key == key ) {
      heap_remove(i);
      i--;
      break;
    }
  }

  if ( i == heap_current_size) {
    printk("Failed software extract: %d\t%X\n", key, val);
  }
}

static inline 
void sparc64_splitpq_drain( int qid )
{
  heap_current_size = 0;
}




void sparc64_spillpq_initialize( int max_pq_size )
{
  sparc64_splitpq_initialize(max_pq_size);
}
void sparc64_spillpq_handle_spill(int queue_idx)
{
  sparc64_splitpq_handle_spill(queue_idx);
}
void sparc64_spillpq_handle_fill(int queue_idx)
{
  sparc64_splitpq_handle_fill(queue_idx);
}
void sparc64_spillpq_handle_extract(int queue_idx)
{
  sparc64_splitpq_handle_extract(queue_idx);
}
void sparc64_spillpq_drain( int queue_id )
{
  sparc64_splitpq_drain(queue_id );
}

