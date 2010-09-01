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
 * FileName:  mshr.C
 * Synopsis:  implements a miss status holding register related objects
 * Author:    zilles, xu -- refactorized hopefully made a better design
 * Version:   $Id: mshr.C 1.37 03/07/10 18:07:48-00:00 milo@cottons.cs.wisc.edu $
 */

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include "hfa.h"
#include "hfacore.h"
#include "cache.h"

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Variable declarations                                                  */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Forward declarations                                                   */
/*------------------------------------------------------------------------*/

// C++ Template: explicit instantiation
template class replacement_manager_t<miss_t>;
template class replacement_manager_t<stream_t>;

/*------------------------------------------------------------------------*/
/* replacement_manager_t methods                                          */
/*------------------------------------------------------------------------*/

//***************************************************************************
template <class Type>
bool replacement_manager_t<Type>::Find(Type *t) {
  // linear search
  Type *tmp = m_head;
  while(tmp) {
    if(tmp == t) return true;
    tmp = tmp->replace_next;
  }

  // not found
  return false;
}

//***************************************************************************
template <class Type>
void replacement_manager_t<Type>::Remove(Type *t) {
  ASSERT(t && t->manager == this);
#ifdef EXPENSIVE_ASSERTIONS
  ASSERT(Find(t));
#endif

  if(m_head == t) m_head = t->replace_next;
  if(m_tail == t) m_tail = t->replace_prev;

  // removes self from current list
  delink(t);

  // remove the reference
  t->manager      = NULL;
  t->replace_prev = NULL;
  t->replace_next = NULL;

  m_size--;
}

//**************************************************************************
template <class Type>
void replacement_manager_t<Type>::SetMRU(Type *t) {
  ASSERT(t);

  if(t->manager && t->manager != this) t->manager->Remove(t);

  if(t->manager) movetoHead(t); // already in this manager
  else insertHead(t);           // a new object
}

//**************************************************************************
template <class Type>
void replacement_manager_t<Type>::SetLRU(Type *t) {
  ASSERT(t);

  if(t->manager && t->manager != this) t->manager->Remove(t);

  if(t->manager) movetoTail(t); // already in this manager
  else insertTail(t);           // a new object
}

//***************************************************************************
template <class Type>
void replacement_manager_t<Type>::insertHead(Type *t) {
  ASSERT(t && t->manager == NULL);
#ifdef EXPENSIVE_ASSERTIONS
  ASSERT(!Find(t));
#endif

  if(m_head) m_head->replace_prev = t;

  t->replace_next = m_head;
  t->replace_prev = NULL;
  t->manager      = this;

  m_head = t;
  if(!m_tail) m_tail = t;

  m_size++;
}

//***************************************************************************
template <class Type>
void replacement_manager_t<Type>::insertTail(Type *t) {
  ASSERT(t && t->manager == NULL);
#ifdef EXPENSIVE_ASSERTIONS
  ASSERT(!Find(t));
#endif

  if(m_tail) m_tail->replace_next = t;

  t->replace_prev = m_tail;
  t->replace_next = NULL;
  t->manager      = this;

  m_tail = t;
  if(!m_head) m_head = t;

  m_size++;
}

//***************************************************************************
template <class Type>
void replacement_manager_t<Type>::movetoHead(Type *t) {
  ASSERT(t && t->manager == this);
#ifdef EXPENSIVE_ASSERTIONS
  ASSERT(Find(t));
#endif

  if(m_head == t) return;

  if(m_tail == t) m_tail = t->replace_prev;

  delink(t);

  m_head->replace_prev = t;
  t->replace_prev      = NULL;
  t->replace_next      = m_head;
  m_head               = t;
}

//***************************************************************************
template <class Type>
void replacement_manager_t<Type>::movetoTail(Type *t) {
  ASSERT(t && t->manager == this);
#ifdef EXPENSIVE_ASSERTIONS
  ASSERT(Find(t));
#endif

  if(m_tail == t) return;

  if(m_head == t) m_head = t->replace_next;

  delink(t);

  m_tail->replace_next = t;
  t->replace_next      = NULL;
  t->replace_prev      = m_tail;
  m_tail               = t;
}

/*------------------------------------------------------------------------*/
/* Miss_t methods                                                         */
/*------------------------------------------------------------------------*/

