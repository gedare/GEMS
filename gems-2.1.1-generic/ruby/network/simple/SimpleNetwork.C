
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
 * SimpleNetwork.C
 *
 * Description: See SimpleNetwork.h
 *
 * $Id$
 *
 */

#include "SimpleNetwork.h"
#include "Profiler.h"
#include "System.h"
#include "Switch.h"
#include "NetDest.h"
#include "Topology.h"
#include "TopologyType.h"
#include "MachineType.h"
#include "MessageBuffer.h"
#include "Protocol.h"
#include "Map.h"

// ***BIG HACK*** - This is actually code that _should_ be in Network.C

// Note: Moved to Princeton Network
// calls new to abstract away from the network
/*
Network* Network::createNetwork(int nodes)
{
  return new SimpleNetwork(nodes);
}
*/

SimpleNetwork::SimpleNetwork(int nodes)
{
  m_nodes = MachineType_base_number(MachineType_NUM);

  m_virtual_networks = NUMBER_OF_VIRTUAL_NETWORKS;
  m_endpoint_switches.setSize(m_nodes);

  m_in_use.setSize(m_virtual_networks);
  m_ordered.setSize(m_virtual_networks);
  for (int i = 0; i < m_virtual_networks; i++) {
    m_in_use[i] = false;
    m_ordered[i] = false;
  }
  
  // Allocate to and from queues
  m_toNetQueues.setSize(m_nodes);
  m_fromNetQueues.setSize(m_nodes);
  for (int node = 0; node < m_nodes; node++) {
    m_toNetQueues[node].setSize(m_virtual_networks);
    m_fromNetQueues[node].setSize(m_virtual_networks);
    for (int j = 0; j < m_virtual_networks; j++) {
      m_toNetQueues[node][j] = new MessageBuffer;
      m_fromNetQueues[node][j] = new MessageBuffer;
    }
  }

  // Setup the network switches
  m_topology_ptr = new Topology(this, m_nodes);
  int number_of_switches = m_topology_ptr->numSwitches();
  for (int i=0; i<number_of_switches; i++) {
    m_switch_ptr_vector.insertAtBottom(new Switch(i, this));
  }
  m_topology_ptr->createLinks(false);  // false because this isn't a reconfiguration
}

void SimpleNetwork::reset()
{
  for (int node = 0; node < m_nodes; node++) {
    for (int j = 0; j < m_virtual_networks; j++) {
      m_toNetQueues[node][j]->clear();
      m_fromNetQueues[node][j]->clear();
    }
  }

  for(int i=0; i<m_switch_ptr_vector.size(); i++){
    m_switch_ptr_vector[i]->clearBuffers();
  }
}

SimpleNetwork::~SimpleNetwork()
{
  for (int i = 0; i < m_nodes; i++) {
    m_toNetQueues[i].deletePointers();
    m_fromNetQueues[i].deletePointers();
  }
  m_switch_ptr_vector.deletePointers();
  m_buffers_to_free.deletePointers();
  delete m_topology_ptr;
}

// From a switch to an endpoint node
void SimpleNetwork::makeOutLink(SwitchID src, NodeID dest, const NetDest& routing_table_entry, int link_latency, int link_weight, int bw_multiplier, bool isReconfiguration)
{
  assert(dest < m_nodes);
  assert(src < m_switch_ptr_vector.size());
  assert(m_switch_ptr_vector[src] != NULL);
  if(!isReconfiguration){
    m_switch_ptr_vector[src]->addOutPort(m_fromNetQueues[dest], routing_table_entry, link_latency, bw_multiplier);
    m_endpoint_switches[dest] = m_switch_ptr_vector[src];
  } else {
    m_switch_ptr_vector[src]->reconfigureOutPort(routing_table_entry);
  }
}

