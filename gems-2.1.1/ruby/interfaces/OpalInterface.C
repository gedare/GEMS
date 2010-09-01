
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
 */

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include "OpalInterface.h"
#include "interface.h"
#include "System.h"
#include "SubBlock.h"
#include "mf_api.h"
#include "Chip.h"
#include "RubyConfig.h"
#include "XactIsolationChecker.h"
#include "TransactionInterfaceManager.h"
#include "TransactionVersionManager.h"
#include "Sequencer.h"

/*------------------------------------------------------------------------*/
/* Forward declarations                                                   */
/*------------------------------------------------------------------------*/

static CacheRequestType get_request_type( OpalMemop_t opaltype );
static OpalMemop_t get_opal_request_type( CacheRequestType type );

/// The static opalinterface instance
OpalInterface *OpalInterface::inst = NULL;

/*------------------------------------------------------------------------*/
/* Constructor(s) / destructor                                            */
/*------------------------------------------------------------------------*/

//**************************************************************************
OpalInterface::OpalInterface(System* sys_ptr) {
  clearStats();
  ASSERT( inst == NULL );
  inst = this;
  m_opal_intf = NULL;
}

/*------------------------------------------------------------------------*/
/* Public methods                                                         */
/*------------------------------------------------------------------------*/

//**************************************************************************
void OpalInterface::printConfig(ostream& out) const {
  out << "Opal_ruby_multiplier: " << OPAL_RUBY_MULTIPLIER << endl;
  out << endl;
}

void OpalInterface::printStats(ostream& out) const {
  out << endl;
  out << "Opal Interface Stats" << endl;
  out << "----------------------" << endl;
  out << endl;
}

//**************************************************************************
void OpalInterface::clearStats() {
}

//**************************************************************************
integer_t OpalInterface::getInstructionCount(int procID) const {
  return ((*m_opal_intf->getInstructionCount)(procID));
}

//*************************************************************************
uint64 OpalInterface::getOpalTime(int procID) const {
  return ((*m_opal_intf->getOpalTime)(procID));
}

/************  For WATTCH power stats ************************************/
//*************************************************************************
void OpalInterface::incrementL2Access(int procID) const{
  ((*m_opal_intf->incrementL2Access)(procID));
}

//*************************************************************************
void OpalInterface::incrementPrefetcherAccess(int procID, int num_prefetches, int isinstr) const{
  ((*m_opal_intf->incrementPrefetcherAccess)(procID, num_prefetches, isinstr));
}
/******************** END WATTCH power stats ****************************/

// Notifies Opal of an L2 miss
//*************************************************************************
void OpalInterface::notifyL2Miss(int procID, physical_address_t physicalAddr, OpalMemop_t type, int tagexists) const{
  ((*m_opal_intf->notifyL2Miss)(procID, physicalAddr, type, tagexists));
}

/******************************************************************
 * void hitCallback(int cpuNumber)
 * Called by Sequencer.  Calls opal.
 ******************************************************************/

//**************************************************************************
void OpalInterface::hitCallback(NodeID proc, SubBlock& data, CacheRequestType type, int thread) {
  // notify opal that the transaction has completed
  (*m_opal_intf->hitCallback)( proc, data.getAddress().getAddress(), get_opal_request_type(type), thread );
}

//**************************************************************************
// Useful functions if Ruby needs to read/write physical memory when running with Opal
integer_t OpalInterface::readPhysicalMemory(int procID, 
                                           physical_address_t address,
                                           int len ){
  return SIMICS_read_physical_memory(procID, address, len);
}

//**************************************************************************
void OpalInterface::writePhysicalMemory( int procID, 
                                        physical_address_t address,
                                        integer_t value, 
                                        int len ){
  SIMICS_write_physical_memory(procID, address, value, len);
}

//***************************************************************
// notifies Opal to print debug info
void OpalInterface::printDebug(){
  (*m_opal_intf->printDebug)();
}

//***************************************************************

