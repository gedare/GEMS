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
 * FileName:  system.C
 * Synopsis:  Implementation of the processor, memory model system 
 * Author:     cmauer
 * Version:    $Id$
*/

/*------------------------------------------------------------------------*/

/* Includes                                                               */
/*------------------------------------------------------------------------*/
#include "hfa.h"
#include "hfacore.h"

// getdirents headers
#ifdef USE_DIRENT
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
//#include <sys/stat.h>
//#include <fcntl.h>
#else
// sparc V9
#include <sys/types.h>
#include <sys/dir.h>
#endif
// end getdirent

// #include <sys/dirent.h>

#include "fileio.h"
#include "pseq.h"
#include "scheduler.h"         // event_t definitions
#include "transaction.h"       // transaction_t definitions
#include "confio.h"            // confio_t definitions
#include "hfa_init.h"
#include "mf_api.h"
#include "pipestate.h"
#include "rubycache.h"
#include "flow.h"
#include "sysstat.h"
#include "ptrace.h"
#include "symtrace.h"
#include "chain.h"
#include "opal.h"              // hfa_conf_object declaration
#include "exec.h"              // exec_fn_initialize() declaration
#include "system.h"

// The Simics API Wrappers (for 3.0)
#include "interface.h"

/*------------------------------------------------------------------------*/
/* Variable declarations                                                  */
/*------------------------------------------------------------------------*/

/** global system object */
system_t  *system_t::inst = NULL;

/** global ruby interface object */
extern mf_opal_api_t hfa_ruby_interface;

/** The highest number the SIM_current_proc_no() can return */
static int32 s_max_processor_id = 0;

/*------------------------------------------------------------------------*/
/* Forward declarations                                                   */
/*------------------------------------------------------------------------*/

/// used to count the number of processors
static void s_count_processors( conf_object_t *processor, void *count_i32 );

/// used as a selector in readInstructionMap
static int  selectImapFiles( const struct dirent * );

// handles transaction completion events
static void system_transaction_callback( void *system_obj, conf_object_t *cpu,
                                         uint64 immediate );

// handles breakpoints
static void system_break_handler( void *pseq_obj, uint64 access_type,
                                  uint64 break_num,
                                  void *reg_ptr, uint64 reg_size );

// reads the MB of memory used, total (using linux /var file system)
static double process_memory_total( void );
static double process_memory_resident( void );

// external function defined in ruby/tester/testFramework.h
// CM fix: should structure includes so system.C sees the same definition
//         as ruby. This is used only when ruby and opal are compiled into
//         a stand-alone tester.
extern void tester_install_opal( mf_opal_api_t  *opal_api, mf_ruby_api_t *ruby_api );

// slew of functions for handling memory transactions
void     system_post_tick( void );

cycles_t system_memory_snoop( conf_object_t *obj,
                              conf_object_t *space,
                              map_list_t *map, 
                              generic_transaction_t *g);

cycles_t system_memory_operation_stats( conf_object_t *obj,
                                        conf_object_t *space,
                                        map_list_t *map, 
                                        generic_transaction_t *g);

cycles_t system_memory_operation_trace( conf_object_t *obj,
                                        conf_object_t *space,
                                        map_list_t *map, 
                                        generic_transaction_t *g);

cycles_t system_memory_operation_mlp_trace( conf_object_t *obj,
                                            conf_object_t *space,
                                            map_list_t *map, 
                                            generic_transaction_t *g);

cycles_t system_memory_operation_warmup( conf_object_t *obj,
                                         conf_object_t *space,
                                         map_list_t *map, 
                                         generic_transaction_t *g);

cycles_t system_memory_operation_symbol_mode( conf_object_t *obj,
                                              conf_object_t *space,
                                              map_list_t *map, 
                                              generic_transaction_t *g);

static void system_exception_handler( void *obj, conf_object_t *proc,
                                      uint64 exception );

static void system_exception_tracer( void *obj, conf_object_t *proc,
                                     uint64 exception );

// C++ Template: explicit instantiation
template class map<breakpoint_id_t, breakpoint_action_t *>;

/*------------------------------------------------------------------------*/
/* Constructor(s) / destructor                                            */
/*------------------------------------------------------------------------*/

//**************************************************************************
system_t::system_t( const char *configurationFile )
{
#ifdef MODINIT_VERBOSE
  DEBUG_OUT("[ 10] system_t() constructor\n");
#endif
  hfa_checkerr("system initialization");

  if (configurationFile == NULL)
    m_configFile = NULL;
  else
    m_configFile = strdup( configurationFile );

  // establish how many processors there are in the system
  int numProcs = SIM_number_processors();   //for SMT, this is the total number of LOGICAL processors in the system
  m_numProcs   = numProcs;
  s_max_processor_id = 0;
  SIM_for_all_processors( s_count_processors, &s_max_processor_id );
  if (m_numProcs > 1)
    CONFIG_MULTIPROCESSOR = true;
  else
    CONFIG_MULTIPROCESSOR = false;

  m_mhinstalled      = false;
  m_snoop_installed  = false;
  m_sim_status = SIMSTATUS_BREAK;
  sprintf( m_sim_message_buffer, "situation nominal" );

  // establish opal ruby api (see ruby/interfaces/mf_api.h & ruby/interfaces/OpalInterface.C)
  m_opal_api = &hfa_ruby_interface;
  m_opal_api->hitCallback    = &system_t::rubyCompletedRequest;
  m_opal_api->notifyCallback = &system_t::rubyNotifyRecieve;
  m_opal_api->getInstructionCount = &system_t::rubyInstructionQuery;
  m_opal_api->getOpalTime = &system_t::rubyGetTime;
  m_opal_api->printDebug = &system_t::rubyPrintDebug;

  // These need to be implemented in OpalInterface.C
  m_opal_api->incrementL2Access = &system_t::rubyIncrementL2Access;
  m_opal_api->incrementPrefetcherAccess = &system_t::rubyIncrementPrefetcherAccess;
  m_opal_api->notifyL2Miss = &system_t::rubyL2Miss;

  queryRubyInterface();

  m_sys_stat = new sys_stat_t( m_numProcs );
  m_breakpoint_table = new BreakpointTable();
  m_symtrace = NULL;

  // create a number of sequencers ...
  // Since we moved CONFIG_LOGICAL_PER_PHY_PROC and corresponding
  //       physical register number constants to mfacet.py we need to compute
  //       safe default values (single-threaded values) as needed
  if(CONFIG_LOGICAL_PER_PHY_PROC == 0)
    CONFIG_LOGICAL_PER_PHY_PROC= 1;   
  if(CONFIG_IREG_PHYSICAL == 0)
    CONFIG_IREG_PHYSICAL= 224;
  if(CONFIG_FPREG_PHYSICAL == 0)
    CONFIG_FPREG_PHYSICAL = 192;
  if(CONFIG_CCREG_PHYSICAL == 0)
    CONFIG_CCREG_PHYSICAL = 69;
  ASSERT(CONFIG_LOGICAL_PER_PHY_PROC > 0);
  ASSERT(CONFIG_IREG_PHYSICAL >= 160);   //minimum allowed (without extra renaming registers)
  ASSERT(CONFIG_FPREG_PHYSICAL >= 64);
  ASSERT(CONFIG_CCREG_PHYSICAL >= 5);
  m_numSMTProcs = numProcs / CONFIG_LOGICAL_PER_PHY_PROC;
 #ifdef DEBUG_PSEQ
    ERROR_OUT("numProcs[%d] numSMTProcs[%d]\n",numProcs,m_numSMTProcs);
 #endif
  ASSERT(m_numSMTProcs > 0);

  m_seq   = (pseq_t **)   malloc( sizeof(pseq_t *)*m_numSMTProcs);
  m_state = (pstate_t **) malloc( sizeof(pstate_t *)*m_numSMTProcs);

  // this is to track Thread physical address
  m_proc_map = (pa_t *) malloc(sizeof(pa_t)*m_numSMTProcs);

  //For now, have separate m_chains for each logical processor:
  m_chain = (chain_t **) malloc( sizeof(chain_t *)*m_numSMTProcs );
  int id;
   for ( int i = 0, id = 0; i < m_numSMTProcs; i++, id += CONFIG_LOGICAL_PER_PHY_PROC ) {
    // new sequencer
#ifdef MODINIT_VERBOSE
    DEBUG_OUT("[400] pseq_t() construction proc=%d\n", i);
#endif
    // must create the state interface objects first!
    m_state[i] = new pstate_t( id );

  #ifdef MODINIT_VERBOSE
    DEBUG_OUT("***LUKE - creating m_seq %d\n",i);
   #endif
    m_seq[i]   = new pseq_t( id );

  #ifdef MODINIT_VERBOSE
    DEBUG_OUT("***LUKE - creating m_chain %d\n",i);
  #endif

    m_chain[i] = new chain_t( id, IWINDOW_WIN_SIZE );

    //set all thread addresses to invalid
    m_proc_map[i] = -1;
   }

  #ifdef MODINIT_VERBOSE
    DEBUG_OUT("***LUKE after the big initialization for loop\n");
  #endif

  // initalize other global objects
   m_trace = new ptrace_t( m_numProcs );  //HAVE to have numProcs (ptrace.C relies on this  many m_trace objects)
  dynamic_inst_t::initialize();
  exec_fn_initialize();
  
  // discover the thread pointer for these processors
  initThreadPointers();  

  // global debugging messages always appear
  debugio_t::setDebugTime( 0 );
  m_ruby_api = NULL;
  m_global_cycles = 0;
#ifdef MODINIT_VERBOSE
  DEBUG_OUT("[425] system_t() finished\n");
#endif
}

//***************************************************************************
system_t::~system_t( void )
{
  // free each processor
  for ( int i = 0; i < m_numSMTProcs; i++ ) {
    // delete processor, state pointer for proc #i
    if (m_seq[i])
      delete m_seq[i];
    if (m_state[i])
      delete m_state[i];
    if (m_chain[i])
    delete m_chain[i];
  }

  if (m_symtrace)
    delete m_symtrace;
  if (m_sys_stat)
    delete m_sys_stat;
  if (m_breakpoint_table)
    delete m_breakpoint_table;

  // delete the sequencer array
    free( m_seq );
    free( m_state );

    if(m_proc_map){
      free(m_proc_map);
    }
}

/*------------------------------------------------------------------------*/
/* Public methods                                                         */
/*------------------------------------------------------------------------*/

