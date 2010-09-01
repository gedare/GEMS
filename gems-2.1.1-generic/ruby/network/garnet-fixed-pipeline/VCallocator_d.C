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
 * VCallocator_d.C
 *
 * Niket Agarwal, Princeton University
 *
 * */

#include "NetworkConfig.h"
#include "VCallocator_d.h"
#include "Router_d.h"
#include "InputUnit_d.h"
#include "OutputUnit_d.h"
#include "GarnetNetwork_d.h"

VCallocator_d::VCallocator_d(Router_d *router)
{
	m_router = router;
	m_num_vcs = m_router->get_num_vcs();
	m_vc_per_vnet = m_router->get_vc_per_vnet();
	m_local_arbiter_activity = 0;
	m_global_arbiter_activity = 0;
}

void VCallocator_d::init()
{
	m_input_unit = m_router->get_inputUnit_ref();
	m_output_unit = m_router->get_outputUnit_ref();

	m_num_inports = m_router->get_num_inports();
	m_num_outports = m_router->get_num_outports();
	m_round_robin_invc.setSize(m_num_inports); 
	m_round_robin_outvc.setSize(m_num_outports); 
	m_outvc_req.setSize(m_num_outports); 
	m_outvc_is_req.setSize(m_num_outports); 
	
	for(int i = 0; i < m_num_inports; i++)
	{
		m_round_robin_invc[i].setSize(m_num_vcs);

		for(int j = 0; j < m_num_vcs; j++)
		{
			m_round_robin_invc[i][j] = 0;
		}
	}

	for(int i = 0; i < m_num_outports; i++)
	{
		m_round_robin_outvc[i].setSize(m_num_vcs);
		m_outvc_req[i].setSize(m_num_vcs); 
		m_outvc_is_req[i].setSize(m_num_vcs); 

		for(int j = 0; j < m_num_vcs; j++)
		{
			m_round_robin_outvc[i][j].first = 0;
			m_round_robin_outvc[i][j].second = 0;
			m_outvc_is_req[i][j] = false; 

			m_outvc_req[i][j].setSize(m_num_inports); 

			for(int k = 0; k < m_num_inports; k++)
			{
				m_outvc_req[i][j][k].setSize(m_num_vcs); 
				for(int l = 0; l < m_num_vcs; l++)
				{
					m_outvc_req[i][j][k][l] = false;
				}
			}
		}
	}
}

void VCallocator_d::clear_request_vector()
{
	for(int i = 0; i < m_num_outports; i++)
	{
		for(int j = 0; j < m_num_vcs; j++)
		{
			if(!m_outvc_is_req[i][j])
				continue;
			m_outvc_is_req[i][j] = false;
			for(int k = 0; k < m_num_inports; k++)
			{
				for(int l = 0; l < m_num_vcs; l++)
				{
					m_outvc_req[i][j][k][l] = false;
				}
			}
		}
	}
}

void VCallocator_d::wakeup()
{
	arbitrate_invcs(); // First stage of allocation
	arbitrate_outvcs(); // Second stage of allocation

	clear_request_vector();
        check_for_wakeup();
}

bool VCallocator_d::is_invc_candidate(int inport_iter, int invc_iter)
{
	int outport = m_input_unit[inport_iter]->get_route(invc_iter);
	int vnet = get_vnet(invc_iter);
	int t_enqueue_time = m_input_unit[inport_iter]->get_enqueue_time(invc_iter);

	int invc_base = vnet*m_vc_per_vnet;

	if((m_router->get_net_ptr())->isVNetOrdered(vnet))
	{
		for(int vc_offset = 0; vc_offset < m_vc_per_vnet; vc_offset++)
		{
			int temp_vc = invc_base + vc_offset;
			if(m_input_unit[inport_iter]->need_stage(temp_vc, VC_AB_, VA_) && (m_input_unit[inport_iter]->get_route(temp_vc) == outport) && (m_input_unit[inport_iter]->get_enqueue_time(temp_vc) < t_enqueue_time))
			{
				return false;
			}
		}
	}
	return true;
}

