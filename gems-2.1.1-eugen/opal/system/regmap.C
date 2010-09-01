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
 * FileName:  regmap.C
 * Synopsis:  Logical register file
 * Author:    zilles
 * Version:   $Id$
 */

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include "hfa.h"
#include "regmap.h"

/*------------------------------------------------------------------------*/
/* Constructor(s) / destructor                                            */
/*------------------------------------------------------------------------*/

/// This function initializes all of the physical registers to zero.
//**************************************************************************
reg_map_t::reg_map_t(physical_file_t *p, int num_logical) {
  
  m_physical = p;
  m_num_physical = p->getNumRegisters();
  m_num_logical  = num_logical;
  ASSERT( m_num_physical >= m_num_logical );

  //each logical proc should have its own rename map/rename count:
  m_logical_reg = (uint16 **) malloc(sizeof(uint16 *) * CONFIG_LOGICAL_PER_PHY_PROC);
  m_logical_rename_count = (uint16 **) malloc(sizeof(uint16 *) * CONFIG_LOGICAL_PER_PHY_PROC);
  for(uint k=0; k < CONFIG_LOGICAL_PER_PHY_PROC; ++k){
    m_logical_reg[k] = (uint16 *) malloc( sizeof(uint16) * m_num_logical );
    m_logical_rename_count[k] = (uint16 *) malloc( sizeof(uint16) * m_num_logical );
    for (uint32 i = 0 ; i < m_num_logical ; i ++) {
      m_logical_reg[k][i] = PSEQ_INVALID_REG;
      m_logical_rename_count[k][i] = 0;
    }
  }
  
  /* put all registers on the freelist */
  m_freelist = (uint16 *) malloc( sizeof(uint16) * m_num_physical );
  for (uint32 i = 0 ; i < m_num_physical ; i ++) {
    m_freelist[i] = i;
  }
#ifdef CHECK_REGFILE
  m_bits = new bitfield_t( m_num_physical );
  for (uint32 i = 0 ; i < m_num_physical ; i ++) {
    m_bits->setBit(i, true);
  }
#endif
  m_freelist_index = m_num_physical - 1;
  
  m_validate_map = (bool *) malloc(sizeof(bool) * m_num_physical);
}

/// This function initializes all of the physical registers to zero.
//**************************************************************************
reg_map_t::~reg_map_t() {
#ifdef CHECK_REGFILE
  if (m_bits)
    delete m_bits;
#endif
  if (m_freelist)
    free( m_freelist );
  for(uint k=0; k < CONFIG_LOGICAL_PER_PHY_PROC; ++k){
    if (m_logical_reg[k])
      free( m_logical_reg[k] );
    if (m_logical_rename_count[k])
      free( m_logical_rename_count[k] );
  }
  if(m_logical_reg)
    free(m_logical_reg);
  if(m_logical_rename_count)
    free(m_logical_rename_count);
  if (m_validate_map)
    free( m_validate_map );
}

//**************************************************************************
uint16
reg_map_t::getMapping(uint16 logical_reg, uint32 proc) const
{ 
  ASSERT(proc < CONFIG_LOGICAL_PER_PHY_PROC);

  if ( logical_reg > m_num_logical) {
    fprintf(stderr, "getmapping: invalid logical register %d > %d\n",
            logical_reg, m_num_logical);
    return PSEQ_INVALID_REG;
  }
  return m_logical_reg[proc][logical_reg];     
}

//**************************************************************************
void
reg_map_t::setMapping(uint16 logical_reg, uint16 phys_reg, uint32 proc)
{
  ASSERT(proc < CONFIG_LOGICAL_PER_PHY_PROC);
  ASSERT(logical_reg < m_num_logical);
  ASSERT(phys_reg < m_num_physical);
  ASSERT(phys_reg != PSEQ_INVALID_REG);
  m_logical_reg[proc][logical_reg] = phys_reg;
}

