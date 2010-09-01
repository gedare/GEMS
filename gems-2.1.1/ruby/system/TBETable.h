
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
 * TBETable.h
 * 
 * Description: 
 *
 * $Id$
 *
 */

#ifndef TBETABLE_H
#define TBETABLE_H

#include "Global.h"
#include "Map.h"
#include "Address.h"
#include "Profiler.h"
#include "AbstractChip.h"
#include "System.h"

template<class ENTRY>
class TBETable {
public:

  // Constructors
  TBETable(AbstractChip* chip_ptr);

  // Destructor
  //~TBETable();
  
  // Public Methods

  static void printConfig(ostream& out) { out << "TBEs_per_TBETable: " << NUMBER_OF_TBES << endl; }

  bool isPresent(const Address& address) const;
  void allocate(const Address& address);
  void deallocate(const Address& address);
  bool areNSlotsAvailable(int n) const { return (NUMBER_OF_TBES - m_map.size()) >= n; }

  ENTRY& lookup(const Address& address);
  const ENTRY& lookup(const Address& address) const;

  // Print cache contents
  void print(ostream& out) const;
private:
  // Private Methods

  // Private copy constructor and assignment operator
  TBETable(const TBETable& obj);
  TBETable& operator=(const TBETable& obj);
  
  // Data Members (m_prefix)
  Map<Address, ENTRY> m_map;
  AbstractChip* m_chip_ptr;
};

// Output operator declaration
//ostream& operator<<(ostream& out, const TBETable<ENTRY>& obj);

// ******************* Definitions *******************

// Output operator definition
template<class ENTRY>
extern inline 
ostream& operator<<(ostream& out, const TBETable<ENTRY>& obj)
{
  obj.print(out);
  out << flush;
  return out;
}


// ****************************************************************

template<class ENTRY>
extern inline 
TBETable<ENTRY>::TBETable(AbstractChip* chip_ptr)
{
  m_chip_ptr = chip_ptr;
}

// PUBLIC METHODS

// tests to see if an address is present in the cache
template<class ENTRY>
extern inline 
bool TBETable<ENTRY>::isPresent(const Address& address) const
{
  assert(address == line_address(address));
  assert(m_map.size() <= NUMBER_OF_TBES);
  return m_map.exist(address);
}

template<class ENTRY>
extern inline 
void TBETable<ENTRY>::allocate(const Address& address) 
{
  assert(isPresent(address) == false);
  assert(m_map.size() < NUMBER_OF_TBES);
  g_system_ptr->getProfiler()->L2tbeUsageSample(m_map.size());
  m_map.add(address, ENTRY());
}

template<class ENTRY>
extern inline 
void TBETable<ENTRY>::deallocate(const Address& address) 
{
  assert(isPresent(address) == true);
  assert(m_map.size() > 0);
  m_map.erase(address);
}

// looks an address up in the cache
template<class ENTRY>
extern inline 
ENTRY& TBETable<ENTRY>::lookup(const Address& address)
{
  assert(isPresent(address) == true);
  return m_map.lookup(address);
}

// looks an address up in the cache
template<class ENTRY>
extern inline 
const ENTRY& TBETable<ENTRY>::lookup(const Address& address) const
{
  assert(isPresent(address) == true);
  return m_map.lookup(address);
}

template<class ENTRY>
extern inline 
void TBETable<ENTRY>::print(ostream& out) const
{ 
}

#endif //TBETABLE_H
