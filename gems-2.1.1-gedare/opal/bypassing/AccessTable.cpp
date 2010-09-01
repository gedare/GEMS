
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
#include "AccessTable.h"
#include <stdio.h>

AccessTable::AccessTable(void) 
{
    //================================================
    // FIXME: need anything here?
    //================================================
}

AccessTable::~AccessTable(void)
{
}

//  void AccessTable::dump()
//  {
//      PCMap::iterator iter;
//      for (iter = statHash.begin(); iter != statHash.end(); iter++)
//      {
//          uint32_t first_word = (int) (iter->first >> 32);
//          uint32_t second_word = (int) (iter->first & 0xffffffff);
//      if (m_fp) {
//              fprintf(stderr, "PC: %#0X%X\n", first_word, second_word);
//      }
//          iter->second->dumpStats(m_fp);
//      }
//  }

void AccessTable::addAccessEntry(Address tag, int lastAccess)
{
    accessHash.insert(AccessMap::value_type(tag, lastAccess));
}

bool AccessTable::getAccessEntry(Address tag,  int * &lastAccess)
{
    AccessMap::iterator iter;
    iter = accessHash.find(tag);
    if (iter != accessHash.end())
    {
        lastAccess = &iter->second;
        return true;
    }
    return false;
}

//  // table tester
//  void main(void) 
//  {
//      AccessTable st;
    
//      StatEntry se1(12, 56);
//      st.addStatEntry(444, &se1);

//      StatEntry se2(22, 98);
//      st.addStatEntry(555, &se2);

//      StatEntry se3(23, 92);
//      st.addStatEntry(667, &se3);
    
//      st.dump();
//  }
    
