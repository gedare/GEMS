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
#ifndef _CONTROLOP_H_
#define _CONTROLOP_H_
/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include "dynamic.h"

/*------------------------------------------------------------------------*/
/* Class declarations                                                     */
/*------------------------------------------------------------------------*/

/**
* An "in-flight" control instruction in the processor model.
*
* @see     dynamic_inst_t, waiter_t
* @author  zilles
* @version $Id$
*/
class control_inst_t : public dynamic_inst_t {

public:
  /** @name Constructor(s) / Destructor */
  //@{
  /**
   *  Constructor: uses default constructor
   */
  control_inst_t( static_inst_t *s_inst, 
                  int32 window_index,
                  pseq_t *pseq,
                  abstract_pc_t *fetch_at,
                  pa_t physicalPC,
                  trap_type_t trapgroup,
                  uint32 proc
                  );

  /**
   * Destructor: uses default destructor.
   */
  virtual ~control_inst_t();

  /** Allocates object throuh allocator interface */
  void *operator new( size_t size ) {
    return ( m_myalloc.memalloc( size ) );
  }
  
  /** frees object through allocator interface */
  void operator delete( void *obj ) {
    m_myalloc.memfree( obj );
  }
  //@}

  /** execute: modify the machine's state by executing this instruction
   *  implemented differently by control, exec, mem instrs
   */
  void Execute();
  /** squash an instruction in the pipe. As it rolls back the state it
   *  is implemented differently by control, exec, mem instrs.
   */
  void Squash();

  /** retire: change the machines in-order architected state according
   *  to an instruction.
   *  (implemented differently by control, exec, mem instrs).
   *  @param a   The next in-order pc, when this instruction is retired.
   */
  void Retire( abstract_pc_t *a );

  /// get the "last good" copy of the predictor state out of this instruction
  predictor_state_t &getPredictorState() {
    return m_pred_state;
  }
  
  /// makes a copy of speculative predictor state to enable 'ideal' restores
  void              setPredictorState(predictor_state_t &p) {
    m_pred_state = p;
  }
  
  /// Stores the predicted PC for later comparision with the actual PC
  void              setPredictedTarget(abstract_pc_t *a) {
    m_predicted = *a;
  }

  /// Stores if this branch is taken or not
  void              setTaken( bool taken ) {
    m_isTaken = taken;
  }

  /** returns a pointer to the actual target information.
   *  NOTE: The actual target is initialized with the abstract PC state
   *        prior to this control instructions execution. If you don't
   *        modify a field, it will be the same as the previous instructions!
   */
  abstract_pc_t    *getActualTarget( void ) {
    return (&m_actual);
  }

  /// print out detailed information
  void        printDetail( void );

  /** @name memory allocation:: */
  //@{
  static listalloc_t m_myalloc;
  //@}

private:
  /** predictor state after this instruction */
  predictor_state_t m_pred_state;
  /** The predicted target of this control inst */
  abstract_pc_t     m_predicted;

  /** The actual target (PC, nPC pair) after execution */
  abstract_pc_t     m_actual;
  /** true if the branch was taken, false if not */
  bool              m_isTaken;
};

#endif /* _CONTROLOP_H_ */
