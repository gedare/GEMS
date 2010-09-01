
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

#include "NonCountingBloomFilter.h"
#include "Map.h"
#include "Address.h"

NonCountingBloomFilter::NonCountingBloomFilter(string str) 
{
  string tail(str);
  string head = string_split(tail, '_');
  
  // head contains filter size, tail contains bit offset from block number
  int smt_threads = RubyConfig::numberofSMTThreads();
  m_filter_size = atoi(head.c_str());
  m_offset = atoi(tail.c_str());
  m_filter_size_bits = log_int(m_filter_size);


  m_filter.setSize(m_filter_size);
  clear();
}

NonCountingBloomFilter::~NonCountingBloomFilter(){
}

void NonCountingBloomFilter::clear()
{
  for (int i = 0; i < m_filter_size; i++) {
    m_filter[i] = 0;
  }
}

void NonCountingBloomFilter::increment(const Address& addr)
{
  // Not used
}


void NonCountingBloomFilter::decrement(const Address& addr)
{
  // Not used
}

void NonCountingBloomFilter::merge(AbstractBloomFilter * other_filter){
  // assumes both filters are the same size!
  NonCountingBloomFilter * temp = (NonCountingBloomFilter*) other_filter;
  for(int i=0; i < m_filter_size; ++i){
    m_filter[i] |= (*temp)[i];
  }
  
}

void NonCountingBloomFilter::set(const Address& addr)
{
  int i = get_index(addr);
  m_filter[i] = 1;
}

void NonCountingBloomFilter::unset(const Address& addr)
{
  int i = get_index(addr);
  m_filter[i] = 0;
}

bool NonCountingBloomFilter::isSet(const Address& addr) 
{
  int i = get_index(addr);
  return (m_filter[i]);
}


int NonCountingBloomFilter::getCount(const Address& addr)
{
  return m_filter[get_index(addr)];
}

int NonCountingBloomFilter::getTotalCount()
{
  int count = 0;

  for (int i = 0; i < m_filter_size; i++) {
    count += m_filter[i];
  }
  return count;
}

void NonCountingBloomFilter::print(ostream& out) const
{
}

int NonCountingBloomFilter::getIndex(const Address& addr) 
{
  return get_index(addr);
}

int NonCountingBloomFilter::readBit(const int index) {
  return m_filter[index];
}

void NonCountingBloomFilter::writeBit(const int index, const int value) {
  m_filter[index] = value;
}

int NonCountingBloomFilter::get_index(const Address& addr) 
{
  return addr.bitSelect( RubyConfig::dataBlockBits() + m_offset, 
                         RubyConfig::dataBlockBits() + m_offset + m_filter_size_bits - 1);
}


