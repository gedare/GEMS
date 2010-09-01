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
 * VirtualChannel_d.h
 *
 * Niket Agarwal, Princeton University
 *
 * */

#ifndef VIRTUAL_CHANNEL_D_H
#define VIRTUAL_CHANNEL_D_H

#include "NetworkHeader.h"
#include "flitBuffer_d.h"

class VirtualChannel_d {
public:
	VirtualChannel_d(int id);
	~VirtualChannel_d();

	bool need_stage(VC_state_type state, flit_stage stage);
	bool need_stage_nextcycle(VC_state_type state, flit_stage stage);
	void set_outport(int outport);
	void grant_vc(int out_vc);

	inline Time get_enqueue_time()
	{
		return m_enqueue_time;
	}
	
	inline void set_enqueue_time(Time time)
	{
		m_enqueue_time = time;
	}

	inline VC_state_type get_state() 
	{ 
		return m_vc_state.first; 
	}
	inline int get_outvc() 
	{ 
		return m_output_vc; 
	}
	inline bool isReady() 
	{ 
		return m_input_buffer->isReady(); 
	}	
	inline bool has_credits() 
	{ 
		return (m_credit_count > 0); 
	}
	inline int get_route() 
	{ 
		return route; 
	}
	inline void update_credit(int credit)
	{
		m_credit_count = credit;	
	}
	inline void increment_credit()
	{
		m_credit_count++;	
	}
	inline void insertFlit(flit_d *t_flit)
	{
		m_input_buffer->insert(t_flit);
	}
	inline void set_state(VC_state_type m_state)
	{
		m_vc_state.first = m_state;
		m_vc_state.second = g_eventQueue_ptr->getTime() + 1;
	}

	inline flit_d* peekTopFlit()
	{
		return m_input_buffer->peekTopFlit();
	}

	inline flit_d* getTopFlit()
	{
		return m_input_buffer->getTopFlit();
	}

private:
	int m_id;
	flitBuffer_d *m_input_buffer;
	pair<VC_state_type, Time> m_vc_state; // I/R/V/A/C
	int route;
	Time m_enqueue_time;
	int m_output_vc;
	int m_credit_count;
};
#endif
