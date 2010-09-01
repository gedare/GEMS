/*
    Copyright (C) 1999-2008 by Mark D. Hill and David A. Wood for the
    Wisconsin Multifacet Project.  Contact: gems@cs.wisc.edu
    http://www.cs.wisc.edu/gems/

    --------------------------------------------------------------------

    This file is part of Orion (Princeton's interconnect power model),
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
#ifndef _POWER_ROUTER_INIT_H
#define _POWER_ROUTER_INIT_H

#include "power_array.h"
#include "power_arbiter.h"
#include "power_crossbar.h"

/* ------------ Models ------------------------ */
/*typedef enum {
	GENERIC_SEL = 1,
	SEL_MAX_MODEL
} power_sel_model;
*/

/* ------------ Misc --------------------------- */

/*typedef struct {
	int model;
	unsigned width;
	unsigned long long n_anyreq;
	unsigned long long n_chgreq;
	unsigned long long n_grant;
	unsigned long long n_enc[MAX_SEL_LEVEL];
	double e_anyreq;
	double e_chgreq;
	double e_grant;
	double e_enc[MAX_SEL_LEVEL];
} power_sel;
*/

/* --------------- Loading --------------- */
typedef enum {
	ACTUAL_LOADING =1,
	HALF_LOADING,
	MAX_LOADING
}loading;

/* ----------------- Router --------------- */

typedef struct {
	power_crossbar crossbar;
	power_array in_buf;
	power_arbiter vc_in_arb;
	power_arbiter vc_out_arb;
	power_arbiter sw_in_arb;
	power_arbiter sw_out_arb;
	double i_leakage;
} power_router;

typedef struct {
	//general
	unsigned n_in;
	unsigned n_out;
	unsigned flit_width;
	//vc
	unsigned n_v_channel;
	unsigned n_v_class;
	int in_share_buf;
	int out_share_buf;
	int in_share_switch;
	int out_share_switch;
	//xbar
	int crossbar_model;
	int degree;
	int connect_type;
	int trans_type;
	double crossbar_in_len;
	double crossbar_out_len;

    int in_buf;
	
	//buffer
	power_array_info in_buf_info;
	unsigned pipe_depth;
	//arbiter
	int vc_in_arb_model;
	int vc_out_arb_model;
	int vc_in_arb_ff_model;
	int vc_out_arb_ff_model;
	int sw_in_arb_model;
	int sw_out_arb_model;
	int sw_in_arb_ff_model;
	int sw_out_arb_ff_model;

	power_array_info vc_in_arb_queue_info;
	power_array_info vc_out_arb_queue_info;
	power_array_info sw_in_arb_queue_info;
	power_array_info sw_out_arb_queue_info;
	//derived
	unsigned n_total_in;
	unsigned n_total_out;
	unsigned in_n_switch;
	unsigned n_switch_in;
	unsigned n_switch_out;
} power_router_info;

extern int power_router_init(power_router *router, power_router_info *info);
#endif
