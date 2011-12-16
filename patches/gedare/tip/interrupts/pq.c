#include "pq.h"
#include "freelist.h"

void sparc64_hwpq_allocate( size_t max_pq_size )
{
  sparc64_hwpq_allocate_freelist(max_pq_size, sizeof(pq_node));
}

