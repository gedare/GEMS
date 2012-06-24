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
 * FileName:  ras.C
 * Synopsis:  Return address stack implemenation for the processor
 * Author:    zilles
 * Version:   $Id$
 */

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include "hfa.h"
#include "hfacore.h"
#include "ras.h"

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/
#define RAS_INCREMENT(A) (((A) + 1) % m_size)

/*------------------------------------------------------------------------*/
/* Constructor(s) / destructor                                            */
/*------------------------------------------------------------------------*/

//**************************************************************************
ras_t::ras_t(pseq_t *pseq, uint32 size, uint32 excep_bits) {
  m_pseq = pseq;
  m_size = size;
  m_return_target_table = (my_addr_t *) malloc( m_size * sizeof(my_addr_t) );
  m_call_target_table = (my_addr_t *) malloc( m_size * sizeof(my_addr_t) );
  m_last_TOS = (ras_pointer_t *) malloc( m_size * sizeof(ras_pointer_t) );

  m_excep_size = 1<<excep_bits;
  m_excep_mask = m_excep_size-1;
  m_exception_table = (my_addr_t *) malloc( m_excep_size * sizeof(my_addr_t) );

  init();
}

//**************************************************************************
ras_t::~ras_t() {
  if (m_return_target_table != NULL) {
    free(m_return_target_table);
    m_return_target_table = NULL;
  }
  if (m_call_target_table != NULL) {
    free(m_call_target_table);
    m_call_target_table = NULL;
  }
  if (m_last_TOS != NULL) {
    free(m_last_TOS);
    m_last_TOS = NULL;
  }
  if(m_exception_table != NULL){
    free(m_exception_table);
    m_exception_table = NULL;
  }
}

//**************************************************************************
void
ras_t::init() {
  memset((void *) m_return_target_table, 0, m_size * sizeof(my_addr_t));
  memset((void *) m_call_target_table, 0, m_size * sizeof(my_addr_t));
  memset((void *) m_last_TOS, 0, m_size * sizeof(ras_pointer_t));
  memset((void *) m_exception_table, 0, m_excep_size * sizeof(my_addr_t));
}

//**************************************************************************
void ras_t::push(my_addr_t return_target_PC, my_addr_t call_target_PC, ras_state_t *rs) {
  if ( rs->next_free >= m_size ) {
    ERROR_OUT("warning: ras_t: push to stack: next free = %d, size = %d\n",
              rs->next_free, m_size );
    rs->next_free = rs->next_free % m_size;
    // SIM_HALT;
  }
  
  /* do not push execptions */
  if (m_exception_table[exceptionIndex(return_target_PC)] == return_target_PC) {
    if (DEBUG_RAS) m_pseq->out_info("*********** FILTERING *%llx*********\n", return_target_PC);
    return;
  }

  /* fill in new entry in table */
  m_return_target_table[rs->next_free] = return_target_PC;
  m_call_target_table[rs->next_free] = call_target_PC;
  m_last_TOS[rs->next_free] = rs->TOS;
  
  /* update pointers */
  rs->TOS = rs->next_free;
  rs->next_free = RAS_INCREMENT(rs->next_free);

  if (DEBUG_RAS) m_pseq->out_info("ras push: %llx, free %d, TOS %d\n",
                                  return_target_PC, rs->next_free, rs->TOS);
}

//**************************************************************************
my_addr_t ras_t::pop(ras_state_t *rs) {
  ASSERT(rs->TOS < m_size);
  
  /* get the top-of-stack */
  my_addr_t ret_val = m_return_target_table[rs->TOS];
  
  /* update pointers */
  rs->TOS = m_last_TOS[rs->TOS];
  /* next_free -- no change */

  if (DEBUG_RAS) m_pseq->out_info("ras pop: %llx, free %d, TOS %d\n",
                                  ret_val, rs->next_free, rs->TOS);

  return ret_val;
}

//**************************************************************************
void ras_t::getTop(ras_state_t *rs, my_addr_t* a) {
  ASSERT(rs->TOS < m_size);
  
  /* get the top-of-stack */
  if (a) *a = m_return_target_table[rs->TOS];
}

//**************************************************************************
void ras_t::getTop2(ras_state_t *rs, my_addr_t* a1, my_addr_t* a2) {
  if (a1) *a1 = m_call_target_table[rs->TOS];
  if (a2) *a2 = m_call_target_table[m_last_TOS[rs->TOS]];
}

//**************************************************************************
void ras_t::markException(my_addr_t a) {
  m_exception_table[exceptionIndex(a)] = a;
}

//**************************************************************************
void ras_t::unmarkException(my_addr_t a) {
  if (m_exception_table[exceptionIndex(a)] == a)
    m_exception_table[exceptionIndex(a)] = 0;
}

//**************************************************************************
void
ras_t::print( ras_state_t *rs)
{
  for (uint32 i = 0; i < m_size; i++) {
    if ( i == rs->TOS && i == rs->next_free )
      DEBUG_OUT( "   ! [%2d] %2d 0x%0llx\n", i, m_last_TOS[i], m_return_target_table[i] );
    else if ( i == rs->TOS )
      DEBUG_OUT( "   * [%2d] %2d 0x%0llx\n", i, m_last_TOS[i], m_return_target_table[i] );
    else if ( i == rs->next_free )
      DEBUG_OUT( "   F [%2d] %2d 0x%0llx\n", i, m_last_TOS[i], m_return_target_table[i] );
    else
      DEBUG_OUT( "     [%2d] %2d 0x%0llx\n", i, m_last_TOS[i], m_return_target_table[i] );
  }
}
