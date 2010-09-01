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
#ifndef _IWINDOW_H_
#define _IWINDOW_H_

/* Description:
 *
 *  Implements a revolving buffer of instruction in different phases of
 *  execution.
 *     F - Fetched
 *     D - Decoded
 *     E - Executing
 *     C - Completed (but not retired)
 *     O - NULL (retired instruction)
 *
 *  -----------------------------------------------
 *  |D|D|F|F|F|F|O|O|O|O|O|O|O|C|C|E|C|C|E|D|D|D|D|
 *  -----------------------------------------------
 *     ^       ^             ^           ^
 *     |       |             |           \last_scheduled
 *     |       \last_fetched \last_retired
 *     \last_decoded
 *
 *  These will always be in window order (newest to oldest):
 *    last_fetched
 *    last_decoded
 *    last_scheduled
 *    last_retired
 *
 * Instructions in E&C stages are in "instruction window", the
 * whole buffer represents the ROB.
 *
 * Note: There is NO physical states for "F", "D", "E", "C", and "O".
 *       They are maintained logically with last_* pointers.
 */

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include "statici.h"
#include "dynamic.h"

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Class declaration(s)                                                   */
/*------------------------------------------------------------------------*/

/**
* Instruction window: a revolving buffer which holds fetched, decoded, 
* executed instructions.
*
* Instructions are fetched in order, and are placed in the instruction
* window. They may be executed out-of-order, but retire in-order. Hence,
* The iwindow contains the instructions in fetch and retire order.
*
* @see     pseq_t
* @author  cmauer
* @version $Id$
*/

class iwindow_t {

public:


  /**
   * @name Constructor(s) / destructor
   */
  //@{

  /**
   * Constructor: creates object
   * @param rob_size  Size of re-order buffer: total # of instructions in machine.
   * @param win_size  Size of scheduling window.
   */
  iwindow_t(uint32 rob_size, uint32 win_size);

  iwindow_t();

  // function used to initialize iwindow object for dynamic array of iwindow_t objects
  void set(uint32 rob_size, uint32 win_size, uint32 proc);     

  /**
   * Destructor: frees object.
   */
  ~iwindow_t();
  //@}

  /**
   * @name Methods
   */
  //@{

  /** return the wrapped index */
  uint32 iwin_wrap( uint32 index ) {
    return index % m_rob_size;
  }
  
  /** increments a window index. */
  uint32 iwin_increment( uint32 index ) {
    if (index == m_rob_size - 1)
      return 0;
    return (index + 1);
  }
  
  /** decrements a window index. */
  uint32 iwin_decrement( uint32 index ) {
    if (index == 0)
      return (m_rob_size - 1);
    return (index - 1);
  }
  
  /** returns a window index where the next instruction will be inserted!
   *  @return windex  The next open index (wrapped around window).
   */
  uint32 getInsertIndex( void );

  /** inserts an instruction in the instruction window.
   *  @param instr    The dynamic instruction
   */
  void            insertInstruction( dynamic_inst_t *instr );

  /** gets the next dynamic instruction to decode:
   *  sideeffect  Increments the last_decoded variable
   *  @return     The dynamic instruction to be decoded.
   */
  dynamic_inst_t *decodeInstruction( void );

  /** gets the next dynamic instruction to schedule:
   *  sideeffect  Increments the last_scheduled variable
   *  @return     The dynamic instruction to be scheduled.
   */
  dynamic_inst_t *scheduleInstruction( void );

  /** gets the next dynamic instruction ready to retire --
   *  returns NULL if there are none.
   *  sideeffect  changes the last_retired variable
   *  @return     The dynamic instruction to be retired, or NULL if none.
   */
  dynamic_inst_t *retireInstruction( void );

  /**
   *   squashes all the instructions in the instruction window back to
   *   the "last_good" indexed instruction. This function also rolls back
   *   the branch predictors state to the last known state.
   *
   *   @param last_good The last instruction to successfully complete in
   *          the window.
   *   @param arch_bp   The architected branch predictors state
   *   @param spec_bp   The speculative branch predictors state
   *   @param num_decoded The number of instructions currently decoded, 
   *          be issued next cycle
   */
  void  squash( pseq_t* the_pseq, int32 last_good, int32 &num_decoded );

  // Blindly squashes all instructions in window. Does not restore register mappings
  void  squash_all(pseq_t * the_pseq);

  //@}

  /**
   * @name Accessor(s) / mutator(s)
   */
  //@{

  /// returns the iwindow size
  uint32 getSize( void ) { return m_rob_size; };

  /// returns the index of the last fetched instruction
  uint32  getLastFetched( void );
  /// returns the index of the last decoded instruction
  uint32  getLastDecoded( void );
  /// returns the index of the last scheduled instruction
  uint32  getLastScheduled( void );
  /// returns the index of the last retired instruction.
  uint32  getLastRetired( void );
  /// returns true if the testNum is in the instruction window
  bool   isInWindow( uint32 testNum );
  /// returns true if the instruction d is the same as the one at position pos
  bool   isAtPosition(dynamic_inst_t *d, uint32 position) {
    return (m_window[iwin_wrap(position)] == d);
  }
  /// returns true if there is space left in the window
  bool   isSlotAvailable( void ) {
    return ( iwin_increment(m_last_fetched) != m_last_retired );
  }

  /// subtract two window indices
  int32  rangeSubtract( uint32 index1, uint32 index2 );

  /// returns how many slots are taken in the iwindow structure
  int32 getNumSlotsTaken();

  /// returns how many instructions are waiting to retire
  int32 getNumCanRetire();

  /// returns how many instructions are waiting to be scheduled
  int32 getNumCanSched();

  /// returns how many instructions are waiting to be decoded
  int32 getNumCanDecode();

  /// Returns a pointer to instr older than "index" and has pc "PC"
  dynamic_inst_t * lookupOlder( const uint32 & index, const la_t & pc);

  /** prints out the contents of the instruction window. */
  void   print( void );
  /** print in more detail */
  void   printDetail( void );
  /** sets the instruction window to "initial" values. */
  void   initialize( void );
  /** debugging only!!: peek into the instruction window.
   *   This has a side effect of incrementing the index!! */
  dynamic_inst_t *peekWindow( int &index );
  //@}

private:
  /// checks to see if lowerNum <= testNum <= upperNum with wrap around
  bool     rangeOverlap( uint32 lowerNum, uint32 upperNum, uint32 testNum );

  /// The revolving buffer of dynamic instructions
  dynamic_inst_t  **m_window;

  /// size of the window(ROB), doesn't have to be power of 2
  uint32    m_rob_size;

  /// size of instruction window, from where instruction can be scheduled
  uint32    m_win_size;
  
  /// last instruction fetched
  uint32     m_last_fetched;
  /// last instruction decoded
  uint32     m_last_decoded;
  /// last instruction queued for execution
  uint32     m_last_scheduled;
  /// last instruction retired
  uint32     m_last_retired;
  
  /// an ID indicating which logical proc this window belongs to
  uint32    m_proc;
};

/*------------------------------------------------------------------------*/
/* Global variables                                                       */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Global functions                                                       */
/*------------------------------------------------------------------------*/

#endif  /* _IWINDOW_H_ */
