
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
 * RubyConfig.C
 * 
 * Description: See RubyConfig.h
 *
 * $Id$
 *
 */

#include "RubyConfig.h"
#include "protocol_name.h"
#include "util.h"
#include "interface.h"
#include "Protocol.h"

#define CHECK_POWER_OF_2(N) { if (!is_power_of_2(N)) { ERROR_MSG(#N " must be a power of 2."); }}
#define CHECK_ZERO(N) { if (N != 0) { ERROR_MSG(#N " must be zero at initialization."); }}
#define CHECK_NON_ZERO(N) { if (N == 0) { ERROR_MSG(#N " must be non-zero."); }}


void RubyConfig::init()
{
  // MemoryControl:
  CHECK_NON_ZERO(MEM_BUS_CYCLE_MULTIPLIER);
  CHECK_NON_ZERO(BANKS_PER_RANK);
  CHECK_NON_ZERO(RANKS_PER_DIMM);
  CHECK_NON_ZERO(DIMMS_PER_CHANNEL);
  CHECK_NON_ZERO(BANK_QUEUE_SIZE);
  CHECK_NON_ZERO(BANK_BUSY_TIME);
  CHECK_NON_ZERO(MEM_CTL_LATENCY);
  CHECK_NON_ZERO(REFRESH_PERIOD);
  CHECK_NON_ZERO(BASIC_BUS_BUSY_TIME);

  CHECK_POWER_OF_2(BANKS_PER_RANK);
  CHECK_POWER_OF_2(RANKS_PER_DIMM);
  CHECK_POWER_OF_2(DIMMS_PER_CHANNEL);

  CHECK_NON_ZERO(g_MEMORY_SIZE_BYTES);
  CHECK_NON_ZERO(g_DATA_BLOCK_BYTES);
  CHECK_NON_ZERO(g_PAGE_SIZE_BYTES);
  CHECK_NON_ZERO(g_NUM_PROCESSORS);
  CHECK_NON_ZERO(g_PROCS_PER_CHIP);
  if(g_NUM_SMT_THREADS == 0){ //defaults to single-threaded
    g_NUM_SMT_THREADS = 1;
  }
  if (g_NUM_L2_BANKS == 0) {  // defaults to number of ruby nodes
    g_NUM_L2_BANKS = g_NUM_PROCESSORS;
  }
  if (g_NUM_MEMORIES == 0) {  // defaults to number of ruby nodes
    g_NUM_MEMORIES = g_NUM_PROCESSORS;
  }

  CHECK_ZERO(g_MEMORY_SIZE_BITS);
  CHECK_ZERO(g_DATA_BLOCK_BITS);
  CHECK_ZERO(g_PAGE_SIZE_BITS);
  CHECK_ZERO(g_NUM_PROCESSORS_BITS);
  CHECK_ZERO(g_NUM_CHIP_BITS);
  CHECK_ZERO(g_NUM_L2_BANKS_BITS);
  CHECK_ZERO(g_NUM_MEMORIES_BITS);
  CHECK_ZERO(g_PROCS_PER_CHIP_BITS);
  CHECK_ZERO(g_NUM_L2_BANKS_PER_CHIP);
  CHECK_ZERO(g_NUM_L2_BANKS_PER_CHIP_BITS);
  CHECK_ZERO(g_NUM_MEMORIES_BITS);
  CHECK_ZERO(g_MEMORY_MODULE_BLOCKS);
  CHECK_ZERO(g_MEMORY_MODULE_BITS);
  CHECK_ZERO(g_NUM_MEMORIES_PER_CHIP);

  CHECK_POWER_OF_2(g_MEMORY_SIZE_BYTES);
  CHECK_POWER_OF_2(g_DATA_BLOCK_BYTES);
  CHECK_POWER_OF_2(g_NUM_PROCESSORS);
  CHECK_POWER_OF_2(g_NUM_L2_BANKS);
  CHECK_POWER_OF_2(g_NUM_MEMORIES);
  CHECK_POWER_OF_2(g_PROCS_PER_CHIP);

  ASSERT(g_NUM_PROCESSORS >= g_PROCS_PER_CHIP);  // obviously can't have less processors than procs/chip
  g_NUM_CHIPS = g_NUM_PROCESSORS/g_PROCS_PER_CHIP;
  ASSERT(g_NUM_L2_BANKS >= g_NUM_CHIPS);  // cannot have a single L2cache across multiple chips
  
  g_NUM_L2_BANKS_PER_CHIP = g_NUM_L2_BANKS/g_NUM_CHIPS;

  ASSERT(L2_CACHE_NUM_SETS_BITS > log_int(g_NUM_L2_BANKS_PER_CHIP));  // cannot have less than one set per bank
  L2_CACHE_NUM_SETS_BITS = L2_CACHE_NUM_SETS_BITS - log_int(g_NUM_L2_BANKS_PER_CHIP);

  if (g_NUM_CHIPS > g_NUM_MEMORIES) {
    g_NUM_MEMORIES_PER_CHIP = 1;  // some chips have a memory, others don't
  } else {
    g_NUM_MEMORIES_PER_CHIP = g_NUM_MEMORIES/g_NUM_CHIPS;
  }

  g_NUM_CHIP_BITS = log_int(g_NUM_CHIPS);
  g_MEMORY_SIZE_BITS = log_int(g_MEMORY_SIZE_BYTES);
  g_DATA_BLOCK_BITS = log_int(g_DATA_BLOCK_BYTES);
  g_PAGE_SIZE_BITS = log_int(g_PAGE_SIZE_BYTES);
  g_NUM_PROCESSORS_BITS = log_int(g_NUM_PROCESSORS);
  g_NUM_L2_BANKS_BITS = log_int(g_NUM_L2_BANKS);
  g_NUM_L2_BANKS_PER_CHIP_BITS = log_int(g_NUM_L2_BANKS_PER_CHIP);  
  g_NUM_MEMORIES_BITS = log_int(g_NUM_MEMORIES);
  g_PROCS_PER_CHIP_BITS = log_int(g_PROCS_PER_CHIP);

  g_MEMORY_MODULE_BITS = g_MEMORY_SIZE_BITS - g_DATA_BLOCK_BITS - g_NUM_MEMORIES_BITS;
  g_MEMORY_MODULE_BLOCKS = (int64(1) << g_MEMORY_MODULE_BITS);

  if ((!Protocol::m_CMP) && (g_PROCS_PER_CHIP > 1)) {
    ERROR_MSG("Non-CMP protocol should set g_PROCS_PER_CHIP to 1");
  }

  // Randomize the execution
  srandom(g_RANDOM_SEED);
}

