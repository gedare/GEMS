
/*
    Copyright (C) 1999-2008 by Mark D. Hill and David A. Wood for the
    Wisconsin Multifacet Project.  Contact: gems@cs.wisc.edu
    http://www.cs.wisc.edu/gems/

    --------------------------------------------------------------------

    This file is part of the Ruby Multiprocessor Memory System Simulator, 
    a component of the Multifacet GEMS (General Execution-driven 
    Multiprocessor Simulator) software toolset originally developed at 
    the University of Wisconsin-Madison.

    Ruby was originally developed primarily by Milo Martin and Daniel
    Sorin with contributions from Ross Dickson, Carl Mauer, and Manoj
    Plakal.

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
 * $Id$
 */

#include "EventQueue.h"
#include "RubyConfig.h"
#include "Consumer.h"
#include "Profiler.h"
#include "System.h"
#include "PrioHeap.h"
#include "EventQueueNode.h"

// Class public method definitions

EventQueue::EventQueue()
{
  m_prio_heap_ptr = NULL;
  init();
}

EventQueue::~EventQueue()
{
  delete m_prio_heap_ptr;
}

void EventQueue::init()
{
  m_globalTime = 1;
  m_timeOfLastRecovery = 1;
  m_prio_heap_ptr = new PrioHeap<EventQueueNode>;
  m_prio_heap_ptr->init();
}

bool EventQueue::isEmpty() const 
{
  return (m_prio_heap_ptr->size() == 0);
}

void EventQueue::scheduleEventAbsolute(Consumer* consumer, Time timeAbs)
{
  // Check to see if this is a redundant wakeup
  //  Time time = timeDelta + m_globalTime;
  ASSERT(consumer != NULL);
  if (consumer->getLastScheduledWakeup() != timeAbs) {
    // This wakeup is not redundant
    EventQueueNode thisNode;
    thisNode.m_consumer_ptr = consumer;
    assert(timeAbs > m_globalTime);
    thisNode.m_time = timeAbs;
    m_prio_heap_ptr->insert(thisNode);
    consumer->setLastScheduledWakeup(timeAbs);
  }
}

void EventQueue::triggerEvents(Time t) 
{
  EventQueueNode thisNode;

  while(m_prio_heap_ptr->size() > 0 && m_prio_heap_ptr->peekMin().m_time <= t) {
    m_globalTime = m_prio_heap_ptr->peekMin().m_time;
    thisNode = m_prio_heap_ptr->extractMin();
    assert(thisNode.m_consumer_ptr != NULL);
    DEBUG_EXPR(EVENTQUEUE_COMP,MedPrio,*(thisNode.m_consumer_ptr));
    DEBUG_EXPR(EVENTQUEUE_COMP,MedPrio,thisNode.m_time);
    thisNode.m_consumer_ptr->triggerWakeup();
  }
  m_globalTime = t;
}

void EventQueue::triggerAllEvents()
{
  // FIXME - avoid repeated code
  EventQueueNode thisNode;

  while(m_prio_heap_ptr->size() > 0) {
    m_globalTime = m_prio_heap_ptr->peekMin().m_time;
    thisNode = m_prio_heap_ptr->extractMin();
    assert(thisNode.m_consumer_ptr != NULL);
    DEBUG_EXPR(EVENTQUEUE_COMP,MedPrio,*(thisNode.m_consumer_ptr));
    DEBUG_EXPR(EVENTQUEUE_COMP,MedPrio,thisNode.m_time);
    thisNode.m_consumer_ptr->triggerWakeup();
  }
}

// Class private method definitions

void 
EventQueue::print(ostream& out) const
{
  out << "[Event Queue: " << *m_prio_heap_ptr << "]";
}
