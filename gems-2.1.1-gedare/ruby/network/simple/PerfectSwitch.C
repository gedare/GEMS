
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
 * PerfectSwitch.C
 *
 * Description: See PerfectSwitch.h
 *
 * $Id$
 *
 */


#include "PerfectSwitch.h"
#include "NetworkMessage.h"
#include "Profiler.h"
#include "System.h"
#include "SimpleNetwork.h"
#include "util.h"
#include "MessageBuffer.h"
#include "Protocol.h"

const int PRIORITY_SWITCH_LIMIT = 128;

// Operator for helper class
bool operator<(const LinkOrder& l1, const LinkOrder& l2) {
  return (l1.m_value < l2.m_value);
}

PerfectSwitch::PerfectSwitch(SwitchID sid, SimpleNetwork* network_ptr)
{
  m_virtual_networks = NUMBER_OF_VIRTUAL_NETWORKS;  // FIXME - pass me as a parameter?
  m_switch_id = sid;
  m_round_robin_start = 0;
  m_network_ptr = network_ptr;
  m_wakeups_wo_switch = 0;
}

void PerfectSwitch::addInPort(const Vector<MessageBuffer*>& in)
{
  assert(in.size() == m_virtual_networks);
  NodeID port = m_in.size();
  m_in.insertAtBottom(in);
  for (int j = 0; j < m_virtual_networks; j++) {
    m_in[port][j]->setConsumer(this);
    string desc = "[Queue from port " +  NodeIDToString(m_switch_id) + " " + NodeIDToString(port) + " " + NodeIDToString(j) + " to PerfectSwitch]";
    m_in[port][j]->setDescription(desc);
  }
}

void PerfectSwitch::addOutPort(const Vector<MessageBuffer*>& out, const NetDest& routing_table_entry)
{
  assert(out.size() == m_virtual_networks);

  // Setup link order
  LinkOrder l;
  l.m_value = 0;
  l.m_link = m_out.size();
  m_link_order.insertAtBottom(l);

  // Add to routing table
  m_out.insertAtBottom(out);
  m_routing_table.insertAtBottom(routing_table_entry);

  if (g_PRINT_TOPOLOGY) {
    m_out_link_vec.insertAtBottom(out);  
  }
}

void PerfectSwitch::clearRoutingTables()
{
  m_routing_table.clear();
}

void PerfectSwitch::clearBuffers()
{
  for(int i=0; i<m_in.size(); i++){
    for(int vnet=0; vnet < m_virtual_networks; vnet++) {
      m_in[i][vnet]->clear();
    }
  }

  for(int i=0; i<m_out.size(); i++){
    for(int vnet=0; vnet < m_virtual_networks; vnet++) {
      m_out[i][vnet]->clear();
    }
  }
}

void PerfectSwitch::reconfigureOutPort(const NetDest& routing_table_entry)
{
  m_routing_table.insertAtBottom(routing_table_entry);
}

PerfectSwitch::~PerfectSwitch()
{
}

