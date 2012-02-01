#include "spillpq.h"

sparc64_spillpq_operations *spillpq_ops = NULL; 

int sparc64_spillpq_initialize( int max_pq_size )
{
  return spillpq_ops->initialize(max_pq_size);
}

int sparc64_spillpq_handle_spill(int queue_idx)
{
  return spillpq_ops->spill(queue_idx);
}

int sparc64_spillpq_handle_fill(int queue_idx)
{
  return spillpq_ops->fill(queue_idx);
}

int sparc64_spillpq_handle_extract(int queue_idx)
{
  return spillpq_ops->extract(queue_idx);
}

int sparc64_spillpq_drain( int queue_id )
{
  return spillpq_ops->drain(queue_id);
}

