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
 * FileName:  chain.C
 * Synopsis:  Implements dependence tracking & statistics on memory chains
 * Author:    cmauer
 * Version:   $Id: chain.C 1.9 05/08/14 17:07:09-05:00 lyen@opus.cs.wisc.edu $
 */

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include "hfa.h"
#include "hfacore.h"
#include "ipagemap.h"
#include "histogram.h"
#include "dependence.h"
#include "flow.h"
#include "ptrace.h"
#include "sstat.h"
#include "chain.h"

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

#define CHAIN_MEMORY_LATENCY   80

/// The number of instructions that have been analyzed (system wide)
uint32 chain_t::s_num_instructions;

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
chain_t::chain_t( int id, uint32 window_size )
{
  ASSERT( system_t::inst == NULL );

  s_num_instructions = 0;
  m_pseq    = NULL;
  m_pstate  = NULL;
  m_id      = id;

   m_imap = new ipagemap_t* [CONFIG_LOGICAL_PER_PHY_PROC];
   m_constant_pred = new flow_inst_t * [CONFIG_LOGICAL_PER_PHY_PROC];

   for(uint k=0; k <  CONFIG_LOGICAL_PER_PHY_PROC; ++k){
     m_imap[k]    = new ipagemap_t( 16384 );
     m_constant_pred[k] = NULL;
    }
  m_is_init = false;

  //m_constant_pred = NULL;
  m_state = new mstate_t[CONFIG_LOGICAL_PER_PHY_PROC];

  m_last_fetched = new flow_inst_t *[CONFIG_LOGICAL_PER_PHY_PROC];
  m_null_static = new static_inst_t *[CONFIG_LOGICAL_PER_PHY_PROC];
  m_window = (flow_inst_t ***) malloc(sizeof(flow_inst_t **) * CONFIG_LOGICAL_PER_PHY_PROC);
  m_window_size = new uint32[CONFIG_LOGICAL_PER_PHY_PROC];
  m_window_limit = new uint32[CONFIG_LOGICAL_PER_PHY_PROC];
  m_window_utilization = new uint32[CONFIG_LOGICAL_PER_PHY_PROC];
  m_tail = new uint32[CONFIG_LOGICAL_PER_PHY_PROC];
  m_last_analyzed = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_head = new uint32[CONFIG_LOGICAL_PER_PHY_PROC];

  m_inst_consumer = new pf_consumer_t[CONFIG_LOGICAL_PER_PHY_PROC];
  m_data_consumer = new pf_consumer_t[CONFIG_LOGICAL_PER_PHY_PROC];
  m_overall_timer = new utimer_t[CONFIG_LOGICAL_PER_PHY_PROC];
  m_instructions_per_miss = new int64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_memory_depth = new int64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_max_memory_depth = new int64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_got_deeper = new int64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_got_shallower = new int64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_control_flow_depth = new int64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_window_flow_depth = new int64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_most_recent_fetch = new pa_t[CONFIG_LOGICAL_PER_PHY_PROC];

  m_max_branch_count = new uint32[CONFIG_LOGICAL_PER_PHY_PROC];
  m_inst_count = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_mem_count  = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_consumable_mem_count  = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_mem_miss_count  = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_mem_unusual_miss_count   = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_except_count   = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_reissue_count  = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_non_cpu_ini_count  = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_not_memory_count  = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_not_stallable_count  = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_st_ld_forward_count  = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_marked_counted  = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_deleted_misses  = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_repeated_misses  = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_repeated_accesses  = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_invalid_load_address  = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_invalid_store_address  = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  
   for(uint k=0; k <  CONFIG_LOGICAL_PER_PHY_PROC; ++k){
     m_last_fetched[k]  = NULL;
     m_null_static[k] = NULL;
    
     // initialize the instruction window
    m_window_size[k]   = window_size;
    m_window_limit[k]  = window_size + 20;
    m_window_utilization[k] = 0;
    m_head[k]          = m_window_limit[k] - 1;
    m_last_analyzed[k] = 0;
    m_tail[k]          = m_window_limit[k] - 1;
    m_window[k] = (flow_inst_t **) malloc( sizeof(flow_inst_t *) * m_window_limit[k] );
    for (uint32 i = 0; i < m_window_limit[k]; i++ ) {
      m_window[k][i] = NULL;
    }

    m_memory_depth[k]     = -1;
    m_instructions_per_miss[k] = 0;
    m_max_memory_depth[k] = -1;
    m_control_flow_depth[k] = -1;
    m_window_flow_depth[k] = -1;
    m_got_deeper[k]       = 0;
    m_got_shallower[k]    = 0;
    m_max_branch_count[k] = 0;
    m_marked_counted[k]   = 0;
    m_deleted_misses[k]   = 0;
    m_repeated_misses[k]  = 0;
    m_repeated_accesses[k]  = 0;
    m_invalid_load_address[k] = 0;
    m_invalid_store_address[k] = 0;
   }   //end for all logical procs

  char *hist_name;
  hist_name = (char *) malloc(30 * sizeof(char));
  sprintf( hist_name, "RelativeDepth %d", m_id );
  m_hist_depth = new histogram_t( hist_name, 4096 );
  hist_name = (char *) malloc(30 * sizeof(char));
  sprintf( hist_name, "MaxRelDepth %d", m_id );
  m_max_hist_depth = new histogram_t( hist_name, 4096 );

  hist_name = (char *) malloc(30 * sizeof(char));
  sprintf( hist_name, "NonCritical %d", m_id );
  m_hist_pc_non_critical = new histogram_t( hist_name, 16384 );
  hist_name = (char *) malloc(30 * sizeof(char));
  sprintf( hist_name, "Critical %d", m_id );
  m_hist_pc_critical = new histogram_t( hist_name, 16384 );

#if 0
  for (int32 i = 0; i < CHAIN_MAX_DEPTH; i++) {
    hist_name = (char *) malloc(30 * sizeof(char));
    sprintf( hist_name, "ChainDepth %d", m_id );
    m_hist_depth[i] = new histogram_t( hist_name, 4096 );
  }
#endif

  hist_name = (char *) malloc(20 * sizeof(char));
  sprintf( hist_name, "Dep %d", m_id );
  m_hist_dependent = new histogram_t( hist_name, 4096 );
  hist_name = (char *) malloc(20 * sizeof(char));
  sprintf( hist_name, "DepMiss %d", m_id );
  m_hist_dependent_miss = new histogram_t( hist_name, 4096 );
  hist_name = (char *) malloc(20 * sizeof(char));
  sprintf( hist_name, "Indep %d", m_id );
  m_hist_independent = new histogram_t( hist_name, 4096 ); 
  hist_name = (char *) malloc(20 * sizeof(char));
  sprintf( hist_name, "IndepMiss %d", m_id );
  m_hist_independent_miss = new histogram_t( hist_name, 4096 );

  hist_name = (char *) malloc(20 * sizeof(char));
  sprintf( hist_name, "IperM %d", m_id );
  m_hist_inst_per_misses = new histogram_t( hist_name, 4096 );

  hist_name = (char *) malloc(20 * sizeof(char));
  sprintf( hist_name, "DepCTI %d", m_id );
  m_hist_dep_cti_dist = new histogram_t( hist_name, 4096 );
  hist_name = (char *) malloc(20 * sizeof(char));
  sprintf( hist_name, "DepBranch %d", m_id );
  m_hist_dep_branch_dist = new histogram_t( hist_name, 4096 );
  hist_name = (char *) malloc(20 * sizeof(char));
  sprintf( hist_name, "IndCTI %d", m_id );
  m_hist_ind_cti_dist = new histogram_t( hist_name, 4096 );
  hist_name = (char *) malloc(20 * sizeof(char));
  sprintf( hist_name, "IndBranch %d", m_id );
  m_hist_ind_branch_dist = new histogram_t( hist_name, 4096 );

  hist_name = (char *) malloc(20 * sizeof(char));
  sprintf( hist_name, "Producers %d", m_id );
  m_hist_cpus_read_from = new histogram_t( hist_name, 4096 );
  
  hist_name = (char *) malloc(20 * sizeof(char));
  sprintf( hist_name, "Overwritten %d", m_id );
  m_hist_cpus_overwritten = new histogram_t( hist_name, 4096 );


  hist_name = (char *) malloc(20 * sizeof(char));
  sprintf( hist_name, "StaticPred %d", m_id );
  m_hist_static_pred = new histogram_t( hist_name, 4096 );
  hist_name = (char *) malloc(20 * sizeof(char));
  sprintf( hist_name, "StaticSucc %d", m_id );
  m_hist_static_succ = new histogram_t( hist_name, 4096 );
  hist_name = (char *) malloc(20 * sizeof(char));
  sprintf( hist_name, "StaticMiss %d", m_id );
  m_hist_static_miss_count = new histogram_t( hist_name, 4096 );
}

