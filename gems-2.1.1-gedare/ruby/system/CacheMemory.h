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
 * CacheMemory.h
 * 
 * Description: 
 *
 * $Id: CacheMemory.h,v 3.7 2004/06/18 20:15:15 beckmann Exp $
 *
 */

#ifndef CACHEMEMORY_H
#define CACHEMEMORY_H

#include "AbstractChip.h"
#include "Global.h"
#include "AccessPermission.h"
#include "Address.h"
#include "CacheRecorder.h"
#include "CacheRequestType.h"
#include "Vector.h"
#include "DataBlock.h"
#include "MachineType.h"
#include "RubySlicc_ComponentMapping.h"
#include "PseudoLRUPolicy.h"
#include "LRUPolicy.h"
#include <vector>
#include "string.h"

template<class ENTRY>
class CacheMemory {
public:

  // Constructors
  CacheMemory(AbstractChip* chip_ptr, int numSetBits, int cacheAssoc, const MachineType machType, const string& description);

  // Destructor
  ~CacheMemory();
  
  // Public Methods
  void printConfig(ostream& out);

  // perform a cache access and see if we hit or not.  Return true on a hit.
  bool tryCacheAccess(const Address& address, CacheRequestType type, DataBlock*& data_ptr);

  // similar to above, but doesn't require full access check
  bool testCacheAccess(const Address& address, CacheRequestType type, DataBlock*& data_ptr);

  // tests to see if an address is present in the cache
  bool isTagPresent(const Address& address) const;

  // Returns true if there is:
  //   a) a tag match on this address or there is 
  //   b) an unused line in the same cache "way"
  bool cacheAvail(const Address& address) const;

  // find an unused entry and sets the tag appropriate for the address
  void allocate(const Address& address);

  // Explicitly free up this address
  void deallocate(const Address& address);

  // Returns with the physical address of the conflicting cache line
  Address cacheProbe(const Address& address) const;

  // looks an address up in the cache
  ENTRY& lookup(const Address& address);
  const ENTRY& lookup(const Address& address) const;

  // Get/Set permission of cache block
  AccessPermission getPermission(const Address& address) const;
  void changePermission(const Address& address, AccessPermission new_perm);

  // Hook for checkpointing the contents of the cache
  void recordCacheContents(CacheRecorder& tr) const;
  void setAsInstructionCache(bool is_icache) { m_is_instruction_cache = is_icache; }

  // Set this address to most recently used
  void setMRU(const Address& address);

  void getMemoryValue(const Address& addr, char* value,
                      unsigned int size_in_bytes );
  void setMemoryValue(const Address& addr, char* value,
                      unsigned int size_in_bytes );

  // Print cache contents
  void print(ostream& out) const;
  void printData(ostream& out) const;

private:
  // Private Methods

  // convert a Address to its location in the cache
  Index addressToCacheSet(const Address& address) const;

  // Given a cache tag: returns the index of the tag in a set.
  // returns -1 if the tag is not found.
  int findTagInSet(Index line, const Address& tag) const;
  int findTagInSetIgnorePermissions(Index cacheSet, const Address& tag) const;

  // Private copy constructor and assignment operator
  CacheMemory(const CacheMemory& obj);
  CacheMemory& operator=(const CacheMemory& obj);
  
  // Data Members (m_prefix)
  AbstractChip* m_chip_ptr;
  MachineType m_machType;
  string m_description;
  bool m_is_instruction_cache;

  // The first index is the # of cache lines.
  // The second index is the the amount associativity.
  Vector<Vector<ENTRY> > m_cache;

  AbstractReplacementPolicy *m_replacementPolicy_ptr;

  int m_cache_num_sets;
  int m_cache_num_set_bits;
  int m_cache_assoc;
};

// Output operator declaration
//ostream& operator<<(ostream& out, const CacheMemory<ENTRY>& obj);

// ******************* Definitions *******************

// Output operator definition
template<class ENTRY>
inline 
ostream& operator<<(ostream& out, const CacheMemory<ENTRY>& obj)
{
  obj.print(out);
  out << flush;
  return out;
}


// ****************************************************************

template<class ENTRY>
inline 
CacheMemory<ENTRY>::CacheMemory(AbstractChip* chip_ptr, int numSetBits, 
                                      int cacheAssoc, const MachineType machType, const string& description)

