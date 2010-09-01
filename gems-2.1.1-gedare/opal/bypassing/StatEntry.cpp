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

#include "StatEntry.h"
#include <stdio.h>

StatEntry::StatEntry( Address pc, void)
{
    this->pc = pc;
    numHits = 0;
    numLoads = 0;
    doBypass[0] = false;
    doBypass[1] = false;
    totalBypasses = 0;
}

StatEntry::StatEntry( Address pc, int numHits, int numLoads)
{
    this->pc = pc;
    this->numHits = numHits;
    this->numLoads = numLoads;
    doBypass[0] = false;
    doBypass[1] = false;
    totalBypasses = 0;
}

void StatEntry::addHits(int numNewHits) 
{
    numHits += numNewHits;
}

void StatEntry::addLoad(void)
{
    numLoads++;
}

int StatEntry::getHits(void)
{
    return (numHits);
}

int StatEntry::getLoads(void) 
{
    return (numLoads);
}

int StatEntry::getTotalBypasses(void) 
{
    return (totalBypasses);
}

bool StatEntry::bypass(void)
{
    bool  predict;
    if (doBypass[0] && doBypass[1]) {
        if (bypassCount == 0) {
            doBypass[0] = false;
            doBypass[1] = false;            
            predict = false;
        } else {
            totalBypasses++;
            bypassCount--;
            predict = true;
        }
    } else {
        predict = false;
    }
    return (predict);
}

void StatEntry::updateBypass( bool newState, int bypassMaxPredicts )
{
    doBypass[1] = doBypass[0];
    doBypass[0] = newState;
    if (doBypass[0] && doBypass[1]) {
        bypassCount = bypassMaxPredicts;
    }
}

Address StatEntry::getPC( void )
{
    return (pc);
}

void StatEntry::dumpStats(FILE *fp)
{
    fprintf(fp, "Hits: %d\nLoads: %d\nHits/Load: %f\n", numHits, numLoads, (float) numHits/numLoads);
}

