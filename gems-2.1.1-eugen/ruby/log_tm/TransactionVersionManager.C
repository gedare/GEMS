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

#include "TransactionVersionManager.h"
#include "TransactionInterfaceManager.h"
#include "RubyConfig.h"
#include "CacheRequestType.h"
#include "interface.h"
#include "Driver.h"
#include "Address.h"

#include <assert.h>
#include <stdlib.h>

#define g_ADDRESS_SIZE 4
#define g_ENTRY_MASK 0x0000003f
#define g_ADDRESS_MASK 0xffffffc0

#define XACT_MGR g_system_ptr->getChip(m_chip_ptr->getID())->getTransactionInterfaceManager(m_version)

TransactionVersionManager::TransactionVersionManager(AbstractChip* chip_ptr, int version) {
  m_chip_ptr = chip_ptr;
  m_version = version;
  int smt_threads = RubyConfig::numberofSMTThreads();
 
  m_logSize           = new int[smt_threads];
  m_tid               = new int[smt_threads];
  m_numCommitActions  = new int[smt_threads];
  m_maxLogSize        = new int[smt_threads];
  
  m_logBase                 = new Address[smt_threads];
  m_logPointer              = new Address[smt_threads];
  m_logFramePointer         = new Address[smt_threads];
  m_logCommitActionBase     = new Address[smt_threads];
  m_logCommitActionPointer  = new Address[smt_threads];
  m_tm_contextAddress       = new Address[smt_threads];

  m_logPointer_physical         = new Address[smt_threads];
  m_logFramePointer_physical    = new Address[smt_threads];

  m_logAddressFilter_ptr = new Map<Address, Address>*[smt_threads];
  m_logAddressTLB        = new Map<physical_address_t, physical_address_t>;
  m_registers.setSize(smt_threads);
  
  for(int i = 0; i < smt_threads; i++){
    m_logAddressFilter_ptr[i] = new Map<Address, Address>;
    m_registers[i].setSize(1);
    m_registers[i][0] = new RegisterState(getProcID() * RubyConfig::numberofSMTThreads() + i);

    m_logSize[i]          = 0;
    m_tid[i]              = -1;
    m_numCommitActions[i] = 0;
    m_maxLogSize[i]       = 0;
  }
  
  m_logAddressTLB->clear();           

}

TransactionVersionManager::~TransactionVersionManager() {
}

void TransactionVersionManager::setVersion(int version) {
  m_version = version;
}

int TransactionVersionManager::getVersion() const {
  return m_version;
}

int TransactionVersionManager::getProcID() const{
  return m_chip_ptr->getID()*RubyConfig::numberOfProcsPerChip()+m_version;
}

// This computes the logical Simics processor number (for SMT systems)
int TransactionVersionManager::getLogicalProcID(int thread) const{
  return getProcID()*RubyConfig::numberofSMTThreads() + thread;
}

physical_address_t TransactionVersionManager::getDataTranslation(physical_address_t virtual_address, int thread) {
   int proc_no = getProcID() * RubyConfig::numberofSMTThreads() + thread;
   int page_offset = virtual_address % g_PAGE_SIZE_BYTES;
   physical_address_t virtual_page = virtual_address - page_offset;
   
   physical_address_t physical_address;
   
   if (!m_logAddressTLB->exist(virtual_page)){     
     physical_address_t temp_address = SIMICS_translate_data_address( proc_no, Address(virtual_page));
     if (temp_address == 0){
        /* DATA TRANSLATION FAILED */
        /* GENERATE XACT TRAP TO SW */
        XACT_MGR->setTrapTLBMiss(thread, Address(virtual_page));
        if (XACT_DEBUG){
          cout << " " << g_eventQueue_ptr->getTime() << " " << getLogicalProcID(thread) << " [" << getProcID() << "," << thread << "] SET TRAP TLB MISS " << Address(virtual_page) << " PC = " << SIMICS_get_program_counter(getLogicalProcID(thread)) << endl;        
        }
        return 0;  
     } else {
        m_logAddressTLB->add(virtual_page, temp_address);
        physical_address = temp_address + page_offset;         
     }
   } else {
      physical_address = m_logAddressTLB->lookup(virtual_page) + page_offset;
   }
   return physical_address;     
}

