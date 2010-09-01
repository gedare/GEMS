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

#include "SimicsDriver.h"
#include "SimicsHypervisor.h"
#include "TransactionInterfaceManager.h"
#include "TransactionIsolationManager.h"
#include "TransactionConflictManager.h"
#include "TransactionVersionManager.h"
#include "LazyTransactionVersionManager.h"
#include "RockTransactionManager.h"
#include "XactIsolationChecker.h"
#include "OpalInterface.h"
#include "interface.h"
#include "XactCommitArbiter.h"
#include "XactVisualizer.h"

#include <assert.h>
#include <stdlib.h>

#define SEQUENCER g_system_ptr->getChip(m_chip_ptr->getID())->getSequencer(m_version)

TransactionInterfaceManager::TransactionInterfaceManager(AbstractChip* chip_ptr, int version) {
  int smt_threads = RubyConfig::numberofSMTThreads();
  m_chip_ptr = chip_ptr;
  m_version = version;

  m_xactVersionManager   = new TransactionVersionManager(chip_ptr, version);
  m_xactIsolationManager = new TransactionIsolationManager(chip_ptr, version);
  m_xactConflictManager  = new TransactionConflictManager(chip_ptr, version);

  m_xactLazyVersionManager   = new LazyTransactionVersionManager(chip_ptr, version);

  if (ATMTP_ENABLED) {
    m_xactRockManager    = new RockTransactionManager*[smt_threads];
  }
  m_transactionLevel  = new int[smt_threads];
  m_xid               = new int[smt_threads];
  m_tid               = new int[smt_threads];
  m_escapeDepth       = new int[smt_threads];
  m_inLoggedException = new int[smt_threads]; 
  m_handlerAddress    = new Address[smt_threads];

  m_processingTrap     = new bool[smt_threads];
  m_shouldTrap         = new bool[smt_threads];
  m_trapTakeCheckpoint = new bool[smt_threads];
  m_trapType           = new int[smt_threads]; // 1 - Abort, 2 - getTranslation, 3 - conflict
  m_trapAddress        = new Address[smt_threads];
  m_trapPC             = new Address[smt_threads];
  m_trapTime           = new Time[smt_threads];
  m_ignoreWatchpointFlag = new bool[smt_threads];
  m_continueCheckpoint.setSize(smt_threads);

  m_numLoadOverflows   = new int[smt_threads];
  m_numStoreOverflows  = new int[smt_threads];
  m_numLoadMisses      = new int[smt_threads];
  m_numStoreMisses     = new int[smt_threads];
  m_xactBeginCycle     = new Time[smt_threads];
  m_xactInstrCount     = new long long[smt_threads];

  m_xactLoadAddressMap_ptr.setSize(smt_threads);
  m_xactStorePredictorPC_ptr = new Map<physical_address_t, int>;

  for (int i = 0; i < smt_threads; i++){
    m_transactionLevel[i]  = 0;
    m_xid[i]               = 0;  
    m_tid[i]               = -1;
    m_escapeDepth[i]       = 0;
    m_inLoggedException[i] = 0;
    m_handlerAddress[i]    = Address(0);

    m_processingTrap[i]     = false;
    m_shouldTrap[i]         = false;
    m_trapTakeCheckpoint[i] = false;
    m_trapType[i]           = 0;
    m_trapAddress[i]        = Address(0);
    m_trapPC[i]             = Address(0);
    m_trapTime[i]           = 0;
    m_ignoreWatchpointFlag[i] = false;
    m_continueCheckpoint[i] = new RegisterState(getLogicalProcID(i));

    m_numLoadOverflows[i]   = 0;
    m_numStoreOverflows[i]  = 0;
    m_numLoadMisses[i]      = 0;
    m_numStoreMisses[i]     = 0;
    m_xactBeginCycle[i]     = 0;
    m_xactInstrCount[i]     = 0;
    m_xactLoadAddressMap_ptr[i]   = new Map <physical_address_t, physical_address_t>;
    if (ATMTP_ENABLED) {
      m_xactRockManager[i] = new RockTransactionManager(chip_ptr, version, i);
    }
  }            
  
}

TransactionInterfaceManager::~TransactionInterfaceManager() {
}

void TransactionInterfaceManager::setVersion(int version) {
  m_version = version;
  m_xactVersionManager->setVersion(version);
  m_xactIsolationManager->setVersion(version);
  m_xactConflictManager->setVersion(version);
}

int TransactionInterfaceManager::getVersion() const {
  return m_version;
}

int TransactionInterfaceManager::getProcID() const{
  return (m_chip_ptr->getID()*RubyConfig::numberOfProcsPerChip() + m_version);
}

int TransactionInterfaceManager::getLogicalProcID(int thread) const{
  return (getProcID() * RubyConfig::numberofSMTThreads() + thread);
}

TransactionIsolationManager* TransactionInterfaceManager::getXactIsolationManager(){
  return m_xactIsolationManager;
}

TransactionVersionManager* TransactionInterfaceManager::getXactVersionManager(){
  return m_xactVersionManager;
}

TransactionConflictManager* TransactionInterfaceManager::getXactConflictManager(){
  return m_xactConflictManager;
}                              
          
LazyTransactionVersionManager* TransactionInterfaceManager::getXactLazyVersionManager(){
  return m_xactLazyVersionManager;
}                              

void TransactionInterfaceManager::beginTransaction(int thread, int xid, bool isOpen){
  assert(thread >= 0);
  
  g_system_ptr->getProfiler()->profileBeginTransaction(getProcID(), m_tid[thread], xid, thread, SIMICS_get_program_counter(getLogicalProcID(thread)), isOpen);

  m_transactionLevel[thread]++;
  
  if (!XACT_LAZY_VM){
    bool result = m_xactVersionManager->beginTransaction(thread);
    if (!result){
      m_transactionLevel[thread]--;
      return;
    }
  } else {
    getXactLazyVersionManager()->beginTransaction(thread);
  }                    
  m_xactIsolationManager->beginTransaction(thread);
  m_xactConflictManager->beginTransaction(thread);

  if (m_transactionLevel[thread] == 1){
    if (ATMTP_ENABLED) {
      m_xactRockManager[thread]->beginTransaction();
    }
    m_xid[thread] = xid;
    m_xactBeginCycle[thread] = g_eventQueue_ptr->getTime();        
    m_xactInstrCount[thread] = SIMICS_get_insn_count(getLogicalProcID(thread));
    g_system_ptr->getXactVisualizer()->moveToXact(getLogicalProcID(thread));
  }            
  
  if (XACT_DEBUG && XACT_DEBUG_LEVEL > 0){
    cout << " " << g_eventQueue_ptr->getTime() << " " << getLogicalProcID(thread) << " " << "[" << getProcID() << "," << thread << "]" << " BEGIN XACT: TID " << m_tid[thread] << " XID " << m_xid[thread] << " XACT_LEVEL: " << m_transactionLevel[thread] << " PC: " << SIMICS_get_program_counter(getLogicalProcID(thread)) << endl; 
  }
    
              
}

