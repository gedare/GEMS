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
 * FileName:  symtrace.C
 * Synopsis:  Implementation of thread and instruction PC tracking.
 * Author:    cmauer
 * Version:   $Id: symtrace.C 1.5 05/08/14 17:07:14-05:00 lyen@opus.cs.wisc.edu $
 */

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include "hfa.h"
#include "hfacore.h"
#include "threadstat.h"
#include "symtrace.h"

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

const uint64  SYMTRACE_NO_PROCESS = (uint64) -1;
const int32   PROF_OP_COUNT       = 1;
const int32   PROF_PAGESIZE       = 4096;

/*------------------------------------------------------------------------*/
/* Variable declarations                                                  */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Forward declarations                                                   */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Constructor(s) / destructor                                            */
/*------------------------------------------------------------------------*/

//**************************************************************************
symtrace_t::symtrace_t( int32 numProcs )
{
  m_proc_addr = (uint64 *) malloc( sizeof(uint64)*numProcs );
  m_step_count = (pc_step_t *) malloc( sizeof(pc_step_t)*numProcs );
  for ( int32 i = 0; i < numProcs; i++ ) {
    m_proc_addr[i] = SYMTRACE_NO_PROCESS;
    m_step_count[i] = 0;
  }
}

//**************************************************************************
symtrace_t::~symtrace_t( )
{
  if (m_proc_addr)
    free( m_proc_addr );
  if (m_step_count)
    free( m_step_count );
}

static uint64 s_skipped_pages = 0;
static PhysicalToLogicalMap s_total_exec_per_page;
static PhysicalToLogicalMap *s_exec_per_page;

//**************************************************************************
static integer_t s_get_profile_value( integer_t ppage )
{
  // CM doesn't work 5/15/2003
#if 0
  integer_t total = SIM_get_profile_value( PROF_OP_COUNT,
                                           ppage, ppage + PROF_PAGESIZE );
#endif

//   attr_value_t val = SIM_profiling_info( ppage, PROF_PAGESIZE );
//   if (val.kind == Sim_Val_List) {
//     attr_value_t element = val.u.list.vector[0];
//     if (element.kind != Sim_Val_Integer) {
//       return 0;
//     }
//     return element.u.integer;
//   }
  return 0;
}

//**************************************************************************
static uint64 s_total_profile( integer_t ppage )
{
  s_total_exec_per_page[ppage] = s_get_profile_value( ppage );
  // DEBUG_OUT("t: 0x%0llx %lld\n", ppage, s_total_exec_per_page[ppage]);
  return 0;
}

//**************************************************************************
static uint64 s_accumulate_profile( integer_t ppage )
{
  integer_t new_total = s_get_profile_value( ppage );
  integer_t delta     = 0;
  
  if (s_total_exec_per_page.find(ppage) != s_total_exec_per_page.end()) {
    integer_t old_total = s_total_exec_per_page[ppage];
    if (old_total == new_total) {
      return 0;
    } else if (old_total > new_total) {
      DEBUG_OUT("warning! page totals decreasing %lld %lld\n",
                old_total, new_total);
    } else {
      delta = new_total - old_total;
    }
  }
  
  // add (or insert) the count to the page map
  if (delta != 0) {
    if (s_exec_per_page->find( ppage ) == s_exec_per_page->end()) {
      (*s_exec_per_page)[ppage] = delta;
    } else {
      (*s_exec_per_page)[ppage] += delta;
    }
  }
  s_total_exec_per_page[ppage] = new_total;
  return 0;
}

//**************************************************************************
void symtrace_t::threadSwitch( int32 cpu, uint64 threadid )
{
  //Luke - the actual sequencer number
  int32 seq_num = cpu / CONFIG_LOGICAL_PER_PHY_PROC;
  uint32 logical_proc_num = cpu % CONFIG_LOGICAL_PER_PHY_PROC;
  ASSERT(logical_proc_num < CONFIG_LOGICAL_PER_PHY_PROC);
  ASSERT(seq_num >= 0 && seq_num < system_t::inst->m_numSMTProcs);

  // check the process table (threadid should be unique)
  if ( m_thread_stats.find(threadid) == m_thread_stats.end() ) {
    // add the thread id to the table
    DEBUG_OUT("  symtrace: created new thread: 0x%0llx\n",
              threadid);
    m_thread_stats[threadid] = new thread_stat_t( threadid, -1, cpu );
  }

  // accumulate the profiling info for prior thread (if any)
  if (m_proc_addr[cpu] == SYMTRACE_NO_PROCESS) {
#ifdef SYM_PROFILE
    SIM_dump_caches();
    SIM_for_all_memory_pages( (void (*)(long long int)) s_total_profile,
                              0 );
#endif
  } else {
    thread_stat_t *prior_thread = m_thread_stats[m_proc_addr[cpu]];

#ifdef SYM_PROFILE
    // accumulate profiling info
    s_exec_per_page = &(prior_thread->m_exec_per_page);
    SIM_dump_caches();
    SIM_for_all_memory_pages( (void (*)(long long int)) s_accumulate_profile,
                              0 );
    s_exec_per_page = NULL;
#endif

    // account for a number of instructions (and a latency) for each thread
    pc_step_t count = system_t::inst->m_state[seq_num]->getTime(logical_proc_num);
    if (count >= m_step_count[cpu]) {
      // add the step size sample to the thread utilization
      pc_step_t sample = count - m_step_count[cpu];
      prior_thread->m_execution_count += sample;
    } else {
      // ignore sample: print warning
      ERROR_OUT("  symtrace_t::threadSwitch called with negative step time %lld < %ldd \n", count, m_step_count[cpu]);
    }
  }

  // update the process ID & time (note: must occur after accumulating time)
  m_proc_addr[cpu] = threadid;
  m_step_count[cpu] = system_t::inst->m_state[seq_num]->getTime(logical_proc_num);
}

