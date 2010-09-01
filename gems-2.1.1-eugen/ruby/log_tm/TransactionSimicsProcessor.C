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

#include "TransactionSimicsProcessor.h"
#include "TransactionInterfaceManager.h"
#include "TransactionVersionManager.h"
#include "TransactionConflictManager.h"
#include "TransactionIsolationManager.h"
#include "XactCommitArbiter.h"
#include "XactIsolationChecker.h"
#include "LazyTransactionVersionManager.h"
#include "CacheMsg.h"
#include "CacheRequestType.h"
#include "interface.h"
#include "SimicsDriver.h"
#include "SimicsHypervisor.h"

#include <assert.h>

// Macros to get privileged mode bit
#ifdef SPARC
  #define PRIV_MODE (mem_trans->priv)
#else
// x86
  #define PRIV_MODE (mem_trans->mode)
#endif

TransactionSimicsProcessor::TransactionSimicsProcessor(System* sys_ptr, int proc_no) {
  m_proc = proc_no;        
  // get corresponding sequencer
  m_sequencer = sys_ptr->getChip(m_proc/RubyConfig::numberOfProcsPerChip())->getSequencer(m_proc%RubyConfig::numberOfProcsPerChip());
  assert(m_sequencer);

  m_xact_mgr = NULL;
  // get corresponding transaction manager
  if (XACT_MEMORY) {
    m_xact_mgr = sys_ptr->getChip(m_proc/RubyConfig::numberOfProcsPerChip())->getTransactionInterfaceManager(m_proc%RubyConfig::numberOfProcsPerChip());
    assert(m_xact_mgr);
  }

  m_outstandingXactStoreRefCount = 0;
	m_magic_cross_call = false;
}

TransactionSimicsProcessor::~TransactionSimicsProcessor() {
}

bool TransactionSimicsProcessor::getMagicCrossCall() {
	return m_magic_cross_call;
}

void TransactionSimicsProcessor::setMagicCrossCall(bool value) {
	m_magic_cross_call = value;
}

TransactionInterfaceManager* TransactionSimicsProcessor::getTransactionInterfaceManager() {
  return m_xact_mgr;
}

void TransactionSimicsProcessor::setTransactionInterfaceManager(TransactionInterfaceManager* xact_mgr) {
  m_xact_mgr = xact_mgr;
  assert(m_xact_mgr);
}

bool TransactionSimicsProcessor::generateLogRequest(memory_transaction_t *mem_trans) {
  assert(mem_trans->s.may_stall);
  assert((m_outstandingXactStoreRefCount == 0));
  assert(!XACT_LAZY_VM);

  if (XACT_ENABLE_TOURMALINE)
    return true;        

  Address logBlockAddressPhy = m_xact_mgr->getXactVersionManager()->getLogPointerAddressPhysical(0);
  Address logBlockAddressVirt = m_xact_mgr->getXactVersionManager()->getLogPointerAddress(0);
  logBlockAddressPhy.makeLineAddress();
  logBlockAddressVirt.makeLineAddress();
  
  CacheMsg logMsg(logBlockAddressPhy,
                  logBlockAddressPhy,
                  CacheRequestType_ST,
                  SIMICS_get_program_counter(mem_trans->s.ini_ptr),
                  AccessModeType_UserMode,
                  RubyConfig::dataBlockBytes(), 
                  PrefetchBit_No,
                  0,
                  logBlockAddressVirt,
                  0,
                  m_xact_mgr->getXactConflictManager()->getTimestamp(0),
                  1     // Log Blocks are escaped accesses by default,
                  );

  // check if Ruby is ready to accept requests
  if (m_sequencer->isReady(logMsg) == false) {
    DEBUG_MSG(SEQUENCER_COMP,MedPrio, "Sequencer was NOT READY");
    cerr << "Sequencer for " << m_proc << " is not ready to handle logging request! " << endl;
    return false;
  }
  
  // Wait for both the program store and LOG store to complete 
  // before unstalling SIMICS
  m_outstandingXactStoreRefCount = 2;
  m_sequencer->makeRequest(logMsg);
  return true;
}        

