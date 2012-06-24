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
#ifndef _PTRACE_H_
#define _PTRACE_H_

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

typedef uint8 pt_memory_flags_t;
const pt_memory_flags_t PTMEM_IS_IO          = 1 << 0;
const pt_memory_flags_t PTMEM_NON_CPU_INI    = 1 << 1;
const pt_memory_flags_t PTMEM_NOT_STALLABLE  = 1 << 2;
const pt_memory_flags_t PTMEM_UNUSED3        = 1 << 3;
const pt_memory_flags_t PTMEM_UNUSED4        = 1 << 4;
const pt_memory_flags_t PTMEM_UNUSED5        = 1 << 5;
const pt_memory_flags_t PTMEM_UNUSED6        = 1 << 6;
const pt_memory_flags_t PTMEM_UNUSED7        = 1 << 7;

/*------------------------------------------------------------------------*/
/* Class declaration(s)                                                   */
/*------------------------------------------------------------------------*/

/**
 * The generic superclass for records traced in the file.
 */
class pt_record_t {
public:
  uint8    m_record_type;
};

/**
 * A record of an instruction's execution
 */
class pt_inst_t : public pt_record_t {
public:
  /// The processor requesting executing this instruction
  uint8     m_pid;

  /// The 32-bit instruction
  uint32    m_inst;
  /// The 64-bit address of this instruction
  uint64    m_physical_addr;
};

/**
 * A record of a memory operation
 */
class pt_memory_t : public pt_record_t {
public:
  /** @name Flags */
  //@{
  /// set a flag
  void      setFlag( const pt_memory_flags_t flag, bool value ) {
    if (value) {
      m_flags |= flag;
    } else {
      m_flags &= ~flag;
    }
  }
  /** returns true or false (one bit flags) **/
  bool      getFlag( const pt_memory_flags_t flag ) const { 
    return ((m_flags & flag) != 0); }
  
  /// prints out this transaction
  void      print( void );
  //@}

  /// The processor requesting this data
  uint8     m_pid;
  /// The size of the request
  uint8     m_size;
  /// The type of request (load/store/atomic/prefetch/flush)
  uint8     m_type;
  /// Flags associated with this data structure
  pt_memory_flags_t  m_flags;
  /// The stall time of this transaction
  uint16    m_stall_time;

  /// The physical address of this memory transaction
  uint64    m_physical_addr;
};

/**
 * A record of a memory operation that is waiting to be completed
 */

class pt_memory_waiter_t : public waiter_t, public pt_memory_t {
public:
  /// Write this transaction out to disk
  void writeTransaction( void );
  /// wake up this instruction after the memory's execution is complete
  void Wakeup( void );

  /// The start cycle for executing this memory transaction.
  uint64    m_start_cycle;
  /// True, if this is a disk trace. False, it is a on-line analysis
  bool      m_is_disk_trace;
};

/// Pointer to a function that is a ptrace consumer
typedef void (*pf_consumer_t)( pt_record_t * );

/**
* A processor tracing object.
*
* @see     system_t
* @author  cmauer
* @version $Id: ptrace.h 1.4 04/08/06 13:40:17-05:00 lyen@opus.cs.wisc.edu $
*/
class ptrace_t {
  /// Friend pt_memory_waiter only updates the m_last_access_satisfied field.
  friend class pt_memory_waiter_t;

public:
  /**
   * @name Constructor(s) / destructor
   */
  //@{
  /// Constructor: creates object
  ptrace_t( int32 numProcs );
  
  /// Destructor: frees object.
  ~ptrace_t();
  //@}

  /**
   * @name Methods
   */
  //@{
  /**
   * mlpMemopDispatch: MLP tracing "timing_operate" function. Takes a
   * transaction, and decides how long it should stall.
   *
   * @param id      The CPU the transaction operates on
   * @param mem_op  The memory transaction
   * @return int64  A stall time
   */
  static int64 mlpMemopDispatch( int id, memory_transaction_t *mem_op );

  /** mlpExceptionDispatch: MLP tracing "exception occured" function.
   *  It is called by a processor with an exception #.
   */
  static void  mlpExceptionDispatch( int id, uint32 exception );

