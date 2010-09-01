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
#ifndef _LISTALLOC_H_
#define _LISTALLOC_H_

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Class declaration(s)                                                   */
/*------------------------------------------------------------------------*/

typedef struct block_node {
  struct block_node *next;
  int                pad;
} block_node_t;

typedef struct alloc_node {
  struct alloc_node *next;
} alloc_node_t;

/**
* A simple list-based re-use allocator. Overrides the new and delete functions
* to provide faster allocation.
*
* @author  cmauer
* @version $Id: listalloc.h 1.4 03/02/28 20:06:01-00:00 cmauer@cottons.cs.wisc.edu $
*/
class listalloc_t {

public:

  /**
   * @name Constructor(s) / destructor
   */
  //@{
  /**
   * Constructor: creates object
   */
  listalloc_t() {
    m_size = 0;
    m_poolsize = 0;
    m_freelist = NULL;
    m_blocklist = NULL;

    m_name = NULL;
    m_malloc_calls = 0;
    m_allocs = 0;
    m_frees = 0;
  };

  /**
   * initialize the allocator object: must be called before use
   * @param mysize   The size of the object.
   * @param poolsize The number of objects to put in a pool each time.

   */
  void init( const char *name, size_t mysize, int poolsize ) {
    m_name = name;
    m_size = mysize;
    m_poolsize = poolsize;
  };

  /**
   * Destructor: frees object.
   */
  ~listalloc_t() {
    alloc_node_t *block_ptr = m_blocklist;
    alloc_node_t *next_ptr = NULL;
    while (block_ptr != NULL) {
      next_ptr = block_ptr->next;
      free( block_ptr );
      block_ptr = next_ptr;
    }
  };
  //@}

  /**
   * @name Methods
   */
  //@{

  /**
   * Allocate a block of 'size'.
   *
   * @param size The size of the block to allocate. Must be equal to m_size.
   * @return A pointer to a block of 'size'.
   */
  void *memalloc( size_t size );


  /**
   * Allocate a block of 'size'.
   *
   * @param size The size of the block to allocate. Must be equal to m_size.
   * @return A pointer to a block of 'size'.
   */
  void memfree( void *obj );
  //@}
  
  /**
   * Prints out a set of statistics for this allocation pool
   */
  void print( out_intf_t *log );

private:
  /// class member

  /// size of blocks in this free list
  size_t         m_size;
  /// the number of blocks initially placed in the list
  int            m_poolsize;

  /// my free list
  alloc_node_t  *m_freelist;

  /// a linked list of blocks (each with poolsize element in it)
  alloc_node_t  *m_blocklist;
  
  /// The name of this list allocation structure
  const char    *m_name;  
  /// The total number of calls to "malloc()" (depends on block size)
  int32          m_malloc_calls;
  /// The total number of allocates
  int64          m_allocs;
  /// The total number of frees
  int64          m_frees;
};

/*------------------------------------------------------------------------*/
/* Global variables                                                       */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Global functions                                                       */
/*------------------------------------------------------------------------*/

#endif  /* _LISTALLOC_H_ */
