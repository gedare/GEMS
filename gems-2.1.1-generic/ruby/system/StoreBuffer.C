
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
 * $Id$
 *
 */

#include "Global.h"
#include "RubyConfig.h"
#include "StoreBuffer.h"
#include "AbstractChip.h"
#include "System.h"
#include "Driver.h"
#include "Vector.h"
#include "EventQueue.h"
#include "AddressProfiler.h"
#include "Sequencer.h"
#include "SubBlock.h"
#include "Profiler.h"

// *** Begin Helper class ***
struct StoreBufferEntry {
  StoreBufferEntry() {} // So we can allocate a vector of StoreBufferEntries
  StoreBufferEntry(const SubBlock& block, CacheRequestType type, const Address& pc, AccessModeType access_mode, int size, int thread) : m_subblock(block) {
    m_type = type;
    m_pc = pc;
    m_access_mode = access_mode;
    m_size = size;
    m_thread = thread;
    m_time = g_eventQueue_ptr->getTime();
  }

  void print(ostream& out) const
  {
    out << "[StoreBufferEntry: " 
        << "SubBlock: " << m_subblock
        << ", Type: " << m_type
        << ", PC: " << m_pc
        << ", AccessMode: " << m_access_mode
        << ", Size: " << m_size 
        << ", Thread: " << m_thread
        << ", Time: " << m_time 
        << "]";
  }
  
  SubBlock m_subblock;
  CacheRequestType m_type;
  Address m_pc;
  AccessModeType m_access_mode;
  int m_size;
  int m_thread;
  Time m_time;
};

extern inline 
ostream& operator<<(ostream& out, const StoreBufferEntry& obj)
{
  obj.print(out);
  out << flush;
  return out;
}

// *** End Helper class ***

const int MAX_ENTRIES = 128;

static void inc_index(int& index)
{
  index++;
  if (index >= MAX_ENTRIES) {
    index = 0;
  }
}

StoreBuffer::StoreBuffer(AbstractChip* chip_ptr, int version) :
  m_store_cache()
{
  m_chip_ptr = chip_ptr; 
  m_version = version;
  m_queue_ptr = new Vector<StoreBufferEntry>(MAX_ENTRIES);
  m_queue_ptr->setSize(MAX_ENTRIES);
  m_pending = false;
  m_seen_atomic = false;
  m_head = 0;
  m_tail = 0;
  m_size = 0;
  m_deadlock_check_scheduled = false;
}

StoreBuffer::~StoreBuffer()
{
  delete m_queue_ptr;
}

// Used only to check for deadlock
void StoreBuffer::wakeup()
{
  // Check for deadlock of any of the requests
  Time current_time = g_eventQueue_ptr->getTime();

  int queue_pointer = m_head;
  for (int i=0; i<m_size; i++) {
    if (current_time - (getEntry(queue_pointer).m_time) >= g_DEADLOCK_THRESHOLD) {
      WARN_EXPR(getEntry(queue_pointer));
      WARN_EXPR(m_chip_ptr->getID());
      WARN_EXPR(current_time);
      ERROR_MSG("Possible Deadlock detected");
    }
    inc_index(queue_pointer);
  }

  if (m_size > 0) { // If there are still outstanding requests, keep checking
    g_eventQueue_ptr->scheduleEvent(this, g_DEADLOCK_THRESHOLD);    
  } else {
    m_deadlock_check_scheduled = false;
  }
}

void StoreBuffer::printConfig(ostream& out)
{
  out << "Store buffer entries: " << MAX_ENTRIES << " (Only valid if TSO is enabled)" << endl;
}

