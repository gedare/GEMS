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
 * FileName:  pfile.C
 * Synopsis:  Physical register file (integer or floating point)
 * Author:    zilles
 * Version:   $Id$
 */

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include "hfa.h"
#include "hfacore.h"
#include "regfile.h"

/*------------------------------------------------------------------------*/
/* Constructor(s) / destructor                                            */
/*------------------------------------------------------------------------*/

//**************************************************************************
physical_file_t::physical_file_t( uint16 num_physical, pseq_t *seq) {

  #ifdef DEBUG_REG
  DEBUG_OUT("physical_file_t:constructor max_physical[%d]\n",num_physical);
 #endif
  m_num_physical = num_physical;
  m_pseq = seq;
  m_reg = (physical_reg_t *) malloc( sizeof(physical_reg_t) * num_physical );

  /* initialize all registers as not ready */
  for (uint16 i = 0 ; i < m_num_physical ; i ++) {
    m_reg[i].wait_list.wl_reset();
    m_reg[i].isReady = false;
    m_reg[i].isFree  = true;
    m_reg[i].as_int  = 0;
    m_reg[i].l2Miss  = false;
    m_reg[i].l2MissDependents = 0;
    m_reg[i].writerSeqnum = 0;
    m_reg[i].writtenCycle = 0;
  }

  m_watch = (uint16) -1;
}

//***************************************************************************
physical_file_t::~physical_file_t() {
  free( m_reg );
}

//***************************************************************************
void
physical_file_t::dependenceError( uint16 reg_no ) const
{
  ERROR_OUT("dependence error: register %d is not ready.\n", reg_no);
  m_pseq->out_info("error: register %d is not ready.\n", reg_no);
  m_pseq->out_info("error: cycle = %lld\n", m_pseq->getLocalCycle());
  m_pseq->printInflight();
  ASSERT(0);
}

//***************************************************************************
void 
physical_file_t::print(uint16 reg_no)
{
  DEBUG_OUT("Register: %d\n", reg_no);
  DEBUG_OUT("intvalue= 0x%0llx\n", m_reg[reg_no].as_int);
  DEBUG_OUT("isReady = %d\n", m_reg[reg_no].isReady);
  DEBUG_OUT("isFree  = %d\n", m_reg[reg_no].isFree);
  DEBUG_OUT("wl      = ");
  cout << m_reg[reg_no].wait_list << endl;
}

/** @name Accessors / Mutators */

/// wait on this register to be written (i.e. to become ready)
//***************************************************************************
void
physical_file_t::waitResult( uint16 reg_no, dynamic_inst_t *d_instr )
{
  ASSERT(reg_no < m_num_physical);
  ASSERT( m_reg[reg_no].isFree == false );

  #ifdef DEBUG_REG
    DEBUG_OUT("physical_file_t::waitResult() physical[ %d ] seqnum[ %d ]\n",reg_no, d_instr->getSequenceNumber());
  #endif 
  d_instr->InsertWaitQueue( m_reg[reg_no].wait_list );
}

//***************************************************************************
bool
physical_file_t::isWaitEmpty(uint16 reg_no)
{
  return (m_reg[reg_no].wait_list.Empty());
}
