
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

#ifndef CS752_CACHEMANAGER_H
#define CS752_CACHEMANAGER_H

/** SimICS memory includes */
extern "C" {
#include "processor.h"
#include "memory.h"
#include "command.h"
#include "event.h"
#include "simutils.h"
#include "sparc.h"
#include "sparc_fields.h"
};
#include <sys/types.h>
#include "StatEntry.h"
#include "StatTable.h"
#include "CacheController.h"
#include "Cache.h"
#include "VictimBuffer.h"
#include "MissProfiler.h"
//#include "Debug.h"

class CacheManager : public CacheController
{
    
public:
    CacheManager( char *baseLogFile, 
                  char *cacheName, 
                  int cacheSize, 
                  int cacheAssociativity, 
                  int cacheLineSize,
                  int L1InstrCacheSize, 
                  int L1DataCacheSize, 
                  int victimBufferSize);    

    CacheManager(char *logFile, 
                 char *cacheName, 
                 int cacheSize, 
                 int cacheAssociativity, 
                 int cacheLineSize, 
                 int L1InstrCacheSize, 
                 int L1DataCacheSize, 
                 int bypassThreshold, 
                 int bypassMaxPredicts,
                 int bufferSize); // size of victim/bypass buffer

    ~CacheManager();
    
    void cacheAccess(memory_transaction_t *mem_trans, Address pc );
    bool statCacheAccess(generic_transaction_t *mem_trans);
    void statVictimHit(generic_transaction_t *mem_trans);
    void statBypassBufferHit(generic_transaction_t *mem_trans);
    void statBypass(generic_transaction_t *mem_trans);
    void statMainCacheHit(generic_transaction_t *mem_trans);
    void clearRecentStats(void);
    void clearTotalStats(void);
    void logStats(void);
    
private:
    void initCache(char *logFile, 
                   char *cacheName, 
                   int cacheSize, 
                   int cacheAssociativity, 
                   int cacheLineSize, 
                   int L1InstrCacheSize, 
                   int L1DataCacheSize, 
                   bool doBypassing, 
                   int bypassThreshold, 
                   int bypassMaxPredicts,
                   int bufferSize); // size of victim/bypass buffer
  
    Cache *m_cache_p;
    Cache *m_bypassBuffer_p;
    Cache *m_victimBuffer_p;

    Cache *m_instr_cache_p;
    Cache *m_data_cache_p;
    MissProfiler *m_missProfiler_p;
    
    StatTable *m_stat_p;
    FILE *m_fp;

    int m_bypass_threshold;
    int m_bypass_max_predicts;

    bool m_doBypassing;
    
    char *m_cacheName;
    
    // totals
    int m_total_i_loads;
    int m_total_d_loads;
    int m_total_i_stores;
    int m_total_d_stores;

    int m_total_main_i_hits;
    int m_total_main_d_hits;

    int m_total_i_bypasses;
    int m_total_d_bypasses;
    int m_total_bypass_i_hits;
    int m_total_bypass_d_hits;
    int m_total_victim_i_hits;
    int m_total_victim_d_hits;

    // last X accesses (recent) 
    int m_recent_i_loads;
    int m_recent_d_loads;
    int m_recent_i_stores;
    int m_recent_d_stores;

    int m_recent_main_i_hits;
    int m_recent_main_d_hits;

    int m_recent_i_bypasses;
    int m_recent_d_bypasses;
    int m_recent_bypass_i_hits;
    int m_recent_bypass_d_hits;
    int m_recent_victim_i_hits;
    int m_recent_victim_d_hits;

};

#endif

