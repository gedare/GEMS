
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

#include "StoreTrace.h"
#include "EventQueue.h"

bool StoreTrace::s_init = false; // Total number of store lifetimes of all lines
int64 StoreTrace::s_total_samples = 0; // Total number of store lifetimes of all lines
Histogram* StoreTrace::s_store_count_ptr = NULL;
Histogram* StoreTrace::s_store_first_to_stolen_ptr = NULL;
Histogram* StoreTrace::s_store_last_to_stolen_ptr = NULL;
Histogram* StoreTrace::s_store_first_to_last_ptr = NULL;

StoreTrace::StoreTrace(const Address& addr) :
  m_store_count(-1), m_store_first_to_stolen(-1), m_store_last_to_stolen(-1), m_store_first_to_last(-1)
{
  StoreTrace::initSummary();
  m_addr = addr;
  m_total_samples = 0;
  m_last_writer = -1;  // Really -1 isn't valid, so this will trigger the initilization code
  m_stores_this_interval = 0;
}

StoreTrace::~StoreTrace()
{
}

void StoreTrace::print(ostream& out) const
{
  out << m_addr;
  out << " total_samples: " << m_total_samples << endl;
  out << "store_count: " << m_store_count << endl;
  out << "store_first_to_stolen: " << m_store_first_to_stolen << endl;
  out << "store_last_to_stolen: " << m_store_last_to_stolen << endl; 
  out << "store_first_to_last: " << m_store_first_to_last  << endl;
}

// Class method
void StoreTrace::initSummary()
{
  if (!s_init) {
    s_total_samples = 0;
    s_store_count_ptr = new Histogram(-1);
    s_store_first_to_stolen_ptr = new Histogram(-1);
    s_store_last_to_stolen_ptr = new Histogram(-1);
    s_store_first_to_last_ptr = new Histogram(-1);
  }
  s_init = true;
}

// Class method
void StoreTrace::printSummary(ostream& out)
{
  out << "total_samples: " << s_total_samples << endl;
  out << "store_count: " << (*s_store_count_ptr) << endl;
  out << "store_first_to_stolen: " << (*s_store_first_to_stolen_ptr) << endl;
  out << "store_last_to_stolen: " << (*s_store_last_to_stolen_ptr) << endl; 
  out << "store_first_to_last: " << (*s_store_first_to_last_ptr) << endl;
}

// Class method
void StoreTrace::clearSummary()
{
  StoreTrace::initSummary();
  s_total_samples = 0;
  s_store_count_ptr->clear();
  s_store_first_to_stolen_ptr->clear();
  s_store_last_to_stolen_ptr->clear();
  s_store_first_to_last_ptr->clear();
}

void StoreTrace::store(NodeID node)
{
  Time current = g_eventQueue_ptr->getTime();

  assert((m_last_writer == -1) || (m_last_writer == node));

  m_last_writer = node;
  if (m_last_writer == -1) {
    assert(m_stores_this_interval == 0);
  }

  if (m_stores_this_interval == 0) {
    // A new proessor just wrote the line, so reset the stats
    m_first_store = current;
  }

  m_last_store = current;
  m_stores_this_interval++;
}

void StoreTrace::downgrade(NodeID node)
{
  if (node == m_last_writer) {
    Time current = g_eventQueue_ptr->getTime();
    assert(m_stores_this_interval != 0);
    assert(m_last_store != 0);
    assert(m_first_store != 0);
    assert(m_last_writer != -1);

    // Per line stats
    m_store_first_to_stolen.add(current - m_first_store);
    m_store_count.add(m_stores_this_interval);
    m_store_last_to_stolen.add(current - m_last_store);
    m_store_first_to_last.add(m_last_store - m_first_store);
    m_total_samples++;

    // Global stats
    assert(s_store_first_to_stolen_ptr != NULL);
    s_store_first_to_stolen_ptr->add(current - m_first_store);
    s_store_count_ptr->add(m_stores_this_interval);
    s_store_last_to_stolen_ptr->add(current - m_last_store);
    s_store_first_to_last_ptr->add(m_last_store - m_first_store);
    s_total_samples++;

    // Initilize for next go round
    m_stores_this_interval = 0;
    m_last_store = 0;
    m_first_store = 0;
    m_last_writer = -1;
  }
}

bool node_less_then_eq(const StoreTrace* n1, const StoreTrace* n2)
{
  return (n1->getTotal() > n2->getTotal());
}
