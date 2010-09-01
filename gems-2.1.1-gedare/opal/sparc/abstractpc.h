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
#ifndef _ABSTRACTPC_H_
#define _ABSTRACTPC_H_

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Class declaration(s)                                                   */
/*------------------------------------------------------------------------*/

/**
 *  This object represents all the control information that an instruction
 *  needs to be speculatively fetched, and decoded. This includes
 *  the current PC, the next PC, the trap level, and the current window ptr,
 *  and the global set.
 *
 *  Its called the abstract pc, because it reflects "what you need to
 *  fetch an instruction" in a simple machine. Information which is used
 *  so early in the pipeline that you simply must speculate to have it then.
 */
class abstract_pc_t {
public:
  /// Constructor
  abstract_pc_t( void ) {
    init();
  }

  /// init: initalizes members to be invalid
  void init( void ) {
    pc  = (my_addr_t) -1;
    npc = (my_addr_t) -1;
    tl  = (uint16) -1;
    pstate = (uint16) -1;
    cwp = (uint16) -1;
    gset= (uint16) -1;
    asi = (uint16) -1;
  }

  void copy( abstract_pc_t * other){
    pc = other->pc;
    npc = other->npc;
    tl = other->tl;
    pstate = other->pstate;
    cwp = other->cwp;
    gset = other->gset;
    asi = other->asi;
  }
  
  /// The current program counter
  my_addr_t     pc;
  /// The current next program counter
  my_addr_t     npc;
  /// The trap level
  uint16        tl;
  /// The processor state
  uint16        pstate;
  /// The current window pointer
  uint16        cwp;
  /** global set: the global set reflected in the architected register state
   *              see REG_GLOBAL constants for more explaination */
  uint16        gset;
  /// The current ASI
  uint16        asi;
};

/*------------------------------------------------------------------------*/
/* Global variables                                                       */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Global functions                                                       */
/*------------------------------------------------------------------------*/

#endif  /* _ABSTRACTPC_H_ */


