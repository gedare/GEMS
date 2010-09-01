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
 * InputUnit_d.C
 *
 * Niket Agarwal, Princeton University
 *
 * */

#include "InputUnit_d.h"
#include "Router_d.h"

InputUnit_d::InputUnit_d(int id, Router_d *router)
{
	m_id = id;
	m_router = router;
	m_num_vcs = m_router->get_num_vcs();
	
	m_num_buffer_reads = 0;
	m_num_buffer_writes = 0;

	creditQueue = new flitBuffer_d();
	// Instantiating the virtual channels
	m_vcs.setSize(m_num_vcs);
	for(int i=0; i < m_num_vcs; i++)
	{	
		m_vcs[i] = new VirtualChannel_d(i);
	}
}
	
InputUnit_d::~InputUnit_d()
{
	delete creditQueue;
	m_vcs.deletePointers();
}

void InputUnit_d::wakeup()
{
	flit_d *t_flit;
	if(m_in_link->isReady())
	{
		t_flit = m_in_link->consumeLink();
		int vc = t_flit->get_vc();
		if((t_flit->get_type() == HEAD_) || (t_flit->get_type() == HEAD_TAIL_))
                {
			assert(m_vcs[vc]->get_state() == IDLE_);
			m_router->route_req(t_flit, this, vc); // Do the route computation for this vc        
			m_vcs[vc]->set_enqueue_time(g_eventQueue_ptr->getTime());
                }
                else
                {
                        t_flit->advance_stage(SA_);
			m_router->swarb_req();
                }
		m_vcs[vc]->insertFlit(t_flit);   // write flit into input buffer
		m_num_buffer_writes++;
		m_num_buffer_reads++; // same as read because any flit that is written will be read only once
	}
}


void InputUnit_d::printConfig(ostream& out)
{
	out << endl;
	out << "InputUnit Configuration" << endl;
	out << "---------------------" << endl;
	out << "id = " << m_id << endl;
	out << "In link is " << m_in_link->get_id() << endl;
}
