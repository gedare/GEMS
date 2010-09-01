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
 * FileName:  tracefile.C
 * Synopsis:  implementation of a compressed trace file
 * Author:    cmauer
 * Version:   $Id: tracefile.C 1.13 03/07/10 18:07:51-00:00 milo@cottons.cs.wisc.edu $
 */

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include "hfa.h"
#include "pstate.h"
#include "fileio.h"
#include "tracefile.h"

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

/// base filename to save traces into
const char *TRACE_DIRNAME      = "/scratch.local/";
const char *TRACE_FILENAME     = "trace";

/*------------------------------------------------------------------------*/
/* Variable declarations                                                  */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Forward declarations                                                   */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Constructor(s) / destructor                                            */
/*------------------------------------------------------------------------*/

/** opens a trace for reading */
//**************************************************************************
tracefile_t::tracefile_t( char *traceFileName, 
                          core_state_t *sim_state,
                          context_id_t *context_p )
{
  int   vnumber;
  char  filename[FILEIO_MAX_FILENAME];
  char  cmd[FILEIO_MAX_FILENAME];

  if (traceFileName == NULL) {
    ERROR_OUT("error: NULL filename passed to tracefile\n");
    sprintf(filename, "%s%s", TRACE_FILENAME, "-p1-000");
    traceFileName = filename;
  }
  m_filename = (char *) malloc( sizeof(char)*strlen(traceFileName) + 1 );
  strcpy( m_filename, traceFileName );

  m_fp = fopen( traceFileName, "r" );
  if (m_fp == NULL) {
    ERROR_OUT("error opening file: %s\n", traceFileName);
    exit(1);
  } else {
    fclose( m_fp );
    sprintf( cmd, "/s/std/bin/zcat %s", traceFileName );
    m_fp = popen( cmd, "r" );
    m_isReading = true;

    int nread = myfread( &vnumber, sizeof(int), 1, m_fp );
    if ( nread != 1 || vnumber != TRACE_MAGIC_NUM ) {
      ERROR_OUT("error: log file format unknown: %s 0x%0x\n", 
             traceFileName, vnumber);
    }

    // read the initial context
    nread = myfread( &vnumber, sizeof(int), 1, m_fp );
    if (nread != 1 || vnumber != TRACE_CTX_SWITCH) {
      ERROR_OUT("error: log file format unknown: missing ctx control token\n");
    }  

    // read the new context in
    nread = myfread( &vnumber, sizeof(int), 1, m_fp );
    if (nread != 1) {
      ERROR_OUT("tracefile: log file format unknown\n");
    }
    *context_p = vnumber;

    // Task: initialize the processor state
    //   a) allocate, zero out the logged processor state
    m_state = new core_state_t();
    memset( m_state, 0, sizeof(core_state_t) );
    m_sim_state = sim_state;
    memset( m_sim_state, 0, sizeof(core_state_t) );

    //   b) read the initial state from the file
    uint32 context = *context_p;
    bool success = readStateChange( context );
    if (!success || context != *context_p) { 
      ERROR_OUT("warning: context switch on first instruction?\n");
    }
    // now successfully opened!!
    DEBUG_OUT( "tracefile_t: successfully opened for reading\n");
  }
}

/** opens a trace for writing */
//**************************************************************************
tracefile_t::tracefile_t( char *traceFileName,
                          core_state_t *sim_state,
                          uint32 context )
{
  int32  index = 0;
  char   filename[FILEIO_MAX_FILENAME];
  char   testname[FILEIO_MAX_FILENAME];
  char   cmd[FILEIO_MAX_FILENAME];

  // allocate, zero out the logged processor state
  m_state = new core_state_t();
  memset( m_state, 0, sizeof(core_state_t) );

  // NULL is a valid filename, select a random file name
  if (traceFileName == NULL) {
    sprintf(filename, "%s%s", TRACE_DIRNAME, TRACE_FILENAME);
  } else {
    sprintf(filename, "%s%s", TRACE_DIRNAME, traceFileName);
  }

  if (!findSeqFilename( filename, testname, &index )) {
    ERROR_OUT("error opening tracefile: aborting!\n");
    exit(1);
  }
  m_filename = (char *) malloc( sizeof(char)*(strlen(testname)+1) );
  strcpy( m_filename, testname );

  sprintf( cmd, "/s/std/bin/gzip > %s", m_filename );
  m_fp = popen( cmd, "w" );
  if (m_fp == NULL) {
    // print warning...
    ERROR_OUT("warning unable to open file %s\n", m_filename);

  } else {
    DEBUG_OUT("opening file %s\n", m_filename);
    // write log header
    int magic_number = TRACE_MAGIC_NUM;
    int numwritten = myfwrite( &magic_number, sizeof(int), 1, m_fp );
    if (numwritten != 1) {
      ERROR_OUT("error writing file: magic number write fails\n");
    }
    if (!writeContextSwitch(context)) {
      ERROR_OUT("error writing context switch!\n");
    }
  }
  m_sim_state = sim_state;
  m_isReading = false;
}

