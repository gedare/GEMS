
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

#ifndef TRANSACTIONISOLATIONMANAGER_H
#define TRANSACTIONISOLATIONMANAGER_H

#include "Global.h"
#include "System.h"
#include "Profiler.h"
#include "Vector.h"
#include "Map.h"
#include "Address.h"
#include "GenericBloomFilter.h"
#include "Sequencer.h"

class TransactionIsolationManager {
public:
  TransactionIsolationManager(AbstractChip* chip_ptr, int version);
  ~TransactionIsolationManager();

  void beginTransaction(int thread);
  void commitTransaction(int thread, bool isOpen);
  void abortTransaction(int thread, int new_xact_level);
  void releaseIsolation(int thread, int xact_level);
  void releaseReadIsolation(int thread);

  bool existLoadConflict(int thread, Address addr);
  bool existStoreConflict(int thread, Address addr);

  /* These functions query the perfect filter */
  bool isInReadSetPerfectFilter(int thread, Address addr);
  bool isInReadSetPerfectFilter(int thread, Address addr, int transactionLevel);
  bool isInWriteSetPerfectFilter(int thread, Address addr);
  bool isInWriteSetPerfectFilter(int thread, Address addr, int transactionLevel);
  void addToReadSetPerfectFilter(int thread, Address addr, int transactionLevel);
  void addToWriteSetPerfectFilter(int thread, Address addr, int transactionLevel);
  void clearReadSetPerfectFilter(int thread, int transactionLevel);
  void clearWriteSetPerfectFilter(int thread, int transactionLevel);
  void setFiltersToXactLevel(int thread, int new_xact_level, int old_xact_level);

  /* These functions are used to manipulate the Read/Write Bloom filters */
  bool isInReadSetFilter(int thread, Address addr);
  bool isInWriteSetFilter(int thread, Address addr);
  bool isInReadSetFilterSummary(Address addr);
  bool isInWriteSetFilterSummary(Address addr);
  void addToReadSetFilter(int thread, Address addr);
  void addToWriteSetFilter(int thread, Address addr);
  void clearReadSetFilter(int thread);
  void clearWriteSetFilter(int thread);
  int getTotalReadSetCount(int thread);
  int getTotalWriteSetCount(int thread);
	GenericBloomFilter * getReadSetFilter(int thread);
	GenericBloomFilter * getWriteSetFilter(int thread);

  /* These functions manipulate the virtualized Read/Write filter */
	Map<Address, char> &getVirtualReadSetPerfectFilter(int thread);
	Map<Address, char> &getVirtualWriteSetPerfectFilter(int thread);
	GenericBloomFilter *getVirtualReadSetFilter(int thread);
	GenericBloomFilter *getVirtualWriteSetFilter(int thread);
  bool isInVirtualReadSetPerfectFilter(int thread, Address addr);
  bool isInVirtualWriteSetPerfectFilter(int thread, Address addr);
  bool isInVirtualReadSetFilter(int thread, Address addr);
  bool isInVirtualWriteSetFilter(int thread, Address addr);
  void addToVirtualReadSetPerfectFilter(int thread, Address addr);
  void addToVirtualWriteSetPerfectFilter(int thread, Address addr);
  void addToVirtualReadSetFilter(int thread, Address addr);
  void addToVirtualWriteSetFilter(int thread, Address addr);
  void clearVirtualReadSetPerfectFilter(int thread);
  void clearVirtualWriteSetPerfectFilter(int thread);
  void clearVirtualReadSetFilter(int thread);
  void clearVirtualWriteSetFilter(int thread);

