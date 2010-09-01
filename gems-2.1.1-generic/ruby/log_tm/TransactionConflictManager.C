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

#include "TransactionConflictManager.h"
#include "TransactionInterfaceManager.h"
#include "TransactionIsolationManager.h"
#include "RubySlicc_ComponentMapping.h"
#include "interface.h"
#include "OpalInterface.h"
#include "XactVisualizer.h"
#include "XactIsolationChecker.h"

#include <assert.h>
#include <stdlib.h>

#define SEQUENCER g_system_ptr->getChip(m_chip_ptr->getID())->getSequencer(m_version)
#define XACT_MGR g_system_ptr->getChip(m_chip_ptr->getID())->getTransactionInterfaceManager(m_version)

TransactionConflictManager::TransactionConflictManager(AbstractChip* chip_ptr, int version) {
  int smt_threads = RubyConfig::numberofSMTThreads();
  m_chip_ptr = chip_ptr;
  m_version = version;

  m_timestamp       = new uint64[smt_threads];
  m_lock_timestamp  = new bool[smt_threads];
  m_possible_cycle  = new bool[smt_threads];
  m_lowestConflictLevel = new int[smt_threads];
  
  /*
  m_max_waiting_ts  = new uint64[smt_threads];
  m_waiting_proc    = new int[smt_threads];
  m_waiting_thread  = new int[smt_threads];
  m_delay_thread    = new bool[smt_threads];
  */
  
  m_shouldTrap      = new bool[smt_threads];
  m_enemyProc       = new int[smt_threads];
  m_numRetries      = new int[smt_threads];
  m_nacked          = new bool[smt_threads];
  m_nackedBy        = new bool*[smt_threads];
  m_nackedByTimestamp = new uint64*[smt_threads];

  m_needMagicWait      = new bool[smt_threads];
  m_enableMagicWait    = new bool[smt_threads];
  m_magicWaitingOn     = new int[smt_threads];
  m_magicWaitingTime   = new Time[smt_threads];

  for (int i = 0; i < smt_threads; i++){
    m_timestamp[i]      = 0;
    m_possible_cycle[i] = false;
    m_lock_timestamp[i] = false;
    m_lowestConflictLevel[i] = -1;
    /*
    m_max_waiting_ts[i] = 0;
    m_waiting_proc[i]   = 0;
    m_waiting_thread[i] = 0;
    m_delay_thread[i]   = false;
    */
    m_shouldTrap[i]         = false;
    m_enemyProc[i]          = 0;
    m_numRetries[i]         = 0;
    m_nacked[i]             = false;
    m_needMagicWait[i]      = false;
    m_enableMagicWait[i]    = false;
    m_magicWaitingOn[i]     = 0;
    m_magicWaitingTime[i]   = 0;
    m_nackedBy[i]           = new bool[RubyConfig::numberOfProcessors()];
    m_nackedByTimestamp[i]  = new uint64[RubyConfig::numberOfProcessors()];
    for (int j = 0; j < RubyConfig::numberOfProcessors(); j++){
      m_nackedBy[i][j]      = false;      
    }    
  }
           
}

TransactionConflictManager::~TransactionConflictManager() {
}

void TransactionConflictManager::setVersion(int version) {
  m_version = version;
}

int TransactionConflictManager::getVersion() const {
  return m_version;
}

int TransactionConflictManager::getProcID() const{
  return m_chip_ptr->getID()*RubyConfig::numberOfProcsPerChip()+m_version;
}

int TransactionConflictManager::getLogicalProcID(int thread) const{
  return getProcID() * RubyConfig::numberofSMTThreads() + thread;        
}

