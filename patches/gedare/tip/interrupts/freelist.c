
#include "freelist.h"

void freelist_initialize(
    Freelist_Control *fc,
    size_t node_size,
    size_t bump_count
) {
  _Chain_Initialize_empty( &fc->freelist );
  fc->node_size = node_size;
  fc->bump_count = bump_count;
  fc->free_count = 0;
  freelist_bump(fc);
}

size_t freelist_bump(Freelist_Control *fc)
{
  void *nodes;
  int i;
  size_t count = fc->bump_count;
  size_t size = fc->node_size;

  /* better to use workspace or malloc? */
  nodes = _Workspace_Allocate(count * size);
  if (!nodes) {
    printk("Unable to allocate free list of size: \n", count * size);
    return 0;
  }

  fc->free_count += count;
  for ( i = 0; i < count; i++ ) {
    _Chain_Append_unprotected( &fc->freelist, (size_t)nodes+i*size );
  }
  return count;
}

void *freelist_get_node(Freelist_Control *fc) {
  if ( fc->free_count == 0 ) {
    if ( !freelist_bump(fc) ) {
      return NULL;
    }
  }
  fc->free_count--;
  return _Chain_Get_first_unprotected( &fc->freelist );
}

void freelist_put_node(Freelist_Control *fc, void *n) {
  _Chain_Prepend_unprotected( &fc->freelist, n );
  fc->free_count++;
}