void TransactionInterfaceManager::commitTransaction(int thread, int xid, bool isOpen){
  if(m_transactionLevel[thread] < 1){
    cout <<"commitTransaction ERROR NOT IN XACT proc =" << getProcID() << " logical_proc = " << getLogicalProcID(thread) << " xid = " << xid << " isOpen = " << isOpen << " tid = " << m_tid[thread] << " pc = " << SIMICS_get_program_counter(getLogicalProcID(thread)) << " level = " << m_transactionLevel[thread] << " time = " << g_eventQueue_ptr->getTime() << endl;
  }

  assert(m_transactionLevel[thread] >= 1);

  if (m_transactionLevel[thread] == 1){
    isOpen = true;        
    if (ATMTP_ENABLED) {
      m_xactRockManager[thread]->commitTransaction();
    }

    if (XACT_LAZY_VM){
      if (!XACT_EAGER_CD){      
        int owner = g_system_ptr->getXactCommitArbiter()->getTokenOwner();
        if (owner != getLogicalProcID(thread)){
          if (shouldTrap(thread)){
            abortLazyTransaction(thread);
            return;
          }                  
          g_system_ptr->getXactCommitArbiter()->requestCommitToken(getLogicalProcID(thread));
          /* IMP - order of these two operations should not be changed. SIMICS_set_program_counter changes NPC too.
             When executed these two operations occur in reverse order due to stacked callbacks from SIMICS. */
          SIMICS_set_next_program_counter(getLogicalProcID(thread), SIMICS_get_npc(getLogicalProcID(thread)));
          SIMICS_set_program_counter(getLogicalProcID(thread), SIMICS_get_program_counter(getLogicalProcID(thread)));      
          g_system_ptr->getXactVisualizer()->moveToStalled(getLogicalProcID(thread));
          return;      
        } else {
          assert(!shouldTrap(thread));                  
          g_system_ptr->getXactVisualizer()->moveToCommiting(getLogicalProcID(thread));
          getXactLazyVersionManager()->commitTransaction(thread, isOpen);
          return;
        }
      } else {
          if (shouldTrap(thread)){
            abortLazyTransaction(thread);
            return;
          }                  
          g_system_ptr->getXactVisualizer()->moveToCommiting(getLogicalProcID(thread));
          g_system_ptr->getXactCommitArbiter()->requestCommitToken(getLogicalProcID(thread));
          getXactLazyVersionManager()->commitTransaction(thread, isOpen);
          return;
      }                              
    } else {  // !XACT_LAZY_VM
      if (getXactVersionManager()->getNumCommitActions(thread) > 0){
        setTrapCommitActions(thread);
      }
      cycles_t total_cycles = g_eventQueue_ptr->getTime() - m_xactBeginCycle[thread];
      assert(total_cycles > 0);
      g_system_ptr->getProfiler()->profileTransaction(getXactIsolationManager()->getReadSetSize(thread, 1) + getXactIsolationManager()->getWriteSetSize(thread, 1), getXactVersionManager()->getLogSize(thread), getXactIsolationManager()->getReadSetSize(thread, 1),getXactIsolationManager()->getWriteSetSize(thread, 1), m_numLoadOverflows[thread], m_numStoreOverflows[thread], getXactConflictManager()->getNumRetries(thread), total_cycles, false, m_numLoadMisses[thread], m_numStoreMisses[thread], SIMICS_get_insn_count(getLogicalProcID(thread)) - m_xactInstrCount[thread], m_xid[thread]);
      m_numLoadOverflows[thread]  = 0;
      m_numStoreOverflows[thread] = 0; 
      m_numLoadMisses[thread]     = 0;
      m_numStoreMisses[thread]    = 0;
      m_xactInstrCount[thread]    = SIMICS_get_insn_count(getLogicalProcID(thread)); 

      clearLoadAddressMap(thread);

      if (XACT_ENABLE_VIRTUALIZATION_LOGTM_SE) {
        SimicsDriver* simics_interface_ptr = static_cast<SimicsDriver*>(g_system_ptr->getDriver());
				simics_interface_ptr->getHypervisor()->completeTransaction(this);
      }
    }        
    g_system_ptr->getXactVisualizer()->moveToCommiting(getLogicalProcID(thread));
    g_system_ptr->getXactVisualizer()->moveToNonXact(getLogicalProcID(thread));
  }
    
  if (!XACT_LAZY_VM)
    m_xactVersionManager->commitTransaction(thread, isOpen);

  m_xactConflictManager->commitTransaction(thread);        
  m_xactIsolationManager->commitTransaction(thread, isOpen);
  
  m_transactionLevel[thread]--;
  g_system_ptr->getProfiler()->profileCommitTransaction(getProcID(), m_tid[thread], xid, thread, SIMICS_get_program_counter(getLogicalProcID(thread)), isOpen);

  if (XACT_DEBUG && XACT_DEBUG_LEVEL > 0){
    cout << " " << g_eventQueue_ptr->getTime() << " " << getLogicalProcID(thread) << " " << "[" << getProcID() << "," << thread << "]" << " COMMIT XACT: TID " << m_tid[thread] << " XID " << m_xid[thread] << " XACT_LEVEL: " << m_transactionLevel[thread] << " PC: " << SIMICS_get_program_counter(getLogicalProcID(thread)) << endl; 
  }

}

void TransactionInterfaceManager::commitLazyTransaction(int thread){
  assert(XACT_LAZY_VM);
  if (!XACT_EAGER_CD)
    assert(g_system_ptr->getXactCommitArbiter()->getTokenOwner() == getLogicalProcID(thread));
  else  
    assert(g_system_ptr->getXactCommitArbiter()->existCommitTokenRequest(getLogicalProcID(thread)));
  assert(m_transactionLevel[thread] == 1);
        
  cycles_t total_cycles = g_eventQueue_ptr->getTime() - m_xactBeginCycle[thread];
  assert(total_cycles > 0);
  g_system_ptr->getProfiler()->profileTransaction(getXactIsolationManager()->getReadSetSize(thread, 1) + getXactIsolationManager()->getWriteSetSize(thread, 1), getXactVersionManager()->getLogSize(thread), getXactIsolationManager()->getReadSetSize(thread, 1),getXactIsolationManager()->getWriteSetSize(thread, 1), m_numLoadOverflows[thread], m_numStoreOverflows[thread], getXactConflictManager()->getNumRetries(thread), total_cycles, false, m_numLoadMisses[thread], m_numStoreMisses[thread], SIMICS_get_insn_count(getLogicalProcID(thread)) - m_xactInstrCount[thread], m_xid[thread]);
  g_system_ptr->getProfiler()->profileCommitTransaction(getProcID(), m_tid[thread], m_xid[thread], thread, SIMICS_get_program_counter(getLogicalProcID(thread)), true);
  m_numLoadOverflows[thread]  = 0;
  m_numStoreOverflows[thread] = 0; 
  m_numLoadMisses[thread]     = 0;
  m_numStoreMisses[thread]    = 0;
  m_xactInstrCount[thread]    = SIMICS_get_insn_count(getLogicalProcID(thread)); 

  getXactConflictManager()->commitTransaction(thread);
  getXactIsolationManager()->commitTransaction(thread, true);
  if (!XACT_EAGER_CD)
    g_system_ptr->getXactCommitArbiter()->releaseCommitToken(getLogicalProcID(thread));
  else  
    g_system_ptr->getXactCommitArbiter()->removeCommitTokenRequest(getLogicalProcID(thread));
  g_system_ptr->getXactVisualizer()->moveToNonXact(getLogicalProcID(thread));
  
  m_transactionLevel[thread]  = 0; 

  if (XACT_DEBUG && XACT_DEBUG_LEVEL > 0){
    cout << " " << g_eventQueue_ptr->getTime() << " " << getLogicalProcID(thread) << " " << "[" << getProcID() << "," << thread << "]" << " COMMIT XACT: TID " << m_tid[thread] << " XID " << m_xid[thread] << " XACT_LEVEL: " << m_transactionLevel[thread] << " PC: " << SIMICS_get_program_counter(getLogicalProcID(thread)) << endl; 
  }
}  
            
        
void TransactionInterfaceManager::abortTransaction(int thread, int xid){
  setAbortFlag(thread, Address(0));       
}            

void TransactionInterfaceManager::beginEscapeAction(int thread){
  assert(m_escapeDepth[thread] >= 0);
  m_escapeDepth[thread]++;
  if (XACT_DEBUG && XACT_DEBUG_LEVEL > 1){
    cout << " " << g_eventQueue_ptr->getTime() << " " << getLogicalProcID(thread) << " " << "[" << getProcID() << "," << thread << "]" << " Begin ESCAPE ACTION - ESCAPE DEPTH: " << m_escapeDepth[thread] << " PC " << SIMICS_get_program_counter(getLogicalProcID(thread)) << endl; 
  }
}

void TransactionInterfaceManager::endEscapeAction(int thread){
  if(m_escapeDepth[thread] < 1){
    cout << g_eventQueue_ptr->getTime() << " " << getLogicalProcID(thread) << " " << "[" << getProcID() << "," << thread << "]" << " endEscapeAction WARNING escape depth < 1. Depth = " << m_escapeDepth[thread] << endl;
    return;
  }
  assert(m_escapeDepth[thread] >= 1);
  m_escapeDepth[thread]--;
  if (XACT_DEBUG && XACT_DEBUG_LEVEL > 1){
    cout << " " << g_eventQueue_ptr->getTime() << " " << getLogicalProcID(thread) << " " << "[" << getProcID() << "," << thread << "]" << " END ESCAPE ACTION - ESCAPE DEPTH: " << m_escapeDepth[thread] << " PC " << SIMICS_get_program_counter(getLogicalProcID(thread)) << endl; 
  }
}                    