//**************************************************************************
void chain_t::allocate( void )
{
  //should be called ONCE for every SMT chip!!

  //since m_id is the actual Simics processor number,
  //,           we need to divide to get original index of the m_seq and m_state we want 
  int32 seq_num = m_id / CONFIG_LOGICAL_PER_PHY_PROC;
  ASSERT(seq_num < system_t::inst->m_numSMTProcs);
  
  m_pseq   = system_t::inst->m_seq[seq_num];
  m_pstate = system_t::inst->m_state[seq_num];

  for(uint k=0; k <  CONFIG_LOGICAL_PER_PHY_PROC; ++k){
    // make a predecessor node for "all prior instructions"
    // this involves "faking up" a static instruction
    // make the logical proc default to k:
    m_null_static[k] = new static_inst_t( ~(my_addr_t)0, STATIC_INSTR_MOP );
    m_null_static[k]->incrementRefCount();

    // 1/16/03 CM "fake" null_dep should get elimiated as flow inst evolves
    m_mem_dep[k] = new mem_dependence_t();

    // the "constant predecessor" is a senteniel value.
    m_constant_pred[k] = new flow_inst_t( m_null_static[k], m_pseq,
                                     NULL, m_mem_dep[k], k );  
    // give the ol' constant predecessor some breathing room
    for (int i = 0; i < 100; i++) {
      m_constant_pred[k]->incrementRefCount();
    }
  }  // end for all logical procs

  // allocate the register files for dependence analysis
  // (changed function call, so now need loop)
  for(uint k=0; k <  CONFIG_LOGICAL_PER_PHY_PROC; ++k){
    m_pseq->allocateFlatRegBox( m_state[k], k );
  }
  
  // clear all dependence information
  // call this EACH LOGICAL PROC bc we have different sets of Integer, Global, etc for
  //            each logical proc
 for(uint k=0; k <  CONFIG_LOGICAL_PER_PHY_PROC; ++k){
    m_pseq->clearFlatRegDeps( m_state[k], m_constant_pred[k], k );

    m_inst_count[k]   = 0;
    m_mem_count[k]    = 0;
    m_except_count[k] = 0;
    m_reissue_count[k]= 0;

    m_non_cpu_ini_count[k] = 0;
    m_not_memory_count[k] = 0;
    m_not_stallable_count[k] = 0;
    m_st_ld_forward_count[k] = 0;
    m_mem_miss_count[k] = 0;
    m_mem_unusual_miss_count[k] = 0;
    m_consumable_mem_count[k] = 0;
 }  //end for all logical procs

  m_is_init = true;
}

