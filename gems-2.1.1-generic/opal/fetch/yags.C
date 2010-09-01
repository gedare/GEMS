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
 * FileName:  yags.C
 * Synopsis:  A yags branch predictor
 * Author:    zilles
 * Version:   $Id$
 */

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include "hfa.h"
#include "confio.h"
#include "yags.h"

/*------------------------------------------------------------------------*/
/* Variable declarations                                                  */
/*------------------------------------------------------------------------*/

/** new saturated counter values, looked up based on old counter value
    and update direction */
static const byte_t   taken_update[4] = {1, 2, 3, 3};
static const byte_t   not_taken_update[4] = {0, 0, 1, 2};
static const uint32 YAGS_CHECKPT_FIELDS = 5;

/*------------------------------------------------------------------------*/
/* Constructor(s) / destructor                                            */
/*------------------------------------------------------------------------*/

//**************************************************************************
yags_t::yags_t( uint32 pht_entries, uint32 exception_entries, uint32 tag_bits )
{
  m_pht_size = 1 << pht_entries;
  m_pht_mask = m_pht_size - 1;

  /* initialize to weakly not-taken */
  m_pht = (byte_t *) malloc( m_pht_size * sizeof(byte_t) );
  memset( (void *) m_pht, 1, m_pht_size * sizeof(byte_t) );

  m_exception_size = 1 << exception_entries;
  m_exception_mask = m_exception_size - 1;
  m_tag_bits = tag_bits;
  m_tag_mask = (1 << m_tag_bits) - 1;

  /* initialize tags to all zeros (a basically random value) */
  tkn_tag   = (tag_t *) malloc( m_exception_size * sizeof(tag_t) );
  n_tkn_tag = (tag_t *) malloc( m_exception_size * sizeof(tag_t) );
  memset((void *)tkn_tag,  0, m_exception_size * sizeof(tag_t));
  memset((void *)n_tkn_tag,0, m_exception_size * sizeof(tag_t));

  /* initialize predictions to agree to bias */
  tkn_pred   = (byte_t *) malloc( m_exception_size * sizeof(byte_t) );
  n_tkn_pred = (byte_t *) malloc( m_exception_size * sizeof(byte_t) );
  memset((void *)tkn_pred, 3, m_exception_size * sizeof(tag_t));
  memset((void *)n_tkn_pred, 0, m_exception_size * sizeof(tag_t));
}

//**************************************************************************
yags_t::~yags_t()
{
  if (m_pht) {
    free( m_pht );
    m_pht = NULL;
  }
  if (tkn_tag) {
    free( tkn_tag );
    tkn_tag = NULL;
  }
  if (n_tkn_tag) {
    free( n_tkn_tag );
    n_tkn_tag = NULL;
  }
  if (tkn_pred) {
    free( tkn_pred );
    tkn_pred = NULL;
  }
  if (n_tkn_pred) {
    free( n_tkn_pred );
    n_tkn_pred = NULL;
  }
}

//**************************************************************************
bool yags_t::Predict(my_addr_t branch_PC, word_t history, bool staticPred, uint32 proc)
{
  word_t shifted_PC = MungePC(branch_PC, proc);
  word_t choice_index = GenerateChoice(shifted_PC, proc);
  ASSERT(choice_index < m_pht_size);

  bool taken_bias, predict;

  taken_bias = (m_pht[choice_index] >= 2);
  predict = taken_bias;

  word_t except_index = GenerateIndex(shifted_PC, history);  
  ASSERT(except_index < m_exception_size);
  tag_t tag = GenerateTag(shifted_PC, history);
  
  if (taken_bias) {
    if (tkn_tag[except_index] == tag) {
      // hit in the taken exception table
      predict = (tkn_pred[except_index] >= 2);
    }
  } else { 
    if (n_tkn_tag[except_index] == tag) {
      // hit in the not-taken exception table
      predict = (n_tkn_pred[except_index] >= 2);
    }
  }
  
  return predict;
}

//**************************************************************************
void yags_t::Update(my_addr_t branch_PC, word_t history,
                    bool staticPred, bool taken, uint32 proc)
{
  word_t shifted_PC = MungePC(branch_PC, proc);
  word_t choice_index = GenerateChoice(shifted_PC, proc);
  ASSERT(choice_index < m_pht_size);
  word_t except_index = GenerateIndex(shifted_PC, history);  
  ASSERT(except_index < m_exception_size);
  tag_t tag = GenerateTag(shifted_PC, history);

  byte_t old_pred = m_pht[choice_index];

  const byte_t *update_function;

  if (taken) { /* was taken */
    update_function = taken_update;
  } else { /* was not taken */
    update_function = not_taken_update;
  }

  m_pht[choice_index] = update_function[old_pred];

  if (old_pred > 1) { /* was biased taken */
    /* update entry if tag match */
    if (tkn_tag[except_index] == tag) {
      byte_t prediction = tkn_pred[except_index];
      tkn_pred[except_index] = update_function[prediction];
    } else if (!taken) {
      /* allocate entry if against bias */
      tkn_tag[except_index] = tag;
      tkn_pred[except_index] = 1;
    }
  } else { /* was biased not-taken */
    /* update entry if tag match */
    if (n_tkn_tag[except_index] == tag) {
      byte_t prediction = n_tkn_pred[except_index];
      n_tkn_pred[except_index] = update_function[prediction];
    } else if (taken) {
      /* allocate entry if against bias */
      n_tkn_tag[except_index] = tag;
      n_tkn_pred[except_index] = 2;
    }
  }
}