void TransactionSimicsProcessor::updateStoreSetPredictor(CacheRequestType type, Address address) {
  assert(XACT_EAGER_CD);      
  if(type == CacheRequestType_LD_XACT){
    m_xact_mgr->addToLoadAddressMap(0, SIMICS_get_program_counter(m_proc).getAddress(), address.getAddress());
  } else if (type == CacheRequestType_ST_XACT){
    m_xact_mgr->updateStorePredictor(0, address.getAddress());
  }
}

/**
  * SimicsProcessor about to issue a memory request to the cache coherence protocol.
  * Update Store Set Predictor 
  * Issue coherence request for log block if necessary.
	*
	* Return Values
	*	 0: Not Ready
	*  1: Not Ready - Summary conflict
	*  2: Ready 
  */
unsigned int TransactionSimicsProcessor::startingMemRef(memory_transaction_t *mem_trans, CacheRequestType type) {
  Address logicalAddress = Address(mem_trans->s.logical_address);
  logicalAddress.makeLineAddress();      
	
 	if (XACT_ENABLE_VIRTUALIZATION_LOGTM_SE) {
		SimicsDriver* simics_interface_ptr = static_cast<SimicsDriver*>(g_system_ptr->getDriver());
		Address conflictAddress;
	  bool isEscapeAction, request_is_read; 
		unsigned int conflictType;
	  if (type == CacheRequestType_LDX_XACT || 
 		    type == CacheRequestType_ST_XACT ||
    		type == CacheRequestType_ST ||
      	type == CacheRequestType_ATOMIC) {
	    request_is_read = false;
 		} else {
   		request_is_read = true;
	  }
		conflictAddress = Address(mem_trans->s.physical_address);
	  isEscapeAction = (m_xact_mgr->getEscapeDepth(0) > 0)|| mem_trans->s.type==Sim_Trans_Instr_Fetch || PRIV_MODE;
  	if (!isEscapeAction && 
					(conflictType = simics_interface_ptr->getHypervisor()->existSummarySignatureConflict(conflictAddress, request_is_read, m_xact_mgr)))
		{
   		if (XACT_DEBUG && XACT_DEBUG_LEVEL > 1) {
        	cout << g_eventQueue_ptr->getTime() << " " << m_proc
							  << " [" << m_proc / RubyConfig::numberofSMTThreads() << "," << m_proc % RubyConfig::numberofSMTThreads() << "]"       
	    				  << " SUMMARY CONFLICT  TYPE " << conflictType << "  VA " << Address(mem_trans->s.logical_address) 
	    				  << "  PA " << Address(mem_trans->s.physical_address) 
								<< "  PC " << SIMICS_get_program_counter(m_proc) << endl;
			}
	  	m_xact_mgr->setSummaryConflictFlag(0, conflictAddress);
	  	m_xact_mgr->getXactIsolationManager()->setSummaryConflictAddress(conflictAddress);
	  	m_xact_mgr->getXactIsolationManager()->setSummaryConflictType(conflictType);
			return 1;
		}
 	}

  if (m_xact_mgr->magicWait(0)) 
    return 0;
 
  if (XACT_EAGER_CD) {
    updateStoreSetPredictor(type, logicalAddress);              
  }  
  
  if (!XACT_LAZY_VM){      
    if((type == CacheRequestType_ST_XACT) || (type == CacheRequestType_LDX_XACT)){
      bool addEntry = m_xact_mgr->getXactVersionManager()->shouldLog(0, logicalAddress, m_xact_mgr->getTransactionLevel(0) );
      if(addEntry)
        if (generateLogRequest(mem_trans) == false)
          return 0;
    }
  }  
  return 2;
}    

int TransactionSimicsProcessor::computeFirstXactAccessCost(CacheRequestType type, Address physicalAddress){
  return 0;        
}        

