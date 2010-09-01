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
#ifndef _PSEQ_H_
#define _PSEQ_H_

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

#include "Vector.h"
#include "Map.h"

/// The status of the fetch unit: ready to fetch instructions, or stalled
enum pseq_fetch_status_t {
  PSEQ_FETCH_READY = 0,
  PSEQ_FETCH_ICACHEMISS,
  PSEQ_FETCH_SQUASH,
  PSEQ_FETCH_ITLBMISS,
  PSEQ_FETCH_WIN_FULL,
  PSEQ_FETCH_BARRIER,
  PSEQ_FETCH_WRITEBUFFER_FULL,
  PSEQ_FETCH_MAX_STATUS
};

/// The status of the retire unit
enum pseq_retire_status_t {
  PSEQ_RETIRE_READY = 0,
  PSEQ_RETIRE_UPDATEALL,
  PSEQ_RETIRE_SQUASH,
  PSEQ_RETIRE_LIMIT,
  PSEQ_RETIRE_MAX_STATUS
};

/// The number of table entries in the instruction page map.
const int PSEQ_IPAGE_TABLESIZE       = 16384;

/// The number of retire opportunities before raising the "deadlock" flag
const int PSEQ_MAX_FWD_PROGRESS_TIME = 1024*64*64;

/// The number of instruction to fastforward past (when skipping miss handlers)
const uint32 PSEQ_MAX_FF_LENGTH      = 65536;


/*------------------------------------------------------------------------*/
/* Class declaration(s)                                                   */
/*------------------------------------------------------------------------*/

//forward declaration
class proc_waiter_t;


/**
* Processor Sequencer - one processor's sequencer - represents a single
* thread of execution on a SMT machine.
*
* Each sequencer can wait on the L1 i-cache. if instruction fetch stalls,
* the sequencer waits for the cache to return with the data (e.g. a call
* to "Wakeup()". The next cycle the sequencer resumes fetching.
*
* @see     iwindow_t, waiter_t
* @author  cmauer
* @version $Id$
*/

class pseq_t : public waiter_t, public out_intf_t {

public:
  /// @name debug constants
  //@{
  /// debug simple front end
  static const bool DEBUG_SIMPLE_FE = false;
  /// debug multicycle front end
  static const bool DEBUG_MULTICYCLE_FE = false;
  //@}

  /// @name constants
  //@{
  /// invalid line address used in the multicycle I$ frontend
  static const pa_t PSEQ_INVALID_LINE_ADDRESS = ~(pa_t)0;
  //@}

  /**
   * @name Constructor(s) / destructor
   */
  //@{

  /**
   * Constructor: creates object
   * @param id    Integer idenfier for this sequencer
   */
  pseq_t( int32 id );

  /**
   * Destructor: frees object.
   */
  virtual ~pseq_t();
  //@}

  /**
   * @name Methods
   */
  //@{

  //Called by Write Buffer to stall the front-end
  void WBStallFetch();

  //Called by Write Buffer to unstall the front-end
  void WBUnstallFetch();

  /** advances one clock cycle for this processor.
   *
   *  Includes fetching, decoding, scheduling, executing, and retiring
   *  instructions. Each of these sub-tasks is broken out as private
   *  methods of this class.
   */
  void advanceCycle( void );

  /// Fetch a number of instructions past the current PC
  void fetchInstruction();
  /// Decodes instructions which have been fetched.
  void decodeInstruction();
  /** Schedules instructions which have been decoded on the functional
   *  units in the processor. */
  void scheduleInstruction();
  /** Retires instructions from the instruction window. */
  void retireInstruction();

  /** Cause an exception on this sequencer. This schedules an exception
   *  if there are no outstanding exceptions more recent.
   *
   *  @param type      The type of exception
   *  @param win_index The point in the program where the exception occurred.
   *  @param offender  The type of instruction causing the exception
   *  @param continue_at  The program counter to start fetch at
   *  @param access_addr If a memory operation caused the exception, the
   *                     address of the memory location being accessed.
   *   @param proc      The logical processor number of a SMT chip
   */
  void raiseException(exception_t type, uint32 win_index,
                      enum i_opcode offender,
                      abstract_pc_t *continue_at,
                      my_addr_t access_addr,
                      uint32 execption_penalty,
                      unsigned int proc);   
  /** clears the exception fields. */
  void clearException(unsigned int proc );

  /** Takes a trap.
   *  @param  instruction  The instruction that causes the trap
   *  @param  inorder_at   Contains a cache of the inorder fetch/pstate info
   *  @param  is_timing    True if called from the timing context
   *  @param proc          The logical processor number in a SMT chip
   *  @return bool         True if the instruction must be checked
   *                       (e.g. unimplemented instruction) */
  bool takeTrap( dynamic_inst_t *instruction, 
                 abstract_pc_t  *inorder_at,
                 bool is_timing, unsigned int proc );

  /** Squash execution back to a last_good instruction. Start executing
   *  with a new program counter (abstract pc).
   *  @param last_good  The last good instruction in the instr window to exec
   *  @param offender   The instruction causing the exception
   *  @param fetch_at   The PC to start executing at.
   *  @param proc        The logical processor number in a SMT chip
   */
  void partialSquash( uint32 last_good, abstract_pc_t *fetch_at,
                      enum i_opcode offender, unsigned int proc, bool squash_all);
  
  /** Squash all instructions in the window. The offending instruction
   *  is required for statistics. Fetch restarts at the inorder pc & npc
   *  of the architected state.
   *  @param offender   The instruction causing the exception
   *  @param proc        The logical processor number in a SMT chip
   */
  void fullSquash( enum i_opcode offender, unsigned int proc );

  /** postEvent is called to schedule a wakeup sometime in the future.
   *  This interface calls to the 'm_scheduler' that owns the event queue.
   *  @param waiter The waiter to be woken up in the future
   *  @param cyclesInFuture The number of cycles in the future when the
   *                        wakeup should occur.
   */
  void postEvent( waiter_t *waiter, uint32 cyclesInFuture );

  /** Wakeup(): is called when the L1 i-cache returns with a the next
   *  instruction to be fecthed cache line.
   */
  void     Wakeup(){
    // NOT USED. Wakeup(uint32 proc) is used instead, to unstall a specific SMT thread
  }

  void     Wakeup(uint32 proc);

  /** unstallFetch: is called by retiring instructions to restart
   *   instruction fetch. This allows instructions to have 'in-order'
   *   barrier-like semantics in an out-of-order machine. Like, I am
   *   going to modify the control register, and there is a dependence
   *   between what I do and the next instruction fetched. Another
   *   example is writes to the MMU-- translation can't complete until
   *   the store that maps a page has been executed.
   *
   *   @param proc    The logical processor number in a SMT chip
   */
  void    unstallFetch( unsigned int proc );

  /** prints out the control flow graph for debugging */
  void    printCFG( unsigned int proc );         

  /** Get an idealized (in-order) register file for this processor */
  reg_box_t &getIdealBox(uint32 proc ) {
    ASSERT(proc < CONFIG_LOGICAL_PER_PHY_PROC);
    return m_inorder[proc].regs;
  }

  /** Simulates an in-order functional processor. */
  void                stepInorder( void );
  /** Runs the inorder functional processor up to a certain inorder index */
  pseq_fetch_status_t idealRunTo( int64 runto_seq_num, unsigned int proc );   
  /** provides an oracle branch prediction */
  pseq_fetch_status_t oraclePredict( dynamic_inst_t *d, bool *taken, 
                                     abstract_pc_t *inorder_at );

  /** re-synchronize between the oracle predictor and simics */
  void    idealResync( void );

  /** provides an interface for oracle fetch */
  void    oracleFetch( int64 seq_num);
  /** Runs the inorder functional processor ahead of the timing model */
  void    runInorderAhead( void );
  /** synchronize the checked state with simics's */
  void    idealCheckTo( int64 seq_num );

  /** warmup the cache.
   *  This interface takes a memory transaction from simics, and inserts
   *  the address into the cache hierarchy. It does not provide any
   *  additional timing information, it just "busts" the data in there.
   */
  void    warmupCache( memory_transaction_t *mem_op );

  /** start the wall-clock performance timing */
  void    startTime( void );
  /** stop  the wall-clock performance timing */
  void    stopTime( void );

  /** log the number of logical */
  void    logLogicalRenameCount( rid_type_t rid, half_t logical, 
                                 uint32 count );
  //@}

  /**
   * @name Accessor(s) / mutator(s)
   */
  //@{
  
  /** returns the id of this sequencer */
  int          getID( void ) {
    return m_id;
  }
  /** Gets a pointer to the load/store queue -- callers may modify the lsq.
   *  @returns A pointer to the load/store queue */
  lsq_t       *getLSQ( unsigned int proc ) {
    ASSERT(proc < CONFIG_LOGICAL_PER_PHY_PROC);
    return m_lsq[proc]; }

