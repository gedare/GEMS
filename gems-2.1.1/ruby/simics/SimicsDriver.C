
/*
    Copyright (C) 1999-2008 by Mark D. Hill and David A. Wood for the
    Wisconsin Multifacet Project.  Contact: gems@cs.wisc.edu
    http://www.cs.wisc.edu/gems/

    --------------------------------------------------------------------

    This file is part of the Ruby Multiprocessor Memory System Simulator, 
    a component of the Multifacet GEMS (General Execution-driven 
    Multiprocessor Simulator) software toolset originally developed at 
    the University of Wisconsin-Madison.

    Ruby was originally developed primarily by Milo Martin and Daniel
    Sorin with contributions from Ross Dickson, Carl Mauer, and Manoj
    Plakal.

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
 * $Id: SimicsDriver.C 1.12 05/01/19 13:12:31-06:00 mikem@maya.cs.wisc.edu $
 *
 * Description: See SimicsDriver.h for document.
 */

#include "SimicsDriver.h"
#include "interface.h"
#include "System.h"
#include "SubBlock.h"
#include "Profiler.h"
#include "AddressProfiler.h"
#include "SimicsProcessor.h"
#include "SimicsHypervisor.h"

// Simics includes
extern "C" {
#include "simics/api.h"
}

// Macros to get privileged mode bit
#ifdef SPARC
  #define PRIV_MODE (mem_trans->priv)
#else
// x86
  #define PRIV_MODE (mem_trans->mode)
#endif

#ifdef SIMICS30
#define IS_DEV_MEM_OP(foo) (foo == Sim_Initiator_Device)
#define IS_OTH_MEM_OP(foo) (foo == Sim_Initiator_Other)
#endif

// Contructor
SimicsDriver::SimicsDriver(System* sys_ptr) {
  clearStats();

  // make sure that MemoryTransactionResult enum and s_stallCycleMap[] are the same size
  assert(s_stallCycleMap[LAST].result == LAST);

  // setup processor objects
  m_processors.setSize(RubyConfig::numberOfProcessors());
  m_perfect_memory_firstcall_flags.setSize(RubyConfig::numberOfProcessors());
  for(int i=0; i < RubyConfig::numberOfProcessors(); i++){
    m_processors[i] = new SimicsProcessor(sys_ptr, i);
    m_perfect_memory_firstcall_flags[i] = true;
  }
  
  // setup hypervisor object
  m_hypervisor = new SimicsHypervisor(sys_ptr, m_processors);

  // in interface.C, this method posts a wakeup on Simics' event queue
  SIMICS_wakeup_ruby();

  // in interface.C, this method causes "core_exception_callback"
  SIMICS_install_exception_callback();
}

// Destructor
SimicsDriver::~SimicsDriver() {
  // in interface.C, this method removes the wakeup on Simics's event queue
  SIMICS_remove_ruby_callback();

  // in interface.C, this method removes the core exception callback
  SIMICS_remove_exception_callback();

  // destory all processors
  for(int i=0; i < RubyConfig::numberOfProcessors(); i++){
    delete m_processors[i];
    m_processors[i] = NULL;
  }

}

// print the config parameters
void SimicsDriver::printConfig(ostream& out) const {
  out << "Simics ruby multiplier: " << SIMICS_RUBY_MULTIPLIER << endl;
  out << "Simics stall time: " << SIMICS_STALL_TIME << endl;
  out << endl;
}

