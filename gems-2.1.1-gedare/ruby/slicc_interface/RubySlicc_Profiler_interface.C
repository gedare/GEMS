
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
 * slicc_util.C
 * 
 * Description: See slicc_util.h
 *
 * $Id$
 *
 */

#include "Global.h"
#include "System.h"
#include "Profiler.h"
#include "AddressProfiler.h"
#include "Protocol.h"
#include "RubySlicc_Profiler_interface.h"
#include "RubySlicc_ComponentMapping.h"
#include "TransactionInterfaceManager.h"

void profile_request(int cache_state, Directory_State directory_state, GenericRequestType request_type)
{
  string requestStr = L1Cache_State_to_string(L1Cache_State(cache_state))+":"+
    Directory_State_to_string(directory_state)+":"+
    GenericRequestType_to_string(request_type);
  g_system_ptr->getProfiler()->profileRequest(requestStr);
}

void profile_request(const string& L1CacheState, const string& L2CacheState, const string& directoryState, const string& requestType)
{
  string requestStr = L1CacheState+":"+L2CacheState+":"+directoryState+":"+requestType;
  g_system_ptr->getProfiler()->profileRequest(requestStr);
}

void profile_outstanding_request(int outstanding)
{
  g_system_ptr->getProfiler()->profileOutstandingRequest(outstanding);
}

void profile_outstanding_persistent_request(int outstanding)
{
  g_system_ptr->getProfiler()->profileOutstandingPersistentRequest(outstanding);
}

void profile_average_latency_estimate(int latency)
{
  g_system_ptr->getProfiler()->profileAverageLatencyEstimate(latency);
}

void profile_sharing(const Address& addr, AccessType type, NodeID requestor, const Set& sharers, const Set& owner)
{
  g_system_ptr->getProfiler()->profileSharing(addr, type, requestor, sharers, owner);
}

void profile_miss(const CacheMsg& msg, NodeID id) 
{
  // CMP profile address after L1 misses, not L2
  ASSERT (!Protocol::m_CMP);
  g_system_ptr->getProfiler()->addAddressTraceSample(msg, id);

  g_system_ptr->getProfiler()->profileConflictingRequests(msg.getAddress());

  g_system_ptr->getProfiler()->addSecondaryStatSample(msg.getType(),
                                                      msg.getAccessMode(), msg.getSize(), msg.getPrefetch(), id);
}

void profile_L1Cache_miss(const CacheMsg& msg, NodeID id)
{
  // only called by protocols assuming non-zero cycle hits
  ASSERT (REMOVE_SINGLE_CYCLE_DCACHE_FAST_PATH);
  
  g_system_ptr->getProfiler()->addPrimaryStatSample(msg, id);
}

void profileMsgDelay(int virtualNetwork, int delayCycles)
{
  g_system_ptr->getProfiler()->profileMsgDelay(virtualNetwork, delayCycles);
}

void profile_L2Cache_miss(GenericRequestType requestType, AccessModeType type, int msgSize, PrefetchBit pfBit, NodeID nodeID)
{
  g_system_ptr->getProfiler()->addSecondaryStatSample(requestType, type, msgSize, pfBit, nodeID);
}

void profile_token_retry(const Address& addr, AccessType type, int count)
{
  g_system_ptr->getProfiler()->getAddressProfiler()->profileRetry(addr, type, count);
}

void profile_filter_action(int action)
{
  g_system_ptr->getProfiler()->profileFilterAction(action);
}

void profile_persistent_prediction(const Address& addr, AccessType type)
{
  g_system_ptr->getProfiler()->getAddressProfiler()->profilePersistentPrediction(addr, type);
}

void profile_multicast_retry(const Address& addr, int count)
{
  g_system_ptr->getProfiler()->profileMulticastRetry(addr, count);
}

void profileGetX(const Address& datablock, const Address& PC, const Set& owner, const Set& sharers, NodeID requestor)
{
  g_system_ptr->getProfiler()->getAddressProfiler()->profileGetX(datablock, PC, owner, sharers, requestor);
}

void profileGetS(const Address& datablock, const Address& PC, const Set& owner, const Set& sharers, NodeID requestor)
{
  g_system_ptr->getProfiler()->getAddressProfiler()->profileGetS(datablock, PC, owner, sharers, requestor);
}

void profileOverflow(const Address & addr, MachineID mach)
{
  if(mach.type == MACHINETYPE_L1CACHE_ENUM){
    // for L1 overflows
    int proc_num = L1CacheMachIDToProcessorNum(mach);
    int chip_num = proc_num/RubyConfig::numberOfProcsPerChip();
    g_system_ptr->getChip(chip_num)->m_L1Cache_xact_mgr_vec[proc_num]->profileOverflow(addr, true);
  }
  else if(mach.type == MACHINETYPE_L2CACHE_ENUM){
    // for L2 overflows
    int chip_num = L2CacheMachIDToChipID(mach);
    for(int p=0; p < RubyConfig::numberOfProcessors(); ++p){
      g_system_ptr->getChip(chip_num)->m_L1Cache_xact_mgr_vec[p]->profileOverflow(addr, false);
    }
  }
}


