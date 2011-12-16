
// FIXME: will this have to be protected for multitasking?
static Chain_Control freelist;

static inline pq_node *sparc64_alloc_node() {
  return _Chain_Get_unprotected( &freelist );
}

static inline void sparc64_free_node(pq_node *n) {
  _Chain_Append_unprotected( &freelist, n );
}

static inline void sparc64_hwpq_initialize_freelist( )
{
  _Chain_Initialize_empty ( &freelist );
}

static inline void sparc64_hwpq_allocate_freelist( int size )
{
  int i;
  pq_node *the_nodes;
  the_nodes = _Workspace_Allocate(size * sizeof(pq_node));
  if (!the_nodes) {
    printk("Unable to allocate free list of size: \n", size * sizeof(pq_node));
    while (1);
  }

  for ( i = 0; i < size; i++ ) {
    _Chain_Append_unprotected(&freelist, &the_nodes[i].Node);
  }
}

