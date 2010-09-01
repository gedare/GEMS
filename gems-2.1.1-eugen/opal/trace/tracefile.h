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
#ifndef _TRACEFILE_H_
#define _TRACEFILE_H_

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

/// magic number for processor log files (PC plus data)
const int TRACE_MAGIC_NUM = 0x62902100;

/// trace file control instructions
const int TRACE_PC_TOKEN    =     1;
/// trace file token for observing context switches
const int TRACE_CTX_SWITCH  =     2;
/// trace file memop
const int TRACE_MEMOP_VALUE = 10001;
/// trace file reg_modification format
const int TRACE_REG_BASE    =  1000;
/// trace file maximum register modification format
const int TRACE_REG_MAX     = 10000;

/*------------------------------------------------------------------------*/
/* Class declaration(s)                                                   */
/*------------------------------------------------------------------------*/

/**
* Interface to a binary file which contains a trace (or multiple traces).
*
* This file currently represents a uniprocessor trace - with the virtual PC,
* the instruction at that PC, and a log of all modifications in the processor
* state.
*
* Format:
*     Magic Number
*     Context
*     state before instruction_1
*     instruction_1
*     state before instruction_2 (after instruction_1)
*     instruction_2 ...
*
* @see     pstate_t
* @author  cmauer
* @version $Id: tracefile.h 1.10 03/07/10 18:07:51-00:00 milo@cottons.cs.wisc.edu $
*/
class tracefile_t {

public:

  /**
   * @name Constructor(s) / destructor
   */
  //@{

  /**
   * Constructor: opens a trace file for reading
   * @param traceFileName The name of the file be written
   * @param sim_state     The processor state associated with this trace file
   * @param context_t     A pointer to the current context of this processor
   */
  tracefile_t( char *traceFileName, core_state_t *sim_state,
               context_id_t *context_p );

  /**
   * Constructor: opens a trace file for writing
   * @param traceFileName The name of the file be written
   * @param sim_state     The processor state associated with this trace file
   * @param context       The current context of the processor being traced
   */
  tracefile_t( char *traceFileName, core_state_t *sim_state,
               uint32  context );

  /**
   * Destructor: frees object.
   */
  ~tracefile_t();
  //@}

  /**
   * @name Methods
   */
  //@{

  /** only write the PC (no difference of state)
   *  @return true if the write succeeds, false if not
   */
  bool   writeTracePC( la_t curPC, pa_t physPC, unsigned int nextinstr,
                       uint32 local_time );

  /** write the PC and any difference between the current state and this state
   *  @return true if the write succeeds, false if not
   */
  bool   writeTraceStep( la_t curPC, pa_t physPC, unsigned int nextinstr,
                         uint32 local_time );

  /** write out the new context to the file
   *  @param new_context The context identifier being switched to.
   */
  bool   writeContextSwitch( uint32 new_context );

  /** load the next PC, instruction in the trace.
   *  @param  vpc     a reference to the next virtual pc in the trace
   *  @param  instr   reference to the next instruction in the trace
   *  @param  context possible new context
   *  @return bool true if valid, false if at the end of a trace
   */
  bool   readTraceStep( la_t &vpc, pa_t &ppc, unsigned int &instr,
                        uint32 &localtime, uint32 &context );

  /** debugging: validate the contents of a trace file
   */
  void   validate( void );

  /** sets the pointer to the state of the running simulator.
   *  This can be used to enable a "run ahead" sliding window w/ idealized
   *  execution. do not use this if you don't know what you're doing!
   */
  void   setCoreState( core_state_t *sim_state ) {
    m_sim_state = sim_state;
  }

  /** returns true if the trace is being read
   *  @return bool true if the trace is being read
   */
  bool   isReading( void ) {
    return (m_isReading);
  }

  /** returns false if the trace is being written
   *  @return bool false if the trace is being writting
   */
  bool   isWriting( void ) {
    return (!m_isReading);
  }

  /** returns true when the trace is finished
   *  @return bool true if the trace is finished reading
   */
  bool   isFinished( void ) {
    return (feof(m_fp) != 0);
  }

  //@}

private:
  
  /** read the file until a new PC is seen
   *  @param context Any context changes will modify this variable
   */
  bool    readStateChange( uint32 &context );

  /// read a new program counter
  bool    readPC( la_t &vpc, pa_t &ppc, unsigned int &instr,
                  uint32 &localtime );

  /// The current trace file (being read or written)
  FILE                    *m_fp;

  /// The name of the trace file
  char                    *m_filename;

  /// The state associated with a trace file being written
  core_state_t            *m_state;

  /// A pointer to the current context (on read, we will modify this context)
  uint32                  *m_context;

  /// A pointer to the state of the running simulator (correct state)
  core_state_t            *m_sim_state;

  /// Indicates if this trace is being read or written
  bool                     m_isReading;
};

/*------------------------------------------------------------------------*/
/* Global variables                                                       */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Global functions                                                       */
/*------------------------------------------------------------------------*/

#endif  /* _TRACEFILE_H_ */
