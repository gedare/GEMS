
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
 * FileName:  usd.cpp
 * Synopsis:  ultra sparc decoder
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

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

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

  char *s, buf[200], buffer[200];
  long long instr;

  // needed for some translations
  //reg_box_t      rbox;
  //rbox.initializeMappings( );

  // initialize the state
  simstate_init();

  // initialize our module
  hfa_init_local();
  char *ruby_param[] = { "tester.exec",
                         "-p 1",
                         "-l 100000",
                         "-r 256",
  };

  // read the configuration
  attr_value_t val;
  val.kind     = Sim_Val_String;
  val.u.string = "";
  initvar_dispatch_set( (void *) "init", NULL, &val, NULL );

  // make a static instruction
  while (!feof(stdin)) {

    scanf("%s", buf);
    s = NULL;
    instr = strtoll( buf, &s, 0 );
    DEBUG_OUT( "0x%0llx = %lld\n", instr, instr );
    printIBits( instr );
    DEBUG_OUT("\n");
    static_inst_t *sinst = new static_inst_t(0x0ULL, instr);
    DEBUG_OUT("opcode = %s (0x%0x)\n\n",
              decode_opcode( (enum i_opcode) sinst->getOpcode()),
              sinst->getInst() );
    sinst->printDisassemble( buffer );
  }

  // close down our module
  hfa_fini_local();

  simstate_fini();
  exit(0);
}