//***************************************************************************
static void
system_breakpoint( void *data, conf_object_t *cpu, integer_t parameter )
{
  if ( parameter != 4UL << 16 ) {
    // ignore all transaction completion calls
    /* sprintf( system_t::inst->m_sim_message_buffer,
             "other breakpoint (ignoring) 0x%llx", parameter );
    */
    //cout << "system_breakpoint called" << endl;
    // MAGIC breakpoints get intercepted here.  Opal currently does not do anything special here

    return;
  }
 
  //currently IGNORE the system_breakpoint (parameter = 0x4000)...
  #if 0
  ERROR_OUT("system_t::system_breakpoint REACHED param[ 0x%x ]\n", parameter);

  sprintf( system_t::inst->m_sim_message_buffer,
           "magic breakpoint reached" );
  HALT_SIMULATION;
  return;
  #endif
}

//***************************************************************************
void system_t::simulate( uint64 instrCount )
{
  //  
  // event driven simulation main loop
  //
#ifdef MODINIT_VERBOSE
  DEBUG_OUT("[1000] system_t() simulation beginning\n");
#endif
  if (CONFIG_MULTIPROCESSOR) {
    // this is multi-processor optimization: disable all processors then
    // enbable only those which should run...

    // disable all processors, so we can step one at a time
    for (int j = 0; j < m_numSMTProcs; j++ ) {
      m_state[j]->disable();
    }
  }

  if (CONFIG_IN_SIMICS) {
    // register for magic breakpoints: 4 represents magic_call_break
    hap_type_t magic_break = SIM_hap_get_number("Core_Magic_Instruction");
#ifdef SIMICS22X
    callback_arguments_t args      = SIM_get_hap_arguments( magic_break, 0 );
    const char          *paramlist = SIM_get_callback_argument_string( args );
    if (strcmp(paramlist, "nocI" )) {
      ERROR_OUT("error: system_t::installHapHandlers: expect hap to take parameters %s. Current simics executable takes: %s\n",
                "nocI", paramlist );
      SYSTEM_EXIT;
    }
#endif
    SIM_hap_add_callback( "Core_Magic_Instruction", (obj_hap_func_t) system_breakpoint, NULL );
  }

  for (int j = 0; j < m_numSMTProcs; j++ ) {
    m_seq[j]->startTime();
  }
  while ( m_sim_status == SIMSTATUS_OK &&
          instrCount >= m_seq[0]->m_stat_committed[0] ) { 
    
    for (int j = 0; j < m_numSMTProcs; j++ ) {
     #ifdef DEBUG_PSEQ
        DEBUG_OUT("system:simulate: advanceCycle BEGIN m_id[%d]\n",j);
     #endif

        /*WATTCH power */
        //clear all of the access counters before each cycle
        m_seq[j]->clearAccessStats();

      // cycle each of the processors one step
      m_seq[j]->advanceCycle();
      #ifdef DEBUG_PSEQ
        DEBUG_OUT("system:simulate: advanceCycle END m_id[%d]\n",j);
     #endif
    }
    
    // increment the global cycle count...
    m_global_cycles++;
    
    //
    // advance the ruby cache time
    //
    if (CONFIG_WITH_RUBY) {
      if ( getGlobalCycle() % RUBY_CLOCK_DIVISOR == 0 )
        m_ruby_api->advanceTime();
    }
    /* WATTCH power */
    // compute the resulting power for this cycle AFTER advancing Ruby !!
    for (int j = 0; j < m_numSMTProcs; j++ ) {
      m_seq[j]->updatePowerStats();
    }
  }

  for (int j = 0; j < m_numSMTProcs; j++ ) {
    m_seq[j]->stopTime();
  }
  for (int j = 0; j < m_numSMTProcs; j++ ) {
    for(uint k=0; k < CONFIG_LOGICAL_PER_PHY_PROC; ++k){
      // j is the SMT proc number (the sequencer index number), k is the logical proc number within the SMT chip
      m_sys_stat->observeThreadSwitch( j, k );     
     }
  }
  DEBUG_OUT("simulate: completed %lld instructions, cycle: %lld\n",
            m_seq[0]->m_stat_committed[0], m_seq[0]->getLocalCycle() );
}

//***************************************************************************
void system_t::printBuildParameters( void )
{
#ifdef FAKE_RUBY
  DEBUG_OUT( "error: macro FAKE_RUBY should never defined in opal.\n");
  SIM_HALT;
#endif

#ifdef MT_OPAL
  DEBUG_OUT( "error: macro MT_OPAL should never defined in opal.\n");
  SIM_HALT;
#endif

#ifdef EXPENSIVE_ASSERTIONS
  DEBUG_OUT( "warning: macro EXPENSIVE_ASSERTIONS defined: run time may be slow\n");
#endif

#ifdef CHECK_REGFILE
  DEBUG_OUT( "warning: macro CHECK_REGFILE defined: run time may be slow\n");
#endif
}

//***************************************************************************
void system_t::breakSimulation( void )
{
  ERROR_OUT("system_t::breakSimulation() BEGIN\n");

  HALT_SIMULATION;
  sprintf( m_sim_message_buffer, "external call to break_simulation\n");
}

//***************************************************************************
void system_t::printStats( void )
{
  for (int j = 0; j < m_numSMTProcs; j++ ) {
    m_seq[j]->printStats();
  }
  m_sys_stat->printStats();
}

//***************************************************************************
void system_t::printInflight( void )
{
  for ( int i = 0; i < m_numSMTProcs; i++ ) {
    DEBUG_OUT("SEQUENCER: %d\n", i);
    m_seq[i]->printInflight();
  }
}

//***************************************************************************
void system_t::printMemoryStats( void )
{
  out_intf_t *log = m_seq[0];
  if (log == NULL) {
    return;
  }
  dynamic_inst_t::m_myalloc.print( log );
  load_inst_t::m_myalloc.print( log );
  store_inst_t::m_myalloc.print( log );
  atomic_inst_t::m_myalloc.print( log );
  prefetch_inst_t::m_myalloc.print( log );
  control_inst_t::m_myalloc.print( log );

  ruby_request_t::m_myalloc.print( log );
  flow_inst_t::m_myalloc.print( log );

  for ( int i = 0; i < m_numSMTProcs; i++ ) {
    if (m_chain[i]->isInitialized()) {
      m_chain[i]->printMemoryStats();
    }
  }
}

//***************************************************************************
tick_t system_t::getGlobalCycle( void )
{
  return ( m_global_cycles );
}

//***************************************************************************
void   system_t::stepInorder( uint64 instrCount )
{
  DEBUG_OUT("step inorder: called: %d\n", instrCount );
  for ( uint64 k = 0; k < instrCount; k++ ) {
    for ( int i = 0; i < m_numSMTProcs; i++ ) {
      m_seq[i]->stepInorder();
    }
  }
}

//***************************************************************************
void system_t::openLogfiles( char *logname )
{
  debugio_t::openLog( logname );
  printLogHeader();

  //FOR DEBUG ONLY
  /*
  for ( int i = 0; i < m_numSMTProcs; i++ ) {
    m_state[i]->checkSparcIntf();
  }
  */

  for ( int i = 0; i < m_numSMTProcs; i++ ) {
    m_seq[i]->setLog( debugio_t::getLog() );
    m_seq[i]->installInterfaces();
    m_seq[i]->registerCheckpoint();
  }
  setupBreakpointDispatch( false );
  installHapHandlers();
  installExceptionHandler( SIMSTATUS_OK );
  m_sim_status = SIMSTATUS_OK;
}

//***************************************************************************
void system_t::closeLogfiles( void )
{
  for ( int i = 0; i < m_numSMTProcs; i++ ) {
    m_seq[i]->removeInterfaces();
  }

  removeHapHandlers();
  removeExceptionHandler();
  removeMemoryHierarchy();
  
  debugio_t::closeLog();
  m_sim_status = SIMSTATUS_BREAK;
}

