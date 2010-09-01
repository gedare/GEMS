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
 * FileName:  lsq.C
 * Synopsis:  Implements the functionality of the load/store queue
 * Author:    cmauer
 * Version:   $Id$
 */

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include "hfa.h"
#include "hfacore.h"
#include "lsq.h"

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Variable declarations                                                  */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Forward declarations                                                   */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Constructor(s) / destructor                                            */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Public methods                                                         */
/*------------------------------------------------------------------------*/

//**************************************************************************
void lsq_t::initialize( void ) {
    for (uint32 i = 0 ; i < LSQ_HASH_SIZE ; i ++) {
      m_queue[i] = NULL;
      //initialize stats
      memset((void *) &hash_bucket_stats[i], 0, sizeof(lsq_stats_t));
      memset((void *) &addr_overlap_stats[i], 0, sizeof(lsq_stats_t));
      memset((void *) &ld_st_conflict_stats[i], 0, sizeof(lsq_stats_t));
    }
    //initialize rest of stats
    memset((void *) &global_stats, 0, sizeof(lsq_stats_t));
    memset((void *) &addr_replay_stat, 0, sizeof(replay_stats_t));
    memset((void *) &ld_st_replay_stat, 0, sizeof(replay_stats_t));
}

//**************************************************************************
void lsq_t::doInsert( memory_inst_t *memop ) {

  uint64          lsqe_seq_num = memop->getSequenceNumber();
  memory_inst_t  *trav = m_queue[ generateIndex(memop->getLineAddress()) ];

  //update LSQ stats
  updateStats(true, memop);
  
#ifdef EXPENSIVE_ASSERTIONS
  verifyQueues();
#endif

  ASSERT( !memop->getEvent(EVENT_LSQ_INSERT) );
#ifdef EXPENSIVE_ASSERTIONS
  m_pseq->out_info("lsq insert: %d 0x%0x\n", memop->getWindowIndex(), (int) memop);
#endif
  memop->markEvent(EVENT_LSQ_INSERT);
  
  /* insert into the queue -- the LSQ is 'perfectly disambiguated' by the
   * sequence number of the instruction.
   */

  if (trav == NULL || (trav->getSequenceNumber() <= lsqe_seq_num)) {
#ifdef EXPENSIVE_ASSERTIONS
    ASSERT( trav != memop );
    m_pseq->out_info("lsq head insert seq:%d address:0x%0llx index: 0x%0x\n",
                     memop->getWindowIndex(),
                     memop->getLineAddress(), 
                     generateIndex(memop->getLineAddress()));
#endif
    /* insert at head of list */
    memop->lsq_prev = NULL;
    memop->lsq_next = trav;
    if (trav != NULL) { trav->lsq_prev = memop; }
    m_queue[ generateIndex(memop->getLineAddress()) ] = memop;
  } else {
    /* insert in middle of list (can happen as we are executing O-o-O),
     * or at the end of the list.
     */
    for ( ;; ) {
      ASSERT(trav != NULL);
      if ((trav->lsq_next == NULL) || 
          (trav->lsq_next->getSequenceNumber() < lsqe_seq_num)) {
        if (trav->lsq_next != NULL) { trav->lsq_next->lsq_prev = memop; }
        memop->lsq_prev = trav;
        memop->lsq_next = trav->lsq_next;
        trav->lsq_next = memop;
        break;
      }
      trav = trav->lsq_next;
    }
  }
}

