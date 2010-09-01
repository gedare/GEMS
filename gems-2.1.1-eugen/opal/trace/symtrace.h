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
#ifndef _INCLUDE_H_
#define _INCLUDE_H_

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
* The symbolic trace class helps determine which physical addresses are
* used by which threads (user & context info). It provides a facility to
* do physical-address to symbol table mapping.
*
* @see     system_t
* @author  cmauer
* @version $Id: symtrace.h 1.1 03/06/01 18:51:26-00:00 cmauer@cottons.cs.wisc.edu $
*/
class symtrace_t {

public:
  /**
   * @name Constructor(s) / destructor
   */
  //@{

  /**
   * Constructor: creates object
   * @param numProcs  The number of processors in the system
   */
  symtrace_t( int32 numProcs );

  /**
   * Destructor: frees object.
   */
  ~symtrace_t();
  //@}

  /**
   * @name Methods
   */
  //@{
  /// A instruction memory fetch occured on the cpu
  void memoryAccess( int32 cpu, memory_transaction_t *memop );

  /// A new thread was scheduled on the cpu
  void threadSwitch( int32 cpu, uint64 threadid );

  /// A transaction is complete
  void transactionCompletes( int32 cpu );

  /// print the associated statistics
  void printStats( void );
  
  /// get the process running on a CPU  (-1 if none)
  uint64  getProcess( int32 cpu ) {
    return (m_proc_addr[cpu]);
  }
  //@}

  /**
   * @name Accessor(s) / mutator(s)
   */
  //@{
  /**
   * [Memo].
   *
   * [Documentation]
   * @param param1 [Description of what param1 is]
   * @return [Description of return value]
   */
  //@}

private:
  /// The address of the process currently running on this CPU
  uint64           *m_proc_addr;

  /// The number of instructions last time a context switch occurred
  pc_step_t        *m_step_count;

  /// A map of thread id to thread statistics
  ThreadStatTable   m_thread_stats;

};

/*------------------------------------------------------------------------*/
/* Global variables                                                       */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Global functions                                                       */
/*------------------------------------------------------------------------*/

#endif  /* _INCLUDE_H_ */
