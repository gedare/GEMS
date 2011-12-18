#include "spillpq.h"

void sparc64_spillpq_initialize( int max_pq_size )
{
  spillpq_ops.initialize(max_pq_size);
}

void sparc64_spillpq_handle_spill(int queue_idx)
{
  spillpq_ops.spill(queue_idx);
}

void sparc64_spillpq_handle_fill(int queue_idx)
{
  spillpq_ops.fill(queue_idx);
}

void sparc64_spillpq_handle_extract(int queue_idx)
{
  spillpq_ops.extract(queue_idx);
}

void sparc64_spillpq_drain( int queue_id )
{
  spillpq_ops.drain(queue_id);
}

