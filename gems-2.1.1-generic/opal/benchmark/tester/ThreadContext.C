
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

#include "ThreadContext.h"

ThreadContext::ThreadContext()
{
  m_num_actions = 0;
}

ThreadContext::~ThreadContext()
{
}

void ThreadContext::setThread( Thread *my_th )
{
  m_th = my_th;
}

void ThreadContext::doAction( void )
{
  // pick a random action #
  
  int pick_one = lrand48() % m_num_actions;
  int read_or_write = lrand48() % 2;
  MemAssign *assn = m_mem_actions[pick_one];

  if (read_or_write == 0) {

    // do read
    int cur_value = *(assn->ptr);
    // perform check (if correct to do so...)
    if ( assn->check && cur_value != assn->value ) {
      // check failed
      printf( "assert fails: *(%d) = %d  (should be %d)\n", 
              (int) assn->ptr, *assn->ptr, assn->value );
    }

  } else if (read_or_write == 1) {

    // do write (action/check pair)
    if (assn->check) {
      // perform check
      if ( *(assn->ptr) != assn->value) {
        // check failed
        printf( "assert fails: *(%d) = %d  (should be %d)\n", 
                (int) assn->ptr, *assn->ptr, assn->value );
        
      } else {
        // check is OK
        assn->check = !assn->check;
        // assign invalid value
        *assn->ptr = ~assn->value;
      }
    } else {
      // perform action
      *(assn->ptr) = assn->value;
      assn->check = !assn->check;
    }
  }

}

void ThreadContext::addMemCheck( int *ptr )
{
  MemAssign *assn = new MemAssign();
  int id = m_num_actions++;

  assn->ptr = ptr;
  assn->value = (int) lrand48();
  assn->check = false;  

  m_mem_actions[id] = assn;
}
