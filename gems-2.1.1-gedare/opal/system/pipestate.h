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
#ifndef _PIPESTATE_H_
#define _PIPESTATE_H_

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
* Maintains data associated with pipeline state (current state in the
* pipeline), successors and predecessors.
*
* Heir apparent to the dreaded 'wait_list' in the project.
*
* @see     pipestate_t
* @author  cmauer
* @version $Id: pipestate.h 1.1 02/10/02 18:29:34-00:00 cmauer@cottons.cs.wisc.edu $
*/

class pipestate_t {
  friend class pipepool_t;
public:
  /**
   * @name Constructor(s) / destructor
   */
  //@{

  /**
   * Constructor: creates object
   * @param waiter  The object that is being placed on the list
   */
  pipestate_t( waiter_t *waiter );

  /**
   * Destructor: frees object.
   */
  virtual ~pipestate_t();
  //@}

  /**
   * @name Methods
   */
  //@{
  /// insert this element into a list of elements
  void insertElement( pipestate_t *pool );

  /// insert this element into a decending ordered list
  void insertOrdered( pipestate_t *head );

  /// remove a specific element in the list, returns false if not found
  bool          removeElement( pipestate_t *element );

  /// remove the first element in the list
  pipestate_t  *removeHead( void );

  /// returns the element in the linked list
  waiter_t     *getElement( void ) { return m_element; }

  /// initialize this element to point at itself
  void initHead( void ) {
    m_next = this;
    m_prev = this;
  }
  
  /// virtual function for getting the cardinality of this element
  virtual uint64 getSortOrder( void ) { return 0; }

  /// virtual function for printing debugging information
  virtual void   print( void ) {
    DEBUG_OUT("pipestate_t: generic next 0x%0x. prev 0x%0x\n", m_next, m_prev);
  }
  //@}
  
private:
  /// the object being stored in the pool at this location
  waiter_t     *m_element;

  /// a pointer to the next element in the pool
  pipestate_t  *m_next;
  /// a pointer to the previous element in the pool
  pipestate_t  *m_prev;
};

#endif  /* _PIPESTATE_H_ */
