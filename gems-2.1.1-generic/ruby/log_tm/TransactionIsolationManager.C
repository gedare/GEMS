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

#include "TransactionIsolationManager.h"
#include "TransactionInterfaceManager.h"
#include "XactIsolationChecker.h"

#include <assert.h>
#include <stdlib.h>

#define SEQUENCER g_system_ptr->getChip(m_chip_ptr->getID())->getSequencer(m_version)
#define XACT_MGR g_system_ptr->getChip(m_chip_ptr->getID())->getTransactionInterfaceManager(m_version)

TransactionIsolationManager::TransactionIsolationManager(AbstractChip* chip_ptr, int version) {
  int smt_threads = RubyConfig::numberofSMTThreads();
  m_chip_ptr = chip_ptr;
  m_version = version;
 
  m_readSet.setSize(smt_threads);
  m_writeSet.setSize(smt_threads);
  m_readSetFilter.setSize(smt_threads);
  m_writeSetFilter.setSize(smt_threads);
 
  m_virtualReadSet.setSize(smt_threads);
  m_virtualWriteSet.setSize(smt_threads);
  m_virtualReadSetFilter.setSize(smt_threads);
  m_virtualWriteSetFilter.setSize(smt_threads);
  m_summaryReadSet.setSize(smt_threads);
  m_summaryWriteSet.setSize(smt_threads);
  m_summaryReadSetFilter.setSize(smt_threads);
  m_summaryWriteSetFilter.setSize(smt_threads);

  m_xact_readCount.setSize(smt_threads);
  m_xact_writeCount.setSize(smt_threads);
  m_xact_overflow_readCount.setSize(smt_threads);
  m_xact_overflow_writeCount.setSize(smt_threads);
  
  /* For SMT conflicts */
  int p;
  for(p=0; p < smt_threads; ++p){
    m_readSet[p].setSize(1);
    m_writeSet[p].setSize(1);

    if (!PERFECT_FILTER) {
      m_readSetFilter[p] = new GenericBloomFilter(m_chip_ptr, READ_WRITE_FILTER);
      m_writeSetFilter[p] = new GenericBloomFilter(m_chip_ptr, READ_WRITE_FILTER);
    }
    if (!PERFECT_VIRTUAL_FILTER) {
      m_virtualReadSetFilter[p] = new GenericBloomFilter(NULL, VIRTUAL_READ_WRITE_FILTER);
      m_virtualWriteSetFilter[p] = new GenericBloomFilter(NULL, VIRTUAL_READ_WRITE_FILTER);
    }
    if (!PERFECT_SUMMARY_FILTER) {
      m_summaryReadSetFilter[p] = new GenericBloomFilter(NULL, SUMMARY_READ_WRITE_FILTER);
      m_summaryWriteSetFilter[p] = new GenericBloomFilter(NULL, SUMMARY_READ_WRITE_FILTER);
    }
      
    m_xact_readCount[p].setSize(1);
    m_xact_writeCount[p].setSize(1);
    m_xact_overflow_readCount[p].setSize(1);
    m_xact_overflow_writeCount[p].setSize(1);
  }  
 
	m_summaryConflictAddress = Address(0); 
	m_summaryConflictType = 0;
}

TransactionIsolationManager::~TransactionIsolationManager() {
}

void TransactionIsolationManager::setVersion(int version) {
  m_version = version;
}

int TransactionIsolationManager::getVersion() const {
  return m_version;
}

int TransactionIsolationManager::getProcID() const{
  return m_chip_ptr->getID()*RubyConfig::numberOfProcsPerChip()+m_version;
}
   
int TransactionIsolationManager::getLogicalProcID(int thread) const{
  return getProcID() * RubyConfig::numberofSMTThreads() + thread;
}

void TransactionIsolationManager::setSummaryConflictAddress(Address addr) {
	m_summaryConflictAddress = addr;
}

void TransactionIsolationManager::setSummaryConflictType(unsigned int type) {
	m_summaryConflictType = type;
}

Address TransactionIsolationManager::getSummaryConflictAddress() {
	return m_summaryConflictAddress;
}

unsigned int TransactionIsolationManager::getSummaryConflictType() {
	return m_summaryConflictType;
}

