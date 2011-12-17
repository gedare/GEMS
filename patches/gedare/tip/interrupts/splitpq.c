#include "pq.h"
#include "freelist.h"

static Chain_Control queues[10];
static size_t queue_size[10];

/* split pq: the heap */
static pq_node *the_heap
static int heap_current_size;

void sparc64_hwpq_heap_allocate( size_t max_pq_size )
{
  the_heap = _Workspace_Allocate(size * sizeof(pq_node*));
  if ( ! the_heap ) {
    printk("Unable to allocate heap of size: \n", size * sizeof(pq_node*));
    while (1);
  }
}

void sparc64_hwpq_heap_initialize( )
{
  heap_current_size = 0;
}

static inline void swap_entries(int a, int b) {
  pq_node *tmp = the_heap[a];
  int tmpIndex = the_heap[a]->hIndex;
  the_heap[a]->hIndex = the_heap[b]->hIndex;
  the_heap[b]->hIndex = tmpIndex;
  the_heap[a] = the_heap[b];
  the_heap[b] = tmp;
}

static void bubble_up( int i )
{
  while ( i > 1 && the_heap[i]->key < the_heap[HEAP_PARENT(i)]->key ) {
    swap_entries (i, HEAP_PARENT(i));
    i = HEAP_PARENT(i);
  }
}