/** print compile and run time information into the log file */
//**************************************************************************
void system_t::printLogHeader( void )
{
  FILE *logfp = debugio_t::getLog();

  if (logfp == NULL) {
    DEBUG_OUT("error: log file not open: not printing header to file\n");
    hfa_list_param( NULL );
    return;
  }

  if (system_t::inst->m_configFile != NULL)
    fprintf( logfp, "# Configuration File named       : %s\n", system_t::inst->m_configFile );
  fprintf( logfp, "# system status: warmup          : %d\n", system_t::inst->isWarmingUp() );
  fprintf( logfp, "# system status: simulation      : %d\n", system_t::inst->isSimulating() );
  fprintf( logfp, "# system status: trace           : %d\n", system_t::inst->isTracing() );
  
  fprintf( logfp, "# Logical processors (SMT threads) per physical processor (SMT chip):       %d\n",
           CONFIG_LOGICAL_PER_PHY_PROC);
  fprintf( logfp, "# SMT Threads to fetch from every cycle:      %d\n",CONFIG_FETCH_THREADS_PER_CYCLE); 
  fprintf( logfp, "# IWINDOW_ROB_SIZE:                %d\n", IWINDOW_ROB_SIZE);
  fprintf( logfp, "# IWINDOW_WIN_SIZE:                %d\n", IWINDOW_WIN_SIZE);
  
  fprintf( logfp, "# Cache model:\n");
  if(!CONFIG_WITH_RUBY) {
    fprintf( logfp, "# L1 data cache: block: %dB, %dx assoc, %d sets. %dB total.\n",
             1 << DL1_BLOCK_BITS, DL1_ASSOC, DL1_NUM_SETS,
             (1 << DL1_BLOCK_BITS) * DL1_ASSOC * DL1_NUM_SETS );
    fprintf( logfp, "# DL1_MSHR_ENTRIES:            %d\n", DL1_MSHR_ENTRIES);
    fprintf( logfp, "# DL1_STREAM_BUFFERS:          %d\n", DL1_STREAM_BUFFERS);
    fprintf( logfp, "# DL1_IDEAL:                   %d\n", DL1_IDEAL);
  
    fprintf( logfp, "# L1 inst cache: block: %dB, %dx assoc, %d sets. %dB total.\n",
             1 << IL1_BLOCK_BITS, IL1_ASSOC, IL1_NUM_SETS,
             (1 << IL1_BLOCK_BITS) * IL1_ASSOC * IL1_NUM_SETS );
    fprintf( logfp, "# IL1_MSHR_ENTRIES:            %d\n", IL1_MSHR_ENTRIES);
    fprintf( logfp, "# IL1_MSHR_QUEUE_SIZE:         %d\n", IL1_MSHR_QUEUE_SIZE);
    fprintf( logfp, "# IL1_MSHR_QUEUE_ISSUE_WIDTH:  %d\n", IL1_MSHR_QUEUE_ISSUE_WIDTH);
    fprintf( logfp, "# IL1_STREAM_BUFFERS:          %d\n", IL1_STREAM_BUFFERS);
    fprintf( logfp, "# IL1_IDEAL:                   %d\n", IL1_IDEAL);
  
    fprintf( logfp, "# L2 unified cache: block: %dB, %dx assoc, %d sets. %dB total.\n",
             1 << L2_BLOCK_BITS, L2_ASSOC, (1 << L2_SET_BITS),
             (1 << L2_BLOCK_BITS) * L2_ASSOC * (1 << L2_SET_BITS) );
    fprintf( logfp, "# L2_MSHR_ENTRIES:            %d\n", L2_MSHR_ENTRIES);
    fprintf( logfp, "# L2_STREAM_BUFFERS:          %d\n", L2_STREAM_BUFFERS);
    fprintf( logfp, "# L2_LATENCY:                 %d\n", L2_LATENCY);
    fprintf( logfp, "# L2_IDEAL:                   %d\n", L2_IDEAL);
    fprintf( logfp, "# MEMORY_DRAM_LATENCY:        %d\n", MEMORY_DRAM_LATENCY);
    fprintf( logfp, "# L1_FILL_BUS_CYCLES:         %d\n", L1_FILL_BUS_CYCLES);
    fprintf( logfp, "# L2_FILL_BUS_CYCLES:         %d\n", L2_FILL_BUS_CYCLES);
  } else {
    fprintf( logfp, "# RUBY_CLOCK_DIVISOR          %d\n", RUBY_CLOCK_DIVISOR );
  }
  fprintf( logfp, "\n");

  fprintf( logfp, "# Branch Predictor model:\n");  
  fprintf( logfp, "#   Predictor Type: %s\n", BRANCHPRED_TYPE);
  fprintf( logfp, "#   mispenalty    : %d\n", BRANCHPRED_MISPRED_PENALTY);
  fprintf( logfp, "\n");

  direct_predictor_t * bp = m_seq[0]->getDirectBP();
  if(bp){
    bp->printInformation(logfp);
  }
  /*
  fprintf( logfp, "#   YAGS Predictor:\n");
  fprintf( logfp, "#     PHT table         : %d entries\n", 1<<BRANCHPRED_PHT_BITS);
  fprintf( logfp, "#     Exception table   : %d entries\n", 1<<BRANCHPRED_EXCEPTION_BITS);
  fprintf( logfp, "#     Tag bits          : %d bits\n", BRANCHPRED_TAG_BITS);
  fprintf( logfp, "#     size = (2*PHT + 2*Except*(TAG+2))/8 = %d bytes\n",
           (2*(1 << BRANCHPRED_PHT_BITS) + 2 * (1 << BRANCHPRED_EXCEPTION_BITS) * (BRANCHPRED_TAG_BITS+2))/8);
  */
  fprintf( logfp, "\n" );
  
  fprintf( logfp, "# Cascaded Indirect Predictor:\n");
  fprintf( logfp, "#   simple size   : %d entries\n", 1 << CAS_TABLE_BITS );
  fprintf( logfp, "#   except size   : %d entries\n", 1 << CAS_EXCEPT_BITS );
  fprintf( logfp, "#   tag size      : %d bits\n", 1 << CAS_EXCEPT_SHIFT );
  fprintf( logfp, "\n" );

  fprintf( logfp, "# RAS size        : %d entries\n", 1 << RAS_BITS );
  fprintf( logfp, "# RAS exception   : %d entries\n", 1 << RAS_EXCEPTION_TABLE_BITS );
  fprintf( logfp, "\n");

  fprintf( logfp, "# CONFIG_IREG_PHYSICAL           : %d\n", CONFIG_IREG_PHYSICAL);
  fprintf( logfp, "# CONFIG_IREG_LOGICAL            : %d\n", CONFIG_IREG_LOGICAL);
  fprintf( logfp, "# CONFIG_CCREG_PHYSICAL          : %d\n", CONFIG_CCREG_PHYSICAL);
  fprintf( logfp, "# CONFIG_NUM_CONTROL_SETS        : %d\n", CONFIG_NUM_CONTROL_SETS);
  fprintf( logfp, "\n");

  fprintf( logfp, "# Maximum trap level (MAXTL)     : %d\n", MAXTL);
  fprintf( logfp, "# Number of windows  (NUMWINDOWS): %d\n", NWINDOWS);
  fprintf( logfp, "# Num logical integer registers (IREG_LOGICAL)   : %d\n", CONFIG_IREG_LOGICAL );
  fprintf( logfp, "# Num logical fp registers      (FPREG_LOGICAL)  : %d\n", CONFIG_FPREG_LOGICAL );
  fprintf( logfp, "# Num logical cc registers      (CCREG_LOGICAL)  : %d\n", CONFIG_FPREG_LOGICAL );

  fprintf( logfp, "# Pipeline model:\n");
  fprintf( logfp, "#    I$ latency              : %d\n", ICACHE_CYCLE );
  fprintf( logfp, "#    fetch pass cache line   : %d\n", FETCH_PASS_CACHE_LINE );
  fprintf( logfp, "#    fetch pass taken branch : %d\n", FETCH_PASS_TAKEN_BRANCH );
  fprintf( logfp, "#    fetch stages            : %d\n", FETCH_STAGES );
  fprintf( logfp, "#    decode stages           : %d\n", DECODE_STAGES );
  fprintf( logfp, "#    retire stages           : %d\n", RETIRE_STAGES );
  fprintf( logfp, "#    max fetch    / cycle: %d\n", MAX_FETCH );
  fprintf( logfp, "#    max decode   / cycle: %d\n", MAX_DECODE );
  fprintf( logfp, "#    max dispatch / cycle: %d\n", MAX_DISPATCH );
  fprintf( logfp, "#    max execute  / cycle: %d\n", MAX_EXECUTE );
  fprintf( logfp, "#    max retire   / cycle: %d\n", MAX_RETIRE );

  fprintf( logfp, "# FU TYPE             #   LATENCY\n");
  for (uint32 i = 0; i < sizeof(CONFIG_NUM_ALUS)/sizeof(char); i++ ) {
    fprintf( logfp, "# %-12s: %8d %8d\n",
             pstate_t::fu_type_menomic( (fu_type_t) i ),
             CONFIG_NUM_ALUS[i], CONFIG_ALU_LATENCY[i] );
  }

  fprintf( logfp, "#    autogenerated listing of all parameters\n");
  hfa_list_param( logfp );
}

/*------------------------------------------------------------------------*/
/* Breakpoint methods                                                     */
/*------------------------------------------------------------------------*/

//**************************************************************************
void system_t::haltSimulation( void )
{
  ERROR_OUT("HALT_SIMULATION_CYCLE[ %lld ]\n", system_t::inst->getGlobalCycle());  
  m_sim_status = SIMSTATUS_BREAK;
}

//**************************************************************************
void system_t::systemExit( void )
{
  ERROR_OUT("SYSTEM_EXIT_CYCLE[ %lld ]\n", system_t::inst->getGlobalCycle());   
  exit(1);
}

//**************************************************************************
void system_t::postedMagicBreak( void )
{
  int32 id = SIM_get_current_proc_no();
  //The actual sequencer number
  int32 seq_num = id / CONFIG_LOGICAL_PER_PHY_PROC;
  // the logical proc number within the SMT chip 
  uint32 logical_proc_num = id % CONFIG_LOGICAL_PER_PHY_PROC;
  ASSERT(seq_num >= 0 && seq_num < m_numSMTProcs);
  ASSERT(logical_proc_num < CONFIG_LOGICAL_PER_PHY_PROC);

  if (m_symtrace) {
    m_symtrace->transactionCompletes( id );   
  }

  m_sys_stat->observeTransactionComplete( seq_num, logical_proc_num );
}

//**************************************************************************
void system_t::postedBreakpoint( uint32 id, uint32 access )
{
  // search the breakpoint table for this id
  BreakpointTable::iterator iter;
  iter = m_breakpoint_table->find(id);
  if (iter == m_breakpoint_table->end()) {
    //DEBUG_OUT("Posted breakpoint: %u. Unable to find in dispatch table.\n", id);
    return;
  }
  ERROR_OUT("system_t:postedBreakpoint BP[ %d ] cycle[ %lld ]\n", id, m_global_cycles);
  breakpoint_action_t *bp = iter->second;
  breakpointf_t pmf = bp->action;
  (this->*pmf)( bp->address, (access_t) access, bp->user_data );

  //LUKE - pulled code from acquire.py to get Thread physical addresses
  if(id == 1){
    int proc_num = SIM_get_current_proc_no();
    ERROR_OUT("****ACQUIRING thread address for proc[ %d ]\n", proc_num);
    if(m_proc_map[proc_num] != -1){
      //we already found the thread address
      SIM_clear_exception();
      return;
    }
    // read the physical registers
    conf_object_t * cpu = SIM_current_processor();
    sparc_v9_interface_t * sparc_intf = (sparc_v9_interface_t*) SIM_get_interface( cpu, SPARC_V9_INTERFACE );
    int simmap = SIM_get_register_number( cpu, "cwp");
    ireg_t cwp = SIM_read_register( cpu, simmap );
    la_t va = sparc_intf->read_window_register( cpu, cwp, 25 ) + 16;
    //    la_t va  = SIM_read_window_register( cpu, cwp, 25 ) + 16;
    pa_t pa = SIM_logical_to_physical( cpu, Sim_DI_Data, va );
    //print "m_seq[%d]->setThreadPhysAddress( 0x%x );" % (proc_num, pa)
    ERROR_OUT("===========> m_seq[%d]->setThreadPhysAddress( 0x%llx)\n", proc_num, pa);
    m_proc_map[proc_num] = pa;
    bool done = true;
    for(int i=0; i < m_numSMTProcs; ++i){
      if(m_proc_map[i] == -1){
        done = false;
      }
    }
    if(done){
      ERROR_OUT("********************DONE COLLECTING THREAD ADDRESSES");
      //ASSERT(0);
    }
  }
}

//**************************************************************************
void system_t::setSimicsBreakpoint( la_t address, void *user_data )
{
  setSimicsBreakpoint( address, Sim_Break_Virtual, Sim_Access_Execute,
                       &system_t::breakpoint_print, user_data );
}