void TransactionIsolationManager::beginTransaction(int thread){
  int xact_level = XACT_MGR->getTransactionLevel(thread);
  
  if (xact_level > m_readSet[thread].size()){
    m_readSet[thread].setSize(xact_level);
  }
  m_readSet[thread][xact_level - 1].clear();
  if (xact_level > m_writeSet[thread].size()){
    m_writeSet[thread].setSize(xact_level);
  }
  m_writeSet[thread][xact_level - 1].clear();
}                            
   
void TransactionIsolationManager::commitTransaction(int thread, bool isOpen){
  int old_xact_level = XACT_MGR->getTransactionLevel(thread);   
  assert(old_xact_level >= 1);
  int new_xact_level = old_xact_level - 1;

  if (!isOpen) {
    Vector<Address> readSet = m_readSet[thread][old_xact_level-1].keys();
    Vector<Address> writeSet = m_writeSet[thread][old_xact_level-1].keys();

    if (new_xact_level > 0){
      for (int i = 0; i < readSet.size(); i++)
        addToReadSetPerfectFilter(thread, readSet[i], new_xact_level);
      for (int i = 0; i < writeSet.size(); i++)
        addToWriteSetPerfectFilter(thread, writeSet[i], new_xact_level);
      if (XACT_EAGER_CD) {  
        g_system_ptr->getXactIsolationChecker()->copyReadSet(getLogicalProcID(thread), old_xact_level, new_xact_level);  
        g_system_ptr->getXactIsolationChecker()->copyWriteSet(getLogicalProcID(thread), old_xact_level, new_xact_level);  
      }  
    }
  }
  else{
    // profile signatures (for top-level commit)
    int xid = XACT_MGR->getXID(thread);
    profileReadSetFilterActivity(xid, thread, true);
    profileWriteSetFilterActivity(xid, thread, true);
  }
  
  clearReadSetPerfectFilter(thread, old_xact_level);
  clearWriteSetPerfectFilter(thread, old_xact_level);        
  
  setFiltersToXactLevel(thread, new_xact_level, old_xact_level);     
  
  if (XACT_EAGER_CD) {
    for (int i = old_xact_level; i > new_xact_level; i--){
      g_system_ptr->getXactIsolationChecker()->clearReadSet(getLogicalProcID(thread), i);        
      g_system_ptr->getXactIsolationChecker()->clearWriteSet(getLogicalProcID(thread), i);
    }
  }        
}
                 
void TransactionIsolationManager::abortTransaction(int thread, int new_xact_level){
  int old_xact_level = XACT_MGR->getTransactionLevel(thread);
  assert(old_xact_level >= 1);
  assert(new_xact_level < old_xact_level);
  assert(new_xact_level >= 0);

  if(new_xact_level == 0){
    // profile signatures (for top-level abort)
    int xid = XACT_MGR->getXID(thread);
    profileReadSetFilterActivity(xid, thread, false);
    profileWriteSetFilterActivity(xid, thread, false);
  }

  for (int i = new_xact_level; i < old_xact_level; i++){
    clearReadSetPerfectFilter(thread, i + 1);
    clearWriteSetPerfectFilter(thread, i + 1);
  }
  
  setFiltersToXactLevel(thread, new_xact_level, old_xact_level);
}    

void TransactionIsolationManager::releaseIsolation(int thread, int xact_level){
  assert(xact_level >= 0);

  if(xact_level-1 == 0){
    // profile signatures (for top-level abort)
    int xid = XACT_MGR->getXID(thread);
    profileReadSetFilterActivity(xid, thread, false);
    profileWriteSetFilterActivity(xid, thread, false);
  }

  clearReadSetPerfectFilter(thread, xact_level);                
  clearWriteSetPerfectFilter(thread, xact_level);                
  
  setFiltersToXactLevel(thread, xact_level - 1, xact_level);
}  
          
void TransactionIsolationManager::releaseReadIsolation(int thread){
  
  int levels = XACT_MGR->getTransactionLevel(thread);
  for (int i = 0; i < levels; i++){
    clearReadSetPerfectFilter(thread, i + 1);
  }

  if(!PERFECT_FILTER){
    m_readSetFilter[thread]->clear();
  }

  if (XACT_DEBUG && XACT_DEBUG_LEVEL > 0){
    cout << g_eventQueue_ptr->getTime() << " " << getLogicalProcID(thread) << " [" << getProcID() << "," << thread << "] ABORT RELEASING READ ISOLATION " << endl;
  }


}    
          
