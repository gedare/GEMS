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
 * SWallocator_d.C
 *
 * Niket Agarwal, Princeton University
 *
 * */

#include "SWallocator_d.h"
#include "Router_d.h"
#include "InputUnit_d.h"
#include "OutputUnit_d.h"
#include "GarnetNetwork_d.h"

SWallocator_d::SWallocator_d(Router_d *router)
{
	m_router = router;
	m_num_vcs = m_router->get_num_vcs();
	m_vc_per_vnet = m_router->get_vc_per_vnet();

	m_local_arbiter_activity = 0;
	m_global_arbiter_activity = 0;
}

void SWallocator_d::init()
{
	m_input_unit = m_router->get_inputUnit_ref();
	m_output_unit = m_router->get_outputUnit_ref();

	m_num_inports = m_router->get_num_inports();
	m_num_outports = m_router->get_num_outports();
	m_round_robin_outport.setSize(m_num_outports);
	m_round_robin_inport.setSize(m_num_inports);
	m_port_req.setSize(m_num_outports);
	m_vc_winners.setSize(m_num_outports);

	for(int i = 0; i < m_num_inports; i++)
	{
		m_round_robin_inport[i] = 0;
	}

	for(int i = 0; i < m_num_outports; i++)
	{
		m_port_req[i].setSize(m_num_inports);
		m_vc_winners[i].setSize(m_num_inports);

		m_round_robin_outport[i] = 0;
		
		for(int j = 0; j < m_num_inports; j++)
		{
			m_port_req[i][j] = false; // [outport][inport]
		}
	}
}

void SWallocator_d::wakeup()
{
	arbitrate_inports(); // First stage of allocation
	arbitrate_outports(); // Second stage of allocation

	clear_request_vector();
        check_for_wakeup();

}

void SWallocator_d::arbitrate_inports()
{
	// First I will do round robin arbitration on a set of input vc requests
        for(int inport = 0; inport < m_num_inports; inport++)
        {
                int invc = m_round_robin_inport[inport];
                m_round_robin_inport[inport]++;

                if(m_round_robin_inport[inport] >= m_num_vcs)
                        m_round_robin_inport[inport] = 0;
                for(int j = 0; j < m_num_vcs; j++)
		{
			invc++;
			if(invc >= m_num_vcs)
				invc = 0;
			if(m_input_unit[inport]->need_stage(invc, ACTIVE_, SA_) && m_input_unit[inport]->has_credits(invc))
			{
				if(is_candidate_inport(inport, invc))
				{
					int outport = m_input_unit[inport]->get_route(invc);
					m_local_arbiter_activity++;
					m_port_req[outport][inport] = true;
					m_vc_winners[outport][inport]= invc;
					break; // got one vc winner for this port
				}
			}
		}
        }
}	

bool SWallocator_d::is_candidate_inport(int inport, int invc)
{
	int outport = m_input_unit[inport]->get_route(invc);
	int t_enqueue_time = m_input_unit[inport]->get_enqueue_time(invc);
	int t_vnet = get_vnet(invc);
	int vc_base = t_vnet*m_vc_per_vnet;
	if((m_router->get_net_ptr())->isVNetOrdered(t_vnet))
	{
		for(int vc_offset = 0; vc_offset < m_vc_per_vnet; vc_offset++)
		{
			int temp_vc = vc_base + vc_offset;
			if(m_input_unit[inport]->need_stage(temp_vc, ACTIVE_, SA_) && (m_input_unit[inport]->get_route(temp_vc) == outport) && (m_input_unit[inport]->get_enqueue_time(temp_vc) < t_enqueue_time))
			{
				return false;		
				break;
			}
		}
	}
	return true;
}


void SWallocator_d::arbitrate_outports()
{
// now I have a set of input vc requests for output vcs. Again do round robin arbitration on these requests
        for(int outport = 0; outport < m_num_outports; outport++)
	{
		int in_port = m_round_robin_outport[outport];
		m_round_robin_outport[outport]++;

		if(m_round_robin_outport[outport] >= m_num_outports)
			m_round_robin_outport[outport] = 0;

		for(int inport = 0; inport < m_num_inports; inport++)
		{
			in_port++;
			if(in_port >= m_num_inports)
				in_port = 0;
			if(m_port_req[outport][in_port]) // This Inport has a request this cycle for this port
			{
				m_port_req[outport][in_port] = false;
				int invc = m_vc_winners[outport][in_port];
				int outvc = m_input_unit[in_port]->get_outvc(invc);
				flit_d *t_flit = m_input_unit[in_port]->getTopFlit(invc); // removes flit from Input Unit
				t_flit->advance_stage(ST_);
				t_flit->set_vc(outvc);
				t_flit->set_outport(outport);
				t_flit->set_time(g_eventQueue_ptr->getTime() + 1);
				m_output_unit[outport]->decrement_credit(outvc);
				m_router->update_sw_winner(in_port, t_flit);
				m_global_arbiter_activity++;

				if((t_flit->get_type() == TAIL_) || t_flit->get_type() == HEAD_TAIL_)
				{
					m_input_unit[in_port]->increment_credit(invc, true); // Send a credit back along with the information that this VC is not idle
					assert(m_input_unit[in_port]->isReady(invc) == false); // This Input VC should now be empty

					m_input_unit[in_port]->set_vc_state(IDLE_, invc);
					m_input_unit[in_port]->set_enqueue_time(invc, INFINITE_);	
				}
				else 
				{
					m_input_unit[in_port]->increment_credit(invc, false); // Send a credit back but do not indicate that the VC is idle
				}
				break; // got a in request for this outport
			}
		}
	}
}
	
void SWallocator_d::check_for_wakeup()
{
        for(int i = 0; i < m_num_inports; i++)
	{
		for(int j = 0; j < m_num_vcs; j++)
		{
			if(m_input_unit[i]->need_stage_nextcycle(j, ACTIVE_, SA_))
			{
				g_eventQueue_ptr->scheduleEvent(this, 1);
				return;
			}
		}
	}
}

int SWallocator_d::get_vnet(int invc)
{
	for(int i = 0; i < NUMBER_OF_VIRTUAL_NETWORKS; i++)
	{
		if(invc >= (i*m_vc_per_vnet) && invc < ((i+1)*m_vc_per_vnet))
		{
			return i;
		}
	}
	ERROR_MSG("Could not determine vc");
	return -1;
}

void SWallocator_d::clear_request_vector()
{
        for(int i = 0; i < m_num_outports; i++)
        {
	        for(int j = 0; j < m_num_inports; j++)
                {
		        m_port_req[i][j] = false;
		}
	}
}	