//**************************************************************************
void system_t::setSimicsBreakpoint( la_t address, breakpoint_kind_t breaktype,
                                    access_t access, breakpointf_t action_f,
                                    void *user_data )
{
  breakpoint_action_t *bp = new breakpoint_action_t();
  bp->address = address;
  bp->action  = action_f;
  bp->user_data = user_data;
  breakpoint_id_t id;
  if (breaktype == Sim_Break_Virtual) {
    id = SIM_breakpoint( SIM_get_object("primary-context"),
                         breaktype, access, address,
    // 4 == length of 32-bit intstr in bytes. 0 == always trigger breakpoint
                         4, 0 );
  } else if (breaktype == Sim_Break_Physical) {
    id = SIM_breakpoint( SIM_get_object("phys_mem0"),
                         breaktype, access, address,
    // 4 == length of 32-bit intstr in bytes. 0 == always trigger breakpoint
                         4, 0 );
    
  } else {
    ERROR_OUT("setBreakpoint: unknown break type = %d\n", breaktype);
    SIM_HALT;
  }
  ASSERT( m_breakpoint_table->find(id) == m_breakpoint_table->end() );
  (*m_breakpoint_table)[id] = bp;
}

//**************************************************************************
void system_t::breakpoint_print( uint64 address, access_t access,
                                 void *name )
{
  DEBUG_OUT( "\t0x%llx %s\n", address, (char *) name );
}

//**************************************************************************
void system_t::breakpoint_switch( uint64 address, access_t access, void *data )
{
#if 0
  // typically: resume() 0x1002c824 or 0x1002c8c4
  DEBUG_OUT("-- observing switch: PC: 0x%0llx\n",
            m_state[(int) data]->getControl( CONTROL_PC ));
#endif

  int32 seq_num = (uint64) data / CONFIG_LOGICAL_PER_PHY_PROC;
  uint32 logical_proc_num = (uint64) data % CONFIG_LOGICAL_PER_PHY_PROC;
  ASSERT(seq_num >= 0 && seq_num < m_numSMTProcs);
  ASSERT(logical_proc_num < CONFIG_LOGICAL_PER_PHY_PROC);

  m_sys_stat->observeThreadSwitch( seq_num, logical_proc_num );   
}

//**************************************************************************
void system_t::breakpoint_os_resume( uint64 address, access_t access,
                                     void *unused )
{
  // find the cpu for the current processor
  int cpuno = SIM_get_current_proc_no();

  int32 seq_num = cpuno / CONFIG_LOGICAL_PER_PHY_PROC;
  uint32 logical_proc_num = cpuno % CONFIG_LOGICAL_PER_PHY_PROC;
  ASSERT(seq_num >= 0 && seq_num < m_numSMTProcs);
  ASSERT(logical_proc_num < CONFIG_LOGICAL_PER_PHY_PROC);

  pstate_t *state = system_t::inst->m_state[seq_num];   
  int cwp         = state->getControl( CONTROL_CWP, logical_proc_num );   
  ireg_t threadid = state->getIntWp( 9, cwp, logical_proc_num );  

  // set the current process thread id
  if (m_symtrace) {
    m_symtrace->threadSwitch( cpuno, threadid );
  }
}

//**************************************************************************
void system_t::breakpoint_memcpy( uint64 address, access_t access,
                                  void *unused )
{
  // find the cpu for the current processor
  int cpuno = SIM_get_current_proc_no();

  int32 seq_num = cpuno / CONFIG_LOGICAL_PER_PHY_PROC;
  uint32 logical_proc_num = cpuno % CONFIG_LOGICAL_PER_PHY_PROC;
  ASSERT(seq_num >= 0 && seq_num < m_numSMTProcs);
  ASSERT(logical_proc_num < CONFIG_LOGICAL_PER_PHY_PROC);

  pstate_t *state = system_t::inst->m_state[seq_num];
  int cwp         = state->getControl( CONTROL_CWP, logical_proc_num );
  ireg_t o0 = state->getIntWp( 8, cwp, logical_proc_num );
  ireg_t o1 = state->getIntWp( 9, cwp, logical_proc_num );
  ireg_t o2 = state->getIntWp(10, cwp, logical_proc_num );
  
  // o0 is src
  // o1 is dest
  // o2 is size
  DEBUG_OUT( "0x%0llx 0x%0llx %lld\n", o0, o1, o2 );
}

//**************************************************************************
void system_t::setupBreakpointDispatch( bool symbolTracing )
{
  if (!CONFIG_IN_SIMICS)
    return;

  // install the core breakpoint handler
  hap_type_t break_hap = SIM_hap_get_number( "Core_Breakpoint" );

#ifdef SIMICS22X
  // verify the type safety of this call
  callback_arguments_t args      = SIM_get_hap_arguments( break_hap, 0 );
  const char          *paramlist = SIM_get_callback_argument_string( args );
  if (strcmp(paramlist, "noIIvI" )) {
    ERROR_OUT("error: system_t::setupBreakpointDispatch: expect hap to take parameters %s. Current simics executable takes: %s\n",
              "noIIvI", paramlist );
    SYSTEM_EXIT;
  }
#endif

  int rv = SIM_hap_add_callback( "Core_Breakpoint",
                                     (obj_hap_func_t) &system_break_handler,
                                     (void *) this );
  ASSERT( rv != -1 );

  if (symbolTracing) {
    // Sets a breakpoint in resume(), after the thread ID is loaded in %o1
    // How to find the hardcoded constant for other Solaris versions is
    // described in multifacet/docs/symbol-table.html.  
    setSimicsBreakpoint( 0x102a984, Sim_Break_Virtual, Sim_Access_Execute,
                         &system_t::breakpoint_os_resume, NULL );
    setSimicsBreakpoint( 0xf93884e0, Sim_Break_Physical, Sim_Access_Execute,
                         &system_t::breakpoint_memcpy, NULL );
  }
  
#if 0
  // The following examples apply to (Solaris 7) SunFire checkpoints
  setSimicsBreakpoint( 0x1002c794, Sim_Break_Virtual, Sim_Access_Execute,
                       &system_t::breakpoint_os_resume, NULL );

  // add a set of functions to break on-- dispatching them to simple functions
  setSimicsBreakpoint( 0x10034c20,
                       (void *) "mutex_enter" );
  setSimicsBreakpoint( 0x10034c80,
                       (void *) "mutex_tryenter" );
  setSimicsBreakpoint( 0x10034ca0,
                       (void *) "mutex_exit" );
  setSimicsBreakpoint( 0x10034d00,
                       (void *) "rw_enter" );
  setSimicsBreakpoint( 0x100480d4,
                       (void *) "rw_tryenter" );
  setSimicsBreakpoint( 0x10034d60,
                       (void *) "rw_exit" );
  setSimicsBreakpoint( 0x100779a0,
                       (void *) "cv_wait" );
  setSimicsBreakpoint( 0x100779f0,
                       (void *) "cv_timedwait" );
  setSimicsBreakpoint( 0x10077aac,
                       (void *) "cv_wait_sig" );
  setSimicsBreakpoint( 0x10103bb8,
                       (void *) "turnstile_block" );
  setSimicsBreakpoint( 0x7f81ad30,
                       (void *) "turnstile_block" );
  setSimicsBreakpoint( 0x10034b7c,
                       (void *) "disp_lock_enter" );
  setSimicsBreakpoint( 0x10034bc8,
                       (void *) "disp_lock_exit" );
  
  for (int32 i = 0; i < m_numSMTProcs; i++ ) {
    setSimicsBreakpoint( m_seq[i]->getThreadPhysAddress(), Sim_Break_Physical,
                         Sim_Access_Write, &system_t::breakpoint_switch,
                         (void *) i );
  }
#endif
}

/*------------------------------------------------------------------------*/
/* Tracing methods                                                        */
/*------------------------------------------------------------------------*/

//***************************************************************************
set_error_t system_t::commandSetDispatch( attr_value_t *val )
{
  // count the number of arguments in the command
  if (val->kind != Sim_Val_List) {
    ERROR_OUT("system: commandSetDispatch: non-list passed to trace subcommand.\n");
    return Sim_Set_Illegal_Value;
  }

  uint32 list_size       = val->u.list.size;
  attr_value_t  attr_fn  = val->u.list.vector[0];
  if (attr_fn.kind != Sim_Val_String) {
    ERROR_OUT("system: commandSetDispatch: non-string command\n");
    return Sim_Set_Illegal_Value;
  }
  
  const char *command = attr_fn.u.string;
  if (!strcmp( command, "start" )) {

    if (!isRubyLoaded()) {
      ERROR_OUT("system: commandSetDispatch: ruby is not loaded. unable to start timing.\n");
      return Sim_Set_Illegal_Value;
    }

    // open, initialize trace file
    if (list_size != 2) {
      ERROR_OUT("system: commandSetDispatch: need more arguments for start command (got %d)\n", list_size);
      return Sim_Set_Illegal_Value;
    }

    attr_value_t attr_filename = val->u.list.vector[1];
    if (attr_filename.kind != Sim_Val_String) {
      ERROR_OUT("system: commandSetDispatch: need string argument for start command.\n");
      return Sim_Set_Illegal_Value;
    }

    const char *filename = attr_filename.u.string;
    m_trace->writeTrace( filename );
    
    // install tracing interfaces
    system_post_tick();
    installMemoryHierarchy( SIMSTATUS_MLP_TRACE );
    installExceptionHandler( SIMSTATUS_MLP_TRACE );

  } else if (!strcmp( command, "start-depend" )) {
    if (!isRubyLoaded()) {
      ERROR_OUT("system: commandSetDispatch: ruby is not loaded. unable to start timing.\n");
      return Sim_Set_Illegal_Value;
    }
    // allocate each chain analyzer
    for (int i = 0; i < m_numSMTProcs; i++) {  
      m_chain[i]->allocate();
    }

    // connect the chain analyzer to the ptrace object
    m_trace->onlineAnalysis( chain_t::chain_inst_consumer,
                             chain_t::chain_mem_consumer );

    // install tracing interfaces
    system_post_tick();
    installMemoryHierarchy( SIMSTATUS_MLP_TRACE );
    installExceptionHandler( SIMSTATUS_MLP_TRACE );

  } else if (!strcmp( command, "start-symbol" )) {
    m_symtrace = new symtrace_t( m_numProcs );    

    // install a memory hierarchy
    //    (to build page table & monitor thread memory accesses)
    installMemoryHierarchy( SIMSTATUS_SYMBOL_MODE );

    // post a breakpoint to determine which threads are running
    setupBreakpointDispatch( true );

    // install a transaction counting metric
    installHapHandlers();
    
  } else if (!strcmp( command, "print-symbol" )) {
    if (m_symtrace) {
      m_symtrace->printStats();
    }
    
  } else if (!strncmp( command, "open-log", strlen("open-log") )) {
    // read the filenae from the command string
    int32 len = strlen("open-log");
    char  filename[FILEIO_MAX_FILENAME];

    strcpy( filename, &command[len + 1] );
    debugio_t::openLog( filename );
    printLogHeader();
    for ( int i = 0; i < m_numSMTProcs; i++ ) {
      m_seq[i]->setLog( debugio_t::getLog() );    
    }

  } else if (!strcmp( command, "print-depend" )) {
    for (int i = 0; i < m_numSMTProcs; i++) {
      m_chain[i]->printStats();                       
    }

  } else if (!strcmp( command, "print-memory" )) {
    system_t::inst->printMemoryStats();

  } else if (!strcmp( command, "start-time" )) {
    for (int i = 0; i < m_numSMTProcs; i++) {
      for(uint k=0; k <  CONFIG_LOGICAL_PER_PHY_PROC; ++k){
        m_chain[i]->startTimer(k);                      
      }
    }
  } else if (!strcmp( command, "run" )) {
    // run a trace
    DEBUG_OUT("system: commandSetDispatch: running...\n");
    
  } else if (!strcmp( command, "stop" )) {
    DEBUG_OUT("system: commandSetDispatch: stopping...\n");

    m_trace->closeTrace();
    debugio_t::closeLog();

  } else {
    ERROR_OUT("error: system: commandSetDispatch: unrecognized trace subcommand: %s.\n", command);
    return Sim_Set_Illegal_Value;
  }
  return Sim_Set_Ok;
}