/******************************************************************
 * Called by opal's memory operations (memop.C)
 * May call Sequencer.
 ******************************************************************/

//****************************************************************************
int OpalInterface::getNumberOutstanding( int cpuNumber ){
  Sequencer* targetSequencer_ptr = g_system_ptr->getChip(cpuNumber/RubyConfig::numberOfProcsPerChip())->getSequencer(cpuNumber%RubyConfig::numberOfProcsPerChip());
  
  return targetSequencer_ptr->getNumberOutstanding();
}

//****************************************************************************
int OpalInterface::getNumberOutstandingDemand( int cpuNumber ){
  Sequencer* targetSequencer_ptr = g_system_ptr->getChip(cpuNumber/RubyConfig::numberOfProcsPerChip())->getSequencer(cpuNumber%RubyConfig::numberOfProcsPerChip());
  
  return targetSequencer_ptr->getNumberOutstandingDemand();
}

//****************************************************************************
int OpalInterface::getNumberOutstandingPrefetch( int cpuNumber ){
  Sequencer* targetSequencer_ptr = g_system_ptr->getChip(cpuNumber/RubyConfig::numberOfProcsPerChip())->getSequencer(cpuNumber%RubyConfig::numberOfProcsPerChip());
  
  return targetSequencer_ptr->getNumberOutstandingPrefetch();
}

//**************************************************************************
int  OpalInterface::isReady( int cpuNumber, la_t logicalAddr, pa_t physicalAddr, OpalMemop_t typeOfRequest, int thread ) {
  // Send request to sequencer
  Sequencer* targetSequencer_ptr = g_system_ptr->getChip(cpuNumber/RubyConfig::numberOfProcsPerChip())->getSequencer(cpuNumber%RubyConfig::numberOfProcsPerChip());

  // FIXME - some of these fields have bogus values sinced isReady()
  // doesn't need them.  However, it would be cleaner if all of these
  // fields were exact.

  return (targetSequencer_ptr->isReady(CacheMsg(Address( physicalAddr ),
                                                Address( physicalAddr ),
                                                get_request_type(typeOfRequest),
                                                Address(0),
                                                AccessModeType_UserMode,   // User/supervisor mode
                                                0,   // Size in bytes of request
                                                PrefetchBit_No,  // Not a prefetch
                                                0,              // Version number
                                                Address(logicalAddr),   // Virtual Address
                                                thread,              // SMT thread 
                                                0,          // TM specific - timestamp of memory request
                                                false      // TM specific - whether request is part of escape action
                                                ) 
                                       ));
}

// FIXME: duplicated code should be avoided
//**************************************************************************
void  OpalInterface::makeRequest(int  cpuNumber, la_t logicalAddr, pa_t physicalAddr,
                                 int  requestSize, OpalMemop_t typeOfRequest,
                                 la_t virtualPC, int isPriv, int thread) {
  // Issue the request to the sequencer.
  // set access type (user/supervisor)
  AccessModeType access_mode;  
  if (isPriv) {
    access_mode = AccessModeType_SupervisorMode;
  } else {
    access_mode = AccessModeType_UserMode;
  }

  // Send request to sequencer
  Sequencer* targetSequencer_ptr = g_system_ptr->getChip(cpuNumber/RubyConfig::numberOfProcsPerChip())->getSequencer(cpuNumber%RubyConfig::numberOfProcsPerChip());

  targetSequencer_ptr->makeRequest(CacheMsg(Address( physicalAddr ),
                                            Address( physicalAddr ),
                                            get_request_type(typeOfRequest),
                                            Address(virtualPC),
                                            access_mode,   // User/supervisor mode
                                            requestSize,   // Size in bytes of request
                                            PrefetchBit_No, // Not a prefetch
                                            0,              // Version number
                                            Address(logicalAddr),   // Virtual Address
                                            thread,              // SMT thread 
                                            0,          // TM specific - timestamp of memory request
                                            false      // TM specific - whether request is part of escape action
                                            )
                                   );
}


