
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

/*
  Thread.h
*/

#include <map>
#include <stdio.h>
#include <thread.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/processor.h>

class Thread {
 public:

  // constructor:
  //     threads call an action function in the context of some object
  Thread( char *name, void *context, void *(*action)(void *) );
  // destructor for Thread
  ~Thread();

  // start the thread running
  int     start(void);
  // cancel the thread from running
  void    cancel(void);

  // sets a barrier: waits until all threads have called barrier before
  // continuing
  void    barrier( void );

  // sets the thread's processor_id affinity (use only before calling run())
  void    bind( processorid_t procid );

  // private method: runs the action function *do NOT call externally*
  void   *run(void);

  static  void printStats(void);

  // pseudo-private member variables
  processorid_t  m_procid;           // processor id
  thread_t       m_tid;              // thread id(needed for cancelling)
  
 private:
  // count of registered threads
  static int                count;
  // static table for registring threads...
  static map<int, Thread *> registry;
  // mutex for modifying the registry
  static mutex_t            thread_mtx;
  static int                thread_count;
  // mutex, count, cv for creating a barrier
  static mutex_t            barrier_mtx;
  static int                barrier_count;
  static cond_t             barrier_cv;

  int            m_id;               // number of registered thread
  char          *m_name;             // name of the thread
  void          *m_context;          // object context to call the action in 
  void        *(*m_action)(void *);  // action method
};

#endif