//***************************************************************************
void system_t::openTrace( char *tracename )
{
  for ( int i = 0; i < m_numSMTProcs; i++ ) {
      m_seq[i]->installInterfaces();          
      for(uint k=0; k < CONFIG_LOGICAL_PER_PHY_PROC; ++k){
      m_seq[i]->openTrace( tracename, k );
    }
  }
  installHapHandlers();
  m_sim_status = SIMSTATUS_TRACING;
}

//***************************************************************************
void system_t::writeTraceStep( void )
{
  for ( int i = 0; i < m_numSMTProcs; i++ ) {
    for(uint k=0; k <  CONFIG_LOGICAL_PER_PHY_PROC; ++k){  
      m_seq[i]->writeTraceStep(k);
    }
  }
}

//***************************************************************************
void system_t::writeSkipTraceStep( void )
{
  for ( int i = 0; i < m_numSMTProcs; i++ ) {
    for(uint k=0; k <  CONFIG_LOGICAL_PER_PHY_PROC; ++k){  
      m_seq[i]->writeSkipTraceStep(k);
    }
  }
}

//***************************************************************************
void system_t::closeTrace( void )
{
  for ( int i = 0; i < m_numSMTProcs; i++ ) {
    for(uint k=0; k <  CONFIG_LOGICAL_PER_PHY_PROC; ++k){  
      m_seq[i]->closeTrace(k);
    }
      m_seq[i]->removeInterfaces();   
    // doesnt work 7/13/00
    // m_seq[i]->writeInstructionTable( NULL );
    
  }
  removeHapHandlers();
  removeMemoryHierarchy();
  m_sim_status = SIMSTATUS_BREAK;
}

/*------------------------------------------------------------------------*/
/* Branch Trace methods                                                   */
/*------------------------------------------------------------------------*/

//***************************************************************************
void system_t::openBranchTrace( char *tracename )
{
  for ( int i = 0; i < m_numSMTProcs; i++ ) {
    m_seq[i]->installInterfaces();
    for(uint k=0; k <  CONFIG_LOGICAL_PER_PHY_PROC; ++k){  
      m_seq[i]->openBranchTrace( tracename, k );
    }
  }
  installHapHandlers();
  m_sim_status = SIMSTATUS_TRACING;
}

//***************************************************************************
void system_t::writeBranchStep( void )
{
  for ( int i = 0; i < m_numSMTProcs; i++ ) {
    for(uint k=0; k <  CONFIG_LOGICAL_PER_PHY_PROC; ++k){  
      m_seq[i]->writeBranchStep(k);
    }
  }
}

//***************************************************************************
bool system_t::writeBranchNextFile( void )
{
  bool  rc = false;
  for ( int i = 0; i < m_numSMTProcs; i++ ) {
    for(uint k=0; k <  CONFIG_LOGICAL_PER_PHY_PROC; ++k){  
      rc = m_seq[i]->writeBranchNextFile(k);
    }
  }
  return (rc);
}

//***************************************************************************
void system_t::closeBranchTrace( void )
{
  for ( int i = 0; i < m_numSMTProcs; i++ ) {
    for(uint k=0; k <  CONFIG_LOGICAL_PER_PHY_PROC; ++k){  
      m_seq[i]->closeTrace(k);
    }
      m_seq[i]->removeInterfaces();   
    // doesnt work 7/13/00
    // m_seq[i]->writeInstructionTable( NULL );
    
  }
  removeHapHandlers();
  removeMemoryHierarchy();
  m_sim_status = SIMSTATUS_BREAK;
}

/** Prints the contents of a trace
 */
//***************************************************************************
void system_t::printTrace( void )
{
  for ( int i = 0; i < m_numSMTProcs; i++ ) {
    for(uint k=0; k <  CONFIG_LOGICAL_PER_PHY_PROC; ++k){  
      m_seq[i]->validateTrace(k);   
    }
  }
}

/** builds and save the instruction mappings
 */
//***************************************************************************
void system_t::saveInstructionMap( char *baseFilename, char *traceFilename )
{
  char          filename[FILEIO_MAX_FILENAME];
  abstract_pc_t apc;
  la_t          program_counter;
  pa_t          physical_pc;
  unsigned int  instr;
  int           i;
  bool          success = true;
  static_inst_t *query_instr;

  // attach the trace files to each sequencer
  for ( i = 0; i < m_numSMTProcs; i++ ) {
    for(uint k=0; k <  CONFIG_LOGICAL_PER_PHY_PROC; ++k){  
      // attach to this trace
      sprintf( filename, "%s%s", baseFilename, traceFilename );
      m_seq[i]->attachTrace( filename, false, k );                
      //  m_seq[i]->validateTrace();

      // read the trace
      while (success) {
        success = m_seq[i]->readTraceStep( program_counter,
                                         physical_pc, instr, k );
        apc.pc = program_counter;
        apc.tl = m_state[i]->getControl(CONTROL_TL, k);
      
        // add this PC, instruction to the ipage map
        if (success) {
        
          // only insert the instruction if hasn't already been inserted...
          if ( !m_seq[i]->queryInstruction( physical_pc, query_instr, k) ||
               query_instr->getInst() != instr ) {
            m_seq[i]->insertInstruction( physical_pc, instr, k );
            /* DEBUG_OUT("0x%0llx 0x%x [%d]\n", program_counter, instr, success); */
          }
        }
      }
      m_seq[i]->closeTrace(k);
      m_seq[i]->printIpage( false, k ); 
    
      sprintf( filename, "%s%s%s", baseFilename, "imap-", traceFilename);
      m_seq[i]->writeInstructionTable( filename, k );
    }   //end for loop over all logical procs
  }
}

/** builds and save the instruction mappings
 */
//***************************************************************************
void system_t::readInstructionMap( char *baseFilename, char *traceFilename,
                                   bool validate )
{  
  char          *hyphen_ptr;
  char           filename[MAXNAMLEN];

  // attach the trace files to each sequencer
  // Each sequencer should have CONFIG_LOGICAL_PER_PHY_PROC trace files (one for each SMT thread)
  for ( int i = 0; i < m_numSMTProcs; i++ ) {
    for(uint k=0; k <  CONFIG_LOGICAL_PER_PHY_PROC; ++k){

    // search the directory entry for things matching imap-*
    int   nread;
#ifdef USE_DIRENT
    DIR           *basedp = opendir( baseFilename );
    struct dirent *mydent;
    if ( basedp == NULL ) {
      ERROR_OUT("error: unable to open directory %s\n", baseFilename);
      return;
    }
    // initialize nread to a bogus value,
    //    we will break out of the loop when were done
    nread = 1000;
#elif 0
    int    fd = open( baseFilename, O_RDONLY, 0 );
    int    dircount = 100;
    struct dirent *dirbuf = (struct dirent *) malloc(dircount*sizeof(struct dirent));

    if (fd < 0) {
      ERROR_OUT("error: unable to open directory %s\n", baseFilename);
      return;
    }
    int nread = getdents( fd, dirbuf, dircount*sizeof(struct dirent) );
    if (nread < 0) {
      ERROR_OUT("error: getdents call fails! nread = %d\n", nread);
      return;
    }
    nread = nread / sizeof(struct dirent);
    for (int j = 0; j < nread; j++) {
      ERROR_OUT("%d file %s\n", j, dirbuf[j].d_name);
    }
#else
    struct dirent **namelist;
    nread = scandir( baseFilename, &namelist, selectImapFiles,
                     NULL );
    if (nread == -1) {
      ERROR_OUT("error: unable to access directory: %s\n", baseFilename);      
      return;
    }
#endif

    for (int j = 0; j < nread; j++) {

#ifdef USE_DIRENT
      mydent = readdir( basedp );
      if ( mydent == NULL ) {
        closedir( basedp );
        break;
      }
      strcpy( filename, mydent->d_name );
      if (!selectImapFiles( mydent ))
        continue;
#elif 0
      strcpy( filename, mydent->d_name );
      DEBUG_OUT("file %s\n", filename);
      if (!selectImapFiles( &dirbuf[j] ))
        continue;
      DEBUG_OUT("--selected\n");
#else     
      strcpy( filename, namelist[j]->d_name );
#endif

      hyphen_ptr = strrchr( filename, '-' );
      if (hyphen_ptr == NULL) {
        ERROR_OUT("warning: malformed imap filename: %s\n", filename);
        continue;
      }
      hyphen_ptr++;
      int context = atoi( hyphen_ptr );
      DEBUG_OUT("system_t(): readInstructionMap found context %s %d\n",
                filename, context);

      //   DEBUG_OUT("****LUKE : Filename before %s\n",filename);

      // for each pid in the system, read the instruction table
      sprintf( filename, "%s%s%s", baseFilename, "imap-", traceFilename);

      //  DEBUG_OUT("****LUKE : About to read file %s\n",filename);

      bool success = m_seq[i]->readInstructionTable( filename, context, k );   //k represents the logical proc number (0 through CONFIG_LOGICAL_PER_PHY_PROC)
      if (!success) {
        DEBUG_OUT("Error: unable to read instruction map file iter[%d]: %s\n", k, filename);
        SYSTEM_EXIT;
      }
    
    } // end j (read all instruction pages)

    if (validate) {
      bool          success = true;
      la_t          program_counter;
      pa_t          physical_pc;
      unsigned int  instr;
      static_inst_t *s_instr;
      bool          found = false;
      abstract_pc_t apc;

      // attach to this trace
      sprintf( filename, "%s%s", baseFilename, traceFilename );
      m_seq[i]->attachTrace( filename, true, k );                          
      
      // read the trace
      while (success) {
        success = m_seq[i]->readTraceStep( program_counter,
                                           physical_pc, instr, k );                      

        apc.pc = program_counter;
        apc.tl = m_state[i]->getControl(CONTROL_TL, k);                   
        
        found = m_seq[i]->queryInstruction( physical_pc, s_instr, k );      
        if (found) {
          if ( s_instr->getInst() != instr ) {
            DEBUG_OUT("warning: CONFLICT: 0x%0llx 0x%0x 0x%0x\n", 
                      program_counter, s_instr->getInst(), instr);
          }
        } else {
          DEBUG_OUT("not found: 0x%0llx 0x%0llx 0x%0x\n",
                    program_counter, physical_pc, instr);
        }
      } // end while loop
      
      m_seq[i]->closeTrace(k);                       
      
      m_seq[i]->printIpage( true, k );              
      // sprintf( filename, "%s%s", baseFilename, "imap-p1-00");
      // m_seq[i]->writeInstructionTable( filename );
    } // end if validate
    } // end for loop over all logical procs
  } // end i loop
}

