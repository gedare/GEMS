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

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <math.h>

#include "hfa.h"
#include "CacheManager.h"
#include "StatEntry.h"
#include "Registry.h"

CacheManager::CacheManager( char *logFile, 
                            char *cacheName, 
                            int cacheSize, 
                            int cacheAssociativity, 
                            int cacheLineSize,
                            int L1InstrCacheSize, 
                            int L1DataCacheSize, 
                            int victimBufferSize) 
{
    this->initCache(logFile, 
                    cacheName, 
                    cacheSize, 
                    cacheAssociativity, 
                    cacheLineSize, 
                    L1InstrCacheSize,
                    L1DataCacheSize, 
                    false, 
                    0,
                    0,
                    victimBufferSize);
}

CacheManager::CacheManager(char *logFile, 
                           char *cacheName, 
                           int cacheSize, 
                           int cacheAssociativity, 
                           int cacheLineSize, 
                           int L1InstrCacheSize, 
                           int L1DataCacheSize, 
                           int bypassThreshold, 
                           int bypassMaxPredicts,
                           int bufferSize)
{
    this->initCache(logFile, 
                    cacheName, 
                    cacheSize, 
                    cacheAssociativity, 
                    cacheLineSize, 
                    L1InstrCacheSize,
                    L1DataCacheSize, 
                    true, 
                    bypassThreshold, 
                    bypassMaxPredicts,
                    bufferSize);
}


CacheManager::~CacheManager()
{
    fclose(m_fp);

    delete m_cache_p;
    delete m_bypassBuffer_p;
}


void CacheManager::initCache(char *baseLogFile, 
                             char *cacheName, 
                             int cacheSize, 
                             int cacheAssociativity, 
                             int cacheLineSize, 
                             int L1InstrCacheSize, 
                             int L1DataCacheSize, 
                             bool doBypassing, 
                             int bypassThreshold, 
                             int bypassMaxPredicts,
                             int bufferSize) // size of victim/bypass buffer
{

    char logFile[256];
    sprintf(logFile, "%s_size%d_ass%d_lsize%d_by%d_th%d_pr%d_bs%d.log", 
            baseLogFile, 
            cacheSize, 
            cacheAssociativity, 
            cacheLineSize, 
            doBypassing, 
            bypassThreshold, 
            bypassMaxPredicts,
            bufferSize);

    m_fp = fopen(logFile, "w");
    if (m_fp == NULL) {
      printf("error: could not open %s\n", logFile);
    }

    // Add to registry in the Create Caches code
    // Registry::instance->addController( strdup(logFile), this );

    fprintf(m_fp, "Size = %d\n", cacheSize);
    fprintf(m_fp, "Ass. = %d\n", cacheAssociativity);
    fprintf(m_fp, "Line Size = %d\n", cacheLineSize);
    fprintf(m_fp, "Bypassing = %d\n", doBypassing);
    fprintf(m_fp, "Threshold = %d\n", bypassThreshold);
    fprintf(m_fp, "Max Predicts = %d\n", bypassMaxPredicts);
    fprintf(m_fp, "Buffer Size = %d\n", bufferSize);

    m_cache_p = new Cache(cacheSize, cacheAssociativity, cacheLineSize);

    if (doBypassing) 
    {
        m_bypassBuffer_p = new Cache(bufferSize, bufferSize/cacheLineSize, cacheLineSize);
    }
    else
    {
        m_bypassBuffer_p = NULL;
    }

    if (!m_doBypassing){
         m_victimBuffer_p = new VictimBuffer(bufferSize, cacheLineSize);
    }

    // create L1 instruction and data caches (4-way set assoc == 4)
    m_instr_cache_p = new Cache(L1InstrCacheSize, 4, cacheLineSize);
    m_data_cache_p = new Cache(L1DataCacheSize, 4, cacheLineSize);

    m_stat_p = new StatTable();
    m_bypass_threshold = bypassThreshold;
    m_bypass_max_predicts = bypassMaxPredicts;
    m_doBypassing = doBypassing;
    m_cacheName = cacheName;

    clearRecentStats();
    clearTotalStats();

    m_missProfiler_p = new MissProfiler(baseLogFile, 
                                      cacheSize, 
                                      cacheAssociativity, 
                                      cacheLineSize);

}

