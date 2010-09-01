
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
 * AddressProfiler.h
 * 
 * Description: 
 *
 * $Id$
 *
 */

#ifndef ADDRESSPROFILER_H
#define ADDRESSPROFILER_H

#include "Global.h"
#include "NodeID.h"
#include "Histogram.h"
#include "Address.h"
#include "CacheMsg.h"
#include "AccessType.h"

class AccessTraceForAddress;
class Set;
template <class KEY_TYPE, class VALUE_TYPE> class Map;

class AddressProfiler {
public:
  // Constructors
  AddressProfiler();

  // Destructor
  ~AddressProfiler();  
  
  // Public Methods
  void printStats(ostream& out) const;
  void clearStats();

  void addTraceSample(Address data_addr, Address pc_addr, CacheRequestType type, AccessModeType access_mode, NodeID id, bool sharing_miss);
  void profilePersistentPrediction(const Address& data_addr, AccessType type);
  void profileRetry(const Address& data_addr, AccessType type, int count);
  void profileGetX(const Address& datablock, const Address& PC, const Set& owner, const Set& sharers, NodeID requestor);
  void profileGetS(const Address& datablock, const Address& PC, const Set& owner, const Set& sharers, NodeID requestor);

  void print(ostream& out) const;
private:
  // Private Methods

  // Private copy constructor and assignment operator
  AddressProfiler(const AddressProfiler& obj);
  AddressProfiler& operator=(const AddressProfiler& obj);
  
  // Data Members (m_ prefix)
  int64 m_sharing_miss_counter;

  Map<Address, AccessTraceForAddress>* m_dataAccessTrace;
  Map<Address, AccessTraceForAddress>* m_macroBlockAccessTrace;
  Map<Address, AccessTraceForAddress>* m_programCounterAccessTrace;
  Map<Address, AccessTraceForAddress>* m_retryProfileMap;
  Map<Address, AccessTraceForAddress>* m_persistentPredictionProfileMap;
  Histogram m_persistentPredictionProfileHisto;
  Histogram m_retryProfileHisto;
  Histogram m_retryProfileHistoWrite;
  Histogram m_retryProfileHistoRead;
  Histogram m_getx_sharing_histogram;
  Histogram m_gets_sharing_histogram;
};

// Output operator declaration
ostream& operator<<(ostream& out, const AddressProfiler& obj);

// ******************* Definitions *******************

// Output operator definition
extern inline 
ostream& operator<<(ostream& out, const AddressProfiler& obj)
{
  obj.print(out);
  out << flush;
  return out;
}

#endif //ADDRESSPROFILER_H
