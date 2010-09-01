
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
 * FileName:  makeipage.cpp
 * Synopsis:  creates the instruction pages
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
#include "opal.h"

#include <stdio.h>
#include <sys/types.h>
#define Address uint64_t
#include "statici.h"

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

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

  if (argc != 3) {
    printf("usage: %s tracedir tracefile\n", argv[0]);
    exit(1);
  }

  // initialize the state
  simstate_init();

  // initialize our module
  hfa_init_local();

  // construct using the default configuration
  attr_value_t val;
  val.kind     = Sim_Val_String;
  val.u.string = "";
  hfa_dispatch_set( (void *) "init", NULL, &val, NULL );

  // open a log
  val.kind     = Sim_Val_String;
  val.u.string = "/scratch.local/makeipage.log";
  hfa_dispatch_set( (void *) "sim-start", NULL, &val, NULL );

  // turn on debugging
  val.kind     = Sim_Val_String;
  val.u.string = "0 0";
  hfa_dispatch_set( (void *) "debugtime", NULL, &val, NULL );

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

  // attach translation information using a file
  system_t::inst->attachTLBTrace( argv[1], argv[2] );

  // read the instruction map
  system_t::inst->saveInstructionMap( argv[1], argv[2] );

  // close down our module
  hfa_fini_local();
  // close down the state
  simstate_fini();
}

