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
 * FileName:  pseq.C
 * Synopsis:  Implements an out-of-order sequencer
 * Author:    cmauer
 * Version:   $Id$
 */

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include "hfa.h"
#include "hfacore.h"

// branch prediction schemes
#include "gshare.h"
#include "agree.h"
#include "yags.h"
#include "igshare.h"
#include "mlpredict.h"

#include "utimer.h"
#include "lsq.h"
#include "ipagemap.h"
#include "tracefile.h"
#include "branchfile.h"
#include "memtrace.h"
#include "fileio.h"
#include "confio.h"
#include "debugio.h"
#include "checkresult.h"
#include "pipestate.h"
#include "rubycache.h"
#include "mf_api.h"
#include "histogram.h"
#include "stopwatch.h"
#include "sysstat.h"
#include "dtlb.h"
#include "writebuffer.h"
#include "power.h"
#include "storeSet.h"
#include "regstate.h"

#include "flow.h"
#include "actor.h"
#include "flatarf.h"
#include "dependence.h"
#include "ptrace.h"
#include "Vector.h"    

#include "pseq.h"

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

// AM64:  AddressMask 64-bits. This corresponds to the AM bit in the PSTATE
//        register word, that makes all PC accesses only use the lower 32-bits.
#define AM64( address )       \
  ( (int32) address & 0xffffffff )

// ISADDRESS32:  Strips the AM bit out of the PSTATE register word.
#define ISADDRESS32( PSTATE ) \
  ( ((PSTATE) & 0x8) == 0x8 )

// defines the OS page size, used for performance reasons in the ITLB mini-
// cache (8K pages in solaris == 13 bits of mask)
const la_t PSEQ_OS_PAGE_MASK = 0xffffffffffffe000ULL;

// M_PSTATE:  pstate used to be a member variable, now it is system-wide.
#define M_PSTATE \
  system_t::inst->m_state[m_id/CONFIG_LOGICAL_PER_PHY_PROC]

/// size of register use histogram fields
const uint32  PSEQ_REG_USE_HIST       = 200;

/// size of recently retired instruction buffer
///   This is like the "flight data recorder" for the simulation.
const int32   PSEQ_RECENT_RETIRE_SIZE = 10;

/// size of instructions returned for more decoding
const int32   PSEQ_HIST_DECODE        = 30;

/*------------------------------------------------------------------------*/
/* Variable declarations                                                  */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Forward declarations                                                   */
/*------------------------------------------------------------------------*/

// handles mmu context switches
static void pseq_mmu_reg_handler( void *pseq_obj, void *ptr,
                                  uint64 regindex, uint64 newcontext );

/*------------------------------------------------------------------------*/
/* Constructor(s) / destructor                                            */
/*------------------------------------------------------------------------*/

//**************************************************************************
pseq_t::pseq_t( int32 id )
  : out_intf_t( id ){

    #ifdef DEBUG_PSEQ
       ERROR_OUT("pseq_t:constructor BEGIN ID = %d\n",id);
    #endif

  m_iwin = new iwindow_t[CONFIG_LOGICAL_PER_PHY_PROC];

  //per thread Instruction Counter (used for SMT scheduling)
  m_icount = new uint32[CONFIG_LOGICAL_PER_PHY_PROC];

  for(uint k=0; k <  CONFIG_LOGICAL_PER_PHY_PROC; ++k){
    // Note: each logical processor has a IWINDOW_ROB_SIZE instruction window, but
    //     in order to simulate a shared instruction window we always keep the total number of used slots
    //      <= IWINDOW_ROB_SIZE (ie no individual logical proc can use up the entire window)
    m_iwin[k].set(IWINDOW_ROB_SIZE+1, IWINDOW_WIN_SIZE, k);
    m_icount[k] = 0;
  }

  ASSERT(CONFIG_FETCH_THREADS_PER_CYCLE > 0);
  // indicates how many threads we should fetch from in each cycle:
  m_threads_per_cycle = CONFIG_FETCH_THREADS_PER_CYCLE;       
  m_lsq                 = NULL;
  m_write_buffer = NULL;
  m_bpred               = NULL;
  m_ipred               = NULL;
  m_spec_bpred          = NULL;
  m_arch_bpred          = NULL;
  m_ras                 = NULL;
  m_tlstack             = NULL;
  m_conf                = NULL;
  m_imap                = NULL;
  m_tracefp             = NULL;
  m_branch_trace        = NULL;
  m_memtracefp          = NULL;
  m_control_rf          = NULL;
  m_control_arf         = NULL;
  l2_mshr               = NULL;
  l2_cache              = NULL;
  il1_mshr              = NULL;
  dl1_mshr              = NULL;
  l1_inst_cache         = NULL;
  l1_data_cache         = NULL;
  m_ruby_cache          = NULL;
  
 /* WATTCH power */
  m_power_stats = NULL;
  if(WATTCH_POWER){
    m_power_stats = new power_t(this, id);

    //calculate static power:
    m_power_stats->calculate_power();
  }

  // StoreSet memory dependence predictor
  m_store_set_predictor = new storeset_predictor_t( this );

  // Regstate predictor (for BRANCH_PRIV)
  m_regstate_predictor = new regstate_predictor_t( this );

  m_proc_waiter = NULL;
  m_local_sequence_number = NULL;
  m_local_inorder_number = NULL;
  m_local_icount = NULL;
  m_mshr = NULL;
  m_scheduler = NULL;
  m_inorder_at = NULL;
  m_fetch_at = NULL;
  m_cc_rf = NULL;
  m_cc_retire_map = NULL;
  m_control_rf = NULL;
  m_control_arf = NULL;
  m_recent_retire_index = NULL;
  m_recent_retire_instr = NULL;
  m_mmu_access = NULL;
  m_mmu_asi = NULL;
  m_primary_ctx = NULL;
  m_ctxt_id = NULL;
  m_itlb_physical_address = NULL;
  m_itlb_logical_address = NULL;
  m_standalone_tlb = NULL;
  m_thread_physical_address = NULL;
  m_ideal_retire_count = NULL;
  m_ideal_last_checked = NULL;
  m_ideal_last_freed = NULL;
  m_ideal_first_predictable = NULL;
  m_ideal_last_predictable = NULL;
  m_ideal_control_rf = NULL;
  m_ideal_opstat = NULL;
  m_ideal_status = NULL;
  m_cfg_index = NULL;
  m_inorder = NULL;
  m_mem_deps = NULL;

  // These are dynamic upper limits:
  m_max_fetch_per_cycle = MAX_FETCH/CONFIG_LOGICAL_PER_PHY_PROC;
  m_max_decode_per_cycle = MAX_DECODE/CONFIG_LOGICAL_PER_PHY_PROC;
  m_max_dispatch_per_cycle = MAX_DISPATCH/CONFIG_LOGICAL_PER_PHY_PROC;
  m_max_retire_per_cycle = MAX_RETIRE/CONFIG_LOGICAL_PER_PHY_PROC;

  if(m_max_fetch_per_cycle <= 0){
    m_max_fetch_per_cycle = 1;
  }
  if(m_max_decode_per_cycle <= 0){
    m_max_decode_per_cycle = 1;
   }
  if(m_max_dispatch_per_cycle <= 0){
    m_max_dispatch_per_cycle = 1;
  }
  if(m_max_retire_per_cycle <= 0){
    m_max_retire_per_cycle = 1;
  }
  /*
   * core initialization
   */
  m_id     = id;
  m_local_cycles = 0;

  m_proc_waiter = new proc_waiter_t* [CONFIG_LOGICAL_PER_PHY_PROC];

  m_local_inorder_number = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_local_sequence_number =  new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_local_icount =  new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  for(uint k=0; k <  CONFIG_LOGICAL_PER_PHY_PROC; ++k){
    m_local_inorder_number[k] = m_local_sequence_number[k] = m_local_icount[k] = 0;
    // Each logical processor's waiter points to the current sequencer object 
    m_proc_waiter[k] = new proc_waiter_t(this, k);
  }

 
  // Create separate lsqs, one for each logical processor
  m_lsq = new lsq_t * [CONFIG_LOGICAL_PER_PHY_PROC];
  m_ctxt_id = new ireg_t [CONFIG_LOGICAL_PER_PHY_PROC];

  for(uint k=0; k <  CONFIG_LOGICAL_PER_PHY_PROC; ++k){
    m_lsq[k]    = new lsq_t( this );
    m_ctxt_id[k] = 0;
  }

  branch_predictor_type_t  predictor_type;
  if ( !strcmp( BRANCHPRED_TYPE, "GSHARE" ) ) {
    predictor_type = BRANCHPRED_GSHARE;
  } else if ( !strcmp( BRANCHPRED_TYPE, "AGREE" ) ) {
    predictor_type = BRANCHPRED_AGREE;
  } else if ( !strcmp( BRANCHPRED_TYPE, "YAGS" ) ) {
    predictor_type = BRANCHPRED_YAGS;
  } else if ( !strcmp( BRANCHPRED_TYPE, "IGSHARE" ) ) {
    predictor_type = BRANCHPRED_IGSHARE;
  } else if ( !strcmp( BRANCHPRED_TYPE, "MLPREDICT" ) ) {
    predictor_type = BRANCHPRED_MLPRED;
  } else if ( !strcmp( BRANCHPRED_TYPE, "EXTREME" ) ) {
    predictor_type = BRANCHPRED_EXTREME;
  } else {
    DEBUG_OUT("Unknown branch predictor type: %s\n", BRANCHPRED_TYPE );
    DEBUG_OUT("   Defaulting to YAGS branch predictor\n");
    predictor_type = BRANCHPRED_YAGS;
  }

  switch (predictor_type) {
  case BRANCHPRED_GSHARE:
    m_bpred = new gshare_t( BRANCHPRED_PHT_BITS );
    break;
  case BRANCHPRED_AGREE:
    m_bpred = new agree_t( BRANCHPRED_PHT_BITS );
    break;
  case BRANCHPRED_YAGS:
    m_bpred = new yags_t( BRANCHPRED_PHT_BITS, BRANCHPRED_EXCEPTION_BITS,
                          BRANCHPRED_TAG_BITS );
    break;
  case BRANCHPRED_IGSHARE:
    m_bpred = new igshare_t( BRANCHPRED_PHT_BITS, BRANCHPRED_USE_GLOBAL );
    break;
  case BRANCHPRED_MLPRED:
    m_bpred = new mlpredict_t( BRANCHPRED_PHT_BITS, 32, 32 );
    ((mlpredict_t *) m_bpred)->readTags( NULL );
    break;
  case BRANCHPRED_EXTREME:
    SIM_HALT;
    break;
  default:
    ERROR_OUT("Unknown branch predictor type: %d\n", BRANCHPRED_TYPE);
    SIM_HALT;
  }
  m_ipred = new cascaded_indirect_t();

  m_spec_bpred = new predictor_state_t();
  m_arch_bpred = new predictor_state_t();
  memset( (void *) m_spec_bpred, 0, sizeof(predictor_state_t) );
  memset( (void *) m_arch_bpred, 0, sizeof(predictor_state_t) );

  m_ras = new ras_t * [CONFIG_LOGICAL_PER_PHY_PROC];
  m_tlstack = new tlstack_t * [CONFIG_LOGICAL_PER_PHY_PROC];
  for(uint k=0; k <  CONFIG_LOGICAL_PER_PHY_PROC; ++k){
    m_ras[k] = new ras_t( this, (1 << RAS_BITS), RAS_EXCEPTION_TABLE_BITS );
    m_tlstack[k] = new tlstack_t( (MAXTL + 2) );
  }

  m_bpred->InitializeState( &m_arch_bpred->cond_state );
  m_ipred->InitializeState( &m_arch_bpred->indirect_state );

  // For Architectural state
  for(uint k=0; k <  CONFIG_LOGICAL_PER_PHY_PROC; ++k){
    m_ras[k]->InitializeState( &m_arch_bpred->ras_state );
  }

  m_bpred->InitializeState( &m_spec_bpred->cond_state );
  m_ipred->InitializeState( &m_spec_bpred->indirect_state );

  // For speculative state
  for(uint k=0; k <  CONFIG_LOGICAL_PER_PHY_PROC; ++k){
    m_ras[k]->InitializeState( &m_spec_bpred->ras_state );
  }

  ASSERT( m_spec_bpred->ras_state.TOS      == 0 );
  ASSERT( m_spec_bpred->ras_state.next_free == 1 );

  m_conf  = new confio_t();

  //have to allocate memory for the instruction maps used by each logical proc
  m_imap = new ipagemap_t * [CONFIG_LOGICAL_PER_PHY_PROC];
  m_tracefp = new tracefile_t * [CONFIG_LOGICAL_PER_PHY_PROC];
  m_branch_trace = new branchfile_t * [CONFIG_LOGICAL_PER_PHY_PROC];
  m_memtracefp = new memtrace_t * [CONFIG_LOGICAL_PER_PHY_PROC];

  m_ideal_status = new pseq_fetch_status_t[CONFIG_LOGICAL_PER_PHY_PROC];
  m_cfg_index = new CFGIndex[CONFIG_LOGICAL_PER_PHY_PROC];

  for(uint k=0; k <  CONFIG_LOGICAL_PER_PHY_PROC; ++k){
    m_imap[k]  = new ipagemap_t(PSEQ_IPAGE_TABLESIZE);
    //Be sure to set these trace pointers to NULL:
    m_tracefp[k] = NULL;
    m_branch_trace[k] = NULL;
    m_memtracefp[k] = NULL;
  }

  // initialize scheduler
  m_scheduler = new scheduler_t( this );

  /*
   * register file initialization
   */
  // Each logical processor should have its own m_fetch_at and m_inorder_at
  //          these definitions were moved from allocateRegBox():
  m_fetch_at = new abstract_pc_t * [CONFIG_LOGICAL_PER_PHY_PROC];
  m_inorder_at = new abstract_pc_t *[CONFIG_LOGICAL_PER_PHY_PROC];
  m_control_rf = (physical_file_t ***) malloc(sizeof(physical_file_t **) * CONFIG_LOGICAL_PER_PHY_PROC);

  allocateRegBox( m_ooo );
 
  /*
   * cache initialization
   */
  if (!CONFIG_WITH_RUBY) {
    /* Second level mshr, backed up by main memory */
    l2_mshr = new mshr_t("L2.mshr", /* memory */ NULL, m_scheduler,
                         L2_BLOCK_BITS, MEMORY_DRAM_LATENCY, L2_FILL_BUS_CYCLES,
                         L2_MSHR_ENTRIES, L2_STREAM_BUFFERS);
  
    /* Second cache backs up first level MSHR */
    l2_cache = new generic_cache_template<generic_cache_block_t>(
                 "L2.unified", l2_mshr, m_scheduler, L2_SET_BITS, L2_ASSOC,
                 L2_BLOCK_BITS, (L2_IDEAL != 0));

    /* First level mshr, backed up by L2 cache */
    il1_mshr = new mshr_t("IL1.mshr", l2_cache, m_scheduler,
                          IL1_BLOCK_BITS, L2_LATENCY, L1_FILL_BUS_CYCLES,
                          IL1_MSHR_ENTRIES, IL1_STREAM_BUFFERS);
    
    dl1_mshr = new mshr_t("DL1.mshr", l2_cache, m_scheduler,
                          DL1_BLOCK_BITS, L2_LATENCY, L1_FILL_BUS_CYCLES,
                          DL1_MSHR_ENTRIES, DL1_STREAM_BUFFERS);
    
    /* first level instruction caches */  
    l1_inst_cache = new generic_cache_template<generic_cache_block_t>(
                      "L1.inst", il1_mshr, m_scheduler, IL1_SET_BITS,
                      IL1_ASSOC, IL1_BLOCK_BITS, (IL1_IDEAL != 0) );

    /* first level data caches */  
    l1_data_cache = new generic_cache_template<generic_cache_block_t>(
                      "L1.data", dl1_mshr, m_scheduler, DL1_SET_BITS,
                      DL1_ASSOC, DL1_BLOCK_BITS, (DL1_IDEAL != 0) );
  } else {
    /* CONFIG_WITH_RUBY  */
    m_ruby_cache = new rubycache_t( m_id, L2_BLOCK_BITS, m_scheduler );
     //create a new Write buffer, shared by all the logical processors
    m_write_buffer = new writebuffer_t( m_id, L2_BLOCK_BITS, m_scheduler );
  }

  // set MMU related fields
  m_primary_ctx = new context_id_t[CONFIG_LOGICAL_PER_PHY_PROC];
  m_itlb_physical_address = new pa_t[CONFIG_LOGICAL_PER_PHY_PROC];
  m_itlb_logical_address = new la_t[CONFIG_LOGICAL_PER_PHY_PROC];
  m_standalone_tlb = new dtlb_t *[CONFIG_LOGICAL_PER_PHY_PROC];
  m_thread_physical_address = new  pa_t[CONFIG_LOGICAL_PER_PHY_PROC];
  m_mmu_access = new mmu_interface_t *[CONFIG_LOGICAL_PER_PHY_PROC];
  m_mmu_asi = new  mmu_interface_t * [CONFIG_LOGICAL_PER_PHY_PROC];

  for(uint k=0; k <  CONFIG_LOGICAL_PER_PHY_PROC; ++k){
    m_primary_ctx[k] = 0;
    m_itlb_physical_address[k] = (pa_t) 0;
    m_itlb_logical_address[k] = (la_t) 0;
    m_thread_physical_address[k] = (pa_t) 0;
    m_mmu_access[k] = NULL;
    m_mmu_asi[k] = NULL;
 
    if (CONFIG_IN_SIMICS) {
    char mmuname[200];
    pstate_t::getMMUName( m_id+k, mmuname, 200 );
    #ifdef DEBUG_PSEQ
        DEBUG_OUT("\t***MMUName proc[%d] name[%s]\n",k,mmuname);
    #endif
    conf_object_t *mmu = SIM_get_object( mmuname );
    if (mmu == NULL) {
      ERROR_OUT("error: unable to locate object: %s\n", mmuname);
      SIM_HALT;
    }
    m_mmu_access[k] = (mmu_interface_t *) SIM_get_interface( mmu, "mmu" );
    if (m_mmu_access[k] == NULL) {
      ERROR_OUT("error: object does not implement mmu interface: %s\n",
                mmuname);
      SIM_HALT;
    }
   }
  }

  m_shadow_pstate = 0;
 
  // set fetch related fields
  m_fetch_status = new pseq_fetch_status_t[CONFIG_LOGICAL_PER_PHY_PROC];
  m_saved_fetch_status = new pseq_fetch_status_t[CONFIG_LOGICAL_PER_PHY_PROC];
  m_fetch_ready_cycle = new tick_t[CONFIG_LOGICAL_PER_PHY_PROC];
  m_next_line_address = new pa_t[CONFIG_LOGICAL_PER_PHY_PROC];
  m_line_buffer_fetch_stall = new bool[CONFIG_LOGICAL_PER_PHY_PROC];
  m_fetch_requested_line = new pa_t[CONFIG_LOGICAL_PER_PHY_PROC];
  m_wait_for_request = new bool[CONFIG_LOGICAL_PER_PHY_PROC];
  m_last_fetch_physical_address = new pa_t[CONFIG_LOGICAL_PER_PHY_PROC];
  m_last_line_buffer_address = new pa_t[CONFIG_LOGICAL_PER_PHY_PROC];
  m_unchecked_retires = new uint32[CONFIG_LOGICAL_PER_PHY_PROC];
  m_unchecked_retire_top = new uint32[CONFIG_LOGICAL_PER_PHY_PROC];
  m_unchecked_instr = (dynamic_inst_t ***) malloc(sizeof(dynamic_inst_t **) * CONFIG_LOGICAL_PER_PHY_PROC);

  for(uint k=0; k <  CONFIG_LOGICAL_PER_PHY_PROC; ++k){
    m_unchecked_instr[k] = (dynamic_inst_t **) malloc( (PSEQ_MAX_UNCHECKED + 1)*
                                                  sizeof(dynamic_inst_t *) );
    m_unchecked_retires[k] = 0;
    m_unchecked_retire_top[k] = 0;
  
    m_fetch_status[k] = PSEQ_FETCH_READY;
    m_saved_fetch_status[k] = m_fetch_status[k];
    m_fetch_ready_cycle[k] = 0;
    m_next_line_address[k] = 0;
    m_line_buffer_fetch_stall[k] = false;
    m_fetch_requested_line[k] = 0;
    m_wait_for_request[k] = false;
    m_last_fetch_physical_address[k] = 0;
    m_last_line_buffer_address[k] = 0;
   }

  m_recent_retire_index = new int32[CONFIG_LOGICAL_PER_PHY_PROC];
  m_recent_retire_instr = (dynamic_inst_t ***) malloc(sizeof(dynamic_inst_t **) * CONFIG_LOGICAL_PER_PHY_PROC);
  m_fetch_itlbmiss = new static_inst_t * [CONFIG_LOGICAL_PER_PHY_PROC];

  for(uint k=0; k <  CONFIG_LOGICAL_PER_PHY_PROC; ++k){
    m_recent_retire_index[k] = 0;
    m_recent_retire_instr[k] = (dynamic_inst_t **) malloc(PSEQ_RECENT_RETIRE_SIZE*
                                                      sizeof(dynamic_inst_t *) );
    for ( int32 i = 0; i < PSEQ_RECENT_RETIRE_SIZE; i++ ) {
      m_recent_retire_instr[k][i] = NULL;
    }
    
   m_fetch_itlbmiss[k] = new static_inst_t( ~(my_addr_t)0, STATIC_INSTR_MOP);
   m_fetch_itlbmiss[k]->incrementRefCount();

  }

  // init the I$ pipeline
  m_i_cache_line_queue.setSize( min_power_of_two(ICACHE_CYCLE) );
  m_i_cache_line_queue.init( PSEQ_INVALID_LINE_ADDRESS );
  // init the fetch pipeline
  m_fetch_per_cycle = new FiniteCycle<uint32>[CONFIG_LOGICAL_PER_PHY_PROC];
  // init the decode pipeline
  m_decode_per_cycle = new FiniteCycle<uint32>[CONFIG_LOGICAL_PER_PHY_PROC];

  m_act_schedule = new act_schedule_t( this, 64 );

  // initialize retire related members
  m_fwd_progress_cycle = 1 << 22;

  m_posted_interrupt = new trap_type_t[CONFIG_LOGICAL_PER_PHY_PROC];
  m_except_penalty = new uint32[CONFIG_LOGICAL_PER_PHY_PROC];
  m_next_exception =  new uint32[CONFIG_LOGICAL_PER_PHY_PROC];
  m_except_offender = new enum i_opcode[CONFIG_LOGICAL_PER_PHY_PROC];
  m_except_continue_at = new abstract_pc_t[CONFIG_LOGICAL_PER_PHY_PROC];
  m_except_access_addr = new my_addr_t[CONFIG_LOGICAL_PER_PHY_PROC];
  m_except_type = new exception_t[CONFIG_LOGICAL_PER_PHY_PROC];

  for(uint k=0; k <  CONFIG_LOGICAL_PER_PHY_PROC; ++k){
    m_posted_interrupt[k] = Trap_NoTrap;
    m_except_penalty[k] = 0;
    m_next_exception[k] = 0;
    m_fetch_per_cycle[k].setSize( min_power_of_two(FETCH_STAGES) );
    m_fetch_per_cycle[k].init( 0 );
    m_decode_per_cycle[k].setSize( min_power_of_two(DECODE_STAGES) );
    m_decode_per_cycle[k].init( 0 );
    clearException(k);
  }

  // check that for each ALU type, there exists at least one resource
  for (uint32 i = 0; i < FU_NUM_FU_TYPES; i++) {
    if (CONFIG_NUM_ALUS[CONFIG_ALU_MAPPING[i]] == 0) {
      DEBUG_OUT("error: ALU resource %s has no functional units allocated\n",
                pstate_t::fu_type_menomic( (fu_type_t) i ));
      ERROR_OUT("error: check CONFIG_ALU_MAPPING for map errors. (zero functional units).\n");
      SIM_HALT;
    }
  }

   m_last_icache_miss_addr = new pa_t[CONFIG_LOGICAL_PER_PHY_PROC];
   m_last_icache_l2miss = new bool[CONFIG_LOGICAL_PER_PHY_PROC];

  //
  // Statistics
  // 
  initializeStats();
#ifdef RENAME_EXPERIMENT
  for (uint32 i = 0; i < RID_NUM_RID_TYPES; i++ ) {

    uint32 logical;
    switch (i) {
    case RID_INT:
      logical = CONFIG_IREG_LOGICAL;
      break;

    case RID_SINGLE:
      logical = CONFIG_FPREG_LOGICAL;
      break;

    case RID_CC:
      logical = CONFIG_CCREG_LOGICAL;
      break;

    default:
      // we don't track rename statistics on all other registers
      continue;
    }

    m_reg_use[i] = (uint32 **) malloc( logical * sizeof(uint32 *) );
    for (uint32 j = 0; j < logical; j++) {
      m_reg_use[i][j] = (uint32 *) malloc( PSEQ_REG_USE_HIST*sizeof(uint32) );
      for (uint32 k = 0; k < PSEQ_REG_USE_HIST; k++) {
        m_reg_use[i][j][k] = 0;
      }
    }
  }
#endif

  ASSERT( m_spec_bpred->ras_state.TOS      == 0 );
  ASSERT( m_spec_bpred->ras_state.next_free == 1 );

  ASSERT( m_arch_bpred->ras_state.TOS      == 0 );
  ASSERT( m_arch_bpred->ras_state.next_free == 1 );

  #ifdef DEBUG_PSEQ
  DEBUG_OUT("pseq_t:constructor END\n");
  #endif
}

//**************************************************************************
pseq_t::~pseq_t() {

  #ifdef DEBUG_PSEQ
  DEBUG_OUT("pseq_t:destructor BEGIN\n");
  #endif

  if(m_fetch_at){
    free( m_fetch_at);
    m_fetch_at = NULL;
  }
  if(m_last_traptype){
    for(int i=0; i < CONFIG_LOGICAL_PER_PHY_PROC; ++i){
      if(m_last_traptype[i]){
        delete [] m_last_traptype[i];
        m_last_traptype[i] = NULL;
      }  
    }
    if( m_last_traptype ){
      delete [] m_last_traptype;
      m_last_traptype = NULL;
    }
  }
  if(m_last_traptype_cycle){
    for(int i=0; i < CONFIG_LOGICAL_PER_PHY_PROC; ++i){
      if(m_last_traptype_cycle[i]){
        delete [] m_last_traptype_cycle[i];
        m_last_traptype_cycle[i] = NULL;
      }  
    }
    if( m_last_traptype_cycle ){
      delete [] m_last_traptype_cycle;
      m_last_traptype_cycle = NULL;
    }
  }

  if(m_inorder_at){
    free( m_inorder_at);
    m_inorder_at = NULL;
  }
  if(m_icount)
    delete [] m_icount;
  if(m_iwin)
    delete [] m_iwin;
  if(m_local_sequence_number)
    delete [] m_local_sequence_number;
  if(m_local_inorder_number)
    delete [] m_local_inorder_number;
  if(m_local_icount)
    delete [] m_local_icount;
  if(m_fetch_per_cycle)
    delete [] m_fetch_per_cycle;
  if(m_decode_per_cycle)
    delete [] m_decode_per_cycle;
  if(m_ctxt_id)
    delete [] m_ctxt_id;
  if(m_primary_ctx)
    delete [] m_primary_ctx;
  if(m_itlb_physical_address)
    delete [] m_itlb_physical_address;
  if(m_itlb_logical_address)
    delete [] m_itlb_logical_address;
  if(m_thread_physical_address)
    delete [] m_thread_physical_address;
  if(m_fetch_status)
    delete [] m_fetch_status;
  if(m_saved_fetch_status)
    delete [] m_saved_fetch_status;
  if(m_fetch_ready_cycle)
    delete [] m_fetch_ready_cycle;
  if(m_next_line_address)
    delete [] m_next_line_address;
  if(m_line_buffer_fetch_stall)
    delete [] m_line_buffer_fetch_stall;
  if(m_fetch_requested_line)
    delete [] m_fetch_requested_line;
  if(m_wait_for_request)
    delete [] m_wait_for_request;
  if(m_last_fetch_physical_address)
    delete [] m_last_fetch_physical_address;
  if(m_last_line_buffer_address)
    delete [] m_last_line_buffer_address;
  if(m_unchecked_retires)
    delete [] m_unchecked_retires;
  if(m_unchecked_retire_top)
    delete [] m_unchecked_retire_top;
  if(m_posted_interrupt)
    delete [] m_posted_interrupt;
  if(m_next_exception)
    delete [] m_next_exception;
  if(m_except_offender)
    delete [] m_except_offender;
  if(m_except_continue_at)
    delete [] m_except_continue_at;
  if(m_except_access_addr)
    delete [] m_except_access_addr;
  if(m_except_type)
    delete [] m_except_type;
  if(m_except_penalty)
    delete [] m_except_penalty;
  if(m_ideal_status)
    delete [] m_ideal_status;
  if(m_cfg_index)
    delete [] m_cfg_index;
  if(m_ideal_retire_count)
    delete [] m_ideal_retire_count;
  if(m_ideal_last_checked)
    delete [] m_ideal_last_checked;
  if(m_ideal_last_freed)
    delete [] m_ideal_last_freed;
  if(m_ideal_first_predictable)
    delete [] m_ideal_first_predictable;
  if(m_ideal_last_predictable)
    delete [] m_ideal_last_predictable;

  if(m_write_buffer)
    delete m_write_buffer;
  if(m_bpred)
   delete m_bpred;
 if(m_ipred)
   delete m_ipred;
 if(m_spec_bpred)
   delete m_spec_bpred;
 if(m_arch_bpred)
   delete m_arch_bpred;

  if (m_control_rf) {
     for(uint k=0; k <  CONFIG_LOGICAL_PER_PHY_PROC; ++k){
      for ( uint32 i = 0; i < CONFIG_NUM_CONTROL_SETS; i++ ) {
        delete m_control_rf[k][i];
      }
      free( m_control_rf[k] );
     }
    free( m_control_rf );
  }

  if (m_control_arf)
    delete m_control_arf;

  if(m_mmu_access)
    delete [] m_mmu_access;

  if(m_mmu_asi)
    delete [] m_mmu_asi;

  // free the cache!
  if (l1_inst_cache)
    delete l1_inst_cache;
  if (l1_data_cache)
    delete l1_data_cache;
  if (l2_cache)
    delete l2_cache;
  if (il1_mshr)
    delete il1_mshr;
  if (dl1_mshr)
    delete dl1_mshr;
  if (l2_mshr)
    delete l2_mshr;
  
  for(uint k=0; k <  CONFIG_LOGICAL_PER_PHY_PROC; ++k){
    if(m_imap[k])
      delete m_imap[k];
      if(m_tracefp[k])
      delete m_tracefp[k];
    if(m_branch_trace[k])
      delete m_branch_trace[k];
     if(m_memtracefp[k])
      delete m_memtracefp[k];
  }
  if (m_imap)
    delete [] m_imap;
  if (m_conf)
    delete m_conf;
    if (m_tracefp)
    delete [] m_tracefp;
   if (m_memtracefp)
    delete [] m_memtracefp;
   if(m_branch_trace)
     delete [] m_branch_trace;

  // Free statistics

#ifdef RENAME_EXPERIMENT
  for (uint32 i = 0; i < RID_NUM_RID_TYPES; i++ ) {
    uint32 logical;
    switch (i) {
    case RID_INT:
      logical = CONFIG_IREG_LOGICAL;
      break;

    case RID_SINGLE:
      logical = CONFIG_FPREG_LOGICAL;
      break;

    case RID_CC:
      logical = CONFIG_CCREG_LOGICAL;
      break;

    default:
      // we don't track rename statistics on all other registers
      continue;
    }

    for (uint32 j = 0; j < logical; j++) {
      if (m_reg_use[i][j])
        free (m_reg_use[i][j]);
    }
    if (m_reg_use[i] != NULL)
      free( m_reg_use[i] );
  }
#endif
  if (m_hist_fetch_per_cycle)
    free(m_hist_fetch_per_cycle);
  for(uint32 i=0; i < CONFIG_LOGICAL_PER_PHY_PROC; ++i){
    if(m_hist_fetch_per_thread[i]){
      delete [] m_hist_fetch_per_thread[i];
    }
    if(m_hist_decode_per_thread[i]){
      delete [] m_hist_decode_per_thread[i];
    }
    if(m_hist_schedule_per_thread[i]){
      delete [] m_hist_schedule_per_thread[i];
    }
    if(m_hist_retire_per_thread[i]){
      delete [] m_hist_retire_per_thread[i];
    }
  }
  if(m_hist_fetch_per_thread){
    delete [] m_hist_fetch_per_thread;
    m_hist_fetch_per_thread = NULL;
  }
  if(m_hist_decode_per_thread){
    delete [] m_hist_decode_per_thread;
    m_hist_decode_per_thread = NULL;
  }
  if(m_hist_schedule_per_thread){
    delete [] m_hist_schedule_per_thread;
    m_hist_schedule_per_thread = NULL;
  }
  if(m_hist_retire_per_thread){
    delete [] m_hist_retire_per_thread;
    m_hist_retire_per_thread = NULL;
  }
  if(m_hist_smt_fetch_per_cycle)
    free(m_hist_smt_fetch_per_cycle);
  if (m_hist_decode_per_cycle)
    free(m_hist_decode_per_cycle);
  if (m_hist_schedule_per_cycle)
    free(m_hist_schedule_per_cycle);
  if (m_hist_retire_per_cycle)
    free(m_hist_retire_per_cycle);

  if (m_hist_fetch_stalls)
    free(m_hist_fetch_stalls);
  if (m_hist_decode_return)
    free(m_hist_decode_return);
  if (m_hist_retire_stalls)
    free(m_hist_retire_stalls);
  if (m_hist_ff_length)
    free(m_hist_ff_length);
  if (m_hist_ideal_coverage)
    free(m_hist_ideal_coverage);
  if (m_stat_fu_utilization)
    free(m_stat_fu_utilization);
  if(m_stat_fu_stall)
    free(m_stat_fu_stall);

  for(uint k=0; k <  CONFIG_LOGICAL_PER_PHY_PROC; ++k){  
   if (m_stat_fu_util_retired && m_stat_fu_util_retired[k])
     free(m_stat_fu_util_retired[k]);
   if(m_ras[k])
     delete m_ras[k];
   if(m_mem_deps && m_mem_deps[k])
     delete m_mem_deps[k];
  }
  if(m_stat_fu_util_retired)
    free(m_stat_fu_util_retired);
 
  if(m_ras)
    delete [] m_ras;

  if(m_mem_deps)
    delete [] m_mem_deps;

  //free stat pointers
  if(m_stat_count_functionalretire)
    delete [] m_stat_count_functionalretire;
  if(m_stat_count_badretire)
    delete [] m_stat_count_badretire;
  if(m_stat_count_retiresquash)
    delete [] m_stat_count_retiresquash;

  for(uint k=0; k <  CONFIG_LOGICAL_PER_PHY_PROC; ++k){
    if(m_overall_timer[k])
      delete m_overall_timer[k];
    if(m_thread_timer[k])
      delete m_thread_timer[k];
    if(m_thread_histogram[k])
      delete m_thread_histogram[k];
    if(m_lsq[k])
      delete m_lsq[k];
  }
  if(m_lsq)
    delete [] m_lsq;
  if(m_overall_timer)
    delete [] m_overall_timer;
  if(m_thread_timer)
    delete [] m_thread_timer;
  if(m_thread_histogram)
    delete [] m_thread_histogram;


  if(m_exclude_count)
    delete [] m_exclude_count;
  if(m_thread_count)
    delete [] m_thread_count;
  if(m_thread_count_idle)
    delete [] m_thread_count_idle;
  
  if(m_pred_count_stat)
    delete [] m_pred_count_stat;
  if(m_pred_count_taken_stat)
    delete [] m_pred_count_taken_stat;
  if(m_pred_count_nottaken_stat)
    delete [] m_pred_count_nottaken_stat;
  if(m_nonpred_count_stat)
    delete [] m_nonpred_count_stat;
  if(m_pred_reg_count_stat)
    delete [] m_pred_reg_count_stat;
  if(m_pred_reg_taken_stat)
    delete [] m_pred_reg_taken_stat;
  if(m_pred_reg_nottaken_stat)
    delete [] m_pred_reg_nottaken_stat;
  if(m_pred_retire_count_stat)
    delete [] m_pred_retire_count_stat;
  if(m_pred_retire_count_taken_stat)
    delete [] m_pred_retire_count_taken_stat;
  if(m_pred_retire_count_nottaken_stat)
    delete [] m_pred_retire_count_nottaken_stat;
  if(m_nonpred_retire_count_stat)
    delete [] m_nonpred_retire_count_stat;
  if(m_pred_reg_retire_count_stat)
    delete [] m_pred_reg_retire_count_stat;
  if(m_pred_reg_retire_taken_stat)
    delete [] m_pred_reg_retire_taken_stat;
  if(m_pred_reg_retire_nottaken_stat)
    delete [] m_pred_reg_retire_nottaken_stat;
  if(m_reg_stall_count_stat)
    delete [] m_reg_stall_count_stat;
  if(m_decode_stall_count_stat)
    delete [] m_decode_stall_count_stat;
  if(m_iwin_stall_count_stat)
    delete [] m_iwin_stall_count_stat;
  if(m_schedule_stall_count_stat)
    delete [] m_schedule_stall_count_stat;
  if(m_stat_early_store_bypass)
    delete [] m_stat_early_store_bypass;

  if(m_opstat)
    delete [] m_opstat;
  if(m_ideal_opstat)
    delete [] m_ideal_opstat;

  if(m_stat_trace_insn)
    delete [] m_stat_trace_insn;
  if(m_stat_committed)
    delete [] m_stat_committed;
  if(m_stat_total_squash)
    delete [] m_stat_total_squash;
  if(m_stat_commit_squash)
    delete [] m_stat_commit_squash;
  if(m_stat_count_asistoresquash)
    delete [] m_stat_count_asistoresquash;
  if(m_stat_commit_good)
    delete [] m_stat_commit_good;
  if(m_stat_commit_bad)
    delete [] m_stat_commit_bad;
  if(m_stat_commit_unimplemented)
    delete [] m_stat_commit_unimplemented;
  if(m_stat_count_except)
    delete [] m_stat_count_except;

  if(m_stat_loads_retired)
    delete [] m_stat_loads_retired;
  if(m_stat_stores_retired)
    delete [] m_stat_stores_retired;
  if(m_stat_retired_stores_no_permission)
    delete [] m_stat_retired_stores_no_permission;
  if(m_stat_retired_atomics_no_permission)
    delete [] m_stat_retired_atomics_no_permission;
  if(m_stat_retired_loads_no_permission)
    delete [] m_stat_retired_loads_no_permission;
  if(m_stat_retired_loads_nomatch)
    delete [] m_stat_retired_loads_nomatch;
  if(m_stat_atomics_retired)
    delete [] m_stat_atomics_retired;
  if(m_stat_prefetches_retired)
    delete [] m_stat_prefetches_retired;
  if(m_stat_control_retired)
    delete [] m_stat_control_retired;

  if(m_stat_fetched)
    delete [] m_stat_fetched;
  if(m_stat_mini_itlb_misses)
    delete [] m_stat_mini_itlb_misses;
  if(m_stat_decoded)
    delete [] m_stat_decoded;
  if(m_stat_total_insts)
    delete [] m_stat_total_insts;

  if(m_stat_loads_exec)
    delete [] m_stat_loads_exec;
  if(m_stat_stores_exec)
    delete [] m_stat_stores_exec;
  if(m_stat_atomics_exec)
    delete [] m_stat_atomics_exec;
  if(m_stat_prefetches_exec)
    delete [] m_stat_prefetches_exec;
  if(m_stat_control_exec)
    delete [] m_stat_control_exec;

  if(m_stat_loads_found)
    delete [] m_stat_loads_found;
  if(m_stat_loads_notfound)
    delete [] m_stat_loads_notfound;
  if(m_stat_spill)
    delete [] m_stat_spill;
  if(m_stat_fill)
    delete [] m_stat_fill;

  if(m_stat_miss_count)
    delete [] m_stat_miss_count;
  if(m_stat_last_miss_seq)
    delete [] m_stat_last_miss_seq;
  if(m_stat_last_miss_fetch)
    delete [] m_stat_last_miss_fetch;
  if(m_stat_last_miss_issue)
    delete [] m_stat_last_miss_issue;
  if(m_stat_last_miss_retire)
    delete [] m_stat_last_miss_retire;

  if(m_stat_miss_effective_ind)
    delete [] m_stat_miss_effective_ind;
  if(m_stat_miss_effective_dep)
    delete [] m_stat_miss_effective_dep;
  if(m_stat_miss_inter_cluster)
    delete [] m_stat_miss_inter_cluster;

  //lsq stats
  if(m_stat_load_bypasses)
    delete [] m_stat_load_bypasses;
  if(m_stat_atomic_bypasses)
    delete [] m_stat_atomic_bypasses;
  if(m_stat_num_early_stores)
    delete [] m_stat_num_early_stores;
  if(m_stat_num_early_store_bypasses)
    delete [] m_stat_num_early_store_bypasses;
  if(m_stat_num_early_atomics)
    delete [] m_stat_num_early_atomics;
  if(m_stat_load_store_conflicts)
    delete [] m_stat_load_store_conflicts;
  if(m_stat_load_incorrect_store)
    delete [] m_stat_load_incorrect_store;
  if(m_stat_atomic_incorrect_store)
    delete [] m_stat_atomic_incorrect_store;

  // StoreSet predictor stats
  if(m_stat_storeset_stall_load)
    delete [] m_stat_storeset_stall_load;
  if(m_stat_storeset_stall_atomic)
    delete [] m_stat_storeset_stall_atomic;

  if(m_stat_stale_predictions)
    delete [] m_stat_stale_predictions;
  if(m_stat_stale_success)
    delete [] m_stat_stale_success;

  //Write Buffer stats
  if(m_stat_num_write_buffer_hits)
    delete [] m_stat_num_write_buffer_hits;
  if(m_stat_num_write_buffer_full)
    delete [] m_stat_num_write_buffer_full;

  //cache stats
  if(m_stat_num_icache_miss)
     delete [] m_stat_num_icache_miss;;
  if(m_stat_num_dcache_miss)
     delete [] m_stat_num_dcache_miss;
  if(m_stat_icache_mshr_hits)
     delete [] m_stat_icache_mshr_hits;
  if(m_stat_retired_dcache_miss)
     delete [] m_stat_retired_dcache_miss;
  if(m_stat_retired_memory_miss)
     delete [] m_stat_retired_memory_miss;
  if(m_stat_retired_mshr_hits)
     delete [] m_stat_retired_mshr_hits;
  if(m_stat_count_io_access)
     delete [] m_stat_count_io_access;
  if(m_num_cache_not_ready)
     delete [] m_num_cache_not_ready;

  if(m_stat_stale_histogram)
    delete [] m_stat_stale_histogram;

  if(m_trapstat){
    delete [] m_trapstat;
    m_trapstat = NULL;
  }
  if(m_software_trapstat){
    delete [] m_software_trapstat;
    m_software_trapstat = NULL;
  }
  if(m_simtrapstat){
    delete [] m_simtrapstat;
    m_simtrapstat = NULL;
  }
  if(m_completed_trapstat){
    delete [] m_completed_trapstat;
    m_completed_trapstat = NULL;
  }

  if(m_stat_continue_calls)
    delete [] m_stat_continue_calls;
  if(m_stat_modified_instructions)
    delete [] m_stat_modified_instructions;
  if(m_inorder_partial_success)
    delete [] m_inorder_partial_success;

  if(m_inorder)
    delete [] m_inorder;

  //icount stats
  if(m_icount_stats){
    delete [] m_icount_stats;
    m_icount_stats = NULL;
  }

  //cache miss latency stats
  if(m_ifetch_miss_latency){
    delete [] m_ifetch_miss_latency;
    m_ifetch_miss_latency = NULL;
  }
  if(m_load_miss_latency){
    delete [] m_load_miss_latency;
    m_load_miss_latency = NULL;
  }
  if(m_store_miss_latency){
    delete [] m_store_miss_latency;
    m_store_miss_latency = NULL;
  }
  if(m_atomic_miss_latency){
    delete [] m_atomic_miss_latency;
    m_atomic_miss_latency = NULL;
  }

  if(m_stat_exceed_scheduling_window){
    delete [] m_stat_exceed_scheduling_window;
    m_stat_exceed_scheduling_window = NULL;
  }
  if(m_stat_not_enough_registers){
    delete [] m_stat_not_enough_registers;
    m_stat_not_enough_registers = NULL;
  }

  //for squash histogram
  if(m_hist_squash_stage){
    for(int i=0; i < dynamic_inst_t::MAX_INST_STAGE; ++i){
      if(m_hist_squash_stage[i]){
        free(m_hist_squash_stage[i]);
        m_hist_squash_stage[i] = NULL;
      }
    }
    free(m_hist_squash_stage);
  }

  // for retire not-ready histogram
  if(m_stat_retire_notready_stage){
    delete [] m_stat_retire_notready_stage;
    m_stat_retire_notready_stage = NULL;
  }
  if(m_last_traplevel){
    delete [] m_last_traplevel;
    m_last_traplevel = NULL;
  }
  if(m_last_icache_miss_addr){
    delete [] m_last_icache_miss_addr;
    m_last_icache_miss_addr = NULL;
  }
  if(m_last_icache_l2miss){
    delete [] m_last_icache_l2miss;
    m_last_icache_l2miss = NULL;
  }
  if(m_stat_memread_exception){
    delete [] m_stat_memread_exception;
    m_stat_memread_exception = NULL;
  }
  
  #ifdef DEBUG_PSEQ
  DEBUG_OUT("pseq_t:destructor END\n");
  #endif
 
}

/*------------------------------------------------------------------------*/
/* Public methods                                                         */
/*------------------------------------------------------------------------*/

/* Write Buffer specific functions */
//**************************************************************************
void pseq_t::WBStallFetch(){
  //called by the Write Buffer, saves all of the current fetch statuses, and sets all of them to be stalled
  for(uint i=0; i < CONFIG_LOGICAL_PER_PHY_PROC; ++i){
    m_saved_fetch_status[i] = m_fetch_status[i];
    m_fetch_status[i] = PSEQ_FETCH_WRITEBUFFER_FULL;
  }
  m_wb_stall_start_cycle = m_local_cycles;
}

//*************************************************************************
void pseq_t::WBUnstallFetch(){
  //restores each logical proc's saved fetch status after the WB has free entries:
  for(uint i=0; i < CONFIG_LOGICAL_PER_PHY_PROC; ++i){
    #ifdef DEBUG_WRITE_BUFFER
        DEBUG_OUT("unstallFetch: fetch status = [%s] saved fetch status = [%s]\n",
                  fetch_menomic(m_fetch_status[i]), fetch_menomic(m_saved_fetch_status[i]));
   #endif
      if(m_fetch_status[i] == PSEQ_FETCH_WRITEBUFFER_FULL){
        m_fetch_status[i] = m_saved_fetch_status[i];
        //an exception to look out for is whether we are in the PSEQ_FETCH_SQUASH status, and
        //  we have completely overlapped the pipeline flushing penalty...if so, we immediately set to
        //  PSEQ_FETCH_READY (or else we will fail assertion in fetchInstrSimple() )
        if((m_fetch_status[i] == PSEQ_FETCH_SQUASH) && (m_fetch_ready_cycle[i] < getLocalCycle()) ){
          //ERROR_OUT("unstallFetch, WB setting FETCH READY proc[ %d ] cycle[ %lld ]\n", m_id, m_local_cycles);
          m_fetch_status[i] = PSEQ_FETCH_READY;
        }
      }
    }
    m_wb_stall_end_cycle = m_local_cycles;
}
/* END Write Buffer specific functions */

//*************************************************************************
/** advances one clock cycle for this processor.
 *
 *  Includes fetching, decoding, scheduling, executing, and retiring
 *  instructions. Each of these sub-tasks is broken out as private
 *  methods of this class.
 *
 */
//**************************************************************************
void pseq_t::advanceCycle( void )
{
  #ifdef DEBUG_PSEQ
    DEBUG_OUT("pseq_t:advanceCycle BEGIN m_id[%d] cycle[%d] \n",m_id, m_local_cycles);
  #endif

#ifdef PIPELINE_VIS 
  for(uint k=0; k <  CONFIG_LOGICAL_PER_PHY_PROC; ++k){
    DEBUG_OUT("proc %d\n", k);
    DEBUG_OUT( "cycle %u\n", (uint32) getLocalCycle());
    ireg_t pstate    = m_control_arf->getRetireRF(k)->getInt( CONTROL_PSTATE );
    if ( m_shadow_pstate != (uint16) pstate ) {
      DEBUG_OUT( "set pstate 0x%0llx\n", pstate );
      m_shadow_pstate = pstate;
    }
    printPC( m_fetch_at[k] );
  }
#endif

  // I. Fetch instructions
  fetchInstruction();

  // II. Decode instructions in the pipe
  decodeInstruction();

  // III. Schedule decoded instructions
  scheduleInstruction();

  // (Executing scheduled instructions on functional units takes place
  //  implicitly after their dependencies become resolved) 

  // IV. Retire completed instructions
  retireInstruction();

#ifdef CHECK_REGFILE
  // verify the free lists
  for(uint k=0; k <  CONFIG_LOGICAL_PER_PHY_PROC; ++k){
    m_ooo.regs.validateMapping( m_id, m_iwin[k] );  
  }
#endif

  // The sequencer must try to flush write buffer on every cycle:
  if(CONFIG_WITH_RUBY){
    m_write_buffer->issueWrites();
  }

  // tell the sequencer to do its action on the executing instructons
  m_scheduler->Execute( getLocalCycle() );       

  //
  // If running a uniprocessor simulation, or running in 'fake ruby' mode
  // this processor needs to advance the time of its local caches.
  //
  if (CONFIG_WITH_RUBY) {
#ifdef FAKE_RUBY
    m_ruby_cache->advanceTimeout();
#endif
  } else {
    // Tick L2 first
    l2_mshr->Tick();

    // then tick L1
    il1_mshr->Tick();
    dl1_mshr->Tick();
  }

  // advance local time one cycle (do this only after all threads have finished going through pipeline)
  localCycleIncrement();               

  #ifdef DEBUG_PSEQ
    DEBUG_OUT("pseq_t:advanceCycle END m_id[%d] cycle[%d] \n",m_id,m_local_cycles);
  #endif
}

//**************************************************************************
void pseq_t::scheduleWBWakeup()
{
  //FIXME: scheduling the WB with the event queue is currently causing WB inconsistencies.., use issueWrites() instead
  if(m_write_buffer->useWriteBuffer()){
    m_write_buffer->scheduleWakeup();
  }
}

//**************************************************************************
/** Fetch a number of instructions past the current PC.
 *  This is where the "fetch policy" is implemented.
 *  Currently, the policy can be simple (idealizing some features),
 *  or multi-cycle (a more detailed model).
 */
//**************************************************************************
void pseq_t::fetchInstruction( )
{
  fetchInstrSimple();
}

/** Fetch a number of instructions past the current PC.
 *  This simple front-end uses some idealizations that are not present
 *  in the complex "multi-cycle" front-end.
 */
//**************************************************************************
void pseq_t::fetchInstrSimple( )
{
 #ifdef DEBUG_PSEQ
          DEBUG_OUT("pseq_t:fetchInstrSimple BEGIN cycle[ %d ]\n",getLocalCycle());
  #endif

  uint   num_threads_fetched = 0;
  uint32  total_fetched = 0;
  //because each logical proc could potentially go through the fetch loop multiple times we need
  //   to keep track of the total instrs fetched per thread:
  uint32 num_fetched[CONFIG_LOGICAL_PER_PHY_PROC];
  uint32 timeout = 0;        //to check for infinite loops!
  //done array indicates whether in this cycle this logical processor is done with instruction fetching
  //  This occurs under 3 conditions: 1) It has fetched some instrs 2) It has a pending Icache miss 3) It has a ITLB miss
  bool done[CONFIG_LOGICAL_PER_PHY_PROC];
  for(uint i=0; i < CONFIG_LOGICAL_PER_PHY_PROC; ++i){
    done[i] = false;
    num_fetched[i] = 0;
  }

  while(continueFetch(total_fetched, done, num_threads_fetched)){
    ASSERT(timeout < 300); 
    for(uint proc=0; (proc <  CONFIG_LOGICAL_PER_PHY_PROC); ++proc){
      timeout++;

        #ifdef DEBUG_PSEQ
          DEBUG_OUT("pseq_t:fetchInstrSimple BEGIN proc[%d] cycle %d\n",proc, getLocalCycle());
          printPC(m_fetch_at[proc]);
         #endif

       dynamic_inst_t  *d_instr = NULL;
       static_inst_t   *s_instr = NULL;
       pa_t             fetchPhysicalPC;

        // XU DEBUG
       static bool last_mode = false;
       static dynamic_inst_t* last_d_call_instr = NULL;
       static bool last_fetch_is_call = false;

       // check if we have waited long enough for squash penalty
       if (m_fetch_status[proc] == PSEQ_FETCH_SQUASH) {
         
         ASSERT(m_fetch_ready_cycle[proc] >= getLocalCycle());

         if (m_fetch_ready_cycle[proc] == getLocalCycle()) {
           //ERROR_OUT("fetchinstr setting FETCH READY proc[ %d ] cycle[ %lld ]\n", m_id, m_local_cycles);
           m_fetch_status[proc] = PSEQ_FETCH_READY;
         }
       }

       static const bool fetch_pass_cache_line = (FETCH_PASS_CACHE_LINE > 0);
       static const bool fetch_pass_taken_branch = (FETCH_PASS_TAKEN_BRANCH > 0);


       if (m_fetch_status[proc] != PSEQ_FETCH_READY) {
         // if not ready, skip fetching
         if (DEBUG_SIMPLE_FE) DEBUG_OUT("fetch is not ready: %s\n", fetch_menomic(m_fetch_status[proc]));
         ASSERT( m_fetch_status[proc] < PSEQ_FETCH_MAX_STATUS );

        #ifdef DEBUG_PSEQ
            DEBUG_OUT("\tfetch not ready, status = %s cycle[ %lld ]\n", fetch_menomic(m_fetch_status[proc]), m_local_cycles);
         #endif
         //set this logical proc's done flag to true:
         done[proc] = true;

         // check whether outstanding icache miss is an L2 miss.
         // if so,set our flag
         if( CONFIG_WITH_RUBY && m_fetch_status[proc] == PSEQ_FETCH_ICACHEMISS){
           ASSERT( m_last_icache_miss_addr[proc] != -1);
           if( getRubyCache()->isRequestL2Miss(m_last_icache_miss_addr[proc])){
             m_last_icache_l2miss[proc] = true;
           }
         }
    
       } else {
         // fetch engine is ready
      
         //only fetch instrs from thread that has smallest Icount:
        if(isLowestICount(proc, done) &&  (num_threads_fetched < m_threads_per_cycle)){
         for (; !done[proc] && total_fetched < MAX_FETCH; ) {
           //  Check to see that the instruction window has space for an instruction
           if (!isSlotAvailable(proc)) {
             if(DEBUG_SIMPLE_FE) DEBUG_OUT("stalling fetch: no slots available\n");
             m_fetch_status[proc] = PSEQ_FETCH_WIN_FULL;

             done[proc] = true; 
             break;
           }
           
           lookupInstruction(m_fetch_at[proc], &s_instr, &fetchPhysicalPC, proc);

           if ( (s_instr == m_fetch_itlbmiss[proc]) ) {
             
                   /** When the fetch unit is unable to perform an address translation,
                    *  the pseq.C fetch state is changed to FETCH_ITLBMISS. Fetch is
                    *  restarted when any instruction squashes. So when the TLB
                    *  exception is handled the fetch is restarted.
                    */
                   m_fetch_status[proc] = PSEQ_FETCH_ITLBMISS;
                   done[proc] = true;
           } else {        
              #ifdef DEBUG_PSEQ
                 //DEBUG_OUT("\tITLB hit, now access cache...\n");
              #endif
    
             // ITLB hit, now access cache
             bool hit = false;
             if (CONFIG_WITH_RUBY) {
#ifdef DEBUG_RUBY
          DEBUG_OUT("IFETCH: PC 0x%0llx\n", fetchPhysicalPC);
#endif
          bool mshr_hit = false;
          bool mshr_stall = false;
          bool conflicting_access = false;
     
          hit = getRubyCache()->access( m_fetch_at[proc]->pc, fetchPhysicalPC, OPAL_IFETCH, m_fetch_at[proc]->pc,
                                        (m_fetch_at[proc]->pstate >> 2) & 0x1,
                                        m_proc_waiter[proc], mshr_hit, mshr_stall, conflicting_access, proc);
          
    
          if (mshr_hit) {
            STAT_INC( m_stat_icache_mshr_hits[proc] );            
          }
          if (mshr_stall) {
            // sequencer fetch context will never stall: it has a dedicated
            // MSHR resource assigned to it.
            SIM_HALT;
          }
          if( conflicting_access ){
            // increment some stats here??
          }

          // if not in L1 cache: stall fetch pending i-cache return
          ASSERT(hit != NOT_READY);

          // for perfect ICache
          if( PERFECT_ICACHE && !hit ){
            //remove waiter from wait list
            ASSERT(m_proc_waiter[proc]->Waiting() == true);
            m_proc_waiter[proc]->RemoveWaitQueue();
            hit = true;
          }

          if (!hit) {
            STAT_INC( m_stat_num_icache_miss[proc] );
            if(DEBUG_SIMPLE_FE) DEBUG_OUT("l1 cache miss ... 0x%0llx\n",
                                         fetchPhysicalPC);
            m_fetch_status[proc] = PSEQ_FETCH_ICACHEMISS;
            m_last_icache_miss_addr[proc] = fetchPhysicalPC;
            done[proc] = true;
            break;
          }
          
        } else {  // !CONFIG_WITH_RUBY
          /** check the ifetch line buffer
             *  This include the case where pc1 == pc2, when last fetch was
             *  a miss and it is then fetched again after woken up.
             */
           #ifdef DEBUG_PSEQ
               DEBUG_OUT("\tWe hit in the L1-Icache!\n");
           #endif
             if(! l1_inst_cache->same_line(m_last_fetch_physical_address[proc], fetchPhysicalPC)) {
                  #ifdef DEBUG_PSEQ
                     DEBUG_OUT("\tUpdating our line buffer...\n");
                   #endif
                     // Modified to take in the logical proc's waiter object:
               hit = l1_inst_cache->Read( fetchPhysicalPC, m_proc_waiter[proc], false );

                /* WATTCH power */
               if(WATTCH_POWER){
                 getPowerStats()->incrementICacheAccess();
               }

                #ifdef DEBUG_PSEQ
                     DEBUG_OUT("\tAfter reading L1Icache, hit[%d]..\n",hit);
                   #endif
               m_last_fetch_physical_address[proc] = fetchPhysicalPC;             //update our line buffer for the new instr addr being read out
               // if not in L1 cache: stall fetch pending i-cache return
               if (!hit) {
                 STAT_INC( m_stat_num_icache_miss[proc] );
                 if (DEBUG_SIMPLE_FE) DEBUG_OUT("l1 cache miss ... 0x%0llx\n",
                                               fetchPhysicalPC);
                 m_fetch_status[proc] = PSEQ_FETCH_ICACHEMISS;
                 m_last_icache_miss_addr[proc] = fetchPhysicalPC;
                 done[proc] = true; 
                 break;
               }
               // check shall we fetch pass cache line? (first fetch this cycle to new line is allowed)
               if(!fetch_pass_cache_line && num_fetched[proc] != 0) {
                 if(DEBUG_SIMPLE_FE) DEBUG_OUT("do not pass cache line ... 0x%0llx\n", fetchPhysicalPC);

                 //account for this condition by setting the done flag:
                 done[proc] = true;
                 break;
               }
             } else {
               /* fetch from the line buffer */
               hit = true;
               if(DEBUG_SIMPLE_FE) DEBUG_OUT("line buffer hit ... 0x%0llx\n", fetchPhysicalPC);
             }
        } // ! ruby cache
     } // !tlbmiss

     #ifdef DEBUG_PSEQ
        DEBUG_OUT("\tAbout to assign old_pc...\n");
      #endif

      abstract_pc_t old_pc = *(m_fetch_at[proc]);
      //   allocate a new dynamic instruction
      d_instr = createInstruction( s_instr, m_fetch_at[proc], fetchPhysicalPC, proc );
      //   add the dynamic instruction to the instruction window
      m_iwin[proc].insertInstruction( d_instr );

      // If this instruction experienced a L2 Icache miss, mark it as such
      if( m_last_icache_l2miss[proc] == true){
        d_instr->markEvent(EVENT_L2_INSTR_MISS);
        //reset flag and last icache miss
        m_last_icache_miss_addr[proc] = -1;
        m_last_icache_l2miss[proc] = false;
      }

       //increment the num_fetch and total_fetch here:
      num_fetched[proc]++;
      //NOTE: only count non-mop instructions (no TLB-miss indicators) in total fetched
      if(s_instr != m_fetch_itlbmiss[proc]){ 
        total_fetched++;
      }

      /* WATTCH power */
      if(WATTCH_POWER){
        getPowerStats()->incrementWindowAccess();
      }

      #ifdef DEBUG_PSEQ
            DEBUG_OUT("\tAfter insertInstruction\n");
       #endif  
      // if this is a barrier-type instruction, stall fetch
      // Need to stall for correctness for ITLB and DTLB - the stxa instruction
      if  (s_instr->getFlag( SI_FETCH_BARRIER )) {
            #ifdef DEBUG_PSEQ
             //DEBUG_OUT("\tInstruction fetched is a BARRIER...stall fetch!!\n");
            #endif

          m_fetch_status[proc] = PSEQ_FETCH_BARRIER;
          done[proc] = true;
      }
      
      // whether this is the delay slot of a taken branch?
      bool taken  = (m_fetch_at[proc]->pc - old_pc.pc) != sizeof(uint32);
      if (!fetch_pass_taken_branch && taken) {
        if(DEBUG_SIMPLE_FE) DEBUG_OUT("do not pass branch from V:0x%0llx to V:0x%0llx ... \n", old_pc.pc, m_fetch_at[proc]->pc);
        done[proc] = true;
      }
      
      // excluding call&restore combination as CALL instr
      if (ras_t::PRECISE_CALL) {
        if(s_instr->getBranchType() == BRANCH_CALL) {
          last_fetch_is_call = true;
          last_d_call_instr = d_instr;
        } else {
          // call after restore is annulled
          if(last_fetch_is_call && s_instr->getOpcode() == i_restore) {
            // undo the push of last call
            m_ras[proc]->pop(&getSpecBPS()->ras_state);
            ASSERT(last_d_call_instr);
            ((control_inst_t *) last_d_call_instr)->setPredictorState(*getSpecBPS());
          }
          last_fetch_is_call = false;
          last_d_call_instr = NULL;
        }
      } // ras_t::PRECISE_CALL
      
         } // end for
        }    // end isLowestICount()
       } // end else (fetch is not ready)
  
       if (d_instr) last_mode = d_instr->getPrivilegeMode();
  
       /** insert the number of instructions fetched this cycle into the pipeline
        *  these instructions become ready in FETCH_STAGES cycles
        */
       
#ifdef DEBUG_PSEQ
         DEBUG_OUT("\tinserting %d instrs to be decoded at cycle[%d] proc[%d] fetch_status[ %s ]\n",num_fetched[proc],
                         getLocalCycle()+FETCH_STAGES,proc, fetch_menomic(m_fetch_status[proc]));
#endif

       ASSERT( num_fetched[proc] <= MAX_FETCH );    
       ASSERT( total_fetched <= MAX_FETCH);
       
       //Now set the dynamic upper limit for the remaining logical procs:
       if(num_fetched[proc] != 0 &&  m_fetch_status[proc] != PSEQ_FETCH_ITLBMISS){
         // We should only increment our num_threads_fetched counter only when we are
         //          sure that there is no TLB miss for this thread (but we might have a L1Icache miss, in
         //           which case we can let the remaining threads fetch more instrs)
         //        Note: we fetch NOPs when we have TLB misses
         num_threads_fetched++;
         //set this logical proc's done flag (it has finished fetching instrs) 
         done[proc] = true; 
       }
      m_icount[proc] = m_iwin[proc].getNumSlotsTaken();                    //set m_icount for this thread
       
   #ifdef DEBUG_PSEQ
     printPC(m_fetch_at[proc]);
     DEBUG_OUT("pseq_t:fetchinstrSimple END m_id[%d] proc[%d] cycle[%d]\n",m_id, proc,getLocalCycle());
   #endif
    } //end for loop over all logical procs
  } // end while(continueFetch())

  //Now it is safe to insert the number of instructions fetched for each logical processor:
  for(uint proc=0; (proc <  CONFIG_LOGICAL_PER_PHY_PROC); ++proc){ 
    m_fetch_per_cycle[proc].insertItem( getLocalCycle(), FETCH_STAGES, num_fetched[proc] );
    
    //record the per-thread fetch histogram:
    m_hist_fetch_per_thread[proc][num_fetched[proc]]++;
    
    // record the stall reason
    if (num_fetched[proc] == 0){
      m_hist_fetch_stalls[m_fetch_status[proc]]++;
    }

    // reset status if it was a window full (bc we will be entering Execution stage later)
    if (m_fetch_status[proc] == PSEQ_FETCH_WIN_FULL){
      //ERROR_OUT("fetchInstr WIN FULL setting FETCH READY proc[ %d ] cycle[ %lld ]\n", m_id, m_local_cycles);
      m_fetch_status[proc] = PSEQ_FETCH_READY;
    }

    //collect icount stats
    addICountStat(proc, m_icount[proc]);

  } //end for all logical procs
       
#ifdef DEBUG_PSEQ
  //out_info("num fetched PC 0x%0llx NPC 0x%0llx %d\n",
  //       m_fetch_at[proc]->pc, m_fetch_at[proc]->npc, num_fetched);
#endif
  // record the per-core fetch histogram
  m_hist_fetch_per_cycle[total_fetched]++;
 // record the SMT fetch histogram 
 m_hist_smt_fetch_per_cycle[num_threads_fetched]++;

  #ifdef DEBUG_PSEQ
     DEBUG_OUT("pseq_t:fetchinstrSimple END m_id[%d] cycle[ %d ]\n",m_id, getLocalCycle());
   #endif

}

/** Task: check to see if this instruction is cached in the ipagemap
 *        (if it was seen before it is cached there). This results
 *        in the staticly decoded instruction pointer being set.
 */
//**************************************************************************
void pseq_t::lookupInstruction( const abstract_pc_t *apc,
                                static_inst_t **s_instr_ptr,
                                pa_t* fetchPhysicalPC_ptr, unsigned int proc ) {
  #ifdef DEBUG_PSEQ
   DEBUG_OUT("pseq_t:lookupInstruction BEGIN m_id[%d] proc[%d\n",m_id,proc);
      printPC((abstract_pc_t *) apc);
  #endif

  bool            isValid = false;
  bool            inTLB = false;
  uint32          next_instr = 0;

  *s_instr_ptr = NULL;
  if ( CONFIG_IN_SIMICS ) {
    // (1)  perform address translation
#ifdef USE_MINI_ITLB
    // if enabled, use a mini-tlb (last TLB returned by simics) to a-translate
    // Note:
    //     Caching decoded instructions can impact correctness especially
    //     in the case of self-modifying code. If using the mini-TLB cache
    //     causes your correctness to go down, the cache of instructions
    //     (in the ipage/ipagemap) may not be getting properly invalidated.
    //     Historically, there was a problem with simics not notifying us
    //     on context changes which would also cause this instruction cache
    //     to get polluted. Uncomment code around 'CTXT_SWITCH' identifiers
    //     to see if we are notified of context switches. Finally, it should
    //     be noted the SPARC instruction 'FLUSH' is not properly implemented
    //     (and is used for self modifying code).
    bool miniTLB_hit = false;
    if ( (apc->pc & PSEQ_OS_PAGE_MASK) == m_itlb_logical_address[proc] ) {

        #ifdef DEBUG_PSEQ
            // DEBUG_OUT("\tmini_tlb HIT\n");
         #endif

      *fetchPhysicalPC_ptr = ( m_itlb_physical_address[proc] | 
                               (apc->pc & ~PSEQ_OS_PAGE_MASK) );
      inTLB = true;
      miniTLB_hit = true;
#ifdef PIPELINE_VIS
      DEBUG_OUT("mini-itlb hit: itlb:0x%0llx bits:0x%0llx == 0x%0llx\n",
               m_itlb_physical_address[proc], (apc->pc & ~PSEQ_OS_PAGE_MASK),
               *fetchPhysicalPC_ptr);
#endif
    } else {

        #ifdef DEBUG_PSEQ
           DEBUG_OUT("\tmini_tlb MISS...updating our mini_tlb...\n");
        #endif

      inTLB   = getInstr( apc->pc, apc->tl, apc->pstate,
                          fetchPhysicalPC_ptr, &next_instr, proc);
      if (inTLB) {
        m_itlb_logical_address[proc]  = apc->pc & PSEQ_OS_PAGE_MASK;                          
         //update our mini TLB cache to this latest translation
        m_itlb_physical_address[proc] = *fetchPhysicalPC_ptr & PSEQ_OS_PAGE_MASK;
        STAT_INC( m_stat_mini_itlb_misses[proc] );
      }
    }
#else
    // not USE_MINI_ITLB
  #ifdef DEBUG_PSEQ
    DEBUG_OUT("\tNOT using mini_tlb, now performing addr translation...\n");
  #endif

    bool miniTLB_hit = false;
    inTLB   = getInstr( apc->pc, apc->tl, apc->pstate,
                        fetchPhysicalPC_ptr, &next_instr, proc);

   #ifdef DEBUG_PSEQ
    DEBUG_OUT("\tfetchphysicalpc[%0x]\n",*fetchPhysicalPC_ptr);
    DEBUG_OUT("\tnext_instr[%x]\n",next_instr);
    DEBUG_OUT("\tproc[%x]\n",proc);
  #endif
#endif
    
    // (2)  find or construct decoded instruction
    if (!inTLB) {
      // instruction tlb miss
      
      // PC is not mapped by the current TLB ...
      // cause a trap on this instruction by fetching
      // a nop, and dynamically setting its trap flag

  #ifdef DEBUG_PSEQ
       DEBUG_OUT("\tInstruction TLB MISS, assigning the NOP instr... for proc[%d]\n",proc);
  #endif

      *s_instr_ptr = m_fetch_itlbmiss[proc];
    } else {
      // TLB hit
#ifndef REDECODE_EACH
      // read pre-decoded instruction from our cache
      isValid = queryInstruction( *fetchPhysicalPC_ptr, *s_instr_ptr, proc );
      if (isValid) {
           #ifdef DEBUG_PSEQ
          DEBUG_OUT("\tInstruction is valid and in Opal's Ipage cache!\n");
           #endif

          // instr is in tlb and is valid (in opal's ipage cache)
          //   if we read from simics, verify instruction hasn't changed.
          if ( !miniTLB_hit &&
               next_instr != (*s_instr_ptr)->getInst() ) {
            STAT_INC( m_stat_modified_instructions[proc] );    //means we have overwritten the next instr
            // INSTRUCTION_OVERWRITE
            /*DEBUG_OUT("overwrite vpc:0x%0llx pc:0x%0llx i:0x%0x new:0x%0x\n",
              apc->pc, *fetchPhysicalPC_ptr,
              (*s_instr_ptr)->getInst(), next_instr );
            */

              #ifdef DEBUG_PSEQ
                  DEBUG_OUT("\t\tWe need to update our Imap...(instruction in our Imap has changed!)\n");
              #endif

            *s_instr_ptr = insertInstruction( *fetchPhysicalPC_ptr,
                                              next_instr, proc );                               //update our imap
          }
      } else {
        // hit in ITLB cache, but missing a cached decoded instruction (in our imap).
        if (miniTLB_hit) {
            #ifdef DEBUG_PSEQ
            DEBUG_OUT("\tHit in our ITLB cache, but missing a cache decoded instr\n");
          #endif
          // read next instruction from simics
          inTLB = getInstr( apc->pc, apc->tl, apc->pstate,
                            fetchPhysicalPC_ptr, &next_instr, proc);
        }
        // add the instruction to opal's ipage cache
         #ifdef DEBUG_PSEQ
            DEBUG_OUT("\tHit in our ITLB cache, and inserting into our Imap...\n");
          #endif

        *s_instr_ptr = insertInstruction( *fetchPhysicalPC_ptr,
                                          next_instr, proc);

          #ifdef DEBUG_PSEQ
              char b[128];
              DEBUG_OUT("\tlookupinstruction: about to call printDisassemble, instrtype[%d]\n",
                                  (*s_instr_ptr)->getType());
              int len = (*s_instr_ptr)->printDisassemble(b);
              if(len >0){
                    DEBUG_OUT("\tlookupinstruction: Disassembled instr = %s\n",b);
               }
          #endif


      } // end if valid
#else
         #ifdef DEBUG_PSEQ
            DEBUG_OUT("\tWe are redecoding each static instr!\n");
          #endif
      miniTLB_hit = true;     // useless assignment to prevent warnings
      if (!inTLB) {
            #ifdef DEBUG_PSEQ
            DEBUG_OUT("\tITLB miss, assigning NOP\n");
          #endif
        *s_instr_ptr = m_fetch_itlbmiss[proc];
      } else {
            #ifdef DEBUG_PSEQ
            DEBUG_OUT("\tCalling static_inst_t constructor...\n");
          #endif
        static_inst_t *decoded_instr = new static_inst_t( *fetchPhysicalPC_ptr,
                                                          next_instr); 
        decoded_instr->setFlag( SI_TOFREE, true );

         #ifdef DEBUG_PSEQ
              char b[128];
              DEBUG_OUT("\tlookupinstruction: about to call printDisassemble, instrtype[%d]\n",
                                  (decoded_instr )->getType());
              int len = (decoded_instr)->printDisassemble(b);
              if(len >0){
                    DEBUG_OUT("\tlookupinstruction: Disassembled instr = %s\n",b);
               }
          #endif
        *s_instr_ptr = decoded_instr;
      }
#endif // ifndef REDECODE_EACH
    } // end if TLB miss
  } else {
    //
    // CONFIG_IN_SIMICS == false: running stand-alone
    //
    
    // translate virtual to physical address using (idealized) D-TLB
    // Each logical proc currently has its own standalone TLB
    bool inTLB = m_standalone_tlb[proc]->translateAddress( apc->pc, m_primary_ctx[proc], 4,
                                                     false,
                                                     fetchPhysicalPC_ptr );
    
    if (!inTLB) {
      // no translation information: fetch the infamous nop instruction
          #ifdef DEBUG_PSEQ
            DEBUG_OUT("\tNOT in our TLB, so fetching NOP!...\n");
          #endif

      *s_instr_ptr = m_fetch_itlbmiss[proc];
#if 0
      DEBUG_OUT("Trace driven simulation couldn't translate instruction!\n");
      DEBUG_OUT("vpc = 0x%0llx\n", apc->pc );
#endif
    } else {

      isValid = queryInstruction( *fetchPhysicalPC_ptr, *s_instr_ptr, proc );
      // if the fetch PC and the retire PC are the same...
      // fetch the "MOP" instruction
      //if (m_iwin[proc].getLastRetired() == m_iwin[proc].getLastFetched()) {
      if (!isValid) {

        // NOTE: in stand-alone, may want to report error (as all 
        //       non-speculative instructions should be in map)
        //       However, this could be a speculative instruction.
          #ifdef DEBUG_PSEQ
            DEBUG_OUT("\tIn our ITLB, but is NOT VALID, assigning NOP!\n");
          #endif
        *s_instr_ptr = m_fetch_itlbmiss[proc];
#if 0
        DEBUG_OUT("Trace driven simulation couldn't translate instruction!\n");
        ireg_t archtl    = m_control_arf->getRetireRF()->getInt(CONTROL_TL);
        ireg_t traplevel = apc->tl;
        DEBUG_OUT("vpc = 0x%0llx archtl = 0x%0llx futuretl = 0x%0llx\n", 
                 apc->pc, archtl, traplevel );
        *s_instr_ptr = m_fetch_itlbmiss[proc];
#endif
      }
    }
  } // end not in simics
  ASSERT( *s_instr_ptr != NULL );

  #ifdef DEBUG_PSEQ
      printPC((abstract_pc_t *)apc);
      char b[128];
      DEBUG_OUT("\tlookupinstruction: about to call printDisassemble, instrtype[%d]\n",
                (*s_instr_ptr)->getType());
      int len = (*s_instr_ptr)->printDisassemble(b);
       if(len >0){
           DEBUG_OUT("\tlookupinstruction: Disassembled instr = %s\n",b);
       }
      DEBUG_OUT("pseq_t:lookupinstruction END m_id[%d] proc[%d]\n",m_id,proc);
  #endif
}

/** Task: Fetch an instruction, insert it in the instruction window,
 *        Tag this instruction as Fetched,
 *        Advance PC/NPC state contained in fetch_at speculatively.
 */
//**************************************************************************
dynamic_inst_t* pseq_t::createInstruction( static_inst_t *s_instr,
                                           abstract_pc_t *fetch_at, pa_t physicalPC, unsigned int proc ) {
  #ifdef DEBUG_PSEQ
   DEBUG_OUT("pseq_t:createInstruction BEGIN m_id[%d] proc[%d]\n",m_id,proc);
   printPC(m_fetch_at[proc]);
  #endif

  ASSERT(proc < CONFIG_LOGICAL_PER_PHY_PROC);

  dynamic_inst_t *d_instr = NULL;
  int32 index = m_iwin[proc].getInsertIndex();

  ASSERT(s_instr != NULL);
  #ifdef DEBUG_PSEQ
  char b[128];
  int len = s_instr->printDisassemble(b);
  if(len >0){
    DEBUG_OUT("\tcreateInstruction: Disassembled instr = %s\n",b);
  }
  DEBUG_OUT("\tcreateInstruction index[%d] proc[%d]\n",index,proc);
  #endif

  STAT_INC( m_stat_fetched[proc] );

  // create a new dynamic instruction ... of the correct type
#ifdef DEBUG_PSEQ
   DEBUG_OUT("\tabout to execute switch statement, instr_type[%d] proc[%d]\n",s_instr->getType(),proc);
#endif
  switch ( s_instr->getType() ) {
    case DYN_EXECUTE:
       #ifdef DEBUG_PSEQ
             DEBUG_OUT("\tDYN_EXECUTE type proc[%d]\n",proc);
       #endif
      d_instr = new dynamic_inst_t(s_instr, index, this, fetch_at, physicalPC, m_last_traptype[proc][m_last_traplevel[proc]], proc);
       #ifdef DEBUG_PSEQ
             DEBUG_OUT("\tDYN_EXECUTE creation SUCCESS proc[%d]\n",proc);
       #endif                        
      // an i-tlb miss occured on this instruction ...
      if (s_instr == m_fetch_itlbmiss[proc] ) {
        d_instr->setTrapType( Trap_Fast_Instruction_Access_MMU_Miss );
      }
      break;

    case DYN_CONTROL:
        #ifdef DEBUG_PSEQ
             DEBUG_OUT("\tDYN_CONTROL type proc[%d]\n",proc);
       #endif
      d_instr = new control_inst_t(s_instr, index, this, fetch_at, physicalPC,  m_last_traptype[proc][m_last_traplevel[proc]], proc);
        #ifdef DEBUG_PSEQ
             DEBUG_OUT("\tDYN_CONTROL creation success proc[%d]\n",proc);
       #endif
      break;
            
    case DYN_LOAD:
        #ifdef DEBUG_PSEQ
             DEBUG_OUT("\tDYN_LOAD type proc[%d]\n",proc);
       #endif
      d_instr = new load_inst_t(s_instr, index, this, fetch_at, physicalPC,  m_last_traptype[proc][m_last_traplevel[proc]], proc);
         #ifdef DEBUG_PSEQ
             DEBUG_OUT("\tDYN_LOAD creation success proc[%d]\n",proc);
       #endif
      break;

    case DYN_STORE:
        #ifdef DEBUG_PSEQ
             DEBUG_OUT("\tDYN_STORE type proc[%d]\n",proc);
       #endif
      d_instr = new store_inst_t(s_instr, index, this, fetch_at, physicalPC,  m_last_traptype[proc][m_last_traplevel[proc]], proc);
         #ifdef DEBUG_PSEQ
             DEBUG_OUT("\tDYN_STORE creation success proc[%d]\n",proc);
       #endif
      break;

    case DYN_PREFETCH:
        #ifdef DEBUG_PSEQ
             DEBUG_OUT("\tDYN_PREFETCH type proc[%d]\n",proc);
       #endif
      d_instr = new prefetch_inst_t(s_instr, index, this, fetch_at, physicalPC,  m_last_traptype[proc][m_last_traplevel[proc]], proc);
         #ifdef DEBUG_PSEQ
             DEBUG_OUT("\tDYN_PREFETCH creation success proc[%d]\n",proc);
       #endif
      break;

    case DYN_ATOMIC:
        #ifdef DEBUG_PSEQ
             DEBUG_OUT("\tDYN_ATOMIC type proc[%d]\n",proc);
       #endif
      d_instr = new atomic_inst_t(s_instr, index, this, fetch_at, physicalPC,  m_last_traptype[proc][m_last_traplevel[proc]], proc);
         #ifdef DEBUG_PSEQ
             DEBUG_OUT("\tDYN_ATOMIC creation success proc[%d]\n",proc);
       #endif
      break;

    default:
      ERROR_OUT("error: unimplemented instruction type %d\n", 
                (int) s_instr->getType());
      SIM_HALT;
      break;
  }

  ASSERT( d_instr != NULL );
#ifdef PIPELINE_VIS
  char            buffer[128];
  s_instr->printDisassemble( buffer );
  // print out the instruction for later vis
  out_log("instr %d 0x%12.12llx 0x%8.8x \"%s\"\n",
          index, m_fetch_at[proc]->pc, s_instr->getInst(), buffer);
  out_log("cwp %d gset %d\n", m_fetch_at[proc]->cwp, m_fetch_at[proc]->gset);
  /* DEBUG_OUT("instr %d 0x%12.12llx 0x%8.8x \"%s\"\n",
   *          index, m_fetch_at->pc, s_instr->getInst(), buffer);
   */
#endif
      
  // Branch prediction: advance the program counter by calling member function "nextPC". 
  pmf_nextPC pmf = s_instr->nextPC;
  (d_instr->*pmf)( fetch_at );
  

  #ifdef DEBUG_PSEQ
  //printPC(&m_ooo.at[proc]);
  printPC(m_fetch_at[proc]);
    DEBUG_OUT("pseq_t:createInstruction END m_id[%d] proc[%d]\n",m_id,proc);
  #endif

  // CM AddressMask64 AM64 ?
  return d_instr;
}

/** Decodes instructions which have been fetched.
 *
 *  Decode does the register renaming on the source and destination registers,
 *  set its sequence number (a unique number-- a.k.a. priority).
 */
//**************************************************************************
void pseq_t::decodeInstruction( )
{
  #ifdef DEBUG_PSEQ
     DEBUG_OUT("pseq_t:decodeInstruction BEGIN m_id[%d] cycle[ %d ]\n",m_id,getLocalCycle());
     //printPC(&m_ooo.at[proc]);
  #endif

  uint32 total_decoded = 0;   //number of instrs decoded for all logical procs
  uint32 timeout = 0;
  // For SMT, implement Round - Robin decoding of instructions
  //  first, set up the arrays:
  uint32 num_decoded[CONFIG_LOGICAL_PER_PHY_PROC];
  uint32 decode_avail[CONFIG_LOGICAL_PER_PHY_PROC];
  uint32 decode_remaining[CONFIG_LOGICAL_PER_PHY_PROC];
  for(uint proc=0; proc <  CONFIG_LOGICAL_PER_PHY_PROC; ++proc){
    num_decoded[proc] = 0;
    decode_avail[proc] = m_fetch_per_cycle[proc].peekCurrent( getLocalCycle() );
    decode_remaining[proc] = decode_avail[proc];
  }

  while(continueDecode(total_decoded, decode_remaining)){
    ASSERT(timeout <= 300);
    for(uint proc=0; proc <  CONFIG_LOGICAL_PER_PHY_PROC && total_decoded < MAX_DECODE; ++proc){

      timeout++;

      #ifdef DEBUG_PSEQ
         DEBUG_OUT("pseq_t:decodeInstruction BEGIN m_id[%d] proc[%d] cycle %d\n",m_id,proc,getLocalCycle());
         DEBUG_OUT("\ttotal_decoded[%d]\n",total_decoded); 
         //printPC(&m_ooo.at[proc]);
         printPC(m_fetch_at[proc]);
      #endif

#ifdef DEBUG_PSEQ
   DEBUG_OUT( "\tDecoding...%d instructions available proc[%d]\n", decode_remaining[proc],proc);
#endif
   uint32 decoded_this_cycle = 0;
   uint32 decode_remaining_temp = decode_remaining[proc];
   while ( (decoded_this_cycle < decode_remaining_temp) &&
           (decoded_this_cycle < m_max_decode_per_cycle) &&
           (total_decoded < MAX_DECODE) &&
           m_ooo.regs.registersAvailable(proc) ) {

         // decode the appropriate instructions in the instruction window
        dynamic_inst_t *d_instr = m_iwin[proc].decodeInstruction();
     
        STAT_INC( m_stat_decoded[proc] );
        ASSERT(d_instr != NULL);
        d_instr->Decode( getLocalCycle() );
        num_decoded[proc]++;
        decoded_this_cycle++;
        decode_remaining[proc]--;
        total_decoded++;

       }
   //track how many times we have run out of registers
   if(!m_ooo.regs.registersAvailable(proc)){
     STAT_INC(m_stat_not_enough_registers[proc]);
   }
   
   #ifdef DEBUG_PSEQ
    DEBUG_OUT("\tTotal decoded = %d\n",total_decoded);
    DEBUG_OUT("\tDecode remaining[%d] = %d\n",proc, decode_remaining[proc]);
   #endif
    }  // end for loop over all logical procs
  }  // end continueDecode()

  ASSERT( total_decoded <= MAX_DECODE);

  // did not decode all available instructions, add remaining to next cycle
  for(uint proc=0; proc <  CONFIG_LOGICAL_PER_PHY_PROC; ++proc){
    ASSERT(num_decoded[proc] + decode_remaining[proc] == decode_avail[proc]);
    if (num_decoded[proc] < decode_avail[proc]) {
         #ifdef DEBUG_PSEQ
           DEBUG_OUT("\tthere are leftover instrs avail[%d] decoded[%d] leftover[%d] proc[%d] cycle %d\n", 
          decode_avail[proc], num_decoded[proc], decode_avail[proc]-num_decoded[proc], proc,getLocalCycle());
         #endif

         int leftover = decode_avail[proc] - num_decoded[proc];
         ASSERT( leftover >= 0 );
         ASSERT(leftover <= m_iwin[proc].getNumCanDecode());   //sanity check

         /* get number of instruction ready to be decoded next cycle
          * add leftover number up to that
          */
        uint32 numfetched = m_fetch_per_cycle[proc].peekItem( getLocalCycle(), 1 );
        #ifdef DEBUG_PSEQ
            DEBUG_OUT("\tinserting leftovers leftovers[%d] proc[%d] in cycle[%d]\n",
                           numfetched+leftover, proc,getLocalCycle()+1);
         #endif

        m_fetch_per_cycle[proc].insertItem( getLocalCycle(), 1, numfetched + leftover );
    
        m_reg_stall_count_stat[proc]++;
        m_decode_stall_count_stat[proc] += leftover;
    }
    else {  //reset the number of fetched for this cycle (ie we have no more to decode for this cycle)
      ASSERT(decode_remaining[proc] == 0);   //sanity check
      m_fetch_per_cycle[proc].insertItem( getLocalCycle(), 0, 0);
    }

    ASSERT( num_decoded[proc] <= MAX_DECODE );
   
    //record per-thread decode histogram
    m_hist_decode_per_thread[proc][num_decoded[proc]]++;

    // insert the number of instructions not decoded this cycle to next cycle
    #ifdef DEBUG_PSEQ
        DEBUG_OUT("\tdecode: adding %d instructions to be scheduled at cycle %d\n",num_decoded[proc],
                            getLocalCycle()+DECODE_STAGES);
    #endif
    m_decode_per_cycle[proc].insertItem( getLocalCycle(), DECODE_STAGES,
                                             num_decoded[proc] );

#ifdef DEBUG_PSEQ
   DEBUG_OUT("num decoded %d\n", num_decoded[proc]);
#endif

  }  //end for loop over all logical procs

  //update per-core decode histogram:
  m_hist_decode_per_cycle[total_decoded]++;

 #ifdef DEBUG_PSEQ
  //printPC(&m_ooo.at[proc]);
  DEBUG_OUT("pseq_t:decodeInstruction END m_id[%d] cycle[ %d ]\n",m_id,getLocalCycle());
 #endif
}

/** Schedules instructions which have been decoded on the functional
 *  units in the processor.
 *
 *  This function takes instructions from the decoder and identifies
 *  whether they are ready for execution.  If they are ready, they
 *  are passed on to the (centralized) scheduler associated with this
 *  sequencer.  Otherwise they are stuck in a waiting list for one of
 *  their un-ready register values.  (When the register value becomes
 *  ready they are rescheduled.
 */
//**************************************************************************
void pseq_t::scheduleInstruction( )
{
  #ifdef DEBUG_PSEQ
    DEBUG_OUT("pseq_t:scheduleInstruction BEGIN m_id[%d] cycle[ %lld ]\n",m_id,m_local_cycles);
    //printPC(&m_ooo.at[proc]);
   #endif

  uint32 total_scheduled = 0;
  uint32 timeout = 0;
  //first, set up the variables:
  uint32 num_scheduled[CONFIG_LOGICAL_PER_PHY_PROC];
  uint32 schedule_avail[CONFIG_LOGICAL_PER_PHY_PROC];
  uint32 sched_remaining[CONFIG_LOGICAL_PER_PHY_PROC];
  bool done[CONFIG_LOGICAL_PER_PHY_PROC];
  for(uint proc=0; proc <  CONFIG_LOGICAL_PER_PHY_PROC; ++proc){
    done[proc] = false;                      //have we exceeded the scheduling window size?
    num_scheduled[proc] = 0;
    schedule_avail[proc] = m_decode_per_cycle[proc].peekCurrent( getLocalCycle() );
    sched_remaining[proc] = schedule_avail[proc];
  }

  while(continueSchedule(total_scheduled, sched_remaining, done)){
    ASSERT(timeout <= 300);
    for(uint proc=0; proc <  CONFIG_LOGICAL_PER_PHY_PROC && total_scheduled < MAX_DISPATCH; ++proc){

      timeout++;

        #ifdef DEBUG_PSEQ
           DEBUG_OUT("pseq_t:scheduleInstruction BEGIN m_id[%d] proc[%d] cycle[ %lld ]\n",m_id,proc, m_local_cycles);
           //printPC(&m_ooo.at[proc]);
           printPC(m_fetch_at[proc]);
       #endif

#ifdef DEBUG_PSEQ  
     DEBUG_OUT("scheduling ...%d  instructions available\n", schedule_avail[proc]);
#endif

     /* This function schedules exactly the number of decoded instruction
      * available. It is equivalent to the "issue/dispatch" stage in the
      * processor. Instructions are executed OoO with m_schedule.Execute().
      * So their state in i_window changes from E->C OoO. And C->O in retire
      * stage.
      *
      * m_iwin.scheduleInstruction() return a valid instruction if
      * the instruction window is not full.(Even ROB entries still
      * available.
      */
     // get instructions which can be scheduled now... see if they are ready
     uint32 sched_remaining_temp = sched_remaining[proc];
     for (uint32 scheduled_this_cycle = 0;
           scheduled_this_cycle < sched_remaining_temp && 
            scheduled_this_cycle < m_max_dispatch_per_cycle &&
             total_scheduled < MAX_DISPATCH;
           num_scheduled[proc]++, total_scheduled++, sched_remaining[proc]--, scheduled_this_cycle++) {
       dynamic_inst_t *d_instr = m_iwin[proc].scheduleInstruction();
       if (!d_instr){
         done[proc] = true;         //this logical proc has exceeded its scheduling window
         //track how many times scheduling window is exceeded
         STAT_INC(m_stat_exceed_scheduling_window[proc]);
         break;
       }
       d_instr->testSourceReadiness();
       d_instr->Schedule();

       //decrement per thread ICount
       m_icount[proc]--;
       ASSERT(m_icount[proc] >= 0);
     }
    }  // end for loop over all logical procs
  }  // end continueSchedule()

 ASSERT( total_scheduled <= MAX_DISPATCH);

  // did not schedule all available instructions, add remaining to next cycle
 for(uint proc=0; proc <  CONFIG_LOGICAL_PER_PHY_PROC; ++proc){
   ASSERT(num_scheduled[proc] + sched_remaining[proc] == schedule_avail[proc]);
   if (num_scheduled[proc] < schedule_avail[proc]) {
     int leftover = schedule_avail[proc] - num_scheduled[proc];
     ASSERT( leftover >= 0 );
     ASSERT(leftover <= m_iwin[proc].getNumCanSched());     //sanity check

     /* get number of instruction ready to be scheduled next cycle
      * add leftover number up to that
      */
     uint32 numdecoded = m_decode_per_cycle[proc].peekItem( getLocalCycle(), 1 );
     m_decode_per_cycle[proc].insertItem( getLocalCycle(), 1, numdecoded + leftover );
     
     m_iwin_stall_count_stat[proc]++;
     m_schedule_stall_count_stat[proc] += leftover;
   }
   else{
     ASSERT(sched_remaining[proc] == 0);   //sanity check
   }
   
   ASSERT( num_scheduled[proc] <= MAX_DISPATCH );
   //ASSERT( total_scheduled <= MAX_DISPATCH*CONFIG_LOGICAL_PER_PHY_PROC);
  
#ifdef DEBUG_PSEQ
    DEBUG_OUT("num scheduled %d\n", num_scheduled[proc]);
#endif

    //record per-thread schedule histogram:
    m_hist_schedule_per_thread[proc][num_scheduled[proc]]++;

  }  //end for loop over all logical procs
 //record per-core schedule histogram:
 m_hist_schedule_per_cycle[total_scheduled]++;

 #ifdef DEBUG_PSEQ
       //printPC(&m_ooo.at[proc]);
        DEBUG_OUT("pseq_t:scheduleInstruction END m_id[%d] cycle[ %lld ]\n",m_id, m_local_cycles);
  #endif
}

/** Retires instructions from the instruction window.
 */
//**************************************************************************
void pseq_t::retireInstruction( )
{
  #ifdef DEBUG_PSEQ
    DEBUG_OUT("pseq_t:retireInstruction BEGIN m_id[%d] cycle[%d]\n",m_id, getLocalCycle());
    //printPC(&m_ooo.at[proc]);
   #endif

    // for DEBUG purposes
    if( 0 && (m_local_cycles >= 100000) && (m_local_cycles % 30000 == 0)){
      printFetchDebug();
      //print();
      //m_ruby_cache->print();
    }

  uint total_retired = 0;
  uint32 timeout = 0;
  // first, initialize the variables:
  uint32 retire_count[CONFIG_LOGICAL_PER_PHY_PROC];
  pseq_retire_status_t status[CONFIG_LOGICAL_PER_PHY_PROC];
  bool done[CONFIG_LOGICAL_PER_PHY_PROC];
  for(uint proc=0; proc <  CONFIG_LOGICAL_PER_PHY_PROC; ++proc){ 
    retire_count[proc] = 0;
    done[proc] = false;                                        // indicates if this proc is done with retiring
    status[proc] = PSEQ_RETIRE_READY;
  }

  while(continueRetire(total_retired, done)){
    ASSERT(timeout <= 300);
    for(uint proc=0; proc <  CONFIG_LOGICAL_PER_PHY_PROC && total_retired < MAX_RETIRE; ++proc){ 
      timeout++;

          #ifdef DEBUG_PSEQ
            DEBUG_OUT("pseq_t:retireInstruction BEGIN m_id[%d] proc[%d] cycle[%d]\n",m_id,proc,getLocalCycle());
            //printPC(&m_ooo.at[proc]);
            printPC(m_fetch_at[proc]);
          #endif
          
          // control variables for the retirement check
          check_result_t check_result;      // contains the result of retirement check
          bool           squash_pipeline;   // true if the pipeline must be squashed
          bool           must_check;        // true if the retired instr must be
                                                    // checked (unimplemented instructions
                                                    // for example)
          bool           must_update_all = false; // true if non-perfect instructions
                                                  // will cause a complete state reload
          bool           step_ok;           // true if the next step was readable

          // set whenever we fastforwarded Opal past trap handler
          bool          fastforward_opal = false;

          uint32 retired_this_cycle = 0;            //how many instrs this proc retired this round
          /* if there is an outstanding exception, squash and refetch */

          //reset the retirement status each retirement round
          status[proc] = PSEQ_RETIRE_READY;                 //status of the retire stage

          ASSERT(m_except_type != NULL);

          if (m_except_type[proc] != EXCEPT_NONE) {
#ifdef PIPELINE_VIS
    out_log("exception 0x%0llx 0x%0llx %s\n", 
            m_except_continue_at[proc].pc, m_except_continue_at[proc].npc,
            pstate_t::async_exception_menomic(m_except_type[proc]) );
#endif
               partialSquash(m_next_exception[proc], &(m_except_continue_at[proc]), m_except_offender[proc], proc, false);
               clearException(proc);
          }
  
          // detect deadlock- issue a forward progress error if it takes too long
          // between retiring instructions
          if ( getLocalCycle() > m_fwd_progress_cycle ) {
            ERROR_OUT( "*** Lack of forward progress detected ***\n" );
            ERROR_OUT( "    Cycle = %d\n", getLocalCycle() );
            ERROR_OUT("     Fwd Progress Cycle = %d\n", m_fwd_progress_cycle);
            ERROR_OUT("     Last retired cycle = %d\n", getLocalCycle() -  (1 << 22));
            ERROR_OUT("*** Lack of forward progress detected ***\n");
            ERROR_OUT("WB Full stall start cycle = %d\n", m_wb_stall_start_cycle);
            ERROR_OUT("WB Full stall end cycle = %d\n", m_wb_stall_end_cycle);
            // end debug filtering if there is an error
            setDebugTime( 0 );
            print();
            m_write_buffer->print();
            ERROR_OUT("\n");
            m_ruby_cache->print();
            ERROR_OUT("\n");
            HALT_SIMULATION;
            return;
          }


          // NOTE: Any conditional statements to stop retirement for the current SMT thread 
          // should be _before_ the retireInstruction() call to the iwindow, as it
          //   WILL ADVANCE the m_last_retired pointer!!

          /* Retire as many instructions which are ready to retire */
          dynamic_inst_t *d  = m_iwin[proc].retireInstruction();
          enum i_opcode   op = i_mop;
          uint64 seqnum = 0;

          if(d == NULL){
            #ifdef DEBUG_PSEQ
               DEBUG_OUT("\t retireInstruction, instruction pointer is NULL (done retiring)\n");
            #endif
            done[proc] = true;        //we should stop retiring immediately, and go to the next thread
          }
          while ( d != NULL ) {

            /* WATTCH power */
            if(WATTCH_POWER){
              getPowerStats()->incrementWindowAccess();
            }
            squash_pipeline  = false;
            must_check       = false;
            fastforward_opal = false;
            static_inst_t *s = d->getStaticInst();
            op = (enum i_opcode) s->getOpcode();
            seqnum = d->getSequenceNumber();

            // CM FIX:
            //   At this point, the inorder_at.pc may not be masked correctly so
            //   this can incorrectly generate a warning. It should be masked,
            //   and this check can be put back in.
#if 0
    if (getVPC() != m_inorder_at[proc].pc &&
        getVPC() != (my_addr_t) -1) {
      // warning -- not retiring the correct instruction ...
      DEBUG_OUT("retireInstruction: warning PC at retire differs 0x%0llx 0x%0llx\n", getVPC(), m_inorder_at[proc].pc );
    }
#endif

    //*************** DEBUG: print out the retiring instruction
    physical_file_t *control_state_rf = NULL;
    control_state_rf = m_control_arf->getRetireRF(proc);
    ireg_t tba;
    ireg_t tl;
    ireg_t pstate;
    ireg_t cwp;
    ireg_t gset;
     #ifdef DEBUG_DYNAMIC
    //only print out those instruction that are NOT in trap handler
    if(d->getTrapGroup() == Trap_NoTrap){
      char buf[128];
      s->printDisassemble(buf);
      DEBUG_OUT("[ %d ] RETIRE ", m_id);
      uint64 fetchTime = d->getFetchTime();
      if(s->getType() == DYN_LOAD || s->getType() == DYN_STORE || s->getType() == DYN_ATOMIC || s->getType() == DYN_PREFETCH){
        //print out memory instructions
        DEBUG_OUT("%s seqnum[ %d ] VPC[ 0x%llx ] PC[ 0x%llx ] virtual_addr[ 0x%llx ] physical_addr[ 0x%llx ] lineaddr[ 0x%llx ] iscacheable[ %d ]", buf,d->getSequenceNumber(), d->getVPC(), d->getPC(), (static_cast<memory_inst_t *>(d)->getAddress()), static_cast<memory_inst_t *>(d)->getPhysicalAddress(), ((static_cast<memory_inst_t *> (d)->getPhysicalAddress()) & ~( (1 << 6) - 1) ), static_cast<memory_inst_t *>(d)->isCacheable() );
        //print out ASI, if this instruction is not NAV
        if(d->getInstrNAV() == false){
          DEBUG_OUT(" ASI[ 0x%x ]", static_cast<memory_inst_t *>(d)->getASI());
        }
        //print out data
        int access_size_8bytes = ( (static_cast<memory_inst_t *>(d)->getAccessSize()) + 7)/8;
        DEBUG_OUT(" Data (%d bytes) [ ", static_cast<memory_inst_t *>(d)->getAccessSize());
        ireg_t * data_storage = static_cast<memory_inst_t *>(d)->getData();
        for(int i=0; i < access_size_8bytes; ++i){
          DEBUG_OUT("0x%llx ", data_storage[i]);
        }
        DEBUG_OUT("]");
        if(d->getEvent(EVENT_L2_MISS)){
          DEBUG_OUT("  L2MISS");
        }
        else if(d->getEvent(EVENT_DCACHE_MISS)){
          DEBUG_OUT("  L1MISS");
        }
        else{
          DEBUG_OUT("  L1HIT");
        }
      }
      else{
        //print out non-mem instructions
        DEBUG_OUT("%s seqnum[ %d ] VPC[ 0x%llx ] PC[ 0x%llx ]", buf,d->getSequenceNumber(), d->getVPC(), d->getPC());
      }
      //print out event times and traps
      DEBUG_OUT(" fetched[ %lld ] decode[ %lld ] execute[ %lld ] cont_execute[ %lld ] execute_done[ %lld ] current_cycle[ %lld ] traptype[ 0x%x ]", d->getFetchTime(), d->getFetchTime()+d->getEventTime(EVENT_TIME_DECODE), d->getFetchTime()+d->getEventTime(EVENT_TIME_EXECUTE), d->getFetchTime()+d->getEventTime(EVENT_TIME_CONTINUE_EXECUTE), d->getFetchTime()+d->getEventTime(EVENT_TIME_EXECUTE_DONE), m_local_cycles, d->getTrapType());
      //print source and dest regs
      DEBUG_OUT(" SOURCES: ");
      for(int i=0; i < SI_MAX_SOURCE; ++i){
        reg_id_t & source = d->getSourceReg(i);
        if(!source.isZero()){
          DEBUG_OUT("( [%d] V: %d P: %d Arf: %s NAV: %d )", i,source.getVanilla(), source.getPhysical(), source.rid_type_menomic( source.getRtype() ), source.getARF()->getNAV(source, d->getProc()) );
        }
      }
      DEBUG_OUT(" DESTS: ");
      for(int i=0; i < SI_MAX_DEST; ++i){
        reg_id_t & dest = d->getDestReg(i);
        if(!dest.isZero()){
          DEBUG_OUT("( [%d] V: %d P: %d Arf: %s NAV: %d )", i,dest.getVanilla(), dest.getPhysical(), dest.rid_type_menomic( dest.getRtype() ), dest.getARF()->getNAV(dest, d->getProc()) );
        }
      }
      DEBUG_OUT("\n");
      //print out TL, CWP, GSET, PSTATE
      //pulled from takeTrap()
      tba     = control_state_rf->getInt( CONTROL_TBA );
      tl      = control_state_rf->getInt( CONTROL_TL );
      pstate  = control_state_rf->getInt( CONTROL_PSTATE );
      cwp     = control_state_rf->getInt( CONTROL_CWP );
      gset = pstate_t::getGlobalSet( pstate );
      DEBUG_OUT("\tBEGIN TL[ %d ] CWP[ 0x%x ] GSET[ 0x%x ] PSTATE[ 0x%x ]\n", tl, cwp, gset, pstate);
    }  //end instrs NOT in trap handler
    #endif
  //*****************END DEBUG instruction print

  //********************************************************************************
  // Keep track of which traps get completed 
    if( d->getStaticInst()->getOpcode() == i_retry || d->getStaticInst()->getOpcode() == i_done){
      if( d->getEvent( EVENT_BRANCH_MISPREDICT) == false){
        //IMPORTANT: DO NOT update stats for mispredicted retries!
        if( m_last_traptype[proc][m_last_traplevel[proc]] != Trap_NoTrap){
          m_completed_trapstat[proc][m_last_traptype[proc][m_last_traplevel[proc]]]++;
        }
      }
    }
  //*******************************************************************************

    /*
     * Task: retire the instruction, unless it causes a trap.
     */
    // The traptype of the retiring instruction
    trap_type_t traptype = d->getTrapType();
    if ( traptype == Trap_NoTrap ) {
      // Retire the instruction, unless single stepping simics
      // (we detect interrupts, and retire if single stepping)
      if ( PSEQ_MAX_UNCHECKED != 1) {
         #ifdef DEBUG_DYNAMIC_RET
            DEBUG_OUT("calling Retire (1) proc[%d]\n",proc);
         #endif
            dyn_execute_type_t type = d->getStaticInst()->getType();            
            d->Retire( m_inorder_at[proc] );
      }
    } else if ( traptype == Trap_Use_Functional ) {
      must_check = true;
      m_opstat[proc].functionalOp( op );          
      STAT_INC( m_stat_count_functionalretire[proc] );
    }
    else {
      if ( PSEQ_MAX_UNCHECKED != 1) {
        must_check = takeTrap( d, m_inorder_at[proc], true, proc );   
        squash_pipeline = true;
      } // else UNCHECKED == 1 (must_check is irrelevant,as it will be checked)

      // REGISTER FILL/SPILL DEBUG OUTPUT...
      // Spills are trapnums 0x80 - 0xbc
      if( 0 && (traptype >= Trap_Spill_0_Normal) && (traptype <= Trap_Spill_7_Other ) ){
        #ifdef DEBUG_PSEQ
          DEBUG_OUT("retirement SPILL traptype[ 0x%x ] cycle[ %lld ]\n", traptype, m_local_cycles);
        #endif        
      }
      if( 0 && (traptype >= Trap_Fill_0_Normal) && (traptype <= Trap_Fill_7_Other) ){
        #ifdef DEBUG_PSEQ
          DEBUG_OUT("retirement FILL traptype[ 0x%x ] cycle[ %lld ]\n", traptype, m_local_cycles);
        #endif
      }
    }   //end else

    // Add the instruction to the list of 'unchecked retires'
    // and update the forward progress cycle.
    must_check = must_check | uncheckedRetire( d, proc );   //do this per logical processor
    m_fwd_progress_cycle = getLocalCycle() + (1 << 22);
    
    /*
     * TASK: compare our results with SimIcs's results
     */
    if (must_check) {
       #ifdef DEBUG_PSEQ
        DEBUG_OUT("[ %d ] retireInstruction: before calling advanceSimics\n", m_id);
       #endif
       step_ok = advanceSimics(proc);    //advance Simics one logical processor at a time
       #ifdef DEBUG_PSEQ
           DEBUG_OUT("[ %d ] retireInstruction: after calling advanceSimics, stepok = %d\n",m_id, step_ok);
       #endif
       if (step_ok) {
        ASSERT(m_posted_interrupt != NULL);

        if (PSEQ_MAX_UNCHECKED == 1) {
          // if no interrupts were posted, retire 'd' normally
          if ( m_posted_interrupt[proc] == Trap_NoTrap ) {    
            if (traptype == Trap_NoTrap) {
              d->Retire( m_inorder_at[proc] );
            } else if (traptype == Trap_Use_Functional) {
              // read register from simics
              updateInstructionState( d, proc );         
              d->Retire( m_inorder_at[proc] );
            } else {
              if (TLB_IS_IDEAL &&
                  ((traptype == Trap_Fast_Instruction_Access_MMU_Miss) ||
                   (traptype == Trap_Fast_Data_Access_MMU_Miss))) {
                // fastforward through this trap
                fastforwardSimics(proc);
                // squash the pipeline
                partialSquash(m_iwin[proc].getLastRetired(), m_inorder_at[proc], op, proc, false);
                d->Squash();
                squash_pipeline = true;
                must_update_all = true;
              } else {
                // take the trap
                takeTrap( d, m_inorder_at[proc], true, proc );
                squash_pipeline = true;
              }
            }
          } else {
            // interrupt was posted
            #ifdef DEBUG_PSEQ
                DEBUG_OUT("\tSIMICS POSTED INTERRUPT 0x%x cycle[ %lld ]\n", m_posted_interrupt[proc], m_local_cycles);
            #endif
            d->setTrapType( m_posted_interrupt[proc] );
            takeTrap( d, m_inorder_at[proc], true, proc );
            squash_pipeline = true;

            // This final else clause used to cause a small number of
            // retiring (stores mostly) to fail. If the store
            // experiences a TLB miss, _and_ an interrupt occurs,
            // the machine model specifies that the interrupt wins.
            // However, we would have squashed and taken the trap
            // (i.e. the interrupt is not detected until after retirement)
            // so in this case, this instruction will deviate.
            // I corrected this by making "takeTrap" not execute above
            // when MAX_UNCHECKED == 1.

            // reset posted interrupt
            m_posted_interrupt[proc] = Trap_NoTrap;
          }
          
        } else {  // PSEQ_MAX_UNCHECKED != 1
          // for bundled retires, update state after stepping simics
          if (traptype == Trap_Use_Functional) {
            // read register from simics
            updateInstructionState( d, proc );       
            d->Retire( m_inorder_at[proc] );
          }
        }

        //   do the commit check for the important (PC, PSTATE) registers
        check_result.perfect_check = true;
        check_result.critical_check = true;

        checkCriticalState( &check_result, &(m_ooo), proc );
        if (!check_result.critical_check) {
          STAT_INC( m_stat_count_badretire[proc] );
          must_update_all = true;
        }

        //   do the commit check each unchecked instruction
        dynamic_inst_t *dcheck = getNextUnchecked(proc);
#ifdef DEBUG_FUNCTIONALITY
        if (!check_result.critical_check) {
          DEBUG_OUT("critial check fails time: %lld\n", getLocalCycle() );
          dcheck->printDetail();
        }
#endif
        while (dcheck != NULL) {

          // DEBUG - print out all retired loads/stores
          static_inst_t * s = dcheck->getStaticInst();
          memory_inst_t * memop = static_cast<memory_inst_t *>(dcheck);
          if( 0 && m_local_cycles >= 100000 && (s->getType() == DYN_LOAD || s->getType() == DYN_ATOMIC || s->getType() == DYN_STORE)){
            char buf[128];
            s->printDisassemble(buf);
            if(memop->isCacheable()){
              if(memop->getPhysicalAddress() == (pa_t) -1){
                if(dcheck->getTrapType() == Trap_Unimplemented){
                  printf("%lld [%d, %d] %s UNIMPLEMENTED VA: 0x%llx VPC: 0x%llx PC: 0x%llx\n", m_local_cycles, m_id, proc, buf, memop->getAddress(), dcheck->getVPC(), dcheck->getPC());
                }
                else{
                  printf("%lld [%d, %d] %s TLB MISS VA: 0x%llx VPC: 0x%llx PC: 0x%llx\n", m_local_cycles, m_id, proc, buf, memop->getAddress(), dcheck->getVPC(), dcheck->getPC());
                }
              }
              else if(memop->isIOAccess(memop->getPhysicalAddress())){
                printf("%lld [%d, %d] %s is IO ACCESS VA: 0x%llx VPC: 0x%llx PC: 0x%llx\n", m_local_cycles, m_id, proc, buf, memop->getAddress(), dcheck->getVPC(), dcheck->getPC());
              }
              else{
                //regular cacheable load/store
                printf("%lld [%d, %d] %s VA: 0x%llx PA: 0x%llx VPC: 0x%llx PC: 0x%llx\n", m_local_cycles, m_id, proc, buf, memop->getAddress(), memop->getPhysicalAddress(), dcheck->getVPC(), dcheck->getPC());
              }
            }
            else{
              printf("%lld [%d, %d] %s is NON-CACHEABLE VA: 0x%llx VPC: 0x%llx PC: 0x%llx\n", m_local_cycles, m_id, proc, buf, memop->getAddress(), dcheck->getVPC(), dcheck->getPC());
            }
          }
          // end DEBUG


          op = (enum i_opcode) dcheck->getStaticInst()->getOpcode();

          // if stat committed isn't incremented, the simulation will never
          // terminate (other stats should be able to be turned off however)
          // STAT_INC( m_stat_committed );
          m_stat_committed[proc]++;
          ASSERT( dcheck->getEvent( EVENT_FINALIZED ) == true );

          // send this instruction to the committed instruction stream proc
          commitObserver( dcheck );

          // do a commit check for all unchecked instructions
#ifdef PIPELINE_VIS
          checkAllState( &check_result, &(m_ooo), proc );   
#else
          checkChangedState( &check_result, &(m_ooo), dcheck, proc );   
#endif

          if (check_result.critical_check && check_result.perfect_check) {
            if (dcheck->getTrapType() == Trap_Unimplemented) {
              // force updates on unimplemented instructions
#ifdef PIPELINE_VIS
              out_log("dy %d unimpl %s\n", dcheck->getWindowIndex(),
                      decode_opcode( op ));
#endif
              STAT_INC( m_stat_commit_unimplemented[proc] );
              must_update_all = true;
            } else {
              // Successful commit!
              // DEBUG
              if( 0 && m_local_cycles >= 100000 && (s->getType() == DYN_LOAD || s->getType() == DYN_ATOMIC || s->getType() == DYN_STORE)){
                char buf[128];
                s->printDisassemble(buf);
                printf("%lld [%d, %d] %s SUCCESS VA: 0x%llx PA: 0x%llx VPC: 0x%llx PC: 0x%llx\n", m_local_cycles, m_id, proc, buf, memop->getAddress(), memop->getPhysicalAddress(), dcheck->getVPC(), dcheck->getPC());
              }

#ifdef PIPELINE_VIS
              out_log("dy %d success %s\n", dcheck->getWindowIndex(),
                      decode_opcode( op ));
#endif
              STAT_INC( m_stat_commit_good[proc] );
              m_opstat[proc].successOp( op );  
            }
          } else {
            // Deviation from Simics!
            // DEBUG
            if( 0 && m_local_cycles >= 100000 && (s->getType() == DYN_LOAD || s->getType() == DYN_ATOMIC || s->getType() == DYN_STORE)){
              char buf[128];
              s->printDisassemble(buf);
              printf("%lld [%d, %d] %s DEVIATION VA: 0x%llx PA: 0x%llx VPC: 0x%llx PC: 0x%llx\n", m_local_cycles, m_id, proc, buf, memop->getAddress(), memop->getPhysicalAddress(), dcheck->getVPC(), dcheck->getPC());
            }

            // print out which instruction fails
#ifdef PIPELINE_VIS
            out_log("dy %d fail %s\n", dcheck->getWindowIndex(),
                    decode_opcode( op ) );
#endif
#ifdef RETIRE_ZERO_TOLERANCE
            must_update_all = true;
#endif

            // DEBUG
        #if 0
            char buf[128];
            dcheck->getStaticInst()->printDisassemble(buf);
            bool no_trap = (dcheck->getTrapType() == Trap_NoTrap) ? true : false;
            ERROR_OUT("[ %d ] INSTR FAILED %s seqnum[ %lld ] cycle[ %lld ] Traptype[ 0x%llx ] notrap[ %d ] regs:\n", m_id, buf, dcheck->getSequenceNumber(), m_local_cycles, dcheck->getTrapType(), no_trap);
            for(int i=0; i < SI_MAX_DEST; ++i){
              reg_id_t & dest = dcheck->getDestReg(i);
              if( !dest.isZero() ){
                ERROR_OUT("\tDest[ %d ] Vanilla[ %d ] Physical[ %d ] Arf[ %s ]\n", i, dest.getVanilla(), dest.getPhysical(), dest.rid_type_menomic( dest.getRtype() ) );
              }
            }
            ERROR_OUT("\n");
        #endif
            // CM FIX: should re-read and re-decode this instruction
            STAT_INC( m_stat_commit_bad[proc] );
          }

          pushRetiredInstruction( dcheck, proc );      
          dcheck = getNextUnchecked(proc);   
        }
      } else { // end step_ok
        // the step will only 'fail' if we're running a trace. this signals
        // the end of simulation. Note: there may be some unchecked
        // instructions if the step size is > 1.
        HALT_SIMULATION;
        return;
      }
      ASSERT( m_unchecked_retires[proc] == 0 );
    } // if (must_check) results

    if (must_update_all) {
      // update all state
#ifdef PIPELINE_VIS
      checkAllState( &check_result, &(m_ooo), proc );
#else
      check_result.update_only = false;
      checkAllState( &check_result, &(m_ooo), proc );
      check_result.update_only = false;
#endif
      // the last op retired (if bundling retires) is charged for the squash
      #ifdef DEBUG_PSEQ
        DEBUG_OUT("[ %d ] FULLSQUASH %s thread[ %d ] cycle[ %lld ] VPC[ 0x%llx ] VNPC[ 0x%llx ]\n", m_id, decode_opcode(op), proc, m_local_cycles, m_fetch_at[proc]->pc, m_fetch_at[proc]->npc);
        //print();
      #endif

      fullSquash( op, proc );    
      status[proc] = PSEQ_RETIRE_UPDATEALL;
      done[proc] = true;                        //we cannot continue retiring
      // exit while loop, goto next thread
      break;
    }  // if(must_update_all)
      
    /* limit the number of instructions which can retire in one opportunity */
    retire_count[proc]++;
    retired_this_cycle++;
    total_retired++;

    //ASSERT(m_icount[proc] == m_iwin[proc].getNumSlotsTaken());

    if (retired_this_cycle >= m_max_retire_per_cycle ||
        total_retired >= MAX_RETIRE) {      
      status[proc] = PSEQ_RETIRE_LIMIT;
      // Note we don't set done[proc] to true because we can (possibly) retire more instrs bc our limit
      //     has not been reached yet
      break;
    }
    
    // if the pipeline was squashed -- stop retiring instrs
    if (squash_pipeline) {
      STAT_INC( m_stat_commit_squash[proc] );
      status[proc] = PSEQ_RETIRE_SQUASH;
      done[proc] = true;           //we have a Squash, so can no longer retire
      // exit while loop, goto next thread
      break;
    }
    
    // fetch the next instruction to retire
    d = m_iwin[proc].retireInstruction();

    if(d == NULL){
      done[proc] = true;          //no more instrs to retire
      // exit while loop, goto next thread
    }

    #ifdef DEBUG_PSEQ
    //print out TL, CWP, GSET, PSTATE
    control_state_rf = m_control_arf->getRetireRF(proc);
    //pulled from takeTrap()
    tba     = control_state_rf->getInt( CONTROL_TBA );
    tl      = control_state_rf->getInt( CONTROL_TL );
    pstate  = control_state_rf->getInt( CONTROL_PSTATE );
    cwp     = control_state_rf->getInt( CONTROL_CWP );
    gset = pstate_t::getGlobalSet( pstate );
    DEBUG_OUT("\tEND TL[ %d ] CWP[ 0x%x ] GSET[ 0x%x ] PSTATE[ 0x%x ]\n", tl, cwp, gset, pstate);
    #endif
  }  // end while(d != NULL) 

  #ifdef DEBUG_PSEQ
    printPC(m_fetch_at[proc]);
    DEBUG_OUT("pseq_t:retireInstruction END m_id[%d] proc[%d] cycle[%d] proc_retired[ %d ]\n",m_id,proc,getLocalCycle(), retire_count[proc]);
  #endif

    }  // end for loop over all logical procs
  }  // end continueRetire()
  
  /** Note: the main loop breaks out to this point, so nothing should be 
   *        added, if it is NOT shared by all cases below:
   *                    No more inst to retire;
   *                    reached retire_max;
   *                    pipeline squash;
   *                    fullSquash()
   */
  ASSERT(total_retired <= MAX_RETIRE);

  for(uint proc=0; proc <  CONFIG_LOGICAL_PER_PHY_PROC; ++proc){ 
    ASSERT( retire_count[proc] <= MAX_RETIRE );

    //record per-thread retire histogram
    m_hist_retire_per_thread[proc][retire_count[proc]]++;

    //  if (status != PSEQ_RETIRE_LIMIT) {
    m_hist_retire_stalls[status[proc]]++;       //TODO: RETIRE_LIMIT status stat is NOT correct....
    // }
  } //end for loop over all logical procs

     #ifdef DEBUG_PSEQ
       //printPC(&m_ooo.at[proc]);
       DEBUG_OUT("\tTOTAL RETIRED = %d\n", total_retired);
       DEBUG_OUT("pseq_t:retireInstruction END m_id[%d] cycle[ %d ]\n",m_id,getLocalCycle());
     #endif

  //update per cycle retirement stats (per-core)
  m_hist_retire_per_cycle[total_retired]++;
}


//**************************************************************************

void 
pseq_t::raiseException(exception_t type, 
                       uint32 win_index,
                       enum i_opcode offender,
                       abstract_pc_t *continue_at,
                       my_addr_t access_addr,
                       uint32 exception_penalty,
                       unsigned int proc)
{

  #ifdef DEBUG_PSEQ
  DEBUG_OUT("pseq_t:raiseException BEGIN m_id[%d] proc[%d] cycle[%d]\n",m_id,proc,getLocalCycle());
  printPC(&m_ooo.at[proc]);
  #endif

  ASSERT(proc < CONFIG_LOGICAL_PER_PHY_PROC);

  ASSERT( m_iwin[proc].isInWindow(win_index) );
  if (type < EXCEPT_NUM_EXCEPT_TYPES)
    m_exception_stat[proc][type]++;
  
  /* if this exeception is sooner than any other one outstanding, keep it */
  if ( (m_except_type[proc] == EXCEPT_NONE) ||
       (m_iwin[proc].rangeSubtract( win_index, m_next_exception[proc] ) < 0) ||
       ((win_index == m_next_exception[proc]) && (type == EXCEPT_MISPREDICT)) ) {

    // DEBUG_OUT("cycle %lld: new E %d %d 0x%0llx\n", getLocalCycle(), type, win_index, m_except_continue_at.pc );

#if 0
    // CM FIX
    // This condition is when you're trying to squash an instruction that
    // has already retired. It only occurs when stale data speculation is
    // turned on, but should generate a warning!
    if ( m_iwin[proc].getLastRetired() == m_iwin[proc].iwin_increment(win_index) ) {
      DEBUG_OUT("exception: last window: %0lld 0x%0llx\n",
               getLocalCycle(), m_except_continue_at[proc].pc );
    }
#endif

#ifdef PIPELINE_VIS
    out_log("raisedException\n");
    out_log("cycle %lld: new E %d %d 0x%0llx\n",
            getLocalCycle(), type, win_index, m_except_continue_at[proc].pc );
    printPC( continue_at );
#endif
    m_except_type[proc] = type;
    m_next_exception[proc]     = win_index;
    m_except_offender[proc]    = offender;
    m_except_continue_at[proc] = *continue_at;
    m_except_access_addr[proc] = access_addr;
    m_except_penalty[proc]     = exception_penalty;
  }

  #ifdef DEBUG_PSEQ
      printPC(&m_ooo.at[proc]);
     DEBUG_OUT("pseq_t:raiseException END m_id[%d] proc[%d] cycle[%d]\n",m_id,proc,getLocalCycle());
  #endif
}

//**************************************************************************
void 
pseq_t::clearException(unsigned int proc )
{
  #ifdef DEBUG_PSEQ
    DEBUG_OUT("pseq_t:clearException  BEGIN m_id[%d] proc[%d] cycle[%d]\n",m_id,proc,getLocalCycle());
  #endif

  ASSERT(proc < CONFIG_LOGICAL_PER_PHY_PROC);

     m_next_exception[proc]     =  0;
     m_except_offender[proc]    =  i_nop;
     m_except_continue_at[proc].init();
     m_except_access_addr[proc] =  0;
     m_except_type[proc]        = EXCEPT_NONE;

  #ifdef DEBUG_PSEQ
     DEBUG_OUT("pseq_t:clearException END m_id[%d] proc[%d] cycle[%d]\n",m_id,proc,getLocalCycle());
  #endif
}

//**************************************************************************
bool
pseq_t::takeTrap( dynamic_inst_t *instruction, abstract_pc_t *inorder_at,
                  bool is_timing, unsigned int proc )
{
  #ifdef DEBUG_PSEQ
     DEBUG_OUT("pseq_t:takeTrap BEGIN m_id[%d] proc[%d] istiming[ %d ] cycle[%d]\n",m_id,proc,is_timing, getLocalCycle());
     printPC(&m_ooo.at[proc]);
  #endif

  ASSERT(proc < CONFIG_LOGICAL_PER_PHY_PROC);

  trap_type_t      traptype = Trap_NoTrap;
  physical_file_t *control_state_rf = NULL;
  
  if (is_timing) {
    enum i_opcode op = (enum i_opcode) instruction->getStaticInst()->getOpcode();
    traptype = instruction->getTrapType();

    // CM don't use full squash here: m_fetch_at won't be reset correctly
    partialSquash(m_iwin[proc].getLastRetired(), inorder_at, op, proc, false);
    // NOTE: due to rollback of register renaming, must squash this
    //       instruction AFTER calling squash (always reverse order)
    instruction->Squash();
    
    // NOTE: increment lsq index. Squash decrements it, but simics
    //       will be retiring an instruction. keep in step with it. :)
    stepSequenceNumber(proc);

    control_state_rf = m_control_arf->getRetireRF(proc);
  } else {
    // please forgive me, my light day hacker friend -- The Night Hacker!
    flow_inst_t *my_flow = (flow_inst_t *) instruction;
    traptype = my_flow->getTrapType();
    control_state_rf = m_ideal_control_rf[proc];
  }

  // if this instruction is not implemented, merely tell the caller
  // that the complete state must be checked before retiring it.
  if (traptype == Trap_Unimplemented) {
    return true;
  }

  // The following implements trap handling in accordance to SPARC V9 docs
  ireg_t tba     = control_state_rf->getInt( CONTROL_TBA );
  ireg_t tl      = control_state_rf->getInt( CONTROL_TL );
  ireg_t old_tl = tl;
  ireg_t pstate  = control_state_rf->getInt( CONTROL_PSTATE );
  ireg_t old_pstate = pstate;
  ireg_t cwp     = control_state_rf->getInt( CONTROL_CWP );
  uint16 gset    = REG_GLOBAL_NORM;
  uint16 ccr     = 0;

  if (is_timing) {
    ccr = m_cc_rf->getInt( m_cc_retire_map->getMapping( REG_CC_CCR, proc ) );
  } else {
    ccr = control_state_rf->getInt( CONTROL_CCR );
  }

  // write the trap type to the TT register
  int tt = (int) traptype;
  if ( tt < TRAP_NUM_TRAP_TYPES ) {
    // DEBUG for Kernel traps
    #if 0
        // print out the instruction causing the trap..
        char buf[128];
        instruction->getStaticInst()->printDisassemble(buf);
        DEBUG_OUT("takeTrap: regular mode, trap[ %s ] cycle[ %lld ] TL[ %lld ] %s seq_num[ %lld ]\n", pstate_t::trap_num_menomic( traptype ), m_local_cycles, tl, buf, instruction->getSequenceNumber());
    #endif

      m_trapstat[proc][tt]++;
  }
  else{
    ERROR_OUT("takeTrap(): trap OUT OF RANGE tt[ 0x%x ] cycle[ %lld ]\n", tt, m_local_cycles);
    ASSERT(0);
  }

  // DEBUG for SW initiated traps
  #if 0
  if( (tt >= 0x100) && tt <= (0x100+ 0x17f)){
    // 256 is added to software trap # during execution
    // print out the instruction causing the trap..
    char buf[128];
    instruction->getStaticInst()->printDisassemble(buf);
    ERROR_OUT("takeTrap: regular mode, trap[ Software_Trap_%d ] NUM_TRAPS[ %d ] cycle[ %lld ] TL[ %lld ] %s seq_num[ %lld ]\n", tt, TRAP_NUM_TRAP_TYPES, m_local_cycles, tl, buf, instruction->getSequenceNumber());
    m_software_trapstat[proc][tt - 256]++;
  }
  #endif

  //ERROR_OUT("setting CONTROL_TT1 TL[ %d ] CONTROL_TT1[ %d ] TT[ 0x%x ]\n", tl, CONTROL_TT1, tt);

  control_state_rf->setInt( CONTROL_TT1 + tl, tt );
#ifdef PIPELINE_VIS
  out_log("trap 0x%0x\n", tt);
#endif

#ifdef DEBUG_PSEQ
  out_log("TAKING TRAP 0x%0x 0x%0x 0x%0x\n", tt, (tt << 5),
           ((tl != 0) << 14));
#endif

  // preserve the existing state
  ireg_t trapstate = 0;
  uint16 asi    = control_state_rf->getInt( CONTROL_ASI ) & 
    makeMask64( reg_box_t::controlRegSize(CONTROL_ASI) - 1, 0 );
  uint16 old_asi =  control_state_rf->getInt( CONTROL_ASI );

  trapstate |=  cwp;
  trapstate |= ((ireg_t) pstate << 8);
  trapstate |= ((ireg_t) asi << 24);
  trapstate |= ((ireg_t) ccr << 32);
  
  #ifdef DEBUG_PSEQ
  char buf[128];
  instruction->getStaticInst()->printDisassemble(buf);
  DEBUG_OUT("takeTrap: %s setting TCWP[ 0x%x ] TPSTATE[ 0x%x ] TASI[ 0x%x ] TPC1[ 0x%llx ] TNPC1[ 0x%llx ]  at TL[ %d ] cycle[ %lld ] seqnum[ %lld ]\n", buf, cwp, pstate, asi, inorder_at->pc, inorder_at->npc, tl, m_local_cycles, instruction->getSequenceNumber());
 #endif

  control_state_rf->setInt( CONTROL_TSTATE1 + tl, trapstate );
  control_state_rf->setInt( CONTROL_TPC1 + tl, inorder_at->pc );
  control_state_rf->setInt( CONTROL_TNPC1 + tl, inorder_at->npc );
  if (is_timing) {
    // also: push the old trap address (PC,NPC, etc.) on to a stack (predictor)
    m_tlstack[proc]->push( inorder_at->pc, inorder_at->npc, (uint16) pstate,
                     (uint16) cwp, (uint16) tl );
  }

  /*
   *  Task:  update the processor state PSTATE
   */
  // mask off everything except TLE and MM
  pstate  =  pstate & 0x1c0;
  // move TLE over to CLE, mask back on
  pstate |= ((pstate << 1) & 0x200);
  // set architecturally defined (known) mode bits
  // 0x10 == PEF  floating point enabled
  // 0x04 == PRIV privileged mode
  pstate |= 0x14;
  
  // set global bits
  if (tt == Trap_Interrupt_Vector) {
    // IG == 11 bits over
    pstate |= (1 << 11);
  } else if (tt == Trap_Fast_Instruction_Access_MMU_Miss ||
             tt == Trap_Fast_Data_Access_MMU_Miss ||
             tt == Trap_Fast_Data_Access_Protection ||
             tt == Trap_Instruction_Access_Exception ||
             tt == Trap_Data_Access_Exception) {
    // MG == 10 bits over
    pstate |= (1 << 10);
  } else {
    // AG == 0 bits over (alternate globals)
    pstate |= 1;
  }
  control_state_rf->setInt( CONTROL_PSTATE, pstate );
  //
  // immediately find the new global set (in architected state)
  //
  gset = pstate_t::getGlobalSet( pstate );

  /*
   * Task: update the CWP (if a spill / fill trap)
   *       I tried to do this in the execute logic, but the wrong CWP was saved
   */
  if (tt == Trap_Clean_Window) {
    // CLEAN WINDOW TRAP
    // increment cwp (for trap handler)
    cwp++;
    cwp &= (NWINDOWS - 1);
    control_state_rf->setInt( CONTROL_CWP, cwp );
  } else if ( (uint32) tt >= (uint32) 0x80  &&  (uint32) tt < (uint32) 0xbf ) {
    // BUGFIX - should be >= 0x80
    // WINDOW SPILL TRAP
    // set cwp (for trap handler) to be CWP + CANSAVE + 2
    ireg_t cansave = control_state_rf->getInt( CONTROL_CANSAVE );
    cwp  = cwp + cansave + 2;
    cwp &= (NWINDOWS - 1);
    control_state_rf->setInt( CONTROL_CWP, cwp );

  } else if ( (uint32) tt >= (uint32) 0xc0  &&  (uint32) tt < (uint32) 0xff ) {
    //BUGFIX - should be >= 0xc0
    // WINDOW FILL TRAP
    // set cwp (for trap handler) to be CWP - 1
    cwp--;
    cwp &= (NWINDOWS - 1);
    control_state_rf->setInt( CONTROL_CWP, cwp );
  }
  
  if ( control_state_rf->getInt( CONTROL_CWP ) > NWINDOWS ) {
    DEBUG_OUT("error: pseq: trap: cwp set to %lld\n",
             control_state_rf->getInt( CONTROL_CWP ));
  }
  
  // compute and set the program counter based on the trap number
  la_t eff_addr = tba | (tt << 5) | ((tl != 0) << 14);
  
  // Increment Trap Level register TL
  tl++;
  //set last traplevel to be equal to current traplevel
  m_last_traplevel[proc] = tl;
  ASSERT(0 <= m_last_traplevel[proc] && m_last_traplevel[proc] <= MAXTL );

  if (tl > MAXTL) {
    DEBUG_OUT("tl out of range: 0x%0llx cycle[ %lld ]\n", tl, m_local_cycles);
    tl = MAXTL;
  }
#ifdef DEBUG_PSEQ
  DEBUG_OUT("writing trap level: %lld\n", tl);
#endif
  control_state_rf->setInt( CONTROL_TL, tl );

  // CM AddressMask64 AM64
  inorder_at->pc     = eff_addr;
  inorder_at->npc    = eff_addr + sizeof(uint32);
  inorder_at->tl     = tl;
  inorder_at->pstate = pstate;
  inorder_at->cwp    = cwp;
  inorder_at->gset   = gset;

  // save this traptype
  m_last_traptype[proc][m_last_traplevel[proc]] = traptype;
  m_last_traptype_cycle[proc][m_last_traplevel[proc]] = m_local_cycles;
  #ifdef DEBUG_PSEQ
  DEBUG_OUT("\tSETTING TRAPTYPE TL[ %d ] Trap[ 0x%x ] cycle[ %lld ]\n", m_last_traplevel[proc], traptype, m_local_cycles);
  #endif

  // DEBUG
  if(  tt == Trap_Fast_Instruction_Access_MMU_Miss){
    // ERROR_OUT("**********takeTrap: setting m_last_traptype to ITLB miss cycle[ %lld ]\n", m_local_cycles);
  }
  if(  tt == Trap_Fast_Data_Access_MMU_Miss){
    // ERROR_OUT("**********takeTrap: setting m_last_traptype to DTLB miss cycle[ %lld ]\n", m_local_cycles);
  }

  // re-start the timing model's fetch at the inorder PC
  if (is_timing) {
    //*m_fetch_at = *inorder_at;
    *(m_fetch_at[proc]) = *inorder_at;
  }

  #ifdef DEBUG_PSEQ
     printPC(&m_ooo.at[proc]);
     DEBUG_OUT("pseq_t:takingTrap END m_id[%d] proc[%d] cycle[%d]\n",m_id,proc,getLocalCycle());
  #endif

  return false;
}

//**************************************************************************
void
pseq_t::partialSquash(uint32 last_good, abstract_pc_t *fetch_at,
                      enum i_opcode offender, unsigned int proc, bool squash_all)
{

  #ifdef DEBUG_PSEQ
     DEBUG_OUT("\npseq_t:partialSquash BEGIN : m_id[$d] proc[%d] %s newfetchPC[ 0x%llx ] newfetchNPC[ 0x%llx ] cycle[ %lld ]\n", m_id, proc, decode_opcode( offender ), fetch_at->pc, fetch_at->npc, m_local_cycles);
   #endif

  int32 num_decoded = 0;

  STAT_INC( m_stat_total_squash[proc] );

  #ifdef DEBUG_PSEQ
    DEBUG_OUT("pseq_t:partialSquash before squashOp proc[%d] cycle[%d]\n",proc,getLocalCycle());
  #endif

  m_opstat[proc].squashOp( offender );   
  
  // squash the instructions out of the iwindow 
  // Do this for only a specified SMT thread
  #ifdef DEBUG_PSEQ
    DEBUG_OUT("pseq_t:partialSquash before Iwin squash proc[%d] cycle[%d]\n",proc,getLocalCycle());
  #endif

  if(!squash_all){
    m_iwin[proc].squash(this, last_good, num_decoded);
  }
  else{
    m_iwin[proc].squash_all(this);
  }

  #ifdef DEBUG_PSEQ
    DEBUG_OUT("pseq_t:partialSquash after Iwin squash proc[%d] cycle[%d]\n",proc,getLocalCycle());
  #endif
  /******************** Fetch-related fields ********************/ 

  /* if this sequencer is waiting for an i-cache miss, stop waiting */
  // I-cache misses are now handled by the m_proc_waiter objects:
  ASSERT(m_proc_waiter[proc] != NULL);
  if(m_proc_waiter[proc]->Waiting()){
    #ifdef DEBUG_PSEQ
    DEBUG_OUT("REMOVING WAITER BEFORE for proc[%d] Disconnected[%d]\n",
              proc,m_proc_waiter[proc]->Disconnected());
    #endif
    
     m_proc_waiter[proc]->RemoveWaitQueue();

    #ifdef DEBUG_PSEQ
     DEBUG_OUT("REMOVING WAITER AFTER for proc[%d] Disconnected[%d]\n",
              proc,m_proc_waiter[proc]->Disconnected());
     #endif
  }

  // flush the fetch pipe
  #ifdef DEBUG_PSEQ
    DEBUG_OUT("pseq_t:partialSquash before initializing m_fetch_per_cycle proc[%d] cycle[%d]\n",proc,getLocalCycle());
  #endif

  m_fetch_per_cycle[proc].init(0);
  if ( ( (m_except_type[proc] == EXCEPT_MISPREDICT) || (m_except_type[proc] == EXCEPT_MEMORY_REPLAY_TRAP))  
        && (m_except_penalty[proc] != 0) ) {
    // if mis-prediction, charge some penalty
    // Be careful when using Write buffers, bc the PSEQ_FETCH_WRITEBUFFER_FULL may already be set
    //            during the execution of instructions, so we only modify the saved fetch status. This also means
    //            we are overlapping the flush penalty with the WB full stall penalties
    if(m_fetch_status[proc] == PSEQ_FETCH_WRITEBUFFER_FULL){
      m_saved_fetch_status[proc] = PSEQ_FETCH_SQUASH;
    }
    else{
      //ERROR_OUT("setting fetch status to SQUASH proc[ %d ] cycle[ %lld ]\n", m_id, m_local_cycles);
      m_fetch_status[proc] = PSEQ_FETCH_SQUASH;
    }
    m_fetch_ready_cycle[proc] = getLocalCycle() + m_except_penalty[proc];   
  } else {
    // otherwise, immediately resume fetch
    // Be careful when using Write buffers, bc the PSEQ_FETCH_WRITEBUFFER_FULL may already be set
    //            during the execution of instructions, so we only modify the saved fetch status. This also means
    //            we are overlapping the flush penalty with the WB full stall penalties
    if(m_fetch_status[proc] == PSEQ_FETCH_WRITEBUFFER_FULL){
      m_saved_fetch_status[proc] = PSEQ_FETCH_READY;
    }
    else{
      //ERROR_OUT("setting fetch status to READY proc[ %d ] cycle[ %lld ]\n", m_id, m_local_cycles);
      m_fetch_status[proc] = PSEQ_FETCH_READY;
    }
  }
  
  // reset the fetch control fields to reflect the 'future' architected state
  // where the squash is occuring
  #ifdef DEBUG_PSEQ
    DEBUG_OUT("pseq_t:partialSquash before setting m_fetch_at proc[%d] cycle[%d]\n",proc,getLocalCycle());
  #endif
  ASSERT(fetch_at != NULL);
  ASSERT(m_fetch_at[proc] != NULL);

  *(m_fetch_at[proc]) = *fetch_at;

  // for multicycle I$, reset waiting flag, can set waiting addr cause TLB
  if (ICACHE_CYCLE > 1 && m_wait_for_request[proc]) m_wait_for_request[proc] = false;

  /******************** Decode-related fields *******************/ 

  // flush the decode pipe

  #ifdef DEBUG_PSEQ
    DEBUG_OUT("pseq_t:partialSquash before initalizing m_decode_per_cycle  proc[%d] cycle[%d]\n",proc,getLocalCycle());
  #endif

  m_decode_per_cycle[proc].init(0);
  m_decode_per_cycle[proc].insertItem( getLocalCycle(), 1, num_decoded);
  if (num_decoded < IWINDOW_WIN_SIZE) {
    m_hist_decode_return[num_decoded]++;
  } else {
    m_hist_decode_return[IWINDOW_WIN_SIZE - 1]++;
  }

  /************** Schedule/Execute-related fields ***************/ 
  /* force a recount of inflight instructions?? */

  #ifdef DEBUG_PSEQ
    printPC(&m_ooo.at[proc]);
    DEBUG_OUT("pseq_t:partialSquash END m_id[%d] proc[%d] cycle[%d]\n",m_id,proc,getLocalCycle());
  #endif

    /* WATTCH power */
    if(WATTCH_POWER){
      getPowerStats()->incrementWindowAccess();
    }

}

//**************************************************************************
void
pseq_t::fullSquash( enum i_opcode offender, unsigned int proc )
{

  #ifdef DEBUG_PSEQ
     DEBUG_OUT("pseq_t:fullSquash BEGIN m_id[%d] proc[%d] cycle[%d]\n",m_id,proc,getLocalCycle());
     printPC(&m_ooo.at[proc]);
  #endif

  ASSERT(proc < CONFIG_LOGICAL_PER_PHY_PROC);

  int32         last_good = m_iwin[proc].getLastRetired();

  // restart fetch based on the architected control state
  abstract_pc_t fetch_at;
  partialSquash( last_good, &fetch_at, offender, proc, false);

  physical_file_t *control_rf = m_control_arf->getRetireRF(proc);
  m_fetch_at[proc]->pc     = control_rf->getInt( CONTROL_PC );
  m_fetch_at[proc]->npc    = control_rf->getInt( CONTROL_NPC );
  m_fetch_at[proc]->tl     = control_rf->getInt( CONTROL_TL );
  m_fetch_at[proc]->pstate = control_rf->getInt( CONTROL_PSTATE );
  m_fetch_at[proc]->cwp    = control_rf->getInt( CONTROL_CWP );

  m_fetch_at[proc]->gset = pstate_t::getGlobalSet( m_fetch_at[proc]->pstate );
  if ( ISADDRESS32(m_fetch_at[proc]->pstate) ) {
    m_fetch_at[proc]->pc  = AM64( m_fetch_at[proc]->pc );
    m_fetch_at[proc]->npc = AM64( m_fetch_at[proc]->npc );
  }
#ifdef PIPELINE_VIS
  DEBUG_OUT("AFTER full squash\n");
  printPC( &m_ooo.at[proc] );
#endif

#ifdef DEBUG_PSEQ
  //printPC(&m_ooo.at[proc]);
  DEBUG_OUT("[ %d ] fullSquash END %s thread[ %d ] cycle[ %lld ] END newfetchPC[ 0x%llx ] newfetchNPC[ 0x%llx ]\n", m_id, decode_opcode( offender), proc, m_local_cycles, m_fetch_at[proc]->pc, m_fetch_at[proc]->npc);
#endif

    /* WATTCH power */
    if(WATTCH_POWER){
      getPowerStats()->incrementWindowAccess();
    }   
}

//**************************************************************************
void
pseq_t::postEvent( waiter_t *waiter, uint32 cyclesInFuture )
{
  m_scheduler->queueInsert( waiter, m_local_cycles, cyclesInFuture - 1 );
}

//**************************************************************************
void pseq_t::Wakeup( uint32 proc )
{
#if defined DEBUG_PSEQ || defined DEBUG_RUBY
  DEBUG_OUT("pseq: wakeup(): cycle: %lld\n", m_local_cycles);
#endif

  ASSERT(CONFIG_LOGICAL_PER_PHY_PROC > 0);
  uint32 logical_proc_num = SIM_get_current_proc_no() % CONFIG_LOGICAL_PER_PHY_PROC;
  ASSERT(logical_proc_num < CONFIG_LOGICAL_PER_PHY_PROC);

#ifdef DEBUG_PSEQ
  DEBUG_OUT("\tWaking up: computed proc[%d], arg proc[%d]\n",logical_proc_num,proc);
#endif

  // we only woken up by the I$, don't reset fetch_status if not waiting
  // When using the Write Buffer we may already be in the PSEQ_FETCH_WRITEBUFFER_FULL status
  //     so we modify the saved fetch status 
  if( m_fetch_status[proc] == PSEQ_FETCH_WRITEBUFFER_FULL){
    m_saved_fetch_status[proc] = PSEQ_FETCH_READY;
  }
  else if( m_fetch_status[proc] == PSEQ_FETCH_ICACHEMISS){
    //ERROR_OUT("Wakeup 2, setting FETCH READY proc[ %d ] cycle[ %lld ]\n", m_id, m_local_cycles);
    m_fetch_status[proc] = PSEQ_FETCH_READY;
  }

#ifdef DEBUG_PSEQ
  DEBUG_OUT("pseq_t:wakeup END\n");
#endif
}

//**************************************************************************
// Called by BARRIER instructions (see controlop.C, memop.C)
void pseq_t::unstallFetch( unsigned int proc )
{
  #ifdef DEBUG_PSEQ
  DEBUG_OUT("pseq_t:unstallFetch BEGIN m_id[%d] proc[%d]\n",m_id,proc);
  #endif

  ASSERT(proc < CONFIG_LOGICAL_PER_PHY_PROC);

  // When using a write buffer we may already be in PSEQ_FETCH_WRITEBUFFER_FULL status, so
  // we only modify the saved status
  if(m_fetch_status[proc] == PSEQ_FETCH_WRITEBUFFER_FULL){
    ERROR_OUT("unstallFetch() called when WRITEBUFFER FULL\n");
    ASSERT(m_saved_fetch_status[proc] == PSEQ_FETCH_BARRIER);
    m_saved_fetch_status[proc] = PSEQ_FETCH_READY;
  }
  else{
    if( !(m_fetch_status[proc] == PSEQ_FETCH_BARRIER) ){
      ERROR_OUT("unstallFetch() called when NO STALL status[ %s ] cycle[ %lld ]\n", fetch_menomic(m_fetch_status[proc]), m_local_cycles);
    }
    ASSERT( m_fetch_status[proc] == PSEQ_FETCH_BARRIER);
    //ERROR_OUT("=========> unstallFetch() proc[ %d ] thread[ %d ] UNSTALLING FETCH BARRIER cycle[ %lld ]\n", m_id, proc, m_local_cycles);
    m_fetch_status[proc] = PSEQ_FETCH_READY;
  }

  #ifdef DEBUG_PSEQ
  DEBUG_OUT("pseq_t:unstallFetch END m_id[%d] proc[%d]\n",m_id,proc);
  #endif
}

/*------------------------------------------------------------------------*/
/* Functional Inorder Processor Model                                     */
/*------------------------------------------------------------------------*/

//**************************************************************************
void pseq_t::printCFG( unsigned int proc )
{
  // walk the current CFG, printing each node
  // print CFG starting at start index
  DEBUG_OUT( "pseq_t:: printCFG called\n");
  m_cfg_pred[proc]->printGraph( 0 );
}

//**************************************************************************
pseq_fetch_status_t pseq_t::idealRunTo( int64 runto_seq_num, unsigned int proc )
{
  #ifdef DEBUG_PSEQ
    DEBUG_OUT("pseq_t:idealRunTo BEGIN\n");
  #endif

  ASSERT(proc < CONFIG_LOGICAL_PER_PHY_PROC);

  static_inst_t   *s_instr = NULL;
  pa_t             fetchPhysicalPC;

  // check to find if this sequence number is ready or not
  //    if its to low < last_checked, we have no information on it
  if (runto_seq_num < m_ideal_last_checked[proc]) {
    ERROR_OUT("error: idealRunTo: unable to run to: too far in past [%lld < %lld]!\n", runto_seq_num, m_ideal_last_checked[proc] );
    return (PSEQ_FETCH_BARRIER);
  }

  //    if the number is between last_checked and last_executed, we results now
  if (runto_seq_num > m_ideal_first_predictable[proc] &&
      m_ideal_last_predictable[proc] <= runto_seq_num ) {

    DEBUG_OUT("  searching for predictable\n");

    // find the index in the hash table and return the result
    flow_inst_t *node = m_cfg_index[proc][runto_seq_num];
    if (node == NULL) {
      ERROR_OUT( "error: idealRunTo: unable to find node. (%lld)\n",
                 runto_seq_num );
      return (PSEQ_FETCH_BARRIER);
    } else {
      // return found!
      return (PSEQ_FETCH_READY);
    }
  }

  // if the number is ahead of last_predictable, run until it is predictable
  while ( m_ideal_last_predictable[proc] < runto_seq_num ) {
    // fetch an instruction
    //#ifdef DEBUG_IDEAL
    DEBUG_OUT("pseq_t: predictable %lld runto %lld. running.\n",
              m_ideal_last_predictable[proc] < runto_seq_num );
    printPC( &(m_inorder[proc].at[proc]) );
    //#endif
    lookupInstruction(&(m_inorder[proc].at[proc]), &s_instr, &fetchPhysicalPC, proc);   
    
    // allocate a new node in the control flow graph
    // connects it to its sources and destination dependencies
    // Pass in the logical proc specifics:
    flow_inst_t *node = new flow_inst_t( s_instr, this,
                                         &(m_inorder[proc]),
                                         m_mem_deps[proc], proc );

    if ( s_instr == m_fetch_itlbmiss[proc] ) {
      // take the in-order trap
      node->setTrapType( Trap_Fast_Instruction_Access_MMU_Miss );

      // NOTE: After this ITLB miss is completed Simics's MMU state
      //   may still not contain the translation.
    } else {
      // "Execute them!" "Bogus!" (name the movie this is from)
      node->readMemory();
      node->execute();
    }

    /*
     * Task: retire the instruction, unless it causes a trap.
     */
    // The traptype and window index of the retiring instr (if any)
    trap_type_t traptype = node->getTrapType();
    if ( traptype == Trap_NoTrap ) {
      // Retire the instruction
      node->advancePC();

    } else if ( traptype == Trap_Use_Functional ||
                traptype == Trap_Unimplemented ) {
      // stall the in-order model until the processor advances
      node->advancePC();

      // return that we were not able to execute this instruction
      // stall the in-order model until the processor advances
      STAT_INC( m_inorder_partial_success[proc] );    
      return (PSEQ_FETCH_BARRIER);
    } else {
      // functionally take the trap
      takeTrap( (dynamic_inst_t *) node, node->getActualTarget(), false, proc );  
    }

    // advance PC, update where we are fetching / executing from
    m_inorder[proc].at[proc] = *node->getActualTarget();
    //#if DEBUG_IDEAL
    DEBUG_OUT("pseq_t: stepInorder: retirement\n");
    node->print();
    printPC( &(m_inorder[proc].at[proc] ));
    //#endif

    // increment the last_predictable index
    m_ideal_last_predictable[proc]++;
  }

  #ifdef DEBUG_PSEQ
    DEBUG_OUT("pseq_t:idealRunTo END\n");
  #endif
  
  return (PSEQ_FETCH_READY);
}

//**************************************************************************
pseq_fetch_status_t pseq_t::oraclePredict( dynamic_inst_t *d,
                                           bool *taken, 
                                           abstract_pc_t *inorder_at )
{
  #ifdef DEBUG_PSEQ
    DEBUG_OUT("pseq_t:oraclePredict BEGIN\n");
  #endif

  pseq_fetch_status_t status = PSEQ_FETCH_READY;
  uint64 seq_num = d->getSequenceNumber();

  DEBUG_OUT("oracle predict. called\n");
  // look up the sequence number in the ideal table
  if (m_cfg_index[d->getProc()].find( seq_num ) == m_cfg_index[d->getProc()].end()) {  
    // not found: run inorder processor until it is found
    status = idealRunTo( seq_num, d->getProc() );  
    if (status != PSEQ_FETCH_READY) {
      // if not able to execute to this instruction, return
      status = PSEQ_FETCH_SQUASH;
    }
  }
  
  // query the mapping to get the flow instruction
  if (m_cfg_index[d->getProc()].find( seq_num ) != m_cfg_index[d->getProc()].end()) {
    flow_inst_t *flow_inst  = m_cfg_index[d->getProc()][seq_num];
    
    // if the instruction is found OK, read / compare the instruction
    if ( flow_inst->getStaticInst()->getInst() ==
         d->getStaticInst()->getInst() ) {
      // make the prediction based on flow instruction
      DEBUG_OUT("oracle predict. making prediction!\n");
      *taken = flow_inst->getTaken();
      *inorder_at = *flow_inst->getActualTarget();
    } else {
      // not the same instruction
      status = PSEQ_FETCH_SQUASH;
    }
  }
  
  // return successful (ready)
  DEBUG_OUT("oracle predict. returns: %d\n", status);
  m_hist_ideal_coverage[status]++;

  #ifdef DEBUG_PSEQ
    DEBUG_OUT("pseq_t:oraclePredict END\n");
  #endif

  return (status);
}

//**************************************************************************
void pseq_t::idealCheckTo( int64 seq_num )
{

  #ifdef DEBUG_PSEQ
    DEBUG_OUT("pseq_t:idealCheckTo BEGIN\n");
  #endif

  // if m_ideal_last_checked is less than seq_num, check each instruction
  // until synchronized
  out_info("idealCheckTo. checking inorder processor: %lld\n", seq_num);
  
#if 0
  while ( m_ideal_last_checked < seq_num ) {
    
    enum i_opcode op = (enum i_opcode) s_instr->getOpcode();
    m_ideal_opstat.seenOp( op );

    // here we actually need to check a value from the _past_

    check_result_t result;
    result.verbose = true;
    checkAllState( &result, &(m_inorder[proc]) );

#if defined( DEBUG_FUNCTIONALITY ) && defined( DEBUG_IDEAL )
    if (!result.perfect_check) {
      out_info("perfect check fails\n");
      node->print();
    }
#endif
    
    if (result.perfect_check) {
#ifdef PIPELINE_VIS
      out_log("dy %lld success %s\n", node->getID(), decode_opcode( op ));
#endif
      m_ideal_opstat.successOp( op );
    } else {
      node->print();
#ifdef PIPELINE_VIS
      out_log("dy %lld fail %s\n", node->getID(), decode_opcode( op ) );
#endif
    }
  }
#endif

  #ifdef DEBUG_PSEQ
    DEBUG_OUT("pseq_t:idealCheckTo END\n");
  #endif
}

//**************************************************************************
void pseq_t::stepInorder( void )
{
  #ifdef DEBUG_PSEQ
    DEBUG_OUT("pseq_t:stepInorder BEGIN\n");
  #endif

  static_inst_t   *s_instr = NULL;
  pa_t             fetchPhysicalPC;

  // In order to simulate stepping a SMT processor, need to loop through simulating all individual logical processors
  for(uint k=0; k <  CONFIG_LOGICAL_PER_PHY_PROC; ++k){  

#if 0
  if (m_ideal_status[k] != PSEQ_FETCH_READY) {
    return false;
  }
#endif

  // fetch an instruction
#ifdef DEBUG_IDEAL
  out_info("pseq_t: stepInorder: fetch\n");
  printPC( m_inorder[k].at[k] );  
#endif
  lookupInstruction(&(m_inorder[k].at[k]), &s_instr, &fetchPhysicalPC, k);  

  enum i_opcode op = (enum i_opcode) s_instr->getOpcode();
  m_ideal_opstat[k].seenOp( op, Trap_NoTrap, s_instr->getType() );  

  // allocate a new node in the control flow graph
  // connects it to its sources and destination dependencies
  flow_inst_t *node = new flow_inst_t( s_instr, this,
                                       &(m_inorder[k]), m_mem_deps[k], k );

  if ( s_instr == m_fetch_itlbmiss[k] ) {
    // take the in-order trap
    node->setTrapType( Trap_Fast_Instruction_Access_MMU_Miss );
    // stall the in-order model until the processor advances
    //m_ideal_status = PSEQ_FETCH_ITLBMISS;
  } else {
    // "Execute them!" "Bogus!" (name the movie this is from)
    node->readMemory();     
    node->execute();
  }

  /*
   * Task: retire the instruction, unless it causes a trap.
   */

  // The traptype and window index of the retiring instr (if any)
  trap_type_t traptype = node->getTrapType();
  if ( traptype == Trap_NoTrap ) {
    // Retire the instruction
    node->advancePC();

  } else if ( traptype == Trap_Use_Functional ||
              traptype == Trap_Unimplemented ) {
    // stall the in-order model until the processor advances
    node->advancePC();

  }
  
  // print out the PC, instruction that is retiring
#if DEBUG_IDEAL
  out_info("pseq_t: stepInorder: retirement\n");
  node->print();
  printPC( node->getActualTarget() );
#endif
  
  // check this instruction
  m_unchecked_retires[k] = 1;
  advanceSimics(k);   
  STAT_INC( m_ideal_retire_count[k] );

  if (m_posted_interrupt[k] == Trap_NoTrap) {
    if ( !( traptype == Trap_NoTrap ||
            traptype == Trap_Use_Functional ||
            traptype == Trap_Unimplemented) ) {
      // if we should have taken a trap before, functionally take it now
      takeTrap( (dynamic_inst_t *) node, &(m_inorder[k].at[k]), false, k );
      *node->getActualTarget() = m_inorder[k].at[k];
    }
  } else {
    // trap posted, get it then reset
    node->setTrapType( m_posted_interrupt[k] );
    m_posted_interrupt[k] = Trap_NoTrap;
    
    // take the trap here (if any), as execute may have modified
    takeTrap( (dynamic_inst_t *) node, &(m_inorder[k].at[k]), false, k );
    *node->getActualTarget() = m_inorder[k].at[k];
  }
  
  if (traptype == Trap_Use_Functional) {
    // read the result register out of simics
    updateInstructionState( node, k );     
  }

  // update/advance the PC of this instruction
  m_inorder[k].at[k] = *node->getActualTarget();
  
  // check all state
  check_result_t result;
  result.verbose = true;
  checkAllState( &result, &(m_inorder[k]), k );
  
#if defined( DEBUG_FUNCTIONALITY ) && defined( DEBUG_IDEAL )
  if (!result.perfect_check) {
    out_info("perfect check fails\n");
    node->print();
  }
#endif

  if (result.perfect_check) {
#ifdef PIPELINE_VIS
    out_log("dy %lld success %s\n", node->getID(), decode_opcode( op ));
#endif
    m_ideal_opstat[k].successOp( op );  
  } else {
    node->print();
#ifdef PIPELINE_VIS
    out_log("dy %lld fail %s\n", node->getID(), decode_opcode( op ) );
#endif
  }

  // add this flow instruction to the map (id->instruction)
  m_cfg_index[k][node->getID()] = node;   

  } // end for loop over all logical processors

  // CM FIX: use flow instruction ref counting to free the node appropriately
  //         (e.g. after retirement count has passed it & no one depends onit)
  // delete node;

  #ifdef DEBUG_PSEQ
    DEBUG_OUT("pseq_t:stepInorder END\n");
  #endif
}

//**************************************************************************
void pseq_t::warmupCache( memory_transaction_t *mem_op )
{
  #ifdef DEBUG_PSEQ
    DEBUG_OUT("pseq_t:warmupCache BEGIN\n");
  #endif

  cache_t *l1_cache;
  pa_t     physaddr = mem_op->s.physical_address;

  // case instruction or data cache  
  if ( mem_op->s.type == Sim_Trans_Instr_Fetch ) {
    l1_cache = l1_inst_cache;
  } else {
    l1_cache = l1_data_cache;
  }

  // read or write to the 1l caches, r/w the l2's on a miss
  if ( SIM_mem_op_is_write( &mem_op->s ) ) {
    if (! l1_cache->Write( physaddr, NULL ) )
      l2_cache->Write( physaddr, NULL );
  } else {
    if (! l1_cache->Read( physaddr, NULL, true, NULL ) )
      l2_cache->Read( physaddr, NULL, true, NULL );
  }
  
  localCycleIncrement();
  
  // Warm up the instruction or data cache, depending on type of reference
  if ( SIM_mem_op_is_instruction( &mem_op->s ) ) {
    l1_inst_cache->Warmup( mem_op->s.physical_address);
  } else {
    // mem_op->s.type is data Load or Store
    l1_data_cache->Warmup( mem_op->s.physical_address);
  }
  l2_cache->Warmup( mem_op->s.physical_address);

  #ifdef DEBUG_PSEQ
    DEBUG_OUT("pseq_t:warmupCache END\n");
  #endif
}

/** log the number of logical renames while we've been in flight */
//**************************************************************************
void pseq_t::logLogicalRenameCount( rid_type_t rid, half_t logical, 
                                    uint32 count )
{
  #ifdef DEBUG_PSEQ
  DEBUG_OUT("pseq_t:logLogicalRenameCount BEGIN\n");
  #endif

#ifdef RENAME_EXPERIMENT
  uint32  rindex;
  switch (rid) {
  case RID_INT:
  case RID_INT_GLOBAL:
    rindex = RID_INT;
    break;
  case RID_SINGLE:
  case RID_DOUBLE:
  case RID_QUAD:
    rindex = RID_SINGLE;
    break;
  case RID_CC:
    rindex = RID_CC;
    break;
  case RID_CONTROL:
    // ignore control registers as they are not renamed currently.
    return;
  default:
    ERROR_OUT("unknown logical register type: %d\n", rid);
    return;
  }

  if ( count >= PSEQ_REG_USE_HIST) {
    ERROR_OUT("logicalrenamecount: trimming %d\n", count);
    count = PSEQ_REG_USE_HIST - 1;
  }
  m_reg_use[rindex][logical][count]++;

  #ifdef DEBUG_PSEQ
  DEBUG_OUT("pseq_t:logLogicalRenameCount END\n");
  #endif

#endif
}

/** close the trace */
//**************************************************************************
void pseq_t::startTime( void )
{
  m_simulation_timer.startTimer();
}

/** close the trace */
//**************************************************************************
void pseq_t::stopTime( void )
{
  m_simulation_timer.stopTimer();
  m_simulation_timer.accumulateTime();
}

/*------------------------------------------------------------------------*/
/* Simics Interfaces                                                      */
/*------------------------------------------------------------------------*/

//**************************************************************************
void pseq_t::installInterfaces()
{
  #ifdef DEBUG_PSEQ
  DEBUG_OUT("pseq_t:installInterfaces BEGIN m_id[%d]\n", m_id);
  #endif

  // Note: this function is called on every SMT chip (see system.C)
 for(uint proc=0; proc <  CONFIG_LOGICAL_PER_PHY_PROC; ++proc){  
  if (CONFIG_IN_SIMICS) {
    ASSERT( m_mmu_access[proc] != NULL );      
    char mmuname[200];
    #ifdef DEBUG_PSEQ
      DEBUG_OUT("\tBEFORE getMMUName id[%d] iter[%d]\n",m_id+proc,proc);
    #endif
    pstate_t::getMMUName( m_id+proc, mmuname, 200 );       
    #ifdef DEBUG_PSEQ
      DEBUG_OUT("\tAFTER getMMUName id[%d] iter[%d] name[%s]\n",m_id+proc,proc,mmuname);
    #endif
    conf_object_t *mmu = SIM_get_object( mmuname );   
      #ifdef DEBUG_PSEQ
    DEBUG_OUT("\tAFTER SIM_get_object  id[%d] iter[%d]\n",m_id+proc,proc);
    #endif
    if (mmu == NULL) {
      ERROR_OUT("error: unable to locate object: %s\n", mmuname);
      SIM_HALT;
    }

    // FIXFIXFIX
    // Currently the MMU ASI is broken (as of 1.2.5 and 1.4.4).
    m_mmu_asi[proc] = NULL;  
#if 0
    m_mmu_asi[proc] = (mmu_interface_t *) SIM_get_interface( mmu, "asi" );
    if (m_mmu_asi[proc] == NULL) {
      ERROR_OUT("[%d] error: object does not implement \"asi\" interface: %s\n",
             m_id, mmuname);
      ERROR_OUT("     : you need to compile the spitfire-mmu object from the multifacet tree\n");
      SYSTEM_EXIT;
    }
#endif
    hfa_checkerr("install Interfaces");

    // get the primary context register of the sfmmu
    attr_value_t   index;
    index.kind       = Sim_Val_String;
    index.u.string   = "ctxt_primary";
    #ifdef DEBUG_PSEQ
       DEBUG_OUT("\tBEFORE SIM_get_attribute_idx id[%d] iter[%d]\n",m_id+proc,proc);
    #endif
    attr_value_t   ctxt_reg = SIM_get_attribute_idx(mmu, "registers", &index);
    #ifdef DEBUG_PSEQ
      DEBUG_OUT("\tAFTER SIM_get_attribute_idx id[%d] iter[%d] index[%d]\n",m_id+proc,proc,index);
    #endif
    //    ireg_t         ctxt_id  = 0;
    if (ctxt_reg.kind == Sim_Val_Integer) {
      //ctxt_id = ctxt_reg.u.integer;
      m_ctxt_id[proc] = ctxt_reg.u.integer;
    } else {
      ERROR_OUT("error: unable to read attribute on %s: registers[]\n",
                mmuname);
      SYSTEM_EXIT;
    }
    
    // We should only register the MMU_Register_Write callback when we are the last logical
    //          proc in the SMT chip... (after we have collected all the ctxt_ids from the logical procs)
    if(proc == (CONFIG_LOGICAL_PER_PHY_PROC - 1)){
      hap_type_t mmu_register_write = SIM_hap_get_number("MMU_Register_Write");

#ifdef SIMICS22X
      // verify the type safety of this call
      callback_arguments_t args      = SIM_get_hap_arguments( mmu_register_write , 0);
      const char          *paramlist = SIM_get_callback_argument_string( args );
      if (strcmp(paramlist, "nocII" )) {
        ERROR_OUT("error: system_t::installHapHandlers: expect hap to take parameters %s. Current simics executable takes: %s\n",
                  "nocII", paramlist );
        SYSTEM_EXIT;
      }
#endif

      //      m_mmu_haphandle = SIM_hap_install_callback_idx( mmu_register_write, 
      //                                (hap_func_t) &pseq_mmu_reg_handler,
      //                                                ctxt_id,
      //                                      (void *) this ); 

    m_mmu_haphandle = SIM_hap_add_callback_range( "MMU_Register_Write", 
                                                     (obj_hap_func_t) &pseq_mmu_reg_handler, 
                                                      (void *) this, getLowID(m_ctxt_id),
                                                      getHighID(m_ctxt_id)); 

      if (m_mmu_haphandle <= 0) {
        ERROR_OUT("error: installHapHandlers: mmu hap handle <= 0\n");
        SYSTEM_EXIT;
      }
    }  // end of MMU_Register_Write callback registration

    // copy all of simics's state into the pstate data structure
    // Make sure for now that this function is called only once for each SMT chip
    if(proc == 0){
        #ifdef DEBUG_PSEQ
           DEBUG_OUT("\tBEFORE checkPointState id[%d] iter[%d]\n",m_id+proc,proc);
         #endif
           //M_PSTATE->checkSparcIntf();

       M_PSTATE->checkpointState();   
          #ifdef DEBUG_PSEQ
            DEBUG_OUT("\tAFTER checkPointState id[%d] iter[%d]\n",m_id+proc,proc);
          #endif
     }

    // get the current context
    m_primary_ctx[proc] = M_PSTATE->getContext(proc);    
   } //end if(CONFIG_IN_SIMICS)

  // copy all simics's state into the in-order register files
  // Make sure for now that this function is called only once for each SMT chip
  if(proc == 0){
    #ifdef DEBUG_PSEQ
       DEBUG_OUT("\tBEFORE allocateIdealState id[%d] iter[%d]\n",m_id+proc,proc);
    #endif
     allocateIdealState();  
    #ifdef DEBUG_PSEQ
        DEBUG_OUT("\tAFTER allocateIdealState id[%d] iter[%d]\n",m_id+proc,proc);
    #endif
  }

  // reset the timers using the current instruction counts
  m_overall_timer[proc]->reset();   
  m_thread_timer[proc]->reset();

   // set the initial PC, nPC pair
   m_fetch_at[proc]->pc  = M_PSTATE->getControl(CONTROL_PC, proc);
   m_fetch_at[proc]->npc = M_PSTATE->getControl(CONTROL_NPC, proc);
   m_fetch_at[proc]->tl  = M_PSTATE->getControl(CONTROL_TL, proc);
   m_fetch_at[proc]->pstate = M_PSTATE->getControl( CONTROL_PSTATE, proc );
   m_fetch_at[proc]->cwp = M_PSTATE->getControl(CONTROL_CWP, proc);
   
   m_fetch_at[proc]->gset = pstate_t::getGlobalSet( m_fetch_at[proc]->pstate ); 
   if ( ISADDRESS32(m_fetch_at[proc]->pstate) ) {
     m_fetch_at[proc]->pc  = AM64( m_fetch_at[proc]->pc );
     m_fetch_at[proc]->npc = AM64( m_fetch_at[proc]->npc );
   }

   m_ooo.at[proc] = *(m_fetch_at[proc]);
   m_inorder[proc].at[proc] = *(m_fetch_at[proc]);
  
  // perform address translation (warm the itlb mini-cache)
   if (CONFIG_IN_SIMICS) {
     uint32 next_instr;
     getInstr( m_fetch_at[proc]->pc, m_fetch_at[proc]->tl, m_fetch_at[proc]->pstate,
               &(m_itlb_physical_address[proc]), &next_instr, proc );
     m_itlb_logical_address[proc]  = m_fetch_at[proc]->pc & PSEQ_OS_PAGE_MASK;
     m_itlb_physical_address[proc] = m_itlb_physical_address[proc] & PSEQ_OS_PAGE_MASK;
   }
   out_info("[%d]\tPC 0x%0llx\tNPC 0x%0llx\tctx 0x%x\n",
            m_id+proc, m_fetch_at[proc]->pc, m_fetch_at[proc]->npc, m_primary_ctx[proc] );
  
   // copy from the pstate structure into our data structures
   check_result_t  result;
   checkAllState( &result, &(m_ooo), proc );
   checkAllState( &result, &(m_inorder[proc]), proc );
   
  }  //end for loop over all logical processors

  #ifdef DEBUG_PSEQ
  DEBUG_OUT("pseq_t:installInterfaces END m_id[%d]\n", m_id);
  #endif
}

//**************************************************************************
void pseq_t::removeInterfaces( void )
{
    #ifdef DEBUG_PSEQ
  DEBUG_OUT("pseq_t:removeInterfaces BEGIN\n");
  #endif

  if (CONFIG_IN_SIMICS) {
    hap_type_t mmu_register_write = SIM_hap_get_number("MMU_Register_Write");
    if (mmu_register_write == 0) {
      ERROR_OUT("error: removeHapHandlers: system unable to get mmu hap event\n");
    }
    SIM_hap_delete_callback_id( "MMU_Register_Write", m_mmu_haphandle );
    hfa_checkerr("check: removeHapHandlers");
  }

  #ifdef DEBUG_PSEQ
  DEBUG_OUT("pseq_t:removeInterfaces END\n");
  #endif
}

//***************************************************************************
void pseq_t::fastforwardSimics(unsigned int proc )
{

   #ifdef DEBUG_PSEQ
  DEBUG_OUT("pseq_t:fastforwardSimics BEGIN m_id[%d]\n", m_id);
  #endif

  bool   hit_retry = false;
  uint32 count     = 0;
  bool   found;
  uint32 next_instr;
  ireg_t pc;
  pa_t   physical_pc;
  ireg_t traplevel;
  ireg_t pstate;

  // PSEQ_MAX_FF_LENGTH == arbitrary limit on how far this should go on
  while (hit_retry == false && count < PSEQ_MAX_FF_LENGTH - 3) {
    advanceSimics(proc);
    count++;

    // decode the current instruction
    pc        = M_PSTATE->getControl( CONTROL_PC, proc );
    traplevel = M_PSTATE->getControl( CONTROL_TL, proc );
    pstate    = M_PSTATE->getControl( CONTROL_PSTATE, proc );  

    if ( ISADDRESS32(pstate) ) {
      pc = AM64( pc );
    }

    // out_info("FF: 0x%0llx ", pc );
    found = getInstr( pc, traplevel, pstate,
                      &physical_pc, &next_instr, proc );
    if (found) {
      static_inst_t decoded_instr( physical_pc, next_instr);  

      #ifdef DEBUG_PSEQ
      // output 
      char buf[128];
      decoded_instr.printDisassemble(buf);
      DEBUG_OUT("FF: %s VPC[ 0x%llx ] PC[ 0x%llx ] count[ %d ]\n", buf, pc, physical_pc, count);
     #endif

      decoded_instr.setFlag( SI_TOFREE, true );
      if (decoded_instr.getOpcode() == i_retry) {
        hit_retry = true;
      }
    }
    else{
      #ifdef DEBUG_PSEQ
         DEBUG_OUT("FF: ITLB Miss, count[ %d ]\n", count);
      #endif
    }
  }

  // step past the retried instruction itself
  advanceSimics(proc);
  count++;

  // also print out the instruction skipped...
  // decode the current instruction
  pc        = M_PSTATE->getControl( CONTROL_PC, proc );
  traplevel = M_PSTATE->getControl( CONTROL_TL, proc );
  pstate    = M_PSTATE->getControl( CONTROL_PSTATE, proc );  

  if ( ISADDRESS32(pstate) ) {
    pc = AM64( pc );
  }

    // out_info("FF: 0x%0llx ", pc );
    found = getInstr( pc, traplevel, pstate,
                      &physical_pc, &next_instr, proc );
    if (found) {
      static_inst_t decoded_instr( physical_pc, next_instr);  
      
#ifdef DEBUG_PSEQ
      // output 
      char buf[128];
      decoded_instr.printDisassemble(buf);
      DEBUG_OUT("FF: %s VPC[ 0x%llx ] PC[ 0x%llx ] count[ %d ]\n", buf, pc, physical_pc, count);
#endif

      decoded_instr.setFlag( SI_TOFREE, true );
    }
    else{
      #ifdef DEBUG_PSEQ
         DEBUG_OUT("FF: ITLB Miss, count[ %d ]\n", count);
      #endif
    }

    /* END of fastforward */
  if ( count < PSEQ_MAX_FF_LENGTH ) {
    m_hist_ff_length[count]++;
  }

   #ifdef DEBUG_PSEQ
   DEBUG_OUT("pseq_t:fastforwardSimics: END m_id[%d] after %d instructions (%d)\n",
           m_id, count, hit_retry );
   #endif  
}

//***************************************************************************
bool pseq_t::advanceSimics(unsigned int proc )
{

  #ifdef DEBUG_PSEQ
     DEBUG_OUT("pseq_t:advanceSimics BEGIN m_id[%d]\n", m_id);
     printPC(&m_ooo.at[proc]);
  #endif

  ASSERT(proc < CONFIG_LOGICAL_PER_PHY_PROC);

  bool read_ok = true;
  if (CONFIG_IN_SIMICS == true) {
    // step simics some number of steps
    STAT_INC( m_stat_continue_calls[proc] );     

    // Turn off timing for faster simulations
    //m_retirement_timer.startTimer();
    if (CONFIG_MULTIPROCESSOR) {
      M_PSTATE->simcontinue( m_unchecked_retires[proc], proc );
    } else {
      SIM_continue( m_unchecked_retires[proc] );
    }
    if ( SIM_get_pending_exception() ) {
      // clear the exception
      SIM_clear_exception();
      out_info( "retire instruction error message: %s\n",
                SIM_last_error() );
      system_t::inst->breakSimulation();
    }

    // end retirement check timing
    //m_retirement_timer.stopTimer();
    //m_retirement_timer.accumulateTime();
  } else {
    // variables for reading the trace
    la_t  program_counter;   // the PC of the instruction
    pa_t  physical_pc;       // the address of the instruction
    unsigned int instr;      // the correct instruction at a given PC

    // read the next trace step
    for (uint32 i = 0; i < m_unchecked_retires[proc]; i++) {
      bool success = readTraceStep( program_counter, physical_pc, instr, proc );
      if (!success)
        read_ok = false;

      //IMPORTANT - this is the reason for different unchecked_instr buffers for each
      //           logical proc - bc it will overwrite the proc's ipage map!!!
      // If the instruction is different update the ipage map
      if ( success &&
           m_unchecked_instr[proc][i]->getStaticInst()->getInst() != instr ) {
        // INSTRUCTION_OVERWRITE
        /* out_info("overwriting vpc:0x%0llx pc:0x%0llx i:0x%0x new:0x%0x\n",
           program_counter, physical_pc, 
           m_unchecked_instr[i]->getStaticInst()->getInst(), instr );
        */
        // overwrite the existing instruction in the ipagemap

        abstract_pc_t  pc;
        pc.pc = program_counter;
        pc.tl = m_control_arf->getRetireRF(proc)->getInt( CONTROL_TL );
        insertInstruction( physical_pc, instr, proc );
      }
    }
  }

  #ifdef DEBUG_PSEQ
     printPC(&m_ooo.at[proc]);
    DEBUG_OUT("pseq_t:advanceSimics END m_id[%d]\n", m_id);
  #endif

  return (read_ok);
}

//**************************************************************************
void pseq_t::contextSwitch( context_id_t new_context, unsigned int proc )
{
  #ifdef DEBUG_PSEQ
    DEBUG_OUT("pseq_t:contextSwitch BEGIN m_id[%d] proc[%d]\n",m_id,proc);
  #endif

  ASSERT(proc < CONFIG_LOGICAL_PER_PHY_PROC);
  ASSERT(m_primary_ctx != NULL);

  // CTXT_SWITCH
  // out_info("seq: context: %u\n", new_context);

  #ifdef DEBUG_PSEQ
  DEBUG_OUT("\tbefore if statement new context = %x, m_primary_ctx[proc] = %x\n",new_context, m_primary_ctx[proc]);
  #endif
  if (new_context != m_primary_ctx[proc]) {

    // if we're taking a trace, emit the context switch...
    ASSERT(m_tracefp != NULL);
    ASSERT(m_branch_trace != NULL);

    if (m_tracefp[proc]) {
    
  #ifdef DEBUG_PSEQ
  DEBUG_OUT("\tbefore tracefp writeContextSwitch\n");
  #endif
      m_tracefp[proc]->writeContextSwitch( new_context );
  
  #ifdef DEBUG_PSEQ
  DEBUG_OUT("\tafter tracefp writeContextSwitch\n");
  #endif
    }
    
    if (m_branch_trace[proc]) {

  #ifdef DEBUG_PSEQ
  DEBUG_OUT("\tbefore branch_trace writeContextSwitch\n");
  #endif
      m_branch_trace[proc]->writeContextSwitch( new_context );


  #ifdef DEBUG_PSEQ
  DEBUG_OUT("\tafter branch_trace writeContextSwitch\n");
  #endif
    }
    
    // update the primary context
    m_primary_ctx[proc] = new_context;
  }

  #ifdef DEBUG_PSEQ
    DEBUG_OUT("pseq_t:contextSwitch END m_id[%d] proc[%d] \n",m_id,proc);
  #endif
}

/// remove memory timing interface
//***************************************************************************
void pseq_t::completedRequest( pa_t physicalAddr, bool abortRequest, OpalMemop_t type, int thread )
{
  bool found_cache = m_ruby_cache->checkOutstandingRequests(physicalAddr);
  bool fastpath_cache =  m_ruby_cache->checkFastPathOutstanding(physicalAddr);
  bool found_writebuffer = m_write_buffer->checkOutstandingRequests(physicalAddr);

  if( !( ( (found_cache || fastpath_cache) && (!found_writebuffer)) ||
         ( (!found_cache && !fastpath_cache) && (found_writebuffer) ) ||
          ( (fastpath_cache || found_cache) && found_writebuffer ) ) ) {
    DEBUG_OUT("\n**completedRequest REQUEST NOT FOUND!, cycle = %d\n",getLocalCycle());
    DEBUG_OUT("\tAddr = 0x%x\n",physicalAddr);
    DEBUG_OUT("found_cache = %d\n",found_cache);
    DEBUG_OUT("fastpath_cache = %d\n",fastpath_cache);
    DEBUG_OUT("found_writebuffer = %d\n",found_writebuffer);
    m_ruby_cache->print();
    m_write_buffer->print();
    m_write_buffer->checkOutstandingRequestsDebug(physicalAddr);

  }
  // 3 valid cases of requests w/ address physicalAddr outstanding:
  // 1) Request is only found in MSHR and not in Write Buffer - send completion to MSHR
  // 2) Request is only found in Write Buffer and not in MSHR - send completion to Write Buffer
  // 3) Request found in BOTH MSHR and Write Buffer - this occurs if a request was forwarded to MSHR (bc
  //        there was already a LD w/ same address outstanding in MSHR), but that ST could not be enqueued
  //        due to receiving NOT_READY, thus it doesn't show up in the MSHR until it can be enqueued! -
  //        send completion to MSHR bc MSHR has priority over Write Buffer
  ASSERT( ( (found_cache || fastpath_cache) && (!found_writebuffer)) ||
          ( (!found_cache && !fastpath_cache) && (found_writebuffer) ) || 
          ( (fastpath_cache || found_cache) && found_writebuffer ));

  if(found_cache || fastpath_cache){
    //request belongs to cache
    m_ruby_cache->complete( physicalAddr, abortRequest, type, thread );
  }
  else if(found_writebuffer){
    //request belongs to write buffer
    m_write_buffer->complete(physicalAddr, abortRequest, type, thread);
  }
}

//**************************************************************************
// inform rubycache that L2miss occurred
void pseq_t::markL2Miss(pa_t physicalAddr, OpalMemop_t type, int tagexists){
  m_ruby_cache->markL2Miss(physicalAddr, type, tagexists);
}

//*************************************************************************
// Update StoreSet predictor
void pseq_t::updateStoreSet(la_t store_vpc, la_t load_vpc){
  m_store_set_predictor->update(store_vpc, load_vpc);
}

//**************************************************************************
bool pseq_t::readPhysicalMemory( pa_t phys_addr, int size, ireg_t *result_reg, unsigned int proc )
{
   #ifdef DEBUG_PSEQ
  DEBUG_OUT("pseq_t:readPhysicalMemory BEGIN\n");
  #endif

  //proc needed so that we can try to speculatively read a logical proc's unchecked retired instr's values
  ASSERT(proc < CONFIG_LOGICAL_PER_PHY_PROC);

  // read memory ...
  if (PSEQ_MAX_UNCHECKED > 1) {
    memory_inst_t *uncheckedRetire = uncheckedValueForward( phys_addr, size, proc );  
    if ( uncheckedRetire != NULL ) {
      if ( uncheckedRetire->isDataValid() ) {
        // try to forward the value from this instruction
        byte_t  length = uncheckedRetire->getAccessSize();
      
        // forward value from store to load, by copying byte-by-byte
      
        // get the proper offset into the data
        int index1 = (int) (uncheckedRetire->getPhysicalAddress() - phys_addr);
        if ( index1 < 0 ) {
          ERROR_OUT( "readPhysicalMemory(): warning: negative offset: %d\n",
                     index1);
          return false;
        }
      
        char *p1 = (char *) uncheckedRetire->getData();
        char *p2 = (char *) result_reg;
        // out_info("copying: size %d, index1 %d\n", size, index1);
        while ( size > 0 ) {
          if ( index1 >= length ) {
            ERROR_OUT("readPhysicalMemory(): warning: array bounds read sz=%d (%d on %d).\n", size, index1, length);
            return (false);
          }
          *p2++ = p1[index1++];
          size--;
        }
        return (true);
      } else {
        // since we know memory is written, but we're not sure of the value
        // just give up on this load.
        // ERROR_OUT("data not valid\n");
        return (false);
      }
    }
  }

    #ifdef DEBUG_PSEQ
  DEBUG_OUT("pseq_t:readPhysicalMemory END\n");
  #endif

  return ( M_PSTATE->readPhysicalMemory( phys_addr, size, result_reg, proc ) );
}

//**************************************************************************
trap_type_t pseq_t::mmuAccess( la_t address, uint16 asi,
                               uint16 accessSize, OpalMemop_t op,
                               uint16 pstate, uint16 tl, pa_t *physAddress,
                               unsigned int proc, bool inquiry)
{
    #ifdef DEBUG_PSEQ
  DEBUG_OUT("pseq_t:mmuAccess BEGIN\n");
  #endif

  ASSERT(proc < CONFIG_LOGICAL_PER_PHY_PROC);

  memory_transaction_t mem_op;
  exception_type_t e = Sim_PE_No_Exception;

  memset( &mem_op, 0, sizeof(mem_op) );
  mem_op.s.logical_address = address;
  mem_op.s.physical_address = 0;
  mem_op.s.size = accessSize;

  switch (op) {
  case OPAL_LOAD:
    mem_op.s.type = Sim_Trans_Load;
    break;
  case OPAL_STORE:
    mem_op.s.type = Sim_Trans_Store;
    break;
  case OPAL_IFETCH:
    mem_op.s.type = Sim_Trans_Instr_Fetch;
    break;
  case OPAL_ATOMIC:
    mem_op.s.type = Sim_Trans_Store;
    break;
  default:
    ERROR_OUT("mmuAccess: Wrong type of memory access!");
  }

  if( inquiry ){
    mem_op.s.inquiry        = 1;
    //also set speculative flag to 0
    mem_op.s.speculative = 0;
  }
  else{
    //WARNING: this query WILL update the MMU fault registers!
    mem_op.s.inquiry       = 0;
    //indicate we want to update the MMU regs via speculative flag
    mem_op.s.speculative = 1;
  }
  mem_op.s.inverse_endian = 0;
  mem_op.s.ini_type    = Sim_Initiator_CPU;
  mem_op.s.ini_ptr     = M_PSTATE->getSimicsProcessor(proc);   

  /*
   * Task: get the current pstate, am, priv, cle bits
   */

  mem_op.priv             = (pstate >> 2) & 0x1;
  mem_op.s.inverse_endian = (pstate >> 9) & 0x1;
  mem_op.address_space    = asi;
  int address_mask        = (pstate >> 3) & 0x1;
  if (address_mask) {
    mem_op.s.logical_address = AM64( mem_op.s.logical_address );
  }

  // The ASI must be set at this point (not uninit or IMPLICIT)
  ASSERT( asi != (uint16) -1 && asi != 0x100 );
  ASSERT( m_mmu_access[proc] != NULL );      
  
  // inside simics, call interface to query physical address
  e = (*m_mmu_access[proc]->logical_to_physical)( M_PSTATE->getSimicsMMU(proc),&mem_op );   
  if ( e == Sim_PE_No_Exception ) {
    *physAddress = mem_op.s.physical_address;

    ASSERT(physAddress != NULL);
   #ifdef DEBUG_PSEQ
  DEBUG_OUT("pseq_t:mmuAccess (no exception) END\n");
  #endif

    return ( Trap_NoTrap );
  }
  if ( (int) e > (int) Sim_PE_No_Exception ) {
#ifdef DEBUG_LSQ
    // else: an exception occured during translation -- cause an exception
    // check to see if simics is returning a real or pseudo exception...
    out_info("mmuAccess: pseudo exception: addr 0x%0llx pa 0x%0llx asi 0x%0x err=0x%x\n",
             address, mem_op.s.physical_address, asi, (int) e);
#endif
    *physAddress = mem_op.s.physical_address;

    ASSERT(physAddress != NULL);
   #ifdef DEBUG_PSEQ
  DEBUG_OUT("pseq_t:mmuAccess (pseudo exception) END\n");
  #endif

    return Trap_NoTrap;
  }

#ifdef DEBUG_LSQ
  out_info("mmuAccess: exception=0x%0x: va:0x%0llx ma:0x%0llx %c %c pa:0x%0llx pstate:0x%0x tl:0x%x asi:0x%0x result:0x%0x\n",
           (int) e, address, mem_op.s.logical_address,
           (mem_op.s.type == Sim_Trans_Load || mem_op.s.type == Sim_Trans_Store) ? 'D' : 'I',
           (op == OPAL_STORE) ? 'W' : 'R',
           mem_op.s.physical_address, pstate, tl, asi, e);
#endif

  #ifdef DEBUG_PSEQ
    DEBUG_OUT("pseq_t:mmuAccess (real exception) END\n");
  #endif

    /*
    if( e == Trap_Fast_Instruction_Access_MMU_Miss){
      //try it again
      e = Trap_NoTrap;
      e = (*m_mmu_access[proc]->logical_to_physical)( M_PSTATE->getSimicsMMU(proc),&mem_op ); 
      if( e != Trap_Fast_Instruction_Access_MMU_Miss){
        ERROR_OUT("\nmmuAccess: ERROR: ITLB miss exception doesn't appear 2nd time! cycle[ %lld ]\n", m_local_cycles);
        ASSERT(0);
      }
    }
    */

  return ((trap_type_t) e);
}

//**************************************************************************
bool pseq_t::getInstr( la_t cur_pc, int32 traplevel, int32 pstate,
                       pa_t *physical_pc, unsigned int *next_instr_p, unsigned int proc )
{
  //proc is the logical processor number
  #ifdef DEBUG_PSEQ
    DEBUG_OUT("pseq_t:getInstr BEGIN proc[%d] cur_pc[%d] traplevel[%d] pstate[%0x]\n",
        proc, cur_pc, traplevel, pstate);
  #endif

  ASSERT(proc < CONFIG_LOGICAL_PER_PHY_PROC);

  // translate from logical to physical memory
  uint16 asi;

  if (traplevel == 0) {
    asi = ASI_PRIMARY;
  } else {
    asi = ASI_NUCLEUS;
  }
  trap_type_t t = mmuAccess( cur_pc, asi, 4, OPAL_IFETCH, pstate, traplevel,
                             physical_pc, proc, true );
  if (t == Trap_NoTrap) {
    #ifdef DEBUG_PSEQ
    DEBUG_OUT("\tabout to read phys memory, no exception prc[%d]\n",proc);
    #endif
    ASSERT(M_PSTATE->getSimicsProcessor(proc) != NULL);
    //modified to take in logical proc number
    *next_instr_p = SIM_read_phys_memory( (processor_t *) M_PSTATE->getSimicsProcessor(proc),
                                          *physical_pc, 4 );
    ASSERT(next_instr_p != NULL);
  } else {
    // ITLB Miss
    // emit a special token at this point
    int isexcept = SIM_get_pending_exception();
    if ( isexcept != 0 ) {
      sim_exception_t except_code = SIM_clear_exception();
      // ignore Memory exception at this point -- no translation
      if (except_code != SimExc_Memory) {
        ERROR_OUT( "getInstr: Exception error message: %s\n",
                   SIM_last_error() );
      }
    }
    *next_instr_p = STATIC_INSTR_MOP;
    return (false);
  }
  // hfa_checkerr("getInstr: after SIM_read_phys_memory");

  #ifdef DEBUG_PSEQ
    DEBUG_OUT("pseq_t:getInstr next_instr[%0x] physical_pc[%0x] proc[%x]END\n", *next_instr_p,*physical_pc, proc);
  #endif

  return (true);
}

/**
 * read an instruction from a given current program counter
 */
//**************************************************************************
bool pseq_t::getInstr( la_t cur_pc, 
                       pa_t *physical_pc, unsigned int *next_instr_p, unsigned int proc )
{
  ASSERT(proc < CONFIG_LOGICAL_PER_PHY_PROC);

  // implicitly get the trap level, and pstate from simics
  ireg_t traplevel = M_PSTATE->getControl( CONTROL_TL, proc );
  ireg_t pstate    = M_PSTATE->getControl( CONTROL_PSTATE, proc );
  return (getInstr( cur_pc, traplevel, pstate, physical_pc, next_instr_p, proc ));
}

//**************************************************************************
void pseq_t::postException( uint32 exception, unsigned int proc )
{
  #ifdef DEBUG_PSEQ
  DEBUG_OUT("pseq_t:postException BEGIN\n");
  #endif

  ASSERT(proc < CONFIG_LOGICAL_PER_PHY_PROC);
  ASSERT( exception < TRAP_NUM_TRAP_TYPES);

  m_simtrapstat[proc][exception]++;

  #ifdef DEBUG_PSEQ
     DEBUG_OUT("postException BEGIN INTERRUPT 0x%x\n", m_posted_interrupt[proc]);
  #endif
   // if this exception is one that opal can't model, set the interrupt flag
  //    The interrupt flag is checked as instructions retire
  if ( exception >= Trap_Interrupt_Level_1 &&
       exception <= Trap_Interrupt_Vector ) {
#ifdef PIPELINE_VIS
    if (system_t::inst->isSimulating()) {
      out_info("postedException: exception 0x%0x\n", exception );
    }
#endif
    m_posted_interrupt[proc] = (trap_type_t) exception;
  }

  // set the last Simics exception here
  m_last_simexception[proc] = (trap_type_t ) exception;

 #ifdef DEBUG_PSEQ
     DEBUG_OUT("postException END INTERRUPT 0x%x\n", m_posted_interrupt[proc]);
  #endif

    #ifdef DEBUG_PSEQ
  DEBUG_OUT("pseq_t:postException END\n");
  #endif

}

//**************************************************************************
int32 pseq_t::getPid(unsigned int proc )
{
  // now: "thread" is the virtual address address of the thread pointer--
  la_t thread = getCurrentThread(proc);
  return getPid( thread, proc );   
}

//**************************************************************************
int32 pseq_t::getPid( la_t thread_p, unsigned int proc )  
{
    #ifdef DEBUG_PSEQ
  DEBUG_OUT("pseq_t:getPid BEGIN\n");
  #endif

  // dereference virtual addresses plus offset (Solaris 9)
  la_t procp    = M_PSTATE->dereference( thread_p + 0x110, 8, proc );
  la_t pidp     = M_PSTATE->dereference( procp + 0xb0, 8, proc );
  int32 pid     = M_PSTATE->dereference( pidp + 0x4, 4, proc );

#if 0
  // Solaris 8
  la_t procp    = M_PSTATE->dereference( thread + 0x130, 8 );
  la_t pidp     = M_PSTATE->dereference( procp + 0xb0, 8 );
  int32 pid     = M_PSTATE->dereference( pidp + 0x4, 4 );
#endif
  
  // can find the hardware context too...
#if 0    
  la_t as   = M_PSTATE->dereference( procp + 0x8, 8 );
  la_t hat  = M_PSTATE->dereference( as + 0x10, 8 );
  la_t ctx  = M_PSTATE->dereference( hat + 0x2e, 2 );
#endif

  #ifdef DEBUG_PSEQ
  DEBUG_OUT("pseq_t:getPid END\n");
  #endif

  return pid;
}

//**************************************************************************
la_t  pseq_t::getCurrentThread(unsigned int proc )
{
  #ifdef DEBUG_PSEQ
  DEBUG_OUT("pseq_t:getCurrentThread BEGIN\n");
  #endif

  ASSERT(proc < CONFIG_LOGICAL_PER_PHY_PROC);

  // previously: read thread from cpu_list offset
  // la_t   thread   = dereference( cpu_list + 0x10, 8 );

  // now: read thread pointer directly from physical memory
  la_t thread = 0;
  bool success = M_PSTATE->readPhysicalMemory( m_thread_physical_address[proc],
                                               8, (ireg_t *) &thread, proc );
  if (!success) {
    out_info( "getCurrentThread: read physical memory failed: 0x%0llx\n",
              thread );
  }

  #ifdef DEBUG_PSEQ
  DEBUG_OUT("pseq_t:getCurrentThread END\n");
  #endif

  return thread;
}

//**************************************************************************
void pseq_t::setThreadPhysAddress( la_t thread_phys_addr, unsigned int proc )
{
  ASSERT(proc <  CONFIG_LOGICAL_PER_PHY_PROC);

  m_thread_physical_address[proc] = thread_phys_addr;
}

//**************************************************************************
la_t pseq_t::getThreadPhysAddress(unsigned int proc )
{
  ASSERT(proc <  CONFIG_LOGICAL_PER_PHY_PROC);
  return m_thread_physical_address[proc];
}


/*------------------------------------------------------------------------*/
/* Configuration Interfaces                                               */
/*------------------------------------------------------------------------*/

/// register checkpoint interfaces
//**************************************************************************
void  pseq_t::registerCheckpoint( void ) 
{
  #ifdef DEBUG_PSEQ
  DEBUG_OUT("pseq_t:registerCheckpoint BEGIN\n");
  #endif

  /* caches: */
  if(!CONFIG_WITH_RUBY) {
    l1_data_cache->registerCheckpoint( m_conf );
    l1_inst_cache->registerCheckpoint( m_conf );
    l2_cache->registerCheckpoint( m_conf );
  }

  /* branch predictors */
  if ( !strcmp( BRANCHPRED_TYPE, "YAGS" ) ) {
    ((yags_t *) m_bpred)->registerCheckpoint( m_conf );
  }
  m_ipred->registerCheckpoint( m_conf );

  #ifdef DEBUG_PSEQ
  DEBUG_OUT("pseq_t:registerCheckpoint END\n");
  #endif

}

/// begin writing new checkpoint
//**************************************************************************
void  pseq_t::writeCheckpoint( char *checkpointName )
{
  #ifdef DEBUG_PSEQ
  DEBUG_OUT("pseq_t:writeCheckpoint BEGIN\n");
  #endif

  int rc = m_conf->writeConfiguration( checkpointName  );
  if ( rc < 0 )
    ERROR_OUT("error: unable to write checkpoint: %s\n", checkpointName);

  #ifdef DEBUG_PSEQ
  DEBUG_OUT("pseq_t:writeCheckpoint END\n");
  #endif
}

/// open a checkpoint for reading
//**************************************************************************
void  pseq_t::readCheckpoint( char *checkpointName )
{
  #ifdef DEBUG_PSEQ
  DEBUG_OUT("pseq_t:readCheckpoint BEGIN\n");
  #endif

  int rc = m_conf->readConfiguration( checkpointName, OPAL_RELATIVE_PATH );
  if ( rc < 0 )
    ERROR_OUT("error: unable to read checkpoint: %s\n", checkpointName);

  #ifdef DEBUG_PSEQ
  DEBUG_OUT("pseq_t:readCheckpoint END\n");
  #endif
}

/*------------------------------------------------------------------------*/
/* Trace Interfaces                                                       */
/*------------------------------------------------------------------------*/

//**************************************************************************
void pseq_t::openTrace( char *traceFileName, unsigned int proc )
{
  #ifdef DEBUG_PSEQ
  DEBUG_OUT("pseq_t:openTrace BEGIN\n");
  #endif

  ASSERT(proc < CONFIG_LOGICAL_PER_PHY_PROC);

  // read the primary context from the mmu
  int primary_ctx = M_PSTATE->getContext(proc);
  m_tracefp[proc] = new tracefile_t( traceFileName, M_PSTATE->getProcessorState(proc),
                               primary_ctx );

  //should we free up old memory?
  if(m_memtracefp[proc])
    delete m_memtracefp[proc];

  m_memtracefp[proc] = new memtrace_t( traceFileName, true );

  #ifdef DEBUG_PSEQ
  DEBUG_OUT("pseq_t:openTrace END\n");
  #endif

}

//**************************************************************************
void pseq_t::writeTraceStep(unsigned int proc )
{

  #ifdef DEBUG_PSEQ
  DEBUG_OUT("pseq_t:writeTraceStep BEGIN\n");
  #endif

  ASSERT(proc < CONFIG_LOGICAL_PER_PHY_PROC);

  bool         rc;             // Return Code
  la_t         curPC = 0;
  pa_t         physicalPC = 0;
  unsigned int instr;

  // read current PC
  curPC = M_PSTATE->getControl( CONTROL_PC, proc );
  // read instruction from memory
  rc    = getInstr(curPC, &physicalPC, &instr, proc);
  
  if (rc) {
    // read simics's state changes
    // Do this only once for each SMT chip:
    if(proc == 0){
      M_PSTATE->checkpointState();
    }

    // write the PC and any difference between the current state and this state
    //timecheck  pc_step_t    newtime;
    //timecheck  newtime = M_PSTATE->getTime();
    //timecheck  m_local_icount = newtime - m_simics_time;
    m_tracefp[proc]->writeTraceStep( curPC, physicalPC, instr, m_local_icount[proc] );
    m_local_icount[proc]++;
    //timecheck  m_simics_time = newtime;
    
  } else {
    // else our last instruction read failed (e.g. probably an itlb miss)
    // ERROR_OUT("error: pc read failed\n");
  }

  #ifdef DEBUG_PSEQ
  DEBUG_OUT("pseq_t:writeTraceStep END\n");
  #endif

}


//**************************************************************************
void pseq_t::writeSkipTraceStep(unsigned int proc )
{

  #ifdef DEBUG_PSEQ
  DEBUG_OUT("pseq_t:writeSkipTraceStep BEGIN\n");
  #endif

  ASSERT(proc < CONFIG_LOGICAL_PER_PHY_PROC);

  bool         rc;             // Return Code
  la_t         curPC  = 0;
  la_t         curNPC = 0;
  pa_t         physicalPC = 0;
  pa_t         physicalNPC = 0;
  unsigned int instr;

  // read current PC
  curPC = M_PSTATE->getControl( CONTROL_PC, proc );
  // read instruction from memory
  rc    = getInstr(curPC, &physicalPC, &instr, proc);
  
  if (rc) {

    // just write the PC
    m_tracefp[proc]->writeTracePC( curPC, physicalPC, instr, m_local_icount[proc] );
    m_local_icount[proc]++;
  } else {
    // else our last instruction read failed (e.g. probably an itlb miss)
    // ERROR_OUT("error: pc read failed (1) 0x%0llx\n", curPC);
  }

  // read "next" instruction
  curNPC = M_PSTATE->getControl( CONTROL_NPC, proc );
  rc     = getInstr(curNPC, &physicalNPC, &instr, proc);
  
  if (rc) {
    // read simics's state changes
    // Call this function only once for each SMT chip:
    if(proc == 0){
      M_PSTATE->checkpointState();
     }
    
    // write the PC and any difference between the current state and this state
    m_tracefp[proc]->writeTraceStep( curNPC, physicalNPC, instr, m_local_icount[proc] );
    m_local_icount[proc]++;
    
  } else {
    // else our last instruction read failed (e.g. probably an itlb miss)
    // ERROR_OUT("error: pc read failed (2) 0x%0llx\n", curNPC);
  }

  #ifdef DEBUG_PSEQ
  DEBUG_OUT("pseq_t:writeSkipTraceStep END\n");
  #endif
}

//**************************************************************************
void pseq_t::closeTrace(unsigned int proc )
{

  #ifdef DEBUG_PSEQ
  DEBUG_OUT("pseq_t:closeTrace BEGIN\n");
  #endif

  ASSERT(proc < CONFIG_LOGICAL_PER_PHY_PROC);

  if (m_tracefp[proc]) {
    delete m_tracefp[proc];
    m_tracefp[proc] = NULL;
  } 
  if (m_memtracefp[proc]) {
    delete m_memtracefp[proc];
    m_memtracefp[proc] = NULL;
  }

  #ifdef DEBUG_PSEQ
  DEBUG_OUT("pseq_t:closeTrace END\n");
  #endif
}

//**************************************************************************
void pseq_t::attachTrace( char *traceFileName, bool withImap, unsigned int proc )
{
  #ifdef DEBUG_PSEQ
  DEBUG_OUT("pseq_t:attachTrace BEGIN\n");
  #endif

  pa_t   physical_pc;       // the address of the instruction
  la_t   pc;
  uint32 instr;

  ASSERT(proc < CONFIG_LOGICAL_PER_PHY_PROC);

  // must attach to memory trace first!
  if (withImap) {
    ASSERT( m_memtracefp[proc] != NULL );
  }

  //  DEBUG_OUT("***LUKE : before tracefile_t constructor\n");

  m_tracefp[proc] = new tracefile_t( traceFileName, M_PSTATE->getProcessorState(proc),
                               &m_primary_ctx[proc] );

  // DEBUG_OUT("****LUKE : after tracefile_t constructor\n");
  
  if (m_tracefp[proc]) {
    out_info("Initial primary context: 0x%0x\n", m_primary_ctx[proc]);

    // step the trace one cycle
    if (!readTraceStep( pc, physical_pc, instr, proc )) {    
      ERROR_OUT("pseq_t: error: unexpected end of trace.\n");
      ERROR_OUT("        Is the trace attached correctly?\n");
      SYSTEM_EXIT;
    }
    
    // set the initial PC, nPC pair
    m_fetch_at[proc]->pc  = M_PSTATE->getControl(CONTROL_PC, proc);
    m_fetch_at[proc]->npc = M_PSTATE->getControl(CONTROL_NPC, proc);
    m_fetch_at[proc]->tl  = M_PSTATE->getControl(CONTROL_TL, proc);
    m_fetch_at[proc]->pstate = M_PSTATE->getControl(CONTROL_PSTATE, proc);
    m_fetch_at[proc]->cwp = M_PSTATE->getControl(CONTROL_CWP, proc);
    m_fetch_at[proc]->gset = pstate_t::getGlobalSet( m_fetch_at[proc]->pstate );
    out_info("PC 0x%0llx NPC 0x%0llx\n", m_fetch_at[proc]->pc, m_fetch_at[proc]->npc );

    // write all simics's registers into our registers
    check_result_t result;
    checkAllState( &result, &(m_ooo), proc );

  } else { // end if tracing
    ERROR_OUT("error: unable to attach to trace %s.\n",
              traceFileName);
  }

  #ifdef DEBUG_PSEQ
  DEBUG_OUT("pseq_t:attachTrace END\n");
  #endif
}

//***************************************************************************
void pseq_t::attachMemTrace( char *traceFilename, unsigned int proc )
{ 
  ASSERT(proc < CONFIG_LOGICAL_PER_PHY_PROC);
  m_memtracefp[proc] = new memtrace_t( traceFilename );
}

//***************************************************************************
void pseq_t::attachTLBTrace( char *traceFileName, unsigned int proc )
{
  #ifdef DEBUG_PSEQ
  DEBUG_OUT("pseq_t:attachTLBTrace BEGIN\n");
  #endif

  ASSERT(proc < CONFIG_LOGICAL_PER_PHY_PROC);
 
  // Currently each thread has its own TLB, should we change the individual
  //         TLB sizes accordingly?
  m_standalone_tlb[proc] = new dtlb_t( TLB_NUM_ENTRIES, TLB_NUM_PAGE_SIZES,
                                 TLB_PAGE_SIZES );
  
  FILE *fp = fopen( traceFileName, "r" );
  if (fp == NULL) {
    ERROR_OUT("error: unable to attach to TLB file: %s\n",
              traceFileName);
    SYSTEM_EXIT;
  }
  DEBUG_OUT("Reading TLB translation information...\n");
  m_standalone_tlb[proc]->readTranslation( fp );

  #ifdef DEBUG_PSEQ
  DEBUG_OUT("pseq_t:attachTLBTrace END\n");
  #endif
}

//**************************************************************************
void pseq_t::writeTraceMemop( transaction_t *trans, unsigned int proc )
{

  #ifdef DEBUG_PSEQ
  DEBUG_OUT("pseq_t:writeTraceMemop BEGIN\n");
  #endif

  ASSERT(proc < CONFIG_LOGICAL_PER_PHY_PROC);

  bool         rc;             // Return Code
  pa_t         physicalPC = 0;
  la_t         curPC = 0;
  unsigned int instr;

  // read current PC
  curPC = M_PSTATE->getControl( CONTROL_PC, proc );
  // read instruction from memory
  rc    = getInstr(curPC, &physicalPC, &instr, proc);

  if (m_memtracefp[proc]) {
    m_memtracefp[proc]->writeTransaction( curPC, instr, m_local_icount[proc], trans );
  }

  #ifdef DEBUG_PSEQ
  DEBUG_OUT("pseq_t:writeTraceMemop END\n");
  #endif
}

//**************************************************************************
void pseq_t::openBranchTrace( char *traceFileName, unsigned int proc )
{
  #ifdef DEBUG_PSEQ
  DEBUG_OUT("pseq_t:openBranchTrace BEGIN\n");
  #endif

  ASSERT(proc < CONFIG_LOGICAL_PER_PHY_PROC);

  // read the primary context from the mmu
  int primary_ctx = M_PSTATE->getContext(proc);
  
  m_branch_trace[proc] = new branchfile_t( traceFileName, primary_ctx );

  #ifdef DEBUG_PSEQ
  DEBUG_OUT("pseq_t:openBranchTrace END\n");
  #endif
}

//**************************************************************************
void pseq_t::writeBranchStep(unsigned int proc )
{
  #ifdef DEBUG_PSEQ
  DEBUG_OUT("pseq_t:writeBranchStep BEGIN\n");
  #endif

  ASSERT(proc < CONFIG_LOGICAL_PER_PHY_PROC);

  abstract_pc_t at;
  pa_t          ppc;
  branch_type_t branch_type;
  static_inst_t *s_instr;
  
  at.pc      = M_PSTATE->getControl( CONTROL_PC, proc );
  at.npc     = M_PSTATE->getControl( CONTROL_NPC, proc );
  at.tl      = M_PSTATE->getControl( CONTROL_TL, proc );
  at.pstate  = M_PSTATE->getControl( CONTROL_PSTATE, proc );

  ASSERT( CONFIG_IN_SIMICS == true );

  // check last branch, if any
  m_branch_trace[proc]->checkLastBranch( at.pc, at.npc );

  lookupInstruction( &at, &s_instr, &ppc, proc ); 
  if (s_instr != m_fetch_itlbmiss[proc]) {
    branch_type = s_instr->getBranchType();
    if (branch_type == BRANCH_COND ||
        branch_type == BRANCH_PCOND) {
      // write instruction
      m_branch_trace[proc]->writeBranch( at.pc, at.npc, at.tl, s_instr );
    }
  }

  #ifdef DEBUG_PSEQ
  DEBUG_OUT("pseq_t:writeBranchTrace END\n");
  #endif
}

//**************************************************************************
bool pseq_t::writeBranchNextFile(unsigned int proc )
{
  ASSERT(proc < CONFIG_LOGICAL_PER_PHY_PROC);

  return (m_branch_trace[proc]->writeNextFile());
}

//**************************************************************************
void pseq_t::closeBranchTrace(unsigned int proc )
{
  ASSERT(proc < CONFIG_LOGICAL_PER_PHY_PROC);

  if (m_branch_trace[proc]) {
    delete m_branch_trace[proc];
    m_branch_trace[proc] = NULL;
  } 
}

//**************************************************************************
bool pseq_t::readTraceStep( la_t &vpc, pa_t &ppc, unsigned int &instr, unsigned int proc )
{

  #ifdef DEBUG_PSEQ
  DEBUG_OUT("pseq_t:readTraceStep BEGIN\n");
  #endif

  ASSERT(proc < CONFIG_LOGICAL_PER_PHY_PROC);

  bool          success;
  uint32        context;
  uint32        localtime;

  if (m_tracefp[proc]) {
    context = m_primary_ctx[proc];

    //    DEBUG_OUT("*****BEFORE readTraceStep\n");

    success = m_tracefp[proc]->readTraceStep( vpc, ppc, instr, localtime, context );

    // DEBUG_OUT("****AFTER readTraceStep, success[%d]\n",success);

    // check to see if context has changed
    if (context != m_primary_ctx[proc]) {
      m_primary_ctx[proc] = context;
    }

    // update the local time
    m_local_icount[proc] = localtime;

    // If the instruction is different update the ipage map
    static_inst_t *s_instr = NULL;
    abstract_pc_t  pc;
    pc.pc   = vpc;
    pc.tl   = M_PSTATE->getControl(CONTROL_TL, proc);
    bool inMap = queryInstruction( ppc, s_instr, proc );  
    if ( success && inMap &&
         s_instr->getInst() != instr ) {
      out_info("overwriting vpc:0x%0llx pc:0x%0llx i:0x%0x new:0x%0x\n",
               vpc, ppc, s_instr->getInst(), instr );
      
      // overwrite the existing instruction in the ipagemap
      insertInstruction( ppc, instr, proc );   
    }
    
    // scan in the memory trace
    if (m_memtracefp[proc]) {
#ifdef PIPELINE_VIS
      out_log("set localtime %d\n", localtime);
#endif
      m_memtracefp[proc]->advanceToTime( localtime );
    }
    
  } else {
    success = false;
  }

  #ifdef DEBUG_PSEQ
  DEBUG_OUT("pseq_t:readTraceStep END\n");
  #endif

  return (success);
}

//***************************************************************************
bool pseq_t::queryInstruction( pa_t fetch_ppc, static_inst_t * &s_instr, unsigned int proc )
{
  ASSERT(proc < CONFIG_LOGICAL_PER_PHY_PROC);

  return ( m_imap[proc]->queryInstr( fetch_ppc, s_instr ));
}
  
//***************************************************************************
static_inst_t *pseq_t::insertInstruction( pa_t fetch_ppc, unsigned int instr, unsigned int proc )
{
   #ifdef DEBUG_PSEQ
  DEBUG_OUT("pseq_t:insertInstruction BEGIN\n");
  #endif

  ASSERT(proc < CONFIG_LOGICAL_PER_PHY_PROC);

  uint16         status;
  static_inst_t *s_instr;
  
  // out_info("inserting:: 0x%0llx 0x%0x\n", a->pc, instr );

  // insert this instruction into the physical page map
  status = m_imap[proc]->insertInstr( fetch_ppc, instr, s_instr );  

  // observe this static instruction
  system_t::inst->m_sys_stat->observeStaticInstruction( this, s_instr, proc );  

  #ifdef DEBUG_PSEQ
  
  DEBUG_OUT("pseq_t:insertInstruction END\n");
  #endif

  return (s_instr);
}

//**************************************************************************
void pseq_t::invalidateInstruction( pa_t address, unsigned int proc )
{
  ASSERT(proc < CONFIG_LOGICAL_PER_PHY_PROC);

  // invalidate the decoded instructions in this physical address
  m_imap[proc]->invalidateInstr( address );
}

//**************************************************************************
void pseq_t::writeInstructionTable( char *imapFileName, unsigned int proc )
{
  ASSERT(proc < CONFIG_LOGICAL_PER_PHY_PROC);

  char     filename[FILEIO_MAX_FILENAME];

  // save the instruction map
  sprintf(filename, "%s.map", imapFileName );
  bool success = m_imap[proc]->saveFile( filename );
  if (!success) {
    ERROR_OUT("sequencer: error writing the imap table: %s\n", filename);
  }
}

//**************************************************************************
bool pseq_t::readInstructionTable( char *imapFileName, int context, unsigned int proc )
{
   #ifdef DEBUG_PSEQ
  DEBUG_OUT("pseq_t:readInstructionTable BEGIN\n");
  #endif

  // printf("*****readinstructiontable, imapfilename=[%s]\n",imapFileName);

  char        filename[FILEIO_MAX_FILENAME];

  ASSERT(proc < CONFIG_LOGICAL_PER_PHY_PROC);
  ASSERT(m_imap != NULL);

  sprintf(filename, "%s.map", imapFileName);

  //should we do this to free up memory?
  //  if(m_imap[proc])
  //  delete m_imap[proc];

  //  printf("*****readinstructiontable, imapfilename=[%s]\n",imapFileName);
  //  printf("******readInstructionTable, filename=[%s]\n",filename);

  ipagemap_t *imap = new ipagemap_t(PSEQ_IPAGE_TABLESIZE);
  bool        success = imap->readFile(filename);  
  if (success) {
    m_imap[proc] = imap;
  } else {
    ERROR_OUT("error reading imap file\n");
    return false;
  }

  #ifdef DEBUG_PSEQ
  DEBUG_OUT("pseq_t:readInstructionTable END\n");
  #endif

  return true;
}

//***************************************************************************
void pseq_t::allocateFlatRegBox( mstate_t &inorder, uint32 proc )
{
  ASSERT(proc < CONFIG_LOGICAL_PER_PHY_PROC);

  #ifdef DEBUG_PSEQ
  DEBUG_OUT("pseq_t:allocateFlatRegBox BEGIN\n");
  #endif

  reg_box_t &rbox = inorder.regs;
  // Here is one location where the pstate object must be initialized
  // (in system, you should NOT be trying to create pseq before pstate objs).

  // The problem here is that now flat register files are allowed
  // to access the M_PSTATE object, but it hasn't be initialized yet.

  #ifdef DEBUG_PSEQ
  //DEBUG_OUT("\tBefore initializeMappings\n");
  #endif
  //NOTE: this call is NOT necessary, as the m_ooo.regs were initialized, and this m_inorder.regs
  //          uses the SAME mappings...
  ASSERT(rbox.getInit() == false);
  rbox.initializeMappings( );

  #ifdef DEBUG_PSEQ
  //DEBUG_OUT("\tBefore addRegisterHandler\n");
  #endif
  rbox.addRegisterHandler( RID_NONE, 
                           new arf_none_t( NULL, NULL, NULL ) );
  
  // NOTE: creating flat register file with # of logical registers

    physical_file_t * int_file_rf = new physical_file_t( CONFIG_IREG_LOGICAL,
                                                                        this );
   #ifdef DEBUG_PSEQ
    //DEBUG_OUT("\tMaking a INT register file with %d logical regs\n",CONFIG_IREG_LOGICAL);
   #endif

    flat_int_t * fit = new flat_int_t( int_file_rf, M_PSTATE, proc );

  #ifdef DEBUG_PSEQ
    //DEBUG_OUT("\tpseq_t: adding RID_INT register handler, iteration %d\n", k);
  #endif
   
    rbox.addRegisterHandler( RID_INT, fit );

  #ifdef DEBUG_PSEQ
    //DEBUG_OUT("\tpseq_t: adding RID_INT_GLOBAL register handler, iteration %d\n",k);
  #endif

    rbox.addRegisterHandler( RID_INT_GLOBAL,
                                        new flat_int_global_t( int_file_rf, M_PSTATE, proc ) );
  
    physical_file_t * fp_file_rf = new physical_file_t( CONFIG_FPREG_LOGICAL,
                                                       this );
    flat_single_t * flat_single_arf = new flat_single_t( fp_file_rf, M_PSTATE, proc );      
    rbox.addRegisterHandler( RID_SINGLE, flat_single_arf );
  
    flat_double_t * flat_double_arf = new flat_double_t( fp_file_rf, M_PSTATE,
                                                             flat_single_arf->getLastWriter() );
    rbox.addRegisterHandler( RID_DOUBLE, flat_double_arf );
  
  // make a flat control register
    m_ideal_control_rf[proc] = new physical_file_t( MAX_CTL_REGS, this );
    flat_control_t * flat_control_arf = new flat_control_t( m_ideal_control_rf[proc],
                                                            M_PSTATE, proc );
 #ifdef DEBUG_PSEQ
    //DEBUG_OUT("\tpseq_t: adding RID_CONTROL (flat) register handler, iteration %d\n", k);
  #endif

    rbox.addRegisterHandler( RID_CONTROL, flat_control_arf );    

 #ifdef DEBUG_PSEQ
    //DEBUG_OUT("\tpseq_t: adding RID_CC (flat) register handler, iteration %d\n", k);
  #endif

    rbox.addRegisterHandler( RID_CC, flat_control_arf );
  
  //
  // define some "container" register types
  //
  flat_container_t *flat_container = new flat_container_t();

  flat_container->openRegisterType( CONTAINER_BLOCK, 8 );
  // 64-byte block renames 8 double registers,
  //    each with an offset of 2 from the original register specifier
  for ( int32 i = 0; i < 8; i++ ) {
    flat_container->addRegister( RID_DOUBLE, 2*i, flat_double_arf );
  }
  flat_container->closeRegisterType();
  
  // IMPORTANT - changed flat_container to have 1 set of YCC registers for EACH logical
  //          processor in the SMT chip...hence we need a loop for this
  //          TODO - where is RID_CONTAINER and its RID_CC and RID_CONTROL being called??
  flat_container->openRegisterType( CONTAINER_YCC, 2);  
  //changed to allocate 1 set for all logical procs

  // YCC register type writes both CC registers and control registers
 #ifdef DEBUG_PSEQ
    // DEBUG_OUT("\tpseq_t: adding RID_CC (container) register handler, iteration %d\n", k);
  #endif
    flat_container->addRegister( RID_CC, REG_CC_CCR, flat_control_arf );

  #ifdef DEBUG_PSEQ
    //DEBUG_OUT("\tpseq_t: adding RID_CONTROL (container) register handler, iteration %d\n", k);
  #endif

    flat_container->addRegister( RID_CONTROL, CONTROL_Y,  flat_control_arf );
    // }

  flat_container->closeRegisterType();
  
  flat_container->openRegisterType( CONTAINER_QUAD, 4);

  // quad block renames 4 single-precision registers,
  //    each with an offset of 1 from the original register specifier
  for ( int32 i = 0; i < 4; i++ ) {
   #ifdef DEBUG_PSEQ
    //DEBUG_OUT("\tpseq_t: adding RID_SINGLE register handler, iteration %d\n", i);
  #endif
   
    flat_container->addRegister( RID_SINGLE, i, flat_single_arf );       
  }
  flat_container->closeRegisterType();
  rbox.addRegisterHandler( RID_CONTAINER, flat_container );  

  #ifdef DEBUG_PSEQ
  DEBUG_OUT("pseq_t:allocateFlatRegBox END\n");
  #endif
}

//***************************************************************************
void pseq_t::clearFlatRegDeps( mstate_t &inorder, flow_inst_t *predecessor, unsigned int proc)
{

  #ifdef DEBUG_PSEQ
    DEBUG_OUT("pseq_t:clearFlatRegDeps BEGIN\n");
  #endif

  ASSERT(proc < CONFIG_LOGICAL_PER_PHY_PROC);

  // for each register type, add this register as a predecessor
  reg_id_t        rid;
  abstract_rf_t  *arf;

  // clear all windowed integer registers
  rid.setRtype( RID_INT );
  arf = inorder.regs.getARF( rid.getRtype() );   
  for (uint32 cwp = 0; cwp < NWINDOWS; cwp++ ) {
    rid.setVanillaState( cwp, 0 );
    // compare the IN and LOCAL registers
    for (int reg = 31; reg >= 16; reg --) {
      #ifdef DEBUG_PSEQ
      //  DEBUG_OUT("\tsetting RID_INT dependences proc[%d] cwp[%d] reg[%d]\n",proc,cwp,reg);
     #endif
      rid.setVanilla( reg );
      arf->setDependence( rid, predecessor, proc );    //RID_INT requires the logical proc num
    }
  }

  // clear the global registers
  rid.setRtype( RID_INT_GLOBAL );
  arf = inorder.regs.getARF( rid.getRtype() );    
  for (uint16 gset = REG_GLOBAL_NORM; gset <= REG_GLOBAL_INT; gset++) {
    rid.setVanillaState( 0, gset );
    // no dependence on register %g0
    rid.setVanilla( 0 );
    arf->setDependence( rid, NULL, proc );      
    for (int reg = 1; reg < 8; reg ++) {
     #ifdef DEBUG_PSEQ
      //  DEBUG_OUT("\tsetting RID_INT_GLOBAL dependences proc[%d] gset[%d] reg[%d]\n",proc,gset,reg);
     #endif
      rid.setVanilla( reg );
      arf->setDependence( rid, predecessor, proc );   //RID_INT_GLOBAL requires logical proc num
    }
  }
  
  // clear the dependences on single registers
  rid.setRtype( RID_SINGLE );
  arf = inorder.regs.getARF( rid.getRtype() );          
  // FLOAT_REGS is in terms of doubles, so clear 2 regs at a time
  for (uint32 reg = 0; reg < MAX_FLOAT_REGS; reg++) {
      #ifdef DEBUG_PSEQ
    //  DEBUG_OUT("\tsetting RID_SINGLE (fp) dependences proc[%d] reg[%d]\n",proc,reg);
     #endif
    rid.setVanilla( reg*2 );
    arf->setDependence( rid, predecessor, proc );  
    rid.setVanilla( reg*2 + 1 );
    arf->setDependence( rid, predecessor, proc );   
  }

  // clear the dependences on control registers
  rid.setRtype( RID_CONTROL );
  arf = inorder.regs.getARF( rid.getRtype() );       
  for (uint32 reg = 0; reg < CONTROL_NUM_CONTROL_PHYS; reg++) {
     #ifdef DEBUG_PSEQ
        //   DEBUG_OUT("\tsetting RID_CONTROL dependences proc[%d] reg[%d]\n",proc,reg);
     #endif
    rid.setVanilla( reg );
    arf->setDependence( rid, predecessor, proc );  //RID_CONTROL does NOT require logical proc num
  }
    
  // clear the dependences on condition code registers
  rid.setRtype( RID_CC );
  arf = inorder.regs.getARF( rid.getRtype() );        
  for (int reg = REG_CC_CCR; reg <= REG_CC_FCC3; reg++) {
     #ifdef DEBUG_PSEQ
        // DEBUG_OUT("\tsetting RID_CC dependences proc[%d] reg[%d]\n",proc,reg);
     #endif
    rid.setVanilla( reg );
    arf->setDependence( rid, predecessor, proc );   //RID_CC does NOT requires the logical proc num
  }

  #ifdef DEBUG_PSEQ
    DEBUG_OUT("pseq_t:clearFlatRegDeps END\n");
  #endif
}

/*------------------------------------------------------------------------*/
/* Check Interfaces                                                       */
/*------------------------------------------------------------------------*/

//**************************************************************************
void pseq_t::commitObserver( dynamic_inst_t *dinstr )
{
  system_t::inst->m_sys_stat->observeInstruction( this, dinstr );
}

/** Buffers retirement information for a given instruction
 */
//**************************************************************************
bool pseq_t::uncheckedRetire( dynamic_inst_t *dinstr, unsigned int proc )
{

  #ifdef DEBUG_PSEQ
  DEBUG_OUT("pseq_t:uncheckedRetire BEGIN\n");
  printPC(&m_ooo.at[proc]);
  #endif

  // We need to have separate uncheckedRetire queues bc we need to differentiate
  //           between unchecked instrs for each logical processor
  ASSERT(proc < CONFIG_LOGICAL_PER_PHY_PROC);

  // if this instruction is a store, we must check the results immediately
  dyn_execute_type_t mytype = dinstr->getStaticInst()->getType();
  if (mytype == DYN_STORE || mytype == DYN_ATOMIC) {
    dinstr->markEvent( EVENT_VALUE_PRODUCER );
  }
  m_unchecked_instr[proc][m_unchecked_retires[proc]++] = dinstr;
  
  ASSERT( m_unchecked_retires[proc] <= PSEQ_MAX_UNCHECKED );

  // This indicates whether we need to check bc we have reached our upper limit of
  //           unchecked instrs

  #ifdef DEBUG_PSEQ
  printPC(&m_ooo.at[proc]);
  DEBUG_OUT("pseq_t:uncheckedRetire END\n");
  #endif

  return (m_unchecked_retires[proc] >= PSEQ_MAX_UNCHECKED);
}

/** reads unchecked retires out of the instruction buffer.
 *  Used at check time to update statistics. You must free
 *  these instructions!!
 */
//**************************************************************************
dynamic_inst_t *pseq_t::getNextUnchecked(unsigned int proc )
{
  #ifdef DEBUG_PSEQ
  DEBUG_OUT("pseq_t:getNextUnchecked BEGIN\n");
  #endif

  ASSERT(proc < CONFIG_LOGICAL_PER_PHY_PROC);

  if ( m_unchecked_retires[proc] <= 0 )
    return NULL;

  dynamic_inst_t *d = m_unchecked_instr[proc][m_unchecked_retire_top[proc]];
  m_unchecked_instr[proc][m_unchecked_retire_top[proc]] = NULL;
  m_unchecked_retires[proc]--;
  m_unchecked_retire_top[proc]++;
  if (m_unchecked_retires[proc] == 0)
    m_unchecked_retire_top[proc] = 0;

  #ifdef DEBUG_PSEQ
  DEBUG_OUT("pseq_t:getNextUnchecked END\n");
  #endif

  return (d);
}

//***************************************************************************
memory_inst_t *pseq_t::uncheckedValueForward( pa_t phys_addr, int my_size, unsigned int proc )
{
  #ifdef DEBUG_PSEQ
  DEBUG_OUT("pseq_t:uncheckedValueForward BEGIN\n");
  #endif

  int       index = (int) m_unchecked_retires[proc] - 1;
  my_addr_t my_cacheline = phys_addr & ~MEMOP_BLOCK_MASK;
  int       my_offset    = phys_addr &  MEMOP_BLOCK_MASK;

  // out_info("searching: 0x%0llx\n", my_cacheline );
  while ( index >= (int) m_unchecked_retire_top[proc] ) {
    dynamic_inst_t *d = m_unchecked_instr[proc][index];
    if ( d->getEvent( EVENT_VALUE_PRODUCER ) ) {
      memory_inst_t *m = (memory_inst_t *) d;
      my_addr_t other_cacheline = m->getPhysicalAddress() & ~MEMOP_BLOCK_MASK;
      int       other_offset    = m->getPhysicalAddress() &  MEMOP_BLOCK_MASK;
      int       other_size      = m->getAccessSize();

      // out_info("   0x%0llx\n", other_cacheline );
      if (my_cacheline == other_cacheline) {
        /*
        out_info("address overlap 0x%0llx ?= 0x%0llx\n",
               my_cacheline, other_cacheline);
        out_info(" my line  0x%0llx 0x%0x\n",
               my_cacheline, my_offset);
        out_info(" other line  0x%0llx 0x%0x\n",
               other_cacheline, other_offset);
        */

        if ( my_offset == other_offset &&
             other_size >= my_size ) {
          // exact match -- can forward for sure
          return m;
        } // CM FIX: can forward in other situations too

        // FIXFIXFIX
        // CM FIX: CAN match in other situations
      } // end if cache line matches
    }
    index--;
  }

  #ifdef DEBUG_PSEQ
  DEBUG_OUT("pseq_t:uncheckedValueForward END\n");
  #endif

  return NULL;
}

//**************************************************************************
void pseq_t::uncheckedPrint(unsigned int proc )
{
  #ifdef DEBUG_PSEQ
  DEBUG_OUT("pseq_t:uncheckedPrint BEGIN\n");
  #endif

  ASSERT(proc < CONFIG_LOGICAL_PER_PHY_PROC);

  int index = (int) m_unchecked_retires[proc] - 1;
  while ( index >= (int) m_unchecked_retire_top[proc] ) {
    dynamic_inst_t *d = m_unchecked_instr[proc][index];
    out_info("unchecked instruction: %d\n", index );
    d->printDetail();
    index--;
  }

  #ifdef DEBUG_PSEQ
  DEBUG_OUT("pseq_t:uncheckedPrint END\n");
  #endif
}

//***************************************************************************
void pseq_t::pushRetiredInstruction( dynamic_inst_t *dinstr, unsigned int proc )
{
  #ifdef DEBUG_PSEQ
  DEBUG_OUT("pseq_t:pushRetiredInstruction BEGIN\n");
  #endif

  ASSERT(proc < CONFIG_LOGICAL_PER_PHY_PROC);

  // check to see if insertion point is null
  if ( m_recent_retire_instr[proc][m_recent_retire_index[proc]] != NULL ) {
    delete m_recent_retire_instr[proc][m_recent_retire_index[proc]];
  }
  m_recent_retire_instr[proc][m_recent_retire_index[proc]] = dinstr;
  m_recent_retire_index[proc] = m_recent_retire_index[proc] + 1;
  if (m_recent_retire_index[proc] >= PSEQ_RECENT_RETIRE_SIZE) {
    m_recent_retire_index[proc] = 0;
  }

  #ifdef DEBUG_PSEQ
  DEBUG_OUT("pseq_t:pushRetiredInstruction END\n");
  #endif
}

//***************************************************************************
void pseq_t::printRetiredInstructions(unsigned int proc )
{
  #ifdef DEBUG_PSEQ
  DEBUG_OUT("pseq_t:printRetiredInstructions BEGIN\n");
  #endif

  ASSERT(proc < CONFIG_LOGICAL_PER_PHY_PROC);

  int32 count = 0;
  int32 index = m_recent_retire_index[proc] - 1;
  if (index < 0) {
    index = PSEQ_RECENT_RETIRE_SIZE - 1;
  }
  while (count < PSEQ_RECENT_RETIRE_SIZE) {
    dynamic_inst_t *d = m_recent_retire_instr[proc][index];
    if (d == NULL) {
      out_info("recently retired instruction: %d is NULL.\n", count );
    } else {
      out_info("recently retired instruction: %d\n", count );
      d->printDetail();
    }
    // decrement index, wrapping it around in the retire window size
    index = index - 1;
    if (index < 0) {
      index = PSEQ_RECENT_RETIRE_SIZE - 1;
    }
    count++;
  }

  #ifdef DEBUG_PSEQ
  DEBUG_OUT("pseq_t:printRetiredInstructions END\n");
  #endif
}

//***************************************************************************
void pseq_t::checkCriticalState( check_result_t *result, mstate_t *mstate, unsigned int proc )
{
  #ifdef DEBUG_PSEQ
  DEBUG_OUT("pseq_t:checkCriticalState BEGIN m_id[%d] proc[%d] \n",m_id,proc);
  printPC(&m_ooo.at[proc]);
  #endif

  ASSERT(proc < CONFIG_LOGICAL_PER_PHY_PROC);

  ireg_t simreg;
  bool   mask = false;
  
  result->critical_check = true;
  simreg = M_PSTATE->getControl( CONTROL_PSTATE, proc );
  if ( ISADDRESS32(simreg) ) {
    mask = true;
  }
  
#ifdef DEBUG_PSEQ
     result->verbose = true;
     DEBUG_OUT("\tcheckCriticalState BEGIN\n");
#endif

  // CM AddressMask64
  simreg = M_PSTATE->getControl( CONTROL_PC, proc );
  if (mask)
    simreg = AM64( simreg );
  if (simreg != mstate->at[proc].pc) {
    //DEBUG_OUT("[ %d ] patch  PC: 0x%0llx 0x%0llx cycle[ %lld ]\n", m_id, mstate->at[proc].pc, simreg, m_local_cycles);
    if (result->verbose)
      DEBUG_OUT("\t[ %d ] patch  PC: 0x%0llx 0x%0llx cycle[ %lld ]\n", m_id, mstate->at[proc].pc, simreg, m_local_cycles);
    mstate->at[proc].pc = simreg;
    result->perfect_check = false;
    result->critical_check = false;
  }
  else{
    if(result->verbose){
      //DEBUG_OUT("\tchecking  PC: 0x%0llx 0x%0llx cycle[ %lld ]\n", mstate->at[proc].pc, simreg, m_local_cycles);
    }
  }
  
  simreg = M_PSTATE->getControl( CONTROL_NPC, proc );
  if (mask)
    simreg = AM64( simreg );
  if (simreg != mstate->at[proc].npc) {
    //DEBUG_OUT("[ %d ] patch  NPC: 0x%0llx 0x%0llx cycle[ %lld ]\n", m_id, mstate->at[proc].npc, simreg, m_local_cycles);
    if (result->verbose)
      DEBUG_OUT("\t[ %d ] patch  NPC: 0x%0llx 0x%0llx cycle[ %lld ]\n", m_id, mstate->at[proc].npc, simreg, m_local_cycles);
    // CM AddressMask64
    mstate->at[proc].npc = simreg;
    result->perfect_check = false;
    result->critical_check = false;
  }
  else{
    if(result->verbose){
      //DEBUG_OUT("\tchecking  NPC: 0x%0llx 0x%0llx cycle[ %lld ]\n", mstate->at[proc].npc, simreg, m_local_cycles);
    }
  }


  // Walk the list of registers to check in the abstract register file.
  for (uint32 i = 0; i < PSEQ_CRITICAL_REG_COUNT; i++) {
    mstate->critical_regs[proc][i].getARF()->check( mstate->critical_regs[proc][i],
                                              M_PSTATE, result, proc );
  #ifdef DEBUG_PSEQ
    if(result->verbose){
      if( result->perfect_check == false ){
        switch(i) {
        case 0:
          DEBUG_OUT("\t\tchecking CONTROL_PSTATE ");
          break;
        case 1:
          DEBUG_OUT("\t\tchecking CWP ");
          break;
        case 2:
          DEBUG_OUT("\t\tchecking TL ");
          break;
      case 3:
        DEBUG_OUT("\t\tchecking REG_CC_CCR ");
        break;
        }
        DEBUG_OUT(" FAILED\n");
      }
    }
    #endif
  }
  
  if ( result->perfect_check == false ) {
    result->critical_check = false;

    // update all the fields in the fetch_at structure
    physical_file_t *control_state_rf = NULL;
    if ( mstate == &(m_ooo) ) {
      // checking the criticalness of the timing ('inorder') registers
      control_state_rf = m_control_arf->getRetireRF(proc);
    } else if ( mstate == &(m_inorder[proc]) ) {
      // check the criticalness of the functional ('ideal') registers
      control_state_rf = m_ideal_control_rf[proc];
    } else {
      ERROR_OUT("error: pseq_t: check \"critical\" state fails: invariant violated\n");
      SIM_HALT;
    }
   
#ifdef DEBUG_PSEQ
    DEBUG_OUT("\t\tBEFORE: PSTATE[ 0x%x ] CWP[ 0x%x ] TL[ %d ] GSET[ 0x%x ] ASI[ 0x%x ]\n", mstate->at[proc].pstate, mstate->at[proc].cwp,
              mstate->at[proc].tl, mstate->at[proc].gset, mstate->at[proc].asi);
#endif
    mstate->at[proc].pstate = control_state_rf->getInt( CONTROL_PSTATE );
    mstate->at[proc].cwp    = control_state_rf->getInt( CONTROL_CWP );
    mstate->at[proc].tl     = control_state_rf->getInt( CONTROL_TL );
    mstate->at[proc].gset   = pstate_t::getGlobalSet( mstate->at[proc].pstate );
    mstate->at[proc].asi    = control_state_rf->getInt( CONTROL_ASI );

#ifdef DEBUG_PSEQ
    DEBUG_OUT("\t\tAFTER: PSTATE[ 0x%x ] CWP[ 0x%x ] TL[ %d ] GSET[ 0x%x ] ASI[ 0x%x ]\n", mstate->at[proc].pstate, mstate->at[proc].cwp,
              mstate->at[proc].tl, mstate->at[proc].gset, mstate->at[proc].asi);
#endif

  }

#ifdef DEBUG_PSEQ
     result->verbose = false;
     printPC(&m_ooo.at[proc]);
     DEBUG_OUT("pseq_t:checkCriticalState END m_id[%d] proc[%d]\n",m_id,proc);
#endif
}

//***************************************************************************
void pseq_t::checkAllState( check_result_t *result, mstate_t *mstate, unsigned int proc )
{
  #ifdef DEBUG_PSEQ
    DEBUG_OUT("pseq_t:checkAllState BEGIN m_id[%d] proc[%d]\n",m_id,proc);
  #endif

  ASSERT(proc < CONFIG_LOGICAL_PER_PHY_PROC);

#ifdef DEBUG_PSEQ
  DEBUG_OUT("  check all state()\n");
#endif
  result->critical_check = true;
  result->perfect_check = true;

  // check the ultimately important (PC, PSTATE) state 
  checkCriticalState( result, mstate, proc );

#ifdef DEBUG_PSEQ
     result->verbose = true;
     DEBUG_OUT("\tcheckAllState BEGIN\n");
#endif
  reg_id_t        rid;
  abstract_rf_t  *arf;
  pstate_t       *pstate = M_PSTATE;

  // compare all windowed integer registers
  rid.setRtype( RID_INT );
  arf = mstate->regs.getARF( rid.getRtype() );      
  for (uint32 cwp = 0; cwp < NWINDOWS; cwp++ ) {
    rid.setVanillaState( cwp, 0 );
    // compare the IN and LOCAL registers
    for (int reg = 31; reg >= 16; reg --) {
      rid.setVanilla( reg );

#ifdef DEBUG_PSEQ
      DEBUG_OUT("\tchecking RID_INT proc[%d] cwp[%d] reg[%d] critical_check[ %d ] perfect_check[ %d ]\n",proc,cwp,reg, result->critical_check, result->perfect_check);
#endif

      arf->check( rid, pstate, result, proc ); 
    }
  }

  // compare the global registers
  rid.setRtype( RID_INT_GLOBAL );
  arf = mstate->regs.getARF( rid.getRtype() );    
  for (uint16 gset = REG_GLOBAL_NORM; gset <= REG_GLOBAL_INT; gset++) {
    rid.setVanillaState( 0, gset );
    // ignore register %g0
    for (int reg = 1; reg < 8; reg ++) {
      rid.setVanilla( reg );

#ifdef DEBUG_PSEQ
      DEBUG_OUT("\tchecking RID_INT_GLOBAL proc[%d] gset[%d] reg[%d] critical_check[ %d ] perfect_check[ %d ]\n",proc,gset,reg, result->critical_check, result->perfect_check);
#endif

      arf->check( rid, pstate, result, proc );  
    }
  }
  
  // compare the fp registers
  rid.setRtype( RID_DOUBLE );
  arf = mstate->regs.getARF( rid.getRtype() );       
  for (uint32 reg = 0; reg < MAX_FLOAT_REGS; reg++) {
    rid.setVanilla( (reg*2) );

#ifdef DEBUG_PSEQ
    DEBUG_OUT("\tchecking RID_DOUBLE proc[%d] reg[%d] critical_check[ %d ] perfect_check[ %d ]\n",proc,reg, result->critical_check, result->perfect_check);
#endif

  arf->check( rid, pstate, result, proc );        
  }
  hfa_checkerr( "FP registers: end\n" );
  
  // compare the control registers
  rid.setRtype( RID_CONTROL );
  //Modified the getARF call below in order to return a logical processor's control RF
  arf = mstate->regs.getARF( rid.getRtype() );         
  hfa_checkerr("control registers: start\n");
  for (uint32 reg = 0; reg < CONTROL_NUM_CONTROL_PHYS; reg++) {
    rid.setVanilla( reg );

#ifdef DEBUG_PSEQ
    DEBUG_OUT("\tchecking RID_CONTROL proc[%d] reg[%d] critical_check[ %d ] perfect_check[ %d ]\n",proc,reg, result->critical_check, result->perfect_check);
#endif

    arf->check( rid, pstate, result, proc );        
  }
  hfa_checkerr("control registers: end\n");  
  
  // patch up condition codes %ccr, %fcc0, %fcc1, %fcc2, %fcc3
  // Modfied the getARF 
  rid.setRtype( RID_CC );
  arf = mstate->regs.getARF( rid.getRtype());         
  for (int reg = REG_CC_CCR; reg <= REG_CC_FCC3; reg++) {
    rid.setVanilla( reg );

#ifdef DEBUG_PSEQ
    DEBUG_OUT("\tchecking RID_CC proc[%d] reg[%d] critical_check[ %d ] perfect_check[ %d ]\n",proc,reg, result->critical_check, result->perfect_check);
#endif

    arf->check( rid, pstate, result, proc );        //modified check for arf_cc_t class
  }
  hfa_checkerr("ccr, fcc# registers: end\n");
  
#ifdef DEBUG_PSEQ
  result->verbose = false;
  DEBUG_OUT("  check all state() ends m_id[%d] proc[%d]\n",m_id,proc);
#endif
}

//***************************************************************************
void pseq_t::checkChangedState( check_result_t *result, mstate_t *mstate,
                                dynamic_inst_t *d_instr, unsigned int proc )
{
  #ifdef DEBUG_PSEQ
    DEBUG_OUT("pseq_t:checkChangedState BEGIN m_id[%d] proc[%d]\n",m_id,proc);
  #endif

  result->perfect_check = true;

  #ifdef DEBUG_PSEQ
       result->verbose = true;
       DEBUG_OUT("\tcheckChangedState BEGIN\n");
   #endif
  if ( d_instr->getTrapType() == Trap_NoTrap ) {
    #ifdef DEBUG_PSEQ
       DEBUG_OUT("\t\tchecking Dest Regs\n");
    #endif
    // check the registers we think should have changed
    for (int i = 0; i < SI_MAX_DEST; i++) {
      reg_id_t &rid = d_instr->getDestReg(i);
      rid.getARF()->check( rid, M_PSTATE, result, proc );        
    }
  } else {
    // checking an instruction that takes a trap:
    //    explicitly check some control registers

    // invariant: checking changed state only: never updating
    //          : this messes up arf->check() routine
    ASSERT( result->update_only != true );

    // make some reg ids that point to control registers
    reg_id_t        rid;
    abstract_rf_t  *arf = mstate->regs.getARF( RID_CONTROL );    
    // check PSTATE
    rid.setRtype( RID_CONTROL );
    rid.setVanilla( CONTROL_PSTATE );
    #ifdef DEBUG_PSEQ
        DEBUG_OUT("\t\tchecking PSTATE\n");
    #endif

    arf->check( rid, M_PSTATE, result, proc );

    // check CWP
    rid.setVanilla( CONTROL_CWP );
     #ifdef DEBUG_PSEQ
        DEBUG_OUT("\t\tchecking CWP\n");
    #endif
    arf->check( rid, M_PSTATE, result, proc );

    // check TL
    rid.setVanilla( CONTROL_TL );
    #ifdef DEBUG_PSEQ
        DEBUG_OUT("\t\tchecking TL\n");
    #endif
    arf->check( rid, M_PSTATE, result, proc );

    // check CC
    arf = mstate->regs.getARF( RID_CC );
    rid.setRtype( RID_CC );
    rid.setVanilla( REG_CC_CCR );
     #ifdef DEBUG_PSEQ
        DEBUG_OUT("\t\tchecking CC_CCR\n");
    #endif
    arf->check( rid, M_PSTATE, result, proc );
  }

  #ifdef DEBUG_PSEQ
     result->verbose = false;
     DEBUG_OUT("\tcheckChangedState END\n");
  #endif

  #ifdef DEBUG_PSEQ
     DEBUG_OUT("pseq_t:checkChangedState END m_id[%d] proc[%d]\n",m_id,proc);
  #endif
}

//***************************************************************************
void pseq_t::updateInstructionState( dynamic_inst_t *dinstr, unsigned int proc )
{
  check_result_t result;

  result.update_only = true;
  // update the registers this instruction writes
  for (int i = 0; i < SI_MAX_DEST; i++) {
    reg_id_t &rid = dinstr->getDestReg(i);
    rid.getARF()->check( rid, M_PSTATE, &result, proc );     
  }
  hfa_checkerr("error: after update dynamic instruction state.\n");
}

//***************************************************************************
void pseq_t::updateInstructionState( flow_inst_t *flow_inst, unsigned int proc )
{
  check_result_t result;

  result.update_only = true;
  // update the registers this instruction writes
  for (int i = 0; i < SI_MAX_DEST; i++) {
    reg_id_t &rid = flow_inst->getDestReg(i);
    rid.getARF()->check( rid, M_PSTATE, &result, proc );     
  }
  hfa_checkerr("error: after update flow instruction state.\n");
}

//***************************************************************************
void pseq_t::allocateRegBox( mstate_t &ooo )
{
  #ifdef DEBUG_PSEQ
     DEBUG_OUT("pseq_t:allocateRegBox BEGIN m_id[%d] \n", m_id);
  #endif

  reg_box_t &rbox = ooo.regs;

  // CM FIX: Most of this memory is leaked, as pointers to it are not
  //         kept outside of the register box (either free them or keep
  //         private pointers to them and free at the destructor).

 
  for(uint k=0; k <  CONFIG_LOGICAL_PER_PHY_PROC; ++k){
    /// initialize the fetch_at pointer
    m_fetch_at[k]   = new abstract_pc_t();

      #ifdef DEBUG_PSEQ
      DEBUG_OUT("\t assigning m_inorder_at, iter[%d]\n",k);
     #endif
       ASSERT(m_inorder_at != NULL);
       ASSERT(ooo.at != NULL);
       m_inorder_at[k] = &(ooo.at[k]);              //for SMT, modified the mstate_t class' at object to construct multiple abstract PCs
  }

  #ifdef DEBUG_PSEQ
    DEBUG_OUT("\tabout to initialize Mappings\n");
  #endif

  //We should never have inited the regs before this...
  ASSERT(rbox.getInit() == false);
  rbox.initializeMappings( );

  arf_none_t * none_arf =  new arf_none_t( NULL, NULL, NULL );

  //DEBUG_OUT("P_%d allocateRegBox(): NONE_ARF = 0x%llx\n", m_id, none_arf);

  rbox.addRegisterHandler( RID_NONE, none_arf);

  //each reg_map_t has its own internal reg map for each logical proc:
  physical_file_t * int_rf = new physical_file_t( CONFIG_IREG_PHYSICAL, this);
  reg_map_t * int_decode_map = new reg_map_t( int_rf, CONFIG_IREG_LOGICAL );
  reg_map_t * int_retire_map = new reg_map_t( int_rf, CONFIG_IREG_LOGICAL );
   
  #ifdef DEBUG_PSEQ
        DEBUG_OUT("\tadding RID_INT register handler \n");
  #endif

  arf_int_t * int_arf =  new arf_int_t( int_rf,
                                         int_decode_map,
                                         int_retire_map, 4);
  //DEBUG_OUT("P_%d allocateRegBox(): INT_ARF = 0x%llx\n", m_id, int_arf);

  rbox.addRegisterHandler( RID_INT, int_arf);

  #ifdef DEBUG_PSEQ
        DEBUG_OUT("\tadding RID_INT_GLOBAL register handler \n");
  #endif

  arf_int_global_t * global_int_arf =   new arf_int_global_t( int_rf,
                                                              int_decode_map,
                                                              int_retire_map);

  //DEBUG_OUT("P_%d allocateRegBox(): GLOBAL_INT_ARF = 0x%llx\n", m_id, global_int_arf);
  
  rbox.addRegisterHandler( RID_INT_GLOBAL, global_int_arf);

  physical_file_t * fp_rf = new physical_file_t( CONFIG_FPREG_PHYSICAL, this );
  reg_map_t * fp_decode_map = new reg_map_t( fp_rf, CONFIG_FPREG_LOGICAL );
  reg_map_t * fp_retire_map = new reg_map_t( fp_rf, CONFIG_FPREG_LOGICAL );
  
  arf_single_t * single_arf = new arf_single_t( fp_rf,
                                                fp_decode_map,
                                                fp_retire_map, 16 );

  #ifdef DEBUG_PSEQ
        DEBUG_OUT("\tadding RID_SINGLE register handler\n");
  #endif

  //DEBUG_OUT("P_%d allocateRegBox(): SINGLE_ARF = 0x%llx\n", m_id, single_arf);
        
  rbox.addRegisterHandler( RID_SINGLE, single_arf );
  
  arf_double_t * double_arf   = new arf_double_t( fp_rf,
                                                  fp_decode_map,
                                                  fp_retire_map );

  #ifdef DEBUG_PSEQ
    DEBUG_OUT("\tadding RID_DOUBLE register handler\n");
  #endif

  //DEBUG_OUT("P_%d allocateRegBox(): DOUBLE_ARF = 0x%llx\n", m_id, double_arf);

  rbox.addRegisterHandler( RID_DOUBLE, double_arf );

  m_cc_rf = new physical_file_t( CONFIG_CCREG_PHYSICAL, this );
  reg_map_t * cc_decode_map = new reg_map_t( m_cc_rf, CONFIG_CCREG_LOGICAL );
  m_cc_retire_map = new reg_map_t( m_cc_rf, CONFIG_CCREG_LOGICAL );
  
  arf_cc_t *cc_arf = new arf_cc_t( m_cc_rf, cc_decode_map,
                                   m_cc_retire_map, 2 );
  
  #ifdef DEBUG_PSEQ
    DEBUG_OUT("\tadding RID_CC register handler \n");
  #endif

  //DEBUG_OUT("P_%d allocateRegBox(): CC_ARF = 0x%llx\n", m_id, cc_arf);

  rbox.addRegisterHandler( RID_CC, cc_arf );
    
  // each logical proc should have its own control set registers:
  for(uint k=0; k < CONFIG_LOGICAL_PER_PHY_PROC; ++k){
    m_control_rf[k] = (physical_file_t **) malloc( sizeof(physical_file_t *) * 
                                                   CONFIG_NUM_CONTROL_SETS );
    
    for ( uint32 i = 0; i < CONFIG_NUM_CONTROL_SETS; i++ ) {
      m_control_rf[k][i] = new physical_file_t( CONTROL_NUM_CONTROL_TYPES, this );
    }
  }  // end for all logical procs

  m_control_arf = new arf_control_t( m_control_rf, CONFIG_NUM_CONTROL_SETS );

  #ifdef DEBUG_PSEQ
    DEBUG_OUT("\tadding RID_CONTROL register handler \n");
  #endif

  //DEBUG_OUT("P_%d allocateRegBox(): CONTROL_ARF = 0x%llx\n", m_id, m_control_arf);

  rbox.addRegisterHandler( RID_CONTROL, m_control_arf );

  //
  // define some "container" register types
  //
  arf_container_t *container_arf = new arf_container_t();

  container_arf->openRegisterType( CONTAINER_BLOCK, 8 );
  // 64-byte block renames 8 double registers,
  //    each with an offset of 2 from the original register specifier
  for ( int32 i = 0; i < 8; i++ ) {
    container_arf->addRegister( RID_DOUBLE, 2*i, double_arf );       
  }
  container_arf->closeRegisterType();
  
  container_arf->openRegisterType( CONTAINER_YCC, 2 ); 

  // YCC register type writes both CC registers and control registers
   #ifdef DEBUG_PSEQ
    DEBUG_OUT("\tadding RID_CC register handler \n");
  #endif

    container_arf->addRegister( RID_CC, REG_CC_CCR, cc_arf );

  #ifdef DEBUG_PSEQ
    DEBUG_OUT("\tadding RID_CONTROL register handler \n");
  #endif

  container_arf->addRegister( RID_CONTROL, CONTROL_Y,  m_control_arf );

  container_arf->closeRegisterType();
  
  container_arf->openRegisterType( CONTAINER_QUAD, 4 );
  // quad block renames 4 single-precision registers,
  //    each with an offset of 1 from the original register specifier
  for ( int32 i = 0; i < 4; i++ ) {
    container_arf->addRegister( RID_SINGLE, i, single_arf );       
  }
  container_arf->closeRegisterType();

  //DEBUG_OUT("P_%d allocateRegBox(): CONTAINER_ARF = 0x%llx\n", m_id, container_arf);

  rbox.addRegisterHandler( RID_CONTAINER, container_arf );  

  // allocate and initialize the register identifiers for "critical" checks
  // The order of these fields MATTERS: must be PSTATE, CWP, TL, CCR
  //          (used in checkCriticalState())
  // NOTE: these function copy from ooo.regs to ooo.critical_regs
  for(uint k=0; k <  CONFIG_LOGICAL_PER_PHY_PROC; ++k){
      #ifdef DEBUG_PSEQ
    DEBUG_OUT("\tsetting critical registers iter[%d]\n", k);
     #endif
    uint32  critical_count = 0;
    ooo.critical_regs[k][critical_count].setRtype( RID_CONTROL );
    ooo.critical_regs[k][critical_count].setVanilla( CONTROL_PSTATE );
    ooo.critical_regs[k][critical_count].copy( ooo.critical_regs[k][critical_count],
                                               ooo.regs );
    critical_count++;
    ooo.critical_regs[k][critical_count].setRtype( RID_CONTROL );
    ooo.critical_regs[k][critical_count].setVanilla( CONTROL_CWP );
    ooo.critical_regs[k][critical_count].copy( ooo.critical_regs[k][critical_count],
                                               ooo.regs );
    critical_count++;
    ooo.critical_regs[k][critical_count].setRtype( RID_CONTROL );
    ooo.critical_regs[k][critical_count].setVanilla( CONTROL_TL );
    ooo.critical_regs[k][critical_count].copy( ooo.critical_regs[k][critical_count],
                                               ooo.regs );
    critical_count++;
    ooo.critical_regs[k][critical_count].setRtype( RID_CC );
    ooo.critical_regs[k][critical_count].setVanilla( REG_CC_CCR );
    ooo.critical_regs[k][critical_count].copy( ooo.critical_regs[k][critical_count],
                                               ooo.regs );
    critical_count++;
    ASSERT( critical_count == PSEQ_CRITICAL_REG_COUNT );
  }

  #ifdef DEBUG_PSEQ
     DEBUG_OUT("pseq_t:allocateRegBox END m_id[%d]\n", m_id);
  #endif
}

//***************************************************************************
void pseq_t::allocateIdealState( void )
{

  #ifdef DEBUG_PSEQ
    DEBUG_OUT("pseq_t:allocateIdealState BEGIN m_id[%d]\n",m_id);
  #endif

  /*
   * ideal register file initialization
   */
  
  //allocate memory first:
 m_ideal_retire_count = new int64[CONFIG_LOGICAL_PER_PHY_PROC];
 m_ideal_last_checked = new int64[CONFIG_LOGICAL_PER_PHY_PROC];
 m_ideal_last_freed = new int64[CONFIG_LOGICAL_PER_PHY_PROC];
 m_ideal_first_predictable = new int64[CONFIG_LOGICAL_PER_PHY_PROC];
 m_ideal_last_predictable = new int64[CONFIG_LOGICAL_PER_PHY_PROC];

 for(uint k=0; k <  CONFIG_LOGICAL_PER_PHY_PROC; ++k){  
   // initialize the indecies
   m_ideal_retire_count[k]     = 0;
   m_ideal_last_checked[k]     = -1;
   m_ideal_last_freed[k]       = -1;
   m_ideal_last_predictable[k] = -1;
 }

  // build the jump table for the flow instructions
  flow_inst_t::ix_build_jump_table();

   m_inorder = new mstate_t[CONFIG_LOGICAL_PER_PHY_PROC];
   m_ideal_control_rf = new physical_file_t *[CONFIG_LOGICAL_PER_PHY_PROC];
  /// build the ideal register file
   for(uint k=0; k <  CONFIG_LOGICAL_PER_PHY_PROC; ++k){
    allocateFlatRegBox( m_inorder[k], k );    
    }

  //
  // Initialize the critical register file checking:
  //    copy( source, destination )
  //    e.g. inorder.critical_regs <- ooo.critical_regs

  #ifdef DEBUG_PSEQ
  DEBUG_OUT("\tBeginning to copy registers...\n");
  #endif

  for(uint k=0; k <  CONFIG_LOGICAL_PER_PHY_PROC; ++k){  
    uint32  count = 0;
    m_inorder[k].critical_regs[k][count].copy( m_ooo.critical_regs[k][count],
                                               m_inorder[k].regs );
    count++;
    m_inorder[k].critical_regs[k][count].copy( m_ooo.critical_regs[k][count],
                                               m_inorder[k].regs);
    count++;
    m_inorder[k].critical_regs[k][count].copy( m_ooo.critical_regs[k][count],
                                               m_inorder[k].regs );
    count++;
    m_inorder[k].critical_regs[k][count].copy( m_ooo.critical_regs[k][count],
                                               m_inorder[k].regs);
    count++;
    ASSERT( count == PSEQ_CRITICAL_REG_COUNT );
  }

  #ifdef DEBUG_PSEQ
  DEBUG_OUT("\tEnd of copying registers...\n");
  #endif
  //
  // Control flow graph
  //
  // allocate memory dependence tracking structure

  //For now, allocate a separate dependence structure for each logical processor:

  m_mem_deps = new mem_dependence_t *[CONFIG_LOGICAL_PER_PHY_PROC];
  m_cfg_pred = new flow_inst_t *[CONFIG_LOGICAL_PER_PHY_PROC];

  for(uint k=0; k <  CONFIG_LOGICAL_PER_PHY_PROC; ++k){
    m_mem_deps[k] = new mem_dependence_t();
    
    // allocate a 'predecessor node' for the control flow graph
    
    //assume this instr runs on logical proc k
    m_cfg_pred[k] = new flow_inst_t( m_fetch_itlbmiss[k], this, NULL, m_mem_deps[k], k );
    
    // clear all dependence information (set to m_cfg_pred)
    clearFlatRegDeps( m_inorder[k], m_cfg_pred[k],  k);     
  }

  #ifdef DEBUG_PSEQ
    DEBUG_OUT("pseq_t:allocateIdealState END m_id[%d]\n",m_id);
  #endif
}

/*------------------------------------------------------------------------*/
/* Debugging / Testing                                                    */
/*------------------------------------------------------------------------*/

//**************************************************************************
void pseq_t::printPC( abstract_pc_t *a )
{
  char buffer[128]; // big enough to hold 32 bits + white space

  DEBUG_OUT( "PC :    0x%0llx\n", a->pc );
  DEBUG_OUT( "NPC:    0x%0llx\n", a->npc );
  DEBUG_OUT( "TL :      %d\n", a->tl );
  sprintBits( a->pstate, buffer );
  DEBUG_OUT( "PSTATE:   %s\n", buffer );
  DEBUG_OUT( "CWP:      %d\n", a->cwp );
  DEBUG_OUT( "GSET:     %d\n", a->gset );
}

//**************************************************************************
void pseq_t::print( void )
{

  #ifdef DEBUG_PSEQ
  DEBUG_OUT("pseq_t:print BEGIN\n");
  #endif

  //reset debug cycle
  debugio_t::setDebugTime( 0 );

  //out_info("fetch PC:  0x%0llx\n", m_fetch_at->pc);
  //out_info("fetch NPC: 0x%0llx\n", m_fetch_at->npc);
  //#ifdef DEBUG_PSEQ
  out_info("[ %d ] PSEQ PIPELINE PRINT cycle[ %lld ]\n", m_id, m_local_cycles);
  out_info("Primary CTX: %d\n", m_primary_ctx);

  // print fetch/decode per cycles
  //out_info("I$ pipeline");
  //m_i_cache_line_queue.print();

 for(uint k=0; k <  CONFIG_LOGICAL_PER_PHY_PROC; ++k){  
  out_info("fetch per cycle proc[%d] thread[%d]",m_id, k);
  m_fetch_per_cycle[k].print();
 
  out_info("decode per cycle proc[%d] thread[%d]",m_id,k);
  m_decode_per_cycle[k].print();
 }
 //#endif

  // print the instruction window and associated pipelines
  // DEBUG_RETIRE

  for(uint k=0; k <  CONFIG_LOGICAL_PER_PHY_PROC; ++k){
    m_ooo.regs.printStats(this, k);  
    out_info("[ %d ] ***Instruction Window for thread %d\n", m_id, k);
    m_iwin[k].print();
    out_info("Fetch status: %s\n", fetch_menomic( m_fetch_status[k] ));
    m_iwin[k].printDetail();
    out_info("Slot avail? : %d\n", isSlotAvailable(k));
  }

  // print out the outstanding transactions
  if (m_ruby_cache)
    m_ruby_cache->print();

  // print out 'unretired' instructions
  for(uint k=0; k <  CONFIG_LOGICAL_PER_PHY_PROC; ++k){  
    uncheckedPrint(k);    
    
    // print out recently retired instructions (like a crash log)
    printRetiredInstructions(k);  
    
    // print out load/store queue
    m_lsq[k]->printLSQ();
    
  }    // end for all logical procs
  // print out the register mapping
  // m_int_decode_map->print();

  #ifdef DEBUG_PSEQ
  DEBUG_OUT("pseq_t:print END\n");
  #endif
}

//**************************************************************************
void pseq_t::printFetchDebug()
{
  for(uint k=0; k <  CONFIG_LOGICAL_PER_PHY_PROC; ++k){
    m_ooo.regs.printStats(this, k);  
    out_info("[ %d ] ***Fetch Status for thread %d\n", m_id, k);
    out_info("VPC: 0x%llx\n", m_fetch_at[k]->pc);
    out_info("VNPC: 0x%llx\n", m_fetch_at[k]->npc);
    out_info("Fetch status: %s\n", fetch_menomic( m_fetch_status[k] ));
    out_info("Slot avail? : %d\n", isSlotAvailable(k));
    out_info("Slots taken (this thread): %d\n", getNumSlotsTaken(k));
    out_info("Total slots taken: %d\n", getTotalSlotsTaken());
    out_info("Cycle: %lld\n\n", getLocalCycle());
  }
}
 
//***************************************************************************
void pseq_t::printInflight( void )
{
 for(uint k=0; k <  CONFIG_LOGICAL_PER_PHY_PROC; ++k){  
   out_info("*** Inflight instructions\n");
   out_info("Fetch status: %s\n", fetch_menomic( m_fetch_status[k] ));
   out_info("\n");
   m_iwin[k].printDetail();

 }
   if (m_ruby_cache)
    m_ruby_cache->print();

  // print out recently retired instructions (like a crash log)
 for(uint k=0; k <  CONFIG_LOGICAL_PER_PHY_PROC; ++k){  
   printRetiredInstructions(k);
 }
}

//***************************************************************************
void pseq_t::printDebug(){
  // end debug filtering if there is an error
  ERROR_OUT("[ %d ]*****PROCESSOR DEBUG INFO*** cycle[ %lld ]\n", m_id, m_local_cycles);
  setDebugTime( 0 );
  print();
  m_write_buffer->print();
  ERROR_OUT("\n");
  m_ruby_cache->print();
  ERROR_OUT("\n");
}

//***************************************************************************
void pseq_t::printStats( void )
{

  #ifdef DEBUG_PSEQ
    DEBUG_OUT("pseq_t:printStats BEGIN\n");
  #endif

  // temporarily stop debug filtering-- you asked for the stats right?
  uint64 filterTime = getDebugTime();
  setDebugTime( 0 );

   char   user_mode = '?';
   char   buf[80], buf1[80], buf2[80], buf3[80], buf4[80], buf5[80], buf6[80], buf7[80], buf8[80], buf9[80];
   double total_pct = 0.0;
   uint64 total_ideal_coverage = 0;

 for(uint k=0; k <  CONFIG_LOGICAL_PER_PHY_PROC; ++k){  
   if (m_stat_committed[k] > 0) {  
     out_info("*** Opcode stats [logical proc %d]:\n",k);
     m_opstat[k].print( this );
     out_info("\n");
   }

   if (m_ideal_retire_count[k] > 0) {
     out_info("*** Ideal opcode stats [logical proc %d]:\n",k);
    m_ideal_opstat[k].print( this );
    out_info("\n");
    out_info("Ideal predictor rating:\n");
    for (uint32 i = 0; i < PSEQ_FETCH_MAX_STATUS; i++) {
      total_ideal_coverage += m_hist_ideal_coverage[i];
    }
    for (uint32 i = 0; i < PSEQ_FETCH_MAX_STATUS; i++) {
      if (total_ideal_coverage == 0)
        total_pct = 0.0;
      else
        total_pct = ((double) m_hist_ideal_coverage[i] / (double) total_ideal_coverage)
          *100.0;
      out_info("%-20.20s: %24.24s %6.2lf%%\n",
               fetch_menomic( (pseq_fetch_status_t) i ),
               commafmt(m_hist_ideal_coverage[i], buf, 80),
               total_pct);
    }
    out_info("\n");
  }
   out_info("\n");

  out_info("*** Trap   stats [logical proc: %d]:\n",k);
  out_info("  [Trap#]  Times-Taken Times-Complete Simics-Taken    Name\n");
  for (int i = 0; i < TRAP_NUM_TRAP_TYPES; i ++) {
    if ( m_trapstat[k][i] != 0 || m_simtrapstat[k][i] != 0) {
      out_info("  [%3d] %14lld  %14lld %14lld  %s\n", i, m_trapstat[k][i], m_completed_trapstat[k][i], m_simtrapstat[k][i],
               pstate_t::trap_num_menomic( (trap_type_t) i ));
    }
  }

  #if 0
  //now output software initiated traps (using tcc instr)
  for(int i=0; i < MAX_SOFTWARE_TRAPS; ++i){
    if(m_software_trapstat[k][i] != 0){
      out_info("  [%3d] %14lld %14lld  Software_Trap_%d\n", i+256, m_software_trapstat[k][i], 0, i+256);
    }
  }
  #endif
  
 }   //end for loop over all logical procs  

 out_info("\n");

 for(uint k=0; k <  CONFIG_LOGICAL_PER_PHY_PROC; ++k){  
   out_info("*** Internal exception stats [logical proc %d]:\n",k);
   out_info("###: seen    name\n");
   for (int i = 0; i < EXCEPT_NUM_EXCEPT_TYPES; i ++) {
     if ( m_exception_stat[k][i] != 0 ) {
       out_info("  [%3d] %14lld  %s\n", i, m_exception_stat[k][i],
                pstate_t::async_exception_menomic( (exception_t) i ));
     }
   }
   out_info("\n");
 }  //end for loop over all logical procs
    
  out_info("*** ASI    stats:\n");
  out_info("  ASI     Reads   Writes  Atomics\n");
  for (uint32 i = 0; i < MAX_NUM_ASI; i ++) {
    if ( m_asi_rd_stat[i] != 0 ||
         m_asi_wr_stat[i] != 0 ||
         m_asi_at_stat[i] != 0 ) {
      out_info("  0x%02x %8lld %8lld %8lld\n", 
               i, m_asi_rd_stat[i], m_asi_wr_stat[i], m_asi_at_stat[i] );
    }
  }

  out_info("\n");

  uint64 total_pred = 0;
  uint64 total_seen = 0;
  uint64 total_right = 0;
  uint64 total_wrong = 0;
  double bpaccuracy;
  out_info("*** Branch   stats: (user, kernel, total)\n");
  out_info("  Type           Preds      Retired        Right       Wrong       %%Right\n");
  for (uint32 i = 0; i < BRANCH_NUM_BRANCH_TYPES; i++) {
    for (uint32 j = 0; j < TOTAL_INSTR_MODE; j++) {

      // get the mode string
      switch (j) {
      case 0:
        user_mode = 'U';
        break;
      case 1:
        user_mode = 'K';
        break;
      case 2:
        user_mode = 'T';
        break;
      default:
        ERROR_OUT("pseq_t::PrintStats(): unknown branch type=%d\n", j);
        SIM_HALT;
      }
      
      if ( m_branch_seen_stat[i][j] != 0) {
        bpaccuracy = ((double) m_branch_right_stat[i][j] / (double) m_branch_seen_stat[i][j]) * 100;
      } else {
        bpaccuracy = 0.0;
      }

      out_info("   %-6.6s %12.12s %12.12s %12.12s %12.12s         %c:%6.2f%%\n",
               pstate_t::branch_type_menomic[i] + 7,
               commafmt( m_branch_pred_stat[i][j], buf, 80 ),
               commafmt( m_branch_seen_stat[i][j], buf1, 80 ),
               commafmt( m_branch_right_stat[i][j], buf2, 80 ),
               commafmt( m_branch_wrong_stat[i][j], buf3, 80 ),
               user_mode, bpaccuracy);
      if (j==2) { // total
        total_pred  += m_branch_pred_stat[i][j];
        total_seen  += m_branch_seen_stat[i][j];
        total_right += m_branch_right_stat[i][j];
        total_wrong += m_branch_wrong_stat[i][j];
      }
    } // j
  } // i
  if (total_seen != 0)
    bpaccuracy = ((double) total_right / (double) total_seen) * 100;
  else
    bpaccuracy = 0.0;
  out_info("   %-6.6s %12.12s %12.12s %12.12s %12.12s %6.2f%%\n",
           "TOTALB",
           commafmt( total_pred, buf, 80 ),
           commafmt( total_seen, buf1, 80 ),
           commafmt( total_right, buf2, 80 ),
           commafmt( total_wrong, buf3, 80 ),
           bpaccuracy);

  out_info("*** Histogram   stats:\n");
  uint64 total_fetch       = 0;
  uint64 total_fetch_thread =0;
  uint64 total_smt_fetch   = 0;
  uint64 total_decode      = 0;
  uint64 total_decode_thread = 0;
  uint64 total_schedule    = 0;
  uint64 total_schedule_thread = 0;
  uint64 total_retire      = 0;
  uint64 total_retire_thread = 0;
  uint64 total_fetch_stall = 0;
  uint64 total_retire_stall = 0;
  for (uint32 i = 0; i < MAX_FETCH + 1; i++) {
    total_fetch  += m_hist_fetch_per_cycle[i];
  }
  for (uint32 i = 0; i < MAX_DECODE + 1; i++) {
    total_decode += m_hist_decode_per_cycle[i];
  }
  for (uint32 i = 0; i < MAX_DISPATCH + 1; i++) {
    total_schedule += m_hist_schedule_per_cycle[i];
  }
  for (uint32 i = 0; i < MAX_RETIRE + 1; i++) {
    total_retire += m_hist_retire_per_cycle[i];
  }
  for (uint32 i = 0; i < m_threads_per_cycle + 1; i++) {
    total_smt_fetch += m_hist_smt_fetch_per_cycle[i];
  }
  // For SMT thread fetch stats:
  out_info("\n");
  for (uint32 i = 0; i < m_threads_per_cycle + 1; i++) {
    if (total_smt_fetch == 0)
      total_pct = 0.0;
    else
      total_pct = ((double) m_hist_smt_fetch_per_cycle[i] / (double) total_smt_fetch)
        *100.0;
    out_info("SMT Fetch Threads [%d]: %24.24s %6.2lf%%\n",
             i, commafmt(m_hist_smt_fetch_per_cycle[i], buf, 80), total_pct);
  }
  out_info("\n");
  for (uint32 i = 0; i < MAX_FETCH + 1; i++) {
    if (total_fetch == 0)
      total_pct = 0.0;
    else
      total_pct = ((double) m_hist_fetch_per_cycle[i] / (double) total_fetch)
        *100.0;
    out_info("Core_Fetch  [%d]: %24.24s %6.2lf%%\n",
             i, commafmt(m_hist_fetch_per_cycle[i], buf, 80), total_pct);
  }
  out_info("\n");
  for (uint32 i = 0; i < MAX_DECODE + 1; i++) {
    if (total_decode == 0)
      total_pct = 0.0;
    else
      total_pct = ((double) m_hist_decode_per_cycle[i] / (double) total_decode)
        *100.0;
    out_info("Core_Decode [%d]: %24.24s %6.2lf%%\n",
             i, commafmt(m_hist_decode_per_cycle[i], buf, 80), total_pct);
  }
  out_info("\n");
  for (uint32 i = 0; i < MAX_DISPATCH + 1; i++) {
    if (total_schedule == 0)
      total_pct = 0.0;
    else
      total_pct = ((double) m_hist_schedule_per_cycle[i] / (double) total_schedule) *100.0;
    out_info("Core_Schedule [%d]: %24.24s %6.2lf%%\n",
             i, commafmt(m_hist_schedule_per_cycle[i], buf, 80), total_pct);
  }
  out_info("\n");
  for (uint32 i = 0; i < MAX_RETIRE + 1; i++) {
    if (total_retire == 0)
      total_pct = 0.0;
    else
      total_pct = ((double) m_hist_retire_per_cycle[i] / (double) total_retire)
        *100.0;
    out_info("Core_Retire [%d]: %24.24s %6.2lf%%\n",
             i, commafmt(m_hist_retire_per_cycle[i], buf, 80), total_pct);
  }
  
  //output per-thread histogram...
  for(uint32 proc=0; proc < CONFIG_LOGICAL_PER_PHY_PROC; ++proc){
    total_fetch_thread = total_decode_thread = total_schedule_thread = total_retire_thread = 0;

    for (uint32 i = 0; i < MAX_FETCH + 1; i++) {
      total_fetch_thread  += m_hist_fetch_per_thread[proc][i];
    }
    for (uint32 i = 0; i < MAX_DECODE + 1; i++) {
      total_decode_thread += m_hist_decode_per_thread[proc][i];
    }
    for (uint32 i = 0; i < MAX_DISPATCH + 1; i++) {
      total_schedule_thread += m_hist_schedule_per_thread[proc][i];
    }
    for (uint32 i = 0; i < MAX_RETIRE + 1; i++) {
      total_retire_thread += m_hist_retire_per_thread[proc][i];
    }

    out_info("\n");
    for (uint32 i = 0; i < MAX_FETCH + 1; i++) {
      if (total_fetch_thread == 0)
        total_pct = 0.0;
      else
        total_pct = ((double) m_hist_fetch_per_thread[proc][i] / (double) total_fetch_thread)
          *100.0;
      out_info("Thread_%d_Fetch  [%d]: %24.24s %6.2lf%%\n", proc,
               i, commafmt(m_hist_fetch_per_thread[proc][i], buf, 80), total_pct);
    }
    out_info("\n");
    for (uint32 i = 0; i < MAX_DECODE + 1; i++) {
      if (total_decode_thread == 0)
        total_pct = 0.0;
      else
        total_pct = ((double) m_hist_decode_per_thread[proc][i] / (double) total_decode_thread)
          *100.0;
      out_info("Thread_%d_Decode [%d]: %24.24s %6.2lf%%\n", proc,
               i, commafmt(m_hist_decode_per_thread[proc][i], buf, 80), total_pct);
    }
    out_info("\n");
    for (uint32 i = 0; i < MAX_DISPATCH + 1; i++) {
      if (total_schedule_thread == 0)
        total_pct = 0.0;
      else
        total_pct = ((double) m_hist_schedule_per_thread[proc][i] / (double) total_schedule_thread) *100.0;
      out_info("Thread_%d_Schedule [%d]: %24.24s %6.2lf%%\n", proc,
               i, commafmt(m_hist_schedule_per_thread[proc][i], buf, 80), total_pct);
    }
    out_info("\n");
    for (uint32 i = 0; i < MAX_RETIRE + 1; i++) {
      if (total_retire_thread == 0)
        total_pct = 0.0;
      else
        total_pct = ((double) m_hist_retire_per_thread[proc][i] / (double) total_retire_thread)
          *100.0;
      out_info("Thread_%d_Retire [%d]: %24.24s %6.2lf%%\n", proc,
               i, commafmt(m_hist_retire_per_thread[proc][i], buf, 80), total_pct);
    }
  } //end for all threads

  // FIXFIXFIX
#if 0
  for (uint32 i = 0; i < PSEQ_HIST_DECODE; i++) {
    out_info( "Return [%d]: %24.24s\n",
              i, commafmt(m_hist_decode_return[i], buf, 80) );
  }
  out_info("\n");
#endif

#if 0
  out_info("\n***Squash Histogram:\n");
  for (uint32 i = 0; i < dynamic_inst_t::MAX_INST_STAGE; i++) {
    for (uint32 j = 0; j < IWINDOW_WIN_SIZE; j++) {
      if (m_hist_squash_stage[i][j] != 0) {
        out_info( "%s [%d]: %24.24s\n",
                  dynamic_inst_t::printStage( (dynamic_inst_t::stage_t) i ),
                  j, commafmt(m_hist_squash_stage[i][j], buf, 80) );
      }
    }
    out_info("\n");
  }
#endif

  out_info("\n");
  out_info("Reasons for fetch stalls:\n");
  total_fetch_stall = 0;
  for (uint32 i = 0; i < PSEQ_FETCH_MAX_STATUS; i++) {
    total_fetch_stall += m_hist_fetch_stalls[i];
  }
  for (uint32 i = 0; i < PSEQ_FETCH_MAX_STATUS; i++) {
    if (total_fetch_stall == 0)
      total_pct = 0.0;
    else
      total_pct = ((double) m_hist_fetch_stalls[i] / (double) total_fetch_stall)
        *100.0;
    out_info("%-20.20s: %24.24s %6.2lf%%\n",
             fetch_menomic( (pseq_fetch_status_t) i ),
             commafmt(m_hist_fetch_stalls[i], buf, 80),
             total_pct);
  }

  out_info("\n");

  out_info("Other fetch statistics:\n");
  total_pct = ((double) m_stat_no_fetch_across_lines / (double) getLocalCycle()) * 100.0;
  out_info("%-34.34s: %20.20s %6.2lf%%\n",
           "Fetch stalled at line boundary",
           commafmt(m_stat_no_fetch_across_lines, buf, 80),
           total_pct);
  total_pct = ((double) m_stat_no_fetch_taken_branch / (double) getLocalCycle()) * 100.0;
  out_info("%-34.34s: %20.20s %6.2lf%%\n",
           "Fetch stalled at taken branch",
           commafmt(m_stat_no_fetch_taken_branch, buf, 80),
           total_pct);
  out_info("\n");

  total_retire_stall = 0;
  out_info("Reasons for retire limits:\n");
  for (uint32 i = 0; i < PSEQ_RETIRE_MAX_STATUS; i++) {
    total_retire_stall += m_hist_retire_stalls[i];
  }
  for (uint32 i = 0; i < PSEQ_RETIRE_MAX_STATUS; i++) {
    if (total_retire_stall == 0)
      total_pct = 0.0;
    else
      total_pct = ((double) m_hist_retire_stalls[i] / (double) total_retire_stall)
        *100.0;
    out_info("%-20.20s: %24.24s %6.2lf%%\n",
             retire_menomic( (pseq_retire_status_t) i ),
             commafmt(m_hist_retire_stalls[i], buf, 80),
             total_pct);
  }
  out_info("\n");

  out_info("\n***Retire Not-Ready Stage Histogram\n");
  uint64 total_stages = 0;
  for(int i=0; i < dynamic_inst_t::MAX_INST_STAGE; ++i){
    total_stages += m_stat_retire_notready_stage[i];
  }

  if(total_stages > 0){
    for(int i=0; i < dynamic_inst_t::MAX_INST_STAGE; ++i){
      if(m_stat_retire_notready_stage[i] != 0){
        out_info("\t%s   = %lld   (%6.3f%)\n", dynamic_inst_t::printStage( (dynamic_inst_t::stage_t) i),
                 m_stat_retire_notready_stage[i], 100.0*m_stat_retire_notready_stage[i]/total_stages);
      }
    }
  }

  out_info("\n");


  out_info("Execution unit statistics:\n");
  out_info("TYPE (id)         : (# units) [Average in use per cycle] [Resource stalls] :\n");
  for (uint32 i = 0; i < FU_NUM_FU_TYPES; i++) {
    if (CONFIG_NUM_ALUS[CONFIG_ALU_MAPPING[i]] != 0) {
      //calculate fraction of FU stalls for each FU
      double fraction_fu_stall = 0.0;
      if( (m_stat_fu_stall[CONFIG_ALU_MAPPING[i]] + m_stat_fu_utilization[CONFIG_ALU_MAPPING[i]] ) > 0){
        fraction_fu_stall =  ((double) m_stat_fu_stall[CONFIG_ALU_MAPPING[i]] / (double) (m_stat_fu_stall[CONFIG_ALU_MAPPING[i]] + m_stat_fu_utilization[CONFIG_ALU_MAPPING[i]] ) );
      }
      out_info("%-20.20s: (map:%d) (units:%d) %6.2lf %lld     %6.3lf %lld\n",
               pstate_t::fu_type_menomic( (fu_type_t) i ),
               CONFIG_ALU_MAPPING[i],
               CONFIG_NUM_ALUS[CONFIG_ALU_MAPPING[i]],
               ((double) m_stat_fu_utilization[CONFIG_ALU_MAPPING[i]] /
                (double) getLocalCycle()),
               m_stat_fu_utilization[CONFIG_ALU_MAPPING[i]],
               fraction_fu_stall,
               m_stat_fu_stall[CONFIG_ALU_MAPPING[i]]
               );
    }
  }
  out_info("\n");

  //loop through all logical procs:
  for(uint k=0; k <  CONFIG_LOGICAL_PER_PHY_PROC; ++k){  
    out_info("Execution unit retirement [logical proc %d]:\n", k);
    out_info("TYPE (id)         : (retired # of instructions)\n");
    for (uint32 i = 0; i < FU_NUM_FU_TYPES; i++) {
      if ( m_stat_fu_util_retired[k][i] != 0 ) {
        if (m_stat_committed[k] == 0)
          total_pct = 0.0;
        else
          total_pct = ((double) m_stat_fu_util_retired[k][i] / (double) m_stat_committed[k]) * 100.0;
        out_info("%-20.20s: %6.2f\t%lld\n",
                 pstate_t::fu_type_menomic( (fu_type_t) i ),
               total_pct,
               m_stat_fu_util_retired[k][i] );
      }
    }
    out_info("\n");
  }  //end for loop over all logical procs

  out_info("\n");
  out_info("*** Fastforward statistics:\n");
  for (uint32 i = 0; i < PSEQ_MAX_FF_LENGTH; i++) {
    if (m_hist_ff_length[i] != 0) {
      out_info("%5d: %24.24s\n",
               i,
               commafmt(m_hist_ff_length[i], buf, 80) );
    }
  }

 //loop through all logical procs:
 for(uint k=0; k <  CONFIG_LOGICAL_PER_PHY_PROC; ++k){  
    out_info("\n");
    out_info("*** Static prediction stats [logical proc %d]:\n",k);
    out_info("  When      NotPred       Pred      Taken        Not\n");
    out_info("  Decode %10.10s %10.10s %10.10s %10.10s\n",
             commafmt( m_nonpred_count_stat[k], buf, 80 ),
             commafmt( m_pred_count_stat[k], buf1, 80 ),
             commafmt( m_pred_count_taken_stat[k], buf2, 80 ),
             commafmt( m_pred_count_nottaken_stat[k], buf3, 80 ) );
    out_info("  Retire %10.10s %10.10s %10.10s %10.10s\n",
             commafmt( m_nonpred_retire_count_stat[k], buf, 80 ),
             commafmt( m_pred_retire_count_stat[k], buf1, 80 ),
             commafmt( m_pred_retire_count_taken_stat[k], buf2, 80 ),
             commafmt( m_pred_retire_count_nottaken_stat[k], buf3, 80 ));
    out_info("  Regs              %10.10s %10.10s %10.10s\n",
             commafmt( m_pred_reg_count_stat[k], buf1, 80 ),
             commafmt( m_pred_reg_taken_stat[k], buf2, 80 ),
             commafmt( m_pred_reg_nottaken_stat[k], buf3, 80 ));
    out_info("  RegRet            %10.10s %10.10s %10.10s\n",
             commafmt( m_pred_reg_retire_count_stat[k], buf1, 80 ),
             commafmt( m_pred_reg_retire_taken_stat[k], buf2, 80 ),
             commafmt( m_pred_reg_retire_nottaken_stat[k], buf3, 80 ));
 }  //end for loop over all logical procs

  int index = BRANCH_PCOND;
  if ( m_branch_seen_stat[index][2] != 0)
    bpaccuracy = (double) (m_branch_seen_stat[index][2] - m_branch_wrong_static_stat) / (double) m_branch_seen_stat[index][2] * 100;
  else
    bpaccuracy = 0.0;
  out_info( "   STATIC  %12.12s     %6.2lf%%\n",
            commafmt( m_branch_wrong_static_stat, buf, 80 ),
            bpaccuracy);

  // MLP statistics
  out_info( "\n" );
  out_info( "*** # of outstanding misses\n" );
  m_stat_hist_misses->print( this );
  out_info( "*** Interarrival times\n" );
  m_stat_hist_interarrival->print( this );
  out_info( "*** Dependent ops instrs\n" );
  m_stat_hist_dep_ops->print( this );

  out_info( "*** Effective\n" );
  m_stat_hist_effective_ind->print( this );
  out_info( "*** Not Effective\n" );
  m_stat_hist_effective_dep->print( this );
  out_info( "*** inter cluster\n" );
  m_stat_hist_inter_cluster->print( this );

  //loop through all logical procs:
 for(uint k=0; k <  CONFIG_LOGICAL_PER_PHY_PROC; ++k){  
  out_info("\n*** Pipeline statistics [logical proc %d]:\n", k);
  out_info("  %-50.50s %10llu\n", "cycle of decode stalls:", m_reg_stall_count_stat[k] );
  out_info("  %-50.50s %10llu\n", "insts of decode of stall:", m_decode_stall_count_stat[k] );
  out_info("  %-50.50s %10llu\n", "cycle of schedule stalls:", m_iwin_stall_count_stat[k] );
  out_info("  %-50.50s %10llu\n", "insts of schedule of stall:", m_schedule_stall_count_stat[k] );
  out_info("  %-50.50s %10llu\n", "count early store bypasses:", m_stat_early_store_bypass[k] );
  out_info("  %-50.50s %10llu\n", "total number of asi store squashes:", m_stat_count_asistoresquash[k]);
  out_info("  %-50.50s %10llu\n", "count failed retirements:", m_stat_count_badretire[k] );
  out_info("  %-50.50s %10llu\n", "count functional retirements:", m_stat_count_functionalretire[k] );
  out_info("  %-50.50s %10llu\n", "count of I/O loads/stores:", m_stat_count_io_access[k] );
  out_info("  %-50.50s %10llu\n", "count done/retry squashes:", m_stat_count_retiresquash[k] );
  out_info("  %-50.50s %10llu\n", "total number of times ideal processor couldn't reach end of Iwindow:", m_inorder_partial_success[k] );
  
  /* register baseline stats */
  out_info("  %-50.50s %10llu\n", "number of instructions read from trace:", m_stat_trace_insn[k]);
  out_info("  %-50.50s %10llu\n", "total number of instructions committed:", m_stat_committed[k]);
  out_info("  %-50.50s %10llu\n", "total number of times squash is called:", m_stat_total_squash[k]);
  out_info("  %-50.50s %10llu\n", "total number of times we overwrote next instr:", m_stat_modified_instructions[k]);
  out_info("  %-50.50s %10llu\n", "total number of instructions squashing at commit:", m_stat_commit_squash[k]);
  out_info("  %-50.50s %10llu\n", "total number of instructions committing successfully:", m_stat_commit_good[k]);
  out_info("  %-50.50s %10llu\n", "total number of instructions committing unsuccessfully:", m_stat_commit_bad[k]);
  out_info("  %-50.50s %10llu\n", "total number of unimplemented instructions committing:", m_stat_commit_unimplemented[k]);

  out_info("  %-50.50s %10llu\n", "total number of instructions fetched:", m_stat_fetched[k]);
  out_info("  %-50.50s %10llu\n", "total number of mini-itlb misses:", m_stat_mini_itlb_misses[k]);
  out_info("  %-50.50s %10llu\n", "total number of instructions decoded:", m_stat_decoded[k]);
  out_info("  %-50.50s %10llu\n", "total number of instructions executed:", m_stat_total_insts[k]);

  out_info("  %-50.50s %10llu\n", "total number of loads executed:", m_stat_loads_exec[k]);
  out_info("  %-50.50s %10llu\n", "total number of stores executed:", m_stat_stores_exec[k]);
  out_info("  %-50.50s %10llu\n", "total number of atomics executed:", m_stat_atomics_exec[k]);
  out_info("  %-50.50s %10llu\n", "total number of prefetches executed:", m_stat_prefetches_exec[k]);
  out_info("  %-50.50s %10llu\n", "total number of control insts executed:", m_stat_control_exec[k]);

  out_info("  %-50.50s %10llu\n", "total number of loads retired:",
           m_stat_loads_retired[k]);
  out_info("  %-50.50s %10llu\n", "total number of stores retired:",
           m_stat_stores_retired[k]);
  out_info("  %-50.50s %10llu\n", "retiring stores w/o correct cache perm:",
           m_stat_retired_stores_no_permission[k]);
  out_info("  %-50.50s %10llu\n", "retiring atomics w/o correct cache perm:",
           m_stat_retired_atomics_no_permission[k]);
  out_info("  %-50.50s %10llu\n", "retiring loads w/o correct cache perm:",
           m_stat_retired_loads_no_permission[k]);
  out_info("  %-50.50s %10llu\n", "retiring loads with non-matching data:",
           m_stat_retired_loads_nomatch[k]);
  out_info("  %-50.50s %10llu\n", "total number of atomics retired:",
           m_stat_atomics_retired[k]);
  out_info("  %-50.50s %10llu\n", "total number of prefetches retired:",
           m_stat_prefetches_retired[k]);
  out_info("  %-50.50s %10llu\n", "total number of control instrs committed:", m_stat_control_retired[k]);

  out_info("  %-50.50s %10llu\n", "loads with valid data at execute:", m_stat_loads_found[k]);
  out_info("  %-50.50s %10llu\n", "loads with invalid data at execute:", m_stat_loads_notfound[k]);
  out_info("  %-50.50s %10llu\n", "total number of spill traps:", m_stat_spill[k]);
  out_info("  %-50.50s %10llu\n", "total number of fill traps:", m_stat_fill[k]);
 
  /* cache misses per retired instruction stats */
  out_info("  %-50.50s %10llu\n", "number fetches executed which miss in icache",
           m_stat_num_icache_miss[k]);
  out_info("  %-50.50s %10llu\n", "number misses fetches that hit in mshr",
           m_stat_icache_mshr_hits[k] );

  out_info("  %-50.50s %10llu\n", "number loads executed which miss in dcache",
           m_stat_num_dcache_miss[k]);
  out_info("  %-50.50s %10llu\n", "retiring data cache misses",
           m_stat_retired_dcache_miss[k] );
  out_info("  %-50.50s %10llu\n", "retiring L2 misses",
           m_stat_retired_memory_miss[k] );
  out_info("  %-50.50s %10llu\n", "retiring MSHR hits",
           m_stat_retired_mshr_hits[k] );
  out_info("  %-50.50s %10llu\n", "number of times ruby is not ready",
           m_num_cache_not_ready[k]);
} //end for loop over all logical procs
  
  /* lsq stats */

 for(uint k=0; k <  CONFIG_LOGICAL_PER_PHY_PROC; ++k){  
  out_info("\n*** LSQ stats [logical proc %d]: \n",k);
  out_info("  %-50.50s %10llu\n", "number of load-store conflicts causing replay:", m_stat_load_store_conflicts[k]);
  out_info("  %-50.50s %10llu\n", "number of times load bypassed from incorrect store:", m_stat_load_incorrect_store[k]);
  out_info("  %-50.50s %10llu\n", "number of times atomic bypassed from incorrect store:", m_stat_atomic_incorrect_store[k]);
  out_info("  %-50.50s %10llu\n", "number of loads satisfied by store queue:", m_stat_load_bypasses[k]);
  out_info("  %-50.50s %10llu\n", "number of atomics satisfied by store queue:", m_stat_atomic_bypasses[k]);
  out_info("  %-50.50s %10llu\n", "number of stores scheduled before value:", m_stat_num_early_stores[k]);
  out_info("  %-50.50s %10llu\n", "number of loads waiting for early store resolution:", m_stat_num_early_store_bypasses[k]);
  out_info("  %-50.50s %10llu\n", "number of atomics scheduled before value:", m_stat_num_early_atomics[k]);
  out_info("  %-50.50s %10llu\n", "number of stale data speculations", m_stat_stale_predictions[k]);
  out_info("  %-50.50s %10llu\n", "number of successful stale predictions:", m_stat_stale_success[k]);
  if (m_stat_stale_predictions[k] != 0)
    out_info("  %-50.50s %6.2lf%%\n", "percent stale value predictions",
             100.0 * ((float) m_stat_stale_success[k] / (float) m_stat_stale_predictions[k]) );
  for (uint32 i=0; i < (log_base_two(MEMOP_MAX_SIZE*8)+1); i++) {
    out_info("  %-50.50s %4u %10llu\n", "stale prediction size = ", 1 << i,
             m_stat_stale_histogram[k][i] );
  }

  // StoreSet predictor stats
  out_info("\n*** StoreSet predictor stats [logical proc %d]:\n", k);
  out_info("  %-50.50s %10llu\n", "number of times loads were stalled:", m_stat_storeset_stall_load[k]);
  out_info("  %-50.50s %10llu\n", "number of times atomics were stalled:", m_stat_storeset_stall_atomic[k]);
  //print out structure of Storeset
  m_store_set_predictor->printConfig(this);

  // Write Buffer Stats
  out_info("\n*** Write Buffer stats [logical proc %d]: \n",k);
  out_info("  %-50.50s %10llu\n", "number of loads which hit WB:", m_stat_num_write_buffer_hits[k]);
  out_info("  %-50.50s %10llu\n", "number of times WB was full:", m_stat_num_write_buffer_full[k]);

 } // end for loop over all logical procs

  
  out_info("\n***Unimplemented inst stats (ldxa)\n");
  out_info("  %-50.50s %10llu\n", "virtual addr out of range:", m_stat_va_out_of_range);

  out_info("\n***Uncacheable ASI stats\n");
  out_info("\tASI #    Reads    Writes    Atomics\n");
  for(int i=0; i < MAX_NUM_ASI; ++i){
    if(m_stat_uncacheable_read_asi[i] != 0 || m_stat_uncacheable_write_asi[i] != 0 || m_stat_uncacheable_atomic_asi[i] != 0){
      out_info("\tASI[ 0x%llx ] = %lld    %lld    %lld\n", i, m_stat_uncacheable_read_asi[i], m_stat_uncacheable_write_asi[i], m_stat_uncacheable_atomic_asi[i]);
    }
  }

  out_info("\n***Functional ASI stats\n");
  out_info("\tASI #    Reads    Writes    Atomics\n");
  for(int i=0; i < MAX_NUM_ASI; ++i){
    if(m_stat_functional_read_asi[i] != 0 || m_stat_functional_write_asi[i] != 0 || m_stat_functional_atomic_asi[i] != 0){
      out_info("\tASI[ 0x%llx ] = %lld    %lld    %lld\n", i, m_stat_functional_read_asi[i], m_stat_functional_write_asi[i], m_stat_functional_atomic_asi[i]);
    }
  }

  if(CONFIG_WITH_RUBY){
    m_ruby_cache->printNLPrefetcherStats(this);
    
    //  m_ruby_cache->printInstrPageStats(this);

    m_ruby_cache->printL2MissStats(this);

    //print out dual addrs
    m_ruby_cache->printDualAddrs(this);
  }

  //print out detailed LSQ stats
  //m_lsq[k]->printStats( this, m_local_cycles);

  //print out structure of regstate predictor
  m_regstate_predictor->printConfig(this);

  /* WATTCH power stats */
  if(WATTCH_POWER){
    //print out L1 bank access stats...
     //print bank num stats
    out_info("\n***Bank Num Stats:\n");
    out_info("8 banks:\n");
    int64 total_l1isamples = 0;
    int64 total_l1dsamples = 0;
    for(int i=0; i < 9; ++i){
      total_l1isamples += m_l1i_8bank_histogram[i];
      total_l1dsamples += m_l1d_8bank_histogram[i];
    }
    
    if(total_l1isamples > 0){
      for(int i=0; i < 9; ++i){
        out_info("\tL1I_Bank_%d per cycle: %ld     %6.2f%%\n",i, m_l1i_8bank_histogram[i], m_l1i_8bank_histogram[i]*100.0/total_l1isamples);
      }
    }
    if(total_l1dsamples > 0){
      for(int i=0; i < 9; ++i){
        out_info("\tL1D_Bank_%d per cycle: %ld     %6.2f%%\n",i, m_l1d_8bank_histogram[i], m_l1d_8bank_histogram[i]*100.0/total_l1dsamples);
      }
    }
    
    total_l1isamples = 0;
    total_l1dsamples = 0;
    for(int i=0; i < 5; ++i){
      total_l1isamples += m_l1i_4bank_histogram[i];
      total_l1dsamples += m_l1d_4bank_histogram[i];
    }
    
    out_info("\n4 Banks:\n");
    if(total_l1isamples > 0){
      for(int i=0; i < 5; ++i){
        out_info("\tL1I_Bank_%d per cycle: %ld     %6.2f%%\n",i, m_l1i_4bank_histogram[i], m_l1i_4bank_histogram[i]*100.0/total_l1isamples);
      }
    }
    if(total_l1dsamples > 0){
      for(int i=0; i < 5; ++i){
        out_info("\tL1D_Bank_%d per cycle: %ld     %6.2f%%\n",i, m_l1d_4bank_histogram[i], m_l1d_4bank_histogram[i]*100.0/total_l1dsamples);
      }
    }
    
    total_l1isamples = 0;
    total_l1dsamples = 0;
    for(int i=0; i < 3; ++i){
      total_l1isamples += m_l1i_2bank_histogram[i];
      total_l1dsamples += m_l1d_2bank_histogram[i];
    }
    
    out_info("\n2 Banks:\n");
    if(total_l1isamples > 0){
      for(int i=0; i < 3; ++i){
        out_info("\tL1I_Bank_%d per cycle: %ld     %6.2f%%\n",i, m_l1i_2bank_histogram[i], m_l1i_2bank_histogram[i]*100.0/total_l1isamples);
      }
    }
    if(total_l1dsamples > 0){
      for(int i=0; i < 3; ++i){
        out_info("\tL1D_Bank_%d per cycle: %ld     %6.2f%%\n",i, m_l1d_2bank_histogram[i], m_l1d_2bank_histogram[i]*100.0/total_l1dsamples);
      }
    }
    
    //print WATTCH stats...
    uint64 total_insn = 0;
    //first get the total number of insns committed:
    for(uint i=0; i < CONFIG_LOGICAL_PER_PHY_PROC; ++i){
      total_insn += m_stat_committed[i];
    }
    out_info("\n*** WATTCH power stats:\n");
    m_power_stats->power_reg_stats(m_local_cycles, total_insn);
    out_info("\n");
  }

  // print cache statistics only if ruby is not being used
  if (!CONFIG_WITH_RUBY) {
    l2_cache->printStats( this );
    l2_mshr->printStats( this );
    l1_data_cache->printStats( this );
    dl1_mshr->printStats( this );
    l1_inst_cache->printStats( this );
    il1_mshr->printStats( this );
  }

  //print out SMT icount stats
  for(uint32 k=0; k <  CONFIG_LOGICAL_PER_PHY_PROC; ++k){  
    out_info("\n***ICount Stats [logical proc %d]: \n",k);
    double avg_icount = 0.0;
    if(m_icount_stats[k].num_samples >0){
      avg_icount = (double) (m_icount_stats[k].icount_sum)/(double)(m_icount_stats[k].num_samples);
    }
    out_info("\tICount: max[ %lld ]  samples[ %lld ] avg[ %6.2f ]\n", m_icount_stats[k].max_icount, m_icount_stats[k].num_samples, avg_icount);
  }
  
  //print out cache latency stats...
  for(uint32 k=0; k <  CONFIG_LOGICAL_PER_PHY_PROC; ++k){  
    out_info("\n***Cache Miss Latency [logical proc %d]: \n",k);
    double ifetch_avg_latency, load_avg_latency, store_avg_latency, atomic_avg_latency;
    ifetch_avg_latency = load_avg_latency = store_avg_latency = atomic_avg_latency = 0.0;
    if(m_ifetch_miss_latency[k].num_misses > 0){
      ifetch_avg_latency = (double) (m_ifetch_miss_latency[k].latency_sum)/(double) (m_ifetch_miss_latency[k].num_misses);
    }
    if(m_load_miss_latency[k].num_misses >0){
      load_avg_latency = (double) (m_load_miss_latency[k].latency_sum)/(double) (m_load_miss_latency[k].num_misses);
    }
    if(m_store_miss_latency[k].num_misses > 0){
      store_avg_latency = (double) (m_store_miss_latency[k].latency_sum)/(double) (m_store_miss_latency[k].num_misses);
    }
    if(m_atomic_miss_latency[k].num_misses > 0){
      atomic_avg_latency = (double) (m_atomic_miss_latency[k].latency_sum)/(double) (m_atomic_miss_latency[k].num_misses);
    }
    out_info("\tIFETCH avg latency: fastpath[ %lld ] miss[ %lld ] %6.2f\n", m_ifetch_miss_latency[k].num_fastpath,m_ifetch_miss_latency[k].num_misses, ifetch_avg_latency);
    out_info("\tLOAD avg latency: fastpath[ %lld ] miss[ %lld ] %6.2f\n", m_load_miss_latency[k].num_fastpath, m_load_miss_latency[k].num_misses, load_avg_latency);
    out_info("\tSTORE avg latency: fastpath[ %lld ] miss[ %lld ] %6.2f\n", m_store_miss_latency[k].num_fastpath, m_store_miss_latency[k].num_misses, store_avg_latency);
    out_info("\tATOMIC avg latency: fastpath[ %lld ] miss[ %lld ] %6.2f\n", m_atomic_miss_latency[k].num_fastpath, m_atomic_miss_latency[k].num_misses, atomic_avg_latency);
  }

  for(uint32 k=0; k <  CONFIG_LOGICAL_PER_PHY_PROC; ++k){  
   out_info("\nNot enough registers [logical proc %d]: %lld\n", k, m_stat_not_enough_registers[k]);
   m_ooo.regs.printStats(this, k);
   out_info("Scheduling window exceeded [logical proc %d]: %lld\n",k, m_stat_exceed_scheduling_window[k]);
  }

  //MLP - print out MLP stats
  if(CONFIG_WITH_RUBY){
    m_ruby_cache->printMLP(m_local_cycles, this);
  }
  
  // L2 miss dep - print out stats on instr dependent on L2 miss dest regs
  printL2MissDepStats();
  
  // print out instruction page statistics
  for(uint k=0; k <  CONFIG_LOGICAL_PER_PHY_PROC; ++k){  
   out_info("\n***Imap Stats [logical proc %d]: \n",k);
   m_imap[k]->printStats();
  }
  
  // print how long it took to simulate, and time per cycle
  int64 sec_expired  = 0;
  int64 usec_expired = 0;
  m_simulation_timer.getCumulativeTime( &sec_expired, &usec_expired );

 for(uint k=0; k <  CONFIG_LOGICAL_PER_PHY_PROC; ++k){  
  out_info("\n*** Runtime statistics [logical proc %d]:\n",k);
  out_info("  %-50.50s %10llu\n", "Total number of instructions",
           m_stat_committed[k] );
  out_info("  %-50.50s %10llu\n", "Total number of cycles",
           getLocalCycle() );
  out_info("  %-50.50s %10llu\n", "number of continue calls", m_stat_continue_calls[k] );
  out_info("  %-50.50s %g\n", "Instruction per cycle:",
           (double) m_stat_committed[k] / (double) getLocalCycle() );
 } //end for loop over all logical procs

  out_info("  %-50.50s %lu sec %lu usec\n", "Total Elapsed Time:",
           sec_expired, usec_expired );

  int64 retire_sec  = 0;
  int64 retire_usec = 0;
  m_retirement_timer.getCumulativeTime( &retire_sec, &retire_usec );
  out_info("  %-50.50s %lld sec %lld usec\n", "Total Retirement Time:",
           retire_sec, retire_usec );
  
  double dsec = (double) sec_expired + ((double) usec_expired / (double) 1000000);

  out_info("  %-50.50s %g\n", "Approximate cycle per sec:",
           (double) getLocalCycle() / dsec);

 for(uint k=0; k <  CONFIG_LOGICAL_PER_PHY_PROC; ++k){  
   out_info("  %-50.50s [logical proc %d] %g\n", "Approximate instructions per sec:",k,
             (double) m_stat_committed[k] / dsec );
 }

  double dretire = (double) retire_sec + ((double) retire_usec / (double) 1000000);
  double overhead = 100.0;
  if ( dsec != 0 ) {
    overhead = (dretire / dsec) * 100.0;
  }
  out_info("  %-50.50s %6.2lf%%\n", "This processor's Simics overhead (retire/elapsed):", overhead );

 for(uint k=0; k <  CONFIG_LOGICAL_PER_PHY_PROC; ++k){  
  if (m_stat_continue_calls[k] != 0) {
    out_info("  %-50.50s [logical proc %d] %6.2lf\n", "Average number of instructions per continue:", k, ((double) m_stat_committed[k] / (double) m_stat_continue_calls[k]) );
    }
  }

#ifdef RENAME_EXPERIMENT
  for (uint32 i = 0; i < RID_NUM_RID_TYPES; i++ ) {
    uint32 logical;
    switch (i) {
    case RID_INT:
      logical = CONFIG_IREG_LOGICAL;
      out_info("Integer information\n");
      break;

    case RID_SINGLE:
      logical = CONFIG_FPREG_LOGICAL;
      out_info("Floating point info\n");
      break;

    case RID_CC:
      logical = CONFIG_CCREG_LOGICAL;
      out_info("Condition code info\n");
      break;

    default:
      // we don't track rename statistics on all other registers
      continue;
    }

    double sum;
    for (uint32 j = 0; j < logical; j++) {
      sum = 0.0;
      for (uint32 k = 0; k < PSEQ_REG_USE_HIST; k++) {
        sum += (double) m_reg_use[i][j][k];
        out_info("%d ", m_reg_use[i][j][k]);
      }
      out_info("(%d %f)\n", j, sum);
    }
    out_info("\n\n");
  }
#endif
  setDebugTime( filterTime );

  #ifdef DEBUG_PSEQ
  DEBUG_OUT("pseq_t:printStats END\n");
  #endif
}

//**************************************************************************
void pseq_t::validateTrace(unsigned int proc )
{

  #ifdef DEBUG_PSEQ
  DEBUG_OUT("pseq_t:validateTrace BEGIN\n");
  #endif

  ASSERT(proc < CONFIG_LOGICAL_PER_PHY_PROC);

   if (m_tracefp[proc]){
     out_info("\n***Trace Validation [logical proc %d] \n",proc);
     m_tracefp[proc]->validate();
   }

  #ifdef DEBUG_PSEQ
  DEBUG_OUT("pseq_t:validateTrace END\n");
  #endif
}

//**************************************************************************
uint32 pseq_t::printIpage( bool verbose, unsigned int proc )
{
  ASSERT(proc < CONFIG_LOGICAL_PER_PHY_PROC);

  // count the total number of instructions on an instruction page
  out_info("total static instruction count [logical proc %d]:\n",proc);
  uint32 count = m_imap[proc]->print( verbose );
  return (count);
}

//***************************************************************************
const char *pseq_t::fetch_menomic( pseq_fetch_status_t status )
{
  switch ( status ) {
  case PSEQ_FETCH_READY:
    return("Fetch ready");
    break;
  case PSEQ_FETCH_ICACHEMISS:
    return("Fetch i-cache miss");
    break;
  case PSEQ_FETCH_SQUASH:
    return("Fetch squash");
    break;
  case PSEQ_FETCH_ITLBMISS:
    return("Fetch I-TLB miss");
    break;
  case PSEQ_FETCH_WIN_FULL:
    return("Window Full");
    break;
  case PSEQ_FETCH_BARRIER:
    return("Fetch Barrier");
    break;
  case PSEQ_FETCH_WRITEBUFFER_FULL:
    return("Write Buffer Full");
    break;
  default:
    return("Unknown Fetch");
  }
  return("Unknown Fetch");
}

//***************************************************************************
const char *pseq_t::retire_menomic( pseq_retire_status_t status )
{
  switch ( status ) {
  case PSEQ_RETIRE_READY:
    return("Retire ready");
    break;
  case PSEQ_RETIRE_UPDATEALL:
    return("Retire Updating...");
    break;
  case PSEQ_RETIRE_SQUASH:
    return("Retire Squash");
    break;
  case PSEQ_RETIRE_LIMIT:
    return("Retire Limit");
    break;
  default:
    return("Unknown Retire");
  }
  return("Unknown Retire");
}

//***************************************************************************
// void  pseq_t::slidingWindowFill( void )

/*------------------------------------------------------------------------*/
/* Statistics Gathering                                            
 */
/*------------------------------------------------------------------------*/

//**************************************************************************
void 
pseq_t::initializeStats(void) {
  #ifdef DEBUG_PSEQ
  DEBUG_OUT("pseq_t:initializeStats BEGIN\n");
  #endif

  //WATTCH power: bank num stats
  for(int i=0; i < 8; ++i){
    m_l1i_banknum_accesses[i] = 0;
    m_l1d_banknum_accesses[i] = 0;
  }
  for(int i=0; i < 9; ++i){
    m_l1i_8bank_histogram[i] = 0;
    m_l1d_8bank_histogram[i] = 0;
  }
  for(int i=0; i < 5; ++i){
    m_l1i_4bank_histogram[i] = 0;
    m_l1d_4bank_histogram[i] = 0;
  }
  for(int i=0; i < 3; ++i){
    m_l1i_2bank_histogram[i] = 0;
    m_l1d_2bank_histogram[i] = 0;
  }

  m_overall_timer = new stopwatch_t *[CONFIG_LOGICAL_PER_PHY_PROC];
  m_thread_timer = new stopwatch_t *[CONFIG_LOGICAL_PER_PHY_PROC];
  m_thread_histogram = new histogram_t *[CONFIG_LOGICAL_PER_PHY_PROC];
  m_exclude_count = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_thread_count = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_thread_count_idle = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_opstat = new decode_stat_t[CONFIG_LOGICAL_PER_PHY_PROC]; 
  m_ideal_opstat = new decode_stat_t[CONFIG_LOGICAL_PER_PHY_PROC]; 

  //stats for full scheduling window and not enough registers
  m_stat_exceed_scheduling_window = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_stat_not_enough_registers = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];

  //allocate per thread cache miss latency stats
  m_ifetch_miss_latency = new miss_latency_histogram_t[CONFIG_LOGICAL_PER_PHY_PROC];
  m_load_miss_latency = new miss_latency_histogram_t[CONFIG_LOGICAL_PER_PHY_PROC];
  m_store_miss_latency = new miss_latency_histogram_t[CONFIG_LOGICAL_PER_PHY_PROC];
  m_atomic_miss_latency = new miss_latency_histogram_t[CONFIG_LOGICAL_PER_PHY_PROC];

  m_icount_stats = new icount_histogram_t[CONFIG_LOGICAL_PER_PHY_PROC];

  assert(m_ifetch_miss_latency != NULL);
  assert(m_load_miss_latency != NULL);
  assert(m_store_miss_latency != NULL);
  assert(m_atomic_miss_latency != NULL);

  //allocate per-thread trap stats...
  m_trapstat = new uint64*[CONFIG_LOGICAL_PER_PHY_PROC];
  m_simtrapstat = new uint64*[CONFIG_LOGICAL_PER_PHY_PROC];
  m_software_trapstat =  new uint64*[CONFIG_LOGICAL_PER_PHY_PROC];
  m_completed_trapstat =  new uint64*[CONFIG_LOGICAL_PER_PHY_PROC];

  for(uint32 proc=0; proc < CONFIG_LOGICAL_PER_PHY_PROC; ++proc){
    m_trapstat[proc] = new uint64[TRAP_NUM_TRAP_TYPES];
    m_simtrapstat[proc] = new uint64[TRAP_NUM_TRAP_TYPES];
    m_software_trapstat[proc] = new uint64[MAX_SOFTWARE_TRAPS];
    m_completed_trapstat[proc] = new uint64[TRAP_NUM_TRAP_TYPES];

    for (int i = 0; i < TRAP_NUM_TRAP_TYPES; i ++) {
      m_trapstat[proc][i] = 0;
      m_simtrapstat[proc][i] = 0;
      m_completed_trapstat[proc][i] = 0;
    }
    for(int i=0; i < MAX_SOFTWARE_TRAPS; ++i){
      m_software_trapstat[proc][i] = 0;
    }

    m_stat_exceed_scheduling_window[proc] = 0;
    m_stat_not_enough_registers[proc] = 0;
    m_icount_stats[proc].icount_sum = 0;
    m_icount_stats[proc].num_samples = 0;
    m_icount_stats[proc].max_icount = 0;

    m_ifetch_miss_latency[proc].num_misses = 0;
    m_ifetch_miss_latency[proc].latency_sum =0;
    m_ifetch_miss_latency[proc].num_fastpath = 0;
    m_load_miss_latency[proc].num_misses = 0;
    m_load_miss_latency[proc].latency_sum =0;
    m_load_miss_latency[proc].num_fastpath = 0;
    m_store_miss_latency[proc].num_misses = 0;
    m_store_miss_latency[proc].latency_sum =0;
    m_store_miss_latency[proc].num_fastpath = 0;
    m_atomic_miss_latency[proc].num_misses = 0;
    m_atomic_miss_latency[proc].latency_sum =0;
    m_atomic_miss_latency[proc].num_fastpath = 0;
  }

  for (uint32 i = 0; i < MAX_NUM_ASI; i ++) {
    m_asi_rd_stat[i] = 0;
    m_asi_wr_stat[i] = 0;
    m_asi_at_stat[i] = 0;
  }
  m_hist_fetch_per_cycle = (uint64 *) malloc( sizeof(uint64) * (MAX_FETCH+1));
  for (uint32 i = 0; i < MAX_FETCH + 1; i++) {
    m_hist_fetch_per_cycle[i] = 0;
  }
  //per-thread histogram
  m_hist_fetch_per_thread = new uint64*[CONFIG_LOGICAL_PER_PHY_PROC];
  m_hist_decode_per_thread = new uint64*[CONFIG_LOGICAL_PER_PHY_PROC];
  m_hist_schedule_per_thread = new uint64*[CONFIG_LOGICAL_PER_PHY_PROC];
  m_hist_retire_per_thread = new uint64*[CONFIG_LOGICAL_PER_PHY_PROC];

  for(uint32 j=0; j < CONFIG_LOGICAL_PER_PHY_PROC; ++j){
    m_hist_fetch_per_thread[j] = new uint64[MAX_FETCH+1];
    m_hist_decode_per_thread[j] = new uint64[MAX_DECODE+1];
    m_hist_schedule_per_thread[j] = new uint64[MAX_DISPATCH+1];
    m_hist_retire_per_thread[j] = new uint64[MAX_RETIRE+1];

    for (uint32 i = 0; i < MAX_FETCH + 1; i++) {
      m_hist_fetch_per_thread[j][i] = 0;
    }
    for (uint32 i = 0; i < MAX_DECODE + 1; i++) {
      m_hist_decode_per_thread[j][i] = 0;
    }
    for (uint32 i = 0; i < MAX_DISPATCH + 1; i++) {
      m_hist_schedule_per_thread[j][i] = 0;
    }
    for (uint32 i = 0; i < MAX_RETIRE + 1; i++) {
      m_hist_retire_per_thread[j][i] = 0;
    }
  }

 m_hist_smt_fetch_per_cycle = (uint64 *) malloc( sizeof(uint64) * (m_threads_per_cycle+1));
  for (uint32 i = 0; i < m_threads_per_cycle + 1; i++) {
    m_hist_smt_fetch_per_cycle[i] = 0;
  }
  m_hist_decode_per_cycle = (uint64 *) malloc( sizeof(uint64) * (MAX_DECODE+1) );
  for (uint32 i = 0; i < MAX_DECODE + 1; i++) {
    m_hist_decode_per_cycle[i] = 0;
  }
  m_hist_schedule_per_cycle = (uint64 *) malloc( sizeof(uint64) * (MAX_DISPATCH+1) );
  for (uint32 i = 0; i < MAX_DISPATCH + 1; i++) {
    m_hist_schedule_per_cycle[i] = 0;
  }
  m_hist_retire_per_cycle = (uint64 *) malloc( sizeof(uint64) * (MAX_RETIRE+1) );
  for (uint32 i = 0; i < MAX_RETIRE + 1; i++) {
    m_hist_retire_per_cycle[i] = 0;
  }

  m_hist_fetch_stalls = (uint64 *) malloc( sizeof(uint64) * PSEQ_FETCH_MAX_STATUS );
  for (uint32 i = 0; i < PSEQ_FETCH_MAX_STATUS; i++) {
    m_hist_fetch_stalls[i] = 0;
  }
  m_hist_retire_stalls = (uint64 *) malloc( sizeof(uint64) * PSEQ_RETIRE_MAX_STATUS );
  for (uint32 i = 0; i < PSEQ_RETIRE_MAX_STATUS; i++) {
    m_hist_retire_stalls[i] = 0;
  }

  m_hist_squash_stage = (uint64 **) malloc( sizeof(uint64 *) *
                                            (dynamic_inst_t::MAX_INST_STAGE) );
  for (uint32 i = 0; i < dynamic_inst_t::MAX_INST_STAGE; i++) {
    m_hist_squash_stage[i] = (uint64 *) malloc( sizeof(uint64) * (IWINDOW_ROB_SIZE) );
    for (int32 j = 0; j < IWINDOW_ROB_SIZE; j++) {
      m_hist_squash_stage[i][j] = 0;
    }
  }

  m_stat_retire_notready_stage = new uint64[dynamic_inst_t::MAX_INST_STAGE];
  for(int i=0; i < dynamic_inst_t::MAX_INST_STAGE; ++i){
    m_stat_retire_notready_stage[i] = 0;
  }
  
  m_hist_decode_return = (uint64 *) malloc( sizeof(uint64) * (PSEQ_HIST_DECODE) );
  for (int32 i = 0; i < PSEQ_HIST_DECODE; i++) {
    m_hist_decode_return[i] = 0;
  }

  m_hist_ff_length = (uint64 *) malloc( sizeof(uint64) * PSEQ_MAX_FF_LENGTH );
  for (uint32 i = 0; i < PSEQ_MAX_FF_LENGTH; i++) {
    m_hist_ff_length[i] = 0;
  }

  m_hist_ideal_coverage = (uint64 *) malloc( sizeof(uint64) * PSEQ_FETCH_MAX_STATUS );
  for (uint32 i = 0; i < PSEQ_FETCH_MAX_STATUS; i++) {
    m_hist_ideal_coverage[i] = 0;
  }

  m_stat_no_fetch_taken_branch = 0;
  m_stat_no_fetch_across_lines = 0;
  
  m_stat_fu_utilization = (uint64 *) malloc( sizeof(uint64) * 
                                             FU_NUM_FU_TYPES );
  m_stat_fu_stall = (uint64 *) malloc( sizeof(uint64) * 
                                             FU_NUM_FU_TYPES );
  for (uint32 i = 0; i < FU_NUM_FU_TYPES; i++) {
    m_stat_fu_utilization[i] = 0;
    m_stat_fu_stall[i] = 0;
  }

  m_stat_fu_util_retired = (uint64 **) malloc(sizeof(uint64 *) * CONFIG_LOGICAL_PER_PHY_PROC);
  for(uint k=0; k <  CONFIG_LOGICAL_PER_PHY_PROC; ++k){  
    m_stat_fu_util_retired[k] = (uint64 *) malloc( sizeof(uint64) * 
                                                   FU_NUM_FU_TYPES );
    for (uint32 i = 0; i < FU_NUM_FU_TYPES; i++) {
      m_stat_fu_util_retired[k][i] = 0;
    }
  }
    
  for (uint32 i = 0; i < BRANCH_NUM_BRANCH_TYPES; i++) {
    for (uint32 j = 0; j < TOTAL_INSTR_MODE; j++) {
      m_branch_pred_stat[i][j]  = 0;
      m_branch_seen_stat[i][j]  = 0;
      m_branch_right_stat[i][j] = 0;
      m_branch_wrong_stat[i][j] = 0;
    }
    m_branch_except_stat[i] = 0;
  }

  m_stat_memread_exception =  new uint64[CONFIG_LOGICAL_PER_PHY_PROC]; 
  m_last_simexception = new trap_type_t[CONFIG_LOGICAL_PER_PHY_PROC];
  m_last_traptype = new trap_type_t*[CONFIG_LOGICAL_PER_PHY_PROC];
  m_last_traptype_cycle = new uint64*[CONFIG_LOGICAL_PER_PHY_PROC];
  m_last_traplevel = new int[CONFIG_LOGICAL_PER_PHY_PROC];
  for(uint i=0; i < CONFIG_LOGICAL_PER_PHY_PROC; ++i){
    m_last_traplevel[i] = 0;
    m_last_simexception[i] = Trap_NoTrap;
    m_last_traptype[i] = new trap_type_t[MAXTL+1];
    m_last_traptype_cycle[i] = new uint64[MAXTL+1];
    for(int j=0; j <= MAXTL; ++j){
      m_last_traptype[i][j] = Trap_NoTrap;
      m_last_traptype_cycle[i][j] = 0;
    }
  }

  m_stat_va_out_of_range = 0;

  for(int i=0; i < MAX_NUM_ASI; ++i){
    m_stat_uncacheable_read_asi[i] = 0;
    m_stat_uncacheable_write_asi[i] = 0;
    m_stat_uncacheable_atomic_asi[i] = 0;
    m_stat_functional_read_asi[i] = 0;
    m_stat_functional_write_asi[i] = 0;
    m_stat_functional_atomic_asi[i] = 0;
  }

  //Used to track start and end of WB stall cycles
  m_wb_stall_start_cycle = 0;
  m_wb_stall_end_cycle = 0;

  m_stat_count_functionalretire = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_stat_early_store_bypass = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_stat_count_badretire = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_stat_count_retiresquash = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  
  m_stat_trace_insn = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_stat_committed = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_stat_commit_squash = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_stat_count_asistoresquash = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_stat_commit_good = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_stat_commit_bad = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_stat_commit_unimplemented = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_stat_count_except = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];

  m_stat_loads_retired = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_stat_stores_retired = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_stat_retired_stores_no_permission = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_stat_retired_atomics_no_permission = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_stat_retired_loads_no_permission = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_stat_retired_loads_nomatch = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_stat_atomics_retired = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_stat_prefetches_retired = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_stat_control_retired = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_stat_fetched = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_stat_mini_itlb_misses = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_stat_decoded = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_stat_total_insts = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];

  m_stat_loads_exec = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_stat_stores_exec = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_stat_atomics_exec = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_stat_prefetches_exec = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_stat_control_exec = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_stat_loads_found = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_stat_loads_notfound = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_stat_total_squash = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_stat_spill = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_stat_fill = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];

  m_stat_miss_count = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_stat_last_miss_seq = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_stat_last_miss_fetch = new tick_t[CONFIG_LOGICAL_PER_PHY_PROC];
  m_stat_last_miss_issue = new tick_t[CONFIG_LOGICAL_PER_PHY_PROC];
  m_stat_last_miss_retire = new tick_t[CONFIG_LOGICAL_PER_PHY_PROC];

  m_stat_miss_effective_ind = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_stat_miss_effective_dep = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_stat_miss_inter_cluster = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];

  m_stat_load_bypasses = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_stat_atomic_bypasses = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_stat_num_early_stores = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_stat_num_early_store_bypasses = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_stat_num_early_atomics = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_stat_load_store_conflicts = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_stat_load_incorrect_store = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_stat_atomic_incorrect_store = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_stat_storeset_stall_load = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_stat_storeset_stall_atomic = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_stat_stale_predictions = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_stat_stale_success = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_stat_stale_histogram = new uint64 *[CONFIG_LOGICAL_PER_PHY_PROC];

  m_pred_count_stat = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_pred_count_taken_stat = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_pred_count_nottaken_stat = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_nonpred_count_stat = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_pred_reg_count_stat = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_pred_reg_taken_stat = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_pred_reg_nottaken_stat = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  
  m_pred_retire_count_stat = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_pred_retire_count_taken_stat = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_pred_retire_count_nottaken_stat = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_nonpred_retire_count_stat = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_pred_reg_retire_count_stat = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_pred_reg_retire_taken_stat = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_pred_reg_retire_nottaken_stat = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  
  m_reg_stall_count_stat = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_decode_stall_count_stat = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_iwin_stall_count_stat = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_schedule_stall_count_stat = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];

  m_exception_stat = new uint64 *[CONFIG_LOGICAL_PER_PHY_PROC];
  for(uint k=0; k <  CONFIG_LOGICAL_PER_PHY_PROC; ++k){
    m_exception_stat[k] = new uint64[EXCEPT_NUM_EXCEPT_TYPES];
  }

  // Comment in if want to perform retirement timing
  //  m_simulation_timer = new utimer_t[CONFIG_LOGICAL_PER_PHY_PROC];
  //  m_retirement_timer = new utimer_t[CONFIG_LOGICAL_PER_PHY_PROC];

  m_stat_continue_calls = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_stat_modified_instructions = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_inorder_partial_success = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  
  for(uint k=0; k <  CONFIG_LOGICAL_PER_PHY_PROC; ++k){
    m_overall_timer[k] = new stopwatch_t(m_id+k);         
    m_thread_timer[k] = new stopwatch_t(m_id+k);          
    m_thread_histogram[k] = new histogram_t( "ThreadPC", 2048 );
    m_exclude_count[k] = 0;
    m_thread_count[k] = 0;
    m_thread_count_idle[k] = 0;

    m_stat_early_store_bypass[k] = 0;
    //    m_branch_wrong_static_stat[k] = 0;
    m_stat_count_badretire[k] = 0;
    m_stat_count_functionalretire[k] = 0;
    m_stat_count_retiresquash[k] = 0;

    m_stat_trace_insn[k] = 0;
    m_stat_committed[k] = 0;
    m_stat_commit_squash[k] = 0;
    m_stat_count_asistoresquash[k] = 0;
    m_stat_commit_good[k] = 0;
    m_stat_commit_bad[k] = 0;
    m_stat_commit_unimplemented[k] = 0;
    m_stat_count_except[k] = 0;
    
    m_stat_loads_retired[k] = 0;
    m_stat_stores_retired[k] = 0;
    m_stat_retired_stores_no_permission[k] = 0;
    m_stat_retired_atomics_no_permission[k] = 0;
    m_stat_retired_loads_no_permission[k] = 0;
    m_stat_retired_loads_nomatch[k] = 0;
    m_stat_atomics_retired[k] = 0;
    m_stat_prefetches_retired[k] = 0;
    m_stat_control_retired[k] = 0;
    m_stat_fetched[k] = 0;
    m_stat_mini_itlb_misses[k] = 0;
    m_stat_decoded[k] = 0;
    m_stat_total_insts[k] = 0;

    m_stat_loads_exec[k] = 0;
    m_stat_stores_exec[k] = 0;
    m_stat_atomics_exec[k] = 0;
    m_stat_prefetches_exec[k] = 0;
    m_stat_control_exec[k] = 0;
    m_stat_loads_found[k] = 0;
    m_stat_loads_notfound[k] = 0;
    m_stat_total_squash[k] = 0;
    m_stat_spill[k] = 0;
    m_stat_fill[k] = 0;

    //main memory stats
    m_stat_miss_count[k] = 0;
    m_stat_last_miss_seq[k] = 0;
    m_stat_last_miss_fetch[k] = 0;
    m_stat_last_miss_issue[k] = 0;
    m_stat_last_miss_retire[k] = 0;

    m_stat_miss_effective_ind[k] = 0;
    m_stat_miss_effective_dep[k] = 0;
    m_stat_miss_inter_cluster[k] = 0;
    
    // lsq stats 
    m_stat_load_bypasses[k] = 0;
    m_stat_atomic_bypasses[k] = 0;
    m_stat_num_early_stores[k] = 0;
    m_stat_num_early_store_bypasses[k] = 0;
    m_stat_num_early_atomics[k] = 0;
    m_stat_load_store_conflicts[k] = 0;
    m_stat_load_incorrect_store[k] = 0;
    m_stat_atomic_incorrect_store[k] = 0;
    m_stat_stale_predictions[k] = 0;
    m_stat_stale_success[k] = 0;
    m_stat_stale_histogram[k] = (uint64 *)malloc( sizeof(uint64)*
                                                  (log_base_two(MEMOP_MAX_SIZE*8)+1) );
    for (uint32 i = 0; i < (log_base_two(MEMOP_MAX_SIZE*8) + 1); i++) {  
      m_stat_stale_histogram[k][i] = 0;
    }

    // StoreSet stats
    m_stat_storeset_stall_load[k] = 0;
    m_stat_storeset_stall_atomic[k] = 0;

    // predictation statistics
    m_pred_count_stat[k] = 0;
    m_pred_count_taken_stat[k] = 0;
    m_pred_count_nottaken_stat[k] = 0;
    m_nonpred_count_stat[k] = 0;
    m_pred_reg_count_stat[k] = 0;
    m_pred_reg_taken_stat[k] = 0;
    m_pred_reg_nottaken_stat[k] = 0;
    
    m_pred_retire_count_stat[k] = 0;
    m_pred_retire_count_taken_stat[k] = 0;
    m_pred_retire_count_nottaken_stat[k] = 0;
    m_nonpred_retire_count_stat[k] = 0;
    m_pred_reg_retire_count_stat[k] = 0;
    m_pred_reg_retire_taken_stat[k] = 0;
    m_pred_reg_retire_nottaken_stat[k] = 0;
    
    m_reg_stall_count_stat[k] = 0;
    m_decode_stall_count_stat[k] = 0;
    m_iwin_stall_count_stat[k] = 0;
    m_schedule_stall_count_stat[k] = 0;

    m_stat_continue_calls[k] = 0;
    m_stat_modified_instructions[k] = 0;
    m_inorder_partial_success[k] = 0;

    for (int i = 0; i < EXCEPT_NUM_EXCEPT_TYPES; i ++) {
      m_exception_stat[k][i] = 0;
    }
  }   //end for all logical procs

  m_branch_wrong_static_stat = 0;  
 

  /* write buffer stats */
  m_stat_num_write_buffer_hits = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_stat_num_write_buffer_full = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];

  /* cache stats */
  m_stat_num_icache_miss = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_stat_icache_mshr_hits = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_stat_num_dcache_miss = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_stat_retired_memory_miss = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_stat_retired_dcache_miss = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_stat_retired_mshr_hits = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_stat_count_io_access = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];
  m_num_cache_not_ready = new uint64[CONFIG_LOGICAL_PER_PHY_PROC];

  for(uint k=0; k <  CONFIG_LOGICAL_PER_PHY_PROC; ++k){
    m_stat_num_write_buffer_hits[k] = 0;
    m_stat_num_write_buffer_full[k] = 0;

    m_stat_num_icache_miss[k] = 0;
    m_stat_num_dcache_miss[k] = 0;
    m_stat_icache_mshr_hits[k] = 0;
    m_stat_retired_dcache_miss[k] = 0;
    m_stat_retired_memory_miss[k] = 0;
    m_stat_retired_mshr_hits[k] = 0;
    m_stat_count_io_access[k] = 0;
    m_num_cache_not_ready[k] = 0;
  }

  m_stat_hist_misses = new histogram_t( "Misses", 1024 );
  m_stat_hist_interarrival = new histogram_t( "Bursts", 4096 );

  m_stat_hist_effective_ind  = new histogram_t( "EffInd", 4096 );
  m_stat_hist_effective_dep  = new histogram_t( "EffDep", 4096 );
  m_stat_hist_inter_cluster  = new histogram_t( "IClust", 4096 );
  m_stat_hist_dep_ops        = new histogram_t( "DepOps", 4096 );

  #ifdef DEBUG_PSEQ
    DEBUG_OUT("pseq_t:initializeStats END\n");
  #endif
  
}

//**************************************************************************
void pseq_t::L1IBankNumStat(int banknum){
  m_l1i_banknum_accesses[banknum]++;
}

//**************************************************************************
void pseq_t::L1DBankNumStat(int banknum){
  m_l1d_banknum_accesses[banknum]++;
}

//**************************************************************************
void pseq_t::collectBankNumStat(){
  //  cout << "collectbanknumstat BEGIN" << endl;
  int unique_l1i_8banks = 0;
  int unique_l1d_8banks = 0;
  int unique_l1i_4banks = 0;
  int unique_l1d_4banks = 0;
  int unique_l1i_2banks = 0;
  int unique_l1d_2banks = 0;

  //do for 8 banks
  for(int i=0; i < 8; ++i){
    if(m_l1i_banknum_accesses[i] > 0){
      unique_l1i_8banks++;
    }
  }

  //do the same for data cache
  for(int i=0; i < 8; ++i){
    if(m_l1d_banknum_accesses[i] > 0){
      unique_l1d_8banks++;
    }
  }

  //do for 4 banks
  for(int i=0; i < 4; i=i+2){
    //look at first bank pair
    if( m_l1i_banknum_accesses[i] || m_l1i_banknum_accesses[i+1]){
      unique_l1i_4banks++;
    }
    //look at 2nd bank pair in second half
    if(m_l1i_banknum_accesses[i+4] || m_l1i_banknum_accesses[i+5]){
      unique_l1i_4banks++;
    }
  }
  
  //do the same for data cache
  for(int i=0; i < 4; i=i+2){
     //look at first bank pair
    if( m_l1d_banknum_accesses[i] || m_l1d_banknum_accesses[i+1]){
      unique_l1d_4banks++;
    }
    //look at 2nd bank pair in second half
    if(m_l1d_banknum_accesses[i+4] || m_l1d_banknum_accesses[i+5]){
      unique_l1d_4banks++;
    }
  }

  //do for 2 banks
  //look at 1st half
  if( m_l1i_banknum_accesses[0] || m_l1i_banknum_accesses[1] || 
          m_l1i_banknum_accesses[2] || m_l1i_banknum_accesses[3]){
      unique_l1i_2banks++;
  }

  //2nd half
   if( m_l1i_banknum_accesses[4] || m_l1i_banknum_accesses[5] || 
          m_l1i_banknum_accesses[6] || m_l1i_banknum_accesses[7]){
      unique_l1i_2banks++;
  }

  //do the same for data cache
   if( m_l1d_banknum_accesses[0] || m_l1d_banknum_accesses[1] || 
          m_l1d_banknum_accesses[2] || m_l1d_banknum_accesses[3]){
     unique_l1d_2banks++;
  }

  //2nd half
   if( m_l1d_banknum_accesses[4] || m_l1d_banknum_accesses[5] || 
          m_l1d_banknum_accesses[6] || m_l1d_banknum_accesses[7]){
      unique_l1d_2banks++;
  }

  //bin the histogram results
  ASSERT(unique_l1i_8banks < 9);
  ASSERT(unique_l1d_8banks < 9);
  ASSERT(unique_l1i_4banks < 5);
  ASSERT(unique_l1d_4banks < 5);
  ASSERT(unique_l1i_2banks < 3);
  ASSERT(unique_l1d_2banks < 3);

  m_l1i_8bank_histogram[unique_l1i_8banks]++;
  m_l1d_8bank_histogram[unique_l1d_8banks]++;
  m_l1i_4bank_histogram[unique_l1i_4banks]++;
  m_l1d_4bank_histogram[unique_l1d_4banks]++;
  m_l1i_2bank_histogram[unique_l1i_2banks]++;
  m_l1d_2bank_histogram[unique_l1d_2banks]++;

  if(WATTCH_POWER){
    bank_num_t temp_bank_num;
    temp_bank_num.num_l1i_8banks = unique_l1i_8banks;
    temp_bank_num.num_l1d_8banks = unique_l1d_8banks;
    temp_bank_num.num_l1i_4banks = unique_l1i_4banks;
    temp_bank_num.num_l1d_4banks = unique_l1d_4banks;
    temp_bank_num.num_l1i_2banks = unique_l1i_2banks;
    temp_bank_num.num_l1d_2banks = unique_l1d_2banks;
    
    m_power_stats->notifyUniqueBankNum(temp_bank_num);
  }

  //reset the bank access counters
  for(int i=0; i < 8; ++i){
    m_l1i_banknum_accesses[i] = 0;
    m_l1d_banknum_accesses[i] = 0;
  }
}

//**************************************************************************
void pseq_t::clearAccessStats(){
  if(WATTCH_POWER){
    m_power_stats->clear_access_stats();
  }
}

//**************************************************************************
void pseq_t::updatePowerStats(){
  if(WATTCH_POWER){
    //first collect our bank num stats and pass this info to WATTCH
    //  This is used to calculate clock-gating of banked L1 caches
    collectBankNumStat();
    m_power_stats->update_power_stats();
  }
}

//**************************************************************************
void pseq_t::incrementL2Access(){
  if(WATTCH_POWER){
    m_power_stats->incrementL2CacheAccess();
  }
}

//*************************************************************************
void pseq_t::incrementPrefetcherAccess(int num_prefetches, int isinstr){
  if(WATTCH_POWER && PREFETCHER_POWER){
    m_power_stats->incrementPrefetcherAccess(num_prefetches, isinstr);
  }
}

// prints the rubycache
void pseq_t::printRubyCache(){
  if(CONFIG_WITH_RUBY){
    //reset debug cycle
    debugio_t::setDebugTime( 0 );
    //print Ruby outstanding
    m_ruby_cache->print();
  }
}

/*------------------------------------------------------------------------*/
/* Global Functions                                                       */
/*------------------------------------------------------------------------*/

//**************************************************************************
static void pseq_mmu_reg_handler( void *pseq_obj, void *ptr,
                                  uint64 regindex, uint64 newcontext )
{

  #ifdef DEBUG_PSEQ
  DEBUG_OUT("pseq_t:mmu_reg_handler BEGIN\n");
  #endif

  pseq_t *pseq = (pseq_t *) pseq_obj;
  uint32  context_as_int = (uint32) newcontext;

  //We need to get the logical processor number, to pass in to contextSwitch:
  uint32 logical_proc_num = SIM_get_current_proc_no() % CONFIG_LOGICAL_PER_PHY_PROC;
  ASSERT(logical_proc_num < CONFIG_LOGICAL_PER_PHY_PROC);

  // CTXT_SWITCH
  pseq->contextSwitch( context_as_int, logical_proc_num );      

  #ifdef DEBUG_PSEQ
  DEBUG_OUT("pseq_t:mmu_reg_handler END proc[%d] \n", logical_proc_num);
  #endif
}

