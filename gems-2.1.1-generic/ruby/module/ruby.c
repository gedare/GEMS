
/*
    Copyright (C) 1999-2008 by Mark D. Hill and David A. Wood for the
    Wisconsin Multifacet Project.  Contact: gems@cs.wisc.edu
    http://www.cs.wisc.edu/gems/

    --------------------------------------------------------------------

    This file is part of the Ruby Multiprocessor Memory System Simulator, 
    a component of the Multifacet GEMS (General Execution-driven 
    Multiprocessor Simulator) software toolset originally developed at 
    the University of Wisconsin-Madison.

    Ruby was originally developed primarily by Milo Martin and Daniel
    Sorin with contributions from Ross Dickson, Carl Mauer, and Manoj
    Plakal.

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
   This file has been modified by Kevin Moore and Dan Nussbaum of the
   Scalable Systems Research Group at Sun Microsystems Laboratories
   (http://research.sun.com/scalable/) to support the Adaptive
   Transactional Memory Test Platform (ATMTP).

   Please send email to atmtp-interest@sun.com with feedback, questions, or
   to request future announcements about ATMTP.

   ----------------------------------------------------------------------

   File modification date: 2008-02-23

   ----------------------------------------------------------------------
*/

/*
 * Description: Interface between Ruby and SimICS
 * 
 * $Id$
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <strings.h>

#include "simics/api.h"
#include "simics/alloc.h"

#include "mf_api.h"
#include "commands.h"
#include "init.h"

//#include "Rock.h"

/****************************************************************
 * Hooks into C++ Ruby code
 ****************************************************************/

///provides a dispatch mechanism that catches a few commands to get variables
extern attr_value_t initvar_dispatch_get( void *id, conf_object_t *obj,
                                          attr_value_t *idx );

///provides a dispatch mechanism that catches a few commands to set variables
extern set_error_t  initvar_dispatch_set( void *id, conf_object_t *obj, 
                                          attr_value_t *val, attr_value_t *idx );

/****************************************************************/

/* struct for the ruby-class */
typedef struct {
  conf_object_t obj;
  timing_model_interface_t *timing_interface;
} ruby_object_t;

timing_model_interface_t *ruby_timing_interface;
timing_model_interface_t *ruby_observe_interface;
mf_ruby_api_t            *opal_interface;

/* VTMEMGEN specifics */
#if defined(VTMEMGEN_GENERATED)
#include "global_marks.h"
#include "module_marks.h"
#include "ruby_mark.c"
#endif

static int mm_id = -1;

#ifdef SPARC
  #define MEMORY_TRANSACTION_TYPE v9_memory_transaction_t
#else
  #define MEMORY_TRANSACTION_TYPE x86_memory_transaction_t
#endif

conf_object_t *ruby_new_instance(parse_object_t *pa) {
  ruby_object_t *obj = MM_ZALLOC(1, ruby_object_t);
  SIM_object_constructor((conf_object_t *)obj, pa);
  obj->timing_interface = ruby_timing_interface;
        
  return (conf_object_t *)obj;
}

/* The core of the memory hierarchy. This function is called once for every
   memory operation.  */
cycles_t ruby_operate(conf_object_t *obj, conf_object_t *space,
                      map_list_t *map, generic_transaction_t *g) {
  // We're currently ignoring the first 3 arguments to this function

  MEMORY_TRANSACTION_TYPE *mem_op = (MEMORY_TRANSACTION_TYPE *) g;
  
  // Pass this request off (via simics/command.C)
  return mh_memorytracer_possible_cache_miss(mem_op);
}

/* The memory observer: called after each memory transaction completes, it
 * has access to the value read or written by the memory access. This
 * observer is (unfortunately) called the 'snoop-memory' in simics 1.0
 */
cycles_t ruby_observe(conf_object_t *obj, conf_object_t *space,
                      map_list_t *map, generic_transaction_t *g) {
  // We're currently ignoring the first 3 arguments to this function

  MEMORY_TRANSACTION_TYPE *mem_op = (MEMORY_TRANSACTION_TYPE *) g;

  // Pass this request off (via simics/command.C)
  mh_memorytracer_observe_memory(mem_op);
  // always return zero stall time
  return 0;
}

//**************************************************************************
attr_value_t ruby_session_get( void *id, conf_object_t *obj,
                               attr_value_t *idx )
{
  //const char *command = (const char *) id;  
  attr_value_t ret;

  // all session attributes default to return invalid
  ret.kind = Sim_Val_Invalid;
  return ret;
}

