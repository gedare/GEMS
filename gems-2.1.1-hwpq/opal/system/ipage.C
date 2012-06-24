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
 * FileName:  ipage.C
 * Synopsis:  Implementation of an instruction page
 * Author:    cmauer
 * Version:   $Id: ipage.C 1.19 04/08/09 11:25:06-05:00 lyen@opus.cs.wisc.edu $
 */

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include "hfa.h"
#include "statici.h"
#include "fileio.h"
#include "ipage.h"

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
ipage_t::ipage_t()
{
  int check_mask = maskBits32( ~0, 0, IPAGE_PAGE_BITS - 1 );
  if (check_mask != IPAGE_PAGE_MASK) {
    DEBUG_OUT("error: page mask set incorrectly\n");
    DEBUG_OUT("it is       : ");
    printBits(IPAGE_PAGE_MASK);
    DEBUG_OUT("should be   : ");    
    printBits(check_mask);
  }
  memset( m_instr, 0, sizeof(static_inst_t *) * IPAGE_MAX_INSTR );
}

//**************************************************************************
ipage_t::~ipage_t( void )
{
  // walk all static instructions freeing any that are not 
  // being used 
  for (uint32 i = 0; i < IPAGE_MAX_INSTR; i++) { 
    if (m_instr[i] != NULL) { 
      delete m_instr[i]; 
    } 
  } 
}

//**************************************************************************
uint16 ipage_t::insertInstruction( pa_t address, uint32 instr,
                                   static_inst_t * &s_instr )
{
  uint16 rc     = IPAGE_NEW;
  uint32 offset = address & IPAGE_PAGE_MASK;

  if (m_instr[offset]) {
    // This offset has been seen before!
    s_instr = m_instr[offset];
    if (s_instr->getInst() == instr) {
      return (IPAGE_SEEN);
    } else {
      /** DEBUG_OUT("warning: address 0x%0llx has two instructions:0x%0x and 0x%0x\n", address, instr, m_instr[offset]->getInst()); */
      
      // if the old static static instruction is not used, free immediately
      if (s_instr->getRefCount() == 0) {
        delete s_instr;
      } else {
        // mark the in-flight static instruction as garbage to be freed
        // by the last dynamic instructions deconstructor
        s_instr->setFlag( SI_TOFREE, true );
      }
      
      // construct the static instruction, replace the one in the table
      s_instr = new static_inst_t( address, instr);       
      m_instr[offset] = s_instr;
      rc = IPAGE_CONFLICT;
    }
  } else {
    
    // never seen this offset before, cache this instruction
    s_instr = new static_inst_t( address, instr );       
    m_instr[offset] = s_instr;
    rc = IPAGE_NEW;
  }
  
  return (rc);
}

//**************************************************************************
static_inst_t *ipage_t::getInstruction( pa_t address )
{
  uint32 offset = address & IPAGE_PAGE_MASK;
  return (m_instr[offset]);
}

//**************************************************************************
void ipage_t::invalidateInstruction( pa_t address )
{
  uint32 offset = address & IPAGE_PAGE_MASK;
  static_inst_t *s_instr = m_instr[offset];

  if (s_instr == NULL)
    return;

  // if the old static static instruction is not used, free immediately
  if (s_instr->getRefCount() == 0) {
    // DEBUG_OUT("invalidateInstr: eliminating target instr...\n");
    delete s_instr;
    m_instr[offset] = NULL;

  } else {
    // DEBUG_OUT("invalidateInstr: found target instr...\n");
    // mark the in-flight static instruction as garbage to be freed
    // by the last dynamic instructions deconstructor
    s_instr->setFlag( SI_TOFREE, true );
    m_instr[offset] = NULL;
  }
}

//**************************************************************************
void   ipage_t::setPageAddress( pa_t address )
{
  // strip off the low order bits for the address and physical address
  uint64 addrMask = makeMask64( 63, IPAGE_PAGE_BITS );
  m_header.m_physical_addr = address & addrMask;
}

//**************************************************************************
uint32 ipage_t::print( bool verbose )
{
  uint32 index;
  uint32 count = 0;

  // print out the address, and the instruction in this page
  for (index = 0; index < IPAGE_MAX_INSTR; index ++) {
    if (m_instr[index]) {
      count++;
      if (verbose) {
        DEBUG_OUT("\t0x%0x\n", m_instr[index]->getInst() );
      }
    }
  }
  return (count);
}

//**************************************************************************
bool ipage_t::walkMap( void *user_data, ipage_callback_t si_callback )
{
  uint32 index;
  bool   rc;

  // print out the address, and the instruction in this page
  for (index = 0; index < IPAGE_MAX_INSTR; index ++) {
    if (m_instr[index]) {
      rc = (*si_callback)( user_data, m_instr[index] );
      if (rc)
        return (rc);
    }
  }
  return (false);
}

//**************************************************************************
bool ipage_t::writePage( FILE *fp )
{
  uint64   pc;
  uint32   index;
  uint32   instr;
  int      numwritten;

  // write the physical PC
  ASSERT ( sizeof(pa_t) == sizeof(uint64) );
  pc = m_header.m_physical_addr;
  numwritten = myfwrite( &pc, sizeof(uint64), 1, fp );
  if (numwritten != 1) {
    ERROR_OUT("write fails: ipage_t writePage (%d)\n", numwritten);
    return (false);
  }

  // for each non-NULL instruction
  for (index = 0; index < IPAGE_MAX_INSTR; index ++) {
    if (m_instr[index]) {
      // get the instruction from the static instruction
      instr = m_instr[index]->getInst();
    } else {
      // a version of the illegal trap instruction
      instr = 0;
    }
    numwritten = myfwrite( &instr, sizeof(uint32), 1, fp );
    if (numwritten != 1) {
      ERROR_OUT("write fails: ipage_t writePage (%d)\n", numwritten);
      return (false);
    }
  }
  return (true);
}

//**************************************************************************
bool ipage_t::readPage( FILE *fp, pa_t tag, uint32 &totalread )
{
  uint64   pc;
  uint32   index;
  uint32   instr;
  int      numread;
  int      count = 0;

  // right shift the bits, so the offset can simply be masked on
  tag = tag << IPAGE_PAGE_BITS;

  // read the physical PC
  ASSERT ( sizeof(pa_t) == sizeof(uint64) );
  numread = myfread( &pc, sizeof(uint64), 1, fp );
  if (numread != 1) {
    ERROR_OUT("read fails: ipage_t readPage (%d)\n", numread);
    return (false);
  }
  m_header.m_physical_addr = pc;

  for (index = 0; index < IPAGE_MAX_INSTR; index ++) {

    numread = myfread( &instr, sizeof(uint32), 1, fp );
    if (numread != 1) {
      ERROR_OUT("read fails: ipage_t readPage (%d)\n", numread);
      return (false);
    }
    
    if (instr == 0) {
      m_instr[index] = NULL;
    } else {
      // decode and add this instruction to the page
      pa_t ppc = m_header.m_physical_addr | index;
      m_instr[index] = new static_inst_t( ppc, instr );    
      count++;
    }
  }

  totalread += count;
  return (true);
}
