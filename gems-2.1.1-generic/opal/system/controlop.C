/*
    Copyright (C) 1999-2008 by Mark D. Hill and David A. Wood for the
    Wisconsin Multifacet Project.  Contact: gems@cs.wisc.edu
    http://www.cs.wisc.edu/gems/

    --------------------------------------------------------------------

    This file is part of the Opal Timing-First Microarchitectural
    Simulator, a component of the Multifacet GEMS (General 
    Execution-driven Multiprocessor Simulator) software toolset 
    originally developed at the University of Wisconsin-Madison.

    Opal was originally developed by Carl Mauer based upon code by
    Craig Zilles.

    Substantial further development of Multifacet GEMS at the
    University of Wisconsin was performed by Alaa Alameldeen, Brad
    Beckmann, Jayaram Bobba, Ross Dickson, Dan Gibson, Pacia Harper,
    Derek Hower, Milo Martin, Michael Marty, Carl Mauer, Michelle Moravan,
    Kevin Moore, Andrew Phelps, Manoj Plakal, Daniel Sorin, Haris Volos, 
    Min Xu, and Luke Yen.
    --------------------------------------------------------------------

    If your use of this software contributes to a published paper, we
    request that you (1) cite our summary paper that appears on our
    website (http://www.cs.wisc.edu/gems/) and (2) e-mail a citation
    for your published paper to gems@cs.wisc.edu.

    If you redistribute derivatives of this software, we request that
    you notify us and either (1) ask people to register with us at our
    website (http://www.cs.wisc.edu/gems/) or (2) collect registration
    information and periodically send it to us.

    --------------------------------------------------------------------

    Multifacet GEMS is free software; you can redistribute it and/or
    modify it under the terms of version 2 of the GNU General Public
    License as published by the Free Software Foundation.

    Multifacet GEMS is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with the Multifacet GEMS; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307, USA

    The GNU General Public License is contained in the file LICENSE.

### END HEADER ###
*/
/*
 * FileName:  dynamic.C
 * Synopsis:  Represents an in-flight instruction in the processor
 * Description: Contains information related to an instruction being executed,
 *              which is distinct from the generic decoded instruction.
 * Author:    zilles
 * Maintainer:cmauer
 * Version:   $Id$
 */

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/
#include "hfa.h"
#include "hfacore.h"
#include "exec.h"
#include "controlop.h"

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Variable declarations                                                  */
/*------------------------------------------------------------------------*/

/// memory allocator for controlop.C
listalloc_t control_inst_t::m_myalloc;

/*------------------------------------------------------------------------*/
/* Forward declarations                                                   */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Constructor(s) / destructor                                            */
/*------------------------------------------------------------------------*/

//***************************************************************************
control_inst_t::control_inst_t( static_inst_t *s_inst, 
                                int32 window_index,
                                pseq_t *pseq,
                                abstract_pc_t *fetch_at,
                                pa_t physicalPC,
                                trap_type_t trapgroup,
                                uint32 proc)
  : dynamic_inst_t( s_inst, window_index, pseq, fetch_at, physicalPC, trapgroup, proc ) {

  m_isTaken =  false;
  m_actual  = *fetch_at;
  //also initialize predicted to be fetch_at
  //m_predicted = *fetch_at;

  /* Task: update the cwp for saves and restores
   *  Save/Restores (1) read from sources in old windows
   *                (2) write to dest in new windows
   * The source registers already have the cwp epoch '1'. We calculate 
   * the cwp for '2', and update the destination registers here.
   */
  if (s->getFlag( SI_UPDATE_CWP )) {
    uint16 cwp  = fetch_at->cwp;
    uint16 gset = fetch_at->gset;
    if (s->getOpcode() == i_save) {
      cwp++;
      cwp &= (NWINDOWS - 1);
    } else if (s->getOpcode() == i_restore) {
      cwp--;
      cwp &= (NWINDOWS - 1);
    } else {
      SIM_HALT;
    }
    for (int i = 0; i < SI_MAX_DEST; i++) {
      m_dest_reg[i].setVanillaState( cwp, gset );
      m_tofree_reg[i].setVanillaState( cwp, gset );
    }
  }
}