{
  //cout << "CacheMemory constructor numThreads = " << numThreads << endl;
  m_chip_ptr = chip_ptr;
  m_machType = machType;
  m_description = MachineType_to_string(m_machType)+"_"+description;
  m_cache_num_set_bits = numSetBits;
  m_cache_num_sets = 1 << numSetBits;
  m_cache_assoc = cacheAssoc;
  m_is_instruction_cache = false;

  m_cache.setSize(m_cache_num_sets);
  if(strcmp(g_REPLACEMENT_POLICY, "PSEDUO_LRU") == 0)
    m_replacementPolicy_ptr = new PseudoLRUPolicy(m_cache_num_sets, m_cache_assoc);
  else if(strcmp(g_REPLACEMENT_POLICY, "LRU") == 0)
    m_replacementPolicy_ptr = new LRUPolicy(m_cache_num_sets, m_cache_assoc);
  else
    assert(false);
  for (int i = 0; i < m_cache_num_sets; i++) {
    m_cache[i].setSize(m_cache_assoc);
    for (int j = 0; j < m_cache_assoc; j++) {
      m_cache[i][j].m_Address.setAddress(0);
      m_cache[i][j].m_Permission = AccessPermission_NotPresent;
    }
  }
  

  //  cout << "Before setting trans address list size" << endl;
  //create a trans address for each SMT thread
//   m_trans_address_list.setSize(numThreads);
//   for(int i=0; i < numThreads; ++i){
//     cout << "Setting list size for list " << i << endl;
//     m_trans_address_list[i].setSize(30);
//   }
  //cout << "CacheMemory constructor finished" << endl;
}

template<class ENTRY>
inline 
CacheMemory<ENTRY>::~CacheMemory()
{
  if(m_replacementPolicy_ptr != NULL)
    delete m_replacementPolicy_ptr;
}

template<class ENTRY>
inline 
void CacheMemory<ENTRY>::printConfig(ostream& out)
{
  out << "Cache config: " << m_description << endl;
  out << "  cache_associativity: " << m_cache_assoc << endl;
  out << "  num_cache_sets_bits: " << m_cache_num_set_bits << endl;
  const int cache_num_sets = 1 << m_cache_num_set_bits;
  out << "  num_cache_sets: " << cache_num_sets << endl;
  out << "  cache_set_size_bytes: " << cache_num_sets * RubyConfig::dataBlockBytes() << endl;
  out << "  cache_set_size_Kbytes: " 
      << double(cache_num_sets * RubyConfig::dataBlockBytes()) / (1<<10) << endl;
  out << "  cache_set_size_Mbytes: " 
      << double(cache_num_sets * RubyConfig::dataBlockBytes()) / (1<<20) << endl;
  out << "  cache_size_bytes: " 
      << cache_num_sets * RubyConfig::dataBlockBytes() * m_cache_assoc << endl;
  out << "  cache_size_Kbytes: " 
      << double(cache_num_sets * RubyConfig::dataBlockBytes() * m_cache_assoc) / (1<<10) << endl;
  out << "  cache_size_Mbytes: " 
      << double(cache_num_sets * RubyConfig::dataBlockBytes() * m_cache_assoc) / (1<<20) << endl;
}

// PRIVATE METHODS

// convert a Address to its location in the cache
template<class ENTRY>
inline 
Index CacheMemory<ENTRY>::addressToCacheSet(const Address& address) const
{
  assert(address == line_address(address));
  Index temp = -1;
  switch (m_machType) {
  case MACHINETYPE_L1CACHE_ENUM:
    temp = map_address_to_L1CacheSet(address, m_cache_num_set_bits);
    break;
  case MACHINETYPE_L2CACHE_ENUM:
    temp = map_address_to_L2CacheSet(address, m_cache_num_set_bits);
    break;
  default:
    ERROR_MSG("Don't recognize m_machType");
  }
  assert(temp < m_cache_num_sets);
  assert(temp >= 0);
  return temp;
}

// Given a cache index: returns the index of the tag in a set.
// returns -1 if the tag is not found.
template<class ENTRY>
inline 
int CacheMemory<ENTRY>::findTagInSet(Index cacheSet, const Address& tag) const
{
  assert(tag == line_address(tag));
  // search the set for the tags
  for (int i=0; i < m_cache_assoc; i++) {
    if ((m_cache[cacheSet][i].m_Address == tag) && 
        (m_cache[cacheSet][i].m_Permission != AccessPermission_NotPresent)) {
      return i;
    }
  }
  return -1; // Not found
}

// Given a cache index: returns the index of the tag in a set.
// returns -1 if the tag is not found.
template<class ENTRY>
inline 
int CacheMemory<ENTRY>::findTagInSetIgnorePermissions(Index cacheSet, const Address& tag) const
{
  assert(tag == line_address(tag));
  // search the set for the tags
  for (int i=0; i < m_cache_assoc; i++) {
    if (m_cache[cacheSet][i].m_Address == tag)  
      return i;
  }
  return -1; // Not found
}

