
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
 * $Id: SimicsDriver.h 1.9 05/01/19 13:12:31-06:00 mikem@maya.cs.wisc.edu $
 *
 * Description: This class is a "dispatcher" object in front of the
 *              SimicsProcessor objects. Basically, this object takes simics
 *              memory/execption/IO requests and demux them on processor basis.
 *              At the same time, this object is a filter for those unsupported
 *              simics operations by SimicsProcessor object. Finally, this
 *              object als serve as a stats counter which counts various events
 *              that observed from the simics interface.
 *
 *              Another goal of breaking SimicsInterface into SimicsDriver and
 *              SimicsProcessor is to separating policy from mechanism.
 *              SimicsDriver should contains the policy: what is support, what
 *              is not; hit, miss cycle; what to do when sequencer is busy,
 *              etc. SimicsProcessor should contain mechanism: how to perform
 *              an access; how to get a cache value; etc.
 *
 *              You should expect simics interface related hack localized in
 *              this object. You should not add simics hacks to SimicsProcessor
 *              object.
 *
 */

#ifndef SimicsDriver_H
#define SimicsDriver_H

#include "Global.h"
#include "Driver.h"
#include "interface.h"

extern "C" {
#include "simics/api.h"
}

class System;
class SimicsProcessor;
class SimicsHypervisor;

// Used when we count IOs
static const int MAX_ADDRESS_SPACE_ID = (1<<8);

// a super long time that makes simics process stall until we unstall it
static const int SIMICS_STALL_TIME = 2000000000;
static const int SIMICS_RETRY_TIME = 1;
static const int SIMICS_CONFLICT_RETRY_TIME = 3;

typedef enum {
  Hit           = 0, /* request hit */
  Miss,              /* request missed */
  Non_Stallable,     /* request non-stallable */
  Unhandled,         /* request not handled by ruby */
  Not_Ready,         /* not ready to handle */
  I_Replay,          /* Replay of I-Fetch */
  D_Replay,          /* Replay of Data access */
  A_Replay,          /* Replay of Atomic instruction */
  Conflict_Retry,    /* Retry */
  LAST,              /* UNUSED.  FOR TYPE-CHECKING */
} MemoryTransactionResult;

typedef struct { MemoryTransactionResult result; int cycles; } StallCycle;

// A map between transaction results and stall cycles
// The order below MUST match the order above!!
static StallCycle s_stallCycleMap[] = {
  { Hit            , 0},
  { Miss           , SIMICS_STALL_TIME},
  { Non_Stallable  , 0},
  { Unhandled      , 0},
  { Not_Ready      , SIMICS_RETRY_TIME},
  { I_Replay       , 0},
  { D_Replay       , 0},
  { A_Replay       , 0},
  { Conflict_Retry , SIMICS_CONFLICT_RETRY_TIME},
  { LAST, -1},
};

MemoryTransactionResult string_to_MemoryTransactionResult(const string& str);
ostream& operator<<(ostream& out, const MemoryTransactionResult& obj);

class SimicsDriver : public Driver {
public:
  // Constructors
  SimicsDriver(System* sys_ptr);

  // Destructor
  virtual ~SimicsDriver();
  
  // Public Methods

  // methods called by simics
  int  makeRequest(memory_transaction_t *mem_trans);
  void observeMemoryAccess(memory_transaction_t *mem_trans);
  void exceptionCallback(conf_object_t *cpu, integer_t exc);
#ifdef SPARC
  exception_type_t asiCallback(conf_object_t *cpu, generic_transaction_t *g);
#endif

  // methods called by ruby
  void hitCallback(NodeID proc, SubBlock& data, CacheRequestType type, int thread);
  void abortCallback(NodeID proc, SubBlock& data, CacheRequestType type, int thread);
  void conflictCallback(NodeID proc, SubBlock& data, CacheRequestType type, int thread);
  
  integer_t getInstructionCount(int procID) const;
  integer_t getCycleCount(int procID) const;

  integer_t readPhysicalMemory(int procID, physical_address_t address,
                               int len );

  void writePhysicalMemory( int procID, physical_address_t address,
                            integer_t value, int len );

  void abortAll();

  void printStats(ostream& out) const;
  void printConfig(ostream& out) const;

  void print(ostream& out) const;

  // These are called during trap handling
  void notifyTrapStart( int cpuNumber, const Address & handlerPC, int threadID, int smtThread);
  void notifyTrapComplete( int cpuNumber, const Address & newPC, int smtThread);

  SimicsHypervisor* getHypervisor(); 

private:

  // Private Methods

  // assign zeros to all counters
  void clearStats();

  // Private copy constructor and assignment operator
  SimicsDriver(const SimicsDriver& obj);
  SimicsDriver& operator=(const SimicsDriver& obj);
  
  // A memory transaction stats counter
  void recordTransactionStats(memory_transaction_t *mem_trans);

  // A memory transaction filter
  bool isUnhandledTransaction(memory_transaction_t *mem_trans);

  // A memory transaction stats counter
  void recordTransactionResultStats(MemoryTransactionResult result);

  // Data Members (m_ prefix)

  // Processors that actually process the request
  Vector<SimicsProcessor*> m_processors;

  // Tracks calls to makeRequest for PERFECT_MEMORY_SYSTEM flag
  Vector<bool> m_perfect_memory_firstcall_flags;

  // Hypervisor 
  SimicsHypervisor *m_hypervisor; 
  
  //  Stats counters

  // transaction stats
  integer_t m_insn_requests;
  integer_t m_data_requests;

  integer_t m_mem_mapped_io_counter;
  integer_t m_device_initiator_counter;
  integer_t m_other_initiator_counter;

  integer_t m_atomic_load_counter;
  integer_t m_exception_counter;
  integer_t m_non_stallable;
  integer_t m_prefetch_counter;
  integer_t m_cache_flush_counter;

  Vector<integer_t> m_asi_counters;

  // transition results stats
  integer_t m_fast_path_counter;
  integer_t m_request_missed_counter;
  integer_t m_sequencer_not_ready_counter;
  integer_t m_duplicate_instruction_fetch_counter;
  integer_t m_hit_return_counter;
  integer_t m_atomic_last_counter;

};

// Output operator declaration
ostream& operator<<(ostream& out, const SimicsDriver& obj);

// ******************* Definitions *******************

// Output operator definition
extern inline 
ostream& operator<<(ostream& out, const SimicsDriver& obj)
{
//  obj.print(out);
  out << flush;
  return out;
}

#endif // SimicsDriver_H

