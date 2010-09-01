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

extern "C" {
#include "processor.h"
#include "memory.h"
#include "command.h"
#include "event.h"
#include "simutils.h"
#include "sparc.h"
#include "sparc_fields.h"
#include "memory_v9.h"
};

#include "MissProfiler.h"
#include "Registry.h"

MissProfiler::MissProfiler( char *baseLogFile, 
                            int cacheSize, 
                            int cacheAssociativity, 
                            int cacheLineSize)
{
  fprintf(stderr, "Creating Miss-Profile");
  m_cache_p = new Cache(cacheSize, cacheAssociativity, cacheLineSize);
  m_numLines = m_cache_p->getNumLines();

  m_accessTable = new AccessTable[m_numLines];

  char logFile[256];
  sprintf(logFile, "miss-prof-%s_size%d_ass%d_lsize%d",
          baseLogFile, 
          cacheSize, 
          cacheAssociativity, 
          cacheLineSize);
  
  m_fp = fopen(logFile, "w");
  if (m_fp == NULL) {
    printf("error: could not open %s\n", logFile);
  }

  fprintf(m_fp, "Size = %d\n", cacheSize);
  fprintf(m_fp, "Ass. = %d\n", cacheAssociativity);
  fprintf(m_fp, "Line Size = %d\n", cacheLineSize);

  clearTotalStats();
  
  // Do this in create caches
  //Registry::instance->addController( strdup(logFile), this );
}

MissProfiler::~MissProfiler(){
  delete m_cache_p;
  delete m_accessTable;
}

void MissProfiler::cacheAccess(memory_transaction_t *mem_trans, 
                               Address pc )
{
  ++m_totalAccesses;
  ++m_recentAccesses;  

  generic_transaction_t *gen_trans = (generic_transaction_t *) mem_trans;
  CacheEntry * entry;  
  Address address = (Address) mem_trans->s.physical_address;
  bool isHit = m_cache_p->isHit(address, entry);
  
  if (isHit){
    m_recent_hits++;
    m_total_hits++;
  }
  else {
    // Load the new address into the cache (Don't care about PC)
    CacheEntry victim;
    m_cache_p->load(address, 0, victim, gen_trans->read_or_write, true);

    // Determine type of miss
    int line = m_cache_p->getIndex(address);
    Address tag = m_cache_p->getTag(address);
    int* lastAccess;
    bool found = m_accessTable[line].getAccessEntry(tag, lastAccess);
    
    // Cache line was accessed previously
    if(found){

      int prevAccess = *lastAccess;
      if(m_lineAccesses - prevAccess > m_numLines*m_cacheAss){
        // capacity miss
        m_recent_capacityMisses++;
        m_total_capacityMisses++;
      }
      else{
        // conflict miss
        m_recent_conflictMisses++;
        m_total_conflictMisses++;
      }

      // Update status
      *lastAccess = m_lineAccesses;

    }
    else {
      // Compulsory Miss
      m_recent_compulsoryMisses++;
      m_total_compulsoryMisses++;
    
      m_accessTable[line].addAccessEntry(tag, m_lineAccesses);
    }

    if(line != m_lastIndex){
      m_lineAccesses++;
    }

  }

  if(m_recentAccesses >= 100000){
    logStats();
    clearRecentStats();
  }
}
    
void MissProfiler::clearRecentStats(void){
  m_recent_compulsoryMisses = 0;
  m_recent_conflictMisses = 0;
  m_recent_capacityMisses = 0;
  m_recent_hits = 0;
  m_recentAccesses = 0;
}

void MissProfiler::clearTotalStats(void){
  m_total_compulsoryMisses = 0;
  m_total_conflictMisses = 0;
  m_total_capacityMisses = 0;
  m_total_hits = 0;
  m_totalAccesses = 0;
}

void MissProfiler::logStats(void){
  fprintf(m_fp, "%d %d %d %d %d \t%d %d %d %d %d\n",  
          m_recent_compulsoryMisses,
          m_recent_conflictMisses,
          m_recent_capacityMisses,
          m_recent_hits,
          m_recentAccesses,
          m_total_compulsoryMisses,
          m_total_conflictMisses,
          m_total_capacityMisses,
          m_total_hits,
          m_totalAccesses );      
  fflush(m_fp);
}








