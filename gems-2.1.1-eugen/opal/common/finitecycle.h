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
#ifndef _FINITE_CYCLE_H_
#define _FINITE_CYCLE_H_
/*
 * Copyright (c) 1999 by Mark Hill and David Wood for the Wisconsin
 * Multifacet Project.  ALL RIGHTS RESERVED.  
 *
 * ##HEADER##
 *
 * This software is furnished under a license and may be used and
 * copied only in accordance with the terms of such license and the
 * inclusion of the above copyright notice.  This software or any
 * other copies thereof or any derivative works may not be provided or
 * otherwise made available to any other persons.  Title to and
 * ownership of the software is retained by Mark Hill and David Wood.
 * Any use of this software must include the above copyright notice.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS".  THE LICENSOR MAKES NO
 * WARRANTIES ABOUT ITS CORRECTNESS OR PERFORMANCE.
 * */

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Global variables                                                       */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Class declaration(s)                                                   */
/*------------------------------------------------------------------------*/

/**
* FiniteCycle implements a delay buffer for pipeline stages.
*
* A finite buffer which automatically advances its contents as the
* global cycle count advances. Accessing element [n] gives the
* n-th cycle in the future (wrapped around in the finite buffer).
* [0] gives the current cycles elements.
*
* @author  zilles
* @version $Id: finitecycle.h 1.10 03/04/16 20:40:34-00:00 cmauer@cottons.cs.wisc.edu $
*/

template <class Type>
class FiniteCycle {

public:

  FiniteCycle( void ) {
    m_max_elements = 0;
    m_circularBuffer = NULL;
  }
  ~FiniteCycle( void ) {
    if (m_circularBuffer)
      delete [] m_circularBuffer;
  }
  
  /** sets the maximum number of elements in the finite cycle */
  void setSize(uint32 max_elements) {
    ASSERT( max_elements != 0 );
    m_max_elements = max_elements;
    m_circularBuffer = new Type[m_max_elements];
  }
  /** Initializes all the items in the buffer to be a certain value. */
  void init(Type init_val) {
    for (uint32 i = 0; i < m_max_elements ; i ++) {
      m_circularBuffer[i] = init_val;
    }
  }

  /** Inserts an item into the rotating buffer, a number of cycles in the 
   *  "future"
   *  @param  current_cycle The current cycle
   *  @param  delay_cycles  Number of cycles to delay the item in the buffer
   *  @param  item          The item to insert into the buffer
   */
  void insertItem( tick_t current_cycle, uint32 delay_cycles, Type item ) {
    ASSERT( delay_cycles <= m_max_elements );
    uint32 moduloCycles = uint(current_cycle + delay_cycles) % m_max_elements;
    m_circularBuffer[moduloCycles] = item;
  }

  /** Peeks at a time in the future: returns the item associated w/ that time.
   *  peekItem( current_cycle, 0 ) is the same as "peekCurrent()".
   *  @param  current_cycle The current cycle
   *  @param  delay_cycles  Number of cycles to peek in the future
   *  @return the item in the buffer
   */
  Type &peekItem( tick_t current_cycle, uint32 delay_cycles ) {
    ASSERT( delay_cycles <= m_max_elements );
    uint32 moduloCycles = uint(current_cycle + delay_cycles) % m_max_elements;
    return (m_circularBuffer[moduloCycles]);
  }

  /** returns items related to the current cycle.
   *  (e.g. zero cycles in the future) */
  Type &peekCurrent( tick_t current_cycle ) {
    return (peekItem(current_cycle, 0));
  }

  /** print out the contents of the finite cycle starting at 0
   */
  void print( void ) const {
    cout << "|";
    for (uint32 i = 0; i < m_max_elements; i++) {
      cout << m_circularBuffer[i] << "|";
    }
    cout << endl;
  }

protected:
  /// The maximum length that elements can be delayed
  uint32 m_max_elements;

  /// The circular buffer for storing the delayed items
  Type  *m_circularBuffer;
};

/*------------------------------------------------------------------------*/
/* Global functions                                                       */
/*------------------------------------------------------------------------*/

#endif  /* _FINITE_CYCLE_H_ */
