
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

#include "AccessTraceForAddress.h"
#include "Histogram.h"

AccessTraceForAddress::AccessTraceForAddress()
{
  m_histogram_ptr = NULL;
}

AccessTraceForAddress::AccessTraceForAddress(const Address& addr)
{
  m_addr = addr;
  m_total = 0;
  m_loads = 0;
  m_stores = 0;
  m_atomics = 0;
  m_user = 0;
  m_sharing = 0;
  m_histogram_ptr = NULL;
}

AccessTraceForAddress::~AccessTraceForAddress()
{
  if (m_histogram_ptr != NULL) {
    delete m_histogram_ptr;
    m_histogram_ptr = NULL;
  }
}

void AccessTraceForAddress::print(ostream& out) const
{
  out << m_addr;

  if (m_histogram_ptr == NULL) {
    out << " " << m_total;
    out << " | " << m_loads;
    out << " " << m_stores;
    out << " " << m_atomics;
    out << " | " << m_user;
    out << " " << m_total-m_user;
    out << " | " << m_sharing;
    out << " | " << m_touched_by.count();
  } else {
    assert(m_total == 0);
    out << " " << (*m_histogram_ptr);
  }
}

void AccessTraceForAddress::update(CacheRequestType type, AccessModeType access_mode, NodeID cpu, bool sharing_miss)
{
  m_touched_by.add(cpu);
  m_total++;
  if(type == CacheRequestType_ATOMIC) {
    m_atomics++;
  } else if(type == CacheRequestType_LD){
    m_loads++;
  } else if (type == CacheRequestType_ST){
    m_stores++;
  } else {
    //  ERROR_MSG("Trying to add invalid access to trace");
  }
  
  if (access_mode == AccessModeType_UserMode) {
    m_user++;
  }

  if (sharing_miss) {
    m_sharing++;
  }
}

int AccessTraceForAddress::getTotal() const
{
  if (m_histogram_ptr == NULL) {
    return m_total;
  } else {
    return m_histogram_ptr->getTotal();
  }
}

void AccessTraceForAddress::addSample(int value)
{
  assert(m_total == 0); 
  if (m_histogram_ptr == NULL) {
    m_histogram_ptr = new Histogram;
  }
  m_histogram_ptr->add(value);
}

bool node_less_then_eq(const AccessTraceForAddress* n1, const AccessTraceForAddress* n2)
{
  return (n1->getTotal() > n2->getTotal());
}
