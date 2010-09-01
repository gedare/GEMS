
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
 * MultiBitSelBloomFilter.h
 * 
 * Description: 
 *
 *
 */

#ifndef MULTIBITSEL_BLOOM_FILTER_H
#define MULTIBITSEL_BLOOM_FILTER_H

#include "Map.h"
#include "Global.h"
#include "AbstractChip.h"
#include "System.h"
#include "Profiler.h"
#include "RubyConfig.h"
#include "Address.h"
#include "AbstractBloomFilter.h"

class MultiBitSelBloomFilter : public AbstractBloomFilter {
public:

  ~MultiBitSelBloomFilter();
  MultiBitSelBloomFilter(string config);

  void clear();
  void increment(const Address& addr);
  void decrement(const Address& addr);
  void merge(AbstractBloomFilter * other_filter);
  void set(const Address& addr);
  void unset(const Address& addr);

  bool isSet(const Address& addr);
  int getCount(const Address& addr);
  int getTotalCount();
  void print(ostream& out) const;

  int getIndex(const Address& addr);
  int readBit(const int index);
  void writeBit(const int index, const int value);

  int operator[](const int index) const{
    return this->m_filter[index];
  }

private:

  int get_index(const Address& addr, int hashNumber);

  int hash_bitsel(uint64 value, int index, int jump, int maxBits, int numBits);

  Vector<int> m_filter;
  int m_filter_size;
  int m_num_hashes;
  int m_filter_size_bits;
  int m_skip_bits;
  
  int m_par_filter_size;
  int m_par_filter_size_bits;

  int m_count_bits;
  int m_count;

  bool isParallel;

};

#endif 