  /* Returns the pointer to the Write buffer */
  writebuffer_t * getWriteBuffer(){
    return m_write_buffer;
  }

  /* Returns the pointer to the power stats */
  power_t * getPowerStats(){
    return m_power_stats;
  }

  /* Used by the system object to clear access counters, for WATTCH */
  void clearAccessStats();

  /* Used by the system object to update the power stats, for WATTCH */
  void updatePowerStats();

  /* Used by Ruby to increment L2 access counter */
  void incrementL2Access();

   /* Used by Ruby to increment prefetches */
  void incrementPrefetcherAccess(int num_prefetches, int isinstr);

  /* Used by Ruby to get the number of accesses to L1I cache */
  int getNumberICacheAccesses();

  /* Used by Ruby to get the number of accesses to L1D cache */
  int getNumberDCacheAccesses();

  // Used by the rubycache to notify the sequencer to schedule WB flushes
  void scheduleWBWakeup();

  /** Gets a pointer to the instruction window */
  iwindow_t   *getIwindow(uint32 proc ) {
    ASSERT(proc < CONFIG_LOGICAL_PER_PHY_PROC);
    return &(m_iwin[proc]); }
  /** Gets a pointer to the scheduler */
  scheduler_t *getScheduler( void ) {
    return m_scheduler; }
  /** Gets a pointer to the Miss Status Holding Register */
  mshr_t      *getMSHR( void ) {
    return m_mshr; }

  /** Gets a pointer to direct branch predictor */
  direct_predictor_t  *getDirectBP( ) {
    return m_bpred;
  }
  /** Gets a pointer to indirect branch predictor */
  cascaded_indirect_t *getIndirectBP( ) {
    return m_ipred;
  }
  /** get a pointer to the speculative branch predictor state */
  predictor_state_t *getSpecBPS() {
    return (m_spec_bpred);
  }
  /** get a pointer to the architected branch predictor state */
  predictor_state_t *getArchBPS( ) {
    return (m_arch_bpred);
  }
  /** set the architected branch predictor state */
  void setArchBPS(predictor_state_t &new_arch_bpred) {
    *(m_arch_bpred) = new_arch_bpred;
  }
  /** set the architected branch predictor state */
  void setSpecBPS(predictor_state_t &new_arch_bpred) {
    *(m_spec_bpred) = new_arch_bpred;
  }
  /** Gets a return address stack pointer */
  ras_t  *getRAS(unsigned int proc ) {
    ASSERT(proc < CONFIG_LOGICAL_PER_PHY_PROC);
    return m_ras[proc];
  }
  /** Gets the trap level stack predictor */
  tlstack_t *getTLstack(unsigned int proc ) {
    ASSERT(proc < CONFIG_LOGICAL_PER_PHY_PROC);
    return m_tlstack[proc];
  }

  /** gets the memory management unit's ASI interface 
   *       This interface deals with reading and writing to the MMU's 
   *       internal registers, so use with care. */
  mmu_interface_t *getASIinterface(unsigned int  proc ) {
    ASSERT(proc < CONFIG_LOGICAL_PER_PHY_PROC);
    return m_mmu_asi[proc];
  }

  /** Abstract interface to all register files */
  reg_box_t &getRegBox( void ) {
    return m_ooo.regs;
  }

  /** get the memory operation trace */
  memtrace_t *getMemtrace( unsigned int proc ) {
    ASSERT(proc < CONFIG_LOGICAL_PER_PHY_PROC);
    return m_memtracefp[proc];
  }
  /** get the l1 data cache interface */
  cache_t *getDataCache( void ) {
    return (l1_data_cache);
  }
  /** get the l1 instruction cache interface */
  cache_t *getInstructionCache( void ) {
    return (l1_inst_cache);
  }
  /** get the ruby cache interface */
  // for SMT, all logical procs should see same Ruby interface
  rubycache_t *getRubyCache( void ) {
    return (m_ruby_cache);
  }

  /* returns the current fetch PC */
  abstract_pc_t * getFetchPC(int proc) {
    return m_fetch_at[proc];
  }

  // prints the rubycache
  void printRubyCache();

  /** get the number of instructions retired on this processor */
  uint64       getRetiredICount(unsigned int proc ) {
    return (m_local_icount[proc]);
  }
  /** get the local cycle count */
  tick_t       getLocalCycle() {
    return (m_local_cycles);
  }
  /** step the sequence number */
  uint64       stepSequenceNumber(unsigned int proc ) {
    uint64 number = m_local_sequence_number[proc];
    m_local_sequence_number[proc] = m_local_sequence_number[proc] + 1;
    return (number);
  }
  /** unstep the sequence number::
   *  This should only be called when instructions are squashed.
   *  Otherwise, the LSQ state will get corrupted (relies on sequence ordering)
   */
  void         decrementSequenceNumber(unsigned int proc ) {
    m_local_sequence_number[proc] = m_local_sequence_number[proc] - 1;
  }

 //returns current sequencer number
  uint64 getSequenceNumber(unsigned int proc){
    return m_local_sequence_number[proc];
  }

  /** step the sequence number */
  uint64       stepInorderNumber(unsigned int proc ) {
    uint64 number = m_local_inorder_number[proc];
    m_local_inorder_number[proc] = m_local_inorder_number[proc] + 1;
    return (number);
  }
  
  /** increment the local cycle counter */
  void         localCycleIncrement() {
    m_local_cycles++;
  }

  /** statistics object holding per dynamic instruction stats */
  decode_stat_t* getOpstat(uint32 proc ) {
    return ( &(m_opstat[proc]) );    
  }
  //@}

  /**
   * @name Simics specific interfaces
   */
  //@{
  /// Simics Specific: get/set interfaces (specifically the mmu)
  void   installInterfaces();

  /// Simics Specific: remove interfaces on uninstall
  void   removeInterfaces( void );

  /// Advance Simics (or a trace) a given number of steps
  bool   advanceSimics( unsigned int proc );

  /// Fastforward Simics until a retry statement is hit
  void   fastforwardSimics( unsigned int proc );

  /** context switch at a processor.
   *  @param context    The new context for this processor
   */
  void   contextSwitch( context_id_t new_context, unsigned int proc );

  /** RUBY specific: called when a memory request is completed */
  void   completedRequest( pa_t physicalAddr, bool abortRequest, OpalMemop_t type, int thread );

  // notifies rubycache that request is L2 miss
  void markL2Miss( pa_t physicalAddr, OpalMemop_t type, int tagexists);
  
  /** read a physical memory location
   *
   * @param phys_addr The physical address to read from.
   * @param size      The number of bytes to read
   * @param result_reg A pointer to where the result should be written
   * @param proc      The logical processor number
   * @return bool true if the read succeeds, false if TBE access fails
   */
  bool   readPhysicalMemory( pa_t phys_addr, int size, ireg_t *result_reg, unsigned int proc );

  /** read an instruction from simics memory
   *
   * @param cur_pc       The virtual (logical) PC to read the instruction from.
   * @param physical_pc  The physical address of this program counter.
   * @param traplevel    The interrupt level of the processor
   * @param pstate       The PSTATE register of the processor
   * @param next_instr_p A 32-bit integer where the next instruction will
   *       be written
   * @param proc       The logical processor number
   * @return bool true if the get succeeds, false if it fails
   */
  bool   getInstr( la_t cur_pc, int32 traplevel, int32 pstate,
                   pa_t *physical_pc, unsigned int *next_instr_p, unsigned int proc );

  /** read an instruction from memory, using simics's current trap level
   *  and processor state as an implicit parameter.  Otherwise,
   *  same as getInstr().
   */
  bool getInstr( la_t cur_pc, 
                 pa_t *physical_pc, unsigned int *next_instr_p, unsigned int proc );

  /** access the memory management unit to do address translation */
  trap_type_t mmuAccess( la_t address, uint16 asi, uint16 accessSize,
                         OpalMemop_t op, uint16 pstate, uint16 tl,
                         pa_t *physAddress, unsigned int proc, bool inquiry);

  /** post an exception to this processor */
  void postException( uint32 exception, unsigned int proc );

  /** find the pid of this processor */
  int32 getPid(unsigned int proc );
  /** given a logical address to start with, find the PID */
  int32 getPid( la_t thread_p, unsigned int proc );

  /** get the thread pointer (of the currently running thread on this proc) */
  la_t getCurrentThread(unsigned int proc );  
  
  /** set the physical address of this processors 'thread' pointer */
  void setThreadPhysAddress( la_t thread_phys_addr, unsigned int proc );

  /** get the physical address of this processors 'thread' pointer */
  la_t getThreadPhysAddress(unsigned int proc );
  //@}

  /**
   * @name Configuration interfaces: used to save/restore state.
   */
  //@{
  /// register checkpoint interfaces
  void  registerCheckpoint( void );

