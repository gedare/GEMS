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
#ifndef _DYNAMIC_H_
#define _DYNAMIC_H_

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include "listalloc.h"
#include "wait.h"
#include "statici.h"

/*------------------------------------------------------------------------*/
/* Class declarations                                                     */
/*------------------------------------------------------------------------*/

/// interesting times of events in dynamic instruction's lifecycles
enum inst_time_t {
  EVENT_TIME_DECODE = 0,
  EVENT_TIME_OPERANDS_READY,    //when all operands are ready
  EVENT_TIME_EXECUTE,                   //when this instr starts execution
  EVENT_TIME_CONTINUE_EXECUTE,     //when load is unstalled
  EVENT_TIME_EXECUTE_DONE,
  EVENT_TIME_RETIRE,
  EVENT_TIME_NUM_EVENT_TIMES
};

/// a bit field of interesting events that may occur to a dynamic instruction
typedef uint16 inst_event_t;
const inst_event_t EVENT_DCACHE_MISS       = 1 << 0;
const inst_event_t EVENT_LSQ_BYPASS        = 1 << 1;
const inst_event_t EVENT_VALUE_PRODUCER    = 1 << 2;
const inst_event_t EVENT_LSQ_INSERT        = 1 << 3;
const inst_event_t EVENT_IWINDOW_REMOVED   = 1 << 4;
const inst_event_t EVENT_MSHR_HIT          = 1 << 5;
const inst_event_t EVENT_FINALIZED         = 1 << 6;
const inst_event_t EVENT_BRANCH_MISPREDICT = 1 << 7;
const inst_event_t EVENT_MSHR_STALL        = 1 << 8;
// indicates this instr had L2 Data miss
const inst_event_t EVENT_L2_MISS               = 1 << 9;
// indicates this instr was dependent on an instr that had L2 miss
const inst_event_t EVENT_DEP_L2_MISS       = 1 << 10;
// indicates this instr was stalled to to address overlap
const inst_event_t EVENT_ADDR_OVERLAP_STALL   = 1 << 11;
// indicates this instr had L2 Instr miss
const inst_event_t EVENT_L2_INSTR_MISS   =  1<< 12;

/** pointer to member function: dynamicExecute - executes an instruction on
 *  a dynamic instruction and a sequencer */
typedef void (dynamic_inst_t::*pmf_dynamicExecute)( void );

/**
* An "in-flight" instruction which is being scheduled, executed, or retired
* in the processor model.
*
* Instructions are represented as the "static" portion, which contains
* generic 'decode', 'functional unit', 'timing' information, and as
* "dynamic" portion, which contains the execution stage, the resources
* it holds or is waiting on (?), and other generic in-flight information.
*
* @see     static_inst_t, waiter_t, control_inst_t, memory_inst_t
* @author  zilles
* @version $Id$
*/
class dynamic_inst_t : public waiter_t {

public:
  /** @name Constructor(s) / Destructor */
  //@{
  /**
   *  Constructor:
   *     assumes the dynamic instruction is in the Fetch state,
   *     resets the counters for the object.
   *  @param s_inst The static instruction related to this in flight instance
   *  @param window_index The index in the seqencers instr window (iwin).
   *  @param pseq   The sequence which issues this instruction
   *                (may be removed)
   *  @param sequence_num A strictly incrementing sequence number counter,
   *                representing the total number of instructions executed.
   *  @param at     The pc and npc when this instruction is fetched.
   *  @param  proc   The logical proc number in the SMT chip which is running this instr
   */
  dynamic_inst_t( static_inst_t *s_inst, 
                  int32 window_index,
                  pseq_t *pseq,
                  abstract_pc_t *at,
                  pa_t physicalPC,
                  trap_type_t trapgroup,
                  uint32 proc
                  );
  /**
   * Destructor: uses default destructor.
   */
  virtual ~dynamic_inst_t();

  /** Allocates object through allocator interface */
  void *operator new( size_t size ) {
    return ( m_myalloc.memalloc( size ) );
  }
  