//**************************************************************************
tracefile_t::~tracefile_t( )
{
  if (m_fp) {
    int rc = pclose(m_fp);
    if (rc < 0) {
      ERROR_OUT("error closing file: %d\n", rc);
    }
    m_fp = NULL;
  }

  if (m_filename) {
    DEBUG_OUT("closing file %s\n", m_filename);
    free( m_filename );
    m_filename = NULL;
  }

  if (m_state) {
    delete m_state;
    m_state = NULL;
  }
}

/*------------------------------------------------------------------------*/
/* Public methods                                                         */
/*------------------------------------------------------------------------*/

//**************************************************************************
bool tracefile_t::writeTracePC( la_t curPC, pa_t physPC,
                                unsigned int nextinstr, uint32 local_time )
{
  unsigned int mytoken;
  int          numwritten;

  // write a PC/instr/mod time stamp (token?)
  mytoken = TRACE_PC_TOKEN;
  numwritten = myfwrite( &mytoken, sizeof(int), 1, m_fp );
  if (numwritten < 1) {
    ERROR_OUT("write fails: token\n");
    return (false);
  }

  // write pc && instruction to file
#if 0
  // DEBUG
  DEBUG_OUT("     0x%0llx %lld\n", curPC, curPC);
#endif

  ASSERT( sizeof(uint64) == sizeof(la_t) );
  numwritten = myfwrite( &curPC, sizeof(uint64), 1, m_fp );
  if (numwritten < 1) {
    ERROR_OUT("write fails: PC\n");
    return (false);
  }
  ASSERT( sizeof(uint64) == sizeof(pa_t) );
  numwritten = myfwrite( &physPC, sizeof(uint64), 1, m_fp );
  if (numwritten < 1) {
    ERROR_OUT("write fails: PC\n");
    return (false);
  }
  numwritten = myfwrite( &nextinstr, sizeof(unsigned int), 1, m_fp );
  if (numwritten < 1) {
    ERROR_OUT("write fails: instr\n");
    return (false);
  }
  numwritten = myfwrite( &local_time, sizeof(uint32), 1, m_fp );
  if (numwritten < 1) {
    ERROR_OUT("write fails: instr\n");
    return (false);
  }

  return (true);  
}

//**************************************************************************
bool tracefile_t::writeTraceStep( la_t curPC, pa_t physPC,
                                  unsigned int nextinstr, uint32 local_time )
{
  unsigned int mytoken;
  int          numwritten;
  
  //
  // write modifications between the architected state and the trace state
  //
  mytoken = TRACE_REG_BASE;
  uint64 *p1   = (uint64 *)  m_sim_state;
  uint64 *p2   = (uint64 *)  m_state;
  uint64 *endp = (uint64 *) ((char *)p1 + sizeof(core_state_t));

  /* DEBUG_OUT("start mod 0x%0x 0x%0x\n", (int) p1, (int) p2); */
  while ( p1 < endp ) {
    if ( *p1 != *p2 ) {
      /* DEBUG_OUT("log mod 0x%0x 0x%0x data 0x%0x 0x%0x\n", 
             (int) p1, (int) p2, *p1, *p2);
      */

      // emit token
      numwritten = myfwrite( &mytoken, sizeof(int), 1, m_fp );
      if (numwritten != 1) {
        ERROR_OUT("error: disk write error: %d\n", numwritten);
        return (false);
      }
      
      // write register
      numwritten = myfwrite( p1, sizeof(uint64), 1, m_fp );
      if (numwritten != 1) {
        ERROR_OUT("error: disk write error: %d\n", numwritten);
        return (false);
      }
      
      // update the logged state
      *p2 = *p1;
    }
    mytoken++;
    p1++;
    p2++;
  }

  return (writeTracePC( curPC, physPC, nextinstr, local_time ));
}

//***************************************************************************
bool   tracefile_t::writeContextSwitch( uint32 new_context )
{
  unsigned int mytoken;
  int          numwritten;

  // write a ctx switch token/new ctx pair
  mytoken = TRACE_CTX_SWITCH;
  numwritten = myfwrite( &mytoken, sizeof(unsigned int), 1, m_fp );
  if (numwritten < 1) {
    ERROR_OUT("write fails: PC\n");
    return (false);
  }
  
  // write to file
  numwritten = myfwrite( &new_context, sizeof(uint32), 1, m_fp );
  if (numwritten < 1) {
    ERROR_OUT("write fails: instr\n");
    return (false);
  }
  
  return (true);
}

//**************************************************************************
bool tracefile_t::readTraceStep( la_t &vpc, pa_t &ppc, unsigned int &instr,
                                 uint32 &localtime, uint32 &context )
{
  bool success;
#ifdef DEBUG_TESTER
  core_state_t  oldstate;
#endif

  success = readPC( vpc, ppc, instr, localtime );
  if (!success)
    return (success);
#ifdef DEBUG_TESTER
  memcpy( &oldstate, m_sim_state, sizeof(core_state_t) );
#endif
  success = readStateChange( context );
#ifdef DEBUG_TESTER
  DEBUG_OUT("TRACESTEP\n");
  bool aresame = ps_compare( &oldstate, m_sim_state );
  if (aresame)
    DEBUG_OUT("TRACESTEP SAME\n");
  else
    DEBUG_OUT("TRACESTEP DIFFERENT\n");
#endif
  return (success);
}