//***************************************************************************
miss_t::miss_t( void ) {

  m_mshr       = NULL;
  manager      = NULL;
  replace_prev = NULL;
  replace_next = NULL;

}

//***************************************************************************
void miss_t::init(mshr_t *mshr) {
  m_mshr       = mshr;
  init();
}

//***************************************************************************
void miss_t::init(void) {

  m_address    = MISS_NOT_ALLOCATED;
  m_miss_state = PREFETCH_STATE;
  m_descendent = NULL;
  m_stream     = NULL;

  m_mshr->AppendFreeMSHR(this);
};

//***************************************************************************
void 
miss_t::OracleCleanse() {
  /* if this is in any wait lists, remove it */
  if (Waiting()) { RemoveWaitQueue(); }

  /* tell the stream that we used it's pre-fetch */
  if (m_stream) {
         m_stream->PrefetchMoreOnPrefetchHit(this); 
  }

  /* remove this miss from mshr */
  m_mshr->RemoveOutstanding(this);
  if (m_descendent) {
         m_descendent->OracleCleanse();
  }

  /* re-initialize state */
  init();

  /* fire off any load, store, instruction, mshr misses */
  m_wait_list.WakeupChain();

}

//***************************************************************************
void 
miss_t::Wakeup() {

  // only these state can be woken up
  ASSERT((m_miss_state == WAIT_STATE) ||
         (m_miss_state == TRANSFER_STATE));

  if(m_miss_state == WAIT_STATE) { 
    m_mshr->CacheAccessDone(this);
  } else if(m_miss_state == TRANSFER_STATE) { 
    m_mshr->BusTransferDone(this);
  } else {
    SIM_HALT;
  }
}

//***************************************************************************
void 
miss_t::Allocate(pa_t a, cache_t *c, mshr_priority_t p, waiter_t *w) {
  ASSERT(m_address == miss_t::MISS_NOT_ALLOCATED);

  m_address       = a;
  m_miss_state    = QUEUED_STATE;
  m_priority      = p;
  m_dirty         = (p == STORE_DEMAND_PRIORITY) ? true : false;
  m_cache_to_fill = (cache_t *)c;

  m_wait_list.next = NULL;

  if (w) { w->InsertWaitQueue(m_wait_list); }
  m_descendent = NULL;
  
}

//***************************************************************************
void 
miss_t::FillAndSwapOutVictim() {
  ASSERT(m_miss_state != QUEUED_STATE);
  ASSERT(m_cache_to_fill);
  ASSERT(m_stream == NULL);
  ASSERT(m_descendent == NULL);
  ASSERT(m_priority > PREFETCH_PRIORITY);

  m_mshr->RemoveOutstanding(this);
  m_address = m_cache_to_fill->Fill(m_address, m_dirty, &m_dirty,
                                    m_priority == PREFETCH2CACHE_PRIORITY);

  /* add back the victim */
  m_miss_state    = VICTIM_STATE;
  m_priority      = PREFETCH_PRIORITY;
  m_cache_to_fill = NULL;

  if (m_address != MISS_NOT_ALLOCATED) {
    m_mshr->InsertVictim(this);
  } else {
    m_mshr->AppendFreeMSHR(this);
  }

  /** fire off any load, store, instruction, mshr misses
   *  NOTE:
   *  This must be done after mshr's state changes, otherwise
   *  the mshr is left in a inconsistant state during the
   *  next cache access caused by "WakeupChain()"
   */
  m_wait_list.WakeupChain();

}

/*------------------------------------------------------------------------*/
/* Stream Buffers                                                         */
/*------------------------------------------------------------------------*/

//***************************************************************************
stream_t::stream_t( void ) {
  manager      = NULL;
  replace_prev = NULL;
  replace_next = NULL;

  Deallocate();
}

//***************************************************************************
void
stream_t::setMSHR( mshr_t *mshr )
{
  m_mshr = mshr;
  mshr->AppendFreeStream(this);
}

//***************************************************************************
void 
stream_t::Allocate(pa_t prefetch_addr, word_t o, bool pos) {
  if(m_prefetch_addr) Deallocate();

  m_prefetch_addr   = prefetch_addr;
  m_outstanding     = o;
  m_positive_stride = pos;
                
  /* move this stream up to the MRU position */
  manager->SetMRU(this);

  Prefetch(m_outstanding);
}

