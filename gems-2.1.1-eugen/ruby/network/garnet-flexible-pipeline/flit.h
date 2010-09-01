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
 * flit.h
 *
 * Niket Agarwal, Princeton University
 *
 * */

#include "NetworkHeader.h"
#include "Message.h"

#ifndef FLIT_H
#define FLIT_H

class flit {
public:
	flit(int id, int vc, int vnet, int size, MsgPtr msg_ptr);

	int get_size();
	int get_id();
	Time get_time();
	Time get_enqueue_time();
	void set_time(Time time);	
	int get_vnet();
	int get_vc();
	void set_vc(int vc);
	MsgPtr& get_msg_ptr();
	flit_type get_type();
	void print(ostream&out) const;

private:
/************Data Members*************/
	int m_id;
	int m_vnet;
	int m_vc;	
	int m_size;
	Time m_enqueue_time, m_time; 
	flit_type m_type;
	MsgPtr m_msg_ptr;

};

inline extern bool node_less_then_eq(flit* n1, flit* n2);

inline extern
bool node_less_then_eq(flit* n1, flit* n2)
{
  if (n1->get_time() == n2->get_time()) {
//    ASSERT(n1->flit_id != n2->flit_id);
    return (n1->get_id() <= n2->get_id());
  } else {
    return (n1->get_time() <= n2->get_time());
  }
}

// Output operator declaration
ostream& operator<<(ostream& out, const flit& obj);

// ******************* Definitions *******************

// Output operator definition
extern inline 
ostream& operator<<(ostream& out, const flit& obj)
{
  obj.print(out);
  out << flush;
  return out;
}

#endif
