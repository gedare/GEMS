
#ifndef __SPARC64_HWPQ_EXCEPTIONS_H
#define __SPARC64_HWPQ_EXCEPTIONS_H

#include <rtems/rtems/types.h> 
#include <rtems/score/sparc64.h>

#ifdef __cplusplus
extern "C" {
#endif

void sparc64_hwpq_initialize(void);

// formerly sparc64_interrupt_1
void sparc64_hwpq_spill_fill(uint64_t vector, CPU_Interrupt_frame *istate);

// formerly sparc64_interrupt_3
void sparc64_hwpq_software_extract(uint64_t vector, CPU_Interrupt_frame *istate);

void sparc64_hwpq_drain_queue( int qid );

#ifdef __cplusplus
}
#endif

#endif

