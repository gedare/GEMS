
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
/*
 * FileName:  arraymb.C
 * Synopsis:  
 *     Microbenchmark to test array traversal:
 *     options include array element size, array length, and step distance
 *     The code performs a read pass and a read-modify-write pass,
 *     on the array.
 *     
 * Author:    cmauer
 * Version:   $Id$
 */

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "timelib.h"

typedef struct element {
  int  data;
} element_t;

/// size of array
const  int ARRAY_LENGTH = 1024;

/// size of array element in bytes
const  int ELEMENT_SIZE = 256;

/// size of stride in the array
const  int STRIDE_SIZE = 1;

/*------------------------------------------------------------------------*/
/* Main function                                                          */
/*------------------------------------------------------------------------*/

int main(int argc, char* argv[])
{
  if ( argc != 2 ) {
    printf("usage: %s number-of-trials\n", argv[0]);
    exit(1);
  }
  int     numTrials = atoi( argv[1] );
  /** preamble: setup this microbenchmark */

  long long int sum = 0;
  int value;
  assert ( sizeof(element_t) <= (unsigned int) ELEMENT_SIZE );
  element_t *array = (element_t *) malloc( ELEMENT_SIZE * ARRAY_LENGTH );

  // prefetch (write) elements of array
  for (int i = 0; i < ARRAY_LENGTH; i++) {
    array[i].data = i;
  }
  

  /** begin trial */
  timelib timer;
  timer.start();
  for (int i = 0; i < numTrials; i++) {

    // do a simple read-pass of the array
    for (int j = 0; j < ARRAY_LENGTH; j++) {
      sum += array[j].data;
    }

    // do a read-modify-write pass of the array
    for (int j = 0; j < ARRAY_LENGTH; j++) {
      value = array[j].data;
      value = value - 1;
      array[j].data = value;
    }

  }
  /** end trial */
  timer.stop();

  /** postamble: free resources used by this microbenchmark */
  printf("%lld\n", sum);
  for (int j = 0; j < ARRAY_LENGTH; j++) {
    printf("array[%d] = %d\n", j, array[j].data);
  }
  timer.report();
  return 0;
}