//**************************************************************************
void lsq_t::remove( memory_inst_t *memop ) {
  memory_inst_t *next = memop->lsq_next;

  ASSERT( memop->lsq_next != memop );

  //update LSQ stats
  updateStats( false, memop);

  // link the next element to our previous element
  if (next != NULL) {
    if (next->lsq_prev != memop) {
      m_pseq->out_info("lsq: warning: I am          0x%0x\n", (uint64) memop);
      memop->printDetail();
      m_pseq->out_info("lsq: warning: next is       0x%0x\n", (uint64) next);
      next->printDetail();
      m_pseq->out_info("lsq: warning: next->prev is 0x%0x\n", (uint64) next->lsq_prev);
      ASSERT(next->lsq_prev == memop);      
    }
    next->lsq_prev = memop->lsq_prev;
  }

  // link the previous element to our next element
  if (memop->lsq_prev == NULL) {
    ASSERT( memop->getLineAddress() != (my_addr_t) -1 );
#ifdef EXPENSIVE_ASSERTIONS
    if (next != NULL)
      ASSERT( generateIndex(memop->getLineAddress()) ==
              generateIndex(next->getLineAddress()) );
#endif
    m_queue[ generateIndex(memop->getLineAddress()) ] = next;
  } else {
    memop->lsq_prev->lsq_next = next;
  }

  memop->lsq_prev = NULL;
  memop->lsq_next = NULL;
  
#ifdef EXPENSIVE_ASSERTIONS
  verifyQueues();
#endif
}

//**************************************************************************
bool lsq_t::loadSearch( memory_inst_t *memop ) {

  // snoop for stores which match this address
  memory_inst_t *trav = m_queue[ generateIndex(memop->getLineAddress()) ];
  my_addr_t lsq_line_addr = memop->getLineAddress();
  uint64    lsq_seq_num   = memop->getSequenceNumber();
  int32     listcount = 0;

#ifdef DEBUG_LSQ
  m_pseq->out_info("DEBUG_LSQ: loadSearch: index:%d addr:0x%0llx line:0x%0x seq:%d\n",
                   memop->getWindowIndex(), memop->getAddress(),
                   lsq_line_addr, lsq_seq_num);
#endif
  
  while (trav != NULL) {
    /* only get a match if is a store to the same address and has
       retire or has an older sequencer number */
    bool match = ( (trav->getLineAddress() == lsq_line_addr) &&
                   (trav->getSequenceNumber() < lsq_seq_num) &&
                   (trav->getStaticInst()->getType() != DYN_LOAD) );
#ifdef DEBUG_LSQ
    m_pseq->out_info("DEBUG_LSQ: index:%d line:0x%0llx addr:0x%0llx match:%d\n",
                     trav->getWindowIndex(), trav->getLineAddress(),
                     trav->getAddress(), match);
#endif
    if ( match ) {
      /* Check for overlap between accesses */
      uint16 overlap = memop->addressOverlap(trav);
#ifdef DEBUG_LSQ
      m_pseq->out_info("DEBUG_LSQ: overlap myindex:%d index:%d line:0x%0llx %d\n",
                       memop->getWindowIndex(), trav->getWindowIndex(),
                       memop->getLineAddress(), overlap);
#endif
      load_interface_t  *bypass_interface;
      if (memop->getStaticInst()->getType() == DYN_LOAD) {
        bypass_interface = static_cast<load_interface_t *>(static_cast<load_inst_t *>(memop));
      } else if (memop->getStaticInst()->getType() == DYN_ATOMIC) {
        bypass_interface = static_cast<load_interface_t *>(static_cast<atomic_inst_t *>(memop));
      } else {
        SIM_HALT;
      }

      if (overlap == MEMOP_EXACT) {

        /* if there is a complete match, bypass the data */
        store_inst_t      *depend = static_cast<store_inst_t *>(trav);
        bypass_interface->setDepend( depend );
        if (depend->isDataValid()) {
#ifdef DEBUG_LSQ
          m_pseq->out_info("DEBUG_LSQ: bypassing index:%d -> %d\n",
                           depend->getWindowIndex(), memop->getWindowIndex() );
#endif
          bypass_interface->lsqBypass();
        } else {
          /* we found a match in the store queue, but it's value
             wasn't available, so wait for the store value */
#ifdef DEBUG_LSQ
          m_pseq->out_info("DEBUG_LSQ: !svr: waiting... index:%d -> %d\n",
                           depend->getWindowIndex(), memop->getWindowIndex() );
#endif
          bypass_interface->lsqWait();
        }
        return true;
        
      } else if (overlap == MEMOP_OVERLAP) {
        
#ifdef DEBUG_LSQ
        m_pseq->out_info("DEBUG_LSQ: replay trap triggered instr %d\n",
                         memop->getWindowIndex());
#endif
        /* partial overlap -put load on wait list for Store to retire*/
        store_inst_t      *depend = static_cast<store_inst_t *>(trav);
        bypass_interface->addrOverlapWait(depend);
        //update conflict stats
        updateConflictStats( true, trav);

        //bypass_interface->replay();
        return true;
      }
    }
    trav = trav->lsq_next;

    // check that the LSQ is not insanely large
    //    This could be a performance issue, as large capacity LSQs 
    //    imply lots of linear linked-list searches.
    listcount++;
    if ( listcount >= 2*IWINDOW_WIN_SIZE ) {
      ERROR_OUT("[ %d ] error: LSQ is far too large: %d cycle[ %lld ]\n", m_pseq->getID(), listcount, m_pseq->getLocalCycle());

      // LUKE - for right now, just print when this occurs
      //print sequencer
      m_pseq->print();
      //printLSQ();
      SIM_HALT;
    }
  }
  return false;
}

