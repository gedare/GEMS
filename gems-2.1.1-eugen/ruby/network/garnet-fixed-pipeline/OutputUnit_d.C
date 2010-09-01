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
 * OutputUnit_d.C
 *
 * Niket Agarwal, Princeton University
 *
 * */

#include "OutputUnit_d.h"
#include "Router_d.h"
#include "NetworkConfig.h"

OutputUnit_d::OutputUnit_d(int id, Router_d *router)
{
	m_id = id;
	m_router = router;
	m_num_vcs = m_router->get_num_vcs();
	m_out_buffer = new flitBuffer_d();

	for(int i = 0; i < m_num_vcs; i++)
	{
		m_outvc_state.insertAtBottom(new OutVcState_d(i));
	}
}

OutputUnit_d::~OutputUnit_d()
{
	delete m_out_buffer;
	m_outvc_state.deletePointers();
}

void OutputUnit_d::decrement_credit(int out_vc)
{
	m_outvc_state[out_vc]->decrement_credit();
	m_router->update_incredit(m_outvc_state[out_vc]->get_inport(), m_outvc_state[out_vc]->get_invc(), m_outvc_state[out_vc]->get_credit_count());
}

void OutputUnit_d::wakeup()
{
	if(m_credit_link->isReady())
	{
		flit_d *t_flit = m_credit_link->consumeLink();
		int out_vc = t_flit->get_vc();
		m_outvc_state[out_vc]->increment_credit();
		m_router->update_incredit(m_outvc_state[out_vc]->get_inport(), m_outvc_state[out_vc]->get_invc(), m_outvc_state[out_vc]->get_credit_count());

		if(t_flit->is_free_signal())
			set_vc_state(IDLE_, out_vc);				

		delete t_flit;
	}
}

flitBuffer_d* OutputUnit_d::getOutQueue()
{
	return m_out_buffer;
}	

void OutputUnit_d::set_out_link(NetworkLink_d *link)
{
	m_out_link = link;
}

void OutputUnit_d::set_credit_link(CreditLink_d *credit_link)
{
	m_credit_link = credit_link;
}

void OutputUnit_d::update_vc(int vc, int in_port, int in_vc)
{
        m_outvc_state[vc]->setState(ACTIVE_, g_eventQueue_ptr->getTime() + 1);
        m_outvc_state[vc]->set_inport(in_port);
        m_outvc_state[vc]->set_invc(in_vc);
        m_router->update_incredit(in_port, in_vc, m_outvc_state[vc]->get_credit_count());
}

void OutputUnit_d::printConfig(ostream& out)
{
	out << endl;
	out << "OutputUnit Configuration" << endl;
	out << "---------------------" << endl;
	out << "id = " << m_id << endl;
	out << "Out link is " << m_out_link->get_id() << endl;
}