//***************************************************************************
void 
stream_t::Castout(miss_t *miss) {
  /* decrement number of outstanding misses */
  ASSERT(m_outstanding >= 1);
  m_outstanding--;
                
  /* remove pointer to miss_t which is no longer part of this stream
     because it has been castout and reallocated */
  for (uint32 i = 0 ; i < STREAM_BUFFER_SIZE ; i++ ) { 
         if (m_misses[i] == miss) { 
                m_misses[i]->m_stream = NULL; 
                m_misses[i] = NULL; 
                return; 
         }
  }
  /* miss wasn't found to be part of this stream */
  SIM_HALT;
}

//***************************************************************************
void 
stream_t::PrefetchMoreOnPrefetchHit(miss_t *miss) {
  miss->m_stream = NULL; 

  // exponentially prefetch ahead
  word_t new_outstanding = m_outstanding * 2;

  // make sure only prefetch # = STREAM_BUFFER_SIZE
  if (new_outstanding > STREAM_BUFFER_SIZE) { 
         new_outstanding = STREAM_BUFFER_SIZE;
  }

  /* conserve free MSHR's if they are scarce */
  if (!m_mshr->LoadMSHRsAvailable()) { new_outstanding = m_outstanding; }

  /* decrement number of outstanding misses */
  ASSERT(m_outstanding >= 1);
  m_outstanding--;
                
  /* remove pointer to miss_t which is no longer part of this stream
     because it has been swapped with the block that was evicted
     when it was placed in the cache */
  uint32 i;
  for (i = 0 ; i < STREAM_BUFFER_SIZE ; i++ ) { 
         if (m_misses[i] == miss) { 
                m_misses[i] = NULL;
                break; 
         }
  }
  ASSERT(i < STREAM_BUFFER_SIZE);
                
  /* move this stream up to the MRU position */
  manager->SetMRU(this);

  /* request additional prefetches */
  Prefetch(new_outstanding - m_outstanding);
  m_outstanding = new_outstanding;
}

//***************************************************************************
void 
stream_t::Prefetch(uint32 num_prefetches) {
  uint32 free_miss = 0;
  int prefetch_stride = m_mshr->GetBlockSize();
  if (!m_positive_stride) prefetch_stride = -prefetch_stride;
                
  for (uint32 i = 0 ; i < num_prefetches ; i++ ) {
    // Abort this stream if prefetch_addr hit in cache or outstanding in mshr
    if (m_mshr->IsAddressInCache(m_prefetch_addr)) {
      Deallocate();
      return;
    }
                  
    while (m_misses[free_miss]) { 
      free_miss++;
      ASSERT(free_miss < STREAM_BUFFER_SIZE);
    }
                
    miss_t *m = m_mshr->StreamPrefetch(m_prefetch_addr);
    m->m_stream = this;
    m_misses[free_miss++] = m;
  
    // next prefetch address
    m_prefetch_addr += prefetch_stride;
  }
}

//***************************************************************************
void 
stream_t::Deallocate() {
  m_prefetch_addr   = 0;
  m_outstanding     = 0;
  m_positive_stride = true;

  // remove all misses from this stream
  for (uint32 i = 0 ; i < STREAM_BUFFER_SIZE ; i++ ) { 
         if (m_misses[i]) { 
                m_misses[i]->m_stream = NULL;
                m_misses[i] = NULL; 
         }
  }

  // put myself back to the tail of the replacement manager
  manager->SetLRU(this);
}

/*------------------------------------------------------------------------*/
/* MSHR Methods                                                           */
/*------------------------------------------------------------------------*/

