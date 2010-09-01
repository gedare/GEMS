
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
 * NonCountingBloomFilter.C
 * 
 * Description: 
 *
 *
 */

#include "MultiBitSelBloomFilter.h"
#include "Map.h"
#include "Address.h"

MultiBitSelBloomFilter::MultiBitSelBloomFilter(string str) 
{
  
  string tail(str);
  string head = string_split(tail, '_');
  
  // head contains filter size, tail contains bit offset from block number
  m_filter_size = atoi(head.c_str());

  head = string_split(tail, '_');
  m_num_hashes = atoi(head.c_str());

  head = string_split(tail, '_');
  m_skip_bits = atoi(head.c_str());

  if(tail == "Regular") {
    isParallel = false;
  } else if (tail == "Parallel") {
    isParallel = true;
  } else {
    cout << "ERROR: Incorrect config string for MultiBitSel Bloom! :" << str << endl;
    assert(0);
  }

  m_filter_size_bits = log_int(m_filter_size);
  
  m_par_filter_size = m_filter_size/m_num_hashes;
  m_par_filter_size_bits = log_int(m_par_filter_size);

  m_filter.setSize(m_filter_size);
  clear();
}

MultiBitSelBloomFilter::~MultiBitSelBloomFilter(){
}

void MultiBitSelBloomFilter::clear()
{
  for (int i = 0; i < m_filter_size; i++) {
    m_filter[i] = 0;
  }
}

void MultiBitSelBloomFilter::increment(const Address& addr)
{
  // Not used
}


void MultiBitSelBloomFilter::decrement(const Address& addr)
{
  // Not used
}

void MultiBitSelBloomFilter::merge(AbstractBloomFilter * other_filter){
  // assumes both filters are the same size!
  MultiBitSelBloomFilter * temp = (MultiBitSelBloomFilter*) other_filter;
  for(int i=0; i < m_filter_size; ++i){
    m_filter[i] |= (*temp)[i];
  }
  
}

void MultiBitSelBloomFilter::set(const Address& addr)
{
  for (int i = 0; i < m_num_hashes; i++) {
    int idx = get_index(addr, i);
    m_filter[idx] = 1;
    
    //Profile hash value distribution
    g_system_ptr->getProfiler()->getXactProfiler()->profileHashValue(i, idx);
  }
}

void MultiBitSelBloomFilter::unset(const Address& addr)
{
  cout << "ERROR: Unset should never be called in a Bloom filter";
  assert(0);
}

bool MultiBitSelBloomFilter::isSet(const Address& addr) 
{
  bool res = true;
  
  for (int i=0; i < m_num_hashes; i++) {
    int idx = get_index(addr, i);
    res = res && m_filter[idx];
  }
  return res;
}


int MultiBitSelBloomFilter::getCount(const Address& addr)
{
  return isSet(addr)? 1: 0;
}

int MultiBitSelBloomFilter::getIndex(const Address& addr) 
{
  return 0;
}

int MultiBitSelBloomFilter::readBit(const int index) {
  return 0;
}

void MultiBitSelBloomFilter::writeBit(const int index, const int value) {

}

int MultiBitSelBloomFilter::getTotalCount()
{
  int count = 0;

  for (int i = 0; i < m_filter_size; i++) {
    count += m_filter[i];
  }
  return count;
}

void MultiBitSelBloomFilter::print(ostream& out) const
{
}

int MultiBitSelBloomFilter::get_index(const Address& addr, int i) 
{
  // m_skip_bits is used to perform BitSelect after skipping some bits. Used to simulate BitSel hashing on larger than cache-line granularities
  uint64 x = (addr.getLineAddress()) >> m_skip_bits;
  int y = hash_bitsel(x, i, m_num_hashes, 30, m_filter_size_bits);
  //36-bit addresses, 6-bit cache lines
  
  if(isParallel) {
    return (y % m_par_filter_size) + i*m_par_filter_size;
  } else {
    return y % m_filter_size;
  }
}


int MultiBitSelBloomFilter::hash_bitsel(uint64 value, int index, int jump, int maxBits, int numBits) {
  uint64 mask = 1;
  int result = 0;
  int bit, i;

  for(i = 0; i < numBits; i++) {
    bit = (index + jump*i) % maxBits;
    if (value & (mask << bit)) result += mask << i;
  }
  return result;
}
