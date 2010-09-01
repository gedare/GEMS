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
 * FileName:  listalloc
 * Synopsis:  
 * Author:    cmauer
 * Version:   $Id: listalloc.C 1.5 03/07/10 18:07:45-00:00 milo@cottons.cs.wisc.edu $
 */

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include "hfa.h"
#include "listalloc.h"

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
void *listalloc_t::memalloc( size_t size )
{
  alloc_node_t  *result;

  if ( m_size != size ) {
    ERROR_OUT("error: m_size %d != size %d\n", m_size, size );
    ASSERT( m_size == size );
  }

  m_allocs++;
  if (m_freelist == NULL) {
    // allocate a new pool of blocks, add them to the freelist
    alloc_node_t *block;
    char         *poolstart;
    alloc_node_t *nextnode;

    m_malloc_calls++;
    block = (alloc_node_t *) malloc( m_size * m_poolsize +
                                     sizeof(block_node_t) );
    block->next = m_blocklist;
    m_blocklist = block;
      
    poolstart = ( (char *) block + sizeof(block_node_t) );
    for (int i = 0; i < m_poolsize; i++) {
      nextnode = (alloc_node_t *) (poolstart + i * m_size);
      nextnode->next = m_freelist;
      m_freelist = nextnode;
    }
  }

  result = m_freelist;
  m_freelist = m_freelist->next;
  return ((void *) result);
}

//**************************************************************************
void listalloc_t::memfree( void *obj )
{
  alloc_node_t  *node;

  m_frees++;
  node = (alloc_node_t *) obj;
  node->next = m_freelist;
  m_freelist = node;
}

//**************************************************************************
void listalloc_t::print( out_intf_t *log )
{
  if ( (m_allocs == 0) && 
       (m_frees == 0) && 
       (m_malloc_calls == 0) ) {
    // nothing to report
    return;
  }

  if (m_name != NULL) {
    log->out_info( "%s: allocation stats\n", m_name );
  }
  log->out_info( "      # in use        : %d\n", (m_allocs - m_frees) );
  log->out_info( "      # malloc calls  : %d\n", m_malloc_calls );
  log->out_info( "      # allocations   : %d\n", m_allocs );
  log->out_info( "      # frees         : %d\n", m_frees );
}

/*------------------------------------------------------------------------*/
/* Public methods                                                         */
/*------------------------------------------------------------------------*/

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