//***************************************************************************
mshr_t::mshr_t(const char *n, generic_cache_template<generic_cache_block_t> *c,
               scheduler_t *eventQueue, word_t bb, word_t lat, word_t bc,
               word_t num_misses, word_t num_streams) {

  ASSERT(strlen(n) <= 31);
  strcpy( m_name, n );

  /* this is the cache which handles this MSHR's misses */
  m_cache_fetch_from = c;
  m_event_queue      = eventQueue;
  m_latency          = lat;
  m_bus_cycles       = bc;
  m_max_misses       = num_misses;

  /* make a bit mask for zeroing out block offset */
  m_block_bits = bb;
  m_block_mask = ((1 << bb) - 1);
#ifdef DEBUG_CACHE
  DEBUG_OUT("%s: max: %d bus: %d blockbits: %d (mask 0x%0llx)\n",
            m_name, m_max_misses, m_bus_cycles, m_block_bits, m_block_mask);
#endif  
  // init hash table
  for (uint32 i = 0 ; i < MSHR_HASH_SIZE ; i++ ) {
    m_miss_hash[i] = NULL;
  }

  // init bus
  m_bus_next_free = m_event_queue->getCycle();

  // init last fetch buffer
  m_last_fetch_index = 0;
  for (uint32 i = 0 ; i < LAST_FETCH_SIZE ; i++) {
         m_last_fetches[i] = 0;
  }

  /* the constructor of these puts them in present miss manager */
  if (m_max_misses) {
    m_misses = new miss_t[m_max_misses];
    // Forte compiler 6.2 requires no parameters for array allocation
    for (uint32 i = 0; i < m_max_misses; i++) {
      m_misses[i].init( this );
    }
  } else {
    m_misses = NULL;
  }

  /* the constructor of these puts them in stream manager */
  if (num_streams) {
    m_streams = new stream_t[num_streams];
    for (uint32 i = 0; i < num_streams; i++) {
      m_streams[i].setMSHR( this );
    }
  } else {
    m_streams = NULL;
  }

  /* initialize stats to zeros */
  m_num_active = m_num_queued = m_primary = m_secondary = 0;

  m_prefetch_prefetch  = 0;
  m_stream_prefetches  = 0;
  m_partial_prefetches = 0;
  m_prefetch_hits      = 0;
  m_victim_hits        = 0;

  // init histogram
  m_hist_active = (uint64 *) malloc( sizeof(uint64) * m_max_misses );
  for ( uint32 i = 0; i < m_max_misses; i++ ) {
    m_hist_active[i] = 0;
  }
}

//**************************************************************************
mshr_t::~mshr_t( void )
{
  if (m_misses)
    delete [] m_misses;
  if (m_streams)
    delete [] m_streams;
}

//**************************************************************************
void
mshr_t::VerifyMSHR() {
  uint32 queued = 0, inflight = 0, present = 0;
  /* count how many entries of the MSHR are in different states */
  for (uint32 i = 0 ; i <  MAX_PRIORITY ; i++ ) {
         uint32 size = 0;
         for (miss_t *m = queued_miss_manager[i].GetMRU() ;
              m != NULL ; m = m->replace_next ) { 
                queued++;
                size++;
         }
         ASSERT(size == queued_miss_manager[i].GetSize());
  }
  ASSERT(queued == m_num_queued);
  for (miss_t *m = inflight_miss_manager.GetMRU() ;
       m != NULL ; m = m->replace_next ) { 
         inflight++;
  }
  ASSERT(inflight == inflight_miss_manager.GetSize());
  ASSERT((inflight + queued) == m_num_active);
  for (miss_t *m = present_miss_manager.GetMRU() ;
       m != NULL ; m = m->replace_next ) { 
         present++;
  }
  ASSERT(present == present_miss_manager.GetSize());
  ASSERT((inflight + queued + present) == m_max_misses);
}

/* This function returns NULL if the request hit an entry that was
   present (either a pre-fetch or a victim) */
