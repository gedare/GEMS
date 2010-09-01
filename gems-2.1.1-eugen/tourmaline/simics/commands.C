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
 * commands.C
 *
 * Description: 
 * See commands.h
 * 
 * $Id: commands.C 1.00 05/11/09 14:02:03-05:00 gibson@mustardseed.cs.wisc.edu $
 *
 */

#include "Tourmaline_Global.h"
#include "TransactionControl.h"
#include "MemoryAccess.h"
#include "Profiler.h"
#include "interface.h"

// Simics includes
extern "C" {
#include "simics/api.h"
}

//////////////////////// extern "C" api ////////////////////////////////

extern "C" 
void tourmaline_observe_memory(memory_transaction_t *mem_trans)
{
  if(g_trans_control==NULL) {
    WARNING("g_trans_control is NULL!\nDid you forget to issue \"tourmaline0.init\"?\n");
  }
 
  assert(g_trans_control!=NULL);

  if(mem_trans->s.type == Sim_Trans_Instr_Fetch) {
    return; // do nothing for IFetches
  }
  
  // physical address being accessed
  physical_address_t pa = mem_trans->s.physical_address;
  
  // Detemine the type of access
  MemoryAccessType type;
  switch(mem_trans->s.type) {
  case Sim_Trans_Load:
    if(mem_trans->s.atomic) {
      type = MemoryAccessType_ATOMIC_R;
    } else {
      type = MemoryAccessType_LD;
    }
    break;
  case Sim_Trans_Store:
    if(mem_trans->s.atomic) {
      type = MemoryAccessType_ATOMIC_W;
    } else {
      type = MemoryAccessType_ST;
    }
    break;
  default:
    ERROR( "Unrecognized memory transaction type: not LD or ST: %i\n",mem_trans->s.type);
    return; // do nothing ( wouldn't know what to do anyway )
  }

  // Determine access size
  unsigned int size = mem_trans->s.size;

  // Determine originating program counter
  logical_address_t pc;
  unsigned int proc;
  if(mem_trans->s.ini_type == Sim_Initiator_CPU ||
     (mem_trans->s.ini_type >= Sim_Initiator_CPU_V9 && mem_trans->s.ini_type <=Sim_Initiator_CPU_UV)) {
    pc = SIMICS_get_program_counter(mem_trans->s.ini_ptr);
    proc = SIMICS_get_proc_no(mem_trans->s.ini_ptr);
  } else {
    pc = 0;
    proc = 0; // probably shouldn't have this be an error case
  }

  MemoryAccessOriginType origin = MemoryAccessOriginType_SNOOP_INTERFACE;
  
  // Create the MemoryAccess object
  MemoryAccess access(pa, type, size, pc, proc, origin, (void*) mem_trans);

  // Pass the memory access to the transaction controller
  g_trans_control->finishRequest(access);
  
}

extern "C" 
cycles_t tourmaline_operate_memory(memory_transaction_t *mem_trans)
{
  if(g_trans_control==NULL) {
    WARNING("g_trans_control is NULL!\nDid you forget to issue \"tourmaline0.init\"?\n");
  }
  
  assert(g_trans_control!=NULL);
  if(mem_trans->s.type == Sim_Trans_Instr_Fetch) {
    return 0; // do nothing for IFetches
  }
  
  // physical address being accessed
  physical_address_t pa = mem_trans->s.physical_address;
  
  // Detemine the type of access
  MemoryAccessType type;
  switch(mem_trans->s.type) {
  case Sim_Trans_Load:
    if(mem_trans->s.atomic) {
      type = MemoryAccessType_ATOMIC_R;
    } else {
      type = MemoryAccessType_LD;
    }
    break;
  case Sim_Trans_Store:
    if(mem_trans->s.atomic) {
      type = MemoryAccessType_ATOMIC_W;
    } else {
      type = MemoryAccessType_ST;
    }
    break;
  default:
    ERROR( "Unrecognized memory transaction type: not LD or ST: %i\n",mem_trans->s.type);
    return 0; // do nothing ( wouldn't know what to do anyway )
  }

  // Determine access size
  unsigned int size = mem_trans->s.size;

  // Determine originating program counter
  logical_address_t pc;
  unsigned int proc;
  if(mem_trans->s.ini_type == Sim_Initiator_CPU ||
     (mem_trans->s.ini_type >= Sim_Initiator_CPU_V9 && mem_trans->s.ini_type <=Sim_Initiator_CPU_UV)) {
    pc = SIMICS_get_program_counter(mem_trans->s.ini_ptr);
    proc = SIMICS_get_proc_no(mem_trans->s.ini_ptr);
  } else {
    pc = 0;
    proc = 0; // probably shouldn't have this be an error case
  }

  MemoryAccessOriginType origin = MemoryAccessOriginType_TIMING_INTERFACE;
  
  // Create the MemoryAccess object
  MemoryAccess access(pa, type, size, pc, proc, origin, (void*) mem_trans);
  
  // Pass the memory access to the transaction controller
  g_trans_control->makeRequest(access);
  
  return 0;
}

extern "C" 
void magic_instruction_callback(void* desc, void* cpu, integer_t val)
{
  // Use magic callbacks to start and end transactions

  if(g_trans_control==NULL) {
    WARNING("g_trans_control is NULL!\nDid you forget to issue \"tourmaline0.init\"?\n");
  }
  
  assert(g_trans_control!=NULL);

  val = val >> 16;
  if (val == 3) {
    // printf("Observing the beginning of a work transaction.\n");
  } else if (val == 4) {
    ; // magic breakpoint
  } else if (val == 5) {
    // printf("Observing the end of a work transaction.\n");
  } else if (val >=20 && val < 40) { // begin TM-Transaction
    g_trans_control->beginTransaction(SIMICS_current_processor_number(),val-20);

  } else if (val >=40 && val < 60) { // end TM-transaction
    g_trans_control->endTransaction(SIMICS_current_processor_number(),val-40);

  } else {
    WARNING("Unexpected magic call, val = %i\n",val);
  }
}

/* -- Clear stats */
extern "C"
void tourmaline_clear_stats()
{
  if(g_profiler==NULL) {
    ERROR(" Cannot clear stats: No profiler exists. Have you invoked \"tourmalin0.init\"?\n");
    return;
  }
  
  cout << "Clearing stats..." << flush;
  g_profiler->clearStats();
  cout << "Done." << endl << flush;
}

/* -- Dump stats */
extern "C"
// File name, dump to file
void tourmaline_dump_stats(char* filename)
{
  if(g_profiler==NULL) {
    ERROR(" Cannot dump stats: No profiler exists. Have you invoked \"tourmalin0.init\"?\n");
    return;
  }
  
  if(filename==NULL) {
    g_profiler->printStats(cout);
  } else {
    cout << "Dumping stats to output file '" << filename << "'..." << flush;
    ofstream outfile;
    outfile.open(filename);
    if(outfile==NULL) {
      cerr << "Failed." << endl << "Error opening file '" << filename << "' with write permission." << endl << flush;
      return;
    }

    g_profiler->printStats(outfile);
    outfile.close();
    cout << "Done." << endl << flush;
  }
}

