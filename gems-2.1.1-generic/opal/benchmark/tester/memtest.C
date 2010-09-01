
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

#include <stdlib.h>
#include <stdio.h>
#include <thread.h>
#include <sched.h>
#include <errno.h>

#include <unistd.h>
#include <sys/time.h>
#include <sys/unistd.h>
#include <sys/processor.h>
#include <sys/types.h>
#include <sys/procset.h>

#include "global.h"
#include "FrontEnd.h"
#include "Thread.h"
#include "ThreadContext.h"


// forward declarations
void  *MemTestThread( void *data );

int main( int argc, char *argv[] )
{
  int           status;
  int           thread_num = 0;
  processorid_t proc_id;
  char          thread_name[200];
  void         *thread_return_status;

  struct timeval tp;
  gettimeofday( &tp, NULL );
  g_rand_seed = tp.tv_usec;

  // parse the command line arguments
  FrontEnd fe( argc, argv );


  printf( " array size  = %d\n", g_array_size );
  printf( " num trials  = %d\n", g_num_trials );
  printf( " random seed = %d\n", g_rand_seed );
  printf( " continual   = %d\n", g_continual );
  printf( " verbose     = %d\n", g_verbose );
  srand48( g_rand_seed );

  int num_procs = 15;
  int proc_array[] = { 92, 93, 80, 81, 84, 85, 64, 65, 68, 72, 73, 76, 77, 89, 88 };
  Thread        *threads[num_procs + 1];
  ThreadContext *contexts[num_procs + 1];

  for ( proc_id = 0; proc_id < num_procs; proc_id++) {
    // create a new thread
    if (g_verbose)
      printf("making thread %d\n", thread_num);
    sprintf( thread_name, "thread_%d", proc_array[proc_id] );
    ThreadContext *mycontext = new ThreadContext();
    Thread  *mythread = new Thread( thread_name, mycontext,
                                    &MemTestThread );
    mycontext->setThread( mythread );
    threads[thread_num] = mythread;
    contexts[thread_num] = mycontext;
    thread_num++;
    
    // bind each thread to a different processor
    mythread->bind( proc_array[proc_id] );
  }
  
#if 0
  // get the number of processors, allocate an array of threads
  int            num_procs = sysconf(_SC_NPROCESSORS_ONLN);
  Thread        *threads[num_procs + 1];
  ThreadContext *contexts[num_procs + 1];
  
  for (proc_id = 0; num_procs > 0; proc_id++) {
    status = p_online(proc_id, P_STATUS);
    if (status == -1 && errno == EINVAL)
      continue;
    
    // create a new thread
    if (g_verbose)
      printf("making thread %d\n", thread_num);
    sprintf( thread_name, "thread_%d", proc_id );
    ThreadContext *mycontext = new ThreadContext();
    Thread  *mythread = new Thread( thread_name, mycontext,
                                    &MemTestThread );
    mycontext->setThread( mythread );
    threads[thread_num] = mythread;
    contexts[thread_num] = mycontext;
    thread_num++;
    
    // bind each thread to a different processor
    mythread->bind( proc_id );
    
    // start the thread running
    //    mythread->start();
    
    num_procs--;
  }
#endif
  
  // initialize memory system ( decide which thread will read what mem locs )

  // allocate and distribute addresses in a block of memory
  int *test_block = (int *) malloc( g_array_size * sizeof(int) );
  
  // walk a small block of memory, allocating each block to a different thread
  for ( int i = 0; i < g_array_size; i++ ) {
    
    int proc_id = lrand48() % thread_num;
    
    // assign ith memory reference to thread #proc_id
    contexts[proc_id]->addMemCheck( &test_block[i] );
    
  }

  printf( "running: " );
  for ( int i = 0; i < thread_num; i++ ) {

    // start the thread running
    threads[i]->start();
    
  }

  // wait for all thread to join (terminate)...
  thr_join( 0, NULL, &thread_return_status );
  if (g_verbose)
    printf( "return code: %d\n", (int) thread_return_status );
  return (0);
}

void  *MemTestThread( void *data )
{
  int            firstTest = true;
  ThreadContext *tc = (ThreadContext *) data;
  Thread *mythread = tc->m_th;

  while (g_continual || firstTest) {

    if (!firstTest) {
      printf(".");
      fflush(stdout);
    }
    // read and write to any number of memory addresses
    for (int i = 0; i < g_num_trials; i++) {
      tc->doAction();
    }
    
    // call barrier
    mythread->barrier();
    firstTest = false;
  }

  return (NULL);
}