  /// begin writing new checkpoint
  void  writeCheckpoint( char *checkpointName );

  /// open a checkpoint for reading
  void  readCheckpoint( char *checkpointName );
  //@}

  /**
   * @name Trace recording interfaces
   */
  //@{
  /** open a trace file for writing
   *  @param traceFileName The name of the trace file.
   */
  void    openTrace( char *traceFileName, unsigned int proc );
  /** Log the PC, instruction, and register information to the trace file */
  void    writeTraceStep(unsigned int proc );
  /** Log the just the PC ... to the trace file */
  void    writeSkipTraceStep(unsigned int proc);
  /** close the trace */
  void    closeTrace(unsigned int proc );

  /** attach to a trace file for reading
   *  @param traceFileName The name of the trace file.
   *  @param withImap      True if the imap is saved in a file, false if not
   *  @param proc            The logical processor number
   */

  void    attachTrace( char *traceFileName, bool withImap, unsigned int proc );
  /** attach to a memory trace file for reading */
  void    attachMemTrace( char *traceFileName, unsigned int proc );
  /** attach to a TLB map file */
  void    attachTLBTrace( char *traceFileName, unsigned int proc );
  /** log a memory transaction to the trace file */
  void    writeTraceMemop( transaction_t *trans, unsigned int proc );

  /** open a branch trace file for writing
   *  @param traceFileName The name of the trace file.
   */
  void    openBranchTrace( char *traceFileName, unsigned int proc );
  /** write branch step: write the next instruction in the trace */
  void    writeBranchStep(unsigned int proc );
  /** write next file: write next sequential file in branch trace */
  bool    writeBranchNextFile(unsigned int proc );
  /** close the trace */
  void    closeBranchTrace(unsigned int proc );
  
  /** load the next PC, instruction in the trace.
   *  @param  a       Contains virtual pc, and tl of this instruction
   *  @param  ppc     reference to the next physical pc in the trace
   *  @param  instr   reference to the next instruction in the trace
   *  @param  proc   The logical processor number
   *  @return bool true if valid, false if at the end of a trace
   */
  bool    readTraceStep( la_t &vpc, pa_t &ppc, unsigned int &instr, unsigned int proc );

  /** query for an instruction in the instruction maps.
   */
  bool    queryInstruction( pa_t fetch_ppc, static_inst_t * &s_instr, unsigned int proc );
  
  /** inserts an instruction into the instruction cache.
   *  @param  ppc     The physical pc of ths instruction
   *  @param  instr   The instruction at the virtual address "vpc"
   *  @param  proc   The logical processor number
   *  @return static_inst_t A pointer to the new static instruction.
   */
  static_inst_t *insertInstruction( pa_t fetch_ppc, unsigned int instr, unsigned int proc );

  /** Invalidates instruction in the decoded instruction cache.
   *  This refers to the Opal's (software simulator) cache of decoded
   *  instructions, not invalidating entries in the i-cache.
   *  @param  address   The physical address to be invalidated.
   *  @param  proc        The logical processor number
   */
  void    invalidateInstruction( pa_t address, unsigned int proc );

  /** Write out the (flat) instruction file (to enable prefetching) */
  void    writeInstructionTable( char *imapFileName, unsigned int proc );

  /** Read in the (flat) instruction file (to enable prefetching)
   *  @return true if successful, false if not */
  bool    readInstructionTable( char *imapFileName, int context, unsigned int proc );

  /** Allocate and initialize the ideal in-order processor register file */
  //void    allocateFlatRegBox( mstate_t &inorder );
  // the version that takes in logical proc numbers
  void    allocateFlatRegBox( mstate_t &inorder, uint32 proc );
  /** Clear flat register dependencies */
  void    clearFlatRegDeps( mstate_t &inorder, flow_inst_t *predecessor, unsigned int proc );
  //@}

  /**
   * @name Statistics functions
   */
  //@{
  /// initialize statistics
  void    initializeStats( void );
  //@}

  /**
   * @name Debugging / Testing
   */
  //@{
  /** Formats and print out an abstract pc to the log */
  void    printPC( abstract_pc_t *a );

  /** Print out the state of the entire microprocessor */
  void    print( void );
  /** print the in flight instructions */
  void    printInflight( void );
  /** print statistics */
  void    printStats( void );
  /** prints debug info */
  void    printDebug();
  /** print fetch debug info */
  void    printFetchDebug();
  /** validate the trace (print out its contents). */
  void    validateTrace(unsigned int proc );
  /** print out the contents of the instruction pages
   *  @param  verbose False if instructions should only be counted not printed
   *  @param   proc     The logical processor number
   *  @return uint32  The number of instructions printed
   */
  uint32  printIpage( bool verbose, unsigned int proc );

  /** translates a fetch status into a string */
  static const char* fetch_menomic( pseq_fetch_status_t status );

  /** translates a retire status into a string */
  static const char* retire_menomic( pseq_retire_status_t status );
  //@}

  // for how many ROB entries a thread has taken
  int getNumSlotsTaken(int thread){
    return m_iwin[thread].getNumSlotsTaken();
  }

  int getTotalSlotsTaken(){
    int total_slots = 0;
    for(int i=0; i < CONFIG_LOGICAL_PER_PHY_PROC; ++i){
      total_slots += m_iwin[i].getNumSlotsTaken();
    }
    return total_slots;
  }

  /** To simulate a shared instruction window requires checking whether the total number of used instruction
   *    slots is <= IWINDOW_ROB_SIZE.  Note that it is possible for a single thread to use up the entire
   *     shared instruction window. To prevent starvation, we reserve RESERVED_ROB_ENTRIES for each thread
   *     and no thread can use the entries reserved for the remaining threads
  */
  bool isSlotAvailable(int thread){
    int32 total_slots = 0;
    int num_zero_slot_threads = 0;
    bool result = false;
    bool total_win_full = false;
    for(uint k = 0; k < CONFIG_LOGICAL_PER_PHY_PROC; ++k){
      int slots_taken = m_iwin[k].getNumSlotsTaken();
      total_slots += slots_taken;
      if(!m_iwin[k].isSlotAvailable()){
        ASSERT(m_iwin[k].getNumSlotsTaken() == IWINDOW_ROB_SIZE);
        total_win_full = true;
      }
      if(slots_taken == 0){
        // keep track of how many threads possibly need ROB entries, including ourselves
        num_zero_slot_threads++;
      }
    }
  
    if(total_win_full){
      if(num_zero_slot_threads >= 1){
        ERROR_OUT("ERROR we have starved other threads! zero_slot_threads[ %d ] total_slots[ %d ] proc[ %d ] thread[ %d ] cycle[ %lld ]\n",
                  num_zero_slot_threads, total_slots, m_id, thread, m_local_cycles);
      }
      // we want to avoid starvation, so make sure there are some reserved entries left
      ASSERT(num_zero_slot_threads == 0);
      //ERROR_OUT("WINDOW FULL total_slots[ %d ] proc[ %d ] thread[ %d ] cycle[ %lld ]\n", total_slots, m_id, thread, m_local_cycles);
    }

    if(total_slots > IWINDOW_ROB_SIZE){
      setDebugTime( 0 );
      print();
      ERROR_OUT("ERROR we exceeded the ROB size total_slots[ %d ] ROB_SIZE[ %d ] num_zero_slots[ %d ] proc[ %d ] thread[ %d ] cycle[ %lld ]\n", total_slots, IWINDOW_ROB_SIZE, num_zero_slot_threads, m_id, thread, m_local_cycles);
    }
    ASSERT( total_slots <= IWINDOW_ROB_SIZE );

    result = (total_slots < IWINDOW_ROB_SIZE);
    // To prevent starvation we have to leave some reserve ROB entries left
    if(num_zero_slot_threads > 0){
      // sanity check - we should have free entries
      if((IWINDOW_ROB_SIZE - total_slots) == 0){
        ERROR_OUT("isSlotAvailable() ERROR NO FREE SLOTS\n");
        print();
      }
      ASSERT((IWINDOW_ROB_SIZE - total_slots) > 0);
      // check whether we are the only one that has zero slots
      if((num_zero_slot_threads == 1) && (m_iwin[thread].getNumSlotsTaken() == 0)){
        // we can go ahead and use the remaining entries
        return result;
      }
      // now check if we have enough entries left for other requestors. 
      // 2 cases: we are one of the zero slot threads ( use >= )
      //         or we are not one of the zero slot threads (use > )
      bool meet_reserve = false;
      if(m_iwin[thread].getNumSlotsTaken() == 0){
        ASSERT(num_zero_slot_threads > 1);
        meet_reserve = ((IWINDOW_ROB_SIZE - total_slots)/num_zero_slot_threads) >= 1;
      }
      else{
        // we are not one of the zero slot threads
        // we use > because we are requesting slots also
        // change 1 to RESERVED_ROB_ENTRIES to guarantee that many are reserved
        meet_reserve = ((IWINDOW_ROB_SIZE - total_slots)/num_zero_slot_threads) > 1;
      }

      return (result && meet_reserve);
    }
    return result;
  }