void TransactionInterfaceManager::setLoggedException(int thread){
  m_inLoggedException[thread]++;        
}

void TransactionInterfaceManager::clearLoggedException(int thread){
  m_inLoggedException[thread]--;
}

void TransactionInterfaceManager::registerCompensatingAction(int thread){
  getXactVersionManager()->registerCompensatingAction(thread);
}

void TransactionInterfaceManager::registerCommitAction(int thread){
  getXactVersionManager()->registerCommitAction(thread);
}

void TransactionInterfaceManager::xmalloc(int thread){
  getXactVersionManager()->xmalloc(thread);
}

void TransactionInterfaceManager::setLogBase(int thread){
  getXactVersionManager()->setLogBase(thread);
}

void TransactionInterfaceManager::setHandlerAddress(int thread){
  assert(thread >= 0);
  int proc_no = getLogicalProcID(thread);
  
  int gn = SIMICS_get_register_number(proc_no, "g2");
  uint64 tl = SIMICS_read_register(proc_no, gn);
  m_handlerAddress[thread] = Address((physical_address_t)tl);   
  if (XACT_DEBUG){
    cout << " " << g_eventQueue_ptr->getTime() << " " << getLogicalProcID(thread) << " " << "[" << getProcID() << "," << thread << "]" << " Setting Handler Address: " << m_handlerAddress[thread] << dec << endl; 
  }
}        
                   
void TransactionInterfaceManager::setTID(int thread, int tid){
  int proc_no = getLogicalProcID(thread);
  m_tid[thread] = tid;
  if (XACT_DEBUG){
    cout << " " << g_eventQueue_ptr->getTime() << " " << getLogicalProcID(thread) <<  " " << "[" << getProcID() << "," << thread << "]" << " Setting TID: " << m_tid[thread] << endl; 
  }
}          
                   
int TransactionInterfaceManager::getTransactionLevel(int thread){
  return m_transactionLevel[thread];
} 

int TransactionInterfaceManager::getEscapeDepth(int thread){
  return m_escapeDepth[thread];
}  

int TransactionInterfaceManager::getXID(int thread){
  return m_xid[thread];
} 

int TransactionInterfaceManager::getTID(int thread){
  return m_tid[thread];
}                   

bool TransactionInterfaceManager::inTransaction(int thread){
  return ((m_transactionLevel[thread] > 0) && (m_escapeDepth[thread] == 0));
}
            
bool TransactionInterfaceManager::inLoggedException(int thread){
  return (m_inLoggedException[thread] > 0);  
}

bool TransactionInterfaceManager::shouldTrap(int thread){
  // shouldn't trap in escape actions
  return (m_shouldTrap[thread] && (m_escapeDepth[thread] == 0));
}

int TransactionInterfaceManager::getTrapType(int thread){
  return m_trapType[thread];
}
        
void TransactionInterfaceManager::profileLoad(int thread, Address logicalAddr, Address physicalAddr, int transactionLevel){
  g_system_ptr->getProfiler()->profileLoad(getProcID(), m_tid[thread], m_xid[thread], thread, physicalAddr, logicalAddr, SIMICS_get_program_counter(getLogicalProcID(thread)));
}

void TransactionInterfaceManager::profileStore(int thread, Address logicalAddr, Address physicalAddr, int transactionLevel){
  g_system_ptr->getProfiler()->profileStore(getProcID(), m_tid[thread], m_xid[thread], thread, physicalAddr, logicalAddr, SIMICS_get_program_counter(getLogicalProcID(thread)));
}

void TransactionInterfaceManager::isolateTransactionLoad(int thread, Address logicalAddr, Address physicalAddr, int transactionLevel){
  
  g_system_ptr->getProfiler()->profileLoadTransaction(getProcID(), m_tid[thread], m_xid[thread], thread, physicalAddr, logicalAddr, SIMICS_get_program_counter(getLogicalProcID(thread)));
  logicalAddr.makeLineAddress();
  physicalAddr.makeLineAddress();

  if (XACT_DEBUG && (XACT_DEBUG_LEVEL > 1)){
    cout << g_eventQueue_ptr->getTime() << " " << getLogicalProcID(thread) << " [" << getProcID() << "," << thread << "] ISOLATE XACT LOAD " << "VA: " << logicalAddr << " PA: " << physicalAddr << " XACT LEVEL: " 
         << transactionLevel << " PC = " << SIMICS_get_program_counter(getLogicalProcID(thread)) << endl;        
  }
        
  //addToLoadAddressMap(thread, SIMICS_get_program_counter(getLogicalProcID(thread)).getAddress(), physicalAddr.getAddress());      
  m_xactIsolationManager->addToReadSetPerfectFilter(thread, physicalAddr, transactionLevel);
  m_xactIsolationManager->addToReadSetFilter(thread, physicalAddr);

  g_system_ptr->getXactVisualizer()->moveToXact(getLogicalProcID(thread));
}

void TransactionInterfaceManager::isolateTransactionStore(int thread, Address logicalAddr, Address physicalAddr, int transactionLevel){
  
  g_system_ptr->getProfiler()->profileStoreTransaction(getProcID(), m_tid[thread], m_xid[thread], thread, physicalAddr, logicalAddr, SIMICS_get_program_counter(getLogicalProcID(thread)));
  logicalAddr.makeLineAddress();
  physicalAddr.makeLineAddress();
  
  if (XACT_DEBUG && (XACT_DEBUG_LEVEL > 1)){
    cout << g_eventQueue_ptr->getTime() << " " << getLogicalProcID(thread) << " [" << getProcID() << "," << thread << "] ISOLATE XACT STORE " << physicalAddr << " XACT LEVEL: " 
         << transactionLevel << " PC = " << SIMICS_get_program_counter(getLogicalProcID(thread)) << endl;        
  }

  //updateStorePredictor(thread, physicalAddr.getAddress());      
  m_xactIsolationManager->addToWriteSetPerfectFilter(thread, physicalAddr, transactionLevel);
  m_xactIsolationManager->addToWriteSetFilter(thread, physicalAddr);
    
  if (!XACT_LAZY_VM)        
    g_system_ptr->getXactVisualizer()->moveToXact(getLogicalProcID(thread));
}

void TransactionInterfaceManager::logTransactionStore(int thread, Address logicalAddr, Address physicalAddr, int transactionLevel){
  logicalAddr.makeLineAddress();
  physicalAddr.makeLineAddress();
  m_xactVersionManager->addUndoLogEntry(thread, logicalAddr, physicalAddr);
}  

bool TransactionInterfaceManager::isInReadSetFilter(int thread, Address physicalAddr){
  return m_xactIsolationManager->isInReadSetFilter(thread, physicalAddr);
}          
  
bool TransactionInterfaceManager::isInWriteSetFilter(int thread, Address physicalAddr){
  return m_xactIsolationManager->isInWriteSetFilter(thread, physicalAddr);
}          
 
void TransactionInterfaceManager::setAbortFlag(int thread, Address addr){
    m_shouldTrap[thread]  = true;
    m_trapType[thread]    = m_trapType[thread] | 0x1;
    m_trapAddress[thread] = addr;
    m_trapPC[thread]      = SIMICS_get_program_counter(getLogicalProcID(thread));
    m_trapTime[thread]    = g_eventQueue_ptr->getTime();
    m_trapTakeCheckpoint[thread] = true;
    if (XACT_LAZY_VM){
      if (g_system_ptr->getXactCommitArbiter()->existCommitTokenRequest(getLogicalProcID(thread))){
        g_system_ptr->getXactCommitArbiter()->removeCommitTokenRequest(getLogicalProcID(thread));
      }
    }                    
    g_system_ptr->getXactIsolationChecker()->setAbortingProcessor(getLogicalProcID(thread));  
    if (XACT_DEBUG && XACT_DEBUG_LEVEL > 1){
      cout << g_eventQueue_ptr->getTime() << " " << getLogicalProcID(thread) << " " << "[" << getProcID() << "," << thread << "]" << " SETTING ABORT FLAG " << " ADDR = " << addr << " PC = " << m_trapPC[thread] << " NPC = "<< SIMICS_get_npc(getLogicalProcID(thread)) << endl; 
    }
}

