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
#ifndef _UMUTEX_H_
#define _UMUTEX_H_

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include <pthread.h>

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Class declaration(s)                                                   */
/*------------------------------------------------------------------------*/

/**
* Encapsulates a mutex lock.
*
* @see     uthread_t
* @author  [Your name and e-mail address]
* @version $Id: umutex.h 1.1 02/02/21 16:52:40-00:00 cmauer@cottons.cs.wisc.edu $
*/
class umutex_t {
public:
  
  /**
   * @name Constructor(s) / destructor
   */
  //@{
  /**
   * Constructor: creates object
   */
  umutex_t( void );
  umutex_t( const char *name );
  /**
   * Destructor: frees object.
   */
  ~umutex_t( void );
  
  /**
   * @name Methods
   */
  //@{
  int32  init( void );
  int    destroy( void );

  // gives this mutex an name
  void   setName( const char *name );

  /// lock the mutex: same interface as pthread_mutex_lock
  int    lock( void );
  /// unlock the mutex: same interface as pthread_mutex_unlock
  int    unlock( void );

  /// returns the pthread mutex structure
  pthread_mutex_t  *getMutex( void );
  /// returns the status after initialization
  int32             getStatus( void );
  /// prints the status of all the registered mutex locks
  static void       printStats(void);
  //@}

  /// used for initializing the static class members
  friend void umutex_init_local( void );
 private:
  /// count of registered Mutexs
  static int                   count;
  /// static table for registering Mutexs...
  static map<int, umutex_t *>  *registry;
  /// mutex to protect mutex registry
  static pthread_mutex_t      *reg_mutex;

  /// for checking constructor return value
  int32             m_status;
  /// unique identifier for this mutex
  int               m_id;
  /// name of this mutex
  char             *m_name;
  /// pthread mutex structure
  pthread_mutex_t   m_mutex;
};

/*------------------------------------------------------------------------*/
/* Global variables                                                       */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Global functions                                                       */
/*------------------------------------------------------------------------*/

/// initalize the global vars (mutex registry)
void umutex_init_local( void );

#endif  /* _UMUTEX_H_ */
