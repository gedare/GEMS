
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

// NOTE: Never include this file directly, this should only be
// included from Set.h

#ifndef SET_H
#define SET_H

#include "Global.h"
#include "Vector.h"
#include "NodeID.h"
#include "RubyConfig.h"

enum PresenceBit {NotPresent, Present};

class Set {
public:
  // Constructors
  // creates and empty set
  Set();
  Set (int size);

  // used during the replay mechanism
  //  Set(const char *str);

  //  Set(const Set& obj);
  //  Set& operator=(const Set& obj);

  // Destructor
  // ~Set();
  
  // Public Methods

  void add(NodeID newElement);
  void addSet(const Set& set);
  void addRandom();
  void remove(NodeID newElement);
  void removeSet(const Set& set);
  void clear();
  void broadcast();
  int count() const;
  bool isEqual(const Set& set) const;

  Set OR(const Set& orSet) const;  // return the logical OR of this set and orSet
  Set AND(const Set& andSet) const;  // return the logical AND of this set and andSet

  // Returns true if the intersection of the two sets is non-empty
  bool intersectionIsNotEmpty(const Set& other_set) const;

  // Returns true if the intersection of the two sets is empty
  bool intersectionIsEmpty(const Set& other_set) const;

  bool isSuperset(const Set& test) const;
  bool isSubset(const Set& test) const { return test.isSuperset(*this); }
  bool isElement(NodeID element) const;
  bool isBroadcast() const;
  bool isEmpty() const;

  NodeID smallestElement() const;

  //  int size() const;
  void setSize (int size);

  // get element for a index
  NodeID elementAt(int index) const;
  int getSize() const { return m_bits.size(); }

  // DEPRECATED METHODS
  void addToSet(NodeID newElement) { add(newElement); }  // Deprecated
  void removeFromSet(NodeID newElement) { remove(newElement); }  // Deprecated
  void clearSet() { clear(); }   // Deprecated
  void setBroadcast() { broadcast(); }   // Deprecated
  bool presentInSet(NodeID element) const { return isElement(element); }  // Deprecated

  void print(ostream& out) const;
private:
  // Private Methods

  // Data Members (m_ prefix)
  Vector<uint8> m_bits;  // This is an vector of uint8 to reduce the size of the set
};

// Output operator declaration
ostream& operator<<(ostream& out, const Set& obj);

// ******************* Definitions *******************

// Output operator definition
extern inline 
ostream& operator<<(ostream& out, const Set& obj)
{
  obj.print(out);
  out << flush;
  return out;
}

#endif //SET_H

