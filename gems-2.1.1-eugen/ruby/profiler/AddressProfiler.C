
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
 * AddressProfiler.C
 * 
 * Description: See AddressProfiler.h
 *
 * $Id$
 *
 */

#include "AddressProfiler.h"
#include "CacheMsg.h"
#include "AccessTraceForAddress.h"
#include "PrioHeap.h"
#include "Map.h"
#include "System.h"
#include "Profiler.h"

// Helper functions
static AccessTraceForAddress& lookupTraceForAddress(const Address& addr, Map<Address, AccessTraceForAddress>* record_map);
static void printSorted(ostream& out, const Map<Address, AccessTraceForAddress>* record_map, string description);

AddressProfiler::AddressProfiler()
{
  m_dataAccessTrace = new Map<Address, AccessTraceForAddress>;
  m_macroBlockAccessTrace = new Map<Address, AccessTraceForAddress>;
  m_programCounterAccessTrace = new Map<Address, AccessTraceForAddress>;
  m_retryProfileMap = new Map<Address, AccessTraceForAddress>;
  m_persistentPredictionProfileMap = new Map<Address, AccessTraceForAddress>;
  clearStats();
}

AddressProfiler::~AddressProfiler()
{
  delete m_dataAccessTrace;
  delete m_macroBlockAccessTrace;
  delete m_programCounterAccessTrace;
  delete m_retryProfileMap;
  delete m_persistentPredictionProfileMap;
}

void AddressProfiler::printStats(ostream& out) const
{
  if (PROFILE_HOT_LINES) {
    out << endl;
    out << "AddressProfiler Stats" << endl;
    out << "---------------------" << endl;
    
    out << endl;
    out << "sharing_misses: " << m_sharing_miss_counter << endl;
    out << "getx_sharing_histogram: " << m_getx_sharing_histogram << endl;
    out << "gets_sharing_histogram: " << m_gets_sharing_histogram << endl;
    
    out << endl;
    out << "Hot Data Blocks" << endl;
    out << "---------------" << endl;
    out << endl;
    printSorted(out, m_dataAccessTrace, "block_address");
    
    out << endl;
    out << "Hot MacroData Blocks" << endl;
    out << "--------------------" << endl;
    out << endl;
    printSorted(out, m_macroBlockAccessTrace, "macroblock_address");
    
    out << "Hot Instructions" << endl;
    out << "----------------" << endl;
    out << endl;
    printSorted(out, m_programCounterAccessTrace, "pc_address");
  }

  if (PROFILE_ALL_INSTRUCTIONS){
    out << endl;
    out << "All Instructions Profile:" << endl;
    out << "-------------------------" << endl;
    out << endl;
    printSorted(out, m_programCounterAccessTrace, "pc_address");
    out << endl;  
  }
  
  if (m_retryProfileHisto.size() > 0) {
    out << "Retry Profile" << endl;
    out << "-------------" << endl;
    out << endl;
    out << "retry_histogram_absolute: " << m_retryProfileHisto << endl;
    out << "retry_histogram_write: " << m_retryProfileHistoWrite << endl;
    out << "retry_histogram_read: " << m_retryProfileHistoRead << endl;

    out << "retry_histogram_percent: ";
    m_retryProfileHisto.printPercent(out);
    out << endl;

    out << "retry_histogram_per_instruction: ";
    m_retryProfileHisto.printWithMultiplier(out, 1.0 / double(g_system_ptr->getProfiler()->getTotalInstructionsExecuted()));
    out << endl;

    printSorted(out, m_retryProfileMap, "block_address");
    out << endl;
  }

  if (m_persistentPredictionProfileHisto.size() > 0) {
    out << "Persistent Prediction Profile" << endl;
    out << "-------------" << endl;
    out << endl;
    out << "persistent prediction_histogram: " << m_persistentPredictionProfileHisto << endl;

    out << "persistent prediction_histogram_percent: ";
    m_persistentPredictionProfileHisto.printPercent(out);
    out << endl;

    out << "persistentPrediction_histogram_per_instruction: ";
    m_persistentPredictionProfileHisto.printWithMultiplier(out, 1.0 / double(g_system_ptr->getProfiler()->getTotalInstructionsExecuted()));
    out << endl;

    printSorted(out, m_persistentPredictionProfileMap, "block_address");
    out << endl;
  }
}

void AddressProfiler::clearStats()
{
  // Clear the maps
  m_sharing_miss_counter = 0;
  m_dataAccessTrace->clear();
  m_macroBlockAccessTrace->clear();
  m_programCounterAccessTrace->clear();
  m_retryProfileMap->clear();
  m_retryProfileHisto.clear();
  m_retryProfileHistoRead.clear();
  m_retryProfileHistoWrite.clear();
  m_getx_sharing_histogram.clear();
  m_gets_sharing_histogram.clear();
}

void AddressProfiler::profileGetX(const Address& datablock, const Address& PC, const Set& owner, const Set& sharers, NodeID requestor)
{
  Set indirection_set;
  indirection_set.addSet(sharers);
  indirection_set.addSet(owner);
  indirection_set.remove(requestor);
  int num_indirections = indirection_set.count();
  
  m_getx_sharing_histogram.add(num_indirections);
  bool indirection_miss = (num_indirections > 0);

  addTraceSample(datablock, PC, CacheRequestType_ST, AccessModeType(0), requestor, indirection_miss);
}

