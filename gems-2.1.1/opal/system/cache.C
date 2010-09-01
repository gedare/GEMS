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
 * FileName:  cache.C
 * Synopsis:  
 * Author:    zilles
 * Version:   $Id: cache.C 1.47 05/08/14 17:07:09-05:00 lyen@opus.cs.wisc.edu $
 */

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include "hfa.h"
// need scheduler for timing reasons --> needs dynamic inst ...
#include "hfacore.h"
#include "bitlib.h"
#include "confio.h"
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
template class generic_cache_template<generic_cache_block_t>;

/*------------------------------------------------------------------------*/
/* Generic Cache                                                          */
/*------------------------------------------------------------------------*/

//**************************************************************************
cache_t::cache_t(const char *n, mshr_t *m, scheduler_t *eventQueue) {
  name = n;
  mshr = m;
  m_eventQueue = eventQueue;

  /* initialize stats to zeros */
  reads = 0;
  read_hit = 0;
  read_miss = 0;
  writes = 0;
  write_hit = 0;
  write_miss = 0;
  write_prefetches = 0;
  replacements = 0;
  writebacks = 0;
}

//**************************************************************************
void cache_t::printStats( pseq_t *pseq )
{
  pseq->out_info("\n" );
  pseq->out_info("  %s %-50.50s: %8llu\n", name,
                 "total number of reads:", reads );
  pseq->out_info("  %s %-50.50s: %8llu\n", name,
                 "total number of read hits:", read_hit );
  pseq->out_info("  %s %-50.50s: %8llu\n", name,
                 "total number of read misses:", read_miss );
  
  pseq->out_info("  %s %-50.50s: %8llu\n", name,
                 "total number of writes:", writes );
  pseq->out_info("  %s %-50.50s: %8llu\n", name,
                 "total number of write hits:", write_hit );
  pseq->out_info("  %s %-50.50s: %8llu\n", name,
                 "total number of write misses:", write_miss );
  
  pseq->out_info("  %s %-50.50s: %8llu\n", name,
                 "number of write prefetches:", write_prefetches);
  pseq->out_info("  %s %-50.50s: %8llu\n", name,
                 "total number of replacements:", replacements );
  pseq->out_info("  %s %-50.50s: %8llu\n", name,
                 "total number of writebacks:", writebacks );

  for(uint k=0; k <  CONFIG_LOGICAL_PER_PHY_PROC; ++k){
    pseq->out_info("  %s %-50.50s: %8.2f\n", name,
                 "demand miss per 1000 retired instructions [logical proc %d]:",
                 1000.0 * (read_miss+write_miss) / pseq->m_stat_committed[k],k );
  }
}

/*------------------------------------------------------------------------*/
/* LN cache                                                               */
/*------------------------------------------------------------------------*/

//**************************************************************************
#if 0
template <class BlockType>
generic_cache_template<BlockType>::generic_cache_template(const char *n, mshr_t *m,
    scheduler_t *eventQueue, uint32 _set_bits, uint32 assoc,
    uint32 _block_bits, bool ideal) :
  cache_t (n, m, eventQueue) {

  n_sets = (1 << _set_bits);
  set_bits = _set_bits;
  set_mask = n_sets - 1;
  block_size = (1 << _block_bits);
  block_bits = _block_bits;
  block_mask = block_size - 1;
  m_assoc = assoc;

  DEBUG_OUT("Ln constructor: block size: %dB, %dx assoc, %d sets.  %dB total\n",
            block_size, m_assoc, n_sets,
            block_size * m_assoc * n_sets );
#ifdef DEBUG_CACHE
  DEBUG_OUT("memop linesize %d 0x%0llx\n", 1 << DL1_BLOCK_BITS, MEMOP_BLOCK_MASK);
  DEBUG_OUT("%s: set_bits: %d (mask 0x%0x) block_bits: %d (mask 0x%0x)\n",
         name, set_bits, set_mask, block_bits, block_mask );
#endif

  /* allocate cache blocks and initialize all entries to zeroes */
  cache = (BlockType *) malloc( sizeof(BlockType) * n_sets * m_assoc );
  /* tag == 0, last_access == 0, prefetched == false, user_data == NULL */
  bzero((void *)cache, sizeof(BlockType) * n_sets * m_assoc);

  m_ideal = ideal;
}
#endif

//**************************************************************************
template <class BlockType>
generic_cache_template<BlockType>::~generic_cache_template()
{
  if (cache) {
    free(cache);
    cache = NULL;
  }
}

