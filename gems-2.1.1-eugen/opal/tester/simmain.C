
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
 * FileName:  simmain.cpp
 * Synopsis:  main function for simulated simics state
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
#include "initvar.h"
#include "opal.h"

#include <stdio.h>
#include <sys/types.h>

#define Address uint64_t

extern void simstate_init( void );
extern void simstate_fini( void );

// ruby interfaces: exported by ruby/tester/testFramework
extern void tester_initialize(int argc, char **argv);
extern void tester_destroy( void );
extern void tester_record_cache( void );

/** [Memo].
 *  [Internal Documentation]
 */
//**************************************************************************
int main( int argc, char *argv[] ) {

  int numcycles;

  if (argc == 4) {
    numcycles = 100;
  } else if (argc == 5) {
    numcycles = atoi( argv[4] );
  } else {
    DEBUG_OUT("usage: %s conf tracedir tracefile numcycles\n", argv[0]);
    exit(1);
  }

  // initialize the state
  simstate_init();

  // initialize our module
  hfa_init_local();
  char *ruby_param[] = { "tester.exec",
                         "-p 1",
                         "-l 100000",
                         "-r 256",
  };
#ifdef UNITESTER
  tester_initialize( sizeof(ruby_param)/sizeof(char *), ruby_param );
#endif

  // read the configuration
  attr_value_t val;
  val.kind     = Sim_Val_String;
  val.u.string = argv[1];
  initvar_dispatch_set( (void *) "readparam", NULL, &val, NULL );
  val.u.string = "";
  initvar_dispatch_set( (void *) "init", NULL, &val, NULL );

  // add trailing "/" to name (if necessary)
  char *dirname = argv[2];
  int   dirlen = strlen(dirname);
  if ( dirname[dirlen - 1] != '/') {
    dirname = (char *) malloc( dirlen + 2 );
    strcpy( dirname, argv[2] );
    dirname[dirlen]     = '/';
    dirname[dirlen + 1] = '\0';
    argv[2] = dirname;
  }

  // read the instruction map
  system_t::inst->readInstructionMap( argv[2], argv[3], false );
  // attach to the memory trace
  system_t::inst->attachMemTrace( argv[2], argv[3] );
  // attach translation information
  system_t::inst->attachTLBTrace( argv[2], argv[3] );
  // processor state file
  system_t::inst->attachTrace( argv[2], argv[3] );

  // start the simulation
  val.kind     = Sim_Val_String;
  char *output_filename = (char *) malloc( 256 * sizeof(char) );
  strcpy( output_filename, "/tmp/opal_tester_XXXXXX" );
  int fd = mkstemp( output_filename );
  close(fd);

  val.u.string = output_filename;
  initvar_dispatch_set( (void *) "sim-start", NULL, &val, NULL );

  // load the cache state
  // system_t::inst->readConfiguration( "./read-1M.conf" );

  // write the configuration
  // system_t::inst->writeConfiguration( "./written-1M.conf" );

  //
  // run the system!
  //
  val.kind      = Sim_Val_Integer;
  val.u.integer = numcycles;
  initvar_dispatch_set( (void *) "sim-step", NULL, &val, NULL );

  // save the cache state
  // system_t::inst->writeConfiguration( "/tmp/conf2" );

  // set debugging cycle
  val.kind          = Sim_Val_List;
  val.u.list.size   = 2;
  val.u.list.vector = (attr_value_t *) malloc( sizeof(attr_value_t) * 2 );
  val.u.list.vector[0].kind      = Sim_Val_Integer;
  val.u.list.vector[0].u.integer = 0;
  val.u.list.vector[1].kind      = Sim_Val_Integer;
  val.u.list.vector[1].u.integer = 0;
  initvar_dispatch_set( (void *) "debugtime", NULL, &val, NULL );
  free( val.u.list.vector );

  // print the statistics
  initvar_dispatch_get( (void *) "sim-stats", NULL, NULL );

#ifdef UNITESTER
  tester_record_cache( );
  tester_destroy( );
#endif
  
  // close log, end simulation
  initvar_dispatch_set( (void *) "sim-stop", NULL, NULL, NULL );

  // delete the temporary log file
  int errcode = unlink( output_filename );
  if (errcode != 0) {
    printf("error: unable to unlink output file: %s\n", output_filename );
  }
  free( output_filename );

  // close down our module
  hfa_fini_local();

  simstate_fini();
}
