
#include "hwpq_exceptions.h"
#include "spillpq.h"
#include <rtems/bspIo.h>

void sparc64_hwpq_initialize()
{
  proc_ptr old;

  _CPU_ISR_install_vector(
    SPARC_SYNCHRONOUS_TRAP(0x41),
    sparc64_hwpq_spill_fill,
    &old
   );

  _CPU_ISR_install_vector(
    SPARC_SYNCHRONOUS_TRAP(0x43),
    sparc64_hwpq_software_extract,
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

  // acknowledge the interrupt (allows isr to schedule another at this level)
  sparc64_clear_interrupt_bits_const(mask);
  //FIXME: Race condition against tick interrupts -- scheduling issue.

  level = sparc_disable_interrupts();

  // get the interrupted state
  HWDS_GET_CONTEXT(context);

  // parse context
  queue_idx = (((uint32_t)(context>>32))&(~0))>>20;
  operation = (((uint32_t)(context>>32))&~(~0 << (3 + 1)));

  // TODO: currently not using the HWDS cache? :(

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
      // FIXME: this is happening in dimacs, not sure why, but the operations
      // are spill (7) or fill (8). Doing nothing seems to work though.
//      printk("Unknown operation (RTEMS): %d\n", operation);
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

  queue_idx = (((uint32_t)(context>>32))&(~0))>>20;
  //operation = (((uint32_t)(context>>32))&~(~0 << (3 + 1)));

  status = sparc64_spillpq_handle_extract(queue_idx);

  sparc64_clear_interrupt_bits_const(mask);
  if ( !status )
    HWDS_CHECK_UNDERFLOW(queue_idx);

  sparc_enable_interrupts(level);
}
