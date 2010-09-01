
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
 * Switch.C
 *
 * Description: See Switch.h
 *
 * $Id$
 *
 */


#include "Switch.h"
#include "PerfectSwitch.h"
#include "MessageBuffer.h"
#include "Throttle.h"
#include "MessageSizeType.h"
#include "Network.h"
#include "Protocol.h"

Switch::Switch(SwitchID sid, SimpleNetwork* network_ptr)
{
  m_perfect_switch_ptr = new PerfectSwitch(sid, network_ptr);
  m_switch_id = sid;
  m_throttles.setSize(0);
}

Switch::~Switch()
{
  delete m_perfect_switch_ptr;

  // Delete throttles (one per output port)
  m_throttles.deletePointers();

  // Delete MessageBuffers
  m_buffers_to_free.deletePointers();
}

void Switch::addInPort(const Vector<MessageBuffer*>& in)
{
  m_perfect_switch_ptr->addInPort(in);
}

void Switch::addOutPort(const Vector<MessageBuffer*>& out, const NetDest& routing_table_entry, int link_latency, int bw_multiplier)
{
  Throttle* throttle_ptr = NULL;

  // Create a throttle
  throttle_ptr = new Throttle(m_switch_id, m_throttles.size(), link_latency, bw_multiplier);
  m_throttles.insertAtBottom(throttle_ptr);
  
  // Create one buffer per vnet (these are intermediaryQueues)
  Vector<MessageBuffer*> intermediateBuffers;
  for (int i=0; i<out.size(); i++) {
    MessageBuffer* buffer_ptr = new MessageBuffer;
    // Make these queues ordered
    buffer_ptr->setOrdering(true);
    if(FINITE_BUFFERING) {
      buffer_ptr->setSize(FINITE_BUFFER_SIZE); 
    }
    intermediateBuffers.insertAtBottom(buffer_ptr);
    m_buffers_to_free.insertAtBottom(buffer_ptr);
  }
  
  // Hook the queues to the PerfectSwitch
  m_perfect_switch_ptr->addOutPort(intermediateBuffers, routing_table_entry);
  
  // Hook the queues to the Throttle
  throttle_ptr->addLinks(intermediateBuffers, out);

}

void Switch::clearRoutingTables()
{
  m_perfect_switch_ptr->clearRoutingTables();
}

void Switch::clearBuffers()
{
  m_perfect_switch_ptr->clearBuffers();
  for (int i=0; i<m_throttles.size(); i++) {
    if (m_throttles[i] != NULL) {
      m_throttles[i]->clear();
    }
  }
}

void Switch::reconfigureOutPort(const NetDest& routing_table_entry)
{
  m_perfect_switch_ptr->reconfigureOutPort(routing_table_entry);
}

const Throttle* Switch::getThrottle(LinkID link_number) const
{
  assert(m_throttles[link_number] != NULL);
  return m_throttles[link_number];
}

const Vector<Throttle*>* Switch::getThrottles() const
{
  return &m_throttles;
}

void Switch::printStats(ostream& out) const
{
  out << "switch_" << m_switch_id << "_inlinks: " << m_perfect_switch_ptr->getInLinks() << endl;
  out << "switch_" << m_switch_id << "_outlinks: " << m_perfect_switch_ptr->getOutLinks() << endl;

  // Average link utilizations
  double average_utilization = 0.0;
  int throttle_count = 0;

  for (int i=0; i<m_throttles.size(); i++) {
    Throttle* throttle_ptr = m_throttles[i];
    if (throttle_ptr != NULL) {
      average_utilization += throttle_ptr->getUtilization();
      throttle_count++;
    }
  }
  average_utilization = (throttle_count == 0) ? 0 : average_utilization / float(throttle_count);

  // Individual link utilizations
  out << "links_utilized_percent_switch_" << m_switch_id << ": " << average_utilization << endl;
  for (int link=0; link<m_throttles.size(); link++) {
    Throttle* throttle_ptr = m_throttles[link];
    if (throttle_ptr != NULL) {
      out << "  links_utilized_percent_switch_" << m_switch_id << "_link_" << link << ": " 
          << throttle_ptr->getUtilization() << " bw: " << throttle_ptr->getLinkBandwidth() 
          << " base_latency: " << throttle_ptr->getLatency() << endl;
    }
  }
  out << endl;

  // Traffic breakdown
  for (int link=0; link<m_throttles.size(); link++) {
    Throttle* throttle_ptr = m_throttles[link];
    if (throttle_ptr != NULL) {
      const Vector<Vector<int> >& message_counts = throttle_ptr->getCounters();
      for (int int_type=0; int_type<MessageSizeType_NUM; int_type++) {
        MessageSizeType type = MessageSizeType(int_type);
        int sum = message_counts[type].sum();
        if (sum != 0) {
          out << "  outgoing_messages_switch_" << m_switch_id << "_link_" << link << "_" << type 
              << ": " << sum << " " << sum * MessageSizeType_to_int(type) 
              << " " << message_counts[type] << " base_latency: " << throttle_ptr->getLatency() << endl;
        }
      }
    }
  }
  out << endl;
}

void Switch::clearStats()
{
  m_perfect_switch_ptr->clearStats();
  for (int i=0; i<m_throttles.size(); i++) {
    if (m_throttles[i] != NULL) {
      m_throttles[i]->clearStats();
    }
  }
}

void Switch::printConfig(ostream& out) const
{
  m_perfect_switch_ptr->printConfig(out);
  for (int i=0; i<m_throttles.size(); i++) {
    if (m_throttles[i] != NULL) {
      m_throttles[i]->printConfig(out);
    }
  }
}

void Switch::print(ostream& out) const
{
  // FIXME printing
  out << "[Switch]";
}

