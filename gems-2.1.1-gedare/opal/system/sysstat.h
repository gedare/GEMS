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
#ifndef _SYSSTATS_H_
#define _SYSSTATS_H_

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

/// table mapping data addresses to lock table statistics
typedef map<la_t, lock_stat_t *>   PerPCLockTable;

/*------------------------------------------------------------------------*/
/* Class declaration(s)                                                   */
/*------------------------------------------------------------------------*/

/**
* Container class for system-wide statistics, including lock statistics,
* thread statistics, and global symbols.
*
* @see     system_t
* @author  cmauer
* @version $Id$
*/
class sys_stat_t {

public:
  /**
   * @name Constructor(s) / destructor
   */
  //@{

  /**
   * Constructor: creates object
   * @param numProcs The number of processors in the system.
   */
  sys_stat_t( int32 numProcs);

  /**
   * Destructor: frees object.
   */
  ~sys_stat_t();
  //@}

  /**
   * @name Methods
   */
  //@{
  /// observe retired instruction stream
  void  observeInstruction( pseq_t *pseq, dynamic_inst_t *dinstr );

  /// observe static instruction stream
  /// Luke
  void  observeStaticInstruction( pseq_t *pseq, static_inst_t *s_instr, uint32 proc );

  /// observe thread switch
  /// Luke
  ///  arg id is the index in the m_seq and m_state arrays, arg proc is the logical
  ///  proc number
  void  observeThreadSwitch( int32 id, uint32 proc );

  /// observe I/O space read or write
  void  observeIOAction( memory_transaction_t *mem_op );

  /// observe when transactions complete
  void  observeTransactionComplete( int32 id, uint32 proc );

  /// print statistics
  void  printStats( void );
  //@}

private:
  /// memory statistics table
  mem_stat_t        *m_mem_stat;
  /// thread statistics table
  ThreadStatTable   *m_thread_stat_table;
  /// A special thread for accounting for I/O device behavoir
  thread_stat_t     *m_io_thread;
  /// per pc lock table: statistics on lock duration
  PerPCLockTable    *m_pc_lock_table;
  /// static instruction addresses of things known to _not_ be locks
  PerPCLockTable    *m_exclude_lock_table;
  /// data instruction of interest:
  la_t               m_trace_address;
  /// physical address of interest:
  pa_t               m_trace_phys_address;
};

/*------------------------------------------------------------------------*/
/* Global variables                                                       */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Global functions                                                       */
/*------------------------------------------------------------------------*/

#endif  /* _SYSSTATS_H_ */


