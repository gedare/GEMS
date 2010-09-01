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
 * FileName:  hfa.C
 * Synopsis:  Loadable module implementing an out-of-order processor simulator
 * Author:    cmauer
 * Version:   $Id: hfa.C 1.51 05/05/15 18:33:44-05:00 xu@everest.cs.wisc.edu $
 */
/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include <time.h>
#include <sys/time.h>
#include <unistd.h>

/* -- hfa includes */
#include "hfa.h"
#include "hfacore.h"
#include "flow.h"              // m_myalloc initialization
#include "confio.h"            // confio_t definitions
#include "pipestate.h"
#include "rubycache.h"
#include "checkresult.h"
#include "initvar.h"

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

//***************************************************************************
// This is a kluge so that we don't use vtmemgen
void dummy_default_mark(void *p)
{
}

/*------------------------------------------------------------------------*/
/* Variable declarations                                                  */
/*------------------------------------------------------------------------*/

// pseudo-constant: are we running inside of simics
uint32 CONFIG_IN_SIMICS = false;

// pseudo-constant: are we running with ruby
uint32 CONFIG_WITH_RUBY = false;

// pseudo-constant: are we running multi-processor mode or not
uint32 CONFIG_MULTIPROCESSOR = false;

// pseudo-constant: is the host big endian or not
uint32 BIG_ENDIAN_HOST = false;

// pseudo-constant: is the target big endian or not
uint32 BIG_ENDIAN_TARGET = false;

// pseudo-constant: is does the host and target endian match
uint32 ENDIAN_MATCHES = false;

// pseudo-constant: what is the relative path to opal dir from the simics dir
const char *OPAL_RELATIVE_PATH = "../../../opal/";

// an object used for initializing the global variables
static initvar_t *g_initvar_obj = NULL;

// A generated file containing the default tester parameters in string form
// The defaults are stored in the variable global_default_param
#include "default_param.h"

/*------------------------------------------------------------------------*/
/* Forward declarations                                                   */
/*------------------------------------------------------------------------*/

// C++ Template: explicit instantiation
template class FiniteCycle<pa_t>;
template class FiniteCycle<uint32>;
template class FiniteCycle<wait_list_t>;

// CFGIndex
template class map<uint64, flow_inst_t *>;

/*------------------------------------------------------------------------*/
/* Static methods                                                         */
/*------------------------------------------------------------------------*/

//***************************************************************************
static void hfa_config_generate_values( void )
{
  /* update generated values, based on input configuration */
  MEMOP_BLOCK_MASK = (1 << DL1_BLOCK_BITS) - 1;
  IL1_NUM_SETS = (1 << IL1_SET_BITS);
  DL1_NUM_SETS = (1 << DL1_SET_BITS);

  /* RAS generated values */
  if (RAS_BITS > 16) {
    ERROR_OUT("system_t: error: ras_bits = %d. ras_pointer_t isn't big enough.\n",
              RAS_BITS);
    SIM_HALT;
  }
  
  /* indirect branch predictor values */
  CAS_TABLE_SIZE         =  1 << CAS_TABLE_BITS;
  CAS_TABLE_MASK         = (1 << CAS_TABLE_BITS) - 1;
  CAS_EXCEPT_SIZE        =  1 << CAS_EXCEPT_BITS;
  CAS_EXCEPT_MASK        = (1 << CAS_EXCEPT_BITS) - 1;
}


