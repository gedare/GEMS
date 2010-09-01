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
#ifndef _MSHR_H_
#define _MSHR_H_
/*
 * Copyright (c) 1999 by Mark Hill and David Wood for the Wisconsin
 * Multifacet Project.  ALL RIGHTS RESERVED.  
 *
 * ##HEADER##
 *
 * This software is furnished under a license and may be used and
 * copied only in accordance with the terms of such license and the
 * inclusion of the above copyright notice.  This software or any
 * other copies thereof or any derivative works may not be provided or
 * otherwise made available to any other persons.  Title to and
 * ownership of the software is retained by Mark Hill and David Wood.
 * Any use of this software must include the above copyright notice.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS".  THE LICENSOR MAKES NO
 * WARRANTIES ABOUT ITS CORRECTNESS OR PERFORMANCE.
 * */

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include "wait.h"

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

enum mshr_priority_t { DELAYED_PRIORITY,        /* ??? not used?         */
                       PREFETCH_PRIORITY,       /* prefetch to L1 mshrs  */
                       PREFETCH2CACHE_PRIORITY, /* prefetch to cache     */
                       INST_DEMAND_PRIORITY,    /* processor read instr  */
                       DATA_DEMAND_PRIORITY,    /* processor read data   */
                       STORE_DEMAND_PRIORITY,   /* processor store data  */
                       MAX_PRIORITY };

enum miss_state_t { QUEUED_STATE,   /* waiting for bus free or lower level MSHR available */
                    WAIT_STATE,     /* waiting for lower level cache access               */
                    TRANSFER_STATE, /* waiting for bus transfer                           */
                    PREFETCH_STATE, /* finished prefetch                                  */
                    VICTIM_STATE,   /* finished demand miss                               */
                    MAX_STATE };

// candidates for simulation parameters
const word_t MSHR_HASH_BITS = 5;
const word_t LAST_FETCH_SIZE = 16;
const word_t STREAM_BUFFER_SIZE = 4;

// generated from above
const word_t MSHR_HASH_SIZE = 1 << MSHR_HASH_BITS;
const word_t MSHR_HASH_MASK = MSHR_HASH_SIZE - 1;
const word_t LAST_FETCH_MASK = LAST_FETCH_SIZE - 1;

/*------------------------------------------------------------------------*/
/* Class declaration(s)                                                   */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Replacement List                                                       */
/*------------------------------------------------------------------------*/

/**
* Replacement manager - maintains LRU MRU information for any class.
*
* It is a basically double linked list. Any object has prev & next
* pointers can be managed by this class.
*
* @see     mshr_t
* @author  zilles, xu rewrite it into template
* @version $Id: mshr.h 1.25 03/07/10 18:07:48-00:00 milo@cottons.cs.wisc.edu $
*/

template <class Type>
class replacement_manager_t {
public:

  /// constructor
  replacement_manager_t() {
    m_size = 0;
    m_head = m_tail = NULL;
  }

  /// find a object in the manager
  bool   Find(Type *t);

  /// remove this object from the manager (linked list)
  void   Remove(Type *t);

  /// set a object to be the LRU(tail)
  void   SetMRU(Type *t);

  /// set a object to be the MRU(head)
  void   SetLRU(Type *t);

  /// return LRU object
  Type  *GetLRU() { return m_tail; }

  /// return MRU object
  Type  *GetMRU() { return m_head; }

  /// return size
  uint32 GetSize() { return m_size; };

private:
  /// insert object at the head
  void insertHead(Type *t);

  /// insert object at the tail
  void insertTail(Type *t);

  /// Move a object to the head
  void movetoHead(Type *t);

  /// Move a object to the tail
  void movetoTail(Type *t);

  /// delink a object
  void delink(Type *t) {
    if(t->replace_prev) t->replace_prev->replace_next = t->replace_next;
    if(t->replace_next) t->replace_next->replace_prev = t->replace_prev;
  }

  /// head and tail pointer
  Type  *m_head, *m_tail;

  /// number of objects in the manager
  uint32 m_size;
};

/*------------------------------------------------------------------------*/
/* Cache Miss                                                             */
/*------------------------------------------------------------------------*/

/** a cache miss: waits for cache line to be returned
 *                After fill the cache, it contains the victim data
 *                until next miss allocate it.
 *
 * @see     mshr_t
 * @author  zilles, xu -- refactorized
 * @version $Id: mshr.h 1.25 03/07/10 18:07:48-00:00 milo@cottons.cs.wisc.edu $
 */

class miss_t : public waiter_t {
  /// mshr_t can manager this object
  friend class mshr_t;
  /// cache_t & stream_t also need some access
  template <class BlockType>
  friend class generic_cache_template;
  friend class stream_t;

public:
  /// constant
  static const pa_t MISS_NOT_ALLOCATED = ~((pa_t) 0);

