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
#include "hfa.h"
#include "hfacore.h"
#include "fileio.h"
#include "decode.h"

/// a constant to indicate uninitialized min latency
const uint64  DECODE_DEFAULT_MIN_LATENCY = 10000;

/// constructor
//***************************************************************************
decode_stat_t::decode_stat_t()
{
  m_num_unmatched = 0;
  m_op_seen = (uint64 *) malloc( sizeof(uint64) * (i_maxcount + 1) );
  m_op_succ = (uint64 *) malloc( sizeof(uint64) * (i_maxcount + 1) );
  m_op_functional = (uint64 *) malloc( sizeof(uint64) * (i_maxcount + 1) );
  m_op_squash = (uint64 *) malloc( sizeof(uint64) * (i_maxcount + 1) );
  m_op_noncompliant = (uint64 *) malloc( sizeof(uint64) * (i_maxcount + 1) );
  m_op_l2miss = (uint64 *) malloc(sizeof(uint64) * (i_maxcount+ 1));
  m_op_l2instrmiss = (uint64 *) malloc(sizeof(uint64) * (i_maxcount+ 1));
  m_op_depl2miss = (uint64 *) malloc(sizeof(uint64) * (i_maxcount+ 1));

  m_op_memory_counter = (uint64 *) malloc( sizeof(uint64) * (i_maxcount + 1) );
  m_op_memory_latency = (uint64 *) malloc( sizeof(uint64) * (i_maxcount + 1) );
  m_op_min_execute = (uint64 *) malloc( sizeof(uint64) * (i_maxcount + 1) );
  m_op_max_execute = (uint64 *) malloc( sizeof(uint64) * (i_maxcount + 1) );
  m_op_total_execute = (uint64 *) malloc( sizeof(uint64) * (i_maxcount + 1) );

  m_op_min_contexecute = (uint64 *) malloc( sizeof(uint64) * (i_maxcount + 1) );
  m_op_max_contexecute = (uint64 *) malloc( sizeof(uint64) * (i_maxcount + 1) );
  m_op_total_contexecute = (uint64 *) malloc( sizeof(uint64) * (i_maxcount + 1) );

  m_op_min_decode = (uint64 *) malloc( sizeof(uint64) * (i_maxcount + 1) );
  m_op_max_decode = (uint64 *) malloc( sizeof(uint64) * (i_maxcount + 1) );
  m_op_total_decode = (uint64 *) malloc( sizeof(uint64) * (i_maxcount + 1) );

  m_op_min_operandsready = (uint64 *) malloc( sizeof(uint64) * (i_maxcount + 1) );
  m_op_max_operandsready = (uint64 *) malloc( sizeof(uint64) * (i_maxcount + 1) );
  m_op_total_operandsready = (uint64 *) malloc( sizeof(uint64) * (i_maxcount + 1) );

  m_op_min_scheduler = (uint64 *) malloc( sizeof(uint64) * (i_maxcount + 1) );
  m_op_max_scheduler = (uint64 *) malloc( sizeof(uint64) * (i_maxcount + 1) );
  m_op_total_scheduler = (uint64 *) malloc( sizeof(uint64) * (i_maxcount + 1) );

  m_op_min_retire = (uint64 *) malloc( sizeof(uint64) * (i_maxcount + 1) );
  m_op_max_retire = (uint64 *) malloc( sizeof(uint64) * (i_maxcount + 1) );
  m_op_total_retire = (uint64 *) malloc( sizeof(uint64) * (i_maxcount + 1) );

  // for trap group stats
  m_op_trap_load = new uint64[TRAP_NUM_TRAP_TYPES];
  m_op_trap_store = new uint64[TRAP_NUM_TRAP_TYPES];
  m_op_trap_atomic = new uint64[TRAP_NUM_TRAP_TYPES];
  m_op_trap_load_l2miss = new uint64[TRAP_NUM_TRAP_TYPES];
  m_op_trap_store_l2miss = new uint64[TRAP_NUM_TRAP_TYPES];
  m_op_trap_atomic_l2miss = new uint64[TRAP_NUM_TRAP_TYPES];
  m_op_trap_l2instrmiss =  new uint64[TRAP_NUM_TRAP_TYPES];
  m_op_trap_seen =  new uint64[TRAP_NUM_TRAP_TYPES];

  for(int i=0; i < TRAP_NUM_TRAP_TYPES; ++i){
    m_op_trap_load[i] = 0;
    m_op_trap_store[i] = 0;
    m_op_trap_atomic[i] = 0;
    m_op_trap_load_l2miss[i] = 0;
    m_op_trap_store_l2miss[i] = 0;
    m_op_trap_atomic_l2miss[i] = 0;
    m_op_trap_l2instrmiss[i] = 0;
    m_op_trap_seen[i] = 0;
  }

  for ( int i = 0; i < i_maxcount; i++ ) {
    m_op_seen[i] = 0;
    m_op_succ[i] = 0;
    m_op_functional[i] = 0;
    m_op_squash[i] = 0;
    m_op_noncompliant[i] = 0;
    m_op_l2miss[i] = 0;
    m_op_depl2miss[i] = 0;
    m_op_l2instrmiss[i] = 0;

    m_op_min_execute[i] = DECODE_DEFAULT_MIN_LATENCY;
    m_op_max_execute[i] = 0;
    m_op_total_execute[i] = 0;

    m_op_min_contexecute[i] = DECODE_DEFAULT_MIN_LATENCY;
    m_op_max_contexecute[i] = 0;
    m_op_total_contexecute[i] = 0;

    m_op_min_decode[i] = DECODE_DEFAULT_MIN_LATENCY;
    m_op_max_decode[i] = 0;
    m_op_total_decode[i] = 0;

    m_op_min_operandsready[i] = DECODE_DEFAULT_MIN_LATENCY;
    m_op_max_operandsready[i] = 0;
    m_op_total_operandsready[i] = 0;

    m_op_min_scheduler[i] = DECODE_DEFAULT_MIN_LATENCY;
    m_op_max_scheduler[i] = 0;
    m_op_total_scheduler[i] = 0;

    m_op_min_retire[i] = DECODE_DEFAULT_MIN_LATENCY;
    m_op_max_retire[i] = 0;
    m_op_total_retire[i] = 0;

    m_op_memory_counter[i] = 0;
    m_op_memory_latency[i] = 0;
  }
}

