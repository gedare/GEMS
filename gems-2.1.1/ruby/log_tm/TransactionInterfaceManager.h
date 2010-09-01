
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

#ifndef TRANSACTIONINTERFACEMANAGER_H
#define TRANSACTIONINTERFACEMANAGER_H

#include "Global.h"
#include "System.h"
#include "Profiler.h"
#include "Vector.h"
#include "Map.h"
#include "Address.h"
#include "GenericBloomFilter.h"
#include "Sequencer.h"
#include "RegisterStateWindowed.h"

class TransactionVersionManager;
class TransactionConflictManager;
class TransactionIsolationManager;
class LazyTransactionVersionManager;
class RockTransactionManager;

typedef enum {
  XACT_LOWEST_CONFLICT_LEVEL = 0,      
  XACT_FRAME_POINTER,
  XACT_LEVEL,
  THREADID,
  XACT_LOG_SIZE,
  NUM_RETRIES,
  TRAP_ADDRESS,
  TRAP_TYPE,
  FIRST_ENTRY,
  INVALID_ENTRY
} SoftwareTransactionStruct;

typedef struct { SoftwareTransactionStruct entry; int offset;} SoftwareXactStruct;

static SoftwareXactStruct s_softwareXactStructMap[] = {
  {XACT_LOWEST_CONFLICT_LEVEL, -32},       
  {XACT_FRAME_POINTER, -28},
  {XACT_LEVEL, -24},
  {THREADID, -20},
  {XACT_LOG_SIZE, -16},
  {NUM_RETRIES, -12},
  {TRAP_ADDRESS, -8},
  {TRAP_TYPE, -4},    
  {FIRST_ENTRY, -32},      
  {INVALID_ENTRY, 0},
};        

class TransactionInterfaceManager {
public:
  TransactionInterfaceManager(AbstractChip* chip_ptr, int version);
  ~TransactionInterfaceManager();

  TransactionIsolationManager* getXactIsolationManager();
  TransactionConflictManager*  getXactConflictManager();
  TransactionVersionManager*   getXactVersionManager();
  
  LazyTransactionVersionManager*   getXactLazyVersionManager();

  void beginTransaction(int thread, int xid, bool isOpen);
  void commitTransaction(int thread, int xid, bool isOpen);
  void commitLazyTransaction(int thread);
  void abortTransaction(int thread, int xid); 
  void abortLazyTransaction(int thread);

  void registerCompensatingAction(int thread);
  void registerCommitAction(int thread);
  void xmalloc(int thread);

  void setLogBase(int thread);
  void setHandlerAddress(int thread); 
  void setTID(int thread, int tid);

  void beginEscapeAction(int thread);
  void endEscapeAction(int thread);
  void setLoggedException(int thread_num);
  void clearLoggedException(int thread_num);

  int getTransactionLevel(int thread);
  int getEscapeDepth(int thread);
  int getXID(int thread);
  int getTID(int thread);
  bool inTransaction(int thread);
  bool inLoggedException(int thread);
  bool shouldTrap(int thread);
  int getTrapType(int thread);

  void profileLoad(int thread, Address logicalAddr, Address physicalAddr, int transactionLevel);
  void profileStore(int thread, Address logicalAddr, Address physicalAddr, int transactionLevel);
  void isolateTransactionLoad(int thread, Address logicalAddr, Address physicalAddr, int transactionLevel);
  void isolateTransactionStore(int thread, Address logicalAddr, Address physicalAddr, int transactionLevel);
  void logTransactionStore(int thread, Address logicalAddr, Address physicalAddr, int transactionLevel);
  bool isInReadSetFilter(int thread, Address physicalAddr);
  bool isInWriteSetFilter(int thread, Address physicalAddr);

  bool shouldTakeTrapCheckpoint(int thread);
  void takeContinueCheckpoint(int thread);
  void trapToHandler(int thread);
  bool shouldUseHardwareAbort(int thread);
  void hardwareAbort(int thread);
  void restartTransaction(int thread);
  void restartTransactionCallback(int thread);
  void continueExecution(int thread);
  void continueExecutionCallback(int thread);
  void releaseIsolation(int thread);
  void setAbortFlag(int thread, Address addr);
  void setTrapTLBMiss(int thread, Address addr);
  void setTrapCommitActions(int thread);
	void setSummaryConflictFlag(int thread, Address addr);
  void setInterruptTrap(int thread);

  bool existLoadConflict(Address addr);
  bool existStoreConflict(Address addr);
  bool shouldNackLoad(Address addr, uint64 remote_timestamp, MachineID remote_id);
  bool shouldNackStore(Address addr, uint64 remote_timestamp, MachineID remote_id);
  bool checkReadWriteSignatures(Address addr);
  bool checkWriteSignatures(Address addr); 
  void notifyReceiveNack(int thread, Address addr, uint64 local_timestamp, uint64 remote_timestamp, MachineID remote_id);
  void notifyReceiveNackFinal(int thread, Address addr);
  void notifySendNack(Address addr, uint64 remote_timestamp, MachineID remote_id);
  uint64 getTimestamp(int thread);
  uint64 getOldestTimestamp();
  int getOldestThreadExcludingThread(int thread);
  void setPossibleCycle(int thread);
  bool possibleCycle(int thread);