//**************************************************************************
chain_t::~chain_t( )
{
    for(uint k=0; k <  CONFIG_LOGICAL_PER_PHY_PROC; ++k){
    if(m_imap[k])
      delete m_imap[k];
    if(m_last_fetched && m_last_fetched[k])
     delete m_last_fetched[k];
    if(m_null_static[k])
      delete m_null_static[k];
    if(m_constant_pred[k])
      delete m_constant_pred[k];
   }
    
    if(m_null_static)
      delete [] m_null_static;
    if(m_constant_pred)
      delete [] m_constant_pred;
     if(m_imap)
      delete [] m_imap;
    if(m_last_fetched)
      delete [] m_last_fetched;
 
    if(m_state)
      delete [] m_state;
  // should delete register files allocated in m_state as well
}

//**************************************************************************
void chain_t::startTimer( unsigned int proc )
{
  ASSERT(proc < CONFIG_LOGICAL_PER_PHY_PROC);

  // start the timer
  m_overall_timer[proc].startTimer();
}

//**************************************************************************
static bool static_inst_callback( void *chain_ptr, static_inst_t *s )
{
  // statistics accumulating function for static instructions
  chain_t *chain_p = (chain_t *) chain_ptr;
  static_stat_t *stat = s->getStats();
  
#if 0
  int32 count = stat->predecessorCount();
  chain_p->m_hist_static_pred->touch( count, 1 );
  
  count = stat->successorCount();
  chain_p->m_hist_static_succ->touch( count, 1 );
#endif

  if (stat->m_miss_count > 0) {
    chain_p->m_hist_pc_non_critical->touch( stat->m_physical_pc,
                                            stat->m_miss_count - stat->m_miss_critical_count );
    chain_p->m_hist_pc_critical->touch( stat->m_physical_pc,
                                        stat->m_miss_critical_count );
  }
  
  //chain_p->m_hist_static_miss_count->touch( stat->m_miss_count, 1 );
  return false;  // to continue traversing
}

//**************************************************************************
void chain_t::printStats( void )
{
  // stop the timer
 for(uint k=0; k <  CONFIG_LOGICAL_PER_PHY_PROC; ++k){
    m_overall_timer[k].stopTimer();
  
    int64   sec_expired, usec_expired;
    m_overall_timer[k].getElapsedTime( &sec_expired, &usec_expired );
    m_pseq->out_info("\n  %-50.50s %lu sec %lu usec\n", "Total Elapsed Time [logical proc %d]:",
             sec_expired, usec_expired, k );
  
    double dsec = (double) sec_expired + ((double) usec_expired / (double) 1000000);
    m_pseq->out_info("  %-50.50s %10u\n", "Total number of instructions",
                     s_num_instructions );
    m_pseq->out_info("  %-50.50s %10u\n", "This processor\'s instructions [logical proc %d]",
                    m_inst_count[k], k );
    m_pseq->out_info("  %-50.50s %g\n", "Approximate instructions per sec [Total over logical proc %d]:",
                     (double) s_num_instructions / dsec, k );
  
    m_pseq->out_info("[%d] inst   count [logical proc %d]: %lld\n", m_id, k, m_inst_count );
    m_pseq->out_info("[%d] memory count [logical proc %d]: %lld\n", m_id, k, m_mem_count ); 
    m_pseq->out_info("[%d] repeated accesses [logical proc %d]: %lld\n", m_id, k, 
                     m_repeated_accesses );
    m_pseq->out_info("[%d] repeated misses [logical proc %d]: %lld\n", m_id, k, 
                     m_repeated_misses );  
    m_pseq->out_info("\n");
    m_pseq->out_info("[%d] consumable mem [logical proc %d]: %lld\n", m_id, k, 
                     m_consumable_mem_count );
    m_pseq->out_info("[%d] memory misses [logical proc %d]: %lld\n", m_id, k, 
                     m_mem_miss_count );
    m_pseq->out_info("[%d] unusual misses [logical proc %d]: %lld\n", m_id, k,
                     m_mem_unusual_miss_count );
    m_pseq->out_info("[%d] marked counted [logical proc %d]: %lld\n", m_id, k,
                     m_marked_counted );
    m_pseq->out_info("[%d] deleted misses [logical proc %d]: %lld\n", m_id, k,
                     m_deleted_misses );
    m_pseq->out_info("\n");

    m_pseq->out_info("[%d] invalid_load_address [logical proc %d]: %lld\n", m_id, k,
                     m_invalid_load_address );
    m_pseq->out_info("[%d] invalid_store_address [logical proc %d]: %lld\n", m_id, k, 
                     m_invalid_store_address );
    m_pseq->out_info("[%d] except count [logical proc %d]: %lld\n", m_id, k, m_except_count ); 
    m_pseq->out_info("[%d] reissu count [logical proc %d]: %lld\n", m_id, k, m_reissue_count ); 
    m_pseq->out_info("[%d] non-cpu ini count [logical proc %d]: %lld\n", m_id, k,
                     m_non_cpu_ini_count );
    m_pseq->out_info("[%d] not memory  count [logical proc %d]: %lld\n", m_id, k,
                     m_not_memory_count );
    m_pseq->out_info("[%d] not stallable [logical proc %d]: %lld\n", m_id, k,
                     m_not_stallable_count );
    m_pseq->out_info("[%d] forwards st->ld [logical proc %d]: %lld\n", m_id, k,
                     m_st_ld_forward_count);
    m_pseq->out_info("[%d] memory depth [logical proc %d]: %lld\n", m_id, k,
                     m_memory_depth );
    m_pseq->out_info("[%d] max memory depth [logical proc %d]: %lld\n", m_id, k,
                     m_max_memory_depth );
    m_pseq->out_info("[%d] control flow depth [logical proc %d]: %lld\n", m_id, k,
                     m_control_flow_depth );
    m_pseq->out_info("[%d] memory deepens [logical proc %d]: %lld\n", m_id, k,
                     m_got_deeper);
    m_pseq->out_info("[%d] memory shallows [logical proc %d]: %lld\n", m_id, k,
                     m_got_shallower);
 }  // end for all logical procs

    m_pseq->out_info("[%d] CHAIN_IDEAL_BRANCH_PRED: %d\n", m_id,
                     CHAIN_IDEAL_BRANCH_PRED);
    m_pseq->out_info("[%d] CHAIN_ST_LD_FORWARDING: %d\n", m_id,
                     CHAIN_ST_LD_FORWARDING);
  
  m_hist_inst_per_misses->printDistribution( m_pseq, 0 );
  //m_hist_depth->printDistribution( m_pseq, 0 );
  //m_max_hist_depth->printDistribution( m_pseq, 0 );
  m_hist_cpus_read_from->printDistribution( m_pseq, 0 );
  m_hist_cpus_overwritten->printDistribution( m_pseq, 0 );

  // walk the instruction page map
  for(uint k=0; k <  CONFIG_LOGICAL_PER_PHY_PROC; ++k){
    m_imap[k]->walkMap( (void *) this, static_inst_callback );
  }

  m_hist_pc_non_critical->print( m_pseq );
  m_hist_pc_critical->print( m_pseq );

#if 0
  // walk the instruction page map, building a histogram dependent usages
  m_imap->walkMap( (void *) this, static_inst_callback );
  m_hist_static_pred->printDistribution( m_pseq, 0 );
  m_hist_static_succ->printDistribution( m_pseq, 0 );
  //m_hist_static_miss_count->printDistribution( m_pseq, 0 );
  
  m_hist_dependent->printDistribution(m_pseq, 0);
  m_hist_dependent_miss->printDistribution(m_pseq, 0);
  m_hist_independent->printDistribution(m_pseq, 0);
  m_hist_independent_miss->printDistribution(m_pseq, 0);

  m_hist_dep_cti_dist->printDistribution(m_pseq, 0);
  m_hist_dep_branch_dist->printDistribution(m_pseq, 0);
  m_hist_ind_cti_dist->printDistribution(m_pseq, 0);
  m_hist_ind_branch_dist->printDistribution(m_pseq, 0);
#endif
}