  /// constructor & destructor
  miss_t(void);

  /// Clear this outstanding miss anyway
  void OracleCleanse();

  /// Woken up on the waiting list
  void Wakeup();

  /// Allocate (assign outstanding address) this entry
  void Allocate(pa_t a, cache_t *c, mshr_priority_t p, waiter_t *w);

  /// Fill the cache and put victim in this entry
  void FillAndSwapOutVictim();

  /// if this entry is allocated
  bool IsAllocated() { return (m_address == MISS_NOT_ALLOCATED); }

private:
  /// empty this miss structure (sets the mshr field)
  void init(mshr_t *mshr);
  /// empty this miss structure
  void init();

  /// address outstanding from the memory system
  pa_t            m_address;
  miss_state_t    m_miss_state;
  mshr_priority_t m_priority;
  bool            m_dirty;

  cache_t        *m_cache_to_fill;
  mshr_t         *m_mshr;
  wait_list_t     m_wait_list;  /* things to wakeup when line is brought in */
  miss_t         *m_descendent;

  /** The stream this miss might belong to */
  stream_t       *m_stream;

  /** hash bucket chains */
  miss_t         *hash_next;
  
  /* necessary for keeping things in replacement order */
  friend class replacement_manager_t<miss_t>;
  miss_t *replace_prev, *replace_next;
  replacement_manager_t<miss_t> *manager;
};

/*------------------------------------------------------------------------*/
/* Stream Buffer class                                                    */
/*------------------------------------------------------------------------*/

/** stream buffer class. prefetches stride based streams.
 *
 * @see     mshr_t
 * @author  zilles, xu -- refactorized
 * @version $Id: mshr.h 1.25 03/07/10 18:07:48-00:00 milo@cottons.cs.wisc.edu $
 */

class stream_t {
public:
  /// Constructor
  stream_t();

  /// set the mshr this stream is associated with (see note: why isn't this in the constructor)
  void setMSHR( mshr_t *mshr );

  /// Allocate (assign) this stream to a particular stride
  void Allocate(pa_t prefetch_addr, word_t o, bool pos = true);

  /// a miss register is removed (casted out) from the stream
  void Castout(miss_t *miss);

  /// prefetch more down the stream, when previous prefetch hits
  void PrefetchMoreOnPrefetchHit(miss_t *miss);

private:

  /// Prefetch n blocks according to the stride step
  void     Prefetch(uint32 num_prefetches = 1);

  /// deallocate this stream
  void     Deallocate();

  /// current head (next prefetch address) of the prefetching stream
  pa_t     m_prefetch_addr;

  /// current number of outstanding misses
  word_t   m_outstanding;

  /// direction of the stride
  bool     m_positive_stride;

  /// maximum number of outstanding misses this stream buffer can hold
  miss_t  *m_misses[STREAM_BUFFER_SIZE];

  /// the mshr who owns this stream object
  mshr_t  *m_mshr;

  /* necessary for keeping things in replacement order */
  friend class replacement_manager_t<stream_t>;
  stream_t *replace_prev, *replace_next;
  replacement_manager_t<stream_t> *manager;
};

/*------------------------------------------------------------------------*/
/* MSHR class                                                             */
/*------------------------------------------------------------------------*/

/**
* Represents all miss status holding registers (MSHRs) in the machine.
*
* Constains certains miss_t and stream_t objects. Each stream_t is a
* finite size stream prefetch buffer. And each miss register is also
* victim buffer after miss data is satisfied.
*
* @see     miss_t, stream_t, replace_manager_t
* @author  zilles, xu -- refactorized
* @version $Id: mshr.h 1.25 03/07/10 18:07:48-00:00 milo@cottons.cs.wisc.edu $
*/

class mshr_t {
  friend class miss_t;
  friend class stream_t;
public:

  /// debug flag
  static const bool MSHR_DEBUG = false;

  /**
   * @name Constructor(s) / destructor
   */
  //@{

  /// Constructor: creates object
  mshr_t(const char *n, generic_cache_template<generic_cache_block_t> *c,
         scheduler_t *eventQueue, word_t bb, word_t lat, word_t bc,
         word_t num_misses, word_t num_streams);
  /// Destructor: frees object.
  virtual ~mshr_t();
  //@}

  /**
   * @name Methods
   */
  //@{

  /// create a new outstanding miss
  miss_t *Miss(pa_t a, cache_t *c, mshr_priority_t p, waiter_t *w, 
               bool *primary = NULL);

  /// create a new prefetch miss
  miss_t *StreamPrefetch(pa_t a);

  /// tick and object
  virtual void Tick();