/// allocate a new logical register (returns index)
//***************************************************************************
uint16
reg_map_t::regAllocate(uint16 logical_reg, uint32 proc)
{
  ASSERT(m_freelist_index >= 0);   
  ASSERT(proc < CONFIG_LOGICAL_PER_PHY_PROC);
  uint32 physical_reg = m_freelist[m_freelist_index];

 #ifdef DEBUG_REG
      DEBUG_OUT("reg_map_t:regAllocate BEGIN physical[%d] logical[%d] m_freelist_index[%d] max_regs[%d]\n",physical_reg,logical_reg, m_freelist_index, m_num_physical);
      if(m_freelist_index >= m_num_physical){
        DEBUG_OUT("\tERROR in our regmap, dumping contents remaining[%d]...\n",
                   countRegRemaining());
         print();    //something went wrong with our reg map, so print it out!
      }
  #endif
  //this should never be violated....
  ASSERT(m_freelist_index < m_num_physical);
  ASSERT(physical_reg < m_num_physical);
  ASSERT(m_physical->isReady(physical_reg) == false);
  ASSERT(m_physical->isWaitEmpty(physical_reg));
#ifdef CHECK_REGFILE
  ASSERT(m_bits->getBit(physical_reg) == true);
#endif

  m_logical_rename_count[proc][logical_reg]++;
  m_physical->setFree(physical_reg, false);
#ifdef CHECK_REGFILE
  m_bits->setBit(physical_reg, false);
#endif

  //IMPORTANT NOTE: because m_freelist_index is UNSIGNED, when we hit the 
  //      case m_freelist_index == 0, we will decrement it and it will wrap around to be 2^16.  
  //      Then the comparison countRegRemaining() > m_reserves will fail in arf.C
  m_freelist_index --;


  #ifdef DEBUG_REG
      DEBUG_OUT("reg_map_t:regAllocate END\n");
  #endif
  return physical_reg;
}

/// frees a logical register (param index The index to be freed)
//***************************************************************************
void 
reg_map_t::regFree(uint16 logical_reg, uint16 physical_reg, uint32 proc)
{

  ASSERT(proc < CONFIG_LOGICAL_PER_PHY_PROC);
#ifdef DEBUG_DYNAMIC_RET
  DEBUG_OUT("********reg_map_t:regFree BEGIN logical[%d] physical[%d] maxregs[%d]\n",
            logical_reg, physical_reg, m_num_physical);
 #endif
  ASSERT(physical_reg < m_num_physical);
#ifdef CHECK_REGFILE
  ASSERT(m_bits->getBit(physical_reg) == false);
  m_bits->setBit(physical_reg, true);
#endif
  // The register may or may not be ready -- if ready, is at retire,
  // if not ready, freed during squash --

  // wait list must be empty
  ASSERT(m_physical->isWaitEmpty(physical_reg));

  if (m_logical_rename_count[proc][logical_reg] == 0) {
    ERROR_OUT("reg_map: error: zero rename count at free. logical_reg %d\n",
              logical_reg);
    ASSERT(m_logical_rename_count[proc][logical_reg] != 0);
  }
  m_logical_rename_count[proc][logical_reg]--;
  m_freelist_index ++;
  #ifdef DEBUG_DYNAMIC_RET
       DEBUG_OUT("\t m_freelist_index[%d] max_regs[%d]\n",m_freelist_index, m_num_physical);
  #endif
  ASSERT(m_freelist_index < m_num_physical);
  m_freelist[m_freelist_index] = physical_reg;
  m_physical->setUnready(physical_reg);

#ifdef DEBUG_DYNAMIC_RET
  DEBUG_OUT("********reg_map_t:regFree END logical[%d] physical[%d] regmax[%d]\n",
            logical_reg, physical_reg,m_num_physical);
 #endif
}

/// returns the number of logical registers in flight
//***************************************************************************
uint16
reg_map_t::countInFlight( uint16 logical_reg, uint32 proc )
{
  ASSERT(proc < CONFIG_LOGICAL_PER_PHY_PROC);
  if ( logical_reg > m_num_logical ) {
    ERROR_OUT("error: countInFlight: illegal parmeter: %d\n", logical_reg);
    SIM_HALT;
  }
  return (m_logical_rename_count[proc][logical_reg]);
}