//**************************************************************************
template <class BlockType>
bool generic_cache_template<BlockType>::TagSearch(pa_t a, bool setMRU, bool setDirty,
    BlockType** block_ptr, uint32 *way) {

  uint32 index = Set(a);
  pa_t   ba = BlockAddress(a);
  bool   cachehit = false;
  ASSERT(index < n_sets);

  /* search all sets until we find a match */
  BlockType *set = &cache[index * m_assoc];
  for (uint32 i = 0 ; i < m_assoc ; i ++) {
    // if it is found in the cache ...
    if ( (IsValid(set[i]) && getBlockAddress(set[i]) == ba) ) {
      if(setMRU)
        set[i].last_access = m_eventQueue->getCycle();
      if(setDirty)
        set[i].address_state |= CACHE_BLK_DIRTY;
      cachehit = true;
      if(block_ptr) *block_ptr = &set[i];
      if(way) *way = i;
      break;
    }
  }
  
#ifdef DEBUG_CACHE
  DEBUG_OUT( "%s TagSearch 0x%0llx (TagSearch: index: 0x%0x) ... %d\n",
             name, a, index, cachehit);
#endif
  return cachehit;
}

//**************************************************************************
template <class BlockType>
void generic_cache_template<BlockType>::DoBeforeReplace(BlockType *b, uint32 way) {
  /* update stats */
  if(IsValid(*b)) {
    STAT_INC(replacements);
    if (IsDirty(*b)) STAT_INC(writebacks);
  }
}

//**************************************************************************
template <class BlockType>
void generic_cache_template<BlockType>::DoAfterFill(BlockType *b, uint32 way) {
}

//**************************************************************************
template <class BlockType>
void generic_cache_template<BlockType>::DoWhenAccessHit(BlockType *b, bool read, bool data) {
  ASSERT(IsValid(*b));

  if(read) {
    STAT_INC(reads);
    STAT_INC(read_hit);
  } else {
    STAT_INC(writes);
    STAT_INC(write_hit);
  }
}

//**************************************************************************
template <class BlockType>
void generic_cache_template<BlockType>::DoWhenAccessMiss(my_addr_t a, bool read, bool data) {
  if(read) {
    STAT_INC(reads);
    STAT_INC(read_miss);
  } else {
    STAT_INC(writes);
    STAT_INC(write_miss);
  }
}

//**************************************************************************
#if 0
template <class BlockType>
bool generic_cache_template<BlockType>::Access(miss_t *m) {
  pa_t a    = m->m_address;
  pa_t ba   = BlockAddress(a);

#ifdef DEBUG_CACHE
  DEBUG_OUT("%s: access: 0x%0llx  ... block 0x%0llx ... ",
            name, a, ba);
#endif

  /* always hit if ideal */
  if(m_ideal) return true;

  BlockType *b = NULL;
  bool hit = TagSearch( a, /* setMRU */ true, /* setDirty */ m->m_dirty,
                        /* block_ptr */ &b, /* way_ptr */ NULL );
  if (hit) {
    DoWhenAccessHit(b, !m->m_dirty, /* always data for Access() */true);
    return true;
  }

  /* didn't find a match - request a fill from next level */
  ASSERT (m->m_descendent == NULL);
  mshr_priority_t p = m->m_priority;
  /* for lower level cache, we should upgrade prefetch priority */
  if (p == PREFETCH_PRIORITY) p = PREFETCH2CACHE_PRIORITY;
#ifdef DEBUG_CACHE
  DEBUG_OUT(" MISSED!\n");
#endif
  m->m_descendent = mshr->Miss(ba, static_cast<cache_t *>(this), p, m);

  DoWhenAccessMiss(a, !m->m_dirty, /* always "data" for Access() */true);

  return (m->m_descendent == NULL);
}
#endif

//**************************************************************************
template <class BlockType>
uint32 generic_cache_template<BlockType>::GetLRU(BlockType* set) {

  // search all blocks in this set to find InValid or LRU
  uint32 lru       = 0;
  tick_t lru_cycle = set[lru].last_access;
  for (uint32 i = 0 ; i < m_assoc ; i++) {
    if (IsValid(set[i])) {
      if(set[i].last_access < lru_cycle) {
        lru_cycle = set[i].last_access;
        lru = i;
      }
    } else {
      lru = i;
      break;
    }
  }
  return lru;
}

