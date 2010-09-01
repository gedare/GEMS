
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
 * RubyConfig.h
 * 
 * Description: This class has only static members and class methods,
 * and thus should never need to be instantiated.
 *
 * $Id$
 * 
 */

#ifndef RUBYCONFIG_H
#define RUBYCONFIG_H

#include "Global.h"
#define   CONFIG_VAR_FILENAME "config.include"
#include "vardecl.h"
#include "NodeID.h"

#define   MEMORY_LATENCY  RubyConfig::memoryResponseLatency()
#define   ABORT_DELAY  m_chip_ptr->getTransactionManager(m_version)->getAbortDelay()

// Set paramterization
/*
 * This defines the number of longs (32-bits on 32 bit machines,
 * 64-bit on 64-bit AMD machines) to use to hold the set...
 * the default is 4, allowing 128 or 256 different members
 * of the set.
 *
 * This should never need to be changed for correctness reasons,
 * though increasing it will increase performance for larger
 * set sizes at the cost of a (much) larger memory footprint
 *
 */
const int NUMBER_WORDS_PER_SET = 4;

class RubyConfig {
public:

  // CACHE BLOCK CONFIG VARIBLES
  static int dataBlockBits() { return g_DATA_BLOCK_BITS; }
  static int dataBlockBytes() { return g_DATA_BLOCK_BYTES; }
  
  // SUPPORTED PHYSICAL MEMORY CONFIG VARIABLES
  static int pageSizeBits() { return g_PAGE_SIZE_BITS; }
  static int pageSizeBytes() { return g_PAGE_SIZE_BYTES; }
  static int memorySizeBits() { return g_MEMORY_SIZE_BITS; }
  static int64 memorySizeBytes() { return g_MEMORY_SIZE_BYTES; }
  static int memoryModuleBits() { return g_MEMORY_MODULE_BITS; }
  static int64 memoryModuleBlocks() { return g_MEMORY_MODULE_BLOCKS; }

  // returns number of SMT threads per physical processor
  static int numberofSMTThreads() { return g_NUM_SMT_THREADS; }
  // defines the number of simics processors (power of 2)
  static int numberOfProcessors() { return g_NUM_PROCESSORS; }
  static int procsPerChipBits() { return g_PROCS_PER_CHIP_BITS; }
  static int numberOfProcsPerChip() { return g_PROCS_PER_CHIP; }
  static int numberOfChips() { return g_NUM_CHIPS; }

  // MACHINE INSTANIATION CONFIG VARIABLES
  // -------------------------------------
  // L1 CACHE MACHINES
  // defines the number of L1banks - idependent of ruby chips (power of 2)
  // NOTE - no protocols currently supports L1s != processors, just a placeholder
  static int L1CacheBits() { return g_NUM_PROCESSORS_BITS; }
  static int numberOfL1Cache() { return g_NUM_PROCESSORS; }
  static int L1CachePerChipBits() { return procsPerChipBits() ; } // L1s != processors not currently supported
  static int numberOfL1CachePerChip() { return numberOfProcsPerChip(); } // L1s != processors not currently supported
  static int numberOfL1CachePerChip(NodeID myNodeID) { return numberOfL1CachePerChip(); }
  static int L1CacheTransitionsPerCycle() { return L1CACHE_TRANSITIONS_PER_RUBY_CYCLE; }

  // L2 CACHE MACHINES
  // defines the number of L2banks/L2Caches - idependent of ruby chips (power of 2)
  static int L2CacheBits() { return g_NUM_L2_BANKS_BITS; }
  static int numberOfL2Cache() { return g_NUM_L2_BANKS; }
  static int L1CacheNumToL2Base(NodeID L1RubyNodeID);  
  static int L2CachePerChipBits() { return g_NUM_L2_BANKS_PER_CHIP_BITS; }
  static int numberOfL2CachePerChip() { return g_NUM_L2_BANKS_PER_CHIP; }
  static int numberOfL2CachePerChip(NodeID myNodeID) { return numberOfL2CachePerChip(); }
  static int L2CacheTransitionsPerCycle() { return L2CACHE_TRANSITIONS_PER_RUBY_CYCLE; }

  // DIRECTORY/MEMORY MACHINES
  // defines the number of ruby memories - idependent of ruby chips (power of 2)
  static int memoryBits() { return g_NUM_MEMORIES_BITS; }
  static int numberOfDirectory() { return numberOfMemories(); }
  static int numberOfMemories() { return g_NUM_MEMORIES; }
  static int numberOfDirectoryPerChip() { return g_NUM_MEMORIES_PER_CHIP; }
  static int numberOfDirectoryPerChip(NodeID myNodeID) { return g_NUM_MEMORIES_PER_CHIP; }
  static int DirectoryTransitionsPerCycle() { return DIRECTORY_TRANSITIONS_PER_RUBY_CYCLE; }

  // PERSISTENT ARBITER MACHINES
  static int numberOfPersistentArbiter() { return numberOfMemories(); }
  static int numberOfPersistentArbiterPerChip() {return numberOfDirectoryPerChip(); }
  static int numberOfPersistentArbiterPerChip(NodeID myNodeID) {return numberOfDirectoryPerChip(myNodeID); }
  static int PersistentArbiterTransitionsPerCycle() { return L2CACHE_TRANSITIONS_PER_RUBY_CYCLE; }

  // ---- END MACHINE SPECIFIC VARIABLES ----

  // VARIABLE MEMORY RESPONSE LATENCY
  // *** NOTE *** This is where variation is added to the simulation
  // see Alameldeen et al. HPCA 2003 for further details
  static int memoryResponseLatency() { return MEMORY_RESPONSE_LATENCY_MINUS_2+(random() % 5); } 

  static void init();
  static void printConfiguration(ostream& out);

  // Memory Controller
  static int memBusCycleMultiplier () { return MEM_BUS_CYCLE_MULTIPLIER; }
  static int banksPerRank () { return BANKS_PER_RANK; }
  static int ranksPerDimm () { return RANKS_PER_DIMM; }
  static int dimmsPerChannel () { return DIMMS_PER_CHANNEL; }
  static int bankBit0 () { return BANK_BIT_0; }
  static int rankBit0 () { return RANK_BIT_0; }
  static int dimmBit0 () { return DIMM_BIT_0; }
  static int bankQueueSize () { return BANK_QUEUE_SIZE; }
  static int bankBusyTime () { return BANK_BUSY_TIME; }
  static int rankRankDelay () { return RANK_RANK_DELAY; }
  static int readWriteDelay () { return READ_WRITE_DELAY; }
  static int basicBusBusyTime () { return BASIC_BUS_BUSY_TIME; }
  static int memCtlLatency () { return MEM_CTL_LATENCY; }
  static int refreshPeriod () { return REFRESH_PERIOD; }
  static int tFaw () { return TFAW; }
  static int memRandomArbitrate () { return MEM_RANDOM_ARBITRATE; }
  static int memFixedDelay () { return MEM_FIXED_DELAY; }

private:
};

#endif //RUBYCONFIG_H
