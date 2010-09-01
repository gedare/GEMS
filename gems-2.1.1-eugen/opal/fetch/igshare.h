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
#ifndef _IGSHARE_H_
#define _IGSHARE_H_

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include "bitfield.h"
#include "directbp.h"

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Class declaration(s)                                                   */
/*------------------------------------------------------------------------*/


/** Per PC history and predictor table */
class predictor_entry_t {
public:
  /**
   * Constructor: creates object
   * @param pht_entries The log-base-2 number of entries in the history
   */
  predictor_entry_t( uint32 history_size );
  /** Destructor: frees object. */
  ~predictor_entry_t( void );

  /// local history for this PC
  uint32      m_history;
  
  /// Predictor table for this PC
  bitfield_t  m_bitfield;
};

/** Per PC predictor table */
typedef map<la_t, predictor_entry_t *> PerPCTable;


/**
* Implements an infinite (idealized?) gshare predictor.
*
* @author  cmauer
* @version $Id: igshare.h 1.3 04/08/11 16:50:50-05:00 lyen@opus.cs.wisc.edu $
*/
class igshare_t : public direct_predictor_t {

public:


  /**
   * @name Constructor(s) / destructor
   */
  //@{

  /**
   * Constructor: creates object
   * @param history_length  The log-base-2 number of entries in history table
   * @param globalHistory   True == use global history, false == per addr hist
   */
  igshare_t( uint32 history_length, bool globalHistory );

  /**
   * Destructor: frees object.
   */
  virtual ~igshare_t( void );
  //@}

  /**
   * @name Methods
   */
  //@{
  /**  Branch predict on the branch_PC 
   *   @param  branch_PC  The virtual address of the PC.
   *   @param  history    The branch history register associated w/ this pred
   *   @return bool true or false branch prediction
   */
  bool Predict(my_addr_t branch_PC, word_t history, bool staticPred, uint32 proc);

  /**  Update the branch prediction table, based on the actual branch pattern.
   *   @param  taken      True if the last branch was taken.
   */
  void Update(my_addr_t branch_PC, cond_state_t history,
              bool staticPred, bool taken, uint32 proc);

  /** Prints out identifing information about this branch predictor */
  void printInformation( FILE *fp );
  //@}

  /** @name Checkpointing */
  //@{
  /// set_ function for checkpointing
  set_error_t   set_data( attr_value_t *value );
  /// get_ function for checkpointing
  attr_value_t  get_data( void );
  /** register state saving function for the branch predictor */
  int           registerCheckpoint( confio_t *conf );
  //@}

private:

  /// True if we should use the global history
  bool      m_use_global;
  
  /// The length of each history (associated with each PC)
  uint32    m_history_length;

  /// The size of the history tables (2^history_length)
  uint32    m_history_size;
  
  /// A mask for removing all bits, except those used to index the history table
  uint32    m_history_mask;

  /// A hash table for per pc histories
  PerPCTable m_perPCtable;
};

/*------------------------------------------------------------------------*/
/* Global variables                                                       */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Global functions                                                       */
/*------------------------------------------------------------------------*/

#endif  /* _IGSHARE_H_ */


