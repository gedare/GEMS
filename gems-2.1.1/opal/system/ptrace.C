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
 * FileName:  ptrace.C
 * Synopsis:  Implements a tracing object
 * Author:    cmauer
 * Version:   $Id$
 */

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include "hfa.h"
#include "hfacore.h"
#include "fileio.h"
#include "pipestate.h"
#include "rubycache.h"
#include "ptrace.h"
#include "chain.h"

// The Simics API Wrappers (for 3.0)
#include "interface.h"

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

/// The (number of cycles) to stall simics, when doing mlp tracing on a miss
const uint64  LARGE_STALL_TIME        = 10000;

/// The number of cycles it takes to do an I/O access. I'm going with
/// about the same amount of time it takes to access memory
const uint64  PTRACE_IO_STALL_TIME    = 140;

/*------------------------------------------------------------------------*/
/* Variable declarations                                                  */
/*------------------------------------------------------------------------*/

/// The number of atomic instructions seen
uint64         ptrace_t::m_stat_mem_atomics;
/// The number of ldda's seen
uint64         ptrace_t::m_stat_mem_ldda;
/// The number of "ignore" instructions seen
uint64         ptrace_t::m_stat_mem_ignores;
/// The number of transactions not initiated by a cpu
uint64         ptrace_t::m_stat_non_cpu_init;
/// The number of non-stallable transactions seen
uint64         ptrace_t::m_stat_non_stallable;
/// The number of repeat accesses seen (and discarded)
uint64         ptrace_t::m_stat_repeated_access;

// C++ Template: explicit instantiation
//template class list<pt_memory_waiter_t *>;

/*------------------------------------------------------------------------*/
/* Forward declarations                                                   */
/*------------------------------------------------------------------------*/

//**************************************************************************
void   pt_memory_t::print( void )
{
  DEBUG_OUT( "pt_memory:\n" );
  DEBUG_OUT( "  id    : %d\n", m_pid );
  DEBUG_OUT( "  size  : %d\n", m_size );
  DEBUG_OUT( "  type  : %d\n", m_type );
  DEBUG_OUT( "  flags : %d\n", m_flags );
  DEBUG_OUT( "  addr  : 0x%0llx\n", m_physical_addr );
  DEBUG_OUT( "  stall : %d\n", m_stall_time );
}