void PerfectSwitch::wakeup()
{

  DEBUG_EXPR(NETWORK_COMP, MedPrio, m_switch_id);

  MsgPtr msg_ptr;

  // Give the highest numbered link priority most of the time
  m_wakeups_wo_switch++;
  int highest_prio_vnet = m_virtual_networks-1;
  int lowest_prio_vnet = 0;
  int decrementer = 1;
  bool schedule_wakeup = false;
  NetworkMessage* net_msg_ptr = NULL;

  // invert priorities to avoid starvation seen in the component network
  if (m_wakeups_wo_switch > PRIORITY_SWITCH_LIMIT) { 
    m_wakeups_wo_switch = 0;
    highest_prio_vnet = 0;
    lowest_prio_vnet = m_virtual_networks-1;
    decrementer = -1;
  } 

  for (int vnet = highest_prio_vnet; (vnet*decrementer) >= (decrementer*lowest_prio_vnet); vnet -= decrementer) {

    // For all components incoming queues
    int incoming = m_round_robin_start; // This is for round-robin scheduling
    m_round_robin_start++;
    if (m_round_robin_start >= m_in.size()) {
      m_round_robin_start = 0;
    }

    // for all input ports, use round robin scheduling
    for (int counter = 0; counter < m_in.size(); counter++) {

      // Round robin scheduling
      incoming++;
      if (incoming >= m_in.size()) {
        incoming = 0;
      }

      // temporary vectors to store the routing results
      Vector<LinkID> output_links;
      Vector<NetDest> output_link_destinations;

      // Is there a message waiting?
      while (m_in[incoming][vnet]->isReady()) {

        DEBUG_EXPR(NETWORK_COMP, MedPrio, incoming);

        // Peek at message
        msg_ptr = m_in[incoming][vnet]->peekMsgPtr();
        net_msg_ptr = dynamic_cast<NetworkMessage*>(msg_ptr.ref());
        DEBUG_EXPR(NETWORK_COMP, MedPrio, *net_msg_ptr);

        output_links.clear();
        output_link_destinations.clear();
        NetDest msg_destinations = net_msg_ptr->getInternalDestination();

        // Unfortunately, the token-protocol sends some
        // zero-destination messages, so this assert isn't valid
        // assert(msg_destinations.count() > 0);

        assert(m_link_order.size() == m_routing_table.size());
        assert(m_link_order.size() == m_out.size());

        if (g_adaptive_routing) {
          if (m_network_ptr->isVNetOrdered(vnet)) {
            // Don't adaptively route
            for (int outlink=0; outlink<m_out.size(); outlink++) {
              m_link_order[outlink].m_link = outlink;
              m_link_order[outlink].m_value = 0;
            }
          } else {
            // Find how clogged each link is
            for (int outlink=0; outlink<m_out.size(); outlink++) {
              int out_queue_length = 0;
              for (int v=0; v<m_virtual_networks; v++) {
                out_queue_length += m_out[outlink][v]->getSize();
              }
              m_link_order[outlink].m_link = outlink;
              m_link_order[outlink].m_value = 0;
              m_link_order[outlink].m_value |= (out_queue_length << 8);
              m_link_order[outlink].m_value |= (random() & 0xff);
            }
            m_link_order.sortVector();  // Look at the most empty link first
          }
        }

        for (int i=0; i<m_routing_table.size(); i++) {
          // pick the next link to look at
          int link = m_link_order[i].m_link;

          DEBUG_EXPR(NETWORK_COMP, MedPrio, m_routing_table[link]);

          if (msg_destinations.intersectionIsNotEmpty(m_routing_table[link])) {

            // Remember what link we're using
            output_links.insertAtBottom(link);
            
            // Need to remember which destinations need this message
            // in another vector.  This Set is the intersection of the
            // routing_table entry and the current destination set.
            // The intersection must not be empty, since we are inside "if"
            output_link_destinations.insertAtBottom(msg_destinations.AND(m_routing_table[link]));
            
            // Next, we update the msg_destination not to include
            // those nodes that were already handled by this link
            msg_destinations.removeNetDest(m_routing_table[link]);
          }
        }

        assert(msg_destinations.count() == 0);
        //assert(output_links.size() > 0);

        // Check for resources - for all outgoing queues
        bool enough = true;
        for (int i=0; i<output_links.size(); i++) {
          int outgoing = output_links[i];
          enough = enough && m_out[outgoing][vnet]->areNSlotsAvailable(1);
          DEBUG_MSG(NETWORK_COMP, HighPrio, "checking if node is blocked");
          DEBUG_EXPR(NETWORK_COMP, HighPrio, outgoing);
          DEBUG_EXPR(NETWORK_COMP, HighPrio, vnet);
          DEBUG_EXPR(NETWORK_COMP, HighPrio, enough);
        }

        // There were not enough resources
        if(!enough) {
          g_eventQueue_ptr->scheduleEvent(this, 1);
          DEBUG_MSG(NETWORK_COMP, HighPrio, "Can't deliver message to anyone since a node is blocked");
          DEBUG_EXPR(NETWORK_COMP, HighPrio, *net_msg_ptr);
          break; // go to next incoming port
        }
        
       MsgPtr unmodified_msg_ptr;

        if (output_links.size() > 1) {  
          // If we are sending this message down more than one link
          // (size>1), we need to make a copy of the message so each
          // branch can have a different internal destination
          // we need to create an unmodified MsgPtr because the MessageBuffer enqueue func
          // will modify the message
          unmodified_msg_ptr = *(msg_ptr.ref());  // This magic line creates a private copy of the message
        }

        // Enqueue it - for all outgoing queues
        for (int i=0; i<output_links.size(); i++) {
          int outgoing = output_links[i];

          if (i > 0) {  
            msg_ptr = *(unmodified_msg_ptr.ref());  // create a private copy of the unmodified message
          }

          // Change the internal destination set of the message so it
          // knows which destinations this link is responsible for.
          net_msg_ptr = dynamic_cast<NetworkMessage*>(msg_ptr.ref());
          net_msg_ptr->getInternalDestination() = output_link_destinations[i];

          // Enqeue msg
          DEBUG_NEWLINE(NETWORK_COMP,HighPrio);
          DEBUG_MSG(NETWORK_COMP,HighPrio,"switch: " + int_to_string(m_switch_id)
                    + " enqueuing net msg from inport[" + int_to_string(incoming) + "][" 
                    + int_to_string(vnet) +"] to outport [" + int_to_string(outgoing) 
                    + "][" + int_to_string(vnet) +"]" 
                    + " time: " + int_to_string(g_eventQueue_ptr->getTime()) + ".");
          DEBUG_NEWLINE(NETWORK_COMP,HighPrio);

          m_out[outgoing][vnet]->enqueue(msg_ptr);
        }

        // Dequeue msg
        m_in[incoming][vnet]->pop();
      }
    }
  }
}

void PerfectSwitch::printStats(ostream& out) const
{ 
  out << "PerfectSwitch printStats" << endl;
}

void PerfectSwitch::clearStats()
{
}

void PerfectSwitch::printConfig(ostream& out) const
{ 
}

void PerfectSwitch::print(ostream& out) const
{
  out << "[PerfectSwitch " << m_switch_id << "]";
}