  //****************************************************************

  // Stats on unimplemented instructions 
  // see memop.C
  uint64        m_stat_va_out_of_range;

  // How many times readPhysicalMemory raised an exception (see memop.C)
  uint64   *     m_stat_memread_exception;

  // stats on uncacheable ASI usage
  uint64        m_stat_uncacheable_read_asi[MAX_NUM_ASI];
  uint64        m_stat_uncacheable_write_asi[MAX_NUM_ASI];
  uint64        m_stat_uncacheable_atomic_asi[MAX_NUM_ASI];

  //stats on functional ASI usage (IO or non-IO)
  uint64        m_stat_functional_read_asi[MAX_NUM_ASI];
  uint64        m_stat_functional_write_asi[MAX_NUM_ASI];
  uint64        m_stat_functional_atomic_asi[MAX_NUM_ASI];

  //returns pointer to store set predictor
  storeset_predictor_t * getStoreSetPred(){
    return m_store_set_predictor;
  }
  void updateStoreSet( la_t store_vpc, la_t load_vpc );

  // register state predictor
  regstate_predictor_t * getRegstatePred(){
    return m_regstate_predictor;
  }

  // Returns the current fetch status
  pseq_fetch_status_t getFetchStatus( unsigned int proc ){
    return m_fetch_status[proc];
  }

  // Returns fetch status string
  const char * getFetchStatusString( unsigned int proc ){
    return fetch_menomic(m_fetch_status[proc]);
  }

  //sets the transaction level for a processor
  void setTransactionLevel( mf_ruby_api_t * ruby_api );

  //**************************************************************************
 
private:

  /**
   * @name Fetch Helper functions
   */
  /** Implements a simple (easily idealizable) front end */
  void            fetchInstrSimple( );

  /// lookup a static instruction during fetch, used by fetchInstruction
  void            lookupInstruction( const abstract_pc_t *apc,
                                     static_inst_t** s_instr_ptr,
                                     pa_t* fetchPhysicalPC_ptr, unsigned int proc );
  
  /** Allocate a dynamic instruction during fetch, used by fetchInstruction.
   *  This function uses the cwp/gset information and modifies 'fetch_at'.
   *  Task: Fetch an instruction, insert it in the instruction window,
   *        Tag this instruction as Fetched,
   *        Advance PC/NPC state contained in fetch_at speculatively.
   */
  dynamic_inst_t *createInstruction( static_inst_t *s_instr, 
                                     abstract_pc_t *fetch_at, pa_t physicalPC, unsigned int proc );
  
  /**
   * @name Check functions for validating execution.
   */
  //@{

  /** analyze statistics for the last run
   */
  // NOT implemented
  void            threadStatAnalyze( void );  

  /** commitObserver: This method observes the committed instruction stream.
   */
  void            commitObserver( dynamic_inst_t *dinstr );

  /** uncheckedRetire: stores an instruction until simics is stepped past it
   *  @param dinst         The instruction to retire
   *  @return bool         true of retirement must happen immediately
   *                       otherwise unchanged.
   */
  bool            uncheckedRetire( dynamic_inst_t *dinstr, unsigned int proc );

  /** reads unchecked retires out of the instruction buffer.
   *  Used at check time to update statistics. You must free
   *  these instructions!!
   */
  dynamic_inst_t *getNextUnchecked(unsigned int proc );

  /** returns a memory instruction if there is an unchecked store instruction
   *  which has written to the same location.
   */
  memory_inst_t  *uncheckedValueForward( pa_t phys_addr, int my_size, unsigned int proc );

  /** Prints out unchecked instructions.
   */
  void            uncheckedPrint(unsigned int proc );

  /** push a recently retired instruction to a debugging buffer */
  void            pushRetiredInstruction( dynamic_inst_t *dinstr, unsigned int proc );

  /** print out the buffer of recently retired instructions */
  void            printRetiredInstructions(unsigned int proc );

  /** Helper function for checkAll and checkChanged state functions.
   *  This checks the critial portions of the state, such as PC and PSTATE
   *  registers.
   *  @param  result      contains the result of the comparison
   *  @param  mstate      Machine state (register box, abstract PC) to check.
   */
  void    checkCriticalState( check_result_t *result, mstate_t *mstate, unsigned int proc );

  /** Compare and update the simics architectural state with this sequencer's.
   *  This updates all of our registers, and returns if everything matches.
   *  @param  result      contains the result of the comparison
   *  @param  mstate      Machine state (register box, abstract PC) to check.
   */
  void    checkAllState( check_result_t *result, mstate_t *mstate, unsigned int proc );

  /** Compare and update changes we think we have made of a dynamic
   *  instruction. This updates only the registers we think have changed.
   *  @param  result     Contains the check result ('critical', 'verbose', etc)
   *  @param  mstate     The state after this instructions execution
   *  @param  d_instr    The dynamic instruction to be checked.
   */
  void    checkChangedState( check_result_t *result, mstate_t *mstate,
                             dynamic_inst_t *d_instr, unsigned int proc );

  /** Writes the destination registers of dinstr with the results from simics
   */
  void    updateInstructionState( dynamic_inst_t *dinstr, unsigned int proc );
  /** Comes in two varieties: dynamic and flow instructions. */
  void    updateInstructionState( flow_inst_t *flow_inst, unsigned int proc );

  /** allocate and initialize the out-of-order processor state */
  void    allocateRegBox( mstate_t &ooo );

  /** Allocate and initialize the ideal in-order processor state */
  void    allocateIdealState( void );
  //@}

  /**   Return the lowest Context ID in the m_ctxt_id array */
  ireg_t getLowID(ireg_t * ctxt_id){
    ireg_t min = ctxt_id[0];
    for(uint i=1; i < CONFIG_LOGICAL_PER_PHY_PROC; ++i){
      if(ctxt_id[i] < min){
        min = ctxt_id[i];
      }
    }
    return min;
  }

   /**   Return the highest Context ID in the m_ctxt_id array */
  ireg_t getHighID(ireg_t * ctxt_id){
    ireg_t max = ctxt_id[0];
    for(uint i=1; i < CONFIG_LOGICAL_PER_PHY_PROC; ++i){
      if(ctxt_id[i] > max){
        max = ctxt_id[i];
      }
    }
    return max;
  }

 /**  for ICount scheduling, check whether this thread has the lowest ICount, of all
        threads that are still available to be fetched from */
  bool isLowestICount(uint32 proc, bool * done){
    ASSERT(proc < CONFIG_LOGICAL_PER_PHY_PROC);
    //first, find the first available thread which can be fetched from, to initialize
    //    the minICount:
    uint32 minICount = 0;
    int minIndex = 0;
    bool found = false;
    for(uint k = 0; k < CONFIG_LOGICAL_PER_PHY_PROC; ++k){
      if(done[k] == false){
        minICount = m_icount[k];
        found = true;
        minIndex = k;
        break;
      }
    }
    if(found == true){
      //second, find the lowest ICount of the remaining threads that are available to fetch:
      for(uint k = minIndex; k < CONFIG_LOGICAL_PER_PHY_PROC; ++k){
        if(!done[k] && m_icount[k] < minICount){
          minICount = m_icount[k];
        }
      }
      return ( (m_icount[proc] == minICount) ? true: false);
    }
    else{
      //Why would found be false? Because we are looping inside the fetch, which means that
      //  we can get into a situation where our predecessor logical processor was the last 
      //  thread that could be fetched from.  Subsequent threads therefore will not find any avail. 
      //  threads to fetch from!
      return false;
    }
  }

  /** This function is used by fetchInstSimple() in order to decide whether to continue or stop the fetching of
   *  instructions.  There are 2 conditions under which instruction fetching will terminate:
   *     1) We have fetched to our user-defined limit (CONFIG_FETCH_THREADS_PER_CYCLE)
   *     2) All fetchable threads are done (ie already fetched some instrs, have pending ICache miss, 
   *                                         reached fetch barrier, or have ITLB miss)
   **/
  bool continueFetch(uint32 total_fetched, bool * done, uint32 num_threads_fetched){
    ASSERT(done != NULL);
    if(total_fetched >= MAX_FETCH){
      //we have reached the fetch limit
      return false;
    }
    if(num_threads_fetched >= m_threads_per_cycle){
      //we have fulfilled condition 1)
      return false;
    }
    for(uint i=0; i < CONFIG_LOGICAL_PER_PHY_PROC; ++i){
      if(done[i] == false){
        // we can continue fetching
        return true;
      }
    }
    //we have reached condition 2): 
    return false;  
  }

