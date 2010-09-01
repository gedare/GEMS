
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
 * $Id: Sequencer.h 1.70 2006/09/27 14:56:41-05:00 bobba@s1-01.cs.wisc.edu $
 *
 * Description: 
 *
 */

#ifndef SEQUENCER_H
#define SEQUENCER_H

#include "Global.h"
#include "RubyConfig.h"
#include "Consumer.h"
#include "CacheRequestType.h"
#include "AccessModeType.h"
#include "GenericMachineType.h"
#include "PrefetchBit.h"
#include "Map.h"

class DataBlock;
class AbstractChip;
class CacheMsg;
class Address;
class MachineID;

class Sequencer : public Consumer {
public:
  // Constructors
  Sequencer(AbstractChip* chip_ptr, int version);

  // Destructor
  ~Sequencer();
  
  // Public Methods
  void wakeup(); // Used only for deadlock detection 

  static void printConfig(ostream& out);

  // returns total number of outstanding request (includes prefetches)
  int getNumberOutstanding();
  // return only total number of outstanding demand requests
  int getNumberOutstandingDemand();
  // return only total number of outstanding prefetch requests
  int getNumberOutstandingPrefetch();

  // remove load/store request from queue
  void removeLoadRequest(const Address & addr, int thread);
  void removeStoreRequest(const Address & addr, int thread);

  void printProgress(ostream& out) const;

  // returns a pointer to the request in the request tables
  CacheMsg & getReadRequest( const Address & addr, int thread );
  CacheMsg & getWriteRequest( const Address & addr, int thread );
  
  // called by Ruby when transaction completes
  void writeConflictCallback(const Address& address);
  void readConflictCallback(const Address& address);
  void writeConflictCallback(const Address& address, GenericMachineType respondingMach, int thread);
  void readConflictCallback(const Address& address, GenericMachineType respondingMach, int thread);
  
  void writeCallback(const Address& address, DataBlock& data);
  void readCallback(const Address& address, DataBlock& data);
  void writeCallback(const Address& address);
  void readCallback(const Address& address);
  void writeCallback(const Address& address, DataBlock& data, GenericMachineType respondingMach, PrefetchBit pf, int thread);
  void readCallback(const Address& address, DataBlock& data, GenericMachineType respondingMach, PrefetchBit pf, int thread);
  void writeCallback(const Address& address, DataBlock& data, GenericMachineType respondingMach, int thread);
  void readCallback(const Address& address, DataBlock& data, GenericMachineType respondingMach, int thread);
  
  // returns the thread ID of the request
  int getRequestThreadID(const Address & addr);
  // returns the physical address of the request
  Address getRequestPhysicalAddress(const Address & lineaddr);
  // returns whether a request is a prefetch request
  bool isPrefetchRequest(const Address & lineaddr);

  //notifies driver of debug print
  void printDebug();

  // called by Tester or Simics
  void makeRequest(const CacheMsg& request);
  bool doRequest(const CacheMsg& request);
  void issueRequest(const CacheMsg& request);
  bool isReady(const CacheMsg& request) const;
  bool empty() const;
  void resetRequestTime(const Address& addr, int thread);
  Address getLogicalAddressOfRequest(Address address, int thread);
  AccessModeType getAccessModeOfRequest(Address address, int thread);
  //uint64 getSequenceNumberOfRequest(Address addr, int thread);

  void print(ostream& out) const;
  void checkCoherence(const Address& address);

  bool getRubyMemoryValue(const Address& addr, char* value, unsigned int size_in_bytes);
  bool setRubyMemoryValue(const Address& addr, char *value, unsigned int size_in_bytes);

  void removeRequest(const CacheMsg& request);
private:
  // Private Methods
  bool tryCacheAccess(const Address& addr, CacheRequestType type, const Address& pc, AccessModeType access_mode, int size, DataBlock*& data_ptr);
  void conflictCallback(const CacheMsg& request, GenericMachineType respondingMach, int thread);
  void hitCallback(const CacheMsg& request, DataBlock& data, GenericMachineType respondingMach, int thread);
  bool insertRequest(const CacheMsg& request);
 

  // Private copy constructor and assignment operator
  Sequencer(const Sequencer& obj);
  Sequencer& operator=(const Sequencer& obj);
  
  // Data Members (m_ prefix)
  AbstractChip* m_chip_ptr;

  // indicates what processor on the chip this sequencer is associated with
  int m_version;

  // One request table per SMT thread
  Map<Address, CacheMsg>** m_writeRequestTable_ptr;
  Map<Address, CacheMsg>** m_readRequestTable_ptr;
  // Global outstanding request count, across all request tables
  int m_outstanding_count;
  bool m_deadlock_check_scheduled;

};

// Output operator declaration
ostream& operator<<(ostream& out, const Sequencer& obj);

// ******************* Definitions *******************

// Output operator definition
extern inline 
ostream& operator<<(ostream& out, const Sequencer& obj)
{
  obj.print(out);
  out << flush;
  return out;
}

#endif //SEQUENCER_H

