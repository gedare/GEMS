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
 * FileName:  utimer.C
 * Synopsis:  implements a wall-clock timer
 * Author:    cmauer
 * Version:   $Id: utimer.C 1.4 04/11/04 16:40:03-06:00 beckmann@s0-03.cs.wisc.edu $
 */

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include "hfa.h"
#include "utimer.h"

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
utimer_t::utimer_t( )
{
  m_cumulative_secs = 0;
  m_cumulative_usecs = 0;
}

//**************************************************************************
utimer_t::~utimer_t( )
{
}

/*------------------------------------------------------------------------*/
/* Public methods                                                         */
/*------------------------------------------------------------------------*/

/// starts timing an event
//**************************************************************************
void utimer_t::startTimer( void )
{
  if ( gettimeofday( &m_timer_start, NULL ) != 0 ) {
    ERROR_OUT("error: unable to call gettimeofday()\n");
  }
}

/// stops timing an event
//**************************************************************************
void utimer_t::stopTimer( void )
{
  if ( gettimeofday( &m_timer_stop, NULL ) != 0 ) {
    ERROR_OUT("error: unable to call gettimeofday()\n");
  }
}

/// returns the elapsed time
//**************************************************************************
void utimer_t::getElapsedTime( int64 *sec_elapsed, int64 *usec_elapsed )
{
  int64  sec_expired  = m_timer_stop.tv_sec - m_timer_start.tv_sec;
  int64  usec_expired = m_timer_stop.tv_usec - m_timer_start.tv_usec;
  if ( usec_expired < 0 ) {
    usec_expired += 1000000;
    sec_expired--;
  }
  
  if ( !(usec_expired >= 0 && sec_expired >= 0) ) {
//     ERROR_OUT("error: elapsed time is negative, set to zero. %lld sec %ldd usec.\n",
//               sec_expired, usec_expired );
    sec_expired = 0;
    usec_expired = 0;
  }
  *sec_elapsed = sec_expired;
  *usec_elapsed = usec_expired;
}

/// accumulates the most recent 'stopped' time to a cumulative total
//**************************************************************************
void utimer_t::accumulateTime( void )
{
  int64  sec_expired;
  int64  usec_expired;

  getElapsedTime( &sec_expired, &usec_expired );
  m_cumulative_secs  += sec_expired;      
  m_cumulative_usecs += usec_expired;
  if ( m_cumulative_usecs > 1000000 ) {
    m_cumulative_usecs -= 1000000;
    m_cumulative_secs++;
  }
}

/// returns the cumulative total of times
//**************************************************************************
void utimer_t::getCumulativeTime( int64 *sec_cumulative,
                                  int64 *usec_cumulative )
{
  *sec_cumulative = m_cumulative_secs;
  *usec_cumulative = m_cumulative_usecs;
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