bool TransactionConflictManager::isRemoteOlder(int thread, int remote_thread, uint64 local_timestamp, uint64 remote_timestamp, MachineID remote_id){
  assert(local_timestamp != 0);
  assert(remote_timestamp != 0);
  /*
  if(remote_timestamp == 0){
    int remote_processor = L1CacheMachIDToProcessorNum(remote_id);
    int remote_chip      = remote_processor / RubyConfig::numberOfProcsPerChip();
    int remote_chip_ver  = remote_processor % RubyConfig::numberOfProcsPerChip();       
    cout << " " << g_eventQueue_ptr->getTime() << " " << getLogicalProcID(thread) << " [" << getProcID() << "," << thread << "] REMOTE_TIMESTAMP is 0 remote_id: " << remote_processor << endl;
    TransactionInterfaceManager* remote_mgr = g_system_ptr->getChip(remote_chip)->getTransactionInterfaceManager(remote_chip_ver);
    for (int i = 0; i < RubyConfig::numberofSMTThreads(); i++)
      cout << "[ " << i << " XACT_LEVEL: " << remote_mgr->getTransactionLevel(i) << " TIMESTAMP: " << remote_mgr->getXactConflictManager()->getTimestamp(i) << "] ";         
    cout << " current time: " << g_eventQueue_ptr->getTime() << endl;
    assert(0);
  }          
  */

  bool older = false;
  
  if (local_timestamp == remote_timestamp){        
    if (getProcID() == (int) L1CacheMachIDToProcessorNum(remote_id)){   
      older = (remote_thread < thread);
    } else {
      older = (int) L1CacheMachIDToProcessorNum(remote_id) < getProcID();
    }
  } else {
    older = (remote_timestamp < local_timestamp);  
  }
  return older;                                                  
}          

void TransactionConflictManager::beginTransaction(int thread){
  int transactionLevel = XACT_MGR->getTransactionLevel(thread);
  assert(transactionLevel >= 1);
  
  if ((transactionLevel == 1) && !(m_lock_timestamp[thread])){  
    m_timestamp[thread] = g_eventQueue_ptr->getTime();
    m_lock_timestamp[thread] = true;
  }  
}    

void TransactionConflictManager::commitTransaction(int thread){
  int transactionLevel = XACT_MGR->getTransactionLevel(thread);
  assert(transactionLevel >= 1);
  
  if (transactionLevel == 1){
    m_lock_timestamp[thread] = false;
    m_numRetries[thread]     = 0;
    m_nacked[thread]         = false;
    m_lowestConflictLevel[thread] = -1;
    clearPossibleCycle(thread);
  }  
}  

void TransactionConflictManager::restartTransaction(int thread){
  m_numRetries[thread]++;      
  clearPossibleCycle(thread);
  m_lowestConflictLevel[thread] = -1;
  if (ENABLE_MAGIC_WAITING){
    setMagicWait(thread);   
  }        
}                        

int TransactionConflictManager::getNumRetries(int thread){
  return m_numRetries[thread];
}

int TransactionConflictManager::getLowestConflictLevel(int thread){
  return m_lowestConflictLevel[thread];
}

void TransactionConflictManager::setLowestConflictLevel(int thread, Address addr, bool nackedStore){
  int transactionLevel = XACT_MGR->getTransactionLevel(thread);
  bool changeLowestConflictLevel = false;
  int i;

  for (i = 1; i <= transactionLevel; i++){
    bool inReadSet = XACT_MGR->getXactIsolationManager()->isInReadSetPerfectFilter(thread, addr, i);        
    bool inWriteSet = XACT_MGR->getXactIsolationManager()->isInWriteSetPerfectFilter(thread, addr, i);        
    if (inWriteSet){
      if ((i < m_lowestConflictLevel[thread]) || m_lowestConflictLevel[thread] == -1){      
        m_lowestConflictLevel[thread] = i;
        changeLowestConflictLevel = true;
        break;
      }
    }
    if (nackedStore && inReadSet){
      if ((i < m_lowestConflictLevel[thread]) || m_lowestConflictLevel[thread] == -1){      
        m_lowestConflictLevel[thread] = i;
        changeLowestConflictLevel = true;
        break;
      }
    }
  }  
  if (changeLowestConflictLevel){
    if (XACT_DEBUG && XACT_DEBUG_LEVEL > 1){
      cout << " " << g_eventQueue_ptr->getTime() << " " << getLogicalProcID(thread) << " [" << getProcID() << "," << thread << "] SETTING LOWEST CONFLICT LEVEL " << m_lowestConflictLevel[thread] << " CONFLICT ADDR: " << addr << endl;
    }
  }  
}      
  
