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
 * FileName:  memtrace.C
 * Synopsis:  implementation of a memory tracing program
 * Author:    cmauer
 * Version:   $Id: memtrace.C 1.14 03/07/10 18:07:51-00:00 milo@cottons.cs.wisc.edu $
 */

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include "hfa.h"
#include "transaction.h"
#include "fileio.h"
#include "memtrace.h"

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

/// base filename to save mem traces into
const char *MEMTRACE_DIRNAME         = "/scratch.local/";
// APPEND_STRING is prefered to F.I.X. as the later is an important tag
const char *MEMTRACE_APPEND_STRING   = "mem-";
const char *MEMTRACE_FILENAME        = "trace";

/*------------------------------------------------------------------------*/
/* Variable declarations                                                  */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Forward declarations                                                   */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Constructor(s) / destructor                                            */
/*------------------------------------------------------------------------*/

/** Constructor: opens a trace for reading */
//**************************************************************************
memtrace_t::memtrace_t( char *traceFileName )
{
  char  cmd[FILEIO_MAX_FILENAME];

  if (traceFileName == NULL) {
    ERROR_OUT("error: NULL filename passed to tracefile\n");
    SIM_HALT;
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

    int vnumber;
    int nread = myfread( &vnumber, sizeof(int), 1, m_fp );
    if ( nread != 1 || vnumber != MEMTRACE_MAGIC_NUM ) {
      ERROR_OUT("error: mem trace file format unknown: %s 0x%0x\n", 
             traceFileName, vnumber);
    }

    // read the first winsize transactions
    bool           success = true;
    int            index = 0;
    transaction_t *xaction;
    m_window = (transaction_t **) malloc( sizeof(transaction_t *) *
                                          MEMTRACE_WINSIZE );
    while (success && index < MEMTRACE_WINSIZE) {

      xaction = new transaction_t();
      success = readTransaction( xaction );
      if (success) {
        m_window[index] = xaction;
        index++;
      }
    }
  
    // initialize the rest of the positions with "NULL"
    while (index < MEMTRACE_WINSIZE) {
      m_window[index] = NULL;
      index++;
    }
  }  
}

/** Constructor: opens a trace for writing */
//***************************************************************************
memtrace_t::memtrace_t( char *traceFileName, bool doWrite )
{
  int32  index = 0;
  char   filename[FILEIO_MAX_FILENAME];
  char   testname[FILEIO_MAX_FILENAME];
  char   cmd[FILEIO_MAX_FILENAME];

  // NULL is a valid filename, select a random file name
  if (traceFileName == NULL) {
    sprintf(filename, "%s%s%s", MEMTRACE_DIRNAME, MEMTRACE_APPEND_STRING,
            MEMTRACE_FILENAME);
  } else {
    sprintf(filename, "%s%s%s", MEMTRACE_DIRNAME, MEMTRACE_APPEND_STRING,
            traceFileName);
  }

  if (!findSeqFilename( filename, testname, &index )) {
    ERROR_OUT("error opening memtrace file: aborting!\n");
    exit(1);
  }
  m_filename = (char *) malloc( sizeof(char)*(strlen(testname)+1) );
  strcpy( m_filename, testname );
  sprintf( cmd, "/s/std/bin/gzip > %s", m_filename );
  m_fp = popen( cmd, "w" );
#if 0
  m_fp = fopen( m_filename, "w" );
#endif
  if (m_fp == NULL) {
    // print warning...
    ERROR_OUT("warning unable to open file %s\n", m_filename);

  } else {
    DEBUG_OUT("opening file %s\n", m_filename);
    // write log header
    int magic_number = MEMTRACE_MAGIC_NUM;
    int numwritten = myfwrite( &magic_number, sizeof(int), 1, m_fp );
    if (numwritten != 1) {
      ERROR_OUT("error writing file: magic number write fails\n");
    }
  }
  m_isReading = false;
  m_window = NULL;
}

