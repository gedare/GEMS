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
#ifndef _HFA_H_
#define _HFA_H_

using namespace std;

/* 
 * Global include file for entire project.
 * Should be included first in all ".C" project files
 */

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/
#include <string>
#include <map>
#include <set>
#include <list>
#include <fstream>
#include <iostream>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>             // va_start(), va_end()
#include <strings.h>            // declaration of bzero()

#include <sys/time.h>           // gettimeofday() includes
#include <errno.h>
#include <unistd.h>

/*------------------------------------------------------------------------*/
/* Type Includes                                                          */
/*------------------------------------------------------------------------*/

#include "hfatypes.h"

/*------------------------------------------------------------------------*/
/* Forward class declaration(s)                                           */
/*------------------------------------------------------------------------*/

class dynamic_inst_t;
class memory_inst_t;
class control_inst_t;
class load_inst_t;
class store_inst_t;
class abstract_pc_t;
class static_inst_t;

class abstract_rf_t;
class arf_int_t;
class arf_int_global_t;
class arf_single_t;
class arf_double_t;
class arf_cc_t;
class arf_control_t;

class flat_rf_t;
class flat_int_t;
class flat_int_global_t;
class flat_single_t;
class flat_double_t;
class flat_control_t;

class actor_t;
class dp_int_t;
class dp_control_t;
class dp_mem_t;

class cache_t;
class cascaded_indirect_t;
class cghc_t;
class check_result_t;
class chain_t;
class dtlb_t;
class footprint_cache_t;
class iwindow_t;
class ipage_t;
class ipagemap_t;
class generic_cache_block_t;
template <class BlockType> class generic_cache_template;
class memtrace_t;
class miss_t;
class mshr_t;
class page_owner_t;
class physical_file_t;
class pseq_t;
class pstate_t;
class pt_inst_t;
class pt_memory_t;
class ptrace_t;
class ras_t;
class tlstack_t;
class reg_id_t;
class reg_map_t;
class reg_box_t;
class writebuffer_t;
class rubycache_t;
class rubymiss_t;
class scheduler_t;
class sequencer_t;
class stream_t;
class symtrace_t;
class system_t;
class tracefile_t;
class branchfile_t;
class transaction_t;
class yags_t;

class wait_list_t;
class waiter_t;
class free_list_t;
class pipestate_t;
class pipepool_t;

class mem_dependence_t;
class flow_inst_t;
class cfg_list_t;

class lsq_t;
class debugio_t;
class confio_t;

class decode_stat_t;
class histogram_t;
class mem_stat_t;
class thread_stat_t;
class sys_stat_t;
class static_stat_t;
class lock_stat_t;
class stopwatch_t;
class utimer_t;

class power_t;
class storeset_predictor_t;
class regstate_predictor_t;

/// table of thread pointers to statistics
typedef map<la_t, thread_stat_t *> ThreadStatTable;
typedef map<pa_t, la_t>            PhysicalToLogicalMap;
typedef map<uint64, flow_inst_t *>  CFGIndex;
typedef map<uint64, page_owner_t *> PageOwner;

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

// include macros specific to the target system
#include "targetmacros.h"

/// The flat register space
const int    PSEQ_FLATREG_NWIN    = NWINDOWS*16;
/// An invalid physical register number
const uint16 PSEQ_INVALID_REG     = 65535;
/// The %g0 physical register number
const uint16 PSEQ_ZERO_REG        = PSEQ_FLATREG_NWIN;

/** Maximum size of a load or store that may occur to/from the memory system.
 *  (in 64-bit quantities). Currently this is set to 8 * 64-bits = 64-bytes.
 */
const uint32 MEMOP_MAX_SIZE = 8;

/** 64-bit int memory masks */
#define MEM_BYTE_MASK 0x00000000000000ffULL
#define MEM_HALF_MASK 0x000000000000ffffULL
#define MEM_WORD_MASK 0x00000000ffffffffULL
#define MEM_EXTD_MASK 0xffffffffffffffffULL
#define MEM_QUAD_MASK 0xffffffffffffffffULL


/*------------------------------------------------------------------------*/
/* Configuration Parameters                                               */
/*------------------------------------------------------------------------*/

/// The relative path to from simics to opal (used for configuration objects)
extern const char *OPAL_RELATIVE_PATH;

/// pseudo-constant which indicate if we are in simics or not
extern uint32 CONFIG_IN_SIMICS;

/// pseudo-constant which indicates if we are running with ruby or not
extern uint32 CONFIG_WITH_RUBY;

/// pseudo-constant which indicates if we are running multiprocessor or not
extern uint32 CONFIG_MULTIPROCESSOR;

// pseudo-constant: is the host big endian or not
extern uint32 BIG_ENDIAN_HOST;

// pseudo-constant: is the target big endian or not
extern uint32 BIG_ENDIAN_TARGET;

// pseudo-constant: is does the host and target endian match
extern uint32 ENDIAN_MATCHES;

#define  CONFIG_VAR_FILENAME "config.include"
#include "vardecl.h"

#include "system.h"
// define assertions, and debugging output and error routines
// assert is also defined in hfatypes.h
#ifdef OPAL_NO_ASSERT
#define ASSERT(X) {}
#else
#define  ASSERT(X)  \
        if( ((X) == false ) && system_t::inst){   \
            ERROR_OUT("ASSERT_FAILURE_CYCLE[ %lld ]\n", system_t::inst->getGlobalCycle());  \
         }                       \ 
         assert( (X) );
#include <assert.h>
#endif
#define  SIM_HALT  assert(0);         
#include "debugio.h"

//  if( system_t::inst){    \  
//    ERROR_OUT("SIMHALT_FAILURE_CYCLE[ %lld ]\n", system_t::inst->getGlobalCycle());  \   
//   }                   \    

#endif  /* _HFA_H_ */



