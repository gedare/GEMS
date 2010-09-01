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
 * Router_d.h
 *
 * Niket Agarwal, Princeton University
 *
 * */
#ifndef ROUTER_D_H
#define ROUTER_D_H

#include "NetworkHeader.h"
#include "Vector.h"
#include "flit_d.h"
#include "NetDest.h"
#include "power_router_init.h"

class GarnetNetwork_d;
class NetworkLink_d;
class CreditLink_d;
class InputUnit_d;
class OutputUnit_d;
class RoutingUnit_d;
class VCallocator_d;
class SWallocator_d;
class Switch_d;

class Router_d {
public:
	Router_d(int id, GarnetNetwork_d *network_ptr);
	
	~Router_d();

	void init();
	void addInPort(NetworkLink_d *link, CreditLink_d *credit_link);
	void addOutPort(NetworkLink_d *link, const NetDest& routing_table_entry, int link_weight, CreditLink_d *credit_link);
	int get_num_vcs() { return m_num_vcs; }
	int get_vc_per_vnet() {return m_vc_per_vnet; }
	int get_num_inports() { return m_input_unit.size(); }
	int get_num_outports() { return m_output_unit.size(); }
	void printConfig(ostream& out);
	int get_id() { return m_id; }	
	GarnetNetwork_d* get_net_ptr() { return m_network_ptr; }

	Vector<InputUnit_d *>& get_inputUnit_ref() { return m_input_unit; }
	Vector<OutputUnit_d *>& get_outputUnit_ref() { return m_output_unit; }

	void update_sw_winner(int inport, flit_d *t_flit);
	void update_incredit(int in_port, int in_vc, int credit);
	void route_req(flit_d *t_flit, InputUnit_d* in_unit, int invc);
	void vcarb_req();
	void swarb_req();
	
	void power_router_initialize(power_router *router, power_router_info *info);
	double calculate_power();
	double calculate_offline_power(power_router*, power_router_info*);
	void calculate_performance_numbers();

private:
	int m_id;
	int m_virtual_networks, m_num_vcs, m_vc_per_vnet;
	GarnetNetwork_d *m_network_ptr;	
	int m_flit_width;

	double buf_read_count, buf_write_count, crossbar_count, vc_local_arbit_count, vc_global_arbit_count, sw_local_arbit_count, sw_global_arbit_count;

	Vector<InputUnit_d *> m_input_unit;
	Vector<OutputUnit_d *> m_output_unit;
	RoutingUnit_d *m_routing_unit;
	VCallocator_d *m_vc_alloc;
	SWallocator_d *m_sw_alloc;
	Switch_d *m_switch;
};

#endif