void TransactionInterfaceManager::setTrapTLBMiss(int thread, Address addr){
  m_shouldTrap[thread]  = true;
  m_trapType[thread]    = m_trapType[thread]  | 0x2;
  m_trapAddress[thread] = addr;  
  m_trapTime[thread]    = g_eventQueue_ptr->getTime();
  m_trapTakeCheckpoint[thread] = true;
  if (XACT_DEBUG && XACT_DEBUG_LEVEL > 1){
    cout << g_eventQueue_ptr->getTime() << " " << getLogicalProcID(thread) << " " << "[" << getProcID() << "," << thread << "]" << " SETTING TLB MISS " << " ADDR = " << addr << " PC = " << SIMICS_get_program_counter(getLogicalProcID(thread)) << " NPC = " << SIMICS_get_npc(getLogicalProcID(thread)) << endl; 
    }
}  

void TransactionInterfaceManager::setTrapCommitActions(int thread){
  m_shouldTrap[thread]  = true;
  m_trapType[thread]    = m_trapType[thread]  | 0x4;
  m_trapAddress[thread] = getXactVersionManager()->getLogCommitActionBase(thread);  
  m_trapTime[thread]    = g_eventQueue_ptr->getTime();
  m_trapTakeCheckpoint[thread] = true;
  if (XACT_DEBUG && XACT_DEBUG_LEVEL > 1){
    cout << g_eventQueue_ptr->getTime() << " " << getLogicalProcID(thread) << " " << "[" << getProcID() << "," << thread << "]" << " SETTING COMMIT ACTION " << " PC = " << SIMICS_get_program_counter(getLogicalProcID(thread)) << " NPC = " << SIMICS_get_npc(getLogicalProcID(thread)) << endl; 
  }
}

void TransactionInterfaceManager::setSummaryConflictFlag(int thread, Address addr){
    m_shouldTrap[thread]  = true;
    m_trapType[thread]    = m_trapType[thread] | 0x8;
    m_trapAddress[thread] = addr;
    m_trapPC[thread]      = SIMICS_get_program_counter(getLogicalProcID(thread));
    m_trapTime[thread]    = g_eventQueue_ptr->getTime();
    m_trapTakeCheckpoint[thread] = true;
    if (XACT_DEBUG && XACT_DEBUG_LEVEL > 1){
      cout << " " << g_eventQueue_ptr->getTime() << " " << getLogicalProcID(thread) << " "
						<< "[" << getProcID() << "," << thread << "]" << " SETTING SUMMARY SIGNATURE CONFLICT FLAG "
						<< " ADDR = " << addr << " PC = " << m_trapPC[thread] << endl; 
    }
}

void TransactionInterfaceManager::setInterruptTrap(int thread) {
  if (XACT_LAZY_VM) return;      
  m_shouldTrap[thread]  = true;
  m_trapType[thread]    = m_trapType[thread] | 0x10;
  m_trapAddress[thread] = Address(0);
  m_trapTime[thread]    = g_eventQueue_ptr->getTime();
  m_trapTakeCheckpoint[thread] = true;
  if (XACT_DEBUG && XACT_DEBUG_LEVEL > 1){
    cout << g_eventQueue_ptr->getTime() << " " << getLogicalProcID(thread) << " " << "[" << getProcID() << "," << thread << "]" << " SETTING INTERRUPT TRAP " << endl; 
  }
}  

bool TransactionInterfaceManager::shouldTakeTrapCheckpoint(int thread){
  return m_trapTakeCheckpoint[thread];
}
          
void TransactionInterfaceManager::takeContinueCheckpoint(int thread){    
  m_continueCheckpoint[thread]->takeCheckpoint(getLogicalProcID(thread));
  m_trapTakeCheckpoint[thread] = false;
}  

void TransactionInterfaceManager::dumpXactRegisters(int thread){
  int logical_proc_no = getLogicalProcID(thread);        
  physical_address_t tm_log_base = getXactVersionManager()->getLogBasePhysical(thread).getAddress(); 
  
  g_system_ptr->getDriver()->writePhysicalMemory(logical_proc_no, tm_log_base + s_softwareXactStructMap[TRAP_ADDRESS].offset, m_trapAddress[thread].getAddress(), 4);
  int retries = getXactConflictManager()->getNumRetries(thread);
  if (XACT_NO_BACKOFF){
    if (retries > 1) retries = 0;
  }
  int lowestConflictLevel = getXactConflictManager()->getLowestConflictLevel(thread);
  if (lowestConflictLevel == -1) lowestConflictLevel = 1;          
  g_system_ptr->getDriver()->writePhysicalMemory(logical_proc_no, tm_log_base + s_softwareXactStructMap[NUM_RETRIES].offset, retries, 4); // num_retries
  g_system_ptr->getDriver()->writePhysicalMemory(logical_proc_no, tm_log_base + s_softwareXactStructMap[XACT_LOG_SIZE].offset, getXactVersionManager()->getLogSize(thread), 4);
  g_system_ptr->getDriver()->writePhysicalMemory(logical_proc_no, tm_log_base + s_softwareXactStructMap[XACT_LEVEL].offset, getTransactionLevel(thread), 4);
  g_system_ptr->getDriver()->writePhysicalMemory(logical_proc_no, tm_log_base + s_softwareXactStructMap[XACT_LOWEST_CONFLICT_LEVEL].offset, lowestConflictLevel, 4);
  
  if ((m_trapType[thread] & 0x1) ==  0x1){
    g_system_ptr->getDriver()->writePhysicalMemory(logical_proc_no, tm_log_base + s_softwareXactStructMap[TRAP_TYPE].offset, 1, 4);
  } else if ((m_trapType[thread] & 0x2) ==  0x2){
    g_system_ptr->getDriver()->writePhysicalMemory(logical_proc_no, tm_log_base + s_softwareXactStructMap[TRAP_TYPE].offset, 2, 4);
  } else if ((m_trapType[thread] & 0x4) ==  0x4){
    g_system_ptr->getDriver()->writePhysicalMemory(logical_proc_no, tm_log_base + s_softwareXactStructMap[TRAP_TYPE].offset, 3, 4);
  } else if ((m_trapType[thread] & 0x8) ==  0x8){
    g_system_ptr->getDriver()->writePhysicalMemory(logical_proc_no, tm_log_base + s_softwareXactStructMap[TRAP_TYPE].offset, 4, 4);
  } else if ((m_trapType[thread] & 0x10) ==  0x10){
    g_system_ptr->getDriver()->writePhysicalMemory(logical_proc_no, tm_log_base + s_softwareXactStructMap[TRAP_TYPE].offset, 5, 4);
  }
}            
            
