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
#include "parm_technology.h"
#include "power_utils.h"
#include <assert.h>
#include <math.h>

/* ----------- from SIM_power_util.c ------------ */

/* Hamming distance table */
static char h_tab[256] = {0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8};


static unsigned SIM_power_Hamming_slow( unsigned long int old_val, unsigned long int new_val, unsigned long int mask )
{
  /* old slow code, I don't understand the new fast code though */
  /* unsigned long int dist;
  unsigned Hamming = 0;
  
  dist = ( old_val ^ new_val ) & mask;
  mask = (mask >> 1) + 1;
  
  while ( mask ) {
    if ( mask & dist ) Hamming ++;
    mask = mask >> 1;
  }

  return Hamming; */

#define TWO(k) (BIGONE << (k)) 
#define CYCL(k) (BIGNONE/(1 + (TWO(TWO(k))))) 
#define BSUM(x,k) ((x)+=(x) >> TWO(k), (x) &= CYCL(k)) 
  unsigned long int x;
  
  x = (old_val ^ new_val) & mask;
  x = (x & CYCL(0)) + ((x>>TWO(0)) & CYCL(0)); 
  x = (x & CYCL(1)) + ((x>>TWO(1)) & CYCL(1)); 
  BSUM(x,2); 
  BSUM(x,3); 
  BSUM(x,4); 
  BSUM(x,5); 

  return x; 
}


int SIM_power_init(void)
{
  unsigned i;
  
  /* initialize Hamming distance table */
  for (i = 0; i < 256; i++)
    h_tab[i] = SIM_power_Hamming_slow(i, 0, 0xFF);

  return 0;
}


/* assume unsigned long int is unsigned64_t */
unsigned SIM_power_Hamming(unsigned long int old_val, unsigned long int new_val, unsigned long int mask)
{
  union {
	  unsigned long int x;
	  char id[8];
	} u;
  unsigned rval;

  u.x = (old_val ^ new_val) & mask;
  
  rval = h_tab[u.id[0]];
  rval += h_tab[u.id[1]];
  rval += h_tab[u.id[2]];
  rval += h_tab[u.id[3]];
  rval += h_tab[u.id[4]];
  rval += h_tab[u.id[5]];
  rval += h_tab[u.id[6]];
  rval += h_tab[u.id[7]];

  return rval;
}


unsigned SIM_power_Hamming_group(unsigned long int d1_new, unsigned long int d1_old, unsigned long int d2_new, unsigned long int d2_old, unsigned width, unsigned n_grp)
{
  unsigned rval = 0;
  unsigned long int g1_new, g1_old, g2_new, g2_old, mask;

  mask = HAMM_MASK(width);
  
  while (n_grp--) {
    g1_new = d1_new & mask;
    g1_old = d1_old & mask;
    g2_new = d2_new & mask;
    g2_old = d2_old & mask;

    if (g1_new != g1_old || g2_new != g2_old)
      rval ++;

    d1_new >>= width;
    d1_old >>= width;
    d2_new >>= width;
    d2_old >>= width;
  }

  return rval;
}

/* ---------------------------------------------- */

/* ----------------------------------------------- */




double logtwo(double x)
{
  assert(x > 0);
  return log10(x)/log10(2);
}

unsigned SIM_power_logtwo(unsigned long int x)
{
  unsigned rval = 0;
  
  while (x >> rval && rval < sizeof(unsigned long int) << 3) rval++;
  if (x == (BIGONE << rval - 1)) rval--;

  return rval;
}

