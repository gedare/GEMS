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
 * InputUnit_d.h
 *
 * Niket Agarwal, Princeton University
 *
 * */

#ifndef INPUT_UNIT_D_H
#define INPUT_UNIT_D_H

#include "NetworkHeader.h"
#include "flitBuffer_d.h"
#include "Consumer.h"
#include "Vector.h"
#include "VirtualChannel_d.h"
#include "NetworkLink_d.h"
#include "CreditLink_d.h"

class Router_d;

class InputUnit_d : public Consumer {
public:
	InputUnit_d(int id, Router_d *router);
	~InputUnit_d();

	void wakeup();
	void printConfig(ostream& out);
	flitBuffer_d* getCreditQueue() { return creditQueue; }
	void print(ostream& out) const {};

	inline int get_inlink_id()
	{
		return m_in_link->get_id();
	}

	inline void set_vc_state(VC_state_type state, int vc)
	{
		m_vcs[vc]->set_state(state);
	}
	inline void set_enqueue_time(int invc, Time time)
	{
		m_vcs[invc]->set_enqueue_time(time);
	}
	inline Time get_enqueue_time(int invc)
	{
		return m_vcs[invc]->get_enqueue_time();
	}
	inline void update_credit(int in_vc, int credit)
	{
		m_vcs[in_vc]->update_credit(credit);
	}

	inline bool has_credits(int vc)
	{
		return m_vcs[vc]->has_credits();
	}

	inline void increment_credit(int in_vc, bool free_signal)
	{
		flit_d *t_flit = new flit_d(in_vc, free_signal);
		creditQueue->insert(t_flit);
		g_eventQueue_ptr->scheduleEvent(m_credit_link, 1);
	}

	inline int get_outvc(int invc)
	{
		return m_vcs[invc]->get_outvc();
	}

	inline void updateRoute(int vc, int outport)
	{
		m_vcs[vc]->set_outport(outport);
		m_vcs[vc]->set_state(VC_AB_);
	}

	inline void grant_vc(int in_vc, int out_vc)
	{
		m_vcs[in_vc]->grant_vc(out_vc);
	}

	inline flit_d* peekTopFlit(int vc)
	{
		return m_vcs[vc]->peekTopFlit();
	}

	inline flit_d* getTopFlit(int vc)
	{
		return m_vcs[vc]->getTopFlit();
	}

	inline bool need_stage(int vc, VC_state_type state, flit_stage stage)
	{
		return m_vcs[vc]->need_stage(state, stage);
	}

	inline bool need_stage_nextcycle(int vc, VC_state_type state, flit_stage stage)
	{
		return m_vcs[vc]->need_stage_nextcycle(state, stage);
	}

	inline bool isReady(int invc)
	{
		return m_vcs[invc]->isReady();
	}

	inline int get_route(int vc)
	{
		return m_vcs[vc]->get_route();
	}
	inline void set_in_link(NetworkLink_d *link)
	{
		m_in_link = link;
	}

	inline void set_credit_link(CreditLink_d *credit_link)
	{
		m_credit_link = credit_link;
	}
	
	inline double get_buf_read_count()
	{
		return m_num_buffer_reads;
	}

	inline double get_buf_write_count()
	{
		return m_num_buffer_writes;
	}

private:
	int m_id;
	int m_num_vcs;
	double m_num_buffer_writes, m_num_buffer_reads;

	Router_d *m_router;
	NetworkLink_d *m_in_link;
	CreditLink_d *m_credit_link;
	flitBuffer_d *creditQueue;

	// Virtual channels
	Vector<VirtualChannel_d *> m_vcs; // [vc]
};

#endif