  /** frees object through allocator interface */
  void operator delete( void *obj ) {
    m_myalloc.memfree( obj );
  }
  //@}
  
public:
  /** sources and destination registers.
   *  Note: The order of these *_INDEX's is important */
  enum reg_index_t {SOURCE1_INDEX, SOURCE2_INDEX, TO_FREE_INDEX, DEST_INDEX};

  /** stage of execution of the dynamic instruction. */
  enum stage_t {
    FETCH_STAGE,
    DECODE_STAGE,
    WAIT_4TH_STAGE,
    WAIT_3RD_STAGE,
    WAIT_2ND_STAGE,
    WAIT_1ST_STAGE,
    READY_STAGE,
    EXECUTE_STAGE,
    LSQ_WAIT_STAGE,
    EARLY_STORE_STAGE,
    EARLY_ATOMIC_STAGE,
    CACHE_MISS_STAGE,
    CACHE_NOTREADY_STAGE,
    CACHE_MISS_RETIREMENT_STAGE,
    CACHE_NOTREADY_RETIREMENT_STAGE,
    WB_FULL_STAGE,
    ADDR_OVERLAP_STALL_STAGE,
    STORESET_STALL_LOAD_STAGE,
    COMPLETE_STAGE,
    RETIRE_STAGE,
    MAX_INST_STAGE
  };

  /** decode function: renames the physical to logical registers.
   *  Sets the stage to decode.
   *  @param issueTime The scheduling priority (issue time) of this instruction
   *              Earlier instructions (lower issue time) have higher priority.
   */
  void Decode( uint64 issueTime );

  // helper function to allocate destination registers
  void allocateDestRegs();

  /** Test the input source registers readiness. If they are ready,
      this function will change the stage. */
  void testSourceReadiness();

  /** After decoding the instruction we put it in the window. Check
      whether the instruction is ready to be executed.  If register
      value is not ready, put the instruction in a waiting list for
      that register, otherwise send the instruction to the scheduler */
  void Schedule();
  /// stub which sets stage, record stats.
  void         beginExecution();

  /** execute: modify the machine's state by executing this instruction
   *  implemented differently by control, exec, mem instrs
   */
  virtual void Execute();

  /** retire: change the machines in-order architected state according
   *  to an instruction.
   *  (implemented differently by control, exec, mem instrs).
   *  @param a   The next in-order pc, when this instruction is retired.
   */
  virtual void Retire( abstract_pc_t *a );

  /// retire an integer register (free it)
  void         retireRegisters();
  /// remove any register mappings performed by this instruction
  void         UnwindRegisters( void );

  /// squash an instruction which has just been fetched
  void         FetchSquash();

  /** squash an instruction in the pipe. As it rolls back the state it
   *  is implemented differently by control, exec, mem instrs. */
  virtual void Squash();

  /** waiter interface function: 
   *      This function is called when a register file is written, it
   *      schedules this dynamic instruction to run.
   */
  void Wakeup( void );

  /** waiter interface function:
   *      This function should never be called.
   */
  void Release() { SIM_HALT; }

  /** @name Accessor / Mutators */
  //@{
  /// returns member: static instruction
  static_inst_t *getStaticInst( void ) const { return s; }
  /// returns the virtual address of this dynamic instruction
  la_t           getVPC( void ) const {
    return m_virtual_address;
  }
  pa_t          getPC() const{
    return m_physical_pc;
  }
  /// returns member: window index (index in window of in flight instructions)
  int32          getWindowIndex() const { return m_windex; }
  /// returns member: sequence number (used extensively in lsq.C)
  uint64         getSequenceNumber() const { return seq_num; }

  /// returns a source register
  reg_id_t &getSourceReg( int sourcenum ) {
    ASSERT( sourcenum < SI_MAX_SOURCE );
      return (m_source_reg[sourcenum]);
  }
  /// returns a destination register
  reg_id_t &getDestReg( int destnum ) {
    ASSERT( destnum < SI_MAX_DEST );
    return (m_dest_reg[destnum]);
  }
  /// returns a "old" destination register
  reg_id_t &getToFreeReg( int destnum ) {
    ASSERT( destnum < SI_MAX_DEST );
    return (m_tofree_reg[destnum]);
  }