  /** This function is used by decodeInstruction() in deciding whether to stop decoding of instructions.
   *      Currently decodeInstruction() implements a Round-Robin policy, and each logical proc is allowed
   *      to decode up to MAX_DECODE/CONFIG_LOGICAL_PER_PHY_PROC instrs per decode round.
   *      We stop decoding when one of the conditions is met during a decode round:
   *              1) We have decoded MAX_DECODE instrs
   *              2)  We have decoded < MAX_DECODE and no more instrs are left to decode
   */
  bool continueDecode(uint32 total_decoded, uint32 * decode_remaining){
    ASSERT(decode_remaining != NULL);
    if(total_decoded >= MAX_DECODE){
      return false;                 //We have reached our decode limit (case 1)
    }
    for(uint proc=0; proc <  CONFIG_LOGICAL_PER_PHY_PROC; ++proc){
      if( decode_remaining[proc] > 0 && m_ooo.regs.registersAvailable(proc)){
        return true;             // We have not reached our limit and we still have instrs left to decode
      }
    }
    ASSERT( total_decoded < MAX_DECODE);
    return false;       // We have no more instrs left to decode (case 2) 
  }

  /** This function is used by scheduleInstruction(), see the comments for continueDecode() (uses similar
   *       stopping conditions)
   */
  bool continueSchedule(uint32 total_scheduled, uint32 * sched_remaining, bool * done){
    ASSERT(sched_remaining != NULL);
    ASSERT(done != NULL);
    if(total_scheduled >= MAX_DISPATCH){
      return false;                 //We have reached our dispatch limit (case 1)
    }
    for(uint proc=0; proc <  CONFIG_LOGICAL_PER_PHY_PROC; ++proc){
      if( sched_remaining[proc] > 0 && !done[proc]){
        return true;             // We have not reached our limit and we still have instrs left to dispatch
                                    //     and we have not exceeded our scheduling window
      }
    }
    ASSERT( total_scheduled < MAX_DISPATCH);
    return false;       // We have no more instrs left to dispatch (case 2) 
  }

  /** This function is used by retireInstruction(), see the comments for continueDecode() (uses similar
   *       stopping conditions)
   */
  bool continueRetire(uint32 total_retired, bool * done){
    ASSERT(done != NULL);
    if(total_retired >= MAX_RETIRE){
      return false;                 //We have reached our retire limit (case 1)
    }
    for(uint proc=0; proc <  CONFIG_LOGICAL_PER_PHY_PROC; ++proc){
      if(!done[proc]){
        return true;             // We have not reached our limit and we still have instrs left to retire
      }
    }
    ASSERT( total_retired < MAX_RETIRE);
    return false;       // We have no more instrs left to retire (case 2) 
  }

  /** @name Private Variables */
  /// identifier for this sequencer
  int32      m_id;

  /// per logical processor's waiter object
  proc_waiter_t ** m_proc_waiter;

  /// per logical processor's instruction counter (for SMT scheduling)
  uint32   *     m_icount;

  /// a variable indicating from how many unique threads we should fetch from each cycle:
  uint32          m_threads_per_cycle;

  /// a variable indiciating the total number of instrs the current logical proc
  ///          can fetch (in a given cycle).  This is a dynamic upper limit up to MAX_FETCH.
  uint32     m_max_fetch_per_cycle;

  /// variable indicating total number of instrs current logical proc can decode per
  ///            cycle (dynamic limit up to MAX_DECODE)
  uint32      m_max_decode_per_cycle;

  /// variable indicating total number of instrs current logical proc can schedule per
  ///            cycle (dynamic limit up to MAX_DISPATCH)
  uint32      m_max_dispatch_per_cycle;

  /// variable indicating total number of instrs each logical proc can retire per cycle
  ///            This is a dynamic limit (up to MAX_RETIRE)
  uint32       m_max_retire_per_cycle;

  /// local time for this processor
  tick_t     m_local_cycles;
  /// local dynamic (issue) sequence number for this processor:
  ///     # of instructions decoded
  uint64   *  m_local_sequence_number;

  /// local sequence number for inorder model for this processor:
  uint64  *   m_local_inorder_number;

  /// local instruction count for this processor: # of instr's completed.
  uint64   *  m_local_icount;

  /// simics pc step time for last event
  pc_step_t  m_simics_time;
  //NOT used??
  //pc_step_t  * m_simics_time;

  /// instruction window:
  // each logical proc has its own Instruction window/ROB structure (for correct Squash)
  //           We give the illusion of a shared Instruction Buffer by computing how many slots
  //            are taken up by each thread (and keeping this <= IWINDOW_ROB_SIZE)
  iwindow_t   * m_iwin;

  /// miss status holding register
  mshr_t    *m_mshr;

  /**
   * @name  Core state (registers, fetch state)
   */
  //@{
  /** scheduler: Contains the event queue for instructions.
   *             Wakes up dynamic instructions for execution. */
  scheduler_t     *m_scheduler;

  // The out-of-order registers, fetch at state, and critical regs
    mstate_t         m_ooo;
  //  mstate_t  *    m_ooo;

  /** The non-speculative in-order program counter -- 
   *  Only the PC and NPC fields of this structure should be used.
   *  This is maintained separately from the control register information,
   *  because if it were the control PC & NPCs would need to be renamed
   *  for each instruction.
   */
  //each logical proc should have its own inorder PC pointer
  abstract_pc_t   ** m_inorder_at;

  /** The speculative abstract program counter. This structure contains
   *  the PC and NPC, both of which are needed to determine the program's
   *  control flow. The PC points one instruction past the current retired
   *  program counter. (i.e. at the next instruction to fetch)
   *
   *  NOTE: this redundantly points to the PC in the m_ooo structure.
   *  I kept it here because its name is much more descriptive to me.
   */
  //each logical proc should have its own fetch PC pointer
  abstract_pc_t   ** m_fetch_at;

  // used to keep track of last trap encountered
  trap_type_t      **    m_last_traptype;
  // keep track of the last traplevel
  int     *         m_last_traplevel;
  // used to keep track of when last trap occurred
  uint64     **     m_last_traptype_cycle;

  // saves the last I-cache miss addr 
  pa_t     *        m_last_icache_miss_addr;
  // flag indicating whether last icache miss was L2 miss
  bool      *       m_last_icache_l2miss;

  // used to track the last exception generated by Simics
  trap_type_t  *  m_last_simexception;

  storeset_predictor_t  *  m_store_set_predictor;

  regstate_predictor_t *   m_regstate_predictor;

  /// CM FIX: I'd like to remove both the control rf, and the cc rf pointers,
  //          and only leave access through the "arf" interface.
  /// condition-code registers
  /// physical condition code registers
  physical_file_t *m_cc_rf;

  /// condition code map at retire time
  reg_map_t       *m_cc_retire_map;

  /// physical control state register register files
  physical_file_t *** m_control_rf;

  /// control register file abstract interface
  arf_control_t    *m_control_arf;

  /** debugging variable: shadow pstate:
   *  detects when the processor state changes, so we can minimize debugging */
  uint16     m_shadow_pstate;
    
  /** The insertion point into the recent retires buffer. */
  int32 * m_recent_retire_index;

  /** debugging variable: recent_retires
   *  A buffer recording state about recently retired instructions.
   */
  dynamic_inst_t *** m_recent_retire_instr;

  //@}

  /** LSQ: pointer to the load/store queue for this processor */
  lsq_t ** m_lsq;

  /** The pointer to the Write Buffer */
  writebuffer_t * m_write_buffer;

  /** @name Cache support: L1 I/D caches, L2 caches */
  //@{
  /// L1 miss status holding register structure
  mshr_t      *il1_mshr;
  mshr_t      *dl1_mshr;

  /// L1 instruction cache
  generic_cache_template<generic_cache_block_t> *l1_inst_cache;
  /// L1 data cache
  generic_cache_template<generic_cache_block_t> *l1_data_cache;

  /// L2 miss status holding register structure
  mshr_t      *l2_mshr;
  /// L2 (unified) cache
  generic_cache_template<generic_cache_block_t> *l2_cache;
  /// alternative to (uniprocessor cache structure): the ruby cache
  rubycache_t *m_ruby_cache;
  //@}

  /** @name MMU support: I/D-TLB, contexts */
  //@{

  /// MMU: memory management unit interface (may model a Spitfire MMU)
  /// each logical processor should have its own interface
  mmu_interface_t ** m_mmu_access;

  /// MMU: memory management unit's ASI interface
  /// each logical processor should have its own interface
  mmu_interface_t ** m_mmu_asi;

  /** primary context for the current process running on the computer
   */
  context_id_t * m_primary_ctx;

  /** An array of context ids used during installInterfaces, initialized when
   *    going through each logical processor
  */
  ireg_t    *   m_ctxt_id;

  /** A cached translation of logical to physical address for the ITLB.
   *  This single entry gives the physical address for the current page
   *  instruction fetch is being performed on. The physical address for
   *  the itlb mini-cache.
   */
  pa_t * m_itlb_physical_address;