void CacheManager::cacheAccess( memory_transaction_t *mem_trans,
                                Address pc ) {
  
    generic_transaction_t *gen_trans = (generic_transaction_t *) mem_trans;
    CacheEntry  *curEntry;
    StatEntry   *curStats;
    CacheEntry   victim;

    // check to see that this is a valid memory transaction
    if ( !( ((gen_trans->data_or_instr == Instruction) ||
             (gen_trans->data_or_instr == Data)) &&
            ((gen_trans->type == Ld || gen_trans->type == Ld_U ) ||
             (gen_trans->type == St)) ) )
        return;

    // ignore this instruction reference if it can not be cached
    if ( gen_trans->cache_bit ) {
        // currently this never happens
        printf( "c" );
        return;
    }

    // ignore this instruction if it is particularly large
    if ( gen_trans->size == (unsigned) -1 ) {
        printf( " saw this: %u %d\n", gen_trans->size, gen_trans->size );
    }
    if ( gen_trans->size > 64 ) {
        // This actually DOES happen, and size is usually around 1 million!
        // That doesn't seem right ... so best ignore them ...
        // printf( " %u %d ", gen_trans->size, gen_trans->size );
        // fflush( stdout );
        return;
    }

    // check for level one cache hit
    generic_transaction_t  victim_trans;
    CacheEntry             victimEntry;
    CacheEntry            *L1Entry;
    if (gen_trans->data_or_instr == Data) {
        // do load in L1 Data Cache
        bool isHit 
            = m_data_cache_p->isHit((Address) mem_trans->s.physical_address,
                                    L1Entry);

        if (isHit) {
            // HIT IN L1 CACHE
            // CM: could update LRU for associative L1 caches
            m_data_cache_p->updateLRUStatus( (Address) mem_trans->s.physical_address );
            L1Entry->registerHit();
            return;
        } else {
            // MISS IN L1 CACHE (simulate load)

            // do load in L1 data cache
            m_data_cache_p->load((Address)mem_trans->s.physical_address, 
                                 pc, 
                                 victimEntry,
                                 gen_trans->read_or_write,
                                 (gen_trans->data_or_instr == Data));
            
            // remove the victim from the STC
            if ( victimEntry.isValid() ) {
                if (victimEntry.isData())
                    victim_trans.data_or_instr = Data;
                else
                    victim_trans.data_or_instr = Instruction;
                victim_trans.cpu_ptr = mem_trans->s.cpu_ptr;
                victim_trans.read_or_write = 
                    (read_or_write_t) victimEntry.getRorW();
                victim_trans.physical_address = victimEntry.getPhysicalAddress();
                victim_trans.size = (unsigned) -1;
                mem_remove_from_STC( &victim_trans );
                
                /* printf("victim: 0x%016llx waxed by: 0x%016llx\n",
                          victimEntry.getPhysicalAddress(),
                          (Address)mem_trans->s.physical_address);
                */
            }
        }
        
    } else if (gen_trans->data_or_instr == Instruction) {
        bool isHit 
            = m_instr_cache_p->isHit((Address) mem_trans->s.physical_address,
                                     L1Entry);
        
        if (isHit) {
            // HIT IN L1 CACHE 
            m_instr_cache_p->updateLRUStatus( (Address) mem_trans->s.physical_address );
            L1Entry->registerHit();
            return;
        } else {
            // MISS IN L1 CACHE (simulate load)

            // do load in L1 Instruction Cache
            m_instr_cache_p->load((Address)mem_trans->s.physical_address, 
                                  pc, 
                                  victimEntry,
                                  gen_trans->read_or_write,
                                  (gen_trans->data_or_instr == Data));
            
            // remove the victim from the STC
            if ( victimEntry.isValid() ) {
                if (victimEntry.isData())
                    victim_trans.data_or_instr = Data;
                else
                    victim_trans.data_or_instr = Instruction;
                victim_trans.cpu_ptr = mem_trans->s.cpu_ptr;
                victim_trans.read_or_write =
                    (read_or_write_t) victimEntry.getRorW();
                victim_trans.physical_address = victimEntry.getPhysicalAddress();
                victim_trans.size = (unsigned) -1;
                mem_remove_from_STC( &victim_trans );

                /* printf("victim: 0x%016llx waxed by: 0x%016llx\n",
                          victimEntry.getPhysicalAddress(),
                          (Address)mem_trans->s.physical_address);
                */
            }

        }
    }

    // Dump cache stats 
    if ( (m_total_i_loads + m_total_d_loads + 
          m_total_i_stores + m_total_d_stores) % 100000 == 0 ) 
    {
        this->logStats();
        this->clearRecentStats();
    }

    // register the cache load
    if (!this->statCacheAccess( gen_trans ))
        return;

    // Update Miss Profile Statistics
    m_missProfiler_p->cacheAccess( mem_trans, pc );

    // if this is a bypassing cache, check for a hit in the bypass buffer
    if (m_doBypassing) 
    {
        bool isHit 
            = m_bypassBuffer_p->isHit((Address) mem_trans->s.physical_address,
                                      curEntry );
        if (isHit) 
        {
            m_bypassBuffer_p->updateLRUStatus( (Address) mem_trans->s.physical_address );
            curEntry->registerHit();
            this->statBypassBufferHit( gen_trans );
            return;
        } // is hit in bypassBuffer
    } // check for hit in bypass buffer

    // not a bypassBuffer hit or not doing bypassing, check in the reg. cache
    bool isHit = m_cache_p->isHit( (Address) mem_trans->s.physical_address,
                                   curEntry );
    if (isHit) // is a hit in the regular cache
    {
        m_cache_p->updateLRUStatus( (Address) mem_trans->s.physical_address );
        curEntry->registerHit();
        this->statMainCacheHit( gen_trans );
    } 
    // Check for hit in Victim Cache
    else if (!m_doBypassing && 
             m_victimBuffer_p->isHit((Address) mem_trans->s.physical_address,
                                     curEntry ))
    {
         m_victimBuffer_p->updateLRUStatus( (Address) mem_trans->s.physical_address );
         this->statVictimHit( gen_trans );
    }
    else  // not a hit in the regular cache
    {
        // determine if this is a bypass load or not
        bool doBypass = false;
        if (m_doBypassing) 
        {
            bool isFound = m_stat_p->getStatEntry( pc, curStats );
            
            // if we have an entry for this PC, see if we should do the bypass
            if (isFound) {
                doBypass = curStats->bypass();
            }
        }
        
        if (doBypass) 
        {
            // load bypassed info into bypass buffer
            m_bypassBuffer_p->load((Address)mem_trans->s.physical_address, 
                                   pc, 
                                   victim);
            this->statBypass(gen_trans);
        } 
        else // don't bypass this data, load into the regular cache
        {
            // do the regular load
            m_cache_p->load((Address)mem_trans->s.physical_address,
                            pc,
                            victim );
            
            // add number of hits from replaced cache line to
            // corresponding entry in stats table
            StatEntry        *victimStats;    
            int     numHits = victim.getNumHits();
            Address oldPC   = victim.getLoadingPC();
            bool isFound = m_stat_p->getStatEntry( oldPC, victimStats );
            if (!isFound) 
            {
                // allocate a new stat entry and add to the table
                victimStats = new StatEntry( oldPC );
                m_stat_p->addStatEntry( oldPC, victimStats );
                
                //================================================
                // FIXME: should we be adding anything to this 
                //        newly created stat entry?
                //================================================
             
                // Put victim line in the victim buffer
                CacheEntry bufferVictim;
                if(!m_doBypassing){
                     m_victimBuffer_p->load((Address)mem_trans->s.physical_address,
                                            pc,
                                            bufferVictim );
                }
            } 
            else 
            {
                victimStats->addLoad();
                victimStats->addHits( numHits );
            }
            
            bool doBypass = ( numHits <= m_bypass_threshold );
            victimStats->updateBypass( doBypass, m_bypass_max_predicts );
        }
    }
}