// Used by protocol to check whether a read-write conflict occurs between SMT threads.
// Called by load requests.
bool TransactionIsolationManager::existLoadConflict(int thread, Address addr){
  int total_writers = 0;
  int writer_thread = -1;
  int smt_threads = RubyConfig::numberofSMTThreads();
  assert(thread < smt_threads);
  for (int p = 0; p < smt_threads; p++){
    if (thread == p) continue;
    if (isInWriteSetFilter(p, addr)){
        total_writers++;        
        writer_thread = p;
    }
  }
  assert (total_writers <= 1);  
  bool conflict = ( total_writers > 0);          
  return conflict;        
}

// Used by protocol to check whether a read-write conflict occurs between SMT threads
// Called by store requests.
bool TransactionIsolationManager::existStoreConflict(int thread, Address addr){
  int total_writers = 0;
  int total_readers = 0;
  int writer_thread = -1;
  int smt_threads = RubyConfig::numberofSMTThreads();
  int reader_vec[smt_threads];
  assert(thread < smt_threads);
  for(int p=0; p < smt_threads; ++p){
    if (thread == p) continue;
    if (isInWriteSetFilter(p, addr)){
      total_writers++;
      writer_thread = p;
    }
    if (isInReadSetPerfectFilter(p, addr)){
      total_readers++;
      reader_vec[p] = 1;
    }
  }
  assert(total_writers <= 1);
  bool conflict = ( ( total_writers > 0) || (total_readers > 0) );
  
  if(conflict){
    for(int i=0; i < smt_threads; ++i){
      if(reader_vec[i] == 1){
        // see if we need to add to dependency matrix
        if(XACT_MGR->getTimestamp(thread) < XACT_MGR->getTimestamp(i) ){
          //SEQUENCER->addThreadDependency(thread, i);
        }
      }
    }
    // Add writer to the dependency matrix
    if(writer_thread != -1 && XACT_MGR->getTimestamp(thread) < XACT_MGR->getTimestamp(writer_thread) ){
      //we are older, so add to dependency matrix
      // SEQUENCER->addThreadDependency(thread, writer_thread);
    }
  }
  return conflict;
}

bool TransactionIsolationManager::isInReadSetPerfectFilter(int thread, Address addr, int transactionLevel){
  assert(thread >= 0);
  assert(transactionLevel > 0);
  assert(transactionLevel <= m_readSet[thread].size());
  addr.makeLineAddress();
  
  return m_readSet[thread][transactionLevel-1].exist(addr);
}
        
bool TransactionIsolationManager::isInReadSetPerfectFilter(int thread, Address addr){
  assert(thread >= 0);
  addr.makeLineAddress();
  int levels = XACT_MGR->getTransactionLevel(thread);
  
  if (m_readSet[thread].size() < levels)
    levels = m_readSet[thread].size();        
  
  for (int i = 0; i < levels; i++){
    if (m_readSet[thread][i].exist(addr))
      return true;
  }    
  return false;
}                      
          
bool TransactionIsolationManager::isInWriteSetPerfectFilter(int thread, Address addr, int transactionLevel){
  assert(thread >= 0);
  assert(transactionLevel > 0);
  assert(transactionLevel <= m_writeSet[thread].size());
  addr.makeLineAddress();
  
  return m_writeSet[thread][transactionLevel-1].exist(addr);
}
        
bool TransactionIsolationManager::isInWriteSetPerfectFilter(int thread, Address addr){
  assert(thread >= 0);
  addr.makeLineAddress();
  int levels = XACT_MGR->getTransactionLevel(thread);
  
  if (m_writeSet[thread].size() < levels)
    levels = m_writeSet[thread].size();        
  
  for (int i = 0; i < levels; i++){
    if (m_writeSet[thread][i].exist(addr))
      return true;
  }    
  return false;
}
                      
void TransactionIsolationManager::addToReadSetPerfectFilter(int thread, Address addr, int transactionLevel){
  assert(thread >= 0);
  assert(transactionLevel > 0);
  assert(transactionLevel <= m_readSet[thread].size());
  addr.makeLineAddress();
        
  if (!m_readSet[thread][transactionLevel - 1].exist(addr))
    m_readSet[thread][transactionLevel-1].add(addr, 'y');

  assert(m_readSet[thread][transactionLevel-1].exist(addr));
    
}    
                    
