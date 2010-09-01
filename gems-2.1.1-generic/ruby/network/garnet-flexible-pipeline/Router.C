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
 * Router.C
 *
 * Niket Agarwal, Princeton University
 *
 * */

#include "Router.h"
#include "NetworkMessage.h"
#include "InVcState.h"
#include "OutVcState.h"
#include "VCarbiter.h"

Router::Router(int id, GarnetNetwork *network_ptr)
{
	m_id = id;
	m_net_ptr = network_ptr;
	m_virtual_networks = NUMBER_OF_VIRTUAL_NETWORKS;
	m_vc_per_vnet = NetworkConfig::getVCsPerClass();
	m_round_robin_inport = 0;
	m_round_robin_start = 0;
	m_num_vcs = m_vc_per_vnet*m_virtual_networks;
	m_vc_arbiter = new VCarbiter(this);
}

Router::~Router()
{
	for (int i = 0; i < m_in_link.size(); i++) 
	{
		m_in_vc_state[i].deletePointers();
	}
	for (int i = 0; i < m_out_link.size(); i++) 
	{
		m_out_vc_state[i].deletePointers();
		m_router_buffers[i].deletePointers();
	}
	m_out_src_queue.deletePointers();
	delete m_vc_arbiter;

}

void Router::addInPort(NetworkLink *in_link)
{
	int port = m_in_link.size();
	Vector<InVcState *> in_vc_vector;
	for(int i = 0; i < m_num_vcs; i++)
	{
		in_vc_vector.insertAtBottom(new InVcState(i));
		in_vc_vector[i]->setState(IDLE_, g_eventQueue_ptr->getTime());
	}
	m_in_vc_state.insertAtBottom(in_vc_vector);
	m_in_link.insertAtBottom(in_link);
	in_link->setLinkConsumer(this);
	in_link->setInPort(port);

	int start = 0;
	m_round_robin_invc.insertAtBottom(start);

}

void Router::addOutPort(NetworkLink *out_link, const NetDest& routing_table_entry, int link_weight)
{
	int port = m_out_link.size();
	out_link->setOutPort(port);
	int start = 0;
	m_vc_round_robin.insertAtBottom(start);  

	m_out_src_queue.insertAtBottom(new flitBuffer());

	m_out_link.insertAtBottom(out_link);
	m_routing_table.insertAtBottom(routing_table_entry);
	out_link->setSourceQueue(m_out_src_queue[port]);
	out_link->setSource(this);

	Vector<flitBuffer *> intermediateQueues;
	for(int i = 0; i < m_num_vcs; i++)
	{
		intermediateQueues.insertAtBottom(new flitBuffer(NetworkConfig::getBufferSize()));
	}
	m_router_buffers.insertAtBottom(intermediateQueues);

	Vector<OutVcState *> out_vc_vector;
	for(int i = 0; i < m_num_vcs; i++)
	{
		out_vc_vector.insertAtBottom(new OutVcState(i));
		out_vc_vector[i]->setState(IDLE_, g_eventQueue_ptr->getTime());
	}
	m_out_vc_state.insertAtBottom(out_vc_vector);
	m_link_weights.insertAtBottom(link_weight);
}

bool Router::isBufferNotFull(int vc, int inport)
{
	int outport = m_in_vc_state[inport][vc]->get_outport();
	int outvc = m_in_vc_state[inport][vc]->get_outvc();
	
	return (!m_router_buffers[outport][outvc]->isFull());
}

// A request for an output vc has been placed by an upstream Router/NI. This has to be updated and arbitration performed
void Router::request_vc(int in_vc, int in_port, NetDest destination, Time request_time)
{
	assert(m_in_vc_state[in_port][in_vc]->isInState(IDLE_, request_time));	

	int outport = getRoute(destination); 
	m_in_vc_state[in_port][in_vc]->setRoute(outport);
	m_in_vc_state[in_port][in_vc]->setState(VC_AB_, request_time);
	assert(request_time >= g_eventQueue_ptr->getTime());
	if(request_time > g_eventQueue_ptr->getTime())
		g_eventQueue_ptr->scheduleEventAbsolute(m_vc_arbiter, request_time);
	else 
		vc_arbitrate();
}