void TransactionInterfaceManager::trapToHandler(int thread){
  int logical_proc_no = getLogicalProcID(thread);
  // We temporarily flip the LSB of PSTATE so that simics can access the program
  // global registers instead of the alternate globals. Note that we are currently
  // in a system trap.
  int pstate_rn_no = SIMICS_get_register_number(logical_proc_no, "pstate");
  uint64 pstate_val = SIMICS_read_register(logical_proc_no, pstate_rn_no);
  SIMICS_write_register(logical_proc_no, pstate_rn_no, (pstate_val ^ 0x1));
  int tid_rn_no = SIMICS_get_register_number(logical_proc_no, "g2");
  SIMICS_write_register(logical_proc_no, tid_rn_no, m_tid[thread]);
  SIMICS_write_register(logical_proc_no, pstate_rn_no, pstate_val);
  
  bool abortTrap = false;
  
  if (OpalInterface::isOpalLoaded()){
    SIMICS_set_pc(logical_proc_no, m_handlerAddress[thread]);
  } else{ 
    SIMICS_write_register(logical_proc_no, SIMICS_get_register_number(logical_proc_no, "tnpc1"), m_handlerAddress[thread].getAddress());
  }                              
  
  dumpXactRegisters(thread);

  if ((m_trapType[thread] & 0x1) ==  0x1){
    m_trapType[thread] = m_trapType[thread] & 0xfffffffe;
    abortTrap = true;
    // Do not release Read Isolation all the way through in case
    // we use partial rollback. 
    //getXactIsolationManager()->releaseReadIsolation(thread);
  } else if ((m_trapType[thread] & 0x2) ==  0x2){
    m_trapType[thread] = m_trapType[thread] & 0xfffffffd;
  } else if ((m_trapType[thread] & 0x4) ==  0x4){
    m_trapType[thread] = m_trapType[thread] & 0xfffffffb;
  } else if ((m_trapType[thread] & 0x8) ==  0x8){
    m_trapType[thread] = m_trapType[thread] & 0xfffffff7;
  } else if ((m_trapType[thread] & 0x10) ==  0x10){
    m_trapType[thread] = m_trapType[thread] & 0xffffffef;
  }
  m_shouldTrap[thread] = false;
  m_processingTrap[thread] = true;

  physical_address_t tm_log_base = getXactVersionManager()->getLogBasePhysical(thread).getAddress(); 
  if (XACT_DEBUG && XACT_DEBUG_LEVEL > 0){
    cout << " " << g_eventQueue_ptr->getTime() << " " << getLogicalProcID(thread) << " [" << getProcID() << "," << thread << "] TRAP TO HANDLER: " << 
    "TID: " << m_tid[thread] << 
    " TRAP_TYPE " << g_system_ptr->getDriver()->readPhysicalMemory(logical_proc_no, tm_log_base + s_softwareXactStructMap[TRAP_TYPE].offset, 4) <<
    " TRAP ADDRESS 0x" << hex << g_system_ptr->getDriver()->readPhysicalMemory(logical_proc_no, tm_log_base + s_softwareXactStructMap[TRAP_ADDRESS].offset, 4) << dec <<
    " NUM_RETRIES " << g_system_ptr->getDriver()->readPhysicalMemory(logical_proc_no, tm_log_base + s_softwareXactStructMap[NUM_RETRIES].offset, 4) <<          
    " LOG_SIZE " << g_system_ptr->getDriver()->readPhysicalMemory(logical_proc_no, tm_log_base + s_softwareXactStructMap[XACT_LOG_SIZE].offset, 4) <<          
    " XACT_LEVEL " << g_system_ptr->getDriver()->readPhysicalMemory(logical_proc_no, tm_log_base + s_softwareXactStructMap[XACT_LEVEL].offset, 4) << 
    " XACT_LOWEST_CONFLICT_LEVEL " << g_system_ptr->getDriver()->readPhysicalMemory(logical_proc_no, tm_log_base + s_softwareXactStructMap[XACT_LOWEST_CONFLICT_LEVEL].offset, 4) << 
    " Handler Address = " << m_handlerAddress[thread] << " PC = " << SIMICS_get_program_counter(getLogicalProcID(thread)) <<          
    endl;
  }
  
  beginEscapeAction(thread); // BEGIN IMPLICIT ESCAPE ACTION
   
  if (abortTrap)  
    g_system_ptr->getXactVisualizer()->moveToAborting(getLogicalProcID(thread));
}

void TransactionInterfaceManager::continueExecution(int thread){
  SIMICS_post_continue_execution(getLogicalProcID(thread));        
}

void TransactionInterfaceManager::continueExecutionCallback(int thread){
  m_processingTrap[thread] = false;
  m_continueCheckpoint[thread]->restoreCheckpoint(getLogicalProcID(thread));
  m_continueCheckpoint[thread]->restorePC(getLogicalProcID(thread));
  if (XACT_DEBUG){
    cout << " " << g_eventQueue_ptr->getTime() << " " << getLogicalProcID(thread) << " " << "[" << getProcID() << "," << thread << "]" << " CONTINUE EXECUTION CALLBACK New PC:" << SIMICS_get_program_counter(getLogicalProcID(thread)) << endl; 
  }
  if (XACT_EAGER_CD && !XACT_LAZY_VM) 
    endEscapeAction(thread);            // END IMPLICIT ESCAPE ACTION
}          
        
void TransactionInterfaceManager::restartTransaction(int thread){
  SIMICS_post_restart_transaction(getLogicalProcID(thread));        
}

void TransactionInterfaceManager::restartTransactionCallback(int thread){
  int logical_proc_no = getLogicalProcID(thread);
  
  if(XACT_DEBUG && XACT_DEBUG_LEVEL > 1){
    cout << "restartTransactionCallback proc = " << getLogicalProcID(thread) << " thread = " << thread << " time = " << g_eventQueue_ptr->getTime() << endl;
  }
  g_system_ptr->getXactIsolationChecker()->clearAbortingProcessor(getLogicalProcID(thread));
  
  if (XACT_EAGER_CD && !XACT_LAZY_VM) 
    endEscapeAction(thread);            // END IMPLICIT ESCAPE ACTION

  m_processingTrap[thread] = false;
  Time trap_time = g_eventQueue_ptr->getTime() - m_trapTime[thread];
  assert(trap_time >= 0);
  g_system_ptr->getProfiler()->profileAbortTransaction(getProcID(), m_tid[thread], m_xid[thread], thread, trap_time, 0 /* aborting thread */, getXactConflictManager()->getEnemyProc(thread), m_trapAddress[thread], m_trapPC[thread]);

  if (XACT_LAZY_VM){
    m_shouldTrap[thread] = false;
    m_transactionLevel[thread] = 0;
    m_numLoadMisses[thread]     = 0;
    m_numStoreMisses[thread]    = 0;
    m_xactInstrCount[thread]    = 0;
    m_xactBeginCycle[thread] = g_eventQueue_ptr->getTime();        
    getXactLazyVersionManager()->restartTransaction(thread, 0);
    getXactConflictManager()->restartTransaction(thread);
    if (ATMTP_ENABLED) {
      m_xactRockManager[thread]->restoreFailPC();
    }
    if (!XACT_NO_BACKOFF){
      int backoff =  random() % ((getXactConflictManager()->getNumRetries(thread) + 1) * 100);
      //cout << "BACKOFF for proc " << getLogicalProcID(thread) << " " << backoff << " RETRIES = " << getXactConflictManager()->getNumRetries(thread) << endl;

      SIMICS_post_stall_proc(getLogicalProcID(thread), backoff);
      g_system_ptr->getXactVisualizer()->moveToBackoff(getLogicalProcID(thread));
    }           
  } else {         
    physical_address_t tm_log_base = getXactVersionManager()->getLogBasePhysical(thread).getAddress(); 
    int new_xact_level = g_system_ptr->getDriver()->readPhysicalMemory(logical_proc_no, tm_log_base + s_softwareXactStructMap[XACT_LEVEL].offset, 4);
    int new_log_size   = g_system_ptr->getDriver()->readPhysicalMemory(logical_proc_no, tm_log_base + s_softwareXactStructMap[XACT_LOG_SIZE].offset, 4);
    int tid            = g_system_ptr->getDriver()->readPhysicalMemory(logical_proc_no, tm_log_base + s_softwareXactStructMap[THREADID].offset, 4);
    if (XACT_DEBUG && XACT_DEBUG_LEVEL > 1){
      cout << logical_proc_no << " [" << getProcID() << "," << thread << "]" << " TID " << tid << " RESTART TRANSACTION AT XACT LEVEL: " <<  new_xact_level  
            << " LOG_SIZE: " << new_log_size << endl; 
    }
  
    m_transactionLevel[thread] = new_xact_level; 
    if (new_xact_level == 0){
      m_numLoadMisses[thread]     = 0;
      m_numStoreMisses[thread]    = 0;
      m_xactInstrCount[thread]    = 0;
      m_xactBeginCycle[thread] = g_eventQueue_ptr->getTime();        
    }  

    getXactVersionManager()->restartTransaction(thread, new_xact_level);
    getXactConflictManager()->restartTransaction(thread);
  }  
}

void TransactionInterfaceManager::releaseIsolation(int thread){
  int logical_proc_no = getLogicalProcID(thread);      
  int xact_level = SIMICS_read_register(logical_proc_no, SIMICS_get_register_number(logical_proc_no, "g2"));
  assert(xact_level > 0);
  if (XACT_DEBUG && XACT_DEBUG_LEVEL > 1){
    cout << logical_proc_no << " " << "[" << getProcID() << "," << thread << "]" << " RELEASE ISOLATION AT XACT LEVEL: " <<  xact_level << endl; 
  }

  getXactIsolationManager()->releaseIsolation(thread, xact_level);
  if (xact_level == 1){
    if (XACT_ENABLE_VIRTUALIZATION_LOGTM_SE) {
      SimicsDriver* simics_interface_ptr = static_cast<SimicsDriver*>(g_system_ptr->getDriver());
			simics_interface_ptr->getHypervisor()->completeTransaction(this);
    }

    if (m_trapType[thread] ==  0x1){
      m_trapType[thread] = 0x0;
      m_shouldTrap[thread] = false;
    }
    if(!XACT_NO_BACKOFF){
      g_system_ptr->getXactVisualizer()->moveToBackoff(getLogicalProcID(thread));
    }    
  }    
}