/// returns the number of physical registers remaining for allocation
//***************************************************************************
uint16
reg_map_t::countRegRemaining(void)
{
  // when m_freelist_index becomes negative, this signals that we have no more registers left:
  if(m_freelist_index < 0){
    return 0;
  }
  return (m_freelist_index + 1);
}

//**************************************************************************
bool *
reg_map_t::buildFreelist( ) const
{

    /* build a bitmap of allocated registers, then test all logical
     registers */

  /* initialize all as free */
  for (uint32 i = 0 ; i < m_num_physical; i ++ ) {
    m_validate_map[i] = true;
  }

  // do this for all rename maps:
  for(uint k=0; k < CONFIG_LOGICAL_PER_PHY_PROC; ++k){
    /* mark allocated ones as not free */
    for (uint32 i = 0 ; i < m_num_logical ; i ++ ) { 
      if (m_logical_reg[k][i] != PSEQ_INVALID_REG) {
        m_validate_map[m_logical_reg[k][i]] = false;
      }
    }
  }

  /* compare a list of the allocated registers from a logical register
     file versus the global logical free list */
  for (int i = m_freelist_index ; i >= 0 ; i --) {
    if (m_validate_map[m_freelist[i]] == true) {
      // set this register to be "free"
      m_validate_map[m_freelist[i]] = false;
    } else {
      // register should never be already "free"
      SIM_HALT;
    }
  }

  return (m_validate_map);
}

//***************************************************************************
void
reg_map_t::leakCheck( uint32 id, const char *map_name ) const
{
  /* check for 'register leaks' */
  for (int i = 0; i < (int) m_num_physical; i++) {
    if (m_validate_map[i] == true) {
      // not on free list or allocated
      ERROR_OUT("[%d] register leak: %s. %d not allocated, and not on free list\n",
                id, map_name, i);
      print();
      SIM_HALT;
    }
  }
}

//**************************************************************************
bool
reg_map_t::addFreelist( uint16 id )
{
  if (m_validate_map[id] == true) {
    m_validate_map[id] = false;
  } else {
    return false;
  }
  return true;
}

//**************************************************************************
bool 
reg_map_t::equals(reg_map_t *other, uint32 proc) 
{
  ASSERT(proc < CONFIG_LOGICAL_PER_PHY_PROC);
  ASSERT(m_physical == other->m_physical);

  /* index from logical register to physical register */
  for (int i = 0 ; i < (int) m_num_logical; i ++) {
    if (m_logical_reg[proc][i] != other->m_logical_reg[proc][i]) {
      return false;
    }
  }
  return true;
}

//***************************************************************************
void
reg_map_t::print() const
{
  DEBUG_OUT("m_num_logical[%d]\n",m_num_logical);
  DEBUG_OUT("m_num_physical[%d]\n",m_num_physical);
  DEBUG_OUT("m_freelist_index[%d]\n",m_freelist_index);
  //check if there are any register map "holes":
  bool * map_check = buildFreelist();
  if(map_check == NULL){
    DEBUG_OUT("ERROR in map check!\n");
  }
  else{
    leakCheck(0, "corrupt_reg_map_debug");
  }

  for(uint k=0; k < CONFIG_LOGICAL_PER_PHY_PROC; ++k){
    DEBUG_OUT("\nlogical reg  -->  physical reg [logical proc %d]\n", k);
    /* index from logical register to physical register */
    for (int i = 0 ; i < (int) m_num_logical; i ++) {
      DEBUG_OUT("%3d %3d\n", i, m_logical_reg[k][i]);
    }
  }

  DEBUG_OUT("freelist\n");
  for (int i = m_freelist_index ; i >= 0 ; i --) {
    DEBUG_OUT("    %3d\n", m_freelist[i] );
  }  
}