void TransactionIsolationManager::addToWriteSetPerfectFilter(int thread, Address addr, int transactionLevel){
  assert(thread >= 0);
  assert(transactionLevel > 0);
  assert(transactionLevel <= m_writeSet[thread].size());
  addr.makeLineAddress();
        
  if (!m_writeSet[thread][transactionLevel - 1].exist(addr))
    m_writeSet[thread][transactionLevel-1].add(addr, 'y');

  assert(m_writeSet[thread][transactionLevel-1].exist(addr));
}

void TransactionIsolationManager::clearReadSetPerfectFilter(int thread, int transactionLevel){
  assert(thread >= 0);
  assert(transactionLevel > 0);
  assert(transactionLevel <= m_readSet[thread].size());
  
  m_readSet[thread][transactionLevel - 1].clear();
  assert(m_readSet[thread][transactionLevel - 1].keys().size() == 0);
}
           
void TransactionIsolationManager::clearWriteSetPerfectFilter(int thread, int transactionLevel){
  assert(thread >= 0);
  assert(transactionLevel > 0);
  assert(transactionLevel <= m_writeSet[thread].size());
  
  m_writeSet[thread][transactionLevel - 1].clear();
  assert(m_writeSet[thread][transactionLevel - 1].keys().size() == 0);
}
           
void TransactionIsolationManager::addToReadSetFilter(int thread, Address addr){
  addr.makeLineAddress();
  if(!PERFECT_FILTER){
    m_readSetFilter[thread]->set(addr);
  }
}

void TransactionIsolationManager::addToWriteSetFilter(int thread, Address addr){
  addr.makeLineAddress();
  if(!PERFECT_FILTER){
    m_writeSetFilter[thread]->set(addr);
  }
}

// returns a summary result. If any filter has addr in read set, returns true
bool TransactionIsolationManager::isInReadSetFilterSummary(Address physicalAddr){
  physicalAddr.makeLineAddress();
  int smt_threads = RubyConfig::numberofSMTThreads();
  bool result = false;
  for(int p=0; p < smt_threads; ++p){
    result = result || isInReadSetFilter(p, physicalAddr);
  }
  return result;
}

bool TransactionIsolationManager::isInWriteSetFilterSummary(Address physicalAddr){
  physicalAddr.makeLineAddress();
  int smt_threads = RubyConfig::numberofSMTThreads();
  bool result = false;
  for(int p=0; p < smt_threads; ++p){
    result = result || isInWriteSetFilter(p, physicalAddr);
  }
  return result;
}

// used to query either the Perfect or Bloom read set filter
bool TransactionIsolationManager::isInReadSetFilter(int thread, Address physicalAddr){
  physicalAddr.makeLineAddress();
  if(PERFECT_FILTER){
    // use perfect filters
    bool is_read = isInReadSetPerfectFilter(thread, physicalAddr);
    g_system_ptr->getProfiler()->profileReadSet(physicalAddr, is_read, is_read, getProcID(), thread);
    return is_read;
  }
  else{
    // use Bloom filters
    bool result = m_readSetFilter[thread]->isSet(physicalAddr);
    bool is_read = isInReadSetPerfectFilter(thread, physicalAddr);
    assert( result || !is_read);  // NO FALSE NEGATIVES
    g_system_ptr->getProfiler()->profileReadSet(physicalAddr, result, is_read, getProcID(), thread);
    return result;
  }
}

bool TransactionIsolationManager::isInWriteSetFilter(int thread, Address physicalAddr){
  physicalAddr.makeLineAddress();
  if(PERFECT_FILTER){
    // use perfect filters
    bool is_read = isInWriteSetPerfectFilter(thread, physicalAddr);
    g_system_ptr->getProfiler()->profileWriteSet(physicalAddr, is_read, is_read, getProcID(), thread);
    return is_read;
  }
  else{
    // use Bloom filters
    bool result = m_writeSetFilter[thread]->isSet(physicalAddr);
    bool is_read = isInWriteSetPerfectFilter(thread, physicalAddr);
    assert( result || !is_read);  // NO FALSE NEGATIVES
    g_system_ptr->getProfiler()->profileWriteSet(physicalAddr, result, is_read, getProcID(), thread);
    return result;
  }
}