//**************************************************************************
void chain_t::printMemoryStats( void )
{
  m_pseq->out_info("chain memory stats %d\n", m_id );
  for(uint k=0; k <  CONFIG_LOGICAL_PER_PHY_PROC; ++k){
    m_pseq->out_info("[logical proc %d]\n",k);
    m_imap[k]->printMemoryStats( m_pseq );
  }
}    

/*------------------------------------------------------------------------*/
/* Public methods                                                         */
/*------------------------------------------------------------------------*/

//**************************************************************************
void chain_t::inst_arrival( pt_inst_t *inst, unsigned int proc )
{
  //we need to compute the logical processor number from the Simics processor number (m_id), bc
  //  this is needed by m_state:
  ASSERT(proc < CONFIG_LOGICAL_PER_PHY_PROC);

  static_inst_t *s_instr;

  // check to see if this instruction is "re-issued"
  // (e.g. if it has the same PC & bits, we don't count it
  bool found = m_imap[proc]->queryInstr( inst->m_physical_addr, s_instr );

  flow_inst_t *head = m_window[proc][m_head[proc]];
  if (found && head != NULL) {
    // wait? issuing the same static instruction on back to back cycles?
    //       I don't think so. Just count how often this occurs.
    if (head->getStaticInst() == s_instr) {
      m_reissue_count[proc]++;
      return;
    }
  }
  m_inst_count[proc]++;

#if 0
  printf("\t%d\tid:%d inst: 0x%0x\n", s_num_instructions,
         inst->m_pid, inst->m_inst);
#endif
  if (found) {
    // check that the instruction hasn't changed.
    if ( s_instr->getInst() != inst->m_inst ) {
      m_imap[proc]->insertInstr( inst->m_physical_addr, inst->m_inst,
                                 s_instr );             
      s_instr->getStats()->m_physical_pc = inst->m_physical_addr;
    }
  } else {
    // insert this instruction into the physical page map
    m_imap[proc]->insertInstr( inst->m_physical_addr, inst->m_inst,
                               s_instr );          
    s_instr->getStats()->m_physical_pc = inst->m_physical_addr;
  }
  
  //
  // Task: Insert the instruction in the window
  //
  m_state[proc].at[proc].pc  = m_pstate->getControl( CONTROL_PC, proc );
  m_state[proc].at[proc].npc = m_pstate->getControl( CONTROL_NPC, proc );
  m_state[proc].at[proc].tl  = m_pstate->getControl( CONTROL_TL, proc );
  m_state[proc].at[proc].pstate = m_pstate->getControl( CONTROL_PSTATE, proc );
  m_state[proc].at[proc].gset = pstate_t::getGlobalSet( m_state[proc].at[proc].pstate);
  m_state[proc].at[proc].cwp = m_pstate->getControl( CONTROL_CWP, proc );
  m_state[proc].at[proc].asi = m_pstate->getControl( CONTROL_ASI, proc );

  flow_inst_t *next_fetch = new flow_inst_t( s_instr, m_pseq,
                                             &m_state[proc], m_mem_dep[proc], proc  );  
  next_fetch->setID( m_inst_count[proc] );
  
  // insert the last fetched instruction into the iwindow
  if (m_last_fetched[proc] != NULL) {
    m_head[proc] = chain_increment( m_head[proc], proc );
    ASSERT( m_window[proc][m_head[proc]] == NULL );
    m_window[proc][m_head[proc]] = m_last_fetched[proc];

    // increment count of instructions in window
    m_window_utilization[proc]++;

    // do the dependence analysis for this most recent instruction
    analyzeDependence(proc);
  }
  m_last_fetched[proc] = next_fetch;

  // don't analyze until the window gets full
  if (m_window_utilization[proc] < m_window_size[proc] + 1) {
    return;
  }

  // if the tail of the window is a memory miss, then analyze the window
  m_tail[proc] = chain_increment( m_tail[proc], proc );
  flow_inst_t *last_flow_inst = m_window[proc][m_tail[proc]];
  if ( CHAIN_IDEAL_WINDOW ) {
    // only analyze the window if the depth increases
    if ( last_flow_inst ) {
      analyzeWindow(proc);
    }
  } else {
    if ( last_flow_inst->getEvent( FLOW_MEMORY_MISS ) &&
         !(last_flow_inst->getEvent( FLOW_IS_COUNTED )) ) {
      analyzeWindow(proc);
    }
  }

  // pull out the last instruction from the instruction window
  if ( last_flow_inst->getEvent( FLOW_MEMORY_MISS ) ) {
    m_deleted_misses[proc]++;
  }
  delete_flow_inst( last_flow_inst );
  m_window[proc][m_tail[proc]] = NULL;
  m_window_utilization[proc]--;
  
  // print out the instruction
}