void TransactionInterfaceManager::abortLazyTransaction(int thread){
  assert(XACT_LAZY_VM);
  g_system_ptr->getXactIsolationChecker()->setAbortingProcessor(getLogicalProcID(thread));  
  for (int i = m_transactionLevel[thread]; i > 0; i--)
    getXactIsolationManager()->releaseIsolation(thread, i);
  g_system_ptr->getXactVisualizer()->moveToAborting(getLogicalProcID(thread));
  restartTransaction(thread);
  if(XACT_DEBUG && (XACT_DEBUG_LEVEL >= 2)){
    cout << g_eventQueue_ptr->getTime() << " " << getLogicalProcID(thread) << " ABORT LAZY TRANSACTION DONE " << endl;
  }
  
}            

bool TransactionInterfaceManager::shouldUseHardwareAbort(int thread){
  if (ATMTP_ENABLED) {
    return true;
  } 

  if (XACT_EAGER_CD && !XACT_LAZY_VM){      
    if ((m_trapType[thread] & 0x1) == 0x1){
      if (getXactVersionManager()->getLogSize(thread) < XACT_LOG_BUFFER_SIZE){
        return true;
      }
      return false;
    }
    return false;
  }  
  return false;
}                        
          
                  
void TransactionInterfaceManager::hardwareAbort(int thread){
  int logical_proc_no = getLogicalProcID(thread);      
  physical_address_t tm_log_base = getXactVersionManager()->getLogBasePhysical(thread).getAddress(); 
  dumpXactRegisters(thread);      
  
  if ((m_trapType[thread] & 0x1) ==  0x1){
    m_trapType[thread] = m_trapType[thread] & 0xfffffffe;
    getXactIsolationManager()->releaseReadIsolation(thread);
  } else if ((m_trapType[thread] & 0x2) ==  0x2){
    m_trapType[thread] = m_trapType[thread] & 0xfffffffd;
  } else if ((m_trapType[thread] & 0x4) ==  0x4){
    m_trapType[thread] = m_trapType[thread] & 0xfffffffb;
  }
  m_shouldTrap[thread] = false;

  if (XACT_DEBUG && XACT_DEBUG_LEVEL > 0){
    cout << " " << g_eventQueue_ptr->getTime() << " " << getLogicalProcID(thread) << " [" << getProcID() << "," << thread << "] HARDWARE ROLLBACK: " << 
    "TID: " << m_tid[thread] << 
    " NUM_RETRIES " << g_system_ptr->getDriver()->readPhysicalMemory(logical_proc_no, tm_log_base + s_softwareXactStructMap[NUM_RETRIES].offset, 4) <<          
    " LOG_SIZE " << g_system_ptr->getDriver()->readPhysicalMemory(logical_proc_no, tm_log_base + s_softwareXactStructMap[XACT_LOG_SIZE].offset, 4) <<          
    " XACT_LEVEL " << g_system_ptr->getDriver()->readPhysicalMemory(logical_proc_no, tm_log_base + s_softwareXactStructMap[XACT_LEVEL].offset, 4) <<          
    endl;
  }
  
  getXactVersionManager()->hardwareRollback(thread);      
  
  if (XACT_DEBUG && XACT_DEBUG_LEVEL > 2){
    cout << " " << g_eventQueue_ptr->getTime() << " " << getLogicalProcID(thread) << " [" << getProcID() << "," << thread << "] HARDWARE ROLLBACK DONE: " << 
    "TID: " << m_tid[thread] << 
    " NUM_RETRIES " << g_system_ptr->getDriver()->readPhysicalMemory(logical_proc_no, tm_log_base + s_softwareXactStructMap[NUM_RETRIES].offset, 4) <<          
    " LOG_SIZE " << g_system_ptr->getDriver()->readPhysicalMemory(logical_proc_no, tm_log_base + s_softwareXactStructMap[XACT_LOG_SIZE].offset, 4) <<          
    " XACT_LEVEL " << g_system_ptr->getDriver()->readPhysicalMemory(logical_proc_no, tm_log_base + s_softwareXactStructMap[XACT_LEVEL].offset, 4) <<          
    endl;
  }
  
  restartTransaction(thread);
  getXactConflictManager()->setMagicWait(thread);

  g_system_ptr->getXactVisualizer()->moveToAborting(getLogicalProcID(thread));
}        


bool TransactionInterfaceManager::existLoadConflict(Address addr){ /* DEPRECATED */
  if (XACT_EAGER_CD && !XACT_LAZY_VM){
    return getXactIsolationManager()->isInWriteSetFilterSummary(addr);    
  } else {
    assert(false);      
  }        
}

bool TransactionInterfaceManager::existStoreConflict(Address addr){   /* DEPRECATED */  
  if (XACT_EAGER_CD && !XACT_LAZY_VM){
    return (getXactIsolationManager()->isInWriteSetFilterSummary(addr)
            || getXactIsolationManager()->isInReadSetFilterSummary(addr));    
  } else {
    assert(false);      
  }        
}

bool TransactionInterfaceManager::shouldNackLoad(Address addr, uint64 remote_timestamp, MachineID remote_id){
  return getXactConflictManager()->shouldNackLoad(addr, remote_timestamp, remote_id);
}

bool TransactionInterfaceManager::shouldNackStore(Address addr, uint64 remote_timestamp, MachineID remote_id){
  return getXactConflictManager()->shouldNackStore(addr, remote_timestamp, remote_id);
}

bool TransactionInterfaceManager::checkReadWriteSignatures(Address addr){
  if (XACT_EAGER_CD){
    return (getXactIsolationManager()->isInReadSetFilterSummary(addr) 
            || getXactIsolationManager()->isInWriteSetFilterSummary(addr));
  } else {
    if (isTokenOwner(0)){
      return (getXactIsolationManager()->isInReadSetFilterSummary(addr) || 
            getXactIsolationManager()->isInWriteSetFilterSummary(addr));              
    } else {  
      return getXactIsolationManager()->isInReadSetFilterSummary(addr);
    }  
  }
}    
                             
bool TransactionInterfaceManager::checkWriteSignatures(Address addr){
  if (XACT_EAGER_CD){
    return (getXactIsolationManager()->isInWriteSetFilterSummary(addr)); 
  } else {
    if (isTokenOwner(0)){
      return getXactIsolationManager()->isInWriteSetFilterSummary(addr);              
    } else {  
      return false;
    }  
  }
}    
                             
uint64 TransactionInterfaceManager::getTimestamp(int thread){
  return getXactConflictManager()->getTimestamp(thread);
}

uint64 TransactionInterfaceManager::getOldestTimestamp(){
  return getXactConflictManager()->getOldestTimestamp();
}                    

int TransactionInterfaceManager::getOldestThreadExcludingThread(int thread){
  return getXactConflictManager()->getOldestThreadExcludingThread(thread);
}                    

void TransactionInterfaceManager::setPossibleCycle(int thread){
  getXactConflictManager()->setPossibleCycle(thread);
}                    

bool TransactionInterfaceManager::possibleCycle(int thread){
  return getXactConflictManager()->possibleCycle(thread);
}                    

bool TransactionInterfaceManager::existGlobalLoadConflict(int thread, Address addr){
  int num_processors = RubyConfig::numberOfProcessors();
  bool conflict = false;
  for (int i = 0; i < num_processors; i++){
    if (i != getProcID()){      
      TransactionInterfaceManager *remote_manager = g_system_ptr->getChip(i / RubyConfig::numberOfProcsPerChip())->getTransactionInterfaceManager(i % RubyConfig::numberOfProcsPerChip());
      if (remote_manager->shouldNackLoad(addr, getTimestamp(thread), getL1MachineID(getProcID()))){
        if (0 && XACT_DEBUG && XACT_DEBUG_LEVEL > 1){
          cout << " " << g_eventQueue_ptr->getTime() << " " << getLogicalProcID(thread) << " " << "[" << getProcID() << "," << thread << "]" << " existGlobalLoadConflict: CONFLICT WITH " << i << " on ADD: " << addr << " " << remote_manager->existLoadConflict(addr) << endl; 
        }
        conflict = true;        
        remote_manager->notifySendNack(addr, getTimestamp(thread), getL1MachineID(getProcID()));
        notifyReceiveNack(thread, addr, getTimestamp(thread), remote_manager->getOldestTimestamp(), getL1MachineID(i));
      }
    }    
  }
  if (conflict) {
    notifyReceiveNackFinal(thread, addr);        
  }        
  return conflict;                
}          