  /// gets the stage of execution (CM 10.8.2002 use only for debugging)
  stage_t getStage( void ) { return (m_stage); };
  /// returns status: has this instruction begun executing
  bool   hasExecuted() const { return (m_stage > READY_STAGE); }
  /// returns status: is this instruction ready to retire
  bool   isRetireReady();

  /// sets flags: mark this instruction as performing some event
  void   markEvent(inst_event_t e) { m_events |= e; }
  /// sets flags: mark this instruction as NOT performing some event
  void   unmarkEvent(inst_event_t e) { m_events &= ~e; }
  /// gets flags
  bool   getEvent( inst_event_t e) { return ( (m_events & e) != 0 ); }
  /// gets the times when events have occurred
  uint16 getEventTime( inst_time_t t ) {
    return m_event_times[t];
  }
  /// gets the time when this instruction was fetched
  uint64 getFetchTime( void ) {
    return m_fetch_cycle;
  }

  /// sets the trap type for this instruction
  void   setTrapType( trap_type_t tt ) {
    if (m_traptype > tt) {
#ifdef LSQ_DEBUG
      m_pseq->out_info("index:%d generated trap:%d\n", m_windex, (uint16) tt);
#endif
      m_traptype = (uint16) tt;
    }
  }
  /// gets the trap type for this instruction
  trap_type_t getTrapType( void ) {
    return ((trap_type_t) m_traptype);
  }

  /// sets the trap group for this instr
  void setTrapGroup( trap_type_t tt){
    m_trapgroup = (uint16) tt;
  }

  /// get sthe trap group for this instr
  trap_type_t getTrapGroup(){
    return (trap_type_t) m_trapgroup;
  }

  /// get privilege mode
  bool        getPrivilegeMode( void ) {
    return m_priv;
  }

  /// get the logical proc number
  uint32 getProc(){
    return m_proc;
  }

  //*********************************************

  /// prints out the decoded instruction
  const char *print( void ) const;
  /// prints out a character string representation of the stage
  static const char *printStage( stage_t stage );
  /// prints out an abbreviated character string representation of the stage
  static const char *printStageAbbr( stage_t stage );

  /// print out instruction in readable format
  void printInstruction();

  /// print out detailed information
  virtual void  printDetail( void );
  /// prints out the registers and their mappings
  void          printRegs( bool print_values );
  //@}

  /** @name memory allocation:: */
  //@{
  static listalloc_t m_myalloc;
  //@}

  /// array indicating which source register which experiences L2 miss you are dependent on
  bool          m_source_l2miss_dep[SI_MAX_SOURCE];

  // returns a pointer to our checkpointed fetch at
  abstract_pc_t * getCheckpointFetchAt(){
    return &m_checkpoint_fetch_at;
  }

  /// sets the stage of execution
  void           SetStage(enum stage_t stage);


protected:
  /** sequence number for this instruction */
  uint64         seq_num;

  /** reference to sequencer which owns this inst. */
  pseq_t        *m_pseq;

  /** The logical proc number which is running this instr, SMT chip specific */
  uint32         m_proc;

  /** pointer to associated static instruction. */
  static_inst_t *s;

  /** the virtual address of this instruction */
  la_t           m_virtual_address;

  /** the physical PC of this instruction */
  pa_t          m_physical_pc;

  /** in flight source registers */
  reg_id_t       m_source_reg[SI_MAX_SOURCE];

  /** in flight destination registers */
  reg_id_t       m_dest_reg[SI_MAX_DEST];

  /** previous destination registers -- to be freed at retire time */
  reg_id_t       m_tofree_reg[SI_MAX_DEST];

  /** "pointer" into "seq"'s array of inflight instructions */
  int32         m_windex;

  /** The trap type, if any, generated by executing this instruction
   *    - Trap type of Trap_NoTrap indicated no trap.
   *    - On retirement, the trap handler will be selected if this is set.
   */
  uint16         m_traptype;

