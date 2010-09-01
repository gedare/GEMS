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
 * FileName:  scheduler.C
 * Synopsis:  Implements a scheduler, chooser, and event queue
 * Author:    zilles
 * Version:   $Id: scheduler.C 1.20 05/08/28 14:00:58-05:00 lyen@s0-29.cs.wisc.edu $
 */

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include "hfa.h"
#include "hfacore.h"
#include "finitecycle.h"
#include "dynamic.h"
#include "scheduler.h"

/*------------------------------------------------------------------------*/
/* Constructor(s) / destructor                                            */
/*------------------------------------------------------------------------*/

//**************************************************************************
scheduler_t::scheduler_t( pseq_t *seq ) : 
  wait_list_t(), m_list_tail(this) { 

  m_pseq = seq;
  m_current_cycle = 0;
  m_queue_head = NULL;
  m_queue_tail = NULL;
  m_cycle_wait_list.setSize(EVENT_QUEUE_SIZE);
}

/*------------------------------------------------------------------------*/
/* Public methods                                                         */
/*------------------------------------------------------------------------*/

//**************************************************************************
void 
scheduler_t::Execute( tick_t localcycle )
{
  /* all functional units are completely pipelined (not a perfect
     assumption) so we have a full set of resources every cycle */
  word_t executed = 0;
  byte_t alu_used[FU_NUM_FU_TYPES];
  memset( (void *)alu_used, 0, sizeof(alu_used) );

  /* fire off as many instructions as resources are available; we have
     to keep a whole bunch of pointers active.  "ready" is the list of
     ready instructions we haven't looked at yet.  Instructions which
     we can't issue for resource reasons */

  if (next != &m_list_tail) {
#ifdef EXPENSIVE_ASSERTIONS
    ASSERT(OrderedList());
#endif
    
    free_list_t ready(next);
    m_list_tail.prev->next = NULL;
    m_list_tail.prev = NULL;
    m_list_tail.InsertWaitQueue(*this);
    word_t num_ready = 0;
    
    for (; ((executed < MAX_EXECUTE) && (!ready.Empty())); executed ++) {
      num_ready ++;

      dynamic_inst_t *d = static_cast<dynamic_inst_t *> (ready.RemoveHead());
      
      /* check to see if we have the resources to execute this instruction */
      fu_type_t fu_type   = d->getStaticInst()->getFuType();
      /* the mapped type allows the user flexability for remapping functional
       * unit resources -- making 16 general purpose functional registers,
       * or having fused multiply-adders */
      int32     fu_mapped = CONFIG_ALU_MAPPING[fu_type];
      if (alu_used[fu_mapped] < CONFIG_NUM_ALUS[fu_mapped]) {
        /* WATTCH power */
        if(WATTCH_POWER){
          //don't increment FU_NONE type
          if(fu_type != FU_NONE){
            m_pseq->getPowerStats()->incrementAluAccess();
          }
          
          if((fu_type == FU_INTALU) || (fu_type == FU_INTMULT) || (fu_type == FU_INTDIV)){
              m_pseq->getPowerStats()->incrementIAluAccess();
          }
          else if((fu_type == FU_FLOATADD) || (fu_type == FU_FLOATCMP) || (fu_type == FU_FLOATCVT) || (fu_type == FU_FLOATMULT) || (fu_type == FU_FLOATDIV) || (fu_type == FU_FLOATSQRT)){
              m_pseq->getPowerStats()->incrementFAluAccess();
          }
        }

        alu_used[fu_mapped]++;
        m_pseq->m_stat_fu_utilization[fu_mapped]++;

        d->beginExecution();
        queueInsert( d, localcycle, CONFIG_ALU_LATENCY[fu_type] );
      } else {
        // track resource stalls
        m_pseq->m_stat_fu_stall[fu_mapped]++;

        /* the required resource is not available - put in queue */
        d->InsertWaitQueueBefore(&m_list_tail);
      }
    }
    
    /* put "ready" instructions which we didn't get to back in the ready
       list - we use the Schedule function because newly readied
       instructions might have been already put on the list. */
    if (!ready.Empty()) { 
      m_list_tail.prev->next = ready.next;
      ready.next->prev = m_list_tail.prev;
      
      /* count the number of ready instructions, and point tail at the
         last node */
      waiter_t *w = ready.next;
      while (1) {
        num_ready ++;
        if (w->next == NULL) {
          m_list_tail.prev = w;
          w->next = &m_list_tail;
          break;
        }
        w = w->next;
      }
#ifdef EXPENSIVE_ASSERTION
      ASSERT(OrderedList());
#endif
    }
  }
  
  /* execute all instructions that complete this cycle; they don't
     count towards using any resources this cycle (but maybe they
     should... i.e. they need a write port on the register file) */
  m_cycle_wait_list.peekCurrent( localcycle ).WakeupChain();
  
  /* execute all long latency events this cycle */
  queueAdvance( localcycle );
  m_current_cycle = localcycle;
}

//**************************************************************************
void 
scheduler_t::queueAdvance( tick_t localcycle )
{
  /* do all of the events which are supposed to be done this cycle -
     they all should be in a single event */
  ASSERT((m_queue_head == NULL) || (localcycle <= m_queue_head->cycle));
  
  if ((m_queue_head != NULL) && (m_queue_head->cycle == localcycle)) {
    event_t *temp = m_queue_head;
    m_queue_head = m_queue_head->next_event;
    if (m_queue_head != NULL) {
      m_queue_head->prev_event = NULL;
    } else {
      m_queue_tail = NULL;
    }
    temp->WakeupChain();
    delete temp;
  }
}

//**************************************************************************
void 
scheduler_t::queueInsert(waiter_t *w, tick_t localcycle, tick_t latency)
{
  // schedule the waiter on the short rotating buffer

  // CM NOTE: if latency "<=" EVENT_QUEUE_SIZE instructions with latencies 
  //          exactly equal to EVENT_QUEUE_SIZE would wake up immediately
  if (latency < EVENT_QUEUE_SIZE) {
    w->InsertWaitQueueTail( m_cycle_wait_list.peekItem(localcycle, latency) );
    return;
  }

  /* compute the cycle that w will be woken up in */
  tick_t c = localcycle + latency;
  
  /* we tend to Insert events into the future so traverse the list
     backwards starting at the end */
  event_t *trav = m_queue_tail;
  while ((trav != NULL) && (trav->cycle >= c)) {
    if (trav->cycle == c) { 
      w->InsertWaitQueue(*trav); 
      return;
    }
    /* the current event should be inserted before the "trav" event */
    trav = trav->prev_event;
  }
  /* do the actual insertion, since we are inserting after trav, so
     trav is our prev */
  event_t *e = new event_t();
  e->cycle = c;
  w->InsertWaitQueue(*e);
  e->prev_event = trav;
  
  if (trav == NULL) { /* if we should be inserted at the head of the list */
    e->next_event = m_queue_head; m_queue_head = e; 
  } else { /* inserted somewhere in the list */
    e->next_event = trav->next_event; trav->next_event = e; 
  }
  
  if (e->next_event == NULL) { /* at the end of the list */
    m_queue_tail = e;
  } else { /* there is someone after me who should point to me */
    e->next_event->prev_event = e;
  }
}