bool TransactionSimicsProcessor::tourmalineExistGlobalConflict(CacheRequestType type, Address physicalAddress) {
  bool existConflict = false;      
  switch (type) {
    case CacheRequestType_IFETCH:
    case CacheRequestType_LD:
    case CacheRequestType_LD_XACT:
      if (m_xact_mgr->existGlobalLoadConflict(0, physicalAddress))
        existConflict = true;
      break;            
    case CacheRequestType_ST:
    case CacheRequestType_ST_XACT:
    case CacheRequestType_LDX_XACT:
    case CacheRequestType_ATOMIC:
      if (m_xact_mgr->existGlobalStoreConflict(0, physicalAddress))
        existConflict = true;
      break;            
    default:
      break;
  }

  if (existConflict) {
    if (XACT_DEBUG && XACT_DEBUG_LEVEL > 2) {
      cout << g_eventQueue_ptr->getTime() << " " << m_proc << " [" << m_proc / RubyConfig::numberofSMTThreads() << "," << m_proc % RubyConfig::numberofSMTThreads() << "] TOURMALINE CONFLICT " << physicalAddress << " PC = " << SIMICS_get_program_counter(m_proc) << endl;        
    }
  }
  return existConflict;      
}    

void TransactionSimicsProcessor::addToReadWriteSets(memory_transaction_t *mem_trans, CacheRequestType type) {
  Address physicalAddress = Address(mem_trans->s.physical_address);
  Address logicalAddress  = Address(mem_trans->s.logical_address);
  
  addToReadWriteSets(type, physicalAddress, logicalAddress);
}          

void TransactionSimicsProcessor::addToReadWriteSets(CacheRequestType type, Address physicalAddress, Address logicalAddress){
  int transactionLevel = m_xact_mgr->getTransactionLevel(0);
  if (transactionLevel == 0) return;
  switch (type) {
    case CacheRequestType_LD_XACT:
      m_xact_mgr->isolateTransactionLoad(0, logicalAddress, physicalAddress, transactionLevel);
	  	if(XACT_ENABLE_VIRTUALIZATION_LOGTM_SE) {
			  m_xact_mgr->getXactIsolationManager()->addToVirtualReadSetPerfectFilter(0, physicalAddress);
			  m_xact_mgr->getXactIsolationManager()->addToVirtualReadSetFilter(0, physicalAddress);
			}
      break;
    case CacheRequestType_LDX_XACT:
    case CacheRequestType_ST_XACT:
      m_xact_mgr->isolateTransactionStore(0, logicalAddress, physicalAddress, transactionLevel);
	  	if(XACT_ENABLE_VIRTUALIZATION_LOGTM_SE) {
			  m_xact_mgr->getXactIsolationManager()->addToVirtualWriteSetPerfectFilter(0, physicalAddress);
			  m_xact_mgr->getXactIsolationManager()->addToVirtualWriteSetFilter(0, physicalAddress);
			}
      break;
    default:
      break;
  }
}
        
/** Ruby has either
  * 1) obtained the necessary cache permissions for the block or
  * 2) failed to acquire necessary cache permissions due to transactional conflicts
  * Isolate transactional blocks, if successful.
  * Unstall SIMICS
  *
  * If TOURMALINE, need to do all conflict checks here.
  * - If retry, do not unstall simics
  */ 
