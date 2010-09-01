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
 * GarnetNetwork_d.h
 *
 * Niket Agarwal, Princeton University
 *
 * */

#ifndef GARNETNETWORK_D_H
#define GARNETNETWORK_D_H

#include "NetworkHeader.h"
#include "Vector.h"
#include "NetworkConfig.h"
#include "Network.h"

class NetworkInterface_d;
class MessageBuffer;
class Router_d;
class Topology;
class NetDest;
class NetworkLink_d;
class CreditLink_d;

class GarnetNetwork_d : public Network{
public:
	GarnetNetwork_d(int nodes);

	~GarnetNetwork_d();

	int getNumNodes(){ return m_nodes;}	
	
	// returns the queue requested for the given component
	MessageBuffer* getToNetQueue(NodeID id, bool ordered, int network_num);
	MessageBuffer* getFromNetQueue(NodeID id, bool ordered, int network_num);
	
	void clearStats();
  	void printStats(ostream& out) const;
  	void printConfig(ostream& out) const;
  	void print(ostream& out) const;

	inline void increment_injected_flits()
	{
		m_flits_injected++;
	}
	inline void increment_recieved_flits()
	{
		m_flits_recieved++;
	}
	inline void increment_network_latency(Time latency)
	{
		m_network_latency += latency;
	}
	inline void increment_queueing_latency(Time latency)
	{
		m_queueing_latency += latency;
	}

 	bool isVNetOrdered(int vnet) 
	{
		return m_ordered[vnet]; 
	}
  	bool validVirtualNetwork(int vnet) { return m_in_use[vnet]; }

	Time getRubyStartTime();
  	
  	void reset();
	
        // Methods used by Topology to setup the network
  	void makeOutLink(SwitchID src, NodeID dest, const NetDest& routing_table_entry, int link_latency, int link_weight,  int bw_multiplier, bool isReconfiguration);
  	void makeInLink(SwitchID src, NodeID dest, const NetDest& routing_table_entry, int link_latency, int bw_multiplier, bool isReconfiguration);
  	void makeInternalLink(SwitchID src, NodeID dest, const NetDest& routing_table_entry, int link_latency, int link_weight, int bw_multiplier, bool isReconfiguration);

private:
	void checkNetworkAllocation(NodeID id, bool ordered, int network_num);

// Private copy constructor and assignment operator
	GarnetNetwork_d(const GarnetNetwork_d& obj);
	GarnetNetwork_d& operator=(const GarnetNetwork_d& obj);

/***********Data Members*************/
	int m_virtual_networks; 
	int m_nodes;
	int m_flits_recieved, m_flits_injected;
	double m_network_latency, m_queueing_latency;

	Vector<bool> m_in_use;
	Vector<bool> m_ordered;

	Vector<Vector<MessageBuffer*> > m_toNetQueues; 
  	Vector<Vector<MessageBuffer*> > m_fromNetQueues;

	Vector<Router_d *> m_router_ptr_vector;   // All Routers in Network
	Vector<NetworkLink_d *> m_link_ptr_vector; // All links in the network	
	Vector<CreditLink_d *> m_creditlink_ptr_vector; // All links in the network	
	Vector<NetworkInterface_d *> m_ni_ptr_vector;	// All NI's in Network

	Topology* m_topology_ptr;        	
	Time m_ruby_start;
};

// Output operator declaration
ostream& operator<<(ostream& out, const GarnetNetwork_d& obj);

// ******************* Definitions *******************
// Output operator definition
extern inline
ostream& operator<<(ostream& out, const GarnetNetwork_d& obj)
{
	obj.print(out);
	out << flush;
	return out;
}

#endif //GARNETNETWORK_D_H
