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
#ifndef _BITLIB_H_
#define _BITLIB_H_

/*
 * Function library for manipulating bitfields.
 * @version $Id: bitlib.h 1.16 03/07/10 18:07:45-00:00 milo@cottons.cs.wisc.edu $
 */

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Class declaration(s)                                                   */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Global variables                                                       */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Global functions                                                       */
/*------------------------------------------------------------------------*/

/**
 * Prints a 32-bit integer as a bit field.
 * @param i_number the integer to print out
 */
extern void printBits( int i_number );

/**
 * Prints a 32-bit integer as a bit field into a string.
 * @param i_number the integer to print out.
 * @param buffer   the buffer to print the number into.
 */
extern void sprintBits( int i_number, char *buffer );

/**
 * Prints a 32-bit integer as a instruction.
 * Prints a 32-bit integer as a instruction. used to debug sparc decoding.
 * @param i_number the integer to print out
 */
extern void printIBits( int i_number );

/**
 * Prints a 64-bit integer as a bit field.
 * Prints a 64-bit integer as a bit field. used to debug sparc decoding.
 * @param i_number the int64 to print out
 */
extern void printBits64( int64 i_number );

/**
 * Prints a 64-bit integer as a bit field into a string.
 * @param i_number the integer to print out.
 * @param buffer   the buffer to print the number into.
 * @param num_bits the number of bits to print out (64 == all 64 bits)
 */
extern void sprintBits64( int64 i_number, char *buffer, int32 num_bits );

/**
 * masks off bits of a 32-bit integer.
 *
 * The start and stop bits can be in either order, start < stop or start > stop
 * The preferred order is start < stop. Mask includes start and stop bits.
 * @param data The data to be masked
 * @param start The bit to start masking at.
 * @param stop The bit to stop masking at.
 * @return int32 the masked data.
 */
//**************************************************************************
inline uint32  maskBits32( uint32 data, int start, int stop )
{
  int big;
  int small;
  int mask;

  if (stop > start) {
    big = stop;
    small = start;
  } else {
    big = start;
    small = stop;
  }
  
  if (big >= 31) {
    mask = ~0;
  } else {
    mask = ~(~0 << (big + 1));
  }
  return ((data & mask) >> small);
}

/**
 * masks off bits of a 64-bit integer.
 *
 * The start and stop bits can be in either order, start < stop or start > stop
 * The preferred order is start < stop. Mask includes start and stop bits.
 * @param data The data to be masked
 * @param start The bit to start masking at.
 * @param stop The bit to stop masking at.
 * @return int64 the masked data.
 */
//**************************************************************************
inline uint64  maskBits64( uint64 data, int start, int stop )
{
  int big;
  int small;
  uint64 mask;

  if (stop > start) {
    big = stop;
    small = start;
  } else {
    big = start;
    small = stop;
  }
  
  mask = 0;
  if (big >= 63) {
    mask = ~mask;
  } else {
    mask = ~(~mask << (big + 1));
  }
  return ((data & mask) >> small);
}

/*
 * returns a bit mask from hi to low (inclusive)
 *   i.e.    makeMask64( 6, 3 ) returns ... 0000 0111 1000
 */
//**************************************************************************
inline uint64 makeMask64( int start, int stop ) {
  int   big;
  int   small;
  if (stop > start) {
    big = stop;
    small = start;
  } else {
    big = start;
    small = stop;
  }

  uint64 mask = 0;
  mask = ~mask >> (63 + small - big);
  mask =  mask << small;
  return (mask);
}

/** sign extend an unsigned 32-bit integer w --
 *     w       :  the number to be sign extended
 *     high_bit:  the bit to sign extend on...
 * ASSUMPTIONS:
 *     w is a 32 bit integer
 *     w must only have bits from 0 ... high_bit (then zeros)
 *     high_bit must be the upper-most bit in w
 */
//**************************************************************************
inline int32 sign_ext32( uint32 w, int high_bit )
{
  int    highbit = (w >> high_bit) & 0x1;
  int32  result;

  if ( highbit ) {
    // needs to be sign extended
    result  = ~0 << (high_bit + 1);
    //printBits( result );
    result  = (result | w);
    //printBits( result );
    /*
    DEBUG_OUT("sign x (0x%0lx bit %d)= 0x%0lx = 0x%0lx  %d ? %d\n", 
           w, high_bit,
           w, result, w, result);
    */
  } else {
    result = w;
  }
  return (result);
}

/** sign extend an unsigned integer w --
 *     w       :  the number to be sign extended
 *     high_bit:  the bit to sign extend on...
 * ASSUMPTIONS:
 *     w is a 64 bit integer
 *     w must only have bits from 0 ... high_bit (then zeros)
 *     high_bit must be the upper-most bit in w
 */
//**************************************************************************
inline int64 sign_ext64( uint64 w, int high_bit )
{
  int    highbit = (w >> high_bit) & 0x1;
  int64  result;

  if ( highbit ) {
    // needs to be sign extended
    result  = 0;
    result  = ~result << (high_bit + 1);
    //printBits64(result);
    result  = (result | w);
    //printBits64(result);
    /*
      DEBUG_OUT("sign x (0x%0llx bit %d)= 0x%0llx = 0x%0lx  %llu ? %d\n", 
      w, high_bit,
      w, result, w, result);
    */
  } else {
    result = w;
  }
  return (result);
}

/**
 * return log of a number to the base 2
 **/
inline uint32 log_base_two(int n)
{
  int power = 0;
  if (n <= 0 || (n & (n-1)) != 0) {
    ERROR_OUT("log_base_two: error: given non power of two: %d\n", n);
    SIM_HALT;
  }
  while (n >>= 1)
    power++;
  return power;
}

/**
 * return The smallest power of two greater than the number
 **/
inline uint32 min_power_of_two(uint32 n)
{
  uint32 min_power = 1;
  while ( min_power <= n ) {
    min_power = min_power << 1;
    ASSERT ( min_power != 0 );
  }
  ASSERT ( min_power > n );
  return ( min_power );
}

#endif  /* _BITLIB_H_ */
