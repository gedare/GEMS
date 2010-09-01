
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
 * $Id: interface.h 1.33 05/01/19 13:12:32-06:00 mikem@maya.cs.wisc.edu $
 *
 * Description: 
 *
 */

#ifndef INTERFACE_H
#define INTERFACE_H

#include "Global.h"
#include "mf_api.h"
#include "Address.h"

// Simics includes
extern "C" {
#include "simics/api.h"
}

#ifdef SIMICS30
#ifdef SPARC
typedef v9_memory_transaction_t memory_transaction_t;
#else
typedef x86_memory_transaction_t memory_transaction_t;
#endif
#endif

// simics memory access
integer_t SIMICS_read_physical_memory(int procID, physical_address_t address,
                                      int len );
void SIMICS_read_physical_memory_buffer(int procID, physical_address_t addr,
                                         char* buffer, int len );
void SIMICS_write_physical_memory( int procID, physical_address_t address,
                                   integer_t value, int len );
void SIMICS_write_physical_memory_buffer(int procID, physical_address_t addr,
                                         char* buffer, int len );
bool SIMICS_check_memory_value(int procID, physical_address_t addr,
                               char* buffer, int len);
const char *SIMICS_disassemble_physical( int procID, physical_address_t pa );

// simics VM translation, decoding, etc.
physical_address_t SIMICS_translate_address( int procID, Address address );
physical_address_t SIMICS_translate_data_address( int procID, Address address );
#ifdef SPARC
bool SIMICS_is_ldda(const memory_transaction_t *mem_trans);
#endif

// simics timing
void SIMICS_unstall_proc(int cpuNumber);
void SIMICS_unstall_proc(int cpuNumber, int cycles);
void SIMICS_stall_proc(int cpuNumber, int cycles);
void SIMICS_post_stall_proc(int cpuNumber, int cycles);
void SIMICS_wakeup_ruby();

// simics callbacks
void SIMICS_remove_ruby_callback( void );
void SIMICS_install_timing_model( void );
void SIMICS_remove_timing_model( void );
void SIMICS_install_exception_callback( void );
void SIMICS_remove_exception_callback( void );
#ifdef SPARC
void SIMICS_install_asi_callback( void );
void SIMICS_remove_asi_callback( void );
#endif

// simics PC, IC
integer_t SIMICS_get_insn_count( int cpuNumber );
integer_t SIMICS_get_cycle_count(int cpuNumber);
Address SIMICS_get_program_counter( conf_object_t *cpu );
Address SIMICS_get_program_counter( int procID );
Address SIMICS_get_npc(int procID);
void SIMICS_set_program_counter( int procID, Address newPC );
void SIMICS_set_next_program_counter( int procID, Address newPC );
void SIMICS_set_pc( int procID, Address newPC );
void SIMICS_set_npc( int procID, Address newNPC );

void SIMICS_post_continue_execution(int procID);
void SIMICS_post_restart_transaction(int procID);

// simics processor number
int SIMICS_number_processors( void );
processor_t * SIMICS_current_processor( void );
int SIMICS_current_processor_number( void );
int SIMICS_get_proc_no( conf_object_t *cpu );
conf_object_t* SIMICS_get_proc_ptr( int cpuNumber );

// simics version
void SIMICS_print_version(ostream& out);

// opal
mf_opal_api_t *SIMICS_get_opal_interface( void );

// STC related, should not be used anymore!
void SIMICS_flush_STC(int cpuNumber);
void SIMICS_invalidate_from_STC(const Address& address, int cpuNumber);
void SIMICS_downgrade_from_STC(const Address& address, int cpuNumber);

// KM -- from Nikhil's SN code
uinteger_t SIMICS_read_control_register(int cpuNumber, int registerNumber);
uinteger_t SIMICS_read_window_register(int cpuNumber, int window, int registerNumber);
uinteger_t SIMICS_read_global_register(int cpuNumber, int globals, int registerNumber);
//uint64 SIMICS_read_fp_register_x(int cpuNumber, int registerNumber);

// KM -- new version based on reg names
int SIMICS_get_register_number(int cpuNumber, const char * reg_name);
const char * SIMICS_get_register_name(int cpuNumber, int reg_num);
uinteger_t SIMICS_read_register(int cpuNumber, int registerNumber);
void SIMICS_write_register(int cpuNumber, int registerNumber, uinteger_t value);

void SIMICS_write_control_register(int cpuNumber, int registerNumber, uinteger_t value);
void SIMICS_write_window_register(int cpuNumber, int window, int registerNumber, uinteger_t value);
void SIMICS_write_global_register(int cpuNumber, int globals, int registerNumber, uinteger_t value);
void SIMICS_write_fp_register_x(int cpuNumber, int registerNumber, uint64 value);
void SIMICS_enable_processor(int cpuNumber);
void SIMICS_disable_processor(int cpuNumber);
void SIMICS_post_disable_processor(int cpuNumber);
bool SIMICS_processor_enabled(int cpuNumber);

void ruby_abort_transaction(conf_object_t *cpu, lang_void *parameter);
void ruby_set_program_counter(conf_object_t *cpu, lang_void *parameter);
void ruby_set_pc(conf_object_t *cpu, lang_void *parameter);
void ruby_set_npc(conf_object_t *cpu, lang_void *parameter);
void ruby_continue_execution(conf_object_t *cpu, lang_void *parameter);
void ruby_restart_transaction(conf_object_t *cpu, lang_void *parameter);
void ruby_stall_proc(conf_object_t *cpu, lang_void *parameter);
void ruby_disable_processor(conf_object_t *cpu, lang_void *parameter);

#endif //INTERFACE_H