void SimicsDriver::printStats(ostream& out) const {
  out << endl;
  out << "Simics Driver Transaction Stats" << endl;
  out << "----------------------------------" << endl;
  out << "Insn requests: " << m_insn_requests << endl;
  out << "Data requests: " << m_data_requests << endl;

  out << "Memory mapped IO register accesses: " << m_mem_mapped_io_counter << endl;
  out << "Device initiated accesses: " << m_device_initiator_counter << endl;
  out << "Other initiated accesses: " << m_other_initiator_counter << endl;

  out << "Atomic load accesses: " << m_atomic_load_counter << endl;
  out << "Exceptions: " << m_exception_counter << endl;
  out << "Non stallable accesses: " << m_non_stallable << endl;
  out << "Prefetches: " << m_prefetch_counter << endl;
  out << "Cache Flush: " << m_cache_flush_counter << endl;
  out << endl;
  for (int i = 0; i < MAX_ADDRESS_SPACE_ID; i++) {
    if (m_asi_counters[i] != 0) {
      out << "Requests of asi 0x" << hex << i << dec << ": " << m_asi_counters[i] << endl;
    }
  }
  out << endl;

  out << "Simics Driver Transaction Results Stats" << endl;
  out << "------------------------------------------" << endl;
  out << "Fast path: " << m_fast_path_counter << endl;
  out << "Request missed: " << m_request_missed_counter << endl;
  out << "Sequencer not ready: " << m_sequencer_not_ready_counter << endl;
  out << "Duplicate instruction fetches: " << m_duplicate_instruction_fetch_counter << endl;
  out << "Hit return: " << m_hit_return_counter << endl;
  out << "Atomic last accesses: " << m_atomic_last_counter << endl;
}

void SimicsDriver::clearStats() {
  // transaction stats
  m_insn_requests = 0;
  m_data_requests = 0;

  m_mem_mapped_io_counter = 0;
  m_device_initiator_counter = 0;
  m_other_initiator_counter = 0;

  m_atomic_load_counter = 0;
  m_exception_counter = 0;
  m_non_stallable = 0;
  m_prefetch_counter = 0;
  m_cache_flush_counter = 0;

  m_asi_counters.setSize( MAX_ADDRESS_SPACE_ID );
  for(int i=0; i < MAX_ADDRESS_SPACE_ID; i++) {
    m_asi_counters[i] = 0;
  }

  // transaction results stats
  m_fast_path_counter = 0;
  m_request_missed_counter = 0;
  m_sequencer_not_ready_counter = 0;
  m_duplicate_instruction_fetch_counter = 0;
  m_hit_return_counter = 0;
  m_atomic_last_counter = 0;

}

integer_t SimicsDriver::getInstructionCount(int proc) const {
  return m_processors[proc]->getInstructionCount();
}

integer_t SimicsDriver::getCycleCount(int proc) const {
  return m_processors[proc]->getCycleCount();
}


/******************************************************************
 * void hitCallback(int cpuNumber)
 * Called by Sequencer when the data is ready in the cache
 ******************************************************************/
void SimicsDriver::hitCallback(NodeID proc, SubBlock& data, CacheRequestType type, int thread) {
  m_processors[proc]->hitCallback(type, data);
}

void SimicsDriver::conflictCallback(NodeID proc, SubBlock& data, CacheRequestType type, int thread) {
  m_processors[proc]->conflictCallback(type, data);
}

void SimicsDriver::abortCallback(NodeID proc, SubBlock& data, CacheRequestType type, int thread) {
  m_processors[proc]->abortCallback(data);
}

/******************************************************************
 * makeRequest(memory_transaction_t *mem_trans)
 * Called by function
 * mh_memorytracer_possible_cache_miss(memory_transaction_t *mem_trans)
 * which was called by Simics via ruby.c.
 * May call Sequencer.
 ******************************************************************/
int SimicsDriver::makeRequest(memory_transaction_t *mem_trans) {

  // Special case here
  if (PERFECT_MEMORY_SYSTEM && !isUnhandledTransaction(mem_trans)) {
    int proc = SIMICS_get_proc_no(mem_trans->s.ini_ptr);

    // we need to notify simics...
    if(proc == -1 || PERFECT_MEMORY_SYSTEM_LATENCY==0) {
            
      // this is OK, since returning zero won't stall simics
      return s_stallCycleMap[Hit].cycles;
    } else {
      assert(PERFECT_MEMORY_SYSTEM_LATENCY>0);

      // Simics will re-call makeRequest after the specified number
      // of cycles...at which time we should return 0
      if(m_perfect_memory_firstcall_flags[proc]) {
        m_perfect_memory_firstcall_flags[proc] = false;
        return PERFECT_MEMORY_SYSTEM_LATENCY;
      } else {
        m_perfect_memory_firstcall_flags[proc] = true;
        return s_stallCycleMap[Hit].cycles;
      }
    }    
  }

  MemoryTransactionResult ret;

  // skip unsupported types
  if (isUnhandledTransaction(mem_trans)) {
    ret = Unhandled;
  } else {
    // perform makeRequest()
    int proc = SIMICS_get_proc_no(mem_trans->s.ini_ptr);
    ret = m_processors[proc]->makeRequest(mem_trans);
  }

  // record transaction stats if they are not duplicated
  if( ret != Not_Ready &&
      ret != I_Replay &&
      ret != D_Replay &&
      ret != A_Replay ) {
    recordTransactionStats(mem_trans);
  }

  // record result stats
  recordTransactionResultStats(ret);

  // stall time
  int latency = s_stallCycleMap[ret].cycles;
  return latency;
}

