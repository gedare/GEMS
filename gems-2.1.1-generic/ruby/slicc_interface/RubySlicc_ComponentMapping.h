
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
 */

#ifndef COMPONENTMAPPINGFNS_H
#define COMPONENTMAPPINGFNS_H

#include "Global.h"
#include "RubyConfig.h"
#include "NodeID.h"
#include "MachineID.h"
#include "Address.h"
#include "Set.h"
#include "NetDest.h"
#include "GenericMachineType.h"

#ifdef MACHINETYPE_L1Cache
#define MACHINETYPE_L1CACHE_ENUM MachineType_L1Cache
#else
#define MACHINETYPE_L1CACHE_ENUM MachineType_NUM
#endif

#ifdef MACHINETYPE_L2Cache
#define MACHINETYPE_L2CACHE_ENUM MachineType_L2Cache
#else
#define MACHINETYPE_L2CACHE_ENUM MachineType_NUM
#endif

#ifdef MACHINETYPE_L3Cache
#define MACHINETYPE_L3CACHE_ENUM MachineType_L3Cache
#else
#define MACHINETYPE_L3CACHE_ENUM MachineType_NUM
#endif

#ifdef MACHINETYPE_PersistentArbiter
#define MACHINETYPE_PERSISTENTARBITER_ENUM MachineType_PersistentArbiter
#else
#define MACHINETYPE_PERSISTENTARBITER_ENUM MachineType_NUM
#endif

#ifdef MACHINETYPE_Collector
#define MACHINETYPE_COLLECTOR_ENUM MachineType_Collector
#else
#define MACHINETYPE_COLLECTOR_ENUM MachineType_NUM
#endif


// used to determine the correct L1 set 
// input parameters are the address and number of set bits for the L1 cache
// returns a value between 0 and the total number of L1 cache sets
inline
int map_address_to_L1CacheSet(const Address& addr, int cache_num_set_bits)  
{
  return addr.bitSelect(RubyConfig::dataBlockBits(), 
                        RubyConfig::dataBlockBits()+cache_num_set_bits-1);
}

// used to determine the correct L2 set 
// input parameters are the address and number of set bits for the L2 cache
// returns a value between 0 and the total number of L2 cache sets
inline
int map_address_to_L2CacheSet(const Address& addr, int cache_num_set_bits)  
{
  assert(cache_num_set_bits == L2_CACHE_NUM_SETS_BITS);  // ensure the l2 bank mapping functions agree with l2 set bits
  
  if (MAP_L2BANKS_TO_LOWEST_BITS) {
    return addr.bitSelect(RubyConfig::dataBlockBits()+RubyConfig::L2CachePerChipBits(),
                          RubyConfig::dataBlockBits()+RubyConfig::L2CachePerChipBits()+cache_num_set_bits-1);
  } else {
    return addr.bitSelect(RubyConfig::dataBlockBits(),
                          RubyConfig::dataBlockBits()+cache_num_set_bits-1);
  }
}

// input parameter is the base ruby node of the L1 cache
// returns a value between 0 and total_L2_Caches_within_the_system
inline
MachineID map_L1CacheMachId_to_L2Cache(const Address& addr, MachineID L1CacheMachId) 
{
  int L2bank = 0;
  MachineID mach = {MACHINETYPE_L2CACHE_ENUM, 0};  

  if (RubyConfig::L2CachePerChipBits() > 0) {
    if (MAP_L2BANKS_TO_LOWEST_BITS) {
      L2bank = addr.bitSelect(RubyConfig::dataBlockBits(),
                         RubyConfig::dataBlockBits()+RubyConfig::L2CachePerChipBits()-1);
    } else {
      L2bank = addr.bitSelect(RubyConfig::dataBlockBits()+L2_CACHE_NUM_SETS_BITS,
                         RubyConfig::dataBlockBits()+L2_CACHE_NUM_SETS_BITS+RubyConfig::L2CachePerChipBits()-1);
    }
  } 

  assert(L2bank < RubyConfig::numberOfL2CachePerChip());
  assert(L2bank >= 0);

  mach.num = RubyConfig::L1CacheNumToL2Base(L1CacheMachId.num)*RubyConfig::numberOfL2CachePerChip() // base #
    + L2bank;  // bank #
  assert(mach.num < RubyConfig::numberOfL2Cache());
  return mach;
}

