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

#ifndef _WRITEBUFFER_H_
#define _WRITEBUFFER_H_

/**
 *  Status for write buffer accesses. The Write buffer can hit in fastpath, be full, or 
 *     successfully enqueue the store request
 */
enum writebuffer_status_t { WB_FULL, WB_OK, WB_MATCHING_MSHR_ENTRY };

class dummy_waiter_t: public waiter_t{
 public:
  //constructor 
  dummy_waiter_t() :waiter_t() { }

  void Wakeup(){
    //Dummy store waiters don't need to do anything!
    return;
  }
};

class writebuffer_t: public waiter_t{
 public:
  ///Constructor
  /// Note that the size of the Write Buffer is determined by the WRITE_BUFFER_SIZE config parameter
  writebuffer_t(uint32 id, uint32 block_bits, scheduler_t * eventQ);

  /// Destructor
  ~writebuffer_t();

  ///Adds a store entry to the write buffer
  writebuffer_status_t addToWriteBuffer(waiter_t * store, la_t logical_address, pa_t physical_address, la_t vpc, bool priv, uint64 posttime, int thread);

  ///Flushes the entire write buffer
  void flushWriteBuffer();

  ///A pseq object calls this when Ruby completes our request
  void complete(pa_t physical_address, bool abortRequest, OpalMemop_t type, int thread);

  ///Used by the pseq object to check which structure  (MSHR or Write Buffer) the completion msg
  ///   should go to
  ///  Checks whether the given PA matches an entry in the WB
  bool checkOutstandingRequests(pa_t physical_address);

  /// This function is used for debugging purposes only...
  void checkOutstandingRequestsDebug(pa_t physical_address);

  /// Used by checkForLoadHit to check and return the write buffer entry, if the load hits to the WB
  bool checkWriteBuffer(pa_t physical_address, ruby_request_t * match);

  /// Used by all load insts to check whether it hits to an entry in the WB. If so, the WB is flushed
  bool checkForLoadHit(waiter_t * waiter, pa_t physical_address);

  /// used to schedule delayed_pool requests to be serviced:
  void scheduleWakeup();

  /// used by the sequencer to try to flush the WB on every cycle
  void issueWrites();

  /// used to wakeup the event queue
  void Wakeup();

  /// prints out the contents of the Write Buffer
  void print();

  /// Returns flag indicating whether we are using the write buffer
  bool useWriteBuffer() { return m_use_write_buffer; }

 private:
  /// id of this write buffer (one per sequencer object)
  uint32        m_id;                       
  /// number of bytes in cacheline
  uint32         m_block_size;
  /// The pointer to the sequencer's event queue for handling delayed store requests:
  scheduler_t * m_event_queue;
  /// A boolean flag indicating whether we have scheduled ourselves in the event queue or not
  bool           m_is_scheduled;
  /// mask to strip off non-cache line bits
  pa_t           m_block_mask;
  /// list of store requests in the write buffer
  pipepool_t * m_request_pool;     
  /// the current length of the write buffer
  uint32          m_buffer_size;      
  /// the count of how many requests are currently being serviced
  uint32          m_outstanding_stores;
  ///whether we want to simulate the write buffer or not:
  bool           m_use_write_buffer;
  /// indicates whether the write buffer is full or not
  bool   m_write_buffer_full;
};

#endif
