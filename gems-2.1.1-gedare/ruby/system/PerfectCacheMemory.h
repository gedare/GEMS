
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
 * PerfectCacheMemory.h
 * 
 * Description: 
 *
 * $Id$
 *
 */

#ifndef PERFECTCACHEMEMORY_H
#define PERFECTCACHEMEMORY_H

#include "Global.h"
#include "Map.h"
#include "AccessPermission.h"
#include "RubyConfig.h"
#include "Address.h"
#include "interface.h"
#include "AbstractChip.h"

template<class ENTRY>
class PerfectCacheLineState {
public:
  PerfectCacheLineState() { m_permission = AccessPermission_NUM; }
  AccessPermission m_permission;
  ENTRY m_entry;
};

template<class ENTRY>
class PerfectCacheMemory {
public:

  // Constructors
  PerfectCacheMemory(AbstractChip* chip_ptr);

  // Destructor
  //~PerfectCacheMemory();
  
  // Public Methods

  static void printConfig(ostream& out);

  // perform a cache access and see if we hit or not.  Return true on
  // a hit.
  bool tryCacheAccess(const CacheMsg& msg, bool& block_stc, ENTRY*& entry);

  // tests to see if an address is present in the cache
  bool isTagPresent(const Address& address) const;

  // Returns true if there is:
  //   a) a tag match on this address or there is 
  //   b) an Invalid line in the same cache "way"
  bool cacheAvail(const Address& address) const;

  // find an Invalid entry and sets the tag appropriate for the address
  void allocate(const Address& address);

  void deallocate(const Address& address);

  // Returns with the physical address of the conflicting cache line
  Address cacheProbe(const Address& newAddress) const;

  // looks an address up in the cache
  ENTRY& lookup(const Address& address);
  const ENTRY& lookup(const Address& address) const;

  // Get/Set permission of cache block
  AccessPermission getPermission(const Address& address) const;
  void changePermission(const Address& address, AccessPermission new_perm);

  // Print cache contents
  void print(ostream& out) const;
private:
  // Private Methods

  // Private copy constructor and assignment operator
  PerfectCacheMemory(const PerfectCacheMemory& obj);
  PerfectCacheMemory& operator=(const PerfectCacheMemory& obj);
  
  // Data Members (m_prefix)
  Map<Address, PerfectCacheLineState<ENTRY> > m_map;
  AbstractChip* m_chip_ptr;
};

// Output operator declaration
//ostream& operator<<(ostream& out, const PerfectCacheMemory<ENTRY>& obj);

// ******************* Definitions *******************

// Output operator definition
template<class ENTRY>
extern inline 
ostream& operator<<(ostream& out, const PerfectCacheMemory<ENTRY>& obj)
{
  obj.print(out);
  out << flush;
  return out;
}


// ****************************************************************

template<class ENTRY>
extern inline 
PerfectCacheMemory<ENTRY>::PerfectCacheMemory(AbstractChip* chip_ptr)
{
  m_chip_ptr = chip_ptr;
}

// STATIC METHODS

template<class ENTRY>
extern inline 
void PerfectCacheMemory<ENTRY>::printConfig(ostream& out)
{
}

// PUBLIC METHODS

template<class ENTRY>
extern inline 
bool PerfectCacheMemory<ENTRY>::tryCacheAccess(const CacheMsg& msg, bool& block_stc, ENTRY*& entry)
{
  ERROR_MSG("not implemented");
}

// tests to see if an address is present in the cache
template<class ENTRY>
extern inline 
bool PerfectCacheMemory<ENTRY>::isTagPresent(const Address& address) const
{
  return m_map.exist(line_address(address));
}

template<class ENTRY>
extern inline 
bool PerfectCacheMemory<ENTRY>::cacheAvail(const Address& address) const
{
  return true;
}

// find an Invalid or already allocated entry and sets the tag
// appropriate for the address
template<class ENTRY>
extern inline 
void PerfectCacheMemory<ENTRY>::allocate(const Address& address) 
{
  PerfectCacheLineState<ENTRY> line_state;
  line_state.m_permission = AccessPermission_Busy;
  line_state.m_entry = ENTRY();
  m_map.add(line_address(address), line_state);
}

// deallocate entry
template<class ENTRY>
extern inline
void PerfectCacheMemory<ENTRY>::deallocate(const Address& address)
{
  m_map.erase(line_address(address));
}

// Returns with the physical address of the conflicting cache line
template<class ENTRY>
extern inline 
Address PerfectCacheMemory<ENTRY>::cacheProbe(const Address& newAddress) const
{
  ERROR_MSG("cacheProbe called in perfect cache");
}

// looks an address up in the cache
template<class ENTRY>
extern inline 
ENTRY& PerfectCacheMemory<ENTRY>::lookup(const Address& address)
{
  return m_map.lookup(line_address(address)).m_entry;
}

// looks an address up in the cache
template<class ENTRY>
extern inline 
const ENTRY& PerfectCacheMemory<ENTRY>::lookup(const Address& address) const
{
  return m_map.lookup(line_address(address)).m_entry;
}

template<class ENTRY>
extern inline
AccessPermission PerfectCacheMemory<ENTRY>::getPermission(const Address& address) const
{
  return m_map.lookup(line_address(address)).m_permission;
}

template<class ENTRY>
extern inline 
void PerfectCacheMemory<ENTRY>::changePermission(const Address& address, AccessPermission new_perm)
{
  Address line_address = address;
  line_address.makeLineAddress();
  PerfectCacheLineState<ENTRY>& line_state = m_map.lookup(line_address);
  AccessPermission old_perm = line_state.m_permission;
  line_state.m_permission = new_perm;
}

template<class ENTRY>
extern inline 
void PerfectCacheMemory<ENTRY>::print(ostream& out) const
{ 
}

#endif //PERFECTCACHEMEMORY_H
