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
#ifndef _BITFIELD_H_
#define _BITFIELD_H_

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

/// number of bits in a word
const int32 BITFIELD_WSIZE = (sizeof(unsigned int) * 8);

/*------------------------------------------------------------------------*/
/* Class declaration(s)                                                   */
/*------------------------------------------------------------------------*/

/**
* A bit field is a compact representation of an array of bits.
*
* Bit field are initialized to zero on creation.
*
* @author  cmauer
* @version $Id: bitfield.h 1.6 02/02/21 16:52:38-00:00 cmauer@cottons.cs.wisc.edu $
*/
class bitfield_t {

public:


  /**
   * @name Constructor(s) / destructor
   */
  //@{

  /**
   * Constructor: creates bit field
   * @param length The static length of the bit field.
   */
  bitfield_t( int length );

  /**
   * Constructor: creates bit field (used for arrays)
   */
  bitfield_t( void ) {
    m_length = -1;
    m_array  = NULL;
  }

  /**
   * Constructor: allocates memory for bit field (used for arrays)
   */
  void alloc( int length );

  /**
   * Destructor: frees object.
   */
  ~bitfield_t();
  //@}

  /**
   * @name Accessor(s) / mutator(s)
   */
  //@{

  /**
   * Sets a bit in the bit field.
   *
   * @param bitnum The bit to set.
   * @param bitnum The value (true or false) to set the bit to).
   */
  void setBit( int bitnum, bool value );

  /**
   * Gets a bit in the bit field.
   *
   * @param bitnum The bit to get.
   * @return       The bits value (true if 1) (false if 0).
   */
  bool getBit( int bitnum );

  /**
   * prints out the contents of a bitfield.
   */
  void print( void );
  //@}

private:

  /// length of bit vector
  int            m_length;

  /// array of bits # of entries == (m_length/sizeof(unsigned int))
  unsigned int  *m_array;
};


/*------------------------------------------------------------------------*/
/* Global variables                                                       */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Global functions                                                       */
/*------------------------------------------------------------------------*/

#endif  /* _BITFIELD_H_ */
