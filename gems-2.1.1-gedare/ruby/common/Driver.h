
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

#ifndef DRIVER_H
#define DRIVER_H

#include "Global.h"
#include "Consumer.h"
#include "NodeID.h"
#include "CacheRequestType.h"

class System;
class SubBlock;
class Address;
class MachineID;
class SimicsHypervisor;

class Driver {
public:
  // Constructors
  Driver();

  // Destructor
  virtual ~Driver() = 0;
  
  // Public Methods
  virtual void get_network_config() {} 
  virtual void hitCallback(NodeID proc, SubBlock& data, CacheRequestType type, int thread) = 0; // Called by sequencer
  virtual void conflictCallback(NodeID proc, SubBlock& data, CacheRequestType type, int thread) { assert(0) }; // Called by sequencer
  virtual integer_t getInstructionCount(int procID) const { return 1; }
  virtual integer_t getCycleCount(int procID) const { return 1; }
  virtual SimicsHypervisor * getHypervisor() { return NULL; }
  virtual void notifySendNack( int procID, const Address & addr, uint64 remote_timestamp, const MachineID & remote_id) { assert(0); };   //Called by Sequencer
  virtual void notifyReceiveNack( int procID, const Address & addr, uint64 remote_timestamp, const MachineID & remote_id) { assert(0); };   //Called by Sequencer
  virtual void notifyReceiveNackFinal( int procID, const Address & addr) { assert(0); }; // Called by Sequencer
  virtual void notifyTrapStart( int procID, const Address & handlerPC, int threadID, int smtThread ) { assert(0); } //called by Sequencer
  virtual void notifyTrapComplete( int procID, const Address & newPC, int smtThread ) {assert(0);  }  // called by Sequencer
  virtual int getOpalTransactionLevel(int procID, int thread) const { 
    cout << "Driver.h getOpalTransactionLevel() " << endl;
   return 0; }  //called by Sequencer
  virtual void addThreadDependency(int procID, int requestor_thread, int conflict_thread) const { assert(0);}
  virtual uint64 getOpalTime(int procID) const{ return 0; }  //called by Sequencer
  virtual uint64 getOpalTimestamp(int procID, int thread) const{ 
    cout << "Driver.h getOpalTimestamp " << endl;
 return 0; } // called by Sequencer
  virtual int inTransaction(int procID, int thread ) const{ 
    cout << "Driver.h inTransaction " << endl;
return false; } //called by Sequencer
  virtual void printDebug(){}  //called by Sequencer

  virtual void printStats(ostream& out) const = 0;
  virtual void clearStats() = 0;

  virtual void printConfig(ostream& out) const = 0;

  //virtual void abortCallback(NodeID proc){}

  virtual integer_t readPhysicalMemory(int procID, physical_address_t address,
                                       int len ){ ASSERT(0); return 0; }
  
  virtual void writePhysicalMemory( int procID, physical_address_t address,
                                    integer_t value, int len ){ ASSERT(0); }

protected:
  // accessible by subclasses

private:
  // inaccessible by subclasses

};

#endif //DRIVER_H
