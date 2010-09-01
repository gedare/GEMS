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
 * FileName:  sstat.C
 * Synopsis:  Static Instruction statistics class.
 * Author:    cmauer
 * Version:   $Id: sstat.C 1.2 03/06/01 19:02:59-00:00 cmauer@cottons.cs.wisc.edu $
 */

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include "hfa.h"
#include "sstat.h"

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
static_stat_t::static_stat_t( )
{
  m_miss_count = 0;
  m_miss_critical_count = 0;

  for (int32 i = 0; i < SI_MAX_SUCCESSORS; i++) {
    m_successors[i] = NULL;
    m_succ_count[i] = 0;
  }
  for (int32 i = 0; i < SI_MAX_PREDECESSORS; i++) {
    m_predecessors[i] = NULL;
    m_pred_count[i] = 0;
  }
  m_total_preds = 0;
  m_total_succs = 0;
  m_max_successors   = false;
  m_max_predecessors = false;
  
#ifdef STATICI_PROFILE
  m_count_execute    = 0;
  m_count_taken      = 0;
  m_count_mispredict = 0;
  m_count_static_mispredict = 0;
  m_count_mispredict_inf = 0;
#endif
}

//**************************************************************************
static_stat_t::~static_stat_t( )
{
}

//**************************************************************************
void static_stat_t::pushPredecessor( static_inst_t *pred )
{
  m_total_preds++;
  if (m_max_predecessors) {
    return;
  }

  for (int32 i = 0; i < SI_MAX_PREDECESSORS; i++) {
    if (m_predecessors[i] == pred) {
      m_pred_count[i]++;
      return;
    }
    if (m_predecessors[i] == NULL) {
      // got to null entry, not found. add to this entry, increment & return
      m_predecessors[i] = pred;
      m_pred_count[i] = 1;
      return;
    }
  }
  
  // not found in list, no null entries: set max_predecessor
  m_max_predecessors = true;
}

//**************************************************************************
void static_stat_t::pushSuccessor( static_inst_t *pred )
{
  m_total_succs++;
  if (m_max_successors) {
    return;
  }

  for (int32 i = 0; i < SI_MAX_SUCCESSORS; i++) {
    if (m_successors[i] == pred) {
      m_succ_count[i]++;
      return;
    }
    if (m_successors[i] == NULL) {
      // got to null entry, not found. add to this entry, increment & return
      m_successors[i] = pred;
      m_succ_count[i] = 1;
      return;
    }
  }
  
  // not found in list, no null entries: set max_predecessor
  m_max_successors = true;
}

//**************************************************************************
int32 static_stat_t::predecessorCount( void )
{
  if (m_max_predecessors)
    return (SI_MAX_PREDECESSORS + 2);
  int32 count = 0;
  for (int32 i = 0; i < SI_MAX_PREDECESSORS; i++) {
    if (m_successors[i] != NULL) {
      count++;
    }
  }
  return count;
}

//**************************************************************************
int32 static_stat_t::successorCount( void )
{
  if (m_max_successors)
    return (SI_MAX_SUCCESSORS + 2);

  int32 count = 0;
  for (int32 i = 0; i < SI_MAX_SUCCESSORS; i++) {
    if (m_successors[i] != NULL) {
      count++;
    }
  }
  return count;
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