bool TransactionInterfaceManager::existGlobalStoreConflict(int thread, Address addr){
  int num_processors = RubyConfig::numberOfProcessors();
  bool conflict = false;
  for (int i = 0; i < num_processors; i++){
    if (i != getProcID()){       
      TransactionInterfaceManager *remote_manager = g_system_ptr->getChip(i / RubyConfig::numberOfProcsPerChip())->getTransactionInterfaceManager(i % RubyConfig::numberOfProcsPerChip());
      if (remote_manager->shouldNackStore(addr, getTimestamp(thread), getL1MachineID(getProcID()))){
        conflict = true;       
        remote_manager->notifySendNack(addr, getTimestamp(thread), getL1MachineID(getProcID()));
        notifyReceiveNack(thread, addr, getTimestamp(thread), remote_manager->getOldestTimestamp(), getL1MachineID(i));
        if (0 && XACT_DEBUG && XACT_DEBUG_LEVEL > 1){
          cout << " " << g_eventQueue_ptr->getTime() << " " << getLogicalProcID(thread) << " " << "[" << getProcID() << "," << thread << "]" << " existGlobalStoreConflict: CONFLICT WITH " << i << " on ADD: " << addr << " " << remote_manager->existStoreConflict(addr) << endl; 
        }
      }
    }    
  }
  if (conflict) {
    notifyReceiveNackFinal(thread, addr);        
  }        
  return conflict;                
}          

bool TransactionInterfaceManager::checkGlobalLoadConflict(int thread, Address addr){
  int num_processors = RubyConfig::numberOfProcessors();
  bool conflict = false;
  for (int i = 0; i < num_processors; i++){
    if (i == getProcID()) continue;      
    TransactionInterfaceManager *remote_manager = g_system_ptr->getChip(i / RubyConfig::numberOfProcsPerChip())->getTransactionInterfaceManager(i % RubyConfig::numberOfProcsPerChip());
    if (remote_manager->existLoadConflict(addr)){
      conflict = true;        
      if (0 && XACT_DEBUG){
        cout << " " << g_eventQueue_ptr->getTime() << " " << getLogicalProcID(thread) << " " << "[" << getProcID() << "," << thread << "]" << " GlobalLoadConflictCheck: CONFLICT WITH " << i << " on ADD: " << addr << endl; 
      }
    }    
  }
  return conflict;                
}          

bool TransactionInterfaceManager::checkGlobalStoreConflict(int thread, Address addr){
  int num_processors = RubyConfig::numberOfProcessors();
  bool conflict = false;
  for (int i = 0; i < num_processors; i++){
    if (i == getProcID()) continue;      
    TransactionInterfaceManager *remote_manager = g_system_ptr->getChip(i / RubyConfig::numberOfProcsPerChip())->getTransactionInterfaceManager(i % RubyConfig::numberOfProcsPerChip());
    if (remote_manager->existStoreConflict(addr)){
      conflict = true;        
      if (0 && XACT_DEBUG){
        cout << " " << g_eventQueue_ptr->getTime() << " " << getLogicalProcID(thread) << " " << "[" << getProcID() << "," << thread << "]" << " GlobalStoreConflictCheck: CONFLICT WITH " << i << " on ADD: " << addr << endl; 
      }
    }    
  }
  return conflict;                
}          

void TransactionInterfaceManager::notifyReceiveNack(int thread, Address addr, uint64 local_timestamp, uint64 remote_timestamp, MachineID remote_id){
  // get the remote thread that NACKed us
  int remote_procnum = L1CacheMachIDToProcessorNum(remote_id);
  TransactionConflictManager * remote_conflict_mgr = g_system_ptr->getChip(m_chip_ptr->getID())->getTransactionInterfaceManager(remote_procnum)->getXactConflictManager();
  int    remote_thread   = remote_conflict_mgr->getOldestThread(); 
  int remote_logicalprocnum = remote_procnum*RubyConfig::numberofSMTThreads()+remote_thread;

  getXactConflictManager()->notifyReceiveNack(thread, addr, local_timestamp, remote_timestamp, remote_id);
  g_system_ptr->getProfiler()->profileNack(getProcID(), m_tid[thread], m_xid[thread], thread, remote_thread /* remote_thread */, L1CacheMachIDToProcessorNum(remote_id), addr, Address(0) /* Logical_address */, SIMICS_get_program_counter(getLogicalProcID(thread)), local_timestamp, remote_timestamp, getXactConflictManager()->possibleCycle(thread));
}

void TransactionInterfaceManager::notifyReceiveNackFinal(int thread, Address addr){
  getXactConflictManager()->notifyReceiveNackFinal(thread, addr);
  if (getTransactionLevel(thread) > 0)      
    g_system_ptr->getXactVisualizer()->moveToStalled(getLogicalProcID(thread));
}

void TransactionInterfaceManager::notifySendNack(Address addr, uint64 remote_timestamp, MachineID remote_id){
  getXactConflictManager()->notifySendNack(addr, remote_timestamp, remote_id);
}

void TransactionInterfaceManager::profileOverflow(Address addr, bool l1_overflow){
    for(int thread=0; thread < RubyConfig::numberofSMTThreads(); ++thread ){
      bool is_read = getXactIsolationManager()->isInReadSetPerfectFilter(thread, addr);
      bool is_written = getXactIsolationManager()->isInWriteSetPerfectFilter(thread, addr);
      
      if(is_written){
        g_system_ptr->getProfiler()->profileStoreOverflow(getProcID(), m_tid[thread], m_xid[thread], thread, addr, l1_overflow);
        m_numStoreOverflows[thread]++;
      } else if (is_read){ 
        g_system_ptr->getProfiler()->profileLoadOverflow(getProcID(), m_tid[thread], m_xid[thread], thread, addr, l1_overflow);
        m_numLoadOverflows[thread]++;
      }
    }     
}

void TransactionInterfaceManager::profileTransactionMiss(int thread, bool load){
  if ((getTransactionLevel(thread) == 0) || (m_escapeDepth[thread] > 0)) 
    return;
  if (load)          
    m_numLoadMisses[thread]++;
  else
    m_numStoreMisses[thread]++;
}                      

bool TransactionInterfaceManager::isInReadFilterSummary(Address addr){
  return getXactIsolationManager()->isInReadSetFilterSummary(addr);
}          

bool TransactionInterfaceManager::isInWriteFilterSummary(Address addr){
  return getXactIsolationManager()->isInWriteSetFilterSummary(addr);
}          

bool TransactionInterfaceManager::isTokenOwner(int thread){
  assert(XACT_LAZY_VM);
  if (XACT_EAGER_CD){
    return g_system_ptr->getXactCommitArbiter()->existCommitTokenRequest(getLogicalProcID(thread));
  } else {          
    return (g_system_ptr->getXactCommitArbiter()->getTokenOwner() == getLogicalProcID(thread));
  }  
}    

bool TransactionInterfaceManager::isRemoteOlder(uint64 remote_timestamp){
  return (remote_timestamp < getOldestTimestamp());
}                                  
               
bool TransactionInterfaceManager::magicWait(int thread){
  return getXactConflictManager()->magicWait(thread);
}

bool TransactionInterfaceManager::shouldUpgradeLoad(physical_address_t curr_pc){
  if (!XACT_EAGER_CD) return false;      
  //return m_xactStorePredictorPC_ptr->exist(curr_pc);
  bool upgrade = false;
  if (m_xactStorePredictorPC_ptr->exist(curr_pc)){
    if(m_xactStorePredictorPC_ptr->lookup(curr_pc) > XACT_STORE_PREDICTOR_THRESHOLD){
      upgrade = true;
    }
  }
  return upgrade;
}

void TransactionInterfaceManager::addToLoadAddressMap(int thread, physical_address_t curr_pc, physical_address_t load_addr){
  if(!m_xactLoadAddressMap_ptr[thread]->exist(load_addr)){
    if (m_xactLoadAddressMap_ptr[thread]->size() < XACT_STORE_PREDICTOR_HISTORY){
      m_xactLoadAddressMap_ptr[thread]->add(load_addr, curr_pc);
    }                  
  }        
}