  /// print MSHR stats
  virtual void printStats( pseq_t *pseq );
  //@}

protected:

  /// name of this mshr
  char             m_name[32];
  /// mask of block address
  pa_t             m_block_mask;
  /// offset bits in the block
  word_t           m_block_bits;
  /// lower level cache
  generic_cache_template<generic_cache_block_t> *m_cache_fetch_from;
  /// m_cache_fetch_from fetch latency
  word_t           m_latency;
  /// bus transfer latecy
  word_t           m_bus_cycles;
  /// number of MSHRs
  word_t           m_max_misses;
  /// event Queue
  scheduler_t     *m_event_queue;

  /** array of miss registers */
  miss_t          *m_misses;

  /** array of streams */
  stream_t        *m_streams;

  /**
   * @name  Protected Hash Table Functions
   */
  //@{
  /// make sure all mshrs are in proper manager
  void    VerifyMSHR();

  /// upgrade priority of a miss register
  void UpgradePriorityIfNecessary(miss_t* m, mshr_priority_t p);

  /// put a victim miss register in
  void    InsertVictim(miss_t* m) {
    ASSERT(m->m_address != miss_t::MISS_NOT_ALLOCATED);
    HashInsert(m);
    present_miss_manager.SetMRU(m);
  }

  /// remove a outstanding miss
  void    RemoveOutstanding(miss_t* m);

  /// manager miss register after cache access
  void    CacheAccessDone(miss_t* m);

  /// manager miss register after bus transfer
  void    BusTransferDone(miss_t* m);

  /// Access a address without any latency constraint
  void    OracleAccess(pa_t a);

  /// get a free miss register
  miss_t *GetIdleMiss();

  // CM imprecise? how many MSHRs are available?
  bool    LoadMSHRsAvailable() const { 
         return (m_num_active < (m_max_misses - (CONFIG_NUM_ALUS[FU_RDPORT]*
                                                 CONFIG_ALU_LATENCY[FU_RDPORT])));
  }
  bool    GratuitousPrefetchMSHRsAvailable() const { 
         return ((m_num_active < (m_max_misses/2)) && (m_num_queued < (m_max_misses/8))); 
  }
  bool    MSHRsAvailable() const { return (m_num_active < m_max_misses); }
  word_t  GetBlockSize() const { return m_block_mask + 1; }

  void    AppendFreeMSHR(miss_t* m) {
        present_miss_manager.SetLRU(m);
  };

  void    AppendFreeStream(stream_t* s) {
        stream_manager.SetLRU(s);
  };

  bool    IsAddressInCache(pa_t addr);
  //@}

  /**
   * @name Hash table & related functions
   */
  //@{
  /// Hashtable buckets
  miss_t   *m_miss_hash[MSHR_HASH_SIZE];
  /// hash function
  uint32  HashIndex(pa_t a) { return (uint32)(a>>m_block_bits)&MSHR_HASH_MASK; }
  /// make sure a miss register is not in the hash table
  void    HashVerify(miss_t *m);
  /// find a miss register with the block address a, NULL if none
  miss_t *HashMatch(pa_t a);
  /// insert a miss register to the head of the designated bucket
  void    HashInsert(miss_t *m);
  /// remove a miss register from the hash table
  void    HashRemove(miss_t *m);
  //@}

  /// track the bus utilization
  void    ScheduleBusTransfer(miss_t *m);
  /// time that bus is free
  tick_t  m_bus_next_free;

  /// detect a stride pattern and direction
  bool    DetectStride(pa_t block_address, bool *prefetch_pos);

  /// record block_address in the circular buffer to hold last N fetch address
  void    RecentFetch(pa_t block_address);
  /// circular buffer to track last fetches to detect strides
  word_t    m_last_fetch_index;
  pa_t      m_last_fetches[LAST_FETCH_SIZE];

  /** stats */
  uint64   *m_hist_active;
  uint64    m_num_active;
  uint64    m_num_queued;
  uint64    m_primary;
  uint64    m_secondary;
  uint64    m_prefetch_prefetch;
  uint64    m_stream_prefetches;
  uint64    m_prefetch_hits;
  uint64    m_victim_hits;
  uint64    m_partial_prefetches;

  /** track the LRU of entrieS IN THE miss table and streams */
  replacement_manager_t<miss_t>   present_miss_manager;
  replacement_manager_t<miss_t>   inflight_miss_manager;
  replacement_manager_t<miss_t>   queued_miss_manager[MAX_PRIORITY];
  replacement_manager_t<stream_t> stream_manager;
};

/*------------------------------------------------------------------------*/
/* Global variables                                                       */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Global functions                                                       */
/*------------------------------------------------------------------------*/

#endif  /* _MSHR_H_ */

