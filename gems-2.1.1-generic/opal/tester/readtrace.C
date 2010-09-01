
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
 * FileName:  readtrace.cpp
 * Synopsis:  reads the trace
 * Author:    cmauer
 * Version:   $Id$
 */

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/


#include "hfa.h"
#include "hfacore.h"
#include "tracefile.h"
#include "branchfile.h"
extern "C" {
#include "hfa_init.h"
}

#include <stdio.h>
#include <sys/types.h>
#define Address uint64_t
#include "statici.h"

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

const int  DIST_SIZE = 40;

/*------------------------------------------------------------------------*/
/* Variable declarations                                                  */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Forward declarations                                                   */
/*------------------------------------------------------------------------*/

extern void simstate_init( void );
extern void simstate_fini( void );

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

  if (argc != 2) {
    printf("usage: %s tracefile\n", argv[0]);
    exit(1);
  }

  // initialize the state
  simstate_init();

  // initialize our module
  hfa_init_local();

#if 0
  branchfile_t *m_tracefp;
  uint32        primary_ctx;
  branchfile_t *m_branchfp;

  m_branchfp = new branchfile_t( argv[1], &primary_ctx );
  bool isOK = m_branchfp->validate();
  printf("validation = %d\n", isOK);
#endif

#if 1
  // open a trace
  core_state_t  m_state; 
  tracefile_t  *m_tracefp;

  la_t          vpc;
  pa_t          ppc;
  uint32        instr;
  uint32        localtime;
  uint32        context;
  uint32        primary_ctx;
  bool          success = true;
  int           dist[DIST_SIZE];
  uint64        tick = 0;
  
  m_tracefp = new tracefile_t( argv[1], &m_state, &primary_ctx );
  // read all instructions from the trace::

  while (success) {
    context = primary_ctx;
    success = m_tracefp->readTraceStep( vpc, ppc, instr, localtime, context );

    if (success) {
      printf("tick: %lld  vpc: 0x%0llx ppc: 0x%0llx instr: 0x%0x\n",
             tick, vpc, ppc, instr);
    }

    // check to see if context has changed
    if (context != primary_ctx) {
      primary_ctx = context;
    }
    
    // localtime = -localtime;
    if (localtime > 0 && localtime < DIST_SIZE - 1) {
      dist[localtime]++;
    } else {
#if 0
      printf("tick: %lld\n", tick);
      printf("localtime: %d\n", localtime);
      printf("vpc: 0x%0llx ppc: 0x%0llx instr: 0x%0x\n",
             vpc, ppc, instr);
#endif
    }
    tick++;
  }

  for (int i = 0; i < DIST_SIZE; i++) {
    printf("%2d: %d\n", i, dist[i]);
  }
#endif

  // close down our module
  hfa_fini_local();
  // close down the state
  simstate_fini();
}

