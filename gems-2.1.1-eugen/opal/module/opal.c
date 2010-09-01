
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
 * FileName:  opal.C
 * Synopsis:  Loadable module implementing an out-of-order processor simulator
 * Author:    cmauer
 * Version:   $Id: opal.c 1.6 03/12/06 22:08:04-00:00 xu@cottons.cs.wisc.edu $
 */


/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <math.h>

/* -- hfa includes */
#include "hfatypes.h"
#include "opal.h"
#include "hfa_init.h"

#include "simics/alloc.h"              /* needed for "malloc()" interface */

/** stand-alone build macros */
#ifndef INIT_FUNC
#define INIT_FUNC hfa_init_local
#endif

#ifndef FINI_FUNC
#define FINI_FUNC hfa_fini_local
#endif

#ifndef MODULE_INFO
#define MODULE_INFO module_info
#endif

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

static class_data_t              hfa_class_data;
static timing_model_interface_t  hfa_timing_interface;
static timing_model_interface_t  hfa_snoop_interface;
static event_poster_interface_t  hfa_event_poster_interface;
mf_opal_api_t                    hfa_ruby_interface;
hfa_object_t                    *hfa_conf_object;

/* VTMEMGEN specifics */
#if defined(VTMEMGEN_GENERATED)
#include "global_marks.h"
#include "module_marks.h"
#include "opal_mark.c"
#endif

// VTMEMGEN memory allocation identifier
static int mm_id = -1;

/*------------------------------------------------------------------------*/
/* Variable declarations                                                  */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Forward declarations                                                   */
/*------------------------------------------------------------------------*/

///provides a dispatch mechanism that catches a few commands to get variables
extern attr_value_t initvar_dispatch_get( void *id, conf_object_t *obj,
                                          attr_value_t *idx );

///provides a dispatch mechanism that catches a few commands to set variables
extern set_error_t  initvar_dispatch_set( void *id, conf_object_t *obj, 
                                          attr_value_t *val, attr_value_t *idx );

/*------------------------------------------------------------------------*/
/* SIMICS INTERFACES                                                      */
/*------------------------------------------------------------------------*/

/**
 */
//**************************************************************************
static conf_object_t *hfa_new_instance(parse_object_t *pa)
{
  hfa_object_t *obj = MM_ZALLOC(1, hfa_object_t);
  //hfa_object_t *obj;
  
  SIM_object_constructor((conf_object_t *)obj, pa);
  obj->timing_interface       = &hfa_timing_interface;
  obj->snoop_interface        = &hfa_snoop_interface;
  obj->event_poster_interface = &hfa_event_poster_interface;
        
  return (conf_object_t *)obj;
}

/*------------------------------------------------------------------------*/
/* EVENT INTERFACES                                                       */
/*------------------------------------------------------------------------*/

//**************************************************************************
static void trace_hook( conf_object_t *obj, void *unused )
{
  //  hfa_writeTraceStep();
  static int count = 0;
  if (count++ % 10000 == 0) {
    printf(".");
    fflush(stdout);
  }

  SIM_step_post( obj, 1, trace_hook, 0 );
}

//**************************************************************************
static void
hfa_get_event(attr_value_t *ret, conf_object_t *obj, event_function_t func, void *info)
{
  ret->kind = Sim_Val_String;
  ret->u.string = "opal trace";
}

//**************************************************************************
static int
hfa_set_event(conf_object_t *obj, attr_value_t *value, event_function_t *out_func, void **out_info)
{
  *out_func = trace_hook;
  *out_info = 0;
  return 0;
}

//**************************************************************************
static const char *
hfa_describe_event(conf_object_t *obj, event_function_t func, void *info)
{
  return "opal trace event";
}

//**************************************************************************
static attr_value_t hfa_post_get( void *id, conf_object_t *obj,
                                  attr_value_t *idx )
{
  attr_value_t ret;

  printf("get value: trace hook (unused)");
  memset( &ret, 0, sizeof(ret) );
  ret.kind = Sim_Val_Integer;
  ret.u.integer = 0;
  return ret;
}

//**************************************************************************
static set_error_t hfa_post_set( void *id, conf_object_t *obj, 
                                 attr_value_t *val, attr_value_t *idx )
{
  if ( val->kind == Sim_Val_Integer ) {
    printf("hfa_post_set(): installing trace hook\n");
    SIM_step_post( obj, 0, trace_hook, 0 );
    return Sim_Set_Ok;
  }
  return Sim_Set_Illegal_Value;
}

/*
 */
