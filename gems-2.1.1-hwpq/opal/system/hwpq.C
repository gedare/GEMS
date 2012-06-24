/*
 * FileName:  hwpq.C
 * Synopsis:  Implements a hardware priority queue
 * Author:    gedare
 * Version:   $Id$
 */

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include "hfa.h"
#include "hfacore.h"

#include "hwpq.h"
#include <stdio.h>
#include <vector>

/// memory allocator for hwpq.C
listalloc_t hwpq_inst_t::m_myalloc;

hwpq_inst_t::hwpq_inst_t( static_inst_t *s_inst,
      int32 window_index,
      pseq_t *pseq,
      abstract_pc_t *at,
      pa_t physicalPC,
      trap_type_t trapgroup,
      uint32 proc) 
  : dynamic_inst_t( s_inst, window_index, pseq, at, physicalPC, trapgroup, proc) {
  // default dynamic_inst_t constructor
  m_type = HWDS_INST_UNUSED;
  m_queue_idx = 0;
}

hwpq_inst_t::~hwpq_inst_t() {

}

void hwpq_inst_t::Squash() {
  STAT_INC( m_pseq->m_stat_hwds_squashed[m_proc][0] );
  // sanity
  ASSERT(m_proc < CONFIG_LOGICAL_PER_PHY_PROC);
  ASSERT( !getEvent( EVENT_FINALIZED ) );
  ASSERT(m_stage != RETIRE_STAGE);

  // only know which type of operation it is after Execute
  // (Could decode but why?)
  if ( m_stage == COMPLETE_STAGE )
    STAT_INC( m_pseq->m_stat_hwds_squashed[m_proc][m_type] );

  // adjust any of the 'soft' state of this instruction

  // generic dynamic_inst_t::Squash() actions
  if (Waiting())
    RemoveWaitQueue();

  UnwindRegisters();

  m_pseq->decrementSequenceNumber(m_proc);

  markEvent( EVENT_FINALIZED );
#ifdef PIPELINE_VIS
    m_pseq->out_log("squash %d\n", getWindowIndex());
#endif
}

void hwpq_inst_t::Retire( abstract_pc_t *a ) {
  uint latency = 0;
  char buf[101];
  STAT_INC( m_pseq->m_stat_hwds_retired[m_proc][0] );

  // generic dynamic_inst_t::Retire() actions
  // record when execution takes place
  m_event_times[EVENT_TIME_RETIRE] = m_pseq->getLocalCycle() - m_fetch_cycle;
  latency = m_event_times[EVENT_TIME_RETIRE];
  //m_pseq->m_stat_hwds_cycles[m_proc][m_type] += latency;

  /*
  sprintf(buf, "hwds %d %s instruction latency: ",
      m_queue_idx,
      m_pseq->stat_hwds_mnemonic(m_type));
  m_pseq->out_info("  %-50.50s %10u\n", buf, latency );
  */

  ASSERT( !getEvent( EVENT_FINALIZED ) );
  
  ASSERT( s->getFlag( SI_FETCH_BARRIER ) );
  m_pseq->unstallFetch(m_proc);   
  
  STAT_INC( m_pseq->m_stat_hwds_retired[m_proc][m_type] );

  retireRegisters();
  SetStage(RETIRE_STAGE);
  nextPC_execute( a );
  markEvent( EVENT_FINALIZED );
}

// dx.i macros
#define IREGISTER ireg_t

#define SOURCE1 getSourceReg(0).getARF()->readInt64( getSourceReg(0), getProc() )

#define SOURCE2 getSourceReg(1).getARF()->readInt64( getSourceReg(1), getProc() )

#define S2ORI (getStaticInst()->getFlag(SI_ISIMMEDIATE) ? \
    getStaticInst()->getImmediate() :   \
    SOURCE2)

// slightly modified
#define WRITE_DEST(A) \
  getDestReg(0).getARF()->writeInt64( getDestReg(0), (A), getProc() ); 

void hwpq_inst_t::Execute() {

  uint32 queue_idx = 0;
  int operation = 0;
  int value = 0;

  STAT_INC( m_pseq->m_stat_hwds_executed[m_proc][0] );

  // generic dynamic_inst_t::Execute() actions
  // record when execution takes place
  m_event_times[EVENT_TIME_EXECUTE_DONE] = 
    m_pseq->getLocalCycle() - m_fetch_cycle;

  static_inst_t *si = getStaticInst();
  queue_idx = maskBits32(S2ORI,31,20);
  value = maskBits64(SOURCE1,63,32);
  operation = maskBits32(S2ORI,16,0);

  // check the immediate bit: hwds instructions use 2 source regs
  if (si->getFlag(SI_ISIMMEDIATE)) {
    DEBUG_OUT("Unimplemented instruction: 0x%0x\n", si->getInst());
    goto out;
  }
  if (queue_idx > CONFIG_NUM_HWPQS) {
    DEBUG_OUT("Invalid queue_idx: %d\n", queue_idx);
    DEBUG_OUT("Instruction: 0x%0x\n", getStaticInst()->getInst());
    goto out;
  }
  if (operation > HWDS_NUM_INST_TYPES) {
    DEBUG_OUT("Unknown operation: %d\n", operation);
    goto out;
  }

  m_queue_idx = queue_idx;
  m_type = (enum hwds_inst_type_t)operation;

  STAT_INC( m_pseq->m_stat_hwds_executed[m_proc][m_type] );

out:
  WRITE_DEST(0);
  SetStage(COMPLETE_STAGE);
}