  /** The logical address for the itlb mini-cache */
  //la_t          m_itlb_logical_address;
  la_t * m_itlb_logical_address;

  /** A TLB- used in stand-alone runs- to examine the effects of different
   *  virtual->physical memory mappings
   */
  dtlb_t       **m_standalone_tlb;

  /** the mmu hap handler */
  hap_handle_t  m_mmu_haphandle;
  
  /** the physical address of this sequencers thread id */
  pa_t * m_thread_physical_address;

  //@}

  /** control flow predictors - direct branch predictor
   * (bpred and ipred can share resources with other sequencers) */
  direct_predictor_t   * m_bpred;
  /** control flow predictors - indirect branch predictor */
  cascaded_indirect_t  * m_ipred;

  /** pointer to the return address stack - */
  ras_t ** m_ras;

  /** pointer to the trap return stack */
  tlstack_t ** m_tlstack;

  /** speculative predictor state -- */
  predictor_state_t * m_spec_bpred;
  /** architected predictor state -- */
  predictor_state_t * m_arch_bpred;
  
  /** @name Fetch unit fields. */
  //@{
  /** The fetch unit status. PSEQ_FETCH_* constants only
   */
  //each SMT logical proc should have its own fetch state
  pseq_fetch_status_t * m_fetch_status;

  /// Saved fetch statuses for when the Write Buffer stalls the front-end:
  pseq_fetch_status_t * m_saved_fetch_status;

  /// The future time when the fetch will resume after the squash
  tick_t  *   m_fetch_ready_cycle;

  /// The next line address used to access multicycle I$
  pa_t   *    m_next_line_address;

  /// flag to tell line buffer fetch to stall
   bool   *    m_line_buffer_fetch_stall;

  /// requested line address by the fetch engine
   pa_t   *    m_fetch_requested_line;

  /// flag to tell if the engine is in waiting mode
   bool   *    m_wait_for_request;

  /** last physical address fetched from the I$, used to emulate
   *  the line buffer, which means each cache line is fetch only
   *  once if !same_line(pc1, pc2)
   *
   *  In the case of multicycle I$ frontend, this is the current
   *  line buffer address
   */
  pa_t  *     m_last_fetch_physical_address;

  /// In multicycle I$, this is the line buffer address of last cycle
  pa_t   *    m_last_line_buffer_address;

  /** Indicates the number of instructions retired, since stepping simics.
   */
  uint32    *       m_unchecked_retires;

  /** top of the unchecked retire stack: lets retires be fetched in order. */
   uint32    *       m_unchecked_retire_top;

  /** a map from store address to a dynamic instruction for unchecked retires*/

  /** A buffer recording state about retired instructions, which haven't
   *  yet been commited by simics.
   */
  //all logical procs use the same global buffer (should be correct for now....) - TODO -
  //           check this for multiple SMT threads, if using a common buffer causes problems
   dynamic_inst_t *** m_unchecked_instr;

  /** a static instruction which is fetched when an i-tlb miss happens
   */
   static_inst_t   ** m_fetch_itlbmiss;  

  /** A circular buffer holding the address of I$ lines successfully
   *  fetched each cycle. */
  FiniteCycle<pa_t>   m_i_cache_line_queue;

  /** A circular buffer holding the number of instructions successfully
   *  fetched each cycle. */
  // each logical proc should have its own buffer - this is OK bc the instruction fetch is limited by
  //          instruction window size, and we are assumin threads are running in "lockstep" (ie we don't
  //          increment m_local_cycle until all threads have finished going through all pipe stages)
  FiniteCycle<uint32> * m_fetch_per_cycle;

  /** A circular buffer holding the number of instructions successfully
   *  decoded each cycle. */
  FiniteCycle<uint32> * m_decode_per_cycle;

  // FIXFIXFIX: put in a better place

  /// The scheduling actor: waits for inputs to become ready, then executes the instruction
  actor_t    *m_act_schedule;
  //@}

  /**
   * @name Retire related members -- includes exceptions
   */
  //@{  
  /** The number of instruction retire opportunities, before asserting that
   *  something bad happened (like dead or live lock). */
  uint64      m_fwd_progress_cycle;
  
  /// If simics reports an interrupt occurs on this processor it is recorded here
  trap_type_t * m_posted_interrupt;

  /** pointer to last good instruction */
  uint32 *     m_next_exception;

  /** The instruction which caused this exception */
  enum i_opcode * m_except_offender;

  /** program counter to fetch at after exception */
  abstract_pc_t * m_except_continue_at;

  /** memory address for TLB miss */
  my_addr_t  * m_except_access_addr;

  /** type of exception */
  exception_t * m_except_type;

  /** cycle penalty of this exception */
  uint32  *    m_except_penalty;

  //@}

  /**
   * @name Cache of static (decoded) instructions
   */
  //@{
  /// current instruction map of decoded static instructions by physical PC
  ipagemap_t   ** m_imap;

  //@}

  /** @name Configuration support: reading / writing state to disk */
  //@{
  confio_t     *m_conf;
  //@}

  /// A file interface object to the trace being read or written
  tracefile_t    ** m_tracefp;

  /// A file interface object to a branch trace trace being read or written
  branchfile_t   ** m_branch_trace;

  /// A file interface object to a memory trace being read or written
  memtrace_t     ** m_memtracefp;

  /**
   * @name  Idealized fields for the CPU
   */
  //@{
  /// number of instructions retired in "ideal mode" by opal
  int64    *           m_ideal_retire_count;

  /// sequence number of last ideal instruction checked by simics
  int64       *        m_ideal_last_checked;

  /** sequence number of first instruction that is predictable.
   *  This marks the boundary between "freed" flow_insts and those that are
   *  live.
   */
  int64    *           m_ideal_last_freed;

  /// sequence number of the first instruction that is predictable
  int64      *         m_ideal_first_predictable;

  /// sequence number of the last instruction that is predictable
  int64    *           m_ideal_last_predictable;

  /// machine state (future file) for the in-order functional processor
    mstate_t       *        m_inorder;

  /// machine state (architected file) for the in-order functional processor
  mstate_t            m_check;

  /// ideal predecessor register
  flow_inst_t ** m_cfg_pred;

  /// The physical register file (touches internal regfile of m_inorder struct)
  physical_file_t    ** m_ideal_control_rf;

  /// ideal opstat statistics
  decode_stat_t    *   m_ideal_opstat;

  /// ideal fetch unit status (possibly stalled on ITLB miss)
  pseq_fetch_status_t * m_ideal_status;

  /// control flow graph index: one node for each sequence number
  CFGIndex      *      m_cfg_index;

  /// memory   dependence chain
  mem_dependence_t   ** m_mem_deps;
  //@}

  /**
   * @name Statistics related functions
   */
  //@{
  
  /// opcode statistics
  decode_stat_t   *   m_opstat;

  /// opal internal squash statistics
  uint64    **         m_exception_stat;

  /// opal internal register use statistics
  uint32           **m_reg_use[RID_NUM_RID_TYPES];

  /// timer to measure how long simulation takes
  utimer_t           m_simulation_timer;
  //utimer_t     *      m_simulation_timer;

  /// timer to measure how long retirement checking (stepping simics) takes
  utimer_t           m_retirement_timer;

  /// number of times the functional simulator is used to retire instructions
  uint64      *       m_stat_count_functionalretire;

  /// number of times squashing due to retirement problems
  uint64      *       m_stat_count_badretire;

  /// number of times squashing due to done/retries
  uint64      *       m_stat_count_retiresquash;

  ///number of times the scheduling windows was exceeded
  uint64 * m_stat_exceed_scheduling_window;
  
  /// note how many times we run out of registers...
  uint64 * m_stat_not_enough_registers;

  /// Pointer to the WATTCH power stats
  power_t * m_power_stats;



public:
  /// per cpu stopwatch counting simulated time
  stopwatch_t      ** m_overall_timer;
  /// per cpu stopwatch for current thread's simulated time
  stopwatch_t       ** m_thread_timer;
  /// histogram of instructions retired by the current thread
  histogram_t       ** m_thread_histogram;
  /// count of atomic instructions excluded from the lock structure
  uint64      *       m_exclude_count;
  /// number of instructions executed by this thread
  uint64      *       m_thread_count;
  /// number of idle instructions executed by this thread
  uint64       *      m_thread_count_idle;

  /// # of times each trap occurs (in opal) 
  uint64     **        m_trapstat;
  /// # times each trap completes
  uint64     **        m_completed_trapstat;
  /// # times a software initiated trap occurs (in opal) 
  uint64    **         m_software_trapstat;

  /// # of times each trap occurs (in simics)
  uint64     **        m_simtrapstat;
  /// asi read-access statistics
  uint64             m_asi_rd_stat[MAX_NUM_ASI];
  /// asi write-access statistics
  uint64             m_asi_wr_stat[MAX_NUM_ASI];
  /// asi atomic-access statistics
  uint64             m_asi_at_stat[MAX_NUM_ASI];

