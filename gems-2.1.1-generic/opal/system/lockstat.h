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
#ifndef _LOCKSTAT_H_
#define _LOCKSTAT_H_
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
* Statistics counting object for locks in the system.
*
* @see     sysstat_t
* @author  cmauer
* @version $Id: lockstat.h 1.4 04/08/06 13:31:53-05:00 lyen@opus.cs.wisc.edu $
*/
class lock_stat_t {
public:
  /**
   * @name Constructor(s) / destructor
   */
  //@{

  /**
   * Constructor: creates object
   */
  lock_stat_t( void );
  /**
   * Destructor: frees object.
   */
  ~lock_stat_t( void );
  //@}

  /**
   * @name Methods
   */
  //@{
  /// register a load operation on this lock
  void lock_read( pseq_t *pseq, load_inst_t *loadop );

  /// register a store operation on this lock
  void lock_write( pseq_t *pseq, store_inst_t *storeop );

  /// register an atomic operation on this lock
  void lock_atomic( pseq_t *pseq, atomic_inst_t *atomicop );

  /// print the lock statistics
  void print( out_intf_t *stream );
  //@}

private:
  /// physical address of this lock
  pa_t                  m_physical_address;
  /// one virtual address of this lock
  la_t                  m_virtual_address;
  histogram_t          *m_length_held;
  histogram_t          *m_atomic_pc;
  histogram_t          *m_store_pc;
  uint32                m_acquire_time;
  uint32                m_releases;
  // count of times that an atomic fails on this address
  uint32                m_atomic_fails;
  // count of number of acquires that are contended
  uint32                m_contended;
  // the number of times the lock is transfered between cpus
  uint32                m_cpu_transfer;
  // true if lock is held
  bool                  m_held;
  // cpu id of last toucher
  int32                 m_last_touched;
  // thread id of last toucher
  la_t                  m_last_thread;
  
public:
  // total number of lock acquires (used for sorting lock stats)
  uint32                m_acquires;
};

/*------------------------------------------------------------------------*/
/* Global variables                                                       */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Global functions                                                       */
/*------------------------------------------------------------------------*/

#endif  /* _LOCKSTAT_H_ */