//**************************************************************************
void lsq_t::storeSearch( store_inst_t *memop ) {

  // snoop for loads which match this address that are younger
  memory_inst_t *trav = m_queue[ generateIndex(memop->getLineAddress()) ];
  my_addr_t lsq_line_addr = memop->getLineAddress();
  uint64    lsq_seq_num   = memop->getSequenceNumber();
  int32     listcount = 0;

#ifdef DEBUG_LSQ
  m_pseq->out_info("DEBUG_LSQ: storeSearch: index:%d addr:0x%0llx line:0x%0x seq:%d\n",
                   memop->getWindowIndex(), memop->getAddress(),
                   lsq_line_addr, lsq_seq_num);
#endif
  
  while (trav != NULL) {
    
    bool match = ((trav->getLineAddress() == lsq_line_addr) &&
                  (trav->getSequenceNumber() > lsq_seq_num) &&
                  (trav->getStaticInst()->getType() != DYN_STORE));
#ifdef DEBUG_LSQ
    m_pseq->out_info("DEBUG_LSQ: index:%d line:0x%0llx addr:0x%0llx match:%d\n",
                     trav->getWindowIndex(), trav->getLineAddress(),
                     trav->getAddress(), match);
#endif
    if (match) {
      /* find all younger loads to the same address and Replay all that
       * data misspeculated (i.e. loaded values older than this store)
       */

      /* Check for overlap between accesses */
      if (memop->addressOverlap(trav)) {
        
#ifdef DEBUG_LSQ
        m_pseq->out_info("DEBUG_LSQ: overlap myindex:%d index:%d line:0x%0x %d\n",
                  memop->getWindowIndex(), trav->getWindowIndex(),
                  memop->getLineAddress());
#endif
        
        /* use the depend field in the load to avoid searching for
         * the younger store. */
        load_interface_t  *bypass_interface;
        if (trav->getStaticInst()->getType() == DYN_LOAD) {
          bypass_interface = static_cast<load_interface_t *>(static_cast<load_inst_t *>(trav));
        } else if (trav->getStaticInst()->getType() == DYN_ATOMIC) {
          bypass_interface = static_cast<load_interface_t *>(static_cast<atomic_inst_t *>(trav));
        } else {
          SIM_HALT;
        }
        
        store_inst_t *orig_store = bypass_interface->getDepend();
        if ((orig_store == NULL) || 
            (lsq_seq_num > orig_store->getSequenceNumber())) {
          /* no dependent store, or was older than this store, hence
             data misspeculation */
          //LUKE - update stats based on which condition caused replay
          if(orig_store == NULL){
            STAT_INC(m_pseq->m_stat_load_store_conflicts[trav->getProc()] );
          }
          else if(lsq_seq_num > orig_store->getSequenceNumber()){
            //update stat based on which type (LD or ATOMIC) got replayed
            if(trav->getStaticInst()->getType() == DYN_LOAD){
              STAT_INC( m_pseq->m_stat_load_incorrect_store[trav->getProc()] );
            }
            else if(trav->getStaticInst()->getType() == DYN_ATOMIC){
              STAT_INC( m_pseq->m_stat_atomic_incorrect_store[trav->getProc()] );
            }
          }
#ifdef DEBUG_LSQ
          m_pseq->out_info("DEBUG_LSQ: store search: replay trap triggered by instr %d on instr %d\n",
                           memop->getWindowIndex(), trav->getWindowIndex());
#endif
          //keep stats on memory dependence
          la_t store_vpc = memop->getVPC();
          la_t load_vpc = trav->getVPC();

          //update StoreSet predictor
          if(STORESET_PREDICTOR){
            m_pseq->updateStoreSet( store_vpc, load_vpc );
          }
          //update conflict stats
          updateConflictStats( false, memop);
          bypass_interface->replay();
        }
      }
    }
    trav = trav->lsq_next;

    // check that the LSQ is not insanely large
    listcount++;
    if ( listcount >= 2*IWINDOW_WIN_SIZE ) {
      ERROR_OUT("[ %d ] error: LSQ is far too large: %d cycle[ %lld ]\n", m_pseq->getID(), listcount, m_pseq->getLocalCycle());

      // LUKE - for right now, just print when this occurs
      //print sequencer
      m_pseq->print();
      //printLSQ();
      SIM_HALT;
    }
  }
}

