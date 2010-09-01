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
 * FileName:  bitlib.C
 * Synopsis:  library of bit-oriented manipulation, output routines
 * Author:    cmauer
 * Version:   $Id: bitlib.C 1.11 03/07/10 18:07:45-00:00 milo@cottons.cs.wisc.edu $
 */

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include "hfa.h"
#include "bitlib.h"

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

/*------------------------------------------------------------------------*/
/* Public methods                                                         */
/*------------------------------------------------------------------------*/

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


//**************************************************************************
void sprintBits( int i_number, char *buffer)
{
  strcpy( buffer, "" );
  for (int i = 31; i >= 0; i--) {
    if ( i == 15 ) {
      strcat( buffer, ":");
    } else if ((i+1) % 4 == 0) {
      strcat( buffer, " ");
    }
    int bit_n = i_number & (1 << i);
    if (bit_n)
      strcat( buffer, "1");
    else
      strcat( buffer, "0");
  }
}

//**************************************************************************
void printBits( int i_number )
{
  char buffer[128]; // big enough to hold 32 bits + while space
  sprintBits( i_number, buffer );
  strcat( buffer, "\n");
  DEBUG_OUT( buffer );
}

//**************************************************************************
void printIBits( int i_number )
{
  int  index = 31;
  char buffer[128];
  strcpy( buffer, "" );
  while ( index >= 0 ) {

    int bit_n = i_number & (1 << index);
    if (bit_n)
      strcat( buffer, "1");
    else
      strcat( buffer, "0");
    if ( (index == 30) ||
         (index == 25) ||
         (index == 19) ||
         (index == 14) ||
         (index == 13) ||
         (index ==  5) ) {
      strcat( buffer, " ");
    }
    index--;
  }
  DEBUG_OUT( buffer );
}

//**************************************************************************
void printBits64( int64 i_number )
{
  // big enough to hold 64 bits and whitespace
  char buffer[256];
  sprintBits64( i_number, buffer, 64 );
  strcat( buffer, "\n");
  DEBUG_OUT( buffer );
}

//**************************************************************************
void sprintBits64( int64 i_number, char *buffer, int32 num_bits )
{
  strcpy( buffer, "" );
  for (int i = num_bits - 1; i >= 0; i--) {
    if ( i == 31 ) {
      strcat( buffer, "::");
    } else if ( i == 15 || i == 47 ) {
      strcat( buffer, ":");
    } else if ((i+1) % 4 == 0) {
      strcat( buffer, " ");
    }
    int64 bit_n = i_number & (1ull << i);
    if (bit_n)
      strcat( buffer, "1");
    else
      strcat( buffer, "0");
  }
}

/**
 * returns the bits from hi to low (inclusive), right justfied in the long
 *   i.e.    maskBits( 110000, 5, 4 ); returns 11
 */
//**************************************************************************

/*
 * returns the bits from hi to low (inclusive), right justfied in the long
 *   i.e.    maskBits( 110000, 5, 4 ); returns 11
 */
//**************************************************************************

/** sign extend an unsigned 32-bit integer w --
 *     w       :  the number to be sign extended
 *     high_bit:  the bit to sign extend on...
 * ASSUMPTIONS:
 *     w is a 32 bit integer
 *     w must only have bits from 0 ... high_bit (then zeros)
 *     high_bit must be the upper-most bit in w
 */
//***************************************************************************

/** sign extend an unsigned 64-bit integer w --
 *     w       :  the number to be sign extended
 *     high_bit:  the bit to sign extend on...
 * ASSUMPTIONS:
 *     w is a 32 bit integer
 *     w must only have bits from 0 ... high_bit (then zeros)
 *     high_bit must be the upper-most bit in w
 */
//**************************************************************************
