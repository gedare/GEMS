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

#ifndef _INTERFACE_H_
#define _INTERFACE_H_

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

// Simics includes
// Defines all the Simics API calls
extern "C" {
#include "simics/api.h"
}

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

#ifdef SIMICS30
#define IS_DEV_MEM_OP(foo) (foo == Sim_Initiator_Device)
#define IS_OTH_MEM_OP(foo) (foo == Sim_Initiator_Other)
#define IS_CPU_MEM_OP(foo) (foo == Sim_Initiator_CPU)
#endif

/*------------------------------------------------------------------------*/
/* Functions                                                              */
/*------------------------------------------------------------------------*/

breakpoint_id_t      SIMICS_breakpoint(conf_object_t * obj,
                                       breakpoint_kind_t kind,
                                       access_t access,
                                       uint64 address,
                                       uint64 length,
                                       unsigned flags);
void                 SIMICS_break_simulation(const char *msg);
sim_exception_t      SIMICS_clear_exception(void);
pc_step_t            SIMICS_continue(integer_t steps);
conf_object_t*       SIMICS_current_processor(void);
cycles_t             SIMICS_cycle_count(conf_object_t *obj);
void                 SIMICS_disable_processor(conf_object_t *cpu);
tuple_int_string_t*  SIMICS_disassemble(conf_object_t *cpu,
                                        generic_address_t addr,
                                        int type);
void                 SIMICS_dump_caches(void);
void                 SIMICS_enable_processor(conf_object_t *cpu);
void                 SIMICS_for_all_memory_pages(cb_func_nI_t func, 
                                                 int i_or_d);
void                 SIMICS_for_all_processors(cb_func_nco_t func,
                                               lang_void *user_data);
void                 SIMICS_free_attribute(attr_value_t value);
attr_value_t         SIMICS_get_all_registers(conf_object_t *cpu);
attr_value_t         SIMICS_get_attribute(conf_object_t *obj, 
                                          const char *name);
attr_value_t         SIMICS_get_attribute_idx(conf_object_t * obj,
                                              const char * name,
                                              attr_value_t *index);
int                  SIMICS_get_current_proc_no(void);
const char*          SIMICS_get_exception_name(conf_object_t *cpu, 
                                               int exc);
void*                SIMICS_get_interface(const conf_object_t *obj, 
                                          const char *name);
conf_object_t*       SIMICS_get_object(const char *name);
sim_exception_t      SIMICS_get_pending_exception(void);
int                  SIMICS_get_proc_no(const conf_object_t *cpu);
logical_address_t    SIMICS_get_program_counter(conf_object_t *cpu);
int                  SIMICS_get_register_number(conf_object_t *cpu,
                                                const char *name);
const char *         SIMICS_get_register_name(conf_object_t *cpu,
                                              int reg);
hap_handle_t         SIMICS_hap_add_callback(const char *hap,
                                             obj_hap_func_t func,
                                             lang_void *user_data);
hap_handle_t         SIMICS_hap_add_callback_index(const char * hap,
                                                   obj_hap_func_t func,
                                                   lang_void *user_data,
                                                   integer_t index);
void                 SIMICS_hap_delete_callback_id(const char *hap,
                                                   hap_handle_t handle);
hap_type_t           SIMICS_hap_get_number(const char *hap);
const char*          SIMICS_last_error(void);
physical_address_t   SIMICS_logical_to_physical(conf_object_t *cpu,
                                                data_or_instr_t data_or_instr,
                                                logical_address_t vaddr);
int                  SIMICS_mem_op_is_data(const generic_transaction_t *mop);
int                  SIMICS_mem_op_is_instruction(const generic_transaction_t *mop);
int                  SIMICS_mem_op_is_read(const generic_transaction_t *mop);
int                  SIMICS_mem_op_is_write(const generic_transaction_t *mop);
int                  SIMICS_number_processors(void);
conf_object_t*       SIMICS_proc_no_2_ptr(int proc_no);
uinteger_t           SIMICS_read_phys_memory(conf_object_t *cpu,
                                             physical_address_t paddr,
                                             int length);
uinteger_t           SIMICS_read_register(conf_object_t *cpu, 
                                          int reg);
set_error_t          SIMICS_set_attribute(conf_object_t *obj,
                                          const char * name,
                                          attr_value_t *value);
void                 SIMICS_stall_cycle(conf_object_t *obj, cycles_t cycles);
pc_step_t            SIMICS_step_count(conf_object_t *obj);
void                 SIMICS_time_post_cycle(conf_object_t * obj,
                                            cycles_t cycles,
                                            sync_t sync,
                                            event_handler_t func,
                                            lang_void *user_data);
void                 SIMICS_write_register(conf_object_t *cpu,
                                           int reg,
                                           uinteger_t value);

#ifdef SIMICS22X
callback_arguments_t SIMICS_get_hap_arguments(hap_type_t hap,
                                              int use_obj);
const char*          SIMICS_get_callback_argument_string(callback_arguments_t cb);
#endif


#endif