//**************************************************************************
void lsq_t::verify(memory_inst_t *lsq_slice, bool halt_on_error) {
  memory_inst_t *trav = lsq_slice;
  memory_inst_t *prev = NULL;
  
  while (trav != NULL) {
    
    m_pseq->out_info("lsq: %d: 0x%0x: index:0x%0x address: 0x%0llx\n",
                     trav->getWindowIndex(),
                     (uint64) trav, generateIndex(trav->getLineAddress()),
                     trav->getLineAddress());
    switch (trav->getStaticInst()->getType()) {
    case DYN_LOAD:
      static_cast<load_inst_t *>(trav)->printDetail();
      break;
    case DYN_STORE:
      static_cast<store_inst_t *>(trav)->printDetail();
      break;
    case DYN_ATOMIC:
      static_cast<atomic_inst_t *>(trav)->printDetail();
      break;
    default:
      static_cast<memory_inst_t *>(trav)->printDetail();
    }

    if (m_queue[ generateIndex(trav->getLineAddress()) ] != lsq_slice) {
      m_pseq->out_info( "error: LSQ index not in slice: 0x%0llx %d\n",
                        trav->getLineAddress(),
                        generateIndex(trav->getLineAddress()) );
      if (halt_on_error) {
        SIM_HALT;
      }
    }
    
    if ( trav->lsq_prev != prev ) {
      m_pseq->out_info( "error: LSQ is not a doubly-linked list.\n" );
      if (halt_on_error) {
        SIM_HALT;
      }
    }

    if (prev != NULL) {
      if (prev->getSequenceNumber() <= trav->getSequenceNumber()) {
        m_pseq->out_info( "error: LSQ is not in increasing order!\n");
        m_pseq->out_info( "error: prev seq %lld, current seq %lld\n",
                          prev->getSequenceNumber(), 
                          trav->getSequenceNumber() );
        if (halt_on_error) {
          SIM_HALT;
        }
      }
    }
    
    if (trav->getEvent(EVENT_LSQ_INSERT) != true ) {
      m_pseq->out_info( "error: memory instruction not marked as LSQ insert\n");
      trav->printDetail();
      if (halt_on_error) {
        SIM_HALT;
      }
    }
    
    // Verify the Instruction is at the correct WindowPosition
    if ( !(trav->getEvent(EVENT_IWINDOW_REMOVED)) ) {
      if (!m_pseq->getIwindow(trav->getProc())->isAtPosition(trav, trav->getWindowIndex())) {
        m_pseq->out_info("lsq window violation: index:%d\n",
                         trav->getWindowIndex());
        trav->printDetail();
        if (halt_on_error) {
          SIM_HALT;
        }
      }
    }

    prev = trav;
    trav = trav->lsq_next;
  }
}

