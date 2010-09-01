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
 * This is the implementation for a histogram like
 * statistic information tracker.
 *
 * It works just like a simple hash table
 *
 * author: mxu
 * data: 03/29/01
 */

#include "hfa.h"
#include "debugio.h"
#include "histogram.h"

//**************************************************************************
static int bucket_compare( const void *el1, const void *el2 )
{
  const struct hash_ent **bucket1 = (const struct hash_ent **) el1;
  const struct hash_ent **bucket2 = (const struct hash_ent **) el2;
  
  if ( (*bucket1)->x > (*bucket2)->x ) {
    return 1;
  } else if ( (*bucket1)->x == (*bucket2)->x ) {
    return 0;
  }

  // else (*bucket1)->x < (*bucket2)->x )
  return -1;
}

/* create a new histogram with name "name"
   all pointer in the buckets are initiated NULL. */
//**************************************************************************
histogram_t::histogram_t(const char* name, int size)
{
  /* check size */
  if (size < 1 && (size & (size-1)) != 0) {
    fprintf(stderr, "size to the histogram must be a power of two.");
    SIM_HALT;
  }

  m_name = name; /* assume name is static resources */
  m_size = size;
  m_totalcount = 0;
  m_hash_table = (struct hash_ent **) malloc( sizeof(struct hash_ent *) * m_size );
  if (!m_hash_table)
    SIM_HALT;
  for(int i=0; i<m_size; i++) {
    m_hash_table[i] = NULL;
  }
}

/** free a histogram structure */
//**************************************************************************
histogram_t::~histogram_t( void )
{
  struct hash_ent *entry, *next_entry;  
  for (int i = 0; i < m_size; i++) {
    entry = m_hash_table[i];
    while ( entry != NULL ) {
      next_entry = entry->next;
      free( entry );
      entry = next_entry;
    }
  }

  // don't free the name: it is a static resource.
  // free the hash table and set it equal to null
  if (m_hash_table)
    free( m_hash_table );
}

/** walk the histogram table, calling back F() on each entry */
//**************************************************************************
int histogram_t::walk( void (*f)(histogram_t *, int, int) )
{
  struct hash_ent *entry, *next_entry;  
  for (int i = 0; i < m_size; i++) {
    entry = m_hash_table[i];
    while ( entry != NULL ) {
      next_entry = entry->next;
      (*f)( this, entry->x, entry->y );
      entry = next_entry;
    }
  }
  
  return 0;
}

/* increment/decrement value in X by Y */
//**************************************************************************
int histogram_t::touch(histo_x_t X, histo_y_t Y) {
  struct hash_ent *entry, *last_entry;

  /* find the bucket */
  /* bucket = X % h->size; */
  int bucket = X % m_size;
  if (bucket < 0) {
    bucket = 0;
  }
  if (bucket >= m_size) {
    bucket = m_size - 1;
  }

  /* find the hash entry */
  last_entry = entry = m_hash_table[bucket];
  while(entry) {
    if(entry->x == X) break;

    last_entry = entry;
    entry = entry->next;
  };
  
  if (entry) {
    entry->y += Y;
  } else {
    /* create a new entry and append it to the end of the link list. */
    m_totalcount++;
    entry = (struct hash_ent*) malloc(sizeof(struct hash_ent));
    entry->x = X;
    entry->y = Y;
    entry->prev = last_entry;
    entry->next = NULL;
    if (last_entry) {
      last_entry->next = entry;
    } else {
      m_hash_table[bucket] = entry;
    }
  }

  return 0;
}

//**************************************************************************
int histogram_t::printDistribution( out_intf_t *stream, bool is_weighted )
{
  // compute a count of all the points in the file
  // AND
  // sort the hash table entries by their numerical value.
  // allocate an array of size total count...
  struct hash_ent **sorted_array = (struct hash_ent **) malloc( m_totalcount * sizeof(struct hash_ent *) );
  int       index = 0;
  histo_x_t total_x  = 0;
  histo_y_t total_y  = 0;
  int64     total_wt = 0;
  for (int i=0; i<m_size; i++) {
    if (m_hash_table[i]) {
      struct hash_ent* entry = m_hash_table[i];
      while (entry) {
        sorted_array[index] = entry;
        index++;
        total_x  = total_x  + entry->x;
        total_y  = total_y  + entry->y;
        total_wt = total_wt + entry->x*entry->y;
        entry = entry->next;
      }
    }
  }
  ASSERT( index == m_totalcount );
  
  // sort the array containing the data
  qsort( sorted_array, m_totalcount, sizeof(struct hash_ent *), 
         bucket_compare );
  
  // walk the sorted bucket array, printing out cumulative distribution info
  histo_x_t sum_x  = 0;
  histo_x_t sum_y  = 0;
  int64     sum_wt = 0;
  for ( int i=0; i < m_totalcount; i++ ) {
    struct hash_ent *bucket = sorted_array[i];
    sum_x = sum_x  + bucket->x;
    sum_y = sum_y  + bucket->y;
    sum_wt= sum_wt + (bucket->x * bucket->y);
    if (is_weighted) {
      stream->out_info( "%s %d %6.4f %d\n", m_name, bucket->x,
                        ((float) sum_wt / (float) total_wt), bucket->y );
    } else {
      stream->out_info( "%s %d %6.4f %d\n", m_name, bucket->x,
                        ((float) sum_y / (float) total_y), bucket->y );
    }
  }
  free( sorted_array );
  return 0;
}

/* print out the data from the histogram */
//**************************************************************************
int histogram_t::print( out_intf_t *stream )
{
  stream->out_info( "Hash Table entries: %d\n", m_totalcount );
  for (int i=0; i<m_size; i++) {
    if (m_hash_table[i]) {
      struct hash_ent* entry = m_hash_table[i];
      while (entry) {
        /* print X:Y in a line */
        //stream->out_info( "%s\t%d\t%d\n", m_name, entry->x, entry->y );
        stream->out_info( "%s\t0x%x\t%d\n", m_name, entry->x, entry->y );
        entry = entry->next;
      }
    }
  }
  return 0;
}
