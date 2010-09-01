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
 * Routingunit_d.C
 *
 * Niket Agarwal, Princeton University
 *
 * */

#include "RoutingUnit_d.h"
#include "Router_d.h"
#include "InputUnit_d.h"
#include "NetworkMessage.h"

RoutingUnit_d::RoutingUnit_d(Router_d *router)
{
	m_router = router;
	m_routing_table.clear();
	m_weight_table.clear();
}

void RoutingUnit_d::addRoute(const NetDest& routing_table_entry)
{
	m_routing_table.insertAtBottom(routing_table_entry);
}

void RoutingUnit_d::addWeight(int link_weight)
{
	m_weight_table.insertAtBottom(link_weight);
}

void RoutingUnit_d::RC_stage(flit_d *t_flit, InputUnit_d *in_unit, int invc)
{
	int outport = routeCompute(t_flit);
	in_unit->updateRoute(invc, outport);
	t_flit->advance_stage(VA_);
	m_router->vcarb_req();
}

int RoutingUnit_d::routeCompute(flit_d *t_flit)
{
	MsgPtr msg_ptr = t_flit->get_msg_ptr();
	NetworkMessage* net_msg_ptr = NULL;
        net_msg_ptr = dynamic_cast<NetworkMessage*>(msg_ptr.ref());
        NetDest msg_destination = net_msg_ptr->getInternalDestination();

	int output_link = -1;
	int min_weight = INFINITE_;

	for(int link = 0; link < m_routing_table.size(); link++)
        {
                if (msg_destination.intersectionIsNotEmpty(m_routing_table[link]))
                {
			if(m_weight_table[link] >= min_weight)
				continue;
			output_link = link;
			min_weight = m_weight_table[link];
		}
	}
	if(output_link == -1)
	{
		ERROR_MSG("Fatal Error:: No Route exists from this Router.");
		exit(0);
	}
	return output_link;

}