//**************************************************************************
void hfa_init_local( void )
{
  attr_value_t   attr;
  conf_class_t  *conf_class;
  conf_object_t *conf_cpu;
  memset( &attr, 0, sizeof(attr) );

  hfa_checkerr("BEGIN INIT");

  if (SIM_number_processors() == 0) {
#ifdef SIMICS22X
    printf("opal: No processors defined -- load a checkpoint first!\n");
    printf("opal: Please restart simics and try again.\n");
    return;
#endif
#ifdef SIMICS30
    /*
     * This case arises because of Simics 3.0:
     * Simics 3.0 loads and "digitally signs" modules immediately after compiling them.
     * This raises havoc with Multifacet's modules, since most of them require a checkpoint
     * to be loaded BEFORE the module.
     */
    printf("\033[34;1m\n");
    printf(" /***************************************************************************\\\n");
    printf(" > Opal found no processors. If you are NOT compiling Opal and you see this  <\n");
    printf(" > message, something is wrong.                                              <\n");
    printf(" > This message is part of the normal compilation process.                   <\n");
    printf(" \\***************************************************************************/\033[m\n\n");
    SIM_clear_exception();
    return;
#endif  
  }
  
  /* Initialize and register the class "opal". */
  memset(&hfa_class_data, 0, sizeof(class_data_t));
  hfa_class_data.new_instance = hfa_new_instance;
  conf_class = SIM_register_class("opal", &hfa_class_data);

  /* Initialize and register the timing-model interface */
  hfa_timing_interface.operate = NULL;
  SIM_register_interface(conf_class, TIMING_MODEL_INTERFACE,
                         &hfa_timing_interface);

  /* Initialize and register the snoop-device interface */
  hfa_snoop_interface.operate = NULL;
  SIM_register_interface(conf_class, SNOOP_MEMORY_INTERFACE,
                         &hfa_snoop_interface);

  /* Initialize and register the event poster interface */
  hfa_event_poster_interface.get_event_info = hfa_get_event;
  hfa_event_poster_interface.set_event_info = hfa_set_event;
  hfa_event_poster_interface.describe_event = hfa_describe_event;
  SIM_register_interface(conf_class, "event-poster",
                         &hfa_event_poster_interface);

  /* Initialize and register multifacet-ruby interface */
  memset(&hfa_ruby_interface, 0, sizeof(hfa_ruby_interface));
  SIM_register_interface(conf_class, "mf-opal-api", &hfa_ruby_interface);

  /** create the hfa object */
  hfa_conf_object = (hfa_object_t *) SIM_new_object(conf_class, "opal0");

  /* register with cpu 0 event queue */
  attr.kind = Sim_Val_Object;
  conf_cpu  = SIM_next_queue(0);
  if (conf_cpu) {
    attr.u.object = conf_cpu;
    printf("Queue registration %s\n", conf_cpu->name );
    set_error_t install_error = SIM_set_attribute( (conf_object_t *) hfa_conf_object, "queue", &attr );
    if (install_error == Sim_Set_Ok) {
      printf( "successful installation of the opal queue.\n");
    } else {
      printf( "error installing opal queue.\n");
      exit(1);
    }

    
  } else {
    printf("error: unable to register queue interface\n");
  }

  /* register interfaces on the hfa0 object */

  /* event-poster interface */
  SIM_register_attribute( conf_class, "install-event-poster", 
                          hfa_post_get, (void *) "install-event-poster",
                          hfa_post_set, (void *) "install-event-poster",
                          Sim_Attr_Pseudo,
                          "Post a trace-related function to event queue" );

  // Allocate and initialize a configuration reader
  hfa_construct_initvar();
  hfa_checkerr("hfa_construct_initvar(): check");
  
  // register a number of commands
#define OPAL_COMMAND( COMMAND ) \
  SIM_register_attribute( conf_class, COMMAND,  \
                          initvar_dispatch_get, (void *) COMMAND,  \
                          initvar_dispatch_set, (void *) COMMAND,  \
                          Sim_Attr_Pseudo,  \
                          "See documentation with associated opal command." )

  /** read a configuration file */
  OPAL_COMMAND( "init" );
  OPAL_COMMAND( "readparam" );
  OPAL_COMMAND( "saveparam" );

  /** interfaces for recording a trace to disk */
  OPAL_COMMAND( "trace-start" );
  OPAL_COMMAND( "trace-stop" );
  OPAL_COMMAND( "take-trace" );
  OPAL_COMMAND( "skip-trace" );
  
  OPAL_COMMAND( "branch-trace-start" ); 
  OPAL_COMMAND( "branch-trace-stop" );
  OPAL_COMMAND( "branch-trace-take" );
  OPAL_COMMAND( "branch-trace-inf" );

  OPAL_COMMAND( "stepper" );
  OPAL_COMMAND( "param" );

  /** interfaces to run inside of simics */
  OPAL_COMMAND( "sim-flag" );
  OPAL_COMMAND( "sim-start" );
  OPAL_COMMAND( "sim-stop" );
  OPAL_COMMAND( "sim-step" );
  OPAL_COMMAND( "break_simulation" );
  OPAL_COMMAND( "sim-inorder-step" );
  OPAL_COMMAND( "sim-warmup" );
  OPAL_COMMAND( "cycle" );
  OPAL_COMMAND( "sim-stats" );
  OPAL_COMMAND( "sim-inflight" );

  OPAL_COMMAND( "sim-rd-check" );
  OPAL_COMMAND( "sim-wr-check" );
  OPAL_COMMAND( "debugtime" );
  OPAL_COMMAND( "stall" );
  OPAL_COMMAND( "togglemh" );
  OPAL_COMMAND( "install-snoop" );
  OPAL_COMMAND( "mlp-trace" );
  // ADD_SIMCOMMAND

  // check for errors
  hfa_checkerr("class regististration");

  // check that we are compatible with this version of the simulator
  hfa_simcheck();
  hfa_checkerr("hfa_simcheck(): check");
  
  printf("hfa_init_local done:\n");
  return;
}

//**************************************************************************
void hfa_fini_local( void )
{
  /* Delete commands */

  // free all global memory
  hfa_deallocate( );
  printf("high fidelity architecture code uninstalled.\n");
}

/* This function is called when simics loads the object.
   It installs the memory hierarchy routines defined above and
   the simics commands "mem-trace" and "unmem-trace".
   */
//**************************************************************************
void init_local()
{
  hfa_init_local();
}


//**************************************************************************
void fini_local()
{
  hfa_fini_local();
}