/** "constructor" for simics interface */
//**************************************************************************
void hfa_allocate( void )
{
  system_t::inst  = NULL;
#ifdef MODINIT_VERBOSE
  DEBUG_OUT("[  0] hfa_allocate() start\n");
#endif

#ifdef MODINIT_VERBOSE
  DEBUG_OUT("[ 10] hfa_allocate() check initalization\n");
#endif

  // determine host/target endianess
  uint32 test_value = 0x12345678;
  char * test_ptr   = (char *) &test_value;
  if ( *test_ptr == 0x12 ) {
    BIG_ENDIAN_HOST = true;
  } else if ( *test_ptr == 0x78 ) {
    BIG_ENDIAN_HOST = false;
  } else {
    ERROR_OUT( "hfa_allocate: unable to determine endianess (0x%0x)\n",
               (int) *test_ptr );
    SIM_HALT;
  }
  // should be in sparc include file (SPARC is default big endian)
  BIG_ENDIAN_TARGET = true;
  ENDIAN_MATCHES = ( BIG_ENDIAN_HOST == BIG_ENDIAN_TARGET );

  // validate that all configuration values have been initialized
  g_initvar_obj->checkInitialization();
  
  // Allocate dynamic memory allocators
  /// memory allocator for dynamic instrucions (dynamic.C)
  dynamic_inst_t::m_myalloc.init( "dynamic_inst_t",
                                  sizeof(dynamic_inst_t), 100 );
  /// memory allocator for memory instructions (memop.C)
  load_inst_t::m_myalloc.init( "load_inst_t",
                               sizeof(load_inst_t), 100 );
  store_inst_t::m_myalloc.init( "store_inst_t",
                                sizeof(store_inst_t), 100 );
  atomic_inst_t::m_myalloc.init( "atomic_inst_t",
                                 sizeof(atomic_inst_t), 100 );
  prefetch_inst_t::m_myalloc.init( "prefetch_inst_t",
                                   sizeof(prefetch_inst_t), 100 );
  /// memory allocator for branch, etc. instructions (controlop.C)
  control_inst_t::m_myalloc.init( "control_inst_t",
                                  sizeof(control_inst_t), 100 );
  ruby_request_t::m_myalloc.init( "ruby_request_t",
                                  sizeof(ruby_request_t), 100 );
  /// memory allocator for flow instrucions (flow.C)
  flow_inst_t::m_myalloc.init( "flow_inst_t",
                               sizeof(flow_inst_t), 1000 );

  // create all global objects
  system_t::inst = new system_t( g_initvar_obj->get_config_name() );

  // initialize list allocation objects
#ifdef MODINIT_VERBOSE
  DEBUG_OUT("[990] hfa_allocate() dynamic object creation\n");
#endif

  hfa_checkerr("hfa_allocate(): check");
  
#ifdef MODINIT_VERBOSE
  DEBUG_OUT("[995] hfa_allocate() ruby notification\n");
#endif

  // query ruby for its interface
  system_t::inst->queryRubyInterface();
  // notify it that opal has loaded
  system_t::inst->rubyNotifySend( 1 );

#ifdef MODINIT_VERBOSE
  DEBUG_OUT("[999] hfa_allocate() end\n");
#endif

  // print out build parameters
  system_t::inst->printBuildParameters();
}

/** "destructor" for simics interface */
//**************************************************************************
void hfa_deallocate( void )
{
  // destory any global objects
  if ( system_t::inst != NULL ) {
    delete system_t::inst;
    system_t::inst = NULL;
  }
  if ( g_initvar_obj != NULL ) {
    delete g_initvar_obj;
  }
}

//**************************************************************************
static integer_t
stepper_breakpoint(void *data, integer_t category, integer_t parameter)
{
  int *isDone = (int *) data;

  DEBUG_OUT("  breakpoint: %0lld %0lld\n", category, parameter);
  la_t curpc = SIM_get_program_counter( SIM_current_processor() );
  DEBUG_OUT("PC = 0x%0llx\n", curpc);
  if (isDone) {
    *isDone = true;
  }
  return 0;
}

/** Step the simulator */
//**************************************************************************
static void hfa_stepper( int64 n, int64 k )
{
  struct timeval start_time;
  struct timeval r_time;

  DEBUG_OUT("stepping simulator %d steps of %d\n", n, k );

  // start the timer for the read
  if (gettimeofday( &start_time, NULL ) != 0) {
    ERROR_OUT("error: unable to get real time\n");
    SYSTEM_EXIT;
  }

  for (int64 i=0; i < n; i++) {
    assert( k > 0);
    pc_step_t actual_steps = SIM_continue(k);
    assert ( actual_steps == k );
    // CM 1.4.4 no longer needs clear exception
    SIM_clear_exception();
  }
  
  if (gettimeofday( &r_time, NULL ) != 0) {
    ERROR_OUT("error: unable to get real time\n");
    SYSTEM_EXIT;
  }
  
  // print out time to step simulator n steps
  // print how long it took to simulate, and time per cycle
  if ( gettimeofday( &r_time, NULL ) != 0 ) {
    ERROR_OUT("error: unable to call gettimeofday()\n");
  }
  long  sec_expired  = r_time.tv_sec - start_time.tv_sec;
  long  usec_expired = r_time.tv_usec - start_time.tv_usec;
  if ( usec_expired < 0 ) {
    usec_expired += 1000000;
    sec_expired--;
  }
  double dsec = (double) sec_expired +
    ((double) usec_expired / (double) 1000000);
  
  DEBUG_OUT("  %-40.40s steps=%d stepsize=%d\n",
            "Total number of instructions",
            n, k);
  DEBUG_OUT("  %-40.40s sec= %lu usec= %lu\n", "Time",
            sec_expired, usec_expired );
  DEBUG_OUT("  %-40.40s %10u\n", "Total number of instructions",
            n * k );
  DEBUG_OUT("  %-40.40s %g\n", "Instruction per second:",
            (double) (n * k) / (double) dsec );
}