//**************************************************************************
void yags_t::printInformation( FILE *fp )
{
  if (fp) {
    fprintf(fp, "YAGS Branch Predictor\n");
    fprintf(fp, "   PHT size               : %d\n", m_pht_size );
    fprintf(fp, "   Exception table sizes  : %d\n", m_exception_size );
    fprintf(fp, "   Number of tag bits     : %d\n", m_tag_bits );
    fprintf(fp, "   size = 2*PHT  + 2*Except*(TAG + 2)\n");
    fprintf(fp, "   size = %d + %d         : %d B\n",
            2 * m_pht_size, 2 * m_exception_size * (m_tag_bits + 2),
            2 * m_pht_size + 2 * m_exception_size * (m_tag_bits + 2) );
  }
}

//**************************************************************************
attr_value_t  yags_t::get_data( void )
{
  attr_value_t  ret;
  uint32        arraysize = 0;
  byte_t         *array = NULL;
  attr_value_t *vec;
  attr_value_t *subvec;

  ASSERT( sizeof(tag_t) == sizeof(byte_t) );

  memset( &ret, 0, sizeof(attr_value_t) );
  ret.kind = Sim_Val_List;
  vec = mallocAttribute(YAGS_CHECKPT_FIELDS);
  ret.u.list.size   = YAGS_CHECKPT_FIELDS;
  ret.u.list.vector = vec;
  
  for (uint32 i = 0; i < YAGS_CHECKPT_FIELDS; i++ ) {
    
    if ( i == 0 ) {
      arraysize = m_pht_size;
      array = m_pht;
    } else if ( i == 1 ) {
      arraysize = m_exception_size;
      array = tkn_tag;
    } else if ( i == 2 ) {
      arraysize = m_exception_size;
      array = tkn_pred;
    } else if ( i == 3 ) {
      arraysize = m_exception_size;
      array = n_tkn_tag;
    } else if ( i == 4 ) {
      arraysize = m_exception_size;
      array = n_tkn_pred;
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
set_error_t   yags_t::set_data( attr_value_t *value )
{
  attr_value_t    *vec;
  attr_value_t    *subvec;
  uint32        arraysize = 0;
  byte_t         *array = NULL;

  if (value->kind != Sim_Val_List)
    return Sim_Set_Need_List;
  if (value->u.list.size != YAGS_CHECKPT_FIELDS)
    return Sim_Set_Illegal_Value;
  
  vec = value->u.list.vector;
  for (uint32 i = 0; i < YAGS_CHECKPT_FIELDS; i++ ) {
    if (vec[i].kind != Sim_Val_List)
      return Sim_Set_Need_List;

    if ( i == 0 ) {
      arraysize = m_pht_size;
      array = m_pht;
    } else if ( i == 1 ) {
      arraysize = m_exception_size;
      array = tkn_tag;
    } else if ( i == 2 ) {
      arraysize = m_exception_size;
      array = tkn_pred;
    } else if ( i == 3 ) {
      arraysize = m_exception_size;
      array = n_tkn_tag;
    } else if ( i == 4 ) {
      arraysize = m_exception_size;
      array = n_tkn_pred;
    }
    if (vec[i].u.list.size != arraysize)
      return Sim_Set_Illegal_Value;
    
    subvec = vec[i].u.list.vector;
    for (uint32 j = 0; j < arraysize; j++) {
      if (subvec[j].kind != Sim_Val_Integer) {
        DEBUG_OUT("set_yags: illegal type (non integer)\n");
        return Sim_Set_Illegal_Value;
      }
      array[j] = subvec[j].u.integer;
    }
  }  
  return Sim_Set_Ok;
}

//**************************************************************************
static set_error_t   set_yags_data( void *ptr, void *obj,
                                    attr_value_t *value )
{
  yags_t *bp = (yags_t *) ptr;
  return ( bp->set_data( value ));
}

//**************************************************************************
static attr_value_t  get_yags_data( void *ptr, void *obj )
{
  yags_t *bp = (yags_t *) ptr;
  return ( bp->get_data() );
}

//**************************************************************************
int yags_t::registerCheckpoint( confio_t *conf )
{
  int  rc;

  rc = conf->register_attribute( "bpred_yags",
                                 get_yags_data, (void *) this,
                                 set_yags_data, (void *) this );
  return (rc);

}
