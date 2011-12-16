
#ifndef __SPARC64_PQ_H
#define __SPARC64_PQ_H


#include <rtems/score/chain.h>
#include <rtems/rtems/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define POINTER_TO_KEY(ptr)   (ptr>>32)
#define POINTER_TO_VALUE(ptr) ((uint32_t)ptr)

//#define GAB_DEBUG
//#define GAB_DEBUG_FILL

#if defined(GAB_DEBUG)
#define GAB_DEBUG_FILL
#endif
#include <rtems/bspIo.h>

typedef struct {
  Chain_Node Node;
  uint32_t priority;
  uint32_t pointer;
} pq_node;

#ifdef __cplusplus
}
#endif

#endif
