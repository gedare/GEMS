
#include "freelist.h"

Chain_Control sparc64_hwpq_freelist;

void *sparc64_alloc_node() {
  return _Chain_Get_unprotected( &sparc64_hwpq_freelist );
}

void sparc64_free_node(void *n) {
  _Chain_Append_unprotected( &sparc64_hwpq_freelist, n );
}

void sparc64_hwpq_allocate_freelist( size_t max_pq_size, size_t node_size )
{
  int i;
  void *the_nodes;
  the_nodes = _Workspace_Allocate(max_pq_size * node_size);
  if (!the_nodes) {
    printk("Unable to allocate free list of size: \n", max_pq_size * node_size);
    while (1);
  }

  _Chain_Initialize_empty ( &sparc64_hwpq_freelist );
  for ( i = 0; i < max_pq_size; i++ ) {
    /* this is kind of ugly */
    _Chain_Append_unprotected(
        &sparc64_hwpq_freelist, 
        (size_t)the_nodes+i*node_size
    );
  }
}
