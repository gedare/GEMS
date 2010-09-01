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
#ifndef _DECODE_H_
#define _DECODE_H_
/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Class declaration(s)                                                   */
/*------------------------------------------------------------------------*/

/**
* Count statistics related to the decoded instructions.
*
* @author  cmauer
* @version $Id$
*/
class decode_stat_t {

public:
  /// constructor
  decode_stat_t();
  /// destructor
  ~decode_stat_t();

  /// decoded dynamic instruction
  void    seenOp( enum i_opcode op, trap_type_t traptype, dyn_execute_type_t type ) {
    STAT_INC( m_op_seen[op] );
    //break down by what trap memop instruction located in
    int index = (int) traptype;
    if( traptype >= TRAP_NUM_TRAP_TYPES){
      // this means we have no trap...
      ASSERT(traptype == Trap_NoTrap);
      index = 0;   //put into slot 0
    }
    ASSERT(index < TRAP_NUM_TRAP_TYPES);
    m_op_trap_seen[ index ]++;
    if(type == DYN_LOAD){
      m_op_trap_load[ index ]++;
    }
    else if(type == DYN_STORE){
      m_op_trap_store[index]++;
    }
    else if(type == DYN_ATOMIC){
      m_op_trap_atomic[index]++;
    }
  }

  /// correctly! executed the op code
  void    successOp( enum i_opcode op ) {
    STAT_INC( m_op_succ[op] );
  }
  
  /// functional retirement
  void    functionalOp( enum i_opcode op ) {
    STAT_INC( m_op_functional[op] );
  }

  /// opcode caused a pipeline squash
  void    squashOp( enum i_opcode op ) {
    STAT_INC( m_op_squash[op] );
  }

  /// opcodes that experienced L2 data miss
  void    l2missOp( enum i_opcode op, trap_type_t traptype, dyn_execute_type_t type) {
    STAT_INC( m_op_l2miss[op] );
    //also break down by trap type
    int index = (int) traptype;
    if( traptype >= TRAP_NUM_TRAP_TYPES){
      // this means we have no trap...
      ASSERT(traptype == Trap_NoTrap);
      index = 0;   //put into slot 0
    }
    //means we experienced a trap
    ASSERT(index < TRAP_NUM_TRAP_TYPES);
    if(type == DYN_LOAD){
      m_op_trap_load_l2miss[ index ]++;
    }
    else if(type == DYN_STORE){
      m_op_trap_store_l2miss[index]++;
    }
    else if(type == DYN_ATOMIC){
      m_op_trap_atomic_l2miss[index]++;
    }
  }

 /// opcodes that experienced L2 instr miss
  void    l2instrmissOp( enum i_opcode op, trap_type_t traptype, dyn_execute_type_t type) {
    STAT_INC( m_op_l2instrmiss[op] );
    //also break down by trap type
    int index = (int) traptype;
    if( traptype >= TRAP_NUM_TRAP_TYPES){
      // this means we have no trap...
      ASSERT(traptype == Trap_NoTrap);
      index = 0;   //put into slot 0
    }
    //means we experienced a trap
    ASSERT(index < TRAP_NUM_TRAP_TYPES);
    m_op_trap_l2instrmiss[ index ]++;
  }
  
  /// opcodes that were dependent on instr incurring L2 miss
  void depl2missOp( enum i_opcode op) {
    STAT_INC( m_op_depl2miss[op] );
  }

  /// retire event, measuring its (total) memory latency (if any)
  void    opMemoryLatency( enum i_opcode op, uint16 latency );

  /// fetch to decode latency
  void    opDecodeLatency( enum i_opcode op, uint16 latency );

  /// decode to operand ready latency
  void    opOperandsReadyLatency( enum i_opcode op, uint16 latency );

  /// operand ready to execution latency
  void    opSchedulerLatency( enum i_opcode op, uint16 latency );

  /// execute to ready to retire latency
  void    opExecuteLatency( enum i_opcode op, uint16 latency );

  /// execute to 2nd phase of execution (for load stalls)
  void    opContExecutionLatency( enum i_opcode op, uint16 latency );
  
  /// completion to retiring latency
  void    opRetireLatency( enum i_opcode op, uint16 latency );

  /// non-compliant event, unattributable to single instruction
  void    incrementNoncompliant( enum i_opcode op ) {
    STAT_INC( m_op_noncompliant[op] );
  }

  /// print out table
  void    print( out_intf_t *io );
  
private:
  /// The number of unmatched instructions
  uint64             m_num_unmatched;

  /// An array representing the functional instructions.
  uint64            *m_op_functional;
  /// An array representing which instructions have been seen
  uint64            *m_op_seen;
  /// An array of which instructions have been correctly exectuted
  uint64            *m_op_succ;
  /// An array of which instructions squashed
  uint64            *m_op_squash;
  /// An array non-complaint instructions
  uint64            *m_op_noncompliant;
  /// An array of l2 data miss instructions
  uint64            *m_op_l2miss;
  /// for l2 instr misses
  uint64            *m_op_l2instrmiss;
  /// An array of instructions dependent on l2 miss instructions
  uint64            *m_op_depl2miss;

  /// An array of the minimum execute latency of these instructions
  uint64            *m_op_memory_counter;
  /// An array of the maximum execute of these instruction
  uint64            *m_op_memory_latency;

  //**********************Fetch -> Decode*******************************
  uint64 * m_op_min_decode;
  uint64 * m_op_max_decode;
  uint64            *m_op_total_decode;

  //**********************Decode -> Operands ready*******************
  uint64 * m_op_min_operandsready;
  uint64 * m_op_max_operandsready;
  uint64            *m_op_total_operandsready;

  //**********************Operands ready -> execute*******************
  uint64 * m_op_min_scheduler;
  uint64 * m_op_max_scheduler;
  uint64            *m_op_total_scheduler;

  //**********************Execute -> Completion*************************
  /// An array of the minimum execute latencies (fetch->retire)
  uint64            *m_op_min_execute;
  /// An array of the maximum execute latencies
  uint64            *m_op_max_execute;
  /// An array of the minimum total execution latencies of each instructions
  uint64            *m_op_total_execute;

  //**********************Execute -> 2nd phase of execution*************************
  /// An array of the minimum execute latencies (fetch->retire)
  uint64            *m_op_min_contexecute;
  /// An array of the maximum execute latencies
  uint64            *m_op_max_contexecute;
  /// An array of the minimum total execution latencies of each instructions
  uint64            *m_op_total_contexecute;

  
  //**********************Completion -> Retire****************************
  uint64 * m_op_min_retire;
  uint64 * m_op_max_retire;
  uint64            *m_op_total_retire;

  //**************************************************************************
  // Stats that break down for each trap group how many memops retired and how many l2misses
  // totals for each traptype, for all instr seen
  uint64 * m_op_trap_seen;
  //totals for each traptype, by memop
  uint64 * m_op_trap_load;
  uint64 * m_op_trap_store;
  uint64 * m_op_trap_atomic;
  //l2misses for each traptype
  uint64 * m_op_trap_load_l2miss;
  uint64 * m_op_trap_store_l2miss;
  uint64 * m_op_trap_atomic_l2miss;
  //l2instrmisses for each traptype
  uint64 * m_op_trap_l2instrmiss;
};

#endif  /* _DECODE_H_ */