int TransactionConflictManager::getEnemyProc(int thread){
  //return m_enemyProc[thread];
  return m_magicWaitingOn[thread];
}
  
bool TransactionConflictManager::possibleCycle(int thread){
  return m_possible_cycle[thread];
}

void TransactionConflictManager::setPossibleCycle(int thread){          
  m_possible_cycle[thread] = true; 
}

void TransactionConflictManager::clearPossibleCycle(int thread){
  m_possible_cycle[thread] = false;
}          

bool TransactionConflictManager::nacked(int thread){
  return m_nacked[thread];
}          

uint64 TransactionConflictManager::getTimestamp(int thread){
  if (XACT_MGR->getTransactionLevel(thread) > 0)      
    return m_timestamp[thread];        
  else
    return g_eventQueue_ptr->getTime();        
}

uint64 TransactionConflictManager::getOldestTimestamp(){
  uint64 currentTime = g_eventQueue_ptr->getTime();
  uint64 oldestTime = currentTime;

  for (int i = 0; i < RubyConfig::numberofSMTThreads(); i++){
    if ((XACT_MGR->getTransactionLevel(i) > 0) && (m_timestamp[i] < oldestTime))
        oldestTime = m_timestamp[i];
  }  
  assert(oldestTime > 0);
  return oldestTime;
}

// used by Opal for SMT (intra-processor) conflicts
int TransactionConflictManager::getOldestThreadExcludingThread(int thread){
  uint64 currentTime = g_eventQueue_ptr->getTime();
  uint64 oldestTime = currentTime;
  int oldest_thread = -1;

  for (int i = 0; i < RubyConfig::numberofSMTThreads(); i++){
    if ((thread != i) && (XACT_MGR->getTransactionLevel(i) > 0) && (m_timestamp[i] < oldestTime)){
        oldestTime = m_timestamp[i];
        oldest_thread = i;
    }
  }  
  assert(oldestTime > 0);
  if(oldest_thread == -1){
    //error
    cout << "[ " << getProcID() << "] getOldestThreadExcludingThread ERROR NACKING SMT THREAD NOT FOUND thread = " << thread << " currentTime = " << currentTime << endl;
    for (int i = 0; i < RubyConfig::numberofSMTThreads(); i++){
      int read_count = XACT_MGR->getXactIsolationManager()->getTotalReadSetCount(i);
      int write_count = XACT_MGR->getXactIsolationManager()->getTotalWriteSetCount(i);
      cout << "\tThread " << i << " timestamp = " << m_timestamp[i] << " Level = " << XACT_MGR->getTransactionLevel(i) << " read_set_count = " << read_count << " write_set_count = " << write_count << endl;
    }
  }
  //assert(oldest_thread != -1);
  return oldest_thread;
}