// Called whenever we want are about to call the SW handler
void SimicsDriver::notifyTrapStart( int cpuNumber, const Address & handlerPC, int threadID, int smtThread ){
  assert( cpuNumber <  RubyConfig::numberOfProcessors() );
  m_processors[cpuNumber]->notifyTrapStart(handlerPC);
}

// Called whenever we want the SW handler to inform us when it is finished
void SimicsDriver::notifyTrapComplete( int cpuNumber, const Address & newPC, int smtThread ){
  assert( cpuNumber <  RubyConfig::numberOfProcessors() );
  m_processors[cpuNumber]->notifyTrapComplete(newPC);
}

/******************************************************************
 * observeMemoryAccess(memory_transaction_t *mem_trans)
 * This function is called when memory instructions complete, and
 * it allows the cache hierarchy to observe the load/store value that
 * was read/written by the memory request. It is only called when in
 * Simics and DATA_BLOCK is defined, by the function
 * mh_memorytracer_observe_memory(memory_transaction_t *mem_trans)
 * which was called by Simics via ruby.c.
 ******************************************************************/
void SimicsDriver::observeMemoryAccess(memory_transaction_t *mem_trans) {

  // NOTE: we should NOT do any filtering in this function!
  //
  // makeRequest() can be viewed as an information stream of timing of the
  // memory system.
  // observeMemoryAccess() can be viewed as an information stream of value of
  // the memory system.
  //
  // So makeRequest will change the cache's meta-state, observeMemoryAccess
  // will change the cache's state. What we do in this function is that
  //     1. match cache value (if any) for loads
  //     2. update cache content (if any) for stores

  // NEED TO FILTER THESE OUT FOR LAZY XACT SYSTEMS
  if (isUnhandledTransaction(mem_trans)) 
    return;
            
  // perform observeMemoryAccess()
  int proc = SIMICS_get_proc_no(mem_trans->s.ini_ptr);
  m_processors[proc]->observeMemoryAccess(mem_trans);
}

void SimicsDriver::exceptionCallback(conf_object_t *cpuPtr, integer_t exc) {

  m_exception_counter++;

  int proc = SIMICS_get_proc_no(cpuPtr);
  m_processors[proc]->exceptionCallback(exc);
}

void SimicsDriver::recordTransactionStats(memory_transaction_t* mem_trans) {

  if (mem_trans->s.type == Sim_Trans_Prefetch) {
    m_prefetch_counter++;
  }

  if (mem_trans->s.type == Sim_Trans_Cache) {
    m_cache_flush_counter++;
  }

  if(mem_trans->s.type == Sim_Trans_Instr_Fetch) {
    m_insn_requests++;
  } else {
    m_data_requests++;
  }

  generic_transaction_t *g = (generic_transaction_t *)mem_trans;

  // Check for DMA/IO
  if (IS_DEV_MEM_OP(g->ini_type)) {
    m_device_initiator_counter++;
  } else if (IS_OTH_MEM_OP(g->ini_type)) {
    m_other_initiator_counter++;
  } else if (mem_trans->s.physical_address > uinteger_t(RubyConfig::memorySizeBytes())) {
    m_mem_mapped_io_counter++;
  }

#ifdef SPARC
  // record asi of transaction
  m_asi_counters[mem_trans->address_space]++;

  // atomic loads (ldda), generate LD requests instead of ATOMIC requests
  if ( mem_trans->s.atomic == true && SIMICS_is_ldda(mem_trans)) {
    m_atomic_load_counter++;
  }
#endif

  // count non-stallable requests
  if(!(mem_trans->s.may_stall)) {
    m_non_stallable++;
  }

}

