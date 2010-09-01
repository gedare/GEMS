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
#ifndef _THREAD_H_
#define _THREAD_H_

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
* The thread class encapsulates a single thread: you pass it a "run()"
* function, and tell it when to start.
*
* @author  cmauer
* @version $Id: uthread.h 1.1 02/02/21 16:52:41-00:00 cmauer@cottons.cs.wisc.edu $
*/

class uthread_t {
 public:

  /**
   * @name Constructor(s) / destructor
   */
  //@{

  /** constructor:
   *     threads call an action function in the context of some object */
  uthread_t( const char *name, void *context, void *(*action)(void *) );
  /// destructor for the thread
  ~uthread_t();
  //@}

  /**
   * @name Methods
   */
  //@{
  /// start the thread running
  int32   start(void);
  /// cancel the thread from running
  void    cancel(void);

  /// printing stats after a thread has been freed will crash the program
  static  void printStats(void);
  //@}

  /// used for initializing the static class members
  friend  void  uthread_init_local( void );
  friend  void *uthread_run( void *data );
private:
  /// private method: runs the action function *do NOT call externally*
  void   *run(void);

  /// count of registered threads
  static int                   count;
  /// static table for registring threads...
  static map<int, uthread_t *> *registry;
  /// mutex for modifying the thread registry :)
  static pthread_mutex_t      *reg_mutex;

  /// number of registered thread
  int            m_id;
  /// name of the thread
  char          *m_name;
  /// has this object been started?
  bool           m_isrunning;
  /// object context to call the action in 
  void          *m_context;
  /// action method
  void        *(*m_action)(void *);
  /// thread id(needed for cancelling)
  pthread_t      m_pth_id;
};

/*------------------------------------------------------------------------*/
/* Global variables                                                       */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Global functions                                                       */
/*------------------------------------------------------------------------*/

/// initalize the global vars (thread registry)
void uthread_init_local( void );
/// 
void *uthread_run( void *data );

#endif  /* _THREAD_H_ */
