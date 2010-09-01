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

#include "PartialAddressFilter.h"
#include "AbstractChip.h"

PartialAddressFilter::PartialAddressFilter(AbstractChip* chip, int bits)
{
  m_bits = bits;
  m_size = (1 << m_bits);
  m_readBit = new bool[m_size];
  m_writeBit = new bool[m_size];
  m_chip_ptr = chip;
  clear();
}

PartialAddressFilter::~PartialAddressFilter(){
  delete m_readBit;
  delete m_writeBit;
}

void PartialAddressFilter::printConfig(ostream& out){
  out << "PartialAddressFilter-" << m_bits << "bits" << endl;
}

bool PartialAddressFilter::isRead(Address a){
  return m_readBit[calcIndex(a)];
}

bool PartialAddressFilter::isWrite(Address a){
  return m_writeBit[calcIndex(a)];
}

void PartialAddressFilter::clear(){
  for (int i=0; i<m_size; ++i){
    m_readBit[i] = false;
    m_writeBit[i] = false;
  }
}

void PartialAddressFilter::addEntry(Address a, bool dirty){
  int index = calcIndex(a);
  m_readBit[index] = true;
  if (dirty){
    m_writeBit[index] = true;
  }
  cout << "  " << m_chip_ptr->getID() << " adding entry: " << a << " (" 
       << m_readBit[index] << ", " << m_writeBit[index]
       << ")." << endl;
}

int PartialAddressFilter::calcIndex(Address a){
  int offsetBits = 6;
  
  int index = (int) a.bitSelect(offsetBits, m_bits + offsetBits); 
  return 0;
}