  void registerInstructionConsumer( pf_consumer_t myconsumer );
  void registerMemoryConsumer( pf_consumer_t myconsumer );

  void readTrace( const char *filename );
  void readInstruction( pt_inst_t *irecord );
  void readMemoryOp( pt_memory_t *mrecord );

  /// opens a trace for writing, and installs interfaces
  void writeTrace( const char *filename );
  bool writeInstruction( uint8 pid, uint32 instruction );
  bool writeMemoryOp( uint8 pid, uint8 req_type, uint64 physical_addr,
                      uint8 req_size, bool priv, uint16 stall_time );
  void closeTrace( void );

  /// calls the consumer interfaces with out writing a trace file.
  void onlineAnalysis( pf_consumer_t my_inst_consumer,
                       pf_consumer_t my_memory_consumer );

  /// called when memory trans completes
  void onlineMemoryOp( pt_memory_t *mrecord );

  /// recycles memory waiters after the waiters wake up
  void  pushMemoryWaiter( pt_memory_waiter_t *waiter );
  //@}

protected:
  /// An enumeration of the record types in the trace
  enum pt_token_t { PT_INST_RECORD, PT_MEMORY_RECORD };

  /// An enumeration of possible trace modes
  enum ptrace_mode_t {
    PTRACE_UNINITIALIZED, PTRACE_WRITING, PTRACE_READING, PTRACE_ONLINE
  };

  /// The magic number for identifying trace files (version 1.0)
  static const uint32 PTRACE_MAGIC_NUM = 0x83245656;

  /** @name File reading and writing primitives */
  //@{
  /// writes an 8-bit unsigned integer
  bool write8( uint8 data );
  /// writes an 16-bit unsigned integer
  bool write16( uint16 data );
  /// writes an 32-bit unsigned integer
  bool write32( uint32 data );
  /// writes an 64-bit unsigned integer
  bool write64( uint64 data );

  /// read 8-bit unsigned integer
  uint8  read8( void );
  /// read 16-bit unsigned integer
  uint16 read16( void );
  /// read 32-bit unsigned integer
  uint32 read32( void );
  /// read 64-bit unsigned integer
  uint64 read64( void );
  //@}

  /// returns the mode of operation
  ptrace_mode_t getMode( void ) {
    return m_mode;
  }
  /// returns a memory waiter
  pt_memory_waiter_t  *getMemoryWaiter( void );
  /// consume the formatted file
  bool   consumeTrace( void );

  /// The current instruction consumer for the trace
  pf_consumer_t  m_inst_consumer;

  /// The current memory consumer for the trace
  pf_consumer_t  m_mem_consumer;
  
  /// The mode of this trace
  ptrace_mode_t  m_mode;

  /// The physical address of the last memory access that was satisfied
  pa_t          *m_last_access_satisfied;

  /// A free-list of waiters
  list<pt_memory_waiter_t *> m_waiter_freelist;

  /// The current trace file (being read or written)
  FILE          *m_fp;
  
  /// The file name that is being read or written
  char          *m_filename;

  /// The number of instructions written
  uint64         m_inst_written;

  /// The number of mem ops written
  uint64         m_mem_written;

  /// The number of atomic instructions seen
  static uint64         m_stat_mem_atomics;
  /// The number of ldda's seen
  static uint64         m_stat_mem_ldda;
  /// The number of "ignore" (e.g. a MMU transaction, not a memory one)
  static uint64         m_stat_mem_ignores;
  /// The number of transactions not initiated by a cpu
  static uint64         m_stat_non_cpu_init;
  /// The number of non-stallable transactions seen
  static uint64         m_stat_non_stallable;
  /// The number of repeat accesses seen (and discarded)
  static uint64         m_stat_repeated_access;
};

/*------------------------------------------------------------------------*/
/* Global variables                                                       */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Global functions                                                       */
/*------------------------------------------------------------------------*/

/**
 * Memory hierarchy timing interface.
 */
extern "C" cycles_t ptrace_memory_operation( conf_object_t *obj,
                                             conf_object_t *space,
                                             map_list_t *map,
                                             memory_transaction_t *mem_op);

#endif  /* _PTRACE_H_ */
