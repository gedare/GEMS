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
   This file has been modified by Kevin Moore and Dan Nussbaum of the
   Scalable Systems Research Group at Sun Microsystems Laboratories
   (http://research.sun.com/scalable/) to support the Adaptive
   Transactional Memory Test Platform (ATMTP).

   Please send email to atmtp-interest@sun.com with feedback, questions, or
   to request future announcements about ATMTP.

   ----------------------------------------------------------------------

   File modification date: 2008-02-23

   ----------------------------------------------------------------------
*/

#include "System.h"
#include "SubBlock.h"
#include "Chip.h"
#include "LazyTransactionVersionManager.h"
#include "TransactionInterfaceManager.h"
#include "RubyConfig.h"
#include "CacheRequestType.h"
#include "interface.h"
#include "Driver.h"
#include "Address.h"

#include <assert.h>
#include <stdlib.h>

#define XACT_MGR g_system_ptr->getChip(m_chip_ptr->getID())->getTransactionInterfaceManager(m_version)
#define SEQUENCER g_system_ptr->getChip(m_chip_ptr->getID())->getSequencer(m_version)

LazyTransactionVersionManager::LazyTransactionVersionManager(AbstractChip* chip_ptr, int version) {
  m_chip_ptr = chip_ptr;
  m_version = version;
  int smt_threads = RubyConfig::numberofSMTThreads();
  
 
  m_registers.setSize(smt_threads);
  m_writeBuffer.setSize(smt_threads);
  m_writeBufferBlocks.setSize(smt_threads);
  
  if (ATMTP_ENABLED) {
    m_writeBufferSizes.setSize(smt_threads);
  }

  for(int i = 0; i < smt_threads; i++){
    m_registers[i].setSize(1);      
    m_registers[i][0] = new RegisterState(getProcID() * RubyConfig::numberofSMTThreads() + i);
    m_writeBuffer[i].setSize(1);
    m_writeBufferBlocks[i].setSize(1);
  }
  
}

LazyTransactionVersionManager::~LazyTransactionVersionManager() {
}

int LazyTransactionVersionManager::getProcID() const{
  return m_chip_ptr->getID()*RubyConfig::numberOfProcsPerChip()+m_version;
}

int LazyTransactionVersionManager::getLogicalProcID(int thread) const{
  return getProcID()+thread;
}

bool LazyTransactionVersionManager::existInWriteBuffer(int thread, physical_address_t addr){
  int xact_level = XACT_MGR->getTransactionLevel(thread);
  if (xact_level > 1)
    xact_level     = 1;
  assert(xact_level >= 0);
  assert(xact_level <= m_writeBuffer[thread].size());
  
  bool found = false;
  for (int i = 0; i < xact_level; i++){
    if (m_writeBuffer[thread][i].exist(addr)){
      found = true;
      break;
    }
  }  
  return found;
}      

uint8 LazyTransactionVersionManager::getDataFromWriteBuffer(int thread, physical_address_t addr){
  int xact_level = XACT_MGR->getTransactionLevel(thread);
  if (xact_level > 1)
    xact_level     = 1;
  assert(xact_level >= 0);
  assert(xact_level <= m_writeBuffer[thread].size());
  
  uint8 data;
  bool found = false;
  for (int i = xact_level; i > 0; i--){
    if (m_writeBuffer[thread][i - 1].exist(addr)){
      found = true;
      data  = m_writeBuffer[thread][i - 1].lookup(addr);
      break;
    }  
  }
  assert(found);
  return data;
}  
          
void LazyTransactionVersionManager::addToWriteBuffer(int thread, int transactionLevel, Address physical_address, int size, Vector<uint8> data){
  if (transactionLevel > 1)
    transactionLevel     = 1;
  assert(transactionLevel > 0);
  assert(transactionLevel <= m_writeBuffer[thread].size());
  assert(size == data.size());
  
  if (XACT_DEBUG && XACT_DEBUG_LEVEL > 2){
    cout << " " << g_eventQueue_ptr->getTime() << " " << getLogicalProcID(thread) << " " << "[" << getProcID() << "," << thread << "]" << " ADDING TO WRITE BUFFER " << Address(physical_address) << " SIZE: " << size  << " [ "; 
    for (int i = 0; i < data.size(); i++)
      cout << "0x" << hex << (uinteger_t)data[i] << dec << " ";
    cout << " ] " << endl;
  }  
  // NO PARTIAL ROLLBACK SUPPORT
  physical_address_t addr = physical_address.getAddress();
  // Increment the modeled buffer size here, without doing the
  // store-coalescing that is performed below in the writeBuffer
  // logic.  Rock only coalesces consecutive stores to the same
  // double-word -- ATMTP doesn't model even that.
  //
  if (ATMTP_ENABLED) {
    m_writeBufferSizes[thread]++;
    if (m_writeBufferSizes[thread] >= ATMTP_XACT_MAX_STORES) {
      XACT_MGR->setAbortFlag(thread, physical_address);
      XACT_MGR->setCPS_size(thread);
      int id = getProcID();
      Address pc = SIMICS_get_program_counter(id);
      g_system_ptr->getProfiler()->profileTransactionLogOverflow(id, physical_address, pc);
    }
  }
  
  for (int i = 0; i < size; i++){
    if (m_writeBuffer[thread][transactionLevel - 1].exist(addr + i))
      m_writeBuffer[thread][transactionLevel - 1].lookup(addr + i) = data[i];
    else {
      Address address = Address(addr + i);
      address.makeLineAddress();      
      m_writeBuffer[thread][transactionLevel - 1].add(addr + i, data[i]);

      if(! m_writeBufferBlocks[thread][transactionLevel - 1].exist(address.getAddress()))
        m_writeBufferBlocks[thread][transactionLevel - 1].add(address.getAddress(), 0);    }     
  }   
}

Vector<uint8> LazyTransactionVersionManager::forwardData(int thread, int transactionLevel, Address physical_address, int size){
  if (transactionLevel > 1)
    transactionLevel = 1;              
  assert(transactionLevel > 0);
  assert(transactionLevel <= m_writeBuffer[thread].size());

  bool forwarding = false;
  
  Vector<uint8> data;
  data.setSize(size);
  physical_address_t addr = physical_address.getAddress();
  for (int i = 0; i < size; i++){
    if (existInWriteBuffer(thread, addr + i)){
      data[i] = getDataFromWriteBuffer(thread, addr + i);
      forwarding = true;
      if (XACT_DEBUG && XACT_DEBUG_LEVEL > 2){
        cout << " " << g_eventQueue_ptr->getTime() << " " << getLogicalProcID(thread) << " " << "[" << getProcID() << "," << thread << "]" << " FORWARD FROM WRITE BUFFER " << Address(addr + i) << " 0x" << hex << (uinteger_t)data[i] << dec << endl; 
      }
    } else
      data[i] = SIMICS_read_physical_memory(getLogicalProcID(thread), addr + i, 1);
  }
  if (XACT_DEBUG && XACT_DEBUG_LEVEL > 2){
    cout << " " << g_eventQueue_ptr->getTime() << " " << getLogicalProcID(thread) << " " << "[" << getProcID() << "," << thread << "]" << " FROM WRITE BUFFER " << physical_address << " [ " ; 
    for (int i = 0; i < data.size(); i++){
      cout << "0x" << hex << (uinteger_t)data[i] << dec << " ";
    }
    cout << " ] " << forwarding << endl;
  }            
  return data;                            
}

void LazyTransactionVersionManager::flushWriteBuffer(int thread, int transactionLevel){
  if (transactionLevel > 1)
    return;
  assert(transactionLevel > 0);
  assert(transactionLevel <= m_writeBuffer[thread].size());
  
  if (XACT_DEBUG && XACT_DEBUG_LEVEL > 0){
    cout << " " << g_eventQueue_ptr->getTime() << " " << getLogicalProcID(thread) << " " << "[" << getProcID() << "," << thread << "]" << " FLUSHING WRITE BUFFER " << " PC " << SIMICS_get_program_counter(getLogicalProcID(thread)) << endl; 
  }
  
  if (!XACT_ENABLE_TOURMALINE) {
    issueWriteRequests(thread);
  } else {
    Vector<physical_address_t> addr = m_writeBuffer[thread][0].keys();
    for (int i = 0 ; i < addr.size(); i++){
      if (XACT_MGR->existGlobalStoreConflict(thread, Address(addr[i]))){
        assert(0);
      } else {          
        uint8 val = m_writeBuffer[thread][0].lookup(addr[i]);      
        SIMICS_write_physical_memory(getLogicalProcID(thread), addr[i], val, 1);
      }  
    }
    
    for (int i = 0 ; i < addr.size(); i++){
      if (XACT_DEBUG && XACT_DEBUG_LEVEL > 0 && (addr[i] % 4 == 0)){
        cout << " " << g_eventQueue_ptr->getTime() << " " << getLogicalProcID(thread) << " " << "[" << getProcID() << "," << thread << "]" << " WRITE BUFFER " << Address(addr[i]) << " NEW_VAL: " << hex << SIMICS_read_physical_memory(getLogicalProcID(thread), addr[i], 4) << dec << endl; 
      }
    }
    
    m_writeBuffer[thread][0].clear();
    m_writeBufferBlocks[thread][0].clear();
    XACT_MGR->commitLazyTransaction(thread);
    if (!SIMICS_processor_enabled(getLogicalProcID(thread)))
      SIMICS_enable_processor(getLogicalProcID(thread));
  }                 
}  

void LazyTransactionVersionManager::issueWriteRequests(int thread){
  Vector<physical_address_t> addresses = m_writeBufferBlocks[thread][0].keys();
  if (addresses.size() > 0){
    physical_address_t request_address = addresses[0];
    
    // EL systems can have store requests NACKed due to false conflicts in signatures, so should make them escaped
    bool should_escape = false;
    if(XACT_EAGER_CD && XACT_LAZY_VM){
      should_escape = true;
    }

    Address ad(request_address);
    CacheMsg storeMsg(ad,
                      ad,
                      CacheRequestType_ST_XACT,    
                      SIMICS_get_program_counter(getLogicalProcID(thread)),
                      AccessModeType_UserMode,
                      g_DATA_BLOCK_BYTES,
                      PrefetchBit_No,
                      0,
                      Address(0),
                      thread,
                      g_eventQueue_ptr->getTime(),
                      should_escape
                      );
    SEQUENCER->makeRequest(storeMsg);
    m_writeBufferBlocks[thread][0].erase(request_address);
    m_pendingWriteBufferRequest = Address(request_address);
    if (XACT_DEBUG && XACT_DEBUG_LEVEL > 0){
      cout << " " << g_eventQueue_ptr->getTime() << " " << getLogicalProcID(thread) << " " << "[" << getProcID() << "," << thread << "]" << " WRITE BUFFER ISSUE REQUEST " << Address(request_address) << endl; 
    }
    if (SIMICS_processor_enabled(getLogicalProcID(thread))){
      if (XACT_DEBUG && XACT_DEBUG_LEVEL > 0){
        cout << " " << g_eventQueue_ptr->getTime() << " " << getLogicalProcID(thread) << " " << "[" << getProcID() << "," << thread << "]" << " DISABLE PROCESSOR " << Address(request_address) << endl; 
      }
      SIMICS_post_disable_processor(getLogicalProcID(thread)); 
    }
  } else {
    Vector<physical_address_t> addr = m_writeBuffer[thread][0].keys();

    for (int i = 0 ; i < addr.size(); i++){
      uint8 val = m_writeBuffer[thread][0].lookup(addr[i]);      
      SIMICS_write_physical_memory(getLogicalProcID(thread), addr[i], val, 1);
    }
    for (int i = 0 ; i < addr.size(); i++){
      if (XACT_DEBUG && XACT_DEBUG_LEVEL > 0 && (addr[i] % 4 == 0)){
        cout << " " << g_eventQueue_ptr->getTime() << " " << getLogicalProcID(thread) << " " << "[" << getProcID() << "," << thread << "]" << " WRITE BUFFER " << Address(addr[i]) << " NEW_VAL: " << hex << SIMICS_read_physical_memory(getLogicalProcID(thread), addr[i], 4) << dec << endl; 
      }
    }
    m_writeBuffer[thread][0].clear();
    m_writeBufferBlocks[thread][0].clear();
    XACT_MGR->commitLazyTransaction(thread);
    if (!SIMICS_processor_enabled(getLogicalProcID(thread)))
      SIMICS_enable_processor(getLogicalProcID(thread));
  }                 
}  
                       

void LazyTransactionVersionManager::hitCallback(int thread, Address physicalAddress){
  if (physicalAddress != m_pendingWriteBufferRequest){
     cerr << " " << g_eventQueue_ptr->getTime() << " " << getLogicalProcID(thread) << " " << "[" << getProcID() << "," << thread << "]" << " WRITE BUFFER HIT CALLBACK " << physicalAddress << " pendingRequest " << m_pendingWriteBufferRequest << " ********* WARNING***************" << endl;
     return;
  }     
  issueWriteRequests(thread);            
}          

void LazyTransactionVersionManager::discardWriteBuffer(int thread, int transactionLevel){
  if (transactionLevel > 1)
    transactionLevel = 1;
  assert(transactionLevel > 0);
  assert(transactionLevel <= m_writeBuffer[thread].size());
  
  m_writeBuffer[thread][transactionLevel - 1].clear();
  m_writeBufferBlocks[thread][transactionLevel - 1].clear();
  if (XACT_DEBUG && XACT_DEBUG_LEVEL > 0){
    cout << " " << g_eventQueue_ptr->getTime() << " " << getLogicalProcID(thread) << " " << "[" << getProcID() << "," << thread << "]" << " DISCARDING WRITE BUFFER " << endl; 
  }
}                    

void LazyTransactionVersionManager::takeCheckpoint(int thread){
  int logical_proc_no = getProcID() * RubyConfig::numberofSMTThreads() + thread;      
  int transactionLevel = XACT_MGR->getTransactionLevel(thread);        
  assert(transactionLevel >= 1);

  if (transactionLevel > 1)
    return;        
  
  int temp_size =  m_registers[thread].size();
  assert(temp_size > 0);
  while (temp_size < transactionLevel){
    m_registers[thread].expand(1);
    temp_size = m_registers[thread].size();
    m_registers[thread][temp_size - 1] = new RegisterState(logical_proc_no);
  }
  assert(temp_size >= transactionLevel);
  m_registers[thread][transactionLevel - 1]->takeCheckpoint(logical_proc_no);
}             

void LazyTransactionVersionManager::beginTransaction(int thread){
  takeCheckpoint(thread);
  
  if (ATMTP_ENABLED) {
    int xact_level = XACT_MGR->getTransactionLevel(thread);
    if (xact_level == 1){
      int logical_proc_no = getProcID() * RubyConfig::numberofSMTThreads() + thread; 
      m_registers[thread][0]->disableInterrupts(logical_proc_no);
      m_writeBufferSizes[thread] = 0;
    }
  }
}

void LazyTransactionVersionManager::commitTransaction(int thread, bool isOpen){ 
  int xact_level = XACT_MGR->getTransactionLevel(thread);      
  assert(!isOpen || (xact_level == 1));
  if (isOpen)
    flushWriteBuffer(thread, XACT_MGR->getTransactionLevel(thread));
 
  if (ATMTP_ENABLED && xact_level == 1) {
    int logical_proc_no = getProcID() * RubyConfig::numberofSMTThreads() + thread; 
    m_registers[thread][0]->enableInterrupts(logical_proc_no);
  }
}                   

void LazyTransactionVersionManager::restartTransaction(int thread, int xact_level){
  assert(xact_level == 0);
  
  discardWriteBuffer(thread, xact_level + 1);        
  m_registers[thread][xact_level]->restoreCheckpoint(getLogicalProcID(thread));

  if (ATMTP_ENABLED) {
    m_registers[thread][xact_level]->enableInterrupts(getLogicalProcID(thread));
  }

  if(!ATMTP_ENABLED){
    // In Rock and ATMTP, the PC is not restored to the begining of
    // the transaction, but set to a software-specified value (the
    // fail pc).  This is done when TransactionInterfaceManager calls
    // RockTransactionManager::restoreFailPC.
    //
    m_registers[thread][xact_level]->restorePC(getLogicalProcID(thread));
  }
}  
