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
#ifndef _INDIRECT_H_
#define _INDIRECT_H_

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Parameters                                                             */
/*------------------------------------------------------------------------*/

typedef word_t indirect_state_t;

/*------------------------------------------------------------------------*/
/* Class declarations                                                     */
/*------------------------------------------------------------------------*/

/** cascaded indirect branch predictor
 * @author  zilles
 * @version $Id$
 */
class cascaded_indirect_t {
public:

  /** @name Constructor(s) / Destructor */
  //@{
  /**  Constructor: creates object */
  cascaded_indirect_t();
  /**  Destructor: frees object. */
  ~cascaded_indirect_t();
 //@}

  /** Initialize the branch predictor's state.
   */
  void InitializeState(indirect_state_t *is) const { *is = 0; }

  /**  Branch predict on the branch_PC 
   *   @return bool true or false branch prediction
   */
  my_addr_t Predict(my_addr_t branch_PC, indirect_state_t *is,
                    uint64 *stat_hit, uint32 proc);

  /**  Update the branch prediction table, based on the actual branch pattern.
   */
  void Update( my_addr_t branch_PC, indirect_state_t *is, 
               my_addr_t target_PC, uint32 proc );
  
  /** function used for fixing up history */
  void FixupState(indirect_state_t *old_state, my_addr_t addr) const {
    *old_state = *old_state >> 8;
    UpdateHistory(old_state, addr);
  }

  /// set_ function for checkpointing
  set_error_t   set_data( attr_value_t *value );
  /// get_ function for checkpointing
  attr_value_t  get_data( void );

  /** register state saving function for the branch predictor */
  int  registerCheckpoint( confio_t *conf );

private:
  word_t GenerateIndex(my_addr_t branch_PC, uint32 proc);

  word_t GenerateExceptIndex(my_addr_t branch_PC, indirect_state_t *is, uint32 proc);
                             
  word_t GenerateExceptTag(my_addr_t branch_PC, indirect_state_t *is, uint32 proc);
                           
  void UpdateHistory(indirect_state_t *is, my_addr_t addr) const;

  /// regular indirect predictor
  my_addr_t *m_table;
  /// exception table for cascaded indirect predictor
  my_addr_t *m_except_table;
  /// tags for exceptions to indirect predictory
  my_addr_t *m_except_tags;
};

#endif /* _INDIRECT_H_ */
