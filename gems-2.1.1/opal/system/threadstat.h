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
#ifndef _THREADSTAT_H_
#define _THREADSTAT_H_

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
* Class to track statistics on a per thread basis.
*
* @see     sys_stat_t
* @author  cmauer
* @version $Id$
*/
class thread_stat_t {

public:


  /**
   * @name Constructor(s) / destructor
   */
  //@{

  /**
   * Constructor: creates object
   * @param address The logical address of this process
   * @param pid     The unique ID of this process
   * @param cpuid   The unique ID of this CPU (processor)
   */
  thread_stat_t( la_t address, int32 pid, int32 cpuid );

  /**
   * Destructor: frees object.
   */
  ~thread_stat_t();
  //@}

  /**
   * @name Methods
   */
  //@{
  void runOnCpu( int32 id );
  //@}

  /// The address of the "thread" object (for this process)
  uint64     m_logical_address;
  /// an internal thread identifier
  int32      m_internal_id;
  /// process id variable
  int32      m_pid;
  /// cpu last scheduled on
  int32      m_cpuid;
  /// count of thread migrations for this thread
  int32      m_migrations;
  /// number of completed transactions
  int32      m_transactions_completed;
  /// last time a transaction was completed
  uint64     m_last_transaction_time;
  /// count of the number of instructions executed for this thread
  uint64     m_execution_count;
  /// A mapping of physical addresses to logical ones (for this thread)
  PhysicalToLogicalMap  m_p2l_map;
  /// A mapping of physical addresses to execution counts (for this thread)
  PhysicalToLogicalMap  m_exec_per_page;
};

/*------------------------------------------------------------------------*/
/* Global variables                                                       */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Global functions                                                       */
/*------------------------------------------------------------------------*/

#endif  /* _THREADSTAT_H_ */


