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
 * VirtualChannel_d.C
 *
 * Niket Agarwal, Princeton University
 *
 * */

#include "VirtualChannel_d.h"

VirtualChannel_d::VirtualChannel_d(int id)
{
	m_id = id;
	m_input_buffer = new flitBuffer_d();
	m_vc_state.first = IDLE_;
	m_vc_state.second = g_eventQueue_ptr->getTime();
	m_enqueue_time = INFINITE_;
}

VirtualChannel_d::~VirtualChannel_d()
{
	delete m_input_buffer;
}

void VirtualChannel_d::set_outport(int outport)
{
	route = outport;
}

void VirtualChannel_d::grant_vc(int out_vc)
{
	m_output_vc = out_vc;
	m_vc_state.first = ACTIVE_;
	m_vc_state.second = g_eventQueue_ptr->getTime() + 1;
	flit_d *t_flit = m_input_buffer->peekTopFlit();
        t_flit->advance_stage(SA_);
}

bool VirtualChannel_d::need_stage(VC_state_type state, flit_stage stage)
{
	if((m_vc_state.first == state) && (g_eventQueue_ptr->getTime() >= m_vc_state.second))
        {
                if(m_input_buffer->isReady())
                {
                        flit_d *t_flit = m_input_buffer->peekTopFlit();
                        return(t_flit->is_stage(stage)) ;
                }
        }
        return false;

}

bool VirtualChannel_d::need_stage_nextcycle(VC_state_type state, flit_stage stage)
{
	if((m_vc_state.first == state) && ((g_eventQueue_ptr->getTime()+1) >= m_vc_state.second))
        {
                if(m_input_buffer->isReadyForNext())
                {
                        flit_d *t_flit = m_input_buffer->peekTopFlit();
                        return(t_flit->is_next_stage(stage)) ;
                }
        }
        return false;
}