/** Attach a trace-mode file
 *  Actually Ipage test currently.
 *  @param filename The name of the trace to open for reading
 */
//***************************************************************************
void system_t::attachTrace( char *baseFilename, char *traceFilename )
{
  char          filename[FILEIO_MAX_FILENAME];

  // attach the trace files to each sequencer
  for ( int i = 0; i < m_numSMTProcs; i++ ) {
    for(uint k=0; k <  CONFIG_LOGICAL_PER_PHY_PROC; ++k){
      // -- "/p/multifacet/users/cmauer/multifacet/opal/good/imap-p1-00" );

      // attach to this trace
      sprintf( filename, "%s%s", baseFilename, traceFilename);
      // CMSWAP
      m_seq[i]->attachTrace( filename, true, k );         
      // --"/p/multifacet/users/cmauer/multifacet/opal/good/trace-p1-00" );
    }
  }
}

/** attach to a memory trace file.
 */
//***************************************************************************
void system_t::attachMemTrace( char *baseFilename, char *traceFilename )
{
  char          filename[FILEIO_MAX_FILENAME];
  
  // attach the trace files to each sequencer
  for ( int i = 0; i < m_numSMTProcs; i++ ) {
    for(uint k=0; k <  CONFIG_LOGICAL_PER_PHY_PROC; ++k){
      // attach to this trace
      //DEBUG_OUT("attaching to memtrace: %s -- %s\n", baseFilename, traceFilename);
      sprintf( filename, "%s%s%s", baseFilename, "mem-", traceFilename);
      // CMSWAP
      m_seq[i]->attachMemTrace( filename, k );    
    }
  }
}

//***************************************************************************
void system_t::attachTLBTrace( char *baseFilename, char *traceFilename )
{
  char          filename[FILEIO_MAX_FILENAME];

  // attach the trace files to each sequencer
  for ( int i = 0; i < m_numSMTProcs; i++ ) {
    for(uint k=0; k <  CONFIG_LOGICAL_PER_PHY_PROC; ++k){
      // attach to a TLB map file
      //DEBUG_OUT("attaching to memtrace: %s -- %s\n", baseFilename, traceFilename);
      sprintf( filename, "%s%s%s", baseFilename, "tlb-", traceFilename);
      // CMSWAP
      m_seq[i]->attachTLBTrace( filename, k );      
    }
  }
}

//***************************************************************************
void system_t::installHapHandlers( void )
{
  if (!CONFIG_IN_SIMICS)
    return;

  hap_type_t magic_break = SIM_hap_get_number("Core_Magic_Instruction");
#ifdef SIMICS22X
  // verify the type safety of this call
  callback_arguments_t args      = SIM_get_hap_arguments( magic_break, 0 );
  const char          *paramlist = SIM_get_callback_argument_string( args );
  if (strcmp(paramlist, "nocI" )) {
    ERROR_OUT("error: system_t::installHapHandlers: expect hap to take parameters %s. Current simics executable takes: %s\n",
              "nocI", paramlist );
    SYSTEM_EXIT;
  }
#endif
  m_magic_call_hap = SIM_hap_add_callback("Core_Magic_Instruction",
                                              (obj_hap_func_t) system_transaction_callback, 
                                              (void *) this );
}

//***************************************************************************
void system_t::removeHapHandlers( void )
{
  //STOP
  if (!CONFIG_IN_SIMICS)
    return;

  SIM_hap_delete_callback_id( "Core_Magic_Instruction", m_magic_call_hap );
}

//***************************************************************************
void system_t::installMemoryHierarchy( sim_status_t status )
{
  DEBUG_OUT("installing memory hierarchy\n");
  if (CONFIG_IN_SIMICS && !m_mhinstalled) {
    /* connect the hfa object to the phys-mem object.
     * undef this call if the object is created in the configration file. */
    attr_value_t   val;
    conf_object_t *cpu_obj;
    attr_value_t   phys_attr;
    conf_object_t *phys_mem0;

    cpu_obj     = SIM_proc_no_2_ptr( 0 );
    phys_attr   = SIM_get_attribute(cpu_obj, "physical_memory");
    if (phys_attr.kind != Sim_Val_Object) {
      ERROR_OUT("error: phys_mem0 is not an object as it should be\n");
      SIM_HALT;
    }
    phys_mem0   = phys_attr.u.object;
    if (phys_mem0 == NULL) {
      ERROR_OUT("error: unable to locate object: phys_mem0\n");
      SIM_HALT;
    }
    val.kind  = Sim_Val_String;
    val.u.string = "opal0";

    // change the timing model to point at the appropriate handler
    switch (status) {
    case SIMSTATUS_OK:
      hfa_conf_object->timing_interface->operate = system_memory_operation_stats;
      break;
    case SIMSTATUS_TRACING:
      hfa_conf_object->timing_interface->operate = system_memory_operation_trace;
      break;
    case SIMSTATUS_WARMUP:
      hfa_conf_object->timing_interface->operate = system_memory_operation_warmup;
      break;
    case SIMSTATUS_MLP_TRACE:
      hfa_conf_object->timing_interface->operate = system_memory_operation_mlp_trace;
      break;
    case SIMSTATUS_SYMBOL_MODE:
      hfa_conf_object->timing_interface->operate = system_memory_operation_symbol_mode;
      break;
    default:
      ERROR_OUT("system: installMemoryHierarchy: unknown status: %d\n",
                (int) status );
      return;
    }

    SIM_set_attribute(phys_mem0, "timing_model", &val);
    hfa_checkerr("system: installMemoryHierarchy: phys_mem0");

    m_mhinstalled = true;
  }
}

//***************************************************************************
void system_t::removeMemoryHierarchy( void )
{
  DEBUG_OUT("removing memory hierarchy\n");
  if (CONFIG_IN_SIMICS && m_mhinstalled) {
    attr_value_t   val;
    conf_object_t *cpu_obj;
    conf_object_t *phys_mem0;

    cpu_obj    = SIM_proc_no_2_ptr( 0 );
    phys_mem0  = SIM_get_object(SIM_get_attribute(cpu_obj, "physical_memory").u.string);
    memset( &val, 0, sizeof(attr_value_t) );
    val.kind = Sim_Val_Invalid;  
    SIM_set_attribute(phys_mem0, "timing_model", &val);
    m_mhinstalled = false;
  }
}

//***************************************************************************
void system_t::installMemoryObserver( void )
{
  DEBUG_OUT("installing snoop device interface\n");
  if (CONFIG_IN_SIMICS && !m_snoop_installed) {

    /* connect the hfa object to the phys-mem object.
     * undef this call if the object is created in the configration file. */
    attr_value_t   val;
    conf_object_t *cpu_obj;
    attr_value_t   phys_attr;
    conf_object_t *phys_mem0;

    cpu_obj     = SIM_proc_no_2_ptr( 0 );
    phys_attr   = SIM_get_attribute(cpu_obj, "physical_memory");
    if (phys_attr.kind != Sim_Val_Object) {
      ERROR_OUT("error: phys_mem0 is not an object as it should be\n");
      SIM_HALT;
    }
    phys_mem0   = phys_attr.u.object;
    if (phys_mem0 == NULL) {
      ERROR_OUT("error: unable to locate object: phys_mem0\n");
      SIM_HALT;
    }

    val.kind  = Sim_Val_String;
    val.u.string = "opal0";
    
    // set the operate function for the snoop memory observer
    hfa_conf_object->snoop_interface->operate = system_memory_operation_stats;

    SIM_set_attribute(phys_mem0, "snoop_device", &val);
    hfa_checkerr("system: installMemoryObserver: phys_mem0");
    m_snoop_installed = true;
  }
}

//***************************************************************************
void system_t::removeMemoryObserver( void )
{
  DEBUG_OUT("removing snoop device interface\n");
  if (CONFIG_IN_SIMICS && m_snoop_installed) {
    attr_value_t   val;
    conf_object_t *cpu_obj;
    conf_object_t *phys_mem0;

    cpu_obj    = SIM_proc_no_2_ptr( 0 );
    phys_mem0  = SIM_get_object(SIM_get_attribute(cpu_obj, "physical_memory").u.string);
    memset( &val, 0, sizeof(attr_value_t) );
    val.kind = Sim_Val_Invalid;  
    SIM_set_attribute(phys_mem0, SNOOP_MEMORY_INTERFACE, &val);
    hfa_checkerr("system: removeMemoryObserver: phys_mem0");
    m_snoop_installed = false;
  }
}

//***************************************************************************
void system_t::installExceptionHandler( sim_status_t status )
{
  if (!CONFIG_IN_SIMICS)
    return;

  // install a hap handler on exceptions / interrupts
  hap_type_t simcore_exception = SIM_hap_get_number("Core_Exception");
#ifdef SIMICS22X
  callback_arguments_t args    = SIM_get_hap_arguments( simcore_exception, 0 );
  const char        *paramlist = SIM_get_callback_argument_string( args );
  if (strcmp(paramlist, "nocI" )) {
    ERROR_OUT("error: system_t::installExceptionHandler: expect hap to take parameters %s. Current simics executable takes: %s\n",
              "nocI", paramlist );
    SYSTEM_EXIT;
  }
#endif

  switch (status) {
  case SIMSTATUS_OK:
    m_exception_haphandle = SIM_hap_add_callback( "Core_Exception", 
                                        (obj_hap_func_t) &system_exception_handler,
                                                      NULL );
    break;
  case SIMSTATUS_MLP_TRACE:
    m_exception_haphandle = SIM_hap_add_callback( "Core_Exception", 
                                        (obj_hap_func_t) &system_exception_tracer,
                                                      NULL );
    break;
  default:
    DEBUG_OUT("error: install exception handler: unknown status code: %d\n",
              status);
  }
  if (m_exception_haphandle <= 0) {
    ERROR_OUT("error: installHapHandlers: exception haphandle <= 0\n");
    SYSTEM_EXIT;
  }
}

