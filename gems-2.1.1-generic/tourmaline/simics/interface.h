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
 * interface.h
 *
 * Description: 
 * Layer of indirection between the Tourmaline module and the Simics API. 
 * The interface exists for two reasons:
 *  1) To facilitate easy (easier) transitions between versions of Simics
 *  2) To augment the Simics API with useful module-specific extensions by
 *     creating pseudo-calls out of combinations of provided API calls.
 *     Example: SIMICS_install_timing_interface()
 *
 * Functions in interface.[Ch] are called from within Tourmaline. That is, control
 * flows Tourmaline->interface.[Ch]->Simics. Control flowing Simics->Tourmaline
 * passes though commands.[Ch]
 * 
 * $Id: interface.h 1.00 05/11/09 14:02:03-05:00 gibson@mustardseed.cs.wisc.edu $
 *
 */

#ifndef INTERFACE_H
#define INTERFACE_H

#include "Tourmaline_Global.h"

// Simics includes
extern "C" {
#include "simics/api.h"
}

// simics memory access
integer_t SIMICS_read_physical_memory(int procID, physical_address_t address,
                                      int len );
void SIMICS_write_physical_memory( int procID, physical_address_t address,
                                   integer_t value, int len );

// simics PC, IC
integer_t SIMICS_get_insn_count( int cpuNumber );
logical_address_t SIMICS_get_program_counter(conf_object_t * obj);

// simics registers
int SIMICS_get_register_number(int cpuNumber, const char * reg_name);
const char * SIMICS_get_register_name(int cpuNumber, int reg_num);
uinteger_t SIMICS_read_register(int cpuNumber, int registerNumber);
void SIMICS_write_register(int cpuNumber, int registerNumber, uinteger_t value);

// simics processor number
int SIMICS_number_processors( void );
processor_t * SIMICS_current_processor( void );
int SIMICS_current_processor_number( void );
int SIMICS_get_proc_no( conf_object_t *cpu );
conf_object_t* SIMICS_get_proc_ptr( int cpuNumber );

// Simics Progress control
void SIMICS_disable_processor( int cpuNumber );
void SIMICS_enable_processor( int cpuNumber );
bool SIMICS_cpu_enabled( int cpuNumber );
cycles_t SIMICS_cycle_count( int cpuNumber );
void SIMICS_break_simulation( const char * msg = "Tourmaline_Break" );

// Installing/uninistalling timing / snooping models
void SIMICS_install_timing_model();
void SIMICS_install_snoop_model();
void SIMICS_remove_timing_model();
void SIMICS_remove_snoop_model();

// data manipualation
uinteger_t SIMICS_get_data_from_memtrans(void * p_v9trans);
void SIMICS_set_data_from_memtrans(void * p_v9trans, uinteger_t newData);

// Simics version printing
void SIMICS_print_version(ostream& out);

#endif //INTERFACE_H
