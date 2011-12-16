#include "pq.h"
#include "freelist.i"
static Chain_Control queues[10];
static size_t queue_size[10];

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
  uint64_t pointer;
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
//        HWDS_LAST_PRI(queue_idx,priority);
        HWDS_SPILL(queue_idx, pointer); 
        if (!pointer) {
          printk("Nothing to spill!\n");
          while(1);
        }
        key = POINTER_TO_KEY(pointer);
        value = POINTER_TO_VALUE(pointer);
#ifdef GAB_DEBUG
        printk("spill: queue: %d\tnode: %x\tprio: %d\n",queue_idx,value,key);
#endif
        // sort by ascending priority
        while (!_Chain_Is_head(cc, iter) && 
            key < ((pq_node*)iter)->priority) 
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
    printk("%d\tChain: %X\t%d\t%x\n",i,p,p->priority,p->pointer);
    i++;
    iter = _Chain_Next(iter);
  }
#endif
      printk("Unable to allocate new node while spilling\n");
        while (1);
        }

        // priority >= iter->priority, insert priority node after iter
        new_node->priority = key;
        new_node->pointer = value; // FIXME: not full 64-bits here
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
            queue_idx,p->pointer,p->priority);
#endif
        Chain_Node *tmp = _Chain_Next(iter);
        // add node to hw pq 
        HWDS_FILL(queue_idx, p->priority, p->pointer, exception); 
        
        // remove node from sw pq
        iter = _Chain_Get_unprotected(cc);

        sparc64_free_node((pq_node*)p);
        iter = tmp;

        if (exception) {
          tmp = _Chain_Last(cc);
          // HWDS_LAST_PRI(queue_idx,priority);
          HWDS_SPILL(queue_idx, pointer);
          if (!pointer) {
            printk("nothing spilled!\n");
            while(1);
          }
          key = POINTER_TO_KEY(pointer);
          value = POINTER_TO_VALUE(pointer);
#ifdef GAB_DEBUG_FILL
          printk("Spilling (%d,%X) while filling\n",key,value);
#endif
          // sort by ascending priority
          while (!_Chain_Is_head(cc, tmp) && key < ((pq_node*)tmp)->priority) {
            tmp = _Chain_Previous(tmp);
          }

          new_node = sparc64_alloc_node();
          if (!new_node) {
            printk("Unable to allocate new_node\n");
            while (1);
          }
          // priority >= tmp->priority, insert priority node after tmp
          new_node->priority = key;
          new_node->pointer = value;
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
    printk("Chain: %X\t%d\t%x\n",p,p->priority,p->pointer);
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
  uint64_t pointer;
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

  HWDS_GET_PAYLOAD(pointer);

#ifdef GAB_DEBUG
  printk("Software extract: %x\n",pointer);
#endif

  key = POINTER_TO_KEY(pointer);
  value = POINTER_TO_VALUE(pointer);


  // linear search, ugh
  cc = &queues[queue_idx];
  iter = _Chain_First(cc);
  while(!_Chain_Is_tail(cc,iter)) {
    pq_node *p = (pq_node*)iter;
    if (p->pointer == value) {
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