bool TransactionVersionManager::addLogEntry(int thread, int log_entry_size, Vector<uint32>& entry){
  int logical_proc_no = getLogicalProcID(thread);
  physical_address_t logPointer = m_logPointer[thread].getAddress();
  physical_address_t logPhysicalAddress = m_logPointer_physical[thread].getAddress();
  assert(log_entry_size % 4 == 0);
  
  if (logPointer / g_PAGE_SIZE_BYTES == (logPointer + log_entry_size) / g_PAGE_SIZE_BYTES){
    for (int i = 0; i < log_entry_size; i += 4)
      g_system_ptr->getDriver()->writePhysicalMemory(logical_proc_no, logPhysicalAddress + i, entry[i/4], 4);
    m_logPointer_physical[thread] = Address(logPhysicalAddress + log_entry_size);  
  } else {
    int num_bytes_old_page = g_PAGE_SIZE_BYTES - (logPointer % g_PAGE_SIZE_BYTES);
    assert(num_bytes_old_page <= log_entry_size);

   // sanity check
    if((num_bytes_old_page % 4) != 0){
     cout << "[ " << getProcID() << " ] addLogEntry ERROR 1 num bytes not multiple of 4! page_size = " << g_PAGE_SIZE_BYTES << " log_entry_size = " << log_entry_size << " log_pointer = " << m_logPointer[thread] << " thread = " << thread << " num_bytes_remaining = " << num_bytes_old_page << " bytes_written = " << logPointer % g_PAGE_SIZE_BYTES << " time = " << g_eventQueue_ptr->getTime() << endl;
    }

    assert(num_bytes_old_page % 4 == 0);
    
    physical_address_t newPhysicalAddress = getDataTranslation(logPointer + num_bytes_old_page, thread);
    if (newPhysicalAddress == 0)
      return false;
    for (int i = 0; i < num_bytes_old_page; i += 4)
      g_system_ptr->getDriver()->writePhysicalMemory(logical_proc_no, logPhysicalAddress+ i, entry[i/4], 4);
    for (int i = num_bytes_old_page; i < log_entry_size; i += 4) 
      g_system_ptr->getDriver()->writePhysicalMemory(logical_proc_no, newPhysicalAddress + (i - num_bytes_old_page), entry[i/4], 4);
    m_logPointer_physical[thread] = Address(newPhysicalAddress + (log_entry_size - num_bytes_old_page));  
   }      
   
   m_logPointer[thread] = Address(logPointer + log_entry_size);
   m_logSize[thread] += log_entry_size;
   assert(m_logSize[thread] < (m_maxLogSize[thread] * 1024));
   // sanity check
   int num_bytes_page = g_PAGE_SIZE_BYTES - (m_logPointer[thread].getAddress() % g_PAGE_SIZE_BYTES);
   if((num_bytes_page % 4) != 0){
     cout << "[ " << getProcID() << " ] addLogEntry ERROR 2 num bytes not multiple of 4! page_size = " << g_PAGE_SIZE_BYTES << " log_entry_size = " << log_entry_size << " log_pointer = " << m_logPointer[thread] << " thread = " << thread << " num_bytes_remaining = " << num_bytes_page << " bytes_written = " << m_logPointer[thread].getAddress() % g_PAGE_SIZE_BYTES << " time = " << g_eventQueue_ptr->getTime() << endl;
   }
   assert(num_bytes_page % 4 == 0);
   
   return true;             
}        

