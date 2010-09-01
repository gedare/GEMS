/*
    Copyright (C) 1999-2005 by Mark D. Hill and David A. Wood for the
    Wisconsin Multifacet Project.  Contact: gems@cs.wisc.edu
    http://www.cs.wisc.edu/gems/

    --------------------------------------------------------------------

    This file a component of the Multifacet GEMS (General 
    Execution-driven Multiprocessor Simulator) software toolset 
    originally developed at the University of Wisconsin-Madison.

    Ruby was originally developed primarily by Milo Martin and Daniel
    Sorin with contributions from Ross Dickson, Carl Mauer, and Manoj
    Plakal.
 
    SLICC was originally developed by Milo Martin with substantial
    contributions from Daniel Sorin.

    Opal was originally developed by Carl Mauer based upon code by
    Craig Zilles.

    Substantial further development of Multifacet GEMS at the
    University of Wisconsin was performed by Alaa Alameldeen, Brad
    Beckmann, Jayaram Bobba, Ross Dickson, Dan Gibson, Pacia Harper,
    Derek Hower, Milo Martin, Michael Marty, Carl Mauer, Michelle Moravan,
    Kevin Moore, Manoj Plakal, Daniel Sorin, Haris Volos, Min Xu, and Luke Yen.

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
 *
 */

#ifndef MAP_H
#define MAP_H

#include "Vector.h"

namespace __gnu_cxx {
  template <> struct hash <std::string>
  {
    size_t operator()(const string& s) const { return hash<char*>()(s.c_str()); }
  };
}

typedef unsigned long long uint64;
//hack for uint64 hashes...
namespace __gnu_cxx {
  template <> struct hash <uint64>
  {
    size_t operator()(const uint64 & s) const { return (size_t) s; }
  };
}

template <class KEY_TYPE, class VALUE_TYPE> 
class Map
{
public:
  Map() { /* empty */ }
  ~Map() { /* empty */ }
  
  void add(const KEY_TYPE& key, const VALUE_TYPE& value);
  bool exist(const KEY_TYPE& key) const;
  int size() const { return m_map.size(); }
  void erase(const KEY_TYPE& key) { assert(exist(key)); m_map.erase(key); }
  Vector<KEY_TYPE> keys() const;
  Vector<VALUE_TYPE> values() const;
  void deleteKeys();
  void deleteValues();
  VALUE_TYPE& lookup(const KEY_TYPE& key) const; 
  void clear() { m_map.clear(); }
  void print(ostream& out) const;

  // Synonyms
  void remove(const KEY_TYPE& key) { erase(key); } 
  void deallocate(const KEY_TYPE& key) { erase(key); } 
  void allocate(const KEY_TYPE& key) { add(key, VALUE_TYPE()); }
  void insert(const KEY_TYPE& key, const VALUE_TYPE& value) { add(key, value); }

  // Use default copy constructor and assignment operator
private:
  // Data members

  // m_map is declared mutable because some methods from the STL "map"
  // class that should be const are not.  Thus we define this as
  // mutable so we can still have conceptually const accessors.
  mutable __gnu_cxx::hash_map<KEY_TYPE, VALUE_TYPE> m_map;
};

template <class KEY_TYPE, class VALUE_TYPE> 
ostream& operator<<(ostream& out, const Map<KEY_TYPE, VALUE_TYPE>& map);

// *********************

template <class KEY_TYPE, class VALUE_TYPE> 
void Map<KEY_TYPE, VALUE_TYPE>::add(const KEY_TYPE& key, const VALUE_TYPE& value)
{ 
  // Update or add a new key/value pair
  m_map[key] = value;
}

template <class KEY_TYPE, class VALUE_TYPE> 
bool Map<KEY_TYPE, VALUE_TYPE>::exist(const KEY_TYPE& key) const
{
  return (m_map.count(key) != 0);
}

template <class KEY_TYPE, class VALUE_TYPE> 
VALUE_TYPE& Map<KEY_TYPE, VALUE_TYPE>::lookup(const KEY_TYPE& key) const
{
  assert(exist(key));
  return m_map[key];
}

template <class KEY_TYPE, class VALUE_TYPE> 
Vector<KEY_TYPE> Map<KEY_TYPE, VALUE_TYPE>::keys() const
{
  Vector<KEY_TYPE> keys;
  typename hash_map<KEY_TYPE, VALUE_TYPE>::const_iterator iter;
  for (iter = m_map.begin(); iter != m_map.end(); iter++) {
    keys.insertAtBottom((*iter).first);
  }
  return keys;
}

template <class KEY_TYPE, class VALUE_TYPE> 
Vector<VALUE_TYPE> Map<KEY_TYPE, VALUE_TYPE>::values() const
{
  Vector<VALUE_TYPE> values;
  typename hash_map<KEY_TYPE, VALUE_TYPE>::const_iterator iter;
  pair<KEY_TYPE, VALUE_TYPE> p;
  
  for (iter = m_map.begin(); iter != m_map.end(); iter++) {
    p = *iter;
    values.insertAtBottom(p.second);
  }
  return values;
}

template <class KEY_TYPE, class VALUE_TYPE> 
void Map<KEY_TYPE, VALUE_TYPE>::deleteKeys()
{
  typename hash_map<KEY_TYPE, VALUE_TYPE>::const_iterator iter;
  pair<KEY_TYPE, VALUE_TYPE> p;
  
  for (iter = m_map.begin(); iter != m_map.end(); iter++) {
    p = *iter;
    delete p.first;
  }
}

template <class KEY_TYPE, class VALUE_TYPE> 
void Map<KEY_TYPE, VALUE_TYPE>::deleteValues()
{
  typename hash_map<KEY_TYPE, VALUE_TYPE>::const_iterator iter;
  pair<KEY_TYPE, VALUE_TYPE> p;
  
  for (iter = m_map.begin(); iter != m_map.end(); iter++) {
    p = *iter;
    delete p.second;
  }
}

template <class KEY_TYPE, class VALUE_TYPE> 
void Map<KEY_TYPE, VALUE_TYPE>::print(ostream& out) const
{
  typename hash_map<KEY_TYPE, VALUE_TYPE>::const_iterator iter;
  pair<KEY_TYPE, VALUE_TYPE> p;
  
  out << "[";
  for (iter = m_map.begin(); iter != m_map.end(); iter++) {
    // unparse each basic block
    p = *iter;
    out << " " << p.first << "=" << p.second;
  }
  out << " ]";
}

template <class KEY_TYPE, class VALUE_TYPE> 
ostream& operator<<(ostream& out, const Map<KEY_TYPE, VALUE_TYPE>& map)
{
  map.print(out);
  return out;
}

#endif //MAP_H