  /* These functions manipulate the summary Read/Write filter */
 	Map<Address, char> &getSummaryReadSetPerfectFilter(int thread);
	Map<Address, char> &getSummaryWriteSetPerfectFilter(int thread);
	GenericBloomFilter *getSummaryReadSetFilter(int thread);
	GenericBloomFilter *getSummaryWriteSetFilter(int thread);
  bool isInSummaryReadSetPerfectFilter(int thread, Address addr);
  bool isInSummaryWriteSetPerfectFilter(int thread, Address addr);
  bool isInSummaryReadSetFilter(int thread, Address addr);
  bool isInSummaryWriteSetFilter(int thread, Address addr);
  void addToSummaryReadSetPerfectFilter(int thread, Address addr);
  void addToSummaryWriteSetPerfectFilter(int thread, Address addr);
  void addToSummaryReadSetFilter(int thread, Address addr);
  void addToSummaryWriteSetFilter(int thread, Address addr);
  void removeFromSummaryReadSetPerfectFilter(int thread, Address addr);
  void removeFromSummaryWriteSetPerfectFilter(int thread, Address addr);
  void removeFromSummaryReadSetFilter(int thread, Address addr);
  void removeFromSummaryWriteSetFilter(int thread, Address addr);
  int  readBitSummaryReadSetFilter(int thread, int index);
  void writeBitSummaryReadSetFilter(int thread, int index, int value);
  int  readBitSummaryWriteSetFilter(int thread, int index);
  void writeBitSummaryWriteSetFilter(int thread, int index, int value);
  void clearSummaryReadSetPerfectFilter(int thread);
  void clearSummaryWriteSetPerfectFilter(int thread);
  void clearSummaryReadSetFilter(int thread);
  void clearSummaryWriteSetFilter(int thread);
  int  getIndexSummaryFilter(int thread, Address addr);
 	void mergeVirtualWithSummaryReadSet(int thread, GenericBloomFilter *);
	void mergeVirtualWithSummaryReadSet(int thread, Map<Address, char> &virtualReadSet);
	void mergeVirtualWithSummaryWriteSet(int thread, GenericBloomFilter *);
	void mergeVirtualWithSummaryWriteSet(int thread, Map<Address, char> &virtualWriteSet);

  void profileReadSetFilterActivity(int xid, int thread, bool isCommit);
  void profileWriteSetFilterActivity(int xid, int thread, bool isCommit);

  int  getReadSetSize(int thread, int xact_level);
  int  getWriteSetSize(int thread, int xact_level);

  void setVersion(int version);
  int getVersion() const;
	void setSummaryConflictAddress(Address addr);
	void setSummaryConflictType(unsigned int type);
	Address getSummaryConflictAddress();
	unsigned int getSummaryConflictType();

private:
  int getProcID() const;
  int getLogicalProcID(int thread) const;
  
  AbstractChip * m_chip_ptr;
  int m_version;

	Address m_summaryConflictAddress; 
	unsigned int m_summaryConflictType;

  Vector< Vector< Map <Address, char> > > m_readSet;
  Vector< Vector< Map <Address, char> > > m_writeSet;
  /* The Read/Write set Bloom filters */
  Vector<GenericBloomFilter*>  m_readSetFilter;
  Vector<GenericBloomFilter*>  m_writeSetFilter;

  Vector< Vector<int> > m_xact_readCount;
  Vector< Vector<int> > m_xact_writeCount;
  Vector< Vector<int> > m_xact_overflow_readCount;
  Vector< Vector<int> > m_xact_overflow_writeCount;

  /* 
   * The Virtualized and Summary Read/Write Perfect sets 
   * LOGTM_SE : Virtualized contains physical addresses of a virtualized transaction
   *            Physical addresses are virtualized using page-remapping
   * LOGTM_VSE: Virtualized contains virtual addresses of a virtualized transaction
   */
  Vector< Map<Address, char> > m_virtualReadSet;
  Vector< Map<Address, char> > m_virtualWriteSet;
  Vector< Map<Address, char> > m_summaryReadSet;
  Vector< Map<Address, char> > m_summaryWriteSet;
  /* The Virtualized and Summary Read/Write set Bloom filters */
  Vector<GenericBloomFilter*>  m_virtualReadSetFilter;
  Vector<GenericBloomFilter*>  m_virtualWriteSetFilter;
  Vector<GenericBloomFilter*>  m_summaryReadSetFilter;
  Vector<GenericBloomFilter*>  m_summaryWriteSetFilter;
};  
 

#endif

