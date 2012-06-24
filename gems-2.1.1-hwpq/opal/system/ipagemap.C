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
 * FileName:  ipagemap.C
 * Synopsis:  implementation of a hash table for instruction pages
 * Author:    cmauer
 * Version:   $Id: ipagemap.C 1.13 04/08/09 11:27:00-05:00 lyen@opus.cs.wisc.edu $
 */

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include "hfa.h"
#include "bitlib.h"
#include "fileio.h"
#include "ipagemap.h"

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

/// declaration of ipagemap file location
const char *IPAGEMAP_FILENAME = "./data/imap";

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
ipagemap_t::ipagemap_t( uint32 tablesize )
{
  // check to see tablesize is a power of two
  uint32 exponent = (uint32) ( log_base_two(tablesize) );
  // compute 2^exponent
  uint32 check2   = (uint32) ( 1 << exponent );
  if ( check2 != tablesize ) {
    ERROR_OUT("ipagemap_t: error tablsize must be a power of two! (%d is not)\n",
              tablesize);
    exit(1);
  }
  
  m_table = (imapentry_t **) malloc( sizeof(imapentry_t *) * tablesize );
  memset( m_table, 0, sizeof(imapentry_t *) * tablesize );
  m_tablesize = tablesize;
  m_tablebits = exponent;

  // make bitmask 0x00...111 (where the number of 1's == the tablesize)
  m_hashmask = maskBits32( ~0, 0, exponent-1 );
  m_num_instr = 0;
  m_num_totalpages = 0;
  m_num_collisions = 0;
}

//**************************************************************************
ipagemap_t::~ipagemap_t( )
{
  imapentry_t *entry;
  imapentry_t *nextentry;
  // free entries in the table
  for (uint32 index = 0; index < m_tablesize; index++) {
    if (m_table[index]) {
      entry = m_table[index];
      while (entry != NULL) {
        delete entry->m_entry;
        nextentry = entry->m_next;
        delete entry;
        entry = nextentry;
      }
    }
  }  
  // free the table
  if (m_table)
    free( m_table );
}

/*------------------------------------------------------------------------*/
/* Public methods                                                         */
/*------------------------------------------------------------------------*/

//**************************************************************************
imapentry_t *ipagemap_t::queryLocation( pa_t address, uint32 &hash_index )
{
  uint32 index = hash_function(address);
  pa_t   tag   = address >> IPAGE_PAGE_BITS;

  imapentry_t  *entry;
  entry = m_table[index];
  while (entry != NULL) {
    if (entry->m_tag == tag) {
      return (entry);
    }
    entry = entry->m_next;
  }

  hash_index = index;
  return (NULL);
}

//**************************************************************************
bool ipagemap_t::readFile( char *infilename )
{
  char         filename[200];
  FILE        *fp;
  uint32       index;
  ipage_t     *ipage;
  imapentry_t *entry;
  pa_t         nexttoken;
  int          numread;
  uint32       totalread = 0;
  pa_t         tag;
  bool         rc;

  if (infilename == NULL) {
    sprintf( filename, "%s-p%d-%02d", IPAGEMAP_FILENAME, 1, 0 );
  } else {
    strcpy( filename, infilename );
  }
  fp = fopen( filename, "r" );
  if (fp == NULL) {
    ERROR_OUT("unable to open file: %s\n", filename);
    return (false);
  }

  for ( ; ; ) {

    // read the next token from the file
    numread = myfread( &nexttoken, sizeof(pa_t), 1, fp );
    if (numread != 1) {
      if (feof(fp))
        break;
      ERROR_OUT("ipagemap: read fails: readPage (%d)\n", numread);
      return (false);
    }
    // check that this is a new table entry
    if (nexttoken != IPAGEMAP_INDEX_TOKEN) {
      ERROR_OUT("ipagemap: read page fails: invalid token: 0x%0llx\n", nexttoken);
      return (false);
    }
    // read the index from the file
    numread = myfread( &index, sizeof(uint32), 1, fp );
    if (numread != 1) {
      ERROR_OUT("ipagemap: read fails: readPage (%d)\n", numread);
      return (false);
    }

    for ( ; ; ) {

      // read the next tag
      numread = myfread( &tag, sizeof(pa_t), 1, fp );
      if (numread != 1) {
        if (feof(fp))
          break;
        ERROR_OUT("ipagemap: read fails: readPage (%d)\n", numread);
        return (false);
      }
      if (tag == IPAGEMAP_DONE_TOKEN) {
        break;
      }
      ipage = new ipage_t();
      rc = ipage->readPage( fp, tag, totalread );  
      if (!rc) {
        ERROR_OUT("error reading ipage!\n");
        return (false);
      }
      entry = new imapentry_t( ipage, tag );

      // This reverses the order in the list, but that should be OK
      entry->m_next = m_table[index];
      m_table[index] = entry;
    }
  }

  m_num_instr = totalread;
  fclose( fp );
  return (true);
}

