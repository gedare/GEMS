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
 * FileName:  branchfile.C
 * Synopsis:  implementation of a compressed branch trace
 * Author:    cmauer
 * Version:   $Id: branchfile.C 1.7 03/07/10 18:07:50-00:00 milo@cottons.cs.wisc.edu $
 */

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include "hfa.h"
#include "hfacore.h"

#include "fileio.h"
#include "branchfile.h"

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

/// base filename to save traces into
const char *BRTRACE_DIRNAME      = "/scratch.local/";
const char *BRTRACE_FILENAME     = "trace";

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
branchfile_t::branchfile_t( char *traceFileName, uint32 *context_p,
                            uint32 startIndex )
{
  ASSERT (traceFileName != NULL);
  m_base_filename = (char *) malloc( sizeof(char)*(strlen(traceFileName)+1) );
  strcpy( m_base_filename, traceFileName );
  m_fp_index = startIndex;
  m_context = context_p;
  readFile();
}

/** opens a trace for writing */
//**************************************************************************
branchfile_t::branchfile_t( char *traceFileName, uint32 context )
{
  char   filename[FILEIO_MAX_FILENAME];

  DEBUG_OUT("writing branch file\n");
  // allocate, zero out the logged processor state

  // NULL is a valid filename, select a random file name
  if (traceFileName == NULL) {
    sprintf(filename, "%s%s", BRTRACE_DIRNAME, BRTRACE_FILENAME);
  } else {
    sprintf(filename, "%s%s", BRTRACE_DIRNAME, traceFileName);
  }
  
  m_fp_index = 0;
  m_base_filename = (char *) malloc( sizeof(char)*(strlen(filename)+1) );
  strcpy( m_base_filename, filename );
  m_current_context = context;
  m_last_taken     = false;
  m_last_interrupt = false;
  writeFile();
}

//**************************************************************************
branchfile_t::~branchfile_t( )
{
  if (m_base_filename) {
    free( m_base_filename );
    m_base_filename = NULL;
  }
  closeFile();
}

/*------------------------------------------------------------------------*/
/* Public methods                                                         */
/*------------------------------------------------------------------------*/

//**************************************************************************
bool branchfile_t::writeBranch( my_addr_t curPC, my_addr_t curNPC,
                                bool traplevel, static_inst_t *s_instr )
{
  char         mytoken;
  int          numwritten;

  // write a token
  mytoken = BRTRACE_BRANCH;
  if (m_last_taken) {
    mytoken = mytoken | BRTRACE_TAKEN;
  }
  if (traplevel) {
    mytoken = mytoken | BRTRACE_TRAPLEVEL;
  }
  if (m_last_interrupt) {
    mytoken = mytoken | BRTRACE_INTERRUPT;
  }
  m_last_taken = false;
  m_last_interrupt = false;

  numwritten = myfwrite( &mytoken, sizeof(char), 1, m_fp );
  if (numwritten < 1) {
    ERROR_OUT("write fails: token\n");
    return (false);
  }

  // write virtual PC of this instruction
  ASSERT( sizeof(uint64) == sizeof(my_addr_t) );
  numwritten = myfwrite( &curPC, sizeof(uint64), 1, m_fp );
  if (numwritten < 1) {
    ERROR_OUT("write fails: PC\n");
    return (false);
  }

  // calculate the prediction for the next branch
  m_check_branch = true;
  m_check_taken_pc  = curNPC;
  m_check_taken_npc = s_instr->getOffset();
  if (s_instr->getAnnul()) {
    // cond, not taken, annulled
    m_check_nottaken_pc  = curNPC + sizeof(uint32);
    m_check_nottaken_npc = curNPC + (2*sizeof(uint32));
  } else {
    // cond, not taken, not annulled
    m_check_nottaken_pc  = curNPC;
    m_check_nottaken_npc = curNPC + sizeof(uint32);
  }

  return (true);
}

