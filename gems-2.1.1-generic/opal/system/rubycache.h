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
#ifndef _RUBYCACHE_H_
#define _RUBYCACHE_H_

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Class declaration(s)                                                   */
/*------------------------------------------------------------------------*/

/**
 *  Status for ruby accesses. The memory hierarchy can hit, miss or
 *  not be able to handle a memory request.
 */

#include "Map.h"
#include "Vector.h"

enum ruby_status_t { MISS, HIT, NOT_READY };

#define MAX_PREFETCH_ENTRIES   32
// number of lines to prefetch
#define PREFETCH_DISTANCE          8

// 13 = 8k byte page
#define PAGE_SIZE_BITS     13
// 6 = 64byte line
#define BLOCK_SIZE_BITS     6

#define TOP_MEM_BYTE_MASK 0xff00000000000000ULL

/**
 * ruby_request is a doubly-linked list structure of misses (that
 * may or may not have issued) to the ruby memory cache hierarchy.
 * 
 * Note that some cache accesses wont create a request structure, 
 * as they can hit immediately in the cache. (This is the fast_path
 * hit optimization in the ruby_cache.)
 *
 * ruby_requests may also have not issued to the memory hierarchy
 * (if issued == false there are insufficient resources).
 */

class ruby_request_t : public pipestate_t {
public:
  /** @name Constructor(s) / Destructor */
  //@{
  /// Constructor: creates object
  ruby_request_t( waiter_t *waiter, la_t logical_address, pa_t address, OpalMemop_t requestType,
                  la_t vpc, bool priv, uint64 posttime, int thread);
  /// Destructor: frees object.
  ~ruby_request_t( );

  /** Allocates object throuh allocator interface */
  void *operator new( size_t size ) {
    return ( m_myalloc.memalloc( size ) );
  }
  /** frees object through allocator interface */
  void operator delete( void *obj ) {
    m_myalloc.memfree( obj );
  }
  //@}

  /** @name methods */
  //@{
  bool match( pa_t other_address ) {
    return ( other_address == m_physical_address );
  }
  
  /// get the address of the cache line miss
  pa_t getAddress( void ) {
    return m_physical_address;
  }

  /// The sorting order of these elements: based on their instruction #
  uint64      getSortOrder( void );

  /// A debugging print function
  void        print( void );
  //@}

  /** @name memory allocation:: */
  //@{
  static listalloc_t m_myalloc;
  //@}

  /// The logical address that missed in the cache
  la_t         m_logical_address;
  /// The physical address that missed in the cache (may be cache line address)
  pa_t        m_physical_address;
  
  /// The type of request that this miss is.
  OpalMemop_t m_request_type;
  
  /// virtual address of the program counter that caused this miss
  la_t        m_vpc;

  /// true if the instruction is privileged
  bool        m_priv;

  /// the time this instruction was inserted
  uint64      m_post_time;

  /// the thread that initiated this request
  int          m_thread;

  /// wait list of dynamic instructions
  wait_list_t m_wait_list;

  /// a pointer to the waiter (used by WriteBuffer to delete constructed waiter)
  waiter_t * m_waiter;

  /// flag used by the Write Buffer to indicate whether this store has been sent to Ruby or not
  bool  m_is_outstanding;

  //flag used to indicate whether this request is L2 miss
  bool m_l2miss;

  //used to count the number of dependents waiting for this L2 miss
  int m_l2dependents;

  //the destination registers of a load request
  reg_id_t       m_dest_reg[SI_MAX_DEST];
};

/**
* Interface object between ruby and opal. Handles misses to caches.
*
* @see     rubymiss_t, ruby_request_t
* @author  cmauer
* @version $Id$
*/
class rubycache_t : public waiter_t {

public:
  /**
   * @name Constructor(s) / destructor
   */
  //@{
  /**
   * Constructor: creates object
   * @param id         The id of the sequencer that this cache belongs to
   * @param block_size The size of the cache lines (in bits) in the system
   * @param eventQueue The event queue (used when it is necessary to reschedule due to resource conflicts). If this is NULL, the scheduler relies on the caller to poll the 'm_is_scheduled' variable to detect when 'Wakeup() should be called.
   */
  rubycache_t( uint32 id, uint32 block_size, scheduler_t *eventQueue );

