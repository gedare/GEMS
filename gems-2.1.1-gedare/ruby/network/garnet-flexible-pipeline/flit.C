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
 * flit.C
 *
 * Niket Agarwal, Princeton University
 *
 * */

#include "flit.h"

flit::flit(int id, int  vc, int vnet, int size, MsgPtr msg_ptr)
{
	m_size = size;
	m_msg_ptr = msg_ptr;
	m_enqueue_time = g_eventQueue_ptr->getTime();
	m_time = g_eventQueue_ptr->getTime();
	m_id = id;
	m_vnet = vnet;
	m_vc = vc;
	
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

int flit::get_size()
{
	return m_size;
}
int flit::get_id()
{
	return m_id;
}
Time flit::get_time()
{
	return m_time;
}

Time flit::get_enqueue_time()
{
	return m_enqueue_time;
}
void flit::set_time(Time time)
{
	m_time = time;
}

int flit::get_vnet()
{
	return m_vnet;
}
int flit::get_vc()
{
	return m_vc;
}
void flit::set_vc(int vc)
{
	m_vc = vc;
}
MsgPtr& flit::get_msg_ptr()
{
	return m_msg_ptr;
}
flit_type flit::get_type()
{
	return m_type;
}

void flit::print(ostream& out) const
{
	out << "[flit:: ";
	out << "Id=" << m_id << " ";
	out << "Type=" << m_type << " ";
	out << "Vnet=" << m_vnet << " ";
	out << "VC=" << m_vc << " ";
	out << "Enqueue Time=" << m_enqueue_time << " ";
	out << "]";
}