//**************************************************************************
void chain_t::mem_arrival( pt_memory_t *inst, unsigned int proc )
{
  ASSERT(proc < CONFIG_LOGICAL_PER_PHY_PROC);

  m_mem_count[proc]++;
  
  // discard instructions initiated by non-cpu entities
  if (inst->getFlag( PTMEM_NON_CPU_INI )) {
    m_non_cpu_ini_count[proc]++;
    return;
    // DEBUG_OUT("chain_t: mem_arrival: non-CPU initator: pid: %d addr: 0x%0llx\n", inst->m_pid, inst->m_physical_addr);
  }

  // discard instructions from the I/O space
  if (inst->getFlag( PTMEM_IS_IO )) {
    m_not_memory_count[proc]++;
    return;
    // DEBUG_OUT("chain_t: mem_arrival: I/O instruction: pid: %d\n", inst->m_pid);
  }
  
  // discard non-stallable memory operations
  if (inst->getFlag( PTMEM_NOT_STALLABLE )) {
    m_not_stallable_count[proc]++;
    return;
    // DEBUG_OUT("chain_t: mem_arrival: non-CPU initator: pid: %d addr: 0x%0llx\n", inst->m_pid, inst->m_physical_addr);
  }

  // look at the most recently fetched instruction
  flow_inst_t *flow_inst  = m_last_fetched[proc];
  if (flow_inst->getEvent( FLOW_SEEN_ACCESS )) {
    // already seen an access for this instruction.
    // count it & move on.
    m_repeated_accesses[proc]++;
    if (inst->m_stall_time > CHAIN_MEMORY_LATENCY) {
      // I've seen this on casa and casxa instructions
      // as well as ldda instructions... These make sense as you are
      // loading more than one block. It doesn't make sense for casa instrs.

      //This is a really weird error: it means you missed & missed again...
      //DEBUG_OUT("  repeated memory miss: 0x%0x\n", s_inst->getInst());
      m_repeated_misses[proc]++;
    }
    return;
  } else {
    flow_inst->markEvent( FLOW_SEEN_ACCESS );
    flow_inst->setPhysicalAddress( inst->m_physical_addr );
  }

  m_consumable_mem_count[proc]++;
  static_inst_t *s_inst   = flow_inst->getStaticInst();
  // if this instruction caused a miss to memory... mark this instruction
  // is this a memory operation
  if ((s_inst->getType() == DYN_LOAD) ||
      (s_inst->getType() == DYN_STORE) ||
      (s_inst->getType() == DYN_ATOMIC) ||
      (s_inst->getType() == DYN_PREFETCH)) {

    // set the physical address
    if (inst->m_stall_time > CHAIN_MEMORY_LATENCY) {
      if (!flow_inst->getEvent( FLOW_MEMORY_MISS )) {
        flow_inst->markEvent( FLOW_MEMORY_MISS );
        flow_inst->unmarkEvent( FLOW_IS_COUNTED );
        s_inst->getStats()->m_miss_count++;
        m_mem_miss_count[proc]++;
      } else {
        // should never happen at this point
        ASSERT(0);
      }
    }
  } else {
    if (s_inst->getOpcode() == i_flush) {
      // ignore flush instructions... they are fine
    } else {
      if (!flow_inst->getEvent( FLOW_MEMORY_MISS )) {
        flow_inst->markEvent( FLOW_MEMORY_MISS );
        flow_inst->unmarkEvent( FLOW_IS_COUNTED );
        s_inst->getStats()->m_miss_count++;
        m_mem_unusual_miss_count[proc]++;
      }

#if 0
      // An interrupt or exception occured, causing a change in window head
      DEBUG_OUT("warning: head of window not load or store\n");
      inst->print();
      DEBUG_OUT("[%d] 0x%0llx 0x%0x\n",
                m_id, inst->m_physical_addr, s_inst->getInst());
      
      // read the current PC, instruction, if possible
      uint64 pc = m_pstate->getControl( CONTROL_PC );
      int32  tl = m_pstate->getControl( CONTROL_TL );
      int32  pstate = m_pstate->getControl( CONTROL_PSTATE );
      pa_t   ppc;
      uint32 instr = 0;
      
      bool found = m_pseq->getInstr( pc, tl, pstate, &ppc, &instr );
      DEBUG_OUT("cachedcopy:%d PC: 0x%0llx tl: 0x%0x pstate: 0x%0x i: 0x%0x\n",
                found, pc, tl, pstate, instr );
      
      m_pstate->askSimics( 0, ppc, pc );
      
      pa_t last_fetch_ppc = system_t::inst->m_chain[m_id/CONFIG_LOGICAL_PER_PHY_PROC]->m_most_recent_fetch;
      m_pstate->askSimics( 0, last_fetch_ppc, 0 );
#endif
#if 0
      SIM_frontend_exception( SimExc_Break,
                              "chain: mem_arrival: not memory instruction\n" );
      DEBUG_OUT("warning: excepting instruction: e:0x%x on mem_arrival 0x%0x\n",
                flow_inst->getTrapType(), s_inst->getInst());
#endif
    }
  }
}

