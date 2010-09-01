
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
 * $Id: interface.C 1.39 05/01/19 13:12:31-06:00 mikem@maya.cs.wisc.edu $
 *
 */

#include "Global.h"
#include "System.h"
#include "SimicsDriver.h"
#include "OpalInterface.h"
#include "EventQueue.h"
#include "mf_api.h"
#include "interface.h"
#include "Sequencer.h"
#include "TransactionInterfaceManager.h"

#ifdef CONTIGUOUS_ADDRESSES
#include "ContiguousAddressTranslator.h"

/* Also used in init.C, commands.C */
ContiguousAddressTranslator * g_p_ca_translator = NULL;

#endif // #ifdef CONTIGUOUS_ADDRESSES

////////////////////////   Local helper functions //////////////////////

// Callback when exception occur
static hap_handle_t s_exception_hap_handle;
static void core_exception_callback(void *data, conf_object_t *cpu,
                                    integer_t exc)
{
  SimicsDriver *simics_intf = dynamic_cast<SimicsDriver*>(g_system_ptr->getDriver());
  ASSERT( simics_intf );
  simics_intf->exceptionCallback(cpu, exc);
}

#ifdef SPARC
// Callback when asi accesses occur
static exception_type_t core_asi_callback(conf_object_t * cpu, generic_transaction_t *g)
{
  SimicsDriver *simics_intf = dynamic_cast<SimicsDriver*>(g_system_ptr->getDriver());
  assert( simics_intf );
  return simics_intf->asiCallback(cpu, g);
}
#endif

static void runRubyEventQueue(conf_object_t* obj, lang_void* arg)
{
  Time time = g_eventQueue_ptr->getTime() + 1;
  DEBUG_EXPR(NODE_COMP, HighPrio, time);
  g_eventQueue_ptr->triggerEvents(time);
  conf_object_t* obj_ptr = (conf_object_t*) SIM_proc_no_2_ptr(0);
  SIM_time_post_cycle(obj_ptr, SIMICS_RUBY_MULTIPLIER, Sim_Sync_Processor, &runRubyEventQueue, NULL);
}

////////////////////////   Simics API functions //////////////////////

int SIMICS_number_processors()
{
  return SIM_number_processors();
}

void SIMICS_wakeup_ruby()
{
  conf_object_t* obj_ptr = (conf_object_t*) SIM_proc_no_2_ptr(0);
  SIM_time_post_cycle(obj_ptr, SIMICS_RUBY_MULTIPLIER, Sim_Sync_Processor, &runRubyEventQueue, NULL);
}

// an analogue to wakeup ruby, this function ends the callbacks ruby normally
// recieves from simics. (it removes ruby from simics's event queue). This
// function should only be called when opal is installed. Opal advances ruby's
// event queue independently of simics.
void SIMICS_remove_ruby_callback( void )
{
  conf_object_t* obj_ptr = (conf_object_t*) SIM_proc_no_2_ptr(0);
  SIM_time_clean( obj_ptr, Sim_Sync_Processor, &runRubyEventQueue, NULL);
}

// Install ruby as the timing model (analogous code exists in ruby/ruby.c)
void SIMICS_install_timing_model( void )
{
  conf_object_t *phys_mem0 = SIM_get_object("phys_mem0");
  attr_value_t   val;
  val.kind     = Sim_Val_String;
  val.u.string = "ruby0";
  set_error_t install_error;

  if(phys_mem0==NULL) {
    /* Look for "phys_mem" instead */
    SIM_clear_exception();
    phys_mem0 = SIM_get_object("phys_mem");
  }

  if(phys_mem0==NULL) {
    /* Okay, now panic... can't install ruby without a physical memory object */
    WARN_MSG( "Cannot Install Ruby... no phys_mem0 or phys_mem object found" );
    WARN_MSG( "Ruby is NOT installed." );
    SIM_clear_exception();
    return;
  }

  install_error = SIM_set_attribute(phys_mem0, "timing_model", &val);

  if (install_error == Sim_Set_Ok) {
    WARN_MSG( "successful installation of the ruby timing model" );
  } else {
    WARN_MSG( "error installing ruby timing model" );
    WARN_MSG( SIM_last_error() );
  }
    
}