// used to determine the correct L2 bank
// input parameter is the base ruby node of the L2 cache
// returns a value between 0 and total_L2_Caches_within_the_system
inline
MachineID map_L2ChipId_to_L2Cache(const Address& addr, NodeID L2ChipId) 
{
  assert(L2ChipId < RubyConfig::numberOfChips());

  int L2bank = 0;
  MachineID mach = {MACHINETYPE_L2CACHE_ENUM, 0};

  if (RubyConfig::L2CachePerChipBits() > 0) {
    if (MAP_L2BANKS_TO_LOWEST_BITS) {
      L2bank = addr.bitSelect(RubyConfig::dataBlockBits(),
                         RubyConfig::dataBlockBits()+RubyConfig::L2CachePerChipBits()-1);
    } else {
      L2bank = addr.bitSelect(RubyConfig::dataBlockBits()+L2_CACHE_NUM_SETS_BITS,
                         RubyConfig::dataBlockBits()+L2_CACHE_NUM_SETS_BITS+RubyConfig::L2CachePerChipBits()-1);
    }
  } 

  assert(L2bank < RubyConfig::numberOfL2CachePerChip());
  assert(L2bank >= 0);

  mach.num = L2ChipId*RubyConfig::numberOfL2CachePerChip() // base #
    + L2bank; // bank #
  assert(mach.num < RubyConfig::numberOfL2Cache());
  return mach;
}

// used to determine the home directory
// returns a value between 0 and total_directories_within_the_system
inline
NodeID map_Address_to_DirectoryNode(const Address& addr) 
{
  NodeID dirNode = 0;

  if (RubyConfig::memoryBits() > 0) {
    dirNode = addr.bitSelect(RubyConfig::dataBlockBits(),
                        RubyConfig::dataBlockBits()+RubyConfig::memoryBits()-1);
  }

  //  Index indexHighPortion = address.bitSelect(MEMORY_SIZE_BITS-1, PAGE_SIZE_BITS+NUMBER_OF_MEMORY_MODULE_BITS);
  //  Index indexLowPortion  = address.bitSelect(DATA_BLOCK_BITS, PAGE_SIZE_BITS-1);
    
  //Index index = indexLowPortion | (indexHighPortion << (PAGE_SIZE_BITS - DATA_BLOCK_BITS));

/*

ADDRESS_WIDTH    MEMORY_SIZE_BITS        PAGE_SIZE_BITS  DATA_BLOCK_BITS
  |                    |                       |               |
 \ /                  \ /                     \ /             \ /       0
  -----------------------------------------------------------------------
  |       unused        |xxxxxxxxxxxxxxx|       |xxxxxxxxxxxxxxx|       |
  |                     |xxxxxxxxxxxxxxx|       |xxxxxxxxxxxxxxx|       |
  -----------------------------------------------------------------------
                        indexHighPortion         indexLowPortion
                                        <-------> 
                               NUMBER_OF_MEMORY_MODULE_BITS
  */

  assert(dirNode < RubyConfig::numberOfMemories());
  assert(dirNode >= 0);
  return dirNode;
}

// used to determine the home directory
// returns a value between 0 and total_directories_within_the_system
inline
MachineID map_Address_to_Directory(const Address &addr) 
{
  MachineID mach = {MachineType_Directory, map_Address_to_DirectoryNode(addr)};
  return mach;
}

inline
MachineID map_Address_to_CentralArbiterNode(const Address& addr) 
{
  MachineType t = MACHINETYPE_PERSISTENTARBITER_ENUM;
  MachineID mach = {t, map_Address_to_DirectoryNode(addr)};

  assert(mach.num < RubyConfig::numberOfMemories());
  assert(mach.num >= 0);
  return mach;
}

inline
NetDest getMultiStaticL2BankNetDest(const Address& addr, const Set& sharers)  // set of L2RubyNodes
{
  NetDest dest;
 
  for (int i = 0; i < sharers.getSize(); i++) {
    if (sharers.isElement(i)) {
      dest.add(map_L2ChipId_to_L2Cache(addr,i));
    }
  }
  return dest;
}

inline
NetDest getOtherLocalL1IDs(MachineID L1) 
{
  int start = (L1.num / RubyConfig::numberOfProcsPerChip()) * RubyConfig::numberOfProcsPerChip();
  NetDest ret;

  assert(MACHINETYPE_L1CACHE_ENUM != MachineType_NUM);

  for (int i = start; i < (start + RubyConfig::numberOfProcsPerChip()); i++) {
    if (i != L1.num) {
      MachineID mach = { MACHINETYPE_L1CACHE_ENUM, i };
      ret.add( mach );
    }
  }

  return ret;
}

inline
NetDest getLocalL1IDs(MachineID mach) 
{
  assert(MACHINETYPE_L1CACHE_ENUM != MachineType_NUM);

  NetDest ret;

  if (mach.type == MACHINETYPE_L1CACHE_ENUM) {

    int start = (mach.num / RubyConfig::numberOfL1CachePerChip()) * RubyConfig::numberOfProcsPerChip();

    for (int i = start; i < (start + RubyConfig::numberOfProcsPerChip()); i++) {
      MachineID mach = { MACHINETYPE_L1CACHE_ENUM, i };
      ret.add( mach );
    }
  }
  else if (mach.type == MACHINETYPE_L2CACHE_ENUM) {

    int chip = mach.num/RubyConfig::numberOfL2CachePerChip();
    int start = ( chip*RubyConfig::numberOfL1CachePerChip());
    for (int i = start; i < (start + RubyConfig::numberOfL1CachePerChip()); i++) {
      MachineID mach = { MACHINETYPE_L1CACHE_ENUM, i };
      ret.add( mach );
    }
  }

  return ret;
}

