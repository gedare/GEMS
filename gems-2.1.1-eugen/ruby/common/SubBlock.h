
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
 * $Id$
 *
 */

#ifndef SubBlock_H
#define SubBlock_H

#include "Global.h"
#include "Address.h"
#include "RubyConfig.h"
#include "DataBlock.h"
#include "Vector.h"

class SubBlock {
public:
  // Constructors
  SubBlock() { } 
  SubBlock(const Address& addr, int size);
  SubBlock(const Address& addr, const Address& logicalAddress, int size);

  // Destructor
  ~SubBlock() { }
  
  // Public Methods
  const Address& getAddress() const { return m_address; }
  const Address& getLogicalAddress() const { return m_logicalAddress; }
  void setAddress(const Address& addr) { m_address = addr; }
  void setLogicalAddress(const Address& addr) { m_logicalAddress = addr; }

  int getSize() const { return m_data.size(); }
  void setSize(int size) {  m_data.setSize(size); }
  uint8 getByte(int offset) const { return m_data[offset]; }
  void setByte(int offset, uint8 data) { m_data[offset] = data; }

  // Shorthands
  uint8 readByte() const { return getByte(0); }
  void writeByte(uint8 data) { setByte(0, data); }

  // Merging to and from DataBlocks - We only need to worry about
  // updates when we are using DataBlocks
  void mergeTo(DataBlock& data) const { if (DATA_BLOCK) { internalMergeTo(data); } }
  void mergeFrom(const DataBlock& data) { if (DATA_BLOCK) { internalMergeFrom(data); } }

  void print(ostream& out) const;
private:
  // Private Methods
  //  SubBlock(const SubBlock& obj);
  //  SubBlock& operator=(const SubBlock& obj);
  //  bool bytePresent(const Address& addr) { return ((addr.getAddress() >= m_address.getAddress()) && (addr.getAddress() < (m_address.getAddress()+getSize()))); }
  //  uint8 getByte(const Address& addr) { return m_data[addr.getAddress() - m_address.getAddress()]; }

  void internalMergeTo(DataBlock& data) const;
  void internalMergeFrom(const DataBlock& data);

  // Data Members (m_ prefix)
  Address m_address;
  Address m_logicalAddress;
  Vector<uint> m_data;
};

// Output operator declaration
ostream& operator<<(ostream& out, const SubBlock& obj);

// ******************* Definitions *******************

// Output operator definition
extern inline 
ostream& operator<<(ostream& out, const SubBlock& obj)
{
  obj.print(out);
  out << flush;
  return out;
}

#endif //SubBlock_H