// From an endpoint node to a switch
void SimpleNetwork::makeInLink(NodeID src, SwitchID dest, const NetDest& routing_table_entry, int link_latency, int bw_multiplier, bool isReconfiguration)
{
  assert(src < m_nodes);
  if(!isReconfiguration){
    m_switch_ptr_vector[dest]->addInPort(m_toNetQueues[src]);
  } else {
    // do nothing
  }
}

// From a switch to a switch
void SimpleNetwork::makeInternalLink(SwitchID src, SwitchID dest, const NetDest& routing_table_entry, int link_latency, int link_weight, int bw_multiplier, bool isReconfiguration)
{
  if(!isReconfiguration){
    // Create a set of new MessageBuffers
    Vector<MessageBuffer*> queues;
    for (int i = 0; i < m_virtual_networks; i++) {
      // allocate a buffer
      MessageBuffer* buffer_ptr = new MessageBuffer;
      buffer_ptr->setOrdering(true);
      if(FINITE_BUFFERING) {
        buffer_ptr->setSize(FINITE_BUFFER_SIZE); 
      }
      queues.insertAtBottom(buffer_ptr);
      // remember to deallocate it
      m_buffers_to_free.insertAtBottom(buffer_ptr);
    }
  
    // Connect it to the two switches
    m_switch_ptr_vector[dest]->addInPort(queues);
    m_switch_ptr_vector[src]->addOutPort(queues, routing_table_entry, link_latency, bw_multiplier);
  } else {
    m_switch_ptr_vector[src]->reconfigureOutPort(routing_table_entry);
  }
}

void SimpleNetwork::checkNetworkAllocation(NodeID id, bool ordered, int network_num)
{
  ASSERT(id < m_nodes);
  ASSERT(network_num < m_virtual_networks);

  if (ordered) {
    m_ordered[network_num] = true;
  }
  m_in_use[network_num] = true;
}

MessageBuffer* SimpleNetwork::getToNetQueue(NodeID id, bool ordered, int network_num)
{
  checkNetworkAllocation(id, ordered, network_num);
  return m_toNetQueues[id][network_num];
}

MessageBuffer* SimpleNetwork::getFromNetQueue(NodeID id, bool ordered, int network_num)
{
  checkNetworkAllocation(id, ordered, network_num);
  return m_fromNetQueues[id][network_num];
}

const Vector<Throttle*>* SimpleNetwork::getThrottles(NodeID id) const
{
  assert(id >= 0);
  assert(id < m_nodes);
  assert(m_endpoint_switches[id] != NULL);
  return m_endpoint_switches[id]->getThrottles();
}

void SimpleNetwork::printStats(ostream& out) const
{
  out << endl;
  out << "Network Stats" << endl;
  out << "-------------" << endl;
  out << endl;
  for(int i=0; i<m_switch_ptr_vector.size(); i++) {
    m_switch_ptr_vector[i]->printStats(out);
  }
}

void SimpleNetwork::clearStats()
{
  for(int i=0; i<m_switch_ptr_vector.size(); i++) {
    m_switch_ptr_vector[i]->clearStats();
  }
}

void SimpleNetwork::printConfig(ostream& out) const 
{
  out << endl;
  out << "Network Configuration" << endl;
  out << "---------------------" << endl;
  out << "network: SIMPLE_NETWORK" << endl;
  out << "topology: " << g_NETWORK_TOPOLOGY << endl;
  out << endl;

  for (int i = 0; i < m_virtual_networks; i++) {
    out << "virtual_net_" << i << ": ";
    if (m_in_use[i]) {
      out << "active, ";
      if (m_ordered[i]) {
        out << "ordered" << endl;
      } else {
        out << "unordered" << endl;
      }
    } else {
      out << "inactive" << endl;
    }
  }
  out << endl;
  for(int i=0; i<m_switch_ptr_vector.size(); i++) {
    m_switch_ptr_vector[i]->printConfig(out);
  }

  if (g_PRINT_TOPOLOGY) {
    m_topology_ptr->printConfig(out);
  }
}

void SimpleNetwork::print(ostream& out) const
{
  out << "[SimpleNetwork]";
}
