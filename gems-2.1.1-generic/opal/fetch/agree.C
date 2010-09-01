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
 * FileName:  agree.C
 * Synopsis:  Implements an agree direct-branch predictor
 * Author:    cmauer
 * Version:   $Id: agree.C 1.3 04/08/11 16:50:50-05:00 lyen@opus.cs.wisc.edu $
 */

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include "hfa.h"
#include "agree.h"

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Variable declarations                                                  */
/*------------------------------------------------------------------------*/

static const byte_t taken_update[4] = {1, 2, 3, 3};
static const byte_t not_taken_update[4] = {0, 0, 1, 2};

/*------------------------------------------------------------------------*/
/* Forward declarations                                                   */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Constructor(s) / destructor                                            */
/*------------------------------------------------------------------------*/

//**************************************************************************
agree_t::agree_t( uint32 pht_entries )
{
  m_pht_size = 1 << pht_entries;
  m_pht_mask = m_pht_size - 1;

  m_pht = (char *) malloc( sizeof(char) * m_pht_size );
  // initalize table to weakly agree
  memset( (void *) m_pht, 2, sizeof(char) * m_pht_size );
}

//**************************************************************************
agree_t::~agree_t( )
{
  if ( m_pht ) {
    free( m_pht );
    m_pht = NULL;
  }
}

/*------------------------------------------------------------------------*/
/* Public methods                                                         */
/*------------------------------------------------------------------------*/

//**************************************************************************
bool agree_t::Predict(my_addr_t branch_PC, word_t history, bool staticPred, uint32 proc)
{
  uint32  shiftedPC   = mungePC( branch_PC, proc );
  uint32  choiceIndex = generateIndex(shiftedPC, history); 
  ASSERT(choiceIndex < m_pht_size);
  bool    agree       = (m_pht[choiceIndex] >= 2);
  bool    predict;
  if (agree)
    predict = staticPred;
  else
    predict = !staticPred;
  
  return predict;
}

//**************************************************************************
void agree_t::Update(my_addr_t branch_PC, cond_state_t history,
                     bool staticPred, bool taken, uint32 proc)
{
  uint32  shiftedPC   = mungePC( branch_PC, proc );
  uint32  choiceIndex = generateIndex(shiftedPC, history); 
  ASSERT(choiceIndex < m_pht_size);
  
  char oldpred = m_pht[choiceIndex];
  if (staticPred == taken) {
    m_pht[choiceIndex] = taken_update[oldpred];
  } else {
    m_pht[choiceIndex] = not_taken_update[oldpred];
  }
}

//**************************************************************************
void agree_t::printInformation( FILE *fp )
{
  if (fp) {
    fprintf(fp, "AGREE Branch Predictor\n");
    fprintf(fp, "   PHT size               : %d\n", m_pht_size );
    fprintf(fp, "   Size = 2 * pht_size    : %d\n",
            2 * m_pht_size);
  }
}

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