//*******************************************************************
// used to update the global and hash bucket stats
void lsq_t::updateStats(bool insert, memory_inst_t * memop){
  //get current cycle of insert
  uint64 current_cycle = m_pseq->getLocalCycle();
  uint32 index = generateIndex(memop->getLineAddress());

  //update global stats
  uint64 elapsed_cycles = current_cycle - global_stats.last_total_change_cycle;
  //don't count changes in same cycle
  if(elapsed_cycles > 0){
    //take into account how many cycles LSQ was at previous size
    global_stats.total_size_sum += global_stats.total_current_size*elapsed_cycles;
    hash_bucket_stats[index].total_size_sum += hash_bucket_stats[index].total_current_size*elapsed_cycles;
    if(memop->getStaticInst()->getType() == DYN_LOAD){
      //update load stats
      global_stats.load_size_sum += global_stats.load_current_size*elapsed_cycles;
      hash_bucket_stats[index].load_size_sum += hash_bucket_stats[index].load_current_size*elapsed_cycles;
    }
    else if(memop->getStaticInst()->getType() == DYN_STORE){
      //update store stats
      global_stats.store_size_sum += global_stats.store_current_size*elapsed_cycles;
      hash_bucket_stats[index].store_size_sum += hash_bucket_stats[index].store_current_size*elapsed_cycles;
    }
    else if(memop->getStaticInst()->getType() == DYN_ATOMIC){
      //update atomic stats
      global_stats.atomic_size_sum += global_stats.atomic_current_size*elapsed_cycles;
      hash_bucket_stats[index].atomic_size_sum += hash_bucket_stats[index].atomic_current_size*elapsed_cycles;
    }
  }
  global_stats.last_total_change_cycle = current_cycle;
  if(insert){
    global_stats.total_current_size++;
  }
  else{
    ASSERT(global_stats.total_current_size > 0);
    global_stats.total_current_size--;
  }
  if(global_stats.total_max_size < global_stats.total_current_size){
    global_stats.total_max_size = global_stats.total_current_size;
  }
  hash_bucket_stats[index].last_total_change_cycle = current_cycle;
  if(insert){
    hash_bucket_stats[index].total_current_size++;
  }
  else{
    ASSERT(hash_bucket_stats[index].total_current_size > 0);
    hash_bucket_stats[index].total_current_size--;
  }
  if(hash_bucket_stats[index].total_max_size < hash_bucket_stats[index].total_current_size){
    hash_bucket_stats[index].total_max_size = hash_bucket_stats[index].total_current_size;
  }
  //update specific type stats
  if(memop->getStaticInst()->getType() == DYN_LOAD){
    global_stats.last_load_change_cycle = current_cycle;
    if(insert){
      global_stats.load_current_size++;
    }
    else{
      ASSERT(global_stats.load_current_size > 0);
      global_stats.load_current_size--;
    }
    if(global_stats.load_max_size < global_stats.load_current_size){
      global_stats.load_max_size = global_stats.load_current_size;
    }
    hash_bucket_stats[index].last_load_change_cycle = current_cycle;
    if(insert){
      hash_bucket_stats[index].load_current_size++;
    }
    else{
      ASSERT(hash_bucket_stats[index].load_current_size > 0);
      hash_bucket_stats[index].load_current_size--;
    }
    if(hash_bucket_stats[index].load_max_size < hash_bucket_stats[index].load_current_size){
      hash_bucket_stats[index].load_max_size = hash_bucket_stats[index].load_current_size;
    }
  }
  else if(memop->getStaticInst()->getType() == DYN_STORE){
    global_stats.last_store_change_cycle = current_cycle;
    if(insert){
      global_stats.store_current_size++;
    }
    else{
      ASSERT(global_stats.store_current_size > 0);
      global_stats.store_current_size--;
    }
    if(global_stats.store_max_size < global_stats.store_current_size){
      global_stats.store_max_size = global_stats.store_current_size;
    }
    hash_bucket_stats[index].last_store_change_cycle = current_cycle;
    if(insert){
      hash_bucket_stats[index].store_current_size++;
    }
    else{
      ASSERT(hash_bucket_stats[index].store_current_size > 0);
      hash_bucket_stats[index].store_current_size--;
    }
    if(hash_bucket_stats[index].store_max_size < hash_bucket_stats[index].store_current_size){
      hash_bucket_stats[index].store_max_size = hash_bucket_stats[index].store_current_size;
    }
  }
  else if(memop->getStaticInst()->getType() == DYN_ATOMIC){
    global_stats.last_atomic_change_cycle = current_cycle;
    if(insert){
      global_stats.atomic_current_size++;
    }
    else{
      ASSERT(global_stats.atomic_current_size > 0);
      global_stats.atomic_current_size--;
    }
    if(global_stats.atomic_max_size < global_stats.atomic_current_size){
      global_stats.atomic_max_size = global_stats.atomic_current_size;
    }
    hash_bucket_stats[index].last_atomic_change_cycle = current_cycle;
    if(insert){
      hash_bucket_stats[index].atomic_current_size++;
    }
    else{
      ASSERT(hash_bucket_stats[index].atomic_current_size > 0);
      hash_bucket_stats[index].atomic_current_size--;
    }
    if(hash_bucket_stats[index].atomic_max_size < hash_bucket_stats[index].atomic_current_size){
      hash_bucket_stats[index].atomic_max_size = hash_bucket_stats[index].atomic_current_size;
    }
  }
}