//**************************************************************************
void symtrace_t::transactionCompletes( int32 cpu )
{
  /* DEBUG_OUT("transaction completes: %d 0x%0llx %lld\n",
            cpu, m_proc_addr[cpu], system_t::inst->m_state[cpu]->getTime() );
  */

  //Luke - the actual sequencer number
  int32 seq_num = cpu / CONFIG_LOGICAL_PER_PHY_PROC;
  uint32 logical_proc_num = cpu % CONFIG_LOGICAL_PER_PHY_PROC;
  ASSERT(logical_proc_num < CONFIG_LOGICAL_PER_PHY_PROC);
  ASSERT(seq_num >= 0 && seq_num < system_t::inst->m_numSMTProcs);

  if (m_proc_addr[cpu] == SYMTRACE_NO_PROCESS)
    return;

  thread_stat_t *thread_stat = m_thread_stats[m_proc_addr[cpu]];
#if 0
  if (thread_stat->m_transactions_completed != 0) {
    DEBUG_OUT("  transaction completed: 0x%0llx %lld\n",
              m_proc_addr[cpu],
              (system_t::inst->m_state[seq_num]->getTime(logical_proc_num) -
               thread_stat->m_last_transaction_time) );
  }
#endif

  thread_stat->m_last_transaction_time = system_t::inst->m_state[seq_num]->getTime(logical_proc_num);
  thread_stat->m_transactions_completed++;
}

//**************************************************************************
void symtrace_t::memoryAccess( int32 cpu, memory_transaction_t *memop )
{
  // do nothing unless we know the thread information for this CPU
  if (m_proc_addr[cpu] == SYMTRACE_NO_PROCESS)
    return;

  // get the thread stats for this thread
  thread_stat_t *ts = m_thread_stats[ m_proc_addr[cpu] ];
  if (ts == NULL) {
    DEBUG_OUT("  symtrace: cpu: %d process: 0x%0x not found\n",
              cpu, m_proc_addr[cpu] );
    return;
  }
  
  /// look the physical address up in thread stat table (8KB pgsz hardcoded)
  pa_t  addr    = memop->s.physical_address & ~(0x1fff);
  la_t  la_addr = memop->s.logical_address  & ~(0x1fff);
  if (ts->m_p2l_map.find(addr) == ts->m_p2l_map.end() ) {
    ts->m_p2l_map[addr] = la_addr;
  } else {
    // check that the mapping is already set correctly
    if (ts->m_p2l_map[addr] != la_addr) {
      DEBUG_OUT("symtrace: PA->LA map conflict: 0x%0llx  PA: 0x%0llx  LA: 0x%0llx  OA: 0x%0llx\n",
                ts->m_logical_address, addr, la_addr, ts->m_p2l_map[addr]);
    }
  }
}

//**************************************************************************
void symtrace_t::printStats( void )
{
  // for each thread, print out statistics
  DEBUG_OUT( "Skipped pages: %lld\n", s_skipped_pages );
  DEBUG_OUT( "ID Thread Ptr    Retired PID (if any)\n");
  int32 max_internal_id  = -1;
  ThreadStatTable::iterator iter;
  for ( iter = m_thread_stats.begin();
        iter != m_thread_stats.end();
        iter++ ) {
    thread_stat_t *ts = iter->second;
    if (ts->m_internal_id > max_internal_id) {
      max_internal_id = ts->m_internal_id;
    }
    DEBUG_OUT( "%2d 0x%0llx %0lld %4d %4d\n", ts->m_internal_id,
               iter->first, 
               ts->m_execution_count,
               ts->m_pid,
               ts->m_transactions_completed );
  }

#ifdef SYM_PROFILE
  DEBUG_OUT("Per thread execution profile\n");
  for ( iter = m_thread_stats.begin();
        iter != m_thread_stats.end();
        iter++ ) {
    thread_stat_t *ts = iter->second;
    PhysicalToLogicalMap::iterator p_iter = ts->m_exec_per_page.begin();
    while (p_iter != ts->m_exec_per_page.end() ) {
      pa_t pa = p_iter->first;
      la_t count = p_iter->second;
      DEBUG_OUT( "0x%llx 0x%0llx %lld\n",
                 ts->m_logical_address, pa, count );
      p_iter++;
    }
  }
#endif

  DEBUG_OUT("Per thread address information\n");
  for ( iter = m_thread_stats.begin();
        iter != m_thread_stats.end();
        iter++ ) {
    thread_stat_t *ts = iter->second;
    DEBUG_OUT( "Thread %2d 0x%0llx\n",
               ts->m_internal_id, iter->first );

    PhysicalToLogicalMap::iterator p_iter = ts->m_p2l_map.begin();
    while (p_iter != ts->m_p2l_map.end() ) {
      pa_t pa = p_iter->first;
      la_t la = p_iter->second;
      DEBUG_OUT( "0x%llx 0x%0llx 0x%0llx\n",
                 ts->m_logical_address, pa, la);
      p_iter++;
    }
  }
  
#if 0
  // print out thread statistics
  DEBUG_OUT( "Thread Migration Stats\n");
  for ( iter = m_thread_stats.begin();
        iter != m_thread_stats.end();
        iter++ ) {
    thread_stat_t *ts = iter->second;
    if (ts->m_migrations > 0) {
      DEBUG_OUT( "MIGRATIONS 0x%0llx %d\n", iter->first, ts->m_migrations );
    }
  }
#endif
}

/*------------------------------------------------------------------------*/
/* Public methods                                                         */
/*------------------------------------------------------------------------*/

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

