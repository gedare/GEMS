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
 * FileName:  tlstack.C
 * Synopsis:  Implements a return-address stack for trap returns (done,retrys).
 * Author:    cmauer
 * Version:   $Id$
 */

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include "hfa.h"
#include "abstractpc.h"
#include "tlstack.h"

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

//**************************************************************************
tlstack_t::tlstack_t( uint32 num_elements )
{
  m_size = num_elements;
  if (m_size < MAXTL) {
    ERROR_OUT("Trap Level Stack: size=%d is less than MAXTL=%d\n",
              m_size, MAXTL);
    m_size = MAXTL + 1;
  }
  m_stack = (abstract_pc_t *) malloc( sizeof(abstract_pc_t) * m_size );
  for (uint32 i = 0; i < m_size; i++) {
    m_stack[i].pc = 0;
    m_stack[i].npc = 0;
    m_stack[i].tl = 0;
    m_stack[i].pstate = 0;
    m_stack[i].cwp = 0;
    m_stack[i].gset = 0;
  }
}

//**************************************************************************
tlstack_t::~tlstack_t( )
{
  if (m_stack != NULL) {
    free( m_stack );
  }
}

//**************************************************************************
void tlstack_t::push( my_addr_t pc, my_addr_t npc, uint16 pstate,
                      uint16 cwp, uint16 tl )
{
  if ( tl >= m_size ) {
    DEBUG_OUT( "warning: tlstack: push fails: tl = %d (size=%d)\n",
               tl, m_size);
    return;
  }
  m_stack[tl].pc     = pc;
  m_stack[tl].npc    = npc;
  m_stack[tl].pstate = pstate;
  m_stack[tl].cwp    = cwp;
  m_stack[tl].tl     = tl;
}

//**************************************************************************
void tlstack_t::pop( my_addr_t &pc, my_addr_t &npc, uint16 &pstate,
                     uint16 &cwp, uint16 &tl )
{
  if ( tl >= m_size ) {
    DEBUG_OUT( "warning: tlstack: popping invalid values: tl = %d (size=%d)\n",
               tl, m_size);
    return;
  }
  pc     = m_stack[tl].pc;
  npc    = m_stack[tl].npc;
  pstate = m_stack[tl].pstate;
  cwp    = m_stack[tl].cwp;
  tl     = m_stack[tl].tl;
}

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

