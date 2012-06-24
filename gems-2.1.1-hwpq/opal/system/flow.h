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
#ifndef _FLOW_H_
#define _FLOW_H_

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

/// a bit field of interesting events that may occur to a dynamic instruction
typedef uint16 flow_event_t;
const flow_event_t FLOW_MEMORY_MISS       = 1 << 0;
const flow_event_t FLOW_CONSTANT          = 1 << 1;
const flow_event_t FLOW_SEEN_ACCESS       = 1 << 2;
const flow_event_t FLOW_EXCEPTION         = 1 << 3;
const flow_event_t FLOW_IS_COUNTED        = 1 << 4;
const flow_event_t FLOW_UNUSED5      = 1 << 5;
const flow_event_t FLOW_UNUSED6      = 1 << 6;
const flow_event_t FLOW_UNUSED7      = 1 << 7;
const flow_event_t FLOW_UNUSED8      = 1 << 8;

/** pointer to member function: nextPC - updates PC and NPC for this 
 *  dynamic instruction */
typedef void (flow_inst_t::*pmf_flowExecute)( void );

/*------------------------------------------------------------------------*/
/* Class declaration(s)                                                   */
/*------------------------------------------------------------------------*/

/**
* Implements a node list (multiple instructions) in a dataflow graph.
*
* @see     flow_inst_t
* @author  cmauer
* @version $Id$
*/
class cfg_list_t {
public:
  /**
   * @name Constructor(s) / destructor
   */
  //@{
  /**
   * Constructor: creates object
   */
  cfg_list_t();

  /**
   * Destructor: frees object.
   */
  ~cfg_list_t();
  //@}

  /**
   * @name Methods
   */
  //@{
  /// get this node from this list
  flow_inst_t *getNode( void ) { return m_node; }
  /// get the next node in this list
  cfg_list_t *getNext( void ) { return m_next; }

  /// add a node to the list
  void add( flow_inst_t *node );

  /// returns true if the nodelist contains the cfg node, false if not
  bool contains( flow_inst_t *node );

  /// Adds a cfg node to a node list, if it isn't already on it
  void add_unique( flow_inst_t *node );
  
  /// prints out the cfg # that are in the node list
  void print( void );

  /// removes (and frees) all entries in the node list
  void removeall( void );
  //@}

  /**
   * @name Accessor(s) / mutator(s)
   */
  //@{
  //@}

  /** @name Static Variables */

private:
  /// A pointer to a node in this list
  flow_inst_t *m_node;

  /// A pointer to the next node in the list
  cfg_list_t  *m_next;
};

/**
* Implements a (relatively timing-independent) functional execution of 
* static instructions, as well as being a node (representing an 
* instruction) in a dataflow graph.
*
* @see     static_inst_t, dynamic_inst_t
* @author  cmauer
* @version $Id: flow.h,v 3.6 2004/08/03 20:06:04 lyen Exp $
*/
class flow_inst_t {
public:
  /**
   * @name Constructor(s) / destructor
   */
  //@{
  /**
   * Constructor: creates object
   * @param s       The static instruction
   * @param pseq    The sequencer that executes this instruction
   * @param at      The abstract PC of this instruction
   * @param mem_dep A list of memory dependencies
   */
  flow_inst_t( static_inst_t *s,
               pseq_t *pseq,
               mstate_t *mstate,
               mem_dependence_t *mem_dep,
               uint32 proc );

  // in order to enable SMT support we need to be able to declare an array of flow_inst_t objects, which requires
  //          an empty constructor
  flow_inst_t(){ }

  /**
   * Destructor: frees object.
   */
  ~flow_inst_t();

  /** Allocates object through allocator interface */
  void *operator new( size_t size ) {
    return ( m_myalloc.memalloc( size ) );
  }
  
  /** frees object through allocator interface */
  void operator delete( void *obj ) {
    m_myalloc.memfree( obj );
  }

  /// increment the reference counter (on allocation)
  void      incrementRefCount( void ) {
    m_ref_count++;
  }
  /// decrement the static instruction reference counter (on free)
  void      decrementRefCount( void ) {
  #ifdef DBUG
    printf("about to decrement ref count, refcount = %d\n",m_ref_count);
  #endif
    m_ref_count--;
    ASSERT(m_ref_count >= 0);
  #ifdef DBUG
    printf("***Finishing decrementRefCount...\n");
   #endif
  }
  /// get the reference count
  int32     getRefCount( void ) {
    return (m_ref_count);
  }

  /// get this instruction's memory depth
  int32     getMemDepth( void ) {
    return m_mem_depth;
  }
  
