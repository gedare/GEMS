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
 * Switch_d.C
 *
 * Niket Agarwal, Princeton University
 *
 * */

#include "Switch_d.h"
#include "Router_d.h"
#include "OutputUnit_d.h"

Switch_d::Switch_d(Router_d *router)
{
	m_router = router;
	m_num_vcs = m_router->get_num_vcs();
	m_crossbar_activity = 0;
}

Switch_d::~Switch_d()
{
	m_switch_buffer.deletePointers();
}

void Switch_d::init()
{
	m_output_unit = m_router->get_outputUnit_ref();

	m_num_inports = m_router->get_num_inports();
	m_switch_buffer.setSize(m_num_inports);
	for(int i = 0; i < m_num_inports; i++)
	{
		m_switch_buffer[i] = new flitBuffer_d();
	}
}

void Switch_d::wakeup()
{
	DEBUG_MSG(NETWORK_COMP, HighPrio, "Switch woke up");	
	DEBUG_EXPR(NETWORK_COMP, HighPrio, g_eventQueue_ptr->getTime());	

	for(int inport = 0; inport < m_num_inports; inport++)
        {
		if(!m_switch_buffer[inport]->isReady())
			continue;
                flit_d *t_flit = m_switch_buffer[inport]->peekTopFlit();
                if(t_flit->is_stage(ST_))
                {
                        int outport = t_flit->get_outport();
                        t_flit->advance_stage(LT_);
			t_flit->set_time(g_eventQueue_ptr->getTime() + 1);
                        m_output_unit[outport]->insert_flit(t_flit); // This will take care of waking up the Network Link
                        m_switch_buffer[inport]->getTopFlit();
			m_crossbar_activity++;
                }
        }
        check_for_wakeup();
}

void Switch_d::check_for_wakeup()
{
        for(int inport = 0; inport < m_num_inports; inport++)
        {
                if(m_switch_buffer[inport]->isReadyForNext())
                {
                        g_eventQueue_ptr->scheduleEvent(this, 1);
                        break;
                }
        }
}