/// destructor
//***************************************************************************
decode_stat_t::~decode_stat_t()
{
  free( m_op_seen );
  free( m_op_succ );
  free( m_op_functional );
  free( m_op_squash );
  free (m_op_noncompliant);
  free( m_op_l2miss );
  free( m_op_depl2miss );
  free(m_op_l2instrmiss);

  free (m_op_min_execute);
  free (m_op_max_execute);
  free (m_op_total_execute);

  free (m_op_min_contexecute);
  free (m_op_max_contexecute);
  free (m_op_total_contexecute);

  free (m_op_min_decode);
  free (m_op_max_decode);
  free (m_op_total_decode);

  free (m_op_min_operandsready);
  free (m_op_max_operandsready);
  free (m_op_total_operandsready);

  free (m_op_min_scheduler);
  free (m_op_max_scheduler);
  free (m_op_total_scheduler);

  free (m_op_min_retire);
  free (m_op_max_retire);
  free (m_op_total_retire);

  free (m_op_memory_counter);
  free (m_op_memory_latency);

  if(m_op_trap_load){
    delete [] m_op_trap_load;
    m_op_trap_load = NULL;
  }
  if(m_op_trap_store){
    delete [] m_op_trap_store;
    m_op_trap_store = NULL;
  }
  if(m_op_trap_atomic){
    delete [] m_op_trap_atomic;
    m_op_trap_atomic = NULL;
  }
  if(m_op_trap_load_l2miss){
    delete [] m_op_trap_load_l2miss;
    m_op_trap_load_l2miss = NULL;
  }
  if(m_op_trap_store_l2miss){
    delete [] m_op_trap_store_l2miss;
    m_op_trap_store_l2miss = NULL;
  }
  if(m_op_trap_atomic_l2miss){
    delete [] m_op_trap_atomic_l2miss;
    m_op_trap_atomic_l2miss = NULL;
  }
  if(m_op_trap_l2instrmiss){
    delete [] m_op_trap_l2instrmiss;
    m_op_trap_l2instrmiss = NULL;
  }
  if(m_op_trap_seen){
    delete [] m_op_trap_seen;
    m_op_trap_seen = NULL;
  }
}

