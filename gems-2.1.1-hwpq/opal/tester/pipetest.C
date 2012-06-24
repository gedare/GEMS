
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
 * FileName:  pipetest.cpp
 * Synopsis:  linked list tester
 * Author:    cmauer
 * Version:   $Id$
 */

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/


#include "hfa.h"
#include "hfacore.h"

#include <stdio.h>
#include <sys/types.h>
#define Address uint64_t

#include "wait.h"
#include "pipestate.h"
#include "pipepool.h"
#include "rubycache.h"

extern void simstate_init( void );
extern void simstate_fini( void );

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

    // initialize the state
  simstate_init();

  // initialize our module
  hfa_init_local();

  // read the default configuration
  attr_value_t val;
  val.kind     = Sim_Val_String;
  val.u.string = "";
  hfa_dispatch_set( (void *) "init", NULL, &val, NULL );

  abstract_pc_t  at;
  uint64         lsq_index = 0;
  static_inst_t  *s = new static_inst_t( ~(my_addr_t)0, STATIC_INSTR_MOP );
  dynamic_inst_t *d = new dynamic_inst_t( s, 0,
                                          system_t::inst->m_seq[0],
                                          lsq_index++, &at );
  pipepool_t *m_mshr_pool = new pipepool_t();
  
  // add two items to list
  ruby_link_t *item = new ruby_link_t( d, 0x1000beef,
                                       OPAL_LOAD, 0x0, 1 );
  m_mshr_pool->insertOrdered( item );
  item = new ruby_link_t( d, 0xbeef,
                          OPAL_STORE, 0x1000, 1 );
  m_mshr_pool->insertOrdered( item );

  printf("start\n");
  m_mshr_pool->print();

  // remove two items
  item = NULL;
  item = static_cast<ruby_link_t *>(m_mshr_pool->removeHead());

  printf("one item\n");
  m_mshr_pool->print();
  printf("\n");

  item = static_cast<ruby_link_t *>(m_mshr_pool->removeHead());
  printf("zero items\n");
  m_mshr_pool->print();
  printf("\n");

  // print list
  printf("done\n");
}