//**************************************************************************
#if 0
template <class BlockType>
pa_t generic_cache_template<BlockType>::Fill(pa_t a, bool dirty, bool *writeback, bool prefetched) {
  uint32 index = Set(a);
  ASSERT(index < n_sets);

  ASSERT(!TagSearch(a, false, false, NULL, NULL));

#ifdef DEBUG_CACHE
  DEBUG_OUT("%s: fill: 0x%0llx  ... dirty %d\n", name, a, dirty);
#endif
  
  BlockType *set = &cache[index * m_assoc];
  uint32 lru = GetLRU(set);

  // trigger some actions before replace
  DoBeforeReplace(&set[lru], /* way */ lru);

  /* gather information about the block we're replacing */
  pa_t victim_addr = miss_t::MISS_NOT_ALLOCATED;
  if(IsValid(set[lru])) {
    victim_addr = getBlockAddress(set[lru]);
    if(IsDirty(set[lru])) *writeback = true;
  } 
 
  /* write new block into the cache */
  set[lru].address_state = BlockAddress(a) | CACHE_BLK_VALID |
                           ((dirty)? CACHE_BLK_DIRTY:0);
  set[lru].last_access   = m_eventQueue->getCycle();
  set[lru].prefetched    = prefetched;

  // trigger some actions after fill
  DoAfterFill(&set[lru], lru);

  return victim_addr;
}

#endif

//**************************************************************************
template <class BlockType>
void generic_cache_template<BlockType>::Warmup(pa_t a) {

  uint32 index = Set(a);
  pa_t ba = BlockAddress(a);
  ASSERT(index < n_sets);

  /* search all sets until we find a match */
  BlockType *set = &cache[index * m_assoc];
  for (uint32 i = 0 ; i < m_assoc ; i ++) {
    bool hit = IsValid(set[i]) && (getBlockAddress(set[i]) == ba);
    if (hit) { 
      return;
    }
  }
  int replace_set = random() % m_assoc;
  /* write new block into the cache */
  set[replace_set].address_state = ba | CACHE_BLK_VALID;
}

//**************************************************************************
template <class BlockType>
void generic_cache_template<BlockType>::OracleAccess(pa_t a) {
  /* used when we want the execution of a particular load or store to
   * be prefetched perfectly.
   */
  uint32 index = Set(a);
  pa_t ba = BlockAddress(a);
  ASSERT(index < n_sets);
  STAT_INC(reads);

  /* search all sets until we find a match */
  BlockType *set = &cache[index * m_assoc];
  int replace_set = 0;
  for (uint32 i = 0 ; i < m_assoc ; i ++) {
    bool hit = IsValid(set[i]) && (getBlockAddress(set[i]) == ba);
    if (hit) { 
      STAT_INC(read_hit);
      return;
    } 
    if (set[i].last_access < set[replace_set].last_access) {
      replace_set = i;
    }
  }

  /* write new block into the cache */
  STAT_INC(read_miss);
  set[replace_set].address_state = ba | CACHE_BLK_VALID;
  set[replace_set].last_access = m_eventQueue->getCycle();
}

//**************************************************************************
/* this function is called when a store is retired.  If the address
   hits in the cache or the pre-fetch buffer, the operation is
   complete and true is returned.  Otherwise a miss is launched (or
   upgraded to demand from prefetch) and false is returned */
#if 0
template <class BlockType>
bool generic_cache_template<BlockType>::Write(pa_t a, waiter_t *w) {

  /* always hit if ideal */
  if(m_ideal) return true;

  BlockType *b;
  bool hit = TagSearch( a, true, true, &b, NULL );
  if (hit) {

    DoWhenAccessHit(b, /* read */ false, /* data */ true);

  } else {

    DoWhenAccessMiss(a, /* read */ false, /* data */ true);

    /* check the pre-fetch/victim buffer to make sure we missed */
    ASSERT(w);
    hit = (NULL == mshr->Miss(a, (cache_t *)this, STORE_DEMAND_PRIORITY, w));
  }

  return hit;
}
#endif

//**************************************************************************
/* This function is called for loads and instruction fetches
   ("data_request" discriminates between data and instruction
   requests).  It checks the cache and pre-fetch/victim buffers.
   On a miss it allocates an entry in the mshr's. */
