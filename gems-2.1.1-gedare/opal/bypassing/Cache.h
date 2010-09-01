
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
#ifndef CS752_CACHE_H
#define CS752_CACHE_H

#include <iostream.h>
#include <sys/types.h>
#include "Types.h"
#include "CacheEntry.h"

//  #define CACHE_SIZE 0x800000
//  #define CACHE_LINE_SIZE 0x40
//  #define CACHE_ASSOCIATIVITY 1
//  #define CACHE_NUM_LINES CACHE_SIZE/(CACHE_LINE_SIZE*CACHE_ASSOCIATIVITY)

class Cache 
{
public:
    Cache(uint64_t cacheSize, int cacheAss, int lineSize);

    // address accessors
    Address getTag(Address a);
    Address getIndex(Address a);
    Address getOffset(Address a);
    int getNumLines( void );

    bool isHit(Address a, CacheEntry * &matchedEntry);

    void updateLRUStatus(Address mostRecentAddress);
    void load(Address newPhysicalAddress, 
              Address newLoadingPC, 
              CacheEntry &victimEntry);
    void load(Address newPhysicalAddress, 
              Address newLoadingPC, 
              CacheEntry &victimEntry,
              int     ReadOrWrite,
              bool    isData);
    void dump(void);

protected:
    void updateLRUStatus(Address index, int entryOffset);
    int findLRUIndex( Address index );
    int getLog2(uint64_t num);
    uint64_t getMask(uint64_t numBits);
    
    CacheEntry **lines;
    
    uint64_t m_tagBits;
    uint64_t m_indexBits;
    uint64_t m_offsetBits;
    
    uint64_t m_tagMask;
    uint64_t m_indexMask;
    uint64_t m_offsetMask;

    int m_cacheAss;
    int m_lineSize;
    int m_cacheSize;
    int m_numLines;

};

#endif
