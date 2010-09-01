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
#ifndef _URWLOCK_H_
#define _URWLOCK_H_

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Class declaration(s)                                                   */
/*------------------------------------------------------------------------*/

#include <map>
#include <pthread.h>
#include "umutex.h"

/**
* reader / writer lock.
*
* @see     umutex_t
* @author  cmauer
* @version $Id: urwlock.h 1.1 02/02/21 16:52:40-00:00 cmauer@cottons.cs.wisc.edu $
*/

class urwlock_t {
 public:
  urwlock_t();
  ~urwlock_t();

  int32 read_lock();
  int32 read_unlock();

  int32 write_lock();
  int32 write_unlock();


  // this function upgrades a read lock to a write lock. You must
  // already hold a read lock to call this function. When you are
  // finished writing, you should call either write_unlock() to
  // completely release the lock, or downgrade_write_lock() to
  // downgrade it into a read lock

  // WARNING: THIS FUNCTION HAS NOT YET BEEN FULLY TESTED - IT IS NOT
  // CURRENTLY CONSIDERED SAFE TO USE!!!
  int32 upgrade_read_lock();
  
  // this function downgrades a write lock to a read lock. You must
  // already hold a write lock to call this function. When you are
  // finished reading, you should call read_unlock() to free the lock.

  // WARNING: THIS FUNCTION HAS NOT YET BEEN FULLY TESTED - IT IS NOT
  // CURRENTLY CONSIDERED SAFE TO USE!!!
  int32 downgrade_write_lock();

  int32 setName( const char *name );
  int32 getStatus();
  static void printStats(void);

  /// used for initializing the static class members
  friend void urwlock_init_local( void );
 private:
  /// count of registered urwlock_t's
  static int                    count;
  /// static table for registering urwlock_ts...
  static map<int, urwlock_t *> *registry;
  /// mutex to protect urwlock registry
  static pthread_mutex_t       *reg_mutex;

  // for checking constructor return value
  int32             m_status;
  int               m_id;
  char             *m_name;

  /// Number of readers: should be 0 or greater
  int               m_reader_count;
  /// Number of writers: should only be 1 or 0
  int               m_writer_count;
  /// true if a writer is waiting to get in
  bool              m_writer_waiting;
  
  umutex_t          m_mutex;
  pthread_cond_t    m_lock_free_cv;
};

/*------------------------------------------------------------------------*/
/* Global variables                                                       */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Global functions                                                       */
/*------------------------------------------------------------------------*/

/// initalize the global vars (urwlock registry)
void urwlock_init_local( void );

#endif  /* _URWLOCK_H_ */
