
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

#ifndef TRANSACTIONSIMICSPROCESSOR_H
#define TRANSACTIONSIMICSPROCESSOR_H

#include "Global.h"
#include "System.h"
#include "Profiler.h"
#include "Vector.h"
#include "Map.h"
#include "Address.h"
#include "Sequencer.h"

extern "C" {
#include "simics/api.h"
}                

#ifdef SIMICS30
#ifdef SPARC
typedef v9_memory_transaction_t memory_transaction_t;
#else
typedef x86_memory_transaction_t memory_transaction_t;
#endif
#endif

class TransactionSimicsProcessor {
public:
  TransactionSimicsProcessor(System* sys_ptr, int proc_no);
  ~TransactionSimicsProcessor();

	bool getMagicCrossCall();
	void setMagicCrossCall(bool value);
	TransactionInterfaceManager* getTransactionInterfaceManager();
	void setTransactionInterfaceManager(TransactionInterfaceManager* xact_mgr);
 
  unsigned int startingMemRef(memory_transaction_t *mem_trans, CacheRequestType type);
  void completedMemRef(bool success, CacheRequestType type, Address physicalAddress, Address logicalAddress);
  void readyToRetireMemRef(bool success, memory_transaction_t *mem_trans, CacheRequestType type);
  /* 0 - Retire, 1 - Trap, 2 - Retry */
  int  canRetireMemRef(memory_transaction_t *mem_trans, CacheRequestType type, bool success);
  void retiredMemRef(memory_transaction_t *mem_trans);
  void addToReadWriteSets(memory_transaction_t *mem_trans, CacheRequestType type);

  void updateStoreSetPredictor(CacheRequestType type, Address address);
  bool generateLogRequest(memory_transaction_t *mem_trans);
  
  void processLogStore(memory_transaction_t *mem_trans, CacheRequestType type);
  void processRubyWatchAddress(memory_transaction_t *mem_trans, CacheRequestType type);
  void xactIsolationCheck(memory_transaction_t *mem_trans, CacheRequestType type);
  void redirectStoreToWriteBuffer(memory_transaction_t *mem_trans);
  void addToReadWriteSets(CacheRequestType type, Address physicalAddress, Address logicalAddress);
  
  int computeFirstXactAccessCost(CacheRequestType type, Address physicalAddress);

  void bypassLoadFromWriteBuffer(memory_transaction_t *mem_trans);
  bool tourmalineExistGlobalConflict(CacheRequestType type, Address physicalAddress);

private:
  int m_proc;
  Sequencer *m_sequencer;
  TransactionInterfaceManager* m_xact_mgr;

  int     m_outstandingXactStoreRefCount;
   
  char m_oldValueBuffer[256];
  char m_newValueBuffer[256];
  uint  m_oldValueBufferSize;

	bool m_magic_cross_call;
};  
 
#endif