void TransactionSimicsProcessor::completedMemRef(bool success, CacheRequestType type, Address physicalAddress, Address logicalAddress) {
        
  if (success) { 
    int transactionLevel = m_xact_mgr->getTransactionLevel(0);       
  
    switch (type) {
      case CacheRequestType_ST:  
        m_xact_mgr->profileStore(0, logicalAddress, physicalAddress, transactionLevel);
        break;
      case CacheRequestType_LD:  
        m_xact_mgr->profileLoad(0, logicalAddress, physicalAddress, transactionLevel);
        break;
      default:
        break;
    }      
    
    // REDIRECT WRITE BUFFER REQUESTS
    if (XACT_LAZY_VM && type == CacheRequestType_ST_XACT){
      if ((XACT_EAGER_CD && g_system_ptr->getXactCommitArbiter()->existCommitTokenRequest(m_proc)) ||
          (!XACT_EAGER_CD && g_system_ptr->getXactCommitArbiter()->getTokenOwner() == m_proc)) {
        m_xact_mgr->getXactLazyVersionManager()->hitCallback(0, physicalAddress);
        return;
      }
    }
    
    if (m_xact_mgr->inTransaction(0)) {        
      addToReadWriteSets(type, physicalAddress, logicalAddress);
    }  
  } 

  // UNSTALL SIMICS
  if (m_outstandingXactStoreRefCount > 0)
    m_outstandingXactStoreRefCount -= 1;
    
  if (m_outstandingXactStoreRefCount == 0) {
    int firstAccessCost = computeFirstXactAccessCost(type, physicalAddress);
    if (!success) firstAccessCost = 0;
    SIMICS_unstall_proc(m_proc, firstAccessCost);
  }   
  
  if (0 && XACT_DEBUG && XACT_DEBUG_LEVEL > 2) {
    if (m_xact_mgr->inTransaction(0))      
      cout << g_eventQueue_ptr->getTime() << " " << m_proc << " [" << m_proc / RubyConfig::numberofSMTThreads() << "," << m_proc % RubyConfig::numberofSMTThreads() << "] COMPLETED MEM REF" << physicalAddress << " type " << type << " PC = " << SIMICS_get_program_counter(m_proc) << endl;        
  }    
}        

void TransactionSimicsProcessor::processLogStore(memory_transaction_t *mem_trans, CacheRequestType type) {
  assert(!XACT_LAZY_VM);      
  if (type == CacheRequestType_LDX_XACT ||
      type == CacheRequestType_ST_XACT){ 
    Address phys_addr = Address(mem_trans->s.physical_address);
    Address logical_addr = Address(mem_trans->s.logical_address);
    phys_addr.makeLineAddress();
    logical_addr.makeLineAddress();
    bool shouldLog = m_xact_mgr->getXactVersionManager()->shouldLog(0, logical_addr, m_xact_mgr->getTransactionLevel(0));        
    if (XACT_DEBUG && XACT_DEBUG_LEVEL > 2) {
      cout << g_eventQueue_ptr->getTime() << " " << m_proc << " [" << m_proc / RubyConfig::numberofSMTThreads() << "," << m_proc % RubyConfig::numberofSMTThreads() << "] LOGGING STORE: " << logical_addr << " " << shouldLog << " PC = " << SIMICS_get_program_counter(m_proc) << endl;        
    }
    if (shouldLog){
      // LOG XACT STORES      
      m_xact_mgr->getXactVersionManager()->addUndoLogEntry(0, logical_addr, phys_addr);
    }  
  } else if (((type == CacheRequestType_ST) || 
              (type == CacheRequestType_ATOMIC)) && 
                m_xact_mgr->inLoggedException(0) && (m_xact_mgr->getTransactionLevel(0) > 0)){
    Address phys_addr = Address(mem_trans->s.physical_address);
    Address logical_addr = Address(mem_trans->s.logical_address);
    phys_addr.makeLineAddress();
    logical_addr.makeLineAddress();
    bool shouldLog = m_xact_mgr->getXactVersionManager()->shouldLog(0, logical_addr, m_xact_mgr->getTransactionLevel(0));        
    if (XACT_DEBUG && XACT_DEBUG_LEVEL > 2) {
      cout << g_eventQueue_ptr->getTime() << " " << m_proc << " [" << m_proc / RubyConfig::numberofSMTThreads() << "," << m_proc % RubyConfig::numberofSMTThreads() << "] LOGGING STORE: " << logical_addr << " " << shouldLog << " PC = " << SIMICS_get_program_counter(m_proc) << endl;        
    }
    if (shouldLog){
      // LOG SPILL STORES      
      m_xact_mgr->getXactVersionManager()->addUndoLogEntry(0, logical_addr, phys_addr);
    }  
  }
}    

