#include "spillpq.h"
#include "freelist.h"
#include "gabdebug.h"

static Chain_Control queues[10];
static size_t queue_size[10];
static Freelist_Control free_nodes;

typedef struct {
  Chain_Node Node;
  uint32_t key;
  uint32_t val;
} pq_node;

static inline 
int sparc64_unitedlistpq_initialize( size_t max_pq_size )
{
  int i;
  uint64_t reg = 0;
  freelist_initialize(&free_nodes, sizeof(pq_node), max_pq_size);

  for (i = 0; i < 10; i++) {
    _Chain_Initialize_empty(&queues[i]);
    HWDS_GET_SIZE_LIMIT(i,reg);
    queue_size[i] = reg;
    DPRINTK("Queue %d: Size: %ld\n", i, reg);
  }
  return 0;
}

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
    printk("Nothing to spill!\n");
    while(1);
  }
  key = kv_key(kv);
  val = kv_value(kv);

  DPRINTK("spill: queue: %d\tnode: %x\tprio: %d\n",queue_idx,val,key);

  // sort by ascending priority
  // Note that this is not a stable sort (globally) because it has
  // FIFO behavior for spilled nodes with equal keys. To get global stability
  // we would need to ensure that (1) the key comparison is <= and (2) the
  // last node to be spilled has no ties left in the hwpq.
  while (!_Chain_Is_head(spill_pq, iter) && key < ((pq_node*)iter)->key) 
    iter = _Chain_Previous(iter);

  new_node = freelist_get_node(&free_nodes);
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
  return 0;
}

static inline 
int sparc64_unitedlistpq_fill_node(int queue_idx, Chain_Control *spill_pq)
{
  uint32_t exception;
  pq_node *p;

  p = (pq_node*)_Chain_Get_first_unprotected(spill_pq);

  DPRINTK("fill: queue: %d\tnode: %x\tprio: %d\n", queue_idx, p->key, p->val);

  // add node to hw pq 
  HWDS_FILL(queue_idx, p->key, p->val, exception); 

  freelist_put_node(&free_nodes, p);

  if (exception) {
    DPRINTK("Spilling (%d,%X) while filling\n");
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
  while ( i < queue_size[queue_idx]/2 ) { // FIXME
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
  while (!_Chain_Is_empty(spill_pq) && i < queue_size[queue_idx]/2) {
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

  DPRINTK("software extract: queue: %d\t%X\tprio: %d\n",queue_idx,key,val);

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
      freelist_put_node(&free_nodes, iter);
      break;
    }
    iter = _Chain_Next(iter);
  }

  if (_Chain_Is_tail(spill_pq,iter)) {
    DPRINTK("Failed software extract: %d\t%X\n",key,val);
    return -1;
  }
  return 0;
}

static inline 
int sparc64_unitedlistpq_drain( int qid )
{
  Chain_Node *tmp;
  Chain_Node *iter;
  Chain_Control *spill_pq;
  pq_node *p;

  DPRINTK("drain queue: %d\n", qid);
  spill_pq = &queues[qid];
  iter = _Chain_First(spill_pq);

  return 0;
}

sparc64_spillpq_operations sparc64_unitedlistpq_ops = {
  sparc64_unitedlistpq_initialize,
  sparc64_unitedlistpq_handle_spill,
  sparc64_unitedlistpq_handle_fill,
  sparc64_unitedlistpq_handle_extract,
  sparc64_unitedlistpq_drain
};