bool CacheManager::statCacheAccess(generic_transaction_t *mem_trans)
{
    if (mem_trans->data_or_instr == Instruction &&
        (mem_trans->type == Ld || mem_trans->type == Ld_U ))
    {
        // INSTRUCTION LOAD
        m_total_i_loads++;
        m_recent_i_loads++;
    }
    else if (mem_trans->data_or_instr == Instruction &&
             mem_trans->type == St)
    {
        // INSTRUCTION STORE?
        m_total_i_stores++;
        m_recent_i_stores++;
    }
    else if (mem_trans->data_or_instr == Data &&
             (mem_trans->type == Ld || mem_trans->type == Ld_U))
    {
        // DATA LOAD
        m_total_d_loads++;
        m_recent_d_loads++;
    }
    else if (mem_trans->data_or_instr == Data &&
             mem_trans->type == St)
    {
        // DATA STORE
        m_total_d_stores++;
        m_recent_d_stores++;
    }
    else
    {
        return false;
    }

    return true;
}



void CacheManager::statVictimHit(generic_transaction_t *mem_trans)
{
    if ( mem_trans->data_or_instr == Instruction ) {
        m_total_victim_i_hits++;
        m_recent_victim_i_hits++;
    } else if ( mem_trans->data_or_instr == Data ) {
        m_total_victim_d_hits++;
        m_recent_victim_d_hits++;
    }
    else
    {
        fprintf(stderr, "ERROR: undefined victim hit type\n");
    }
}