//**************************************************************************
void tracefile_t::validate( void )
{
  int         numread;
  int         nexttoken;
  int         reg_data;
  la_t        virtual_pc;

  if (m_fp == NULL) {
    ERROR_OUT("unable to validate: file not open\n");
    return;
  }

  for ( ;; ) {

    // read the control token from the file
    numread = myfread( &nexttoken, sizeof(int), 1, m_fp );
    if (numread != 1) {
      ERROR_OUT("tracefile: readStep: error reading (%d)\n", numread);
      return;
    }
    
    if (nexttoken == TRACE_PC_TOKEN) {

      // read the current PC
      numread = myfread( &virtual_pc, sizeof(la_t), 1, m_fp );
      if (numread != 1) {
        ERROR_OUT("tracefile: readStep: error reading (%d)\n", numread);
        return;
      }

      numread = myfread( &reg_data, sizeof(unsigned int), 1, m_fp );
      if (numread != 1) {
        ERROR_OUT("tracefile: readStep: error reading (%d)\n", numread);
        return;
      }

      DEBUG_OUT("validate PC 0x%0llx 0x%0x\n", virtual_pc, reg_data);

    } else if (nexttoken > TRACE_REG_BASE && nexttoken < TRACE_REG_MAX) {
      
      // read the register modification in
      numread = myfread( &reg_data, sizeof(int), 1, m_fp );
      if (numread != 1) {
        ERROR_OUT("tracefile: readStep: error reading (%d)\n", numread);
        return;
      }
      DEBUG_OUT("log mod 0x%0x 0x%0x\n", (nexttoken - TRACE_REG_BASE), reg_data);
      
    } else {
      ERROR_OUT("unrecognized token %d\n", nexttoken);
    }

  }
}

/*------------------------------------------------------------------------*/
/* Accessor(s) / mutator(s)                                               */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Private methods                                                        */
/*------------------------------------------------------------------------*/

//**************************************************************************
bool  tracefile_t::readStateChange( uint32 &context )
{
  int         numread;
  int         nexttoken;
  uint64      reg_data;
  uint64     *ps = (uint64 *) m_sim_state;

  //DEBUG_OUT("read step");
  for ( ;; ) {

    //DEBUG_OUT(".");
    // read the next control token
    numread = myfread( &nexttoken, sizeof(int), 1, m_fp );
    if (numread != 1) {
      if (feof(m_fp))
        return (false);
      ERROR_OUT("tracefile: readStep: error reading (%d)\n", numread);
      return (false);
    }
    
    if (nexttoken == TRACE_PC_TOKEN) {

      break;

    } else if (nexttoken >= TRACE_REG_BASE && nexttoken <= TRACE_REG_MAX) {

      // read the register modification in
      numread = myfread( &reg_data, sizeof(uint64), 1, m_fp );
      if (numread != 1) {
        ERROR_OUT("tracefile: readStep: error reading (%d)\n", numread);
        return (false);
      }
      int offset = nexttoken - TRACE_REG_BASE;
      if ( offset < 0 || offset*sizeof(int) > sizeof(core_state_t) ) {
        DEBUG_OUT("strange offset = %d (%d). ignoring.\n", offset,
               sizeof(core_state_t));
        return (false);
      }
      ps[offset] = reg_data;
      
    } else if (nexttoken == TRACE_CTX_SWITCH) {
      
      // read the new context in
      numread = myfread( &context, sizeof(uint32), 1, m_fp );
      if (numread != 1) {
        ERROR_OUT("tracefile: readStep: error reading (%d)\n", numread);
        return (false);
      }
      
    } else {
      ERROR_OUT("unrecognized token 0x%0x\n", nexttoken);
      return (false);
    }
  }
  return (true);
}

//**************************************************************************
bool  tracefile_t::readPC( la_t &vpc, pa_t &ppc, unsigned int &instr, 
                           uint32 & localtime )
{
  int         numread;

  // read the PC and instruction from the trace file...
  numread = myfread( &vpc, sizeof(la_t), 1, m_fp );
  if (numread != 1) {
    if (feof(m_fp))
      return (false);
    ERROR_OUT("tracefile: readStep: error reading (%d)\n", numread);
    return (false);
  }

  ASSERT( sizeof(uint64) == sizeof(pa_t) );
  numread = myfread( &ppc, sizeof(la_t), 1, m_fp );
  if (numread != 1) {
    ERROR_OUT("tracefile: readStep: error reading (%d)\n", numread);
    return (false);
  }

  numread = myfread( &instr, sizeof(int), 1, m_fp );
  if (numread != 1) {
    ERROR_OUT("tracefile: readStep: error reading (%d)\n", numread);
    return (false);
  }

  numread = myfread( &localtime, sizeof(uint32), 1, m_fp );
  if (numread != 1) {
    ERROR_OUT("tracefile: readStep: error reading (%d)\n", numread);
    return (false);
  }

  // DEBUG_OUT("\nadvancing 0x%0llx 0x%0x\n", vpc, instr);
  return (true);
}

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