//**************************************************************************
int64 ptrace_t::mlpMemopDispatch( int id, memory_transaction_t *mem_op )
{
  int32 seq_num = id / CONFIG_LOGICAL_PER_PHY_PROC;
  uint32 logical_proc_num = id % CONFIG_LOGICAL_PER_PHY_PROC;
  ASSERT(seq_num >= 0 && seq_num < system_t::inst->m_numSMTProcs);
  ASSERT(logical_proc_num < CONFIG_LOGICAL_PER_PHY_PROC);

  generic_transaction_t *generic_op = (generic_transaction_t *) mem_op;
  ptrace_t     *trace = system_t::inst->m_trace;
  ptrace_mode_t mode = trace->getMode();
  ruby_status_t status;
  bool mshr_hit = false;
  bool mshr_stall = false;
  bool conflicting_access = false;
  OpalMemop_t req;

  if (SIM_mem_op_is_instruction( &mem_op->s )) {
    req = OPAL_IFETCH;

    generic_op->block_STC = 1;

    // read the instruction (if possible)
    uint64    inst    = 0;
    pstate_t *state   = system_t::inst->m_state[seq_num];          
    bool      success = state->readPhysicalMemory( mem_op->s.physical_address,
                                                   4, &inst, logical_proc_num );                

    if ( (!IS_CPU_MEM_OP( generic_op->ini_type )) ||
         (memory_inst_t::isIOAccess(mem_op->s.physical_address)) ) {
      ERROR_OUT("ptrace: mlpMemopDispatch: instruction fetch initiated by non-CPU or in I/O space. Discarding\n");
      return 0;
    }
    
    system_t::inst->m_chain[seq_num]->m_most_recent_fetch[logical_proc_num] = mem_op->s.physical_address; 
    if (success) {
      // record instruction fetch
      if (mode == PTRACE_WRITING) {
        success = trace->writeInstruction( id, inst );
      } else if (mode == PTRACE_ONLINE) {
        // create an instruction record & call the callback
        pt_inst_t   inst_record;
        inst_record.m_record_type = PT_INST_RECORD;
        inst_record.m_pid  = id;
        inst_record.m_inst = inst;
        inst_record.m_physical_addr = mem_op->s.physical_address;
        (*(trace->m_inst_consumer))( &inst_record );
      } else {
        ERROR_OUT("error: ptrace: mlpMemoryDispatch: unknown status!\n");
      }
    } else {
      // CM maybe don't need to report error here... don't know yet.
      ERROR_OUT("error: ptrace: unable to read instruction. (itlb miss)\n");
      return 0;
    }
    return 0;
  } else if (SIM_mem_op_is_read( &mem_op->s )) {
    req = OPAL_LOAD;
  } else if (SIM_mem_op_is_write( &mem_op->s )) {
    req = OPAL_STORE;
  } else {
    ERROR_OUT("pseq: mlpTraceMemory: unknown request type: %d\n",
              mem_op->s.type);
    return 0;
  }

  // filter out "replayed" memory accesses
  // CM FIX: if simics ever gets it together, this shouldn't happen,
  // and we can be more accurate by removing it. Currently, it doesn't
  // differentiate between one atomic access by a processor that gets
  // replayed, and multiple (store) accesses by a processor.
  if ( trace->m_last_access_satisfied[id] != (pa_t) -1) {
    if ( trace->m_last_access_satisfied[id] == mem_op->s.physical_address ) {
      trace->m_last_access_satisfied[id] = (pa_t) -1;
      m_stat_repeated_access++;
      return 0;
    }
  }
  
  // modify the request type if it is an atomic request
  if (generic_op->atomic) {
    req = OPAL_ATOMIC;
    uint64    inst    = 0;
    bool      success = true;
    pstate_t *state   = system_t::inst->m_state[seq_num];              
    la_t      pc      = state->getControl( CONTROL_PC, logical_proc_num );
    pa_t      phys_pc;
    success = state->translateInstruction( pc, 4, phys_pc, logical_proc_num ); 
    if (success) {
      success = state->readPhysicalMemory( phys_pc, 4, &inst, logical_proc_num );  
    }
    if (success) {
      // determine if this is a "ldda" instruction (non-exclusive atomic)
      // ldda bit mask:  1100 0001 1111 1000 == 0xc1f80000
      // ldda match   :  1100 0000 1001 1000 == 0xc0980000
      if ( (inst & 0xc1f80000) == 0xc0980000 ) {
        m_stat_mem_ldda++;
        req = OPAL_LOAD;
      } else {
        req = OPAL_ATOMIC;
      }
    }
  }
  
  if (req == OPAL_ATOMIC) {
    m_stat_mem_atomics++;
  }

  // this transaction is to the MMU (e.g. not a memory-type transaction)
  if ( generic_op->ignore == 1 ) {
    m_stat_mem_ignores++;
    return 0;
  }
  
  // allocate waiter that will record this memory instructions completion time
  rubycache_t        *ruby_cache = system_t::inst->m_seq[seq_num]->getRubyCache();
  pt_memory_waiter_t *pt_waiter  = trace->getMemoryWaiter();

  pt_waiter->m_is_disk_trace = (mode == PTRACE_WRITING);

  pt_waiter->m_pid = id;
  pt_waiter->m_size = mem_op->s.size;
  pt_waiter->m_type = req;
  pt_waiter->m_flags = 0;
  pt_waiter->m_physical_addr = mem_op->s.physical_address;
  // get the current ruby cycle
  pt_waiter->m_start_cycle = system_t::inst->m_ruby_api->getTime();

  // if the initiator is not a CPU, flag this transaction
  if ( !IS_CPU_MEM_OP( generic_op->ini_type ) ) {
    pt_waiter->setFlag( PTMEM_NON_CPU_INI, true );
    pt_waiter->writeTransaction();
    m_stat_non_cpu_init++;
    return 0;
  }

  // if the initiator may not stall, also flag this transaction
  if ( generic_op->may_stall == 0 ) {
    pt_waiter->setFlag( PTMEM_NOT_STALLABLE, true );
    pt_waiter->writeTransaction();
    m_stat_non_stallable++;
    return 0;
  }

  // if the access is to an I/O device ...
  if (memory_inst_t::isIOAccess(mem_op->s.physical_address)) {
    // I/O access:
    //   * can't be issued to ruby
    //   * can't always stall (e.g. must return 0 at some point)
    // solution: treat I/O accesses like misses, and return 0 stall time
    pt_waiter->setFlag( PTMEM_IS_IO, true );
    pt_waiter->m_start_cycle = system_t::inst->m_ruby_api->getTime() - PTRACE_IO_STALL_TIME;
    pt_waiter->writeTransaction();
    
    // return a reasonable stall time (but don't issue to ruby)
    return 0;
  }
  
  // submit this waiter to the cache
#if 0
  DEBUG_OUT( "PT waiter submit: (%lld) 0x%0llx\n", 
             pt_waiter->m_start_cycle,
             mem_op->s.physical_address );
#endif
  status = ruby_cache->access( mem_op->s.logical_address,
                               mem_op->s.physical_address,
                               req,
                               mem_op->s.logical_address,
                               false,
                               pt_waiter,
                               mshr_hit,
                               mshr_stall,
                               conflicting_access,
                               0      //dummy thread id
                               );
  
  if (status == HIT) {
    // complete immediately
    pt_waiter->writeTransaction();
    return 0;
    
  } else if (status == MISS) {
    // wait for the hit callback
    return LARGE_STALL_TIME;

  } else if (status == NOT_READY) {
    // if the resources don't exist to make this request, retry it later
    ERROR_OUT("error: ruby_cache not ready for request.\n");
    SIM_HALT;
  } else {
    ERROR_OUT("error: unrecognized status value\n");
    SIM_HALT;
  }

  return 0;
}
//**************************************************************************
void ptrace_t::mlpExceptionDispatch( int id, uint32 exception )
{
  ptrace_t     *trace = system_t::inst->m_trace;
  if (trace->getMode() == PTRACE_ONLINE) {
      int32 seq_num = id / CONFIG_LOGICAL_PER_PHY_PROC;
      uint32 logical_proc_num = id % CONFIG_LOGICAL_PER_PHY_PROC;
      ASSERT(seq_num >= 0 && seq_num < system_t::inst->m_numSMTProcs);
      ASSERT(logical_proc_num < CONFIG_LOGICAL_PER_PHY_PROC);
      
     chain_t *chain = system_t::inst->m_chain[seq_num];
    
    // post an exception to the instruction on the current processor
    chain->postException( exception, logical_proc_num );
  } else {
    // write that the exception occurs offline
    // CM FIX: incomplete handling of exceptions for trace mode
  }
}

