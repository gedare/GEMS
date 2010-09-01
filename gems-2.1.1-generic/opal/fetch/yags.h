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
#ifndef _YAGS_H_
#define _YAGS_H_

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include "directbp.h"

/*------------------------------------------------------------------------*/
/* Macro Declarations                                                     */
/*------------------------------------------------------------------------*/

/** The number of tag bits in the exception tables */
typedef byte_t tag_t;

/*------------------------------------------------------------------------*/
/* Class declarations                                                     */
/*------------------------------------------------------------------------*/

/**
* YAGS branch predictor object.
*
* Contains the branch prediction state, updates the state upon retirement,
* predicts branches.
*
* @author  zilles
* @version $Id$
*/
class yags_t : public direct_predictor_t {

public:
  
  /**
   * @name Constructor(s) / destructor
   */
  //@{

  /**
   * Constructor: creates object
   * @param pht_entries The log-base-2 number of entries in the PHT.
   * @param exeception_entries The log-base-2 number of entries in the exception table
   * @param tag_bits The number of tag bits to use in the exception table.
   */

  yags_t( uint32 pht_entries, uint32 exception_entries, uint32 tag_bits );

  /**
   * Destructor: frees object.
   */
  virtual ~yags_t();
  //@}

  /** Initialize the branch predictor's state.
   */
  void InitializeState(cond_state_t *history) const {
    *history = 0;
  }

  /**  Branch predict on the branch_PC 
   *   @return bool true or false branch prediction
   */
  bool Predict(my_addr_t branch_PC, cond_state_t history, bool staticPred, uint32 proc);

  /**  Update the branch prediction table, based on the actual branch pattern.
   */
  void Update(my_addr_t branch_PC, cond_state_t history,
              bool staticPred, bool taken, uint32 proc);

  /** Prints out identifing information about this branch predictor */
  void printInformation( FILE *fp );

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
  /// Hash the program counter into the finite branch prediction tables
  word_t MungePC(my_addr_t branch_PC, uint32 proc) const {
    return (word_t) ((branch_PC >> 2) & 0xffffffff);
    //  return (word_t) (( (branch_PC ^ (proc << 16)) >> 2) & 0xffffffff);
  }

  /// Generate the branch prediction
  word_t GenerateChoice(word_t munged_PC, uint32 proc) {
    //return (munged_PC & m_pht_mask);
    return ( (munged_PC | (proc << 27)) & m_pht_mask);
  }

  /// Generate the index
  word_t GenerateIndex(word_t munged_PC, word_t history) {
    return ((munged_PC ^ history) & m_exception_mask);
  }

  /// Generate the tag
  tag_t GenerateTag(word_t munged_PC, word_t history) {
    return (tag_t) (munged_PC & m_tag_mask);
  }

  /** number of entries in the choice table */
  uint32  m_pht_size;
  /** mask for selecting bits which index in the pht table */
  uint32  m_pht_mask;
  /** number of entries in the exception tables */
  uint32  m_exception_size;
  /** mask for selecting bits which index in the exception table */
  uint32  m_exception_mask;
  /** number of tag bits */
  uint32  m_tag_bits;
  /** mask for selecting the tag bits */
  uint32  m_tag_mask;

  /** pattern history table -- stores bias for PC */
  byte_t *m_pht;

  /** "direction caches" -- stores tagged execeptions:
   *  Taken exception table */
  tag_t  *tkn_tag;
  /** Taken predictor: 2 bit saturating counter */
  byte_t *tkn_pred;
  /** Not Taken exception table */
  tag_t  *n_tkn_tag;
  /** Not Taken exception predictor */
  byte_t *n_tkn_pred;
};

#endif /* _YAGS_H_ */
