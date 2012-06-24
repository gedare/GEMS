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
 * FileName:  mlpredict.C
 * Synopsis:  multi-level feedback predictor
 * Author:    cmauer
 * Version:   $Id: mlpredict.C 1.3 04/08/11 16:50:51-05:00 lyen@opus.cs.wisc.edu $
 */

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include "hfa.h"
#include "agree.h"
#include "mlpredict.h"

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
mlpredict_t::mlpredict_t( uint32 pht_entries, int32 table1size,
                          int32 table2size )
{
  m_backup_table = new agree_t( pht_entries );
  m_table1 = new fatpredict_t( table1size, 6, 8 );
  m_table2 = new fatpredict_t( table2size, 6, 8 );

  m_time = 0;
  m_table2_replacements = 0;
  m_table1_promotions = 0;
  m_table1_replacements = 0;
}

//**************************************************************************
mlpredict_t::~mlpredict_t( void )
{
  delete m_backup_table;
  delete m_table1;
  delete m_table2;
}

/*------------------------------------------------------------------------*/
/* Public methods                                                         */
/*------------------------------------------------------------------------*/

//**************************************************************************
bool mlpredict_t::Predict(my_addr_t branch_PC, word_t history, bool staticPred, uint32 proc)
{
#if 0
  // increment predition "time"
  m_time = (m_time + 1) % 1000;
  if (m_time == 999) {

    // walk table one finding something to promote
    int32 promote = m_table1->choosePromotion();
    if (promote != -1) {
      m_table1_promotions++;
      int32 l2replace = m_table2->chooseReplacement();
      uint32 accesses = m_table1->getEntry(promote)->m_accesses;
      m_table2->overwrite( l2replace, branch_PC, accesses );

      // clear out table1 entry
      m_table1->overwrite( promote, branch_PC, 0 );
    }
  }
#endif

  // check to see if its in the highest table (#2) first
  int32  entry;
  entry = m_table2->searchTable( branch_PC );
  if (entry >= 0) {
    return (m_table2->predict( entry, history, staticPred ));
  }
  
  // check to see if its in the second table (#1)
  entry = m_table1->searchTable( branch_PC );
  if (entry >= 0) {
    return (m_table1->predict( entry, history, staticPred ));
  }

  // use the traditional 2-bit gshare algorithm
  return (m_backup_table->Predict( branch_PC, history, staticPred, proc ));
}

//**************************************************************************
void mlpredict_t::Update(my_addr_t branch_PC, cond_state_t history,
                         bool staticPred, bool taken, uint32 proc)
{
  // update the highest table (#2) first, if applicable
  int32  entry;
  entry = m_table2->searchTable( branch_PC );
  if (entry >= 0) {
    m_table2->update( entry, history, staticPred, taken );
    return;
  }
  
  // update the gshare predictor
  m_backup_table->Update( branch_PC, history, staticPred, taken, proc );

#if 0
  // check to see if its in the second table (#1)
  entry = m_table1->searchTable( branch_PC );
  if ( entry == -1 ) {
    // update the gshare predictor
    m_backup_table->Update( branch_PC, history, staticPred, taken );

    // always put information into the 1st level table

    // entry in l1 table
    int32  replace;
    m_table1_replacements++;
    // choose an entry to replace at l1 table
    replace = m_table1->chooseReplacement();
#if 0
    replace = m_table1->choosePromotion();
    if (replace != -1) {
      // promote this entry to the l2 table
      m_table1_promotions++;
      m_table2_replacements++;
      int32 l2replace = m_table2->chooseReplacement();
      m_table2->overwrite( l2replace, branch_PC );
    } else {

    }
#endif
    m_table1->overwrite( replace, branch_PC, 1 );

  } else {
    m_table1->update( entry, history, staticPred, taken );
  }
#endif

}

//**************************************************************************
void mlpredict_t::readTags( char *filename )
{
  // read the tags from the file "filename"
  FILE            *tagfp = NULL;
  char             buffer[200];
  my_addr_t        inputtag;

  tagfp = fopen( "my-mlpredict.txt", "r" );
  if (tagfp == NULL) {
    ERROR_OUT("warning: unable to open tag file\n");
    return;
  }

  int32 index = 0;
  while ( fgets( buffer, 200, tagfp ) != NULL ) {
    inputtag = strtoull( buffer, NULL, 0 );
    m_table2->overwrite( index, inputtag, 0 );
    index++;
  }
#if 0
  // print out the tags to check they were read in properly
  for (uint32 i = 0; i < count; i++) {
    DEBUG_OUT("0x%0llx\n", tagarray[i]);
  }
#endif
}

//**************************************************************************
void mlpredict_t::printInformation( FILE *fp )
{
  if (fp) {
    fprintf(fp, "MLPREDICT Branch Predictor\n");
    fprintf(fp, "   Table 2                : %d\n", m_table2->getSize() );
    fprintf(fp, "   Table 1                : %d\n", m_table1->getSize() );
    
    fprintf(fp, "   Table 2 replacements   : %d\n", m_table2_replacements);
    fprintf(fp, "   Table 1 promotions     : %d\n", m_table1_promotions);
    fprintf(fp, "   Table 1 replacements   : %d\n", m_table1_replacements);
    fprintf(fp, "Top branches: TABLE 2\n");
    for (int i = 0; i < m_table2->getSize(); i++) {
      fprintf(fp, "0x%0llx %lld\n", m_table2->getEntry(i)->m_tag,
              m_table2->getEntry(i)->m_accesses );
    }
    fprintf(fp, "Top branches: TABLE 1\n");
    for (int i = 0; i < m_table1->getSize(); i++) {
      fprintf(fp, "0x%0llx %lld\n", m_table1->getEntry(i)->m_tag,
              m_table1->getEntry(i)->m_accesses );
    }
    m_backup_table->printInformation(fp);
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

