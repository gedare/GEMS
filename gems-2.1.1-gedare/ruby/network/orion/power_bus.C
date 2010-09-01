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
#include <stdio.h>

#include "power_bus.h"
#include "power_ll.h"
#include "parm_technology.h"
#include "SIM_port.h"
#include "power_static.h"
#include "power_utils.h"

/* ------- bus(link) model ---------- */

static int SIM_bus_bitwidth(int encoding, unsigned data_width, unsigned grp_width)
{
  if (encoding && encoding < BUS_MAX_ENC)
    switch (encoding) {
      case IDENT_ENC:
      case TRANS_ENC:	return data_width;
      case BUSINV_ENC:	return data_width + data_width / grp_width + (data_width % grp_width ? 1:0);
      default:	return 0;/* some error handler */
    }
  else
    return -1;
}


/*
 * this function is provided to upper layers to compute the exact binary bus representation
 * only correct when grp_width divides data_width
 */
unsigned long int SIM_bus_state(power_bus *bus, unsigned long int old_data, unsigned long int old_state, unsigned long int new_data)
{
  unsigned long int mask_bus, mask_data;
  unsigned long int new_state = 0;
  unsigned done_width = 0;

  switch (bus->encoding) {
    case IDENT_ENC:	return new_data;
    case TRANS_ENC:	return new_data ^ old_data;

    case BUSINV_ENC:
	 /* FIXME: this function should be re-written for boundary checking */
         mask_data = (BIGONE << bus->grp_width) - 1;
	 mask_bus = (mask_data << 1) + 1;

	 while (bus->data_width > done_width) {
	   if (SIM_power_Hamming(old_state & mask_bus, new_data & mask_data, mask_bus) > bus->grp_width / 2)
	     new_state += (~(new_data & mask_data) & mask_bus) << done_width + done_width / bus->grp_width;
	   else
	     new_state += (new_data & mask_data) << done_width + done_width / bus->grp_width;

	   done_width += bus->grp_width;
	   old_state >>= bus->grp_width + 1;
	   new_data >>= bus->grp_width;
	 }

	 return new_state;

    default:	return 0;/* some error handler */
  }
}


static double SIM_resultbus_cap(void)
{
  double Cline, reg_height;

  /* compute size of result bus tags */
  reg_height = PARM_RUU_size * (RegCellHeight + WordlineSpacing * 3 * PARM_ruu_issue_width); 

  /* assume num alu's = ialu */
  /* FIXME: generate a more detailed result bus network model */
  /* WHS: 3200 should go to PARM */
  /* WHS: use minimal pitch for buses */
  Cline = CCmetal * (reg_height + 0.5 * PARM_res_ialu * 3200 * LSCALE);

  /* or use result bus length measured from 21264 die photo */
  // Cline = CCmetal * 3.3 * 1000;

  return Cline;
}


static double SIM_generic_bus_cap(unsigned n_snd, unsigned n_rcv, double length, double time)
{
  double Ctotal = 0;
  double n_size, p_size;
  
  /* part 1: wire cap */
  /* WHS: use minimal pitch for buses */
  Ctotal += CC2metal * length;

  if ((n_snd == 1) && (n_rcv == 1)) {
    /* directed bus if only one sender and one receiver */

    /* part 2: repeater cap */
    /* FIXME: ratio taken from Raw, does not scale now */
    n_size = Lamda * 10;
    p_size = n_size * 2;

    Ctotal += SIM_power_gatecap(n_size + p_size, 0) + SIM_power_draincap(n_size, NCH, 1) + SIM_power_draincap(p_size, PCH, 1);

    n_size *= 2.5;
    p_size *= 2.5;

    Ctotal += SIM_power_gatecap(n_size + p_size, 0) + SIM_power_draincap(n_size, NCH, 1) + SIM_power_draincap(p_size, PCH, 1);
  }
  else {
    /* otherwise, broadcasting bus */

    /* part 2: input cap */
    /* WHS: no idea how input interface is, use an inverter for now */
    Ctotal += n_rcv * SIM_power_gatecap(Wdecinvn + Wdecinvp, 0);

    /* part 3: output driver cap */
    if (time) {
      p_size = SIM_power_driver_size(Ctotal, time);
      n_size = p_size / 2;
    }
    else {
      p_size = Wbusdrvp;
      n_size = Wbusdrvn;
    }

    Ctotal += n_snd * (SIM_power_draincap(Wdecinvn, NCH, 1) + SIM_power_draincap(Wdecinvp, PCH, 1));
  }

  return Ctotal;
}


/*
 * n_snd -> # of senders
 * n_rcv -> # of receivers
 * time  -> rise and fall time, 0 means using default transistor sizes
 * grp_width only matters for BUSINV_ENC
 */
int power_bus_init(power_bus *bus, int model, int encoding, unsigned width, unsigned grp_width, unsigned n_snd, unsigned n_rcv, double length, double time)
{
  if ((bus->model = model) && model < BUS_MAX_MODEL) {
    bus->data_width = width;
    bus->grp_width = grp_width;
    bus->n_switch = 0;

    switch (model) {
      case RESULT_BUS:
	   /* assume result bus uses identity encoding */
	   bus->encoding = IDENT_ENC;
           bus->e_switch = SIM_resultbus_cap() / 2 * EnergyFactor;
	   break;
	   
      case GENERIC_BUS:
	   if ((bus->encoding = encoding) && encoding < BUS_MAX_ENC) {
             bus->e_switch = SIM_generic_bus_cap(n_snd, n_rcv, length, time) / 2 * EnergyFactor;
	     /* sanity check */
	     if (!grp_width || grp_width > width)
	       bus->grp_width = width;
	   }
	   else return -1;

      default:	break;/* some error handler */
    }

    bus->bit_width = SIM_bus_bitwidth(bus->encoding, width, bus->grp_width);
    bus->bus_mask = HAMM_MASK(bus->bit_width);

    return 0;
  }
  else
    return -1;
}


int bus_record(power_bus *bus, unsigned long int old_state, unsigned long int new_state)
{
  bus->n_switch += SIM_power_Hamming(new_state, old_state, bus->bus_mask);
  return 0;
}


double bus_report(power_bus *bus)
{ 
  return (bus->n_switch * bus->e_switch);
}

/* ------- bus(link) model ---------- */


