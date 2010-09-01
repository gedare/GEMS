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
 * FileName:  fatpredict.C
 * Synopsis:  Fully Associative Table predictor
 * Author:    cmauer
 * Version:   $Id: fatpredict.C 1.2 03/07/10 18:07:46-00:00 milo@cottons.cs.wisc.edu $
 */

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include "hfa.h"
#include "fatpredict.h"

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Variable declarations                                                  */
/*------------------------------------------------------------------------*/

static const char  taken_update[4] = {1, 2, 3, 3};
static const char  not_taken_update[4] = {0, 0, 1, 2};

/*------------------------------------------------------------------------*/
/* Forward declarations                                                   */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Constructor(s) / destructor                                            */
/*------------------------------------------------------------------------*/

//**************************************************************************
fatpredict_t::fatpredict_t( int32 num_entries, uint32 history_bits,
                            uint32 tag_bits )
{
  m_num_entries = num_entries;
  m_history_bits = history_bits;
  m_history_mask = (1 << m_history_bits) - 1;
  m_tag_bits = tag_bits;
  m_fatarray = new fatentry_t[num_entries];
  for (int32 i = 0; i < num_entries; i++) {
    m_fatarray[i].alloc( history_bits );
  }
  m_clock_index = 0;
}

//**************************************************************************
fatpredict_t::~fatpredict_t( void )
{
  delete[] m_fatarray;
}

/*------------------------------------------------------------------------*/
/* Public methods                                                         */
/*------------------------------------------------------------------------*/

//**************************************************************************
int32 fatpredict_t::searchTable( la_t vpc )
{
  for (int32 i = 0; i < m_num_entries; i++) {
    if (m_fatarray[i].m_tag == makeTag( vpc )) {
      return (i);
    }
  }
  return (-1);
}

//**************************************************************************
bool fatpredict_t::inTable( la_t vpc )
{
  return (searchTable(vpc) >= 0);
}

//**************************************************************************
int32 fatpredict_t::chooseReplacement( void )
{
  int32  replacement = m_clock_index;
  int32  index       = m_clock_index;
  uint32 accesses    = m_fatarray[replacement].m_accesses;
  
  for (int32 i = 0; i < m_num_entries; i++) {
    if (m_fatarray[index].m_accesses < accesses) {
      replacement = index;
      accesses    = m_fatarray[replacement].m_accesses;
    }
    index = index + 1 & (m_num_entries - 1);
  }
  m_clock_index = m_clock_index + 1 & (m_num_entries - 1);
  return replacement;
}

//**************************************************************************
int32 fatpredict_t::choosePromotion( void )
{
  int32  promotion   = m_clock_index;
  int32  index       = m_clock_index;
  uint32 accesses    = m_fatarray[promotion].m_accesses;
  
  for (int32 i = 0; i < m_num_entries; i++) {
    if (m_fatarray[index].m_accesses > accesses) {
      promotion = index;
      accesses  = m_fatarray[promotion].m_accesses;
    }
    index = index + 1 & (m_num_entries - 1);
  }
  
  if (accesses <= 100)
    promotion = -1;
  return promotion;
}

//**************************************************************************
void fatpredict_t::overwrite( int32 entry, la_t vpc, uint64 accesses )
{
  ASSERT( entry < m_num_entries );
  ASSERT( entry >= 0 );
  m_fatarray[entry].m_tag = makeTag( vpc );
  m_fatarray[entry].m_accesses = accesses;
  m_fatarray[entry].clearPredict( m_history_bits );
}

//**************************************************************************
bool fatpredict_t::predict( int32 entry, uint32 history, bool staticPred )
{
  ASSERT( entry < m_num_entries );
  ASSERT( entry >= 0 );

  // return the two bit prediction based on the history counter
  return (m_fatarray[entry].getPredict( history & m_history_mask ) >= 2 );
}

//**************************************************************************
void fatpredict_t::clearAccessCounter( void )
{
  for (int32 i = 0; i < m_num_entries; i++) {
    m_fatarray[i].m_accesses = 0;
  }
}

//**************************************************************************
void fatpredict_t::update( int32 entry, uint32 history, bool staticPred,
                           bool taken )
{
  // update the two bit counter
  char    oldpred = m_fatarray[entry].getPredict( history & m_history_mask);
  if (taken) {
    m_fatarray[entry].setPredict( history & m_history_mask,
                                  taken_update[oldpred] );
  } else {
    m_fatarray[entry].setPredict( history & m_history_mask,
                                  not_taken_update[oldpred] );
  }  
  
  // increment the access counter
  m_fatarray[entry].m_accesses++;
}

//**************************************************************************
void fatpredict_t::printInformation( FILE *fp )
{
  if (fp) {
    fprintf(fp, "FULLY-ASSOC Branch Predictor\n");
    fprintf(fp, "   Number of entries        : %d\n", m_num_entries);
    fprintf(fp, "   Size of history bits     : %d\n", m_history_bits);
    fprintf(fp, "   Number of tag bits       : %d\n", m_tag_bits);
    fprintf(fp, "   Size = 2*history*entries : %d\n",
            2*(1 << m_history_bits)*m_num_entries);
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