//**************************************************************************
set_error_t ruby_session_set( void *id, conf_object_t *obj, 
                              attr_value_t *val, attr_value_t *idx )
{
  const char *command = (const char *) id;
  // Add new ruby commands to this function
  
  if (!strcmp(command, "dump-stats" ) ) {
    char* filename = (char*) val->u.string;
    if(strcmp(filename, "")){
      ruby_dump_stats(filename);
    } else {
      ruby_dump_stats(NULL);
    } 
    return Sim_Set_Ok;
  } else if (!strcmp(command, "dump-short-stats" ) ) {
    char* filename = (char*) val->u.string;
    if(strcmp(filename, "")){
      ruby_dump_short_stats(filename);
    } else {
      ruby_dump_short_stats(NULL);
    } 
    return Sim_Set_Ok;
  } else if (!strcmp(command, "periodic-stats-file" ) ) {
    char* filename = (char*) val->u.string;
    ruby_set_periodic_stats_file(filename);
    return Sim_Set_Ok;
  } else if (!strcmp(command, "periodic-stats-interval" ) ) {
    int interval = val->u.integer;
    ruby_set_periodic_stats_interval(interval);
    return Sim_Set_Ok;
  } else if (!strcmp(command, "clear-stats" ) ) {    
    ruby_clear_stats();
    return Sim_Set_Ok;
  } else if (!strcmp(command, "debug-verb" ) ) {
    char* new_verbosity = (char*) val->u.string;
    ruby_change_debug_verbosity(new_verbosity);
    return Sim_Set_Ok;
  } else if (!strcmp(command, "debug-filter" ) ) {
    char* new_debug_filter = (char*) val->u.string;
    ruby_change_debug_filter(new_debug_filter);
    return Sim_Set_Ok;
  } else if (!strcmp(command, "debug-output-file" ) ) {
    char* new_filename = (char*) val->u.string;
    ruby_set_debug_output_file(new_filename);
    return Sim_Set_Ok;
  } else if (!strcmp(command, "debug-start-time" ) ) {
    char* new_start_time = (char*) val->u.string;
    ruby_set_debug_start_time(new_start_time);
    return Sim_Set_Ok;
  } else if (!strcmp(command, "load-caches" ) ) {
    char* filename = (char*) val->u.string;
    ruby_load_caches(filename);
    return Sim_Set_Ok;
  } else if (!strcmp(command, "save-caches" ) ) {
    char* filename = (char*) val->u.string;
    ruby_save_caches(filename);
    return Sim_Set_Ok;
  } else if (!strcmp(command, "dump-cache" ) ) {
    int cpuNumber = val->u.integer;
    ruby_dump_cache(cpuNumber);
    return Sim_Set_Ok;
  } else if (!strcmp(command, "dump-cache-data" ) ) {
    int   cpuNumber = val->u.list.vector[0].u.integer;
    char *filename  = (char*) val->u.list.vector[1].u.string;
    ruby_dump_cache_data( cpuNumber, filename );
    return Sim_Set_Ok;
  } else if (!strcmp(command, "tracer-output-file" ) ) {
    char* new_filename = (char*) val->u.string;
    ruby_set_tracer_output_file(new_filename);
    return Sim_Set_Ok;
  } else if (!strcmp(command, "xact-visualizer-file" ) ) {
    char* new_filename = (char*) val->u.string;
    ruby_xact_visualizer_file(new_filename);
    return Sim_Set_Ok;
  }
  fprintf( stderr, "error: unrecognized command: %s\n", command );
  return Sim_Set_Illegal_Value;
}