//**************************************************************************
void pt_memory_waiter_t::writeTransaction( void )
{
  // get the length of this waiter's execution
  m_stall_time = system_t::inst->m_ruby_api->getTime() - m_start_cycle;
  //DEBUG_OUT( "PT waiter completing: 0x%0llx (%d)\n", m_physical_addr, m_stall_time );
  
  // record this waiters values to the trace
  ptrace_t *trace = system_t::inst->m_trace;
  if (m_is_disk_trace) {
    bool success = trace->writeMemoryOp( m_pid, m_type, m_physical_addr,
                                         m_size, false, m_stall_time );
    if (!success) {
      ERROR_OUT("error: pt_memory_waiter: disk full.\n");
    }
  } else {
    trace->onlineMemoryOp( this );
  }

  trace->m_last_access_satisfied[m_pid] = m_physical_addr;

  // push this instruction to the 'freelist'
  trace->pushMemoryWaiter( this );
}

//**************************************************************************
void pt_memory_waiter_t::Wakeup( void )
{
#if 0
  if (0x7fa0aa18 == m_physical_addr) {
    DEBUG_OUT(" system_t:: wakeup %d 0x%0llx %lld\n",
              m_pid,
              m_physical_addr,
              system_t::inst->m_ruby_api->getTime() );
  }
#endif
  writeTransaction();
  
  // unstall this processor
  int32 seq_num = m_pid / CONFIG_LOGICAL_PER_PHY_PROC;
  uint32 logical_proc_num = m_pid % CONFIG_LOGICAL_PER_PHY_PROC;
  ASSERT(seq_num >= 0 && seq_num < system_t::inst->m_numSMTProcs);
  ASSERT(logical_proc_num < CONFIG_LOGICAL_PER_PHY_PROC);
  
  system_t::inst->m_state[seq_num]->unstall(logical_proc_num);           
}

