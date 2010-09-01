
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
 * MultiGrainBloomFilter.C
 * 
 * Description: 
 *
 *
 */

#include "MultiGrainBloomFilter.h"
#include "Map.h"
#include "Address.h"

MultiGrainBloomFilter::MultiGrainBloomFilter(string str) 
{
  string tail(str);

  // split into the 2 filter sizes
  string head = string_split(tail, '_');

  // head contains size of 1st bloom filter, tail contains size of 2nd bloom filter
  
  m_filter_size = atoi(head.c_str());
  m_filter_size_bits = log_int(m_filter_size);

  m_page_filter_size = atoi(tail.c_str());
  m_page_filter_size_bits = log_int(m_page_filter_size);

  m_filter.setSize(m_filter_size);
  m_page_filter.setSize(m_page_filter_size);
  clear();
}

MultiGrainBloomFilter::~MultiGrainBloomFilter(){
}

void MultiGrainBloomFilter::clear()
{
  for (int i = 0; i < m_filter_size; i++) {
    m_filter[i] = 0;
  }
  for(int i=0; i < m_page_filter_size; ++i){
    m_page_filter[i] = 0;
  }
}

void MultiGrainBloomFilter::increment(const Address& addr)
{
  // Not used
}


void MultiGrainBloomFilter::decrement(const Address& addr)
{
  // Not used
}

void MultiGrainBloomFilter::merge(AbstractBloomFilter * other_filter)
{
  // TODO
}

void MultiGrainBloomFilter::set(const Address& addr)
{
  int i = get_block_index(addr);
  int j = get_page_index(addr);
  assert(i < m_filter_size);
  assert(j < m_page_filter_size);
  m_filter[i] = 1;
  m_page_filter[i] = 1;

}

void MultiGrainBloomFilter::unset(const Address& addr)
{
  // not used
}

bool MultiGrainBloomFilter::isSet(const Address& addr) 
{
  int i = get_block_index(addr);
  int j = get_page_index(addr);
  assert(i < m_filter_size);
  assert(j < m_page_filter_size);
  // we have to have both indices set
  return (m_filter[i] && m_page_filter[i]);
}

int MultiGrainBloomFilter::getCount(const Address& addr)
{
  // not used
  return 0;
}

int MultiGrainBloomFilter::getTotalCount()
{
  int count = 0;

  for (int i = 0; i < m_filter_size; i++) {
    count += m_filter[i];
  }

  for(int i=0; i < m_page_filter_size; ++i){
    count += m_page_filter[i] = 0;
  }

  return count;
}

int MultiGrainBloomFilter::getIndex(const Address& addr)
{
  return 0;
  // TODO
}

int MultiGrainBloomFilter::readBit(const int index) {
  return 0;
  // TODO
}

void MultiGrainBloomFilter::writeBit(const int index, const int value) {
  // TODO
}

void MultiGrainBloomFilter::print(ostream& out) const
{
}

int MultiGrainBloomFilter::get_block_index(const Address& addr) 
{
  // grap a chunk of bits after byte offset
  return addr.bitSelect( RubyConfig::dataBlockBits(), RubyConfig::dataBlockBits() + m_filter_size_bits - 1);
}

int MultiGrainBloomFilter::get_page_index(const Address & addr)
{
  // grap a chunk of bits after first chunk
  return addr.bitSelect( RubyConfig::dataBlockBits() + m_filter_size_bits - 1, 
                         RubyConfig::dataBlockBits() + m_filter_size_bits - 1 + m_page_filter_size_bits  - 1);
}




