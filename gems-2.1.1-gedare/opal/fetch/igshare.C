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
 * FileName:  igshare.C
 * Synopsis:  Implements an infinite (idealized?) gshare predictor
 * Author:    cmauer
 * Version:   $Id: igshare.C 1.3 04/08/11 16:50:50-05:00 lyen@opus.cs.wisc.edu $
 */

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include "hfa.h"
#include "confio.h"
#include "igshare.h"

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
/* Predictor Entry Class                                                  */
/*------------------------------------------------------------------------*/

//**************************************************************************
predictor_entry_t::predictor_entry_t( uint32 history_size )
  :
  m_bitfield( 1 << (history_size + 1) )
{
  if (history_size > 24) {
    ERROR_OUT("warning: huge history size: %d.\n", history_size);
  }
  m_history = 0;
  // update each bitfield to weakly taken
  for (int32 i = 0; i < (1 << history_size); i++) {
    // binary "1 0" == 2 which is weakly taken
    m_bitfield.setBit( i * 2, 1 );
  }
}

//**************************************************************************
predictor_entry_t::~predictor_entry_t( void )
{
  /// nothing allocated, nothing lost
}

/*------------------------------------------------------------------------*/
/* Constructor(s) / destructor                                            */
/*------------------------------------------------------------------------*/

//**************************************************************************
igshare_t::igshare_t( uint32 history_length, bool globalHistory )
{
  m_history_length = history_length;
  m_use_global = globalHistory;
  m_history_size = (1 << m_history_length);
  m_history_mask = m_history_size - 1;
}

//**************************************************************************
igshare_t::~igshare_t( void )
{
  // walk the per pc table freeing all predictor entries
  PerPCTable::iterator iter;
  for (iter = m_perPCtable.begin(); iter != m_perPCtable.end(); iter++){
    delete (*iter).second;
  }
}

/*------------------------------------------------------------------------*/
/* Public methods                                                         */
/*------------------------------------------------------------------------*/

//**************************************************************************
bool igshare_t::Predict(my_addr_t branch_PC, word_t history, bool staticPred, uint32 proc)
{
  // look the PC up in the per PC table
  predictor_entry_t  *predictor;
  if ( m_perPCtable.find( branch_PC ) == m_perPCtable.end() ) {
    // create a new per pc predictor entry
    predictor = new predictor_entry_t( m_history_length );
    m_perPCtable[branch_PC] = predictor;
  } else {
    predictor = m_perPCtable[branch_PC];
  }

  // use the (masked) history to access the 2-bit predictor
  uint32  tableentry = 0;  // 2 bit value in table
  uint32  index = 0;
  if ( m_use_global ) {
    // use the global history to access the table
    /// multiply by 2 (<< 1) because each predictor is a 2-bit predictor
    index = (history & m_history_mask) << 1;
  } else {
    // use the local history to access the table
    index = (predictor->m_history & m_history_mask) << 1;
  }

  tableentry = ( (predictor->m_bitfield.getBit( index ) << 1) |
                  predictor->m_bitfield.getBit( index + 1 ) );
  bool predict = (tableentry >= 2);
  
  return predict;
}

//**************************************************************************
void igshare_t::Update(my_addr_t branch_PC, cond_state_t history,
                       bool staticPred, bool taken, uint32 proc)
{
  // look the PC up in the per PC table
  predictor_entry_t  *predictor;
  ASSERT ( m_perPCtable.find( branch_PC ) != m_perPCtable.end() );
  predictor = m_perPCtable[branch_PC];

  // use the (masked) history to access the 2-bit predictor
  uint32  tableentry = 0;  // 2 bit value in table
  uint32  index = 0;
  if ( m_use_global ) {
    // use the global history to access the table
    /// multiply by 2 (<< 1) because each predictor is a 2-bit predictor
    index = (history & m_history_mask) << 1;
  } else {
    // use the local history to access the table
    index = (predictor->m_history & m_history_mask) << 1;
  }

  tableentry = ( (predictor->m_bitfield.getBit( index ) << 1) |
                  predictor->m_bitfield.getBit( index + 1 ) );
  if (taken) {
    tableentry = taken_update[tableentry];
  } else {
    tableentry = not_taken_update[tableentry];
  }

  // update the 2 bit counter
  predictor->m_bitfield.setBit( index, ((tableentry >> 1) & 0x1) );
  predictor->m_bitfield.setBit( index + 1, (tableentry & 0x1) );

  // update the per pc history
  predictor->m_history = ((predictor->m_history << 1) |
                          (taken & 0x1));
}

//**************************************************************************
void igshare_t::printInformation( FILE *fp )
{
  if (fp) {
    fprintf(fp, "INF GSHARE Branch Predictor\n");
    fprintf(fp, "   History size per PC      : %d\n", 1 << m_history_length );
    fprintf(fp, "   History is global        : %d\n", (int) m_use_global );
    fprintf(fp, "   inital value             : weakly taken\n");
    fprintf(fp, "   # of PCs                 : %d\n", 
            m_perPCtable.size() );
    fprintf(fp, "   Per PC size  (2*history) : %d\n", 
            (1 << (m_history_length + 1)));
    fprintf(fp, "   Total size               : %d\n",
            (1 << (m_history_length + 1)) * m_perPCtable.size());           
  }
}