//***************************************************************************
control_inst_t::~control_inst_t() {
}

//**************************************************************************
void 
control_inst_t::Execute()
{
  STAT_INC( m_pseq->m_stat_control_exec[m_proc] );
  m_event_times[EVENT_TIME_EXECUTE_DONE] = m_pseq->getLocalCycle() - m_fetch_cycle;

  // call the appropriate function
  static_inst_t *si = getStaticInst();

  char buf[128];
  s->printDisassemble(buf);

  if (true) {

     #ifdef DEBUG_DYNAMIC
        char buf[128];
        s->printDisassemble(buf);
        DEBUG_OUT("[ %d ] control_inst_t: EXECUTE %s NAV[ %d ] seqnum[ %lld ] fetched[ %lld ] cycle[ %lld ]", m_pseq->getID(), buf, getInstrNAV(), seq_num, m_fetch_cycle, m_pseq->getLocalCycle());
        //print source and dest regs
        DEBUG_OUT(" SOURCES: ");
        for(int i=0; i < SI_MAX_SOURCE; ++i){
          reg_id_t & source = getSourceReg(i);
          if(!source.isZero()){
            DEBUG_OUT("( [%d] V: %d P: %d Arf: %s WriterSN: %lld WrittenCycle: %lld State: 0x%x)", i,source.getVanilla(), source.getPhysical(), source.rid_type_menomic( source.getRtype() ), source.getARF()->getWriterSeqnum( source, m_proc ), source.getARF()->getWrittenCycle( source, m_proc ), source.getVanillaState() );
          }
        }
        DEBUG_OUT(" DESTS: ");
        for(int i=0; i < SI_MAX_DEST; ++i){
          reg_id_t & dest = getDestReg(i);
          if(!dest.isZero()){
            DEBUG_OUT("( [%d] V: %d P: %d Arf: %s )", i,dest.getVanilla(), dest.getPhysical(), dest.rid_type_menomic( dest.getRtype() ));
          }
        }
        DEBUG_OUT("\n");
     #endif

    // execute the instruction using the jump table
    pmf_dynamicExecute pmf = m_jump_table[si->getOpcode()];
    (this->*pmf)();

     // Due to functional bugs sometimes m_actual.pc will be -1
     //ASSERT( m_actual.pc != (my_addr_t) -1 );

  } else {
    // NOT executed!
    dp_control_t dp_value;
    dp_value.m_at      = &m_actual;
    dp_value.m_taken   = false;
    dp_value.m_annul   = si->getAnnul();
    dp_value.m_offset  = si->getOffset();

    // do the operation
    exec_fn_execute( (i_opcode) si->getOpcode(), (dp_int_t *) &dp_value );
    
    // write result back to this dynamic instruction
    m_isTaken = dp_value.m_taken;
  }

  SetStage(COMPLETE_STAGE);

  #ifdef DEBUG_DYNAMIC
     char buf[128];
     s->printDisassemble(buf);
     DEBUG_OUT("control_inst_t: AFTER Execute %s NAV[ %d ] seqnum[ %lld ] fetched[ %lld ] cycle[ %lld ] PredPC[ 0x%llx ] ActualPC[ 0x%llx ] PredNPC[ 0x%llx ] ActualNPC[ 0x%llx ] PredCWP[ 0x%x ] ActualCWP[ 0x%x ] PredTL[ 0x%x ] ActualTL[ 0x%x ] PredPstate[ 0x%x ] ActualPstate[ 0x%x ]\n", buf, getInstrNAV(), seq_num, m_fetch_cycle, m_pseq->getLocalCycle(), m_predicted.pc, m_actual.pc, m_predicted.npc, m_actual.npc, m_predicted.cwp, m_actual.cwp, m_predicted.tl, m_actual.tl, m_predicted.pstate, m_actual.pstate);
  #endif

      
  // All control op should be checked, all of them can mis-predict
  // if the predicted PC, nPC pairs don't match with actual, cause exeception
  if ( (m_predicted.pc     != m_actual.pc ||
        m_predicted.npc    != m_actual.npc ||
        m_predicted.cwp    != m_actual.cwp ||
        m_predicted.tl     != m_actual.tl ||
        m_predicted.pstate != m_actual.pstate) ) {
    /* There was a branch mis-prediction --
     *    patch up branch predictor state */

     #ifdef DEBUG_DYNAMIC
         char buf[128];
         s->printDisassemble(buf);
         DEBUG_OUT("[ %d ] control_inst_t: MISPREDICT %s NAV[ %d ] seqnum[ %lld ] fetched[ %lld ] cycle[ %lld ] PredPC[ 0x%llx ] ActualPC[ 0x%llx ] PredNPC[ 0x%llx ] ActualNPC[ 0x%llx ] PredCWP[ 0x%x ] ActualCWP[ 0x%x ] PredTL[ 0x%x ] ActualTL[ 0x%x ] PredPstate[ 0x%x ] ActualPstate[ 0x%x ]\n", m_pseq->getID(), buf, getInstrNAV(), seq_num, m_fetch_cycle, m_pseq->getLocalCycle(), m_predicted.pc, m_actual.pc, m_predicted.npc, m_actual.npc, m_predicted.cwp, m_actual.cwp, m_predicted.tl, m_actual.tl, m_predicted.pstate, m_actual.pstate);
     #endif

    // This preformatted debugging information is left for your convenience
    // NOT very useful here, bc this might indicate a misprediction AFTER another misprediction
    // print this out inside Retire()!
    /*
    char buf[128];
    s->printDisassemble( buf );
      if (m_predicted.cwp != m_actual.cwp) {
      m_pseq->out_info("CONTROLOP: CWP mispredict: predicted[ %d ] actual[ %d ] type=%s seqnum[ %lld ]  VPC[ 0x%llx ] PC[ 0x%llx ] fetched[ %lld ] executed[ %lld ] correctPC[ 0x%llx ] %s\n",
      m_predicted.cwp, m_actual.cwp, pstate_t::branch_type_menomic[s->getBranchType()], seq_num, getVPC(), getPC(), m_fetch_cycle, m_event_times[EVENT_TIME_EXECUTE] + m_fetch_cycle, m_actual.pc, buf);
      }
      if (m_predicted.tl != m_actual.tl) {
      m_pseq->out_info("CONTROLOP: TL mispredict: predicted[ %d ] actual[ %d ] type=%s seqnum[ %lld ] VPC[ 0x%llx ] PC[ 0x%llx ] fetched[ %lld ] executed[ %lld ] correctPC[ 0x%llx ] %s\n",
      m_predicted.tl, m_actual.tl, pstate_t::branch_type_menomic[s->getBranchType()], seq_num, getVPC(), getPC(), m_fetch_cycle, m_event_times[EVENT_TIME_EXECUTE] + m_fetch_cycle, m_actual.pc, buf);
      }
      if (m_predicted.pstate != m_actual.pstate) {
      m_pseq->out_info("CONTROLOP: PSTATE mispredict: predicted[ 0x%0x ] actual[ 0x%0x ] type=%s seqnum[ %lld ] VPC[ 0x%llx ] PC[ 0x%llx ] fetched[ %lld ] executed[ %lld ] correctPC[ 0x%llx ] %s\n",
      m_predicted.pstate, m_actual.pstate, pstate_t::branch_type_menomic[s->getBranchType()], seq_num, getVPC(), getPC(), m_fetch_cycle,  m_event_times[EVENT_TIME_EXECUTE] + m_fetch_cycle, m_actual.pc, buf);
      }
    
    */

    markEvent(EVENT_BRANCH_MISPREDICT);
    if (s->getBranchType()      == BRANCH_COND ||
        s->getBranchType()      == BRANCH_PCOND ) {
      // flip the last bit of the history to correct the misprediction
      m_pred_state.cond_state = m_pred_state.cond_state ^ 1;
        
    } else if (s->getBranchType() == BRANCH_INDIRECT ||
               (s->getBranchType() == BRANCH_CALL && s->getOpcode() != i_call) ) {
      /*
        m_pseq->out_info(" INDIRECT:  predicted 0x%0llx, 0x%0llx\n",
        m_predicted.pc, m_predicted.npc);
        m_pseq->out_info(" INDIRECT:  actual    0x%0llx, 0x%0llx\n",
        m_actual.pc, m_actual.npc);
      */
      m_pseq->getIndirectBP()->FixupState(&(m_pred_state.indirect_state), 
                                          getVPC());
    }
    // no predictor fixup on PRIV, TRAP or TRAP_RETURN right now
    
    // TASK: should update the BTB here
    
    // TASK: determine the fetch location (and state)
    
    m_actual.gset = pstate_t::getGlobalSet( m_actual.pstate );
    
    /* cause a branch misprediction exception */
    m_pseq->raiseException(EXCEPT_MISPREDICT, getWindowIndex(),
                           (enum i_opcode) s->getOpcode(),
                           &m_actual, 0, BRANCHPRED_MISPRED_PENALTY, m_proc );
  }
}