//***************************************************************************
void decode_stat_t::opMemoryLatency( enum i_opcode op, uint16 latency )
{
  m_op_memory_counter[op]++;
  m_op_memory_latency[op] += latency;
}

//***************************************************************************
void decode_stat_t::opExecuteLatency( enum i_opcode op, uint16 latency )
{
  if (m_op_min_execute[op] > latency) {
    m_op_min_execute[op] = latency;
  }
  if (m_op_max_execute[op] < latency) {
    m_op_max_execute[op] = latency;
  }
  m_op_total_execute[op] += latency;
}

//***************************************************************************
void decode_stat_t::opContExecutionLatency( enum i_opcode op, uint16 latency )
{
  if (m_op_min_contexecute[op] > latency) {
    m_op_min_contexecute[op] = latency;
  }
  if (m_op_max_contexecute[op] < latency) {
    m_op_max_contexecute[op] = latency;
  }
  m_op_total_contexecute[op] += latency;
}

//***************************************************************************
void decode_stat_t::opDecodeLatency( enum i_opcode op, uint16 latency )
{
  if (m_op_min_decode[op] > latency) {
    m_op_min_decode[op] = latency;
  }
  if (m_op_max_decode[op] < latency) {
    m_op_max_decode[op] = latency;
  }
  m_op_total_decode[op] += latency;
}

//***************************************************************************
void decode_stat_t::opOperandsReadyLatency( enum i_opcode op, uint16 latency )
{
  if (m_op_min_operandsready[op] > latency) {
    m_op_min_operandsready[op] = latency;
  }
  if (m_op_max_operandsready[op] < latency) {
    m_op_max_operandsready[op] = latency;
  }
  m_op_total_operandsready[op] += latency;
}

//***************************************************************************
void decode_stat_t::opSchedulerLatency( enum i_opcode op, uint16 latency )
{
  if (m_op_min_scheduler[op] > latency) {
    m_op_min_scheduler[op] = latency;
  }
  if (m_op_max_scheduler[op] < latency) {
    m_op_max_scheduler[op] = latency;
  }
  m_op_total_scheduler[op] += latency;
}

//***************************************************************************
void decode_stat_t::opRetireLatency( enum i_opcode op, uint16 latency )
{
  if (m_op_min_retire[op] > latency) {
    m_op_min_retire[op] = latency;
  }
  if (m_op_max_retire[op] < latency) {
    m_op_max_retire[op] = latency;
  }
  m_op_total_retire[op] += latency;
}