// Removes ruby as the timing model interface
void SIMICS_remove_timing_model( void )
{
  conf_object_t *phys_mem0 = SIM_get_object("phys_mem0");
  attr_value_t   val;
  memset( &val, 0, sizeof(attr_value_t) );
  val.kind = Sim_Val_Nil;

  if(phys_mem0==NULL) {
    /* Look for "phys_mem" instead */
    SIM_clear_exception();
    phys_mem0 = SIM_get_object("phys_mem");
  }

  if(phys_mem0==NULL) {
    /* Okay, now panic... can't uninstall ruby without a physical memory object */
    WARN_MSG( "Cannot Uninstall Ruby... no phys_mem0 or phys_mem object found" );
    WARN_MSG( "Uninstall NOT performed." );
    SIM_clear_exception();
    return;
  }

  SIM_set_attribute(phys_mem0, "timing_model", &val);
}

// Installs the (SimicsDriver) function to recieve the exeception callback
void SIMICS_install_exception_callback( void )
{
  // install exception callback
  s_exception_hap_handle =
    SIM_hap_add_callback("Core_Exception",
                         (obj_hap_func_t)core_exception_callback, NULL );
}

// removes the exception callback
void SIMICS_remove_exception_callback( void )
{
  // uninstall exception callback
  SIM_hap_delete_callback_id( "Core_Exception",
                              s_exception_hap_handle );
}

#ifdef SPARC
// Installs the (SimicsDriver) function to recieve the asi callback
void SIMICS_install_asi_callback( void )
{
  for(int i = 0; i < SIM_number_processors(); i++) {
    sparc_v9_interface_t *v9_interface = (sparc_v9_interface_t *)
      SIM_get_interface(SIM_proc_no_2_ptr(i), SPARC_V9_INTERFACE);

    // init asi callbacks, 16bit ASI
    for(int j = 0; j < MAX_ADDRESS_SPACE_ID; j++) {
      v9_interface->install_user_asi_handler(core_asi_callback, j);
    }
  }
}

// removes the asi callback
void SIMICS_remove_asi_callback( void )
{
  for(int i = 0; i < SIM_number_processors(); i++) {
    sparc_v9_interface_t *v9_interface = (sparc_v9_interface_t *)
      SIM_get_interface(SIM_proc_no_2_ptr(i), SPARC_V9_INTERFACE);

    // disable asi callback
    for(int j = 0; j < MAX_ADDRESS_SPACE_ID; j++) {
      v9_interface->remove_user_asi_handler(core_asi_callback, j);
    }
  }
}
#endif

// Query simics for the presence of the opal object. 
// returns its interface if found, NULL otherwise
mf_opal_api_t *SIMICS_get_opal_interface( void )
{
  conf_object_t *opal = SIM_get_object("opal0");
  if (opal != NULL) {
    mf_opal_api_t *opal_intf  = (mf_opal_api_t *) SIM_get_interface( opal, "mf-opal-api" );
    if ( opal_intf != NULL ) {
      return opal_intf;
    } else {
      WARN_MSG("error: OpalInterface: opal does not implement mf-opal-api interface.\n");
      return NULL;
    }
  }
  SIM_clear_exception();   
  return NULL;
}

processor_t * SIMICS_current_processor(){
  return SIM_current_processor();
}

int SIMICS_current_processor_number()
{
  return (SIM_get_proc_no((processor_t *) SIM_current_processor()));
}

integer_t SIMICS_get_insn_count(int cpuNumber)
{
  // NOTE: we already pass in the logical cpuNumber (ie Simics simulated cpu number)
  int num_smt_threads = RubyConfig::numberofSMTThreads();
  integer_t total_insn = 0;
  processor_t* cpu = SIM_proc_no_2_ptr(cpuNumber);
  total_insn += SIM_step_count((conf_object_t*) cpu); 
  return total_insn;
}

integer_t SIMICS_get_cycle_count(int cpuNumber)
{
  processor_t* cpu = SIM_proc_no_2_ptr(cpuNumber);
  integer_t result =  SIM_cycle_count((conf_object_t*) cpu); 
  return result;
}

void SIMICS_unstall_proc(int cpuNumber)
{
  conf_object_t* proc_ptr = (conf_object_t *) SIM_proc_no_2_ptr(cpuNumber);
  SIM_stall_cycle(proc_ptr, 0);
}

void SIMICS_unstall_proc(int cpuNumber, int cycles)
{
  conf_object_t* proc_ptr = (conf_object_t *) SIM_proc_no_2_ptr(cpuNumber);
  SIM_stall_cycle(proc_ptr, cycles);
}