//**************************************************************************
memtrace_t::~memtrace_t( )
{
  if (m_fp) {
    int rc = pclose(m_fp);
#if 0
    int rc = fclose(m_fp);
#endif
    if (rc < 0) {
      ERROR_OUT("error closing memop file: %d\n", rc);
    }
    m_fp = NULL;
  }
  // free outstanding transactions (if reading)
  if (m_window) {
    for (int i = 0; i < MEMTRACE_WINSIZE; i++) {
      if (m_window[i] != NULL)
        delete m_window[i];
    }
    free( m_window );
  }

  if (m_filename) {
    DEBUG_OUT("closing memop trace file %s\n", m_filename);
    free( m_filename );
    m_filename = NULL;
  }
}

/*------------------------------------------------------------------------*/
/* Public methods                                                         */
/*------------------------------------------------------------------------*/

/** write a memory operation to a file */
//**************************************************************************
bool   memtrace_t::writeTransaction( la_t curPC, uint32 instr,
                                     uint32 localtime,
                                     transaction_t *xaction )
{
  unsigned int mytoken;
  int          numwritten;
  uint16       mysize;
  uint16       value;

  mytoken = MEMTRACE_TOKEN;
  // write the token
  numwritten = myfwrite( &mytoken, sizeof(int32), 1, m_fp );
  if (numwritten < 1) {
    ERROR_OUT("write fails: Memory Operation\n");
    return (false);
  }

  // write the program counter
  numwritten = myfwrite( &curPC, sizeof(la_t), 1, m_fp );
  if (numwritten < 1) {
    ERROR_OUT("write fails: PC\n");
    return (false);
  }

  // write the instruction at this program counter
  numwritten = myfwrite( &instr, sizeof(uint32), 1, m_fp );
  if (numwritten < 1) {
    ERROR_OUT("write fails: instr\n");
    return (false);
  }

  // write the local time
  numwritten = myfwrite( &localtime, sizeof(uint32), 1, m_fp );
  if (numwritten < 1) {
    ERROR_OUT("write fails: instr\n");
    return (false);
  }

  // write the physical address to the file
  pa_t physical_address = xaction->getPhysical();
  numwritten = myfwrite( &physical_address, sizeof(pa_t), 1, m_fp );
  if (numwritten < 1) {
    ERROR_OUT("write fails: Memory Operation\n");
    return (false);
  }

  // write the logical address to the file
  la_t logical_address = xaction->getVirtual();
  numwritten = myfwrite( &logical_address, sizeof(la_t), 1, m_fp );
  if (numwritten < 1) {
    ERROR_OUT("write fails: Memory Operation\n");
    return (false);
  }

  // write the size of the transaction
  mysize = xaction->getSize();
  numwritten = myfwrite( &mysize, sizeof(uint16), 1, m_fp );
  if (numwritten < 1) {
    ERROR_OUT("write fails: Memory Operation\n");
    return (false);
  }

  // write load or store
  value = xaction->getAction();
  numwritten = myfwrite( &value, sizeof(uint16), 1, m_fp );
  if (numwritten < 1) {
    ERROR_OUT("write fails: Memory Operation\n");
    return (false);
  }

  // write data or instruction
  value = xaction->getIorD();
  numwritten = myfwrite( &value, sizeof(uint16), 1, m_fp );
  if (numwritten < 1) {
    ERROR_OUT("write fails: Memory Operation\n");
    return (false);
  }

  // write user or super
  value = xaction->getSuper();
  numwritten = myfwrite( &value, sizeof(uint16), 1, m_fp );
  if (numwritten < 1) {
    ERROR_OUT("write fails: Memory Operation\n");
    return (false);
  }

  // write atomic information
  value = xaction->getAtomic();
  numwritten = myfwrite( &value, sizeof(uint16), 1, m_fp );
  if (numwritten < 1) {
    ERROR_OUT("write fails: Memory Operation\n");
    return (false);
  }

  if (mysize > 0) {
    // write the actual data to the file
    const integer_t *data = xaction->getData();
    for (int i = 0; i < xaction->getNumWords(); i++) {
      numwritten = myfwrite( &data[i], sizeof(integer_t), 1, m_fp );
      if (numwritten < 1) {
        ERROR_OUT("write fails: Memory Operation\n");
        return (false);
      }
    }
  }
  return (true);
}