  /// set this instruction's memory depth
  void     setMemDepth( int32 mem_depth ) {
    m_mem_depth = mem_depth;
  }
  //@}

  /** @name Accessor / Mutators */
  //@{
  /// returns static instruction member
  static_inst_t *getStaticInst( void ) const { return m_static_inst; }

  /// returns a source register
  reg_id_t      &getSourceReg( int sourcenum ) {
    ASSERT( sourcenum < SI_MAX_SOURCE );
    return (m_source_reg[sourcenum]);
  }
  /// returns a destination register
  reg_id_t      &getDestReg( int destnum ) {
    ASSERT( destnum < SI_MAX_DEST );
    return (m_dest_reg[destnum]);
  }

  /// return the logical proc number
  uint32 getProc(){
    assert(m_proc < CONFIG_LOGICAL_PER_PHY_PROC);
    return m_proc;
  }

  void printInstruction(){
    //does nothing
  }

  /// Execute this instruction
  void           execute( void ) {
    pmf_flowExecute pmf = m_jump_table[getStaticInst()->getOpcode()];
    (this->*pmf)();
  }
  
  /** returns a pointer to the actual target information.
   *  NOTE: The actual target is initialized with the abstract PC state
   *        prior to this instructions execution. If you don't
   *        modify a field, it will be the same as the previous instructions!
   */
  abstract_pc_t *getActualTarget( void ) {
    return (&m_actual_at);
  }

  /** Advance the actual (target) PC to the next PC */
  void           advancePC( void );
  
  /// set if this instruction is a taken branch or not.
  void           setTaken( bool taken ) {
    m_is_taken = taken;
  }
  /// returns if this instruction is taken or not
  bool           getTaken( void ) {
    return m_is_taken;
  }

  /// returns the virtual address of this instruction
  la_t           getVPC( void ) const {
    return m_virtual_address;
  }

  /// sets the trap type for this instruction
  void           setTrapType( trap_type_t tt ) {
    if (m_traptype > tt) {
#ifdef LSQ_DEBUG
      DEBUG_OUT("index:%d generated trap:%d\n", m_windex, (uint16) tt);
#endif
      m_traptype = (uint16) tt;
    }
  }
  /// gets the trap type for this instruction
  trap_type_t getTrapType( void ) {
    return ((trap_type_t) m_traptype);
  }

  /// sets flags: mark this instruction as performing some event
  void   markEvent(flow_event_t e) { m_events |= e; }
  /// sets flags: mark this instruction as NOT performing some event
  void   unmarkEvent(flow_event_t e) { m_events &= ~e; }
  /// gets flags
  bool   getEvent( flow_event_t e) { return ( (m_events & e) != 0 ); }

  /**
   *  @name Memory related interfaces
   */
  //@{
  /// performs the actual memory read
  void        readMemory( void );

  /// returns true if the data is valid, false if not.
  bool        isDataValid( void ) const {
    return m_data_valid;
  }
  /// returns the access size of the load or store
  byte_t      getAccessSize() const {
    return m_access_size;
  }

  /// return a pointer to the data value loaded
  ireg_t     *getData( void ) {
    return ( &m_data_storage[0] );
  }
  /// return the data (masked by the size of the load or store)
  ireg_t      getUnsignedData( void ) {
    if (m_access_size == 4) {
      return (m_data_storage[0] & MEM_WORD_MASK);
    } else if (m_access_size == 8) {
      return m_data_storage[0];
    } else if (m_access_size == 2) {
      return (m_data_storage[0] & MEM_HALF_MASK);
    } else if (m_access_size == 1) {
      return (m_data_storage[0] & MEM_BYTE_MASK);
    } else {
      ERROR_OUT("error: unable to mask access size: %d\n", m_access_size);
      return 0ULL;
    }      
  }
  
  /// return the sign extended data (masked by the size of the load or store)
  ireg_t      getSignedData( void ) {
    // This magic statement "m_access_size*8-1" selects the bit to do the
    // sign extension on (e.g. 63, 15, or 7).
    if (m_access_size == 4) {
      return sign_ext64(m_data_storage[0] & MEM_WORD_MASK, m_access_size*8-1);
    } else if (m_access_size == 8) {
      return m_data_storage[0];
    } else if (m_access_size == 2) {
      return sign_ext64(m_data_storage[0] & MEM_HALF_MASK, m_access_size*8-1);
    } else if (m_access_size == 1) {
      return sign_ext64(m_data_storage[0] & MEM_BYTE_MASK, m_access_size*8-1);
    } else {
      DEBUG_OUT("error: unable to mask access size: %d\n", m_access_size);
      return 0ULL;
    }      
  }