//**************************************************************************
static void hfa_togglemh( void )
{
  ERROR_OUT("[ togglemh: obsolete command: ignoring ]\n");
}

//**************************************************************************
static void hfa_install_snoop( void )
{
  if ( !system_t::inst->m_snoop_installed ) {
    ERROR_OUT("[ installing memory hierarchy ]\n");
    system_t::inst->installMemoryObserver();
  } else {
    ERROR_OUT("[ removing memory hierarchy ]\n");
    system_t::inst->removeMemoryObserver();
  }
}

// ADD_SIMCOMMAND
#if 0
//**************************************************************************
static void hfa_newcommand( void )
{
}
#endif

/*------------------------------------------------------------------------*/
/* TRACE FUNCTION                                                         */
/*------------------------------------------------------------------------*/

/**  Start taking a trace */
//**************************************************************************
static void hfa_start_trace( const char *tracename )
{
  system_t::inst->openTrace( (char *) tracename );
  system_t::inst->installMemoryHierarchy( system_t::SIMSTATUS_TRACING );
}

/**  Stop taking a trace */
//**************************************************************************
static void hfa_stop_trace( void )
{
  system_t::inst->closeTrace();
}

/**  post the trace hook to the event queue */
//**************************************************************************
static void hfa_writeTraceStep( void )
{
  system_t::inst->writeTraceStep();
}

/**  run a trace a number of cycles */
//**************************************************************************
static void hfa_take_trace( int64 numSteps )
{
  static int   isDone;
  static int64 totalSteps = 0;

  isDone = false;

  SIM_hap_add_callback_index( "Core_Magic_Instruction", 
                                 (obj_hap_func_t)stepper_breakpoint, &isDone, 
                              4 << 16 );


  for (int64 i = 0; i < numSteps && !isDone; i++) {
    totalSteps++;
    if (totalSteps % 1000001 == 1000000) {
      DEBUG_OUT("Trace: Total Steps   =  %d\n", totalSteps);
    }
    system_t::inst->writeTraceStep();
    pc_step_t actual_steps = SIM_continue(1);
    assert ( actual_steps == 1 );
  }
}

/**  run a trace a number of cycles */
//**************************************************************************
static void hfa_take_skip_trace( int64 numSteps )
{
  static int   isDone;
  static int64 totalSteps = 0;

  isDone = false;
  SIM_hap_add_callback_index( "Core_Magic_Instruction", 
                                 (obj_hap_func_t)stepper_breakpoint, &isDone, 4 << 16 );

  for (int64 i = 0; i < numSteps && !isDone; i += 2) {
    totalSteps += 2;
    if (totalSteps % 1000001 == 1000000) {
      DEBUG_OUT("Trace: Total Steps   =  %d\n", totalSteps);
    }
    system_t::inst->writeSkipTraceStep();
    pc_step_t actual_steps = SIM_continue(2);
    assert( actual_steps == 2 );
  }
}

/*------------------------------------------------------------------------*/
/* BRANCH TRACE FUNCTION                                                  */
/*------------------------------------------------------------------------*/

/**  Start taking a trace */
//**************************************************************************
static void hfa_start_branch_trace( const char *tracename )
{
  system_t::inst->openBranchTrace( (char *) tracename );
}

