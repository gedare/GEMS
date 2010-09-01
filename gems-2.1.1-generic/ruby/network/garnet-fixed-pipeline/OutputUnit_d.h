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
 * OutputUnit_d.h
 *
 * Niket Agarwal, Princeton University
 *
 * */

#ifndef OUTPUT_UNIT_D_H
#define OUTPUT_UNIT_D_H

#include "NetworkHeader.h"
#include "Consumer.h"
#include "flitBuffer_d.h"
#include "OutVcState_d.h"
#include "NetworkLink_d.h"
#include "CreditLink_d.h"

class Router_d;

class OutputUnit_d : public Consumer {
public:
	OutputUnit_d(int id, Router_d *router);
	~OutputUnit_d();
	void set_out_link(NetworkLink_d *link);
	void set_credit_link(CreditLink_d *credit_link);
	void wakeup();
	flitBuffer_d* getOutQueue();
	void printConfig(ostream& out);
	void update_vc(int vc, int in_port, int in_vc);
  	void print(ostream& out) const {};
	int get_credit_cnt(int vc) { return m_outvc_state[vc]->get_credit_count(); }
	void decrement_credit(int out_vc);

	inline int get_outlink_id()
	{
		return m_out_link->get_id();
	}

	inline void set_vc_state(VC_state_type state, int vc)
	{
		m_outvc_state[vc]->setState(state, g_eventQueue_ptr->getTime() + 1);
	}
	inline bool is_vc_idle(int vc)
	{
		return (m_outvc_state[vc]->isInState(IDLE_, g_eventQueue_ptr->getTime())	);
	}
	inline void insert_flit(flit_d *t_flit)
	{
       		m_out_buffer->insert(t_flit);
        	g_eventQueue_ptr->scheduleEvent(m_out_link, 1);
	}



private:
	int m_id;
	int m_num_vcs;
	Router_d *m_router;
	NetworkLink_d *m_out_link;
	CreditLink_d *m_credit_link;

	flitBuffer_d *m_out_buffer; // This is for the network link to consume
        Vector<OutVcState_d *> m_outvc_state; // vc state of downstream router

};

#endif
