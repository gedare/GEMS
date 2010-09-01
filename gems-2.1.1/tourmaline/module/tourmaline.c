/*
    Copyright (C) 1999-2008 by Mark D. Hill and David A. Wood for the
    Wisconsin Multifacet Project.  Contact: gems@cs.wisc.edu
    http://www.cs.wisc.edu/gems/

    --------------------------------------------------------------------

    This file is part of the Tourmaline Transactional Memory Acclerator, 
    a component of the Multifacet GEMS (General Execution-driven 
    Multiprocessor Simulator) software toolset originally developed at 
    the University of Wisconsin-Madison.

    Tourmaline was originally developed primarily by Dan Gibson, but was
    based on work in the Ruby module performed by Milo Martin and Daniel
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

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <strings.h>

#include "simics/api.h"
#include "simics/alloc.h"

#include "commands.h"
#include "init.h"

/****************************************************************
 * Hooks into Tourmaline's C++ code
 ****************************************************************/
///provides a dispatch mechanism that catches a few commands to get variables
extern attr_value_t initvar_dispatch_get( void *id, conf_object_t *obj,
                                          attr_value_t *idx );

///provides a dispatch mechanism that catches a few commands to set variables
extern set_error_t  initvar_dispatch_set( void *id, conf_object_t *obj, 
                                          attr_value_t *val, attr_value_t *idx );

/* struct for the tourmaline-class */
typedef struct {
  conf_object_t             obj;
  timing_model_interface_t *timing_interface;
} tourmaline_object_t;

timing_model_interface_t *tourmaline_timing_interface;
snoop_memory_interface_t *tourmaline_snoop_interface;

// used by the MM_ZALLOC macro
static int mm_id = -1;

conf_object_t *tourmaline_new_instance(parse_object_t *pa) {
  tourmaline_object_t *tm_obj = MM_ZALLOC(1, tourmaline_object_t);
  SIM_object_constructor((conf_object_t *) tm_obj, pa);
  tm_obj->timing_interface = tourmaline_timing_interface;
        
  return (conf_object_t *) tm_obj;
}

/* Called before a memory operation...allows stalling */
cycles_t tourmaline_operate(conf_object_t *obj, conf_object_t *space,
                            map_list_t *map, generic_transaction_t *g) {
  
  // We currently have no need of the first 3 arguments of this function

  v9_memory_transaction_t *mem_op = (v9_memory_transaction_t *) g;

  // Pass this memop off to the module proper (via simics/command.C)
  return tourmaline_operate_memory(mem_op);
  
}

/* The memory observer: called after each memory transaction completes, it
 * has access to the value read or written by the memory access. This
 * observer is (unfortunately) called the 'snoop-memory' in simics 1.0
 */
cycles_t tourmaline_observe(conf_object_t *obj, conf_object_t *space,
                            map_list_t *map, generic_transaction_t *g) {
  
  // We currently have no need of the first 3 arguments of this function

  v9_memory_transaction_t *mem_op = (v9_memory_transaction_t *) g;

  // Pass this memop off to the module proper (via simics/command.C)
  tourmaline_observe_memory(mem_op);
  
  // always return zero stall time
  return 0;
}

//**************************************************************************
attr_value_t tourmaline_session_get( void *id, conf_object_t *obj,
                                     attr_value_t *idx )
{
  //const char *command = (const char *) id;  
  attr_value_t ret;

  // all session attributes default to return invalid
  ret.kind = Sim_Val_Invalid;
  return ret;
}

//**************************************************************************
set_error_t tourmaline_session_set( void *id, conf_object_t *obj, 
                                    attr_value_t *val, attr_value_t *idx )
{
  const char *command = (const char *) id;

  // Add new commands to this function
  if(!strcmp(command,"dump-stats")) {
    char* filename = (char*) val->u.string;
    if(strcmp(filename,"")) {
      tourmaline_dump_stats(filename);
    } else {
      tourmaline_dump_stats(NULL);
    }
    return Sim_Set_Ok;
  } else if(!strcmp(command,"clear-stats")) {
    tourmaline_clear_stats();
    return Sim_Set_Ok;
  }
   
  fprintf( stderr, "error: unrecognized command: %s\n", command );
  return Sim_Set_Illegal_Value;
}

//**************************************************************************
void init_local() {
  class_data_t   tourmaline_funcs;
  conf_class_t  *tourmaline_class;
  conf_object_t *tourmaline_obj;

  /* Initialize and register the class "tourmaline-class". */
  bzero(&tourmaline_funcs, sizeof(class_data_t));
  tourmaline_funcs.new_instance = tourmaline_new_instance;
  tourmaline_funcs.delete_instance = NULL;
  tourmaline_class = SIM_register_class("tourmaline", &tourmaline_funcs);

  /* initialize the variable reader: sets all global variables to defaults */
  init_variables();

  /* Initialize and register the timing-model interface */
  tourmaline_timing_interface = MM_ZALLOC(1, timing_model_interface_t);
  tourmaline_timing_interface->operate = tourmaline_operate;
  SIM_register_interface(tourmaline_class, "timing-model", tourmaline_timing_interface);

  tourmaline_obj = SIM_new_object(tourmaline_class, "tourmaline0");

  tourmaline_snoop_interface = MM_ZALLOC(1, snoop_memory_interface_t);
  tourmaline_snoop_interface->operate = tourmaline_observe;
  SIM_register_interface(tourmaline_class, "snoop-memory", tourmaline_snoop_interface);
  
  // The timing model interface & snoop interface are now ready to install
  // To do so, call:
  // SIM_set_attrtribute(phys_mem0, "timing_model", &val);
  // SIM_set_attrtribute(phys_mem0, "snoop_device", &val);
  // via SIMICS_ interface (interface.[Ch])
  
  // register a number of commands
#define TOURMALINE_COMMAND( COMMAND ) \
  SIM_register_attribute( tourmaline_class, COMMAND,  \
                          initvar_dispatch_get, (void *) COMMAND,  \
                          initvar_dispatch_set, (void *) COMMAND,  \
                          Sim_Attr_Session,  \
                          "See documentation with associated tourmaline command." )

  TOURMALINE_COMMAND( "init" );
  TOURMALINE_COMMAND( "param" );
  TOURMALINE_COMMAND( "clear-stats" );
  TOURMALINE_COMMAND( "dump-stats" );
  
  // Add magic callback
  SIM_hap_add_callback("Core_Magic_Instruction", (obj_hap_func_t) magic_instruction_callback, NULL);

  printf( "Successful installation of the Tourmaline TM-Acceleration module.\n");
  
}

void fini_local() {
  destroy_simulator();
  printf( "Tourmaline uninstalled.\n");
}