/*------------------------------------------------------------------------*/
/* Constructor(s) / destructor                                            */
/*------------------------------------------------------------------------*/

//**************************************************************************
ptrace_t::ptrace_t( int32 numProcs )
{
  m_mode = PTRACE_UNINITIALIZED;
  m_fp = NULL;
  m_filename = NULL;

  m_inst_consumer = NULL;
  m_mem_consumer = NULL;

  m_last_access_satisfied = (pa_t *) malloc( sizeof(pa_t) * numProcs );
  for (int32 i = 0; i < numProcs; i++) {
    m_last_access_satisfied[i] = (pa_t) -1;
  }
  m_inst_written = 0;
  m_mem_written = 0;

  m_stat_mem_atomics = 0;
  m_stat_mem_ldda = 0;
  m_stat_mem_ignores = 0;
  
  m_stat_non_cpu_init = 0;
  m_stat_non_stallable = 0;
  m_stat_repeated_access = 0;
}

//**************************************************************************
ptrace_t::~ptrace_t( )
{
}

/*------------------------------------------------------------------------*/
/* Public methods                                                         */
/*------------------------------------------------------------------------*/

//**************************************************************************
void ptrace_t::registerInstructionConsumer( pf_consumer_t myconsumer )
{
  ASSERT( m_inst_consumer == NULL );
  m_inst_consumer = myconsumer;
}

//**************************************************************************
void ptrace_t::registerMemoryConsumer( pf_consumer_t myconsumer )
{
  ASSERT( m_mem_consumer == NULL );
  m_mem_consumer = myconsumer;
}

//**************************************************************************
void ptrace_t::readTrace( const char *filename )
{
  char  cmd[FILEIO_MAX_FILENAME];  

  ASSERT( m_mode == PTRACE_UNINITIALIZED );
  if (filename == NULL) {
    ERROR_OUT("error: ptrace: readTrace: NULL filename.\n");
    return;
  }
  m_filename = (char *) malloc( sizeof(char)*strlen(filename) + 1 );
  strcpy( m_filename, filename );
  
  // open the file in read mode
  m_fp = fopen( filename, "r" );
  if (m_fp == NULL) {
    ERROR_OUT("error: ptrace: unable to open file: %s\n", filename);
    SYSTEM_EXIT;
  } else {
    fclose( m_fp );

    sprintf( cmd, "/s/std/bin/zcat %s", filename );
    m_fp = popen( cmd, "r" );
    m_mode = PTRACE_READING;

    uint32 magic_num = read32();
    if ( magic_num != PTRACE_MAGIC_NUM ) {
      ERROR_OUT("error: log file format unknown: %s 0x%0x\n", 
                filename, magic_num);
    }
    
    // call the trace consumer on each record in the file
    consumeTrace();
  }
}

//**************************************************************************
void ptrace_t::writeTrace( const char *filename )
{
  char  cmd[FILEIO_MAX_FILENAME];

  ASSERT( m_mode == PTRACE_UNINITIALIZED );
  m_filename = (char *) malloc( sizeof(char)*(strlen(filename)+1) );
  strcpy( m_filename, filename );
  
  sprintf( cmd, "/s/std/bin/gzip > %s", m_filename );
  m_fp = popen( cmd, "w" );
  if (m_fp == NULL) {
    ERROR_OUT("error: ptrace: unable to open file %s\n", m_filename);

  } else {
    DEBUG_OUT("ptrace: opening file %s\n", m_filename);

    // write log header
    uint32 magic_num = PTRACE_MAGIC_NUM;
    write32( magic_num );
    m_mode = PTRACE_WRITING;
  }
}