//**************************************************************************
bool  branchfile_t::writeChangedBranch( my_addr_t curPC, my_addr_t curNPC,
                                        pa_t curPPC, bool traplevel,
                                        static_inst_t *s_instr )
{
  char         mytoken;
  int          numwritten;
  uint32       nextinstr;

  // write a token
  mytoken = BRTRACE_NEW_BRANCH;
  if (m_last_taken) {
    mytoken = mytoken | BRTRACE_TAKEN;
  }
  if (traplevel) {
    mytoken = mytoken | BRTRACE_TRAPLEVEL;
  }
  if (m_last_interrupt) {
    mytoken = mytoken | BRTRACE_INTERRUPT;
  }
  m_last_taken = false;
  m_last_interrupt = false;

  numwritten = myfwrite( &mytoken, sizeof(char), 1, m_fp );
  if (numwritten < 1) {
    ERROR_OUT("write fails: token\n");
    return (false);
  }

  // write virtual PC of this instruction
  ASSERT( sizeof(uint64) == sizeof(my_addr_t) );
  numwritten = myfwrite( &curPC, sizeof(uint64), 1, m_fp );
  if (numwritten < 1) {
    ERROR_OUT("write fails: PC\n");
    return (false);
  }

  ASSERT( sizeof(uint64) == sizeof(pa_t) );
  numwritten = myfwrite( &curPPC, sizeof(uint64), 1, m_fp );
  if (numwritten < 1) {
    ERROR_OUT("write fails: PC\n");
    return (false);
  }

  nextinstr = s_instr->getInst();
  numwritten = myfwrite( &nextinstr, sizeof(unsigned int), 1, m_fp );
  if (numwritten < 1) {
    ERROR_OUT("write fails: instr\n");
    return (false);
  }

  // calculate the prediction for the next branch
  m_check_branch = true;
  m_check_taken_pc  = curNPC;
  m_check_taken_npc = s_instr->getOffset();
  if (s_instr->getAnnul()) {
    // cond, not taken, annulled
    m_check_nottaken_pc  = curNPC + sizeof(uint32);
    m_check_nottaken_npc = curNPC + (2*sizeof(uint32));
  } else {
    // cond, not taken, not annulled
    m_check_nottaken_pc  = curNPC;
    m_check_nottaken_npc = curNPC + sizeof(uint32);
  }

  return (true);  
}


//***************************************************************************
bool   branchfile_t::writeContextSwitch( uint32 new_context )
{
  char         mytoken;
  int          numwritten;

  // write a ctx switch token
  mytoken = BRTRACE_CTX_SWITCH;
  numwritten = myfwrite( &mytoken, sizeof(char), 1, m_fp );
  if (numwritten < 1) {
    ERROR_OUT("write fails: PC\n");
    return (false);
  }
  
  // write to file
  m_current_context = new_context;
  numwritten = myfwrite( &m_current_context, sizeof(uint32), 1, m_fp );
  if (numwritten < 1) {
    ERROR_OUT("write fails: instr\n");
    return (false);
  }
  
  return (true);
}

//**************************************************************************
bool  branchfile_t::readBranch( branch_record_t *rec )
{
  int         numread;
  my_addr_t   vpc;
  pa_t        ppc;
  uint32      instr;

  if (m_fp == NULL) {
    ERROR_OUT("unable to read branch: file not open\n");
    return (false);
  }

  rec->tlset       = (m_last_token & BRTRACE_TRAPLEVEL) != 0;
  rec->interrupted = (m_last_token & BRTRACE_INTERRUPT) != 0;
  m_last_token = m_last_token & BRTRACE_BITMASK;
  rec->rectype = m_last_token;
  if (m_last_token == BRTRACE_BRANCH) {

    // read the current PC
    numread = myfread( &ppc, sizeof(my_addr_t), 1, m_fp );
    if (numread != 1) {
      ERROR_OUT("branchfile: readStep: error reading branch vpc (%d)\n", numread);
      return (false);
    }
    rec->ppc = ppc;
    // ERROR_OUT("validate PC 0x%0llx\n", vpc);

  } else if (m_last_token == BRTRACE_NEW_BRANCH) {

    ASSERT( sizeof(uint64) == sizeof(my_addr_t) );
    numread = myfread( &vpc, sizeof(uint64), 1, m_fp );
    if (numread != 1) {
      if (feof(m_fp))
        return (false);
      ERROR_OUT("branchfile: readStep: error reading vpc (%d)\n", numread);
      return (false);
    }

    ASSERT( sizeof(uint64) == sizeof(pa_t) );
    numread = myfread( &ppc, sizeof(uint64), 1, m_fp );
    if (numread != 1) {
      ERROR_OUT("branchfile: readStep: error reading ppc (%d)\n", numread);
      return (false);
    }
      
    numread = myfread( &instr, sizeof(int), 1, m_fp );
    if (numread != 1) {
      ERROR_OUT("branchfile: readStep: error reading instr (%d)\n", numread);
      return (false);
    }

    rec->vpc = vpc;
    rec->ppc = ppc;
    rec->instr = instr;
    // DEBUG_OUT("validate: PC: 0x%0llx PPC: 0x%0llx instr: 0x%0x\n", vpc, ppc, instr);

  } else if (m_last_token == BRTRACE_CTX_SWITCH) {

    numread = myfread( m_context, sizeof(int), 1, m_fp );
    if (numread != 1) {
      ERROR_OUT("branchfile: readStep: error reading switch (%d)\n", numread);
      return (false);
    }
    //DEBUG_OUT("read context: %d\n", *m_context);
      

  } else if (m_last_token == BRTRACE_FILE_SWITCH) {

    // close the current file, open the next file
    closeFile();
    readFile();
    return (readBranch(rec));

  } else {
    ERROR_OUT("unrecognized token %d\n", m_last_token);
    return (false);
  }

  // read the control token from the file
  numread = myfread( &m_last_token, sizeof(char), 1, m_fp );
  if (numread != 1) {
    if (feof(m_fp)) {
      return (false);
    }
    ERROR_OUT("branchfile: readStep: error reading last control (%d)\n", numread);
    return (false);
  }

  rec->result = m_last_token & BRTRACE_TAKEN;
  m_last_token = m_last_token & ~BRTRACE_TAKEN;
  return true;
}

