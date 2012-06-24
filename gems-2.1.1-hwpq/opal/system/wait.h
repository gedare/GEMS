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
#ifndef _WAITER_H_
#define _WAITER_H_

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Class declarations                                                     */
/*------------------------------------------------------------------------*/

/**
* A doubly linked-list implementation of a queue of "things" waiting for a 
* contended resource. The head of the queue's previous pointer points to
* itself. The next pointer of the last item points to NULL.
*
* For example, dynamic instructions wait for function units to become
* available. Dynamic instructions also wait for dependancies to become
* resolved by waiting.
*
* @see     dynamic_inst_t, waiter_t, free_list_t
* @author  zilles
* @version $Id: wait.h 1.19 04/08/06 13:48:53-05:00 lyen@opus.cs.wisc.edu $
*/
class wait_list_t {
  friend class waiter_t;
  friend class free_list_t;
  friend class miss_t;
  friend class scheduler_t;

public:
  wait_list_t() { wl_reset(); }

  /** Wakes up all waiters on a chain. */
  void WakeupChain();
  /** Releases all waiters on a chain. */
  void ReleaseChain();
  /** Returns true if the list is empty. */
  bool Empty() const { return (next == NULL); }
  /** Checks all waiters in a list to make sure they in acending priority.
   *  Priority must be greater than zero (0). */
  bool OrderedList();
  /** resets the next pointer to NULL */
  void wl_reset( void ) { next = NULL; }

  /** print out the wait list */
  void print(ostream& out) const;

protected:
  /// pointer to next waiter in the wait list.
  waiter_t *next;
};

/**
* Objects which are waiters may wait in "wait_list_t"s for contended
* resource with priority.
*
* Objects may by inserted into the wait list in order of priority.
*
* @see     dynamic_inst_t, waiter_t, free_list_t
* @author  zilles
* @version $Id: wait.h 1.19 04/08/06 13:48:53-05:00 lyen@opus.cs.wisc.edu $
*/
class waiter_t : public wait_list_t {
  friend class free_list_t;
  friend class wait_list_t;
  friend class scheduler_t;
  friend class rubycache_t;

public:
  /// constructor: assigns default priority of zero.
  waiter_t() : wait_list_t() { prev = NULL; priority = 0; }

  /** Releases this waiter from the wait list. This Virtual Callback
   *  should implement the appropriate behavior for your waiter.
   */
  virtual void Release() {};
  /** Wakes up the waiter, so he/she can take some action. This Virtual 
   *  Callback must implement some appropriate behavoir for your waiter.
   */
  virtual void Wakeup() = 0;

  // make another Wakeup prototype, specifically for use in Waking up the sequencer's logical
  //          processor numbered "proc"
  virtual void Wakeup(uint32 proc) {};

  /// sets the priority of this waiter
  void SetPriority(uint64 _priority) { priority = _priority; }

  /// gets the priority of this waiter
  uint64 getPriority( void ) { return priority; }

  /// inserts a waiter at the head of wait list
  void InsertWaitQueue(wait_list_t &wl) {
         ASSERT(prev == NULL);
         ASSERT(next == NULL);
         prev = &wl;
         next = wl.next;
         if (next != NULL) { next->prev = this; }
         wl.next = this;
  }

  /** inserts a waiter at the end of wait list:
   *      This is a performance degrading instructions but is necessary
   *      to preserve the order of instructions in the scheduler.
   */
  void InsertWaitQueueTail(wait_list_t &wl) {
    ASSERT(prev == NULL);
    ASSERT(next == NULL);
    next = NULL;
    waiter_t *w = wl.next;
    if (w == NULL) {
      prev = &wl;
      wl.next = this;
    } else {
      while (w->next != NULL) {
        w = w->next;
      }
      ASSERT(w->next == NULL);
      w->next = this;
      prev = w;
    }
  }
  
