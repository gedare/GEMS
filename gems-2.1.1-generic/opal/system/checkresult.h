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
#ifndef _CHECK_RESULT_H_
#define _CHECK_RESULT_H_

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

/// The number of critical registers to check
const uint32 PSEQ_CRITICAL_REG_COUNT = 4;

/*------------------------------------------------------------------------*/
/* Class declaration(s)                                                   */
/*------------------------------------------------------------------------*/

/**
 * Result of comparison between our state and simics state.
 */
class check_result_t {
public:
  /**
   * Constructor: creates object
   */
  check_result_t( void ) {
    critical_check  = true;
    perfect_check   = true;
    update_only     = false;
#ifdef PIPELINE_VIS
    verbose         = true;
#else
    verbose         = false;
#endif

  }

  /** true if no critical check fails.
   *  if false, this forces a squash and state reload */
  bool critical_check;
  /** true if all state checks out correctly.
   *  if false, this causes the incorrect state to be "patched up". */
  bool perfect_check;
  /** true if the check method should update, not check the result */
  bool update_only;
  /** true if differences should be printed */
  bool verbose;
};

/**
 * Container class for facilitating comparisions: the machine state object.
 * This object contains an abstract PC, a register file, and a set of
 * "critical registers" that must be checked.
 */
class mstate_t {
public:
  mstate_t( void ) {
    //each logical proc should have its own abstract PC and critical regs:
    at = (abstract_pc_t *) malloc(sizeof(abstract_pc_t)* CONFIG_LOGICAL_PER_PHY_PROC);
     critical_regs = (reg_id_t **) malloc(sizeof(reg_id_t *) * CONFIG_LOGICAL_PER_PHY_PROC);
     for(uint k=0; k <  CONFIG_LOGICAL_PER_PHY_PROC; ++k){
       at[k].init();            //initialize the PC to be invalid
      critical_regs[k] = (reg_id_t *) malloc(sizeof(reg_id_t) * PSEQ_CRITICAL_REG_COUNT);
      }
  }

  ~mstate_t(){
    if(at)
       free(at);
    if(critical_regs)
       free(critical_regs);
  }
  
  /// abstract register file for idealized functional execution
  reg_box_t           regs;

  /// abstract PC (location for next fetch)
  abstract_pc_t   *    at;
  
  /// "critical" registers (for ideal state) to check each cycle
  reg_id_t       **     critical_regs;
};

/*------------------------------------------------------------------------*/
/* Global variables                                                       */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Global functions                                                       */
/*------------------------------------------------------------------------*/

#endif  /* _CHECK_RESULT_H_ */



