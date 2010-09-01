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
 * FileName:  fileio
 * Synopsis:  system input and output file wrappers
 * Author:    cmauer
 * Version:   $Id: fileio.C 1.6 03/07/10 18:07:45-00:00 milo@cottons.cs.wisc.edu $
 */

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include "hfa.h"
#include "fileio.h"

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


/** [Memo].
 *  [Internal Documentation]
 */
//**************************************************************************
char *commafmt( uint64 value, char *buffer, int bufsize )
{
  int len = 1, posn = 1, sign = 1;
  char *ptr = buffer + bufsize - 1;

  if (2 > bufsize)
    {
    ABORT: *buffer = '\0';
    return 0;
    }

  *ptr-- = '\0';
  --bufsize;
  // if the API changes to be signed, add this code back in
#if 0
  if (0L > value)
    {
      sign = -1;
      value = -value;
    }
#endif

  for ( ; len <= bufsize; ++len, ++posn)
    {
      *ptr-- = (char)((value % 10L) + '0');
      if (0L == (value /= 10L))
        break;
      if (0 == (posn % 3))
        {
          *ptr-- = ',';
          ++len;
        }
      if (len >= bufsize)
        goto ABORT;
    }

  if (0 > sign)
    {
      if (len >= bufsize)
        goto ABORT;
      *ptr-- = '-';
      ++len;
    }

  memmove(buffer, ++ptr, len + 1);
  return buffer;
}

//**************************************************************************
bool  findSeqFilename( char *basefile, char *availfile, int *sequence )
{
  int   index;
  FILE *fp;

  for (index = *sequence; index < 1000; index++) {
    if ( index == 0 ) {
      sprintf( availfile, "%s", basefile );
    } else {
      sprintf( availfile, "%s-%03d", basefile, index );
    }
    fp = fopen( availfile, "r" );
    if (fp == NULL) {
      // great found a unused file
      break;
    } else {
      fclose(fp);
    }
  }

  *sequence = index;
  if ( index == 1000 ) {
    ERROR_OUT("unable to open file: %s. No unused trace names found.\n", 
              basefile);
    return false;
  }
  return true;
}

//**************************************************************************
void  nextSeqFilename( char *basefile, char *nextfile, int *sequence )
{
  if ( *sequence == 0 ) {
    sprintf( nextfile, "%s", basefile );
  } else {
    sprintf( nextfile, "%s-%03d", basefile, *sequence );
  }
  *sequence = *sequence + 1;
}