//**************************************************************************
miss_t *
mshr_t::Miss(pa_t a, cache_t *c, mshr_priority_t p, waiter_t *w, bool *primary_bool)
{
  miss_t *m;
  pa_t block_address = a & ~((pa_t) m_block_mask);

  ASSERT(p < MAX_PRIORITY); 
#ifdef DEBUG_CACHE
  DEBUG_OUT("%s: 0x%0llx (0x%0llx)\n", m_name, a, block_address);
#endif

  /* check for existing miss, prefetch or victim */
  if ( (m = HashMatch(block_address)) != NULL ) {
    ASSERT((m->m_cache_to_fill == c) || (m->m_cache_to_fill == NULL));
    if ((m->m_miss_state == PREFETCH_STATE) ||
        (m->m_miss_state == VICTIM_STATE)) {

#ifdef DEBUG_CACHE
      DEBUG_OUT("prefetch/victim hit\n");
#endif
      /* got a pre-fetch hit or victim hit */
      ASSERT(m->m_address != miss_t::MISS_NOT_ALLOCATED);
      ASSERT(m->m_descendent == NULL);

      /* This miss register is most recent used */
      present_miss_manager.SetMRU(m);

      /** if REQUEST is a prefetch(prefetch twice, or prefetch a victim),
       *  leave in pre-fetch buffer
       */
      if (p == PREFETCH_PRIORITY) { 
        STAT_INC(m_prefetch_prefetch); 
        return NULL; 
      }

      if (m->m_miss_state == PREFETCH_STATE) {
        STAT_INC(m_prefetch_hits); 
        if (m->m_stream) m->m_stream->PrefetchMoreOnPrefetchHit(m); 
      } else if(m->m_miss_state == VICTIM_STATE) {
        /* otherwise a victim */
        STAT_INC(m_victim_hits); 
      } else {
        SIM_HALT;
      }
      /* upgrade this block to cache */
      ASSERT(m->m_stream == NULL);
      m->m_cache_to_fill = c;
      m->m_priority = p;
      m->FillAndSwapOutVictim();
      /* we got a hit, so return NULL */
      return NULL;

    } else {
#ifdef DEBUG_CACHE
      DEBUG_OUT("secondary miss\n");
#endif
      /* got a secondary miss */
      STAT_INC(m_secondary);
      /* update priority if necessary */
      UpgradePriorityIfNecessary(m, p);
      if (w) {
        w->InsertWaitQueue(m->m_wait_list);
      }
      if (m->m_cache_to_fill == NULL) {
        m->m_cache_to_fill = c; 
      }
      return m;
    }
  } // not in outstanding MSHRs

#ifdef DEBUG_CACHE
  DEBUG_OUT("primary miss\n");
#endif
  /* this is a primary miss, allocate a new miss register */
  STAT_INC(m_primary);
  if (primary_bool) {
    *primary_bool = true;
  }
  m = GetIdleMiss();

  m->Allocate(block_address, c, p, w);
  queued_miss_manager[p].SetMRU(m);
  HashInsert(m);

  /* keep track of the number of outstanding misses */
  m_num_queued++;
  m_num_active++;

  /* check neighboring blocks to potentially start a stream - only on
   * a demand miss, and only if there are at least N miss_t's free.
   */
  if(m_streams && p >= INST_DEMAND_PRIORITY) {
    bool prefetch_pos = true;
         
    if (GratuitousPrefetchMSHRsAvailable() ||
        (LoadMSHRsAvailable() && 
         DetectStride(block_address, &prefetch_pos))) {
      pa_t next_block = (prefetch_pos) ? block_address + GetBlockSize():
                                         block_address - GetBlockSize();

      if (!c->TagCheck(next_block) && (HashMatch(next_block) == NULL)) {
        /* found a free stream for a stream pre-fetch */
        stream_t *stream = stream_manager.GetLRU();
        if (stream) {  /* only NULL if no stream buffer defined in system */
          stream->Allocate(next_block, 1, prefetch_pos);
        }
      } // stream found
    } else {
      /* No prefetch opportunity for this block, so we save it for future
       * stride detection.
       */
      RecentFetch(block_address);
    }
  }

#ifdef DEBUG_CACHE
  DEBUG_OUT("%s: miss returns 0x%0x\n", m_name, (int) m);
#endif
  /* return a pointer to the mshr for this miss */
  return m;
}

//**************************************************************************
void
mshr_t::OracleAccess(pa_t a) {
  pa_t ba = a & ~((pa_t)m_block_mask);
  miss_t *m = HashMatch(ba);
  if (m) {
         m->OracleCleanse();
  } 
  m_cache_fetch_from->OracleAccess(a);
}

//**************************************************************************
miss_t *
mshr_t::StreamPrefetch(pa_t a) {
  ASSERT((a&m_block_mask) == 0);
  ASSERT(HashMatch(a) == NULL);
  
  miss_t *m = GetIdleMiss();
  m->Allocate(a, NULL, PREFETCH_PRIORITY, NULL);
  queued_miss_manager[PREFETCH_PRIORITY].SetMRU(m);
  HashInsert(m);

  /* keep track of the number of outstanding misses */
  m_num_queued++;
  m_num_active++;
  STAT_INC(m_stream_prefetches);
  
  return m;
}

