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

/*
 * interface.C
 *
 * Description: 
 * See interface.h
 * 
 * $Id: interface.C 1.00 05/11/09 14:02:03-05:00 gibson@mustardseed.cs.wisc.edu $
 *
 */

#include "Tourmaline_Global.h"
#include "interface.h"

bool timing_model_installed = false;
bool snoop_model_installed = false;

////////////////////////   Simics API functions //////////////////////

int SIMICS_number_processors()
{
  return SIM_number_processors();
}

processor_t * SIMICS_current_processor(){
  return SIM_current_processor();
}

int SIMICS_current_processor_number()
{
  return SIM_get_proc_no((processor_t *) SIM_current_processor());
}

logical_address_t SIMICS_get_program_counter(conf_object_t * obj) {
  return SIM_get_program_counter(obj);
}

integer_t SIMICS_get_insn_count(int cpuNumber)
{
  processor_t* cpu = SIM_proc_no_2_ptr(cpuNumber);
  return SIM_step_count((conf_object_t*) cpu); 
}

cycles_t SIMICS_cycle_count(int cpuNumber) 
{
  processor_t* cpu = SIM_proc_no_2_ptr(cpuNumber);
  return SIM_cycle_count((conf_object_t*) cpu); 
}

integer_t SIMICS_read_physical_memory( int procID, physical_address_t address,
                                       int len )
{
  assert( len <= 8 );
  integer_t result = SIM_read_phys_memory( SIM_proc_no_2_ptr(procID),
                                           address, len );
  
  int isexcept = SIM_get_pending_exception();
  if ( !(isexcept == SimExc_No_Exception || isexcept == SimExc_Break) ) {
    sim_exception_t except_code = SIM_clear_exception();
    WARNING( "SIMICS_read_physical_memory: raised exception: %s\n", SIM_last_error() );
    assert(0);
  }
  return ( result );
}

void SIMICS_write_physical_memory( int procID, physical_address_t address,
                                   integer_t value, int len )
{
  assert( len <= 8 );
  processor_t* obj = SIM_proc_no_2_ptr(procID);
  SIM_write_phys_memory(obj, address, value, len );

  int isexcept = SIM_get_pending_exception();
  if ( !(isexcept == SimExc_No_Exception || isexcept == SimExc_Break) ) {
    sim_exception_t except_code = SIM_clear_exception();
    WARNING( "SIMICS_write_physical_memory: raised exception: %s\n", SIM_last_error() );
    assert(0);
  }
}

// return -1 when fail
int SIMICS_get_proc_no(conf_object_t *cpu) {
  return SIM_get_proc_no((processor_t *) cpu);
}

// return NULL when fail
conf_object_t* SIMICS_get_proc_ptr(int cpuNumber) {
  return (conf_object_t *) SIM_proc_no_2_ptr(cpuNumber);
}

void SIMICS_disable_processor( int cpuNumber ) {
  if(SIMICS_cpu_enabled(cpuNumber)) {
    SIM_disable_processor(SIMICS_get_proc_ptr(cpuNumber));
  } else {
    ERROR( "Tried to enable processor %i, but found it was already enabled.\n",cpuNumber);
  }
}

void SIMICS_enable_processor( int cpuNumber ) {
  if(!SIMICS_cpu_enabled(cpuNumber)) {
    SIM_enable_processor(SIMICS_get_proc_ptr(cpuNumber));
  } else {
    ERROR( "Tried to disable processor %i, but found it was already disabled.\n",cpuNumber);
  }
}

bool SIMICS_cpu_enabled( int cpuNumber ) {
  if(SIM_cpu_enabled( SIMICS_get_proc_ptr(cpuNumber) ) == 0) return false;
  return true;
}

void SIMICS_break_simulation( const char * msg ) {
  SIM_break_simulation(msg);
}

void SIMICS_install_timing_model() {
  conf_object_t *phys_mem0 = SIM_get_object("phys_mem0");
  attr_value_t   val;
  val.kind     = Sim_Val_Object;
  val.u.object = SIM_get_object("tourmaline0");
  set_error_t install_error = SIM_set_attribute(phys_mem0, "timing_model", &val);

  if(install_error != Sim_Set_Ok) {
    ERROR("Failed to install Tourmaline as a timing model: %s\n",SIM_last_error());
  }

  timing_model_installed = true;

  conf_object_t *sim_obj = SIM_get_object("sim");
  val.kind = Sim_Val_Integer;
  val.u.integer = 1;
  install_error = SIM_set_attribute(sim_obj, "cpu_switch_time", &val);
  if(install_error != Sim_Set_Ok) {
    ERROR("Failed to set cpu_switch_time to 1: %s\n",SIM_last_error());
  }
}