void TransactionSimicsProcessor::processRubyWatchAddress(memory_transaction_t *mem_trans, CacheRequestType type){
  if (g_system_ptr->getProfiler()->watchAddress(Address(mem_trans->s.logical_address)))
    cout << m_proc << " RUBY WATCH: EVENT " << type
         << " ADDRESS: " << Address(mem_trans->s.logical_address)
         << " VALUE: "   << hex << SIMICS_read_physical_memory(m_proc, mem_trans->s.physical_address, 4)
         << dec << endl;                 
}

void TransactionSimicsProcessor::xactIsolationCheck(memory_transaction_t *mem_trans, CacheRequestType type){
  assert(XACT_EAGER_CD);      
  int proc_num = m_proc;      
  Address addr = Address(mem_trans->s.physical_address);
  addr.makeLineAddress();
  if (! g_system_ptr->getXactIsolationChecker()->checkXACTConsistency(proc_num, addr, type)){
    bool isEscapedAccess = ((m_xact_mgr->getEscapeDepth(0) > 0)|| mem_trans->s.type==Sim_Trans_Instr_Fetch || PRIV_MODE);
    if (!isEscapedAccess){       
      cerr << m_proc << " XACT CONSISTENCY CHECK FAILURE DUE TO OVERLAP BETWEEN NONTRANSACTION AND TRANSACTIONS ";
      cerr << " Address: " << addr << " PC: " << SIMICS_get_program_counter(m_proc) << " RANDOM SEED " << g_RANDOM_SEED << " cycle " << g_eventQueue_ptr->getTime() << endl;
      assert(false);       
    } else {
      cerr << m_proc << " XACT CONSISTENCY CHECK FAILURE DUE TO OVERLAP BETWEEN ESCAPE ACTIONS AND TRANSACTIONS ";
      cerr << " Address: " << addr << " PC: " << SIMICS_get_program_counter(m_proc) << " RANDOM SEED " << g_RANDOM_SEED << " cycle " << g_eventQueue_ptr->getTime() << endl;        
    }        
  }  
  
  int xact_level = m_xact_mgr->getTransactionLevel(0); 
  switch(type){
    case CacheRequestType_ST_XACT:
    case CacheRequestType_LDX_XACT:
      g_system_ptr->getXactIsolationChecker()->addToWriteSet(proc_num, addr, xact_level);
      break;
    case CacheRequestType_LD_XACT:
      g_system_ptr->getXactIsolationChecker()->addToReadSet(proc_num, addr, xact_level);
      break;
    default: 
           ;    
  }
}

/** 
  * Redirects Transactional store to a write buffer. Ensures that the old value remains
  * in committed memory state (SIMICS memory)
  */
void TransactionSimicsProcessor::redirectStoreToWriteBuffer(memory_transaction_t *mem_trans){
  assert(XACT_LAZY_VM);      
  assert(mem_trans->s.size > 0 && mem_trans->s.size < 256); 
  physical_address_t addr = mem_trans->s.physical_address;      
  m_oldValueBufferSize = mem_trans->s.size;
  
  SIM_c_get_mem_op_value_buf( &(mem_trans->s), m_newValueBuffer);
  Vector<uint8> data;
  data.setSize(mem_trans->s.size);
  for (unsigned int i = 0; i < mem_trans->s.size; i++)
    data[i] = (uint8)m_newValueBuffer[i];
  
  m_xact_mgr->getXactLazyVersionManager()->addToWriteBuffer(0, m_xact_mgr->getTransactionLevel(0), Address(mem_trans->s.physical_address), mem_trans->s.size, data);
  
  for (unsigned int i = 0; i < mem_trans->s.size; i++)
    m_oldValueBuffer[i] = SIMICS_read_physical_memory(m_proc, addr + i, 1);
  
  SIM_c_set_mem_op_value_buf( &(mem_trans->s), m_oldValueBuffer);
      
   if (XACT_DEBUG && XACT_DEBUG_LEVEL > 2)
    cout << g_eventQueue_ptr->getTime() << " " << m_proc << " [" << m_proc / RubyConfig::numberofSMTThreads() << "," << m_proc % RubyConfig::numberofSMTThreads() << "] REDIRECTING TO STORE BUFFER" << Address(mem_trans->s.physical_address) << " " << m_oldValueBufferSize << endl;        
}

