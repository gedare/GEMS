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
 * FileName:  stopwatch.C
 * Synopsis:  Implements the stopwatch class
 * Author:    cmauer
 * Version:   $Id: stopwatch.C 1.4 05/08/14 17:07:13-05:00 lyen@opus.cs.wisc.edu $
 */

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include "hfa.h"
#include "hfacore.h"
#include "stopwatch.h"

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

//**************************************************************************
void stopwatch_t::reset( void ) {
  //Luke - get the actual seq_num given the processor number:
  int32 seq_num = m_cpuid / CONFIG_LOGICAL_PER_PHY_PROC;
  uint32 logical_proc_num = m_cpuid % CONFIG_LOGICAL_PER_PHY_PROC;
  ASSERT(seq_num >=0 && seq_num < system_t::inst->m_numSMTProcs);
  ASSERT(logical_proc_num < CONFIG_LOGICAL_PER_PHY_PROC);

  m_instr_start = system_t::inst->m_seq[seq_num]->m_stat_committed[logical_proc_num];
  m_cycle_start = system_t::inst->m_seq[seq_num]->getLocalCycle();
  /* DEBUG_OUT("[%d] stopwatch: start: %lld %lld\n",
     m_cpuid, m_instr_start, m_cycle_start );
 */
}

//**************************************************************************
int64 stopwatch_t::relative_time( void ) {
   //Luke - get the actual seq_num given the processor number:
  int32 seq_num = m_cpuid / CONFIG_LOGICAL_PER_PHY_PROC;
  uint32 logical_proc_num = m_cpuid % CONFIG_LOGICAL_PER_PHY_PROC;
  ASSERT(seq_num >=0 && seq_num < system_t::inst->m_numSMTProcs);
  ASSERT(logical_proc_num < CONFIG_LOGICAL_PER_PHY_PROC);

  int64 localtime = system_t::inst->m_seq[seq_num]->m_stat_committed[logical_proc_num];
  int64 t         = localtime - m_instr_start;
  /* DEBUG_OUT("[%d] stopwatch: stop time: %lld - %lld = %lld\n",
            m_cpuid, localtime, m_instr_start, t );
 */
  return t;
}

//**************************************************************************
int64 stopwatch_t::relative_cycle( void ) {
  //Luke - get the actual seq_num given the processor number:
  int32 seq_num = m_cpuid / CONFIG_LOGICAL_PER_PHY_PROC;
  uint32 logical_proc_num = m_cpuid % CONFIG_LOGICAL_PER_PHY_PROC;
  ASSERT(seq_num >=0 && seq_num < system_t::inst->m_numSMTProcs);
  ASSERT(logical_proc_num < CONFIG_LOGICAL_PER_PHY_PROC);

  int64 localcycle = system_t::inst->m_seq[seq_num]->getLocalCycle();
  int64 t          = localcycle - m_cycle_start;
  /* DEBUG_OUT("[%d] stopwatch: stop cycle: %lld - %lld = %lld\n",
            m_cpuid, localcycle, m_cycle_start, t );
 */
  return t;
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
