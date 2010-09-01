
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
 * FileName:  conftest.C
 * Synopsis:  Tests configuration reading and writing
 * Author:    cmauer
 * Version:   $Id$
 */

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/


#include "hfa.h"
#include "hfacore.h"
extern "C" {
#include "hfa_init.h"
}
#include "ipage.h"
#include "ipagemap.h"
#include "confio.h"
#include <stdio.h>
#include <sys/types.h>

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

// #define TOPADDR (1048576 / 4)
#define OFFSET   0x0000000100000000ULL
#define TOPADDR  (OFFSET + 2 * 1048576)

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

  int numcycles;

  if (argc == 3) {
    numcycles = 100;
  } else if (argc == 4) {
    numcycles = atoi( argv[3] );
  } else {
    printf("usage: %s tracedir tracefile numcycles\n", argv[0]);
    exit(1);
  }

  // initialize the state
  simstate_init();

  // initialize our module
  hfa_init_local();

  // add trailing "/" to name (if necessary)
  char *dirname = argv[1];
  int   dirlen = strlen(dirname);
  if ( dirname[dirlen - 1] != '/') {
    dirname = (char *) malloc( dirlen + 2 );
    strcpy( dirname, argv[1] );
    dirname[dirlen]     = '/';
    dirname[dirlen + 1] = '\0';
    argv[1] = dirname;
  }

  // read the instruction map
  system_t::inst->readInstructionMap( argv[1], argv[2], false );
  // attach to the memory trace
  system_t::inst->attachMemTrace( argv[1], argv[2] );
  // processor state file
  system_t::inst->attachTrace( argv[1], argv[2] );
  // open a log
  system_t::inst->openLogfiles( "/tmp/atest" );

  //
  // run the system!
  //
  system_t::inst->slidingWindow();

  // save the cache state
  system_t::inst->writeConfiguration( "/tmp/conf2" );
  // close log
  system_t::inst->closeLogfiles( );
  // print the statistics
  system_t::inst->printStats();

  // print the in flight instructions
  system_t::inst->printInflight();

  // close down our module
  hfa_fini_local();
  simstate_fini();
  exit(1);

}
