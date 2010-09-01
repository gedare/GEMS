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
 * FileName:  simstate.cpp
 * Synopsis:  a simulated simics environment for debugging
 * Author:    cmauer
 * Version:   $Id: simdist12.C 1.24 05/05/16 10:10:54-05:00 mikem@s0-32.cs.wisc.edu $
 */

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include "hfa.h"

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

#define  UNIMPL      \
  DEBUG_OUT("simdist12: warning: unimplemented simics API %s:%d\n", __FILE__, __LINE__);

/*------------------------------------------------------------------------*/
/* Variable declarations                                                  */
/*------------------------------------------------------------------------*/


/* processor.c */
unsigned number_processors;
unsigned total_number_processors;
 
/// the last program counter
la_t     sim_last_pc;
/// the last instruction executed
unsigned sim_last_instr;

/*------------------------------------------------------------------------*/
/* Forward declarations                                                   */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Constructor(s) / destructor                                            */
/*------------------------------------------------------------------------*/

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

/* BEGIN "C" LINKAGE */
extern "C" {

/** module_info.h */
void module_install(const char *mod_name, int (*mod_info)(void *),
                    void (*init_local)(void),
                    const char *conf_classes[]) {
  DEBUG_OUT("installing module: %s\n", mod_name);
}

void module_uninstall(const char *mod_name, void (*fini_local)(void)) {
  DEBUG_OUT("uninstalling module: %s\n", mod_name);
}

/** WRITE / PRINTF */
int SIM_write(const void *ptr, int size)
{
#undef fwrite
  return (fwrite( ptr, size, (size_t) 1, stdout ));
}

int SIM_printf( const char *fmt, ...)
{
  int count = 0;
  va_list  args;
  va_start(args, fmt);
  count += vfprintf(stdout, fmt, args);
  va_end(args);
  return count;
}

void mm_free(void * ptr, const char * src_file, int src_line) {
#undef free
  free(ptr);
}

uinteger_t SIM_get_mem_op_value_be(generic_transaction_t *mem_op) {
  // UNIMPL;
  return (0);
}

uinteger_t SIM_get_mem_op_value_le(generic_transaction_t *mem_op) 
{
  // UNIMPL;
  return (0);
}

void mm_mark_ptr_dbg(void ** pptr, const char * src_file, int src_line) {
  UNIMPL;
}

void mm_register_type(void (*mf)(void *), const char *type_name)
{
  UNIMPL;
}
void mm_unregister_type(void (*mf)(void *), const char *type_name)
{
  UNIMPL;
}
/* memory_api.h */
void   SIM_memory_add_watchpoint(pa_t addr, int read, int write, int execute, pa_t length) { UNIMPL; }

void SIM_for_all_memory_pages(cb_func_nI_t func, int d_or_i) { UNIMPL; }

void   SIM_memory_clear_watchpoint(pa_t addr, int read, int write, int execute, pa_t length) { UNIMPL; }
uinteger_t SIM_read_phys_memory(processor_t *cpu, physical_address_t address, int len) {
  // UNIMPL;
  return (0);
}
  void SIM_write_phys_memory(processor_t *cpu, physical_address_t address, uinteger_t value, int len) {
    UNIMPL;
  }
physical_address_t SIM_logical_to_physical(processor_t *cpu_ptr, data_or_instr_t data_or_instr, logical_address_t address) {
  // UNIMPL;
  return (0);
}

tuple_int_string_t *SIM_disassemble(processor_t *cpu_ptr, generic_address_t address, int type) {
  return (NULL);
}
#undef SIM_get_exception_name
const char  *SIM_get_exception_name(conf_object_t *cpu_ptr, int exc_num) {
  return ("__faked_opal_exception__");
}
#undef SIM_get_exception_number
int    SIM_get_exception_number(conf_object_t *cpu_ptr, const char *exc_name) {
  UNIMPL;
  return (0);
}

int    SIM_get_num_exceptions() { 
  return ((int)Last_Real_Exception);
}
int    SIM_cpu_get_max_tl(processor_t *cpu_ptr) { UNIMPL; return (0); }
int    SIM_cpu_get_num_windows(processor_t *cpu_ptr) { UNIMPL; return (0); }
int    SIM_cpu_get_current_globals(processor_t *cpu_ptr) {
  UNIMPL; return (0); }
int    SIM_cpu_get_mid(processor_t *cpu_ptr) {UNIMPL; return (0); }
void   SIM_cpu_set_mid(processor_t *cpu_ptr, int mid) { UNIMPL; }
int    SIM_cpu_get_pending_interrupt(processor_t *cpu_ptr) {
  UNIMPL; return (0);
}
int    SIM_cpu_get_pending_exception(processor_t *cpu_ptr) {
  UNIMPL; return (0);
}

/* breakpoint.h */
breakpoint_id_t SIM_breakpoint(conf_object_t *object,
                                        breakpoint_kind_t type,
                                        access_t access,
                                        uint64 address,
                                        uint64 length,
                               unsigned int temporary) {
  return 0;
}

/* processor_api.h */
void SIM_for_all_processors(cb_func_nco_t cb, lang_void *param) {
  UNIMPL;
  return;
}
int  SIM_number_processors() {
  return (1);
}
processor_t *SIM_conf_object_to_processor(conf_object_t* obj) {
  return ((processor_t *) obj);
}
void SIM_enable_processor(processor_t *cpu_ptr) { UNIMPL; }
void SIM_disable_processor(processor_t *cpu_ptr) { UNIMPL; }
void SIM_processor_break(processor_t *, integer_t) { UNIMPL; }
conf_object_t *SIM_current_processor(void) {
  return (NULL);
}
int          SIM_get_proc_no(const processor_t *cpu_ptr) { 
  return (0);
}
int          SIM_get_proc_no_on_node(const processor_t *ptr) {
  return (0);
}

#ifdef SIMICS22X
processor_t *SIM_get_processor(int p, int n) {
  UNIMPL;
  return (NULL);
}
#endif /*SIMICS22X*/

#ifdef SIMICS30
conf_object_t *SIM_get_processor(int p) {
  UNIMPL;
  return (NULL);
}

#endif /*SIMICS30*/

int          SIM_cpu_enabled(processor_t *cpu_ptr) {
  return (true);
}
int          SIM_get_current_proc_no(void) {
  return (0);
}

attr_value_t SIM_get_all_registers(conf_object_t *cpu) { attr_value_t a; UNIMPL; return a; };
ireg_t SIM_read_register(conf_object_t *cpu_ptr, int indx)  { UNIMPL; return (0); }
void   SIM_write_register(conf_object_t *cpu_ptr, int indx, ireg_t value)  { UNIMPL; }
int    SIM_get_register_number(conf_object_t *cpu_ptr, const char *regname)  {
    UNIMPL; return (0); }
const char  *SIM_get_register_name(conf_object_t *cpu_ptr, int reg) {
  return ("King George");
}
la_t   SIM_get_program_counter(processor_t *cpu) {
  // return the last program counter ...
  return (sim_last_pc);
}
la_t   SIM_get_stack_pointer(processor_t *cpu)  {
  UNIMPL;
  return (sim_last_pc);
}
conf_object_t *SIM_proc_no_2_ptr(int id) {
  if (id < 0 || id > (int) number_processors) {
    DEBUG_OUT("SIM_proc_no_2_ptr: bad index\n");
  }
  return (NULL);
}

/* event_api.h */
void SIM_stall_cycle(conf_object_t *obj, cycles_t stall) {
  UNIMPL;
}
cycles_t SIM_cycle_count(conf_object_t *obj) {
  UNIMPL;
  return (0);
}

void SIM_time_post_cycle(conf_object_t * obj, 
                    cycles_t delta, 
                    sync_t sync,
                    event_handler_t handler, 
                    lang_void * arg) {
  UNIMPL;
}
void SIM_break_simulation(const char *msg) {
  UNIMPL;
}
/* control_api.h */
simtime_t SIM_continue(integer_t step) {
  return 1;
}
void SIM_step_post(conf_object_t * obj, 
                   pc_step_t delta, 
                   event_handler_t handler, 
                   lang_void * arg) { UNIMPL; }  
pc_step_t SIM_step_count( conf_object_t *obj ) {
  UNIMPL;
  return (0);
}
conf_object_t* 
SIM_next_queue(conf_object_t * obj){
  UNIMPL;
  return (NULL);
}


/* portions of configuration_api.h */

void SIM_dump_caches(void) {
  UNIMPL;
}

conf_class_t *
SIM_register_class(const char *name, class_data_t *class_data) {
  UNIMPL;
  return (NULL);
}

int
SIM_register_interface(conf_class_t *class_struct, const char *interface_name, void *interface) {
  UNIMPL;
  return (0);
}

interface_t *SIM_get_interface(const conf_object_t *object,
                               const char *interface_name) {
  UNIMPL;
  return (NULL);
}

conf_object_t *
SIM_get_object(const char *name) {
  UNIMPL;
  return (NULL);
}

attr_value_t
SIM_get_attribute(conf_object_t *object, const char *name) {
  attr_value_t e;
  e.kind = Sim_Val_Integer;
  e.u.integer = 0;
  UNIMPL;
  return (e);
}

attr_value_t
SIM_get_attribute_idx(conf_object_t *object, const char *name, attr_value_t *idx) {
  attr_value_t e;
  e.kind = Sim_Val_Integer;
  e.u.integer = 0;
  UNIMPL;
  return (e);
}

set_error_t
SIM_set_attribute(conf_object_t *object, const char *name, attr_value_t *value) {
  set_error_t e = Sim_Set_Ok;
  UNIMPL;
  return (e);
}

void
SIM_free_attribute(attr_value_t value) {
  UNIMPL;
}

void
SIM_object_constructor(conf_object_t *, parse_object_t *) {
  UNIMPL;
}

conf_object_t *
SIM_new_object(conf_class_t *conf_class, const char *instance_name) {
  UNIMPL;
  return (0);
}

int SIM_register_attribute(conf_class_t *, const char *, attr_value_t (*)(void *, conf_object_t *, attr_value_t *), void *, set_error_t (*)(void *, conf_object_t *, attr_value_t *, attr_value_t *), void *, attr_attr_t, const char *) {
  return (0);
}

/* callbacks_api.h */
callback_arguments_t SIM_get_hap_arguments(hap_type_t hap, int type)
{
  UNIMPL;
  callback_arguments_t unused;
  memset( &unused, 0, sizeof(callback_arguments_t) );
  return unused;
}

const char *SIM_get_callback_argument_string(callback_arguments_t cb)
{
  UNIMPL;
  return NULL;
}

hap_type_t SIM_hap_get_number(const char *id){
    UNIMPL;
    return (1);
}
hap_handle_t SIM_hap_install_callback(hap_type_t hap, hap_func_t cb, lang_void *data) {
  UNIMPL;
  return (1);
}
hap_handle_t SIM_hap_install_callback_idx(hap_type_t hap, hap_func_t cb, int64 idx, lang_void *data) {
  UNIMPL;
  return (1);
}
hap_handle_t SIM_hap_install_callback_on_cpu(hap_type_t hap, hap_func_t cb, processor_t *cpu, lang_void *data) {
  UNIMPL;
  return (1);
}

hap_handle_t SIM_hap_add_callback(const char *hap,
                           obj_hap_func_t func,
                           lang_void *user_data){
  UNIMPL;
  return (1);
}


hap_handle_t SIM_hap_add_callback_index(const char *hap,
                           obj_hap_func_t func,
                           lang_void *user_data,
                           integer_t index){
  UNIMPL;
  return (1);
}

hap_handle_t SIM_hap_add_callback_range(const char *hap,
                     obj_hap_func_t func,
                     lang_void *user_data,
                     integer_t start,
                     integer_t end) {
  UNIMPL;
  return (1);
}


void SIM_hap_remove_callback(hap_type_t hap, hap_handle_t hdl) {
    UNIMPL;
}

void SIM_hap_delete_callback_id(const char *hap, hap_handle_t handle){
  UNIMPL;
}

hap_type_t SIM_hap_new_type(const char *id, const char *params, 
                            const char *param_desc,
                            const char *index, const char *desc) {
  return (0);
}

int SIM_hap_occurred(hap_type_t hap, conf_object_t * obj, integer_t value, attr_value_t *list)
{
  return (0);
}
hap_handle_t SIM_hap_register_callback_idx(const char *id, str_hap_func_t cb, integer_t idx, lang_void *data)
{
  UNIMPL;
  return (1);
}

/* statistics_api.h */
attr_value_t SIM_profiling_info(pa_t addr, pa_t length)
{
  UNIMPL;
  attr_value_t result;
  result.kind = Sim_Val_Integer;
  result.u.integer = 0;
  return (result);
}

/* front_api.h */
static int   g_generalException = 0;
static char *g_exceptionString = NULL;
sim_exception_t SIM_check_exception() {
  if (g_generalException)
    return (SimExc_General);
  return (SimExc_No_Exception);
}
sim_exception_t SIM_clear_exception() {
  if (g_generalException)
    return (SimExc_General);
  return (SimExc_No_Exception);
}
int SIM_exception_has_occurred() {
  return (g_generalException);
}
sim_exception_t SIM_get_pending_exception() {
  return ((sim_exception_t) g_generalException);
}
const char *SIM_last_error() {
  if (g_exceptionString)
    return (g_exceptionString);
  return("sim last error\n");
}
void SIM_raise_general_exception(const char *str)
{
  g_generalException = 1;
  g_exceptionString = (char *) str;
}
int SIM_flush(void) {
  fflush(stdout);
  return(0);
}
void __sparc_v9_vtdebug_log_vararg(int param, log_object_t *loggy, const char *format, va_list va)
{
#undef vfprintf
  vfprintf( stdout, format, va );
}

int __sparc_v9_vtvfprintf(FILE *stream, const char *format, va_list va)
{
#undef vfprintf
  return (vfprintf( stream, format, va ));
}

int __l64_p64_vtvfprintf(FILE *stream, const char *format, va_list va)
{
#undef vfprintf
  return (vfprintf( stream, format, va ));
}

int __no_tgt_vtvfprintf(FILE *stream, const char *format, va_list va)
{
#undef vfprintf
  return (vfprintf( stream, format, va ));
}

// These malloc functions work for Simics 2.2.X & 3.0, but NOT 2.0!
//    If compiling with Simics 2.0 we compile with the default else case
#if 1    /* For Simics 3.0 or 2.2.X ONLY */
void *mm_malloc(size_t sz, size_t typesize, const char *type,
                const char *file, int line) {
#undef malloc
  return (malloc(sz));
}

void *mm_zalloc(size_t sz, size_t typesize, const char *type,
                const char *file, int line) {
#undef malloc
  return (malloc(sz));
}

char *mm_strdup(const char *old_str, const char *file, int line) {
  int   len = strlen( old_str ) + 1;
#undef malloc
  char *new_str = (char *) malloc( sizeof(char) * len );
  if (new_str == NULL) {
    DEBUG_OUT("error allocating memory: strdup\n");
  }
  strcpy( new_str, old_str );
  return (new_str);
}

void *mm_realloc(void *ptr, size_t size, size_t typesize, const char *type,
                 const char *file, int line)
{
#undef realloc
  return (realloc(ptr, size));
}
#else  /* For Simics 2.0 */
void *mm_malloc(size_t sz, size_t typesize, const char *type,
                const char *file, int line, int temp) {
#undef malloc
  return (malloc(sz));
}

void *mm_zalloc(size_t sz, size_t typesize, const char *type,
                const char *file, int line, int temp) {
#undef malloc
  return (malloc(sz));
}

char *mm_strdup(const char *old_str, const char *file, int line, int temp) {
  int   len = strlen( old_str ) + 1;
#undef malloc
  char *new_str = (char *) malloc( sizeof(char) * len );
  if (new_str == NULL) {
    DEBUG_OUT("error allocating memory: strdup\n");
  }
  strcpy( new_str, old_str );
  return (new_str);
}

void *mm_realloc(void *ptr, size_t size, size_t typesize, const char *type,
                 const char *file, int line, int temp)
{
#undef realloc
  return (realloc(ptr, size));
}
/* End Simics 2.0 case */

#endif 

/* END "C" LINKAGE */
}

/** Initializes the SimIcs "spoof" state using reasonable values.
 *  Only portions of the SimIcs interface are spoofed.
 */
//**************************************************************************
void simstate_init( void )
{
  DEBUG_OUT("sim-simics-api: V 0.5: initializing simics API simulator\n");

  // make simulated processors
  number_processors = 1;
  total_number_processors = 1;
}

/** Frees all memory associated with simstate
 */
void simstate_fini( void )
{
}