  /// histogram of fetch, decode, dispatch, and retired instructions per cycle
  uint64            *m_hist_fetch_per_cycle;
  ///  how many different threads we fetched from each cycle
  uint64            *m_hist_smt_fetch_per_cycle;     
  uint64            *m_hist_decode_per_cycle;
  uint64            *m_hist_schedule_per_cycle;
  uint64            *m_hist_retire_per_cycle;

  /// per-thread histograms
  uint64            ** m_hist_fetch_per_thread;
  uint64            ** m_hist_decode_per_thread;
  uint64           ** m_hist_schedule_per_thread;
  uint64          ** m_hist_retire_per_thread;

  uint64           **m_hist_squash_stage;
  uint64            *m_hist_decode_return;
  
  /// histogram of fetch stalls
  uint64            *m_hist_fetch_stalls;
  /// histogram of retire stalls
  uint64            *m_hist_retire_stalls;
  /// histogram of fastforward lengths
  uint64            *m_hist_ff_length;
  /// histogram of ideal predictor coverage
  uint64            *m_hist_ideal_coverage;

  /// Retirement Not-Ready stage histogram 
  uint64       * m_stat_retire_notready_stage;

  /// Used to track the last time we initiated a stall due to WB being full
  tick_t              m_wb_stall_start_cycle;

  /// Used to track the last time we unstalled the fetch from WB being full
  tick_t              m_wb_stall_end_cycle;
  /// count of times that fetch stalls across branch boundaries
  uint64             m_stat_no_fetch_taken_branch;
  /// count of times that fetch stalls across cache-line boundaries
  uint64             m_stat_no_fetch_across_lines;

  /// count of functional unit utilization
  uint64            *m_stat_fu_utilization;
  /// count of functional unit stalls
  uint64           * m_stat_fu_stall;
  /// count of functional unit utilization that retired
  uint64            ** m_stat_fu_util_retired;
  
  /// user, kernel, tatal, all three stats are recorded
  static const uint32 TOTAL_INSTR_MODE = 3;
  /// branch predictor statistics
  /// number of times the branch is predicted
  uint64             m_branch_pred_stat[BRANCH_NUM_BRANCH_TYPES][TOTAL_INSTR_MODE];
  /// number of times each type of branch is seen at retire time
  uint64             m_branch_seen_stat[BRANCH_NUM_BRANCH_TYPES][TOTAL_INSTR_MODE];
  /// number of times the branch predictor is correct
  uint64             m_branch_right_stat[BRANCH_NUM_BRANCH_TYPES][TOTAL_INSTR_MODE];
  /// number of times the branch predictor is wrong
  uint64             m_branch_wrong_stat[BRANCH_NUM_BRANCH_TYPES][TOTAL_INSTR_MODE];

  /// number of times a static branch predictor predicts incorrectly
  uint64             m_branch_wrong_static_stat;

  /// number of times the branch hits in the exception table of indirect pred
  uint64             m_branch_except_stat[BRANCH_NUM_BRANCH_TYPES];

  /// number of predicted branches decoded
  uint64       *      m_pred_count_stat;
  /// number of branches predicting "taken"
  uint64       *      m_pred_count_taken_stat;
  /// number of branches predicting "not-taken"
  uint64      *       m_pred_count_nottaken_stat;
  /// number of non-predicated branches decoded
  uint64     *        m_nonpred_count_stat;

  /// number of predictated branches on a register decoded
  uint64     *        m_pred_reg_count_stat;
  /// number of branches predicting "taken"
  uint64     *        m_pred_reg_taken_stat;
  /// number of branches predicting "not-taken"
  uint64     *        m_pred_reg_nottaken_stat;

  /// number of predictated branches retired
  uint64   *          m_pred_retire_count_stat;
  /// number of branches predicting "taken"
  uint64    *         m_pred_retire_count_taken_stat;
  /// number of branches predicting "not-taken"
  uint64     *        m_pred_retire_count_nottaken_stat;
  /// number of non-predicated branches retired
  uint64      *       m_nonpred_retire_count_stat;

  /// number of predictated branches on a register retired
  uint64      *       m_pred_reg_retire_count_stat;
  /// number of branches predicting "taken"
  uint64       *      m_pred_reg_retire_taken_stat;
  /// number of branches predicting "not-taken"
  uint64       *      m_pred_reg_retire_nottaken_stat;

  /// register use statistics
  /// count of cycles when a decode stall is due to insufficient registers
  uint64   *          m_reg_stall_count_stat;
  /// count of number of instructions for decode after a register stall
  uint64    *         m_decode_stall_count_stat;
  /// count of cycles when a schedule stall due to instruction window full
  uint64     *        m_iwin_stall_count_stat;
  /// count of number of instructions for decode after a register stall
  uint64      *       m_schedule_stall_count_stat;
  /// number of times a load requests a bypass value, before it is ready
  uint64     *        m_stat_early_store_bypass;

  //icount histogram
  struct icount_histogram_t{
    uint64 icount_sum;
    uint64 max_icount;
    uint64 num_samples;
  };

  icount_histogram_t * m_icount_stats;
  
  void addICountStat(int thread, int icount){
    m_icount_stats[thread].icount_sum += icount;
    m_icount_stats[thread].num_samples++;
    if(icount > m_icount_stats[thread].max_icount){
      m_icount_stats[thread].max_icount = icount;
    }
  }

  /// Cache miss latencies
  struct miss_latency_histogram_t{
    uint64 latency_sum;
    uint64 num_misses;
    uint64 num_fastpath;
  };

  miss_latency_histogram_t * m_ifetch_miss_latency;
  miss_latency_histogram_t * m_load_miss_latency;
  miss_latency_histogram_t * m_store_miss_latency;
  miss_latency_histogram_t * m_atomic_miss_latency;

  void addIfetchLatencyStat(uint64 latency, int thread){
    
    m_ifetch_miss_latency[thread].num_misses++;
    m_ifetch_miss_latency[thread].latency_sum += latency;
    
  }

  void addLoadLatencyStat(uint64 latency, int thread){
    
    m_load_miss_latency[thread].num_misses++;
    m_load_miss_latency[thread].latency_sum += latency;
    
  }
  
  void addStoreLatencyStat(uint64 latency, int thread){
    
    m_store_miss_latency[thread].num_misses++;
    m_store_miss_latency[thread].latency_sum += latency;
    
  }

  void addAtomicLatencyStat(uint64 latency, int thread){
    
    m_atomic_miss_latency[thread].num_misses++;
    m_atomic_miss_latency[thread].latency_sum += latency;
    
  }

  void addIfetchFastpathStat(int thread){
    m_ifetch_miss_latency[thread].num_fastpath++;
  }

  void addLoadFastpathStat(int thread){
    m_load_miss_latency[thread].num_fastpath++;
  }
  
  void addStoreFastpathStat(int thread){
    m_store_miss_latency[thread].num_fastpath++;
  }

  void addAtomicFastpathStat(int thread){
    m_atomic_miss_latency[thread].num_fastpath++;
  }

  // WATTCH power
  //    Used to simulated power of banked L1 caches
  void L1IBankNumStat(int banknum);
  void L1DBankNumStat(int banknum);
  void collectBankNumStat();

  //bank number accesses and stats
  //assume 8 banks for now
  uint64 m_l1i_banknum_accesses[8];
  uint64 m_l1d_banknum_accesses[8];

  //histogram of how many different banks are accessed each cycle
  uint64 m_l1i_8bank_histogram[9];
  uint64 m_l1d_8bank_histogram[9];

  //assuming only 4 banks
  uint64 m_l1i_4bank_histogram[5];
  uint64 m_l1d_4bank_histogram[5];

  //assuming only 2 banks
  uint64 m_l1i_2bank_histogram[3];
  uint64 m_l1d_2bank_histogram[3];

  // L2 miss dependency stats
  struct l2miss_dep_stat {
    uint64 total_sum;
    uint64 total_samples;
    int min;
    int max;
  };

  l2miss_dep_stat  m_stat_l2miss_dep;

  void resetL2MissDepStats(){
    m_stat_l2miss_dep.total_sum = 0;
    m_stat_l2miss_dep.total_samples = 0;
    m_stat_l2miss_dep.min = 99999;
    m_stat_l2miss_dep.max = 0;
  }

  void updateL2MissDep( int total_dep ){
    if(total_dep < m_stat_l2miss_dep.min){
      m_stat_l2miss_dep.min = total_dep;
    }
    if(total_dep > m_stat_l2miss_dep.max){
      m_stat_l2miss_dep.max = total_dep;
    }
    m_stat_l2miss_dep.total_sum += total_dep;
    m_stat_l2miss_dep.total_samples++;
  }
    