// used for inter-processor conflicts
int TransactionConflictManager::getOldestThread(){
  uint64 currentTime = g_eventQueue_ptr->getTime();
  uint64 oldestTime = currentTime;
  int oldest_thread = -1;

  for (int i = 0; i < RubyConfig::numberofSMTThreads(); i++){
    if ((XACT_MGR->getTransactionLevel(i) > 0) && (m_timestamp[i] < oldestTime)){
        oldestTime = m_timestamp[i];
        oldest_thread = i;
    }
  }  
  assert(oldestTime > 0);
  // This may be true because the oldest thread could have committed, and no other trans. threads remain
  if(oldest_thread == -1){
    oldest_thread = 0;
  }
  return oldest_thread;
}
  
                                       
void TransactionConflictManager::notifyReceiveNack(int thread, Address physicalAddr, uint64 local_timestamp, uint64 remote_timestamp, MachineID remote_id){
  
  bool   possible_cycle  = m_possible_cycle[thread];
  // get the remote thread that NACKed us
  int remote_procnum = L1CacheMachIDToProcessorNum(remote_id);
  TransactionConflictManager * remote_conflict_mgr = g_system_ptr->getChip(m_chip_ptr->getID())->getTransactionInterfaceManager(remote_procnum)->getXactConflictManager();
  int    remote_thread   = remote_conflict_mgr->getOldestThread(); 
  int remote_logicalprocnum = remote_procnum*RubyConfig::numberofSMTThreads()+remote_thread;

  int my_logicalprocnum = getLogicalProcID(thread);
  Address myPC = SIMICS_get_program_counter(my_logicalprocnum);
  Address remotePC = SIMICS_get_program_counter(remote_logicalprocnum);
  //const char * my_instruction = SIMICS_disassemble_physical( my_logicalprocnum, SIMICS_translate_address( my_logicalprocnum, myPC ));
  //const char * remote_instruction = SIMICS_disassemble_physical( remote_logicalprocnum, SIMICS_translate_address( remote_logicalprocnum, remotePC ));
  m_nackedBy[thread][remote_logicalprocnum] = true;
  m_nackedByTimestamp[thread][remote_logicalprocnum] = remote_timestamp;
  m_magicWaitingTime[thread] = remote_timestamp;
  m_magicWaitingOn[thread] = remote_logicalprocnum;
    
  if (possible_cycle && isRemoteOlder(thread, remote_thread, local_timestamp, remote_timestamp, remote_id)){
    m_shouldTrap[thread] = true;
  }                
  
  if (XACT_DEBUG && (XACT_DEBUG_LEVEL > 2)){
    cout << " " << g_eventQueue_ptr->getTime() << " " << getLogicalProcID(thread) << " [" << getProcID() << "," << thread << "] RECEIVED NACK " << physicalAddr << " remote_id: " << remote_logicalprocnum << " [" << remote_procnum << "," << remote_thread << "] myPC: " << myPC << " remotePC: " << remotePC << " local_timestamp: " << local_timestamp << " remote_timestamp: " << remote_timestamp << " shouldTrap " << m_shouldTrap[thread] << endl;
  }
}

void TransactionConflictManager::notifyReceiveNackFinal ( int thread, Address physicalAddr){
  
  if (XACT_DEBUG && (XACT_DEBUG_LEVEL > 1)){
    if (!g_system_ptr->getXactVisualizer()->isStalled(getLogicalProcID(thread))){      
      cout << " " << g_eventQueue_ptr->getTime() << " " << getLogicalProcID(thread) << " [" << getProcID() << "," << thread << "] CONFLICTING REQUEST " << physicalAddr << " possibleCycle: " << m_possible_cycle[thread] << " shouldTrap: " << m_shouldTrap[thread] << endl;
    }  
  }
  
  for (int i = 0; i < RubyConfig::numberOfProcessors(); i++){
    if (m_nackedBy[thread][i]){
      if (m_magicWaitingTime[thread] > m_nackedByTimestamp[thread][i]){
          m_magicWaitingTime[thread] = m_nackedByTimestamp[thread][i];
          m_magicWaitingOn[thread]   = i;
      }           
      m_nackedBy[thread][i] = false;
    }                            
  }
  
  if (m_shouldTrap[thread]){
     // Call sequencer requesting abort     
    // if(SEQUENCER->isPrefetchRequest(line_address(physicalAddr))){
    if (0) {
      // reset state for aborted prefetches
      m_enemyProc[thread] = 0;
      clearPossibleCycle(thread);
      if (XACT_DEBUG && (XACT_DEBUG_LEVEL > 1)){
        cout << " " << g_eventQueue_ptr->getTime() << " " << getLogicalProcID(thread) << " [" << getProcID() << "," << thread << "] PREFETCH REQUEST " << physicalAddr << " possibleCycle: " << m_possible_cycle[thread] << " shouldTrap: " << m_shouldTrap[thread] << endl;
      }
    } else {
      // mark this request as "aborted"
      XACT_MGR->setAbortFlag(thread, line_address(physicalAddr));
      if (ATMTP_ENABLED) {
        XACT_MGR->setCPS_coh(thread);
      }
      setEnemyProcessor(thread, m_magicWaitingOn[thread]);
      m_needMagicWait[thread] = true;
    }
  }
  m_nacked[thread]     = true;
  m_shouldTrap[thread] = false;                        
} 

