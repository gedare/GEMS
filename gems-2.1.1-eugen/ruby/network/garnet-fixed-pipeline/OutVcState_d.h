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
 * OutVCState_d.h 
 *
 * Niket Agarwal, Princeton University
 *
 * */

#ifndef OUT_VC_STATE_D_H
#define OUT_VC_STATE_D_H

#include "NetworkHeader.h"

class OutVcState_d {
public:
	OutVcState_d(int id);

	int get_inport() {return m_in_port; }
	int get_invc() { return m_in_vc; }
	int get_credit_count() {return m_credit_count; }
	void set_inport(int port) {m_in_port = port; }
	void set_invc(int vc) {m_in_vc = vc; }

	inline bool isInState(VC_state_type state, Time request_time)
	{
		return ((m_vc_state == state) && (request_time >= m_time) );
	}

	inline void setState(VC_state_type state, Time time)
	{
		m_vc_state = state;
		m_time = time;	
	}

	inline bool has_credits()
	{
		return (m_credit_count > 0);
	}

	inline void increment_credit()
	{
		m_credit_count++;
	}

	inline void decrement_credit()
	{
		m_credit_count--;
	}

private:
	int m_id ;
	Time m_time; 
	VC_state_type m_vc_state;
	int m_in_port;
	int m_in_vc;
	int m_credit_count;
};

#endif
