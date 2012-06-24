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
 * FileName:  debugio.C
 * Synopsis:  formats, filters debugging log file messages.
 * Author:    
 * Version:   $Id$
 */

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include "hfa.h"
#include "fileio.h"
#include "debugio.h"
#include "system.h"

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

static const bool     g_compressed_output = false;

/*------------------------------------------------------------------------*/
/* Variable declarations                                                  */
/*------------------------------------------------------------------------*/

/// declaration of system wide log file
FILE *debugio_t::m_logfp = NULL;
/// declaration of system wide debug cycle
uint64 debugio_t::m_global_cycle = 0;

/*------------------------------------------------------------------------*/
/* Forward declarations                                                   */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* out_intf_t class                                                       */
/*------------------------------------------------------------------------*/

//**************************************************************************
void out_intf_t::out_info( const char *fmt, ...)
{
  va_list  args;

  // DEBUG_FILTER
#ifdef DEBUG_FILTER
  if (!validDebug())
    return;
#endif
  // fprintf(stdout,  "[%d] ", m_id);
  if (m_logfp)
    fprintf(m_logfp, "[%d] ", m_id);
  va_start(args, fmt);
  int error_rt = vfprintf(stdout, fmt, args);
  va_end(args);
  assert(error_rt >= 0);   
  if (m_logfp) {
    va_start(args, fmt);
    int error_rt = vfprintf(m_logfp, fmt, args);
    va_end(args);
    assert(error_rt >= 0);
  }
}

//**************************************************************************
// version of out_info which doesn't output ID number
void out_intf_t::out_info_noid( const char *fmt, ...)
{
  va_list  args;

  // DEBUG_FILTER
#ifdef DEBUG_FILTER
  if (!validDebug())
    return;
#endif
  // fprintf(stdout,  "[%d] ", m_id);
  va_start(args, fmt);
  int error_rt = vfprintf(stdout, fmt, args);
  va_end(args);
  assert(error_rt >= 0);   
  if (m_logfp) {
    va_start(args, fmt);
    int error_rt = vfprintf(m_logfp, fmt, args);
    va_end(args);
    assert(error_rt >= 0);
  }
}

// print which only goes to the file
//**************************************************************************
void out_intf_t::out_log( const char *fmt, ...)
{
  va_list  args;

  // DEBUG_FILTER
#ifdef DEBUG_FILTER
  if (!validDebug())
    return;
#endif
  if (!m_logfp)
    return;
  fprintf(m_logfp, "[%d] ", m_id);
  va_start(args, fmt);
  int error_rt = vfprintf(m_logfp, fmt, args);
  va_end(args);
  assert(error_rt >= 0);
}

//**************************************************************************
void out_intf_t::setDebugTime(uint64 t)
{
  m_starting_cycle = t;
  debugio_t::setDebugTime( t );
}

//**************************************************************************
bool out_intf_t::validDebug( void )
{
  if ( m_starting_cycle > system_t::inst->getGlobalCycle() ) {
    return false;
  }
  return true;
}
  
/*------------------------------------------------------------------------*/
/* Global debug I/O methods                                               */
/*------------------------------------------------------------------------*/

//**************************************************************************
debugio_t::debugio_t()
{
  m_logfp = NULL;
}

//**************************************************************************
debugio_t::~debugio_t()
{
}

// system wide info
//**************************************************************************
void debugio_t::out_info( const char *fmt, ... )
{
  // DEBUG_FILTER
#ifdef DEBUG_FILTER
  //    if(system_t::inst != NULL){
  //      cout << "m_global cycle = " << m_global_cycle << " system global cycle = " << system_t::inst->getGlobalCycle() << endl;
  //    }
  if ( system_t::inst &&
       m_global_cycle > system_t::inst->getGlobalCycle() )
    return;
#endif
  va_list args;
  va_start(args, fmt);
  int error_rt = vfprintf(stdout, fmt, args);
  va_end(args);
  assert(error_rt >= 0);   
  if (m_logfp) {
    va_start(args, fmt);
    int error_rt = vfprintf(m_logfp, fmt, args);
    va_end(args);
    assert(error_rt >= 0);
  }
}

// system wide log file
//**************************************************************************
void debugio_t::out_log( const char *fmt, ...)
{
  va_list args;
  if (m_logfp) {
    va_start(args, fmt);
    int error_rt = vfprintf(m_logfp, fmt, args);
    va_end(args);
    assert(error_rt >= 0);
  }
}

// system wide error
//**************************************************************************
void debugio_t::out_error( const char *fmt, ... )
{
  // out_error should never be filtered: we must know if something goes wrong
  va_list args;
  va_start(args, fmt);
  int error_rt = vfprintf(stderr, fmt, args);
  va_end(args);
  assert(error_rt >= 0);
  if (m_logfp) {
    va_start(args, fmt);
    int error_rt = vfprintf(m_logfp, fmt, args);
    va_end(args);
    assert(error_rt >= 0);
  }
}

//**************************************************************************
void debugio_t::openLog( const char *logFileName )
{
  if ( g_compressed_output ) {
    char   cmd[FILEIO_MAX_FILENAME];
    sprintf( cmd, "/s/std/bin/gzip > %s.gz", logFileName );
    m_logfp = popen( cmd, "w" );
  } else {
    m_logfp = fopen( logFileName, "w" );
    if ( m_logfp == NULL ) {
      ERROR_OUT("debugio_t: openLog error: unable to open log file %s\n", logFileName );
    }
  }
}

//**************************************************************************
void debugio_t::closeLog( void )
{
  if ( m_logfp != NULL && m_logfp != stdout ) {
    if ( g_compressed_output ) {
      pclose( m_logfp );
    } else {
      fclose( m_logfp );
    }
    m_logfp = NULL;
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

