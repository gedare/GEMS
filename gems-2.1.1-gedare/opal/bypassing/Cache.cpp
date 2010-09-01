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

#include "Cache.h"
#include <stdio.h>

Cache::Cache(uint64_t cacheSize, int cacheAss, int lineSize)
{
    // setup member vars
    m_cacheSize = cacheSize;
    m_cacheAss = cacheAss;
    m_lineSize = lineSize;

    // get number of bits for offset
    m_offsetBits = getLog2(lineSize);
    m_indexBits = getLog2(cacheSize/cacheAss/lineSize);
    m_tagBits = 64 - m_offsetBits - m_indexBits;

    // pre-compute masks
    m_offsetMask = getMask(m_offsetBits);
    m_indexMask = getMask(m_indexBits);
    m_tagMask = getMask(m_tagBits);

     m_numLines = cacheSize/lineSize/cacheAss;
    lines = new CacheEntry *[m_numLines];
    for (int i = 0; i < m_numLines; i++) 
    {
        lines[i] = new CacheEntry[cacheAss];
        for (int j = 0; j < cacheAss; j++) {
          lines[i][j].setLRUStatus( j + 1 );
        }
    }
}

// address accessors
Address Cache::getTag(Address a) 
{ 
    return ((a >> (m_indexBits + m_offsetBits)) & m_tagMask);
}

Address Cache::getIndex(Address a) 
{ 
    return ((a >> m_offsetBits) & m_indexMask); 
}

Address Cache::getOffset(Address a) 
{ 
    return a & m_offsetMask; 
}

int Cache::getNumLines( void ){
  return m_numLines;
}


void Cache::load(Address newPhysicalAddress, 
                 Address newLoadingPC, 
                 CacheEntry &victimEntry)
{
    // replace a block if necessary
    Address index = getIndex(newPhysicalAddress);

    // find the least recently used cache line, and replace it
    int lruEntry = findLRUIndex( index );
    lines[index][lruEntry].setValid( true );
    updateLRUStatus( index, lruEntry );
    victimEntry = lines[index][lruEntry].replace(newPhysicalAddress,
                                                 newLoadingPC);
    return;
}

void Cache::load(Address newPhysicalAddress, 
                 Address newLoadingPC, 
                 CacheEntry &victimEntry,
                 int     ReadOrWrite,
                 bool    isData)
{
    // replace a block if necessary
    Address index = getIndex(newPhysicalAddress);

    // find the least recently used cache line, and replace it
    int lruEntry = findLRUIndex( index );
    lines[index][lruEntry].setValid( true );
    updateLRUStatus( index, lruEntry );
    victimEntry = lines[index][lruEntry].replace(newPhysicalAddress,
                                                 newLoadingPC);
    lines[index][lruEntry].setRorW( ReadOrWrite );
    lines[index][lruEntry].setData( isData );
    return;
}

int Cache::findLRUIndex( Address index )
{
  for (int i = 0; i < m_cacheAss; i++)
  {
      if ( lines[index][i].getLRUStatus() == m_cacheAss )
      {
          return (i);
      }
  }
  for (int i = 0; i < m_cacheAss; i++)
  {
      fprintf(stderr, "%d = %d\n", i, lines[index][i].getLRUStatus());
  }
  fprintf(stderr, "findLRU: couldn't find lru in cache set.\n");
  return (0);
}

void Cache::updateLRUStatus(Address mostRecentAddress)
{
    Address index = getIndex(mostRecentAddress);
    int     previousLRUStatus;
    int     tagMatch = 0;
    int prevOffset = 0;

    // set this cache lines LRU status to "1"
    for (int i = 0; i < m_cacheAss; i++)
    {
        if (getTag(lines[index][i].getPhysicalAddress()) == 
            getTag(mostRecentAddress) &&
            lines[index][i].isValid())
        {
            previousLRUStatus = lines[index][i].getLRUStatus();
            lines[index][i].setLRUStatus( 1 );
            prevOffset = i;
            tagMatch++;
        }
    }
    if (tagMatch != 1) {
      fprintf(stderr, "update LRU: mutliple tag matches %d.\n", tagMatch);
    } 

    // update all other lru status in same line
    for (int i = 0; i < m_cacheAss; i++)
    {
        if (getTag(lines[index][i].getPhysicalAddress()) != 
            getTag(mostRecentAddress) ||
            !lines[index][i].isValid())
        {
            int currentLRUStatus = lines[index][i].getLRUStatus();
            if (currentLRUStatus < previousLRUStatus)
            {
                lines[index][i].setLRUStatus(currentLRUStatus + 1);
            }
        }
    }
}

// update LRU status of a particular cache entry, knowing the index and offset
void Cache::updateLRUStatus(Address index, int entryOffset)
{
    int previousLRUStatus = lines[index][entryOffset].getLRUStatus();
    lines[index][entryOffset].setLRUStatus(1);

    // update all other lru status in same line
    for (int i = 0; i < m_cacheAss; i++)
    {
        if (i != entryOffset || !lines[index][i].isValid()) 
        {
            int currentLRUStatus = lines[index][i].getLRUStatus();
            if (currentLRUStatus < previousLRUStatus)
            {
                lines[index][i].setLRUStatus(currentLRUStatus + 1);
            }
        }
    }
}


bool Cache::isHit(Address a, CacheEntry * &matchedEntry)
{
    bool foundMatch = false;
    
    Address index = getIndex(a);
    for (int i = 0; i < m_cacheAss; i++)
    {
        if (getTag(lines[index][i].getPhysicalAddress()) == getTag(a) &&
            lines[index][i].isValid())
        {
            foundMatch = true;
            matchedEntry = &lines[index][i];
        }
    }
    return foundMatch;
}

int Cache::getLog2(uint64_t num)
{
    int count = 0;
    while (num > 1) 
    {
        num = num / 2;
        count++;
    }
    return count;
}

uint64_t Cache::getMask(uint64_t numBits) 
{
    uint64_t mask = 0;

    for (uint64_t i = 0; i < numBits; i++)
    {
        mask = (mask << 1) | 0x1;
    }
    return mask;
}

void   Cache::dump( void )
{
  cout << "Cache Dump" << endl;
  cout << "tag bits: " << m_tagBits << endl;
  cout << "index bits: " << m_indexBits << endl;
  cout << "offset bits: " << m_offsetBits << endl;
  cout << " -- cache assoc: " << m_cacheAss << endl;
  cout << " -- line size: " << m_lineSize << endl;
  cout << " -- cache size: " << m_cacheSize << endl;
  
  printf("0x%016llx\n", m_tagMask);
  printf("0x%016llx\n", m_indexMask);
  printf("0x%016llx\n", m_offsetMask);

  printf("0x%016llx\n", 0x18000C3ull);
  printf("0x%016llx\n", getTag(0x18000C3ull));
  printf("0x%016llx\n", getIndex(0x18000C3ull));
  printf("0x%016llx\n",getOffset(0x18000C3ull));
}

