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
 * FileName:  transaction.C
 * Synopsis:  implementation of generic memory tranactions
 * Author:    cmauer
 * Version:   $Id: transaction.C 1.9 03/07/10 18:07:51-00:00 milo@cottons.cs.wisc.edu $
 */

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include "hfa.h"
#include "transaction.h"

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
transaction_t::transaction_t( )
{
  m_physical = 0;
  m_virtual  = 0;
  m_size     = 0;
  m_data     = NULL;
  
  m_action = TRANS_LOAD;
  m_iord   = TRANS_DATA;
  m_issuper= TRANS_USER;
  m_atomic = TRANS_ATOMIC_NONE;
}

//**************************************************************************
transaction_t::transaction_t( const memory_transaction_t *mem_trans )
{
  integer_t    test;
  processor_t *cpuptr = SIM_current_processor();
  
  m_physical = mem_trans->s.physical_address;
  m_virtual  = mem_trans->s.logical_address;
  m_size     = mem_trans->s.size;
  m_num_words = m_size / 8;
  if (m_size % 8 != 0) {
    m_num_words ++;
  }
  m_data = (integer_t *) malloc( sizeof(integer_t)*(m_num_words) );
  memset(m_data, 0, sizeof(integer_t)*(m_num_words) );

  int szleft = m_size;
  int sztoread;
  if (m_physical != (pa_t) -1) {
    for (int i = 0; i < m_num_words; i ++) {
      if ( szleft < 8 ) {
        sztoread = szleft;
      } else {
        sztoread = 8;
      }

      m_data[i] = SIM_read_phys_memory( cpuptr,
                                        (m_physical + (i*sizeof(integer_t))),
                                        sztoread );
      if (sztoread == 8) {
        test = SIM_read_phys_memory( cpuptr,
                                     m_physical + i*sizeof(integer_t), 4 );
        if ( (test & 0xffffffff) != ((m_data[i] >> 32) & 0xffffffff) ) {
          DEBUG_OUT("test (1) failed: 0x%0llx 0x%0llx\n", test, m_data[i]);
        }
        test = SIM_read_phys_memory( cpuptr,
                                     m_physical + i*sizeof(integer_t) + 4, 4 );
        if ( (test & 0xffffffff) != (m_data[i] & 0xffffffff) ) {
          DEBUG_OUT("test (2) failed: 0x%0llx 0x%0llx\n", test, m_data[i]);
        }
      }
      szleft -= sztoread;
    }
  }

  if ( SIM_mem_op_is_read( &mem_trans->s ) ) {

    // LD that is not part of an atomic operation
    if (mem_trans->s.atomic == false){           
      m_action = TRANS_LOAD;

    } else {                                    

      // LD as 1st half of atomic is treated like a ST (to issue GETX)
      m_atomic = TRANS_ATOMIC;
      m_action = TRANS_STORE;
    }
  } else if (SIM_mem_op_is_write( &mem_trans->s ) ) {
    m_action = TRANS_STORE;
  } else {
    ERROR_OUT("Error: Not a LD or a ST");
    m_action = TRANS_STORE;
  }

  if ( SIM_mem_op_is_instruction( &mem_trans->s ) ) {
    m_iord = TRANS_INSTRUCTION;
  } else {
    m_iord = TRANS_DATA;
  }
  
  // CM FIX: should convert transactions to use SPARC PRIV, CLI, etc. bits
#if 0
  if (mem_trans->s.mode == User) {
    m_issuper = TRANS_USER;
  } else {
    m_issuper = TRANS_SUPERVISOR;
  }
#endif
}

//**************************************************************************
transaction_t::~transaction_t( )
{
  if (m_data) {
    free( m_data );
  }
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

