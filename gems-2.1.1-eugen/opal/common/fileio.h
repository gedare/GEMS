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
#ifndef _FILEIO_H_
#define _FILEIO_H_

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

const int     FILEIO_MAX_FILENAME = 256;

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
 * Swap the endian-ness of a variable sized block, so it can be written
 * (read) platform independently.
 *
 * @param val The value to be swapped.
 * @returns   The swapped value
 */

extern inline uint16 endian_swap16( uint16 val ) {
#ifdef SWAP_ENDIAN
  uint16 ret;
  ret  = (val & 0x00ff) << 8;
  ret |= (val & 0xff00) >> 8;
  return ret;
#else
  return val;
#endif
}

/**
 * Endian swap for 32-bit values
 * @param val The value to be swapped.
 */
extern inline uint32 endian_swap32( uint32 val ) {
#ifdef SWAP_ENDIAN
  uint32 ret;
  ret  = (val & 0x000000ff) << 24;
  ret |= (val & 0x0000ff00) << 8;
  ret |= (val & 0x00ff0000) >> 8;
  ret |= (val & 0xff000000) >> 24;
  return ret;
#else
  return val;
#endif
}

/**
 * Endian swap for 64-bit values
 * @param val The value to be swapped.
 */
extern inline uint64 endian_swap64( uint64 val ) {
#ifdef SWAP_ENDIAN
  uint64 ret;
  ret  = (val & 0x00000000000000ffULL) << 56;
  ret |= (val & 0x000000000000ff00ULL) << 40;
  ret |= (val & 0x0000000000ff0000ULL) << 24;
  ret |= (val & 0x00000000ff000000ULL) << 8;
  ret |= (val & 0x000000ff00000000ULL) >> 8;
  ret |= (val & 0x0000ff0000000000ULL) >> 24;
  ret |= (val & 0x00ff000000000000ULL) >> 40;
  ret |= (val & 0xff00000000000000ULL) >> 56;
  return ret;
#else
  return val;
#endif
}

/** endian-safe fread function:
 *    just like fread, but can be used cross platforms
 */

inline size_t myfread( void *ptr, size_t size, size_t nitems,
                       FILE *stream)
{
  size_t  nread;
  uint64  temp64;
  uint32  temp32;
  uint16  temp16;

  nread = fread( ptr, size, nitems, stream );
  if (size == 4) {
    temp32 = endian_swap32( *(uint32 *) ptr );
    // DEBUG_OUT("swap: 0x%0x 0x%0x\n", *(uint32 *) ptr, temp32 );
    *(uint32 *) ptr = temp32;
  } else if (size == 8) {
    temp64 = endian_swap64( *(uint64 *) ptr );
    // DEBUG_OUT("swap: 0x%0llx 0x%0llx\n", *(uint64 *) ptr, temp64 );
    *(uint64 *) ptr = temp64;
  } else if (size == 2) {
    temp16 = endian_swap16( *(uint16 *) ptr );
    // DEBUG_OUT("swap: 0x%0x 0x%0x\n", *(uint16 *) ptr, temp16 );
    *(uint16 *) ptr = temp16;
  } else if (size == 1) {
    // do nothing
  } else {
    ERROR_OUT("myfread: unrecognized size %d\n", size);
  }
  return nread;
}

/** endian-safe fwrite function:
 *    just like fwrite, but can be used cross platforms
 */
inline size_t myfwrite(const void *ptr, size_t size, size_t nitems,
                       FILE *stream)
{
  size_t  nwritten = 0;
  uint64  temp64;
  uint32  temp32;
  uint16  temp16;

  if (size == 4) {
    temp32 = endian_swap32( *(uint32 *) ptr );
    nwritten = fwrite( &temp32, size, nitems, stream );
  } else if (size == 8) {
    temp64 = endian_swap64( *(uint64 *) ptr );
    nwritten = fwrite( &temp64, size, nitems, stream );
  } else if (size == 2) {
    temp16 = endian_swap16( *(uint16 *) ptr );
    nwritten = fwrite( &temp16, size, nitems, stream );
  } else if (size == 1) {
    // do nothing
    nwritten = fwrite( ptr, size, nitems, stream );
  } else {
    ERROR_OUT("myfwrite: unrecognized size %d\n", size);
  }
  return nwritten;
}

/** format a value as a string, with commas
 *  @param  value   The value to commize ","
 *  @param  buffer  The buffer to work in
 *  @param  bufsize The length of the buffer
 *  @return A pointer to buffer.
 */
extern char *commafmt( uint64 value, char *buffer, int bufsize );

/** Given a filename, test to see if it exists. If it does exist,
 *  try to open files of the form "file-000" to "file-999".
 *  @return True indicating an error, false indicating success.
 */

extern bool  findSeqFilename( char *basefile, char *availfile, int *sequence );

/** Given a filename, generate the next filename in sequential order */
extern void  nextSeqFilename( char *basefile, char *nextfile, int *sequence );

#endif  /* _FILEIO_H_ */
