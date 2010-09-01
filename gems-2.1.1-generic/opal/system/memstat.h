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
#ifndef _MEMSTAT_H_
#define _MEMSTAT_H_

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Class declaration(s)                                                   */
/*------------------------------------------------------------------------*/

/// maximum number of threads to track stats on
const int32  MEMSTAT_MAX_THREADS = 160;

enum stat_coherence_state_t {
  STAT_COHERENCE_INVALID,
  STAT_COHERENCE_SHARED,
  STAT_COHERENCE_MODIFIED
};

/**
* Tracks statistics on memory addresses accessed.
*
* [Documentation]
* @see     [Related classes, topics, etc.]
* @author  [Your name and e-mail address]
* @version $Id: memstat.h,v 3.5 2003/07/10 18:07:48 milo Exp $
*/
class cache_line_stat_t {
public:
  cache_line_stat_t( la_t virtual_address ) {
    m_virtual_address = virtual_address;
    m_coherence_state = STAT_COHERENCE_INVALID;
    m_sharer_mask     = 0;

    m_last_writer = -1;
    m_bytes_written = 0;

    m_thread_read = 0;
    m_thread_write = 0;
    m_thread_atomic = 0;
    m_ownership_changes = 0;
    m_reads = 0;
    m_writes = 0;
    m_atomics = 0;
  }
  ~cache_line_stat_t();

  /// change the owner of the cache line
  void      setOwner( int32 thread_id );

  /// One virtual address that this cache line was accessed at
  la_t      m_virtual_address;
  /// logical coherence state for this memory address
  stat_coherence_state_t m_coherence_state;

  /// The last writer of this cache line
  int16     m_last_writer;
  /// mask of all readers or writers
  uint64    m_sharer_mask;
  /// a bit map of bytes written in this cache line
  uint64    m_bytes_written;

  /// a bit map of threads that read this cache line
  uint64    m_thread_read;
  /// a bit map of threads that wrote this cache line
  uint64    m_thread_write;
  /// a bit map of threads that atomic'd this cache line
  uint64    m_thread_atomic;
  /// The number of times ownership has changed
  int32     m_ownership_changes;
  /// The read operations that have been performed on this line
  int32     m_reads;
  /// The write operations that have been performed on this line
  int32     m_writes;
  /// The atomic operations that have been performed on this line
  int32     m_atomics;
};

typedef map<pa_t, cache_line_stat_t *>  AddressStatTable;

/**
* Tracks statistics on memory addresses accessed.
*
* @see     cache_line_stat_t
* @author  cmauer
* @version $Id: memstat.h,v 3.5 2003/07/10 18:07:48 milo Exp $
*/
class mem_stat_t {
public:
  /**
   * @name Constructor(s) / destructor
   */
  //@{

  /**
   * Constructor: creates object
   */
  mem_stat_t();

  /**
   * Destructor: frees object.
   */
  ~mem_stat_t();
  //@}

  /**
   * @name Methods
   */
  //@{
  /// read a particular memory address
  void   read( int32 thread_id, la_t virtual_address, pa_t address,
               uint16 access_size );

  /// write a particular memory address
  void   write( int32 thread_id, la_t virtual_address, pa_t address,
                uint16 access_size );

  /// perform an atomic on a particular memory address
  void   atomic( int32 thread_id, la_t virtual_address, pa_t address,
                 uint16 access_size );

  /// print out statistics
  void   printStats( int32 num_threads );
  //@}

  /**
   * @name Accessor(s) / mutator(s)
   */
  //@{
  //@}

private:
  /// Finds a cache line statistics
  cache_line_stat_t *getLine( pa_t address, la_t virtual_address );

  /// call to identify a producer-consumer relation between two threads
  void addThreadConsumer( int16 producer_id, int16 thread_id,
                          uint16 access_size );

  /// Size of cache line tracking in the physical address table
  pa_t               m_cache_line_mask;
  
  /// per physical cache line statistics structure
  AddressStatTable  *m_table;

  /// producer consumer relations between threads
  int32            **m_producer_consumer;
};

/*------------------------------------------------------------------------*/
/* Global variables                                                       */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Global functions                                                       */
/*------------------------------------------------------------------------*/

#endif  /* _MEMSTAT_H_ */