//**************************************************************************
bool ipagemap_t::saveFile( char *outfilename )
{
  char         filename[200];
  FILE        *fp;
  uint32       index;
  imapentry_t *entry;
  pa_t         nexttoken;
  int          numwritten;

  if (outfilename == NULL) {
    sprintf( filename, "%s-p%d-%02d", IPAGEMAP_FILENAME, 1, 0 );
  } else {
    strcpy( filename, outfilename );
  }

  fp = fopen( filename, "w" );
  if (fp == NULL) {
    ERROR_OUT("unable to open file: %s\n", filename);
    return (false);
  }

  // ask each ipage in the map to write itself out to the file
  for (index = 0; index < m_tablesize; index++) {

    entry = m_table[index];
    while (entry != NULL) {

      // write index of this entry once
      nexttoken  = IPAGEMAP_INDEX_TOKEN;
      numwritten = myfwrite( &nexttoken, sizeof(pa_t), 1, fp );
      if (numwritten != 1) {
        ERROR_OUT("error writing ipagemap: disk full? %d\n", numwritten);
        return (false);
      }
      numwritten = myfwrite( &index, sizeof(uint32), 1, fp );
      if (numwritten != 1) {
        ERROR_OUT("error writing ipagemap: disk full? %d\n", numwritten);
        return (false);
      }
      
      while (entry != NULL) {
        // write the tag of this entry
        numwritten = myfwrite( &entry->m_tag, sizeof(pa_t), 1, fp );
        if (numwritten != 1) {
          ERROR_OUT("error writing ipagemap: disk full? %d\n", numwritten);
          return (false);
        }
        bool rc = entry->m_entry->writePage( fp );
        if (!rc) {
          ERROR_OUT("error writing ipage: disk full? %d\n", numwritten);
          return (false);
        }
        entry = entry->m_next;
      }
      
      // write completion token in to the stream
      nexttoken  = IPAGEMAP_DONE_TOKEN;
      numwritten = myfwrite( &nexttoken, sizeof(pa_t), 1, fp );
      if (numwritten != 1) {
        ERROR_OUT("error writing ipagemap: disk full? %d\n", numwritten);
        return (false);
      }

    } // end if entry is not NULL
  } // end for loop
  fclose( fp );
  return (true);
}

/*------------------------------------------------------------------------*/
/* Accessor(s) / mutator(s)                                               */
/*------------------------------------------------------------------------*/

//**************************************************************************
uint16 ipagemap_t::insertInstr( pa_t address, uint32 instr,
                                static_inst_t * &s_instr )
{
  uint32       index;
  pa_t         tag;
  ipage_t     *ipage;
  imapentry_t *entry;

  // query for this virtual pc
  entry = queryLocation( address, index );
  if (entry == NULL) {
    // not found, create and insert a new instruction page
    tag = address >> IPAGE_PAGE_BITS;
    
    m_num_totalpages++;
    ipage = new ipage_t();
    ipage->setPageAddress( address );
    entry = new imapentry_t( ipage, tag );

    if (m_table[index] != NULL)
      m_num_collisions++;
    entry->m_next = m_table[index];
    m_table[index] = entry;
  }

  uint16 status = entry->m_entry->insertInstruction( address, instr, 
                                                     s_instr );  
  if (status == IPAGE_NEW)
    m_num_instr++;
  return (status);
}