//**************************************************************************
void init_local() {
  class_data_t   ruby_funcs;
  conf_class_t  *ruby_class;
  conf_object_t *ruby_obj;
  attr_value_t   val;
  conf_object_t *phys_mem0;

  /* Initialize and register the class "ruby-class". */
  bzero(&ruby_funcs, sizeof(class_data_t));
  ruby_funcs.new_instance = ruby_new_instance;
  ruby_funcs.delete_instance = NULL;
  ruby_class = SIM_register_class("ruby", &ruby_funcs);

  /* initialize the variable reader: sets all global variables to defaults */
  init_variables();

  /* Initialize and register the timing-model interface */
  ruby_timing_interface = MM_ZALLOC(1, timing_model_interface_t);
  ruby_timing_interface->operate = ruby_operate;
  SIM_register_interface(ruby_class, "timing-model", ruby_timing_interface);

  ruby_obj = SIM_new_object(ruby_class, "ruby0");

  phys_mem0 = SIM_get_object("phys_mem0");

  if(phys_mem0 == NULL) {

    /* Look for an object called "phys_mem" instead */
    SIM_clear_exception();
    phys_mem0 = SIM_get_object("phys_mem");

  }

  if(phys_mem0 == NULL) {
    /* Okay, now we can panic */

#ifndef SIMICS30
    /*
     * Must load a checkpoint BEFORE load-module ruby
     */
    printf("Please load a checkpoint BEFORE executing \"load-module ruby\"\n");
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
    printf(" > Physical Memory object cannot be found. If you are NOT compiling Ruby and <\n");
    printf(" > you see this message, something is wrong.                                 <\n");
    printf(" > This message is part of the normal compilation process.                   <\n");
    printf(" \\***************************************************************************/\033[m\n\n");
#endif
    SIM_clear_exception();
    return;
  }

  val.kind = Sim_Val_Object;
  val.u.object = ruby_obj;

  set_error_t install_error = SIM_set_attribute(phys_mem0, "timing_model", &val);

  if (install_error == Sim_Set_Ok) {
    printf( "successful installation of the ruby timing model.\n");
  } else {
    printf( "error installing ruby timing model.\n");
    exit(1);
  }

  /* Initialize the snoop interface if we are tracking values in simics */
  if (init_use_snoop() == 1) {
    ruby_observe_interface = MM_ZALLOC(1, timing_model_interface_t);
    ruby_observe_interface->operate = ruby_observe;
    SIM_register_interface(ruby_class, "snoop-memory", ruby_observe_interface);
    SIM_set_attribute(phys_mem0, "snoop_device", &val);    
  }

  /* init_opal_interface calls to a static function in OpalInterface.C
   * to determine is opal is installed. If it is, it registers itself,
   * and notifies opal that ruby is loaded. Otherwise, it does nothing. */
  opal_interface = MM_ZALLOC(1, mf_ruby_api_t);
  SIM_register_interface(ruby_class, "mf-ruby-api", opal_interface);
  init_opal_interface( opal_interface );

  // register a number of commands
#define RUBY_COMMAND( COMMAND ) \
  SIM_register_attribute( ruby_class, COMMAND,  \
                          initvar_dispatch_get, (void *) COMMAND,  \
                          initvar_dispatch_set, (void *) COMMAND,  \
                          Sim_Attr_Session,  \
                          "See documentation with associated ruby command." )

  RUBY_COMMAND( "init" );
  RUBY_COMMAND( "readparam" );
  RUBY_COMMAND( "saveparam" );
  RUBY_COMMAND( "param" );
  RUBY_COMMAND( "dump-stats" );
  RUBY_COMMAND( "dump-short-stats" );
  RUBY_COMMAND( "periodic-stats-file" );
  RUBY_COMMAND( "periodic-stats-interval" );
  RUBY_COMMAND( "clear-stats" );
  RUBY_COMMAND( "system-recovery" );
  RUBY_COMMAND( "debug-verb" );
  RUBY_COMMAND( "debug-filter" );
  RUBY_COMMAND( "debug-output-file" );
  RUBY_COMMAND( "debug-start-time" );
  RUBY_COMMAND( "set-checkpoint-interval" );
  RUBY_COMMAND( "load-caches" );
  RUBY_COMMAND( "save-caches" );
  RUBY_COMMAND( "dump-cache" );
  RUBY_COMMAND( "dump-cache-data" );
  RUBY_COMMAND( "tracer-output-file" );
  RUBY_COMMAND( "set-procs-per-chip" );
  RUBY_COMMAND( "abort-all" );
  RUBY_COMMAND( "xact-visualizer-file" );

  // Add end_transaction magic callback
  SIM_hap_add_callback("Core_Magic_Instruction", (obj_hap_func_t) magic_instruction_callback, NULL);
#ifdef SPARC
  SIM_hap_add_callback("Core_Exception", (obj_hap_func_t) ctrl_exception_start, NULL);
  SIM_hap_add_callback("Core_Exception_Return", (obj_hap_func_t) ctrl_exception_done, NULL);
  SIM_hap_add_callback("Core_Mode_Change", (obj_hap_func_t) change_mode_callback, NULL); 
  /// for MMU
  SIM_hap_add_callback("MMU_Data_TLB_Demap", (obj_hap_func_t) dtlb_demap_callback, NULL);
  SIM_hap_add_callback("MMU_Data_TLB_Map", (obj_hap_func_t) dtlb_map_callback, NULL);
  SIM_hap_add_callback("MMU_Data_TLB_Overwrite", (obj_hap_func_t) dtlb_overwrite_callback, NULL);
  SIM_hap_add_callback("MMU_Data_TLB_Replace", (obj_hap_func_t) dtlb_replace_callback, NULL);

  // Add callbacks to abort transactions on exceptions in Rock.
  //
  SIM_hap_add_callback("Core_Exception", (obj_hap_func_t) rock_exception_start, (void *) NULL);
  SIM_hap_add_callback("Core_Exception_Return", (obj_hap_func_t) rock_exception_done, (void *) NULL);
  // Add instruction decoder to install handlers for Rock-specific behavior.
  //
  decoder_t* decoder = ATMTP_create_instruction_decoder();
  SIM_register_arch_decoder(decoder, NULL, 0);
#endif

  // CM 2/2003:
  // Note: Please register other callbacks in the appropriate interface file,
  //       instead of here. This module should only register callbacks that
  //       are common to the "Driver" class (parent class to SimicsInterface
  //       and OpalInterface).
  //       If its only used by SimicsInterface, put it in that class.

}

void fini_local() {
  printf("Deleting ruby.\n");

  decoder_t* decoder = ATMTP_get_instruction_decoder();
  SIM_unregister_arch_decoder(decoder, NULL, 0);

  destroy_simulator();
  printf("Ruby memory hierarchy uninstalled.\n");
}