  // trap group for this instruction
  uint16         m_trapgroup;

  /** This dynamic instruction current "owner": fetch, schedule, etc. */
  actor_t       *m_stage_owner;
  
  /** The stage of execution for this dynamic instruction */
  stage_t        m_stage;

  /** keep track of interesting events for this instruction */
  inst_event_t   m_events; 

  /** when this instruction is fetched (created) */
  tick_t         m_fetch_cycle;
  
  /** offset time from the fetch time: when events happened to this instr */
  uint16         m_event_times[EVENT_TIME_NUM_EVENT_TIMES];

  /** privilege mode flag */
  bool           m_priv;

  /** Cycle that this instruction is ready to retire */
  tick_t         m_complete_cycle;

  /// copy of the current m_fetch_at
  abstract_pc_t   m_checkpoint_fetch_at;

public:
  /** @name nextPC: Given the current PC, nPC, these functions
  **  compute the next PC and next nPC. This modifies the state of
  **  the branch predictors, so they must be used accordingly.
  */
  //@{
  /// non-CTI instructions: PC = nPC, nPC = nPC + 4
  void   nextPC_execute( abstract_pc_t *a );
  /// taken non annulled CTI: PC = nPC, nPC = EA
  void   nextPC_taken( abstract_pc_t *a );
  /// taken annulled CTI: PC = EA, nPC = EA + 4
  void   nextPC_taken_a( abstract_pc_t *a );
  /// untaken annulled CTI: PC = nPC + 4, nPC = nPC + 8
  void   nextPC_untaken_a( abstract_pc_t *a );
  /// untaken non annulled CTI: PC = nPC, nPC = nPC + 4
  void   nextPC_untaken( abstract_pc_t *a );
  /// return from trap (done, retry) instructions
  void   nextPC_trap_return( abstract_pc_t *a );
  /** no operation: does not advance PC or NPC
   *  NOTE: this is only used for misfetched NOT for nop instructions */
  void   nextPC_nop( abstract_pc_t *a );

  /** CALL control transfer: functionally similar to nextPC_indirect, but
   *       * updates the RAS predictor as well.
   */
  void   nextPC_call( abstract_pc_t *a );
  /** RET  control transfer: functionally similar to nextPC_indirect, but
   *       * uses the RAS predictor as well.
   *       * taken non annulled CTI: PC = nPC, nPC = EA */
  void   nextPC_return( abstract_pc_t *a );
  /** CWP  control transfer: 
   *       could be used to check predicted cwp == actual cwp
   *       could be used to speculate into cwp traps
   *       currently just: PC = nPC, nPC = nPC + 4
   */
  void   nextPC_cwp( abstract_pc_t *a );
  /** predicted conditional: 
   **    use predicated result to determine PC, nPC result */
  void   nextPC_predicated_branch( abstract_pc_t *a );
  /** unpredicted conditional: use branch prediction to determine
   **    PC, nPC result */
  void   nextPC_predict_branch( abstract_pc_t *a );
  /** register indirect delayed control transfer to effective address.
   *  NOTE: uses and modifies branch predictor, so use with care!! */
  void   nextPC_indirect( abstract_pc_t *a );
  /** trap: indirect non-delayed control transfer to effective address.
   *  NOTE: this WILL use and modify a predictor, but not yet ... */
  void   nextPC_trap( abstract_pc_t *a );
  /** priv: writes to the tl and pstate registers affect how fetch is
   *  handled. */
  void   nextPC_priv( abstract_pc_t *a );
  //@}

  /// Initialize the dynamic instruction jump table
  static void initialize( void );
  /// A jump table for all instructions that are complicated
  static  pmf_dynamicExecute  m_jump_table[i_maxcount];
  /** @name Execution functions:
  **  execute this instruction on the dynamic context.
  */
  //@{
#include "dx.h"
  //@}
};

/*------------------------------------------------------------------------*/
/* Global variables                                                       */
/*------------------------------------------------------------------------*/

#endif /* _DYNAMIC_H_ */
