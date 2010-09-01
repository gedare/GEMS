
/*
    Copyright (C) 1999-2008 by Mark D. Hill and David A. Wood for the
    Wisconsin Multifacet Project.  Contact: gems@cs.wisc.edu
    http://www.cs.wisc.edu/gems/

    --------------------------------------------------------------------

    This file is part of the Ruby Multiprocessor Memory System Simulator, 
    a component of the Multifacet GEMS (General Execution-driven 
    Multiprocessor Simulator) software toolset originally developed at 
    the University of Wisconsin-Madison.

    Ruby was originally developed primarily by Milo Martin and Daniel
    Sorin with contributions from Ross Dickson, Carl Mauer, and Manoj
    Plakal.

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
 * $Id$
 *
 * Description: 
 *
 */

#ifndef OpalInterface_H
#define OpalInterface_H

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include "Global.h"
#include "Driver.h"
#include "mf_api.h"
#include "CacheRequestType.h"

/*------------------------------------------------------------------------*/
/* Class declaration(s)                                                   */
/*------------------------------------------------------------------------*/

class System;
class TransactionInterfaceManager;
class Sequencer;

/**
 * the processor model (opal) calls these OpalInterface APIs to access 
 * the memory hierarchy (ruby).
 * @see     pseq_t
 * @author  cmauer
 * @version $Id$
 */
class OpalInterface : public Driver {
public:
  // Constructors
  OpalInterface(System* sys_ptr);

  // Destructor
  // ~OpalInterface();

  integer_t getInstructionCount(int procID) const;
  void hitCallback( NodeID proc, SubBlock& data, CacheRequestType type, int thread );
  void printStats(ostream& out) const;
  void clearStats();
  void printConfig(ostream& out) const;
  void print(ostream& out) const;

  integer_t readPhysicalMemory(int procID, physical_address_t address,
                               int len );
  
  void writePhysicalMemory( int procID, physical_address_t address,
                            integer_t value, int len );
  uint64 getOpalTime(int procID) const;

  // for WATTCH power
  void incrementL2Access(int procID) const;
  void incrementPrefetcherAccess(int procID, int num_prefetches, int isinstr) const;

  // notifies Opal of an L2 miss
  void notifyL2Miss(int procID, physical_address_t physicalAddr, OpalMemop_t type, int tagexists) const;

  void printDebug();

  /// The static opalinterface instance
  static OpalInterface *inst;

  /// static methods
  static int getNumberOutstanding(int cpuNumber);
  static int getNumberOutstandingDemand(int cpuNumber);
  static int getNumberOutstandingPrefetch( int cpuNumber );

  /* returns true if the sequencer is able to handle more requests.
     This implements "back-pressure" by which the processor knows 
     not to issue more requests if the network or cache's limits are reached.
   */
  static int  isReady( int cpuNumber, la_t logicalAddr, pa_t physicalAddr, OpalMemop_t typeOfRequest, int thread );

  /*
  makeRequest performs the coherence transactions necessary to get the
  physical address in the cache with the correct permissions. More than
  one request can be outstanding to ruby, but only one per block address.
  The size of the cache line is defined to Intf_CacheLineSize.
  When a request is finished (e.g. the cache contains physical address),
  ruby calls completedRequest(). No request can be bigger than
  Opal_CacheLineSize. It is illegal to request non-aligned memory
  locations. A request of size 2 must be at an even byte, a size 4 must
  be at a byte address half-word aligned, etc. Requests also can't cross a
  cache-line boundaries.
  */
  static void makeRequest(int cpuNumber, la_t logicalAddr, pa_t physicalAddr,
                          int requestSize, OpalMemop_t typeOfRequest,
                          la_t virtualPC, int isPriv, int thread);
  
  /* prefetch a given block...
   */
  static void makePrefetch(int cpuNumber, la_t logicalAddr, pa_t physicalAddr,
                           int requestSize, OpalMemop_t typeOfRequest,
                           la_t virtualPC, int isPriv, int thread);

  /*
   * request data from the cache, even if it's state is "Invalid".
   */
  static int staleDataRequest( int cpuNumber, pa_t physicalAddr,
                               int requestSize, int8 *buffer );

  /* notify ruby of opal's status
   */
  static void notify( int status );
  
  /*
   * advance ruby one cycle
   */
  static void advanceTime( void );

  /*
   * return ruby's cycle count.
   */
  static unsigned long long getTime( void );

  /* prints Ruby's outstanding request table */
  static void printProgress(int cpuNumber);

  /*
   * initialize / install the inter-module interface
   */
  static void installInterface( mf_ruby_api_t *api );

  /*
   * Test if opal is loaded or not
   */
  static bool isOpalLoaded( void );
  
  /*
   * query opal for its api
   */
  void queryOpalInterface( void );

  /*
   * remove the opal interface (opal is unloaded).
   */
  void removeOpalInterface( void );

  /*
   * set the opal interface (used if stand-alone testing)
   */
  void setOpalInterface( mf_opal_api_t *opal_intf ) {
    m_opal_intf = opal_intf;
  }

  /**
   * Signal an abort
   */
  //void abortCallback(NodeID proc);

private:
  // Private Methods

  // Private copy constructor and assignment operator
  OpalInterface(const OpalInterface& obj);
  OpalInterface& operator=(const OpalInterface& obj);
  
  // Data Members (m_ prefix)
  mf_opal_api_t  *m_opal_intf;
  Time            m_simicsStartTime;

  static int      s_advance_counter;
};

// Output operator declaration
ostream& operator<<(ostream& out, const OpalInterface& obj);

// ******************* Definitions *******************

// Output operator definition
extern inline 
ostream& operator<<(ostream& out, const OpalInterface& obj)
{
//  obj.print(out);
  out << flush;
  return out;
}

#endif // OpalInterface_H