//**************************************************************************
void chain_t::postException( uint32 exception, unsigned int proc )
{
  ASSERT(proc < CONFIG_LOGICAL_PER_PHY_PROC);

  m_except_count[proc]++;

  // look at the front instruction in the window
  flow_inst_t *flow_inst  = m_last_fetched[proc];
  flow_inst->markEvent( FLOW_EXCEPTION );
  flow_inst->setTrapType( (trap_type_t) exception );
}

//**************************************************************************
void chain_t::delete_flow_inst( flow_inst_t *inst )
{
  inst->decrementRefCount();
  if (inst->getRefCount() == 0) {
    delete inst;
  }
}

//**************************************************************************
void chain_t::analyzeDependence( unsigned int proc )
{
  flow_inst_t   *flow_inst   = m_window[proc][m_head[proc]];
  static_inst_t *this_s      = flow_inst->getStaticInst();
  bool           is_constant = true;
  int32          max_mem_depth = 0;

  // for each register this instruction reads ...
  for (int sr = 0; sr < SI_MAX_SOURCE; sr++) {
    reg_id_t &id = flow_inst->getSourceReg(sr);
    if (!id.isZero()) {
      // get the dependence on this register (if any)
      flow_inst_t *in_p = id.getARF()->getDependence( id, proc );       
      if (in_p == m_constant_pred[proc]) {
        // this predecessor is always a constant (no dependence to track)
        
      } else if ( (id.getRtype() == RID_CONTROL) &&
                  (id.getVanilla() >= CONTROL_NUM_CONTROL_PHYS) ) {
        // no predecessor here either... not implemented
        
      } else if (in_p == NULL) {
        DEBUG_OUT("warning: analyze window: NULL dependence... 0x%x\n",
                  this_s->getInst() );
        id.print();  
      } else {
        // read the predecessors memory depth
        if (in_p->getMemDepth() > max_mem_depth) {
          max_mem_depth = in_p->getMemDepth();
        }

        // Link the predecessor's with this nodes static instruction
        static_inst_t *in_s   = in_p->getStaticInst();
        in_s->getStats()->pushSuccessor( this_s );
        this_s->getStats()->pushPredecessor( in_s );
        
        // if the input instruction is non-constant, then
        // instruction does not produce a "constant" value.
        is_constant = false;
      }
    }
  }

  // load and atomic instructions have dependences on the memory value
  // (e.g. they are never constant).
  dyn_execute_type_t dyn_type = this_s->getType();
  if (dyn_type == DYN_LOAD ||
      dyn_type == DYN_ATOMIC) {
    is_constant = false;

    // look up the load / atomic in the memory map to see if it was written earlier
    if (CHAIN_ST_LD_FORWARDING) {
      int64  earlier_value = amap_search(flow_inst, proc);
      if (earlier_value > max_mem_depth) {
        max_mem_depth = earlier_value;
      }
    }
  }

  if ( !CHAIN_IDEAL_WINDOW ) {
    // if we are limiting due to window size, increment memory depth
    // based on the window depth.
    if ( m_window_flow_depth[proc] > max_mem_depth ) {
      max_mem_depth = m_window_flow_depth[proc];
    }
  }

  // if we are including the control flow instructions in the graph,
  // update the control memory depth variable
  if (CHAIN_IDEAL_BRANCH_PRED == 0) {
    if (dyn_type == DYN_CONTROL) {
      m_control_flow_depth[proc] = max_mem_depth;
    }
    
    // all instructions after a control instruction must obey its memory depth
    if ( m_control_flow_depth[proc] > max_mem_depth ) {
      max_mem_depth = m_control_flow_depth[proc];
    }
  }
  
  // if this instruction is a miss, increment its maximum depth
  if (flow_inst->getEvent( FLOW_MEMORY_MISS )) {
    max_mem_depth++;
  }
  flow_inst->setMemDepth( max_mem_depth );
  
  // write stores / atomics depth to the memory map
  if (CHAIN_ST_LD_FORWARDING) {
    if (dyn_type == DYN_STORE ||
        dyn_type == DYN_ATOMIC) {
      amap_store(flow_inst, max_mem_depth, proc );
    }
  }
  
  // for each destination register, mark it as an output
  for (int d = 0; d < SI_MAX_DEST; d++) {
    reg_id_t &id = flow_inst->getDestReg(d);
    if (!id.isZero()) {
      // if the sources are all constant registers,the result is constant too
      if (is_constant) {
        flow_inst->markEvent( FLOW_CONSTANT );
        id.getARF()->setDependence( id, m_constant_pred[proc], proc );   
      } else {
        // set the dependence on this register
        id.getARF()->setDependence( id, flow_inst, proc );       
      }
    }
  }
  m_instructions_per_miss[proc]++;
}

