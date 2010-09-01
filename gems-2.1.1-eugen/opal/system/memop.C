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
 * FileName:  memop.C
 * Synopsis:  dynamic memory operations (loads / stores)
 * Author:    zilles
 * Version:   $Id$
 */

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include "hfa.h"
#include "hfacore.h"

#include "scheduler.h"
#include "iwindow.h"
#include "lsq.h"
#include "pipestate.h"
#include "rubycache.h"
#include "writebuffer.h"
#include "memtrace.h"
#include "memop.h"

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Variable declarations                                                  */
/*------------------------------------------------------------------------*/

/// memory allocator for memop.C
listalloc_t load_inst_t::m_myalloc;
listalloc_t store_inst_t::m_myalloc;
listalloc_t atomic_inst_t::m_myalloc;
listalloc_t prefetch_inst_t::m_myalloc;

/*------------------------------------------------------------------------*/
/* Forward declarations                                                   */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Constructor(s) / destructor                                            */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Public methods                                                         */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* memory_inst                                                            */
/*------------------------------------------------------------------------*/

//***************************************************************************
memory_inst_t::memory_inst_t( static_inst_t *s_inst, 
                              int32 window_index,
                              pseq_t *pseq,
                              abstract_pc_t *at,
                              pa_t physicalPC,
                              trap_type_t trapgroup,
                              uint32 proc)
  : dynamic_inst_t( s_inst, window_index, pseq, at, physicalPC, trapgroup, proc) {

  // use default dynamic inst constructor
  lsq_prev = NULL;
  lsq_next = NULL;
  m_address       = (my_addr_t) -1;
  m_physical_addr = (my_addr_t) -1;
  setAccessSize( s->getAccessSize() );
  m_data_valid    = false;
  m_asi    = (uint16) -1;
  m_tl     = at->tl;
  m_pstate = at->pstate;

  //initlaize data storage
  for(int i=0; i < MEMOP_MAX_SIZE; ++i){
    m_data_storage[i] = 0;
    m_retirement_data_storage[i] = 0;
  }

  m_addr_valid = false;

  //reset retirement permission flag
  m_retirement_permission = false;
}

//***************************************************************************
memory_inst_t::~memory_inst_t() {

}

//**************************************************************************
void
memory_inst_t::Squash() {
  ASSERT(m_proc < CONFIG_LOGICAL_PER_PHY_PROC);
  ASSERT( !getEvent( EVENT_FINALIZED ) );
  ASSERT(m_stage != RETIRE_STAGE);
  if (getEvent(EVENT_LSQ_INSERT)) { 
    m_pseq->getLSQ(m_proc)->remove( this );

    /* WATTCH power */
    if(WATTCH_POWER){
      m_pseq->getPowerStats()->incrementLSQAccess();
    }
  }
  if (getEvent(EVENT_MSHR_STALL)) { 
    m_pseq->getRubyCache()->squashInstruction( this );
  }
  
  // SC_Checker squash() goes here
//   if( CONFIG_WITH_RUBY ) {
//     mf_ruby_api_t *ruby_api = system_t::inst->m_ruby_api;
//     (*ruby_api->sc_Checker_Squash)( m_physical_addr, getSequenceNumber(), m_pseq->getID()/CONFIG_LOGICAL_PER_PHY_PROC );
//   }
  
#ifdef EXPENSIVE_ASSERTIONS
  m_pseq->getLSQ(m_proc)->verifyQueues();
#endif
  if (Waiting()) { 
    RemoveWaitQueue(); 
}

  UnwindRegisters(); 

  m_pseq->decrementSequenceNumber(m_proc);  

  markEvent( EVENT_FINALIZED );
#ifdef PIPELINE_VIS
  m_pseq->out_log("squash %d\n", getWindowIndex());
#endif
}

//**************************************************************************
void
memory_inst_t::Retire( abstract_pc_t *a )
{

  // SC_Checker Retire() goes here
 //  if(CONFIG_WITH_RUBY) {
//     mf_ruby_api_t *ruby_api = system_t::inst->m_ruby_api;
  
//     if(isCacheable()) {
//       (*ruby_api->sc_Checker_Retire)( m_physical_addr, getSequenceNumber(), m_pseq->getID()/CONFIG_LOGICAL_PER_PHY_PROC );      
//     }
//   }
  
  #ifdef DEBUG_DYNAMIC_RET
    DEBUG_OUT("memory_inst_t:Retire BEGIN, proc[%d]\n",m_proc);
  #endif

  if( getEvent( EVENT_FINALIZED) ){
    char buf[128];
    s->printDisassemble(buf);
    ERROR_OUT("memory_inst_t:Retire() INST ALREADY FINALIZED %s cycle[ %lld ] seqnum[ %lld ] fetched[ %lld ]\n", buf, m_pseq->getLocalCycle(), seq_num, m_fetch_cycle);
  }
  ASSERT( !getEvent( EVENT_FINALIZED ) );
#ifdef DEBUG_RETIRE
  m_pseq->out_log("## Memory Retire Stage\n");
  printDetail();
  m_pseq->out_log("   asi   : 0x%0x\n", m_asi );
  m_pseq->out_log("   pstate: 0x%0x\n", m_pstate );
  m_pseq->out_log("   tl    : 0x%0x\n", m_tl );
#endif

  // record when execution takes place
  m_event_times[EVENT_TIME_RETIRE] = m_pseq->getLocalCycle() - m_fetch_cycle;
  
  // unstall fetch if this instruction is has in-order pipeline semantics
  // IMPORTANT: we need to stall front-end on stxa instruction (for TLB misses)
  if ( s->getFlag( SI_FETCH_BARRIER  ) ) {
    m_pseq->unstallFetch(m_proc);   
  }
  // This is related to the stxa instruction special case
  //      "stxa" instructions can be writing to the MMU to map or demap pages.
  //      They can't be executed in a pipelined manner-- if one executes past
  //      them (and speculates on "retry"), instructions that caused TLB misses
  //      will miss in the TLB again, instead of succeeding. By creating
  //      a dependency on the control registers, we block subsequent
  //      instructions from executing before the stxa is completed.
  //
  //      Here is where the stxa completes, hence the release of the
  //      control dependence.
  if ( getDestReg(1).getRtype() == RID_CONTROL ) {
    if (!getDestReg(1).getARF()->isReady( getDestReg(1), m_proc )) {
      getDestReg(1).getARF()->initializeControl( getDestReg(1), m_proc );    
      getDestReg(1).getARF()->finalizeControl( getDestReg(1), m_proc );     
    }
  }

  if (getEvent(EVENT_LSQ_INSERT)) { 
    m_pseq->getLSQ(m_proc)->remove( this );

    /* WATTCH power */
    if(WATTCH_POWER){
      m_pseq->getPowerStats()->incrementLSQAccess();
    }
  }
#ifdef EXPENSIVE_ASSERTIONS
  m_pseq->getLSQ(m_proc)->verifyQueues();
#endif

  retireRegisters();
  SetStage(RETIRE_STAGE);
  nextPC_execute( a );
  markEvent( EVENT_FINALIZED );

  #ifdef DEBUG_DYNAMIC_RET
    DEBUG_OUT("memory_inst_t:Retire END, proc[%d]\n",m_proc);
  #endif
}

//**************************************************************************
// Marks the destination registers as having been L2 miss, so
//  dependents can see this
void
memory_inst_t::markL2Miss(void){
  for (int i = 0; i < SI_MAX_DEST; i++) {
    reg_id_t &dest    = getDestReg(i);
    // if the register isn't the %g0 register --
    if (!dest.isZero()) {
      dest.getARF()->setL2Miss( dest, m_proc );
    }
  }
  //mark this instruction as having L2Miss
  markEvent( EVENT_L2_MISS );
}

//**************************************************************************
void
memory_inst_t::Execute( void )
{
  // record when execution takes place
  m_event_times[EVENT_TIME_EXECUTE_DONE] = m_pseq->getLocalCycle() - m_fetch_cycle;
  
  // call the appropriate function
  static_inst_t *si = getStaticInst();

  //print out the register that are written to
  #if 0
  char buf[128];
  s->printDisassemble(buf);
  DEBUG_OUT("memory_inst_t:: Execute, about to write to regs, cycle[ %lld ] seqnum[ %lld ] %s physaddr[ 0x%llx ]\n", m_pseq->getLocalCycle(), seq_num, buf, m_physical_addr);
  for(int i=0; i < SI_MAX_DEST; ++i){
    reg_id_t dest = getDestReg(i);
    if( !dest.isZero() ){
      DEBUG_OUT("\tDest[ %d ] Vanilla[ %d ] Physical[ %d ] Arf[ %s ]\n", i, dest.getVanilla(), dest.getPhysical(), dest.rid_type_menomic( dest.getRtype() ) );
    }
  }
  #endif

  // NOT true, since uncacheable instr also execute!
  //  ASSERT(m_physical_addr != (my_addr_t ) -1 );

  // execute the instruction using the jump table
  pmf_dynamicExecute pmf = m_jump_table[si->getOpcode()];
  (this->*pmf)();

#if 0
  if (!si->getFlag( SI_COMPLEX_OP )) {
    if (si->getType() == DYN_LOAD) {
      my_register_t result;
      if (si->getAccessSigned()) {
        result.uint_64 = getSignedData();
      } else {
        result.uint_64 = getUnsignedData();
      }
      getDestReg(0).getARF()->writeRegister( getDestReg(0), result, m_proc );   
    } else if (si->getType() == DYN_STORE) {
      if ( getTrapType() == Trap_Unimplemented ) {
        setTrapType( Trap_NoTrap );
      }
    } else if (si->getType() == DYN_PREFETCH) {
      // no impact on architected state
    } else if (si->getType() == DYN_ATOMIC) {
      m_pseq->out_info("warning: memory_inst_t: Execute: atomic instruction!\n");
      printDetail();
    } else {
      // print a warning
      m_pseq->out_info("warning: memory_inst_t: Execute: unable to determine memory type\n");
      printDetail();
    }
  }
#endif

  // check that this instruction has written its destination registers
  if ( getTrapType() == Trap_NoTrap ) {
    ASSERT( getDestReg(0).getARF()->isReady( getDestReg(0), m_proc ));
    
    // stxa's write the control registers at retire time (when they become 
    // non-speculative). So they need a conditional check.
    // See memory_inst_t::Retire() for more information (9/12/2002)
    if (getStaticInst()->getOpcode() != i_stxa) {
      if( getDestReg(1).getARF()->isReady( getDestReg(1), m_proc ) == false ){
        printInstruction();
      }
      ASSERT( getDestReg(1).getARF()->isReady( getDestReg(1), m_proc ));
    }
  }
  SetStage(COMPLETE_STAGE);
}

