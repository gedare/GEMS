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
#ifndef __RAS_H
#define __RAS_H

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

/// pointer to an element in the return address stack
typedef half_t ras_pointer_t;

/// state in the return address stack
typedef struct _ras_state_t {
  ras_pointer_t TOS;
  ras_pointer_t next_free;
} ras_state_t;


/*------------------------------------------------------------------------*/
/* Class declarations                                                     */
/*------------------------------------------------------------------------*/

/**
* Return address stack: implements a stack of addresses.
*   To enable partial squash, this implements checkpointed
*   RAS in:
*   "The effects of mispredicted-path execution on branch
*    prediction structures" Stephan Jourdan, et. al.
*
*    It supports partial squash very well, and fits into
*    the current u-architecture of opal.
*    The drawback, however, is that it overflow the RAS faster
*    than conventional RAS, so please use bigger RAS stack.
*
* Implements the basic push and pop functionality of a stack.
*
* @author  zilles
* @version $Id$
*/
class ras_t {
public:

  /// @name: constants
  //@{
  /// turn on/off ras debug
  static const bool DEBUG_RAS = false;

  /// precise call sementics detection
  static const bool PRECISE_CALL = true;

  /// use exception table to screen bad calls
  static const bool EXCEPTION_TABLE = true;
  //@}

  /** @name Constructor(s) / Destructor */
  //@{
  /**
   * Constructor: use default constructor
   */
  ras_t(pseq_t *pseq, uint32 size, uint32 excep_bits);
  /**
   * Destructor: use default destructor
   */
  ~ras_t();
  //@}

  /// Initializes the return address stack to all zeros
  void init( void );

  /// initialize the state of the stack
  void InitializeState(ras_state_t *rs) const { 
         rs->TOS = 0; rs->next_free = 1; 
  }

  /// push a return address and callee on the stack
  void      push(my_addr_t return_target_PC, my_addr_t call_target_PC, ras_state_t *rs);
  /// Pop a return address off the stack
  my_addr_t pop(ras_state_t *rs);
  /// get top return address of the stack
  void      getTop(ras_state_t *rs, my_addr_t* a);
  /// get top2 calee address of the stack
  void      getTop2(ras_state_t *rs, my_addr_t* a1, my_addr_t* a2);
  /// mark a return address to be exception
  void      markException(my_addr_t a);
  /// unmark a return address to be exception
  void      unmarkException(my_addr_t a);
  /// print out the RAS, displaying the top of the stack
  void      print( ras_state_t *rs  );

private:
  /// corresponding pseq_t
  pseq_t        *m_pseq;

  /// size of the RAS
  uint32         m_size;
  /// The return address stack
  my_addr_t     *m_return_target_table;
  my_addr_t     *m_call_target_table;
  /// A pointer to the next used element in the stack,
  /// (Confusingly called NOS in Jourdan's paper)
  ras_pointer_t *m_last_TOS;

  /// The exception table, direct-mapped
  my_addr_t     *m_exception_table;

  /// The table size and mask
  uint32         m_excep_size;
  uint32         m_excep_mask;

  /// compute table index from address
  uint32         exceptionIndex(my_addr_t a) {
    return (a>>2)&m_excep_mask;
  }
};

#endif /* __RAS_H */
