
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

#ifndef XACTPROFILER_H
#define XACTPROFILER_H

#include "Global.h"
#include "GenericMachineType.h"
#include "RubyConfig.h"
#include "Histogram.h"
#include "Consumer.h"
#include "AccessModeType.h"
#include "AccessType.h"
#include "NodeID.h"
#include "MachineID.h"
#include "PrefetchBit.h"
#include "Address.h"
#include "Set.h"
#include "CacheRequestType.h"
#include "GenericRequestType.h"

template <class KEY_TYPE, class VALUE_TYPE> class Map;

class XactProfiler {
public:
  // Constructors
  XactProfiler();

  // Destructor
  ~XactProfiler();  
  
  void printStats(ostream& out, bool short_stats=false);
  void printShortStats(ostream& out) { printStats(out, true); }
  void clearStats();
  void printConfig(ostream& out) const;

  void print(ostream& out) const;

  void profileTransCycles(int proc, int cycles);
  void profileNonTransCycles(int proc, int cycles);
  void profileStallTransCycles(int proc, int cycles);
  void profileStallNonTransCycles(int proc, int cycles);
  void profileAbortingTransCycles(int proc, int cycles);
  void profileCommitingTransCycles(int proc, int cycles);
  void profileBarrierCycles(int proc, int cycles);
  void profileBackoffTransCycles(int proc, int cycles);
  void profileGoodTransCycles(int proc, int cycles);
  
  void profileBeginTimer(int proc);
  void profileEndTimer(int proc);

  long long int getTransCycles(int proc_no);
  long long int getGoodTransCycles(int proc_no);
  long long int getStallTransCycles(int proc_no);
  long long int getAbortingTransCycles(int proc_no);
  long long int getCommitingTransCycles(int proc_no);
  long long int getBackoffTransCycles(int proc_no);
  long long int getNonTransCycles(int proc_no);
  long long int getBarrierCycles(int proc_no);
  
  void profileHashValue(int hashFunction, int hashValue);

private:

  long long int * m_xactTransCycles;
  long long int * m_xactStallTransCycles;
  long long int * m_xactStallNonTransCycles;
  long long int * m_xactAbortingCycles;
  long long int * m_xactCommitingCycles;
  long long int * m_xactBackoffCycles;
  long long int * m_BarrierCycles;

  long long int * m_xactGoodTransCycles;
  long long int * m_xactNonTransCycles;
  
  long long int * m_xactTimedCycles;
  long long int * m_xactBeginTimer;

  int max_hashFunction;
  Vector<Histogram> m_hashProfile;
};

// Output operator declaration
ostream& operator<<(ostream& out, const XactProfiler& obj);

// ******************* Definitions *******************

// Output operator definition
extern inline 
ostream& operator<<(ostream& out, const XactProfiler& obj)
{
  obj.print(out);
  out << flush;
  return out;
}

#endif //XACTPROFILER_H


