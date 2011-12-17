#include "unitedpq.h"
#include "freelist.h"
#include "gabdebug.h"

static Chain_Control queues[10];
static size_t queue_size[10];

static inline void sparc64_unitedpq_initialize( size_t max_pq_size )
{
  int i;
  uint64_t reg = 0;
  sparc64_hwpq_allocate_freelist(max_pq_size, sizeof(pq_node));

  for (i = 0; i < 10; i++) {
    _Chain_Initialize_empty(&queues[i]);
    HWDS_GET_SIZE_LIMIT(i,reg);
    queue_size[i] = reg;
  }
}

static inline void sparc64_unitedpq_spill_node(queue_idx, Chain_Control *spill_pq)
{
  Chain_Node *iter;
  uint64_t kv;
  uint32_t key, val;
  pq_node *new_node;
  int i;

  iter = _Chain_Last(spill_pq);

  HWDS_SPILL(queue_idx, kv);
  if (!kv) {
    printk("Nothing to spill!\n");
    while(1);
  }
  key = kv_key(kv);
  val = kv_value(kv);

  DPRINTK("spill: queue: %d\tnode: %x\tprio: %d\n",queue_idx,val,key);

  // sort by ascending priority
  while (!_Chain_Is_head(spill_pq, iter) && key < ((pq_node*)iter)->key) 
    iter = _Chain_Previous(iter);

  new_node = sparc64_alloc_node();
  if (!new_node) {
    // debug output
    iter = _Chain_First(spill_pq);
    i = 0;
    while(!_Chain_Is_tail(spill_pq,iter)) {
      pq_node *p = (pq_node*)iter;
      DPRINTK("%d\tChain: %X\t%d\t%x\n",i,p,p->key,p->val);
      i++;
      iter = _Chain_Next(iter);
    }
    printk("Unable to allocate new node while spilling\n");
    while (1);
  }

  // key > iter->key, insert new node after iter
  new_node->key = key;
  new_node->val = val; // FIXME: not full 64-bits
  _Chain_Insert_unprotected(iter, (Chain_Node*)new_node);
}

static inline void sparc64_unitedpq_fill_node(int queue_idx, Chain_Control *spill_pq)
{
  uint32_t exception;
  pq_node *p;

  p = (pq_node*)_Chain_Get_first_unprotected(spill_pq);

  DPRINTK("fill: queue: %d\tnode: %x\tprio: %d\n", queue_idx, p->key, p->val);

  // add node to hw pq 
  HWDS_FILL(queue_idx, p->key, p->val, exception); 

  sparc64_free_node(p);

  if (exception) {
    DPRINTK("Spilling (%d,%X) while filling\n");
    sparc64_unitedpq_spill_node(queue_idx, spill_pq);
  }
}

static inline void sparc64_unitedpq_handle_spill( int queue_idx )
{
  int i = 0;
  Chain_Control *spill_pq;

  spill_pq = &queues[queue_idx];

  // pop elements off tail of hwpq, merge into software pq
  while ( i < queue_size[queue_idx]/2 ) { // FIXME
    i++;
    sparc64_unitedpq_spill_node(queue_idx, spill_pq);
  }
}

/*
 * Current algorithm pulls nodes from the head of the sorted sw pq
 * and fills them into the hw pq.
 */
static inline void sparc64_unitedpq_handle_fill(int queue_idx)
{
 Chain_Control *spill_pq;
 int            i = 0;

 spill_pq = &queues[queue_idx];

  // FIXME: figure out what threshold to use (right now just half the queue)
  while (!_Chain_Is_empty(spill_pq) && i < queue_size[queue_idx]/2) {
    i++;
    sparc64_unitedpq_fill_node(queue_idx, spill_pq);
  }
}

static inline void sparc64_unitedpq_handle_extract(int queue_idx)
{
  uint64_t kv;
  uint32_t key;
  uint32_t val;
  Chain_Node *iter;
  Chain_Control *spill_pq;

  HWDS_GET_PAYLOAD(kv);
  key = kv_key(kv);
  val = kv_value(kv);

  DPRINTK("software extract: queue: %d\tnode: %x\tprio: %d\n",
      queue_idx,val,key);

  // linear search, ugh
  spill_pq = &queues[queue_idx];
  iter = _Chain_First(spill_pq);
  while(!_Chain_Is_tail(spill_pq,iter)) {
    pq_node *p = (pq_node*)iter;
    if (p->val == val) {
      if (_Chain_Is_first(iter))
        _Chain_Get_first_unprotected(spill_pq);
      else
        _Chain_Extract_unprotected(iter);
      sparc64_free_node(iter);
      break;
    }
    iter = _Chain_Next(iter);
  }

  if (_Chain_Is_tail(spill_pq,iter)) {
    printk("Failed software extract: %d\t%X\n",key,val);
  }
}

static inline void sparc64_unitedpq_drain( int qid )
{
  Chain_Node *tmp;
  Chain_Node *iter;
  Chain_Control *spill_pq;
  pq_node *p;

  spill_pq = &queues[qid];
  iter = _Chain_First(spill_pq);

  while (!_Chain_Is_tail(spill_pq,iter)) {
    p = iter;
    tmp = _Chain_Next(iter);
    iter = _Chain_Get_unprotected(spill_pq);
    sparc64_free_node((pq_node*)p);
    iter = tmp;
  }
}

void sparc64_spillpq_initialize( int max_pq_size )
{
  sparc64_unitedpq_initialize(max_pq_size);
}
void sparc64_spillpq_handle_spill(int queue_idx)
{
  sparc64_unitedpq_handle_spill(queue_idx);
}
void sparc64_spillpq_handle_fill(int queue_idx)
{
  sparc64_unitedpq_handle_fill(queue_idx);
}
void sparc64_spillpq_handle_extract(int queue_idx)
{
  sparc64_unitedpq_handle_extract(queue_idx);
}
void sparc64_spillpq_drain( int queue_id )
{
  sparc64_unitedpq_drain(queue_id );
}

