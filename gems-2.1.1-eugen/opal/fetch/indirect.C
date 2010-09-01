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
 * FileName:  indirect.C
 * Synopsis:  cascaded indirect branch predictor
 * Author:    zilles
 * Version:   $Id$
 */

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include "hfa.h"
#include "hfacore.h"
#include "confio.h"

#ifdef I_DUMP
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
int i_dump;
#endif /* I_DUMP */

/*------------------------------------------------------------------------*/
/* Variable declarations                                                  */
/*------------------------------------------------------------------------*/

/// number of fields in indirect predictor (table, except_table, except_tags)
const uint32 CASCADED_CHECKPT_FIELDS = 3;

/*------------------------------------------------------------------------*/
/* Constructor(s) / destructor                                            */
/*------------------------------------------------------------------------*/

//**************************************************************************
cascaded_indirect_t::cascaded_indirect_t() {
  m_table        = (my_addr_t *) malloc( CAS_TABLE_SIZE * sizeof(my_addr_t) );
  m_except_table = (my_addr_t *) malloc( CAS_EXCEPT_SIZE * sizeof(my_addr_t) );
  m_except_tags  = (my_addr_t *) malloc( CAS_EXCEPT_SIZE * sizeof(my_addr_t) );
  memset((void *)m_table, 0, CAS_TABLE_SIZE * sizeof(my_addr_t));
  memset((void *)m_except_table, 0, CAS_EXCEPT_SIZE * sizeof(my_addr_t));
  memset((void *)m_except_tags, 0, CAS_EXCEPT_SIZE * sizeof(my_addr_t));

#ifdef I_DUMP
  i_dump = creat("indirect.trace", S_IRUSR | S_IWUSR); 
#endif /* I_DUMP */
}

//**************************************************************************
cascaded_indirect_t::~cascaded_indirect_t() {
  if (m_table != NULL) {
    free( m_table );
    m_table = NULL;
  }
  if (m_except_table != NULL) {
    free( m_except_table );
    m_except_table = NULL;
  }
  if (m_except_tags != NULL) {
    free( m_except_tags );
    m_except_tags = NULL;
  }
}

/*------------------------------------------------------------------------*/
/* Public methods                                                         */
/*------------------------------------------------------------------------*/

//**************************************************************************
my_addr_t
cascaded_indirect_t::Predict(my_addr_t branch_PC, indirect_state_t *is,
                             uint64 *exception_stat_hit, uint32 proc) {
  /* munge the branch_PC to generate index; read table */
  word_t index = GenerateIndex(branch_PC, proc);
  ASSERT(index < CAS_TABLE_SIZE);
  word_t except_index = GenerateExceptIndex(branch_PC, is, proc);
  ASSERT(except_index < CAS_EXCEPT_SIZE);
  word_t except_tag = GenerateExceptTag(branch_PC, is, proc);

  my_addr_t ret_val;
  if (m_except_tags[except_index] == except_tag) {
    ret_val = m_except_table[except_index];
    // increment stat counter for exception hits
    STAT_INC(*exception_stat_hit);
  } else {
    ret_val = m_table[index];
  }
  
  /* update history */
  UpdateHistory(is, ret_val);
  return ret_val;
}

//**************************************************************************
void
cascaded_indirect_t::Update( my_addr_t branch_PC, indirect_state_t *is, 
                             my_addr_t target_PC, uint32 proc ) {

  /* we implement a leaky update scheme with a first level table
     without tags.  This means if the first level hits we only update
     the second level if it has already been allocated; if the first
     level prediction is wrong then we update both tables. */

#ifdef I_DUMP
  write(i_dump, &branch_PC, sizeof(my_addr_t));
  write(i_dump, &target_PC, sizeof(my_addr_t));
#endif /* I_DUMP */

  word_t index = GenerateIndex(branch_PC, proc);
  ASSERT(index < CAS_TABLE_SIZE);
  word_t except_index = GenerateExceptIndex(branch_PC, is, proc);
  ASSERT(except_index < CAS_EXCEPT_SIZE);
  word_t except_tag = GenerateExceptTag(branch_PC, is, proc);

  if (m_except_tags[except_index] == except_tag) {
    m_except_table[except_index] = target_PC;
  } else if (m_table[index] != target_PC) {
    /* only update the second level table if already allocated */
    m_table[index] = target_PC;
    m_except_tags[except_index] = except_tag;
    m_except_table[except_index] = target_PC;
  }
}

//**************************************************************************
word_t 
cascaded_indirect_t::GenerateIndex(my_addr_t branch_PC, uint32 proc) {
  //word_t munged_PC = ((branch_PC >> 2) & 0xffffffff);
  word_t munged_PC =  (( (branch_PC ^ (proc << 20)) >> 2) & 0xffffffff);
  word_t index = ((munged_PC ^ (munged_PC >> CAS_TABLE_BITS) ^ 
                   (munged_PC >> (2 * CAS_EXCEPT_BITS))) & CAS_TABLE_MASK);
  return index;
}