//**************************************************************************
void
mshr_t::RemoveOutstanding(miss_t* m) {
  ASSERT(m->m_address != miss_t::MISS_NOT_ALLOCATED);
  HashRemove(m);
  if (m->m_miss_state <= TRANSFER_STATE) { 
    m_num_active--;
    if (m->m_miss_state == QUEUED_STATE) m_num_queued--;
  }
}

//**************************************************************************
void 
mshr_t::CacheAccessDone(miss_t* m) {
  // this miss must had a descendent
  ASSERT(m->m_descendent);
  // no need to keep the descendent now
  m->m_descendent = NULL;
  // change to transfer state
  ScheduleBusTransfer(m);
}

//**************************************************************************
void
mshr_t::BusTransferDone(miss_t* m) {
  // this miss shouldn't have descendent
  ASSERT(m->m_descendent == NULL);

  if(m->m_priority != PREFETCH_PRIORITY) {
    /* swap this line with a line in the cache */
    m->FillAndSwapOutVictim();
  } else {
    /* keep the line in a pre-fetch buffer */
    present_miss_manager.SetMRU(m);
    m->m_miss_state = PREFETCH_STATE;
    m_num_active--;
  }
}

//**************************************************************************
void 
mshr_t::Tick() {

#ifdef EXPENSIVE_ASSERTIONS
  VerifyMSHR();
#endif

  /* make a histogram of the number of memory insts in the queue, each
   * time a new one is added
   */
  STAT_INC(m_hist_active[m_num_active]);

  /* don't do anything if bus isn't free or nothing to do */
  if ((m_bus_next_free >= (m_event_queue->getCycle() + m_latency)) ||
      (m_num_queued == 0)) {
    if(MSHR_DEBUG && (m_bus_next_free >= (m_event_queue->getCycle() + m_latency)))
      fprintf(stdout, "%s: do nothing due to bus contention\n", m_name);
    else if(MSHR_DEBUG && (m_num_queued == 0))
      fprintf(stdout, "%s: nothing to do\n", m_name);
    return;
  }

  /* don't do anything if MSHR's aren't available one level below */
  if ((m_cache_fetch_from) &&
      (!m_cache_fetch_from->getMSHR()->MSHRsAvailable())) {
    if(MSHR_DEBUG)
      fprintf(stdout, "%s: no mshr available from lower level cache\n", m_name);
    return;
  }

  /* check priority queues (from highest priority to lowest), looking
     for the highest priority miss */
  for (int i = MAX_PRIORITY-1 ; i >= 0 ; i-- ) {
         miss_t *m = queued_miss_manager[i].GetLRU();
         if (m) {
                ASSERT(m->Disconnected());
                /* found a miss, remove it from the queue, access the next level
                   of cache */
                inflight_miss_manager.SetMRU(m);

                if(!m_cache_fetch_from || m_cache_fetch_from->Access(m)) {
                  /* we hit in the cache (or we're at the bottom of the memory
                     hierarchy), schedule some bus bandwidth, add an event to
                     the event queue to perform the fill and wake up any waiters */
                  ScheduleBusTransfer(m);
                } else {
                  /* otherwise the cache function will register a miss with the
                     next level of MSHR.  There is nothing more for us to do
                     here */
                  m->m_miss_state = WAIT_STATE;
                  if(MSHR_DEBUG)
                    fprintf(stdout, "%s: mark %llx wait state\n", m_name, m->m_address);
                }
                m_num_queued--;
                break;
         }
  }
}

//**************************************************************************
miss_t *
mshr_t::GetIdleMiss() {
  // We shouldn't run out of MSHR, since we check availability before issue request
  if ( m_num_active == m_max_misses ) {
    ERROR_OUT( "MSHR: Too few mshrs! %s: max misses: %d num_active: %d\n",
               m_name, m_num_active, m_max_misses );
    SIM_HALT;
  }

  miss_t *ret_val = (miss_t *)present_miss_manager.GetLRU();
  ASSERT(ret_val);
  ASSERT(ret_val->m_descendent == NULL);
  ASSERT(ret_val->m_miss_state >= PREFETCH_STATE);
  if (ret_val->m_address != miss_t::MISS_NOT_ALLOCATED) { 
         HashRemove(ret_val); 
         ret_val->m_address = miss_t::MISS_NOT_ALLOCATED;
  }
  if (ret_val->m_stream) { 
         ret_val->m_stream->Castout(ret_val);
         ret_val->m_stream = NULL;
  }
  return ret_val;
}