//***************************************************************************
void system_t::removeExceptionHandler( void )
{
  if (!CONFIG_IN_SIMICS)
    return;

  // remove the callback
  SIM_hap_delete_callback_id( "Core_Exception", m_exception_haphandle );
}

//***************************************************************************
void system_t::queryRubyInterface( void )
{
  // don't use system->inst in this function. it is not initialized yet!
  if (CONFIG_IN_SIMICS) {
    conf_object_t *ruby = SIM_get_object("ruby0");
    if (ruby == NULL) {
      if (SIM_number_processors() > 1) {
        ERROR_OUT("error: unable to locate object: ruby0\n");
      } else {
        DEBUG_OUT("info: using opal cache hierarchy -- ruby not found.\n");
      }
      CONFIG_WITH_RUBY = false;
      // If ruby is loaded second, clear this exception. Otherwise keep to
      // halt simulation.
      SIM_clear_exception();
#ifdef FAKE_RUBY
      DEBUG_OUT( "info: queryRubyInterface: faking ruby interface\n" );
      CONFIG_WITH_RUBY = true;
      SIM_clear_exception();
#endif
    } else {
      m_ruby_api = (mf_ruby_api_t *) SIM_get_interface( ruby, "mf-ruby-api" );
      if (m_ruby_api != NULL) {
        CONFIG_WITH_RUBY = true;
      } else {
        ERROR_OUT("error: queryRubyInterface unable to find mf-ruby-api interface.\n");
      }
    }
  } else {
    // not in simics: stand-alone mode
#ifdef UNITESTER
    // initalize the interface between ruby & opal
    m_ruby_api = new mf_ruby_api_t();
    tester_install_opal( m_opal_api, m_ruby_api );
    CONFIG_WITH_RUBY = true;
#endif
  }
}

//***************************************************************************
void system_t::removeRubyInterface( void )
{
  m_ruby_api = NULL;
  CONFIG_WITH_RUBY = false;
}

//***************************************************************************
void system_t::rubyNotifySend( int status )
{
  if (CONFIG_IN_SIMICS) {
#ifdef DEBUG_RUBY
    DEBUG_OUT("rubyNotifySend( int status )\n");
#endif
    mf_ruby_api_t *ruby_intf = system_t::inst->m_ruby_api;
    if (ruby_intf == NULL) {
      DEBUG_OUT("warning: ruby not present or it does not implement mf-ruby-api interface.\n");
    } else {
      if ( ruby_intf->notifyCallback == NULL ) {
        DEBUG_OUT("warning: notifyCallback() is NULL in mf-ruby-api interface.\n");
      } else {
        (*ruby_intf->notifyCallback)( status );
      }
    }
  }
}

//***************************************************************************
void system_t::rubyNotifyRecieve( int status )
{
#ifdef DEBUG_RUBY
  DEBUG_OUT("opalsystem:: rubyNotifyRecieve( int status ) == %d\n", status);
#endif
  if ( status == 0 ) {
    
  } else if ( status == 1 ) {
    // install notification: query ruby for its interface
    if (CONFIG_WITH_RUBY) {
      ERROR_OUT("Opal: opal-ruby link established.\n");
    }

  } else if ( status == 2 ) {
    // unload notification
    // NOTE: this is not tested, as we can't unload ruby or opal right now.
    system_t::inst->removeRubyInterface();
  }

}

//***************************************************************************
integer_t system_t::rubyInstructionQuery( int cpuNumber)
{
#ifdef DEBUG_RUBY
  DEBUG_OUT("opalsystem:: rubyInstructionQuery( ) == %d\n", cpuNumber);
#endif

  if (system_t::inst != NULL &&
      cpuNumber >= 0 &&
      cpuNumber < system_t::inst->m_numProcs ) {
    int32 seq_index = cpuNumber;
    ASSERT(seq_index >= 0 && seq_index < system_t::inst->m_numSMTProcs);
    uint64 total_committed = 0;
    //go through all logical procs and sum up the number committed for this physical processor
    for(uint proc=0; proc < CONFIG_LOGICAL_PER_PHY_PROC; ++proc){
      total_committed += system_t::inst->m_seq[seq_index]->m_stat_committed[proc];
    }
    //return (system_t::inst->m_seq[seq_index]->m_stat_committed[logical_proc_num]);
    return total_committed;
  }
  return (1);
}

//**************************************************************************
void system_t::rubyIncrementL2Access( int cpuNumber){
  /* WATTCH power */
  if(WATTCH_POWER){
    ASSERT( (cpuNumber >= 0) && (cpuNumber < system_t::inst->m_numProcs) );
    
    int32 seq_index = cpuNumber;
    ASSERT(seq_index >= 0 && seq_index < system_t::inst->m_numSMTProcs);

    system_t::inst->m_seq[seq_index]->incrementL2Access();
  }
}

//**************************************************************************
void system_t::rubyIncrementPrefetcherAccess(int cpuNumber, int num_prefetches, int isinstr){
  /* WATTCH power */
  if(WATTCH_POWER){
    ASSERT( (cpuNumber >= 0) && (cpuNumber < system_t::inst->m_numProcs) );
    
    int32 seq_index = cpuNumber;
    ASSERT(seq_index >= 0 && seq_index < system_t::inst->m_numSMTProcs);

    system_t::inst->m_seq[seq_index]->incrementPrefetcherAccess(num_prefetches, isinstr);
  }
}

//***************************************************************************
uint64 system_t::rubyGetTime( int cpuNumber ){
  int32 seq_index = cpuNumber;
  ASSERT(seq_index >= 0 && seq_index < system_t::inst->m_numSMTProcs);

  return system_t::inst->m_seq[seq_index]->getLocalCycle();
}

//***************************************************************************
void system_t::rubyCompletedRequest( int cpuNumber,
                                     pa_t physicalAddr, OpalMemop_t type, int thread )
{
#ifdef DEBUG_RUBY
  DEBUG_OUT("opalsystem:: cpu %d completed 0x%0llx\n", cpuNumber, physicalAddr );
#endif
  ASSERT( (cpuNumber >= 0) && (cpuNumber < system_t::inst->m_numProcs) );

  int32 seq_index = cpuNumber;
  ASSERT(seq_index >= 0 && seq_index < system_t::inst->m_numSMTProcs);

  system_t::inst->m_seq[seq_index]->completedRequest( physicalAddr, false /*no abort */, type, thread );
}

//***************************************************************************
// Notify sequencer of L2 miss
void system_t::rubyL2Miss(int cpuNumber, pa_t physicalAddr, OpalMemop_t type, int tagexists){
 ASSERT( (cpuNumber >= 0) && (cpuNumber < system_t::inst->m_numProcs) );

  int32 seq_index = cpuNumber;
  ASSERT(seq_index >= 0 && seq_index < system_t::inst->m_numSMTProcs);

  system_t::inst->m_seq[seq_index]->markL2Miss( physicalAddr, type, tagexists );
}

//**************************************************************************
// Prints each processor's debug info when there's a crash
void system_t::rubyPrintDebug(){
  ERROR_OUT("****SYSTEM DEBUG CALLED. Starting to print debug info numProcs[ %d ] numSMTprocs[ %d ]\n", 
      system_t::inst->m_numProcs, system_t::inst->m_numSMTProcs);
  for(int i=0; i < system_t::inst->m_numSMTProcs; ++i){
    system_t::inst->m_seq[i]->printDebug();
    ERROR_OUT("\n");
  }
}

/// write configuration info to disk
//***************************************************************************
void system_t::writeConfiguration( char *configurationName )
{
  for ( int i = 0; i < m_numSMTProcs; i++ ) {
    m_seq[i]->writeCheckpoint(configurationName );
  }
}

/// read configuration info from disk
//***************************************************************************
void system_t::readConfiguration( char *configurationName )
{
  for ( int i = 0; i < m_numSMTProcs; i++ ) {
    m_seq[i]->readCheckpoint(configurationName);
  }
}

/*------------------------------------------------------------------------*/
/* Private methods                                                        */
/*------------------------------------------------------------------------*/

//**************************************************************************
void system_t::initThreadPointers( void ) {
  // The following addresses are found using script acquire.py in opal/python
  // directory. The script is recovered from
  // /p/multifacet/projects/traces3/home/merged
  // directory, I don't really know how exactly Carl get the cpu data structure
  // base address, or breakpoint address. But, with source code and a function
  // name that you know that is modifing the thread it, it should be possible
  // to get the following addresses again for new checkpoints.
  if ( m_numProcs == 1 ) {
    ASSERT(CONFIG_LOGICAL_PER_PHY_PROC == 1);
    m_seq[0]->setThreadPhysAddress( 0x7f81ad30, 0 ); 
  } else if ( m_numProcs == 2 ) {
    la_t  taddr[2] = { 0x7f81ad30, 0x7c4f0018};
    for(int i=0; i< m_numSMTProcs; ++i){
      for(uint k=0; k < CONFIG_LOGICAL_PER_PHY_PROC; ++k){
         //logical proc k gets Thread Address taddr[k+i*CONFIG_LOGICAL_PER_PHY_PROC] 
         m_seq[i]->setThreadPhysAddress(taddr[k + i*CONFIG_LOGICAL_PER_PHY_PROC], k);
      }
    }
  } else if ( m_numProcs == 4 ) {
    la_t taddr[4] = { 0x7f81ad30, 0x7c4f0018,  0x7acfa020, 0x7aa88aa8};
    for(int i=0; i< m_numSMTProcs; ++i){
      for(uint k=0; k < CONFIG_LOGICAL_PER_PHY_PROC; ++k){
        //logical proc k gets Thread Address taddr[k+i*CONFIG_LOGICAL_PER_PHY_PROC] 
         m_seq[i]->setThreadPhysAddress(taddr[k + i*CONFIG_LOGICAL_PER_PHY_PROC], k);
      }
    }
  } else if ( m_numProcs == 8 ) {
    la_t taddr[8] = { 0x7f81ad30, 0x7c4f0018,  0x7acf6020,  0x7a9c0aa8,  
                          0x7a9b5530, 0x7a9b4030, 0x7a830ab8,  0x7b031540};
    for(int i=0; i< m_numSMTProcs; ++i){
      for(uint k=0; k < CONFIG_LOGICAL_PER_PHY_PROC; ++k){
        //logical proc k gets Thread Address taddr[k+i*CONFIG_LOGICAL_PER_PHY_PROC] 
         m_seq[i]->setThreadPhysAddress(taddr[k + i*CONFIG_LOGICAL_PER_PHY_PROC], k);
      }
    }
  } else if ( m_numProcs == 16 ) {
    la_t taddr[16] = {0x7f81ad30,  0x7c4f0018, 0x7a89b550,  0x7a89a050,
                      0x7a796ad8,  0x7a715560, 0x7a714060, 0x7a690ae8,
                      0x7ac66020,  0x7a8b6aa8, 0x7a9ad530,  0x7a9ac030,
                      0x7aa28ab8,  0x7ada1540,  0x7ada0040, 0x7a81cac8};
     for(int i=0; i< m_numSMTProcs; ++i){
      for(uint k=0; k < CONFIG_LOGICAL_PER_PHY_PROC; ++k){
        //logical proc k gets Thread Address taddr[k+i*CONFIG_LOGICAL_PER_PHY_PROC] 
         m_seq[i]->setThreadPhysAddress(taddr[k + i*CONFIG_LOGICAL_PER_PHY_PROC], k);
      }
    }
  } else {
    /* you can remove this code and continue, but some functionality will
     * not be available */
   //  DEBUG_OUT("system_t::initThreadPointers: unrecognized system configuration.\n");
//     DEBUG_OUT("        :: halting...\n");
//     SIM_HALT;
  }
}