//**********************************************************************
// Used to update conflict stats (addr overlap and ld-st dependence)
void lsq_t::updateConflictStats(bool addr_overlap, memory_inst_t * depend){
  // depend points to ST/ATOMIC that causes LD to conflict with it
  uint32 index = generateIndex(depend->getLineAddress());
  lsq_stats_t * stats = NULL;
  if(addr_overlap){
    stats = &addr_overlap_stats[index];
    //update cause of conflict
    if(depend->getStaticInst()->getType() == DYN_STORE){
      addr_replay_stat.store++;
    }
    else if(depend->getStaticInst()->getType() == DYN_ATOMIC){
      addr_replay_stat.atomic++;
    }
  }
  else{
    stats = &ld_st_conflict_stats[index];
    if(depend->getStaticInst()->getType() == DYN_STORE){
      ld_st_replay_stat.store++;
    }
    else if(depend->getStaticInst()->getType() == DYN_ATOMIC){
      ld_st_replay_stat.atomic++;
    }
  }

  stats->total_size_sum += hash_bucket_stats[index].total_current_size;
  stats->num_samples++;
  if(stats->total_max_size < hash_bucket_stats[index].total_current_size){
    stats->total_max_size = hash_bucket_stats[index].total_current_size;
  }
  stats->load_size_sum += hash_bucket_stats[index].load_current_size;
  if(stats->load_max_size < hash_bucket_stats[index].load_current_size){
    stats->load_max_size = hash_bucket_stats[index].load_current_size;
  }
  stats->store_size_sum += hash_bucket_stats[index].store_current_size;
  if(stats->store_max_size < hash_bucket_stats[index].store_current_size){
    stats->store_max_size = hash_bucket_stats[index].store_current_size;
  }
  stats->atomic_size_sum += hash_bucket_stats[index].atomic_current_size;
  if(stats->atomic_max_size < hash_bucket_stats[index].atomic_current_size){
    stats->atomic_max_size = hash_bucket_stats[index].atomic_current_size;
  }
}

