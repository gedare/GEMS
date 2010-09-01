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
#ifndef _CHAIN_H_
#define _CHAIN_H_

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

/// The maximum chain depth ( -depth/2, 0, depth/2 )
const int32 CHAIN_MAX_DEPTH   = 9;

/*------------------------------------------------------------------------*/
/* Class declaration(s)                                                   */
/*------------------------------------------------------------------------*/

/**
* Tool to analyze the dependence information found in memory dependence
* chains.
*
* @see     ptrace_t
* @author  cmauer
* @version $Id: chain.h 1.3 04/08/05 15:59:34-05:00 lyen@opus.cs.wisc.edu $
*/
class chain_t {

public:
  /**
   * @name Constructor(s) / destructor
   */
  //@{

  /**
   * Constructor: creates object
   * @param id           The sequencer's id associated with this analyzer
   * @param window_size  The size of window to analyze.
   */
  chain_t( int id, uint32 window_size );

  /**
   * Destructor: frees object.
   */
  ~chain_t();
  //@}

  /**
   * @name Methods
   */
  //@{
  //@}

  /**
   * @name Accessor(s) / mutator(s)
   */
  //@{
  /** allocates data structures, because dealing with initialization order
   *  can be time consuming. Call after system_t::inst has been initialized.
   */
  void allocate(void);

  /**
   *  Checks to see if this chain structure has been initialized
   */
  bool isInitialized( void ) {
    return m_is_init;
  }

  /// start performance statistics
  void startTimer( unsigned int proc );

  /// print statistics
  void printStats( void );
  
  /// print memory statistics
  void printMemoryStats( void );

  /// processes instructions as they arrive
  void inst_arrival( pt_inst_t *inst, unsigned int proc);

  /// processes memory operations as they arrive
  void mem_arrival( pt_memory_t *inst, unsigned int proc );

  /// processes CPU exceptions as they arrive
  void postException( uint32 exception, unsigned int proc );

  /// delete the flow instruction (using ref counting)
  void delete_flow_inst( flow_inst_t *inst );
  //@}

  /// The number of instructions executed on all processors
  static uint32 s_num_instructions;
  /// a consumer to distribute "on-line" inst records between chain instances
  static void chain_inst_consumer( pt_record_t *rec );
  /// a consumer to distribute "on-line" mem records between chain instances
  static void chain_mem_consumer( pt_record_t *rec );

  /// most recent instruction fetch address
  pa_t  * m_most_recent_fetch;   //used in ptrace.C

  /// PUBLIC: used in static callback function
  /// # of predecessors for each static instrucions
  histogram_t   *m_hist_static_pred;
  /// # of successors for each static instrucions
  histogram_t   *m_hist_static_succ;
  /// # of static misses per static instruction
  histogram_t   *m_hist_static_miss_count;

  /// histogram of PC:non-critical misses
  histogram_t   *m_hist_pc_non_critical;
  /// histogram of PC:critical misses
  histogram_t   *m_hist_pc_critical;

protected:
  /// increments the index in a circular buffer
  uint32 chain_increment( uint32 index, uint32 proc ) {
    if (index == m_window_limit[proc] - 1)
      return 0;
    return (index + 1);
  }

  /**
   *  Analyze the instruction window. This routine walks the instruction
   *  window from the tail of the window to the head. Along the way,
   *  it determines which instructions are dependent on the head instruction
   *  (in terms of register dependences as well as memory dependences).
   *  It does this by using the flat register files in the m_state object
   *  to track if any instruction has written a given register.
   *  Finally, the analysis walks the instruction window counting the
   *  number of independent and dependent instruction.
   */
  void analyzeWindow(unsigned int proc);

  /**
   *  Analyze the next instruction (at the head of the window) looking
   *  at its source and destination registers.
   */
  void analyzeDependence( unsigned int proc );

  /// print out the contents of the (ideal) flow instruction window
  void printWindow( unsigned int proc );
  
  /// search the memory alias table for prior stores
  int64 amap_search( flow_inst_t *flow_inst, unsigned int proc );
  /// enter a new store into the memory alias table
  void amap_store(flow_inst_t *flow_inst, int64 depth, unsigned int proc);

  /// A pointer to this processors sequencer
  pseq_t        *m_pseq;
  
  /// A pointer to this processors pstate object
  pstate_t      *m_pstate;
  
  /// identifier for this sequence
  int            m_id;

  /// current instruction map of decoded static instructions by physical PC
  /// ach logical proc has its own map
  ipagemap_t    ** m_imap;

  /// A register file data structure (used to encode dependence information)
  /// now one per logical proc
  mstate_t  *     m_state;

  /// 1/16/03 CM "fake" null_dep should get elimiated as flow inst evolves
  mem_dependence_t ** m_mem_dep;

  /** A local memory alias table: tracks the last writer to each line.
   *     A map of physical address (cache line size) -> flow instruction.
   *     Either this or the local (per processor) table is used, depending
   *     on the CHAIN_MP_MODE parameter.
   */
  //this should be SHARED in the SMT chip
  CFGIndex       m_alias_table;

