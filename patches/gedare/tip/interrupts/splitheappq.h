
#ifndef __SPARC64_SPLIT_PQ_H
#define __SPARC64_SPLIT_PQ_H


#include <rtems/score/chain.h>
#include <rtems/score/chain.inl>
#include <rtems/score/wkspace.h>

#include <rtems/rtems/types.h>

#include <rtems/score/sparc64.h>

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
  uint32_t key;
  uint32_t val;
  uint32_t heap_index;
} pq_node;

#ifdef __cplusplus
}
#endif

#endif