static void bubble_down( int i ) {
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

void heap_insert( uint64_t kv ) {
  pq_node *n = sparc64_alloc_node();
  n->key = kv_key(kv);
  n->val = kv_value(kv);
  ++heap_current_size;
  the_heap[heap_current_size] = n;
  n->hIndex = heap_current_size;
  bubble_up(heap_current_size);
}

void heap_remove( int i ) {
  swap_entries(i, heap_current_size);
  sparc64_free_node(the_heap[heap_current_size]);
  --heap_current_size;
  bubble_down(i);
}

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

uint64_t heap_min( ) {
  if (heap_current_size) {
    return (HEAP_NODE_TO_KV(the_heap[1]));
  }
  return (uint64_t)-1; // FIXME: error handling
}

uint64_t heap_pop_min( ) {
  uint64_t kv;
  kv = heap_min();
  if ( kv != (uint64_t)-1 )
    heap_remove(1);
  return kv;
}


void sparc64_hwpq_allocate( size_t max_pq_size )
{
  sparc64_hwpq_allocate_freelist(max_pq_size);

  sparc64_hwpq_heap_allocate(max_pq_size);
}

void sparc64_hwpq_initialize()
{
  proc_ptr old;
  int i;
  uint64_t reg = 0;

  for (i = 0; i < 10; i++) {
    _Chain_Initialize_empty(&queues[i]);
    HWDS_GET_SIZE_LIMIT(i,reg);
    queue_size[i] = reg;
  }

  sparc64_hwpq_initialize_freelist();
  sparc64_hwpq_heap_initialize();

  _CPU_ISR_install_vector(
    SPARC_SYNCHRONOUS_TRAP(0x41),
    sparc64_hwpq_spill_fill,
    &old
   );

  _CPU_ISR_install_vector(
    SPARC_SYNCHRONOUS_TRAP(0x43),
    sparc64_hwpq_software_extract,
    &old
   );
}

void hwpq_drain_queue( int qid ) {
  Chain_Node *tmp;
  Chain_Node *iter;
  Chain_Control *cc;
  pq_node *p;

  cc = &queues[qid];
  iter = _Chain_First(cc);

  while (!_Chain_Is_tail(cc,iter)) {
    p = iter;
    tmp = _Chain_Next(iter);
    iter = _Chain_Get_unprotected(cc);
    sparc64_free_node((pq_node*)p);
    iter = tmp;
  }
}

void sparc64_hwpq_spill_fill(uint64_t vector, CPU_Interrupt_frame *istate)
{
  uint32_t level;
  uint64_t context;
  uint32_t queue_idx;
  uint32_t operation;
  uint32_t exception;
  uint64_t val;
  uint32_t key;
  uint32_t value;
  register int mask = (1<<1);
  int i;
  pq_node *new_node;
  Chain_Node *iter;
  Chain_Control *cc;

  // acknowledge the interrupt (allows isr to schedule another at this level)
  sparc64_clear_interrupt_bits(mask);

  level = sparc_disable_interrupts();

  // get the interrupted state
  HWDS_GET_CONTEXT(context);

  // parse context
  queue_idx = (((uint32_t)(context>>32))&(~0))>>20;
  operation = (((uint32_t)(context>>32))&~(~0 << (3 + 1)));

  // TODO: currently not using the HWDS cache? :(

  // TODO: should check exception conditions, since some operations can
  // cause multiple exceptions.

  switch (operation) {
    case 2:
      cc = &queues[queue_idx];
      iter = _Chain_Last(cc);
      // pop elements off tail of hwpq, merge into software pq
      for ( i = 0;
          i < queue_size[queue_idx]/2; // FIXME
          i++ ) {
//        HWDS_LAST_PRI(queue_idx,key);
        HWDS_SPILL(queue_idx, val); 
        if (!val) {
          printk("Nothing to spill!\n");
          while(1);
        }
        key = POINTER_TO_KEY(val);
        value = POINTER_TO_VALUE(val);
#ifdef GAB_DEBUG
        printk("spill: queue: %d\tnode: %x\tprio: %d\n",queue_idx,value,key);
#endif
        // sort by ascending key
        while (!_Chain_Is_head(cc, iter) && 
            key < ((pq_node*)iter)->key) 
          iter = _Chain_Previous(iter);

        // TODO: use a free list instead of workspace_allocate?
        new_node = sparc64_alloc_node(sizeof(pq_node));
        if (!new_node) {
#ifdef GAB_DEBUG
  // debug output
  iter = _Chain_First(cc);
  i=0;
  while(!_Chain_Is_tail(cc,iter)) {
    pq_node *p = (pq_node*)iter;
    printk("%d\tChain: %X\t%d\t%x\n",i,p,p->key,p->val);
    i++;
    iter = _Chain_Next(iter);
  }
#endif
      printk("Unable to allocate new node while spilling\n");
        while (1);
        }

        // key >= iter->key, insert key node after iter
        new_node->key = key;
        new_node->val = value; // FIXME: not full 64-bits here
        _Chain_Insert_unprotected(iter, (Chain_Node*)new_node);
      }
      break;

    case 3: case 6: case 11:
#ifdef GAB_DEBUG_FILL
      printk("fill\tqueue: %d\n",queue_idx);
#endif
      cc = &queues[queue_idx];
      i = 0;
      
      /*
       * Current algorithm pulls nodes from the head of the sorted sw pq
       * and fills them into the hw pq.
       */
      while (!_Chain_Is_empty(cc)) {
        ++i;
        iter = _Chain_First(cc); // Always get the first node
        pq_node *p = iter;
#ifdef GAB_DEBUG_FILL
        printk("fill: queue: %d\tnode: %x\tprio: %d\n",
            queue_idx,p->val,p->key);
#endif
        Chain_Node *tmp = _Chain_Next(iter);
        // add node to hw pq 
        HWDS_FILL(queue_idx, p->key, p->val, exception); 
        
        // remove node from sw pq
        iter = _Chain_Get_unprotected(cc);

        sparc64_free_node((pq_node*)p);
        iter = tmp;

        if (exception) {
          tmp = _Chain_Last(cc);
          // HWDS_LAST_PRI(queue_idx,key);
          HWDS_SPILL(queue_idx, val);
          if (!val) {
            printk("nothing spilled!\n");
            while(1);
          }
          key = POINTER_TO_KEY(val);
          value = POINTER_TO_VALUE(val);
#ifdef GAB_DEBUG_FILL
          printk("Spilling (%d,%X) while filling\n",key,value);
#endif
          // sort by ascending key
          while (!_Chain_Is_head(cc, tmp) && key < ((pq_node*)tmp)->key) {
            tmp = _Chain_Previous(tmp);
          }

          new_node = sparc64_alloc_node();
          if (!new_node) {
            printk("Unable to allocate new_node\n");
            while (1);
          }
          // key >= tmp->key, insert key node after tmp
          new_node->key = key;
          new_node->val = value;
          _Chain_Insert_unprotected(tmp, (Chain_Node*)new_node);
        }

        if (i >= queue_size[queue_idx]/2) // FIXME
          break;
      }
      break;

    default:
#ifdef GAB_DEBUG
      printk("Unknown operation (RTEMS)\n");
#endif
      break;
  }

#ifdef GAB_DEBUG
  // debug output
  iter = _Chain_First(cc);
  while(!_Chain_Is_tail(cc,iter)) {
    pq_node *p = (pq_node*)iter;
    printk("Chain: %X\t%d\t%x\n",p,p->key,p->val);
    iter = _Chain_Next(iter);
  }
#endif

  sparc_enable_interrupts(level);

}


void sparc64_hwpq_software_extract(uint64_t vector, CPU_Interrupt_frame *istate)
{
  register int mask = (1<<3);

  uint32_t level;
  uint64_t context;
  uint32_t queue_idx;
  uint32_t operation;
  uint64_t val;
  uint32_t key;
  uint32_t value;
  pq_node *new_node;
  Chain_Node *iter;
  Chain_Control *cc;

  level = sparc_disable_interrupts();

  // get the interrupted state
  HWDS_GET_CONTEXT(context);

  queue_idx = (((uint32_t)(context>>32))&(~0))>>20;
  operation = (((uint32_t)(context>>32))&~(~0 << (3 + 1)));

  HWDS_GET_PAYLOAD(val);

#ifdef GAB_DEBUG
  printk("Software extract: %x\n",val);
#endif

  key = POINTER_TO_KEY(val);
  value = POINTER_TO_VALUE(val);


  // linear search, ugh
  cc = &queues[queue_idx];
  iter = _Chain_First(cc);
  while(!_Chain_Is_tail(cc,iter)) {
    pq_node *p = (pq_node*)iter;
    if (p->val == value) {
      if (_Chain_Is_first(iter))
        _Chain_Get_first_unprotected(iter);
      else
        _Chain_Extract_unprotected(iter);
      sparc64_free_node(iter);
      break;
    }
    iter = _Chain_Next(iter);
  }

  if (_Chain_Is_tail(cc,iter)) {
    printk("Failed software extract: %d\t%X\n",key,value);
  }

  // acknowledge the interrupt (allows isr to schedule another at this level)
  // need to ack interrupt in case other HWDS operations cause an interrupt
  sparc64_clear_interrupt_bits(mask);
  HWDS_CHECK_UNDERFLOW(queue_idx);

  sparc_enable_interrupts(level);

}