void AddressProfiler::profileGetS(const Address& datablock, const Address& PC, const Set& owner, const Set& sharers, NodeID requestor)
{
  Set indirection_set;
  indirection_set.addSet(owner);
  indirection_set.remove(requestor);
  int num_indirections = indirection_set.count();

  m_gets_sharing_histogram.add(num_indirections);
  bool indirection_miss = (num_indirections > 0);

  addTraceSample(datablock, PC, CacheRequestType_LD, AccessModeType(0), requestor, indirection_miss);
}

void AddressProfiler::addTraceSample(Address data_addr, Address pc_addr, CacheRequestType type, AccessModeType access_mode, NodeID id, bool sharing_miss)
{
  if (PROFILE_HOT_LINES) {
    if (sharing_miss) {
      m_sharing_miss_counter++;
    }
    
    // record data address trace info
    data_addr.makeLineAddress();
    lookupTraceForAddress(data_addr, m_dataAccessTrace).update(type, access_mode, id, sharing_miss);
    
    // record macro data address trace info
    Address macro_addr(data_addr.maskLowOrderBits(10)); // 6 for datablock, 4 to make it 16x more coarse
    lookupTraceForAddress(macro_addr, m_macroBlockAccessTrace).update(type, access_mode, id, sharing_miss);
    
    // record program counter address trace info
    lookupTraceForAddress(pc_addr, m_programCounterAccessTrace).update(type, access_mode, id, sharing_miss);
  }
 
  if (PROFILE_ALL_INSTRUCTIONS) {
    // This code is used if the address profiler is an all-instructions profiler
    // record program counter address trace info
    lookupTraceForAddress(pc_addr, m_programCounterAccessTrace).update(type, access_mode, id, sharing_miss);
  }
}

void AddressProfiler::profileRetry(const Address& data_addr, AccessType type, int count)
{
  m_retryProfileHisto.add(count);
  if (type == AccessType_Read) {
    m_retryProfileHistoRead.add(count);
  } else {
    m_retryProfileHistoWrite.add(count);
  }
  if (count > 1) {
    lookupTraceForAddress(data_addr, m_retryProfileMap).addSample(count);
  }
}

void AddressProfiler::profilePersistentPrediction(const Address& data_addr, AccessType type)
{
  m_persistentPredictionProfileHisto.add(1);
  lookupTraceForAddress(data_addr, m_persistentPredictionProfileMap).addSample(1);
}

// ***** Normal Functions ******

static void printSorted(ostream& out, const Map<Address, AccessTraceForAddress>* record_map, string description)
{
  const int records_printed = 100;

  uint64 misses = 0;
  PrioHeap<AccessTraceForAddress*> heap;
  Vector<Address> keys = record_map->keys();
  for(int i=0; i<keys.size(); i++){
    AccessTraceForAddress* record = &(record_map->lookup(keys[i]));
    misses += record->getTotal();
    heap.insert(record);
  }

  out << "Total_entries_" << description << ": " << keys.size() << endl;
  if (PROFILE_ALL_INSTRUCTIONS)
    out << "Total_Instructions_" << description << ": " << misses << endl;
  else
    out << "Total_data_misses_" << description << ": " << misses << endl;

  out << "total | load store atomic | user supervisor | sharing | touched-by" << endl;

  Histogram remaining_records(1, 100);
  Histogram all_records(1, 100);
  Histogram remaining_records_log(-1);
  Histogram all_records_log(-1);

  // Allows us to track how many lines where touched by n processors
  Vector<int64> m_touched_vec;
  Vector<int64> m_touched_weighted_vec;
  m_touched_vec.setSize(RubyConfig::numberOfProcessors()+1);
  m_touched_weighted_vec.setSize(RubyConfig::numberOfProcessors()+1);
  for (int i=0; i<m_touched_vec.size(); i++) {
    m_touched_vec[i] = 0;
    m_touched_weighted_vec[i] = 0;
  }

  int counter = 0;
  while((heap.size() > 0) && (counter < records_printed)) {
    AccessTraceForAddress* record = heap.extractMin();
    double percent = 100.0*(record->getTotal()/double(misses));
    out << description << " | " << percent << " % " << *record << endl;
    all_records.add(record->getTotal());
    all_records_log.add(record->getTotal());
    counter++;
    m_touched_vec[record->getTouchedBy()]++;
    m_touched_weighted_vec[record->getTouchedBy()] += record->getTotal();
  }

  while(heap.size() > 0) {
    AccessTraceForAddress* record = heap.extractMin();
    all_records.add(record->getTotal());
    remaining_records.add(record->getTotal());
    all_records_log.add(record->getTotal());
    remaining_records_log.add(record->getTotal());
    m_touched_vec[record->getTouchedBy()]++;
    m_touched_weighted_vec[record->getTouchedBy()] += record->getTotal();
  }
  out << endl;
  out << "all_records_" << description << ": " << all_records << endl;
  out << "all_records_log_" << description << ": " << all_records_log << endl;
  out << "remaining_records_" << description << ": " << remaining_records << endl;
  out << "remaining_records_log_" << description << ": " << remaining_records_log << endl;
  out << "touched_by_" << description << ": " << m_touched_vec << endl;
  out << "touched_by_weighted_" << description << ": " << m_touched_weighted_vec << endl;
  out << endl;
}

static AccessTraceForAddress& lookupTraceForAddress(const Address& addr, Map<Address, AccessTraceForAddress>* record_map)
{
  if(record_map->exist(addr) == false){
    record_map->add(addr, AccessTraceForAddress(addr));
  }
  return record_map->lookup(addr);
}
