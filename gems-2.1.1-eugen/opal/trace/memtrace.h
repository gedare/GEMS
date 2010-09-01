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
#ifndef _MEMTRACE_H_
#define _MEMTRACE_H_

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

/// magic number for processor log files (PC plus data)
const int MEMTRACE_MAGIC_NUM = 0x06510013;

/// memtrace control instructions
const int MEMTRACE_TOKEN     = 1;

/// number of transactions to keep outstanding
const int MEMTRACE_WINSIZE   = 100;

/*------------------------------------------------------------------------*/
/* Class declaration(s)                                                   */
/*------------------------------------------------------------------------*/

/**
* Writes memory operations out in a trace.
*
* @see     tracefile_t
* @author  cmauer
* @version $Id: memtrace.h 1.6 03/07/10 18:07:51-00:00 milo@cottons.cs.wisc.edu $
*/
class memtrace_t {

public:

  /**
   * @name Constructor(s) / destructor
   */
  //@{

  /** Constructor: opens a trace for reading */
  memtrace_t( char *traceFileName );

  /** Constructor: opens a trace for writing */
  memtrace_t( char *traceFileName, bool doWrite );

  /**
   * Destructor: frees object.
   */
  ~memtrace_t( void );
  //@}

  /**
   * @name Methods
   */
  //@{

  /** write a memory operation to a file
   *  @param curPC    The current program counter.
   *  @param instr    The instruction at this address.
   *  @param localtime The localized time of the transactoin (on this cpu)
   *  @param xaction  The memory transaction to be performed.
   */
  bool   writeTransaction( la_t curPC, uint32 instr, uint32 localtime,
                           transaction_t *xaction );

  /** read the next memory operation from a file */
  bool   readTransaction( transaction_t *xaction );

  /** scan the next N transactions in a file for a particular transaction.
   *  @param localtime The localized time of the transactoin (on this cpu)
   *  @param address  The virtual address to load/store
   *  @param curPC    The current program counter
   *  @param instr    The instruction trying to load/store
   *  @param physical_addr The translated address
   *  @param size     The size of the transaction.
   *  @param data     A pointer to the data, MUST BE ALLOCATED!
   *  @return bool    True if a match is found, false if not
   */
  bool    scanTransaction( uint32 localtime, la_t address, 
                           la_t curPC, uint32 instr, 
                           pa_t &physical_addr,
                           uint16 size,
                           ireg_t *data );
  
  /** advance the transaction window to the next transaction */
  bool   advanceTransaction( void );

  /** advance to a particular time */
  void   advanceToTime( int localtime );
  //@}

  /**
   * @name Accessor(s) / mutator(s)
   */
  //@{
  //@}

private:
  /// The current trace file (being read or written)
  FILE                    *m_fp;

  /// The name of the trace file
  char                    *m_filename;

  /// Indicates if this trace is being read or written
  bool                     m_isReading;

  /// array of current transactions
  transaction_t          **m_window;
};

/*------------------------------------------------------------------------*/
/* Global variables                                                       */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Global functions                                                       */
/*------------------------------------------------------------------------*/

#endif  /* _MEMTRACE_H_ */