void CacheManager::statBypassBufferHit(generic_transaction_t *mem_trans)
{
    if ( mem_trans->data_or_instr == Instruction ) {
        m_total_bypass_i_hits++;
        m_recent_bypass_i_hits++;
    } else if ( mem_trans->data_or_instr == Data ) {
        m_total_bypass_d_hits++;
        m_recent_bypass_d_hits++;
    }
    else
    {
        fprintf(stderr, "ERROR: undefined bypass buffer hit type\n");
    }
}


void CacheManager::statMainCacheHit(generic_transaction_t *mem_trans)
{
    if ( mem_trans->data_or_instr == Instruction ) {
        m_total_main_i_hits++;
        m_recent_main_i_hits++;
    } else if ( mem_trans->data_or_instr == Data ) {
        m_total_main_d_hits++;
        m_recent_main_d_hits++;
    }
    else
    {
        fprintf(stderr, "ERROR: undefined main cache hit type\n");
    }
}


void CacheManager::statBypass(generic_transaction_t *mem_trans)
{
    if ( mem_trans->data_or_instr == Instruction ) {
        m_total_i_bypasses++;
        m_recent_i_bypasses++;
    } else if ( mem_trans->data_or_instr == Data ) {
        m_total_d_bypasses++;
        m_recent_d_bypasses++;
    }
    else
    {
        fprintf(stderr, "ERROR: undefined bypass type\n");
    }
}


void CacheManager::clearRecentStats(void)
{
    m_recent_i_loads = 0;
    m_recent_d_loads = 0;
    m_recent_i_stores = 0;
    m_recent_d_stores = 0;

    m_recent_main_i_hits = 0;
    m_recent_main_d_hits = 0;

    m_recent_i_bypasses = 0;
    m_recent_d_bypasses = 0;
    m_recent_bypass_i_hits = 0;
    m_recent_bypass_d_hits = 0;
    m_recent_victim_i_hits = 0;
    m_recent_victim_d_hits = 0;

}


void CacheManager::clearTotalStats(void)
{
    m_total_i_loads = 0;
    m_total_d_loads = 0;
    m_total_i_stores = 0;
    m_total_d_stores = 0;

    m_total_main_i_hits = 0;
    m_total_main_d_hits = 0;

    m_total_i_bypasses = 0;
    m_total_d_bypasses = 0;
    m_total_bypass_i_hits = 0;
    m_total_bypass_d_hits = 0;
    m_total_victim_i_hits = 0;
    m_total_victim_d_hits = 0;
}

void CacheManager::logStats(void)
{
    // log recent information first
    fprintf(m_fp, "%d ", m_recent_i_loads);      
    fprintf(m_fp, "%d ", m_recent_d_loads);      
    fprintf(m_fp, "%d ", m_recent_i_stores);     
    fprintf(m_fp, "%d ", m_recent_d_stores);     

    fprintf(m_fp, "%d ", m_recent_main_i_hits);       
    fprintf(m_fp, "%d ", m_recent_main_d_hits);       

    fprintf(m_fp, "%d ", m_recent_i_bypasses);   
    fprintf(m_fp, "%d ", m_recent_d_bypasses);   
    fprintf(m_fp, "%d ", m_recent_bypass_i_hits);
    fprintf(m_fp, "%d ", m_recent_bypass_d_hits);
    fprintf(m_fp, "%d ", m_recent_victim_i_hits);
    fprintf(m_fp, "%d ", m_recent_victim_d_hits);

    // log totals
    fprintf(m_fp, "%d ", m_total_i_loads);
    fprintf(m_fp, "%d ", m_total_d_loads);
    fprintf(m_fp, "%d ", m_total_i_stores);
    fprintf(m_fp, "%d ", m_total_d_stores);

    fprintf(m_fp, "%d ", m_total_main_i_hits);
    fprintf(m_fp, "%d ", m_total_main_d_hits);

    fprintf(m_fp, "%d ", m_total_i_bypasses);
    fprintf(m_fp, "%d ", m_total_d_bypasses);
    fprintf(m_fp, "%d ", m_total_bypass_i_hits);
    fprintf(m_fp, "%d ", m_total_bypass_d_hits);
    fprintf(m_fp, "%d ", m_total_victim_i_hits);
    fprintf(m_fp, "%d ", m_total_victim_d_hits);

    fprintf(m_fp, "\n");

    // flush out the writes
    fflush(m_fp);
}    
