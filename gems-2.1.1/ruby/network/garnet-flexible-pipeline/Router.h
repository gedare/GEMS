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
 * Router.h
 *
 * Niket Agarwal, Princeton University
 *
 * */

#ifndef ROUTER_H
#define ROUTER_H

#include "NetworkHeader.h"
#include "GarnetNetwork.h"
#include "FlexibleConsumer.h"
#include "PrioHeap.h"
#include "NetworkLink.h"
#include "NetDest.h"
#include "flitBuffer.h"
#include "InVcState.h"
#include "OutVcState.h"

class VCarbiter;

class Router : public FlexibleConsumer {
public:
	Router(int id, GarnetNetwork *network_ptr);

	~Router();

	void addInPort(NetworkLink *in_link);
	void addOutPort(NetworkLink *out_link, const NetDest& routing_table_entry, int link_weight);
	void wakeup();
	void request_vc(int in_vc, int in_port, NetDest destination, Time request_time);
	bool isBufferNotFull(int vc, int inport);
	void grant_vc(int out_port, int vc, Time grant_time);
	void release_vc(int out_port, int vc, Time release_time);
	void vc_arbitrate();
	
	void printConfig(ostream& out) const;
        void print(ostream& out) const;

private:
/***************Data Members******************/
	int m_id;
	int m_virtual_networks, m_num_vcs, m_vc_per_vnet;
	GarnetNetwork *m_net_ptr;
	Vector<int > m_vc_round_robin; // For scheduling of out source queues	
	int m_round_robin_inport, m_round_robin_start; // for vc arbitration
	Vector<int > m_round_robin_invc; // For every outport. for vc arbitration

	Vector<Vector<flitBuffer *> > m_router_buffers; // These are essentially output buffers
	Vector<flitBuffer *> m_out_src_queue; // These are source queues for the output link
	Vector<NetworkLink *> m_in_link;
	Vector<NetworkLink *> m_out_link;
	Vector<Vector<InVcState * > > m_in_vc_state;
	Vector<Vector<OutVcState * > > m_out_vc_state;
	Vector<NetDest> m_routing_table;
 	Vector<int > m_link_weights;
	VCarbiter *m_vc_arbiter;

/*********** Private methods *************/
	int getRoute(NetDest destination);
	Vector<int > get_valid_vcs(int invc);
	void routeCompute(flit *m_flit, int inport);
	void checkReschedule();
	void check_arbiter_reschedule();
	void scheduleOutputLinks();
};

#endif

