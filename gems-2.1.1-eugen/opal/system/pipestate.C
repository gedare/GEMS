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
 * FileName:  pipestate.C
 * Synopsis:  Implements the pipeline state functions.
 * Author:    
 * Version:   $Id: pipestate.C 1.3 03/07/10 18:07:48-00:00 milo@cottons.cs.wisc.edu $
 */

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include "hfa.h"
#include "wait.h"
#include "pipestate.h"

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
pipestate_t::pipestate_t( waiter_t *waiter )
{
  m_next = NULL;
  m_prev = NULL;
  m_element = waiter;

  if (waiter != NULL) {
    ASSERT( waiter->Disconnected() );
  }
}

//**************************************************************************
pipestate_t::~pipestate_t( )
{
}

//**************************************************************************
void pipestate_t::insertElement( pipestate_t *pool )
{
  // insert this node at the head of the list
  SIM_HALT;
}

//**************************************************************************
void pipestate_t::insertOrdered( pipestate_t *head )
{
  // TASK: walk the linked list until the insertion point is found

  // current, previous pointers
  pipestate_t *cur  = head->m_next;
  pipestate_t *prev = head;
  uint64       myOrder = getSortOrder();

  while( cur != head && cur->getSortOrder() < myOrder ) {
    prev = cur;
    cur  = cur->m_next;
  }

  /*
  DEBUG_OUT("(head 0x%0x) 0x%0x (%lld) [inserting %lld] 0x%0x (%lld)\n",
            head, prev, prev->getSortOrder(),
            myOrder,
            prev->m_next, prev->m_next->getSortOrder());
  */

  // update this node's pointers
  m_next = prev->m_next;
  m_prev = prev;
  
  // update previous and next element's pointers
  prev->m_next   = this;
  m_next->m_prev = this;
}

//**************************************************************************
bool    pipestate_t::removeElement( pipestate_t *element )
{
  // 'this' object points to head of the list
  pipestate_t *cur = this->m_next;

  // confirm that this element is, in fact, in this list
  while( cur != this && cur != element ) {
    //DEBUG_OUT("scan [0x%0x] 0x%0x\n", cur, cur->getElement());
    cur = cur->m_next;
  }
  
  if ( cur != element ) {
    ERROR_OUT("error: pipestate: removeElement: element not in list\n");
    SIM_HALT;
    return false;
  }
  
  // found element and I'm removing it
  cur->m_prev->m_next = cur->m_next;
  cur->m_next->m_prev = cur->m_prev;
  
  cur->m_next = NULL;
  cur->m_prev = NULL;
  return true;
}

//**************************************************************************
pipestate_t *pipestate_t::removeHead( void )
{
  pipestate_t *cur  = this->m_next;
  pipestate_t *next = cur ->m_next;
  pipestate_t *prev = cur ->m_prev;
  
  if ( cur == this ) {
    ERROR_OUT("error: pipestate_t::removeHead() trying to remove from empty list.\n");
    return NULL;
  }

  next->m_prev = prev;
  prev->m_next = next;

  cur->m_next = NULL;
  cur->m_prev = NULL;
  
  return cur;
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

