
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

#ifndef CS752_MissProfiler_H
#define CS752_MissProfiler_H

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
//#include "Debug.h"
#include "AccessTable.h"


class MissProfiler : public CacheController
{
    
public:
    MissProfiler( char *baseLogFile, 
                  int cacheSize, 
                  int cacheAssociativity, 
                  int cacheLineSize);

    ~MissProfiler();
    
    void cacheAccess(memory_transaction_t *mem_trans, Address pc );
    
    void clearRecentStats(void);
    void clearTotalStats(void);
    void logStats(void);
    
private:
  
    Cache *m_cache_p;
    int m_numLines;
    int m_cacheAss;

    // log file
    FILE *m_fp;    
    
    //Map Cache Line to most recent access
    AccessTable* m_accessTable;

    // totals    
    int m_total_compulsoryMisses;
    int m_total_conflictMisses;
    int m_total_capacityMisses;
    int m_total_hits;
    int m_totalAccesses; 
   
    // recents    
    int m_recent_compulsoryMisses;
    int m_recent_conflictMisses;
    int m_recent_capacityMisses;
    int m_recent_hits;
    int m_recentAccesses;
 
    // Use to check for capacity misses
    int m_lineAccesses;
    int m_lastIndex;
    

};

#endif