//***************************************************************************
bool 
mshr_t::IsAddressInCache(pa_t addr) {
  pa_t ba = addr&(~(pa_t)m_block_mask);
  // XU FIX: should check m_cache_to_fill, not m_cache_fetch_from
  //      The problem is that we are checking the _lower_ level cache
  //      not the upper level cache for the prefetch data.
  //      The current implementation does not have access to this cache *,
  //      so this remains to be fixed.
  return (m_cache_fetch_from->TagCheck(ba) || HashMatch(ba) != NULL);
}

//***************************************************************************
void 
mshr_t::UpgradePriorityIfNecessary(miss_t* m, mshr_priority_t p) {
  if (m->m_priority < p) {
         /* upgrade necessary */
         if (m->m_stream) {
                m->m_stream->PrefetchMoreOnPrefetchHit(m); 
         }

         if ((m->m_priority <= PREFETCH2CACHE_PRIORITY) && (p > PREFETCH2CACHE_PRIORITY)) {
                STAT_INC(m_partial_prefetches);
         }

         m->m_priority = p;
         m->m_dirty = (p == STORE_DEMAND_PRIORITY)? true : false;

         if(m->m_miss_state == QUEUED_STATE) {
                ASSERT(m->m_descendent == NULL);
                queued_miss_manager[p].SetMRU(m);
         } else if (m->m_miss_state == WAIT_STATE) {
                /* transistively upgrade misses to lower levels of the
                   memory hierarchy */
                ASSERT(m->m_descendent);
                m->m_descendent->m_mshr->UpgradePriorityIfNecessary(m->m_descendent, p);
         } else {
                ASSERT(m->m_descendent == NULL);
         } 
  }
}

//**************************************************************************
void
mshr_t::HashVerify(miss_t *m) {
  ASSERT(m);

  // linear search all bucket
  for (uint32 i = 0 ; i < MSHR_HASH_SIZE ; i++) {
    for (miss_t *trav = m_miss_hash[i]; trav != NULL ; trav = trav->hash_next) {
      if (trav == m) { 
        SIM_HALT; /* match found, there shouldn't be one */
      }
    }
  }
  /* all clear */
}

//**************************************************************************
miss_t *
mshr_t::HashMatch(pa_t a) {
  ASSERT((a & m_block_mask) == 0);
  miss_t *trav = m_miss_hash[HashIndex(a)];
  
  while (trav) {
    if ((trav->m_address&(~(pa_t)m_block_mask)) == a) {
      /* found */ return trav;
    }
    trav = trav->hash_next;
  }
  /* not found */
  return NULL;
}

//**************************************************************************
void 
mshr_t::HashInsert(miss_t *m) {
#ifdef EXPENSIVE_ASSERTIONS
  HashVerify(m);
#endif
  uint32 index = HashIndex(m->m_address);
  ASSERT(m_miss_hash[index] != m);

  // insert the this miss register to the head of the list
  m->hash_next = m_miss_hash[index];
  m_miss_hash[index] = m;
}

//**************************************************************************
void 
mshr_t::HashRemove(miss_t *m) {
  uint32 index = HashIndex(m->m_address);
  miss_t *trav = m_miss_hash[index];
  ASSERT(trav);

  /* remove from the linked list */
  if (trav == m) {
    // special case, m is at head
    m_miss_hash[index] = m->hash_next;
  } else {
    // search for m
    while (trav->hash_next != m) {
      trav = trav->hash_next;
      ASSERT(trav);
    }
    ASSERT(trav->hash_next == m);
    // delink m
    trav->hash_next = m->hash_next;
  }

  /* for safety sake, clear the hash_next pointer */
  m->hash_next = NULL;
}