//**************************************************************************
void  OpalInterface::makePrefetch(int  cpuNumber, la_t logicalAddr, pa_t physicalAddr,
                                  int  requestSize, OpalMemop_t typeOfRequest,
                                  la_t virtualPC, int isPriv, int thread) {
  DEBUG_MSG(SEQUENCER_COMP,MedPrio,"Making prefetch request");

  // Issue the request to the sequencer.
  // set access type (user/supervisor)
  AccessModeType access_mode;  
  if (isPriv) {
    access_mode = AccessModeType_SupervisorMode;
  } else {
    access_mode = AccessModeType_UserMode;
  }

  // make the prefetch
  Sequencer* targetSequencer_ptr = g_system_ptr->getChip(cpuNumber/RubyConfig::numberOfProcsPerChip())->getSequencer(cpuNumber%RubyConfig::numberOfProcsPerChip());
  targetSequencer_ptr->makeRequest(CacheMsg(Address( physicalAddr ),
                                            Address( physicalAddr ),
                                            get_request_type(typeOfRequest),
                                            Address(virtualPC),
                                            access_mode, 
                                            requestSize,
                                            PrefetchBit_Yes, // True means this is a prefetch
                                            0,              // Version number
                                            Address(logicalAddr),   // Virtual Address
                                            thread,              // SMT thread 
                                            0,          // TM specific - timestamp of memory request
                                            false      // TM specific - whether request is part of escape action
                                            )
                                   );
  return;
}

//**************************************************************************
int OpalInterface::staleDataRequest( int cpuNumber, pa_t physicalAddr,
                                     int requestSize, int8 *buffer ) {
  // Find sequencer
  Sequencer* targetSequencer_ptr = g_system_ptr->getChip(cpuNumber/RubyConfig::numberOfProcsPerChip())->getSequencer(cpuNumber%RubyConfig::numberOfProcsPerChip());
  assert(targetSequencer_ptr != NULL);
  
  // query the cache for stale data values (if any)
  bool hit = false;
  //hit = targetSequencer_ptr->staleDataAccess( Address(physicalAddr),
  //                                                 requestSize, buffer );
  
  return hit;
}

//**************************************************************************
void  OpalInterface::notify( int status ) {
  if (OpalInterface::inst == NULL) {
    if (status == 1) {
      // notify system that opal is now loaded
      g_system_ptr->opalLoadNotify();
    } else {
      return;
    }
  }
  
  // opal interface must be allocated now
  ASSERT( OpalInterface::inst != NULL );
  if ( status == 0 ) {

  } else if ( status == 1 ) {
    // install notification: query opal for its interface
    OpalInterface::inst->queryOpalInterface();
    if ( OpalInterface::inst->m_opal_intf != NULL ) {
      cout << "OpalInterface: installation successful." << endl;
      // test: (*(OpalInterface::inst->m_opal_intf->hitCallback))( 0, 0xFFULL );
    }
  } else if ( status == 2 ) {
    // unload notification
    // NOTE: this is not tested, as we can't unload ruby or opal right now.
    OpalInterface::inst->removeOpalInterface();
  }
}

// advance ruby time
//**************************************************************************
int OpalInterface::s_advance_counter = 0;

void OpalInterface::advanceTime( void ) {
  s_advance_counter++;
  if (s_advance_counter == OPAL_RUBY_MULTIPLIER) {
    Time time = g_eventQueue_ptr->getTime() + 1;
    DEBUG_EXPR(NODE_COMP, HighPrio, time);
    g_eventQueue_ptr->triggerEvents(time);
    s_advance_counter = 0;
  }
}

// return ruby's time
//**************************************************************************
unsigned long long OpalInterface::getTime( void ) {
  return (g_eventQueue_ptr->getTime());
}