//**************************************************************************
trap_type_t
memory_inst_t::addressGenerate( OpalMemop_t accessType )
{ 
  //
  // calculate the virtual address being accessed
  //
  if ( s->getFlag(SI_ISIMMEDIATE) ) {
    m_address = getSourceReg(0).getARF()->readInt64(getSourceReg(0), m_proc) + 
      s->getImmediate();
  } else {
    m_address = getSourceReg(0).getARF()->readInt64(getSourceReg(0), m_proc) + 
      getSourceReg(1).getARF()->readInt64(getSourceReg(1), m_proc);
  }

  // get the ASI for this memory operation
  setASI();
  
  // if a TLB access: return immediately
  //                : no corresponding physical address!
  if ( !isCacheable() ) {
    m_physical_addr = (my_addr_t) -1;
#ifdef DEBUG_LSQ
    m_pseq->out_log("Address Generate: index:%d type:%d asi:%d address:0x%0llx uncacheable\n",
                    getWindowIndex(), accessType, m_asi, m_address);
#endif
    return Trap_NoTrap;
  }

  // check that this address is aligned with respect to the access size
  if ( (m_address & (m_access_size - 1)) != 0 ) {
#ifdef DEBUG_LSQ
    DEBUG_OUT("ADDRESS NOT ALIGNED: 0x%0llx (%d) %d\n", m_address, m_access_size, getWindowIndex());
    if (s->getFlag(SI_ISIMMEDIATE)) {
      DEBUG_OUT("ADDRESS: 0x%0llx 0x%0llx\n", getSourceReg(0).getARF()->readInt64(getSourceReg(0), m_proc), s->getImmediate());
    } else {
      DEBUG_OUT("ADDRESS: 0x%0llx 0x%0llx\n", getSourceReg(0).getARF()->readInt64(getSourceReg(0), m_proc), getSourceReg(1).getARF()->readInt64(getSourceReg(1), m_proc) );
    }
#endif
    return Trap_Mem_Address_Not_Aligned;
  }
  
  /* Access the TLB */
  //**************************************************************************
  trap_type_t t = Trap_NoTrap;
  if ( CONFIG_IN_SIMICS ) {
    // access the mmu
    t = m_pseq->mmuAccess( m_address, m_asi, m_access_size, accessType,
                           m_pstate, m_tl, &m_physical_addr, m_proc, true );   
  } else {
    // if in stand alone mode: scan the transaction log for a hit
    memtrace_t *mt = m_pseq->getMemtrace(m_proc);   
    mt->scanTransaction( m_pseq->getRetiredICount(m_proc) + 2,
                         m_address, getVPC(),
                         s->getInst(), m_physical_addr,
                         m_access_size, NULL );
    if (m_physical_addr == (pa_t) -1) {
      return Trap_Data_Access_Mmu_Miss;
    }
  }

#ifdef DEBUG_LSQ
  m_pseq->out_log("Address Generate: index:%d type:%d v:0x%0llx p:0x%0llx 0x%0llx size=%d pstate=0x%0x asi=0x%0x result=0x%0x\n",
                  getWindowIndex(), accessType, m_address, m_physical_addr,
                  (m_physical_addr >> DL1_BLOCK_BITS), m_access_size,
                  m_pstate, m_asi, (int) t);
#endif

  return t;
}

//**************************************************************************
trap_type_t
memory_inst_t::mmuRegisterAccess( bool isaWrite )
{
  mmu_interface_t *asi_interface = m_pseq->getASIinterface(m_proc);  
  memory_transaction_t mem_op;
  exception_type_t e = Sim_PE_No_Exception;

  // sanity check the mmu address. All MMU registers are < 0x40
  // (possibly < 0x1F8)
  if ( m_address > 0x1F8 ) {
    if(s->getOpcode() == i_ldxa){
      STAT_INC(m_pseq->m_stat_va_out_of_range);
    }
    return ( Trap_Unimplemented );
  }

  if(CONFIG_IN_SIMICS){
    int32 seq_num = m_pseq->getID() / CONFIG_LOGICAL_PER_PHY_PROC;
    ASSERT(seq_num >= 0 && seq_num < system_t::inst->m_numSMTProcs);
    
    bool error = false;
    ireg_t value = 0;
    error = system_t::inst->m_state[seq_num]->getMMURegisterValue( m_asi, m_address, m_proc, value);
    if( error == false ){
      m_data_storage[0] = value;
      //IMPORTANT: must set data valid flag so that it will be written to dest reg
      m_data_valid = true;
      #ifdef DEBUG_DYNAMIC
         DEBUG_OUT("\tmmuRegisterAccess SUCCESS ASI[ 0x%x ] VA[ 0x%llx ] value[ 0x%llx ]\n", m_asi, m_address, m_data_storage[0]);
      #endif
      return Trap_NoTrap;
    }
    else{
      ERROR_OUT("\tmmuRegisterAccess FAIL ASI[ 0x%x ] VA[ 0x%llx ] value[ 0x%llx ] cycle[ %lld ]\n", m_asi, m_address, m_data_storage[0], m_pseq->getLocalCycle());
    }
  }
  //either not in Simics or failed
  return Trap_Unimplemented;
}

//********************************************************************************
trap_type_t
memory_inst_t::readControlReg()
{
    if(CONFIG_IN_SIMICS){
    int32 seq_num = m_pseq->getID() / CONFIG_LOGICAL_PER_PHY_PROC;
    ASSERT(seq_num >= 0 && seq_num < system_t::inst->m_numSMTProcs);
    
    bool error = false;
    ireg_t value = 0;
    error = system_t::inst->m_state[seq_num]->readControlReg( m_asi, m_address, m_proc, value);
    if( error == false ){
      m_data_storage[0] = value;
      //IMPORTANT: must set data valid flag so that it will be written to dest reg
      m_data_valid = true;
      #ifdef DEBUG_DYNAMIC
         DEBUG_OUT("\treadControlReg SUCCESS ASI[ 0x%x ] VA[ 0x%llx ] value[ 0x%llx ]\n", m_asi, m_address, m_data_storage[0]);
      #endif
      return Trap_NoTrap;
    }
    else{
      ERROR_OUT("\treadControlReg FAIL ASI[ 0x%x ] VA[ 0x%llx ] value[ 0x%llx ] cycle[ %lld ]\n", m_asi, m_address, m_data_storage[0], m_pseq->getLocalCycle());
    }
  }
  //either not in Simics or failed
  return Trap_Unimplemented;
}

//***************************************************************************
uint16
memory_inst_t::addressOverlap( memory_inst_t *other )
{
  uint16  isOverlap = 0;

  /* an access to the same cache line.  Check for overlap between accesses */
  my_addr_t  my_cacheline    = getLineAddress();
  my_addr_t  other_cacheline = other->getLineAddress();
  if (my_cacheline == other_cacheline) { 
    int my_offset    = m_physical_addr & MEMOP_BLOCK_MASK;
    int other_offset = other->m_physical_addr & MEMOP_BLOCK_MASK;
    
#ifdef DEBUG_LSQ
    m_pseq->out_log("address overlap 0x%0llx ?= 0x%0llx\n",
                     m_physical_addr, other->m_physical_addr );
    m_pseq->out_log("  -cacheline 0x%0llx offset 0x%0x\n",
                     my_cacheline, my_offset);
    m_pseq->out_log("  -otherline 0x%0llx offset 0x%0x\n",
                     other_cacheline, other_offset);
#endif
    // common case: access to same byte, word, etc.
    if (my_offset == other_offset) {
      
      if (m_access_size == other->m_access_size) {
        isOverlap = MEMOP_EXACT;
      } 
      else if( m_access_size < other->m_access_size){
        // should be able to bypass if m_access_size < other->m_access_size
        isOverlap = MEMOP_EXACT;
      }
      else{
        // means m_access_size > other->m_access_size, which means LD needs
        //            to wait until ST unpdates memory
        // CM imprecise can do bypassing even when complete overlap,
        // not just partial overlap
#ifdef DEBUG_LSQ
        m_pseq->out_log("PO case 1:: sizes 0x%0x 0x%0x\n", m_access_size, other->m_access_size);
#endif
        isOverlap = MEMOP_OVERLAP;
      }

    } else if (my_offset < other_offset) {
      // if(==) this is fine bc LD is accessing previous byte relative to ST's starting byte
      if (my_offset + m_access_size > other_offset) {
        // CM imprecise can do bypassing here too if m + size >= o + size
#ifdef DEBUG_LSQ
        m_pseq->out_log(".");
#endif
        isOverlap = MEMOP_OVERLAP;
      }
    } else {
      // (my_offset > other_offset)
#ifdef DEBUG_LSQ
      m_pseq->out_log("!");
#endif
      // if(==) this is fine bc LD is accessing next byte following ST's terminating byte
      if (other_offset + other->m_access_size > my_offset) {
        isOverlap = MEMOP_OVERLAP;
      }
    }

  }
  return (isOverlap);
}

//***************************************************************************
void memory_inst_t::printDetail( void )
{
  dynamic_inst_t::printDetail();
  
  DEBUG_OUT("   access: %d\n", m_access_size );
  DEBUG_OUT("   addr  : 0x%0llx\n", m_address );
  DEBUG_OUT("   phys a: 0x%llx\n", m_physical_addr );
  
  DEBUG_OUT("   pstate: 0x%x\n", m_pstate );
  DEBUG_OUT("   tl    : 0x%x\n", m_tl );
  DEBUG_OUT("   asi   : 0x%x\n", m_asi );
  DEBUG_OUT("   valid : %d\n", m_data_valid );
}

//**************************************************************************
void memory_inst_t::setASI( void )
{
  // if this instruction (may) affect an ASI, get the current %ASI register
  if ( s->getFlag( SI_ISASI ) ) {

    bool is_block_load = false;
    m_asi = memop_getASI( s->getInst(), getSourceReg(3), &is_block_load, m_proc );

    if (s->getOpcode() == i_lddfa || s->getOpcode() == i_stdfa) {
      // if the asi is not a block load, and we've reserved 64 bytes,
      // the actual load or store size is 8 bytes
      if (!is_block_load) {
        if ( ((m_asi >> 4) & 0xf) == 0xd ) {
          if ((m_asi & 0x2) == 0x2) {
            // 16-bit load-store
            setAccessSize( 2 );
          } else {
            // 8-bit load-store
            setAccessSize( 1 );
          }
        } else {
          setAccessSize( 8 );
        }         

        // later we'll forward data from the 0th register using readInt64
        if (s->getOpcode() == i_stdfa) {
          getSourceReg(2).setVanilla(0);
        }
      }
    }
  } else {
    int  inverse_endian = (m_pstate >> 9) & 0x1;
    if (!inverse_endian) {
      if ( m_tl == 0 ) {
        m_asi = ASI_PRIMARY;
      } else {
        m_asi = ASI_NUCLEUS;
      }
    } else {
      if ( m_tl == 0 ) {
        m_asi = ASI_PRIMARY_LITTLE;
      } else {
        m_asi = ASI_NUCLEUS_LITTLE;
      }
    }
  }
  
  // ASI reads to the MMU rely on the simics to get their values
  // ASI writes to the MMU must be retired ASAP (not bundled)
  if (PSEQ_MAX_UNCHECKED != 1) {
    if ( m_asi >= 0x50 && m_asi <= 0x5f ) {
      setTrapType( Trap_Use_Functional );
    }
  }
}

/*------------------------------------------------------------------------*/
/* load_inst                                                              */
/*------------------------------------------------------------------------*/

//***************************************************************************
load_inst_t::load_inst_t( static_inst_t *s_inst, 
                          int32 window_index,
                          pseq_t *pseq,
                          abstract_pc_t *fetch_at,
                          pa_t physicalPC,
                          trap_type_t trapgroup,
                          uint32 proc)
  : memory_inst_t( s_inst, window_index, pseq, fetch_at, physicalPC, trapgroup, proc ) {

  // use default dynamic inst constructor
  m_value_pred = false;
  m_depend     = NULL;
  m_fetch_at   = *fetch_at;
}

//***************************************************************************
load_inst_t::~load_inst_t() {
}