void TransactionConflictManager::notifySendNack(Address physicalAddress, uint64 remote_timestamp, MachineID remote_id){
  // get the remote thread that NACKed us
  int remote_procnum = L1CacheMachIDToProcessorNum(remote_id);
  TransactionConflictManager * remote_conflict_mgr = g_system_ptr->getChip(m_chip_ptr->getID())->getTransactionInterfaceManager(remote_procnum)->getXactConflictManager();
  int    remote_thread   = remote_conflict_mgr->getOldestThread(); 

  //assert(RubyConfig::numberofSMTThreads() == 1);      
  for (int i = 0; i < RubyConfig::numberofSMTThreads(); i++){
    if (XACT_MGR->isInReadSetFilter(i, physicalAddress) || XACT_MGR->isInWriteSetFilter(i, physicalAddress)){
      bool writer = XACT_MGR->isInWriteSetFilter(i, physicalAddress) ? true:false;      
      if (isRemoteOlder(i, remote_thread, m_timestamp[i], remote_timestamp, remote_id)){
        //      if (XACT_DEBUG && (XACT_DEBUG_LEVEL > 1)){
        //           cout << " " << g_eventQueue_ptr->getTime() << " " << getLogicalProcID(i) << " [" << getProcID() << "," << i << "] SETTING POSSIBLE CYCLE FLAG " << physicalAddress << " remote_id: " << L1CacheMachIDToProcessorNum(remote_id) << endl;
        //      }
        setPossibleCycle(i);        
        //m_magicWaitingTime[i] = remote_timestamp;
        //m_magicWaitingOn[i] = L1CacheMachIDToProcessorNum(remote_id) * RubyConfig::numberofSMTThreads() + remote_thread;
      }  
    }      
  }
}        
                                      
bool TransactionConflictManager::magicWait(int thread){
  if (m_enableMagicWait[thread]){
    int remote_logical_processor = m_magicWaitingOn[thread];
    int remote_thread            = remote_logical_processor % RubyConfig::numberofSMTThreads();                
    int remote_processor         = remote_logical_processor / RubyConfig::numberofSMTThreads();
    Time remote_timestamp        = g_system_ptr->getChip(remote_processor / RubyConfig::numberOfProcsPerChip())->getTransactionInterfaceManager(remote_processor % RubyConfig::numberOfProcsPerChip())->getTimestamp(remote_thread);
    /* MAGIC WAIT - WAIT FOR XACT COMMIT */
    //if (remote_timestamp > m_magicWaitingTime[thread]){
    //  m_enableMagicWait[thread] = false;
    /* MAGIC WAIT - WAIT FOR XACT NON-STALL */
    if (!g_system_ptr->getXactVisualizer()->isStalled(m_magicWaitingOn[thread]) || (remote_timestamp > m_magicWaitingTime[thread])){
      m_enableMagicWait[thread] = false;
      m_needMagicWait[thread]   = false;
      if (XACT_DEBUG && (XACT_DEBUG_LEVEL > 1)){
        cout << " " << g_eventQueue_ptr->getTime() << " " << getLogicalProcID(thread) << " [" << getProcID() << "," << thread << "] DISABLE MAGIC WAIT ON " << m_magicWaitingOn[thread]  << " WAITING ON TIMESTAMP: " << m_magicWaitingTime[thread] << " CURR_TIMESTAMP: " << remote_timestamp;
        if (XACT_DEBUG && XACT_DEBUG_LEVEL > 1){
          cout << " SYSTEM XACT STATE: " << g_system_ptr->getXactVisualizer()->getTransactionStateVector();
        }
        cout << endl;
      }  
    }  
  }
  return m_enableMagicWait[thread];
}

