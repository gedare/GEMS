
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
 * Description: Common base class for a machine chip.
 *
 */

#ifndef ABSTRACT_CHIP_H
#define ABSTRACT_CHIP_H

#include "Global.h"
#include "NodeID.h"
#include "RubyConfig.h"
#include "L1Cache_Entry.h"
#include "Address.h"
#include "Vector.h"

class Network;
class Sequencer;
class StoreBuffer;
class ENTRY;
class MessageBuffer;
class CacheRecorder;
class TransactionInterfaceManager;

template<class ENTRY> class CacheMemory;

class AbstractChip {
public:
  // Constructors
  AbstractChip(NodeID chip_number, Network* net_ptr);

  // Destructor, prevent from being instantiated
  virtual ~AbstractChip() = 0;
  
  // Public Methods
  NodeID getID() const { return m_id; };
  Network* getNetwork() const { return m_net_ptr; };
  Sequencer* getSequencer(int index) const { return m_L1Cache_sequencer_vec[index]; };
  TransactionInterfaceManager* getTransactionInterfaceManager(int index) const { return m_L1Cache_xact_mgr_vec[index]; };
  void setTransactionInterfaceManager(TransactionInterfaceManager* manager, int index) { m_L1Cache_xact_mgr_vec[index] = manager; }

  // used when CHECK_COHERENCE is enabled.  See System::checkGlobalCoherence()
  virtual bool isBlockExclusive(const Address& addr) const { return false; }
  virtual bool isBlockShared(const Address& addr) const { return false; }

  // cache dump functions
  virtual void recordCacheContents(CacheRecorder& tr) const = 0;
  virtual void dumpCaches(ostream& out) const = 0;
  virtual void dumpCacheData(ostream& out) const = 0;

  virtual void printConfig(ostream& out) = 0;
  virtual void print(ostream& out) const = 0;

  // pulic data structures  
  Vector < CacheMemory<L1Cache_Entry>* > m_L1Cache_L1DcacheMemory_vec;
  Vector < CacheMemory<L1Cache_Entry>* > m_L1Cache_L1IcacheMemory_vec;
  Vector < CacheMemory<L1Cache_Entry>* > m_L1Cache_cacheMemory_vec;
  Vector < CacheMemory<L1Cache_Entry>* > m_L1Cache_L2cacheMemory_vec;
  Vector < CacheMemory<L1Cache_Entry>* > m_L2Cache_L2cacheMemory_vec;

  // added so that the prefetcher and sequencer can access the L1 and L2 request queues
  Vector < MessageBuffer* > m_L1Cache_optionalQueue_vec;
  Vector < MessageBuffer* >m_L1Cache_mandatoryQueue_vec;

  // TSO storebuffer
  Vector < StoreBuffer* > m_L1Cache_storeBuffer_vec;

  // TM transaction manager
  Vector < TransactionInterfaceManager* >  m_L1Cache_xact_mgr_vec;

protected:
  
  // Data Members (m_ prefix)
  NodeID                 m_id;            // Chip id
  Network*               m_net_ptr;       // Points to the Network simulator
  Vector < Sequencer* >  m_L1Cache_sequencer_vec; // All chip should have a sequencer
  

};

// Output operator declaration
ostream& operator<<(ostream& out, const AbstractChip& obj);

// ******************* Definitions *******************

// Output operator definition
extern inline 
ostream& operator<<(ostream& out, const AbstractChip& obj)
{
  obj.print(out);
  out << flush;
  return out;
}

#endif //ABSTRACT_CHIP_H

