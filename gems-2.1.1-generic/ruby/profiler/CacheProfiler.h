
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
 * CacheProfiler.h
 * 
 * Description: 
 *
 * $Id$
 *
 */

#ifndef CACHEPROFILER_H
#define CACHEPROFILER_H

#include "Global.h"
#include "NodeID.h"
#include "Histogram.h"
#include "AccessModeType.h"
#include "PrefetchBit.h"
#include "GenericRequestType.h"

template <class TYPE> class Vector;

class CacheProfiler {
public:
  // Constructors
  CacheProfiler(string description);

  // Destructor
  ~CacheProfiler();  
  
  // Public Methods
  void printStats(ostream& out) const;
  void clearStats();

  void addStatSample(GenericRequestType requestType, AccessModeType type, int msgSize, PrefetchBit pfBit);

  void print(ostream& out) const;
private:
  // Private Methods

  // Private copy constructor and assignment operator
  CacheProfiler(const CacheProfiler& obj);
  CacheProfiler& operator=(const CacheProfiler& obj);
  
  // Data Members (m_ prefix)
  string m_description;
  Histogram m_requestSize;
  int64 m_misses;
  int64 m_demand_misses;
  int64 m_prefetches;
  int64 m_sw_prefetches;
  int64 m_hw_prefetches;
  int64 m_accessModeTypeHistogram[AccessModeType_NUM];

  Vector < int >* m_requestTypeVec_ptr;
};

// Output operator declaration
ostream& operator<<(ostream& out, const CacheProfiler& obj);

// ******************* Definitions *******************

// Output operator definition
extern inline 
ostream& operator<<(ostream& out, const CacheProfiler& obj)
{
  obj.print(out);
  out << flush;
  return out;
}

#endif //CACHEPROFILER_H