/** read a memory operation to a file */
//**************************************************************************
bool   memtrace_t::readTransaction( transaction_t *xaction )
{
  int32    mytoken = -1;
  int      numread;
  la_t     pc;
  uint32   instr;
  uint32   localtime;
  pa_t     physical_address;
  la_t     logical_address;
  uint16   mysize;
  uint16   load_store;
  uint16   data_instr;
  uint16   user_super;
  uint16   atomic_flags;

  // read the token
  numread = myfread( &mytoken, sizeof(int32), 1, m_fp );
  if (numread != 1) {
    if (feof(m_fp))
      return (false);
    ERROR_OUT("read fails: Memory Operation\n");
    return (false);
  }

  if (mytoken != MEMTRACE_TOKEN) {
    ERROR_OUT("file format error: not the memtrace token %d != %d.\n",
           mytoken, MEMTRACE_TOKEN);
    return (false);
  }

  // read the program counter
  numread = myfread( &pc, sizeof(la_t), 1, m_fp );
  if (numread != 1) {
    ERROR_OUT("tracefile: readStep: error reading (%d)\n", numread);
    return (false);
  }
  xaction->setPC( pc );
  
  // read the instruction
  numread = myfread( &instr, sizeof(uint32), 1, m_fp );
  if (numread != 1) {
    ERROR_OUT("tracefile: readStep: error reading (%d)\n", numread);
    return (false);
  }
  xaction->setInst( instr );

  // read the local time
  numread = myfread( &localtime, sizeof(uint32), 1, m_fp );
  if (numread != 1) {
    ERROR_OUT("tracefile: readStep: error reading (%d)\n", numread);
    return (false);
  }
  xaction->setTime( localtime );

  // read the physical address to the file
  numread = myfread( &physical_address, sizeof(pa_t), 1, m_fp );
  if (numread != 1) {
    ERROR_OUT("read fails: Memory Operation\n");
    return (false);
  }
  xaction->setPhysical( physical_address );

  // read the logical address to the file
  numread = myfread( &logical_address, sizeof(la_t), 1, m_fp );
  if (numread != 1) {
    ERROR_OUT("read fails: Memory Operation\n");
    return (false);
  }
  xaction->setVirtual( logical_address );

  // read the size
  numread = myfread( &mysize, sizeof(uint16), 1, m_fp );
  if (numread != 1) {
    ERROR_OUT("read fails: Memory Operation\n");
    return (false);
  }
  xaction->setSize( mysize );

  // read load or store
  numread = myfread( &load_store, sizeof(uint16), 1, m_fp );
  if (numread != 1) {
    ERROR_OUT("read fails: Memory Operation\n");
    return (false);
  }
  xaction->setAction( load_store );

  // read data or instruction
  numread = myfread( &data_instr, sizeof(uint16), 1, m_fp );
  if (numread != 1) {
    ERROR_OUT("read fails: Memory Operation\n");
    return (false);
  }
  xaction->setIorD( data_instr );

  // read user or super
  numread = myfread( &user_super, sizeof(uint16), 1, m_fp );
  if (numread != 1) {
    ERROR_OUT("read fails: Memory Operation\n");
    return (false);
  }
  xaction->setSuper( user_super );

  // read atomic information
  numread = myfread( &atomic_flags, sizeof(uint16), 1, m_fp );
  if (numread != 1) {
    ERROR_OUT("read fails: Memory Operation\n");
    return (false);
 }
  xaction->setAtomic( atomic_flags );

  // read the data
  integer_t *data = NULL;
  if (mysize > 0) {
    uint16 num_words = xaction->getNumWords();
    data = (integer_t *) malloc( sizeof(integer_t)*num_words );
    integer_t  data_in;
    for (int i = 0; i < num_words; i++) {
      numread = myfread( &data_in, sizeof(integer_t), 1, m_fp );
      if (numread != 1) {
        ERROR_OUT("read fails: Memory Operation\n");
        return (false);
      }
      data[i] = data_in;
    }
  }
  xaction->setData( data );

  return (true);
}