//**************************************************************************
void 
control_inst_t::Squash() { 
  ASSERT( !getEvent( EVENT_FINALIZED ) );
  ASSERT(m_stage != RETIRE_STAGE);
  if (Waiting()) {
    RemoveWaitQueue();
  }
  UnwindRegisters( );
  m_pseq->decrementSequenceNumber(m_proc);  

  markEvent( EVENT_FINALIZED );
#ifdef PIPELINE_VIS
  m_pseq->out_log("squash %d\n", getWindowIndex());
#endif
}

//**************************************************************************
void
control_inst_t::Retire( abstract_pc_t *a )
{
  #ifdef DEBUG_DYNAMIC_RET
  DEBUG_OUT("control_inst_t:Retire BEGIN, proc[%d]\n",m_proc);
  #endif

  #ifdef DEBUG_DYNAMIC
  char buf[128];
  s->printDisassemble(buf);
  DEBUG_OUT("\tcontrol_inst_t::Retire BEGIN %s seq_num[ %lld ] cycle[ %lld ] fetchPC[ 0x%llx ] fetchNPC[ 0x%llx ] PredictedPC[ 0x%llx ] PredictedNPC[ 0x%llx ]\n", buf, seq_num, m_pseq->getLocalCycle(), m_actual.pc, m_actual.npc, m_predicted.pc, m_predicted.npc);
     for(int i=0; i < SI_MAX_DEST; ++i){
       reg_id_t & dest = getDestReg(i);
       reg_id_t & tofree = getToFreeReg(i);
       if( !dest.isZero() ){
         DEBUG_OUT("\tDest[ %d ] Vanilla[ %d ] Physical[ %d ] Arf[ %s ] ToFree: Vanilla[ %d ] physical[ %d ] Arf[ %s ]\n", i, dest.getVanilla(), dest.getPhysical(), dest.rid_type_menomic( dest.getRtype() ), tofree.getVanilla(), tofree.getPhysical(), tofree.rid_type_menomic( tofree.getRtype() ) );
       }
     }
  #endif

  ASSERT( !getEvent( EVENT_FINALIZED ) );
  STAT_INC( m_pseq->m_stat_control_retired[m_proc] );
  
  // Need this bc we place fetch barrier on retry instead of stxa:
  if ( (s->getOpcode() == i_retry) || (s->getFlag( SI_FETCH_BARRIER)) ) {
    // if we have a branch misprediction, we already unstalled the fetch in partialSquash():
    if(getEvent(EVENT_BRANCH_MISPREDICT) == false){
      m_pseq->unstallFetch(m_proc);   
    }
  }

  STAT_INC( m_pseq->m_branch_seen_stat[s->getBranchType()][2] );
  STAT_INC( m_pseq->m_branch_seen_stat[s->getBranchType()][m_priv? 1:0] );

  // record when execution takes place
  m_event_times[EVENT_TIME_RETIRE] = m_pseq->getLocalCycle() - m_fetch_cycle;
  
  // update dynamic branch prediction (predicated) instruction statistics
  static_inst_t *s_instr = getStaticInst();
  if (s_instr->getType() == DYN_CONTROL) {
    uint32 inst;
    int    op2;
    int    pred;
    switch (s_instr->getBranchType()) {
    case BRANCH_COND:
      // conditional branch
      STAT_INC( m_pseq->m_nonpred_retire_count_stat[m_proc] );
      break;
      
    case BRANCH_PCOND:
      // predictated conditional
      inst = s_instr->getInst();
      op2  = maskBits32( inst, 22, 24 );
      pred = maskBits32( inst, 19, 19 );
      if ( op2 == 3 ) {
        // integer register w/ predication
        STAT_INC( m_pseq->m_pred_reg_retire_count_stat[m_proc] );
        if (pred) {
          STAT_INC( m_pseq->m_pred_reg_retire_taken_stat[m_proc] );
        } else {
          STAT_INC( m_pseq->m_pred_reg_retire_nottaken_stat[m_proc] );
        }
      } else {
        STAT_INC( m_pseq->m_pred_retire_count_stat[m_proc] );
        if (pred) {
          STAT_INC( m_pseq->m_pred_retire_count_taken_stat[m_proc] );
        } else {
          STAT_INC( m_pseq->m_pred_retire_count_nottaken_stat[m_proc] );
        }
      }
      if (pred == true && m_isTaken == false ||
          pred == false && m_isTaken == true) {
        STAT_INC( m_pseq->m_branch_wrong_static_stat );
      }
      break;
      
    default:
      ;      // ignore non-predictated branches
    }
  }

#ifdef DEBUG_RETIRE
  m_pseq->out_info("## Control Retire Stage\n");
  printDetail();
  m_pseq->printPC( &m_actual );
#endif


  bool mispredict = (m_events & EVENT_BRANCH_MISPREDICT);
  if (mispredict) {

    // incorrect branch prediction
    STAT_INC( m_pseq->m_branch_wrong_stat[s->getBranchType()][2] );
    STAT_INC( m_pseq->m_branch_wrong_stat[s->getBranchType()][m_priv? 1:0] );

    //train BRANCH_PRIV predictor
    if( (s->getBranchType() == BRANCH_PRIV) ){
      if (m_predicted.cwp != m_actual.cwp) {
        m_pseq->getRegstatePred()->update(getVPC(), CONTROL_CWP, m_actual.cwp, m_predicted.cwp);
      }
      if (m_predicted.tl != m_actual.tl) {
        m_pseq->getRegstatePred()->update(getVPC(), CONTROL_TL, m_actual.tl, m_predicted.tl);
      }
      if (m_predicted.pstate != m_actual.pstate) {
        m_pseq->getRegstatePred()->update(getVPC(), CONTROL_PSTATE, m_actual.pstate, m_predicted.pstate);
      }
    }

    //****************************** print out mispredicted inst *****************
    if(s->getBranchType() == BRANCH_PRIV){
      char buf[128];
      s->printDisassemble( buf );
      bool imm = s->getFlag(SI_ISIMMEDIATE); 
      #if 0
        if(m_predicted.pc != m_actual.pc){
          m_pseq->out_info("CONTROLOP: PC mispredict: predicted[ 0x%llx ] actual[ 0x%llx ] type=%s seqnum[ %lld ]  VPC[ 0x%llx ] PC[ 0x%llx ] fetched[ %lld ] executed[ %lld ] correctPC[ 0x%llx ] imm[ %d ] %s\n",
                           m_predicted.pc, m_actual.pc, pstate_t::branch_type_menomic[s->getBranchType()], seq_num, getVPC(), getPC(), m_fetch_cycle, m_event_times[EVENT_TIME_EXECUTE] + m_fetch_cycle, m_actual.pc, imm, buf);
        }
      #endif
      #if 0
        if(m_predicted.npc != m_actual.npc){
          m_pseq->out_info("CONTROLOP: NPC mispredict: predicted[ 0x%llx ] actual[ 0x%llx ] type=%s seqnum[ %lld ]  VPC[ 0x%llx ] PC[ 0x%llx ] fetched[ %lld ] executed[ %lld ] correctPC[ 0x%llx ] imm[ %d ] %s\n",
                           m_predicted.npc, m_actual.npc, pstate_t::branch_type_menomic[s->getBranchType()], seq_num, getVPC(), getPC(), m_fetch_cycle, m_event_times[EVENT_TIME_EXECUTE] + m_fetch_cycle, m_actual.pc, imm, buf);
        }
      #endif
      #if 0
        if (m_predicted.cwp != m_actual.cwp) {
          m_pseq->out_info("CONTROLOP: CWP mispredict: predicted[ %d ] actual[ %d ] type=%s seqnum[ %lld ]  VPC[ 0x%llx ] PC[ 0x%llx ] fetched[ %lld ] executed[ %lld ] correctPC[ 0x%llx ] imm[ %d ] %s\n",
                           m_predicted.cwp, m_actual.cwp, pstate_t::branch_type_menomic[s->getBranchType()], seq_num, getVPC(), getPC(), m_fetch_cycle, m_event_times[EVENT_TIME_EXECUTE] + m_fetch_cycle, m_actual.pc, imm, buf);
        }
      #endif
      #if 0
        if (m_predicted.tl != m_actual.tl) {
          m_pseq->out_info("CONTROLOP: TL mispredict: predicted[ %d ] actual[ %d ] type=%s seqnum[ %lld ] VPC[ 0x%llx ] PC[ 0x%llx ] fetched[ %lld ] executed[ %lld ] correctPC[ 0x%llx ] imm[ %d ] %s\n",
                           m_predicted.tl, m_actual.tl, pstate_t::branch_type_menomic[s->getBranchType()], seq_num, getVPC(), getPC(), m_fetch_cycle, m_event_times[EVENT_TIME_EXECUTE] + m_fetch_cycle, m_actual.pc, imm, buf);
        }
      #endif
      #if 0
        if (m_predicted.pstate != m_actual.pstate) {
          m_pseq->out_info("CONTROLOP: PSTATE mispredict: predicted[ 0x%0x ] actual[ 0x%0x ] type=%s seqnum[ %lld ] VPC[ 0x%llx ] PC[ 0x%llx ] fetched[ %lld ] executed[ %lld ] correctPC[ 0x%llx ] imm[ %d ] %s\n",
                           m_predicted.pstate, m_actual.pstate, pstate_t::branch_type_menomic[s->getBranchType()], seq_num, getVPC(), getPC(), m_fetch_cycle,  m_event_times[EVENT_TIME_EXECUTE] + m_fetch_cycle, m_actual.pc, imm, buf);
        }
      #endif
    }
    //**************************************************************************

    // train ras's exception table
    if (ras_t::EXCEPTION_TABLE && s->getBranchType() == BRANCH_RETURN) {
      my_addr_t tos;
      ras_t *ras = m_pseq->getRAS(m_proc);    
      ras->getTop(&(m_pred_state.ras_state), &tos);
      if(tos == m_actual.npc) {
        ras->markException(m_predicted.npc);
        // update RAS state
        ras->pop(&(m_pred_state.ras_state));
        m_pseq->setSpecBPS(m_pred_state);
        if (ras_t::DEBUG_RAS) m_pseq->out_info("*********** DEBUG_RAS ***********\n");
      } else {
        ras->unmarkException(m_actual.npc);
      }
    }

    // XU DEBUG
    if (0 && s->getBranchType() == BRANCH_RETURN) {
      m_pseq->out_info("**** %c:mispred 0x%llx, pred 0x%llx target 0x%llx, "
                       "next %d TOS %d\n", m_priv? 'K':'U', getVPC(),
                       m_predicted.npc, m_actual.npc,
                       m_pred_state.ras_state.next_free,
                       m_pred_state.ras_state.TOS);

      // print 5 instr after mis-pred
      generic_address_t addr = m_predicted.npc-8;
      for(uint32 i = 0; i < 5; i++, addr+=4) {
        //get the actual seq number for the pointer to m_state:
        int32 seq_num = m_pseq->getID() / CONFIG_LOGICAL_PER_PHY_PROC;
        assert(seq_num >= 0 && seq_num < system_t::inst->m_numSMTProcs);
             
        tuple_int_string_t *disassemble = SIM_disassemble((processor_t *) system_t::inst->m_state[seq_num]->getSimicsProcessor(m_proc), addr, /* logical */ 1);
        if (disassemble) m_pseq->out_info("     %s\n", disassemble->string);
      }
    }
  } else {
    // correct or no prediction
    STAT_INC( m_pseq->m_branch_right_stat[s->getBranchType()][2] );
    STAT_INC( m_pseq->m_branch_right_stat[s->getBranchType()][m_priv? 1:0] );
  }

  /* update branch predictor tables at retirement */
  if (s->getBranchType() == BRANCH_COND ||
      s->getBranchType() == BRANCH_PCOND) {
    m_pseq->getDirectBP()->Update(getVPC(),
                                  (m_pseq->getArchBPS()->cond_state),
                                  s->getFlag( SI_STATICPRED ),
                                  m_isTaken, m_proc);

  } else if (s->getBranchType() == BRANCH_INDIRECT ||
             (s->getBranchType() == BRANCH_CALL && s->getOpcode() != i_call) ) {
    // m_actual.npc is the indirect target
    m_pseq->getIndirectBP()->Update( getVPC(),
                                     &(m_pseq->getArchBPS()->indirect_state),
                                     m_actual.npc, m_proc );
  }

  m_pseq->setArchBPS(m_pred_state);
  
  // no need to update call&return, since we used checkpointed RAS
  // no update on traps right now
  SetStage(RETIRE_STAGE);

  /* retire any register overwritten by link register */
  retireRegisters(); 

  #if 0
  if(m_actual.pc == (my_addr_t) -1){
    char buf[128];
    s->printDisassemble(buf);
    ERROR_OUT("\tcontrol_inst_t::Retire %s seq_num[ %lld ] cycle[ %lld ] fetchPC[ 0x%llx ] fetchNPC[ 0x%llx ] fetched[ %lld ] executed[ %lld ]\n", buf, seq_num, m_pseq->getLocalCycle(), m_actual.pc, m_actual.npc, m_fetch_cycle, m_fetch_cycle+getEventTime(EVENT_TIME_EXECUTE_DONE));
    //print out writer cycle
    for(int i=0; i < SI_MAX_SOURCE; ++i){
      reg_id_t & source = getSourceReg(i);
      if(!source.isZero()){
        uint64 written_cycle = source.getARF()->getWrittenCycle( source, m_proc );
        uint64 writer_seqnum = source.getARF()->getWriterSeqnum( source, m_proc );
        ERROR_OUT("\tSource[ %d ] Vanilla[ %d ] Physical[ %d ] Arf[ %s ] written_cycle[ %lld ] writer_seqnum[ %lld ]\n", i, source.getVanilla(), source.getPhysical(), source.rid_type_menomic( source.getRtype() ), written_cycle, writer_seqnum);
      }
    }
  }
  #endif

  ASSERT( m_actual.pc != (my_addr_t) -1 );

  /* return pc, npc pair which are the results of execution */
  a->pc  = m_actual.pc;
  a->npc = m_actual.npc;
 
  markEvent( EVENT_FINALIZED );

  #ifdef DEBUG_DYNAMIC_RET
     DEBUG_OUT("control_inst_t:Retire END, proc[%d]\n",m_proc);
  #endif
}

//***************************************************************************
void
control_inst_t::printDetail( void )
{
  DEBUG_OUT( "control_inst_t\n");
  DEBUG_OUT( "type    : %s\n",
                    pstate_t::branch_type_menomic[s->getBranchType()] );
  DEBUG_OUT("offset  : 0x%0llx\n", s->getOffset());
  DEBUG_OUT("isTaken : %d\n", m_isTaken);
  DEBUG_OUT("Predicted Target :\n");
  m_pseq->printPC( &m_predicted );
  DEBUG_OUT("Actual Target :\n");
  m_pseq->printPC( &m_actual );
}