// PUBLIC METHODS
template<class ENTRY>
inline 
bool CacheMemory<ENTRY>::tryCacheAccess(const Address& address, 
                                           CacheRequestType type, 
                                           DataBlock*& data_ptr)
{
  assert(address == line_address(address));
  DEBUG_EXPR(CACHE_COMP, HighPrio, address);
  Index cacheSet = addressToCacheSet(address);
  int loc = findTagInSet(cacheSet, address);
  if(loc != -1){ // Do we even have a tag match?
    ENTRY& entry = m_cache[cacheSet][loc];
    m_replacementPolicy_ptr->touch(cacheSet, loc, g_eventQueue_ptr->getTime());
    data_ptr = &(entry.getDataBlk());

    if(entry.m_Permission == AccessPermission_Read_Write) {
      return true;
    } 
    if ((entry.m_Permission == AccessPermission_Read_Only) && 
        (type == CacheRequestType_LD || type == CacheRequestType_IFETCH)) {
      return true;
    }
    // The line must not be accessible
  }
  data_ptr = NULL;
  return false;
}

template<class ENTRY>
inline 
bool CacheMemory<ENTRY>::testCacheAccess(const Address& address, 
                                           CacheRequestType type, 
                                           DataBlock*& data_ptr)
{
  assert(address == line_address(address));
  DEBUG_EXPR(CACHE_COMP, HighPrio, address);
  Index cacheSet = addressToCacheSet(address);
  int loc = findTagInSet(cacheSet, address);
  if(loc != -1){ // Do we even have a tag match?
    ENTRY& entry = m_cache[cacheSet][loc];
    m_replacementPolicy_ptr->touch(cacheSet, loc, g_eventQueue_ptr->getTime());
    data_ptr = &(entry.getDataBlk());

    return (m_cache[cacheSet][loc].m_Permission != AccessPermission_NotPresent);
  }
  data_ptr = NULL;
  return false;
}

// tests to see if an address is present in the cache
template<class ENTRY>
inline 
bool CacheMemory<ENTRY>::isTagPresent(const Address& address) const
{
  assert(address == line_address(address));
  Index cacheSet = addressToCacheSet(address);
  int location = findTagInSet(cacheSet, address);

  if (location == -1) {
    // We didn't find the tag
    DEBUG_EXPR(CACHE_COMP, LowPrio, address);
    DEBUG_MSG(CACHE_COMP, LowPrio, "No tag match");
    return false;
  } 
  DEBUG_EXPR(CACHE_COMP, LowPrio, address);
  DEBUG_MSG(CACHE_COMP, LowPrio, "found");
  return true;
}

// Returns true if there is:
//   a) a tag match on this address or there is 
//   b) an unused line in the same cache "way"
template<class ENTRY>
inline 
bool CacheMemory<ENTRY>::cacheAvail(const Address& address) const
{
  assert(address == line_address(address));

  Index cacheSet = addressToCacheSet(address);

  for (int i=0; i < m_cache_assoc; i++) {
    if (m_cache[cacheSet][i].m_Address == address) {
      // Already in the cache
      return true;
    }      

    if (m_cache[cacheSet][i].m_Permission == AccessPermission_NotPresent) {
      // We found an empty entry
      return true;
    }
  }
  return false;
}

template<class ENTRY>
inline 
void CacheMemory<ENTRY>::allocate(const Address& address) 
{
  assert(address == line_address(address));
  assert(!isTagPresent(address));
  assert(cacheAvail(address));
  DEBUG_EXPR(CACHE_COMP, HighPrio, address);

  // Find the first open slot
  Index cacheSet = addressToCacheSet(address);
  for (int i=0; i < m_cache_assoc; i++) {
    if (m_cache[cacheSet][i].m_Permission == AccessPermission_NotPresent) {
      m_cache[cacheSet][i] = ENTRY();  // Init entry
      m_cache[cacheSet][i].m_Address = address;
      m_cache[cacheSet][i].m_Permission = AccessPermission_Invalid;

      m_replacementPolicy_ptr->touch(cacheSet, i, g_eventQueue_ptr->getTime());

      return;
    }
  }
  ERROR_MSG("Allocate didn't find an available entry");
}

template<class ENTRY>
inline 
void CacheMemory<ENTRY>::deallocate(const Address& address)
{
  assert(address == line_address(address));
  assert(isTagPresent(address));
  DEBUG_EXPR(CACHE_COMP, HighPrio, address);
  lookup(address).m_Permission = AccessPermission_NotPresent;
}