//**************************************************************************
void ptrace_t::closeTrace( void )
{
  DEBUG_OUT("   inst written: %lld\n", m_inst_written );
  DEBUG_OUT("   mem  written: %lld\n", m_mem_written );
  DEBUG_OUT("   atomics seen: %lld\n", m_stat_mem_atomics );
  DEBUG_OUT("   ldda's  seen: %lld\n", m_stat_mem_ldda );
  DEBUG_OUT("   ignores seen: %lld\n", m_stat_mem_ignores );
  DEBUG_OUT("   non cpu init: %lld\n", m_stat_non_cpu_init );
  DEBUG_OUT("   non stallable: %lld\n", m_stat_non_stallable );
  DEBUG_OUT("   repeated access: %lld\n", m_stat_repeated_access );
  
  if (m_fp) {
    int rc = pclose(m_fp);
    if (rc < 0) {
      ERROR_OUT("error closing file: %d\n", rc);
    }
    m_fp = NULL;
  }
}

//**************************************************************************
void ptrace_t::onlineAnalysis( pf_consumer_t my_inst_consumer,
                               pf_consumer_t my_memory_consumer )
{
  m_mode = PTRACE_ONLINE;
  m_inst_consumer = my_inst_consumer;
  m_mem_consumer  = my_memory_consumer;
}

//**************************************************************************
void ptrace_t::onlineMemoryOp( pt_memory_t *mrecord )
{
  (*m_mem_consumer)( mrecord );
}

//**************************************************************************
bool ptrace_t::writeInstruction( uint8 pid, uint32 instruction )
{
  bool success;
  success = write8( PT_INST_RECORD );
  if (!success)
    return (success);
  success = write8( pid );
  if (!success)
    return (success);
  success = write32( instruction );
  m_inst_written++;
  return (success);
}

//**************************************************************************
void ptrace_t::readInstruction( pt_inst_t *irecord )
{
  irecord->m_pid  = read8();
  irecord->m_inst = read32();
}

//**************************************************************************
bool ptrace_t::writeMemoryOp( uint8 pid, uint8 req_type, uint64 physical_addr,
                              uint8 req_size, bool priv, uint16 stall_time )
{
  bool success;
  success = write8( PT_MEMORY_RECORD );
  if (!success)
    return (success);
  success = write8( pid );
  if (!success)
    return (success);
  success = write8( req_size );
  if (!success)
    return (success);
  success = write8( req_type );
  if (!success)
    return (success);
  success = write64( physical_addr );
  if (!success)
    return (success);
  success = write16( stall_time );
  m_mem_written++;
  return (success);
}

//**************************************************************************
void ptrace_t::readMemoryOp( pt_memory_t *mrecord )
{
  mrecord->m_pid   = read8();
  mrecord->m_size  = read8();
  mrecord->m_type  = read8();
  mrecord->m_physical_addr = read64();
  mrecord->m_stall_time    = read16();
}

/*------------------------------------------------------------------------*/
/* Accessor(s) / mutator(s)                                               */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Protected methods                                                      */
/*------------------------------------------------------------------------*/

//**************************************************************************
pt_memory_waiter_t *ptrace_t::getMemoryWaiter( void )
{
  pt_memory_waiter_t *waiter;

  if (m_waiter_freelist.size() == 0) {
    waiter = new pt_memory_waiter_t();    
  } else {
    // get the front of the list, then remove it from the list
    waiter = m_waiter_freelist.front();
    m_waiter_freelist.pop_front();
  }

  // initialize the structure:
  //    OK. its not good form, but the invariants for initialization are
  //    in place when the object is pushed.
  //    waiter_t   : The object is disconnected (prev == next == NULL)
  //    pt_memory_t: (no initialization necessary)

  return waiter;
}