inline
NetDest getExternalL1IDs(MachineID L1) 
{
  NetDest ret;

  assert(MACHINETYPE_L1CACHE_ENUM != MachineType_NUM);

  for (int i = 0; i < RubyConfig::numberOfProcessors(); i++) {
    // ret.add( (NodeID) i);
    MachineID mach = { MACHINETYPE_L1CACHE_ENUM, i };
    ret.add( mach );
  }
  
  ret.removeNetDest(getLocalL1IDs(L1));

  return ret;
}

inline
bool isLocalProcessor(MachineID thisId, MachineID tarID) 
{
  int start = (thisId.num / RubyConfig::numberOfProcsPerChip()) * RubyConfig::numberOfProcsPerChip();

  for (int i = start; i < (start + RubyConfig::numberOfProcsPerChip()); i++) {
    if (i == tarID.num) {
      return true;   
    }
  }
  return false;
}


inline
NetDest getAllPertinentL2Banks(const Address& addr)  // set of L2RubyNodes
{
  NetDest dest;
 
  for (int i = 0; i < RubyConfig::numberOfChips(); i++) {
    dest.add(map_L2ChipId_to_L2Cache(addr,i));
  }
  return dest;
}

inline
bool isL1OnChip(MachineID L1machID, NodeID L2NodeID)
{
  if (L1machID.type == MACHINETYPE_L1CACHE_ENUM) {
    return (L1machID.num == L2NodeID);
  } else {
    return false;
  }
}

inline
bool isL2OnChip(MachineID L2machID, NodeID L2NodeID)
{
  if (L2machID.type == MACHINETYPE_L2CACHE_ENUM) {
    return (L2machID.num == L2NodeID);
  } else {
    return false;
  }
}

inline
NodeID closest_clockwise_distance(NodeID this_node, NodeID next_node)
{
  if (this_node <= next_node) {
    return (next_node - this_node);
  } else {
    return (next_node - this_node + RubyConfig::numberOfChips());
  }
}

inline
bool closer_clockwise_processor(NodeID this_node, NodeID newer, NodeID older)
{
  return (closest_clockwise_distance(this_node, newer) < closest_clockwise_distance(this_node, older));
}

extern inline NodeID getChipID(MachineID L2machID) 
{ 
  return (L2machID.num%RubyConfig::numberOfChips())/RubyConfig::numberOfProcsPerChip(); 
}

extern inline NodeID machineIDToNodeID(MachineID machID) 
{ 
  // return machID.num%RubyConfig::numberOfChips(); 
  return machID.num;
}

extern inline NodeID machineIDToVersion(MachineID machID) 
{ 
  return machID.num/RubyConfig::numberOfChips(); 
}

extern inline MachineType machineIDToMachineType(MachineID machID) 
{ 
  return machID.type; 
}

extern inline NodeID L1CacheMachIDToProcessorNum(MachineID machID)
{
  assert(machID.type == MachineType_L1Cache);
  return machID.num;
}

extern inline NodeID L2CacheMachIDToChipID(MachineID machID)
{
  assert(machID.type == MACHINETYPE_L2CACHE_ENUM);
  return machID.num/RubyConfig::numberOfL2CachePerChip();
}

extern inline MachineID getCollectorDest(MachineID L1MachID)
{
  MachineID mach = {MACHINETYPE_COLLECTOR_ENUM, L1MachID.num};
  return mach;
}

extern inline MachineID getCollectorL1Cache(MachineID colID)
{
  MachineID mach = {MACHINETYPE_L1CACHE_ENUM, colID.num};  
  return mach;
}

extern inline MachineID getL1MachineID(NodeID L1RubyNode)
{
  MachineID mach = {MACHINETYPE_L1CACHE_ENUM, L1RubyNode};
  return mach;
}

extern inline GenericMachineType ConvertMachToGenericMach(MachineType machType) {
  if (machType == MACHINETYPE_L1CACHE_ENUM) {
    return GenericMachineType_L1Cache;
  } else if (machType == MACHINETYPE_L2CACHE_ENUM) {
    return GenericMachineType_L2Cache;
  } else if (machType == MACHINETYPE_L3CACHE_ENUM) {
    return GenericMachineType_L3Cache;
  } else if (machType == MachineType_Directory) {
    return GenericMachineType_Directory;
  } else if (machType == MACHINETYPE_COLLECTOR_ENUM) {
    return GenericMachineType_Collector;
  } else {
    ERROR_MSG("cannot convert to a GenericMachineType");
    return GenericMachineType_NULL;
  }
}


#endif  // COMPONENTMAPPINGFNS_H