void Router::vc_arbitrate()
{
	int inport = m_round_robin_inport;
	m_round_robin_inport++;
	if(m_round_robin_inport == m_in_link.size())
		m_round_robin_inport = 0;

	for(int port_iter = 0; port_iter < m_in_link.size(); port_iter++)
	{
		inport++;
		if(inport >= m_in_link.size())
			inport = 0;
		int invc = m_round_robin_invc[inport];
		m_round_robin_invc[inport]++;

		if(m_round_robin_invc[inport] >= m_num_vcs)
			m_round_robin_invc[inport] = 0;
		for(int vc_iter = 0; vc_iter < m_num_vcs; vc_iter++)
		{	
			invc++;
			if(invc >= m_num_vcs)
				invc = 0;
			InVcState *in_vc_state = m_in_vc_state[inport][invc];

			if(in_vc_state->isInState(VC_AB_, g_eventQueue_ptr->getTime()))
			{
				int outport = in_vc_state->get_outport();
				Vector<int > valid_vcs = get_valid_vcs(invc);
				for(int valid_vc_iter = 0; valid_vc_iter < valid_vcs.size(); valid_vc_iter++)
				{
					if(m_out_vc_state[outport][valid_vcs[valid_vc_iter]]->isInState(IDLE_, g_eventQueue_ptr->getTime()))
					{
						in_vc_state->grant_vc(valid_vcs[valid_vc_iter], g_eventQueue_ptr->getTime());
						m_in_link[inport]->grant_vc_link(invc, g_eventQueue_ptr->getTime());
						m_out_vc_state[outport][valid_vcs[valid_vc_iter]]->setState(VC_AB_, g_eventQueue_ptr->getTime());
						break;
					}
				}
			}
		}
	}
}

Vector<int > Router::get_valid_vcs(int invc)
{
	Vector<int > vc_list;

	for(int vnet = 0; vnet < m_virtual_networks; vnet++)
	{
		if(invc >= (vnet*m_vc_per_vnet) && invc < ((vnet+1)*m_vc_per_vnet))
		{
			int base = vnet*m_vc_per_vnet;
			int vc_per_vnet;
			if(m_net_ptr->isVNetOrdered(vnet))
				vc_per_vnet = 1;		
			else 
				vc_per_vnet = m_vc_per_vnet;

			for(int offset = 0; offset < vc_per_vnet; offset++)
			{
				vc_list.insertAtBottom(base+offset);
			}
			break;	
		}
	}
	return vc_list;
}

void Router::grant_vc(int out_port, int vc, Time grant_time)
{
	assert(m_out_vc_state[out_port][vc]->isInState(VC_AB_, grant_time));
	m_out_vc_state[out_port][vc]->grant_vc(grant_time);
	g_eventQueue_ptr->scheduleEvent(this, 1);
}

void Router::release_vc(int out_port, int vc, Time release_time)
{
	assert(m_out_vc_state[out_port][vc]->isInState(ACTIVE_, release_time));
	m_out_vc_state[out_port][vc]->setState(IDLE_, release_time);
	g_eventQueue_ptr->scheduleEvent(this, 1);
}

// This function calculated the output port for a particular destination. 
int Router::getRoute(NetDest destination)
{
	int output_link = -1;
	int min_weight = INFINITE_;
	for(int link = 0; link < m_routing_table.size(); link++)
	{
		if (destination.intersectionIsNotEmpty(m_routing_table[link]))
		{
			if((m_link_weights[link] >= min_weight))
				continue;
			output_link = link;
			min_weight = m_link_weights[link];
		}
	}
	return output_link;
}

void Router::routeCompute(flit *m_flit, int inport)
{
	int invc = m_flit->get_vc();
	int outport = m_in_vc_state[inport][invc]->get_outport();
	int outvc = m_in_vc_state[inport][invc]->get_outvc();

	assert(NetworkConfig::getNumPipeStages() >= 1);
	m_flit->set_time(g_eventQueue_ptr->getTime() + (NetworkConfig::getNumPipeStages() - 1)); // Becasuse 1 cycle will be consumed in scheduling the output link
	m_flit->set_vc(outvc);
	m_router_buffers[outport][outvc]->insert(m_flit);

	if(NetworkConfig::getNumPipeStages() > 1)
		g_eventQueue_ptr->scheduleEvent(this, NetworkConfig::getNumPipeStages() -1 );
	if((m_flit->get_type() == HEAD_) || (m_flit->get_type() == HEAD_TAIL_))
	{
		NetDest destination = dynamic_cast<NetworkMessage*>(m_flit->get_msg_ptr().ref())->getInternalDestination();
		if(NetworkConfig::getNumPipeStages() > 1)
		{
			m_out_vc_state[outport][outvc]->setState(VC_AB_, g_eventQueue_ptr->getTime() + 1);
			m_out_link[outport]->request_vc_link(outvc, destination, g_eventQueue_ptr->getTime() + 1);	
		}
		else
		{
			m_out_vc_state[outport][outvc]->setState(VC_AB_, g_eventQueue_ptr->getTime());
			m_out_link[outport]->request_vc_link(outvc, destination, g_eventQueue_ptr->getTime());	
		}
	}	
	if((m_flit->get_type() == TAIL_) || (m_flit->get_type() == HEAD_TAIL_))
	{
		m_in_vc_state[inport][invc]->setState(IDLE_, g_eventQueue_ptr->getTime() + 1);
		m_in_link[inport]->release_vc_link(invc, g_eventQueue_ptr->getTime() + 1);
	}
}

