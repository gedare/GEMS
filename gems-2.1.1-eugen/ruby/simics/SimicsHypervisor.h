
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

#ifndef SimicsHypervisor_H
#define SimicsHypervisor_H

#include "SimicsProcessor.h"

#include "Global.h"
#include "System.h"
#include "Profiler.h"
#include "Vector.h"
#include "Map.h"
#include "Address.h"
#include "GenericBloomFilter.h"
#include "RegisterStateWindowed.h"
#include "TransactionInterfaceManager.h"
#include "TransactionSimicsProcessor.h"
//#include "Sequencer.h"

// Simics includes
extern "C" {
#include "simics/api.h"
}

#define CONFLICT_IS_READER 0x0
#define CONFLICT_IS_WRITER 0x4
#define CONFLICT_WITH_SUMMARY_READ 0x1
#define CONFLICT_WITH_SUMMARY_WRITE 0x2

/*
class ShadowPageTableEntry {
public:
	ShadowPageTableEntry (uinteger_t p_vnode, uinteger_t p_offset, uinteger_t pfn, uint64 timestamp) {
		m_p_vnode = p_vnode;
		m_p_offset = p_offset;
		m_pfn = pfn;
		m_timestamp = timestamp;
	}
	uinteger_t m_p_vnode;
	uinteger_t m_p_offset;
	uinteger_t m_pfn;
	uint64 m_timestamp;
};

class ShadowPageTable {
public:
	ShadowPageTable(const unsigned int size) {
		table.setSize(size);
  	for (int i=0; i<size; i++) {
			table[i] = new Vector<ShadowPageTableEntry *>; 
		}	
	}
	ShadowPageTableEntry *newEntry(uinteger_t p_vnode, uinteger_t p_offset, uinteger_t pfn, uint64 timestamp) {
		unsigned int index = p_offset % table.size();
		Vector<ShadowPageTableEntry *> *bucket_list;   
		bucket_list = table[index];
		ShadowPageTableEntry *entry = new ShadowPageTableEntry(p_vnode, p_offset, pfn, timestamp);
		bucket_list->insertAtBottom(entry); 
		return entry;
	}
	ShadowPageTableEntry *findEntry(uinteger_t p_vnode, uinteger_t p_offset) {
		unsigned int index = p_offset % table.size();
		Vector<ShadowPageTableEntry *> *bucket_list = table[index];
		for (int i=0; i<bucket_list->size(); i++) {
			if ((*bucket_list)[i]->m_p_vnode == p_vnode && (*bucket_list)[i]->m_p_offset == p_offset) {
				return (*bucket_list)[i];
			} 
		}  
		return NULL;
	}

private:
	Vector<Vector<ShadowPageTableEntry *> *> table;	
};

*/

// PHEntry Process Hypervisor Entry
// THEntry Thread Hypervisor Entry

class PHEntry;

class THEntry {
public:
  THEntry(TransactionInterfaceManager* xact_mgr) { 
    m_xact_mgr = xact_mgr;
    m_isRunning = false;
    m_isVirtualized = false;
		m_SummaryIsStale = false;
  }
  ~THEntry();

  TransactionInterfaceManager* m_xact_mgr;

  bool m_isRunning;
  bool m_isVirtualized;
  bool m_SummaryIsStale;

  int m_cpuLastRunOn;
  
  //back pointer to process entry 
  PHEntry* m_processPHEntry;

  //per thread statistics
  unsigned int m_numContextSwitches;
  unsigned int m_numXactPageRemaps;
};
   
class PHEntry {
public:
  PHEntry() { 
    m_threadsMap_ptr = new Map<uinteger_t, THEntry*>;
    m_pageMappings_ptr = new Map<physical_address_t, physical_address_t>;
    m_xactPages_ptr = new Map<physical_address_t, char>;
  }
  ~PHEntry();

  Map<uinteger_t, THEntry*>* m_threadsMap_ptr;
  Map<physical_address_t, physical_address_t>* m_pageMappings_ptr;
  Map<physical_address_t, char>* m_xactPages_ptr;
};

class SimicsHypervisor {
public:
  SimicsHypervisor(System* sys_ptr, Vector<SimicsProcessor*>);
  ~SimicsHypervisor();

  void modeUserToPrivilege(void *cpu);
  void modePrivilegeToUser(void *cpu);
  void change_mode_callback(void* desc, void* cpu, integer_t old_mode, integer_t new_mode);
  void dtlb_map_callback(void * desc, void * mmu, integer_t tag_reg, integer_t data_reg);
  void dtlb_demap_callback(void * desc, void * mmu, integer_t tag_reg, integer_t data_reg);
  void dtlb_overwrite_callback(void * desc, void * mmu, integer_t tag_reg, integer_t data_reg);
  void dtlb_replace_callback(void * desc, void * mmu, integer_t tag_reg, integer_t data_reg);

  void registerThreadWithHypervisor(int proc_num);
  unsigned int existSummarySignatureConflict(Address virt_addr, bool isRead, TransactionInterfaceManager * xact_mgr);
  void completeTransaction(TransactionInterfaceManager * xact_mgr);
  void moveVirtualToSummarySignature(TransactionInterfaceManager * xact_mgr);
	void calcMySummarySignature(TransactionInterfaceManager *xact_mgr);
	void recalcAllSummarySignatures(TransactionInterfaceManager *xact_mgr);
	void recalcAllSummarySignatures(PHEntry * proc_entry);
  void resetUnstalledFlag(TransactionInterfaceManager* xact_mgr);
  uinteger_t getUnstalledPC(TransactionInterfaceManager* xact_mgr);
	void postMagicCrossCalls(TransactionInterfaceManager* xact_mgr);
	void postMagicCrossCalls(int proc_num);
	void resolveSummarySignatureConflict(int proc);
	void pageWalk(TransactionInterfaceManager *xact_mgr, physical_address_t old_phys_page, physical_address_t new_phys_page);

private:

  Map<uinteger_t, PHEntry*>          *m_processesMap_ptr;
  Map<uint64, THEntry*>          *m_xactmgrToTHEntryMap_ptr;

  Vector<SimicsProcessor*> m_processors;
  Vector<TransactionInterfaceManager*>   m_idle_xactmgr;

  // per hardware thread
  Vector<uinteger_t>         m_last_user_thread;
  Vector<uinteger_t>         m_g7_UserToPrivilegeMode;
  Vector<uinteger_t>         m_g7_PrivilegeToUserMode;
  Vector<integer_t>         m_last_primary_context;
  Vector<uinteger_t>         m_primary_context_UserToPrivilegeMode;
  Vector<uinteger_t>         m_primary_context_PrivilegeToUserMode;

};  

#endif