//**************************************************************************
void lsq_t::printStats(out_intf_t * out, uint64 sim_cycles){
  out->out_info("\n*** LSQ Internal Stats\n");
  out->out_info("\tGlobal Stats:\n");
  out->out_info("\t--------\n");
  out->out_info("\tTOTAL: max_size[ %d ] avg_size[ %6.3f ]\n", global_stats.total_max_size, 1.0*global_stats.total_size_sum/sim_cycles);
  out->out_info("\tLOAD: max_size[ %d ] avg_size[ %6.3f ]\n", global_stats.load_max_size, 1.0*global_stats.load_size_sum/sim_cycles);
  out->out_info("\tSTORE: max_size[ %d ] avg_size[ %6.3f ]\n", global_stats.store_max_size, 1.0*global_stats.store_size_sum/sim_cycles);
  out->out_info("\tATOMIC: max_size[ %d ] avg_size[ %6.3f ]\n", global_stats.atomic_max_size, 1.0*global_stats.atomic_size_sum/sim_cycles);
  out->out_info("\n");
  out->out_info("\tHash Bucket Stats: %d total buckets\n", LSQ_HASH_SIZE);
  out->out_info("\t-------------\n");
  for(int i=0; i < LSQ_HASH_SIZE; ++i){
    out->out_info("\tBucket[ %d ] TOT: max[ %d ] avg[ %6.3f ] LD: max[ %d ] avg[ %6.3f ] ST: max[ %d ] avg[ %6.3f ] AT: max[ %d ] avg[ %6.3f ]\n", i, hash_bucket_stats[i].total_max_size, 1.0*hash_bucket_stats[i].total_size_sum/sim_cycles, hash_bucket_stats[i].load_max_size, 1.0*hash_bucket_stats[i].load_size_sum/sim_cycles, hash_bucket_stats[i].store_max_size, 1.0*hash_bucket_stats[i].store_size_sum/sim_cycles, hash_bucket_stats[i].atomic_max_size, 1.0*hash_bucket_stats[i].atomic_size_sum/sim_cycles);
  }
  out->out_info("\n");
  out->out_info("\tAddr Overlap Conflict Hash Bucket Stats: %d total buckets\n", LSQ_HASH_SIZE);
  out->out_info("\t-------------\n");
  for(int i=0; i < LSQ_HASH_SIZE; ++i){
    out->out_info("\tBucket[ %d ] TOT: max[ %d ] avg[ %6.3f ] LD: max[ %d ] avg[ %6.3f ] ST: max[ %d ] avg[ %6.3f ] AT: max[ %d ] avg[ %6.3f ]\n", i, addr_overlap_stats[i].total_max_size, 1.0*addr_overlap_stats[i].total_size_sum/addr_overlap_stats[i].num_samples, addr_overlap_stats[i].load_max_size, 1.0*addr_overlap_stats[i].load_size_sum/addr_overlap_stats[i].num_samples, addr_overlap_stats[i].store_max_size, 1.0*addr_overlap_stats[i].store_size_sum/addr_overlap_stats[i].num_samples, addr_overlap_stats[i].atomic_max_size, 1.0*addr_overlap_stats[i].atomic_size_sum/addr_overlap_stats[i].num_samples);
  }
  out->out_info("\n");
  out->out_info("\tLD-ST Conflict Hash Bucket Stats: %d total buckets\n", LSQ_HASH_SIZE);
  out->out_info("\t-------------\n");
  for(int i=0; i < LSQ_HASH_SIZE; ++i){
    out->out_info("\tBucket[ %d ] TOT: max[ %d ] avg[ %6.3f ] LD: max[ %d ] avg[ %6.3f ] ST: max[ %d ] avg[ %6.3f ] AT: max[ %d ] avg[ %6.3f ]\n", i, ld_st_conflict_stats[i].total_max_size, 1.0*ld_st_conflict_stats[i].total_size_sum/ld_st_conflict_stats[i].num_samples, ld_st_conflict_stats[i].load_max_size, 1.0*ld_st_conflict_stats[i].load_size_sum/ld_st_conflict_stats[i].num_samples, ld_st_conflict_stats[i].store_max_size, 1.0*ld_st_conflict_stats[i].store_size_sum/ld_st_conflict_stats[i].num_samples, ld_st_conflict_stats[i].atomic_max_size, 1.0*ld_st_conflict_stats[i].atomic_size_sum/ld_st_conflict_stats[i].num_samples);
  }
  out->out_info("\n");
  out->out_info("\tAddr Overlap Replay Causes:    stores[ %lld ] atomics[ %lld ]\n", addr_replay_stat.store, addr_replay_stat.atomic);
  out->out_info("\tLD-ST Replay Causes:    stores[ %lld ] atomics[ %lld ]\n",ld_st_replay_stat.store, ld_st_replay_stat.atomic);
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


/** [Memo].
 *  [Internal Documentation]
 */
//**************************************************************************

