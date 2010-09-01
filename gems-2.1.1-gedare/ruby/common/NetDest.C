
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
 * NetDest.C
 * 
 * Description: See NetDest.h
 *
 * $Id$
 *
 */

#include "NetDest.h"
#include "RubyConfig.h"
#include "Protocol.h"

NetDest::NetDest()  
{ 
  setSize();
}

void NetDest::add(MachineID newElement)
{
  m_bits[vecIndex(newElement)].add(bitIndex(newElement.num));
}

void NetDest::addNetDest(const NetDest& netDest)
{
  assert(m_bits.size() == netDest.getSize());
  for (int i = 0; i < m_bits.size(); i++) {
    m_bits[i].addSet(netDest.m_bits[i]);
  }
}

void NetDest::addRandom()
{
  int i = random()%m_bits.size();
  m_bits[i].addRandom();
}

void NetDest::setNetDest(MachineType machine, const Set& set)
{
  // assure that there is only one set of destinations for this machine
  assert(MachineType_base_level((MachineType)(machine+1)) - MachineType_base_level(machine) == 1);
  m_bits[MachineType_base_level(machine)] = set;
}

void NetDest::remove(MachineID oldElement)
{
  m_bits[vecIndex(oldElement)].remove(bitIndex(oldElement.num));
}

void NetDest::removeNetDest(const NetDest& netDest)
{
  assert(m_bits.size() == netDest.getSize());
  for (int i = 0; i < m_bits.size(); i++) {  
    m_bits[i].removeSet(netDest.m_bits[i]);
    
  }
}

void NetDest::clear()
{
  for (int i = 0; i < m_bits.size(); i++) {   
    m_bits[i].clear();
  }
}

void NetDest::broadcast()
{
  for (MachineType machine = MachineType_FIRST; machine < MachineType_NUM; ++machine) {   
    broadcast(machine);
  }
}

void NetDest::broadcast(MachineType machineType) {

  for (int i = 0; i < MachineType_base_count(machineType); i++) {
    MachineID mach = {machineType, i};
    add(mach); 
  }
}

//For Princeton Network
Vector<NodeID> NetDest::getAllDest() {
	Vector<NodeID> dest;
	dest.clear();
	for (int i=0; i<m_bits.size(); i++) {
		for (int j=0; j<m_bits[i].getSize(); j++) {
			if (m_bits[i].isElement(j)) {
				dest.insertAtBottom((NodeID) (MachineType_base_number((MachineType) i) + j));
			}
		}
	}
	return dest;
}

int NetDest::count() const
{
  int counter = 0;
  for (int i=0; i<m_bits.size(); i++) {
    counter += m_bits[i].count();
  }
  return counter;
}

NodeID NetDest::elementAt(MachineID index) {
  return m_bits[vecIndex(index)].elementAt(bitIndex(index.num));
}

NodeID NetDest::smallestElement() const
{
  assert(count() > 0);
  for (int i=0; i<m_bits.size(); i++) {
    for (int j=0; j<m_bits[i].getSize(); j++) {
      if (m_bits[i].isElement(j)) {
        return j;
      }
    }
  }
  ERROR_MSG("No smallest element of an empty set.");
}

MachineID NetDest::smallestElement(MachineType machine) const
{
  for (int j = 0; j < m_bits[MachineType_base_level(machine)].getSize(); j++) {
    if (m_bits[MachineType_base_level(machine)].isElement(j)) {
      MachineID mach = {machine, j};
      return mach;
    }
  }
  
  ERROR_MSG("No smallest element of given MachineType.");
}


// Returns true iff all bits are set
bool NetDest::isBroadcast() const
{
  for (int i=0; i<m_bits.size(); i++) {
    if (!m_bits[i].isBroadcast()) {
      return false;
    }
  }
  return true;
}

// Returns true iff no bits are set
bool NetDest::isEmpty() const
{
  for (int i=0; i<m_bits.size(); i++) {
    if (!m_bits[i].isEmpty()) {
      return false;
    }
  }
  return true;
}

// returns the logical OR of "this" set and orNetDest 
NetDest NetDest::OR(const NetDest& orNetDest) const
{
  assert(m_bits.size() == orNetDest.getSize());
  NetDest result;
  for (int i=0; i<m_bits.size(); i++) {  
    result.m_bits[i] = m_bits[i].OR(orNetDest.m_bits[i]);
  }
  return result;
}


// returns the logical AND of "this" set and andNetDest 
NetDest NetDest::AND(const NetDest& andNetDest) const
{
  assert(m_bits.size() == andNetDest.getSize());
  NetDest result;
  for (int i=0; i<m_bits.size(); i++) {    
    result.m_bits[i] = m_bits[i].AND(andNetDest.m_bits[i]);
  }
  return result;
}

// Returns true if the intersection of the two sets is non-empty
bool NetDest::intersectionIsNotEmpty(const NetDest& other_netDest) const
{
  assert(m_bits.size() == other_netDest.getSize());
  for (int i=0; i<m_bits.size(); i++) {      
    if (m_bits[i].intersectionIsNotEmpty(other_netDest.m_bits[i])) {
      return true;
    }
  }
  return false;
}

bool NetDest::isSuperset(const NetDest& test) const
{
  assert(m_bits.size() == test.getSize());
  
  for (int i=0; i<m_bits.size(); i++) {        
    if (!m_bits[i].isSuperset(test.m_bits[i])) {
      return false;
    }
  }
  return true;
}

bool NetDest::isElement(MachineID element) const
{
  return ((m_bits[vecIndex(element)])).isElement(bitIndex(element.num));
}

void NetDest::setSize()
{
  m_bits.setSize(MachineType_base_level(MachineType_NUM));
  assert(m_bits.size() == MachineType_NUM);
  
  for (int i = 0; i < m_bits.size(); i++) {
    m_bits[i].setSize(MachineType_base_count((MachineType)i));
  }
}

void NetDest::print(ostream& out) const
{
  out << "[NetDest (" << m_bits.size() << ") ";
  
  for (int i=0; i<m_bits.size(); i++) {
    for (int j=0; j<m_bits[i].getSize(); j++) {
      out << (bool) m_bits[i].isElement(j) << " ";
    }
    out << " - ";
  }
  out << "]";
}

