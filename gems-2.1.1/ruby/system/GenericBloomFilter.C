
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
 * GenericBloomFilter.h
 * 
 * Description: 
 *
 *
 */

#include "Global.h"
#include "AbstractChip.h"
#include "RubyConfig.h"
#include "Address.h"

#include "GenericBloomFilter.h"
#include "LSB_CountingBloomFilter.h"
#include "NonCountingBloomFilter.h"
#include "BulkBloomFilter.h"
#include "BlockBloomFilter.h"
#include "MultiGrainBloomFilter.h"
#include "MultiBitSelBloomFilter.h"
#include "H3BloomFilter.h"

GenericBloomFilter::GenericBloomFilter(AbstractChip* chip_ptr, string config) 
{
  m_chip_ptr = chip_ptr;

  
  string tail(config);
  string head = string_split(tail,'_'); 

  if (head == "LSB_Counting" ) {
    m_filter = new LSB_CountingBloomFilter(tail);
  }
  else if(head == "NonCounting" ) {
    m_filter = new NonCountingBloomFilter(tail);
  }
  else if(head == "Bulk" ) {
    m_filter = new BulkBloomFilter(tail);
  }
  else if(head == "Block") {
    m_filter = new BlockBloomFilter(tail);
  }
  else if(head == "Multigrain"){
    m_filter = new MultiGrainBloomFilter(tail);
  }
  else if(head == "MultiBitSel"){
    m_filter = new MultiBitSelBloomFilter(tail);
  }
  else if(head == "H3"){
    m_filter = new H3BloomFilter(tail);
  }
  else {
    assert(0);
  }
}

GenericBloomFilter::~GenericBloomFilter() 
{
  delete m_filter;
}

void GenericBloomFilter::clear()
{
  m_filter->clear();
}

void GenericBloomFilter::increment(const Address& addr)
{
  m_filter->increment(addr);
}

void GenericBloomFilter::decrement(const Address& addr)
{
  m_filter->decrement(addr);
}

void GenericBloomFilter::merge(GenericBloomFilter * other_filter)
{
  m_filter->merge(other_filter->getFilter());
}

void GenericBloomFilter::set(const Address& addr)
{
  m_filter->set(addr);
}

void GenericBloomFilter::unset(const Address& addr)
{
  m_filter->unset(addr);
}

bool GenericBloomFilter::isSet(const Address& addr) 
{
  return m_filter->isSet(addr);
}

int GenericBloomFilter::getCount(const Address& addr)
{
  return m_filter->getCount(addr);
}

int GenericBloomFilter::getTotalCount()
{
  return m_filter->getTotalCount();
}

int GenericBloomFilter::getIndex(const Address& addr)
{
  return m_filter->getIndex(addr);
}

int GenericBloomFilter::readBit(const int index) {
  return m_filter->readBit(index);
}

void GenericBloomFilter::writeBit(const int index, const int value) {
  m_filter->writeBit(index, value);
}

void GenericBloomFilter::print(ostream& out) const
{
  return m_filter->print(out);
}