  /// The 'predecessor' node that indicating a constant input
  /// each logical proc should have one
  flow_inst_t ** m_constant_pred;
  /// The last instruction fetched (waiting to see if it was a memory op)
  flow_inst_t   ** m_last_fetched;

  /// The equivalent static instruction for "no predecessor"
  static_inst_t ** m_null_static;

  /// Instruction window data structure
  //@{
  /// The rotating buffer modeling the instruction window
  flow_inst_t *** m_window;

  /// The size of the instruction window in the number of instructions
  uint32 * m_window_size;

  /// The buffer space available in the instruction window (> window_size)
  uint32 * m_window_limit;

  /// A count of the number of instructions in the window
  uint32    *     m_window_utilization;

  /// The next instruction to be removed from the window
    uint32    *     m_tail;

  /// The last instruction to be analyzed in the window
  uint64    *     m_last_analyzed;
  /// A pointer to the last instruction fetched in the window
   uint32   *      m_head;
  //@}

  /// the instruction consumer
  pf_consumer_t  * m_inst_consumer; 
  /// the data consumer
  pf_consumer_t *  m_data_consumer; 

  /// per cpu stopwatch counting simulated time
  utimer_t   *    m_overall_timer;

  /// true if this beast has been allocated!
  bool           m_is_init;
  /// a count of instructions executed between memory misses
   int64    *      m_instructions_per_miss;

  /// depth of memory instructions already reached
   int64     *     m_memory_depth;
  /// The maximum depth of memory instructions
   int64    *      m_max_memory_depth;
  /// the number of times the depth increases
   int64   *       m_got_deeper;
  /// the number of times the depth decreases
  int64    *      m_got_shallower;
  /// the memory depth of the control flow
  int64     *     m_control_flow_depth;
  /// the memory depth due to the window size
  int64    *      m_window_flow_depth;
  /// The relative memory depth of other misses in the window
  histogram_t   *m_hist_depth;
  /// The maximum memory depth of other misses in the window
  histogram_t   *m_max_hist_depth;
  
  /// histogram of instructions between window stalls
  histogram_t   *m_hist_inst_per_misses;
  
  /// histogram of dependent instrs
  histogram_t   *m_hist_dependent;
  /// histogram of dependent misses
  histogram_t   *m_hist_dependent_miss;
  /// histogram of independent instrs
  histogram_t   *m_hist_independent;
  /// histogram of independent misses
  histogram_t   *m_hist_independent_miss;

  /// histogram of # of CTIs between 2 dependent misses (in a window)
  histogram_t   *m_hist_dep_cti_dist;
  /// histogram of # of branches between 2 dependent misses (in a window)
  histogram_t   *m_hist_dep_branch_dist;
  /// histogram of # of CTIs between 2 independent misses (in a window)
  histogram_t   *m_hist_ind_cti_dist;
  /// histogram of # of branches between 2 independent misses (in a window)
  histogram_t   *m_hist_ind_branch_dist;

  /// number of loads reading from other processors (cache line granularity)
  histogram_t   *m_hist_cpus_read_from;
  /// number of stores over-writing other processors
  histogram_t   *m_hist_cpus_overwritten;
    
  // all below were separated for each logical proc:
  /// max # of branch instructions seen for this window size
  uint32    *     m_max_branch_count;

  /// number of instructions "executed" on this CPU
  uint64     *    m_inst_count;
  /// number of memory operations seen by this CPU
  uint64    *     m_mem_count;
  /// number of memory ops actually parsed by the chain tool
  uint64    *     m_consumable_mem_count;
  /// number of misses
  uint64   *      m_mem_miss_count;
  /// number of unusual misses (not caused by loads or stores)
  uint64    *     m_mem_unusual_miss_count;
  /// number of exceptions occuring on the CPU
  uint64    *     m_except_count;
  /// number of instructions "reissued" on this CPU
  uint64    *     m_reissue_count;
  /// number of non-cpu initiated loads/stores
  uint64    *     m_non_cpu_ini_count;
  /// number of non-memory space loads/stores
  uint64    *     m_not_memory_count;
  /// number of non-stallable accesses
  uint64    *     m_not_stallable_count;
  /// number of forwards from prior store values
  uint64    *     m_st_ld_forward_count;

  /// number of misses that are marked "counted"
  uint64    *     m_marked_counted;
  /// number of misses that are deletd
  uint64    *     m_deleted_misses;
  /// number of misses that are repeated
  uint64    *     m_repeated_misses;
  /// number of accesses that are repeated
  uint64     *    m_repeated_accesses;

  /// number of invalid load addresses
  uint64     *    m_invalid_load_address;
  /// number of invalid store addresses
  uint64     *    m_invalid_store_address;
};

/*------------------------------------------------------------------------*/
/* Global variables                                                       */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Global functions                                                       */
/*------------------------------------------------------------------------*/

#endif  /* _CHAIN_H_ */