  /**
   * Destructor: frees object.
   */
  virtual ~rubycache_t();
  //@}

  // Returns the number of outstanding demand requests
  int getOpalNumberOutstandingDemand();
  int getRubyNumberOutstandingDemand();

  //marks a request as being an L2 miss
  void markL2Miss(pa_t physical_address, OpalMemop_t type, int tagexists);

  // Returns true if the outstanding request is a L2 miss
  bool isRequestL2Miss(pa_t physical_address);

  // Returns true if there are any outstanding ST or ATOMIC requests
  bool searchStoreRequest();

  /**
   * @name Methods
   */
  //@{
  /**
   * called to initiate a load / store access.
   * @param physical_address The address which has been loaded or stored
   * @param mode Indicates if this is a load or a store
   * @param waiter The instruction to wake up (only if the instruction misses) when the ld/st is complete
   * @param mshr_hit true indicates a hit in the mshr
   * @param mshr_stall true indicates miss wasn't missed from lack of resources
   * @return bool true indicates hit, false indicates miss
   */
  ruby_status_t access( la_t logical_address, pa_t physical_address, OpalMemop_t requestType, la_t vpc, bool priv, waiter_t *inst, bool &mshr_hit, bool &mshr_stall, bool & conflicting_access, int thread);

  /**
   * called to initiate a prefetch
   * @param physical_address The address which has been loaded or stored
   * @param mode Indicates if this is a load or a store
   */
  ruby_status_t prefetch( la_t logical_address, pa_t physical_address, OpalMemop_t requestType, la_t vpc, bool priv, int thread);

  bool isLineInCache(pa_t physical_address, OpalMemop_t requestType, la_t vpc, bool priv );
  bool isLineInCacheReadable(pa_t physical_address, OpalMemop_t requestType, la_t vpc, bool priv );

  /* Called by pseq_t objects to check whether the incoming callback belongs to cache */
  bool checkOutstandingRequests(pa_t physical_address);

  /* Called by pseq_t objects to check whether the incoming callback belongs to a fastpath HIT */
  bool checkFastPathOutstanding(pa_t physical_address);

  /**
   * called to make a stale data request
   */
  bool  staleDataRequest( pa_t physical_address, char accessSize,
                          ireg_t *prediction );

  /**
   * called when a load / store is completed.
   * @param physical_address The address which has been loaded or stored
   */
  void complete( pa_t physical_address, bool abortRequest, OpalMemop_t type, int thread );

  /**
   *   Schedule the ruby cache to be woken up next cycle to handle delayed misses.
   */
  void scheduleWakeup( void );
  
  /**
   *   Wakeup to launch memory references stalled due to MSHR limits
   */
  void Wakeup( void );
  
  /**
   *   Notify the ruby cache of a squash of an instruction
   */
  void squashInstruction( dynamic_inst_t *inst );

  /**
   *  Debugging interface to advance timing: ticks the internal clock 
   *  of rubycache. After a number of cycles, any outstanding request 
   *  automatically complete.
   */
  void advanceTimeout( void );

  /**
   * Prints all outstanding memory references.
   */
  void print( void );

  /// get cache block size
  uint32 getBlockSize( void ) { return m_block_size; };

  /// get mshr count
  uint32 getMSHRcount() { return m_mshr_count;}

  //@}

  void printMLP(uint64 simulation_cycles, out_intf_t * out){
    double avg_mlp = (1.0*total_mlp_sum)/simulation_cycles;
    out->out_info("\n***MLP Stats: sim_cycles[ %lld ]\n", simulation_cycles);
    out->out_info("\n***Overall MLP (includes RA)\n");
    out->out_info("\tMax sum = %lld\n", max_sum);
    out->out_info("\tMax elapsed_time = %lld\n", max_elapsed_time);
    out->out_info("\tMax outstanding = %d\n", max_outstanding);
    out->out_info("\tTotal sum = %lld\n", total_mlp_sum);
    out->out_info("\tAVG MLP = %6.3f\n", avg_mlp);
    out->out_info("AVG elapsed cycles btw L2 misses = %6.3f\n", 1.0*m_elapsed_l2miss_cycle_sum/m_l2miss_cycle_samples);
    out->out_info("MAX elapsed cycles btw L2 misses = %lld\n", m_max_elapsed_l2miss_cycles);
    
    //for L2 misses
    avg_mlp = (1.0*total_l2_mlp_sum)/simulation_cycles;
    out->out_info("\n***L2Miss MLP\n");
    out->out_info("\tMax sum = %lld\n", max_l2_sum);
    out->out_info("\tMax elapsed_time = %lld\n", max_l2_elapsed_time);
    out->out_info("\tMax outstanding = %d\n", max_l2_outstanding);
    out->out_info("\tTotal sum = %lld\n", total_l2_mlp_sum);
    out->out_info("\tAVG MLP = %6.3f\n", avg_mlp);    
  }

