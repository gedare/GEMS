
/*
    Copyright (C) 1999-2008 by Mark D. Hill and David A. Wood for the
    Wisconsin Multifacet Project.  Contact: gems@cs.wisc.edu
    http://www.cs.wisc.edu/gems/

    --------------------------------------------------------------------

    This file is part of the Opal Timing-First Microarchitectural
    Simulator, a component of the Multifacet GEMS (General 
    Execution-driven Multiprocessor Simulator) software toolset 
    originally developed at the University of Wisconsin-Madison.

    Opal was originally developed by Carl Mauer based upon code by
    Craig Zilles.

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
/*****************************************************************************
 * Microbenchmark to test the overlapping latency for dependent multiplies
 *
 * A command line parameter gives the number of times to run the benchmark.
 *
 ****************************************************************************/

/* -- define magic call for breakpoint */

#define RUBY_MAGIC_CALL( service )                              \
  do                                                            \
  {                                                             \
    asm volatile (                                              \
                   "sethi %0, %%g0"                             \
                   : /* -- no outputs */                        \
                   : "g" ( (((service) & 0xff) << 16) | 0 )     \
                 );                                             \
  } while( 0 );
               

const int CACHE_LINE_SIZE = 64;

const int CACHE_SIZE = 1024;    // in bytes
const int CACHE_ASSOCIATIVITY = 4;  // 4-way set associative

#include "stdio.h"
#include "stdlib.h"

typedef int int32;

int main(int argc, char* argv[])
{

  if ( argc != 2 ) {
    printf("usage: %s number-of-trials\n", argv[0]);
    exit(1);
  }

  /* do n dependent multiplies */
  int numTrials = atoi( argv[1] );
  int numMult = 5;

  int a = -1;
  int b = 1;
  int c = 2;
  int d = 2;
  int e = 3;
  int f = 2;
  int g = 2;
  int m = 2;
  int n = 2;
  int p1 = 1, p2 = 1, p3 = 1, p4 = 1, p5 = 1, p6 = 1, p7 = 1, p8 = 1, p9 = 2;
  p9 = 2;

  /* -- 4 == Do_Breakpoint */
  RUBY_MAGIC_CALL( 4 );    

  for (int i = 0; i < numTrials; i++) {

    for (int j = 0; j < numMult; j++) {
      p1 = p1 * a;
      p2 = p2 * b;
      p3 = p3 * c;
      p4 = p4 * d;
      p5 = p5 * e;
      p6 = p6 * f;
      p7 = p7 * g;
      p8 = p8 * m;
      p9 = p9 * n;
    }

  }

  /* -- 4 == Do_Breakpoint */
  RUBY_MAGIC_CALL( 4 );               

  printf("Total = %d\n", p9);
  return 0;
}

