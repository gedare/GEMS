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
 * flit_d.C
 *
 * Niket Agarwal, Princeton University
 *
 * */

#include "flit_d.h"

flit_d::flit_d(int id, int  vc, int vnet, int size, MsgPtr msg_ptr)
{
	m_size = size;
	m_msg_ptr = msg_ptr;
	m_enqueue_time = g_eventQueue_ptr->getTime();
	m_time = g_eventQueue_ptr->getTime();
	m_id = id;
	m_vnet = vnet;
	m_vc = vc;
	m_stage.first = I_;
	m_stage.second = m_time;
	
	if(size == 1)
	{
		m_type = HEAD_TAIL_;
		return;
	}
	if(id == 0)
		m_type = HEAD_;
	else if(id == (size - 1)) 
		m_type = TAIL_;
	else 
		m_type = BODY_;
}

flit_d::flit_d(int vc, bool is_free_signal)
{
	m_id = 0;
	m_vc = vc;
	m_is_free_signal = is_free_signal;
	m_time = g_eventQueue_ptr->getTime();
}
/*
int flit_d::get_size()
{
	return m_size;
}
Time flit_d::get_enqueue_time()
{
	return m_enqueue_time;
}
int flit_d::get_id()
{
	return m_id;
}
Time flit_d::get_time()
{
	return m_time;
}
void flit_d::set_time(Time time)
{
	m_time = time;
}
int flit_d::get_vnet()
{
	return m_vnet;
}
int flit_d::get_vc()
{
	return m_vc;
}
void flit_d::set_vc(int vc)
{
	m_vc = vc;
}
MsgPtr& flit_d::get_msg_ptr()
{
	return m_msg_ptr;
}
flit_type flit_d::get_type()
{
	return m_type;
}
bool flit_d::is_stage(flit_stage t_stage)
{
	return ((m_stage.first == t_stage) && (g_eventQueue_ptr->getTime() >= m_stage.second));
}
bool flit_d::is_next_stage(flit_stage t_stage)
{
	return ((m_stage.first == t_stage) && ((g_eventQueue_ptr->getTime()+1) >= m_stage.second));
}
void flit_d::advance_stage(flit_stage t_stage)
{
	m_stage.first = t_stage;
	m_stage.second = g_eventQueue_ptr->getTime() + 1;
}
*/
void flit_d::print(ostream& out) const
{
	out << "[flit:: ";
	out << "Id=" << m_id << " ";
	out << "Type=" << m_type << " ";
	out << "Vnet=" << m_vnet << " ";
	out << "VC=" << m_vc << " ";
	out << "Enqueue Time=" << m_enqueue_time << " ";
	out << "]";
}
