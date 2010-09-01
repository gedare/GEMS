
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

#include "H3BloomFilter.h"
#include "Map.h"
#include "Address.h"

H3BloomFilter::H3BloomFilter(string str) 
{
  //TODO: change this ugly init code...
  primes_list[0] = 9323;
  primes_list[1] = 11279;
  primes_list[2] = 10247;
  primes_list[3] = 30637;
  primes_list[4] = 25717;
  primes_list[5] = 43711;
  
  mults_list[0] = 255;
  mults_list[1] = 29;
  mults_list[2] = 51;
  mults_list[3] = 3;
  mults_list[4] = 77;
  mults_list[5] = 43;
  
  adds_list[0] = 841;
  adds_list[1] = 627;
  adds_list[2] = 1555;
  adds_list[3] = 241;
  adds_list[4] = 7777;
  adds_list[5] = 65931;
  
  
  
  string tail(str);
  string head = string_split(tail, '_');
  
  // head contains filter size, tail contains bit offset from block number
  m_filter_size = atoi(head.c_str());

  head = string_split(tail, '_');
  m_num_hashes = atoi(head.c_str());

  if(tail == "Regular") {
    isParallel = false;
  } else if (tail == "Parallel") {
    isParallel = true;
  } else {
    cout << "ERROR: Incorrect config string for MultiHash Bloom! :" << str << endl;
    assert(0);
  }

  m_filter_size_bits = log_int(m_filter_size);
  
  m_par_filter_size = m_filter_size/m_num_hashes;
  m_par_filter_size_bits = log_int(m_par_filter_size);

  m_filter.setSize(m_filter_size);
  clear();
}

H3BloomFilter::~H3BloomFilter(){
}

void H3BloomFilter::clear()
{
  for (int i = 0; i < m_filter_size; i++) {
    m_filter[i] = 0;
  }
}

void H3BloomFilter::increment(const Address& addr)
{
  // Not used
}


void H3BloomFilter::decrement(const Address& addr)
{
  // Not used
}

void H3BloomFilter::merge(AbstractBloomFilter * other_filter){
  // assumes both filters are the same size!
  H3BloomFilter * temp = (H3BloomFilter*) other_filter;
  for(int i=0; i < m_filter_size; ++i){
    m_filter[i] |= (*temp)[i];
  }
  
}

void H3BloomFilter::set(const Address& addr)
{
  for (int i = 0; i < m_num_hashes; i++) {
    int idx = get_index(addr, i);
    m_filter[idx] = 1;
    
    //Profile hash value distribution
    g_system_ptr->getProfiler()->getXactProfiler()->profileHashValue(i, idx);
  }
}

void H3BloomFilter::unset(const Address& addr)
{
  cout << "ERROR: Unset should never be called in a Bloom filter";
  assert(0);
}

bool H3BloomFilter::isSet(const Address& addr) 
{
  bool res = true;
  
  for (int i=0; i < m_num_hashes; i++) {
    int idx = get_index(addr, i);
    res = res && m_filter[idx];
  }
  return res;
}


int H3BloomFilter::getCount(const Address& addr)
{
  return isSet(addr)? 1: 0;
}

int H3BloomFilter::getIndex(const Address& addr) 
{
  return 0;
}

int H3BloomFilter::readBit(const int index) {
  return 0;
}

void H3BloomFilter::writeBit(const int index, const int value) {

}

int H3BloomFilter::getTotalCount()
{
  int count = 0;

  for (int i = 0; i < m_filter_size; i++) {
    count += m_filter[i];
  }
  return count;
}

void H3BloomFilter::print(ostream& out) const
{
}

int H3BloomFilter::get_index(const Address& addr, int i) 
{
  uint64 x = addr.getLineAddress();
  //uint64 y = (x*mults_list[i] + adds_list[i]) % primes_list[i];
  int y = hash_H3(x,i);
  
  if(isParallel) {
    return (y % m_par_filter_size) + i*m_par_filter_size;
  } else {
    return y % m_filter_size;
  }
}

int H3BloomFilter::hash_H3(uint64 value, int index) {
  uint64 mask = 1;
  uint64 val = value;
  int result = 0;

  for(int i = 0; i < 64; i++) {
    if(val&mask) result ^= H3[i][index];
    val = val >> 1;
  }
  return result;
  }