void TransactionVersionManager::takeCheckpoint(int thread){
  int logical_proc_no = getProcID() * RubyConfig::numberofSMTThreads() + thread;      
  int transactionLevel = XACT_MGR->getTransactionLevel(thread);        
  assert(transactionLevel >= 1);
  
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

bool TransactionVersionManager::addNewLogFrame(int thread){ 
  int proc_no = getProcID()*RubyConfig::numberofSMTThreads() + thread;
  Vector<uint32> entry;
  
  /* Add transaction header to the software log */
  int logEntrySize = g_DATA_BLOCK_BYTES + g_ADDRESS_SIZE;
  entry.setSize(logEntrySize/4);
  int offset = g_DATA_BLOCK_BYTES/4;
  entry[offset] = 0x1;
  entry[0]      = m_logFramePointer[thread].getAddress();

  Address newFramePointer          = m_logPointer[thread];
  Address newFramePointer_physical = m_logPointer_physical[thread];

  bool result = addLogEntry(thread, logEntrySize, entry);

  if (!result){
    XACT_MGR->takeContinueCheckpoint(thread);   
  } else {
    m_logFramePointer[thread] = newFramePointer;
    m_logFramePointer_physical[thread] = newFramePointer_physical;
  }  

  if (XACT_DEBUG && XACT_DEBUG_LEVEL > 1){
    cout << " " << g_eventQueue_ptr->getTime() << " " << getLogicalProcID(thread) << " [" << getProcID() << "," << thread << "] ADD XACT FRAME oldLogFramePointer: " 
         << Address(g_system_ptr->getDriver()->readPhysicalMemory(proc_no, m_logFramePointer_physical[thread].getAddress(), 4))
         << " newLogFramePointer: "  << m_logFramePointer[thread] << " " << result <<endl;       
  }
  return result;       
}
            
bool TransactionVersionManager::beginTransaction(int thread){
  bool result = addNewLogFrame(thread);   
  if (result){
    takeCheckpoint(thread);
    m_logAddressFilter_ptr[thread]->clear();
    int xact_level = XACT_MGR->getTransactionLevel(thread);
    if (xact_level == 1)
      m_numCommitActions[thread] = 0;
  }
  return result;  
}

void TransactionVersionManager::commitTransaction(int thread, bool isOpen){
  int logical_proc_no = getLogicalProcID(thread);
        
  if (isOpen){
    m_logPointer[thread] = m_logFramePointer[thread];
    m_logPointer_physical[thread] = m_logFramePointer_physical[thread];      
    m_logSize[thread]    = m_logPointer[thread].getAddress() - m_logBase[thread].getAddress();
    assert(m_logSize[thread] < (m_maxLogSize[thread] * 1024));
  }  
                  
  int offset = g_DATA_BLOCK_BYTES;
  g_system_ptr->getDriver()->writePhysicalMemory(logical_proc_no, m_logFramePointer_physical[thread].getAddress() + offset, 0x2, 4);
  physical_address_t oldFramePointer = g_system_ptr->getDriver()->readPhysicalMemory(logical_proc_no, m_logFramePointer_physical[thread].getAddress(), 4);
  
  if (XACT_DEBUG && XACT_DEBUG_LEVEL > 1){
    cout << " " << g_eventQueue_ptr->getTime() << " " << getLogicalProcID(thread) << " [" << getProcID() << "," << thread << "] COMMIT SET XACT FRAME oldLogFramePointer: " 
         << m_logFramePointer[thread]
         << " newLogFramePointer: "  << Address(oldFramePointer) << endl;       
  }
  
  m_logFramePointer[thread] = Address(oldFramePointer);
  m_logFramePointer_physical[thread] = Address(getDataTranslation(oldFramePointer, thread));

  if (ATMTP_ENABLED && XACT_MGR->getTransactionLevel(thread) == 1) {
    m_registers[thread][0]->enableInterrupts(logical_proc_no);
  }
}

void TransactionVersionManager::restartTransaction(int thread, int xact_level){
  int logical_proc_no = getLogicalProcID(thread);      
  assert(xact_level >= 0);
        
  m_registers[thread][xact_level]->restoreCheckpoint(logical_proc_no);
  m_registers[thread][xact_level]->restorePC(logical_proc_no);
  
  updateLogPointers(thread);        
}        

void TransactionVersionManager::setRestorePC(int thread, int xact_level, unsigned int pc) {
  assert(xact_level >= 0);
  m_registers[thread][xact_level]->setPC(pc);
}
        
void TransactionVersionManager::setLogBase(int thread){
  assert(thread >= 0);
  int proc_no = getLogicalProcID(thread);
  
  int rn_g3 = SIMICS_get_register_number(proc_no, "g3");
  physical_address_t tm_context_address = SIMICS_read_register(proc_no, rn_g3);
  int rn_g4 = SIMICS_get_register_number(proc_no, "g4");
  physical_address_t tm_log_base = SIMICS_read_register(proc_no, rn_g4);
  int rn_g5 = SIMICS_get_register_number(proc_no, "g5");
  m_maxLogSize[thread] = (int) SIMICS_read_register(proc_no, rn_g5);

  m_logBase[thread] = Address(tm_log_base);
  m_logSize[thread] = 0;
  m_logPointer[thread] = m_logBase[thread];
  m_tm_contextAddress[thread] = Address(tm_context_address);
  m_logPointer_physical[thread] = Address(getDataTranslation(m_logPointer[thread].getAddress(), thread));
  if(m_logPointer_physical[thread] == Address(0)){
    if(XACT_DEBUG && XACT_DEBUG_LEVEL > 0) {
      cout << proc_no << " TLB MISS for Virtual Log Pointer " << m_logPointer[thread] << " thread = " << thread << endl;
    }
    assert(0);
  }
  m_logFramePointer[thread] = Address(g_system_ptr->getDriver()->readPhysicalMemory(proc_no, m_logPointer_physical[thread].getAddress() + s_softwareXactStructMap[XACT_FRAME_POINTER].offset, 4));
  m_logFramePointer_physical[thread] = Address(getDataTranslation(m_logFramePointer[thread].getAddress(), thread));

  m_tid[thread] = g_system_ptr->getDriver()->readPhysicalMemory(proc_no, m_logPointer_physical[thread].getAddress() + s_softwareXactStructMap[THREADID].offset, 4);
  XACT_MGR->setTID(thread, m_tid[thread]);
  
  if (XACT_DEBUG && XACT_DEBUG_LEVEL > 0){
    cout << proc_no << " [" << getProcID() << "," << thread << "] CONTEXT_ADDRESS: " << m_tm_contextAddress[thread] << " SET LOG BASE: " << m_logBase[thread] << " " << " LOG POINTER: " << m_logPointer[thread] << " => " << m_logPointer_physical[thread] << " LOG FRAME POINTER: " << m_logFramePointer[thread] << " MAX LOG SIZE: " << m_maxLogSize[thread] << "KB" 
         << endl; 
  }
  
  //m_logFramePointer[thread] = m_logPointer[thread];
  //m_logFramePointer_physical[thread] = m_logPointer_physical[thread];
  
}

void TransactionVersionManager::updateLogPointers(int thread){
  assert(thread >= 0);
  int logical_proc_no = getProcID()*RubyConfig::numberofSMTThreads() + thread;
  physical_address_t tm_log_base = getLogBasePhysical(thread).getAddress();
 
  m_logSize[thread] = g_system_ptr->getDriver()->readPhysicalMemory(logical_proc_no, tm_log_base + s_softwareXactStructMap[XACT_LOG_SIZE].offset, 4); 
  assert(m_logSize[thread] >= 0);
  assert(m_logSize[thread] < (m_maxLogSize[thread] * 1024));
  m_logPointer[thread] = Address(getLogBase(thread).getAddress() + m_logSize[thread]);
  
  physical_address_t tm_log_frame_pointer = g_system_ptr->getDriver()->readPhysicalMemory(logical_proc_no, tm_log_base + s_softwareXactStructMap[XACT_FRAME_POINTER].offset, 4);
  m_logFramePointer[thread] = Address(tm_log_frame_pointer);

  m_logPointer_physical[thread] = Address(getDataTranslation(m_logPointer[thread].getAddress(), thread));
  m_logFramePointer_physical[thread] = Address(getDataTranslation(m_logFramePointer[thread].getAddress(), thread)); 
  
  if (XACT_DEBUG && XACT_DEBUG_LEVEL>1){
    cout << logical_proc_no << " [" << getProcID() << "," << thread << "] SET LOG POINTER: " << m_logPointer[thread] << " LOGFRAME POINTER: " << m_logFramePointer[thread] << " LOG SIZE: " << m_logSize[thread] << endl;
  }
  //m_logFramePointer[thread] = m_logBase[thread]; // TEMPORARY
}

  
Address TransactionVersionManager::getLogPointerAddress(int thread){
  return m_logPointer[thread];        
}

Address TransactionVersionManager::getLogPointerAddressPhysical(int thread){
  return m_logPointer_physical[thread];        
}

Address TransactionVersionManager::getLogBase(int thread){
  return m_logBase[thread];        
}

Address TransactionVersionManager::getLogBasePhysical(int thread){
  return Address(getDataTranslation(m_logBase[thread].getAddress(), thread));        
}

Address TransactionVersionManager::getLogFramePointer(int thread){
  return m_logFramePointer[thread];        
}

Address TransactionVersionManager::getLogCommitActionBase(int thread){
  return m_logCommitActionBase[thread];
}          

int TransactionVersionManager::getLogSize(int thread){
  return m_logSize[thread];        
}
                
int TransactionVersionManager::getNumCommitActions(int thread){
  return m_numCommitActions[thread];        
}
                
void TransactionVersionManager::addUndoLogEntry(int thread, Address logicalAddress, Address physicalAddress){
  Address logAddress = getLogPointerAddress(thread);
  int logEntrySize = g_DATA_BLOCK_BYTES + g_ADDRESS_SIZE;
  Vector<uint32> entry;
  entry.setSize(logEntrySize/4);

  logicalAddress.makeLineAddress();
  physicalAddress.makeLineAddress();

  physical_address_t logical_page = (logicalAddress.getAddress() / g_PAGE_SIZE_BYTES) * g_PAGE_SIZE_BYTES;
  physical_address_t physical_page = (physicalAddress.getAddress() / g_PAGE_SIZE_BYTES) * g_PAGE_SIZE_BYTES;

  int logical_proc_id = getProcID() * RubyConfig::numberofSMTThreads() + thread;
  for (int i = 0; i < g_DATA_BLOCK_BYTES; i += 4){
    entry[i/4] = g_system_ptr->getDriver()->readPhysicalMemory( logical_proc_id,
                                physicalAddress.getAddress() + i, 4);
  }
  entry[g_DATA_BLOCK_BYTES/4] = logicalAddress.getAddress() | 0x4;

  if (!m_logAddressTLB->exist(logical_page)){
    m_logAddressTLB->add(logical_page, physical_page);
  } else {
    if (m_logAddressTLB->lookup(logical_page) != physical_page){
      cout << " LOG TLB MISMATCH VA: " << logicalAddress << " PA: 0x" << hex << physical_page << dec << " TLB: 0x" << hex << m_logAddressTLB->lookup(logical_page)
          << dec << endl;
      // we need to get a new mapping
      m_logAddressTLB->erase(logical_page);
      XACT_MGR->setTrapTLBMiss(thread, Address(logical_page));
      // don't proceed any further
      //SIM_break_simulation("TLB MISMATCH");
      return;
      //assert(0);
    }
  }  
  
  bool result = addLogEntry(thread, logEntrySize, entry);
  if (result){ 
    if (!m_logAddressFilter_ptr[thread]->exist(logicalAddress)){
      m_logAddressFilter_ptr[thread]->add(logicalAddress, physicalAddress);
    }
  }    
  
    
  if (XACT_DEBUG && XACT_DEBUG_LEVEL > 1){
    cout << " " << g_eventQueue_ptr->getTime() << " " << logical_proc_id << " [" << getProcID() << "," << thread << "] ADD UNDO LOG ENTRY: " << logicalAddress << " "
         << physicalAddress << " LogAddress: " << logAddress << " " << result << endl; 
  }

}

void TransactionVersionManager::allocateLogEntry(int thread, Address address, int transactionLevel){
        assert(0);
}

void TransactionVersionManager::registerCompensatingAction(int thread){
  int proc_no = getProcID()*RubyConfig::numberofSMTThreads() + thread;
  int rn_g2 = SIMICS_get_register_number(proc_no, "g2");
  int rn_g3 = SIMICS_get_register_number(proc_no, "g3");
  int inputVector[6];
  uint32 pc = SIMICS_read_register(proc_no, rn_g2);
  uint32 num_inputs = SIMICS_read_register(proc_no, rn_g3);
  
  assert(num_inputs <= 6);
  switch (num_inputs){
    case 6:              
      inputVector[5] = SIMICS_read_register(proc_no, SIMICS_get_register_number(proc_no, "o5"));
    case 5:              
      inputVector[4] = SIMICS_read_register(proc_no, SIMICS_get_register_number(proc_no, "o4"));
    case 4:              
      inputVector[3] = SIMICS_read_register(proc_no, SIMICS_get_register_number(proc_no, "o3"));
    case 3:              
      inputVector[2] = SIMICS_read_register(proc_no, SIMICS_get_register_number(proc_no, "o2"));
    case 2:              
      inputVector[1] = SIMICS_read_register(proc_no, SIMICS_get_register_number(proc_no, "o1"));
    case 1:  
      inputVector[0] = SIMICS_read_register(proc_no, SIMICS_get_register_number(proc_no, "o0"));
    default:
      break;
  }

  if (XACT_DEBUG){
    cout << proc_no << " [" << getProcID() << "," << thread << "] COMPENSATING ACTION: PC 0x" << hex << pc << dec
         << " NUM_INP: " << num_inputs << " INP[0]: " << inputVector[0] << endl;
  }

  /* Add compensation action entry to the software log */
  Address logAddress = getLogPointerAddress(thread);
  int logEntrySize = g_DATA_BLOCK_BYTES + g_ADDRESS_SIZE;
  Vector<uint32> entry;
  entry.setSize(logEntrySize/4);
  
  int offset = g_DATA_BLOCK_BYTES;
  entry[offset/4] = 0x5;
  offset -= 4;
  entry[offset/4] = pc;
  offset -= 4;  
  entry[offset/4] = num_inputs;
  offset -= 4;
  for (uint32 i = 0; i < num_inputs; i++){
    entry[offset/4] = inputVector[i];
    offset -= 4;
  }
  bool result = addLogEntry(thread, logEntrySize, entry);  
  
  if (!result){
    XACT_MGR->takeContinueCheckpoint(thread);   
  }  
}            

void TransactionVersionManager::registerCommitAction(int thread){
  int proc_no = getProcID()*RubyConfig::numberofSMTThreads() + thread;
  int rn_g2 = SIMICS_get_register_number(proc_no, "g2");
  int rn_g3 = SIMICS_get_register_number(proc_no, "g3");
  int inputVector[6];
  uint32 pc = SIMICS_read_register(proc_no, rn_g2);
  uint32 num_inputs = SIMICS_read_register(proc_no, rn_g3);
  
  assert(num_inputs <= 6);
  switch (num_inputs){
    case 6:              
      inputVector[5] = SIMICS_read_register(proc_no, SIMICS_get_register_number(proc_no, "o5"));
    case 5:              
      inputVector[4] = SIMICS_read_register(proc_no, SIMICS_get_register_number(proc_no, "o4"));
    case 4:              
      inputVector[3] = SIMICS_read_register(proc_no, SIMICS_get_register_number(proc_no, "o3"));
    case 3:              
      inputVector[2] = SIMICS_read_register(proc_no, SIMICS_get_register_number(proc_no, "o2"));
    case 2:              
      inputVector[1] = SIMICS_read_register(proc_no, SIMICS_get_register_number(proc_no, "o1"));
    case 1:  
      inputVector[0] = SIMICS_read_register(proc_no, SIMICS_get_register_number(proc_no, "o0"));
    default:
      break;
  }
  
  /* Add compensation action entry to the software log */
  Address logAddress = getLogPointerAddress(thread);
  int logEntrySize = g_DATA_BLOCK_BYTES + g_ADDRESS_SIZE;
  Vector<uint32> entry;
  entry.setSize(logEntrySize/4);
  
  int offset = g_DATA_BLOCK_BYTES;
  entry[offset/4] = 0x6;
  offset -= 4;
  entry[offset/4] = pc;
  offset -= 4;  
  entry[offset/4] = num_inputs;
  offset -= 4;
  for (uint32 i = 0; i < num_inputs; i++){
    entry[offset/4] = inputVector[i];
    offset -= 4;
  }

  Address newCAptr = m_logPointer[thread];
  Address oldCAptr = m_logCommitActionPointer[thread];
  entry[0] = newCAptr.getAddress();
  bool result = addLogEntry(thread, logEntrySize, entry); 
  if (XACT_DEBUG){
    cout << proc_no << " [" << getProcID() << "," << thread << "] COMMIT ACTION: PC 0x" << hex << pc << dec << " NUM_INP: " << num_inputs << " INP[0]: " << inputVector[0] << " CA LIST -> " << m_logCommitActionPointer[thread] << " CA HEAD -> " << m_logCommitActionBase[thread] << " NUM_ENTRIES: " << m_numCommitActions[thread] << " " << newCAptr << endl;
  }
  if (!result){
    XACT_MGR->takeContinueCheckpoint(thread);   
  } else {
    m_logCommitActionPointer[thread] = newCAptr;
    m_numCommitActions[thread]++;
    physical_address_t prevCAEntry_physical;      
    if (m_numCommitActions[thread] == 1){
      m_logCommitActionBase[thread] = newCAptr;
      prevCAEntry_physical = getDataTranslation(newCAptr.getAddress(), thread);
    } else {
      prevCAEntry_physical = getDataTranslation(oldCAptr.getAddress(), thread);
    }                        
    assert(prevCAEntry_physical != 0);
    g_system_ptr->getDriver()->writePhysicalMemory(getLogicalProcID(thread), prevCAEntry_physical, newCAptr.getAddress(), 4);
  }            
  
  if (XACT_DEBUG){
    cout << proc_no << " [" << getProcID() << "," << thread << "] COMMIT ACTION: PC 0x" << hex << pc << dec << " NUM_INP: " << num_inputs << " INP[0]: " << inputVector[0] << " CA LIST -> " << m_logCommitActionPointer[thread] << " CA HEAD -> " << m_logCommitActionBase[thread] << " NUM_ENTRIES: " << m_numCommitActions[thread] << endl;
  }
}            

void TransactionVersionManager::xmalloc(int thread){
  int proc_no = getProcID()*RubyConfig::numberofSMTThreads() + thread;
  int rn_o0 = SIMICS_get_register_number(proc_no, "o0");
  uint32 size = SIMICS_read_register(proc_no, rn_o0);

  size = (size%4>0) ? (size/4+1)*4 : size;   
  
  if (XACT_DEBUG){
    cout << proc_no << " [" << getProcID() << "," << thread << "] XMALLOC" << dec
       << " SIZE: " << size << endl;
  }
  /* Allocate space in the log */
  Address old_logAddress = getLogPointerAddress(thread);
  incrementLogPointer(thread, size);
 
  /* Add xmalloc entry to the software log */
  Address logAddress = getLogPointerAddress(thread);
  int logEntrySize = g_DATA_BLOCK_BYTES + g_ADDRESS_SIZE;
  Vector<uint32> entry;
  entry.setSize(logEntrySize/4);

  int offset = g_DATA_BLOCK_BYTES;
  entry[offset/4] = 0x7;
  offset -= 4;
  entry[offset/4] = size;
  offset -= 4;
  bool result = addLogEntry(thread, logEntrySize, entry);

  SIMICS_write_register(proc_no, rn_o0, old_logAddress.getAddress());
}

void TransactionVersionManager::incrementLogPointer(int thread, int size_in_bytes){
  assert(size_in_bytes > 0);
  m_logPointer[thread] = Address(m_logPointer[thread].getAddress() +
                            size_in_bytes );
  m_logPointer_physical[thread] = Address(m_logPointer_physical[thread].getAddress() +
                            size_in_bytes );
  m_logSize[thread] += size_in_bytes;
  assert(m_logSize[thread] < (m_maxLogSize[thread]*1024));
}
   
bool TransactionVersionManager::shouldLog(int thread, Address logical_address, int transactionLevel){
  logical_address.makeLineAddress();
  return (!m_logAddressFilter_ptr[thread]->exist(logical_address));
}

void TransactionVersionManager::processUndoLogEntry(int thread, physical_address_t log_entry, physical_address_t dest_address){
  int proc_no = getProcID()*RubyConfig::numberofSMTThreads() + thread;
  unsigned int logEntrySize = g_DATA_BLOCK_BYTES + g_ADDRESS_SIZE;
  char buffer[logEntrySize];
  
  physical_address_t dest_page = (dest_address / g_PAGE_SIZE_BYTES) * g_PAGE_SIZE_BYTES;
  assert(m_logAddressTLB->exist(dest_page));
  physical_address_t dest_address_physical = m_logAddressTLB->lookup(dest_page) + dest_address % g_PAGE_SIZE_BYTES;
  
  if (XACT_DEBUG && XACT_DEBUG_LEVEL > 1){
    cout << proc_no << " [" << getProcID() << "," << thread << "] HW ROLLBACK UNDO LOG ENTRY 0x" << hex << log_entry << dec << " DEST ADDRESS VA 0x" << hex << dest_address << dec
          << " PA 0x" << hex << dest_address_physical << dec << endl;
  }
  
  for (int i = 0; i < g_DATA_BLOCK_BYTES; i++){
    physical_address_t log_entry_pointer = log_entry + i;      
    int page_offset =  log_entry_pointer % g_PAGE_SIZE_BYTES;
    physical_address_t virtual_page = log_entry_pointer - page_offset;
    assert(m_logAddressTLB->exist(virtual_page));
    physical_address_t log_entry_pointer_physical = m_logAddressTLB->lookup(virtual_page) + page_offset;
    
    buffer[i] = g_system_ptr->getDriver()->readPhysicalMemory(proc_no, log_entry_pointer_physical, 1);
    g_system_ptr->getDriver()->writePhysicalMemory(proc_no, dest_address_physical + i, buffer[i], 1);
  }                         
}
        
void TransactionVersionManager::hardwareRollback(int thread){
  int proc_no = getProcID()*RubyConfig::numberofSMTThreads() + thread;
  unsigned int logEntrySize = g_DATA_BLOCK_BYTES + g_ADDRESS_SIZE;
  
  unsigned int log_size = g_system_ptr->getDriver()->readPhysicalMemory(proc_no, getLogBasePhysical(thread).getAddress() + s_softwareXactStructMap[XACT_LOG_SIZE].offset, 4);
  unsigned int xact_level = g_system_ptr->getDriver()->readPhysicalMemory(proc_no, getLogBasePhysical(thread).getAddress() + s_softwareXactStructMap[XACT_LEVEL].offset, 4);
  physical_address_t log_pointer = m_logPointer[thread].getAddress();
  physical_address_t log_entry   = log_pointer - logEntrySize;
  
  
  while (log_size > 0){
    physical_address_t log_entry_header = log_entry + logEntrySize - g_ADDRESS_SIZE;      
    int page_offset =  log_entry_header % g_PAGE_SIZE_BYTES;
    physical_address_t virtual_page = log_entry_header - page_offset;
    assert(m_logAddressTLB->exist(virtual_page));
    physical_address_t log_entry_header_physical = m_logAddressTLB->lookup(virtual_page) + page_offset;
    
    physical_address_t dest_address, dest_address_physical;
    unsigned int entry_type = g_system_ptr->getDriver()->readPhysicalMemory(proc_no, log_entry_header_physical, g_ADDRESS_SIZE) & g_ENTRY_MASK;      
    dest_address = g_system_ptr->getDriver()->readPhysicalMemory(proc_no, log_entry_header_physical, g_ADDRESS_SIZE) & g_ADDRESS_MASK;   
    
    switch (entry_type){
      case 0x00000004: // UNDO LOG ENTRY
                       // tm_unroll_log_entry
                      processUndoLogEntry(thread, log_entry, dest_address);
                      log_size -= logEntrySize;
                      log_pointer -= logEntrySize;
                      log_entry -= logEntrySize;
                      break;
      case 0x00000002: // GARBAGE TRANSACTION HEADER                             
      case 0x00000003:  // REGISTER CHECKPOINT
                      log_size -= logEntrySize;
                      log_pointer -= logEntrySize;
                      log_entry -= logEntrySize;
                      break;
      case 0x00000001: // ACTIVE TRANSACTION HEADER  
                      //tm_release_isolation        
                      SIMICS_write_register(proc_no, SIMICS_get_register_number(proc_no, "g2"), xact_level);            
                      XACT_MGR->releaseIsolation(thread);
                      xact_level--;
                      log_size -= logEntrySize;
                      log_pointer -= logEntrySize;
                      log_entry -= logEntrySize;
                      break;
      case 0x00000005: // COMPENSATION ENTRY                      
      case 0x00000006: // COMMIT ENTRY                      
      case 0x00000007: // XMALLOC ENTRY       
                      assert(0);               
      default:
              break;
    }
  }                          
    
  if (log_size != 0)
      cout << " LOG SIZE!!!! " << log_size << " XACT_LEVEL " << xact_level << endl;        
  assert(log_size == 0);
  g_system_ptr->getDriver()->writePhysicalMemory(proc_no, getLogBasePhysical(thread).getAddress() + s_softwareXactStructMap[XACT_LEVEL].offset, xact_level, 4);
  g_system_ptr->getDriver()->writePhysicalMemory(proc_no, getLogBasePhysical(thread).getAddress() + s_softwareXactStructMap[XACT_LOG_SIZE].offset, log_size, 4);
}        
