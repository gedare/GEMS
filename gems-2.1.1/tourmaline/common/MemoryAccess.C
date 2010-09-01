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
 * MemoryAccess.C
 *
 * Description: 
 * See MemoryAccess.h
 * 
 * $Id: MemoryAccess.C 1.00 05/11/09 14:02:03-05:00 gibson@mustardseed.cs.wisc.edu $
 *
 */

#include "MemoryAccess.h"
#include "interface.h"

// used to assign unique IDs to all MemoryAccess Objects
unsigned int g_next_memory_access_id = 0;

MemoryAccess::MemoryAccess( physical_address_t phys_addr,
                            MemoryAccessType type,
                            unsigned int size,
                            logical_address_t pc,
                            unsigned int proc,
                            MemoryAccessOriginType origin,
                            void * p_trans ) 
{
  m_addr      = phys_addr;
  m_type      = type;
  m_size      = size;
  m_pc        = pc;
  m_proc      = proc;
  m_origin    = origin;
  m_p_v9trans = p_trans;
  m_id        = g_next_memory_access_id;

  g_next_memory_access_id++;
  m_data_modified = false;
}
  
MemoryAccess::~MemoryAccess() {

}

uinteger_t MemoryAccess::getData() {
  if( m_origin == MemoryAccessOriginType_TIMING_INTERFACE &&
      (m_type   == MemoryAccessType_LD || m_type == MemoryAccessType_ATOMIC_R) ) {
    ERROR( "Data read is not available for MemoryAccess objects originating from the timing interface with type LD or ATOMIC_R.\n" );
  }
  
  assert( m_origin == MemoryAccessOriginType_SNOOP_INTERFACE );
  
  return SIMICS_get_data_from_memtrans(m_p_v9trans);
}

void MemoryAccess::setData(uinteger_t newData) {
  if( m_origin == MemoryAccessOriginType_TIMING_INTERFACE ) {
    ERROR( "Data write is not available for MemoryAccess objects originating from the timing interface.\n" );
  }
  
  assert( m_origin == MemoryAccessOriginType_SNOOP_INTERFACE );

  SIMICS_set_data_from_memtrans(m_p_v9trans,newData);

  m_data_modified = true;
}

void MemoryAccess::print(ostream& out) const
{
  out << "[ MemoryAccess: " << endl;
  out << "  PhysAddr= 0x " << hex << m_addr << dec << endl;
  out << "  Type= " << m_type << endl;
  out << "  Size= " << m_size << endl;
  out << "  Processor= " << m_proc << endl;
  out << "  PC= 0x " << hex << m_pc << dec << endl;
  out << "  Origin= " << m_origin << endl;
  out << "  ID= " << m_id << endl;
  out << "]" << endl;
}

ostream& operator<<(ostream& out, const MemoryAccessType& obj) 
{
  out << "MemoryAccessType_";
        
  switch(obj) {
  case MemoryAccessType_LD:
    out << "LD";
    break;
  case MemoryAccessType_ST:
    out << "ST";
    break;
  case MemoryAccessType_ATOMIC_R:
    out << "ATOMIC_R";
    break;
  case MemoryAccessType_ATOMIC_W:
    out << "ATOMIC_W";
    break;
  default:
    out << "UNKNOWN";
    break;
  }

  return out;
}

ostream& operator<<(ostream& out, const MemoryAccessOriginType& obj) 
{
  out << "MemoryAccessOriginType_";
        
  switch(obj) {
  case MemoryAccessOriginType_SNOOP_INTERFACE:
    out << "SNOOP_INTERFACE";
    break;
  case MemoryAccessOriginType_TIMING_INTERFACE:
    out << "TIMING_INTERFACE";
    break;
  default:
    out << "UNKNOWN";
    break;
  }

  return out;

}