void TransactionIsolationManager::clearReadSetFilter(int thread){
  if(!PERFECT_FILTER){
    m_readSetFilter[thread]->clear();
  }
}
  
void TransactionIsolationManager::clearWriteSetFilter(int thread){
  if(!PERFECT_FILTER){
    m_writeSetFilter[thread]->clear();
  }
}

int TransactionIsolationManager::getTotalReadSetCount(int thread){
  if(!PERFECT_FILTER){
    return m_readSetFilter[thread]->getTotalCount();
  } else
    return m_readSet[thread].size();
}

int TransactionIsolationManager::getTotalWriteSetCount(int thread){
  if(!PERFECT_FILTER){
    return m_writeSetFilter[thread]->getTotalCount();
  } else
    return m_writeSet[thread].size();
}

void TransactionIsolationManager::setFiltersToXactLevel(int thread, int new_xact_level, int old_xact_level){
  assert((new_xact_level >= 0) && (new_xact_level <= m_readSet[thread].size()) && (new_xact_level <= m_writeSet[thread].size()));

  clearReadSetFilter(thread);
  clearWriteSetFilter(thread);

  for (int i = 0; i < new_xact_level; i++){
    Vector<Address> readSet = m_readSet[thread][i].keys();
    Vector<Address> writeSet = m_writeSet[thread][i].keys(); 

    for (int j = 0; j < readSet.size(); j++)
      addToReadSetFilter(thread, readSet[j]);
    for (int j = 0; j < writeSet.size(); j++)
      addToWriteSetFilter(thread, writeSet[j]);      
  }
  
  if (XACT_EAGER_CD) {
    for (int i = old_xact_level; i > new_xact_level; i--){
      g_system_ptr->getXactIsolationChecker()->clearReadSet(getLogicalProcID(thread), i);        
      g_system_ptr->getXactIsolationChecker()->clearWriteSet(getLogicalProcID(thread), i);
    }
  }          
  
  if (XACT_DEBUG && XACT_DEBUG_LEVEL > 0){
    cout << g_eventQueue_ptr->getTime() << " " << getLogicalProcID(thread) << " [" << getProcID() << "," << thread << "] SETTING FILTERS to XACT LEVEL: " << new_xact_level << endl;
  }
}                      
            

void TransactionIsolationManager::profileReadSetFilterActivity(int xid, int thread, bool isCommit){
  if(!PERFECT_FILTER){
    int bits_set = m_readSetFilter[thread]->getTotalCount();
    g_system_ptr->getProfiler()->profileReadFilterBitsSet(xid, bits_set, isCommit);
  }
}

void TransactionIsolationManager::profileWriteSetFilterActivity(int xid, int thread, bool isCommit){
  if(!PERFECT_FILTER){
    int bits_set = m_writeSetFilter[thread]->getTotalCount();
    g_system_ptr->getProfiler()->profileWriteFilterBitsSet(xid, bits_set, isCommit);
  }
}

int TransactionIsolationManager::getReadSetSize(int thread, int xact_level){
  assert(xact_level >= 1);      
  assert(xact_level <= m_readSet[thread].size());
  return m_readSet[thread][xact_level - 1].size();
}  

int TransactionIsolationManager::getWriteSetSize(int thread, int xact_level){
  assert(xact_level >= 1);      
  assert(xact_level <= m_writeSet[thread].size());
  return m_writeSet[thread][xact_level - 1].size();
}  

Map<Address, char> &TransactionIsolationManager::getVirtualReadSetPerfectFilter(int thread) {
  return m_virtualReadSet[thread];
}

Map<Address, char> &TransactionIsolationManager::getVirtualWriteSetPerfectFilter(int thread) {
  return m_virtualWriteSet[thread];
}

GenericBloomFilter * TransactionIsolationManager::getReadSetFilter(int thread) {
  return m_readSetFilter[thread];
}

GenericBloomFilter * TransactionIsolationManager::getWriteSetFilter(int thread) {
  return m_writeSetFilter[thread];
}


GenericBloomFilter * TransactionIsolationManager::getVirtualReadSetFilter(int thread) {
  return m_virtualReadSetFilter[thread];
}

GenericBloomFilter * TransactionIsolationManager::getVirtualWriteSetFilter(int thread) {
  return m_virtualWriteSetFilter[thread];
}

