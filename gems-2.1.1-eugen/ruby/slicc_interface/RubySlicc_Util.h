
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

#ifndef SLICC_UTIL_H
#define SLICC_UTIL_H

#include "Global.h"
#include "Address.h"
#include "NodeID.h"
#include "MachineID.h"
#include "RubyConfig.h"
#include "CacheMsg.h"
#include "GenericRequestType.h"
#include "CacheRequestType.h"
#include "AccessType.h"
#include "MachineType.h"
#include "Directory_State.h"
#include "L1Cache_State.h"
#include "MessageSizeType.h"
#include "Network.h"
#include "PrefetchBit.h"

#include "RubySlicc_ComponentMapping.h"

class Set;
class NetDest;

extern inline int random(int n)
{
  return random() % n;
}

extern inline bool multicast_retry()
{
  if (RANDOMIZATION) {
    return (random() & 0x1);
  } else {
    return true;
  }
}

extern inline int cache_state_to_int(L1Cache_State state)
{
  return state;
}

extern inline Time get_time()
{
  return g_eventQueue_ptr->getTime();
}

extern inline Time zero_time() 
{
  return 0;
}

extern inline NodeID intToID(int nodenum)
{
  NodeID id = nodenum;
  return id;
}

extern inline int IDToInt(NodeID id)
{
  int nodenum = id;
  return nodenum;
}

extern inline int addressToInt(Address addr)
{
  return (int) addr.getLineAddress();
}

extern inline int MessageSizeTypeToInt(MessageSizeType size_type)
{
  return MessageSizeType_to_int(size_type);
}

extern inline int numberOfNodes()
{
  return RubyConfig::numberOfChips();
}

extern inline int numberOfL1CachePerChip()
{
  return RubyConfig::numberOfL1CachePerChip();
}

extern inline bool long_enough_ago(Time event)
{
  return ((get_time() - event) > 200);
}

extern inline int getAddThenMod(int addend1, int addend2, int modulus)
{
  return (addend1 + addend2) % modulus;
}

extern inline Time getTimeModInt(Time time, int modulus)
{
  return time % modulus;
}

extern inline Time getTimePlusInt(Time addend1, int addend2)
{
  return (Time) addend1 + addend2;
}

extern inline Time getTimeMinusTime(Time t1, Time t2)
{
  ASSERT(t1 >= t2);
  return t1 - t2;
}

extern inline Time getPreviousDelayedCycles(Time t1, Time t2)
{
  if (RANDOMIZATION) {  // when randomizing delayed
    return 0;
  } else {
    return getTimeMinusTime(t1, t2);
  }
}

extern inline void WARN_ERROR_TIME(Time time)
{
  WARN_EXPR(time);
}

// Return type for time_to_int is "Time" and not "int" so we get a 64-bit integer
extern inline Time time_to_int(Time time) 
{
  return time;
}


extern inline bool getFilteringEnabled()
{
  return g_FILTERING_ENABLED;
}

extern inline int getRetryThreshold()
{
  return g_RETRY_THRESHOLD;
}

extern inline int getFixedTimeoutLatency()
{
  return g_FIXED_TIMEOUT_LATENCY;
}

extern inline int N_tokens()
{
  // return N+1 to handle clean writeback
  return g_PROCS_PER_CHIP + 1;
  // return 1;
}

extern inline bool distributedPersistentEnabled()
{
  return g_DISTRIBUTED_PERSISTENT_ENABLED;
}

extern inline bool getDynamicTimeoutEnabled()
{
  return g_DYNAMIC_TIMEOUT_ENABLED;
}

// Appends an offset to an address
extern inline Address setOffset(Address addr, int offset)
{
  Address result = addr;
  result.setOffset(offset);
  return result;
}

// Makes an address into a line address
extern inline Address makeLineAddress(Address addr)
{
  Address result = addr;
  result.makeLineAddress();
  return result;
}

#endif //SLICC_UTIL_H