/*------------------------------------------------------------------------*/
/* Accessor(s) / mutator(s)                                               */
/*------------------------------------------------------------------------*/

/** scan the next N transactions in a file for a particular transaction.
 *  @param localtime The localized time of the transactoin (on this cpu)
 *  @param addr     The virtual address to load/store
 *  @param curPC    The current program counter
 *  @param instr    The instruction trying to load/store
 *  @param physical_addr The translated address
 *  @param data     A pointer to the data
 *  @return bool    True if a match is found, false if not
 */
bool  memtrace_t::scanTransaction( uint32 localtime, la_t address, 
                                   la_t curPC, uint32 instr, 
                                   pa_t &physical_addr, uint16 size,
                                   ireg_t *data )
{
  transaction_t *xaction;
  int  distance;
  int  best_dist = 1000;
  int  best_point = -1;

#ifdef MEMTRACE_DEBUG
  integer_t *tmp_data;
  DEBUG_OUT("scanning for load ... time %u PC 0x%0llx address 0x%0llx\n",
         localtime, curPC, address);
#endif
  for (int i = 0; i < MEMTRACE_WINSIZE; i++) {
    
    xaction = m_window[i];
#ifdef MEMTRACE_DEBUG
    DEBUG_OUT("time: %d VA 0x%0llx", 
           xaction->getTime(), xaction->getVirtual());
    DEBUG_OUT(" PC 0x%0llx inst 0x%0x ",
           xaction->getPC(), xaction->getInst());
    tmp_data = xaction->getData();
    if (tmp_data != NULL) {
      DEBUG_OUT("  data = 0x%0llx", *tmp_data);
    } else {
      DEBUG_OUT("  NULL data");
    }
#endif
    if (xaction == NULL)
      continue;

    if ( xaction->getSize() == size &&
         xaction->getVirtual() == address ) {
      
#ifdef MEMTRACE_DEBUG
      DEBUG_OUT(" !!M!!");
#endif
      distance = xaction->getTime() - localtime;
      if (distance < 0)
        distance = -distance;

      if (distance < best_dist) {
        best_dist = distance;
        best_point = i;
      }
    }
#ifdef MEMTRACE_DEBUG
    DEBUG_OUT("\n");
#endif
  }

  if (best_point != -1) {
    xaction = m_window[best_point];
    if ( data != NULL ) {
      memcpy( data, xaction->getData(), (xaction->getNumWords() *
                                         sizeof(integer_t)) );
    }
    physical_addr = xaction->getPhysical();

#ifdef MEMTRACE_DEBUG
    if (xaction->getIorD() == TRANS_DATA) {
      DEBUG_OUT("D ");
    } else {
      DEBUG_OUT("I ");
    }
    
    if (xaction->getAction() == TRANS_STORE) {
      DEBUG_OUT("STORE");
    } else {
      DEBUG_OUT("LOAD");
    }
    DEBUG_OUT(" %d\n", xaction->getSize());
    DEBUG_OUT("FOUND\n");
#endif
  }

  return (best_point != -1);
}

/** advance the window up to a particular time */
void  memtrace_t::advanceToTime( int localtime )
{
  transaction_t *xaction;
  bool success  = true;
  int  trailing = localtime -25;

  xaction = m_window[0];
  while ( xaction && success && 
          ((int) xaction->getTime() < trailing) ) {
    success = advanceTransaction();
    xaction = m_window[0];
  }
}

/** advance the transaction window to the next transaction */
bool  memtrace_t::advanceTransaction( void )
{
  transaction_t *tofree = m_window[0];

  for (int i = 0; i < MEMTRACE_WINSIZE - 1; i++) {
    m_window[i] = m_window[i + 1];
  }
  transaction_t *xaction;
  xaction = new transaction_t();
  bool success = readTransaction( xaction );
  if (success) {
    m_window[MEMTRACE_WINSIZE - 1] = xaction;
  } else {
    m_window[MEMTRACE_WINSIZE - 1] = NULL;
  }
  
  if (tofree != NULL)
    delete tofree;
  return (success);
}

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

