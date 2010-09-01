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
 * FileName:  dtlb.C
 * Synopsis:  Implements the TLB
 * Author:    zilles
 * Version:   $Id: dtlb.C,v 3.6 2003/07/10 18:07:47 milo Exp $
 */

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include "hfa.h"
#include "bitlib.h"
#include "dtlb.h"

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Variable declarations                                                  */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Forward declarations                                                   */
/*------------------------------------------------------------------------*/

// C++ Template: explicit instantiation
template class map<la_t, dtlb_entry_t *>;

/*------------------------------------------------------------------------*/
/* Constructor(s) / destructor                                            */
/*------------------------------------------------------------------------*/

//**************************************************************************
dtlb_t::dtlb_t( uint32 dtlb_size, uint32 num_page_sizes, uint32 *page_sizes )
{
  char buffer[200];
  m_size = dtlb_size;
  m_num_pages = num_page_sizes;
  m_page_size = (uint32 *) malloc( sizeof(uint32) * m_num_pages );
  m_page_mask = (uint64 *) malloc( sizeof(uint64) * m_num_pages );
  m_table = new AddressTranslationMap[m_num_pages];
  for (uint32 i = 0; i < m_num_pages; i++) {
    m_page_size[i] = page_sizes[i];

    // makemask starts with bit 0 (necessitating "-1" on page size)
    m_page_mask[i] = ~makeMask64( 0, log_base_two(page_sizes[i]) - 1 );
    sprintBits64(m_page_mask[i], buffer, 64 );
    // DEBUG_OUT("DTLB: size: %d bitmask: %s\n", m_page_size[i], buffer );
  }
}

//**************************************************************************
dtlb_t::~dtlb_t( void )
{
  if (m_page_size)
    free( m_page_size );
  if (m_page_mask)
    free( m_page_mask );
  delete [] m_table;
}

/*------------------------------------------------------------------------*/
/* Public methods                                                         */
/*------------------------------------------------------------------------*/

//**************************************************************************
bool dtlb_t::translateAddress( la_t va, context_id_t context, 
                               uint32 accessSize, bool isWrite,
                               pa_t *pa )
{
  for (uint32 i = 0; i < m_num_pages; i++) {
    uint64 page_address = va & m_page_mask[i];
    
    // look in table[i] for match
    if (m_table[i].find( page_address ) != m_table[i].end()) {

      dtlb_entry_t *entry = m_table[i][page_address];
      // compare the context id for a match
      while (entry != NULL) {
        if (entry->m_context == context) {
          // complete match: return the translation
          *pa = entry->p_address | (va & ~m_page_mask[i]);
          // DEBUG_OUT("FOUND: 0x%0llx\n", *pa);
          return true;
        }
        entry = entry->m_next;
      }
    }
  }
  // no match
  //DEBUG_OUT("NOT FOUND: 0x%0llx 0x%0x\n", va, context);
  return false;
}

//**************************************************************************
void dtlb_t::insertEntry( dtlb_entry_t *entry )
{
  uint32 page_size = (uint32) -1;
  for (uint32 i = 0; i < m_num_pages; i++) {
    if (m_page_size[i] == entry->m_pagesize) {
      page_size = i;
    }
  }
  if (page_size == (uint32) -1) {
    ERROR_OUT("error: dTLB: pagesize: 0x%0x not present in configuration.\n",
              entry->m_pagesize);
    return;
  }

  uint64 page_address = entry->v_address & m_page_mask[page_size];
  AddressTranslationMap &table = m_table[page_size];
  // look in table[page_size] for match
  if (table.find( page_address ) == table.end()) {
    // not found, insert new node
    table[page_address] = entry;
  } else {
    // found, add to end of list (if a different context)
    dtlb_entry_t *entry_list = table[page_address];
    while (entry_list != NULL) {
      if (entry_list->m_context == entry->m_context) {
        // already existing in this map: report a warning
        DEBUG_OUT("warning: translation already exists in map for context 0x%0x\n", entry->m_context);
        DEBUG_OUT("       : existing translation 0x%0llx -> 0x%0llx\n",
                  entry_list->v_address, entry_list->p_address );
        DEBUG_OUT("       : adding   translation 0x%0llx -> 0x%0llx\n",
                  entry->v_address, entry->p_address );
        return;
      }
      entry_list = entry_list->m_next;
    }

    // append our entry to the front of the list
    entry->m_next = table[page_address];
    table[page_address] = entry;
  }
}

//**************************************************************************
void dtlb_t::addTranslation( la_t va, context_id_t context, 
                             pa_t pa, uint32 page_size )
{
  // add the record to the translation information
  dtlb_entry_t *entry = new dtlb_entry_t( va, pa, context, page_size );
  // insert looking for conflicting virtual address
  insertEntry( entry );  
}

//**************************************************************************
void dtlb_t::readTranslation( FILE *fp )
{
  char  buffer[400];
  char *token;
  if (fp == NULL)
    return;
  
  la_t         va;
  pa_t         pa;
  context_id_t context;
  uint32       pagesize;

  while (fgets( buffer, 400, fp ) != NULL) {
    // scan the line for the following record format
    // Virtual Address     Physical Address    Context Size
    // 0x0000000010000000  0x000000007fc00000   0000  4M
    token = strtok( buffer, " " );
    va = strtoull( token, NULL, 0 );
    token = strtok( NULL, " " );
    pa = strtoull( token, NULL, 0 );
    token = strtok( NULL, " " );
    context = strtoul( token, NULL, 16 );
    token = strtok( NULL, " " );
    pagesize = strtoul( token, NULL, 0 );
    /* DEBUG_OUT( "0x%0llx 0x%0llx %x %d\n",
       va, pa, context, pagesize );
    */

    // add the record to the translation information
    dtlb_entry_t *entry = new dtlb_entry_t( va, pa, context, pagesize );
    // insert looking for conflicting virtual address
    insertEntry( entry );
  }
}

//**************************************************************************
void dtlb_t::writeTranslation( FILE *fp )
{
  AddressTranslationMap::iterator iter;
  dtlb_entry_t *entry;

  for (iter = m_table->begin(); iter != m_table->end(); iter++) {
    entry = (*iter).second;
    fprintf( fp, "0x%0llx  0x%0llx   %x  %d\n",
             entry->v_address, entry->p_address, entry->m_context,
             entry->m_pagesize );
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