Map<Address, char> &TransactionIsolationManager::getSummaryReadSetPerfectFilter(int thread) {
  return m_summaryReadSet[thread];
}

Map<Address, char> &TransactionIsolationManager::getSummaryWriteSetPerfectFilter(int thread) {
  return m_summaryWriteSet[thread];
}

GenericBloomFilter * TransactionIsolationManager::getSummaryReadSetFilter(int thread) {
  return m_summaryReadSetFilter[thread];
}

GenericBloomFilter * TransactionIsolationManager::getSummaryWriteSetFilter(int thread) {
  return m_summaryWriteSetFilter[thread];
}

bool TransactionIsolationManager::isInVirtualReadSetPerfectFilter(int thread, Address addr){
  assert(thread >= 0);
  addr.makeLineAddress();
  
  return m_virtualReadSet[thread].exist(addr);
}
        
bool TransactionIsolationManager::isInVirtualWriteSetPerfectFilter(int thread, Address addr){
  assert(thread >= 0);
  addr.makeLineAddress();
  
  return m_virtualWriteSet[thread].exist(addr);
}

// used to query either the Perfect or Bloom virtual read set filter
bool TransactionIsolationManager::isInVirtualReadSetFilter(int thread, Address addr){
  addr.makeLineAddress();
  if(PERFECT_VIRTUAL_FILTER){
    // use perfect filters
    return isInVirtualReadSetPerfectFilter(thread, addr);
  }
  else{
    // use Bloom filters
    bool result = m_virtualReadSetFilter[thread]->isSet(addr);
    bool is_read = isInVirtualReadSetPerfectFilter(thread, addr);
    assert( result || !is_read);  // NO FALSE NEGATIVES
    return result;
  }
}

// used to query either the Perfect or Bloom virtual write set filter
bool TransactionIsolationManager::isInVirtualWriteSetFilter(int thread, Address addr){
  addr.makeLineAddress();
  if(PERFECT_VIRTUAL_FILTER){
    // use perfect filters
    return isInVirtualWriteSetPerfectFilter(thread, addr);
  }
  else{
    // use Bloom filters
    bool result = m_virtualWriteSetFilter[thread]->isSet(addr);
    bool is_read = isInVirtualWriteSetPerfectFilter(thread, addr);
    assert( result || !is_read);  // NO FALSE NEGATIVES
    return result;
  }
}
       
void TransactionIsolationManager::addToVirtualReadSetPerfectFilter(int thread, Address addr){
  assert(thread >= 0);
  addr.makeLineAddress();
        
  if (!m_virtualReadSet[thread].exist(addr))
    m_virtualReadSet[thread].add(addr, 'y');

  assert(m_virtualReadSet[thread].exist(addr));
}    

void TransactionIsolationManager::addToVirtualWriteSetPerfectFilter(int thread, Address addr){
  assert(thread >= 0);
  addr.makeLineAddress();
        
  if (!m_virtualWriteSet[thread].exist(addr))
    m_virtualWriteSet[thread].add(addr, 'y');

  assert(m_virtualWriteSet[thread].exist(addr));
}

void TransactionIsolationManager::addToVirtualReadSetFilter(int thread, Address addr){
  addr.makeLineAddress();
  if(!PERFECT_VIRTUAL_FILTER){
    m_virtualReadSetFilter[thread]->set(addr);
  }
}

void TransactionIsolationManager::addToVirtualWriteSetFilter(int thread, Address addr){
  addr.makeLineAddress();
  if(!PERFECT_VIRTUAL_FILTER){
    m_virtualWriteSetFilter[thread]->set(addr);
  }
}

void TransactionIsolationManager::clearVirtualReadSetPerfectFilter(int thread){
  assert(thread >= 0);
  
  m_virtualReadSet[thread].clear();
  assert(m_virtualReadSet[thread].keys().size() == 0);
}

void TransactionIsolationManager::clearVirtualWriteSetPerfectFilter(int thread){
  assert(thread >= 0);
  
  m_virtualWriteSet[thread].clear();
  assert(m_virtualWriteSet[thread].keys().size() == 0);
}

void TransactionIsolationManager::clearVirtualReadSetFilter(int thread){
  if(!PERFECT_VIRTUAL_FILTER){
    m_virtualReadSetFilter[thread]->clear();
  }
}

