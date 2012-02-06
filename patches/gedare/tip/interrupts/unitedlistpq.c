#include "spillpq.h"
#include "freelist.h"
#include "gabdebug.h"

static Chain_Control queues[10];
static size_t queue_max_size[10];
static Freelist_Control free_nodes[10];

typedef struct {
  Chain_Node Node;
  uint32_t key;
  uint32_t val;
} pq_node;

static inline
void sparc64_print_all_queues()
{
  Chain_Node *iter;
  Chain_Control *spill_pq;
  int i,k;
  for ( k = 0; k < 10; k++ ) {
    spill_pq = &queues[k];
    iter = _Chain_First(spill_pq);
    i = 0;
    while(!_Chain_Is_tail(spill_pq,iter)) {
      pq_node *p = (pq_node*)iter;
      printk("%d\tChain: %d\t%X\t%d\t%x\n", k, i, p,p->key,p->val);
      i++;
      iter = _Chain_Next(iter);
    }
  }
}

static inline 
int sparc64_unitedlistpq_initialize( int qid, size_t max_pq_size )
{
  int i;
  uint64_t reg = 0;
  freelist_initialize(&free_nodes[qid], sizeof(pq_node), max_pq_size);

  _Chain_Initialize_empty(&queues[qid]);
  HWDS_GET_SIZE_LIMIT(qid,reg);
  queue_max_size[qid] = reg;
  DPRINTK("%d\tSize: %ld\n", qid, reg);
  return 0;
}

// FIXME: make a single pass to spill all the nodes..
static inline 
int sparc64_unitedlistpq_spill_node(int queue_idx, Chain_Control *spill_pq)
{
  Chain_Node *iter;
  uint64_t kv;
  uint32_t key, val;
  pq_node *new_node;
  int i;

  iter = _Chain_Last(spill_pq);

  HWDS_SPILL(queue_idx, kv);
  if (!kv) {
    DPRINTK("%d\tNothing to spill!\n",queue_idx);
    return 0;
  }
  key = kv_key(kv);
  val = kv_value(kv);

  DPRINTK("%d\tspill: node: %x\tprio: %d\n",queue_idx,val,key);

  // sort by ascending priority
  // Note that this is not a stable sort (globally) because it has
  // FIFO behavior for spilled nodes with equal keys. To get global stability
  // we would need to ensure that (1) the key comparison is <= and (2) the
  // last node to be spilled has no ties left in the hwpq.
  while (!_Chain_Is_head(spill_pq, iter) && key < ((pq_node*)iter)->key) 
    iter = _Chain_Previous(iter);

  new_node = freelist_get_node(&free_nodes[queue_idx]);
  if (!new_node) {
    // debug output
    sparc64_print_all_queues();
    printk("%d\tUnable to allocate new node while spilling\n", queue_idx);
    while (1);
  }

  // key > iter->key, insert new node after iter
  new_node->key = key;
  new_node->val = val; // FIXME: not full 64-bits
  _Chain_Insert_unprotected(iter, (Chain_Node*)new_node);
  return 1;
}

static inline 
int sparc64_unitedlistpq_fill_node(int queue_idx, Chain_Control *spill_pq)
{
  uint32_t exception;
  pq_node *p;

  p = (pq_node*)_Chain_Get_first_unprotected(spill_pq);

  DPRINTK("%d\tfill node: %x\tprio: %d\n", queue_idx, p->val, p->key);

  // add node to hw pq 
  HWDS_FILL(queue_idx, p->key, p->val, exception); 

  freelist_put_node(&free_nodes[queue_idx], p);

  if (exception) {
    DPRINTK("%d\tSpilling while filling\n", queue_idx);
    return sparc64_unitedlistpq_spill_node(queue_idx, spill_pq);
  }
  return 0;
}

static inline 
int sparc64_unitedlistpq_handle_spill( int queue_idx )
{
  int i = 0;
  Chain_Control *spill_pq;

  spill_pq = &queues[queue_idx];
  DPRINTK("spill: queue: %d\n", queue_idx);

  // pop elements off tail of hwpq, merge into software pq
  while ( i < queue_max_size[queue_idx]/2 ) { // FIXME
    i++;
    sparc64_unitedlistpq_spill_node(queue_idx, spill_pq); /* FIXME */
  }
  return 0;
}

/*
 * Current algorithm pulls nodes from the head of the sorted sw pq
 * and fills them into the hw pq.
 */
static inline 
int sparc64_unitedlistpq_handle_fill(int queue_idx)
{
 Chain_Control *spill_pq;
 int            i = 0;

 spill_pq = &queues[queue_idx];
 DPRINTK("fill: queue: %d\n", queue_idx);

  // FIXME: figure out what threshold to use (right now just half the queue)
  while (!_Chain_Is_empty(spill_pq) && i < queue_max_size[queue_idx]/2) {
    i++;
    sparc64_unitedlistpq_fill_node(queue_idx, spill_pq); /* FIXME */
  }
  return 0;
}

static inline 
int sparc64_unitedlistpq_handle_extract(int queue_idx)
{
  uint64_t kv;
  uint32_t key;
  uint32_t val;
  Chain_Node *iter;
  Chain_Control *spill_pq;

  HWDS_GET_PAYLOAD(kv);
  key = kv_key(kv);
  val = kv_value(kv);

  DPRINTK("%d\tsoftware extract:\t%X\tprio: %d\n",queue_idx, key, val);

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
      freelist_put_node(&free_nodes[queue_idx], iter);
      break;
    }
    iter = _Chain_Next(iter);
  }

  if (_Chain_Is_tail(spill_pq,iter)) {
    DPRINTK("%d\tFailed software extract: %d\t%X\n",queue_idx, key,val);
    return -1;
  }
  return 0;
}

static inline 
int sparc64_unitedlistpq_drain( int qid )
{
  Chain_Node *tmp;
  Chain_Control *spill_pq;
  pq_node *p;

  DPRINTK("%d\tdrain queue\n", qid);
  spill_pq = &queues[qid];

  return 0;
}

static inline 
int sparc64_unitedlistpq_context_switch( int qid )
{
  Chain_Control *spill_pq;
  Chain_Node *iter;
  int i = 0;

  DPRINTK("%d\tcontext_switch\n", qid);
  spill_pq = &queues[qid];
  while ( sparc64_unitedlistpq_spill_node(qid, spill_pq) );
  
  // pull back one node (head/first) to satisfy 'read first' requests.
  sparc64_unitedlistpq_fill_node(qid, spill_pq);

#if defined(GAB_DEBUG)
  iter = _Chain_First(spill_pq);
  i = 0;

  while(!_Chain_Is_tail(spill_pq,iter)) {
    pq_node *p = (pq_node*)iter;
    printk("%d\tChain: %d\t%X\t%d\t%x\n",qid,i,p,p->key,p->val);
    i++;
    iter = _Chain_Next(iter);
  }
#endif

  return 0;
}
sparc64_spillpq_operations sparc64_unitedlistpq_ops = {
  sparc64_unitedlistpq_initialize,
  sparc64_unitedlistpq_handle_spill,
  sparc64_unitedlistpq_handle_fill,
  sparc64_unitedlistpq_handle_extract,
  sparc64_unitedlistpq_drain,
  sparc64_unitedlistpq_context_switch
};

