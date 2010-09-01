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

#include "CacheManager.h"
#include "CreateCaches.h"
#include "Registry.h"
#include "MissProfiler.h"
//#include "Debug.h"

void create_caches_cmd(int cacheSize,
                       int cacheAssociativity,
                       int cacheLineSize,
                       int L1InstrCacheSize,
                       int L1DataCacheSize,
                       int bypassThreshold,
                       int bypassMaxPredicts,
                       int bufferSize)
{

    // Create simple non-bypassing cache for baseline stats
    CacheManager *nonBypassManager_p;
    nonBypassManager_p = new CacheManager( "simple", 
                                           "Non-Bypassing Cache", 
                                           cacheSize, 
                                           cacheAssociativity, 
                                           cacheLineSize,
                                           L1InstrCacheSize,
                                           L1DataCacheSize,
                                           bufferSize);
    Registry::instance->addController( nonBypassManager_p );
    
    // Create bypassing cache
    CacheManager *bypassManager_p;
    bypassManager_p = new CacheManager( "bypass", 
                                        "Bypassing Cache", 
                                        cacheSize, 
                                        cacheAssociativity, 
                                        cacheLineSize,
                                        L1InstrCacheSize,
                                        L1DataCacheSize,
                                        bypassThreshold, 
                                        bypassMaxPredicts,
                                        bufferSize );
    Registry::instance->addController( bypassManager_p );
}    


void create_cache_v1( void ) {
    //================================================
    // Cache parameters
    //================================================

    // 8 MB total cache size
//    int cacheSize = 0x800000;

    // 4 MB total cache size
    int cacheSize = 0x400000;

    // 2-way set associative
    int cacheAssociativity = 2;

    // 64 byte lines
    int cacheLineSize = 0x40;
    
    // 16 K direct mapped instruction and data caches
//    int L1InstrCacheSize = 16384;
//    int L1DataCacheSize = 16384;

    int L1InstrCacheSize = 32768;
    int L1DataCacheSize = 32768;

    // bypass buffer threshold
    int bypassThreshold = 3;
    
    // bypass buffer maximimum predictions until reset
    int bypassMaxPredicts = 5;

    // Bypass/Victim buffer size -- 32 lines * 64 bytes
    int bufferSize = 1024;

    // Create simple non-bypassing cache for baseline stats
    CacheManager *nonBypassManager_p;
    nonBypassManager_p = new CacheManager( "simple", 
                                           "Non-Bypassing Cache", 
                                           cacheSize, 
                                           cacheAssociativity, 
                                           cacheLineSize,
                                           L1InstrCacheSize,
                                           L1DataCacheSize,
                                           bufferSize);
    Registry::instance->addController( nonBypassManager_p );
    
    // Create bypassing cache
    CacheManager *bypassManager_p;
    bypassManager_p = new CacheManager( "bypass", 
                                        "Bypassing Cache", 
                                        cacheSize, 
                                        cacheAssociativity, 
                                        cacheLineSize,
                                        L1InstrCacheSize,
                                        L1DataCacheSize,
                                        bypassThreshold, 
                                        bypassMaxPredicts,
                                        bufferSize );
    Registry::instance->addController( bypassManager_p );
}

void destroy_cache_v1( void ) {
    Registry::instance->removeAllControllers();
}

void destroy_all_caches( void ) {
    Registry::instance->removeAllControllers();
}


void create_cache_Dec12(void)
{
    //================================================
    // Cache parameters
    //================================================

    // 8 MB total cache size
    int cacheSize[] = { 0x400000, 0x200000 };
    
    // 4-way set associative
    int cacheAssociativity[] = { 1, 2 };

    // 64 byte lines
    int cacheLineSize[] = { 64 };
    
    // 16 K direct mapped instruction and data caches
    int L1InstrCacheSize = 16384;
    int L1DataCacheSize = 16384;

    // bypass buffer threshold
    int bypassThreshold[] = { 1, 3 };
    
    // bypass buffer maximimum predictions until reset
    int bypassMaxPredicts[] = { 5 };

    // Bypass/Victim buffer size -- 8 lines * 64 bytes = 512 bytes
    int bufferSize[] = { 16, 32 };


    for (int sizeIndex = 0; sizeIndex < 2; sizeIndex++)
    {
        for (int assIndex = 0; assIndex < 2; assIndex++)
        {
            for (int lineSizeIndex = 0; lineSizeIndex < 1; lineSizeIndex++)
            {
                for (int bSizeIndex = 0; bSizeIndex < 2; bSizeIndex++)
                {
                            
    // Create simple non-bypassing cache for baseline stats
    CacheManager *nonBypassManager_p;
    nonBypassManager_p = new CacheManager( "simple", 
                                           "Non-Bypassing Cache", 
                                           cacheSize[sizeIndex], 
                                           cacheAssociativity[assIndex], 
                                           cacheLineSize[lineSizeIndex],
                                           L1InstrCacheSize,
                                           L1DataCacheSize,
                                           bufferSize[bSizeIndex] * cacheLineSize[lineSizeIndex]);
                }
            }
        }
    }

    //================================================
    // BYPASS CACHES
    //================================================
    for (int sizeIndex = 0; sizeIndex < 2; sizeIndex++)
    {
        for (int assIndex = 0; assIndex < 2; assIndex++)
        {
            for (int lineSizeIndex = 0; lineSizeIndex < 1; lineSizeIndex++)
            {
                for (int threshIndex = 0; threshIndex < 2; threshIndex++)
                {
                    for (int predIndex = 0; predIndex < 1; predIndex++)
                    {
                        for (int bSizeIndex = 0; bSizeIndex < 2; bSizeIndex++)
                        {
    
    // Create bypassing cache
    CacheManager *bypassManager_p;
    bypassManager_p = new CacheManager( "bypass", 
                                        "Bypassing Cache", 
                                        cacheSize[sizeIndex], 
                                        cacheAssociativity[assIndex], 
                                        cacheLineSize[lineSizeIndex], 
                                        L1InstrCacheSize,
                                        L1DataCacheSize,
                                        bypassThreshold[threshIndex], 
                                        bypassMaxPredicts[predIndex],
                                        bufferSize[bSizeIndex] * cacheLineSize[lineSizeIndex] );
                        }
                    }
                }
            }
        }
    }
}


void destroy_cache_Dec12( void ) {
    Registry::instance->removeAllControllers();
}


void create_miss_profiler( void ){

  MissProfiler * mp_p = new MissProfiler("Query9",
                                         1024*1024, // 1M
                                         4,
                                         64);
  fprintf(stderr,"Adding miss profiler in createcaches.");
  Registry::instance->addController( mp_p );
                                         
}

void destroy_miss_profiler( void ){
}