//**************************************************************************
void chain_t::analyzeWindow( unsigned int proc )
{
  ASSERT(proc < CONFIG_LOGICAL_PER_PHY_PROC);

  if (m_window[proc][m_tail[proc]] == NULL) {
    DEBUG_OUT("chain_t: analyzeWindow: NULL head of window.\n");
    return;
  }

  int64 newdepth = m_window[proc][m_tail[proc]]->getMemDepth();
  if (newdepth < m_memory_depth[proc]) {
    m_got_shallower[proc]++;
  } else if (newdepth > m_memory_depth[proc]) {
    m_got_deeper[proc]++;
  }
  m_memory_depth[proc] = newdepth;
  if (newdepth > m_max_memory_depth[proc]) {
    m_max_memory_depth[proc] = newdepth;
  } else {
    // if the window didn't get deeper, return
    if ( CHAIN_IDEAL_WINDOW ) {
      return;
    }
  }

  if ( !CHAIN_IDEAL_WINDOW ) {
    // The window stalled on an instruction with depth N, the next instruction
    // fetched will have a depth N + 1
    if ( m_max_memory_depth[proc] > m_window_flow_depth[proc] ) {
      m_window_flow_depth[proc] = m_memory_depth[proc];
    }
  }

  // the head of the window is a "critical miss" (increment its counter)
  m_window[proc][m_tail[proc]]->getStaticInst()->getStats()->m_miss_critical_count++;
  
  // touch the number of instructions that happened between the last miss
  m_hist_inst_per_misses->touch( m_instructions_per_miss[proc], 1 );
  m_instructions_per_miss[proc] = 0;

  int64 rel_depth = 0;
  int64 max_depth = -1;
  // walk the window, count the number of depth 0, 1, 2, 3 instructions
  for ( uint32 count = 0, index = m_tail[proc];
        count < m_window_utilization[proc];
        count++, index = chain_increment(index, proc) ) {
    flow_inst_t  *flow_inst = m_window[proc][index];

    // for each instruction in the window (that has not been counted before)
    if ( flow_inst->getEvent(FLOW_MEMORY_MISS) &&
         !flow_inst->getEvent(FLOW_IS_COUNTED) ) {
      // compute its relative depth to the head of the window
      rel_depth = flow_inst->getMemDepth() - m_memory_depth[proc];
      max_depth = MAX( max_depth, rel_depth );
      // m_hist_depth->touch( rel_depth, 1 );
      if (rel_depth <= 0) {
        m_marked_counted[proc]++;
        flow_inst->markEvent( FLOW_IS_COUNTED );
      }
    }
  }

#if 0
  // a count of the number of all CTIs between the first miss & subsequent ones
  uint32 count_ctis = 0;
  // a count of just the branches between the first miss & subsequent
  uint32 count_branches = 0;

  // walk the list gathering independent / dependent statistics
  int  independent = 0;
  int  dependent   = 0;
  int  independent_miss = 0;
  int  dependent_miss   = 0;
  for ( uint32 count = 0, index = m_tail;
        count < m_window_utilization;
        count++, index = chain_increment(index, proc) ) {
    // check if this instruction reads registers previously written
    flow_inst_t  *flow_inst = m_window[index];
    
    dyn_execute_type_t dyn_type = flow_inst->getStaticInst()->getType();
    if (dyn_type == DYN_CONTROL) {
      count_ctis++;
      branch_type_t branch_type = flow_inst->getStaticInst()->getBranchType();
      if ( (branch_type == BRANCH_UNCOND) ||
           (branch_type == BRANCH_COND) ||
           (branch_type == BRANCH_PCOND) ) {
        count_branches++;
      }
    }
    
    if (flow_inst->getEvent( FLOW_INDEPENDENT )) {
      independent++;
      if (flow_inst->getEvent( FLOW_MEMORY_MISS )) {
        independent_miss++;
        m_hist_ind_cti_dist->touch( count_ctis, 1 );
        m_hist_ind_branch_dist->touch( count_branches, 1 );
      }
    } else {
      dependent++;
      if (flow_inst->getEvent( FLOW_MEMORY_MISS )) {
        dependent_miss++;
        m_hist_dep_cti_dist->touch( count_ctis, 1 );
        m_hist_dep_branch_dist->touch( count_branches, 1 );
      }
    }
  }

  // add this data point to the collection of samples
  m_hist_dependent->touch( dependent, 1 );
  m_hist_dependent_miss->touch( dependent_miss, 1 );
  m_hist_independent->touch( independent, 1 );
  m_hist_independent_miss->touch( independent_miss, 1 );
#endif
}

//**************************************************************************
void chain_t::printWindow( unsigned int proc )
{
  ASSERT(proc < CONFIG_LOGICAL_PER_PHY_PROC);

  static uint32  sample_count = 0;

  if (random() % 100 != 0) {
    return;
  } else {
    sample_count++;
  }
  // don't print more than 10 samples
  if (sample_count >= 10) {
    return;
  }

  // start with m_tail, looking for first (real) entry
  // Adjust the sequence numbers of entries in the iwindow by this first entry
  flow_inst_t *last;
  int64        offset = -1;
  for ( uint32 count = 0, index = m_tail[proc];
        count < m_window_utilization[proc];
        count++, index = chain_increment(index, proc) ) {
    last = m_window[proc][index];
    if (last != NULL) {
      offset = last->getID();
      break;
    }
  }
  ASSERT( offset != -1 );

  // walk the instruction window, printing out the dependencies
  for ( uint32 count = 0, index = m_tail[proc];
        count < m_window_utilization[proc];
        count++, index = chain_increment(index, proc) ) {

    const char *is_mem   = "    ";
    const char *is_counted = "   ";
    const char *is_constant  = " ";
    char  str[255];
    char  depth[255];
    flow_inst_t *flow_inst = m_window[proc][index];

    if (flow_inst == NULL)
      continue;

    strcpy( depth, "" );
    if (flow_inst->getEvent( FLOW_MEMORY_MISS )) {
      is_mem = "MISS";
      int64 round_depth = flow_inst->getMemDepth() % 1000;
      sprintf( depth, "%lld", round_depth );
    }
    if (flow_inst->getEvent( FLOW_IS_COUNTED )) {
      is_counted = "X  ";
    }
    if (flow_inst->getEvent( FLOW_CONSTANT )) {
      is_constant = "C";
    }

    flow_inst->getStaticInst()->printDisassemble( str );
    DEBUG_OUT("[%3.3d] %s %3.3s %4.4s %3.3s 0x%0x %s ", count, is_constant,
              is_counted, is_mem, depth,
              flow_inst->getStaticInst()->getInst(), str );
    
    cfg_list_t *succ = flow_inst->getSuccessor();
    while (succ != NULL) {
      flow_inst_t  *node = succ->getNode();
      if (node != NULL) {
        DEBUG_OUT( "(%d) ", node->getID() - offset );
      }
      succ = succ->getNext();
    }
    DEBUG_OUT("\n");
  }
}

