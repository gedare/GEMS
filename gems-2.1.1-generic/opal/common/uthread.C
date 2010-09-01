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
 * FileName:  Thread.C
 * Synopsis:  Object encapsulating threads
 * Author:    cmauer (contributions by Antony Sargent, Kiran Chitluri)
 * Version:   $Id: uthread.C 1.2 03/07/10 18:07:45-00:00 milo@cottons.cs.wisc.edu $
 */

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include <pthread.h>
#include "hfa.h"
#include "uthread.h"

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Variable declarations                                                  */
/*------------------------------------------------------------------------*/

/// count of the number of threads allocated
int                       uthread_t::count = 0;
/// map from thread # to the thread pointer
map<int, uthread_t *>    *uthread_t::registry = NULL;
/// mutex for modifying the thread registry :)
pthread_mutex_t          *uthread_t::reg_mutex = NULL;

/*------------------------------------------------------------------------*/
/* Forward declarations                                                   */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Constructor(s) / destructor                                            */
/*------------------------------------------------------------------------*/

//**************************************************************************
uthread_t::uthread_t(const char *name, void *context, void *(*action)(void *))
{
  // to be more general, this should be protected by a mutex:
  //    currently threads which create other threads may thrash the registry
  m_name = (char *) malloc( (strlen(name) + 1) * sizeof(char) );
  strcpy( m_name, name );
  m_context = context;
  m_action = action;
  m_isrunning = false;
  
  if ( pthread_mutex_lock( reg_mutex ) != 0 ) {
    ERROR_OUT( "Thread init fails: reg_mutex not allocated" );
  }
  // register this thread
  m_id = uthread_t::count++;
  (*uthread_t::registry)[m_id] = this;
  if ( pthread_mutex_unlock( reg_mutex ) != 0 ) {
    ERROR_OUT( "Thread init fails: reg_mutex could not unlock ");
  }
}

// destructor for uthread_t
//**************************************************************************
uthread_t::~uthread_t()
{
  // free the name of this thread
  if (m_name)
    free( m_name );
}

/*------------------------------------------------------------------------*/
/* Public methods                                                         */
/*------------------------------------------------------------------------*/

//**************************************************************************
int32 uthread_t::start()
{
  int32   status = 0;
  pthread_attr_t pth_defaults; // pthread defaults

  ASSERT( m_isrunning == false );
  m_isrunning = true;
  bzero(&m_pth_id, sizeof(pthread_t));
  
  // Start the DNP listener thread
  //   get the pthreads attributes
  int thread_status = pthread_attr_init( &pth_defaults );
  if (thread_status != 0) {
    // unable to get status defaults
    ERROR_OUT("unable to allocate thread defaults.");
    status = -1;
  }
  if (status == 0) {
    // For now this thread is scheduled pre-emptively by the system
    pthread_attr_setscope( &pth_defaults, PTHREAD_SCOPE_SYSTEM );

    //   create a new thread
    int thread_status = pthread_create( &m_pth_id, &pth_defaults,
                                        uthread_run, (void *)this );
    if (thread_status != 0) {
      ERROR_OUT( "error: unable to allocate a thread rc = %d.\n",
                 thread_status);
      status = -1;
      exit(1);
    }
  }
  if (status == 0) {
    if ( pthread_attr_destroy( &pth_defaults ) != 0 ) {
      status = -1;
    }
  }

  DEBUG_OUT("start thread::returns %d\n", status);
  return (status);
}

//**************************************************************************
void uthread_t::cancel(void)
{
  // cancel this thread
  pthread_cancel(m_pth_id);
}

//**************************************************************************
void *uthread_t::run(void)
{
  // run the object action function
  DEBUG_OUT("thread run: called %d\n", m_id);
  return ((*m_action)( m_context ));
}

// prints out information on all of the threads
//**************************************************************************
void uthread_t::printStats( void )
{
  map<int, uthread_t *>::iterator iter;
  uthread_t   *threadp;
  
  DEBUG_OUT("\nExisting uthread_ts::\n");
  for (iter = uthread_t::registry->begin();
       iter != uthread_t::registry->end();
       iter++) {
    threadp = iter->second;
    DEBUG_OUT("uthread_t: # %d %s\n", threadp->m_id, threadp->m_name);
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

// interface for pthreads_create function
//**************************************************************************
void *uthread_run( void *data )
{
  uthread_t *thread = (uthread_t *) data;

  // calls thread "run()" action in a separate thread
  return (thread->run());
}

//**************************************************************************
void uthread_init_local( void )
{
  uthread_t::count = 0;
  uthread_t::registry = new map<int, uthread_t *>;
  uthread_t::reg_mutex = new pthread_mutex_t();
  if ( pthread_mutex_init( uthread_t::reg_mutex, NULL ) != 0 ) {
    ERROR_OUT( "Thread init local fails: reg_mutex could not be created.");
  }
}