#if 0
template <class BlockType>
bool generic_cache_template<BlockType>::Read( pa_t a, waiter_t *w, bool data_request, bool *primary_bool)
{

  /* always hit if ideal */
  if(m_ideal) return true;

  BlockType *b;
  bool hit = TagSearch( a, true, false, &b, NULL );
  if (hit) {

    DoWhenAccessHit(b, /* read */ true, /* data */ data_request);

  } else {

    DoWhenAccessMiss(a, /* read */ true, /* data */ data_request);

    /* check the pre-fetch/victim buffer to make sure we missed.
     * mshr->Miss() returns NULL if we got a hit in the
     * pre-fetch/victim buffer.
     */
    mshr_priority_t priority = (data_request) ? 
      DATA_DEMAND_PRIORITY : INST_DEMAND_PRIORITY;
    ASSERT(w);
    hit = (NULL == mshr->Miss(a, (cache_t *)this, priority, w, primary_bool));
  }

  return hit;
}
#endif

//**************************************************************************
#if 0
template <class BlockType>
void generic_cache_template<BlockType>::Prefetch(pa_t a) {

  ASSERT(TagCheck(a) == false);
  STAT_INC(write_prefetches); 
  mshr->Miss(a, (cache_t *)this, PREFETCH_PRIORITY, NULL);

}
#endif

//**************************************************************************
template <class BlockType>
attr_value_t generic_cache_template<BlockType>::get_cache_data( void *ptr, void *obj ) {
  generic_cache_template<BlockType> *cache_obj  = static_cast<generic_cache_template<BlockType> *>(ptr);
  BlockType                    *cache_sets = cache_obj->cache;
  uint32                        n_sets = cache_obj->n_sets;
  uint32                        assoc  = cache_obj->m_assoc;
  attr_value_t                  ret;
  attr_value_t                 *vec;
  attr_value_t                 *subvec;

  memset( &ret, 0, sizeof(attr_value_t) );
  ret.kind = Sim_Val_List;
  vec = mallocAttribute(n_sets + 1);
  ret.u.list.size   = n_sets + 1;
  ret.u.list.vector = vec;

  // saves the current global time
  vec[n_sets].kind      = Sim_Val_Integer;
  vec[n_sets].u.integer = cache_obj->getEventQueue()->getCycle();

  for (uint32 i = 0; i < n_sets; i++ ) {
    vec[i].kind        = Sim_Val_List;
    vec[i].u.list.size = assoc * 2;
    subvec             = mallocAttribute(assoc * 2);
    vec[i].u.list.vector = subvec;
    
    for (uint32 j = 0; j < assoc; j++) {
      subvec[j*2].kind      = Sim_Val_Integer;
      subvec[j*2].u.integer = cache_sets[i*assoc + j].address_state;
      subvec[j*2 + 1].kind      = Sim_Val_Integer;
      // assuming cycles will be restarted at 0, we need to shift all
      // mru times by the current cycle value 
      subvec[j*2 + 1].u.integer = cache_sets[i*assoc + j].last_access;
    }
  }
  return (ret);
}

//**************************************************************************
template <class BlockType>
set_error_t generic_cache_template<BlockType>::set_cache_data( void *ptr,
    void *obj, attr_value_t *value ) {
  generic_cache_template<BlockType> *cache_obj  = static_cast<generic_cache_template<BlockType> *>(ptr);
  BlockType                    *cache_sets = cache_obj->cache;
  uint32                       n_sets = cache_obj->n_sets;
  uint32                       assoc  = cache_obj->m_assoc;
  attr_value_t                 *vec;
  attr_value_t                 *subvec;

  if (value->kind != Sim_Val_List)
    return Sim_Set_Need_List;
  if (value->u.list.size != n_sets + 1)
    return Sim_Set_Illegal_Value;
  
  vec = value->u.list.vector;
  
  if (vec[n_sets].kind != Sim_Val_Integer)
    return Sim_Set_Illegal_Value;
  int64 offsetCycle = vec[n_sets].u.integer;
  for (uint32 i = 0; i < n_sets; i++ ) {
    if (vec[i].kind != Sim_Val_List)
      return Sim_Set_Need_List;
    if (vec[i].u.list.size != assoc * 2)
      return Sim_Set_Illegal_Value;

    subvec = vec[i].u.list.vector;
    for (uint32 j = 0; j < assoc; j++) {
      cache_sets[i*assoc + j].address_state = subvec[j*2].u.integer;
      cache_sets[i*assoc + j].last_access   = subvec[j*2 + 1].u.integer -
        offsetCycle;
    }
  }  
  return Sim_Set_Ok;
}

//**************************************************************************
template <class BlockType>
int generic_cache_template<BlockType>::registerCheckpoint( confio_t *conf )
{
  int rc;

  rc = conf->register_attribute( name,
                                 generic_cache_template<BlockType>::get_cache_data, (void *) this,
                                 generic_cache_template<BlockType>::set_cache_data, (void *) this );
  return rc;
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