/** 
  * SimicsProcessor is ready to retire the memory reference 
  */
void TransactionSimicsProcessor::readyToRetireMemRef(bool success, memory_transaction_t *mem_trans, CacheRequestType type) {
        
  if (0 && XACT_DEBUG && XACT_DEBUG_LEVEL > 2) {
    if (m_xact_mgr->inTransaction(0))      
      cout << g_eventQueue_ptr->getTime() << " " << m_proc << " [" << m_proc / RubyConfig::numberofSMTThreads() << "," << m_proc % RubyConfig::numberofSMTThreads() << "] READY TO RETIRE MEM REF" << Address(mem_trans->s.physical_address) << " type " << type << " PC = " << SIMICS_get_program_counter(m_proc) << endl;        
  }    

  if (!success && !XACT_LAZY_VM) {
    conf_object_t* cpu_obj = (conf_object_t*) SIM_proc_no_2_ptr(m_proc);
	  uinteger_t intr_receive =  SIM_read_register(cpu_obj, SIM_get_register_number(cpu_obj, "intr_receive"));
  	uinteger_t intr_receive_busy_flag = (intr_receive & 0x20) >> 5;
	  if (intr_receive_busy_flag) {
		  // There is a pending interrupt and we are stalling SIMICS. How unfortunate! 
  		// SIMICS would not take the interrupt until we unstall this conflicting memory request! 
      m_xact_mgr->setInterruptTrap(0);
      return;
    }
  }  
    
  // processRubyWatchAddress (mem_trans, type);       
  
  if (!XACT_LAZY_VM) {
    if (success) {       
      if (!(m_xact_mgr->shouldTrap(0) && !PRIV_MODE))
        processLogStore(mem_trans, type);
    }    
  } else {
    if (type == CacheRequestType_ST_XACT){      
      redirectStoreToWriteBuffer(mem_trans);
    }
  }                            
}

/**
  * Can SimicsProcessor retire the Memory Reference. 
  * 0 - Retire, 1 - Trap, 2 - Retry
  */
int TransactionSimicsProcessor::canRetireMemRef(memory_transaction_t *mem_trans, CacheRequestType type, bool success){
  int canRetire = 0;      
  if (!success) {      
    if (m_xact_mgr->shouldTrap(0) && !PRIV_MODE)
      canRetire = 1;
    else
      canRetire = 2;
  } else {
    if (m_xact_mgr->shouldTrap(0) && !PRIV_MODE)
      canRetire = 1;
    else
      canRetire = 0;
  }              
  
  if (XACT_EAGER_CD && (canRetire == 0)) {
    if (XACT_ISOLATION_CHECK)      
      xactIsolationCheck(mem_trans, type);
  }
  
  if (0 && XACT_DEBUG && XACT_DEBUG_LEVEL > 2) {
    if (m_xact_mgr->inTransaction(0)) {      
      cout << g_eventQueue_ptr->getTime() << " " << m_proc << " [" << m_proc / RubyConfig::numberofSMTThreads() << "," << m_proc % RubyConfig::numberofSMTThreads() << "] CAN RETIRE " << Address(mem_trans->s.physical_address) << " RESULT: " << canRetire << " PC = " << SIMICS_get_program_counter(m_proc) << " shouldTrap " << m_xact_mgr->shouldTrap(0) << " PRIV " << PRIV_MODE << endl;        
    }
  }    
  
  if (XACT_LAZY_VM && canRetire == 1) {
    // Abort done at the end of current instruction via a callback
    canRetire = 0;
  }
  
  if (canRetire == 1){
    if (m_xact_mgr->shouldTakeTrapCheckpoint(0))
      m_xact_mgr->takeContinueCheckpoint(0);
  }    
  
  return canRetire;                         
}        
        
