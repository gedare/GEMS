
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
 * $Id$
 *
 * Description: 
 *
 */

#ifndef COMMANDS_H
#define COMMANDS_H

#ifdef SPARC
  #define MEMORY_TRANSACTION_TYPE v9_memory_transaction_t
#else
  #define MEMORY_TRANSACTION_TYPE x86_memory_transaction_t
#endif

int  mh_memorytracer_possible_cache_miss(MEMORY_TRANSACTION_TYPE *mem_trans);
void mh_memorytracer_observe_memory(MEMORY_TRANSACTION_TYPE *mem_trans);

void magic_instruction_callback(void* desc, void * cpu, integer_t val);

void ruby_change_debug_verbosity(char* new_verbosity_str);
void ruby_change_debug_filter(char* new_filter_str);
void ruby_set_debug_output_file (const char * new_filename);
void ruby_set_debug_start_time(char* start_time_str);

void ruby_clear_stats();
void ruby_dump_stats(char* tag);
void ruby_dump_short_stats(char* tag);

void ruby_set_periodic_stats_file(char* filename);
void ruby_set_periodic_stats_interval(int interval);

void ruby_load_caches(char* name);
void ruby_save_caches(char* name);

void ruby_dump_cache(int cpuNumber);
void ruby_dump_cache_data(int cpuNumber, char *tag);

void ruby_set_tracer_output_file (const char * new_filename);
void ruby_xact_visualizer_file (char * new_filename);

void ctrl_exception_start(void* desc, void* cpu, integer_t val);
void ctrl_exception_done(void* desc, void* cpu, integer_t val);

void change_mode_callback(void* desc, void* cpu, integer_t old_mode, integer_t new_mode);
void dtlb_map_callback(void* desc, void* chmmu, integer_t tag_reg, integer_t data_reg);
void dtlb_demap_callback(void* desc, void* chmmu, integer_t tag_reg, integer_t data_reg);
void dtlb_replace_callback(void* desc, void* chmmu, integer_t tag_reg, integer_t data_reg);
void dtlb_overwrite_callback(void* desc, void* chmmu, integer_t tag_reg, integer_t data_reg);

integer_t read_reg(conf_object_t *cpu, const char* reg_name);
void dump_registers(conf_object_t *cpu);

// Needed so that the ruby module will compile, but functions are
// implemented in Rock.C.
//
void rock_exception_start(void* desc, conf_object_t* cpu, integer_t val);
void rock_exception_done(void* desc, conf_object_t* cpu, integer_t val);
decoder_t* ATMTP_create_instruction_decoder();
decoder_t* ATMTP_get_instruction_decoder();

#endif //COMMANDS_H
