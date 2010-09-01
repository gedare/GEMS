
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
 * */

#ifndef GLOBAL_H
#define GLOBAL_H

#ifdef SINGLE_LEVEL_CACHE
const bool TWO_LEVEL_CACHE = false;
#define L1I_CACHE_MEMBER_VARIABLE m_L1Cache_cacheMemory_vec[m_version] // currently all protocols require L1s == nodes
#define L1D_CACHE_MEMBER_VARIABLE m_L1Cache_cacheMemory_vec[m_version] // "
#define L2_CACHE_MEMBER_VARIABLE m_L1Cache_cacheMemory_vec[m_version] // "
#define L2_CACHE_VARIABLE m_L1Cache_cacheMemory_vec
#else
const bool TWO_LEVEL_CACHE = true;
#ifdef IS_CMP
#define L1I_CACHE_MEMBER_VARIABLE m_L1Cache_L1IcacheMemory_vec[m_version] 
#define L1D_CACHE_MEMBER_VARIABLE m_L1Cache_L1DcacheMemory_vec[m_version] 
#define L2_CACHE_MEMBER_VARIABLE m_L2Cache_L2cacheMemory_vec[m_version] 
#define L2_CACHE_VARIABLE m_L2Cache_L2cacheMemory_vec
#else  // not IS_CMP
#define L1I_CACHE_MEMBER_VARIABLE m_L1Cache_L1IcacheMemory_vec[m_version] // currently all protocols require L1s == nodes
#define L1D_CACHE_MEMBER_VARIABLE m_L1Cache_L1DcacheMemory_vec[m_version] // "
// #define L2_CACHE_MEMBER_VARIABLE m_L1Cache_L2cacheMemory_vec[m_version] // old exclusive caches don't support L2s != nodes
#define L2_CACHE_MEMBER_VARIABLE m_L1Cache_cacheMemory_vec[m_version] // old exclusive caches don't support L2s != nodes
#define L2_CACHE_VARIABLE m_L1Cache_L2cacheMemory_vec
#endif // IS_CMP
#endif //SINGLE_LEVEL_CACHE

#define DIRECTORY_MEMBER_VARIABLE m_Directory_directory_vec[m_version]
#define TBE_TABLE_MEMBER_VARIABLE m_L1Cache_TBEs_vec[m_version]

typedef unsigned char uint8;
typedef unsigned int uint32;
typedef unsigned long long uint64;

typedef signed char int8;
typedef int int32;
typedef long long int64;

typedef long long integer_t;
typedef unsigned long long uinteger_t;

typedef int64 Time;
typedef uint64 physical_address_t;
typedef uint64 la_t;
typedef uint64 pa_t;
typedef integer_t simtime_t;

// external includes for all classes
#include "std-includes.h"
#include "Debug.h"

// simple type declarations
typedef Time LogicalTime;
typedef int64 Index;            // what the address bit ripper returns
typedef int word;               // one word of a cache line
typedef unsigned int uint;
typedef int SwitchID;
typedef int LinkID;

class EventQueue;
extern EventQueue* g_eventQueue_ptr;

class System;
extern System* g_system_ptr;

class Debug;
extern Debug* g_debug_ptr;

// FIXME:  this is required by the contructor of Directory_Entry.h.  It can't go 
// into slicc_util.h because it opens a can of ugly worms
extern inline int max_tokens()
{
  return 1024;
}


#endif //GLOBAL_H

