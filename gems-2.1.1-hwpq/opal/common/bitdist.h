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
#ifndef _BITDIST_H_
#define _BITDIST_H_

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
* This class provides bit-distribution functions on binary data.
*
* [Documentation]
* @author  cmauer
* @version $Id: bitdist.h 1.2 03/01/06 19:40:53-00:00 cmauer@cottons.cs.wisc.edu $
*/
class bitdist_t {

public:


  /**
   * @name Constructor(s) / destructor
   */
  //@{

  /**
   * Constructor: creates object
   * @param historysize  The number of bits to track statistics on.
   */
  bitdist_t( uint32 historysize );

  /**
   * Destructor: frees object.
   */
  ~bitdist_t();
  //@}

  /**
   * @name Methods
   */
  //@{
  /** print out the distribution observed. */
  void printStats( void );
  //@}

  /**
   * @name Accessor(s) / mutator(s)
   */
  //@{
  /**
   * Push one bit of history on to this function.
   * @param newbit    The one new bit of history.
   */
  void pushHistory( bool newbit, bool mispredict );
  //@}

  /** @name Static Variables */

private:
  
  /// size of history in bits
  uint32      m_hist_size;

  /// size of array (2 ^ m_hist_size)
  uint32      m_array_size;
  
  /// history of last n seen bits
  uint32      m_history;

  /// array which tracks the number of times each pattern is seen.
  uint32     *m_array;

  /// array tracking mispredictions on a per-history basis
  uint32     *m_mispredict;
  
  /** array to track the average bit distribution, per historysize bits
   *    The size of this array is 2*historysize bits: it tracks the number
   *    of times a zero or one is seen.
   */
  uint32     *m_average_seen;
};

/*------------------------------------------------------------------------*/
/* Global variables                                                       */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Global functions                                                       */
/*------------------------------------------------------------------------*/

#endif  /* _BITDIST_H_ */
