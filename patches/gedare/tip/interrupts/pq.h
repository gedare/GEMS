
#ifndef __SPARC64_PQ_H
#define __SPARC64_PQ_H


#include <rtems/score/chain.h>
#include <rtems/score/chain.inl>
#include <rtems/score/wkspace.h>

#include <rtems/rtems/types.h>

#include <rtems/score/sparc64.h>

#ifdef __cplusplus
extern "C" {
#endif

// should be consistent with hwpq-decoder
// TODO: make configurable
#define QUEUE_SIZE (101)

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

void sparc64_hwpq_initialize(void);

void hwpq_drain_queue( int qid );

// formerly sparc64_interrupt_1
void sparc64_hwpq_spill_fill(uint64_t vector, CPU_Interrupt_frame *istate);

// formerly sparc64_interrupt_3
void sparc64_hwpq_software_extract(uint64_t vector, CPU_Interrupt_frame *istate);

#ifdef __cplusplus
}
#endif

#endif