//**************************************************************************
void ptrace_t::pushMemoryWaiter( pt_memory_waiter_t *waiter )
{
  if (waiter->Disconnected() != true ) {
    ERROR_OUT("error: ptrace: pushMemoryWaiter: waiter is not disconnected.\n");
  }
  
  /// store a pointer to this waiter
  m_waiter_freelist.push_front( waiter );
}

//**************************************************************************
bool ptrace_t::consumeTrace( void )
{
  if (m_fp == NULL) {
    ERROR_OUT("error: ptrace: consumeTrace: file is not open.\n");
    return (false);
  }
  
  // read the next token from the file
  uint8       record_type;
  int         num_read;
  pt_inst_t   inst_record;
  pt_memory_t memory_record;

  for ( ;; ) {
    // read the control token from the file
    num_read = myfread( &record_type, sizeof(uint8), 1, m_fp );
    if (num_read != 1) {
      if (feof(m_fp))
        return (true);
      ERROR_OUT("ptrace: consumeTrace: error reading token (%d)\n", num_read);
      return (false);
    }
    
    // based on the control token, read the record, call the consumer
    if (record_type == PT_INST_RECORD) {
      readInstruction( &inst_record );
      (*m_inst_consumer)( &inst_record );

    } else if (record_type == PT_MEMORY_RECORD) {
      readMemoryOp( &memory_record );
      (*m_mem_consumer)( &memory_record );

    } else {
      ERROR_OUT("ptrace: consumeTrace: unknown record type %d\n",
                record_type );
      return (false);
    }
  }
  return (true);
}

//**************************************************************************
bool ptrace_t::write8( uint8 data )
{
  int numwritten = myfwrite( &data, sizeof(uint8), 1, m_fp );
  if (numwritten < 1) {
    ERROR_OUT("error: ptrace_t: write8 fails\n");
    return (false);
  }
  return (true);
}

//**************************************************************************
bool ptrace_t::write16( uint16 data )
{
  int numwritten = myfwrite( &data, sizeof(uint16), 1, m_fp );
  if (numwritten < 1) {
    ERROR_OUT("error: ptrace_t: write16 fails\n");
    return (false);
  }
  return (true);
}

//**************************************************************************
bool ptrace_t::write32( uint32 data )
{
  int numwritten = myfwrite( &data, sizeof(uint32), 1, m_fp );
  if (numwritten < 1) {
    ERROR_OUT("error: ptrace_t: write32 fails\n");
    return (false);
  }
  return (true);
}

//**************************************************************************
bool ptrace_t::write64( uint64 data )
{
  int numwritten = myfwrite( &data, sizeof(uint64), 1, m_fp );
  if (numwritten < 1) {
    ERROR_OUT("error: ptrace_t: write64 fails\n");
    return (false);
  }
  return (true);
}

//**************************************************************************
uint8 ptrace_t::read8( void )
{
  uint8 result;
  int nread = myfread( &result, sizeof(uint8), 1, m_fp );
  if (nread != 1) {
    ERROR_OUT("error: read8: unable to read 8-bit integer\n");
  }
  return (result);
}

//**************************************************************************
uint16 ptrace_t::read16( void )
{
  uint16 result;
  int nread = myfread( &result, sizeof(uint16), 1, m_fp );
  if (nread != 1) {
    ERROR_OUT("error: read16: unable to read 16-bit integer\n");
  }
  return (result);
}

//**************************************************************************
uint32 ptrace_t::read32( void )
{
  uint32 result;
  int nread = myfread( &result, sizeof(uint32), 1, m_fp );
  if (nread != 1) {
    ERROR_OUT("error: read32: unable to read 32-bit integer\n");
  }
  return (result);
}

//**************************************************************************
uint64 ptrace_t::read64( void )
{
  uint64 result;
  int nread = myfread( &result, sizeof(uint64), 1, m_fp );
  if (nread != 1) {
    ERROR_OUT("error: read64: unable to read 64-bit integer\n");
  }
  return (result);
}

/*------------------------------------------------------------------------*/
/* Static methods                                                         */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Global functions                                                       */
/*------------------------------------------------------------------------*/