void TransactionConflictManager::setMagicWait(int thread){
  if (m_needMagicWait[thread]){      
    m_enableMagicWait[thread] = true;
    if (XACT_DEBUG && (XACT_DEBUG_LEVEL > 1)){
      cout << " " << g_eventQueue_ptr->getTime() << " " << getLogicalProcID(thread) << " [" << getProcID() << "," << thread << "] ENABLE MAGIC WAIT ON " << m_magicWaitingOn[thread]  << endl;
    }
    m_needMagicWait[thread] = false;
  }  
}            

void TransactionConflictManager::setEnemyProcessor(int thread, int remote_proc){
}

bool TransactionConflictManager::shouldNackLoad(Address addr, uint64 remote_timestamp, MachineID remote_id){
  bool existConflict = false;   
  int remote_proc = L1CacheMachIDToProcessorNum(remote_id);
  int remote_thread = 0;
  int remote_logical_proc_num = remote_proc * RubyConfig::numberofSMTThreads() + remote_thread;     
  string conflict_res_policy(XACT_CONFLICT_RES);   
  if (XACT_EAGER_CD && !XACT_LAZY_VM){
    existConflict = XACT_MGR->isInWriteFilterSummary(addr);    
    if (existConflict){
      if (conflict_res_policy == "TIMESTAMP"){
        assert(!OpalInterface::isOpalLoaded());      
        if (getTimestamp(0) >= remote_timestamp){
          XACT_MGR->setAbortFlag(0, addr);                        
          if (ATMTP_ENABLED) {
            XACT_MGR->setCPS_coh(0);
          }
          setEnemyProcessor(0, remote_logical_proc_num);
          m_magicWaitingOn[0] = remote_logical_proc_num;
          m_magicWaitingTime[0] = remote_timestamp;
          m_needMagicWait[0] = true;
          // remove this addr from isolation checker
          if(XACT_ISOLATION_CHECK){
            //cout << "REMOVING ADDR " << addr << " FROM READ SET OF " << getProcID() << endl;
            Address temp = addr;
            temp.makeLineAddress();
            int transactionLevel = XACT_MGR->getTransactionLevel(0);
            for(int level=1; level<=transactionLevel; ++level){
              g_system_ptr->getXactIsolationChecker()->removeFromWriteSet(getProcID(), temp, level);
            }
          }   
        }
      }
      for (int i = 0; i < RubyConfig::numberofSMTThreads(); i++){
        setLowestConflictLevel(i, addr, false);
      }  
    }      
    return existConflict;
  } else if (XACT_EAGER_CD && XACT_LAZY_VM){
    if (XACT_MGR->isInWriteFilterSummary(addr)){        
      assert(!OpalInterface::isOpalLoaded());      
      if (XACT_MGR->isTokenOwner(0)) return true;
      if (conflict_res_policy == "BASE"){        
        int proc = getLogicalProcID(0);
        XACT_MGR->setAbortFlag(0, addr);                        
        if (ATMTP_ENABLED) {
          XACT_MGR->setCPS_coh(0);
        }
        setEnemyProcessor(0, remote_logical_proc_num);
        // remove this addr from isolation checker
        if(XACT_ISOLATION_CHECK){
          //cout << "REMOVING ADDR " << addr << " FROM READ SET OF " << getProcID() << endl;
          Address temp = addr;
          temp.makeLineAddress();
          int transactionLevel = XACT_MGR->getTransactionLevel(0);
          for(int level=1; level<=transactionLevel; ++level){
            g_system_ptr->getXactIsolationChecker()->removeFromWriteSet(getProcID(), temp, level);
          }
        }
        return false;
      } else if (conflict_res_policy == "TIMESTAMP"){
        if (getTimestamp(0) >= remote_timestamp){          
          XACT_MGR->setAbortFlag(0, addr);                        
          if (ATMTP_ENABLED) {
            XACT_MGR->setCPS_coh(0);
          }
          setEnemyProcessor(0, remote_logical_proc_num);
          // remove this addr from isolation checker
          if(XACT_ISOLATION_CHECK){
            //cout << "REMOVING ADDR " << addr << " FROM READ SET OF " << getProcID() << endl;
            Address temp = addr;
            temp.makeLineAddress();
            int transactionLevel = XACT_MGR->getTransactionLevel(0);
            for(int level=1; level<=transactionLevel; ++level){
              g_system_ptr->getXactIsolationChecker()->removeFromWriteSet(getProcID(), temp, level);
            }
          }
          return false;
        } 
        return true;
      } else if (conflict_res_policy == "CYCLE"){
        return true;
      } else {
        assert(0);
      }                     
    }
    return false;
  } else {
    // for LL systems
    if (XACT_MGR->isInWriteFilterSummary(addr)){
      assert(!OpalInterface::isOpalLoaded());      
      if (XACT_MGR->isTokenOwner(0)) return true;
      return false;
    }
    return false;  
  }
  assert(0);
  return false;        
}

