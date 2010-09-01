
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
 * MemoryControl.h
 * 
 * Description:  See MemoryControl.C
 *
 * $Id: $
 *
 */

#ifndef MEMORY_CONTROL_H
#define MEMORY_CONTROL_H

#include "Global.h"
#include "Map.h"
#include "Address.h"
#include "Profiler.h"
#include "AbstractChip.h"
#include "System.h"
#include "Message.h"
#include "util.h"
#include "MemoryNode.h"
// Note that "MemoryMsg" is in the "generated" directory:
#include "MemoryMsg.h"
#include "Consumer.h"
#include "AbstractMemOrCache.h"

#include <list>

// This constant is part of the definition of tFAW; see
// the comments in header to MemoryControl.C
#define ACTIVATE_PER_TFAW 4

//////////////////////////////////////////////////////////////////////////////

class Consumer;

class MemoryControl : public Consumer, public AbstractMemOrCache {
public:

  // Constructors
  MemoryControl (AbstractChip* chip_ptr, int version);

  // Destructor
  ~MemoryControl ();
  
  // Public Methods

  void wakeup() ;

  void setConsumer (Consumer* consumer_ptr);
  Consumer* getConsumer () { return m_consumer_ptr; };
  void setDescription (const string& name) { m_name = name; };
  string getDescription () { return m_name; };

  // Called from the directory:
  void enqueue (const MsgPtr& message, int latency );
  void enqueueMemRef (MemoryNode& memRef);
  void dequeue ();
  const Message* peek (); 
  MemoryNode peekNode ();
  bool isReady();
  bool areNSlotsAvailable (int n) { return true; };  // infinite queue length

  //// Called from L3 cache:
  //void writeBack(physical_address_t addr);

  void printConfig (ostream& out);
  void print (ostream& out) const;
  void setDebug (int debugFlag);

private:

  void enqueueToDirectory (MemoryNode req, int latency);
  int getBank (physical_address_t addr);
  int getRank (int bank);
  bool queueReady (int bank);
  void issueRequest (int bank);
  bool issueRefresh (int bank);
  void markTfaw (int rank);
  void executeCycle ();

  // Private copy constructor and assignment operator
  MemoryControl (const MemoryControl& obj);
  MemoryControl& operator=(const MemoryControl& obj);

  // data members
  AbstractChip* m_chip_ptr;
  Consumer* m_consumer_ptr;  // Consumer to signal a wakeup()
  string m_name;
  int m_version;
  int m_msg_counter;
  int m_awakened;

  int m_mem_bus_cycle_multiplier;
  int m_banks_per_rank;
  int m_ranks_per_dimm;
  int m_dimms_per_channel;
  int m_bank_bit_0;
  int m_rank_bit_0;
  int m_dimm_bit_0;
  unsigned int m_bank_queue_size;
  int m_bank_busy_time;
  int m_rank_rank_delay;
  int m_read_write_delay;
  int m_basic_bus_busy_time;
  int m_mem_ctl_latency;
  int m_refresh_period;
  int m_memRandomArbitrate;
  int m_tFaw;
  int m_memFixedDelay;

  int m_total_banks;
  int m_total_ranks;
  int m_refresh_period_system;

  // queues where memory requests live

  list<MemoryNode> m_response_queue;
  list<MemoryNode> m_input_queue;
  list<MemoryNode>* m_bankQueues;

  // Each entry indicates number of address-bus cycles until bank
  // is reschedulable:
  int* m_bankBusyCounter;
  int* m_oldRequest;

  uint64* m_tfaw_shift;
  int* m_tfaw_count;

  // Each of these indicates number of address-bus cycles until
  // we can issue a new request of the corresponding type:
  int m_busBusyCounter_Write;
  int m_busBusyCounter_ReadNewRank;
  int m_busBusyCounter_Basic;

  int m_busBusy_WhichRank;  // which rank last granted
  int m_roundRobin;         // which bank queue was last granted
  int m_refresh_count;      // cycles until next refresh
  int m_need_refresh;       // set whenever m_refresh_count goes to zero
  int m_refresh_bank;       // which bank to refresh next
  int m_ageCounter;         // age of old requests; to detect starvation
  int m_idleCount;          // watchdog timer for shutting down
  int m_debug;              // turn on printf's
};

#endif  // MEMORY_CONTROL_H