#ifdef SPARC
exception_type_t SimicsDriver::asiCallback(conf_object_t * cpu, generic_transaction_t *g) {

  // only real processor should init ASI accesses
  int proc = SIMICS_get_proc_no(cpu);
  assert(proc != -1);

  return m_processors[proc]->asiCallback((memory_transaction_t*)g);

}
#endif

bool SimicsDriver::isUnhandledTransaction(memory_transaction_t* mem_trans) {
  // only handle user data?
  if (USER_MODE_DATA_ONLY) {
    if(PRIV_MODE) {
      return true;
    }
    if(mem_trans->s.type == Sim_Trans_Instr_Fetch) {
      return true;
    }
  }

  // no prefetches
  if (mem_trans->s.type == Sim_Trans_Prefetch) {
    return true;
  }

  // no cache flush
  if (mem_trans->s.type == Sim_Trans_Cache) {
    return true;
  }

  // no DMA & IO
  if (IS_DEV_MEM_OP(mem_trans->s.ini_type) ||
      IS_OTH_MEM_OP(mem_trans->s.ini_type) ||
      mem_trans->s.physical_address > uinteger_t(RubyConfig::memorySizeBytes())
     ) {
    return true;
  }

  return false;
}

void SimicsDriver::recordTransactionResultStats(
  MemoryTransactionResult result) {

  switch(result) {
    case Hit:
      m_fast_path_counter++;
      break;
    case Miss:
      m_request_missed_counter++;
      break;
    case Non_Stallable:
      {/* nothing yet */};
      break;
    case Unhandled:
      {/* nothing yet */};
      break;
    case Not_Ready:
      m_sequencer_not_ready_counter++;
      break;
    case I_Replay:
      m_duplicate_instruction_fetch_counter++;
      break;
    case D_Replay:
      m_hit_return_counter++;
      break;
    case A_Replay:
      m_atomic_last_counter++;
      break;
    case Conflict_Retry:
      break;  
    default:
      ERROR_MSG("Error: Strange memory transaction result");
  }
}

MemoryTransactionResult string_to_MemoryTransactionResult(const string& str) {
  if (str == "Hit") {
    return Hit;
  } else if (str == "Miss") {
    return Miss;
  } else if (str == "Non_Stallable") {
    return Non_Stallable;
  } else if (str == "Unhandled") {
    return Unhandled;
  } else if (str == "Not_Ready") {
    return Not_Ready;
  } else if (str == "I_Replay") {
    return I_Replay;
  } else if (str == "D_Replay") {
    return D_Replay;
  } else if (str == "A_Replay") {
    return A_Replay;
  } else if (str == "Conflict_Retry") {
    return Conflict_Retry;
  } else {
    WARN_EXPR(str);
    ERROR_MSG("Invalid string conversion for type MemoryTransactionResult");
  }
}

integer_t SimicsDriver::readPhysicalMemory(int procID, 
                                           physical_address_t address,
                                           int len ){
  return SIMICS_read_physical_memory(procID, address, len);
}

void SimicsDriver::writePhysicalMemory( int procID, 
                                        physical_address_t address,
                                        integer_t value, 
                                        int len ){
  SIMICS_write_physical_memory(procID, address, value, len);
}

ostream& operator<<(ostream& out, const MemoryTransactionResult& obj) {
  switch(obj) {
    case Hit:
      out << "Hit";
      break;
    case Miss:
      out << "Miss";
      break;
    case Non_Stallable:
      out << "Non_Stallable";
      break;
    case Unhandled:
      out << "Unhandled";
      break;
    case Not_Ready:
      out << "Not_Ready";
      break;
    case I_Replay:
      out << "I_Replay";
      break;
    case D_Replay:
      out << "D_Replay";
      break;
    case A_Replay:
      out << "A_Replay";
      break;
    case Conflict_Retry:
      out << "Conflict_Retry";
      break;
    default:
      ERROR_MSG("Invalid range for type MemoryTransactionResult");
      break;
  }
  out << flush;
  return out;
}

SimicsHypervisor*
SimicsDriver::getHypervisor() 
{
  return m_hypervisor;
}