void TransactionIsolationManager::clearVirtualWriteSetFilter(int thread){
  if(!PERFECT_VIRTUAL_FILTER){
    m_virtualWriteSetFilter[thread]->clear();
  }
}

bool TransactionIsolationManager::isInSummaryReadSetPerfectFilter(int thread, Address addr){
  assert(thread >= 0);
  addr.makeLineAddress();
  
  return m_summaryReadSet[thread].exist(addr);
}
        
bool TransactionIsolationManager::isInSummaryWriteSetPerfectFilter(int thread, Address addr){
  assert(thread >= 0);
  addr.makeLineAddress();
  
  return m_summaryWriteSet[thread].exist(addr);
}

// used to query either the Perfect or Bloom summary read set filter
bool TransactionIsolationManager::isInSummaryReadSetFilter(int thread, Address addr){
  addr.makeLineAddress();
  if(PERFECT_SUMMARY_FILTER){
    // use perfect filters
    return isInSummaryReadSetPerfectFilter(thread, addr);
  }
  else{
    // use Bloom filters
    bool result = m_summaryReadSetFilter[thread]->isSet(addr);
    //bool is_read = isInSummaryReadSetPerfectFilter(thread, addr);
    //assert( result || !is_read);  // NO FALSE NEGATIVES
    return result;
  }
}

// used to query either the Perfect or Bloom summary write set filter
bool TransactionIsolationManager::isInSummaryWriteSetFilter(int thread, Address addr){
  addr.makeLineAddress();
  if(PERFECT_SUMMARY_FILTER){
    // use perfect filters
    return isInSummaryWriteSetPerfectFilter(thread, addr);
  }
  else{
    // use Bloom filters
    bool result = m_summaryWriteSetFilter[thread]->isSet(addr);
    //bool is_read = isInSummaryWriteSetPerfectFilter(thread, addr);
    //assert( result || !is_read);  // NO FALSE NEGATIVES
    return result;
  }
}
 
void TransactionIsolationManager::addToSummaryReadSetPerfectFilter(int thread, Address addr){
  assert(thread >= 0);
  addr.makeLineAddress();
        
  if (!m_summaryReadSet[thread].exist(addr))
    m_summaryReadSet[thread].add(addr, 'y');

  assert(m_summaryReadSet[thread].exist(addr));
}    

void TransactionIsolationManager::addToSummaryWriteSetPerfectFilter(int thread, Address addr){
  assert(thread >= 0);
  addr.makeLineAddress();
        
  if (!m_summaryWriteSet[thread].exist(addr))
    m_summaryWriteSet[thread].add(addr, 'y');

  assert(m_summaryWriteSet[thread].exist(addr));
}

void TransactionIsolationManager::addToSummaryReadSetFilter(int thread, Address addr){
  addr.makeLineAddress();
  if(!PERFECT_SUMMARY_FILTER){
    m_summaryReadSetFilter[thread]->set(addr);
  }
}

void TransactionIsolationManager::addToSummaryWriteSetFilter(int thread, Address addr){
  addr.makeLineAddress();
  if(!PERFECT_SUMMARY_FILTER){
    m_summaryWriteSetFilter[thread]->set(addr);
  }
}

void TransactionIsolationManager::removeFromSummaryReadSetPerfectFilter(int thread, Address addr){
  assert(thread >= 0);
  addr.makeLineAddress();
        
  if (m_summaryReadSet[thread].exist(addr)) {
    m_summaryReadSet[thread].remove(addr);
	}

  assert(!m_summaryReadSet[thread].exist(addr));
}    

void TransactionIsolationManager::removeFromSummaryWriteSetPerfectFilter(int thread, Address addr){
  assert(thread >= 0);
  addr.makeLineAddress();
        
  if (m_summaryWriteSet[thread].exist(addr)) {
    m_summaryWriteSet[thread].remove(addr);
	}

  assert(!m_summaryWriteSet[thread].exist(addr));
}

void TransactionIsolationManager::removeFromSummaryReadSetFilter(int thread, Address addr){
  addr.makeLineAddress();
  if(!PERFECT_SUMMARY_FILTER){
    if (m_summaryReadSetFilter[thread]->isSet(addr)) {
      m_summaryReadSetFilter[thread]->unset(addr);
    }
  }
}