void SIMICS_stall_proc(int cpuNumber, int cycles)
{
  conf_object_t* proc_ptr = (conf_object_t*) SIM_proc_no_2_ptr(cpuNumber);
  if (SIM_stalled_until(proc_ptr) != 0){
    cout << cpuNumber << " Trying to stall. Stall Count currently at " << SIM_stalled_until(proc_ptr) << endl;
  }          
  SIM_stall_cycle(proc_ptr, cycles);
}

void SIMICS_post_stall_proc(int cpuNumber, int cycles)
{
  conf_object_t* proc_ptr = (conf_object_t*) SIM_proc_no_2_ptr(cpuNumber);
  SIM_stacked_post(proc_ptr, ruby_stall_proc, (lang_void *) cycles);
}  
          
integer_t SIMICS_read_physical_memory( int procID, physical_address_t address,
                                       int len )
{
  SIM_clear_exception();      
  ASSERT( len <= 8 );
#ifdef CONTIGUOUS_ADDRESSES
  if(g_p_ca_translator != NULL) {
    address = g_p_ca_translator->TranslateRubyToSimics( address );
  }
#endif // #ifdef CONTIGUOUS_ADDRESSES
  integer_t result = SIM_read_phys_memory( SIM_proc_no_2_ptr(procID),
                                           address, len );
  
  int isexcept = SIM_get_pending_exception();
  if ( !(isexcept == SimExc_No_Exception || isexcept == SimExc_Break) ) {
    sim_exception_t except_code = SIM_clear_exception();
    WARN_MSG( "SIMICS_read_physical_memory: raised exception." );
    WARN_MSG( SIM_last_error() );
    WARN_MSG( Address(address) );
    WARN_MSG( procID );
    ASSERT(0);
  }
  return ( result );
}

/*
 * Read data into a buffer and assume the buffer is already allocated
 */
