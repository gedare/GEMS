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
 * flit_d.h
 *
 * Niket Agarwal, Princeton University
 *
 * */


#ifndef FLIT_D_H
#define FLIT_D_H

#include "NetworkHeader.h"
#include "Message.h"

class flit_d {
public:
	flit_d(int id, int vc, int vnet, int size, MsgPtr msg_ptr);
	flit_d(int vc, bool is_free_signal);
	void set_outport(int port) { m_outport = port; }
	int get_outport() {return m_outport; }
	void print(ostream& out) const;
	bool is_free_signal()
	{
		return m_is_free_signal;
	}

	inline int get_size()
	{
		return m_size;
	}
	inline Time get_enqueue_time()
	{
		return m_enqueue_time;
	}
	inline int get_id()
	{	
		return m_id;
	}
	inline Time get_time()
	{
		return m_time;
	}
	inline void set_time(Time time)
	{
		m_time = time;
	}
	inline int get_vnet()
	{
		return m_vnet;
	}
	inline int get_vc()
	{
		return m_vc;
	}
	inline void set_vc(int vc)
	{
		m_vc = vc;
	}
	inline MsgPtr& get_msg_ptr()
	{
		return m_msg_ptr;
	}
	inline flit_type get_type()
	{
		return m_type;
	}
	inline bool is_stage(flit_stage t_stage)
	{
		return ((m_stage.first == t_stage) && (g_eventQueue_ptr->getTime() >= m_stage.second));
	}
	inline bool is_next_stage(flit_stage t_stage)
	{
		return ((m_stage.first == t_stage) && ((g_eventQueue_ptr->getTime()+1) >= m_stage.second));
	}
	inline void advance_stage(flit_stage t_stage)
	{
		m_stage.first = t_stage;
		m_stage.second = g_eventQueue_ptr->getTime() + 1;
	}
	inline pair<flit_stage, Time> get_stage()
	{
		return m_stage;
	}
	inline void set_delay(int delay)
	{
		src_delay = delay;
	}

	inline int get_delay()
	{
		return src_delay;
	}


private:
	/************Data Members*************/
	int m_id;
	int m_vnet;
	int m_vc;	
	int m_size;
	bool m_is_free_signal;
	Time m_enqueue_time, m_time; 
	flit_type m_type;
	MsgPtr m_msg_ptr;
	int m_outport;
	int src_delay;
	pair<flit_stage, Time> m_stage;

};

inline extern bool node_less_then_eq(flit_d* n1, flit_d* n2);

inline extern
bool node_less_then_eq(flit_d* n1, flit_d* n2)
{
  if (n1->get_time() == n2->get_time()) {
//    ASSERT(n1->flit_id != n2->flit_id);
    return (n1->get_id() <= n2->get_id());
  } else {
    return (n1->get_time() <= n2->get_time());
  }
}

// Output operator declaration
ostream& operator<<(ostream& out, const flit_d& obj);

// ******************* Definitions *******************

// Output operator definition
extern inline 
ostream& operator<<(ostream& out, const flit_d& obj)
{
  obj.print(out);
  out << flush;
  return out;
}

#endif