/*------------------------------------------------------------------------*/
/* Accessor(s) / mutator(s)                                               */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Private methods                                                        */
/*------------------------------------------------------------------------*/

//**************************************************************************
int64 chain_t::amap_search( flow_inst_t *flow_inst, unsigned int proc )
{
  // does the address map contain the address
  pa_t address = flow_inst->getPhysicalAddress();
  if (address == (pa_t) -1) {
    m_invalid_load_address[proc]++;
    return -1;
  }
  address = address & ~MEMOP_BLOCK_MASK;

  CFGIndex *alias_table = NULL;
  if ( CHAIN_MP_MODE == 1 ) {
    alias_table = &system_t::inst->m_alias_table;
  } else {
    alias_table = &m_alias_table;
  }
  if (alias_table->find( address ) != alias_table->end()) {
    // found prior store to this address
    flow_inst_t *prior_node = (*alias_table)[address];
    m_st_ld_forward_count[proc]++;

    if ( prior_node->getSequencerID() != m_id ) {
#if 0
      m_pseq->out_log("  [%d] PA 0x%0llx Other: (%d)\n",
                      m_id, address, prior_node->getSequencerID() );
#endif
      //m_hist_cpus_read_from->touch( prior_node->getSequencerID(), 1 );
    }
    return (prior_node->getMemDepth());
  }
  // not found, return sentinel minimum memory count
  return -1;
}

//**************************************************************************
void chain_t::amap_store(flow_inst_t *flow_inst, int64 depth, unsigned int proc)
{
  // does the address map contain the address
  pa_t address = flow_inst->getPhysicalAddress();
  if (address == (pa_t) -1) {
    m_invalid_store_address[proc]++;
    return;
  }
  address = address & ~MEMOP_BLOCK_MASK;

  CFGIndex *alias_table = NULL;
  if ( CHAIN_MP_MODE == 1 ) {
    alias_table = &system_t::inst->m_alias_table;
  } else {
    alias_table = &m_alias_table;
  }
  if (alias_table->find( address ) != alias_table->end()) {
    //  found prior write to this address ... free the prior node if possible
    flow_inst_t *prior_node = (*alias_table)[address];
    if (prior_node == flow_inst)
      return;

    if ( prior_node->getSequencerID() != m_id ) {
      //m_hist_cpus_overwritten->touch( prior_node->getSequencerID(), 1 );
    }
    prior_node->decrementRefCount();
    if (prior_node->getRefCount() == 0) {
      delete prior_node;
    }
  }
  flow_inst->incrementRefCount();
  (*alias_table)[address] = flow_inst;
}

/*------------------------------------------------------------------------*/
/* Static methods                                                         */
/*------------------------------------------------------------------------*/

//**************************************************************************
void chain_t::chain_inst_consumer( pt_record_t *rec )
{
  pt_inst_t     *inst = (pt_inst_t *) rec;
  int            id   = inst->m_pid;

  //the actual sequencer number
    int32 seq_num = id / CONFIG_LOGICAL_PER_PHY_PROC;
  // the logical proc number within the SMT chip 
   uint32 logical_proc_num = id % CONFIG_LOGICAL_PER_PHY_PROC;
   ASSERT(seq_num < system_t::inst->m_numSMTProcs);
   ASSERT(logical_proc_num < CONFIG_LOGICAL_PER_PHY_PROC);

  if ( seq_num < 0 || seq_num >= system_t::inst->m_numSMTProcs ) {     
    ERROR_OUT("error: chain consumer: id is invalid: id == %d\n", seq_num );
    return;
  }
  s_num_instructions++;
  system_t::inst->m_chain[seq_num]->inst_arrival( inst, logical_proc_num);
}

//**************************************************************************
void chain_t::chain_mem_consumer( pt_record_t *rec )
{
  // print out the data
  pt_memory_t   *mem = (pt_memory_t *) rec;
  int            id  = mem->m_pid;

  //the actual sequencer number
   int32 seq_num = id / CONFIG_LOGICAL_PER_PHY_PROC;
  // the logical proc number within the SMT chip 
  uint32 logical_proc_num = id % CONFIG_LOGICAL_PER_PHY_PROC;
  ASSERT(seq_num < system_t::inst->m_numSMTProcs);
  ASSERT(logical_proc_num < CONFIG_LOGICAL_PER_PHY_PROC);

  if ( seq_num < 0 || seq_num >= system_t::inst->m_numSMTProcs ) {
    ERROR_OUT("error: chain consumer: id is invalid: id == %d\n", seq_num );
    return;
  }
  system_t::inst->m_chain[seq_num]->mem_arrival( mem, logical_proc_num);
}

/*------------------------------------------------------------------------*/
/* Global functions                                                       */
/*------------------------------------------------------------------------*/

/** [Memo].
 *  [Internal Documentation]
 */
//**************************************************************************
