
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

#include "PersistentArbiter.h"
#include "Address.h"
#include "AbstractChip.h"
#include "util.h"

PersistentArbiter::PersistentArbiter(AbstractChip* chip_ptr)
{
  m_chip_ptr = chip_ptr;

  // wastes entries, but who cares
  m_entries.setSize(RubyConfig::numberOfProcessors());

  for (int i = 0; i < m_entries.size(); i++) {
    m_entries[i].valid = false;
  }

  m_busy = false;
  m_locker = -1;

}

PersistentArbiter::~PersistentArbiter()
{
  m_chip_ptr = NULL;
}  


void PersistentArbiter::addLocker(NodeID id, Address addr, AccessType type) {
  //cout << "Arbiter " << getArbiterId() << " adding locker " << id << " " << addr << endl;
  assert(m_entries[id].valid == false);
  m_entries[id].valid = true; 
  m_entries[id].address = addr; 
  m_entries[id].type = type;
  m_entries[id].localId = id;

}

void PersistentArbiter::removeLocker(NodeID id) {
  //cout << "Arbiter " << getArbiterId() << " removing locker " << id << " " << m_entries[id].address << endl;
  assert(m_entries[id].valid == true);
  m_entries[id].valid = false;

  if (!lockersExist()) {
    m_busy = false;
  }
}

bool PersistentArbiter::successorRequestPresent(Address addr, NodeID id) {
  for (int i = (id + 1); i < m_entries.size(); i++) {
    if (m_entries[i].address == addr && m_entries[i].valid) {
      //cout << "m_entries[" << id << ", address " << m_entries[id].address << " is equal to " << addr << endl;
      return true;
    }
  }
  return false;
}

bool PersistentArbiter::lockersExist() {
  for (int i = 0; i < m_entries.size(); i++) {
    if (m_entries[i].valid == true) {
      return true;
    }
  }
  //cout << "no lockers found" << endl;
  return false;
}

void PersistentArbiter::advanceActiveLock() {
  assert(lockersExist());

  //cout << "arbiter advancing lock from " << m_locker;
  m_busy = false;

  if (m_locker < (m_entries.size() - 1)) {
    for (int i = (m_locker+1); i < m_entries.size(); i++) {
      if (m_entries[i].valid == true) {
        m_locker = i;
        m_busy = true;
        //cout << " to " << m_locker << endl;
        return;
      }  
    }
  }

  if (!m_busy) {
    for (int i = 0; i < m_entries.size(); i++) {
      if (m_entries[i].valid == true) {
        m_locker = i;
        m_busy = true;
        //cout << " to " << m_locker << endl;
        return;
      }    
    }

    assert(m_busy)
  }
}

Address PersistentArbiter::getActiveLockAddress() {
  assert( m_entries[m_locker].valid = true );
  return m_entries[m_locker].address;
}


NodeID PersistentArbiter::getArbiterId() {
  return m_chip_ptr->getID()*RubyConfig::numberOfProcsPerChip();
}

bool PersistentArbiter::isBusy() {
  return m_busy;
}

NodeID PersistentArbiter::getActiveLocalId() {
  assert( m_entries[m_locker].valid = true );
  return m_entries[m_locker].localId;
}

void PersistentArbiter::setIssuedAddress(Address addr) {
  m_issued_address = addr;
}

bool PersistentArbiter::isIssuedAddress(Address addr) {
  return (m_issued_address == addr);
}

void PersistentArbiter::print(ostream& out) const {

  out << "[";
  for (int i = 0; i < m_entries.size(); i++) {
    if (m_entries[i].valid == true) {
      out << "( " << m_entries[i].localId  << ", " << m_entries[i].address << ") ";
    }
  }
  out << "]" << endl;

}