void SIMICS_install_snoop_model() {
  conf_object_t *phys_mem0 = SIM_get_object("phys_mem0");
  attr_value_t   val;
  val.kind     = Sim_Val_Object;
  val.u.object = SIM_get_object("tourmaline0");
  set_error_t install_error = SIM_set_attribute(phys_mem0, "snoop_device", &val);

  if(install_error != Sim_Set_Ok) {
    ERROR("Failed to install Tourmaline as a snoop device: %s\n",SIM_last_error());
  }
  
  snoop_model_installed = true;
  
  conf_object_t *sim_obj = SIM_get_object("sim");
  val.kind = Sim_Val_Integer;
  val.u.integer = 1;
  install_error = SIM_set_attribute(sim_obj, "cpu_switch_time", &val);
  if(install_error != Sim_Set_Ok) {
    ERROR("Failed to set cpu_switch_time to 1: %s\n",SIM_last_error());
  }

}

void SIMICS_remove_timing_model() {
  conf_object_t *phys_mem0 = SIM_get_object("phys_mem0");
  attr_value_t   val;
  memset( &val, 0, sizeof(attr_value_t) );
  val.kind = Sim_Val_Nil;
  SIM_set_attribute(phys_mem0, "timing_model", &val);

  timing_model_installed = false;
  
  if(!snoop_model_installed) {
    conf_object_t *sim_obj = SIM_get_object("sim");
    val.kind = Sim_Val_Integer;
    val.u.integer = 1000;
    set_error_t install_error = SIM_set_attribute(sim_obj, "cpu_switch_time", &val);
    if(install_error != Sim_Set_Ok) {
      ERROR("Failed to set cpu_switch_time to 1000: %s\n",SIM_last_error());
    }
  }
}

void SIMICS_remove_snoop_model() {
  conf_object_t *phys_mem0 = SIM_get_object("phys_mem0");
  attr_value_t   val;
  memset( &val, 0, sizeof(attr_value_t) );
  val.kind = Sim_Val_Nil;
  SIM_set_attribute(phys_mem0, "snoop_device", &val);

  snoop_model_installed = false;

  if(!timing_model_installed) {
    conf_object_t *sim_obj = SIM_get_object("sim");
    val.kind = Sim_Val_Integer;
    val.u.integer = 1000;
    set_error_t install_error = SIM_set_attribute(sim_obj, "cpu_switch_time", &val);
    if(install_error != Sim_Set_Ok) {
      ERROR("Failed to set cpu_switch_time to 1000: %s\n",SIM_last_error());
    }  
  }
}

uinteger_t SIMICS_get_data_from_memtrans(void * p_v9trans) {
  return SIM_get_mem_op_value_cpu( (generic_transaction_t*) p_v9trans );
}

void SIMICS_set_data_from_memtrans(void * p_v9trans, uinteger_t newData) {
  SIM_set_mem_op_value_cpu( (generic_transaction_t*) p_v9trans, newData);
}

void SIMICS_print_version(ostream& out) {
  const char* version = SIM_version();
  if(version!=NULL) {
    out << "simics_version: " << version << endl;
  }
}

int SIMICS_get_register_number(int cpuNumber, const char * reg_name){
  return SIM_get_register_number(SIM_proc_no_2_ptr(cpuNumber), reg_name);
}

const char * SIMICS_get_register_name(int cpuNumber, int reg_num){
  return SIM_get_register_name(SIM_proc_no_2_ptr(cpuNumber), reg_num);
}

uinteger_t SIMICS_read_register(int cpuNumber, int registerNumber)
{
  processor_t* cpu = SIM_proc_no_2_ptr(cpuNumber);
  return SIM_read_register(cpu, registerNumber);
}

void SIMICS_write_register(int cpuNumber, int registerNumber, uinteger_t value)
{
  processor_t* cpu = SIM_proc_no_2_ptr(cpuNumber);
  SIM_write_register(cpu, registerNumber, value);
}
