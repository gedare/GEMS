
#ifndef __SPARC64_SPILLPQ_H
#define __SPARC64_SPILLPQ_H


#include <rtems/score/chain.h>
#include <rtems/rtems/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define kv_value(kv) ((uint32_t)kv)
#define kv_key(kv)   (kv>>32)

//#define GAB_DEBUG

#include <rtems/bspIo.h>

typedef void (*SpillPQ_Function)(int argument);

typedef struct {
  SpillPQ_Function  initialize;
  SpillPQ_Function  spill;
  SpillPQ_Function  fill;
  SpillPQ_Function  extract;
  SpillPQ_Function  drain;
} sparc64_spillpq_operations;

extern sparc64_spillpq_operations spillpq_ops;

extern void sparc64_spillpq_initialize( int max_pq_size );
extern void sparc64_spillpq_handle_spill(int queue_idx);
extern void sparc64_spillpq_handle_fill(int queue_idx);
extern void sparc64_spillpq_handle_extract(int queue_idx);
extern void sparc64_spillpq_drain( int queue_id );

#ifdef __cplusplus
}
#endif

#endif
