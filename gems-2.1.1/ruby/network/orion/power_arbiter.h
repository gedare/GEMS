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
// Arbiter


#ifndef _POWER_ARBITER_H 
#define _POWER_ARBITER_H

#include "power_array.h"

typedef enum {
	RR_ARBITER =1,
	MATRIX_ARBITER,
	QUEUE_ARBITER,
	ARBITER_MAX_MODEL
} power_arbiter_model;

typedef enum {
	NEG_DFF = 1,	/* negative egde-triggered D flip-flop */
	FF_MAX_MODEL
} power_ff_model;

typedef struct {
	int model;
	unsigned long int n_switch;
	unsigned long int n_keep_1;
	unsigned long int n_keep_0;
	unsigned long int n_clock;
	double e_switch;
	double e_keep_1;
	double e_keep_0;
	double e_clock;
	double i_leakage;	
} power_ff;

typedef struct{
	int model;
	unsigned req_width;
	unsigned long int n_chg_req;
	unsigned long int n_chg_grant;
	unsigned long int n_chg_carry; //internal node of rr arbiter
	unsigned long int n_chg_carry_in; //internal node of rr arbiter
	unsigned long int n_chg_mint; //internal node of matrix arbiter
	unsigned long int mask;
	double e_chg_req;
	double e_chg_grant;
	double e_chg_carry;
	double e_chg_carry_in;
	double e_chg_mint;
	power_ff pri_ff; //priority ff
	power_array queue; //request queue
	double i_leakage;
} power_arbiter;

extern int arbiter_record(power_arbiter *arb, unsigned long int new_req, unsigned long int old_req, unsigned new_grant, unsigned old_grant);

extern double arbiter_report(power_arbiter *arb);

extern int power_arbiter_init(power_arbiter *arb, int arbiter_model, int ff_model, unsigned req_width, double length, power_array_info *info);

#endif



