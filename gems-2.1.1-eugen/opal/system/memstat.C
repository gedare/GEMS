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
 * FileName:  memstat.C
 * Synopsis:  memory statistics tracking object implementation
 * Author:    cmauer
 * Version:   $Id: memstat.C,v 3.5 2003/07/10 18:07:48 milo Exp $
 */

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include "hfa.h"
#include "bitlib.h"
#include "threadstat.h"
#include "memstat.h"

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
// template class map<pa_t, cache_line_stat_t *>;

//**************************************************************************
void cache_line_stat_t::setOwner( int32 thread_id )
{
  if ( m_last_writer == -1 || m_last_writer == thread_id ) {
    // do nothing
  } else if ( m_last_writer != thread_id ) {
    m_ownership_changes++;
  }
  m_last_writer = thread_id;
}

/*------------------------------------------------------------------------*/
/* Constructor(s) / destructor                                            */
/*------------------------------------------------------------------------*/

//**************************************************************************
mem_stat_t::mem_stat_t( )
{
  m_table = new AddressStatTable();
  m_cache_line_mask = MEMOP_BLOCK_MASK;

  m_producer_consumer = (int32 **) malloc( sizeof(int32 *)*MEMSTAT_MAX_THREADS );
  for (int i = 0; i < MEMSTAT_MAX_THREADS; i++) {
    m_producer_consumer[i] = (int32 *) malloc( sizeof(int32)*MEMSTAT_MAX_THREADS );
    for (int k = 0; k < MEMSTAT_MAX_THREADS; k++) {
      m_producer_consumer[i][k] = 0;
    }
  }
}

//**************************************************************************
mem_stat_t::~mem_stat_t( )
{
  if (m_table)
    delete m_table;
}

/*------------------------------------------------------------------------*/
/* Public methods                                                         */
/*------------------------------------------------------------------------*/

//**************************************************************************
cache_line_stat_t *mem_stat_t::getLine( pa_t address, la_t virtual_address )
{
  // look up memory address in the map
  pa_t cacheline = address & ~m_cache_line_mask;
  if ( m_table->find( cacheline ) == m_table->end() ) {
    (*m_table)[cacheline] = new cache_line_stat_t( (virtual_address & ~m_cache_line_mask) );
  }
  return ( (*m_table)[cacheline] );
}

//**************************************************************************
void mem_stat_t::addThreadConsumer( int16 producer_id, int16 thread_id, 
                                    uint16 access_size )
{
  if ( producer_id >= 0 && thread_id >= 0 &&
       producer_id < MEMSTAT_MAX_THREADS && thread_id < MEMSTAT_MAX_THREADS )
    {
      m_producer_consumer[producer_id][thread_id] += access_size;
    }
}

//**************************************************************************
void mem_stat_t::read( int32 thread_id, la_t virtual_address, pa_t address ,
                       uint16 access_size )
{
  cache_line_stat_t *line = getLine( address, virtual_address );

  switch ( line->m_coherence_state ) {
  case STAT_COHERENCE_INVALID:
    line->m_coherence_state = STAT_COHERENCE_SHARED;
    line->m_sharer_mask = 1 << thread_id;
    break;

  case STAT_COHERENCE_SHARED:
    // add us to the list of sharers
    line->m_sharer_mask |= 1 << thread_id;
    break;

  case STAT_COHERENCE_MODIFIED:
    // m_last_writer is the producer of this value
    line->m_coherence_state = STAT_COHERENCE_SHARED;
    line->m_sharer_mask = 1 << thread_id;
    break;

  default:
    ERROR_OUT( "mem_stat_t: unknown coherence state: %d\n",
               line->m_coherence_state );
  }

  addThreadConsumer( line->m_last_writer, thread_id, access_size );
  line->m_reads++;
  line->m_thread_read |= 1 << thread_id;
}

//**************************************************************************
void mem_stat_t::write( int32 thread_id, la_t virtual_address, pa_t address ,
                        uint16 access_size )
{
  cache_line_stat_t *line = getLine( address, virtual_address );
#if 0
  switch ( line->m_coherence_state ) {
  case STAT_COHERENCE_INVALID:
    break;

  case STAT_COHERENCE_SHARED:
    break;

  case STAT_COHERENCE_MODIFIED:
    break;

  default:
    ERROR_OUT( "mem_stat_t: unknown coherence state: %d\n",
               line->m_coherence_state );
  }
#endif
  line->m_sharer_mask = 1 << thread_id;
  line->m_coherence_state = STAT_COHERENCE_MODIFIED;

  uint32 offset = address & m_cache_line_mask;
  for (uint32 i = 0; i < access_size; i++) {
    line->m_bytes_written |= 1ULL << (offset + access_size);
  }
  line->m_writes++;
  line->m_thread_write |= 1 << thread_id;
  line->setOwner( thread_id );
}

//**************************************************************************
void mem_stat_t::atomic( int32 thread_id, la_t virtual_address, pa_t address ,
                         uint16 access_size )
{
  cache_line_stat_t *line = getLine( address, virtual_address );
  line->m_sharer_mask = 1 << thread_id;
  line->m_coherence_state = STAT_COHERENCE_MODIFIED;
  
  line->m_atomics++;
  line->m_thread_atomic |= 1 << thread_id;
  line->setOwner( thread_id );
}

//**************************************************************************
void mem_stat_t::printStats( int32 num_threads )
{
  char buffer_r[1024];
  char buffer_w[1024];
  char buffer_a[1024];

  DEBUG_OUT( "thread producer-consumer table\n");
  for ( int32 i = 0; i < num_threads + 1; i++ ) {
    for (int k = 0; k < num_threads + 1; k++) {
      DEBUG_OUT( "%10d ", m_producer_consumer[i][k] );
    }
    DEBUG_OUT( "\n");
  }

  AddressStatTable::iterator iter;
  for (iter = m_table->begin(); iter != m_table->end(); iter++) {
    cache_line_stat_t *line = iter->second;
    if (line->m_ownership_changes > 0) {
      sprintBits64( line->m_thread_read, buffer_r, num_threads );
      sprintBits64( line->m_thread_write, buffer_w, num_threads );
      sprintBits64( line->m_thread_atomic, buffer_a, num_threads );
      DEBUG_OUT( "0x%0llx\t0x%0llx\t%d\t%d\t%d\t%d READ %s WRITE %s ATOMIC %s\n",
                 iter->first,
                 line->m_virtual_address,
                 line->m_ownership_changes,
                 line->m_reads,
                 line->m_writes,
                 line->m_atomics,
                 buffer_r, buffer_w, buffer_a );
    }
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