/*------------------------------------------------------------------------*/
/* Static functions                                                       */
/*------------------------------------------------------------------------*/

/// used to count the number of processors
static void s_count_processors( conf_object_t *processor, void *count_i32 )
{
  int32 *count_p = (int32 *) count_i32;
  if ( *count_p < SIM_get_proc_no(processor) ) {
    *count_p = SIM_get_proc_no(processor);
  }
}

//**************************************************************************
static int selectImapFiles( const struct dirent *d )
{
  bool selected = !strncmp(d->d_name, "imap-", 4);
  return (selected);
}

//**************************************************************************
static void system_transaction_callback( void *system_obj, conf_object_t *cpu,
                                         uint64 immediate )
{
  system_t *system = (system_t *) system_obj;
  if ( immediate == 5UL << 16 ) {
    system->postedMagicBreak();
    //ERROR_OUT(">>>>>>>>>>>>>>>>system_transaction: info: posted  magic instruction (0x%0llx) cycle[ %lld ]\n", immediate, system_t::inst->getGlobalCycle());
    return;
  } else {
    //ERROR_OUT(">>>>>>>>>>>>>>>>system_transaction: info: unusual magic instruction (0x%0llx) cycle[ %lld ]\n", immediate, system_t::inst->getGlobalCycle());
  }
}

//**************************************************************************
static void system_break_handler( void *system_obj, uint64 access_type,
                                  uint64 break_num,
                                  void *reg_ptr, uint64 reg_size )
{
  system_t *system = (system_t *) system_obj;
  uint32 break_as_int = (uint32) break_num;
  uint32 access_as_int = (uint32) access_type;
  system->postedBreakpoint( break_as_int, access_as_int );
}

//**************************************************************************
static double process_memory_total( void )
{
  // 4kB page size, 1024*1024 bytes per MB, 
  const double MULTIPLIER = 4096.0/(1024.0*1024.0);

  ifstream proc_file;
  proc_file.open("/proc/self/statm");
  int total_size_in_pages = 0;
  int res_size_in_pages = 0;
  proc_file >> total_size_in_pages;
  proc_file >> res_size_in_pages;
  return double(total_size_in_pages)*MULTIPLIER; // size in megabytes
}

//**************************************************************************
static double process_memory_resident( void )
{
  // 4kB page size, 1024*1024 bytes per MB, 
  const double MULTIPLIER = 4096.0/(1024.0*1024.0);
  ifstream proc_file;
  proc_file.open("/proc/self/statm");
  int total_size_in_pages = 0;
  int res_size_in_pages = 0;
  proc_file >> total_size_in_pages;
  proc_file >> res_size_in_pages;
  return double(res_size_in_pages)*MULTIPLIER; // size in megabytes
}

/*------------------------------------------------------------------------*/
/* Global functions                                                       */
/*------------------------------------------------------------------------*/

//**************************************************************************
void system_mlp_trace_tick( conf_object_t* obj, lang_void* arg )
{
  system_t::inst->m_ruby_api->advanceTime();
  system_post_tick();
}

//**************************************************************************
void system_post_tick( void )
{
  const int SIMICS_MULTIPLIER = 4;

  conf_object_t* obj_ptr = (conf_object_t*) SIM_proc_no_2_ptr(0);
  SIM_time_post_cycle(obj_ptr, SIMICS_MULTIPLIER,
                      Sim_Sync_Processor, &system_mlp_trace_tick, NULL);
}

//**************************************************************************
cycles_t system_memory_snoop( conf_object_t *obj,
                              conf_object_t *space,
                              map_list_t *map, 
                              generic_transaction_t *g)
{
  memory_transaction_t* mem_op = (memory_transaction_t*) g;

  // code to handle I/O memory space transactions
  if ( IS_DEV_MEM_OP( mem_op->s.ini_type )) {
    // DEBUG_OUT( "device access: 0x%0llx\n", mem_op->s.physical_address );
    system_t::inst->m_sys_stat->observeIOAction( mem_op );
  }

  return (0);
}

//**************************************************************************
cycles_t system_memory_operation_warmup( conf_object_t *obj,
                                         conf_object_t *space,
                                         map_list_t *map, 
                                         generic_transaction_t *g)
{
  memory_transaction_t* mem_op = (memory_transaction_t*) g;

  // find the cpu for the current processor
  int cpuno = SIM_get_current_proc_no();

  // we are in "warm-up mode": pass the memory transaction
  //                           directly to the cache interface

  int32 seq_index = cpuno / CONFIG_LOGICAL_PER_PHY_PROC;
  ASSERT(seq_index >= 0 && seq_index < system_t::inst->m_numSMTProcs);

  system_t::inst->m_seq[seq_index]->warmupCache( mem_op );
  return 0;
}

//**************************************************************************
cycles_t system_memory_operation_symbol_mode( conf_object_t *obj,
                                              conf_object_t *space,
                                              map_list_t *map, 
                                              generic_transaction_t *g)
{
  memory_transaction_t* mem_op = (memory_transaction_t*) g;

  // data accesses finish immediately (and can be placed in the STC)
  if ( SIM_mem_op_is_data( &mem_op->s ) )
    return 0;

  // non CPU accesses finish immediately.
  if (!IS_CPU_MEM_OP(mem_op->s.ini_type)) {
    return 0;
  }
  
  // call out to the symbol object for I-fetches
  int cpuno = SIM_get_proc_no(mem_op->s.ini_ptr);
  system_t::inst->m_symtrace->memoryAccess( cpuno, mem_op );   
  return 0;
}

//**************************************************************************
cycles_t system_memory_operation_trace( conf_object_t *obj,
                                        conf_object_t *space,
                                        map_list_t *map, 
                                        generic_transaction_t *g)
{
  memory_transaction_t* mem_op = (memory_transaction_t*) g;

  // find the cpu for the current processor
  int cpuno = SIM_get_current_proc_no();

  // we are in "trace mode": pass the memory transaction to the trace log
  transaction_t my_trans(mem_op);

  int32 seq_index = cpuno / CONFIG_LOGICAL_PER_PHY_PROC;
  uint32 logical_proc_num =  cpuno % CONFIG_LOGICAL_PER_PHY_PROC;
  ASSERT(seq_index >= 0 && seq_index < system_t::inst->m_numSMTProcs);
  ASSERT(logical_proc_num < CONFIG_LOGICAL_PER_PHY_PROC);

  system_t::inst->m_seq[seq_index]->writeTraceMemop( &my_trans, logical_proc_num );  
  return 0;
}

//**************************************************************************
cycles_t system_memory_operation_mlp_trace( conf_object_t *obj,
                                            conf_object_t *space,
                                            map_list_t *map, 
                                            generic_transaction_t *g)
{
  memory_transaction_t* mem_op = (memory_transaction_t*) g;

  // find the cpu for the current processor
  int cpuno = SIM_get_current_proc_no();

#ifdef VERIFY_SIMICS
  // check to see if id is same
  if ( IS_CPU_MEM_OP( mem_op->s.ini_type ) ) {
    conf_object_t *originator = mem_op->s.ini_ptr;
    int check_id = SIM_get_proc_no(originator);
    if ( (check_id != -1) && (check_id != cpuno) ) {
      ERROR_OUT( "warning: originator (%d) != current (%d)\n",
                 check_id, cpuno );
    }
  }
#endif
  
  // to create a trace of a given timing execution, pass the memory op
  // to the sequencer (it will be formatted & passed to ruby)
  return ptrace_t::mlpMemopDispatch( cpuno, mem_op );  
}

//**************************************************************************
cycles_t system_memory_operation_stats( conf_object_t *obj,
                                        conf_object_t *space,
                                        map_list_t *map, 
                                        generic_transaction_t *g)
{
  memory_transaction_t* mem_op = (memory_transaction_t*) g;

  // code to handle I/O memory space transactions
  if ( IS_DEV_MEM_OP( mem_op->s.ini_type )) {
    system_t::inst->m_sys_stat->observeIOAction( mem_op );
    //DEBUG_OUT( "device access: 0x%0llx\n", mem_op->s.logical_address );
  }
  return 0;
}

//**************************************************************************
static void system_exception_handler( void *obj, conf_object_t *proc,
                                      uint64 exception )
{
  uint32  exception_as_int = (uint32) exception;
    
  // find which CPU experienced the interrupt
  int id = SIM_get_proc_no( SIM_current_processor() );

  ASSERT(id >= 0 && id < system_t::inst->m_numProcs);
  int32 seq_index = id / CONFIG_LOGICAL_PER_PHY_PROC;
  uint32 logical_proc_num =  id % CONFIG_LOGICAL_PER_PHY_PROC;
  ASSERT(seq_index >= 0 && seq_index < system_t::inst->m_numSMTProcs);
  ASSERT(logical_proc_num < CONFIG_LOGICAL_PER_PHY_PROC);

  pseq_t *pseq = system_t::inst->m_seq[seq_index];

  // post an exception to the instruction on the current processor
  pseq->postException( exception_as_int, logical_proc_num );
}

//**************************************************************************
static void system_exception_tracer( void *obj, conf_object_t *proc,
                                     uint64 exception )
{
  uint32  exception_as_int = (uint32) exception;

  // find which CPU experienced the interrupt
  int cpuno = SIM_get_proc_no(proc);
  ptrace_t::mlpExceptionDispatch( cpuno, exception_as_int );  
}