void SIMICS_read_physical_memory_buffer(int procID, physical_address_t addr,
                                        char* buffer, int len ) {
  processor_t* obj = SIM_proc_no_2_ptr(procID);

  assert( obj != NULL);
  assert( buffer != NULL );

#ifdef CONTIGUOUS_ADDRESSES
  if(g_p_ca_translator != NULL) {
    addr = g_p_ca_translator->TranslateRubyToSimics( addr );
  }
#endif // #ifdef CONTIGUOUS_ADDRESSES

  int buffer_pos = 0;
  physical_address_t start = addr;
  do {
    int size = (len < 8)? len:8;
    integer_t result = SIM_read_phys_memory( obj, start, size );
    int isexcept = SIM_get_pending_exception();
    if ( !(isexcept == SimExc_No_Exception || isexcept == SimExc_Break) ) {
      sim_exception_t except_code = SIM_clear_exception();
      WARN_MSG( "SIMICS_read_physical_memory_buffer: raised exception." );
      WARN_MSG( SIM_last_error() );
      WARN_MSG( addr );
      WARN_MSG( procID );
      ASSERT( 0 );
    }

#ifdef SPARC
    // assume big endian (i.e. SPARC V9 target)
    for(int i = size-1; i >= 0; i--) {
#else
    // assume little endian (i.e. x86 target)
    for(int i = 0; i<size; i++) {
#endif
      buffer[buffer_pos++] = (char) ((result>>(i<<3))&0xff);
    }

    len -= size;
    start += size;
  } while(len != 0);
}

void SIMICS_write_physical_memory( int procID, physical_address_t address,
                                   integer_t value, int len )
{
  ASSERT( len <= 8 );

  SIM_clear_exception();
  
  processor_t* obj = SIM_proc_no_2_ptr(procID);

#ifdef CONTIGUOUS_ADDRESSES
  if(g_p_ca_translator != NULL) {
    address = g_p_ca_translator->TranslateRubyToSimics( address );
  }
#endif // #ifdef CONTIGUOUS_ADDRESSES
  
  int isexcept = SIM_get_pending_exception();
  if ( !(isexcept == SimExc_No_Exception || isexcept == SimExc_Break) ) {
    sim_exception_t except_code = SIM_clear_exception();
    WARN_MSG( "SIMICS_write_physical_memory 1: raised exception." );
    WARN_MSG( SIM_last_error() );
    WARN_MSG( address );
  }
  
  SIM_write_phys_memory(obj, address, value, len );

  isexcept = SIM_get_pending_exception();
  if ( !(isexcept == SimExc_No_Exception || isexcept == SimExc_Break) ) {
    sim_exception_t except_code = SIM_clear_exception();
    WARN_MSG( "SIMICS_write_physical_memory 2: raised exception." );
    WARN_MSG( SIM_last_error() );
    WARN_MSG( address );
  }
}

/*
 * write data to simics memory from a buffer (assumes the buffer is valid)
 */
void SIMICS_write_physical_memory_buffer(int procID, physical_address_t addr,
                                        char* buffer, int len ) {
  processor_t* obj = SIM_proc_no_2_ptr(procID);

  assert( obj != NULL);
  assert( buffer != NULL );

#ifdef CONTIGUOUS_ADDRESSES
  if(g_p_ca_translator != NULL) {
    addr = g_p_ca_translator->TranslateRubyToSimics( addr );
  }
#endif // #ifdef CONTIGUOUS_ADDRESSES

  int buffer_pos = 0;
  physical_address_t start = addr;
  do {
    int size = (len < 8)? len:8;
    //integer_t result = SIM_read_phys_memory( obj, start, size );
    integer_t value = 0;
#ifdef SPARC
    // assume big endian (i.e. SPARC V9 target)
    for(int i = size-1; i >= 0; i--) {
#else
    // assume little endian (i.e. x86 target)
    for(int i = 0; i<size; i++) {
#endif
      integer_t mask = buffer[buffer_pos++];
      value |= ((mask)<<(i<<3));
    } 


    SIM_write_phys_memory( obj, start, value, size);
    int isexcept = SIM_get_pending_exception();
    if ( !(isexcept == SimExc_No_Exception || isexcept == SimExc_Break) ) {
      sim_exception_t except_code = SIM_clear_exception();
      WARN_MSG( "SIMICS_write_physical_memory_buffer: raised exception." );
      WARN_MSG( SIM_last_error() );
      WARN_MSG( addr );
    }

    len -= size;
    start += size;
  } while(len != 0);
}

bool SIMICS_check_memory_value(int procID, physical_address_t addr,
                               char* buffer, int len) {
  char buf[len];
  SIMICS_read_physical_memory_buffer(procID, addr, buf, len);
  return (memcmp(buffer, buf, len) == 0)? true:false;
}

physical_address_t SIMICS_translate_address( int procID, Address address ) {
  SIM_clear_exception();      
  physical_address_t physical_addr = SIM_logical_to_physical(SIM_proc_no_2_ptr(procID), Sim_DI_Instruction, address.getAddress() );
  int isexcept = SIM_get_pending_exception();
  if ( !(isexcept == SimExc_No_Exception || isexcept == SimExc_Break) ) {
    sim_exception_t except_code = SIM_clear_exception();
    /*
    WARN_MSG( "SIMICS_translate_address: raised exception." );
    WARN_MSG( procID );
    WARN_MSG( address );
    WARN_MSG( SIM_last_error() );
    */
    return 0;
  }

#ifdef CONTIGUOUS_ADDRESSES
  if(g_p_ca_translator != NULL) {
    physical_addr = g_p_ca_translator->TranslateSimicsToRuby( physical_addr );
  }
#endif // #ifdef CONTIGUOUS_ADDRESSES

  return physical_addr;
}

physical_address_t SIMICS_translate_data_address( int procID, Address address ) {
  SIM_clear_exception();      
  physical_address_t physical_addr = SIM_logical_to_physical(SIM_proc_no_2_ptr(procID), Sim_DI_Data, address.getAddress() );
  int isexcept = SIM_get_pending_exception();
  if ( !(isexcept == SimExc_No_Exception || isexcept == SimExc_Break) ) {
    sim_exception_t except_code = SIM_clear_exception();
    /*
    WARN_MSG( "SIMICS_translate_data_address: raised exception." );
    WARN_MSG( procID );
    WARN_MSG( address );
    WARN_MSG( SIM_last_error() );
    */
  }
  return physical_addr;
}

#ifdef SPARC
bool SIMICS_is_ldda(const memory_transaction_t *mem_trans) {
 conf_object_t *cpu  = mem_trans->s.ini_ptr;
 int            proc= SIMICS_get_proc_no(cpu);
 Address        addr = SIMICS_get_program_counter(cpu);
 physical_address_t phys_addr = SIMICS_translate_address( proc, addr );
 uint32         instr= SIMICS_read_physical_memory( proc, phys_addr, 4 );

 // determine if this is a "ldda" instruction (non-exclusive atomic)
 // ldda bit mask:  1100 0001 1111 1000 == 0xc1f80000
 // ldda match   :  1100 0000 1001 1000 == 0xc0980000
 if ( (instr & 0xc1f80000) == 0xc0980000 ) {
   // should exactly be ldda instructions
   ASSERT(!strncmp(SIMICS_disassemble_physical(proc, phys_addr), "ldda", 4));
   //cout << "SIMICS_is_ldda END" << endl;
   return true;
  }
  return false;
}
#endif

const char *SIMICS_disassemble_physical( int procID, physical_address_t pa ) {
#ifdef CONTIGUOUS_ADDRESSES
  if(g_p_ca_translator != NULL) {
    pa = g_p_ca_translator->TranslateRubyToSimics( pa );
  }
#endif // #ifdef CONTIGUOUS_ADDRESSES 
  return SIM_disassemble( SIM_proc_no_2_ptr(procID), pa , /* physical */ 0)->string;
}

Address SIMICS_get_program_counter(conf_object_t *cpu) {
  assert(cpu != NULL);
  return Address(SIM_get_program_counter((processor_t *) cpu));
}

Address SIMICS_get_npc(int procID) {
  conf_object_t *cpu = SIM_proc_no_2_ptr(procID);
  return Address(SIM_read_register(cpu, SIM_get_register_number(cpu, "npc")));
}  

Address SIMICS_get_program_counter(int procID) {
  conf_object_t *cpu = SIM_proc_no_2_ptr(procID);
  assert(cpu != NULL);
  
  Address addr = Address(SIM_get_program_counter(cpu));
  return addr;
}

/* NOTE: SIM_set_program_counter sets NPC to PC+4 */
void SIMICS_set_program_counter(int procID, Address newPC) {
  conf_object_t *cpu = SIM_proc_no_2_ptr(procID);
  assert(cpu != NULL);
  
  SIM_stacked_post(cpu, ruby_set_program_counter, (lang_void*) newPC.getAddress());
}

void SIMICS_set_pc(int procID, Address newPC) {
  // IMPORTANT: procID is the SIMICS simulated proc number (takes into account SMT)
  conf_object_t *cpu = SIM_proc_no_2_ptr(procID);
  assert(cpu != NULL);
  
  if(OpalInterface::isOpalLoaded() == false){
    SIM_set_program_counter(cpu, newPC.getAddress());
  } else {
    // explicitly change PC
    ruby_set_pc( cpu, (lang_void *) newPC.getAddress() );
  }
  int isexcept = SIM_get_pending_exception();
  if ( !(isexcept == SimExc_No_Exception || isexcept == SimExc_Break) ) {
    sim_exception_t except_code = SIM_clear_exception();
    WARN_MSG( "SIMICS_set_pc: raised exception." );
    WARN_MSG( SIM_last_error() );
    ASSERT(0);
  }
}

void SIMICS_set_next_program_counter(int procID, Address newNPC) {
  conf_object_t *cpu = SIM_proc_no_2_ptr(procID);
  assert(cpu != NULL);
  
  SIM_stacked_post(cpu, ruby_set_npc, (lang_void*) newNPC.getAddress());
}

void SIMICS_set_npc(int procID, Address newNPC) {
  conf_object_t *cpu = SIM_proc_no_2_ptr(procID);
  assert(cpu != NULL);
  
  if(OpalInterface::isOpalLoaded() == false){
    SIM_write_register(cpu, SIM_get_register_number(cpu, "npc"), newNPC.getAddress());
  } else {
    // explicitly change NPC
    ruby_set_npc( cpu, (lang_void *) newNPC.getAddress() );
  }
  
  int isexcept = SIM_get_pending_exception();
  if ( !(isexcept == SimExc_No_Exception || isexcept == SimExc_Break) ) {
    sim_exception_t except_code = SIM_clear_exception();
    WARN_MSG( "SIMICS_set_npc: raised exception " );
    WARN_MSG( SIM_last_error() );
    ASSERT(0);
  }
}

void SIMICS_post_continue_execution(int procID){
  conf_object_t *cpu = SIM_proc_no_2_ptr(procID);
  assert(cpu != NULL);
  
  if(OpalInterface::isOpalLoaded() == false){
    SIM_stacked_post(cpu, ruby_continue_execution, (lang_void *) NULL);
  } else{
    ruby_continue_execution( cpu, (lang_void *) NULL );
  }
}  

void SIMICS_post_restart_transaction(int procID){
  conf_object_t *cpu = SIM_proc_no_2_ptr(procID);
  assert(cpu != NULL);
  
  if(OpalInterface::isOpalLoaded() == false){
    SIM_stacked_post(cpu, ruby_restart_transaction, (lang_void *) NULL);
  } else{
    ruby_restart_transaction( cpu, (lang_void *) NULL );
  }
}  

// return -1 when fail
int SIMICS_get_proc_no(conf_object_t *cpu) {
  int proc_no = SIM_get_proc_no((processor_t *) cpu);
  return proc_no;
}

void SIMICS_disable_processor( int cpuNumber ) {
  if(SIM_cpu_enabled(SIMICS_get_proc_ptr(cpuNumber))) {
    SIM_disable_processor(SIMICS_get_proc_ptr(cpuNumber));
  } else {
    WARN_MSG(cpuNumber);
    WARN_MSG( "Tried to disable a 'disabled' processor");
    ASSERT(0);
  }
}

void SIMICS_post_disable_processor( int cpuNumber ) {
  SIM_stacked_post(SIMICS_get_proc_ptr(cpuNumber), ruby_disable_processor, (lang_void*) NULL);
}
          
void SIMICS_enable_processor( int cpuNumber ) {
  if(!SIM_cpu_enabled(SIMICS_get_proc_ptr(cpuNumber))) {
    SIM_enable_processor(SIMICS_get_proc_ptr(cpuNumber));
  } else {
    WARN_MSG(cpuNumber);
    WARN_MSG( "Tried to enable an 'enabled' processor");
  }
}

bool SIMICS_processor_enabled( int cpuNumber ) {
  return SIM_cpu_enabled(SIMICS_get_proc_ptr(cpuNumber));
}          

// return NULL when fail
conf_object_t* SIMICS_get_proc_ptr(int cpuNumber) {
  return (conf_object_t *) SIM_proc_no_2_ptr(cpuNumber);
}

void SIMICS_print_version(ostream& out) {
  const char* version = SIM_version();
  if (version != NULL) {
    out << "simics_version: " << SIM_version() << endl;
  }
}

// KM -- From Nikhil's SN code
//these functions should be in interface.C ??

uinteger_t SIMICS_read_control_register(int cpuNumber, int registerNumber)
{
  processor_t* cpu = SIM_proc_no_2_ptr(cpuNumber);
  uinteger_t result = SIM_read_register(cpu, registerNumber);
  return result;
}

uinteger_t SIMICS_read_window_register(int cpuNumber, int window, int registerNumber)
{
  processor_t* cpu = SIM_proc_no_2_ptr(cpuNumber);
  uinteger_t result = SIM_read_register(cpu, registerNumber);
  return result;
}

uinteger_t SIMICS_read_global_register(int cpuNumber, int globals, int registerNumber)
{
  processor_t* cpu = SIM_proc_no_2_ptr(cpuNumber);
  uinteger_t result = SIM_read_register(cpu, registerNumber);
  return result;
}

/**
   uint64 SIMICS_read_fp_register_x(int cpuNumber, int registerNumber)
   {
   processor_t* cpu = SIM_proc_no_2_ptr(cpuNumber);
   return SIM_read_fp_register_x(cpu, registerNumber);
   }
**/

void SIMICS_write_control_register(int cpuNumber, int registerNumber, uinteger_t value)
{
  processor_t* cpu = SIM_proc_no_2_ptr(cpuNumber);
  SIM_write_register(cpu, registerNumber, value);
}

void SIMICS_write_window_register(int cpuNumber, int window, int registerNumber, uinteger_t value)
{
  processor_t* cpu = SIM_proc_no_2_ptr(cpuNumber);
  SIM_write_register(cpu, registerNumber, value);
}

void SIMICS_write_global_register(int cpuNumber, int globals, int registerNumber, uinteger_t value)
{
  processor_t* cpu = SIM_proc_no_2_ptr(cpuNumber);
  SIM_write_register(cpu, registerNumber, value);
}

/***
    void SIMICS_write_fp_register_x(int cpuNumber, int registerNumber, uint64 value)
    {
    processor_t* cpu = SIM_proc_no_2_ptr(cpuNumber);
    SIM_write_fp_register_x(cpu, registerNumber, value);
    }
***/

// KM -- Functions using new APIs (update from Nikhil's original)

int SIMICS_get_register_number(int cpuNumber, const char * reg_name){
  int result = SIM_get_register_number(SIM_proc_no_2_ptr(cpuNumber), reg_name);
  return result;
}

const char * SIMICS_get_register_name(int cpuNumber, int reg_num){
  const char * result = SIM_get_register_name(SIM_proc_no_2_ptr(cpuNumber), reg_num);
  return result;
}

uinteger_t SIMICS_read_register(int cpuNumber, int registerNumber)
{
  processor_t* cpu = SIM_proc_no_2_ptr(cpuNumber);
  uinteger_t result = SIM_read_register(cpu, registerNumber);
  return result;
}

void SIMICS_write_register(int cpuNumber, int registerNumber, uinteger_t value)
{
  processor_t* cpu = SIM_proc_no_2_ptr(cpuNumber);
  SIM_write_register(cpu, registerNumber, value);
}

// This version is called whenever we are about to jump to the SW handler
void ruby_set_pc(conf_object_t *cpu, lang_void *parameter){
  physical_address_t paddr;
  paddr = (physical_address_t) parameter;
  // Simics' processor number
  int proc_no = SIM_get_proc_no((processor_t *) cpu);
  int smt_proc_no = proc_no / RubyConfig::numberofSMTThreads();
  SIM_set_program_counter(cpu, paddr);
  //cout << "ruby_set_pc setting cpu[ " << proc_no << " ] smt_cpu[ " << smt_proc_no << " ] PC[ " << hex << paddr << " ]" << dec << endl;
  physical_address_t newpc = SIM_get_program_counter(cpu);
  int pc_reg = SIM_get_register_number(cpu, "pc");
  int npc_reg = SIM_get_register_number( cpu, "npc");
  uinteger_t pc = SIM_read_register(cpu, pc_reg);
  uinteger_t npc = SIM_read_register(cpu, npc_reg);
  //cout << "NEW PC[ 0x" << hex << newpc << " ]" << " PC REG[ 0x" << pc << " ] NPC REG[ 0x" << npc << " ]" << dec << endl;

  if(XACT_MEMORY){
    if( !OpalInterface::isOpalLoaded() ){
      // using SimicsDriver
      ASSERT( proc_no == smt_proc_no );
      SimicsDriver *simics_intf = dynamic_cast<SimicsDriver*>(g_system_ptr->getDriver());
      ASSERT( simics_intf );
      simics_intf->notifyTrapStart( proc_no, Address(paddr), 0 /*dummy threadID*/, 0 /* Simics uses 1 thread */ );
    }
    else{
      // notify Opal about changing pc to SW handler 
      //cout << "informing Opal via notifyTrapStart proc = " << proc_no << endl;
      //g_system_ptr->getSequencer(smt_proc_no)->notifyTrapStart( proc_no, Address(paddr) );
    }

  if (XACT_DEBUG && XACT_DEBUG_LEVEL > 1){
    cout <<  g_eventQueue_ptr->getTime() << " " << proc_no
       << " ruby_set_pc PC: " << hex
       << SIM_get_program_counter(cpu) << 
      " NPC is: " << hex << SIM_read_register(cpu, 33) << " pc_val: " << paddr << dec << endl; 
  }
  }
}  

// This version is called whenever we are about to return from SW handler 
void ruby_set_program_counter(conf_object_t *cpu, lang_void *parameter){
  physical_address_t paddr;
  paddr = (physical_address_t) parameter;
  // Simics' processor number
  int proc_no = SIM_get_proc_no((processor_t *) cpu);
  // SMT proc number
  int smt_proc_no = proc_no / RubyConfig::numberofSMTThreads();
  
  // SIM_set_program_counter() also sets the NPC to PC+4. 
  // Need to ensure that NPC doesn't change especially for PCs in the branch delay slot
  uinteger_t npc_val = SIM_read_register(cpu, SIM_get_register_number(cpu, "npc"));
  SIM_set_program_counter(cpu, paddr);
  SIM_write_register(cpu, SIM_get_register_number(cpu, "npc"), npc_val);

  //LUKE - notify Opal of PC change (ie end of all register updates and abort complete)
  //          I moved the register checkpoint restoration to here also, to jointly update the PC and the registers at the same time
  if(XACT_MEMORY){
    if( !OpalInterface::isOpalLoaded() ){
      //using SimicsDriver
      //we should only be running with 1 thread with Simics 
      ASSERT( proc_no == smt_proc_no );
      SimicsDriver *simics_intf = dynamic_cast<SimicsDriver*>(g_system_ptr->getDriver());
      ASSERT( simics_intf );
      simics_intf->notifyTrapComplete(proc_no, Address( paddr ), 0 /* Simics uses 1 thread */ );
   }
   else{
     //using OpalInterface
     // g_system_ptr->getSequencer(smt_proc_no)->notifyTrapComplete( proc_no, Address(paddr) );
   }
  }
  if (XACT_DEBUG && XACT_DEBUG_LEVEL > 1){
    cout <<  g_eventQueue_ptr->getTime() << " " << proc_no
       << " ruby_set_program_counter PC: " << hex
       << SIM_get_program_counter(cpu) << 
      " NPC is: " << hex << SIM_read_register(cpu, 33) << " pc_val: " << paddr << " npc_val: " << npc_val << dec << endl; 
  }
}

void ruby_set_npc(conf_object_t *cpu, lang_void *parameter){
  physical_address_t paddr;
  paddr = (physical_address_t) parameter;
  int proc_no = SIM_get_proc_no((processor_t *) cpu);
  // SMT proc number
  int smt_proc_no = proc_no / RubyConfig::numberofSMTThreads();
  
  SIM_write_register(cpu, SIM_get_register_number(cpu, "npc"), paddr);
  if (XACT_DEBUG && XACT_DEBUG_LEVEL > 1){
    cout <<  g_eventQueue_ptr->getTime() << " " << proc_no
         << " ruby_set_npc val: " << hex << paddr << " PC: " << hex
       << SIM_get_program_counter(cpu) << 
      " NPC is: " << hex << SIM_read_register(cpu, 33) << dec << endl; 
  }
}  

void ruby_continue_execution(conf_object_t *cpu, lang_void *parameter){
  int logical_proc_no = SIM_get_proc_no((processor_t *) cpu);
  int thread = logical_proc_no % RubyConfig::numberofSMTThreads();
  int proc_no = logical_proc_no / RubyConfig::numberofSMTThreads();
  g_system_ptr->getTransactionInterfaceManager(proc_no)->continueExecutionCallback(thread);
}

void ruby_restart_transaction(conf_object_t *cpu, lang_void *parameter){  
  int logical_proc_no = SIM_get_proc_no((processor_t *) cpu);
  int thread = logical_proc_no % RubyConfig::numberofSMTThreads();
  int proc_no = logical_proc_no / RubyConfig::numberofSMTThreads();
  g_system_ptr->getTransactionInterfaceManager(proc_no)->restartTransactionCallback(thread);
}

void ruby_stall_proc(conf_object_t *cpu, lang_void *parameter){
  int logical_proc_no = SIM_get_proc_no((processor_t*)cpu);
  int cycles          = (uint64)parameter;

  SIMICS_stall_proc(logical_proc_no, cycles);
}  
          
void ruby_disable_processor(conf_object_t *cpu, lang_void *parameter){
  int logical_proc_no = SIM_get_proc_no((processor_t*)cpu);
  SIMICS_disable_processor(logical_proc_no);
}  
          
#if 0
/********************************************************************
 * STC management functions called by Ruby, should not be use anymore
 ********************************************************************/
void SIMICS_flush_STC(int cpuNumber)
{
  processor_t* cpu = SIM_proc_no_2_ptr(cpuNumber);
  SIM_STC_flush_cache(cpu);
}

void SIMICS_invalidate_from_STC(const Address& address, int cpuNumber)
{
  Address line_address(address);
  line_address.makeLineAddress();

  processor_t* cpu_ptr = SIM_proc_no_2_ptr(cpuNumber);

  // invalidate from data STCs
  SIM_flush_D_STC_physical(cpu_ptr, line_address.getAddress(), 64, Sim_RW_Read);
  SIM_flush_D_STC_physical(cpu_ptr, line_address.getAddress(), 64, Sim_RW_Write);

  // invalidate from instr STC
  SIM_flush_I_STC_physical(cpu_ptr, line_address.getAddress(), 64);
}

void SIMICS_downgrade_from_STC(const Address& address, int cpuNumber)
{

  Address line_address(address);
  line_address.makeLineAddress();

  processor_t* cpu_ptr = SIM_proc_no_2_ptr(cpuNumber);

  // downgrade from data STCs
  SIM_flush_D_STC_physical(cpu_ptr, line_address.getAddress(), 64, Sim_RW_Write);
}

#endif


