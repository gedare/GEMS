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
// Crossbar

#ifndef _POWER_CROSSBAR_H
#define _POWER_CROSSBAR_H

typedef enum {
	TRANS_GATE,
	TRISTATE_GATE
} power_connect_model;

/* transmission gate type */
typedef enum {
	N_GATE,
	NP_GATE
} power_trans;

typedef enum {
	MATRIX_CROSSBAR =1,
	MULTREE_CROSSBAR,
	CUT_THRU_CROSSBAR,
	CROSSBAR_MAX_MODEL
} power_crossbar_model;


typedef struct {
	int model;
	unsigned n_in;
	unsigned n_out;
	unsigned data_width; 
	unsigned degree;       //used only for multree xbar
	unsigned connect_type;
	unsigned trans_type;
	unsigned long int n_chg_in;
	unsigned long int n_chg_int;
	unsigned long int n_chg_out;
	unsigned long int n_chg_ctr;
	unsigned long int mask;
	double e_chg_in;
	double e_chg_int;
	double e_chg_out;
	double e_chg_ctr;
	unsigned depth;  //used only for multree xbar
	double i_leakage;
} power_crossbar;
	

extern int crossbar_record(power_crossbar *xb, int io, unsigned long int new_data, unsigned long int old_data, unsigned new_port, unsigned old_port);

extern int power_crossbar_init(power_crossbar *crsbar, int model, unsigned n_in, unsigned n_out, unsigned data_width, unsigned degree, int connect_type, int trans_type, double in_len, double out_len, double *req_len);

extern double crossbar_report(power_crossbar *crsbar);

#endif