/// print out table
//***************************************************************************
void    decode_stat_t::print( out_intf_t *io )
{
  int64 total_seen = 0;
  int64 total_succ = 0;
  int64 total_functional = 0;
  int64 total_squash = 0;
  int64 total_noncompliant = 0;
  int64 total_l2miss = 0;
  int64 total_depl2miss = 0;
  int64 total_l2instrmiss = 0;
  char  buf[40], buf1[40], buf2[40], buf3[40], buf4[40], buf5[40], buf6[40], buf7[40], buf8[40], buf9[40], buf10[40], buf11[40], buf12[40], buf13[40];

  io->out_info("###: decode              seen        success function     fail  L2InstrMiss L2DataMiss  Dep_L2DataMiss\n");
  io->out_info( "Unmatched     %lld\n", m_num_unmatched);
  for (int i = 0; i < (int) i_maxcount; i++) {
    if ( m_op_seen[i] || m_op_succ[i] || m_op_functional[i]) {
      io->out_info(  "%03d: %-9.9s %14.14s %14.14s %8.8s %8.8s %8.8s %8.8s %8.8s\n",
               i, 
               decode_opcode((enum i_opcode) i), 
               commafmt( m_op_seen[i], buf, 40 ),
               commafmt( m_op_succ[i] - m_op_noncompliant[i], buf1, 40 ),
               commafmt( m_op_functional[i], buf2, 40 ),
               commafmt( (m_op_seen[i] - m_op_succ[i] + m_op_noncompliant[i]), buf3, 40) ,
               commafmt( m_op_l2instrmiss[i], buf12, 40 ),
               commafmt( m_op_l2miss[i], buf4, 40 ),
               commafmt( m_op_depl2miss[i], buf5, 40 )
              );
      total_seen += m_op_seen[i];
      total_succ += m_op_succ[i];
      total_functional += m_op_functional[i];
      total_noncompliant += m_op_noncompliant[i];
      total_l2miss += m_op_l2miss[i];
      total_depl2miss += m_op_depl2miss[i];
      total_l2instrmiss += m_op_l2instrmiss[i];
    }
  }

  io->out_info( "TOTALI    : %14.14s %14.14s %8.8s %8.8s %8.8s %8.8s %8.8s\n",
                  commafmt( total_seen, buf, 40 ),
                  commafmt( (total_succ - total_noncompliant), buf1, 40 ),
                  commafmt( total_functional, buf2, 40 ),
                  commafmt( (total_seen - total_succ + total_noncompliant),
                            buf3, 40 ), 
                commafmt( total_l2instrmiss, buf12, 40),
                commafmt( total_l2miss, buf4, 40 ),
                commafmt( total_depl2miss, buf5, 40 )
                );
  io->out_info( "NON_COMP  : %14.14s\n",
                  commafmt( total_noncompliant, buf, 40 ) );
  io->out_info( "Percent functional: %f\n",
                  100.0 * ((float) (total_functional) / (float) total_seen));
  io->out_info( "Percent correct      : %f\n",
                  100.0 * ((float) (total_succ - total_noncompliant) / (float) total_seen));
  
  io->out_info( "\n*** Latency and Squashes\n");
  io->out_info( "\n000: %-9.9s %10.10s %10.10s mem latency  min max avg exec\n", "opcode", "# squashed", "# non-comp", "min" );
  for (int i = 0; i < (int) i_maxcount; i++) {
    // 
    if ( m_op_seen[i] &&
         m_op_min_execute[i] != DECODE_DEFAULT_MIN_LATENCY ) {
      double average_mem_latency = 0.0;
      if (m_op_memory_counter[i] != 0) {
        average_mem_latency = (float) m_op_memory_latency[i] / (float) m_op_memory_counter[i];
      }
      double average_exec_latency = 0.0;
      if (m_op_seen[i] != 0) {
        average_exec_latency = ((float) m_op_total_execute[i]/(float)m_op_seen[i]);
      }
      io->out_info(  "%03d: %-9.9s %10.10s %10.10s %6lld %6.1f  %3lld %3lld %6.1f\n",
                       i, decode_opcode((enum i_opcode) i), 
                       commafmt( m_op_squash[i], buf, 40 ),
                       commafmt( m_op_noncompliant[i], buf1, 40 ),
                       m_op_memory_counter[i],
                       average_mem_latency,
                       
                       m_op_min_execute[i],
                       m_op_max_execute[i],
                       average_exec_latency );
      total_squash += m_op_squash[i];
    }
  }
  io->out_info( "SQUASHED  : %14.14s\n", 
                  commafmt( total_squash, buf, 40 ) );

  //***********print out detailed latency for each opcode
  io->out_info("\n***Detailed Latencies\n");
  io->out_info("\n   %-9.9s    %10.10s    %10.10s    %10.10s  %10.10s   %10.10s   %10.10s\n", "opcode", "F->D", "D->Rdy", "Rdy->E", "E->contE", "E->Comp", "Comp->R");
  for (int i = 0; i < (int) i_maxcount; i++) {
    if ( m_op_seen[i] &&
         m_op_min_execute[i] != DECODE_DEFAULT_MIN_LATENCY ) {
      double average_decode_latency = 0.0;
      if (m_op_seen[i] != 0) {
        average_decode_latency = ((float) m_op_total_decode[i]/(float)m_op_seen[i]);
      }
      double average_operandsready_latency = 0.0;
      if (m_op_seen[i] != 0) {
        average_operandsready_latency = ((float) m_op_total_operandsready[i]/(float)m_op_seen[i]);
      }
      double average_scheduler_latency = 0.0;
      if (m_op_seen[i] != 0) {
        average_scheduler_latency = ((float) m_op_total_scheduler[i]/(float)m_op_seen[i]);
      }
      double average_contexec_latency = 0.0;
      if (m_op_seen[i] != 0) {
        average_contexec_latency = ((float) m_op_total_contexecute[i]/(float)m_op_seen[i]);
      }
      double average_exec_latency = 0.0;
      if (m_op_seen[i] != 0) {
        average_exec_latency = ((float) m_op_total_execute[i]/(float)m_op_seen[i]);
      }
      double average_retire_latency = 0.0;
      if (m_op_seen[i] != 0) {
        average_retire_latency = ((float) m_op_total_retire[i]/(float)m_op_seen[i]);
      }
      io->out_info(  "%03d: %-9.9s [ %3lld %3lld %6.3f ]    [ %3lld %3lld %6.3f ]    [ %3lld %3lld %6.3f ]    [ %3lld %3lld %6.3f ]   [ %3lld %3lld %6.3f ]    [ %3lld %3lld %6.3f ]\n",
                     i, decode_opcode((enum i_opcode) i), 
                     m_op_min_decode[i],
                     m_op_max_decode[i],
                     average_decode_latency,
                     m_op_min_operandsready[i],
                     m_op_max_operandsready[i],
                     average_operandsready_latency,
                     m_op_min_scheduler[i],
                     m_op_max_scheduler[i],
                     average_scheduler_latency,
                     m_op_min_contexecute[i],
                     m_op_max_contexecute[i],
                     average_contexec_latency,
                     m_op_min_execute[i],
                     m_op_max_execute[i],
                     average_exec_latency,
                     m_op_min_retire[i],
                     m_op_max_retire[i],
                     average_retire_latency
                     );
    }
  }

  // print out Trap group stats 
  io->out_info("\nTrap Group Stats\n");
  io->out_info("======================\n");
  io->out_info("Trap  <<#LD  #ST  #AT  TotalMemop  TotalInstrs>>  <<#LDL2DataMiss  #STL2DataMiss #ATL2DataMiss  TotalL2DataMiss TotalL2InstrMiss>>\n");
  for(int i=0; i < TRAP_NUM_TRAP_TYPES; ++i){
    if(m_op_trap_seen[i] != 0 || m_op_trap_load[i] != 0 || m_op_trap_store[i] != 0 || m_op_trap_atomic[i] != 0){
      uint64 total_memop = m_op_trap_load[i] + m_op_trap_store[i] + m_op_trap_atomic[i];
      uint64 total_l2miss = m_op_trap_load_l2miss[i] + m_op_trap_store_l2miss[i] + m_op_trap_atomic_l2miss[i];
      io->out_info("%s  <<%lld  %lld  %lld %6lld %lld>>   <<%lld  %lld  %lld %6lld %lld>>\n", pstate_t::trap_num_menomic((trap_type_t) i), m_op_trap_load[i], m_op_trap_store[i], m_op_trap_atomic[i], total_memop, m_op_trap_seen[i], m_op_trap_load_l2miss[i], m_op_trap_store_l2miss[i], m_op_trap_atomic_l2miss[i], total_l2miss, m_op_trap_l2instrmiss[i]);
    }
  }
  
}
