
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

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

#ifndef _MF_MEMORY_API_H_
#define _MF_MEMORY_API_H_

#ifdef SIMICS30
#ifndef pa_t
typedef physical_address_t pa_t;
typedef physical_address_t la_t;
#endif
#endif

/**
 *  Defines types of memory requests
 */
typedef enum OpalMemop {
  OPAL_LOAD,
  OPAL_STORE,
  OPAL_IFETCH,
  OPAL_ATOMIC,
} OpalMemop_t;

/*------------------------------------------------------------------------*/
/* Class declaration(s)                                                   */
/*------------------------------------------------------------------------*/

/**
* structure which provides an interface between ruby and opal.
*/
typedef struct mf_opal_api {
  /**
   * @name Methods
   */
  //@{
  /**
   * notify processor model that data from address address is available at proc
   */
  void (*hitCallback)( int cpuNumber, pa_t phys_address, OpalMemop_t type, int thread );
  
  /**
   * notify opal that ruby is loaded, or removed
   */
  void (*notifyCallback)( int status );

  /**
   * query for the number of instructions executed on a given processor.
   */
  integer_t (*getInstructionCount)( int cpuNumber );

  // for printing out debug info on crash
  void (*printDebug)();

  /** query Opal for the current time */
  uint64 (*getOpalTime)(int cpuNumber);

  /** For WATTCH power stats */
  // Called whenever L2 is accessed
  void (*incrementL2Access)(int cpuNumber);
  // Called whenever prefetcher is accessed
  void (*incrementPrefetcherAccess)(int cpuNumber, int num_prefetches, int isinstr);

  /* Called whenever there's an L2 miss */
  void (*notifyL2Miss)(int cpuNumber, physical_address_t physicalAddr, OpalMemop_t type, int tagexists);

  //@}
} mf_opal_api_t;

typedef struct mf_ruby_api {
  /**
   * @name Methods
   */
  //@{
  /**
   * Check to see if the system is ready for more requests
   */
  int  (*isReady)( int cpuNumber, la_t logicalAddr, pa_t physicalAddr, OpalMemop_t typeOfRequest, int thread );

  /**
   * Make a 'mandatory' request to the memory hierarchy
   */
  void (*makeRequest)( int cpuNumber, la_t logicalAddr, pa_t physicalAddr,
                       int requestSize, OpalMemop_t typeOfRequest,
                       la_t virtualPC, int isPriv, int thread);

  /**
   * Make a prefetch request to the memory hierarchy
   */
  void (*makePrefetch)( int cpuNumber, la_t logicalAddr, pa_t physicalAddr,
                        int requestSize, OpalMemop_t typeOfRequest,
                        la_t virtualPC, int isPriv, int thread);

  /**
   * Ask the memory hierarchy for 'stale' data that can be used for speculation
   * Returns true (1) if the tag matches, false (0) if not.
   */
  int (*staleDataRequest)( int cpuNumber, pa_t physicalAddr,
                           int requestSize, int8 *buffer );
  
  /**
   * Advance ruby's cycle time one step
   */
  void (*advanceTime)( void );

  /**
   * Get ruby's cycle time count.
   */
  uint64 (*getTime)( void );

  /** prints Ruby's outstanding request table */
  void (*printProgress)(int cpuNumber);

  /**
   * notify ruby that opal is loaded, or removed
   */
  void (*notifyCallback)( int status );

  // Returns the number of outstanding request
  int (*getNumberOutstanding)(int cpuNumber);

  // Returns the number of outstanding demand requests
  int (*getNumberOutstandingDemand)(int cpuNumber );

  // Returns the number of outstanding prefetch request
  int (*getNumberOutstandingPrefetch)(int cpuNumber );


  //@}
} mf_ruby_api_t;

#endif //_MF_MEMORY_API_H_