  /// removes a waiter from a wait list
  void RemoveWaitQueue() {
         ASSERT(prev != NULL);
         prev->next = next;
         if (next != NULL) {
                next->prev = prev;
                next = NULL;
         }
         prev = NULL;
  }

  /// returns true if this waiter is "waiting".
  bool Waiting() const { return (prev != NULL); }
  /// returns true if this waiter is completely disconnected
  bool Disconnected() const { return ((prev == NULL) && (next == NULL)); }

  /// inserts a waiter in a wait list, before a specific waiter
  void InsertWaitQueueBefore(waiter_t *w) {
         ASSERT(prev == NULL);
         ASSERT(next == NULL);
         ASSERT(w != NULL);
         ASSERT(w->prev != NULL);
         // CM check bug if w is head of list
         ASSERT(w->prev != w);
         prev = w->prev;
         next = w;
         w->prev = this;
         prev->next = this; 
  }

  /// inserts a waiter in an ordered wait list (by priority)
  void InsertOrderedWaitQueue(wait_list_t *wl, waiter_t *w_last) {
         /* otherwise, start from the back since things tend to be inserted
       in order */
         waiter_t *w = w_last, *w_after;
         ASSERT(w->next == NULL);
         ASSERT(w->Waiting());
         do {
                w_after = w;
                if (w->prev == wl) { break; }
                w = static_cast<waiter_t *> (w->prev);
         } while (w->priority > priority);

         InsertWaitQueueBefore(w_after);
  }

  /// prints out the prev and next pointers
  void print( void ) {
    cout << "prev:" << hex << prev << dec << "next:" << hex << next << dec << endl;
  }
  
  /// print out the waiter
  void print(ostream& out) const {
    out << "P:" << priority;
  }

  /// marks this request as aborted.
  virtual void setRequestAborted(int conflictingProcessor, int conflictingThread) { };

private:
  /// pointer to the previous waiter in the wait list
  wait_list_t *prev;
  /** scheduling priority for this instruction */
  uint64 priority; 
};

/**
* The free list is a "recycling mechanism" for waiters.
* @see     dynamic_inst_t, waiter_t, free_list_t
* @author  zilles
* @version $Id: wait.h 1.19 04/08/06 13:48:53-05:00 lyen@opus.cs.wisc.edu $
*/
class free_list_t : public wait_list_t {

  friend class scheduler_t;

public:
  /// push a waiter on to the free list
  free_list_t(waiter_t *n) : wait_list_t() { 
       next = n; 
       n->prev->next = NULL; 
       n->prev = this; 
  }

  /// free list constructor
  free_list_t() : wait_list_t() { }

  /// remove the front of the free list
  waiter_t *RemoveHead() {
       ASSERT(next != NULL);
       ASSERT(next->prev == this);
       waiter_t *ret_val = next; 

       next = ret_val->next;
       if (next != NULL) { next->prev = this; }

       ret_val->prev = NULL;
       ret_val->next = NULL;
       return ret_val;
  }
};

/**
* The scheduler has a special waiter who tags the end of the wait list.
*
* It should never be woken up. It has the highest possible priority, to
* keep it at the end of the (priority sorted) wait list.
*
* @see     scheduler_t
* @author  zilles
* @version $Id: wait.h 1.19 04/08/06 13:48:53-05:00 lyen@opus.cs.wisc.edu $
*/

class tail_waiter_t : public waiter_t {
  friend class scheduler_t;

public:
  tail_waiter_t(wait_list_t *wl) {
         SetPriority(~0);
         InsertWaitQueue(*wl);
  }

  void Wakeup() { SIM_HALT; }
};

/*------------------------------------------------------------------------*/
/* Global functions                                                       */
/*------------------------------------------------------------------------*/

/// Output operator definition
extern inline ostream& operator<<(ostream& out, const wait_list_t &obj )
{
  obj.print(out);
  out << flush;
  return out;
}

#endif /* _WAITER_H_ */