//**************************************************************************
attr_value_t  igshare_t::get_data( void )
{
  attr_value_t  ret;
  uint32        arraysize = 0;
  attr_value_t *vec;
  attr_value_t *subvec;

  memset( &ret, 0, sizeof(attr_value_t) );
  ret.kind = Sim_Val_List;
  arraysize = m_perPCtable.size() + 3;
  vec = mallocAttribute( arraysize );
  ret.u.list.size   = arraysize;
  ret.u.list.vector = vec;

  // save the history size
  vec[0].kind      = Sim_Val_Integer;
  vec[0].u.integer = m_history_length;
  
  // save the number of PCs
  vec[1].kind      = Sim_Val_Integer;
  vec[1].u.integer = m_perPCtable.size();

  // save true if we used global history
  vec[2].kind      = Sim_Val_Integer;
  vec[2].u.integer = m_use_global;

  // walk the per pc table saving each global history as an array of integers
  uint32 i = 3;
  PerPCTable::iterator iter;
  la_t                 vpc;
  predictor_entry_t   *predictor;

  for (iter = m_perPCtable.begin(); iter != m_perPCtable.end(); iter++){

    ASSERT( i < arraysize );
    vpc = (*iter).first;
    predictor = (*iter).second;
    vec[i].kind        = Sim_Val_List;
    vec[i].u.list.size = m_history_size + 1;
    subvec             = mallocAttribute( m_history_size + 1);
    vec[i].u.list.vector = subvec;
    
    // forgive: tack the vpc at the beginning of the list
    subvec[0].kind      = Sim_Val_Integer;
    subvec[0].u.integer = vpc;

    for (uint32 index = 0; index < m_history_size; index++ ) {
      uint32 tableentry = ( (predictor->m_bitfield.getBit( 2*index ) << 1) |
                            predictor->m_bitfield.getBit( 2*index + 1 ) );
      subvec[index + 1].kind      = Sim_Val_Integer;
      subvec[index + 1].u.integer = tableentry;
    }
    i++;
  }

  return (ret);
}

//**************************************************************************
set_error_t   igshare_t::set_data( attr_value_t *value )
{
  attr_value_t    *vec;
  attr_value_t    *subvec;
  uint32           tablesize = 0;

  if (value->kind != Sim_Val_List)
    return Sim_Set_Need_List;
  vec = value->u.list.vector;

  // read the first three fields
  if (vec[0].kind != Sim_Val_Integer)
    return Sim_Set_Need_Integer;

  if (vec[1].kind != Sim_Val_Integer)
    return Sim_Set_Need_Integer;

  if (vec[2].kind != Sim_Val_Integer)
    return Sim_Set_Need_Integer;

  if ( vec[0].u.integer != m_history_length ) {
    ERROR_OUT( "warning: conf mismatch: igshare: history length %lld %d\n",
            vec[0].u.integer, m_history_length );
    return Sim_Set_Need_Integer;
  }

  // get the size of the table
  tablesize = vec[1].u.integer;

  if ( vec[2].u.integer != m_use_global ) {
    ERROR_OUT( "warning: conf mismatch: igshare: use global %lld %d\n",
            vec[2].u.integer, m_use_global );
    return Sim_Set_Need_Integer;
  }
  
  // for each vpc, add a new entry to the perPCtable
  uint32 i = 3;
  la_t   vpc = 0;
  predictor_entry_t   *predictor;
  while (i < tablesize + 3) {

    // ERROR_OUT("I = %d\n", i);
    if (vec[i].kind != Sim_Val_List) {
      ERROR_OUT("igshare: conf file error: i=%d not a list.\n", i);
      return Sim_Set_Need_List;
    }
    
    subvec = vec[i].u.list.vector;
    vpc = subvec[0].u.integer;

    predictor = new predictor_entry_t( m_history_length );
    m_perPCtable[vpc] = predictor;
    
    for (uint32 index = 0; index < m_history_size; index++ ) {
      if (subvec[index + 1].kind != Sim_Val_Integer) {
        ERROR_OUT("igshare: conf file: subvec=%d not an integer.\n", index + 1);
        return Sim_Set_Need_Integer;
      }
      uint32 tableentry = subvec[index + 1].u.integer;
      // DEBUG_OUT("index %d = %d\n", index, tableentry);
      
      // set the 2 bit counter
      predictor->m_bitfield.setBit( 2*index, ((tableentry >> 1) & 0x1) );
      predictor->m_bitfield.setBit( 2*index + 1, (tableentry & 0x1) );
    }
    i++;
  }
  return Sim_Set_Ok;
}

//**************************************************************************
static set_error_t   set_igshare_data( void *ptr, void *obj,
                                    attr_value_t *value )
{
  igshare_t *bp = (igshare_t *) ptr;
  return ( bp->set_data( value ));
}

//**************************************************************************
static attr_value_t  get_igshare_data( void *ptr, void *obj )
{
  igshare_t *bp = (igshare_t *) ptr;
  return ( bp->get_data() );
}

//**************************************************************************
int igshare_t::registerCheckpoint( confio_t *conf )
{
  int  rc;

  rc = conf->register_attribute( "bpred_igshare",
                                 get_igshare_data, (void *) this,
                                 set_igshare_data, (void *) this );
  return (rc);
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