//**************************************************************************
bool  branchfile_t::writeNextFile( void )
{
  char         mytoken;
  int          numwritten;
  
  // terminate, then close the current file.
  if (m_check_branch == false) {

    mytoken = BRTRACE_FILE_SWITCH;
    if (m_last_taken) {
      mytoken = mytoken | BRTRACE_TAKEN;
    }
    if (m_last_interrupt) {
      mytoken = mytoken | BRTRACE_INTERRUPT;
    }
    m_last_taken = false;
    m_last_interrupt = false;
  
    numwritten = myfwrite( &mytoken, sizeof(char), 1, m_fp );
    if (numwritten < 1) {
      ERROR_OUT("write fails: token\n");
      return (false);
    }
    
  } else {
    // last branch hasn't been checked yet...
    return false;
  }

  // close file
  closeFile();

  // open next sequential file
  writeFile( );
  
  return (true);
}

//**************************************************************************
bool branchfile_t::validate( void )
{
  int         numread;
  uint32      context;
  uint32      instr;
  my_addr_t   vpc;
  pa_t        ppc;

  if (m_fp == NULL) {
    ERROR_OUT("unable to validate: file not open\n");
    return (false);
  }

  for ( ;; ) {

    if (m_last_token == BRTRACE_BRANCH) {

      // read the current PC
      numread = myfread( &vpc, sizeof(my_addr_t), 1, m_fp );
      if (numread != 1) {
        ERROR_OUT("branchfile: readStep: error reading (%d)\n", numread);
        return (false);
      }
      // DEBUG_OUT("validate PC 0x%0llx\n", vpc);

    } else if (m_last_token == BRTRACE_NEW_BRANCH) {

      ASSERT( sizeof(uint64) == sizeof(my_addr_t) );
      numread = myfread( &vpc, sizeof(uint64), 1, m_fp );
      if (numread != 1) {
        if (feof(m_fp))
          return (false);
        ERROR_OUT("branchfile: readStep: error reading (%d)\n", numread);
        return (false);
      }

      ASSERT( sizeof(uint64) == sizeof(pa_t) );
      numread = myfread( &ppc, sizeof(uint64), 1, m_fp );
      if (numread != 1) {
        ERROR_OUT("branchfile: readStep: error reading (%d)\n", numread);
        return (false);
      }
      
      numread = myfread( &instr, sizeof(int), 1, m_fp );
      if (numread != 1) {
        ERROR_OUT("branchfile: readStep: error reading (%d)\n", numread);
        return (false);
      }

      /* DEBUG_OUT("validate: PC: 0x%0llx PPC: 0x%0llx instr: 0x%0x\n",
         vpc, ppc, instr);
      */

    } else if (m_last_token == BRTRACE_CTX_SWITCH) {

      numread = myfread( &context, sizeof(int), 1, m_fp );
      if (numread != 1) {
        ERROR_OUT("branchfile: readStep: error reading (%d)\n", numread);
        return (false);
      }
      // DEBUG_OUT("read context: %d\n", context);

    } else if (m_last_token == BRTRACE_FILE_SWITCH) {

      ERROR_OUT("read file switch: \n");

    } else {
      ERROR_OUT("unrecognized token %d\n", m_last_token);
    }

    // read the control token from the file
    numread = myfread( &m_last_token, sizeof(char), 1, m_fp );
    if (numread != 1) {
      if (feof(m_fp)) {
        // DEBUG_OUT("endoffile\n");
        return (false);
      }
      ERROR_OUT("branchfile: readStep: error reading (%d)\n", numread);
      return (false);
    }

    // bool lastTaken = m_last_token & BRTRACE_TAKEN;
    // DEBUG_OUT("last taken = %d\n", lastTaken);
    m_last_token = m_last_token & ~BRTRACE_TAKEN;
  }
  return true;
}

