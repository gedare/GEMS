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
 * FileName:  bitdist.C
 * Synopsis:  A statistics distribution package for binary data
 * Author:    
 * Version:   $Id: bitdist.C 1.3 03/07/10 18:07:45-00:00 milo@cottons.cs.wisc.edu $
 */

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include "hfa.h"
#include "bitlib.h"
#include "fileio.h"
#include "bitdist.h"

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
bitdist_t::bitdist_t( uint32 historysize )
{
  if ( historysize > 14 ) {
    ERROR_OUT("warning: history is %d bits: allocation may fail.\n", 
              historysize);
  }
  m_history = 0;
  m_hist_size = historysize;
  m_array_size = 1 << m_hist_size;

  m_array = (uint32 *) malloc( sizeof(uint32) * m_array_size );
  m_average_seen = (uint32 *) malloc( sizeof(uint32) * m_hist_size );
  memset( m_array, 0, sizeof(uint32) * m_array_size );
  memset( m_average_seen, 0, sizeof(uint32) * m_hist_size );

  m_mispredict = (uint32 *) malloc( sizeof(uint32) * m_array_size );
  memset( m_mispredict, 0, sizeof(uint32) * m_array_size );
}

//**************************************************************************
bitdist_t::~bitdist_t( )
{
  if (m_array) {
    free( m_array );
    m_array = NULL;
  }
  if (m_average_seen) {
    free( m_average_seen );
    m_average_seen = NULL;
  }
}

/*------------------------------------------------------------------------*/
/* Public methods                                                         */
/*------------------------------------------------------------------------*/

//**************************************************************************
void bitdist_t::pushHistory( bool newbit, bool mispredict )
{
  m_history = (m_history << 1) | newbit;
  m_history = maskBits32( m_history, m_hist_size - 1, 0 );
  
  // increment stats for this history
  ASSERT( m_history < m_array_size );
  m_array[m_history]++;
  if (mispredict)
    m_mispredict[m_history]++;
}

//**************************************************************************
void bitdist_t::printStats( void )
{
  uint64 total = 0;
  uint64 totalmis = 0;
  for (uint32 j = 0; j < m_array_size; j++) {
    total += m_array[j];
    totalmis += m_mispredict[j];
  }
  
  for (uint32 j = 0; j < m_array_size; j++) {
    if ( m_array[j] != 0 ) {

      for (int i = m_hist_size; i >= 0; i--) {
        if ( i == 15 ) {
          DEBUG_OUT(":");
        } else if ((i+1) % 4 == 0) {
          DEBUG_OUT(" ");
        }
        int bit_n = j & (1 << i);
        if (bit_n)
          DEBUG_OUT("1");
        else
          DEBUG_OUT("0");
      }
      
      char buf[40], buf1[40];
      DEBUG_OUT(" %12s %4.1f  %12s %4.1f\n",
             commafmt( m_array[j], buf, 40 ),
             ((double) m_array[j] / (double) total) * 100,
             commafmt( m_mispredict[j], buf1, 40 ),
             ((double) m_mispredict[j] / (double) totalmis) * 100);
    }
  }
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


/** [Memo].
 *  [Internal Documentation]
 */
//**************************************************************************

