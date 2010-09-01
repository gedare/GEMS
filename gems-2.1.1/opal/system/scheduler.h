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
#ifndef _SCHEDULER_H_
#define _SCHEDULER_H_

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include "finitecycle.h"
#include "wait.h"

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

///Separates short latency events ( < EVENT_QUEUE_SIZE ) from long latency evts
const uint32 EVENT_QUEUE_SIZE = 8;

/*------------------------------------------------------------------------*/
/* Class declaration(s)                                                   */
/*------------------------------------------------------------------------*/

/**
* A long latency event scheduled on the event queue.
*
* The event queue schedules long latency events like the completion
* of cache misses. This class is only used within the scheduler to
* implement the nodes of an event queue.
*
* @see     wait_list_t
* @author  zilles
* @version $Id: scheduler.h 1.12 02/07/12 20:21:10-00:00 cmauer@cottons.cs.wisc.edu $
*/
class event_t : public wait_list_t {
  friend class scheduler_t;
public:
  /// constructor
  event_t( void ) : wait_list_t() {
    prev_event = NULL;
    next_event = NULL;
  }

private:
  /// prev event in the queue
  event_t *prev_event;
  /// next event in the queue
  event_t *next_event;
  /// cycle count in the event queue
  tick_t   cycle;
};

/**
* A queue of instructions waiting to be dispatched to functional
* units, or other parts of the processor.
*
* There are two important components of the scheduler: the event queue,
* and a list of instructions waiting to be scheduled. The event queue
* mechanism is described more in the queueInsert() comments. Basically,
* there are two queues, one for short latency events, one for longer
* latency events. The resource limitation of the machine is modeled by
* the "Execute" command, that takes instructions from the list of
* waiting instructions (m_list_tail), and schedules their completion
* based on the instruction latencies. Instructions are added to the
* 'ready to schedule' list by the method "schedule()".
*
* @see     wait_list_t
* @author  zilles
* @version $Id: scheduler.h 1.12 02/07/12 20:21:10-00:00 cmauer@cottons.cs.wisc.edu $
*/

class scheduler_t : public wait_list_t {
public:
  /**
   * @name Constructor(s) / destructor
   */
  //@{
  /**
   * Constructor: creates object
   */
  scheduler_t( pseq_t *seq );
  //@}

  /// schedule a dynamic instruction for a wake-up later
  void schedule(dynamic_inst_t *d) { 
    d->InsertOrderedWaitQueue(this, &m_list_tail); 
  }

  /** Runs the global event queue:
   *  Instructions which have resources available are "completed".
   *
   *  @param localcycle the current local cycle of the cpu
   */
  void Execute( tick_t localcycle );

  /** Insert a waiter into the event queue.
   *      One premature optimization makes the event queue rather confusing:
   *      there are two queues, a short queue (of finite size) and another
   *      queue (a linked list). Events that occur in less than
   *      EVENT_QUEUE_SIZE cycles are scheduled in the short queue
   *      (m_cycle_wait_list), and longer term events are posted to the
   *      linked list.
   *  @param w          The waiter to wake-up later
   *  @param localcycle the current local cycle of the cpu
   *  @param latency    How many cycles in the future to wake 'w' up in
   */
  void queueInsert(waiter_t *w, tick_t localcycle, tick_t latency);

  /** returns the current cycle in the event world */
  tick_t getCycle( void ) {
    return (m_current_cycle);
  }

private:
  /** Advance the event queue one cycle. */
  void queueAdvance( tick_t localcycle );

  /** pointer to the owning sequencer */
  pseq_t       *m_pseq;

  /** current cycle */
  tick_t        m_current_cycle;

  /** sentinel "waiter" marking the end of the wait list */
  tail_waiter_t m_list_tail;

  /// head of long latency event queue
  event_t      *m_queue_head;
  /// tail of long latency event queue
  event_t      *m_queue_tail;
  
  /** pipeline for completion of short latency instructions */
  FiniteCycle<wait_list_t> m_cycle_wait_list;
};

#endif /* _SCHEDULER_H_ */