/*------------------------------------------------------------------------*/
/* Accessor(s) / mutator(s)                                               */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Private methods                                                        */
/*------------------------------------------------------------------------*/

//**************************************************************************
void branchfile_t::writeFile( void )
{
  char   testname[FILEIO_MAX_FILENAME];
  char   cmd[FILEIO_MAX_FILENAME];

  if (!findSeqFilename( m_base_filename, testname, &m_fp_index )) {
    ERROR_OUT("error opening branch trace file: aborting!\n");
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
    DEBUG_OUT("opening file: %s\n", m_filename);
    fflush(stdout);

    // write log header
    uint32 magic_number = BRTRACE_MAGIC_NUM;
    int    numwritten = myfwrite( &magic_number, sizeof(uint32), 1, m_fp );
    if (numwritten != 1) {
      ERROR_OUT("error writing file: magic number write fails\n");
    }
    if (!writeContextSwitch(m_current_context)) {
      ERROR_OUT("error writing context switch!\n");
    }
  }
  m_isReading = false;
}

//**************************************************************************
void branchfile_t::readFile( void )
{
  char  mytoken;
  int   vnumber;
  char  filename[FILEIO_MAX_FILENAME];
  char  cmd[FILEIO_MAX_FILENAME];

  m_isReading = true;
  nextSeqFilename( m_base_filename, filename, &m_fp_index );
  DEBUG_OUT("opening file: %s\n", filename);
  m_fp = fopen( filename, "r" );
  if (m_fp == NULL) {
    ERROR_OUT("error opening file: %s\n", filename);
    return;
  }
  fflush(stdout);
  
  m_filename = (char *) malloc( sizeof(char)*strlen(filename) + 1 );
  strcpy( m_filename, filename );
  sprintf( cmd, "/s/std/bin/zcat %s", filename );
  m_fp = popen( cmd, "r" );
  int nread = myfread( &vnumber, sizeof(uint32), 1, m_fp );
  if ( nread != 1 || vnumber != BRTRACE_MAGIC_NUM ) {
    ERROR_OUT("error: log file format unknown: %s 0x%0x\n", 
           filename, vnumber);
  }

  // read the initial context
  nread = myfread( &mytoken, sizeof(char), 1, m_fp );
  if (nread != 1 || mytoken != BRTRACE_CTX_SWITCH) {
    ERROR_OUT("error: log file format unknown: missing ctx control token\n");
  }  
  
  // read the new context in
  nread = myfread( &vnumber, sizeof(uint32), 1, m_fp );
  if (nread != 1) {
    ERROR_OUT("tracefile: log file format unknown\n");
  }
  m_current_context = vnumber;
  *m_context = vnumber;
  DEBUG_OUT("initial context: %d\n", m_current_context);

  // read the first control byte
  nread = myfread( &mytoken, sizeof(char), 1, m_fp );
  if (nread != 1) {
    ERROR_OUT("error: unexpected end of log file.\n");
  }  

  // mask off low order bit from control byte
  m_last_token = mytoken & BRTRACE_BITMASK;
  // now successfully opened!!
}

//**************************************************************************
void branchfile_t::closeFile()
{
  if (m_fp) {
    int rc = pclose(m_fp);
    if (rc < 0) {
      ERROR_OUT("error closing file: %d\n", rc);
    }
    m_fp = NULL;
  }

  if (m_filename) {
    DEBUG_OUT("closed file: %s\n", m_filename);
    free( m_filename );
    m_filename = NULL;
  }
}

//**************************************************************************
void branchfile_t::print( branch_record_t *record )
{
  DEBUG_OUT("vpc: 0x%0llx\n", record->vpc);
  DEBUG_OUT("ppc: 0x%0llx\n", record->ppc);
  DEBUG_OUT("int: 0x%0x\n", record->instr);
  DEBUG_OUT("res: %d\n", record->result);
  DEBUG_OUT("tl : %d\n", record->tlset);
  DEBUG_OUT("int: %d\n", record->interrupted);
  DEBUG_OUT("type: %d\n", record->rectype);
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