// Handle an incoming store request, this method is responsible for
// calling hitCallback as needed
void StoreBuffer::insertStore(const CacheMsg& request) 
{
  Address addr = request.getAddress();
  CacheRequestType type = request.getType();
  Address pc = request.getProgramCounter();
  AccessModeType access_mode = request.getAccessMode();
  int size = request.getSize();
  int threadID = request.getThreadID();

  DEBUG_MSG(STOREBUFFER_COMP, MedPrio, "insertStore"); 
  DEBUG_EXPR(STOREBUFFER_COMP, MedPrio, g_eventQueue_ptr->getTime());
  assert((type == CacheRequestType_ST) || (type == CacheRequestType_ATOMIC));
  assert(isReady());

  // See if we should schedule a deadlock check
  if (m_deadlock_check_scheduled == false) {
    g_eventQueue_ptr->scheduleEvent(this, g_DEADLOCK_THRESHOLD);
    m_deadlock_check_scheduled = true;
  }

  // Perform the hit-callback for the store
  SubBlock subblock(addr, size);
  if(type == CacheRequestType_ST) {
    g_system_ptr->getDriver()->hitCallback(m_chip_ptr->getID(), subblock, type, threadID);
    assert(subblock.getSize() != 0);
  } else {
    // wait to perform the hitCallback until later for Atomics
  } 

  // Perform possible pre-fetch
  if(!isEmpty()) {
    CacheMsg new_request = request;
    new_request.getPrefetch() = PrefetchBit_Yes;
    m_chip_ptr->getSequencer(m_version)->makeRequest(new_request);
  }

  // Update the StoreCache
  m_store_cache.add(subblock);

  // Enqueue the entry
  StoreBufferEntry entry(subblock, type, pc, access_mode, size, threadID); // FIXME
  enqueue(entry);

  if(type == CacheRequestType_ATOMIC) {
    m_seen_atomic = true;
  }

  processHeadOfQueue();
}

void StoreBuffer::callBack(const Address& addr, DataBlock& data)
{
  DEBUG_MSG(STOREBUFFER_COMP, MedPrio, "callBack"); 
  DEBUG_EXPR(STOREBUFFER_COMP, MedPrio, g_eventQueue_ptr->getTime());
  assert(!isEmpty());
  assert(m_pending == true);
  assert(line_address(addr) == addr);
  assert(line_address(m_pending_address) == addr);
  assert(line_address(peek().m_subblock.getAddress()) == addr);
  CacheRequestType type = peek().m_type;
  int threadID = peek().m_thread;
  assert((type == CacheRequestType_ST) || (type == CacheRequestType_ATOMIC));
  m_pending = false;

  // If oldest entry was ATOMIC, perform the callback
  if(type == CacheRequestType_ST) {
    // We already performed the call back for the store at insert time
  } else {
    // We waited to perform the hitCallback until now for Atomics
    peek().m_subblock.mergeFrom(data);  // copy the correct bytes from DataBlock into the SubBlock for the Load part of the atomic Load/Store
    g_system_ptr->getDriver()->hitCallback(m_chip_ptr->getID(), peek().m_subblock, type, threadID);
    m_seen_atomic = false;

    /// FIXME - record the time spent in the store buffer - split out ST vs ATOMIC
  }
  assert(peek().m_subblock.getSize() != 0);

  // Apply the head entry to the datablock
  peek().m_subblock.mergeTo(data);  // For both the Store and Atomic cases

  // Update the StoreCache
  m_store_cache.remove(peek().m_subblock);

  // Dequeue the entry from the store buffer
  dequeue();

  if (isEmpty()) {
    assert(m_store_cache.isEmpty());
  }

  if(type == CacheRequestType_ATOMIC) {
    assert(isEmpty());
  }

  // See if we can remove any more entries
  processHeadOfQueue();
}

void StoreBuffer::processHeadOfQueue()
{
  if(!isEmpty() && !m_pending) {
    StoreBufferEntry& entry = peek();
    assert(m_pending == false);
    m_pending = true;
    m_pending_address = entry.m_subblock.getAddress();
    CacheMsg request(entry.m_subblock.getAddress(), entry.m_subblock.getAddress(), entry.m_type, entry.m_pc, entry.m_access_mode, entry.m_size, PrefetchBit_No, 0, Address(0), entry.m_thread, 0, false);  
    m_chip_ptr->getSequencer(m_version)->doRequest(request);
  }
}

bool StoreBuffer::isReady() const
{
  return ((m_size < MAX_ENTRIES) && (!m_seen_atomic));
}

// Queue implementation methods

StoreBufferEntry& StoreBuffer::peek()
{
  return getEntry(m_head);
}

void StoreBuffer::dequeue()
{
  assert(m_size > 0); 
  m_size--; 
  inc_index(m_head);
}

void StoreBuffer::enqueue(const StoreBufferEntry& entry)
{ 
  //  assert(isReady()); 
  (*m_queue_ptr)[m_tail] = entry; 
  m_size++;
  g_system_ptr->getProfiler()->storeBuffer(m_size, m_store_cache.size());
  inc_index(m_tail);
}

StoreBufferEntry& StoreBuffer::getEntry(int index)
{ 
  return (*m_queue_ptr)[index];
}

void StoreBuffer::print(ostream& out) const
{
  out << "[StoreBuffer]";
}