  void updateViolatingStore(la_t store_vpc, la_t load_vpc);
  void printViolatingStores(out_intf_t * out);

  // prints stats on Dual mode addresses
  void printDualAddrs(out_intf_t * out);

  // for NL prefetcher
  void checkNLPrefetcher(pa_t physical_address, bool priv);
  void printNLPrefetcherStats(out_intf_t * out);

  // for instr page
  void updateInstrPage( pa_t physical_address, bool priv);
  void printInstrPageStats( out_intf_t * out);

  //l2 miss stats
  void printL2MissStats(out_intf_t * out);

private:
  //***********************************MLP BEGIN ************************
  //the running total of the MLP
  uint64 total_mlp_sum;
  //for tracking L2 misses
  uint64 total_l2_mlp_sum;

  //the current number of total outstading requests...used to calculated MLP
  int total_outstanding;
  //for tracking L2 misses
  int total_l2_outstanding;

  // timestamp of the last insert/deletion from the MSHR
  uint64 last_mshr_change;
  //for tracking L2 misses
  uint64 last_l2_mshr_change;

  //keeps track of the max component in the running MLP sum
  int max_outstanding;
  uint64 max_elapsed_time;
  uint64 max_sum;

  //for tracking L2 misses
  int max_l2_outstanding;
  uint64 max_l2_elapsed_time;
  uint64 max_l2_sum;

  uint64 m_last_l2miss_cycle;
  uint64 m_elapsed_l2miss_cycle_sum;
  uint64 m_max_elapsed_l2miss_cycles;
  uint64 m_l2miss_cycle_samples;

  // Note when we increment out L2 miss counter we can count the same request twice 
  //   (once by normal count and other by L2 count).  Therefore need to decrement both counters
  //   as necessary.
  void incrementTotalOutstanding(uint64 current_cycle){
    total_outstanding++;
    last_mshr_change = current_cycle;
  }

  // for L2 misses
  void incrementTotalL2Outstanding(uint64 current_cycle){
    total_l2_outstanding++;
    last_l2_mshr_change = current_cycle;
  }

  void decrementTotalOutstanding(uint64 current_cycle){
    total_outstanding--;
    last_mshr_change = current_cycle;
    ASSERT(total_outstanding >= 0);
  }

  // for L2 misses
  void decrementTotalL2Outstanding(uint64 current_cycle){
    total_l2_outstanding--;
    last_l2_mshr_change = current_cycle;
    ASSERT(total_l2_outstanding >= 0);
  }

  void updateMLPSum(uint64 current_cycle){
    uint64 elapsed_time = current_cycle - last_mshr_change;
    ASSERT(elapsed_time >= 0);
    // MLP = elapsed cycle * (total outstanding for elapsed cycles)
    uint64 sum =  (elapsed_time)*total_outstanding;
    if(total_outstanding > max_outstanding){
      max_sum = sum;
      max_outstanding = total_outstanding;
      max_elapsed_time = elapsed_time;
    }
    total_mlp_sum += sum;
  }

  //for L2 misses
  void updateL2MLPSum(uint64 current_cycle){
    uint64 elapsed_time = current_cycle - last_l2_mshr_change;
    ASSERT(elapsed_time >= 0);
    // MLP = elapsed cycle * (total outstanding for elapsed cycles)
    uint64 sum =  (elapsed_time)*total_l2_outstanding;
    if(total_l2_outstanding > max_l2_outstanding){
      max_l2_sum = sum;
      max_l2_outstanding = total_l2_outstanding;
      max_l2_elapsed_time = elapsed_time;
    }
    total_l2_mlp_sum += sum;
  }
  //************************************MLP END**************************

