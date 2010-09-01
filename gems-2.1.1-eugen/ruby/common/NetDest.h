
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
 * Set.h
 * 
 * Description: 
 *
 * $Id$
 *
 */

// NetDest specifies the network destination of a NetworkMessage
// This is backward compatible with the Set class that was previously
// used to specify network destinations.
// NetDest supports both node networks and component networks

#ifndef NETDEST_H
#define NETDEST_H

#include "Global.h"
#include "Vector.h"
#include "NodeID.h"
#include "MachineID.h"
#include "RubyConfig.h"
#include "Set.h"
#include "MachineType.h"

class Set;

class NetDest {
public:
  // Constructors
  // creates and empty set
  NetDest();  
  explicit NetDest(int bit_size);  

  NetDest& operator=(const Set& obj);

  // Destructor
  // ~NetDest();
  
  // Public Methods
  void add(MachineID newElement);  
  void addNetDest(const NetDest& netDest); 
  void addRandom();
  void setNetDest(MachineType machine, const Set& set); 
  void remove(MachineID oldElement); 
  void removeNetDest(const NetDest& netDest); 
  void clear(); 
  void broadcast(); 
  void broadcast(MachineType machine);
  int count() const; 
  bool isEqual(const NetDest& netDest);  

  NetDest OR(const NetDest& orNetDest) const;  // return the logical OR of this netDest and orNetDest
  NetDest AND(const NetDest& andNetDest) const;  // return the logical AND of this netDest and andNetDest

  // Returns true if the intersection of the two netDests is non-empty
  bool intersectionIsNotEmpty(const NetDest& other_netDest) const;

  // Returns true if the intersection of the two netDests is empty
  bool intersectionIsEmpty(const NetDest& other_netDest) const;

  bool isSuperset(const NetDest& test) const;
  bool isSubset(const NetDest& test) const { return test.isSuperset(*this); }
  bool isElement(MachineID element) const;
  bool isBroadcast() const; 
  bool isEmpty() const; 

  //For Princeton Network
  Vector<NodeID> getAllDest();

  NodeID smallestElement() const;
  MachineID smallestElement(MachineType machine) const;

  void setSize();  
  int getSize() const { return m_bits.size(); }

  // get element for a index
  NodeID elementAt(MachineID index);

  void print(ostream& out) const;

private:

  // Private Methods
  // returns a value >= MachineType_base_level("this machine") and < MachineType_base_level("next highest machine")
  int vecIndex(MachineID m) const {
    int vec_index = MachineType_base_level(m.type);
    assert(vec_index < m_bits.size());
    return vec_index;
  }
  
  NodeID bitIndex(NodeID index) const {
    return index;
  }
  
  // Data Members (m_ prefix)
  Vector < Set > m_bits;  // a Vector of bit vectors - i.e. Sets

};

// Output operator declaration
ostream& operator<<(ostream& out, const NetDest& obj);

// ******************* Definitions *******************

// Output operator definition
extern inline 
ostream& operator<<(ostream& out, const NetDest& obj)
{
  obj.print(out);
  out << flush;
  return out;
}

#endif //NETDEST_H

