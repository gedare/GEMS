
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

#include "global.h"
#include "Thread.h"

int                Thread::count = 0;
map<int, Thread *> Thread::registry;
mutex_t            Thread::barrier_mtx;
int                Thread::barrier_count = 0;
cond_t             Thread::barrier_cv;
mutex_t            Thread::thread_mtx;
int                Thread::thread_count = 0;

static void *thread_run( void *data );

Thread::Thread( char *name, void *context, void *(*action)(void *) )
{
  m_name = new char[ strlen(name) + 1 ];
  strcpy( m_name, name );
  m_context = context;
  m_action = action;

  // lock the registry ... add this thread to the registry
  if ( Thread::count == 0 ) {
    if ( mutex_init( &thread_mtx, USYNC_THREAD, NULL ) != 0 ) {
      printf( "Thread init fails: thread_mutex could not be created.");
    }
    if ( mutex_init( &barrier_mtx, USYNC_THREAD, NULL ) != 0 ) {
      printf( "Thread init fails: barrier_mutex could not be created.");
    }
    if ( cond_init( &barrier_cv, USYNC_THREAD, NULL ) != 0 ) {
      printf( "Thread init fails: condition var could not be created.");
    }
    barrier_count = 0;
  }
  if ( mutex_lock( &thread_mtx ) != 0 ) {
    printf( "Thread init fails: thread_mutex not allocated" );
  }
  Thread::thread_count++;
  m_id = Thread::count++;
  Thread::registry[m_id] = this;
  if ( mutex_unlock( &thread_mtx ) != 0 ) {
    printf( "Thread init fails: thread_mutex could not unlock ");
  }
  m_procid = (processorid_t) -1;
}

// destructor for Thread
Thread::~Thread() {
  // free the name of this thread
  if (m_name)
    delete [] m_name;
  if ( mutex_lock( &thread_mtx ) != 0 ) {
    printf( "Thread free fails: thread_mutex not allocated" );
  }
  Thread::thread_count--;
  Thread::registry.erase( m_id );
  if ( mutex_unlock( &thread_mtx ) != 0 ) {
    printf( "Thread free fails: thread_mutex could not unlock ");
  }
}

int Thread::start() {

  //   create a Sun-specific LPW thread
  int thread_status = thr_create( NULL, 0, thread_run, (void *) this,
                                  THR_BOUND, &m_tid );
  if (thread_status != 0) {
    printf("unable to allocate a thread. (errcode=%d)\n", thread_status);
  }
  return (thread_status);
}

void Thread::cancel(void) {
  printf("Solaris threads can not be canceled: ignored.\n");
}

void Thread::barrier( void ) {

  if ( mutex_lock( &barrier_mtx ) != 0 ) {
    printf( "Thread free fails: barrier_mutex not allocated" );
  }
  if (g_verbose)
    printf("Thread blocking %d\n", m_id);
  barrier_count++;
  if ( barrier_count < Thread::thread_count ) {
    // wait for other threads to arrive
    while (barrier_count < Thread::thread_count && barrier_count != 0) {
      if (g_verbose)
        printf("Thread waiting %d\n", m_id);
      cond_wait( &barrier_cv, &barrier_mtx );
    }
  } else {
    barrier_count = 0;
    if (g_verbose)
      printf("Thread broadcasting %d\n", m_id);
    cond_broadcast( &barrier_cv );
  }
  if ( mutex_unlock( &barrier_mtx ) != 0 ) {
    printf( "Thread free fails: barrier_mutex could not unlock ");
  }
  if (g_verbose)
    printf("Thread restarting %d\n", m_id);
}

void Thread::bind( processorid_t procid ) {

  m_procid = procid;

}

void *Thread::run(void) {

  // run the object action function
  return ((*m_action)( m_context ));

}

// prints out information on all of the threads
void Thread::printStats( void ) {
  map<int, Thread *>::iterator iter;
  Thread   *threadp;
  
  cout << endl << "Existing Threads::" << endl;
  if ( Thread::count == 0 )
    return;

  if ( mutex_lock( &thread_mtx ) != 0 ) {
    printf( "Thread init fails: thread_mutex not allocated" );
  }
  for (iter = Thread::registry.begin(); iter != Thread::registry.end();
       iter++) {
    threadp = iter->second;
    cout << "Thread: #" << threadp->m_id << " " << threadp->m_name
         << " " << endl;
  }
  if ( mutex_unlock( &thread_mtx ) != 0 ) {
    printf( "Thread init fails: reg_mutex could not unlock ");
  }
}

// interface for pthreads_create function
static void *thread_run( void *data )
{
  processorid_t  oproc_bind;     // previous processor the thread was bound to
  Thread        *thread = (Thread *) data;
  
  // bind this process to a specific processor (if specified)
  if (thread->m_procid != -1) {
    int status = processor_bind( P_LWPID, P_MYID,
                                 thread->m_procid, &oproc_bind );
    if ( status != 0 ) {
      printf( "warning: processor bind status to proc %d == %d\n", 
              thread->m_procid, status );
    }
    // printf( "old proc %d\n", oproc_bind );
  }

  // calls thread "run()" action in a separate thread
  return (thread->run());
}