void TransactionSimicsProcessor::bypassLoadFromWriteBuffer(memory_transaction_t *mem_trans) {
  assert(XACT_LAZY_VM);

  if ((m_xact_mgr->getTransactionLevel(0) > 0) && (m_xact_mgr->getEscapeDepth(0) == 0) 
          && (!PRIV_MODE)){
    char  buffer[256];
    if (mem_trans->s.type == Sim_Trans_Load){ 
      if (XACT_DEBUG && XACT_DEBUG_LEVEL > 2)
        cout << g_eventQueue_ptr->getTime() << " " << m_proc << " [" << m_proc / RubyConfig::numberofSMTThreads() << "," << m_proc % RubyConfig::numberofSMTThreads() << "] REQUESTING DATA FROM BUFFER " << Address(mem_trans->s.physical_address) << endl;        
      
      Vector<uint8> data = m_xact_mgr->getXactLazyVersionManager()->forwardData(0, m_xact_mgr->getTransactionLevel(0), Address(mem_trans->s.physical_address), mem_trans->s.size);
      assert((uint)data.size() == mem_trans->s.size);
      for (unsigned int i = 0; i < mem_trans->s.size; i++){
        buffer[i] = (char)data[i];
      }                  
      SIM_c_set_mem_op_value_buf( &(mem_trans->s), buffer);
    } else if (mem_trans->s.type == Sim_Trans_Store) {
      // SANITY CHECKS
    
      SIM_c_get_mem_op_value_buf( &(mem_trans->s), buffer);
      if (!((m_oldValueBufferSize > 0) && (m_oldValueBufferSize == mem_trans->s.size))){
        cerr << g_eventQueue_ptr->getTime() << " " << m_proc << " [" << m_proc / RubyConfig::numberofSMTThreads() << "," << m_proc % RubyConfig::numberofSMTThreads() << "] RESTORE_BUFFER_SIZE " << m_oldValueBufferSize << " Mem_trans SIZE: " << mem_trans->s.size << " ADDR: " << Address(mem_trans->s.physical_address) << " PC: " << SIMICS_get_program_counter(m_proc) << flush << endl;   
        assert(0);
      }       

      for (unsigned int i = 0; i < mem_trans->s.size; i++){
        if (XACT_DEBUG && XACT_DEBUG_LEVEL > 2){
          if ((uint8)buffer[i] != SIMICS_read_physical_memory(m_proc, mem_trans->s.physical_address + i, 1)){
            cout << g_eventQueue_ptr->getTime() << " " << m_proc << " [" << m_proc / RubyConfig::numberofSMTThreads() << "," << m_proc % RubyConfig::numberofSMTThreads() << " ] PA " << Address(mem_trans->s.physical_address + i) << " BUFFER[" << i << "] " << (int)(buffer[i]) << " MEM_VAL: " << SIMICS_read_physical_memory(m_proc, mem_trans->s.physical_address + i, 1) << " NEW_VAL BUFF: " << (int)(m_newValueBuffer[i]) << " ***************** WARNING *************" << endl;   
          }
        }          
      }       
      
      m_oldValueBufferSize = 0;   
      
      if (XACT_DEBUG && XACT_DEBUG_LEVEL > 2){
        Vector<uint8> data;
        data.setSize(mem_trans->s.size);
        cout << g_eventQueue_ptr->getTime() << " " << m_proc << " [" << m_proc / RubyConfig::numberofSMTThreads() << "," << m_proc % RubyConfig::numberofSMTThreads() << " ] PA " << Address(mem_trans->s.physical_address) << " SIZE " << mem_trans->s.size << " MEM_VAL: " << hex << SIMICS_read_physical_memory(m_proc, mem_trans->s.physical_address, mem_trans->s.size) << dec; 
        cout << " OLD_VAL_BUF: ["; 
        for (unsigned int j = 0; j < mem_trans->s.size; j++){
          data[j] = (uint8)m_oldValueBuffer[j];      
          cout << " 0x" << hex << (uinteger_t)(data[j]) << dec;  
        }
        cout << " ] NEW_VAL_BUF: [" ;  
        for (unsigned int j = 0; j < mem_trans->s.size; j++){
          data[j] = (uint8)m_newValueBuffer[j];      
          cout << " 0x" << hex << (uinteger_t)(data[j]) << dec;  
        }
        cout << " ]" << endl;  
      }               
    }
  }
}