  void printL2MissDepStats(){
    out_info("\n***L2MissDep Stat:\n");
    out_info("\tSum = %lld Samples = %lld\n", m_stat_l2miss_dep.total_sum, m_stat_l2miss_dep.total_samples);
    out_info("\tMin = %lld\n",m_stat_l2miss_dep.min);
    out_info("\tMax = %lld\n", m_stat_l2miss_dep.max);
    double avg = 0.0;
    if(m_stat_l2miss_dep.total_samples > 0){
      avg = 1.0*m_stat_l2miss_dep.total_sum/m_stat_l2miss_dep.total_samples;
    }
    out_info("\tAVG = %6.3f\n", avg);
  }

  /// number of instructions read from trace
  uint64   * m_stat_trace_insn;
  /// total number of instructions committed
  uint64  *  m_stat_committed;
  /// total number of times squash is called (includes branch mispredicts)
  uint64  *  m_stat_total_squash;
  /// total number of instructions squashing on commit (only commit squashes)
  uint64  *  m_stat_commit_squash;
  /// number of times an asi store causes a squash
  uint64  *  m_stat_count_asistoresquash;
  /// total number of instructions committing successfully
  uint64  *  m_stat_commit_good;
  /// total number of instructions committing unsuccessfully
  uint64  *  m_stat_commit_bad;
  /// total number of unimplemented instructions committing
  uint64  *  m_stat_commit_unimplemented;
  /// total number of exceptions
  uint64 *  m_stat_count_except;

  /// total number of loads retired
  uint64  *  m_stat_loads_retired;
  /// total number of stores retired
  uint64   * m_stat_stores_retired;
  /// number of retiring stores which don't have the correct cache line permissions
  uint64   * m_stat_retired_stores_no_permission;
  /// number of retiring atomics which don't have the correct cache line permissions
  uint64   * m_stat_retired_atomics_no_permission;
  /// number of retiring loads which don't have the correct cache line permissions
  uint64   * m_stat_retired_loads_no_permission;
  /// number of retiring loads in which completion and retirement load data differs:
  uint64   * m_stat_retired_loads_nomatch;
  /// total number of atomics retired
  uint64  *  m_stat_atomics_retired;
  /// total number of prefetches retired
  uint64  *  m_stat_prefetches_retired;
  /// total number of control insts committed
  uint64  *  m_stat_control_retired;

  /// total number of instructions fetched
  uint64  *  m_stat_fetched;
  /// total number of instructions that miss in mini-itlb
  uint64   * m_stat_mini_itlb_misses;
  /// total number of instructions decoded
  uint64   * m_stat_decoded;
  /// total number of instructions executed
  uint64   * m_stat_total_insts;

  /// total number of loads executed
  uint64  *  m_stat_loads_exec;
  /// total number of stores executed
  uint64   * m_stat_stores_exec;
  /// total number of atomics executed
  uint64   * m_stat_atomics_exec;
  /// total number of prefetches executed
  uint64   * m_stat_prefetches_exec;
  /// total number of control insts executed
  uint64   * m_stat_control_exec;

  /// loads which are found in the trace
  uint64   * m_stat_loads_found;
  /// loads which are not found in the trace
  uint64   * m_stat_loads_notfound;
  /// total number of spill traps
  uint64   * m_stat_spill;
  /// total number of fill traps
  uint64   * m_stat_fill;

  /* Write buffer stats */
  /// Number of times loads hit to the write buffer
  uint64 *  m_stat_num_write_buffer_hits;
  /// Number of times the write buffer was full
  uint64 * m_stat_num_write_buffer_full;

  /* cache stats */
  /// number fetches which miss in icache
  uint64 *  m_stat_num_icache_miss;
  /// number of fetches that hit in the mhsr (after missing in i-cache)
  uint64 *  m_stat_icache_mshr_hits;
  /// number instructions which miss in dcache
  uint64 *  m_stat_num_dcache_miss;
  /// number of instructions that retire (having missed in the dcache)
  uint64 *  m_stat_retired_dcache_miss;
  /// number of instructions with long latency misses
  uint64 *  m_stat_retired_memory_miss;
  /// number of instructions that hit in the mshr structure
  uint64 *  m_stat_retired_mshr_hits;
  /// number of load/store instructions to I/O space
  uint64 *  m_stat_count_io_access;
  /// number of times the cache stalls when trying to issue a memory request
  uint64 *  m_num_cache_not_ready;

  /// number of main memory misses currently outstanding
  uint64  * m_stat_miss_count;
  /// the last miss to main memory's sequence number
  uint64   * m_stat_last_miss_seq;
  /// the last miss to main memory's fetch time
  tick_t   * m_stat_last_miss_fetch;
  /// the last miss to main memory's issue time
  tick_t   * m_stat_last_miss_issue;
  /// the last miss to main memory's retire time
  tick_t   * m_stat_last_miss_retire;
  
  /// count of instructions that are effective (independent or not)
  uint64  *  m_stat_miss_effective_ind;
  uint64  *  m_stat_miss_effective_dep;
  /// count of instructions that are inter-cluster instructions
  uint64   * m_stat_miss_inter_cluster;

  /// histogram of effective, independent instructions
  histogram_t *m_stat_hist_effective_ind;
  /// histogram of effective, dependent instructions
  histogram_t *m_stat_hist_effective_dep;
  /// histogram of cache inter-arrival times (# of instructions, not cycles)
  histogram_t *m_stat_hist_inter_cluster;
  /// histogram of counts of outstanding cache misses
  histogram_t *m_stat_hist_misses;
  /// histogram of cache miss "inter-arrival" cycles
  histogram_t *m_stat_hist_interarrival;
  /// histogram of dependent memory operations
  histogram_t *m_stat_hist_dep_ops;

  /* lsq stats */
  /// number of loads satisfied by store queue
  uint64 *  m_stat_load_bypasses;
  /// number of atomics satisfied by store queue
  uint64 *  m_stat_atomic_bypasses;
  /// number of stores scheduled before value
  uint64 *  m_stat_num_early_stores;
  /// number of loads waiting for early store resolution
  uint64 *  m_stat_num_early_store_bypasses;
  /// number of atomics scheduled before value
  uint64 *  m_stat_num_early_atomics;
  /// number of load-store conflicts due to load executing before store, but store comes before
  //.    load in program order
  uint64 *  m_stat_load_store_conflicts;
  /// number of load-store conflicts in which load reads incorrect value from an older store than 
  ///    the store it needs to get the data from
  uint64 *  m_stat_load_incorrect_store;
  /// same as above except for atomics reading value from incorrect store
  uint64 *  m_stat_atomic_incorrect_store;

  /* StoreSet predictor stats */
  /// number of times the StoreSet predictor stalled loads
  uint64 *  m_stat_storeset_stall_load;
  // number of times the StoreSet predictor stalled atomics
  uint64 *  m_stat_storeset_stall_atomic;

  /// number of stale data predictions made
  uint64  * m_stat_stale_predictions;
  /// number of stale data predictions successful
  uint64  * m_stat_stale_success;
  /// a histogram of successful stale data speculations
  uint64  * *m_stat_stale_histogram;

  /// number of times simics is stepped
  uint64  * m_stat_continue_calls;
  /// number of instructions overwritten
  uint64 *   m_stat_modified_instructions;

  /// The number of times the ideal processor was not able to reach the end of the window
  uint64  *  m_inorder_partial_success;
  //@}
};

/**
 * This implments a simple waiter object, which is used by the sequencer when waiting for
 * instruction cache misses.  Having multiple proc_waiter_t objects is necessary because for SMT
 * support multiple waiters are necessary per each pseq_t object (one for each logical processor), 
 * so passing a pseq_t pointer to rubycache_t's access() function no longer works.
 * All this object does is to call pseq_t's Wakeup() function, which should wakeup the correct
 * logical processor...
 */

class proc_waiter_t: public waiter_t{
 public:
  //Wakeup function: just call pseq_t's Wakeup() function
  void Wakeup(){
       ASSERT(m_pseq != NULL);
     #ifdef DEBUG_PSEQ
       DEBUG_OUT("proc_waiter_t: Wakeup CALLED!\n");
      #endif
       m_pseq->Wakeup(m_logical_proc);
  }

  //Constructor : takes no arguments (for dynamically allocated proc_waiter_t objects)
  proc_waiter_t(){
    m_pseq = NULL;
    m_logical_proc = 0;
  }

  //Constructor: use default constructor
  proc_waiter_t(pseq_t * pseq, uint32 proc):waiter_t(){
    m_pseq = pseq;  
    m_logical_proc = proc;
  }

 private:
    //The pointer to the pseq object (which you need in order to call its Wakeup()
    pseq_t * m_pseq;
    uint32 m_logical_proc;

};


/*------------------------------------------------------------------------*/
/* Global variables                                                       */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Global functions                                                       */
/*------------------------------------------------------------------------*/

#endif  /* _PSEQ_H_ */
