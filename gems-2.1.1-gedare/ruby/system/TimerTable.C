
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
 */

#include "Global.h"
#include "TimerTable.h"
#include "EventQueue.h"

TimerTable::TimerTable(Chip* chip_ptr)
{
  assert(chip_ptr != NULL);
  m_consumer_ptr  = NULL;
  m_chip_ptr = chip_ptr;
  m_next_valid = false;
  m_next_address = Address(0);
  m_next_time = 0;
}


bool TimerTable::isReady() const
{
  if (m_map.size() == 0) {
    return false;
  } 

  if (!m_next_valid) {
    updateNext();
  }
  assert(m_next_valid);
  return (g_eventQueue_ptr->getTime() >= m_next_time);
}

const Address& TimerTable::readyAddress() const
{
  assert(isReady());

  if (!m_next_valid) {
    updateNext();
  }
  assert(m_next_valid);
  return m_next_address;
}

void TimerTable::set(const Address& address, Time relative_latency)
{
  assert(address == line_address(address));
  assert(relative_latency > 0);
  assert(m_map.exist(address) == false);
  Time ready_time = g_eventQueue_ptr->getTime() + relative_latency;
  m_map.add(address, ready_time);
  assert(m_consumer_ptr != NULL);
  g_eventQueue_ptr->scheduleEventAbsolute(m_consumer_ptr, ready_time);
  m_next_valid = false;

  // Don't always recalculate the next ready address
  if (ready_time <= m_next_time) {
    m_next_valid = false;
  }
}

void TimerTable::unset(const Address& address)
{
  assert(address == line_address(address));
  assert(m_map.exist(address) == true);
  m_map.remove(address);

  // Don't always recalculate the next ready address
  if (address == m_next_address) {
    m_next_valid = false;
  }
}

void TimerTable::print(ostream& out) const
{
}


void TimerTable::updateNext() const
{
  if (m_map.size() == 0) {
    assert(m_next_valid == false);
    return;
  }

  Vector<Address> addresses = m_map.keys();
  m_next_address = addresses[0];
  m_next_time = m_map.lookup(m_next_address);

  // Search for the minimum time
  int size = addresses.size();
  for (int i=1; i<size; i++) {
    Address maybe_next_address = addresses[i];
    Time maybe_next_time = m_map.lookup(maybe_next_address);
    if (maybe_next_time < m_next_time) {
      m_next_time = maybe_next_time;
      m_next_address= maybe_next_address;
    }
  }
  m_next_valid = true;
}
