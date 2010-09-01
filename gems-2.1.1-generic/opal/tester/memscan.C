
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
 * FileName:  memscan.cpp
 * Synopsis:  scans a memory trace and prints out statistics
 * Author:    cmauer
 * Version:   $Id$
 */

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include <ext/hash_map>
#include "hfa.h"

#include <stdio.h>
#include <sys/types.h>
#define Address uint64_t
#include "transaction.h"
#include "memtrace.h"

using namespace __gnu_cxx;

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

#define HASHKEY  uint32

typedef struct data_bucket {
  HASHKEY  key;
  uint32   count;
  pa_t     physical_addr;
  la_t     logical_addr;
} bucket_t;

/*------------------------------------------------------------------------*/
/* Variable declarations                                                  */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Forward declarations                                                   */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Constructor(s) / destructor                                            */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Public methods                                                         */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Accessor(s) / mutator(s)                                               */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Private methods                                                        */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Static methods                                                         */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Global functions                                                       */
/*------------------------------------------------------------------------*/

/** [Memo].
 *  [Internal Documentation]
 */
//**************************************************************************
int main( int argc, char *argv[] ) {

  hash_map<HASHKEY, bucket_t *>           mymap;
  hash_map<HASHKEY, bucket_t *>::iterator iter;
  bucket_t                     *bucket;

  if (argc != 2) {
    printf("usage: %s memtrace\n", argv[0]);
    exit(1);
  }

  // open a memory file
  memtrace_t *mt = new memtrace_t( argv[1] );
  
  // read from the memtrace, building a histogram
  transaction_t xaction;
  bool success     = true;
  uint32 processed = 0;

  while (success) {
    // add it to the histogram
    success = mt->readTransaction( &xaction );

    HASHKEY  key = xaction.getSize();
    if (mymap.find( key ) == mymap.end()) {
      bucket = new bucket_t();
      bucket->key   = key;
      bucket->count = 1;

      mymap[key] = bucket;
    } else {
      bucket = mymap[key];
      bucket->count++;
    }
  }
  
  processed++;
  if (processed % 1000000 == 0) {
    for (iter = mymap.begin(); iter != mymap.end(); iter++) {
      bucket = (*iter).second;
      printf("0x%0x %d\n", bucket->key, bucket->count);
    }
  }
  
  printf("scan completed:\n");
  for (iter = mymap.begin(); iter != mymap.end(); iter++) {
    bucket = (*iter).second;
    printf("0x%0x %d\n", bucket->key, bucket->count);
  }
}