//**************************************************************************
word_t 
cascaded_indirect_t::GenerateExceptIndex(my_addr_t branch_PC, 
                                         indirect_state_t *is, uint32 proc) {
  // word_t shifted_PC = ((branch_PC >> 2) & 0xffffffff);
  word_t shifted_PC =   (( (branch_PC ^ (proc << 20)) >> 2) & 0xffffffff);
  word_t munged_PC = shifted_PC ^ (shifted_PC >> CAS_EXCEPT_BITS) ^ 
    (shifted_PC >> (2 * CAS_EXCEPT_BITS));
  word_t history = (*is & 0xff) ^ (((*is >> 8) & 0xff) << CAS_EXCEPT_SHIFT) ^
    (((*is >> 16) & 0xff) << (2 * CAS_EXCEPT_SHIFT));
  return ((history ^ munged_PC) & CAS_EXCEPT_MASK);
}

//**************************************************************************
word_t 
cascaded_indirect_t::GenerateExceptTag(my_addr_t branch_PC, 
                                       indirect_state_t *is, uint32 proc) {
  // return  ((branch_PC >> 2) & 0xffffffff);
  return (word_t) (( (branch_PC ^ (proc << 20)) >> 2) & 0xffffffff);
}

//**************************************************************************
void 
cascaded_indirect_t::UpdateHistory(indirect_state_t *old_state, 
                                   my_addr_t addr) const {
  word_t PC = addr >> 2;
  *old_state = (*old_state << 8) | ((PC ^ (PC >> 8)) & 0xff);
}


//**************************************************************************
attr_value_t  cascaded_indirect_t::get_data( void )
{
  attr_value_t  ret;
  uint32        arraysize = 0;
  my_addr_t    *array = NULL;
  attr_value_t *vec;
  attr_value_t *subvec;

  memset( &ret, 0, sizeof(attr_value_t) );
  ret.kind = Sim_Val_List;
  vec = mallocAttribute(CASCADED_CHECKPT_FIELDS);
  ret.u.list.size   = CASCADED_CHECKPT_FIELDS;
  ret.u.list.vector = vec;
  
  for (uint32 i = 0; i < CASCADED_CHECKPT_FIELDS; i++ ) {
    
    if ( i == 0 ) {
      arraysize = CAS_TABLE_SIZE;
      array = m_table;
    } else if ( i == 1 ) {
      arraysize = CAS_EXCEPT_SIZE;
      array = m_except_table;
    } else if ( i == 2 ) {
      arraysize = CAS_EXCEPT_SIZE;
      array = m_except_tags;
    }

    vec[i].kind          = Sim_Val_List;
    vec[i].u.list.size   = arraysize;
    subvec               = mallocAttribute(arraysize);
    vec[i].u.list.vector = subvec;
    
    for (uint32 j = 0; j < arraysize; j++) {
      subvec[j].kind      = Sim_Val_Integer;
      subvec[j].u.integer = array[j];
    }
  }
  return (ret);
}

//**************************************************************************
set_error_t   cascaded_indirect_t::set_data( attr_value_t *value )
{
  attr_value_t    *vec;
  attr_value_t    *subvec;
  uint32           arraysize = 0;
  my_addr_t       *array = NULL;

  if (value->kind != Sim_Val_List)
    return Sim_Set_Need_List;
  if (value->u.list.size != CASCADED_CHECKPT_FIELDS)
    return Sim_Set_Illegal_Value;
  
  vec = value->u.list.vector;
  for (uint32 i = 0; i < CASCADED_CHECKPT_FIELDS; i++ ) {
    if (vec[i].kind != Sim_Val_List)
      return Sim_Set_Need_List;

    if ( i == 0 ) {
      arraysize = CAS_TABLE_SIZE;
      array = m_table;
    } else if ( i == 1 ) {
      arraysize = CAS_EXCEPT_SIZE;
      array = m_except_table;
    } else if ( i == 2 ) {
      arraysize = CAS_EXCEPT_SIZE;
      array = m_except_tags;
    }
    if (vec[i].u.list.size != arraysize)
      return Sim_Set_Illegal_Value;
    
    subvec = vec[i].u.list.vector;
    for (uint32 j = 0; j < arraysize; j++) {
      if (subvec[j].kind != Sim_Val_Integer) {
        ERROR_OUT("set_yags: illegal type (non integer)\n");
        return Sim_Set_Illegal_Value;
      }
      array[j] = subvec[j].u.integer;
    }
  }  
  return Sim_Set_Ok;
}

//**************************************************************************
static set_error_t   set_cascade_data( void *ptr, void *obj,
                                    attr_value_t *value )
{
  cascaded_indirect_t *bp = (cascaded_indirect_t *) ptr;
  return ( bp->set_data( value ));
}

//**************************************************************************
static attr_value_t  get_cascade_data( void *ptr, void *obj )
{
  cascaded_indirect_t *bp = (cascaded_indirect_t *) ptr;
  return ( bp->get_data() );
}

//**************************************************************************
int cascaded_indirect_t::registerCheckpoint( confio_t *conf )
{
  int  rc;

  rc = conf->register_attribute( "ipred_cascade",
                                 get_cascade_data, (void *) this,
                                 set_cascade_data, (void *) this );
  return (rc);

}
