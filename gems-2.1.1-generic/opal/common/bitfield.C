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
/*
 * FileName:  bitfield.C
 * Synopsis:  implements a bit vector of a finite size
 * Author:    cmauer
 * Version:   $Id: bitfield.C 1.8 02/02/21 16:52:38-00:00 cmauer@cottons.cs.wisc.edu $
 */

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include "hfa.h"
#include "bitfield.h"

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Variable declarations                                                  */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Forward declarations                                                   */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Constructor(s) / destructor                                            */
/*------------------------------------------------------------------------*/

//**************************************************************************
bitfield_t::bitfield_t( int length )
{
  m_length = -1;
  m_array  = NULL;
  alloc(length);
}

//**************************************************************************
bitfield_t::~bitfield_t( )
{
  if (m_array)
    free( m_array );
}

//**************************************************************************
void bitfield_t::alloc( int length )
{
  int      numwords;

  ASSERT( m_length == -1 );
  if (length < 0) {
    ERROR_OUT("error: bitfield length must be > 0: %d\n", length);
    length = 8;
  }

  m_length = length;
  numwords = (length / BITFIELD_WSIZE);
  // if length is not a multiple of 8, add one
  if (length % BITFIELD_WSIZE != 0) {
    numwords++;
  }
  m_array  = (unsigned int *) malloc( numwords * sizeof(unsigned int) );
  memset( m_array, 0, numwords * sizeof(unsigned int) );
}

/*------------------------------------------------------------------------*/
/* Public methods                                                         */
/*------------------------------------------------------------------------*/

//**************************************************************************
void bitfield_t::setBit( int bitnum, bool value )
{
  int  wordIndex;
  int  bitMask;

  if ( bitnum > m_length ) {
    ERROR_OUT("error: setBit[%d] on array of size %d\n", bitnum, m_length);
    return;
  }
  wordIndex = bitnum / BITFIELD_WSIZE;
  bitMask   = 1 << (bitnum % BITFIELD_WSIZE);
  if (value) {
    m_array[wordIndex] |= bitMask;
  } else {
    m_array[wordIndex] &= ~bitMask;
  }
}

//**************************************************************************
bool bitfield_t::getBit( int bitnum )
{
  int  wordIndex;
  int  bitIndex;
  bool result;

  if ( bitnum > m_length ) {
    ERROR_OUT("error: getBit[%d] on array of size %d\n", bitnum, m_length);
    return false;
  }
  wordIndex =  bitnum / BITFIELD_WSIZE;
  bitIndex  =  bitnum % BITFIELD_WSIZE;
  result    = (m_array[wordIndex] >> bitIndex) & 0x1;

  return ( result );
}

//**************************************************************************
void bitfield_t::print( void )
{
  for (int i = 0; i < m_length; i++) {
    DEBUG_OUT("%d", getBit(i));
  }
  DEBUG_OUT("\n");
}

/*------------------------------------------------------------------------*/
/* Accessor(s) / mutator(s)                                               */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Private methods                                                        */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Static methods                                                         */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Global functions                                                       */
/*------------------------------------------------------------------------*/
