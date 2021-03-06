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
#ifndef _GSHARE_H_
#define _GSHARE_H_

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include "directbp.h"

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Class declaration(s)                                                   */
/*------------------------------------------------------------------------*/

/**
* Implements a basic gshare predictor.
*
* @author  cmauer
* @version $Id: gshare.h 1.3 04/08/11 16:50:50-05:00 lyen@opus.cs.wisc.edu $
*/
class gshare_t : public direct_predictor_t {

public:

  /**
   * @name Constructor(s) / destructor
   */
  //@{

  /**
   * Constructor: creates object
   * @param pht_entries The log-base-2 number of entries in the PHT.
   */
  gshare_t( uint32 pht_entries );

  /**
   * Destructor: frees object.
   */
  virtual ~gshare_t();
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

  /**
   * @name Accessor(s) / mutator(s)
   */
  //@{
  uint32  getPHTSize( void ) {
    return (m_pht_size);
  }
  //@}

  /** @name Static Variables */

private:

  /// Hash the program counter into the finite branch prediction tables
  word_t mungePC(my_addr_t branchPC, uint32 proc) const {
    // return (word_t) ((branchPC >> 2) & 0xffffffff);
    return (word_t) (( (branchPC ^ (proc << 20)) >> 2) & 0xffffffff);
  }

  /// Generate the index for the PHT
  word_t generateIndex(word_t munged_PC, word_t history) {
    return ((munged_PC ^ history) & m_pht_mask);
  }
  
  /// The size of the PHT table
  uint32    m_pht_size;
  
  /// A mask for removing all bits, except those used to index the pht table
  uint32    m_pht_mask;
  
  /// The Pattern History Table (PHT)
  char     *m_pht;
};

/*------------------------------------------------------------------------*/
/* Global variables                                                       */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Global functions                                                       */
/*------------------------------------------------------------------------*/

#endif  /* _GSHARE_H_ */