/**  Stop taking a trace */
//**************************************************************************
static void hfa_stop_branch_trace( void )
{
  system_t::inst->closeBranchTrace();
}

/**  run a trace a number of cycles */
//**************************************************************************
static void hfa_take_branch_trace_inf( void )
{
  int          isDone = false;
  static int64 totalSteps = 0;
  static int64 stepCount = 0;
  bool         success;
  SIM_hap_add_callback_index( "Core_Magic_Instruction", 
                                 (obj_hap_func_t)stepper_breakpoint, &isDone, 4 << 16 );
  while (!isDone) {
    totalSteps++;
    stepCount++;
    system_t::inst->writeBranchStep();
    pc_step_t actual_steps = SIM_continue(1);
    assert( actual_steps == 1 );
    if (stepCount > 500000000) {
      success = system_t::inst->writeBranchNextFile();
      if (success) {
        stepCount = 0;
        DEBUG_OUT("completed: %d\n", stepCount);
        DEBUG_OUT("Current cycle: %lld\n",
               SIM_step_count(SIM_current_processor()) );
      }
    }
  }
}

/**  run a trace a number of cycles */
//**************************************************************************
static void hfa_take_branch_trace( int64 numSteps )
{
  static int totalSteps = 0;

  for (int64 i = 0; i < numSteps; i++) {
    totalSteps++;
    system_t::inst->writeBranchStep();
    pc_step_t actual_steps = SIM_continue(1);
    assert ( actual_steps == 1 );
    if (totalSteps % 1000001 == 1000000) {
      DEBUG_OUT("Trace: Total Steps   =  %d\n", totalSteps);
    }
  }
}

/*------------------------------------------------------------------------*/
/* SIMULATION FUNCTION                                                    */
/*------------------------------------------------------------------------*/

/**  warmup the cache by installing the memory hierarchy */
//**************************************************************************
void hfa_sim_warmup( void )
{
  DEBUG_OUT("warning: warming up:!!!\n");
  system_t::inst->installMemoryHierarchy( system_t::SIMSTATUS_WARMUP );  
}

/**  Start simulation */
//**************************************************************************
static void hfa_sim_start( const char *simname )
{
  system_t::inst->openLogfiles( (char *) simname );
}

/**  Stop simulation */
//**************************************************************************
static void hfa_sim_stop( void )
{
  system_t::inst->closeLogfiles();
}

/**  break (interrupt) simulation */
//**************************************************************************
static void hfa_sim_break( void )
{
  system_t::inst->breakSimulation();
}

/**  simulate a number of cycles */
//*************************************************************************
static void hfa_sim_step( int64 numSteps )
{
  system_t::inst->simulate( numSteps );
}

//**************************************************************************
static void hfa_inorder_step( int64 numSteps )
{
  system_t::inst->stepInorder( numSteps );
}

/** set the debugging time */
//**************************************************************************
static void hfa_debug_time( int procid, int64 dbgtime )
{

  int32 seq_num = procid / CONFIG_LOGICAL_PER_PHY_PROC;
  ASSERT(seq_num >= 0 && seq_num < system_t::inst->m_numSMTProcs);

  out_intf_t *out_obj = system_t::inst->m_seq[seq_num];
  out_obj->setDebugTime( dbgtime );
}

/*------------------------------------------------------------------------*/
/* Opal (Non-static) methods                                              */
/*------------------------------------------------------------------------*/

/** Check for any error conditions */
//**************************************************************************
void hfa_checkerr( const char *location )
{
  int isexcept = SIM_get_pending_exception();
  if ( !(isexcept == SimExc_No_Exception || isexcept == SimExc_Break) ) {
    sim_exception_t except_code = SIM_clear_exception();
    ERROR_OUT( "Exception error message: %s\n", SIM_last_error() );
    ERROR_OUT( "%s: caught exception (#%d).\n",
               location, (int) except_code );
    return;
  }
}

/** Check that simics will work with this version of opal */
//**************************************************************************
void hfa_simcheck( void )
{
  // we simulate the simics API if we run stand-alone. The simics API
  // get_exception_name is tweaked to return a sentinel value "my exception", 
  // so we can identify if we are in simics or not.
  if (!strcmp( SIM_get_exception_name(SIM_current_processor(), 1),
               "__faked_opal_exception__" )) {
    CONFIG_IN_SIMICS = false;
  } else {
    CONFIG_IN_SIMICS = true;
  }
}