void TransactionIsolationManager::removeFromSummaryWriteSetFilter(int thread, Address addr){
  addr.makeLineAddress();
  if(!PERFECT_SUMMARY_FILTER){
    if (m_summaryWriteSetFilter[thread]->isSet(addr)) {
      m_summaryWriteSetFilter[thread]->unset(addr);
    }
  }
}

void TransactionIsolationManager::mergeVirtualWithSummaryReadSet(int thread, GenericBloomFilter * virtualReadSetFilter) {
  if(!PERFECT_SUMMARY_FILTER) {
		m_summaryReadSetFilter[thread]->merge(virtualReadSetFilter);
	}
}

void TransactionIsolationManager::mergeVirtualWithSummaryReadSet(int thread, Map<Address, char> &virtualReadSetPerfectFilter){
	Vector<Address> read_keys = virtualReadSetPerfectFilter.keys();
	for(int k = 0; k < virtualReadSetPerfectFilter.size(); k++) {
 		if (!m_summaryReadSet[thread].exist(read_keys[k])) {
			m_summaryReadSet[thread].add(read_keys[k], 'y');
		}
  	if(!PERFECT_SUMMARY_FILTER) {
	 		m_summaryReadSetFilter[thread]->set(read_keys[k]);
		}
	}
}

void TransactionIsolationManager::mergeVirtualWithSummaryWriteSet(int thread, GenericBloomFilter * virtualWriteSetFilter) {
  if(!PERFECT_SUMMARY_FILTER) {
		m_summaryWriteSetFilter[thread]->merge(virtualWriteSetFilter);
	}
}

void TransactionIsolationManager::mergeVirtualWithSummaryWriteSet(int thread, Map<Address, char> &virtualWriteSetPerfectFilter){
	Vector<Address> write_keys = virtualWriteSetPerfectFilter.keys();
	for(int k = 0; k < virtualWriteSetPerfectFilter.size(); k++) {
 		if (!m_summaryWriteSet[thread].exist(write_keys[k])) {
			m_summaryWriteSet[thread].add(write_keys[k], 'y');
		}
  	if(!PERFECT_SUMMARY_FILTER) {
	 		m_summaryWriteSetFilter[thread]->set(write_keys[k]);
		}
	}
}

void TransactionIsolationManager::clearSummaryReadSetPerfectFilter(int thread){
  assert(thread >= 0);
  
  m_summaryReadSet[thread].clear();
  assert(m_summaryReadSet[thread].keys().size() == 0);
}

void TransactionIsolationManager::clearSummaryWriteSetPerfectFilter(int thread){
  assert(thread >= 0);
  
  m_summaryWriteSet[thread].clear();
  assert(m_summaryWriteSet[thread].keys().size() == 0);
}

void TransactionIsolationManager::clearSummaryReadSetFilter(int thread){
  if(!PERFECT_SUMMARY_FILTER){
    m_summaryReadSetFilter[thread]->clear();
  }
}

void TransactionIsolationManager::clearSummaryWriteSetFilter(int thread){
  if(!PERFECT_SUMMARY_FILTER){
    m_summaryWriteSetFilter[thread]->clear();
  }
}

int TransactionIsolationManager::readBitSummaryReadSetFilter(int thread, int index){
  if(!PERFECT_SUMMARY_FILTER) {
     return m_summaryReadSetFilter[thread]->readBit(index);
  }
  return -1;
}

void TransactionIsolationManager::writeBitSummaryReadSetFilter(int thread, int index, int value){
  if(!PERFECT_SUMMARY_FILTER) {
     m_summaryReadSetFilter[thread]->writeBit(index, value);
  }
}

int TransactionIsolationManager::readBitSummaryWriteSetFilter(int thread, int index){
  if(!PERFECT_SUMMARY_FILTER) {
     return m_summaryWriteSetFilter[thread]->readBit(index);
  }
  return -1;
}

void TransactionIsolationManager::writeBitSummaryWriteSetFilter(int thread, int index, int value){
  if(!PERFECT_SUMMARY_FILTER) {
     m_summaryWriteSetFilter[thread]->writeBit(index, value);
  }
}


int TransactionIsolationManager::getIndexSummaryFilter(int thread, Address addr) {
  addr.makeLineAddress();
  if(!PERFECT_SUMMARY_FILTER){
    return m_summaryWriteSetFilter[thread]->getIndex(addr);
  }
  return -1;
}
