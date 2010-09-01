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
#ifndef _DEBUGIO_H_
#define _DEBUGIO_H_

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Class declaration(s)                                                   */
/*------------------------------------------------------------------------*/

/**
* The out_intf_t class provides a virtual interface for logging
* messages in to separate informational (printed to screen and log) and
* log (printed only to log) messages. This interface allows classes
* associated with a particular sequencer to print their messages
* automatically prefixed by the ID of that sequencer. For instance,
* the cache (for seq #2) message may be: "[2] Total # of reads   1,500".
*
* @see     pseq_t
* @author  cmauer
* @version $Id$
*/

class out_intf_t {
public:
  /// Constructor
  out_intf_t( int id ) {
    m_id    = id;
    m_starting_cycle = 0ULL;
    // m_starting_cycle = 5000000000ULL;
    m_logfp = NULL;
  };

  /// Destructor  
  ~out_intf_t( void ) {};
  
  /** out_info prints out to stdout.
   */
  void   out_info( const char *fmt, ...);

  /** out_info with no ID output
   */
  void   out_info_noid( const char *fmt, ...);

  /** out_log prints out to only the log file.
   */
  void   out_log( const char *fmt, ...);

  /// returns true if this message should _not_ be filtered
  bool   validDebug( void );

  /// sets the debugging time for this debugio filter.
  void   setDebugTime(uint64 cycle);

  /// gets the debugging time for this debugio filter.
  uint64 getDebugTime( void ) {
    return m_starting_cycle;
  }

  /** returns a pointer to this sequencers log file */
  FILE   *getLog( void ) {
    return m_logfp;
  }
  
  /** sets the log file pointer for this object. */
  void    setLog( FILE *fp ) {
    m_logfp = fp;
  }
  
private:
  /// identifier for this output logger
  int             m_id;

  /// The log file for this particular object
  FILE           *m_logfp;

  /// determines starting cycle time to start filtering messages
  uint64          m_starting_cycle;
};

/**
* The DebugIO class formats, filters debugging log file messages.
*
* @author  cmauer
* @version $Id$
*/
class debugio_t {
public:
  /**
   * @name Constructor(s) / destructor
   */
  //@{
  /// Constructors
  debugio_t();

  /// Destructor
  ~debugio_t();
  //@}
  
  /**
   * @name public methods
   */
  //@{
  /** out_info prints out to stdout and a file at the same time.
   */
  static void out_info( const char *fmt, ...);

  /** out_log prints out only to the log file.
   */
  static void out_log( const char *fmt, ...);

  /** out_error prints out to standard error */
  static void out_error( const char *fmt, ...);

  /** returns the system wide out_info log file */
  static FILE *getLog( void ) {
    return m_logfp;
  }

  /** opens the system wide out_info log file */
  static void openLog( const char *logFileName );

  /** closes the system wide out_info log file */
  static void closeLog( void );

  /** sets the debugging time for the global filter */
  static void setDebugTime(uint64 cycle) {
    m_global_cycle = cycle;
  }

  /// gets the debugging time for this debugio filter.
  uint64 getDebugTime( void ) {
    return m_global_cycle;
  }

  //@}

private:
  // Private Methods

  /// Private copy constructor
  debugio_t(const debugio_t& obj);
  /// Private assignment operator
  debugio_t& operator=(const debugio_t& obj);
  
  /// determines the starting cycle for filtering global messages
  static uint64 m_global_cycle;

  /// The system wide (not processor specific) log file.
  // static std::fstream m_logfp;
  static FILE *m_logfp;
};

/*------------------------------------------------------------------------*/
/* Global variables                                                       */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Global functions                                                       */
/*------------------------------------------------------------------------*/

/// global macro for printing out informational messages
#define  DEBUG_OUT \
     debugio_t::out_info

/// global macro for printing out log only (doesn't appear on screen)
#define  DEBUG_LOG \
     debugio_t::out_log

/// global macro for printing out error messages (doesn't halt program)
#define  ERROR_OUT \
     debugio_t::out_error

#endif  /* _DEBUGIO_H_ */