  bool existGlobalLoadConflict(int thread, Address addr);
  bool existGlobalStoreConflict(int thread, Address addr);
  bool checkGlobalLoadConflict(int thread, Address addr);
  bool checkGlobalStoreConflict(int thread, Address addr);

  void profileOverflow(Address addr, bool l1_overflow);
  void profileTransactionMiss(int thread, bool load);

  bool isInReadFilterSummary(Address addr);
  bool isInWriteFilterSummary(Address addr);
  bool isTokenOwner(int thread);
  bool isRemoteOlder(uint64 remote_timestamp);
  bool magicWait(int thread);

  bool shouldUpgradeLoad(physical_address_t curr_pc);
  void addToLoadAddressMap(int thread, physical_address_t curr_pc, physical_address_t load_addr);
  void updateStorePredictor(int thread, physical_address_t store_addr);
  void clearLoadAddressMap(int thread);
  void printConfig(ostream& out);

  void setVersion(int version);
  int getVersion() const;

  void addToSummaryReadSetFilter(int thread, Address addr);
  void addToSummaryWriteSetFilter(int thread, Address addr);
  void removeFromSummaryReadSetFilter(int thread, Address addr);
  void removeFromSummaryWriteSetFilter(int thread, Address addr);
  bool isInSummaryReadSetFilter(int thread, Address addr);
  bool isInSummaryWriteSetFilter(int thread, Address addr);
  int  readBitSummaryReadSetFilter(int thread, int index);
  void writeBitSummaryReadSetFilter(int thread, int index, int value);
  int  readBitSummaryWriteSetFilter(int thread, int index);
  void writeBitSummaryWriteSetFilter(int thread, int index, int value);
  int  getIndexSummaryFilter(int thread, Address addr);
  bool ignoreWatchpointFlag(int thread);
  void setIgnoreWatchpointFlag(int thread, bool value);
  void setRestorePC(int thread, unsigned int pc);

  // For ATMTP, this must be called immediately after beginTransaction()
  // to record the fail_pc.
  //
  void setFailPC(int thread, Address fail_pc);

  void setCPS(int thread, uint32 cps);
  void setCPS_exog(int thread);
  void setCPS_coh(int thread);
  void setCPS_tcc(int thread);
  void setCPS_inst(int thread);
  void setCPS_prec(int thread);
  void setCPS_async(int thread);
  void setCPS_size(int thread);
  void setCPS_ld(int thread);
  void setCPS_st(int thread);
  void setCPS_cti(int thread);
  void setCPS_fp(int thread);
  uint32 getCPS(int thread);
  void clearCPS(int thread);

  bool saveInstShouldAbort(int thread);
  bool restoreInstShouldAbort(int thread); 
  bool inRetryHACK(int thread);
  void setInRetryHACK(int thread);
  void clearInRetryHACK(int thread);
  void restoreFailPC(int thread);
  void xactReplacement(Address addr);

private:
  int getProcID() const;
  int getLogicalProcID(int thread) const;

  void dumpXactRegisters(int thread);
  
  AbstractChip * m_chip_ptr;
  int m_version;
  
  TransactionVersionManager       * m_xactVersionManager;
  TransactionIsolationManager     * m_xactIsolationManager;
  TransactionConflictManager      * m_xactConflictManager;
  LazyTransactionVersionManager   * m_xactLazyVersionManager;
  
  RockTransactionManager         ** m_xactRockManager;

  int*      m_transactionLevel; // nesting depth, where outermost has depth 1
  int*      m_xid; 
  int*      m_escapeDepth;
  int*      m_inLoggedException;
  int*      m_tid;
  Address*  m_handlerAddress;

  int*      m_trapType;
  bool*     m_processingTrap;
  bool*     m_shouldTrap;
  bool*     m_trapTakeCheckpoint;
  Address*  m_trapAddress;
  Address*  m_trapPC;
  Time*     m_trapTime;
  bool*     m_ignoreWatchpointFlag; 
  Vector<RegisterState*> m_continueCheckpoint;

  int*        m_numLoadOverflows;
  int*        m_numStoreOverflows;
  int*        m_numLoadMisses;
  int*        m_numStoreMisses;
  Time*       m_xactBeginCycle;
  long long*  m_xactInstrCount; 

  Map<physical_address_t, int>                          *m_xactStorePredictorPC_ptr;
  Vector<Map<physical_address_t, physical_address_t>* >   m_xactLoadAddressMap_ptr;
};  

#endif



