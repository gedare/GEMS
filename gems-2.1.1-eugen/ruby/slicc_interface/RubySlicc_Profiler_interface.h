
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
 * slicc_util.h
 * 
 * Description: These are the functions that exported to slicc from ruby.
 *
 * $Id$
 *
 */

#ifndef RUBYSLICC_PROFILER_INTERFACE_H
#define RUBYSLICC_PROFILER_INTERFACE_H

#include "Global.h"
#include "Profiler.h"
#include "Address.h"
#include "L1Cache_State.h"
#include "AccessType.h"
#include "GenericRequestType.h"
#include "Directory_State.h"
#include "NodeID.h"

class Set;

void profile_request(int cache_state, Directory_State directory_state, GenericRequestType request_type);
void profile_outstanding_persistent_request(int outstanding);
void profile_outstanding_request(int outstanding);
void profile_sharing(const Address& addr, AccessType type, NodeID requestor, const Set& sharers, const Set& owner);
void profile_request(const string& L1CacheStateStr, const string& L2CacheStateStr, const string& directoryStateStr, const string& requestTypeStr);
void profile_miss(const CacheMsg& msg, NodeID id);
void profile_L1Cache_miss(const CacheMsg& msg, NodeID id);
void profile_L2Cache_miss(GenericRequestType requestType, AccessModeType type, int msgSize, PrefetchBit pfBit, NodeID l2cacheID);
void profile_token_retry(const Address& addr, AccessType type, int count);
void profile_filter_action(int action);
void profile_persistent_prediction(const Address& addr, AccessType type);
void profile_average_latency_estimate(int latency);
void profileMsgDelay(int virtualNetwork, int delayCycles);

void profile_multicast_retry(const Address& addr, int count);
void profileGetX(const Address& datablock, const Address& PC, const Set& owner, const Set& sharers, NodeID requestor);
void profileGetS(const Address& datablock, const Address& PC, const Set& owner, const Set& sharers, NodeID requestor);

void profileOverflow(const Address & addr, MachineID mach);

#endif // RUBYSLICC_PROFILER_INTERFACE_H
