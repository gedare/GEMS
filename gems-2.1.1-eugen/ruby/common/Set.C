
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

#ifdef OPTBIGSET
#include "OptBigSet.C"
#else

#ifdef BIGSET
#include "BigSet.C" // code to supports sets larger than 32
#else

Set::Set()
{ 
  setSize(RubyConfig::numberOfChips());
}

Set::Set(int size)
{ 
  setSize(size);
}

bool Set::isEqual(const Set& set)
{
  return (m_bits == set.m_bits);
}

void Set::add(NodeID index)
{
  assert((m_bits & m_mask) == m_bits);  // check for any bits outside the range
  assert(index < m_size);
  m_bits |= (1 << index);
  assert((m_bits & m_mask) == m_bits);  // check for any bits outside the range
}

void Set::addSet(const Set& set)
{
  assert(m_size == set.m_size);
  m_bits |= set.m_bits;
  assert((m_bits & m_mask) == m_bits);  // check for any bits outside the range
}

void Set::addRandom()
{
  m_bits |= random();
  m_bits &= m_mask;
  assert((m_bits & m_mask) == m_bits);  // check for any bits outside the range
}

void Set::remove(NodeID index)
{
  assert(index < m_size);
  m_bits &= ~(1 << index);
  assert((m_bits & m_mask) == m_bits);  // check for any bits outside the range
}

void Set::removeSet(const Set& set)
{
  assert(m_size == set.m_size);
  m_bits &= ~(set.m_bits);
  assert((m_bits & m_mask) == m_bits);  // check for any bits outside the range
}

void Set::clear()
{
  m_bits = 0;
}

void Set::broadcast()
{
  m_bits = m_mask;
}

int Set::count() const
{
  int counter = 0;
  for (int i=0; i<m_size; i++) {
    if ((m_bits & (1 << i)) != 0) {
      counter++;
    }
  }
  return counter;
}

NodeID Set::elementAt(int index) {
  // count from right to left, index starts from 0
  for (int i=0; i<m_size; i++) {
    if ((m_bits & (1 << i)) != 0) {
      if (index == 0) return i;
      index --;
    }
  }
  assert(0); // index out of range
  return 0;
}

NodeID Set::smallestElement() const
{
  assert(count() > 0);
  int counter = 0;
  for (int i=0; i<m_size; i++) {
    if (isElement(i)) {
      return i;
    }
  }
  ERROR_MSG("No smallest element of an empty set.");
}

// Returns true iff all bits are set
bool Set::isBroadcast() const
{
  assert((m_bits & m_mask) == m_bits);  // check for any bits outside the range
  return (m_mask == m_bits);
}

// Returns true iff no bits are set
bool Set::isEmpty() const
{
  assert((m_bits & m_mask) == m_bits);  // check for any bits outside the range
  return (m_bits == 0);
}

// returns the logical OR of "this" set and orSet 
Set Set::OR(const Set& orSet) const
{
  assert(m_size == orSet.m_size);
  Set result(m_size);
  result.m_bits = (m_bits | orSet.m_bits);
  assert((result.m_bits & result.m_mask) == result.m_bits);  // check for any bits outside the range
  return result;
}

// returns the logical AND of "this" set and andSet 
Set Set::AND(const Set& andSet) const
{
  assert(m_size == andSet.m_size);
  Set result(m_size);
  result.m_bits = (m_bits & andSet.m_bits);
  assert((result.m_bits & result.m_mask) == result.m_bits);  // check for any bits outside the range
  return result;
}

// Returns true if the intersection of the two sets is non-empty
bool Set::intersectionIsNotEmpty(const Set& other_set) const
{
  assert(m_size == other_set.m_size);
  return ((m_bits & other_set.m_bits) != 0);
}

// Returns true if the intersection of the two sets is empty
bool Set::intersectionIsEmpty(const Set& other_set) const
{
  assert(m_size == other_set.m_size);
  return ((m_bits & other_set.m_bits) == 0);
}

bool Set::isSuperset(const Set& test) const
{
  assert(m_size == test.m_size);
  uint32 temp = (test.m_bits & (~m_bits));
  return (temp == 0);
}

bool Set::isElement(NodeID element) const
{
  return ((m_bits & (1 << element)) != 0);
}

void Set::setSize(int size) 
{
  // We're using 32 bit ints, and the 32nd bit acts strangely due to
  // signed/unsigned, so restrict the set size to 31 bits.
  assert(size < 32); 
  m_size = size;
  m_bits = 0;
  m_mask = ~((~0) << m_size);
  assert(m_mask != 0);
  assert((m_bits & m_mask) == m_bits);  // check for any bits outside the range
}

void Set::print(ostream& out) const
{
  out << "[Set (" << m_size << ") ";
  
  for (int i=0; i<m_size; i++) {
    out << (bool) isElement(i) << " ";
  }
  out << "]";
}

#endif // BIGSET

#endif // OPTBIGSET

