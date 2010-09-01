
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
 * Microbenchmark to test the file i/o speed. Also, this calls 
 * gettimeofday() to establish run time.
 *
 * A command line parameter gives the number of times to run the benchmark,
 * as well as the number of elements in the array.
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
               
#include "stdio.h"
#include "stdlib.h"
#include <sys/time.h>

int icomp( const void *i1, const void *i2 )
{
  int a = *((int *) i1);
  int b = *((int *) i2);
  if ( a == b )
    return 0;
  else if ( a < b )
    return -1;
  // a > b
  return 1;
}

int main(int argc, char* argv[])
{

  if ( argc != 3 ) {
    printf("usage: %s number-of-trials filename\n", argv[0]);
    exit(1);
  }

  int   numTrials   = atoi( argv[1] );
  FILE *fp;
  int   numToRead = 262144;
  int   numRead;
  int  *buffer = (int *) malloc( sizeof(int)*numToRead );
  struct timeval  start_tv;
  struct timeval  end_tv;
  long  total;
  int   rc;

  /* -- 4 == Do_Breakpoint */
  RUBY_MAGIC_CALL( 4 );
  rc = gettimeofday( &start_tv, NULL );
  if (rc != 0) {
    printf("error calling: gettimeofday\n");
    exit(1);
  }

  fp = fopen( argv[2], "r" );
  if (fp == NULL) {
    printf("error: unable to open file %s\n", argv[2]);
    exit(1);
  }
  for (int i = 0; i < numTrials; i++) {
    numRead = fwrite( (void *) buffer, sizeof(int), numToRead, fp );
    total  += buffer[0];
  }
  fclose( fp );

  rc = gettimeofday( &end_tv, NULL );
  if (rc != 0) {
    printf("error calling: gettimeofday\n");
    exit(1);
  }
  /* -- 4 == Do_Breakpoint */
  RUBY_MAGIC_CALL( 4 );               
  
  printf("delta sec = %d\n", (end_tv.tv_sec - start_tv.tv_sec));
  printf("delta ms  = %d\n", (end_tv.tv_usec - start_tv.tv_usec));
  printf("delta time= %d\n", 
         (end_tv.tv_sec - start_tv.tv_sec)*1000000 +
         (end_tv.tv_usec - start_tv.tv_usec));

  printf("(element total %ld)\n", total);

  free( buffer );
  return 0;
}