//**************************************************************************
void 
load_inst_t::Execute() {

  #ifdef DEBUG_DYNAMIC
  char buf[128];
  s->printDisassemble(buf);
  DEBUG_OUT("[ %d ] load_inst_t: EXECUTE %s seqnum[ %lld ] fetched[ %lld ] cycle[ %lld ]", m_pseq->getID(), buf, seq_num, m_fetch_cycle, m_pseq->getLocalCycle());
  //print source and dest regs
  DEBUG_OUT(" SOURCES: ");
  for(int i=0; i < SI_MAX_SOURCE; ++i){
    reg_id_t & source = getSourceReg(i);
    if(!source.isZero()){
      DEBUG_OUT("( [%d] V: %d P: %d Arf: %s WriterSN: %lld WrittenCycle: %lld State: 0x%x)", i,source.getVanilla(), source.getPhysical(), source.rid_type_menomic( source.getRtype() ), source.getARF()->getWriterSeqnum( source, m_proc), source.getARF()->getWrittenCycle( source, m_proc), source.getVanillaState() );
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


  STAT_INC( m_pseq->m_stat_loads_exec[m_proc] );
  trap_type_t t = addressGenerate( OPAL_LOAD );

  
  if (t != Trap_NoTrap) {
    /* couldn't correctly generate address - fired off exception */
    SetStage( COMPLETE_STAGE );
    setTrapType( (trap_type_t) t );

    #ifdef DEBUG_DYNAMIC
    char buf[128];
    s->printDisassemble(buf);
    DEBUG_OUT("\tDTLB MISS %s seqnum[ %lld ] logicaladdr[ 0x%llx ]\n", buf, seq_num, m_address);
    #endif

    return;
  }

  /* can't complete I/O accesses, and moreover they are uncacheable */
  if ( m_physical_addr != (pa_t) -1 &&
       isIOAccess( m_physical_addr ) ) {
    SetStage( COMPLETE_STAGE );
    setTrapType( Trap_Use_Functional );
    STAT_INC( m_pseq->m_stat_count_io_access[m_proc] );    
#ifdef DEBUG_LSQ
    m_pseq->out_log("   load I/O access: index: %d\n", getWindowIndex());
#endif
   #ifdef DEBUG_DYNAMIC
    char buf[128];
    s->printDisassemble(buf);
    DEBUG_OUT("\tIO ACCESS %s seqnum[ %lld ]\n", buf, seq_num);
    #endif

    return;
  }
  
  if ( isCacheable() ) {
    ASSERT( m_physical_addr != (pa_t) -1 );

    //set address to be valid
    m_addr_valid = true;

    //charge for LSQ power before the if(), because we might return 
    /* WATTCH power */
    if(WATTCH_POWER){
      m_pseq->getPowerStats()->incrementLSQAccess();
      m_pseq->getPowerStats()->incrementLSQWakeupAccess();
    }

    #ifdef DEBUG_DYNAMIC
    char buf[128];
    s->printDisassemble(buf);
    DEBUG_OUT("\tCACHEABLE %s seqnum[ %lld ] logicaladdr[ 0x%llx ] physaddr[ 0x%llx ]\n", buf, seq_num, m_address, m_physical_addr);
    #endif

    // insert this memory instruction into the load/store queue.
    if (m_pseq->getLSQ(m_proc)->insert( this )) {
      /* we got a hit in the LSQ or some other event so we should stop
         handling it like a normal load */
#ifdef DEBUG_LSQ
      m_pseq->out_log("***** load LSQ exception cycle %lld\n",
                      m_pseq->getLocalCycle() );
#endif

     #ifdef DEBUG_DYNAMIC
    char buf[128];
    s->printDisassemble(buf);
    DEBUG_OUT("\tLSQ exception %s seqnum[ %lld ]\n", buf, seq_num);
    #endif

      return;
    }
  } // is cacheable
  else{
    #ifdef DEBUG_DYNAMIC
    char buf[128];
    s->printDisassemble(buf);
    DEBUG_OUT("\tNOT CACHEABLE %s seqnum[ %lld ]\n", buf, seq_num);
    #endif
  }

  //if no hit in LSQ (for data bypassing or waiting until dependent store retires)
  //           move on to 2nd phase of execution
  continueExecution();
}

//************************************************************************
void
load_inst_t::continueExecution()
{
  m_event_times[EVENT_TIME_CONTINUE_EXECUTE] = m_pseq->getLocalCycle() - m_fetch_cycle;
  if ( isCacheable() ) {
    if(CONFIG_WITH_RUBY){
      if(m_pseq->getWriteBuffer()->useWriteBuffer()){
        if(m_pseq->getWriteBuffer()->checkForLoadHit(this, m_physical_addr)){
          // We treat Write Buffer hits as cache misses, bc we are put on a waiting list until we get woken up!
          markEvent( EVENT_DCACHE_MISS );
          STAT_INC(m_pseq->m_stat_num_write_buffer_hits[m_proc]);
          SetStage(CACHE_MISS_STAGE);
          return;                 //wait until get woken up by write buffer
        }
      }
    }
    // NOT using write buffer OR did NOT hit in write buffer, submit request to cache interface
    // access the cache
    if (!accessCache())
      return;
  } // is cacheable
  else{
    #if 0
    char buf[128];
    s->printDisassemble(buf);
    DEBUG_OUT("\tNOT CACHEABLE 2 %s seqnum[ %lld ]\n", buf, seq_num);
    #endif
  }

  Complete();
}


//**************************************************************************
void 
load_inst_t::replay( void ) {
  m_pseq->raiseException( EXCEPT_MEMORY_REPLAY_TRAP,
                          m_pseq->getIwindow(m_proc)->iwin_decrement(getWindowIndex()),
                          (enum i_opcode) s->getOpcode(),
                          &m_fetch_at, 0 , /* penalty */ LOAD_REPLAY_PENALTY, m_proc );   
}

//**************************************************************************
void
load_inst_t::lsqBypass( void ) {

  STAT_INC( m_pseq->m_stat_load_bypasses[m_proc] );

  //sanity check
  if(!m_depend->getStoreValueReady()){
    ERROR_OUT("***load lsqBypass, ERROR Store value NOT READY, stage[%s]\n", printStage(m_depend->getStage()));
  }

  ASSERT(m_depend->getStoreValueReady());

    /* WATTCH power */
    if(WATTCH_POWER){
      m_pseq->getPowerStats()->incrementLSQAccess();
      m_pseq->getPowerStats()->incrementLSQLoadDataAccess();
      m_pseq->getPowerStats()->incrementLSQPregAccess();
      #ifdef DYNAMIC_AF
           m_pseq->getPowerStats()->incrementLSQNumPopCount();
      #endif
    }

    // get the maximum amount of data that can be bypassed from the other
    // instruction.
    byte_t  min_access_size = MIN( m_access_size, m_depend->getAccessSize() );
    
    // convert the access size (in bytes) to register size (8 byte) quantities
    min_access_size = ((min_access_size + 7)/ 8);
    for ( int i = 0; i < min_access_size; i++ ) {
      m_data_storage[i] = m_depend->getData()[i];
      
      /* WATTCH power */
      if(WATTCH_POWER){
#ifdef DYNAMIC_AF
        //charge power to write back some bits to the dest register
        // process each chunk of data read at a time
        uint64 temp = m_data_storage[i];
        m_pseq->getPowerStats()->incrementLSQTotalPopCount(m_pseq->getPowerStats()->pop_count(temp));
#endif
      }
    }
#ifdef DEBUG_DYNAMIC
      char buf[128];
      s->printDisassemble(buf);
      DEBUG_OUT("load_inst_t lsqBypass() %s seqnum[ %lld ] cycle[ %lld ] datastorage[ 0x%llx ] physaddr[ 0x%llx ]\n", buf, getSequenceNumber(), m_pseq->getLocalCycle(), m_data_storage[0], m_physical_addr);
#endif
      m_data_valid = true;
      markEvent( EVENT_LSQ_BYPASS );
 
 //  if( CONFIG_WITH_RUBY ) {
//     // tell the SC Checker about loads bypassing Ruby via the LSQ
//     mf_ruby_api_t *ruby_api = system_t::inst->m_ruby_api;
//     (*ruby_api->sc_Checker_RubyBypass)( m_physical_addr, getSequenceNumber(), m_pseq->getID()/CONFIG_LOGICAL_PER_PHY_PROC , OPAL_LOAD);
//   }

  /* the lsq copied the memory bits into the instruction; format the
     value and write it into a register */
  ASSERT( m_physical_addr != (my_addr_t) -1 );
  memory_inst_t::Execute(); 
}

//**************************************************************************
void
load_inst_t::lsqWait( void ) {
  STAT_INC( m_pseq->m_stat_early_store_bypass[m_proc] );  
  SetStage( LSQ_WAIT_STAGE );
#ifdef DEBUG_DYNAMIC
      char buf[128];
      char dependbuf[128];
      s->printDisassemble(buf);
      m_depend->getStaticInst()->printDisassemble(dependbuf);
      DEBUG_OUT("\tload_inst_t: lsqWait %s seq_num[ %lld ] physaddr[ 0x%llx ] Depend: %s seq_num[ %lld ] physaddr[ 0x%llx ]\n", buf, seq_num, m_physical_addr, dependbuf, m_depend->getSequenceNumber(), m_depend->getPhysicalAddress());
   #endif        
  /* put ourselves in the wait list for the store value register */
  m_depend->addDependentLoad( this );
}

//*************************************************************************
bool
load_inst_t::checkStoreSet() {
  //checks the store set predictor, and stalls this LD if predictor says
  //  we should stall
  if( m_pseq->getStoreSetPred()->checkForConflict( this ) ){
    //we are already placed on wait list
    STAT_INC(m_pseq->m_stat_storeset_stall_load[m_proc]);
    return true;
  }
  return false;
}

//***********************************************************************
void
load_inst_t::stallLoad(store_inst_t * depend){
#ifdef DEBUG_DYNAMIC
      char buf[128];
      s->printDisassemble(buf);
      DEBUG_OUT("\tload_inst_t: storesetStall %s seq_num[ %lld ] physaddr[ 0x%llx ]\n", buf, seq_num, m_physical_addr);
   #endif      

  //wait until Store executes
  SetStage( STORESET_STALL_LOAD_STAGE );
  depend->addPredictedDependentLoad(this);
}

//*************************************************************************
void
load_inst_t::addrOverlapWait(store_inst_t * depend){
  //wait until Store retires
  #ifdef DEBUG_DYNAMIC
      char buf[128];
      s->printDisassemble(buf);
      DEBUG_OUT("\tload_inst_t: addrOverlapWait %s seq_num[ %lld ] physaddr[ 0x%llx ]\n", buf, seq_num, m_physical_addr);
   #endif      

  SetStage( ADDR_OVERLAP_STALL_STAGE );
  depend->addOverlapLoad(this);
}

//**************************************************************************
bool
load_inst_t::accessCache( void ) {
   if (CONFIG_WITH_RUBY) {
#ifdef DEBUG_RUBY
    m_pseq->out_log("PC: 0x%0llx DATALOAD: 0x%0llx\n",
                    getVPC(), m_physical_addr );
#endif
    bool          mshr_hit   = false;
    bool          mshr_stall = false;
    bool          conflicting_access = false;
    //We should have a valid addr
    ASSERT(m_addr_valid);
    rubycache_t  *rcache = m_pseq->getRubyCache();
    //Differentiate between trans and non-trans loads
    OpalMemop_t requestType = OPAL_LOAD;
    ruby_status_t status;
    //issue an actual request
    status = rcache->access( m_address, m_physical_addr, requestType,
                             getVPC(), (m_pstate >> 2) & 0x1,
                             this, mshr_hit, mshr_stall, conflicting_access, m_proc);
      
    if (mshr_hit == true) {
      // SC_Checker needs to know about a load satisfied from the MSHR
  //     mf_ruby_api_t *ruby_api = system_t::inst->m_ruby_api;
//       (*ruby_api->sc_Checker_RubyBypass)( m_physical_addr, getSequenceNumber(), m_pseq->getID()/CONFIG_LOGICAL_PER_PHY_PROC , OPAL_LOAD);

      markEvent( EVENT_MSHR_HIT );
      
     #ifdef DEBUG_DYNAMIC
      char buf[128];
      s->printDisassemble(buf);
      DEBUG_OUT("\taccessCache MSHR HIT %s seq_num[ %lld ] logicaladdr[ 0x%llx ] physaddr[ 0x%llx ] lineaddr[ 0x%llx ]\n", buf, seq_num, m_address, m_physical_addr, makeLineAddress());
      //rcache->print();
     #endif
    }
    if (mshr_stall == true) {
      markEvent(EVENT_MSHR_STALL );
      #ifdef DEBUG_DYNAMIC
      char buf[128];
      s->printDisassemble(buf);
      DEBUG_OUT("\taccessCache MSHR STALL %s seq_num[ %lld ] logicaladdr[ 0x%llx ] physaddr[ 0x%llx ] lineaddr[ 0x%llx ]\n", buf, seq_num, m_address, m_physical_addr, makeLineAddress());
      //rcache->print();
     #endif
    }

    if ( status == HIT ) {
      // do nothing
      #ifdef DEBUG_DYNAMIC
      char buf[128];
      s->printDisassemble(buf);
      DEBUG_OUT("\taccessCache L1 HIT %s seq_num[ %lld ] logicaladdr[ 0x%llx ] physaddr[ 0x%llx ] lineaddr[ 0x%llx ]\n", buf, seq_num, m_address, m_physical_addr, makeLineAddress());
      //rcache->print();
     #endif
    } else if ( status == MISS ) {
      if ( MEMOP_STALE_DATA ) {
        // try to get stale data under the miss
        bool staleAvailable = rcache->staleDataRequest( m_physical_addr,
                                                        m_access_size,
                                                        m_predicted_storage );

        // current using oracle-like prediction for value prediction
        if (staleAvailable) {
          // perform oracle memory access
#if 0
          m_pseq->readPhysicalMemory( m_physical_addr,
                                      (int) m_access_size,
                                      getData(), m_proc );
          bool use_value_predictor = validateValuePrediction();
#endif
          bool use_value_predictor = true;
          if (use_value_predictor) {
            // perform value speculation using the stale data
            m_value_pred = true;
            STAT_INC(m_pseq->m_stat_stale_predictions[m_proc]);
            
            //   a) copy predicted into actual, set data to be valid
            for (int32 i = 0; i < (m_access_size + 7)/8; i++) {
              m_data_storage[i] = m_predicted_storage[i];
            }
            m_data_valid = true;
            
            //   b) format the value and write it into a register 
            ASSERT(m_physical_addr != (my_addr_t) -1);
            memory_inst_t::Execute();
          }
        }
      }
      markEvent( EVENT_DCACHE_MISS );
      STAT_INC(m_pseq->m_stat_num_dcache_miss[m_proc]);
      SetStage(CACHE_MISS_STAGE);

      #ifdef DEBUG_DYNAMIC
        char buf[128];
        s->printDisassemble(buf);
        DEBUG_OUT("load_inst_t::accessCache CACHE_MISS_STAGE %s cycle[ %lld ] proc[ %d] seqnum[ %lld mshr_hit[ %d ]]\n", buf, m_pseq->getLocalCycle(), m_proc, seq_num, mshr_hit);
        rcache->print();
      #endif

      return false;
    } else if ( status == NOT_READY ) {
     #ifdef DEBUG_DYNAMIC
      char buf[128];
      s->printDisassemble(buf);
      DEBUG_OUT("\taccessCache NOT READY %s seq_num[ %lld ] physaddr[ 0x%llx ] lineaddr[ 0x%llx ]\n", buf, seq_num, m_physical_addr, makeLineAddress());
      //rcache->print();
     #endif

      // NOT_READY cache status causes polling using event queue
#if 0
      m_pseq->out_log("NOTREADY: cycle:0x%lld PC: 0x%0llx DATALOAD: 0x%0llx\n",
                      m_pseq->getLocalCycle(),
                      getVPC(), m_physical_addr );
#endif
      // retry in one cycle
      m_pseq->postEvent( this, 1 );
      STAT_INC(m_pseq->m_num_cache_not_ready[m_proc]);  
      SetStage(CACHE_NOTREADY_STAGE);

      #ifdef DEBUG_DYNAMIC
        char buf[128];
        s->printDisassemble(buf);
        DEBUG_OUT("load_inst_t::accessCache CACHE_NOTREADY_STAGE %s cycle[ %lld ] proc[ %d] seqnum[ %lld mshr_hit[ %d ]]\n", buf, m_pseq->getLocalCycle(), m_proc, seq_num, mshr_hit);
        rcache->print();
      #endif

      return false;
    } else {
      SIM_HALT;
    }
   } else {
     // NOT using Ruby
    /* we missed in the load/store queue, check to see if we hit in
     * the cache-- if so get the value from architected memory,
     * otherwise request the line from the next level cache.
     */

    /* WATTCH power */
    if(WATTCH_POWER){
      m_pseq->getPowerStats()->incrementDCacheAccess();
    }

    cache_t *dcache = m_pseq->getDataCache();
    bool primary_bool = false;
    if ( !dcache->Read( m_physical_addr, this, true, &primary_bool )) {
      markEvent( EVENT_DCACHE_MISS );
      STAT_INC(m_pseq->m_stat_num_dcache_miss[m_proc]);
      SetStage(CACHE_MISS_STAGE);
      return false;
    }
  }
  return true;
}

//*************************************************************************
//  Basically the same as accessCache() except for retirement
//    On HITs sets m_retirement_permission flag to be true
//**************************************************************************
bool
load_inst_t::accessCacheRetirement( void ) {
  //IMPORTANT: We should check whether instruction is cacheable and no traps
  if( (getTrapType() == Trap_NoTrap) && (m_physical_addr != (pa_t) -1) && (!isIOAccess( m_physical_addr ) ) ) {
    if( isCacheable() ) {
      //only issue request if memory address is valid
      if(m_addr_valid) {
        return doCacheRetirement();
      }
    }
  }
  //Not cacheable or address is not valid, so set retirement permission to be true (don't care)
  m_retirement_permission = true;
  return true;
}

//************************************************************************
bool 
load_inst_t::doCacheRetirement(void){
  if (CONFIG_WITH_RUBY) {
#ifdef DEBUG_RUBY
    m_pseq->out_log("PC: 0x%0llx DATALOAD: 0x%0llx\n",
                    getVPC(), m_physical_addr );
#endif
    bool          mshr_hit   = false;
    bool          mshr_stall = false;
    bool          conflicting_access = false;
    //Differentiate between trans and non-trans loads
    OpalMemop_t requestType = OPAL_LOAD;
    rubycache_t  *rcache = m_pseq->getRubyCache();
    ruby_status_t status = rcache->access( m_address, m_physical_addr, requestType,
                                           getVPC(), (m_pstate >> 2) & 0x1,
                                           this, mshr_hit, mshr_stall, conflicting_access, m_proc);
    if (mshr_hit == true) {
      // LUKE - need to update some stats??
      // SC_Checker needs to know about a load satisfied from the MSHR
   //    mf_ruby_api_t *ruby_api = system_t::inst->m_ruby_api;
//       (*ruby_api->sc_Checker_RubyBypass)( m_physical_addr, getSequenceNumber(), m_pseq->getID()/CONFIG_LOGICAL_PER_PHY_PROC , OPAL_LOAD);

      markEvent( EVENT_MSHR_HIT );
    }
    if (mshr_stall == true) {
      markEvent(EVENT_MSHR_STALL );
    }
    if ( status == HIT ) {
      //do nothing
    } else if ( status == MISS ) {
      // LUKE - need to update some stats??
      if ( MEMOP_STALE_DATA ) {
        // try to get stale data under the miss
        bool staleAvailable = rcache->staleDataRequest( m_physical_addr,
                                                        m_access_size,
                                                        m_predicted_storage );

        // current using oracle-like prediction for value prediction
        if (staleAvailable) {
          // perform oracle memory access
#if 0
          m_pseq->readPhysicalMemory( m_physical_addr,
                                      (int) m_access_size,
                                      getData(), m_proc );
          bool use_value_predictor = validateValuePrediction();
#endif
          bool use_value_predictor = true;
          if (use_value_predictor) {
            // perform value speculation using the stale data
            m_value_pred = true;
            STAT_INC(m_pseq->m_stat_stale_predictions[m_proc]);
            
            //   a) copy predicted into actual, set data to be valid
            for (int32 i = 0; i < (m_access_size + 7)/8; i++) {
              m_data_storage[i] = m_predicted_storage[i];
            }
            m_data_valid = true;
            
            //   b) format the value and write it into a register 
            ASSERT( m_physical_addr != (my_addr_t ) -1 );
            memory_inst_t::Execute();
          }
        }
      }
      markEvent( EVENT_DCACHE_MISS );
      STAT_INC(m_pseq->m_stat_num_dcache_miss[m_proc]);
      SetStage(CACHE_MISS_RETIREMENT_STAGE);
      return false;
    } else if ( status == NOT_READY ) {
      // NOT_READY cache status causes polling using event queue
#if 0
      m_pseq->out_log("NOTREADY: cycle:0x%lld PC: 0x%0llx DATALOAD: 0x%0llx\n",
                      m_pseq->getLocalCycle(),
                      getVPC(), m_physical_addr );
#endif
      m_pseq->postEvent( this, 1 );
      STAT_INC(m_pseq->m_num_cache_not_ready[m_proc]);  
      SetStage(CACHE_NOTREADY_RETIREMENT_STAGE);
      return false;
    } else {
      SIM_HALT;
    }
  } else {
    /* we missed in the load/store queue, check to see if we hit in
     * the cache-- if so get the value from architected memory,
     * otherwise request the line from the next level cache.
     */

    /* WATTCH power */
    if(WATTCH_POWER){
      m_pseq->getPowerStats()->incrementDCacheAccess();
    }

    cache_t *dcache = m_pseq->getDataCache();
    bool primary_bool = false;
    if ( !dcache->Read( m_physical_addr, this, true, &primary_bool )) {
      markEvent( EVENT_DCACHE_MISS );
      STAT_INC(m_pseq->m_stat_num_dcache_miss[m_proc]);
      SetStage(CACHE_MISS_RETIREMENT_STAGE);
      return false;
    }
  }  // end !CONFIG_WITH_RUBY
  //Must be a hit, so set retirement permission flag to be true
  m_retirement_permission = true;
  return true;
}

//**************************************************************************
void
load_inst_t::Retire( abstract_pc_t *a ) {
  // check that we have the correct retirement permission

  ASSERT(m_retirement_permission);

  if(CONFIG_WITH_RUBY){
    if( (getTrapType() == Trap_NoTrap) && (m_physical_addr != (pa_t) -1) && (!isIOAccess( m_physical_addr ) )  ) {
      if(isCacheable()) {

        //Perform retirement data check.  If value of load at Complete() and Retire() differs, we need to replay this load (ie external store wrote different value).  Does not catch coherence invalidates but resulting in same value!
        bool mem_valid = m_pseq->readPhysicalMemory( m_physical_addr,
                                                     (int) m_access_size,
                                                     getRetirementData(), m_proc );
        if(!mem_valid){
          //  DEBUG_OUT("load_inst_t: Retire readphysicalMemory FAILED\n");
        }
        else{
          for(int i=0; i < MEMOP_MAX_SIZE; ++i){
            if(m_data_storage[i] != m_retirement_data_storage[i]){
              //  DEBUG_OUT("load_inst_t:Retire memory value differs from completion memory value: index[%d] completion_data[ 0x%llx ] retirement_data[ 0x%llx ]\n", i, m_data_storage[i], m_retirement_data_storage[i]);
              STAT_INC(m_pseq->m_stat_retired_loads_nomatch[m_proc]);
              break;
            }
          }
        }
        
        ASSERT(m_addr_valid);
        if(!m_pseq->getWriteBuffer()->useWriteBuffer()){
          rubycache_t *rcache = m_pseq->getRubyCache();
  #ifdef DEBUG_DYNAMIC
        char buf[128];
        s->printDisassemble(buf);
        DEBUG_OUT("load_inst_t::Retire.  About to check permissions %s cycle[ %lld ] proc[ %d] seqnum[ %lld ]\n", buf, m_pseq->getLocalCycle(), m_proc, seq_num);
        rcache->print();
      #endif
        
        }
      }  //end isCacheable    
    }
  }

  //keep stats on ASI that result in functional traps or is not cacheable
  if( (getTrapType() == Trap_Use_Functional) ){
    if(m_asi != (uint16) -1){
      STAT_INC(m_pseq->m_stat_functional_read_asi[m_asi]);

      #if 0
      //output
      char buf[128];
      s->printDisassemble(buf);
      ERROR_OUT("load FUNCTIONAL: asi[ 0x%x] VA[ 0x%llx ] cycle[ %lld ] %s\n", m_asi, m_address, m_pseq->getLocalCycle(), buf);
    #endif
    }
  }
  else if( !isCacheable() ){
    if( m_asi != (uint16) -1){
      STAT_INC(m_pseq->m_stat_uncacheable_read_asi[m_asi]);
    }
  }


  // update statistics
  STAT_INC( m_pseq->m_stat_loads_retired[m_proc] );

  memory_inst_t::Retire(a);

  #ifdef DEBUG_DYNAMIC
        char buf[128];
        rubycache_t *rcache = m_pseq->getRubyCache();
        s->printDisassemble(buf);
        DEBUG_OUT("load_inst_t::Retire FINISHED %s cycle[ %lld ] proc[ %d] seqnum[ %lld ] \n", buf, m_pseq->getLocalCycle(), m_proc, seq_num);
        rcache->print();
  #endif
}

//**************************************************************************
void 
load_inst_t::Complete( void ) {

  // update the ASI access statistics in the sequencer
  if (s->getFlag( SI_ISASI )) {
    m_pseq->m_asi_rd_stat[m_asi]++;
  }

  /* the value wasn't in the load/store queue, so get the value from
     the architected memory state */
  if ( CONFIG_IN_SIMICS ) {
    if ( isCacheable() ) {
      ASSERT( m_physical_addr != (my_addr_t ) -1 );

      bool need_physmem_value = true;
      if(need_physmem_value){
        // read the value from physical memory
        m_data_valid = m_pseq->readPhysicalMemory( m_physical_addr,
                                                   (int) m_access_size,
                                                   getData(), m_proc );
       #ifdef DEBUG_DYNAMIC
        char buf[128];
        s->printDisassemble(buf);
        DEBUG_OUT("load_inst_t complete() AFTER MEMREAD %s seqnum[ %lld ] cycle[ %lld ] ASI[ 0x%x ] accesssize[ %d ] logicaladdr[ 0x%llx ] physaddr[ 0x%llx ] datastorage[ ", 
                    buf, getSequenceNumber(), m_pseq->getLocalCycle(), m_asi, m_access_size, m_address, m_physical_addr);
          int access_size_8bytes = (m_access_size+7)/8;
          for(int i= 0; i < access_size_8bytes; ++i){
            DEBUG_OUT("( 0x%llx ) ", m_data_storage[i]);
          }
          DEBUG_OUT("]\n");
     #endif

        if(!m_data_valid){
          #ifdef DEBUG_DYNAMIC
          char buf[128];
          s->printDisassemble(buf);
          ERROR_OUT("load_inst_t complete() AFTER_MEMREAD EXCEPTION %s seqnum[ %lld ] cycle[ %lld ] datastorage[ 0x%llx ] data_valid[ %d ] physaddr[ 0x%llx ] access_size[ %d ]\n", buf, getSequenceNumber(), m_pseq->getLocalCycle(), m_data_storage[0], m_data_valid, m_physical_addr, m_access_size);
          #endif

          // got memread exception...need to sync with Simics
          STAT_INC( m_pseq->m_stat_memread_exception[m_proc]);
          SetStage( COMPLETE_STAGE );
          setTrapType( Trap_Use_Functional );
          //wait until retirement 
          return;
        }
        ASSERT(m_data_valid);
      }

      if (m_value_pred) {
        // compare the predicted value to the one from memory
        if (validateValuePrediction()) {
          if (log_base_two(m_access_size) <= log_base_two(MEMOP_MAX_SIZE*8))
            m_pseq->m_stat_stale_histogram[m_proc][log_base_two(m_access_size)]++;  
          else
            m_pseq->out_info("warning: load_t complete: out of range: m_access_size\n");
          m_pseq->m_stat_stale_success[m_proc]++;  
#ifdef STALE_DEBUG
          m_pseq->out_info("VSUCC: PC: 0x%0llx ADDR: 0x%0llx SIZE: %d inst: 0x%0x\n", getVPC(), m_physical_addr, m_access_size, s->getInst());
          m_pseq->out_info("load_inst: value (%d): 0x%0llx\n",
                           m_access_size, m_predicted_storage[0]);
          m_pseq->out_info("load_inst: value prediction succeeded!\n");
#endif
          SetStage(COMPLETE_STAGE);
        } else {
          // trigger a squash on this instruction
#ifdef STALE_DEBUG
          m_pseq->out_info("VFAIL: PC: 0x%0llx ADDR: 0x%0llx SIZE: %d inst: 0x%0x\n", getVPC(), m_physical_addr, m_access_size, s->getInst());
          m_pseq->out_info("load_inst: mispredict (%d): 0x%0llx\n",
                           m_access_size, m_predicted_storage[0]);
          m_pseq->out_info("load_inst: actual        : 0x%0llx\n",
                           getData()[0]);
          m_pseq->out_info("load_inst: time %0lld\n",
                           m_pseq->getLocalCycle());
#endif
          m_pseq->raiseException( EXCEPT_VALUE_MISPREDICT,
                                  m_pseq->getIwindow(m_proc)->iwin_decrement( getWindowIndex() ),
                                  (enum i_opcode) s->getOpcode(),
                                  &m_fetch_at, 0 , /* penalty */ 0, m_proc ); 
        }
        return;
      }
      
#ifdef DEBUG_LSQ
      m_pseq->out_log( "load_complete: index:%d 0x%0llx\n",
                        getWindowIndex(), m_data_storage[0] );
#endif
    } else {
      // do the MMU access (using a backdoor in simics) if not bundling retires
      // By not bundling retires, the MMU state reflects the architected state
      // some errors may still be caused by the out-of-order execution in
      // non-bundled mode, but they are few / minor.
      if (PSEQ_MAX_UNCHECKED == 1) {
#ifdef DEBUG_LSQ
        m_pseq->out_log("   MMU load access: index: %d\n", getWindowIndex());
#endif
        if ( m_asi >= 0x50 && m_asi < 0x5c ){
          #ifdef DEBUG_DYNAMIC
             DEBUG_OUT("\tAbout to call mmuRegisterAccess..\n");
          #endif
          mmuRegisterAccess( false );
        }
        // Opal can handle reading certain control registers..
        else if( m_asi == 0x49 || m_asi == 0x4a || m_asi == 0x7f){
          readControlReg();
        }
        else{
          #ifdef DEBUG_DYNAMIC
            DEBUG_OUT("\tUncacheable! ASI[ 0x%x ] accesssize[ %d ] VA[ 0x%llx ] seqnum[ %lld ] cycle[ %lld ]\n", m_asi, m_access_size, m_address, seq_num, m_pseq->getLocalCycle());
         #endif
        }
      }
    }
  } else {

    // read the value from the memory trace
    memtrace_t *mt = m_pseq->getMemtrace(m_proc);    
    //  m_pseq->out_log("local time %d = %d address 0x%0llx\n", m_pseq->getRetiredICount(), mytime, m_address);
    // plus 2 as the time is at the retire time ... not the current time :)
    m_data_valid = mt->scanTransaction( m_pseq->getRetiredICount(m_proc) + 2,
                                        m_address, getVPC(),
                                        s->getInst(), m_physical_addr,
                                        m_access_size, getData() );
  }
  
  /* format the value and write it into a register */
  memory_inst_t::Execute();
}

//***************************************************************************
bool load_inst_t::validateValuePrediction( void )
{
  bool  match = true;
  char *pred   = (char *) m_predicted_storage;
  char *actual = (char *) getData();

  for (int i=0; i < m_access_size; i++) {
    if (pred[i] != actual[i]) {
#ifdef STALE_DEBUG
      m_pseq->out_info("(%i) SIZE: %d  ADDR: 0x%0llx PRED: 0x%x ACTUAL: 0x%x\n",
                       i, (int) m_access_size, getVPC(),
                       (int) pred[i], (int) actual[i] );
#endif
      match = false;
    }
  }
  return match;
}

//***************************************************************************
void load_inst_t::printDetail( void )
{
  DEBUG_OUT("load_inst_t printDetail()\n");
  memory_inst_t::printDetail();
  if (m_depend) {
    DEBUG_OUT("   depend: %d\n", m_depend->getWindowIndex());
  }
}

/*------------------------------------------------------------------------*/
/* store_inst                                                             */
/*------------------------------------------------------------------------*/

//***************************************************************************
store_inst_t::store_inst_t( static_inst_t *s_inst, 
                            int32 window_index,
                            pseq_t *pseq,
                            abstract_pc_t *at,
                            pa_t physicalPC,
                            trap_type_t trapgroup,
                            uint32 proc)
  : memory_inst_t( s_inst, window_index, pseq, at, physicalPC, trapgroup, proc ) {
  // use default dynamic inst constructor
  m_atomic_swap = false;
}

//***************************************************************************
store_inst_t::~store_inst_t() {
}

//***************************************************************************
bool
store_inst_t::storeDataToCache( void ) {

  // The data to store is in Source Reg 2 (IRD_3)
  reg_id_t &source = getSourceReg(2);
  if ( !source.getARF()->isReady(source, m_proc) ) {
    // not ready quite yet...
    ERROR_OUT("warning: storeDataToCache: stalling: %d\n", getWindowIndex());
    printDetail();

    source.getARF()->waitResult( source, this, m_proc );
    return (false);
  }

  //charge for the data being written to the LSQ here, bc the data wasn't available earlier
  /* WATTCH power */
  if(WATTCH_POWER){
     #ifdef DYNAMIC_AF
            m_pseq->getPowerStats()->incrementLSQNumPopCount();  
     #endif
  }
  
  // read the store value from the register file (source 3)

  bool read_source_data = true;
  if(read_source_data){
    // convert the access size (in bytes) to register size (8 byte) quantities
    byte_t access_size = ((m_access_size + 7)/ 8);
    for (int i = 0; i < access_size; i++) {
      // if copying more than one register-- set a register selector
      // this is used for 64-byte load stores
      if (access_size > 1)
        source.setVanilla( i );
      m_data_storage[i] = source.getARF()->readInt64( source, m_proc );   
      
      /* WATTCH power */
      if(WATTCH_POWER){
#ifdef DYNAMIC_AF
        //charge power to write back some bits to the lsq entry
        // process each chunk of data read at a time
        uint64 temp = m_data_storage[i];
        m_pseq->getPowerStats()->incrementLSQTotalPopCount(m_pseq->getPowerStats()->pop_count(temp));
#endif
      }
#ifdef DEBUG_LSQ
    m_pseq->out_log("storeData2Cache: index:%d size:%d stored data: 0x%0llx\n",
                    getWindowIndex(), i, m_data_storage[i]);
#endif
    }
    m_data_valid = true;  

   #ifdef DEBUG_DYNAMIC
    char buf[128];
    s->printDisassemble(buf);
    DEBUG_OUT("store_inst_t: storeDataToCache() %s seqnum[ %lld ] cycle[ %lld ] datastorage[ 0x%llx ] logicaladdr[ 0x%llx ] physaddr[ 0x%llx ]\n", buf, getSequenceNumber(), m_pseq->getLocalCycle(), m_data_storage[0], m_address, m_physical_addr);
   #endif
  } //end read_source_data

  if(CONFIG_WITH_RUBY){
    if(m_pseq->getWriteBuffer()->useWriteBuffer()){
      //we store data directly to write buffer
      if(!addToWriteBuffer()){
        //we are already going to use polling on addToWriteBuffer() or accessCache()
        return false;
      }
      //we can immediately retire this store
      return true;
    }
  }
  //NOT using a write buffer
  if (!accessCache())
    return false;
  
  return true;
}

//***************************************************************************
bool
store_inst_t::addToWriteBuffer(){
  rubycache_t *rcache = m_pseq->getRubyCache();
  writebuffer_status_t status = m_pseq->getWriteBuffer()->addToWriteBuffer(this, m_address, m_physical_addr, getVPC(), (m_pstate >> 2) & 0x1, m_pseq->getLocalCycle(), m_proc);
  if(status == WB_OK){
    //Enqueued into WB successfully, we can go ahead and retire this store
    return true;
  }
  else if(status == WB_FULL){
    //Write buffer is full...this is a resource problem so we stall this instr until an entry frees up:
    // put this instruction in the event queue and poll addToWriteBuffer():
    m_pseq->postEvent( this, 1 );
    STAT_INC(m_pseq->m_stat_num_write_buffer_full[m_proc]);    
    SetStage(WB_FULL_STAGE);
    return false;
  }
  else if(status == WB_MATCHING_MSHR_ENTRY){
    //There's an outstanding MSHR entry that matches our request, go ahead and issue it to the MSHR instead...
    if(!accessCache()){
      return false;
    }
    return true;
  }
  else{
    // for when status doesn't match any of the known statuses
    return false;
  }
}

//**************************************************************************
void 
store_inst_t::Execute() {

  #ifdef DEBUG_DYNAMIC
  char buf[128];
  s->printDisassemble(buf);
  DEBUG_OUT("[ %d ] ", m_pseq->getID());
  if( s->getType() == DYN_STORE){
    DEBUG_OUT("store_inst_t: ");
  }
  else if( s->getType() == DYN_ATOMIC){
    DEBUG_OUT("atomic_inst_t: ");
  }
  DEBUG_OUT("EXECUTE %s seqnum[ %lld ] fetched[ %lld ] cycle[ %lld ]", buf, seq_num, m_fetch_cycle, m_pseq->getLocalCycle());
  //print source and dest regs
  DEBUG_OUT(" SOURCES: ");
  for(int i=0; i < SI_MAX_SOURCE; ++i){
    reg_id_t & source = getSourceReg(i);
    if(!source.isZero()){
      DEBUG_OUT("( [%d] V: %d P: %d Arf: %s )", i,source.getVanilla(), source.getPhysical(), source.rid_type_menomic( source.getRtype() ) );
    }
  }
  DEBUG_OUT(" DESTS: ");
  for(int i=0; i < SI_MAX_DEST; ++i){
    reg_id_t & dest = getDestReg(i);
    if(!dest.isZero()){
      DEBUG_OUT("( [%d] V: %d P: %d Arf: %s )", i,dest.getVanilla(), dest.getPhysical(), dest.rid_type_menomic( dest.getRtype() ) );
    }
  }
  DEBUG_OUT("\n");
#endif

  if ( s->getType() == DYN_ATOMIC ) {
    STAT_INC( m_pseq->m_stat_atomics_exec[m_proc] );
  } else {
    STAT_INC( m_pseq->m_stat_stores_exec[m_proc] );
  }

  trap_type_t t = addressGenerate( OPAL_STORE );
  if (t != Trap_NoTrap) {
    /* couldn't correctly generate address - fired off exception */
    SetStage( COMPLETE_STAGE );
    setTrapType( (trap_type_t) t );
    return;
  }
  
  /* can't complete I/O accesses, and moreover they are uncacheable */
  if ( m_physical_addr != (pa_t) -1 &&
       isIOAccess( m_physical_addr ) ) {
    SetStage( COMPLETE_STAGE );
    setTrapType( Trap_Use_Functional );
    STAT_INC( m_pseq->m_stat_count_io_access[m_proc] ); 
#ifdef DEBUG_LSQ
    m_pseq->out_log("   store I/O access: index: %d\n", getWindowIndex());
#endif
    return;
  }

  if ( isCacheable() ) {
    //set Address to be valid
    m_addr_valid = true;

    ASSERT( m_physical_addr != (pa_t) -1 );
    
    //charge for LSQ access before the if() because we might return
    /* WATTCH power */
    if(WATTCH_POWER){
      m_pseq->getPowerStats()->incrementLSQAccess();
      m_pseq->getPowerStats()->incrementLSQStoreDataAccess();
      m_pseq->getPowerStats()->incrementLSQPregAccess();
    }

    // insert this store instruction into the load/store queue.
    if (m_pseq->getLSQ(m_proc)->insert( this )) {
      /* we got a hit in the LSQ or some other event so we should stop
         handling it like a normal load */
#ifdef DEBUG_LSQ
      m_pseq->out_log("***** (atomic) load LSQ exception cycle %lld\n",
                      m_pseq->getLocalCycle() );
#endif

      #ifdef DEBUG_DYNAMIC
         DEBUG_OUT("\tLSQ exception seqnum[ %lld ]\n", seq_num);
      #endif

      return;
    }
  } //is cacheable

  // If ST or ATOMIC succeeds in inserting into LSQ, continue onto next phase of execution
  continueExecution();
}

//*****************************************************************
void 
store_inst_t::continueExecution(){
  m_event_times[EVENT_TIME_CONTINUE_EXECUTE] = m_pseq->getLocalCycle() - m_fetch_cycle;
  if( isCacheable() ){
    // are all registers ready
    bool       all_ready = true;
    for (int32 i = 0; i < SI_MAX_SOURCE; i++) {
      reg_id_t &source = getSourceReg(i);
      if ( !source.getARF()->isReady( source, m_proc ) ) {
        all_ready = false;

        if (s->getType() == DYN_STORE) {
          STAT_INC(m_pseq->m_stat_num_early_stores[m_proc]);
          SetStage( EARLY_STORE_STAGE );
        } else if (s->getType() == DYN_ATOMIC) {
          STAT_INC(m_pseq->m_stat_num_early_atomics[m_proc]);
          SetStage( EARLY_ATOMIC_STAGE );
        }
        // wait for the source register to be written
        source.getARF()->waitResult( source, this, m_proc );
        break;
      }
    }  // end for all sources

    if ( all_ready ) {
      // read the register, and do a demand write to the cache
      //    if the cache misses, return immediately
      if ( !storeDataToCache() ) {
        return;
      }
    } else {
      if (CONFIG_WITH_RUBY) {
        // modified for Write Buffers - do not issue prefetches if using Write Buffers:
        if(!m_pseq->getWriteBuffer()->useWriteBuffer()){
          rubycache_t *rcache = m_pseq->getRubyCache();
          // initiate a prefetch... if it fails because ruby isn't ready,
          //                        don't retry it, we'll issue soon enough.
          //Differentiate between Stores and Atomics
          OpalMemop_t requestType = OPAL_STORE;
          if(s->getType() == DYN_ATOMIC){
            requestType = OPAL_ATOMIC;
          }
          rcache->prefetch( m_address, m_physical_addr, requestType,
                            getVPC(), (m_pstate >> 2) & 0x1, m_proc);
        }
      } else {
        // early store: if its not already in our cache, exclusive prefetch block
        cache_t *dcache = m_pseq->getDataCache();
        if ( !dcache->TagCheck(m_physical_addr) )
          dcache->Prefetch( m_physical_addr);
      }
      // scheduled wakeup during "all register ready" loop
      return;
    }
  }  //is cacheable

  // Calls either store_inst_t or atomic_inst_t's Complete()
  Complete();
}


//**************************************************************************
void 
store_inst_t::storeDataWakeup( void ) {
  if ( storeDataToCache() ) {
    // hit in cache -- we can complete
    //   be careful: this could be atomic_inst_t complete!
    Complete();    
  }
  // miss in cache -- we are already placed in cache miss stage...
}

//**************************************************************************
void 
store_inst_t::wakeupDependentLoads() {
  m_dependent_loads.WakeupChain();
}

//*************************************************************************
void
store_inst_t::wakeupPredictedDependentLoads() {
  m_predicted_dependent_loads.WakeupChain();
}

//************************************************************************
void
store_inst_t::wakeupOverlapLoads() {
  m_overlap_loads.WakeupChain();
}

// accessCache() for both store and atomic instructions!
//**************************************************************************
bool
store_inst_t::accessCache( void ) {
  if (CONFIG_WITH_RUBY) {
#ifdef DEBUG_RUBY
    m_pseq->out_log("PC: 0x%0llx DATASTORE: 0x%0llx\n",
                    getVPC(), m_physical_addr );
#endif
    bool          mshr_hit = false;
    bool          mshr_stall = false;
    bool          conflicting_access = false;
    //we should have a valid addr
    ASSERT(m_addr_valid);
    rubycache_t  *rcache   = m_pseq->getRubyCache();
    //Differentiate between Stores and Atomics
    OpalMemop_t requestType = OPAL_STORE;
    if(s->getType() == DYN_ATOMIC){
      requestType = OPAL_ATOMIC;
    }
    ruby_status_t status;   
    status = rcache->access( m_address, m_physical_addr, requestType,
                               getVPC(), (m_pstate >> 2) & 0x1,
                               this, mshr_hit, mshr_stall, conflicting_access, m_proc);

    if (mshr_hit == true) {
      // SC_Checker needs to know about a store satisfied from the MSHR
      // this will cause an immediate assertion failure if the checker is enabled,
      // however, this block should theoretically never be executed anyway
    //   mf_ruby_api_t *ruby_api = system_t::inst->m_ruby_api;
//       (*ruby_api->sc_Checker_RubyBypass)( m_physical_addr, getSequenceNumber(), m_pseq->getID()/CONFIG_LOGICAL_PER_PHY_PROC , OPAL_LOAD);
      
      markEvent( EVENT_MSHR_HIT );

     #ifdef DEBUG_DYNAMIC
      char buf[128];
      s->printDisassemble(buf);
      DEBUG_OUT("\taccessCache MSHR HIT %s seq_num[ %lld ] logicaladdr[ 0x%llx ] physaddr[ 0x%llx ] lineaddr[ 0x%llx ]\n", buf, seq_num, m_address, m_physical_addr, makeLineAddress());
      //rcache->print();
     #endif
    }
    if (mshr_stall == true) {
      markEvent(EVENT_MSHR_STALL );
 #ifdef DEBUG_DYNAMIC
      char buf[128];
      s->printDisassemble(buf);
      DEBUG_OUT("\taccessCache MSHR STALL %s seq_num[ %lld ] logicaladdr[ 0x%llx ] physaddr[ 0x%llx ] lineaddr[ 0x%llx ]\n", buf, seq_num, m_address, m_physical_addr, makeLineAddress());
      //rcache->print();
     #endif
    }

    if ( status == HIT ) {
 #ifdef DEBUG_DYNAMIC
      char buf[128];
      s->printDisassemble(buf);
      DEBUG_OUT("\taccessCache L1 HIT %s seq_num[ %lld ] logicaladdr[ 0x%llx ] physaddr[ 0x%llx ] lineaddr[ 0x%llx ]\n", buf, seq_num, m_address, m_physical_addr, makeLineAddress());
      //rcache->print();
     #endif
      // do nothing
    } else if ( status == MISS ) {
      markEvent( EVENT_DCACHE_MISS );
      STAT_INC(m_pseq->m_stat_num_dcache_miss[m_proc]);
      SetStage(CACHE_MISS_STAGE);
      #ifdef DEBUG_DYNAMIC
      char buf[128];
      s->printDisassemble(buf);
      DEBUG_OUT("\t[ %d ]accessCache L1 MISS %s seq_num[ %lld ] logicaladdr[ 0x%llx ] physaddr[ 0x%llx ] lineaddr[ 0x%llx ] cycle[ %lld ]\n", m_pseq->getID(), buf, seq_num, m_address, m_physical_addr, makeLineAddress(), m_pseq->getLocalCycle());
      rcache->print();
      #endif
      return false;
    } else if ( status == NOT_READY ) {
      // NOT_READY cache status causes polling using event queue. Retry in one cycle.
      m_pseq->postEvent( this, 1 );
      STAT_INC(m_pseq->m_num_cache_not_ready[m_proc]);    
      SetStage(CACHE_NOTREADY_STAGE);
 #ifdef DEBUG_DYNAMIC
      char buf[128];
      s->printDisassemble(buf);
      DEBUG_OUT("\taccessCache CACHE NOT READY %s seq_num[ %lld ] logicaladdr[ 0x%llx ] physaddr[ 0x%llx ] lineaddr[ 0x%llx ]\n", buf, seq_num, m_address, m_physical_addr, makeLineAddress());
      //rcache->print();
     #endif
      return false;
    } else {
      SIM_HALT;
    }
  } else {
    // NOT using Ruby
    // check the cache for presence of this line

    /* WATTCH power */
    if(WATTCH_POWER){
      m_pseq->getPowerStats()->incrementDCacheAccess();
    }

    cache_t *dcache = m_pseq->getDataCache();

    if ( !dcache->Write( m_physical_addr, this ) ) {
      /* we missed: the cache is requesting a fill,
       *            we'll wait in CACHE_MISS_STAGE for the wakeup,
       *            update statistics, return */
      SetStage( CACHE_MISS_STAGE );
      STAT_INC(m_pseq->m_stat_num_dcache_miss[m_proc]);
      return false;
    }
  }

  return true;
}

//**************************************************************************
//   The same as accessCache() except it is called at Retirement
bool
store_inst_t::accessCacheRetirement( void ) {
 //IMPORTANT: We should check whether instruction is cacheable and no traps occurred
  if( (getTrapType() == Trap_NoTrap) && (m_physical_addr != (pa_t) -1) && (!isIOAccess( m_physical_addr ) ) ) {
    if( isCacheable() ) {
      //only issue request if memory address is valid
      if(m_addr_valid) {
        return doCacheRetirement();
      }
    }
  }
  //Not cacheable or address is not valid, so set retirement permission to be true (don't care)
  m_retirement_permission = true;
  return true;
}
 
//**************************************************************************
bool
store_inst_t::doCacheRetirement(void){ 
  if (CONFIG_WITH_RUBY) {
#ifdef DEBUG_RUBY
    m_pseq->out_log("PC: 0x%0llx DATASTORE: 0x%0llx\n",
                    getVPC(), m_physical_addr );
#endif
    bool          mshr_hit = false;
    bool          mshr_stall = false;
    bool          conflicting_access = false;
    ASSERT(m_addr_valid);
    rubycache_t  *rcache   = m_pseq->getRubyCache();
    //Differentiate between Stores and Atomics
    OpalMemop_t requestType = OPAL_STORE;
    if(s->getType() == DYN_ATOMIC){
      requestType = OPAL_ATOMIC;
    }
    ruby_status_t status   = rcache->access( m_address, m_physical_addr, requestType,
                                             getVPC(), (m_pstate >> 2) & 0x1,
                                             this, mshr_hit, mshr_stall, conflicting_access, m_proc );
    if (mshr_hit == true) {
      // LUKE - need to update some stats??
      // SC_Checker needs to know about a store satisfied from the MSHR
      // this will cause an immediate assertion failure if the checker is enabled,
      // however, this block should theoretically never be executed anyway
     //  mf_ruby_api_t *ruby_api = system_t::inst->m_ruby_api;
//       (*ruby_api->sc_Checker_RubyBypass)( m_physical_addr, getSequenceNumber(), m_pseq->getID()/CONFIG_LOGICAL_PER_PHY_PROC , OPAL_LOAD);
      
      markEvent( EVENT_MSHR_HIT );
    }
    if (mshr_stall == true) {
      markEvent(EVENT_MSHR_STALL );
    }
    if ( status == HIT ) {
      // do nothing
    } else if ( status == MISS ) {
      markEvent( EVENT_DCACHE_MISS );
      STAT_INC(m_pseq->m_stat_num_dcache_miss[m_proc]);
      SetStage(CACHE_MISS_RETIREMENT_STAGE);
      return false;
    } else if ( status == NOT_READY ) {
      // NOT_READY cache status causes polling using event queue
      m_pseq->postEvent( this, 1 );
      STAT_INC(m_pseq->m_num_cache_not_ready[m_proc]);    
      SetStage(CACHE_NOTREADY_RETIREMENT_STAGE);
      return false;
    } else {
      SIM_HALT;
    }
  } else {
    // check the cache for presence of this line

    /* WATTCH power */
    if(WATTCH_POWER){
      m_pseq->getPowerStats()->incrementDCacheAccess();
    }

    cache_t *dcache = m_pseq->getDataCache();

    if ( !dcache->Write( m_physical_addr, this ) ) {
      /* we missed: the cache is requesting a fill,
       *            we'll wait in CACHE_MISS_STAGE for the wakeup,
       *            update statistics, return */
      SetStage( CACHE_MISS_RETIREMENT_STAGE );
      STAT_INC(m_pseq->m_stat_num_dcache_miss[m_proc]);
      return false;
    }
  }
  //must be a hit, so set retirement permission flag to be true
  m_retirement_permission = true;
  return true;
}

//**************************************************************************
void
store_inst_t::Retire( abstract_pc_t *a ) {
  // make sure we have the correct permissions
  
  ASSERT(m_retirement_permission == true);

  // update statistics
  STAT_INC( m_pseq->m_stat_stores_retired[m_proc] );

 //keep stats on ASI that result in functional trapss (both IO and non-IO uncacheable)
  if(getTrapType() == Trap_Use_Functional){
    if(m_asi != (uint16) -1){
      STAT_INC(m_pseq->m_stat_functional_write_asi[m_asi]);
    }
  }
  else if( !isCacheable() ){
    if( m_asi != (uint16) -1){
      STAT_INC(m_pseq->m_stat_uncacheable_write_asi[m_asi]);
    }
  }

  memory_inst_t::Retire(a);

  // Need to wakeup any loads that have overlapping addresses 
  wakeupOverlapLoads();
}

//**************************************************************************
void 
store_inst_t::Complete() {
  // update the ASI access statistics in the sequencer
  if (s->getFlag( SI_ISASI )) {
    m_pseq->m_asi_wr_stat[m_asi]++;
  }
  
  /* this function can get called when both a store's address and data
     are available */
  //  ASSERT(m_physical_addr != (my_addr_t ) -1);
  memory_inst_t::Execute(); 

  /* must call wakeup _after_ execute:
   *      wakeup checks the store stage (must be completed)
   *      before bypassing the value */
  wakeupDependentLoads();
  // Also wakeup any loads that are predicted to cause load-store ordering violations
  wakeupPredictedDependentLoads();
}

//***************************************************************************
void store_inst_t::printDetail( void )
{
  DEBUG_OUT("store_inst_t printDetail()\n");
  memory_inst_t::printDetail();
  DEBUG_OUT( "  waitlist: \n");
  m_dependent_loads.print( cout );
  DEBUG_OUT( "  atomic swap: %d\n", m_atomic_swap );
}

/*------------------------------------------------------------------------*/
/* atomic_inst                                                            */
/*------------------------------------------------------------------------*/

//**************************************************************************
atomic_inst_t::atomic_inst_t( static_inst_t *s_inst, 
                              int32 window_index,
                              pseq_t *pseq,
                              abstract_pc_t *at,
                              pa_t physicalPC,
                              trap_type_t trapgroup,
                              uint32 proc)
  : store_inst_t( s_inst, window_index, pseq, at, physicalPC, trapgroup, proc ) {

  // use default dynamic inst constructor
  m_depend   = NULL;
  m_fetch_at = *at;
}

//***************************************************************************
atomic_inst_t::~atomic_inst_t() {

}

//**************************************************************************
void 
atomic_inst_t::replay( void ) {
  m_pseq->raiseException( EXCEPT_MEMORY_REPLAY_TRAP,
                          m_pseq->getIwindow(m_proc)->iwin_decrement( getWindowIndex() ),
                          (enum i_opcode) s->getOpcode(),
                          &m_fetch_at, 0 , /* penalty */ LOAD_REPLAY_PENALTY, m_proc ); 
}

//**************************************************************************
void
atomic_inst_t::lsqBypass( void ) {
  
  STAT_INC( m_pseq->m_stat_atomic_bypasses[m_proc] );

#ifdef DEBUG_LSQ
  m_pseq->out_info("Atomic lsqBypass: index:%d\n", getWindowIndex() );
#endif

  // sanity check
  if(!m_depend->getStoreValueReady()){
    ERROR_OUT("***atomic lsqBypass, ERROR Store value NOT READY, stage[%s]\n", printStage(m_depend->getStage()));
  }
  ASSERT(m_depend->getStoreValueReady());

  /* WATTCH power */
  if(WATTCH_POWER){
    m_pseq->getPowerStats()->incrementLSQAccess();
  }
  
  // get the maximum amount of data that can be bypassed from the other
  // instruction.
  byte_t  min_access_size = MIN( m_access_size, m_depend->getAccessSize() );
  
  // convert the access size (in bytes) to register size (8 byte) quantities
  min_access_size = ((min_access_size + 7)/ 8);
  for ( int i = 0; i < min_access_size; i++ ) {
    m_data_storage[i] = m_depend->getData()[i];
  }
#ifdef DEBUG_DYNAMIC
      char buf[128];
      s->printDisassemble(buf);
      DEBUG_OUT("atomic_inst_t: lsqBypass() %s seqnum[ %lld ] cycle[ %lld ] datastorage[ 0x%llx ] physaddr[ 0x%llx ]\n", buf, getSequenceNumber(), m_pseq->getLocalCycle(), m_data_storage[0], m_physical_addr);
#endif

      // don't set m_data_valid here because the atomic instruction can write
      //           values on execution
      markEvent( EVENT_LSQ_BYPASS );


  // Don't have to tell the SC Checker about the LSQ bypass, since ATOMICS
  // are writes and therefore will EVENTUALLY be sent to Ruby regardless 
  // of a bypass-on-read
  
  // Make sure all registers are ready before calling complete
  for (int32 i = 0; i < SI_MAX_SOURCE; i++) {
    reg_id_t &source = getSourceReg(i);
    if ( !source.getARF()->isReady( source, m_proc ) ) {
      source.getARF()->waitResult( source, this, m_proc );
      SetStage( EARLY_ATOMIC_STAGE );
      return;
    }
  }
  
  Complete();
}

//**************************************************************************
void
atomic_inst_t::lsqWait( void ) {
  STAT_INC( m_pseq->m_stat_early_store_bypass[m_proc] );  
  SetStage( LSQ_WAIT_STAGE );
  
#ifdef DEBUG_DYNAMIC
      char buf[128];
      char dependbuf[128];
      s->printDisassemble(buf);
      m_depend->getStaticInst()->printDisassemble(dependbuf);
      DEBUG_OUT("\tatomic_inst_t: lsqWait %s seq_num[ %lld ] physaddr[ 0x%llx ] Depend: %s seq_num[ %lld ] physaddr[ 0x%llx ]\n", buf, seq_num, m_physical_addr, dependbuf, m_depend->getSequenceNumber(), m_depend->getPhysicalAddress());
   #endif        

  /* put ourselves in the wait list for the store value register */
  m_depend->addDependentLoad( this );
}

//*************************************************************************
bool
atomic_inst_t::checkStoreSet() {
  //checks the store set predictor, and stalls this LD if predictor says
  //  we should stall
  if( m_pseq->getStoreSetPred()->checkForConflict( this ) ){
    //we are already placed on wait list
    STAT_INC(m_pseq->m_stat_storeset_stall_atomic[m_proc]);
    return true;
  }
  return false;
}

//***********************************************************************
void
atomic_inst_t::stallLoad(store_inst_t * depend){
#ifdef DEBUG_DYNAMIC
      char buf[128];
      s->printDisassemble(buf);
      DEBUG_OUT("\tatomic_inst_t: storesetStall %s seq_num[ %lld ] physaddr[ 0x%llx ]\n", buf, seq_num, m_physical_addr);
   #endif      

  //wait until Store executes
  SetStage( STORESET_STALL_LOAD_STAGE );
  depend->addPredictedDependentLoad(this);
}

//*************************************************************************
void
atomic_inst_t::addrOverlapWait(store_inst_t * depend){
 #ifdef DEBUG_DYNAMIC
      char buf[128];
      s->printDisassemble(buf);
      DEBUG_OUT("\tatomic_inst_t: addrOverlapWait %s seq_num[ %lld ] physaddr[ 0x%llx ]\n", buf, seq_num, m_physical_addr);
   #endif      

  //wait until Store retires
  SetStage( ADDR_OVERLAP_STALL_STAGE );
  depend->addOverlapLoad(this);
}

//**************************************************************************
void
atomic_inst_t::Retire( abstract_pc_t *a ) {
  ASSERT(m_retirement_permission);

  // update statistics
  STAT_INC( m_pseq->m_stat_atomics_retired[m_proc] );

  //keep stats on ASI that result in functional trapss (both IO and non-IO uncacheable)
  if(getTrapType() == Trap_Use_Functional){
    if(m_asi != (uint16) -1){
      STAT_INC(m_pseq->m_stat_functional_atomic_asi[m_asi]);
    }
  }
  else if( !isCacheable() ){
    if( m_asi != (uint16) -1){
      STAT_INC(m_pseq->m_stat_uncacheable_atomic_asi[m_asi]);
    }
  }

  memory_inst_t::Retire(a);

  // Need to wakeup any loads that have overlapping addresses 
  wakeupOverlapLoads();
}

//**************************************************************************
void 
atomic_inst_t::Complete() {

  // update the ASI access statistics in the sequencer
  if (s->getFlag( SI_ISASI )) {
    m_pseq->m_asi_at_stat[m_asi]++;
  }

  /* the value wasn't in the load/store queue, so get the value from
     the architected memory state */
  if ( CONFIG_IN_SIMICS ) {
    
    // we may have already gotten the data through the lsq
    if ( isCacheable() ) {

      if (getEvent(EVENT_LSQ_BYPASS)) {
        // already got data through lsq ... don't read from memory
        m_data_valid = true;
      } else {
        bool need_physmem_value = true;
        if(need_physmem_value){
          // read the value from physical memory
          m_data_valid = m_pseq->readPhysicalMemory( m_physical_addr,
                                                     (int) m_access_size,
                                                     getData(), m_proc );

         #ifdef DEBUG_DYNAMIC
          char buf[128];
          s->printDisassemble(buf);
          DEBUG_OUT("atomic_inst_t: complete() AFTER MEMREAD %s seqnum[ %lld ] cycle[ %lld ] datastorage[ 0x%llx ] data_valid[ %d ] physaddr[ 0x%llx ] access_size[ %d ]\n", buf, getSequenceNumber(), m_pseq->getLocalCycle(), m_data_storage[0], m_data_valid, m_physical_addr, m_access_size);

          //check data read
          for( int i=0; i < (m_access_size+7)/8; ++i){
            if(m_data_storage[i] == -1){
              //              ERROR_OUT("atomic_inst_t: complete() AFTER MEMREAD %s seqnum[ %lld ] cycle[ %lld ] datastorage[ 0x%llx ] data_valid[ %d ] physaddr[ 0x%llx ] access_size[ %d ]\n", buf, getSequenceNumber(), m_pseq->getLocalCycle(), m_data_storage[i], m_data_valid, m_physical_addr, m_access_size);
              //ASSERT(m_data_storage[i] != -1);
            }
          }
          #endif
                                                     
          if(!m_data_valid){
            #ifdef DEBUG_DYNAMIC
            char buf[128];
            s->printDisassemble(buf);
            ERROR_OUT("atomic_inst_t: complete() AFTER MEMREAD EXCEPTION %s seqnum[ %lld ] cycle[ %lld ] datastorage[ 0x%llx ] data_valid[ %d ] physaddr[ 0x%llx ] access_size[ %d ]\n", buf, getSequenceNumber(), m_pseq->getLocalCycle(), m_data_storage[0], m_data_valid, m_physical_addr, m_access_size);
            #endif

            // we got memread exception...need to sync with Simics
            STAT_INC( m_pseq->m_stat_memread_exception[m_proc]);
            SetStage( COMPLETE_STAGE );
            setTrapType( Trap_Use_Functional );
            //wait until retirement 
            return;
          }
          ASSERT(m_data_valid);
        }
      }
    }
    
  } else {

    // read the value from the memory trace
    memtrace_t *mt = m_pseq->getMemtrace(m_proc);    
    //  m_pseq->out_log("local time %d = %d address 0x%0llx\n", m_pseq->getRetiredICount(), mytime, m_address);
    // plus 2 as the time is at the retire time ... not the current time :)
    m_data_valid = mt->scanTransaction( m_pseq->getRetiredICount(m_proc) + 2,
                                        m_address, getVPC(),
                                        s->getInst(), m_physical_addr,
                                        m_access_size, getData() );
  }
  
  //  ASSERT(m_physical_addr != (my_addr_t ) -1 );
  memory_inst_t::Execute(); 
  
  /* must call wakeup _after_ execute:
   *      wakeup checks the store stage (must be completed)
   *      before bypassing the value */
  wakeupDependentLoads();
  // Also wakeup any loads that are predicted to cause load-store ordering violations
  wakeupPredictedDependentLoads();
}

//***************************************************************************
void atomic_inst_t::printDetail( void )
{
  DEBUG_OUT("atomic_inst_t printDetail()\n");
  store_inst_t::printDetail();
  if (m_depend) {
    DEBUG_OUT("   depend: %d\n", m_depend->getWindowIndex());
  }
}

/*------------------------------------------------------------------------*/
/* prefetch_inst                                                            */
/*------------------------------------------------------------------------*/

//**************************************************************************
prefetch_inst_t::prefetch_inst_t( static_inst_t *s_inst, 
                              int32 window_index,
                              pseq_t *pseq,
                              abstract_pc_t *at,
                              pa_t physicalPC,
                              trap_type_t trapgroup,
                              uint32 proc)
  : memory_inst_t( s_inst, window_index, pseq, at, physicalPC, trapgroup, proc ) {
}

//***************************************************************************
prefetch_inst_t::~prefetch_inst_t() {
}

//**************************************************************************
void 
prefetch_inst_t::Execute( void ) {
  STAT_INC( m_pseq->m_stat_prefetches_exec[m_proc] );
  // CM FIX: should possibly prefetch in exclusive
  trap_type_t t = addressGenerate( OPAL_LOAD );

  if (t != Trap_NoTrap) {
    /* couldn't correctly generate address -- we're done! */
    SetStage( COMPLETE_STAGE );
    return;
  }

  /* can't complete I/O accesses, and moreover they are uncacheable */
  if ( m_physical_addr != (pa_t) -1 &&
       isIOAccess( m_physical_addr ) ) {
    SetStage( COMPLETE_STAGE );
    STAT_INC( m_pseq->m_stat_count_io_access[m_proc] );  
    return;
  }
  
  if ( isCacheable() ) {
    ASSERT( m_physical_addr != (pa_t) -1 );

    //set address to be valid
    m_addr_valid = true;

    if (!accessCache())
      return;
  } // is cacheable

  Complete();
}

//**************************************************************************
bool
prefetch_inst_t::accessCache( void ) {
  if (CONFIG_WITH_RUBY) {
#ifdef DEBUG_RUBY
    m_pseq->out_log("PC: 0x%0llx PREFETCH: 0x%0llx\n",
                    getVPC(), m_physical_addr );
#endif

    //We should have a valid addr
    ASSERT(m_addr_valid);
    rubycache_t *rcache = m_pseq->getRubyCache();
    
    // CM fix: OPAL_LOAD should depend on type of prefetch
    ruby_status_t status = rcache->prefetch( m_address, m_physical_addr, OPAL_LOAD,
                                             getVPC(), (m_pstate >> 2) & 0x1, m_proc);
    if (status == NOT_READY) {
      // NOT_READY cache status causes polling using event queue
      m_pseq->postEvent( this, 1 );
      STAT_INC(m_pseq->m_num_cache_not_ready[m_proc]); 
      SetStage(CACHE_NOTREADY_STAGE);
      return false;
    }
     #ifdef DEBUG_DYNAMIC
      char buf[128];
      s->printDisassemble(buf);
      DEBUG_OUT("\taccessCache PREFETCH %s seq_num[ %lld ] logicaladdr[ 0x%llx ] physaddr[ 0x%llx ] lineaddr[ 0x%llx ]\n", buf, seq_num, m_address, m_physical_addr, makeLineAddress());
      //rcache->print();
     #endif
  } else {
    /* do a demand prefetch to the cache hierarchy */

    /* WATTCH power */
    if(WATTCH_POWER){
      m_pseq->getPowerStats()->incrementDCacheAccess();
    }

    cache_t *dcache = m_pseq->getDataCache();
    if ( !dcache->TagCheck(m_physical_addr) )
      dcache->Prefetch( m_physical_addr);
  }
  return true;
}

//*************************************************************************
// Same as accessCache(), but called at Retirement
bool
prefetch_inst_t::accessCacheRetirement(void){
  //IMPORTANT: Make sure this instruction can access the cache
  if( (getTrapType() == Trap_NoTrap) && (m_physical_addr != (pa_t) -1) && (!isIOAccess( m_physical_addr ) ) ) {
    if( isCacheable() ) {
      //only issue request if memory address is valid
      if(m_addr_valid) {
        return doCacheRetirement();
      }
    }
  }
  //Not cacheable or address is not valid, so set retirement permission to be true (don't care)
  m_retirement_permission = true;
  return true;
}

//************************************************************************  
bool
prefetch_inst_t::doCacheRetirement(){
  if (CONFIG_WITH_RUBY) {
#ifdef DEBUG_RUBY
    m_pseq->out_log("PC: 0x%0llx PREFETCH: 0x%0llx\n",
                    getVPC(), m_physical_addr );
#endif
    rubycache_t *rcache = m_pseq->getRubyCache();
    
    // CM fix: OPAL_LOAD should depend on type of prefetch
    ruby_status_t status = rcache->prefetch( m_address, m_physical_addr, OPAL_LOAD,
                                             getVPC(), (m_pstate >> 2) & 0x1, m_proc);
    if (status == NOT_READY) {
      // NOT_READY cache status causes polling using event queue
      m_pseq->postEvent( this, 1 );
      STAT_INC(m_pseq->m_num_cache_not_ready[m_proc]);
      SetStage(CACHE_NOTREADY_RETIREMENT_STAGE);
      return false;
    }
  } else {
    /* do a demand prefetch to the cache hierarchy */

    /* WATTCH power */
    if(WATTCH_POWER){
      m_pseq->getPowerStats()->incrementDCacheAccess();
    }

    cache_t *dcache = m_pseq->getDataCache();
    if ( !dcache->TagCheck(m_physical_addr) )
      dcache->Prefetch( m_physical_addr);
  }
  //else must be a hit, so set retirement permission flag
  m_retirement_permission = true;
  return true;
}

//**************************************************************************
void
prefetch_inst_t::Retire( abstract_pc_t *a ) {
  ASSERT(m_retirement_permission);

  // update statistics
  STAT_INC( m_pseq->m_stat_prefetches_retired[m_proc] );
  
  memory_inst_t::Retire(a);
}

//***************************************************************************
void 
prefetch_inst_t::Complete( void ) {
  // update the ASI access statistics in the sequencer
  if (s->getFlag( SI_ISASI )) {
    // CM FIX: not tracking prefetch ASI stats yet.
    // m_pseq->m_asi_rd_stat[m_asi]++;
  }

  // execute the prefetch instruction (change to retire-ready instruction)
  //ASSERT(m_physical_addr != (my_addr_t ) -1 );
  memory_inst_t::Execute(); 
}

//***************************************************************************
void prefetch_inst_t::printDetail( void )
{
  DEBUG_OUT("prefetch_inst_t printDetail()\n");
  memory_inst_t::printDetail();
}

/*------------------------------------------------------------------------*/
/* Accessor(s) / mutator(s)                                               */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Private methods                                                        */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Static methods                                                         */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Global functions                                                       */
/*------------------------------------------------------------------------*/

//***************************************************************************
uint16 memop_getASI( uint32 inst, reg_id_t asi_reg, bool *is_block_load, uint32 proc )
{
  uint16 asi;
  // casa, casxa must set isimmediate for address generation
  //    so (s->getFlag(SI_ISIMMEDIATE)) doesn't work here
  if ( maskBits32( inst, 13, 13 ) == 1 ) {
    /// uses the asi register
    asi = asi_reg.getARF()->readInt64( asi_reg, proc );     
  } else {
    /// read the asi from the static instruction
    asi = maskBits32( inst, 12, 5 );
  }

  // determine if the ASI is a block load ASI or not.
  char upper_asi = (asi >> 4) & 0xf;
  *is_block_load = ( (upper_asi == 0x7) ||
                     (upper_asi == 0xf) ||
                     (upper_asi == 0xe) );
  return (asi);
}
