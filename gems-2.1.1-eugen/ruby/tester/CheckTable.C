
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

#include "CheckTable.h"
#include "Check.h"
#include "Map.h"

CheckTable::CheckTable()
{
  m_lookup_map_ptr = new Map<Address, Check*>;
  physical_address_t physical = 0;
  Address address;

  const int size1 = 32;
  const int size2 = 100;

  // The first set is to get some false sharing
  physical = 1000;
  for (int i=0; i<size1; i++) {
    // Setup linear addresses
    address.setAddress(physical);
    addCheck(address);
    physical += CHECK_SIZE;
  }

  // The next two sets are to get some limited false sharing and cache conflicts
  physical = 1000;
  for (int i=0; i<size2; i++) {
    // Setup linear addresses
    address.setAddress(physical);
    addCheck(address);
    physical += 256;
  }

  physical = 1000 + CHECK_SIZE;
  for (int i=0; i<size2; i++) {
    // Setup linear addresses
    address.setAddress(physical);
    addCheck(address);
    physical += 256;
  }
}

CheckTable::~CheckTable()
{
  int size = m_check_vector.size();
  for (int i=0; i<size; i++) {
    delete m_check_vector[i];
  }
  delete m_lookup_map_ptr;
}

void CheckTable::addCheck(const Address& address)
{
  if (log_int(CHECK_SIZE) != 0) {
    if (address.bitSelect(0,CHECK_SIZE_BITS-1) != 0) {
      ERROR_MSG("Check not aligned");
    }
  }

  for (int i=0; i<CHECK_SIZE; i++) {
    if (m_lookup_map_ptr->exist(Address(address.getAddress()+i))) {
      // A mapping for this byte already existed, discard the entire check
      return;
    }
  }

  Check* check_ptr = new Check(address, Address(100+m_check_vector.size()));
  for (int i=0; i<CHECK_SIZE; i++) {
    // Insert it once per byte
    m_lookup_map_ptr->add(Address(address.getAddress()+i), check_ptr);
  }
  m_check_vector.insertAtBottom(check_ptr);
}

Check* CheckTable::getRandomCheck()
{
  return m_check_vector[random() % m_check_vector.size()];
}

Check* CheckTable::getCheck(const Address& address)
{
  DEBUG_MSG(TESTER_COMP, MedPrio, "Looking for check by address");
  DEBUG_EXPR(TESTER_COMP, MedPrio, address);

  if (m_lookup_map_ptr->exist(address)) {
    Check* check = m_lookup_map_ptr->lookup(address);
    assert(check != NULL);
    return check;
  } else {
    return NULL;
  }
}

void CheckTable::print(ostream& out) const
{
}
