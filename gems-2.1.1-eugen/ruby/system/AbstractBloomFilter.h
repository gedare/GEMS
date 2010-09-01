
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
 * AbstractBloomFilter.h
 * 
 * Description: 
 *
 *
 */

#ifndef ABSTRACT_BLOOM_FILTER_H
#define ABSTRACT_BLOOM_FILTER_H

#include "Global.h"
#include "AbstractChip.h"
#include "RubyConfig.h"
#include "Address.h"

class AbstractBloomFilter {
public:

  virtual ~AbstractBloomFilter() {}; 
  virtual void clear() = 0;
  virtual void increment(const Address& addr) = 0;
  virtual void decrement(const Address& addr) = 0;
  virtual void merge(AbstractBloomFilter * other_filter) = 0;
  virtual void set(const Address& addr) = 0;
  virtual void unset(const Address& addr) = 0;

  virtual bool isSet(const Address& addr) = 0;
  virtual int getCount(const Address& addr) = 0;
  virtual int getTotalCount() = 0;

  virtual void print(ostream& out) const = 0;
  
  virtual int getIndex(const Address& addr) = 0;
  virtual int readBit(const int index) = 0;
  virtual void writeBit(const int index, const int value) = 0;

private:
 
};


#endif 