void TransactionInterfaceManager::updateStorePredictor(int thread, physical_address_t store_addr){
  if (m_xactLoadAddressMap_ptr[thread]->exist(store_addr)){
    physical_address_t pc = m_xactLoadAddressMap_ptr[thread]->lookup(store_addr);
    if(!m_xactStorePredictorPC_ptr->exist(pc)){
      if (m_xactStorePredictorPC_ptr->size() < XACT_STORE_PREDICTOR_ENTRIES){
        m_xactStorePredictorPC_ptr->add(pc, 1);
      } // predictor is full
    } else {
      (m_xactStorePredictorPC_ptr->lookup(pc))++;
    }
    // remove this so it's not repeatedly updated or decremented on commit
    (m_xactLoadAddressMap_ptr[thread]->lookup(store_addr)) = 0; 
  }
}                            

void TransactionInterfaceManager::clearLoadAddressMap(int thread){  
  Vector<physical_address_t> keys = m_xactLoadAddressMap_ptr[thread]->keys();
  for(int i=0; i<keys.size(); i++) {
    physical_address_t pc = m_xactLoadAddressMap_ptr[thread]->lookup(keys[i]);
    
    if(m_xactStorePredictorPC_ptr->exist(pc)){
      (m_xactStorePredictorPC_ptr->lookup(pc))--;
      if(m_xactStorePredictorPC_ptr->lookup(pc) == 0){
        m_xactStorePredictorPC_ptr->remove(pc);
      }
    }
  }
  m_xactLoadAddressMap_ptr[thread]->clear();
}
                         
void TransactionInterfaceManager::addToSummaryReadSetFilter(int thread, Address addr) {
  if(!PERFECT_SUMMARY_FILTER) {
    m_xactIsolationManager->addToSummaryReadSetFilter(thread, addr);
  } else {
    m_xactIsolationManager->addToSummaryReadSetPerfectFilter(thread, addr);
  }
}

void TransactionInterfaceManager::addToSummaryWriteSetFilter(int thread, Address addr) {
  if(!PERFECT_SUMMARY_FILTER) {
    m_xactIsolationManager->addToSummaryWriteSetFilter(thread, addr);
  } else {
    m_xactIsolationManager->addToSummaryWriteSetPerfectFilter(thread, addr);
  }
}

void TransactionInterfaceManager::removeFromSummaryReadSetFilter(int thread, Address addr) {
  if(!PERFECT_SUMMARY_FILTER) {
    m_xactIsolationManager->removeFromSummaryReadSetFilter(thread, addr);
  } else {
    m_xactIsolationManager->removeFromSummaryReadSetPerfectFilter(thread, addr);
  }
}

void TransactionInterfaceManager::removeFromSummaryWriteSetFilter(int thread, Address addr) {
  if(!PERFECT_SUMMARY_FILTER) {
    m_xactIsolationManager->removeFromSummaryWriteSetFilter(thread, addr);
  } else {
    m_xactIsolationManager->removeFromSummaryWriteSetPerfectFilter(thread, addr);
  }
}

bool TransactionInterfaceManager::isInSummaryReadSetFilter(int thread, Address addr) {
  return m_xactIsolationManager->isInSummaryReadSetFilter(thread, addr);
}

bool TransactionInterfaceManager::isInSummaryWriteSetFilter(int thread, Address addr) {
  return m_xactIsolationManager->isInSummaryWriteSetFilter(thread, addr);
}

int TransactionInterfaceManager::readBitSummaryReadSetFilter(int thread, int index){
   return m_xactIsolationManager->readBitSummaryReadSetFilter(thread, index) ;
}

void TransactionInterfaceManager::writeBitSummaryReadSetFilter(int thread, int index, int value){
   m_xactIsolationManager->writeBitSummaryReadSetFilter(thread, index, value) ;
}

int TransactionInterfaceManager::readBitSummaryWriteSetFilter(int thread, int index){
   return m_xactIsolationManager->readBitSummaryWriteSetFilter(thread, index) ;
}

void TransactionInterfaceManager::writeBitSummaryWriteSetFilter(int thread, int index, int value){
   m_xactIsolationManager->writeBitSummaryWriteSetFilter(thread, index, value) ;
}

int TransactionInterfaceManager::getIndexSummaryFilter(int thread, Address addr) {
  return m_xactIsolationManager->getIndexSummaryFilter(thread, addr);
} 

bool TransactionInterfaceManager::ignoreWatchpointFlag(int thread) {
  return m_ignoreWatchpointFlag[thread];  
} 

void TransactionInterfaceManager::setIgnoreWatchpointFlag(int thread, bool value) {
  m_ignoreWatchpointFlag[thread] = value;  
} 

void TransactionInterfaceManager::setRestorePC(int thread, unsigned int pc) {
  m_xactVersionManager->setRestorePC(thread, m_transactionLevel[thread]-1, pc);
}  

void TransactionInterfaceManager::printConfig(ostream& out){
}        


//------------- ATMTP -------------------


void TransactionInterfaceManager::setFailPC(int thread, Address fail_pc){
  m_xactRockManager[thread]->setFailPC(fail_pc);
}

void TransactionInterfaceManager::setCPS(int thread, uint32 cps){
  m_xactRockManager[thread]->setCPS(cps);
}
void TransactionInterfaceManager::setCPS_exog(int thread){
  m_xactRockManager[thread]->setCPS_exog();
}
void TransactionInterfaceManager::setCPS_coh(int thread){
  m_xactRockManager[thread]->setCPS_coh();
}
void TransactionInterfaceManager::setCPS_tcc(int thread){
  m_xactRockManager[thread]->setCPS_tcc();
}
void TransactionInterfaceManager::setCPS_inst(int thread){
  m_xactRockManager[thread]->setCPS_inst();
}
void TransactionInterfaceManager::setCPS_prec(int thread){
  m_xactRockManager[thread]->setCPS_prec();
}
void TransactionInterfaceManager::setCPS_async(int thread){
  m_xactRockManager[thread]->setCPS_async();
}
void TransactionInterfaceManager::setCPS_size(int thread){
  m_xactRockManager[thread]->setCPS_size();
}
void TransactionInterfaceManager::setCPS_ld(int thread){
  m_xactRockManager[thread]->setCPS_ld();
}
void TransactionInterfaceManager::setCPS_st(int thread){
  m_xactRockManager[thread]->setCPS_st();
}
void TransactionInterfaceManager::setCPS_cti(int thread){
  m_xactRockManager[thread]->setCPS_cti();
}
void TransactionInterfaceManager::setCPS_fp(int thread){
  m_xactRockManager[thread]->setCPS_fp();
}
uint32 TransactionInterfaceManager::getCPS(int thread){
  return m_xactRockManager[thread]->getCPS();
}
void TransactionInterfaceManager::clearCPS(int thread){
  m_xactRockManager[thread]->clearCPS();
}

//
// Called when a save instruction is executed within a
// transaction. Returns true if the processor should abort.  As of
// 2007-11-12, we hear rock never aborts on a save (only on a restore
// that follows a save).
//
bool TransactionInterfaceManager::saveInstShouldAbort(int thread){
  return m_xactRockManager[thread]->saveInstShouldAbort();
}

//
// Called when a restore instruction is executed within a transaction.
// Returns true if the processor should abort its transaction. As of
// 2007-11-12, we hear that Rock aborts on a restore if it will
// restore values saved during the transaction.
//
bool TransactionInterfaceManager::restoreInstShouldAbort(int thread){
  return m_xactRockManager[thread]->restoreInstShouldAbort();
}

bool TransactionInterfaceManager::inRetryHACK(int thread){
  return m_xactRockManager[thread]->inRetryHACK();
}

void TransactionInterfaceManager::setInRetryHACK(int thread){
  m_xactRockManager[thread]->setInRetryHACK();
}

void TransactionInterfaceManager::clearInRetryHACK(int thread){
  m_xactRockManager[thread]->clearInRetryHACK();
}

void TransactionInterfaceManager::restoreFailPC(int thread){
  m_xactRockManager[thread]->restoreFailPC();
}

void TransactionInterfaceManager::xactReplacement(Address addr){
  // add profiling code w/ address
  if (ATMTP_ENABLED) {
    setCPS_ld(0);
    Address pc = SIMICS_get_program_counter(getProcID());
    g_system_ptr->getProfiler()->profileTransactionCacheOverflow(getProcID(), addr, pc);
    setAbortFlag(0, addr);
  }
}
