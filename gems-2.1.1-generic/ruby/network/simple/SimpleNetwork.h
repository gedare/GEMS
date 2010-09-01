
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
 * SimpleNetwork.h
 * 
 * Description: The SimpleNetwork class implements the interconnection
 * SimpleNetwork between components (processor/cache components and
 * memory/directory components).  The interconnection network as
 * described here is not a physical network, but a programming concept
 * used to implement all communication between components.  Thus parts
 * of this 'network' may model the on-chip connections between cache
 * controllers and directory controllers as well as the links between
 * chip and network switches.
 *
 * Two conceptual networks, an address and data network, are modeled.
 * The data network is unordered, where the address network provides
 * and conforms to a global ordering of all transactions.  
 *
 * Currently the data network is point-to-point and the address
 * network is a broadcast network. These two distinct conceptual
 * network can be modeled as physically separate networks or
 * multiplexed over a single physical network.
 * 
 * The network encapsulates all notion of virtual global time and is
 * responsible for ordering the network transactions received.  This
 * hides all of these ordering details from the processor/cache and
 * directory/memory modules.
 * 
 * FIXME: Various flavor of networks are provided as a compiler time
 *        configurable. We currently include this SimpleNetwork in the
 *        makefile's vpath, so that SimpleNetwork.C can provide an alternative
 *        version constructor for the abstract Network class. It is easy to
 *        modify this to make network a runtime configuable. Just make the
 *        abstract Network class take a enumeration parameter, and based on
 *        that to initial proper network. Or even better, just make the ruby
 *        system initializer choose the proper network to initiate.
 *
 * $Id$
 *
 */

#ifndef SIMPLENETWORK_H
#define SIMPLENETWORK_H

#include "Global.h"
#include "Vector.h"
#include "Network.h"
#include "NodeID.h"

class NetDest;
class MessageBuffer;
class Throttle;
class Switch;
class Topology; 

class SimpleNetwork : public Network {
public:
  // Constructors
  SimpleNetwork(int nodes);

  // Destructor
  ~SimpleNetwork();
  
  // Public Methods
  void printStats(ostream& out) const;
  void clearStats();
  void printConfig(ostream& out) const;

  void reset();

  // returns the queue requested for the given component
  MessageBuffer* getToNetQueue(NodeID id, bool ordered, int network_num);
  MessageBuffer* getFromNetQueue(NodeID id, bool ordered, int network_num);
  virtual const Vector<Throttle*>* getThrottles(NodeID id) const;
  
  bool isVNetOrdered(int vnet) { return m_ordered[vnet]; }
  bool validVirtualNetwork(int vnet) { return m_in_use[vnet]; }

  int getNumNodes() {return m_nodes; }

  // Methods used by Topology to setup the network
  void makeOutLink(SwitchID src, NodeID dest, const NetDest& routing_table_entry, int link_latency, int link_weight, int bw_multiplier, bool isReconfiguration);
  void makeInLink(SwitchID src, NodeID dest, const NetDest& routing_table_entry, int link_latency, int bw_multiplier, bool isReconfiguration);
  void makeInternalLink(SwitchID src, NodeID dest, const NetDest& routing_table_entry, int link_latency, int link_weight, int bw_multiplier, bool isReconfiguration);

  void print(ostream& out) const;
private:
  void checkNetworkAllocation(NodeID id, bool ordered, int network_num);
  void addLink(SwitchID src, SwitchID dest, int link_latency);
  void makeLink(SwitchID src, SwitchID dest, const NetDest& routing_table_entry, int link_latency);
  SwitchID createSwitch();
  void makeTopology();
  void linkTopology();


  // Private copy constructor and assignment operator
  SimpleNetwork(const SimpleNetwork& obj);
  SimpleNetwork& operator=(const SimpleNetwork& obj);
  
  // Data Members (m_ prefix)
  
  // vector of queues from the components
  Vector<Vector<MessageBuffer*> > m_toNetQueues;
  Vector<Vector<MessageBuffer*> > m_fromNetQueues;

  int m_nodes;
  int m_virtual_networks;
  Vector<bool> m_in_use;
  Vector<bool> m_ordered;
  Vector<Switch*> m_switch_ptr_vector;
  Vector<MessageBuffer*> m_buffers_to_free;
  Vector<Switch*> m_endpoint_switches;
  Topology* m_topology_ptr;
};

// Output operator declaration
ostream& operator<<(ostream& out, const SimpleNetwork& obj);

// ******************* Definitions *******************

// Output operator definition
extern inline 
ostream& operator<<(ostream& out, const SimpleNetwork& obj)
{
  obj.print(out);
  out << flush;
  return out;
}

#endif //SIMPLENETWORK_H