void Router::wakeup()
{
	flit *t_flit;
	
	int incoming_port = m_round_robin_start; // This is for round-robin scheduling of incoming ports
	m_round_robin_start++;
    	if (m_round_robin_start >= m_in_link.size()) {
      		m_round_robin_start = 0;
    	}

	for(int port = 0; port < m_in_link.size(); port++)
	{
		// Round robin scheduling
		incoming_port++;
		if(incoming_port >= m_in_link.size())
			incoming_port = 0;
		if(m_in_link[incoming_port]->isReady()) // checking the incoming link
		{
			DEBUG_EXPR(NETWORK_COMP, HighPrio, m_id);	
			DEBUG_EXPR(NETWORK_COMP, HighPrio, g_eventQueue_ptr->getTime());	
			t_flit = m_in_link[incoming_port]->peekLink();
			routeCompute(t_flit, incoming_port);
			m_in_link[incoming_port]->consumeLink();
		}
	}
	scheduleOutputLinks();
	checkReschedule(); // This is for flits lying in the router buffers
	vc_arbitrate();
	check_arbiter_reschedule();
}

void Router::scheduleOutputLinks()
{	
	for(int port = 0; port < m_out_link.size(); port++)
	{	
		int vc_tolookat = m_vc_round_robin[port];
		m_vc_round_robin[port]++;
		if(m_vc_round_robin[port] == m_num_vcs)
			m_vc_round_robin[port] = 0;

		for(int i = 0; i < m_num_vcs; i++)
		{
			vc_tolookat++;
			if(vc_tolookat == m_num_vcs)
			vc_tolookat = 0;

			if(m_router_buffers[port][vc_tolookat]->isReady())
			{
				if(m_out_vc_state[port][vc_tolookat]->isInState(ACTIVE_, g_eventQueue_ptr->getTime()) && m_out_link[port]->isBufferNotFull_link(vc_tolookat)) 
				// models buffer backpressure
				{	
					flit *t_flit = m_router_buffers[port][vc_tolookat]->getTopFlit();
					t_flit->set_time(g_eventQueue_ptr->getTime() + 1 );
					m_out_src_queue[port]->insert(t_flit);
					g_eventQueue_ptr->scheduleEvent(m_out_link[port], 1);
					break; // done for this port
				}
			}
		}
	}
}

void Router::checkReschedule()
{
	for(int port = 0; port < m_out_link.size(); port++)
	{
		for(int vc = 0; vc < m_num_vcs; vc++)
		{
			if(m_router_buffers[port][vc]->isReadyForNext())
			{
				g_eventQueue_ptr->scheduleEvent(this, 1);
				return;
			}
		}
	}
}

void Router::check_arbiter_reschedule()
{
	for(int port = 0; port < m_in_link.size(); port++)
	{
		for(int vc = 0; vc < m_num_vcs; vc++)
		{
			if(m_in_vc_state[port][vc]->isInState(VC_AB_, g_eventQueue_ptr->getTime() + 1))
			{
				g_eventQueue_ptr->scheduleEvent(m_vc_arbiter, 1);
				return;
			}
		}
	}
}

void Router::printConfig(ostream& out) const
{
	out << "[Router " << m_id << "] :: " << endl;
	out << "[inLink - ";
	for(int i = 0;i < m_in_link.size(); i++)
		out << m_in_link[i]->get_id() << " - ";
	out << "]" << endl;
	out << "[outLink - ";
	for(int i = 0;i < m_out_link.size(); i++)
		out << m_out_link[i]->get_id() << " - ";
	out << "]" << endl;
/*	out << "---------- routing table -------------" << endl;
	for(int i =0; i < m_routing_table.size(); i++)
		out << m_routing_table[i] << endl;
*/
}

void Router::print(ostream& out) const
{
	out << "[Router]";
}