bool TransactionConflictManager::shouldNackStore(Address addr, uint64 remote_timestamp, MachineID remote_id){
  bool existConflict = false;      
  int remote_proc = L1CacheMachIDToProcessorNum(remote_id);
  int remote_thread = 0;
  int remote_logical_proc_num = remote_proc * RubyConfig::numberofSMTThreads() + remote_thread;     
  string conflict_res_policy(XACT_CONFLICT_RES);   
  if (XACT_EAGER_CD && !XACT_LAZY_VM){
    if (XACT_MGR->isInWriteFilterSummary(addr)){
      for (int i = 0; i < RubyConfig::numberofSMTThreads(); i++){
        setLowestConflictLevel(i, addr, true);
      }  
      if (conflict_res_policy == "TIMESTAMP"){
        assert(!OpalInterface::isOpalLoaded());      
        if (getTimestamp(0) >= remote_timestamp){
          XACT_MGR->setAbortFlag(0, addr);                        
          if (ATMTP_ENABLED) {
            XACT_MGR->setCPS_coh(0);
          }
          setEnemyProcessor(0, remote_logical_proc_num);
          m_magicWaitingOn[0] = remote_logical_proc_num;
          m_magicWaitingTime[0] = remote_timestamp;
          m_needMagicWait[0] = true;
          // remove this addr from isolation checker
          if(XACT_ISOLATION_CHECK){
            //cout << "REMOVING ADDR " << addr << " FROM READ SET OF " << getProcID() << endl;
            Address temp = addr;
            temp.makeLineAddress();
            int transactionLevel = XACT_MGR->getTransactionLevel(0);
            for(int level=1; level<=transactionLevel; ++level){
              g_system_ptr->getXactIsolationChecker()->removeFromWriteSet(getProcID(), temp, level);
            }
          }
        }
      } 
      return true;
    } else if (XACT_MGR->isInReadFilterSummary(addr)){  
      for (int i = 0; i < RubyConfig::numberofSMTThreads(); i++){
        setLowestConflictLevel(i, addr, true);
      }  
      if (conflict_res_policy == "HYBRID" || conflict_res_policy == "TIMESTAMP"){
        assert(!OpalInterface::isOpalLoaded());      
        if (getTimestamp(0) >= remote_timestamp){
          XACT_MGR->setAbortFlag(0, addr);                        
          if (ATMTP_ENABLED) {
            XACT_MGR->setCPS_coh(0);
          }
          setEnemyProcessor(0, remote_logical_proc_num);
          // remove this addr from isolation checker
          if(XACT_ISOLATION_CHECK){
            //cout << "REMOVING ADDR " << addr << " FROM READ SET OF " << getProcID() << endl;
            Address temp = addr;
            temp.makeLineAddress();
            int transactionLevel = XACT_MGR->getTransactionLevel(0);
            for(int level=1; level<=transactionLevel; ++level){
              g_system_ptr->getXactIsolationChecker()->removeFromReadSet(getProcID(), temp, level);
            }
          }
          return false;
        } else {
          return true;
        }
      } 
      return true;
    }                        
    return false;            
  } else if (XACT_EAGER_CD && XACT_LAZY_VM){
    if (XACT_MGR->isInWriteFilterSummary(addr) || 
            XACT_MGR->isInReadFilterSummary(addr)){        
      assert(!OpalInterface::isOpalLoaded());      
      if (XACT_MGR->isTokenOwner(0)) return true;
      if (conflict_res_policy == "BASE"){        
        int proc = getLogicalProcID(0);
        //cout << "Proc " << proc << " STORE SETTING ABORT FLAG PC = " << SIMICS_get_program_counter(proc) << " Remote Proc = " << remote_logical_proc_num << " RemotePC = " << SIMICS_get_program_counter(remote_logical_proc_num) << " addr = " << addr << endl;
        XACT_MGR->setAbortFlag(0, addr);                        
        if (ATMTP_ENABLED) {
          XACT_MGR->setCPS_coh(0);
        }
        setEnemyProcessor(0, remote_logical_proc_num);
        // remove this addr from isolation checker
        if(XACT_ISOLATION_CHECK){
          //cout << "REMOVING ADDR " << addr << " FROM READ SET OF " << getProcID() << endl;
          Address temp = addr;
          temp.makeLineAddress();
          if(XACT_MGR->isInWriteFilterSummary(addr)){
            int transactionLevel = XACT_MGR->getTransactionLevel(0);
            for(int level=1; level<=transactionLevel; ++level){
              g_system_ptr->getXactIsolationChecker()->removeFromWriteSet(getProcID(), temp, level);
            }
          }
          if(XACT_MGR->isInReadFilterSummary(addr)){
            int transactionLevel = XACT_MGR->getTransactionLevel(0);
            for(int level=1; level<=transactionLevel; ++level){
              g_system_ptr->getXactIsolationChecker()->removeFromReadSet(getProcID(), temp, level);
            }
          }
        }
        return false;
      } else if (conflict_res_policy == "TIMESTAMP"){
        if (getTimestamp(0) >= remote_timestamp){          
          XACT_MGR->setAbortFlag(0, addr);                        
          if (ATMTP_ENABLED) {
            XACT_MGR->setCPS_coh(0);
          }
          setEnemyProcessor(0, remote_logical_proc_num);
          // remove this addr from isolation checker
          if(XACT_ISOLATION_CHECK){
            //cout << "REMOVING ADDR " << addr << " FROM READ SET OF " << getProcID() << endl;
            Address temp = addr;
            temp.makeLineAddress();
            if(XACT_MGR->isInWriteFilterSummary(addr)){
              int transactionLevel = XACT_MGR->getTransactionLevel(0);
              for(int level=1; level<=transactionLevel; ++level){
                g_system_ptr->getXactIsolationChecker()->removeFromWriteSet(getProcID(), temp, level);
              }
            }
            if(XACT_MGR->isInReadFilterSummary(addr)){
              int transactionLevel = XACT_MGR->getTransactionLevel(0);
              for(int level=1; level<=transactionLevel; ++level){
                g_system_ptr->getXactIsolationChecker()->removeFromReadSet(getProcID(), temp, level);
              }
            }
          }
          return false;
        } 
        return true;
      } else if (conflict_res_policy == "CYCLE"){
        return true;
      } else {
        assert(0);
      }                     
    }
    return false;
  } else {
    // for LL systems
    if (XACT_MGR->isInReadFilterSummary(addr) || 
            XACT_MGR->isInWriteFilterSummary(addr)){
      if (conflict_res_policy != "BASE"){
        cout << "XACT_CONFLICT_RES: " << conflict_res_policy << endl;
        assert(0);
      }  
      assert(!OpalInterface::isOpalLoaded());      
      if (XACT_MGR->isTokenOwner(0)) return true;
      if (XACT_MGR->isInReadFilterSummary(addr)){
        XACT_MGR->setAbortFlag(0, addr);                        
        if (ATMTP_ENABLED) {
          XACT_MGR->setCPS_coh(0);
        }
        setEnemyProcessor(0, remote_logical_proc_num);
      }
      return false;
    }
    return false;                   
  }        
  assert(0);
  return false;       
}   