/**
  * Called 'after' SIMICS has committed the memory reference.
  * XACT_LAZY_VM systems:
  *   Bypass Loads from write buffer if necessary.
  *   Abort Transaction if necessary.
  */
void TransactionSimicsProcessor::retiredMemRef(memory_transaction_t *mem_trans){
  if (XACT_LAZY_VM) {
    bypassLoadFromWriteBuffer(mem_trans);      
    if (m_xact_mgr->shouldTrap(0) && !PRIV_MODE){
      if (mem_trans->s.type == Sim_Trans_Store || mem_trans->s.type == Sim_Trans_Load) {       
        if (m_xact_mgr->getTransactionLevel(0) <= 0){
          cerr << g_eventQueue_ptr->getTime() << " " << m_proc << " SHOULD TRAP IN NON_XACT " << m_xact_mgr->getTransactionLevel(0) << " *****************WARNING********************" << endl; 
          assert(0);
        } else {  
          m_xact_mgr->abortLazyTransaction(0);
        }  
      }  
    }
  }  
}        

/*
int TransactionSimicsProcessor::computeFirstXactAccessCost(memory_transaction_t *mem_trans, CacheMsg &request){
      int firstXactAccessCost = 0;

      if (!(request.m_Type == CacheRequestType_LD_XACT || request.m_Type == CacheRequestType_LDX_XACT || 
              request.m_Type == CacheRequestType_ST_XACT))
        return firstXactAccessCost;                

      if (XACT_FIRST_ACCESS_COST == 0 && XACT_FIRST_PAGE_ACCESS_COST == 0)
        return firstXactAccessCost;        
      
      if (XACT_FIRST_ACCESS_COST > 0){
        bool firstXactAccess = false;
        Address physical_addr = Address(mem_trans->s.physical_address);      
        physical_addr.makeLineAddress();
        if (request.m_Type == CacheRequestType_LD_XACT){
          firstXactAccess = !(m_xact_mgr->getXactIsolationManager()->isInReadSetPerfectFilter(0, physical_addr) 
                          || m_xact_mgr->getXactIsolationManager()->isInWriteSetPerfectFilter(0, physical_addr));
        } else if (request.m_Type == CacheRequestType_LDX_XACT || request.m_Type == CacheRequestType_ST_XACT){              
          firstXactAccess = !m_xact_mgr->getXactIsolationManager()->isInWriteSetPerfectFilter(0, physical_addr);
        }
        if (firstXactAccess){
          firstXactAccessCost += XACT_FIRST_ACCESS_COST;
        }
      }

      if (XACT_FIRST_PAGE_ACCESS_COST > 0){
        bool firstXactPageAccess = true;
        Address check_addr = Address(mem_trans->s.physical_address);
        check_addr.makePageAddress();
        int numberOfBlocks = RubyConfig::pageSizeBytes() / RubyConfig::dataBlockBytes();
        for (int i = 0; i < numberOfBlocks; i++){
          if (request.m_Type == CacheRequestType_LD_XACT){
            firstXactPageAccess = !(m_xact_mgr->getXactIsolationManager()->isInReadSetPerfectFilter(0, check_addr) 
                            || m_xact_mgr->getXactIsolationManager()->isInWriteSetPerfectFilter(0, check_addr));
          } else if (request.m_Type == CacheRequestType_LDX_XACT || request.m_Type == CacheRequestType_ST_XACT){              
            firstXactPageAccess = !m_xact_mgr->getXactIsolationManager()->isInWriteSetPerfectFilter(0, check_addr);
          }
          if (!firstXactPageAccess){
            break;
          }
          check_addr.makeNextStrideAddress(1);          
        }    
        if (firstXactPageAccess){
          firstXactAccessCost += XACT_FIRST_PAGE_ACCESS_COST;
        }                  
      }          
      return firstXactAccessCost;
}      
*/
