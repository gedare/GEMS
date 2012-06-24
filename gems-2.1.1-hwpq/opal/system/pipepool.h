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
#ifndef _PIPEPOOL_H_
#define _PIPEPOOL_H_

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
* Pipepool implements a pool of objects that are to be executed in a
* pipelined finite-state machine. The pipeline pool has certain restrictions
* on the bandwidth and resource requirements for moving from state to state,
* so it only approximates a real (high speed) pipeline.
*
* This class can be used as an ordered, or unordered. Its implementation
* is currently a doubly-linked list.
*
* @see     pipestate_t
* @author  cmauer
* @version $Id: pipepool.h 1.3 03/07/10 18:07:48-00:00 milo@cottons.cs.wisc.edu $
*/
class pipepool_t {

public:
  /**
   * @name Constructor(s) / destructor
   */
  //@{

  /**
   * Constructor: creates object
   */
  pipepool_t();

  /**
   * Destructor: frees object.
   */
  ~pipepool_t();
  //@}

  /**
   * @name Methods
   */
  //@{
  /// Insert a pipestate object into the pool
  void           insertOrdered( pipestate_t *state );

  /// wakes the first (lowest priority) element in the pool
  pipestate_t   *removeHead( void );

  /// Remove an object from the pool
  bool           removeElement( pipestate_t *state );

  /// wakes the first (lowest priority) element in the pool
  void           wakeFront( void );

  /// wakes up all elements in the pool
  void           wakeAll( void );

  /// walk the elements in the linked list (start with NULL pointer)
  pipestate_t   *walkList( pipestate_t *state );

  /// print all the elements in the linked list (excluding head pointer)
  void           print( void );

  /// get a count of the number of elements in this list
  uint32         getCount( void ) {
    return m_count;
  }
  //@}

  /**
   * @name Accessor(s) / mutator(s)
   */
  //@{
  //@}

private:
  /// The "head" node of the list (not used)
  pipestate_t *m_head;

  /// The number of elements in this pool
  uint32       m_count;
};

#endif  /* _PIPEPOOL_H_ */
