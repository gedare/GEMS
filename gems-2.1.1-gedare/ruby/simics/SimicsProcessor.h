
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
 * $Id: SimicsProcessor.h 1.8 05/01/19 13:12:31-06:00 mikem@maya.cs.wisc.edu $
 *
 * Description: This class models a processing board in simics. In my view,
 *              this board may contains a in-order processor core which will
 *              "query" the memory system interface before and after a memory
 *              instruction. The query before determines timing and once the
 *              timing is satisfied, the query after the instruction execution
 *              is responsible for updating memory value. For load
 *              instructions, the query before the instruction execution also
 *              determines the memory value from the memory system.
 *
 *              The simple processor model is also able to take exceptions and
 *              perform memory mapped/instruction based (through ASI) I/O
 *              operations.
 *
 *              Finally, this processing board also has certain I/O devices
 *              attached and they are capable of doing DMA operations.
 *
 *              In the future, with more and more functionality added to the
 *              object, we could imagine this object becomre a good match with
 *              the simics processor model, which can further perform advanced
 *              operations like "cache flush".
 *
 */

#ifndef SimicsProcessor_H
#define SimicsProcessor_H

#include "Global.h"
#include "Driver.h"
#include "Vector.h"
#include "CacheRequestType.h"
#include "Map.h"
#include "CacheMsg.h"
#include "SimicsDriver.h"

extern "C" {
#include "simics/api.h"
}

class System;
class Sequencer;
class TransactionInterfaceManager;
class TransactionSimicsProcessor;

class SimicsProcessor {
public:
  // Constructors
  SimicsProcessor(System* sys_ptr, int proc);

  // Destructor
  ~SimicsProcessor() {};
  
  // Public Methods
	TransactionSimicsProcessor* getTransactionSimicsProcessor();
  TransactionInterfaceManager* getTransactionInterfaceManager();
  void setTransactionInterfaceManager(TransactionInterfaceManager* xact_mgr);

  MemoryTransactionResult makeRequest(memory_transaction_t *mem_trans);
  void observeMemoryAccess(memory_transaction_t *mem_trans);
  void exceptionCallback(integer_t exc);
#ifdef SPARC
  exception_type_t asiCallback(memory_transaction_t *mem_trans);
#endif

  void hitCallback(CacheRequestType type, SubBlock& data);
  void conflictCallback(CacheRequestType type, SubBlock& data);
  void abortCallback(SubBlock& data);
  
  integer_t getInstructionCount() const;
  integer_t getCycleCount() const;
  
  void print(ostream& out) const;

  // called during abort handling
  void notifyTrapStart( const Address & handlerPC );
  void notifyTrapComplete( const Address & newPC );
  void restoreCheckpoint();
  int getTransactionLevel();
  // called by sequencer
  void notifySendNack(const Address & addr, uint64 remote_timestamp, const MachineID & remote_id);
  void notifyReceiveNack(const Address & addr, uint64 remote_timestamp, const MachineID & remote_id);
  void notifyReceiveNackFinal(const Address & addr);
  // for calculating abort delay
  int getAbortDelay();

private:

  // Private Methods

  // Private copy constructor and assignment operator
  SimicsProcessor(const SimicsProcessor& obj);
  SimicsProcessor& operator=(const SimicsProcessor& obj);

#ifdef SPARC
  // translate ASI base on if we are in supervisor mode
  int translateASI(const memory_transaction_t* mem_trans);
#endif

  // clear the active request vector
  void clearActiveRequestVector();

  // get a free slot in the active request vector
  int getFreeSlotInActiveRequestVector();

  // get the current serving request
  int getServingRequestSlot();

  // get the current logging serving request
  int getLoggingRequestSlot();

  // lookup a request and check the size
  int lookupRequest(Address addr, CacheRequestType type, int size);

  // translate a simics memory transaction to a CacheRequest object
  CacheMsg memoryTransactionToCacheMsg(const memory_transaction_t *mem_trans);
  CacheRequestType getRequestType(const memory_transaction_t *mem_trans);
  // Data Members (m_ prefix)

  // OK, the following is a way to solve simics reissuing problem.
  //
  // Here, we keep a vector of "active" request generated by the current
  // executing instruction. The implicit assumption is that each instruction:
  //   1. generating up-to-one i-fetch
  //   2. generating up-to-one read to a given address
  //   3. generating up-to-one write to a given address
  //   4. generating up-to-one atomic to a given address
  //
  // We also keep the size information of the request just for sanity check.
  // The instruction should not generate overlapping request to a given memory
  // location with the same operation type. (e.g. load 0x10 size 4 and load
  // 0x12 size 2)

  enum CacheRequestStatus {
    Invalid,
    Queued,
    Serving,
    Done,
    Trap,
    Retry
  };

  struct CacheRequest {
    CacheRequestStatus status;
    CacheRequestType   type;
    Address            addr;
    int                size;
    bool               success;
  };

  // outstanding instruction count
  integer_t m_current_instruction_count;

  // simics cycle count
  integer_t m_current_cycle_count;

  // requests generated by this instruction, by nature of Simics, there is only
  // one serving request at any given time.
  Vector<struct CacheRequest> m_active_requests;

  // lingering request after an exception
  Address m_lingering_request_address;

  // initial simics IC
  integer_t m_initial_instruction_count;

  // initial simics cycle count
  integer_t m_initial_cycle_count;

  // processor id
  int m_proc;

  // sequence number for memory requests
  uint64 m_seqnum;

  // corresponding sequencer
  Sequencer* m_sequencer;

  // corresponding Transactional Processor
  TransactionSimicsProcessor* m_xact_proc;

  // transactional memory
  TransactionInterfaceManager* m_xact_mgr;

  CacheRequestStatus string_to_CacheRequestStatus(const string& str);

  friend ostream& operator<<(ostream& out, const SimicsProcessor::CacheRequestStatus& obj);
};

// Output operator declaration
ostream& operator<<(ostream& out, const SimicsProcessor& obj);

// ******************* Definitions *******************

// Output operator definition
extern inline 
ostream& operator<<(ostream& out, const SimicsProcessor& obj)
{
//  obj.print(out);
  out << flush;
  return out;
}

#endif // SimicsProcessor_H



