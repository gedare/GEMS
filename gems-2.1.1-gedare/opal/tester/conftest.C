
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
 * FileName:  conftest.C
 * Synopsis:  Tests configuration reading and writing
 * Author:    cmauer
 * Version:   $Id$
 */

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include "hfa.h"
#include "hfacore.h"
#include "ipage.h"
#include "ipagemap.h"
#include "confio.h"
#include <stdio.h>
#include <sys/types.h>
extern "C" {
#include "hfa_init.h"
}

extern void simstate_init( void );
extern void simstate_fini( void );

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

// #define TOPADDR (1048576 / 4)
#define OFFSET   0x0000000100000000ULL
#define TOPADDR  (OFFSET + 2 * 1048576)

/*------------------------------------------------------------------------*/
/* Variable declarations                                                  */
/*------------------------------------------------------------------------*/

tick_t g_cycles;

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

#define PS(t) \
   printf("sizeof ( %s ) == %d\n", #t, sizeof(t));

class dumbwait_t : public waiter_t {
  
public:
  dumbwait_t( bool *lock ) {
    m_lock = lock;
  }
  
  virtual ~dumbwait_t( void ) {
  }
  
  void Release() {};
  /** Wakes up the waiter, so he/she can take some action. This Virtual 
   *  Callback must implement some appropriate behavoir for your waiter.
   */
  void Wakeup() {
    *m_lock = true;
  }

  bool *m_lock;
};

/** Read configuration testing.
 */
//**************************************************************************
void readTest( int argc, char *argv[] ) {

  int numcycles;

  if (argc == 3) {
    numcycles = 100;
  } else if (argc == 4) {
    numcycles = atoi( argv[3] );
  } else {
    printf("usage: %s tracedir tracefile numcycles\n", argv[0]);
    exit(1);
  }

  // initialize the state
  simstate_init();

  // initialize our module
  hfa_init_local();

  // add trailing "/" to name (if necessary)
  char *dirname = argv[1];
  int   dirlen = strlen(dirname);
  if ( dirname[dirlen - 1] != '/') {
    dirname = (char *) malloc( dirlen + 2 );
    strcpy( dirname, argv[1] );
    dirname[dirlen]     = '/';
    dirname[dirlen + 1] = '\0';
    argv[1] = dirname;
  }

  // read the configuration
  hfa_allocate( "./config/one-32-L1:1:16K-L2:4:1M-Y10:8-I6.txt" );

  // read the instruction map
  system_t::inst->readInstructionMap( argv[1], argv[2], false );
  // attach to the memory trace
  system_t::inst->attachMemTrace( argv[1], argv[2] );
  // processor state file
  system_t::inst->attachTrace( argv[1], argv[2] );

  // start the simulation
  hfa_sim_start( "/tmp/atest" );

  // load the cache state
  system_t::inst->readConfiguration( "./read-1M.conf" );

  // write the configuration
  system_t::inst->writeConfiguration( "./written-1M.conf" );
}

/** [Memo].
 *  [Internal Documentation]
 */
//**************************************************************************
int main( int argc, char *argv[] ) {

  readTest( argc, argv );
  exit(1);

  PS( byte_t );
  PS( half_t );
  PS( uint16 );
  PS( word_t );
  PS( reg_id_t );
  PS( my_addr_t );
  PS( pa_t );
  PS( ireg_t );
  PS( dynamic_inst_t );
  PS( static_inst_t );
  PS( ipage_t );
  PS( ipagemap_t );

  mshr_t      *l1_mshr;
  /// L1 instruction cache
  l1_cache_t  *l1_inst_cache;
  /// L1 data cache
  l1_cache_t  *l1_data_cache;
  /// L2 miss status holding register structure
  mshr_t      *l2_mshr;
  /// L2 (unified) cache
  ln_cache_t  *l2_cache;

  bool         loop_lock = false;
  dumbwait_t  *dumbwaiter = new dumbwait_t( & loop_lock );

  /* Second level mshr, backed up by main memory */
  l2_mshr = new mshr_t("L2.mshr", /* memory */ NULL, L2_BLOCK_BITS, 
                       MEMORY_DRAM_LATENCY, L2_FILL_BUS_CYCLES,
                       1, 0);
  // L2_MSHR_ENTRIES, L2_STREAM_BUFFERS);
  
  /* Second cache backs up first level MSHR */
  l2_cache = new ln_cache_t("L2.unified", l2_mshr,
                            L2_SET_BITS, L2_ASSOC, (1 << L2_BLOCK_BITS));

  /* First level mshr, backed up my L2 cache */
  l1_mshr  = new mshr_t("L1.mshr", l2_cache, L1_BLOCK_BITS, 
                        L2_LATENCY, L1_FILL_BUS_CYCLES,
                        1, 0);
  // L1_MSHR_ENTRIES, L1_STREAM_BUFFERS);
  
  /* first level instruction and data caches */  
  l1_inst_cache = new l1_cache_t("L1.inst", l1_mshr);
  l1_data_cache = new l1_cache_t("L1.data", l1_mshr);

  confio_t  *m_conf = new confio_t();
  l1_data_cache->registerCheckpoint( m_conf );
  l1_inst_cache->registerCheckpoint( m_conf );
  l2_cache->registerCheckpoint( m_conf );

  // write sequentially to l1 data cache.
  for (pa_t phys_addr = OFFSET; phys_addr < TOPADDR; phys_addr++) {

    printf("physical addr 0x%0llx\n", phys_addr);
    if ( !l1_data_cache->Write( phys_addr, 0, dumbwaiter ) ) {
      loop_lock = false;
      printf("miss");
      while (!loop_lock) {
        printf(".");
        g_cycles++;
        l2_mshr->Cycle();
        l1_mshr->Cycle();
        event_t::DoCycle();
      }

    } else {
      g_cycles++;
      printf("end cycle: %lld\n", g_cycles);
      l2_mshr->Cycle();
      l1_mshr->Cycle();
      event_t::DoCycle();
    }
  }
  printf("done!\n");

  int rc = m_conf->writeConfiguration( "./conf-test" );
  printf("return code %d\n", rc);
  delete m_conf;
}