//**************************************************************************
void 
mshr_t::ScheduleBusTransfer(miss_t *m) {
  tick_t current_cycle = m_event_queue->getCycle();
  tick_t earliest      = current_cycle + m_latency;
  
  m->m_miss_state = TRANSFER_STATE;
  if (m_bus_next_free <= earliest) {
    /* the bus is idle - we return requested word first, so latency
       is the time for one transfer - we hold the bus for the full
       transfer. */
    m_bus_next_free = earliest + m_bus_cycles;
  } else {
    /* the bus isn't ready - the latency is the time to wait until
       the bus is idle plus the time for a single transfer - we hold
       the bus for the whole transfer */
    m_bus_next_free += m_bus_cycles;
  }
  
  /* schedule the fill to take place when the bus transaction is
   * complete */
  tick_t latency = (m_bus_next_free - current_cycle);
  m_event_queue->queueInsert(m, current_cycle, latency);

  if(MSHR_DEBUG)
    fprintf(stdout, "%s: schedule bus transfer for %llx @ %lld\n", m_name, m->m_address, m_bus_next_free);
}

//**************************************************************************
bool
mshr_t::DetectStride(pa_t block_address, bool *prefetch_pos) {
  // test forward stride
  pa_t prev_block = block_address - GetBlockSize();
  for (uint32 i = 0 ; i < LAST_FETCH_SIZE ; i++) {
    if (prev_block == m_last_fetches[i]) {
      *prefetch_pos = true;
      return true;
    }
  }

  // test backward stride
  pa_t next_block = block_address + GetBlockSize();
  for (uint32 i = 0 ; i < LAST_FETCH_SIZE ; i++) {
    if (next_block == m_last_fetches[i]) {
      *prefetch_pos = false;
      return true;
    }
  }

  return false;
}

//**************************************************************************
void
mshr_t::RecentFetch(pa_t block_address) {
  m_last_fetches[m_last_fetch_index&LAST_FETCH_MASK] = block_address;
  m_last_fetch_index++;
}

//**************************************************************************
void
mshr_t::printStats( pseq_t *pseq )
{
  uint64 total = 0;
  uint64 weighted_total = 0;
  pseq->out_info("\n");
  pseq->out_info("  %s histogram of misses outanding\n", m_name );
  pseq->out_info("    Name     [out]  number    pct\n");
  for ( uint32 i = 0; i < m_max_misses; i++ ) {
      total += m_hist_active[i];
      weighted_total += m_hist_active[i] * i;
  }
  for ( uint32 i = 0; i < m_max_misses; i++ ) {
    // MSHR occupancy
    if ( (m_hist_active[i] != 0) && (total != 0) ) {
      double pct = 100.0 * (double) m_hist_active[i] / (double) total;
      pseq->out_info("    %-8.8s [%3d] %8llu %5.2f%%\n",
                     m_name, i, m_hist_active[i], pct );
    }
  }
  pseq->out_info("    %s [tot] %8llu\n", m_name, total);
  pseq->out_info("  %s %-50.50s: %8.5f\n", m_name, "Average parallel misses",
                 1.0 * weighted_total / (total - m_hist_active[0]) );
  
  /* register stats */
  pseq->out_info("  %s %-50.50s: %8llu\n", m_name,
                 "total number of primary misses", m_primary );
  pseq->out_info("  %s %-50.50s: %8llu\n", m_name,
                 "total number of secondary misses", m_secondary );
  pseq->out_info("  %s %-50.50s: %8llu\n", m_name,
                 "number of prefetches that hit in prefetch buffer",
                 m_prefetch_prefetch );
  pseq->out_info("  %s %-50.50s: %8llu\n", m_name,
                 "number of stream buffer prefetches launched",
                 m_stream_prefetches );
  pseq->out_info("  %s %-50.50s: %8llu\n", m_name,
                 "number of inflight prefetched upgraded to demands",
                 m_partial_prefetches );
  pseq->out_info("  %s %-50.50s: %8llu\n", m_name,
                 "number of misses serviced from prefetch buffer",
                 m_prefetch_hits );
  pseq->out_info("  %s %-50.50s: %8llu\n", m_name,
                 "number of misses serviced from victim buffer",
                 m_victim_hits );
}

/*------------------------------------------------------------------------*/
/* Accessor(s) / mutator(s)                                               */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Private methods                                                        */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Static methods                                                         */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Global functions                                                       */
/*------------------------------------------------------------------------*/

