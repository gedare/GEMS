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
#ifndef _DEPENCENCY_H_
#define _DEPENCENCY_H_
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
* An array showing which control flow graph nodes are currently producing
* register values. This is used to establish predecessors, successors,
* in the control flow graph.
*
* @see     flow_inst_t
* @author  cmauer
* @version $Id: dependence.h 1.2 03/01/06 19:40:55-00:00 cmauer@cottons.cs.wisc.edu $
*/
class reg_dependence_t {
public:
  /**
   * @name Constructor(s) / destructor
   */
  //@{

  /**
   * Constructor: creates object
   */
  reg_dependence_t();

  /**
   * Destructor: frees object.
   */
  ~reg_dependence_t();
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
  //@}

  /** @name Static Variables */

private:
  /// Tracks which flow instruction last wrote the register
};

/**
* An array showing which control flow graph nodes are currently producing
* memory values. This is used to establish predecessors, successors,
* in the control flow graph for memory dependencies.
*
* @see     flow_inst_t, reg_dependence_t
* @author  cmauer
* @version $Id: dependence.h 1.2 03/01/06 19:40:55-00:00 cmauer@cottons.cs.wisc.edu $
*/
class mem_dependence_t {
public:
  /**
   * @name Constructor(s) / destructor
   */
  //@{

  /**
   * Constructor: creates object
   */
  mem_dependence_t();

  /**
   * Destructor: frees object.
   */
  ~mem_dependence_t();
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
  //@}

  /** @name Static Variables */

private:
  /// class member
  int       m_id;
};

/*------------------------------------------------------------------------*/
/* Global variables                                                       */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Global functions                                                       */
/*------------------------------------------------------------------------*/

#endif  /* _DEPENCENCY_H_ */


