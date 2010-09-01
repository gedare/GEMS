
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
 * CacheProfiler.C
 * 
 * Description: See CacheProfiler.h
 *
 * $Id$
 *
 */

#include "CacheProfiler.h"
#include "AccessTraceForAddress.h"
#include "PrioHeap.h"
#include "System.h"
#include "Profiler.h"
#include "Vector.h"

CacheProfiler::CacheProfiler(string description)
  : m_requestSize(-1)
{
  m_description = description;
  m_requestTypeVec_ptr = new Vector<int>;
  m_requestTypeVec_ptr->setSize(int(GenericRequestType_NUM));

  clearStats();
}

CacheProfiler::~CacheProfiler()
{
  delete m_requestTypeVec_ptr;
}

void CacheProfiler::printStats(ostream& out) const
{
  out << m_description << " cache stats: " << endl;
  string description = "  " + m_description;

  out << description << "_total_misses: " << m_misses << endl;
  out << description << "_total_demand_misses: " << m_demand_misses << endl;
  out << description << "_total_prefetches: " << m_prefetches << endl;
  out << description << "_total_sw_prefetches: " << m_sw_prefetches << endl;
  out << description << "_total_hw_prefetches: " << m_hw_prefetches << endl;

  double trans_executed = double(g_system_ptr->getProfiler()->getTotalTransactionsExecuted());
  double inst_executed = double(g_system_ptr->getProfiler()->getTotalInstructionsExecuted());

  out << description << "_misses_per_transaction: " << double(m_misses) / trans_executed << endl;
  out << description << "_misses_per_instruction: " << double(m_misses) / inst_executed << endl;
  out << description << "_instructions_per_misses: ";
  if (m_misses > 0) {
    out << inst_executed / double(m_misses) << endl;
  } else {
    out << "NaN" << endl;
  }
  out << endl;

  int requests = 0;

  for(int i=0; i<int(GenericRequestType_NUM); i++) {
    requests += m_requestTypeVec_ptr->ref(i);
  }
  
  assert(m_misses == requests);

  if (requests > 0) {
    for(int i=0; i<int(GenericRequestType_NUM); i++){
      if (m_requestTypeVec_ptr->ref(i) > 0) {
        out << description << "_request_type_" << GenericRequestType_to_string(GenericRequestType(i)) << ":   "
            << (100.0 * double((m_requestTypeVec_ptr->ref(i)))) / double(requests)
            << "%" << endl;
      }
    }
    
    out << endl;
    
    for(int i=0; i<AccessModeType_NUM; i++){
      if (m_accessModeTypeHistogram[i] > 0) {
        out << description << "_access_mode_type_" << (AccessModeType) i << ":   " << m_accessModeTypeHistogram[i] 
            << "    " << (100.0 * m_accessModeTypeHistogram[i]) / requests << "%" << endl;
      }
    }
  }

  out << description << "_request_size: " << m_requestSize << endl;
  out << endl;

}

void CacheProfiler::clearStats()
{
  for(int i=0; i<int(GenericRequestType_NUM); i++) {
    m_requestTypeVec_ptr->ref(i) = 0;
  }
  m_requestSize.clear();
  m_misses = 0;
  m_demand_misses = 0;
  m_prefetches = 0;
  m_sw_prefetches = 0;
  m_hw_prefetches = 0;
  for(int i=0; i<AccessModeType_NUM; i++){
    m_accessModeTypeHistogram[i] = 0;
  }
}

void CacheProfiler::addStatSample(GenericRequestType requestType, AccessModeType type, int msgSize, PrefetchBit pfBit)
{
  m_misses++;
  
  m_requestTypeVec_ptr->ref(requestType)++;

  m_accessModeTypeHistogram[type]++;
  m_requestSize.add(msgSize);
  if (pfBit == PrefetchBit_No) {
    m_demand_misses++;
  } else if (pfBit == PrefetchBit_Yes) {
    m_prefetches++;
    m_sw_prefetches++;
  } else { // must be L1_HW || L2_HW prefetch
    m_prefetches++;
    m_hw_prefetches++;
  } 
}

