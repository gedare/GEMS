
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
 * Set.C
 * 
 * Description: See Set.h
 *
 * $Id$
 *
 */

#include "Set.h"
#include "RubyConfig.h"

Set::Set()
{ 
  setSize(RubyConfig::numberOfProcessors());
}

Set::Set(int size)
{ 
  setSize(size);
}

void Set::add(NodeID index)
{
  m_bits[index] = Present;
}

void Set::addSet(const Set& set)
{
  assert(m_bits.size() == set.getSize());
  for (int i=0; i<m_bits.size(); i++) {
    if(set.isElement(i)){
      add(i);
    }
  }
}

void Set::addRandom()
{
  int rand = random();
  for (int i=0; i<m_bits.size(); i++) {
    if(rand & 0x1 == 0) {  // Look at the low order bit
      add(i);
    }
    rand = (rand >> 1);  // Shift the random number to look at the next bit
  }
}

void Set::remove(NodeID index)
{
  m_bits[index] = NotPresent;
}

void Set::removeSet(const Set& set)
{
  assert(m_bits.size() == set.getSize());
  for (int i=0; i<m_bits.size(); i++) {
    if(set.isElement(i)){
      remove(i);
    }
  }
}

void Set::clear()
{
  for (int i=0; i<m_bits.size(); i++) {
    m_bits[i] = NotPresent;
  }
}

void Set::broadcast()
{
  for (int i=0; i<m_bits.size(); i++) {
    m_bits[i] = Present;
  }
}

int Set::count() const
{
  int counter = 0;
  for (int i=0; i<m_bits.size(); i++) {
    if (m_bits[i] == Present) {
      counter++;
    }
  }
  return counter;
}

bool Set::isEqual(const Set& set) const
{
  assert(m_bits.size() == set.getSize());
  for (int i=0; i<m_bits.size(); i++) {
    if (m_bits[i] != set.isElement(i)) {
      return false;
    }
  }
  return true;
}

NodeID Set::smallestElement() const
{
  assert(count() > 0);
  for (int i=0; i<m_bits.size(); i++) {
    if (isElement(i)) {
      return i;
    }
  }
  ERROR_MSG("No smallest element of an empty set.");
}

// Returns true iff all bits are set
bool Set::isBroadcast() const
{
  for (int i=0; i<m_bits.size(); i++) {
    if (m_bits[i] == NotPresent) {
      return false;
    }
  }
  return true;
}

// Returns true iff no bits are set
bool Set::isEmpty() const
{
  for (int i=0; i<m_bits.size(); i++) {
    if (m_bits[i] == Present) {
      return false;
    }
  }
  return true;
}

// returns the logical OR of "this" set and orSet 
Set Set::OR(const Set& orSet) const
{
  Set result;
  assert(m_bits.size() == orSet.getSize());
  result.setSize(m_bits.size());
  for (int i=0; i<m_bits.size(); i++) {
    if(m_bits[i] == Present || orSet.isElement(i)){
      result.add(i);
    }else{
      result.remove(i);
    }
  }

  return result;

}

// returns the logical AND of "this" set and andSet 
Set Set::AND(const Set& andSet) const
{
  Set result;
  assert(m_bits.size() == andSet.getSize());
  result.setSize(m_bits.size());
  for (int i=0; i<m_bits.size(); i++) {
    if(m_bits[i] == Present && andSet.isElement(i)){
      result.add(i);
    }else{
      result.remove(i);
    }
  }
  return result;
}

// Returns true if the intersection of the two sets is non-empty
bool Set::intersectionIsNotEmpty(const Set& other_set) const
{
  assert(m_bits.size() == other_set.getSize());
  for(int index=0; index < m_bits.size(); index++){
    if(other_set.isElement(index) && isElement(index)) {
      return true;
    }
  }
  return false;
}

// Returns true if the intersection of the two sets is non-empty
bool Set::intersectionIsEmpty(const Set& other_set) const
{
  assert(m_bits.size() == other_set.getSize());
  for(int index=0; index < m_bits.size(); index++){
    if(other_set.isElement(index) && isElement(index)) {
      return false;
    }
  }
  return true;
}

bool Set::isSuperset(const Set& test) const
{
  assert(m_bits.size() == test.getSize());
  for(int index=0; index < m_bits.size(); index++){
    if(test.isElement(index) && !isElement(index)) {
      return false;
    }
  }
  return true;
}

bool Set::isElement(NodeID element) const
{
  return (m_bits[element] == Present);
}

NodeID Set::elementAt(int index) const
{ 
  if (m_bits[index] == Present) {
    return m_bits[index] == Present; 
  } else {
    return 0;
  }
}

void Set::setSize(int size) 
{
  m_bits.setSize(size);
  clear();
}

void Set::print(ostream& out) const
{
  out << "[Set ";
  for (int i=0; i<m_bits.size(); i++) {
    out << (bool)m_bits[i] << " ";
  }
  out << "]";
}