// Returns with the physical address of the conflicting cache line
template<class ENTRY>
inline 
Address CacheMemory<ENTRY>::cacheProbe(const Address& address) const
{
  assert(address == line_address(address));
  assert(!cacheAvail(address));

  Index cacheSet = addressToCacheSet(address);
  return m_cache[cacheSet][m_replacementPolicy_ptr->getVictim(cacheSet)].m_Address;
}

// looks an address up in the cache
template<class ENTRY>
inline 
ENTRY& CacheMemory<ENTRY>::lookup(const Address& address)
{
  assert(address == line_address(address));
  Index cacheSet = addressToCacheSet(address);
  int loc = findTagInSet(cacheSet, address);
  assert(loc != -1);
  return m_cache[cacheSet][loc];
}

// looks an address up in the cache
template<class ENTRY>
inline 
const ENTRY& CacheMemory<ENTRY>::lookup(const Address& address) const
{
  assert(address == line_address(address));
  Index cacheSet = addressToCacheSet(address);
  int loc = findTagInSet(cacheSet, address);
  assert(loc != -1);
  return m_cache[cacheSet][loc];
}

template<class ENTRY>
inline
AccessPermission CacheMemory<ENTRY>::getPermission(const Address& address) const
{
  assert(address == line_address(address));
  return lookup(address).m_Permission;
}

template<class ENTRY>
inline 
void CacheMemory<ENTRY>::changePermission(const Address& address, AccessPermission new_perm)
{
  assert(address == line_address(address));
  lookup(address).m_Permission = new_perm;
  assert(getPermission(address) == new_perm);
}

// Sets the most recently used bit for a cache block
template<class ENTRY>
inline 
void CacheMemory<ENTRY>::setMRU(const Address& address)
{
  Index cacheSet;

  cacheSet = addressToCacheSet(address);
  m_replacementPolicy_ptr->touch(cacheSet, 
                                 findTagInSet(cacheSet, address), 
                                 g_eventQueue_ptr->getTime());
}

template<class ENTRY>
inline 
void CacheMemory<ENTRY>::recordCacheContents(CacheRecorder& tr) const
{
  for (int i = 0; i < m_cache_num_sets; i++) {
    for (int j = 0; j < m_cache_assoc; j++) {
      AccessPermission perm = m_cache[i][j].m_Permission;
      CacheRequestType request_type = CacheRequestType_NULL;
      if (perm == AccessPermission_Read_Only) {
        if (m_is_instruction_cache) {
          request_type = CacheRequestType_IFETCH;
        } else {
          request_type = CacheRequestType_LD;
        }
      } else if (perm == AccessPermission_Read_Write) {
        request_type = CacheRequestType_ST;
      }

      if (request_type != CacheRequestType_NULL) {
        tr.addRecord(m_chip_ptr->getID(), m_cache[i][j].m_Address, 
                     Address(0), request_type, m_replacementPolicy_ptr->getLastAccess(i, j));
      }
    }
  }
}

template<class ENTRY>
inline 
void CacheMemory<ENTRY>::print(ostream& out) const
{ 
  out << "Cache dump: " << m_description << endl;
  for (int i = 0; i < m_cache_num_sets; i++) {
    for (int j = 0; j < m_cache_assoc; j++) {
      out << "  Index: " << i 
          << " way: " << j 
          << " entry: " << m_cache[i][j] << endl;
    }
  }
}

template<class ENTRY>
inline 
void CacheMemory<ENTRY>::printData(ostream& out) const
{ 
  out << "printData() not supported" << endl;
}

template<class ENTRY>
void CacheMemory<ENTRY>::getMemoryValue(const Address& addr, char* value,
                                           unsigned int size_in_bytes ){
  ENTRY entry = lookup(line_address(addr));
  unsigned int startByte = addr.getAddress() - line_address(addr).getAddress();
  for(unsigned int i=0; i<size_in_bytes; ++i){
    value[i] = entry.m_DataBlk.getByte(i + startByte);
  }
}

template<class ENTRY>
void CacheMemory<ENTRY>::setMemoryValue(const Address& addr, char* value,
                                           unsigned int size_in_bytes ){
  ENTRY& entry = lookup(line_address(addr));
  unsigned int startByte = addr.getAddress() - line_address(addr).getAddress();
  assert(size_in_bytes > 0);
  for(unsigned int i=0; i<size_in_bytes; ++i){
    entry.m_DataBlk.setByte(i + startByte, value[i]);
  }

  entry = lookup(line_address(addr));
}

#endif //CACHEMEMORY_H

