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
#ifndef _TRANSACTION_H_
#define _TRANSACTION_H_
/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

/// load transaction
const uint16 TRANS_LOAD    = 0;
/// load transaction
const uint16 TRANS_STORE   = 1;

/// data transaction
const uint16 TRANS_DATA          = 0;
/// instr transaction
const uint16 TRANS_INSTRUCTION   = 1;

/// user transaction
const uint16 TRANS_USER         = 0;
/// supervisor transaction
const uint16 TRANS_SUPERVISOR   = 1;

/// not an atomic trans
const uint16 TRANS_ATOMIC_NONE = 0;
/// atomic trans
const uint16 TRANS_ATOMIC      = 1;
/// end of an atomic trans
const uint16 TRANS_ATOMIC_END  = 2;

/*------------------------------------------------------------------------*/
/* Class declaration(s)                                                   */
/*------------------------------------------------------------------------*/

/**
* Generic memory transaction, which can be stored to a file & read later.
*
* @see     transaction_t, tracefile_t
* @author  cmauer
* @version $Id: transaction.h 1.3 03/01/06 19:40:59-00:00 cmauer@cottons.cs.wisc.edu $
*/
class transaction_t {

public:

  /**
   * @name Constructor(s) / destructor
   */
  //@{

  /**
   * Constructor: creates object
   */
  transaction_t();
  /**
   * Constructor: creates object
   */
  transaction_t( const memory_transaction_t *mem_trans );

  /**
   * Destructor: frees object.
   */
  ~transaction_t();
  //@}

  /**
   * @name Methods
   */
  //@{
  //@}

  /**
   * @name Accessor(s) / mutator(s)
   */
  //@{
  /// program counter of the instruction
  void   setPC( la_t value ) { m_pc = value; }
  /// the actual 32-bit instruction writing this instruction
  void   setInst( uint32 value ) { m_inst = value; }
  /// physical address of this transaction
  void   setPhysical( pa_t value ) { m_physical = value; }
  /// virtual address of this transaction
  void   setVirtual( la_t value ) { m_virtual = value; }
  /// size of this transaction
  void   setSize( uint16 value ) { 
    m_size = value;
    m_num_words = m_size / 8;
    if (m_size % 8 != 0) {
      m_num_words ++;
    }
  }
  /// pointer to the data for this transaction
  void   setData( integer_t *value ) { m_data = value; }

  /// set the load or store
  void   setAction( uint16 value ) { m_action = value; }
  /// set the instruction or data
  void   setIorD( uint16 value ) { m_iord = value; }
  /// set the supervisor or user transaction
  void   setSuper( uint16 value ) { m_issuper = value; }
  /// set the is atomic transaction or not.
  void   setAtomic( uint16 value ) { m_atomic = value; }
  /// set the the local time recorded in the log
  void   setTime( uint32 value ) { m_local_icount = value; }

  /// program counter of the instruction
  la_t   getPC( void ) const { return m_pc; }
  /// the actual 32-bit instruction writing this instruction
  uint32 getInst( void ) const { return m_inst; }
  /// physical address of this transaction
  pa_t   getPhysical( void ) const { return m_physical; }
  /// virtual address of this transaction
  la_t   getVirtual( void ) const { return m_virtual; }
  /// size of this transaction
  uint16 getSize( void ) const { return m_size; }
  /// get the number of words in this transaction
  uint16 getNumWords( void ) const { return m_num_words; }
  /// pointer to the data for this transaction
  integer_t *getData( void ) { return m_data; }
  
  /// return load or store
  uint16   getAction( void ) const { return m_action; }
  /// return instruction or data
  uint16   getIorD( void ) const { return m_iord; }
  /// return supervisor or user transaction
  uint16   getSuper( void ) const { return m_issuper; }
  /// return is atomic transaction or not.
  uint16   getAtomic( void ) const { return m_atomic; }
  /// return the local time recorded in the log
  uint32   getTime( void ) const { return m_local_icount; }
  //@}

  /** @name Static Variables */

private:

  /// program counter of the instruction
  la_t   m_pc;
  /// the actual 32-bit instruction writing this instruction
  uint32 m_inst;
  /// the local time recorded in the log
  uint32 m_local_icount;
  /// physical address of this transaction
  pa_t   m_physical;
  /// virtual address of this transaction
  la_t   m_virtual;
  /// size of this transaction
  uint16 m_size;
  /// number of "integer_t" words
  uint16 m_num_words;
  /// pointer to the data for this transaction
  integer_t *m_data;

  /// action: load or store
  uint16 m_action;
  /// IorD: instruction or data
  uint16 m_iord;
  /// supervisor or user transaction
  uint16 m_issuper;
  /// is atomic transaction or not.
  uint16 m_atomic;

};

/*------------------------------------------------------------------------*/
/* Global variables                                                       */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Global functions                                                       */
/*------------------------------------------------------------------------*/

#endif  /* _TRANSACTION_H_ */