// print's Ruby outstanding request table
void OpalInterface::printProgress(int cpuNumber){
  Sequencer* targetSequencer_ptr = g_system_ptr->getChip(cpuNumber/RubyConfig::numberOfProcsPerChip())->getSequencer(cpuNumber%RubyConfig::numberOfProcsPerChip());
  assert(targetSequencer_ptr != NULL);

  targetSequencer_ptr->printProgress(cout);
}

// Non-method helper function
//**************************************************************************
static CacheRequestType get_request_type( OpalMemop_t opaltype ) {
  CacheRequestType type;

  if (opaltype == OPAL_LOAD) {
    type = CacheRequestType_LD;
  } else if (opaltype == OPAL_STORE){
    type = CacheRequestType_ST;
  } else if (opaltype == OPAL_IFETCH){
    type = CacheRequestType_IFETCH;
  } else if (opaltype == OPAL_ATOMIC){
    type = CacheRequestType_ATOMIC;
  } else {
    ERROR_MSG("Error: Strange memory transaction type: not a LD or a ST");
  }
  return type;
}

//**************************************************************************
static OpalMemop_t get_opal_request_type( CacheRequestType type ) {
  OpalMemop_t opal_type;

  if(type == CacheRequestType_LD){
    opal_type = OPAL_LOAD;
  }
  else if( type == CacheRequestType_ST){
    opal_type = OPAL_STORE;
  }
  else if( type == CacheRequestType_IFETCH){
    opal_type = OPAL_IFETCH;
  }
  else if( type == CacheRequestType_ATOMIC){
    opal_type = OPAL_ATOMIC;
  }
  else{
    ERROR_MSG("Error: Strange memory transaction type: not a LD or a ST");
  }

  //cout << "get_opal_request_type() CacheRequestType[ " << type << " ] opal_type[ " << opal_type << " ] " << endl;
  return opal_type;
}

//**************************************************************************
void OpalInterface::removeOpalInterface( void ) {
  cout << "ruby: opal uninstalled. reinstalling timing model." << endl;
  SIMICS_install_timing_model();
}

//**************************************************************************
bool OpalInterface::isOpalLoaded( void ) {
  if (!g_SIMICS) {
    return false;
  } else {
    mf_opal_api_t *opal_interface = SIMICS_get_opal_interface();
    if ( opal_interface == NULL ) {
      return false;
    } else {
      return true;
    }
  }
}

//**************************************************************************
void OpalInterface::queryOpalInterface( void ) {
  m_opal_intf = SIMICS_get_opal_interface();
  if ( m_opal_intf == NULL ) {
    WARN_MSG("error: OpalInterface: opal does not implement mf-opal-api interface.\n");
  } else {
    // opal is loaded -- remove the timing_model interface
    cout << "Ruby: ruby-opal link established. removing timing_model." << endl;
    SIMICS_remove_timing_model();

    if (m_opal_intf->notifyCallback != NULL) {
      cout << "opalinterface: doing notify callback\n";
      (*m_opal_intf->notifyCallback)( 1 );
    } else {
      // 2/27/2005, removed spurious error message (MRM)
      // cout << "error: opalinterface: mf-opal-api has NULL notify callback.\n";
    }
  }
}

// install the opal interface to simics
//**************************************************************************
void OpalInterface::installInterface( mf_ruby_api_t *api ) {
  // install ruby interface
  api->isReady          = &OpalInterface::isReady;
  api->makeRequest      = &OpalInterface::makeRequest;
  api->makePrefetch     = &OpalInterface::makePrefetch;
  api->advanceTime      = &OpalInterface::advanceTime;
  api->getTime          = &OpalInterface::getTime;
  api->staleDataRequest = &OpalInterface::staleDataRequest;
  api->notifyCallback   = &OpalInterface::notify;
  api->getNumberOutstanding = &OpalInterface::getNumberOutstanding;
  api->getNumberOutstandingDemand = &OpalInterface::getNumberOutstandingDemand;
  api->getNumberOutstandingPrefetch = &OpalInterface::getNumberOutstandingPrefetch;
  api->printProgress = &OpalInterface::printProgress;
}