  /// read a character array as 16-bit memory value (converting endianess)
  void  read_array_as_memory16( int8 *buffer, uint64 *result, uint32 offset );

  /// read a character array as 32-bit memory value (converting endianess)
  void  read_array_as_memory32( int8 *buffer, uint64 *result, uint32 offset );

  /// read a character array as 64-bit memory value (converting endianess)
  void  read_array_as_memory64( int8 *buffer, uint64 *result, uint32 offset,
                                uint32 rindex );
  
  /// WATTCH power - keep track of the number of block bits
  uint32 m_block_bits;

  /// the sequencer's id for this processsor
  int32          m_id;
  /// the scheduler (used for rescheduling)
  scheduler_t   *m_event_queue;

  /// number of bytes in cacheline
  uint32         m_block_size;
  /// the clock for this module
  uint64         m_timeout_clock;

  /// memory request currently outstanding
  pa_t           m_fastpath_request;
  /// true if there is a 'fast path' request outstanding
  bool           m_fastpath_outstanding;
  /// hit status of last request
  bool           m_fastpath_hit;

  /// pool of instructions that are outstanding to memory
  pipepool_t    * m_request_pool;
  /// count of (non-icache) outstanding misses to the lower level of cache
  uint32         m_mshr_count;

  /// pool of instructions not currently being executed for lack of resources
  pipepool_t    * m_delayed_pool;

  /// boolean value: true if we are scheduled to run, false otherwise
  bool           m_is_scheduled;

  /// mask to strip off non-cache line bits
  pa_t           m_block_mask;

  // List of addresses which occur for IFETCH and LD/ST/AOMIC
  struct dual_info_t{
    // indicates use of addrs
    bool ifetch;
    bool load;
    bool store;
    bool atomic;
    //how many requests of each type
    uint64 count[4];
  };

  Map< pa_t, dual_info_t> * m_dual_addrs;
  void checkDualAddr(pa_t physical_address, OpalMemop_t type);

  //************************For Next Line Prefetching ***********************
  struct prefetch_entry_t{
    pa_t address;
    // the number of cachelines you actually prefetched (up to PREFETCH_DISTANCE)
    int num_pf_lines;
    bool valid;
    //for LRU replacement
    uint64 timestamp;
    // the last address used for prefetching
    pa_t last_pfaddr;
  };

  Vector< prefetch_entry_t > * m_next_line_prefetcher;

  //prefetcher stats
  uint64       m_stat_nl_prefetcher_hist[PREFETCH_DISTANCE+1];
  uint64       m_stat_nl_prefetcher_not_ready;
  uint64       m_stat_nl_prefetcher_accesses;
  uint64       m_stat_nl_prefetcher_hits;

  pa_t m_prev_lineaddr;

  // for separate prefetch table, page based
  Vector< prefetch_entry_t > * m_user_nl_prefetcher;
  Vector< prefetch_entry_t > * m_kernel_nl_prefetcher;

  void doGenericPrefetch(pa_t physical_address, bool priv);
  void doSeparatePrefetch(pa_t physical_address, bool priv);

  struct page_access_entry_t{
    pa_t page_address;
    // global counter of accesses
    uint64 num_accesses;
    // per line access counter
    uint64 line_accesses[128];
    // 0 = user 1 = supervisor
    bool priv[2];
    // per line priv access 
    bool line_priv[128][2];
    // each line keeps track of how many times the next line is accessed
    int    next_line_accesses[128][128];
    //tracks the last line to be accessed on the page
    int prev_line;
  };

  Vector< page_access_entry_t > * m_stat_instr_page;

  //**************************************************************
  // stats on which memops have l2misses, and whether a cache tag exists
  uint64 m_load_l2miss[2];
  uint64 m_store_l2miss[2];
  uint64 m_atomic_l2miss[2];
  uint64 m_ifetch_l2miss[2];
  
};

/*------------------------------------------------------------------------*/
/* Global variables                                                       */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Global functions                                                       */
/*------------------------------------------------------------------------*/

#endif  /* _RUBYCACHE_H_ */
