/*
    Copyright (C) 1999-2008 by Mark D. Hill and David A. Wood for the
    Wisconsin Multifacet Project.  Contact: gems@cs.wisc.edu
    http://www.cs.wisc.edu/gems/

    --------------------------------------------------------------------

    This file is part of Garnet (Princeton's interconnect model),
    a component of the Multifacet GEMS (General Execution-driven 
    Multiprocessor Simulator) software toolset originally developed at 
    the University of Wisconsin-Madison.

    Garnet was developed by Niket Agarwal at Princeton University. Orion was
    developed by Princeton University.

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
 * GarnetNetwork.h
 *
 * Niket Agarwal, Princeton University
 *
 * */

#ifndef GARNET_NETWORK_H
#define GARNET_NETWORK_H

#include "NetworkHeader.h"
#include "Vector.h"
#include "NetworkConfig.h"
#include "Network.h"

class NetworkInterface;
class MessageBuffer;
class Router;
class Topology;
class NetDest;
class NetworkLink;

class GarnetNetwork : public Network{
public:
	GarnetNetwork(int nodes);

	~GarnetNetwork();
	
	// returns the queue requested for the given component
	MessageBuffer* getToNetQueue(NodeID id, bool ordered, int network_num);
	MessageBuffer* getFromNetQueue(NodeID id, bool ordered, int network_num);
	
	void clearStats();
  	void printStats(ostream& out) const;
  	void printConfig(ostream& out) const;
  	void print(ostream& out) const;
 
 	bool isVNetOrdered(int vnet) { return m_ordered[vnet]; }
  	bool validVirtualNetwork(int vnet) { return m_in_use[vnet]; }

	Time getRubyStartTime();
	int getNumNodes(){ return m_nodes; }
  	
  	void reset();
	
        // Methods used by Topology to setup the network
  	void makeOutLink(SwitchID src, NodeID dest, const NetDest& routing_table_entry, int link_latency, int link_weight,  int bw_multiplier, bool isReconfiguration);
  	void makeInLink(SwitchID src, NodeID dest, const NetDest& routing_table_entry, int link_latency, int bw_multiplier, bool isReconfiguration);
  	void makeInternalLink(SwitchID src, NodeID dest, const NetDest& routing_table_entry, int link_latency, int link_weight, int bw_multiplier, bool isReconfiguration);

private:
	void checkNetworkAllocation(NodeID id, bool ordered, int network_num);

// Private copy constructor and assignment operator
	GarnetNetwork(const GarnetNetwork& obj);
	GarnetNetwork& operator=(const GarnetNetwork& obj);

/***********Data Members*************/
	int m_virtual_networks; 
	int m_nodes;

	Vector<bool> m_in_use;
	Vector<bool> m_ordered;

	Vector<Vector<MessageBuffer*> > m_toNetQueues; 
  	Vector<Vector<MessageBuffer*> > m_fromNetQueues;

	Vector<Router *> m_router_ptr_vector;   // All Routers in Network
	Vector<NetworkLink *> m_link_ptr_vector; // All links in the network	
	Vector<NetworkInterface *> m_ni_ptr_vector;	// All NI's in Network

	Topology* m_topology_ptr;        	
	Time m_ruby_start;
};

// Output operator declaration
ostream& operator<<(ostream& out, const GarnetNetwork& obj);

// ******************* Definitions *******************
// Output operator definition
extern inline
ostream& operator<<(ostream& out, const GarnetNetwork& obj)
{
	obj.print(out);
	out << flush;
	return out;
}

#endif //NETWORK_H
