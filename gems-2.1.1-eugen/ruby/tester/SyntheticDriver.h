
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
 * $Id$
 *
 * Description: 
 *
 */

#ifndef SYNTHETICDRIVER_H
#define SYNTHETICDRIVER_H

#include "Global.h"
#include "Driver.h"
#include "Histogram.h"
#include "CacheRequestType.h"

class System;
class RequestGenerator;

class SyntheticDriver : public Driver, public Consumer {
public:
  // Constructors
  SyntheticDriver(System* sys_ptr);

  // Destructor
  ~SyntheticDriver();
  
  // Public Methods
  Address pickAddress(NodeID node);
  void reportDone();
  void recordTestLatency(Time time);
  void recordSwapLatency(Time time);
  void recordReleaseLatency(Time time);

  void hitCallback(NodeID proc, SubBlock& data, CacheRequestType type, int thread);
  void conflictCallback(NodeID proc, SubBlock& data, CacheRequestType type, int thread) {assert(0)};
  void abortCallback(NodeID proc, SubBlock& data, CacheRequestType type, int thread);
  void wakeup();
  void printStats(ostream& out) const;
  void clearStats() {}
  void printConfig(ostream& out) const {}

  integer_t readPhysicalMemory(int procID, physical_address_t address,
                               int len );
  
  void writePhysicalMemory( int procID, physical_address_t address,
                            integer_t value, int len );

  void print(ostream& out) const;

  // For handling NACKs/retries
  //void notifySendNack( int procID, const Address & addr, uint64 remote_timestamp, const MachineID & remote_id);
  //void notifyReceiveNack( int procID, const Address & addr, uint64 remote_timestamp, const MachineID & remote_id);
  //void notifyReceiveNackFinal( int procID, const Address & addr);
  
private:
  // Private Methods
  void checkForDeadlock();

  // Private copy constructor and assignment operator
  SyntheticDriver(const SyntheticDriver& obj);
  SyntheticDriver& operator=(const SyntheticDriver& obj);
  
  // Data Members (m_ prefix)
  Vector<Time> m_last_progress_vector;
  Vector<RequestGenerator*> m_request_generator_vector;
  Vector<NodeID> m_lock_vector;  // Processor last to hold the lock
  int m_done_counter;

  Histogram m_test_latency;
  Histogram m_swap_latency;
  Histogram m_release_latency;
  Time m_finish_time;
};

// Output operator declaration
ostream& operator<<(ostream& out, const SyntheticDriver& obj);

// ******************* Definitions *******************

// Output operator definition
extern inline 
ostream& operator<<(ostream& out, const SyntheticDriver& obj)
{
  obj.print(out);
  out << flush;
  return out;
}

#endif //SYNTHETICDRIVER_H