int RubyConfig::L1CacheNumToL2Base(NodeID L1CacheNum)
{ 
  return L1CacheNum/g_PROCS_PER_CHIP;
} 

static void print_parameters(ostream& out)
{

#define PARAM(NAME) { out << #NAME << ": " << NAME << endl; }
#define PARAM_UINT(NAME) { out << #NAME << ": " << NAME << endl; }
#define PARAM_ULONG(NAME) { out << #NAME << ": " << NAME << endl; }
#define PARAM_BOOL(NAME) { out << #NAME << ": " << bool_to_string(NAME) << endl; }
#define PARAM_DOUBLE(NAME) { out << #NAME << ": " << NAME << endl; }
#define PARAM_STRING(NAME) { assert(NAME != NULL); out << #NAME << ": " << string(NAME) << endl; }
#define PARAM_ARRAY(PTYPE, NAME, ARRAY_SIZE)   \
  {                                            \
    out << #NAME << ": (";                     \
    for (int i = 0; i < ARRAY_SIZE; i++) {     \
      if (i != 0) {                            \
        out << ", ";                           \
      }                                        \
      out << NAME[i];                          \
    }                                          \
    out << ")" << endl;                        \
  }                                            \

    
#include CONFIG_VAR_FILENAME
#undef PARAM
#undef PARAM_UINT
#undef PARAM_ULONG
#undef PARAM_BOOL
#undef PARAM_DOUBLE
#undef PARAM_STRING
#undef PARAM_ARRAY
}

void RubyConfig::printConfiguration(ostream& out) {
  out << "Ruby Configuration" << endl;
  out << "------------------" << endl;

  out << "protocol: " << CURRENT_PROTOCOL << endl;
  SIMICS_print_version(out);
  out << "compiled_at: " << __TIME__ << ", " << __DATE__ << endl;
  out << "RUBY_DEBUG: " << bool_to_string(RUBY_DEBUG) << endl;

  char buffer[100];
  gethostname(buffer, 50);
  out << "hostname: " << buffer << endl;

  print_parameters(out);
}


