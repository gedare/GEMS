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
 * VCallocator_d.h
 *
 * Niket Agarwal, Princeton University
 *
 * */

#ifndef VC_ALLOCATOR_D_H
#define VC_ALLOCATOR_D_H

#include "NetworkHeader.h"
#include "Consumer.h"

class Router_d;
class InputUnit_d;
class OutputUnit_d;

class VCallocator_d : public Consumer {
public:
	VCallocator_d(Router_d *router);
	void init();
	void wakeup();
	void check_for_wakeup();
	void clear_request_vector();
	int get_vnet(int invc);
  	void print(ostream& out) const {};
	void arbitrate_invcs();
	void arbitrate_outvcs();
	bool is_invc_candidate(int inport_iter, int invc_iter);
	void select_outvc(int inport_iter, int invc_iter);
	inline double get_local_arbit_count()
	{
		return m_local_arbiter_activity;
	}
	inline double get_global_arbit_count()
	{
		return m_global_arbiter_activity;
	}

private:
	int m_num_vcs, m_vc_per_vnet;
	int m_num_inports;
	int m_num_outports;

	double m_local_arbiter_activity, m_global_arbiter_activity;

	Router_d *m_router;
	Vector<Vector <int > > m_round_robin_invc; // First stage of arbitration where all vcs select an output vc to content for
	Vector<Vector <pair<int, int> > > m_round_robin_outvc; // Arbiter for every output vc
	Vector<Vector<Vector<Vector<bool > > > > m_outvc_req; // [outport][outvc][inpotr][invc]. set true in the first phase of allocation
	Vector<Vector<bool > > m_outvc_is_req; 

	Vector<InputUnit_d *> m_input_unit ;
	Vector<OutputUnit_d *> m_output_unit ;

};

#endif