//**************************************************************************
attr_value_t hfa_dispatch_get( void *id, conf_object_t *obj,
                               attr_value_t *idx )
{
  attr_value_t ret;
  ret.kind = Sim_Val_Integer;
  ret.u.integer = 0;
  const char *attr_fn = (const char *) id;
  // obj is the hfa0 object

  if (!strcmp(attr_fn, "sim-flag")) {
    ret.kind = Sim_Val_Integer;
    ret.u.integer = system_t::inst->isSimulating();

  } else if (!strcmp(attr_fn, "sim-stats")) {
    // print the statistics
    system_t::inst->printStats();
  } else if (!strcmp(attr_fn, "sim-inflight")) {
    // print the in flight instructions
    system_t::inst->printInflight();

  } else if (!strcmp(attr_fn, "cycle")) {
    DEBUG_OUT("The current cycle is: %lld\n", system_t::inst->getGlobalCycle());
  } else {
    ERROR_OUT( "error: hfa: hfa_dispatch_get: unknown command: %s\n", attr_fn );
  }
  // ADD_SIMCOMMAND

  return ret;
}

//**************************************************************************
set_error_t hfa_dispatch_set( void *id, conf_object_t *obj, 
                              attr_value_t *val, attr_value_t *idx )
{
  const char *attr_fn = (const char *) id;
  // obj is the hfa0 object

  if (!strcmp(attr_fn, "trace-flag")) {
    ERROR_OUT("error: unable to set flag \"trace-flag\". use trs, trf instead\n");
    return Sim_Set_Illegal_Value;
  } else if (!strcmp(attr_fn, "trace-start")) {
    
    if (system_t::inst->isTracing()) {
      ERROR_OUT("error: trace is already in progress.\n");
      return Sim_Set_Illegal_Value;
    }
    // open a trace
    if (val->kind == Sim_Val_String)
      hfa_start_trace( val->u.string );
    else
      return Sim_Set_Need_String;

  } else if (!strcmp(attr_fn, "trace-stop")) {

    if (!system_t::inst->isTracing()) {
      ERROR_OUT("error: trace already finished, can't stop trace.\n");
      return Sim_Set_Illegal_Value;
    }
    // stop the trace!
    hfa_stop_trace();

  } else if (!strcmp(attr_fn, "take-trace")) {

    if (!system_t::inst->isTracing()) {
      ERROR_OUT("error: must start a trace first!\n");
      return Sim_Set_Illegal_Value;
    }
    // try to take trace
    if (val->kind == Sim_Val_Integer)
      hfa_take_trace( val->u.integer );
    
  } else if (!strcmp(attr_fn, "skip-trace")) {

    if (!system_t::inst->isTracing()) {
      ERROR_OUT("error: must start a trace first!\n");
      return Sim_Set_Illegal_Value;
    }
    // try to take trace
    if (val->kind == Sim_Val_Integer) {
      hfa_take_skip_trace( val->u.integer );
    }

  } else if (!strcmp(attr_fn, "branch-trace-start")) {
    // try to open trace
    if (val->kind == Sim_Val_String)
      hfa_start_branch_trace( val->u.string );
    else
      return Sim_Set_Need_String;

  } else if (!strcmp(attr_fn, "branch-trace-stop")) {
    // stop the trace!
    hfa_stop_branch_trace();

  } else if (!strcmp(attr_fn, "branch-trace-take")) {
    
    // try to take trace
    if (val->kind == Sim_Val_Integer)
      hfa_take_branch_trace( val->u.integer );
    
  } else if (!strcmp(attr_fn, "branch-trace-inf")) {
    
    hfa_take_branch_trace_inf();
    
  } else if (!strcmp(attr_fn, "stepper")) {
    if ( val->kind == Sim_Val_List &&
         val->u.list.size == 2 ) {
      hfa_stepper( val->u.list.vector[0].u.integer,
                   val->u.list.vector[1].u.integer );
    }
  } else if (!strcmp(attr_fn, "sim-flag")) {

    ERROR_OUT("error: unable to set flag \"sim-flag\". use start instead\n");
    return Sim_Set_Illegal_Value;

  } else if (!strcmp(attr_fn, "sim-start")) {

    if (system_t::inst->isSimulating()) {
      ERROR_OUT("error: simulation already in progress, unable to start another.\n");
      return Sim_Set_Illegal_Value;
    }
    // try to open output file
    if (val->kind == Sim_Val_String)
      hfa_sim_start( val->u.string );
    else
      return Sim_Set_Need_String;

  } else if (!strcmp(attr_fn, "sim-stop")) {
    hfa_sim_stop();

  } else if (!strcmp(attr_fn, "break_simulation")) {

    if (!system_t::inst->isSimulating()) {
      ERROR_OUT("error: must \'start\' simulation before breaking.\n");
      return Sim_Set_Illegal_Value;
    }
    hfa_sim_break();

  } else if (!strcmp(attr_fn, "sim-step")) {

    if (!system_t::inst->isSimulating()) {
      ERROR_OUT("error: must \'start\' simulation before stepping.\n");
      return Sim_Set_Illegal_Value;
    }
    if (val->kind == Sim_Val_Integer)
      hfa_sim_step( val->u.integer );

  } else if (!strcmp(attr_fn, "sim-inorder-step")) {
    if (val->kind == Sim_Val_Integer)
      hfa_inorder_step( val->u.integer );

  } else if (!strcmp(attr_fn, "sim-rd-check")) {
    if (val->kind == Sim_Val_String)
      system_t::inst->readConfiguration((char*) val->u.string );
  } else if (!strcmp(attr_fn, "sim-wr-check")) {
    if (val->kind == Sim_Val_String)
      system_t::inst->writeConfiguration((char*) val->u.string );
  } else if (!strcmp(attr_fn, "sim-warmup")) {
    hfa_sim_warmup();

  } else if (!strcmp(attr_fn, "debugtime")) {
    if ( val->kind == Sim_Val_List &&
         val->u.list.size == 2 &&
         val->u.list.vector[0].kind == Sim_Val_Integer &&
         val->u.list.vector[0].u.integer < system_t::inst->m_numProcs) {
      hfa_debug_time( val->u.list.vector[0].u.integer,
                      val->u.list.vector[1].u.integer );
    } else {
      return Sim_Set_Illegal_Value;
    }
  } else if (!strcmp(attr_fn, "togglemh")) {
    hfa_togglemh();

  } else if (!strcmp(attr_fn, "install-snoop")) {
    hfa_install_snoop();
  } else if (!strcmp(attr_fn, "stall")) {

    // stall is used to test arbitrary code...
    // this is some leftover primary context handling stuff
    system_t::inst->installHapHandlers();
    conf_object_t *mmu_conf = SIM_get_object("sfmmu0");
    if (mmu_conf == NULL) {
      ERROR_OUT("error: unable to locate object: sfmmu0\n");
      SIM_HALT;
    }
    hfa_checkerr("error: hfa: hfa_dispatch_set: getContext: get sfmmu0");
    
    attr_value_t   attr_reg = SIM_get_attribute( mmu_conf, "ctxt_primary" );
    ireg_t         pctx_reg = 0;
    if (attr_reg.kind == Sim_Val_Integer) {
      pctx_reg = attr_reg.u.integer;
    } else {
      ERROR_OUT("error: unable to read attribute on sfmmu0: ctxt_primary\n");
    }
    
    int      context  = (int) pctx_reg;
    ERROR_OUT("error: pstate_t: getContext: 0x%x\n", context);

  } else if (!strcmp(attr_fn, "mlp-trace")) {
    return system_t::inst->commandSetDispatch( val );
  } else {
    ERROR_OUT( "error: hfa: hfa_dispatch_set: unknown command: %s\n",
               attr_fn );
  }
  // ADD_SIMCOMMAND
  
  return Sim_Set_Ok;
}

//**************************************************************************
void hfa_construct_initvar( void )
{
  g_initvar_obj = new initvar_t( "opal", "../../../opal/",
                                 global_default_param,
                                 &hfa_allocate,
                                 &hfa_config_generate_values,
                                 &hfa_dispatch_get,
                                 &hfa_dispatch_set );
}

//**************************************************************************
void hfa_list_param( FILE *fp )
{
  g_initvar_obj->list_param( fp );
}