  /** Writes a value to memory: used by swap, casa, casxa instructions */
  void        writeValue( ireg_t value ) {
    m_data_storage[0] = value;
    m_data_valid = true;
  }
  //@}

  /** add a predecessor to the control flow graph */
  void    addPredecessor( flow_inst_t *predecessor ) {
    m_pred.add_unique( predecessor );
  }

  /** add a successor to the control flow graph */
  void    addSuccessor( flow_inst_t *successor ) {
    m_succ.add_unique( successor );
  }
  
  /// returns the current successors to this node
  cfg_list_t  *getSuccessor( void ) {
    return (&m_succ);
  }

  /// returns the current successors to this node
  cfg_list_t  *getPredecessor( void ) {
    return (&m_pred);
  }

  /// returns a unique sequence number for this flow instruction
  uint64  getID( void ) { return m_sequence_id; }
  
  /// sets the unique sequence number (used in Chain.C to override pseq #)
  void    setID( uint64 id ) {
    m_sequence_id = id;
  }

  /// returns the CPU # of the processor that issued this instruction
  int32   getSequencerID( void ) {
    if ( m_pseq == NULL ) {
      return -1;
    }
    return m_pseq->getID();
  }
  
  /// prints out the contents of this node
  void    print( void );

  /// prints out the control flow graph starting at this node.
  void    printGraph( uint32 depth );

  /// print out this nodes inputs and output
  void    debugInputs( pstate_t *state );

  /** Sets the physical address of this instruction.
      NOTE: this should not in general be used
      flow insts usually do their own address generation .. but in
      trace mode, we get the exact address from simics.
  */
  void    setPhysicalAddress( pa_t addr ) {
    m_physical_addr = addr;
  }

  /// returns the physical address of this instruction
  pa_t    getPhysicalAddress(void) const {
    return m_physical_addr;
  }

  /// build a jump table for the flow instructions
  static  void     ix_build_jump_table( void );

  /** @name memory allocation:: */
  //@{
  static listalloc_t m_myalloc;
  //@}

private:
  /// A pointer to the sequencer object
  pseq_t          *m_pseq;

  /// The logical proc number this instr is assigned to 
  uint32         m_proc;

  /// This flow instruction's static counterpart
  static_inst_t   *m_static_inst;

  /// This flow objects (unique) sequence number (in order execution)
  uint64           m_sequence_id;
  
  /// The depth of this instruction, in terms of memory misses
  int32            m_mem_depth;
  
  /// A list of predecessor nodes, if any
  cfg_list_t       m_pred;

  /// A list of successor nodes, if any
  cfg_list_t       m_succ;
  
  /** in flight source registers */
  reg_id_t         m_source_reg[SI_MAX_SOURCE];

  /** in flight destination registers */
  reg_id_t         m_dest_reg[SI_MAX_DEST];

  /** The actual direction of this control instruction */
  bool             m_is_taken;

  /** The PC of this flow instruction */
  la_t             m_virtual_address;

  /** The program counter after this instruction executes */
  abstract_pc_t    m_actual_at;

  /** The trap type, if any, generated by executing this instruction
   *    - Trap type of Trap_NoTrap indicated no trap.
   *    - On retirement, the trap handler will be selected if this is set.
   */
  uint16           m_traptype;

  /** events that occur to this instruction. */
  flow_event_t     m_events;

  /** The virtual address of the load/store instruction */
  la_t             m_address;
  /** The physical location in memory to access. */
  pa_t             m_physical_addr;
  /** the address space identifier for this instruction */
  uint16           m_asi;
  /** true if this instructions 'm_data_storage' field is valid */
  bool             m_data_valid;
  /// size of the access (see note in memop.h)
  byte_t           m_access_size;
  /** data location for memory accesses */
  ireg_t           m_data_storage[MEMOP_MAX_SIZE];

  /** A reference counting system for flow instructions */
  int16            m_ref_count;

 //*****************************************************
  //   Transactional memory - NOT USED
  /** Flag indicating whether instruction part of transaction */
    bool           m_transactional_mode;
    
    /** transaction ID of this instruction, if instruction is in transaction */
    uint64         m_transaction_id;
    //*****************************************************
   
  /** @name Execution functions:
   **  execute this instruction on this in-order flow context.
   */
  //@{
  static  pmf_flowExecute  m_jump_table[i_maxcount];
#include "ix.h"
  //@}
};

/*------------------------------------------------------------------------*/
/* Global variables                                                       */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Global functions                                                       */
/*------------------------------------------------------------------------*/

#endif  /* _FLOW_H_ */