void VCallocator_d::select_outvc(int inport_iter, int invc_iter)
{
	int outport = m_input_unit[inport_iter]->get_route(invc_iter);
	int vnet = get_vnet(invc_iter);
	int outvc_base = vnet*m_vc_per_vnet;
	int num_vcs_per_vnet = m_vc_per_vnet;

	int outvc_offset = m_round_robin_invc[inport_iter][invc_iter];
	m_round_robin_invc[inport_iter][invc_iter]++;

	if(m_round_robin_invc[inport_iter][invc_iter] >= num_vcs_per_vnet)
		m_round_robin_invc[inport_iter][invc_iter] = 0;

	for(int outvc_offset_iter = 0; outvc_offset_iter < num_vcs_per_vnet; outvc_offset_iter++)
	{
		outvc_offset++;
		if(outvc_offset >= num_vcs_per_vnet)
			outvc_offset = 0;
		int outvc = outvc_base + outvc_offset;	
		if(m_output_unit[outport]->is_vc_idle(outvc))
		{
			m_local_arbiter_activity++;
			m_outvc_req[outport][outvc][inport_iter][invc_iter] = true;
			if(!m_outvc_is_req[outport][outvc])
				m_outvc_is_req[outport][outvc] = true;
			return; // out vc acquired	
		}
	}
}

void VCallocator_d::arbitrate_invcs()
{
        for(int inport_iter = 0; inport_iter < m_num_inports; inport_iter++)
        {
                for(int invc_iter = 0; invc_iter < m_num_vcs; invc_iter++)
		{
			if(m_input_unit[inport_iter]->need_stage(invc_iter, VC_AB_, VA_))
			{
				if(!is_invc_candidate(inport_iter, invc_iter))
					continue;
				
				select_outvc(inport_iter, invc_iter);
			}
                }
        }
}

void VCallocator_d::arbitrate_outvcs()
{
        for(int outport_iter = 0; outport_iter < m_num_outports; outport_iter++)
        {
                for(int outvc_iter = 0; outvc_iter < m_num_vcs; outvc_iter++)
		{
			if(!m_outvc_is_req[outport_iter][outvc_iter]) // No requests for this outvc in this cycle
				continue;

			int inport = m_round_robin_outvc[outport_iter][outvc_iter].first;
			int invc_offset = m_round_robin_outvc[outport_iter][outvc_iter].second;
			int vnet = get_vnet(outvc_iter);
			int invc_base = vnet*m_vc_per_vnet;
			int num_vcs_per_vnet = m_vc_per_vnet;

			m_round_robin_outvc[outport_iter][outvc_iter].second++;	
			if(m_round_robin_outvc[outport_iter][outvc_iter].second >= num_vcs_per_vnet)
			{
				m_round_robin_outvc[outport_iter][outvc_iter].second = 0;
				m_round_robin_outvc[outport_iter][outvc_iter].first++;
				if(m_round_robin_outvc[outport_iter][outvc_iter].first >= m_num_inports)
					m_round_robin_outvc[outport_iter][outvc_iter].first = 0;
			}
        		for(int in_iter = 0; in_iter < m_num_inports*num_vcs_per_vnet; in_iter++)
			{
				invc_offset++;
				if(invc_offset >= num_vcs_per_vnet)
				{
					invc_offset = 0;
					inport++;
					if(inport >= m_num_inports)
						inport = 0;
				}
				int invc = invc_base + invc_offset;
				if(m_outvc_req[outport_iter][outvc_iter][inport][invc])
				{
					m_global_arbiter_activity++;
					m_input_unit[inport]->grant_vc(invc, outvc_iter);
					m_output_unit[outport_iter]->update_vc(outvc_iter, inport, invc);
					m_router->swarb_req();
					break;
				}
			}
		}
	}
}

int VCallocator_d::get_vnet(int invc)
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

void VCallocator_d::check_for_wakeup()
{
        for(int i = 0; i < m_num_inports; i++)
	{
		for(int j = 0; j < m_num_vcs; j++)
		{
			if(m_input_unit[i]->need_stage_nextcycle(j, VC_AB_, VA_))
			{
				g_eventQueue_ptr->scheduleEvent(this, 1);
				return;
			}
		}
	}
}
