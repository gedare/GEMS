/*
    Copyright (C) 1999-2008 by Mark D. Hill and David A. Wood for the
    Wisconsin Multifacet Project.  Contact: gems@cs.wisc.edu
    http://www.cs.wisc.edu/gems/

    --------------------------------------------------------------------

    This file is part of the Tourmaline Transactional Memory Acclerator, 
    a component of the Multifacet GEMS (General Execution-driven 
    Multiprocessor Simulator) software toolset originally developed at 
    the University of Wisconsin-Madison.

    Tourmaline was originally developed primarily by Dan Gibson, but was
    based on work in the Ruby module performed by Milo Martin and Daniel
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
 * MemoryAccess.h
 *
 * Description: 
 * Wrapper for Simics's v9_memory_transaction_t that is passed to upper 
 * levels of Tourmaline. Analagous to Ruby's CacheMsg object. Contains
 * a void* that can be cast to a v9_memory_transaction_t that may be
 * used by the lower layers. Used void* instead of v9_memory_transaction_t*
 * so that layers that include MemoryAccess.h don't need to include the
 * Simics API headers as well.
 * 
 * $Id: MemoryAccess.h 1.00 05/11/09 14:02:03-05:00 gibson@mustardseed.cs.wisc.edu $
 *
 */

#ifndef MEMORYACCESS_H
#define MEMORYACCESS_H

#include "Tourmaline_Global.h"

enum MemoryAccessType {
  MemoryAccessType_LD = 0,
  MemoryAccessType_ST,
  MemoryAccessType_ATOMIC_R,
  MemoryAccessType_ATOMIC_W
};

enum MemoryAccessOriginType {
  MemoryAccessOriginType_SNOOP_INTERFACE = 0,
  MemoryAccessOriginType_TIMING_INTERFACE
};

class MemoryAccess {
public:
  MemoryAccess( physical_address_t phys_addr,
                MemoryAccessType type,
                unsigned int size,
                logical_address_t pc,
                unsigned int proc,
                MemoryAccessOriginType origin,
                void * p_trans );
  
  ~MemoryAccess();

  // accessors
  const physical_address_t getAddress() const { return m_addr; }
  MemoryAccessType getType() const { return m_type; }
  const unsigned int getSize() const { return m_size; }
  const logical_address_t getProgramCounter() const { return m_pc; }
  const uint64 getId() const { return m_id; }
  const unsigned int getProc() const { return m_proc; }
  MemoryAccessOriginType getOrigin() const { return m_origin; }

  // data-related functions (may invoke Simics API calls)
  uinteger_t getData();
  void setData(uinteger_t newData);
  bool isDataModified() const { return m_data_modified; }
  
  // printing
  void print(ostream& out) const;
  
private:

  physical_address_t     m_addr;
  MemoryAccessType       m_type;
  unsigned int           m_size;
  logical_address_t      m_pc;
  uint64                 m_id;
  unsigned int           m_proc;
  MemoryAccessOriginType m_origin;
  
  bool                   m_data_modified;
  
  // this is void because MemoryAccess objects (and everything
  // that includes this file) doesn't know what a v9_memory_transaction_t
  // is.
  void* m_p_v9trans;
                               
};

// Output operator declarations
ostream& operator<<(ostream& out, const MemoryAccess& obj);
ostream& operator<<(ostream& out, const MemoryAccessType& obj);
ostream& operator<<(ostream& out, const MemoryAccessOriginType& obj);

// Output operator definition for MemoryAccess objects
extern inline
ostream& operator<<(ostream& out, const MemoryAccess& obj)
{
  obj.print(out);
  out << flush;
  return out;
}

#endif // MEMORYACCESS_H