//**************************************************************************
bool  ipagemap_t::queryInstr( pa_t address, static_inst_t * &s_instr )
{
  uint32       index;
  // query for this virtual pc
  imapentry_t *entry = queryLocation( address, index );
  if (entry == NULL) {
    s_instr = NULL;
    return false;
  } else {
    s_instr = entry->m_entry->getInstruction( address );
    if (s_instr == NULL)
      return false;
  }
  
  // returns if found
  return true;
}

//**************************************************************************
void ipagemap_t::invalidateInstr( pa_t address )
{
  // query for this virtual pc
  uint32       index;
  imapentry_t *entry = queryLocation( address, index );
  // only need to invalidate if there is an existing page
  if (entry != NULL) {
    // DEBUG_OUT("invalideInstr: progressing...\n");
    entry->m_entry->invalidateInstruction( address );
  }
}

//**************************************************************************
bool  ipagemap_t::walkMap( void *user_data, ipage_callback_t si_callback )
{
  uint32       index;
  imapentry_t *entry;
  bool         rc;

  // print the contents of each bucket!
  for (index = 0; index < m_tablesize; index++) {
    if (m_table[index]) {
      entry = m_table[index];
      while (entry != NULL) {
        rc = entry->m_entry->walkMap( user_data, si_callback );
        if (rc)
          return (rc);
        entry = entry->m_next;
      }
    }
  }
  return (false);
}

//**************************************************************************
void   ipagemap_t::printStats( void )
{
  DEBUG_OUT("Instruction Page Map Statistics:\n");
  DEBUG_OUT("   Total number of pages     : %d\n", m_num_totalpages );
  DEBUG_OUT("   Total number of collisions: %d\n", m_num_collisions );

#if 0
  uint32       index;
  imapentry_t *entry;
  for (index = 0; index < m_tablesize; index++) {
    if (m_table[index]) {
      entry = m_table[index];
      while (entry != NULL) {
        DEBUG_OUT( "0x%0x %d 0x%0llx 0x%0x\n", index, index, entry->m_tag,
                   hash_function(entry->m_tag << IPAGE_PAGE_BITS) );
        entry = entry->m_next;
      }
    }
  }
#endif
}

//**************************************************************************
void   ipagemap_t::printMemoryStats( out_intf_t *log )
{
  double megabyte_size;
  log->out_info( "Instruction Page Map:\n" );
  log->out_info( "    # static instructions: %d\n", m_num_instr );
  megabyte_size = (double) (sizeof(static_inst_t)*m_num_instr)/(1024.0*1024.0);
  log->out_info( "    Memory size: %.2f MB\n", megabyte_size );

  log->out_info( "    # instruction pages  : %d\n", m_num_totalpages );
  megabyte_size = (double)((sizeof(ipage_t)+sizeof(imapentry_t))*m_num_totalpages)/(1024.0*1024.0);
  log->out_info( "    Memory size: %.2f MB\n", megabyte_size );
}

//**************************************************************************
uint32 ipagemap_t::print( bool verbose )
{
  uint32       index;
  imapentry_t *entry;
  uint32       count = 0;

  // print the contents of each bucket!
  for (index = 0; index < m_tablesize; index++) {
    if (m_table[index]) {
      entry = m_table[index];
      while (entry != NULL) {
        if (verbose) {
          DEBUG_OUT("entry: tag: 0x%0llx\n", entry->m_tag);
        }
        count += entry->m_entry->print( verbose );
        entry = entry->m_next;
      }
    }
  }
  DEBUG_OUT("Total count: %u. Recorded Stats: %u\n", count, m_num_instr);
  return (count);
}

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
