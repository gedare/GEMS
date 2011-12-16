#include "united_pq.h"
#include "freelist.h"

void sparc64_spillpq_initialize( size_t max_pq_size )
{
  sparc64_hwpq_allocate_freelist(max_pq_size, sizeof(pq_node));
}

