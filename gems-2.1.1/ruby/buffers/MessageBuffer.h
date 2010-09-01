
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
 * 
 * Description: Unordered buffer of messages that can be inserted such
 * that they can be dequeued after a given delta time has expired.
 *
 */

#ifndef MESSAGEBUFFER_H
#define MESSAGEBUFFER_H

#include "Global.h"
#include "MessageBufferNode.h"
#include "Consumer.h"
#include "EventQueue.h"
#include "Message.h"
#include "PrioHeap.h"
#include "util.h"

class Chip;

class MessageBuffer {
public:
  // Constructors
  MessageBuffer();
  MessageBuffer(const Chip* chip_ptr); // The chip_ptr is ignored, but could be used for extra debugging

  // Use Default Destructor 
  // ~MessageBuffer()
  
  // Public Methods

  static void printConfig(ostream& out) {}

  // TRUE if head of queue timestamp <= SystemTime
  bool isReady() const { 
    return ((m_prio_heap.size() > 0) && 
            (m_prio_heap.peekMin().m_time <= g_eventQueue_ptr->getTime()));
  }

  bool areNSlotsAvailable(int n);
  int getPriority() { return m_priority_rank; }
  void setPriority(int rank) { m_priority_rank = rank; }
  void setConsumer(Consumer* consumer_ptr) { ASSERT(m_consumer_ptr==NULL); m_consumer_ptr = consumer_ptr; }
  void setDescription(const string& name) { m_name = name; }
  string getDescription() { return m_name;}

  Consumer* getConsumer() { return m_consumer_ptr; }

  const Message* peekAtHeadOfQueue() const;
  const Message* peek() const { return peekAtHeadOfQueue(); }
  const MsgPtr getMsgPtrCopy() const;
  const MsgPtr& peekMsgPtr() const { assert(isReady()); return m_prio_heap.peekMin().m_msgptr; }
  const MsgPtr& peekMsgPtrEvenIfNotReady() const {return m_prio_heap.peekMin().m_msgptr; }

  void enqueue(const MsgPtr& message) { enqueue(message, 1); }
  void enqueue(const MsgPtr& message, Time delta);
  //  void enqueueAbsolute(const MsgPtr& message, Time absolute_time);
  int dequeue_getDelayCycles(MsgPtr& message);  // returns delay cycles of the message
  void dequeue(MsgPtr& message);
  int dequeue_getDelayCycles();  // returns delay cycles of the message
  void dequeue() { pop(); }
  void pop();
  void recycle();
  bool isEmpty() const { return m_prio_heap.size() == 0; }
  
  void setOrdering(bool order) { m_strict_fifo = order; m_ordering_set = true; }
  void setSize(int size) {m_max_size = size;}
  int getSize();
  void setRandomization(bool random_flag) { m_randomization = random_flag; }

  void clear();

  void print(ostream& out) const;
  void printStats(ostream& out);
  void clearStats() { m_not_avail_count = 0; m_msg_counter = 0; }

private:
  // Private Methods  
  int setAndReturnDelayCycles(MsgPtr& message);

  // Private copy constructor and assignment operator
  MessageBuffer(const MessageBuffer& obj);
  MessageBuffer& operator=(const MessageBuffer& obj);

  // Data Members (m_ prefix)
  Consumer* m_consumer_ptr;  // Consumer to signal a wakeup(), can be NULL
  PrioHeap<MessageBufferNode> m_prio_heap;
  string m_name;

  int m_max_size;
  int m_size;

  Time m_time_last_time_size_checked;
  int m_size_last_time_size_checked;

  // variables used so enqueues appear to happen imediately, while pop happen the next cycle
  Time m_time_last_time_enqueue;
  Time m_time_last_time_pop;
  int m_size_at_cycle_start;
  int m_msgs_this_cycle;

  int m_not_avail_count;  // count the # of times I didn't have N slots available
  int m_msg_counter;
  int m_priority_rank;
  bool m_strict_fifo;
  bool m_ordering_set;
  bool m_randomization;
  Time m_last_arrival_time;
};

// Output operator declaration
//template <class TYPE> 
ostream& operator<<(ostream& out, const MessageBuffer& obj);

// ******************* Definitions *******************

// Output operator definition
extern inline 
ostream& operator<<(ostream& out, const MessageBuffer& obj)
{
  obj.print(out);
  out << flush;
  return out;
}

#endif //MESSAGEBUFFER_H
