
#include "hwpq_exceptions.h"
#include "spillpq.h"
#include <rtems/bspIo.h>

void sparc64_hwpq_initialize()
{
  proc_ptr old;

  _CPU_ISR_install_vector(
    SPARC_ASYNCHRONOUS_TRAP(0x41),
    sparc64_hwpq_spill_fill,
    &old
   );

  _CPU_ISR_install_vector(
    SPARC_ASYNCHRONOUS_TRAP(0x43),
    sparc64_hwpq_software_extract,
    &old
   );

  _CPU_ISR_install_vector(
    SPARC_ASYNCHRONOUS_TRAP(0x44),
    sparc64_hwpq_context_switch,
    &old
   );

}

void sparc64_hwpq_drain_queue( int qid ) {
  sparc64_spillpq_drain(qid);
}

void sparc64_hwpq_spill_fill(uint64_t vector, CPU_Interrupt_frame *istate)
{
  uint32_t level;
  uint64_t context;
  uint32_t queue_idx;
  uint32_t operation;
  const int mask = (1<<1);


  level = sparc_disable_interrupts();

  // acknowledge the interrupt (allows isr to schedule another at this level)
  sparc64_clear_interrupt_bits_const(mask);
  //FIXME: Race condition against tick interrupts -- scheduling issue.

  // get the interrupted state
  HWDS_GET_CONTEXT(context);

  // parse context
  queue_idx = (((uint32_t)(context))&(~0))>>20;
  //queue_idx = (((uint32_t)(context>>32))&(~0))>>20;
  //operation = (((uint32_t)(context>>32))&~(~0 << (3 + 1)));
  operation = (((uint32_t)(context))&~(~0 << (3 + 1)));

  // TODO: should check exception conditions, since some operations can
  // cause multiple exceptions.

  switch (operation) {
    case 2:
      sparc64_spillpq_handle_spill(queue_idx);
      break;

    case 3: case 6: case 11:
      sparc64_spillpq_handle_fill(queue_idx);
      break;

    default:
      printk("Unknown operation (RTEMS): %d\n", operation);
      break;
  }

  sparc_enable_interrupts(level);
}

void sparc64_hwpq_software_extract(uint64_t vector, CPU_Interrupt_frame *istate)
{
  const int mask = (1<<3);
  uint32_t level;
  uint64_t context;
  uint32_t queue_idx;
  int status = 0;
  //uint32_t operation;

  level = sparc_disable_interrupts();

  // get the interrupted state
  HWDS_GET_CONTEXT(context);

  //queue_idx = (((uint32_t)(context>>32))&(~0))>>20;
  queue_idx = (((uint32_t)(context))&(~0))>>20;
  //operation = (((uint32_t)(context>>32))&~(~0 << (3 + 1)));

  status = sparc64_spillpq_handle_extract(queue_idx);

  sparc64_clear_interrupt_bits_const(mask);
  if ( !status )
    HWDS_CHECK_UNDERFLOW(queue_idx);

  sparc_enable_interrupts(level);
}

void sparc64_hwpq_context_switch(uint64_t vector, CPU_Interrupt_frame *istate)
{
  const int mask = (1<<4);
  uint32_t level;
  uint64_t context;
  uint64_t queue_idx;
  int status = 0;
  uint32_t trap_context;
  uint32_t trap_idx;
  uint32_t trap_operation;

  level = sparc_disable_interrupts();

  // get the interrupted state
  HWDS_GET_CONTEXT(context);

  //queue_idx = (((uint32_t)(context>>32))&(~0))>>20;
  HWDS_GET_CURRENT_ID(queue_idx);
  //operation = (((uint32_t)(context>>32))&~(~0 << (3 + 1)));

  trap_context = (uint32_t)context;
  trap_idx = ((trap_context)&(~0))>>20;
//  trap_operation = (trap_context)&~(~0 << (3 + 1));


  status = sparc64_spillpq_context_switch(queue_idx);

  HWDS_SET_CURRENT_ID(trap_idx);

  // FIXME: emulate trapping instruction?
  // The hardware currently executes the trapping instruction somewhat
  // as though a 1 instruction buffer were available, but this only works
  // for insert instructions, since pop/extract/first instructions do not have
  // the data available if the queue was previously flushed. pop/first is
  // accomodated by storing the head of every queue in a buffer. extract 
  // fail-over to software extract and do not cause a context switch.
  

  // should be able to demand fetch, but the queue is empty or invalid
  // and the current hardware simulation does not handle it gracefully.
  sparc64_spillpq_handle_fill(trap_idx);

  sparc64_clear_interrupt_bits_const(mask);

  sparc_enable_interrupts(level);
}
