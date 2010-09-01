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

#include "SimicsDriver.h"
#include "interface.h"
#include "System.h"
#include "SubBlock.h"
#include "Profiler.h"
#include "SimicsProcessor.h"
#include "SimicsHypervisor.h"
#include "TransactionInterfaceManager.h"
#include "TransactionIsolationManager.h"

#include <assert.h>
#include <stdlib.h>

// Simics includes
extern "C" {
#include "simics/api.h"
}

#define CHIP g_system_ptr->getChip(SIMICS_current_processor_number()/RubyConfig::numberOfProcsPerChip()/RubyConfig::numberofSMTThreads())

SimicsHypervisor::SimicsHypervisor(System* sys_ptr, Vector<SimicsProcessor*> processors) {
  int num_of_processors = RubyConfig::numberOfProcessors(); 

  m_processors = processors;
  
  m_idle_xactmgr.setSize(num_of_processors);
  for (int i = 0; i < num_of_processors; i++) {
    m_idle_xactmgr[i] = m_processors[i]->getTransactionInterfaceManager();
  }
 
  // per hardware thread 
  m_last_user_thread.setSize(num_of_processors);
 	m_last_primary_context.setSize(num_of_processors);
  m_g7_UserToPrivilegeMode.setSize(num_of_processors);
 	m_g7_PrivilegeToUserMode.setSize(num_of_processors);
  m_primary_context_UserToPrivilegeMode.setSize(num_of_processors);
 	m_primary_context_PrivilegeToUserMode.setSize(num_of_processors);
  for (int i=0; i<RubyConfig::numberOfProcsPerChip(); i++) {
 	  m_last_user_thread[i] = 0;
   	m_last_primary_context[i] = 0;
    m_g7_UserToPrivilegeMode[i] = 0;
 	  m_g7_PrivilegeToUserMode[i] = 0;
   	m_primary_context_UserToPrivilegeMode[i] = 0;
    m_primary_context_PrivilegeToUserMode[i] = 0;
 	}

  m_xactmgrToTHEntryMap_ptr = new Map<uint64, THEntry*>;
  // per process
  m_processesMap_ptr = new Map<uinteger_t, PHEntry*>;
}

SimicsHypervisor::~SimicsHypervisor() {
}

void SimicsHypervisor::change_mode_callback(void* desc, void* cpu, integer_t old_mode, integer_t new_mode)
{
  if (old_mode == 0 && new_mode == 1) {
    modeUserToPrivilege(cpu);
  } else if (old_mode == 1 && new_mode == 0) {
    modePrivilegeToUser(cpu);
  }
}

void SimicsHypervisor::modeUserToPrivilege(void *cpu) {
  int proc_no = SIM_get_proc_no((conf_object_t*) cpu);
  conf_object_t* cpu_obj = (conf_object_t*) cpu;
  conf_object_t* mmu_obj = SIM_get_attribute( cpu_obj, "mmu" ).u.object;
  sparc_v9_interface_t *sparc_v9 = (sparc_v9_interface_t*) SIM_get_interface(cpu_obj, "sparc-v9");
  m_g7_UserToPrivilegeMode[proc_no] = sparc_v9->read_global_register(cpu_obj, 0, 7);
  m_primary_context_UserToPrivilegeMode[proc_no] = SIM_get_attribute(mmu_obj, "ctxt-primary").u.integer & 0x1fff;

  if (0 && XACT_DEBUG && XACT_DEBUG_LEVEL > 1) {
    cout << g_eventQueue_ptr->getTime() << " " << proc_no << " [" << proc_no / RubyConfig::numberofSMTThreads() << "," 
          << proc_no % RubyConfig::numberofSMTThreads() << "]  UserToPrivilege: " << hex << m_primary_context_UserToPrivilegeMode[proc_no] << ":"
          << m_g7_UserToPrivilegeMode[proc_no] << dec << endl;
  }
}

void SimicsHypervisor::modePrivilegeToUser(void *cpu) {
  int proc_no = SIM_get_proc_no((conf_object_t*) cpu);
  conf_object_t* cpu_obj = (conf_object_t*) cpu;
  conf_object_t* mmu_obj = SIM_get_attribute( cpu_obj, "mmu" ).u.object;
  sparc_v9_interface_t *sparc_v9 = (sparc_v9_interface_t*) SIM_get_interface(cpu_obj, "sparc-v9");
  uinteger_t g07 = sparc_v9->read_global_register(cpu_obj, 0, 7);
  m_g7_PrivilegeToUserMode[proc_no] = sparc_v9->read_global_register(cpu_obj, 0, 7);
  m_primary_context_PrivilegeToUserMode[proc_no] = SIM_get_attribute(mmu_obj, "ctxt-primary").u.integer & 0x1fff;

  if (0 && XACT_DEBUG && XACT_DEBUG_LEVEL > 1) {
    cout << g_eventQueue_ptr->getTime() << " " << proc_no << " [" << proc_no / RubyConfig::numberofSMTThreads() << ","
          << proc_no % RubyConfig::numberofSMTThreads() << "]  PrivilegeToUser: " << hex << m_primary_context_PrivilegeToUserMode[proc_no] << ":"
          << m_g7_PrivilegeToUserMode[proc_no] << dec << endl;
  }

  integer_t new_primary_context = m_primary_context_PrivilegeToUserMode[proc_no];
  uinteger_t new_user_thread = m_g7_PrivilegeToUserMode[proc_no];

  int cpuLastRunOn = -1; 
  if ((new_primary_context == m_primary_context_UserToPrivilegeMode[proc_no] 
        && new_user_thread == m_g7_UserToPrivilegeMode[proc_no])) {
    PHEntry* proc_entry;
    THEntry* thread_entry;
    if (m_processesMap_ptr->exist(new_primary_context)) {
      proc_entry = m_processesMap_ptr->lookup(new_primary_context);
      if (proc_entry->m_threadsMap_ptr->exist(new_user_thread)) {
        thread_entry = proc_entry->m_threadsMap_ptr->lookup(new_user_thread);
        cpuLastRunOn = thread_entry->m_cpuLastRunOn;    
      }
    }
  }


  if ( (new_primary_context != m_primary_context_UserToPrivilegeMode[proc_no] 
          || new_user_thread != m_g7_UserToPrivilegeMode[proc_no])
       ||  (cpuLastRunOn != proc_no && cpuLastRunOn != -1))   {
    // context switch event

    // Check if a registered thread that was previously running on the current processor has been descheduled,
    // but not yet migrated on another processor.
    TransactionInterfaceManager *last_thread_xact_mgr = m_processors[proc_no]->getTransactionInterfaceManager();
    if (m_xactmgrToTHEntryMap_ptr->exist((uint64) last_thread_xact_mgr)) {
      // The transaction manager associated with current processor is for a registered thread.
      // This manager gets descheduled, therefore we need to keep its state isolated using virtualized/summary signatures   
      THEntry* last_thread_entry = m_xactmgrToTHEntryMap_ptr->lookup((uint64) last_thread_xact_mgr);
      last_thread_entry->m_isRunning = 0;
      last_thread_entry->m_isVirtualized = 1;
      moveVirtualToSummarySignature(last_thread_xact_mgr);
      last_thread_xact_mgr->getXactIsolationManager()->releaseIsolation(0,1);
    }



    // A new thread is scheduled on the current processor after it was previously descheduled. We need to:
    // - update all other summary signatures with our virtual signature.
    // - clear our physical signature
    // - migrate our transactional manager from the last processor we ran on, to the current processor   

    PHEntry* proc_entry;
    THEntry* thread_entry;
    TransactionInterfaceManager* xact_mgr;
    if (!m_processesMap_ptr->exist(new_primary_context)) {
      // no thread with this context has been registered - ignore it by using the idle transaction manager  
      m_processors[proc_no]->setTransactionInterfaceManager(m_idle_xactmgr[proc_no]);
      CHIP->setTransactionInterfaceManager(m_idle_xactmgr[proc_no], proc_no);
      return;
    } else {
      proc_entry = m_processesMap_ptr->lookup(new_primary_context);
    }
    
    if (!proc_entry->m_threadsMap_ptr->exist(new_user_thread)) {
      // this thread has not been registered - ignore it by using the idle transaction manager  
      m_processors[proc_no]->setTransactionInterfaceManager(m_idle_xactmgr[proc_no]);
      CHIP->setTransactionInterfaceManager(m_idle_xactmgr[proc_no], proc_no);  
      return;
    } else {
      thread_entry = proc_entry->m_threadsMap_ptr->lookup(new_user_thread);
      xact_mgr = thread_entry->m_xact_mgr;
      assert(m_xactmgrToTHEntryMap_ptr->exist((uint64) xact_mgr));
    }

    moveVirtualToSummarySignature(xact_mgr);
    xact_mgr->getXactIsolationManager()->releaseIsolation(0,1);

    xact_mgr->setVersion(proc_no);
    // I migrated, remove my transaction manager from the processor I last run on if my transaction manager is still
    // associated with that processor  
    if (m_processors[thread_entry->m_cpuLastRunOn]->getTransactionInterfaceManager() == xact_mgr) {
      CHIP->setTransactionInterfaceManager(m_idle_xactmgr[thread_entry->m_cpuLastRunOn], thread_entry->m_cpuLastRunOn);  
      m_processors[thread_entry->m_cpuLastRunOn]->setTransactionInterfaceManager(m_idle_xactmgr[thread_entry->m_cpuLastRunOn]);
    }
    m_processors[proc_no]->setTransactionInterfaceManager(xact_mgr);
    CHIP->setTransactionInterfaceManager(xact_mgr, proc_no);  

    thread_entry->m_isRunning = 1;
    thread_entry->m_isVirtualized = 1;
    thread_entry->m_cpuLastRunOn = proc_no;

    if (XACT_DEBUG && XACT_DEBUG_LEVEL > 1) {
      cout << g_eventQueue_ptr->getTime() << " " << proc_no << " [" << proc_no / RubyConfig::numberofSMTThreads() << ","
            << proc_no % RubyConfig::numberofSMTThreads() << "]  CONTEXT SWITCH " << hex << " NEW " 
            << new_primary_context << ":" << new_user_thread  << dec << endl; 
    }
    thread_entry->m_numContextSwitches++;
  }
}

// Catches DTLB MMU Map events
void SimicsHypervisor::dtlb_map_callback(void * desc, void * mmu, integer_t tag_reg, integer_t data_reg){
  conf_object_t * current_cpu = SIM_current_processor();
  int current_proc_no = SIM_get_proc_no(current_cpu);
  const char * mmu_name = ((conf_object_t*) mmu)->name;
  Address my_tag_reg = Address(tag_reg);
  Address my_data_reg = Address(data_reg);
  physical_address_t va = my_tag_reg.bitSelect(13, 63);
  physical_address_t context = my_tag_reg.bitSelect(0, 12);
  physical_address_t pa = my_data_reg.bitSelect(13, 42);
  physical_address_t size = my_data_reg.bitSelect(61,62);
  physical_address_t write_bit = my_data_reg.bitSelect(1,1);
  
  if (0 && XACT_DEBUG && XACT_DEBUG_LEVEL > 1) {
    cout << g_eventQueue_ptr->getTime() << " " << current_proc_no << " [" << current_proc_no / RubyConfig::numberofSMTThreads() << ","
          << current_proc_no % RubyConfig::numberofSMTThreads() << "]  DTLB_MAP  " << hex << context << ": " << va << " --> " << pa << dec << endl;
  }

  if(context == 0x0){
    // disregard any mappings from OS
    return;
  }

  PHEntry* proc_entry;
  if (!m_processesMap_ptr->exist(context)) {
    proc_entry = new PHEntry();
    m_processesMap_ptr->add(context, proc_entry);
  } else {
    proc_entry = m_processesMap_ptr->lookup(context);
  }

  if (proc_entry->m_pageMappings_ptr->exist(va)) {
    physical_address_t oldPhysicalPageMap = proc_entry->m_pageMappings_ptr->lookup(va);
    if (oldPhysicalPageMap != pa) {
      // EVENT: virtual page was paged out and paged in on a different physical page
      if (XACT_DEBUG && XACT_DEBUG_LEVEL > 1) {
        cout << g_eventQueue_ptr->getTime() << " " << current_proc_no << " [" << current_proc_no / RubyConfig::numberofSMTThreads() << ","
              << current_proc_no % RubyConfig::numberofSMTThreads() << "]  PAGE IN " << hex << context <<": old_mapping: " << va << " --> " << oldPhysicalPageMap
              << "  new_mapping: " << va << " --> " << pa << dec << endl;
      }

      // update with new mapping
      proc_entry->m_pageMappings_ptr->erase(va);
      proc_entry->m_pageMappings_ptr->add(va, pa);

      if(proc_entry->m_xactPages_ptr->exist(va) && (proc_entry->m_xactPages_ptr->lookup(va) == true)){
  			Vector<uinteger_t> keys = proc_entry->m_threadsMap_ptr->keys();
		 		for(int i=0; i < keys.size(); ++i) {
  				THEntry * i_thread_entry = proc_entry->m_threadsMap_ptr->lookup(keys[i]);
					TransactionInterfaceManager *i_xact_mgr = i_thread_entry->m_xact_mgr;
	      	i_thread_entry->m_numXactPageRemaps++;
					i_thread_entry->m_isVirtualized = 1;
					if (XACT_ENABLE_VIRTUALIZATION_LOGTM_SE) {
						pageWalk(i_xact_mgr, oldPhysicalPageMap, pa);
					}
      		i_xact_mgr->getXactIsolationManager()->releaseIsolation(0,1);
				}
			}
	 		recalcAllSummarySignatures(proc_entry);
    }
  } else {
    proc_entry->m_pageMappings_ptr->add(va, pa);
    if (0 && XACT_DEBUG && XACT_DEBUG_LEVEL > 1) {
      cout << g_eventQueue_ptr->getTime() << " " << current_proc_no << " [" << current_proc_no / RubyConfig::numberofSMTThreads() << ","
            << current_proc_no % RubyConfig::numberofSMTThreads() << "]  PAGE IN " << hex << context << ": new mapping: " << va
						<< " --> " << pa << dec << endl;
    }
  }
}


// Catches DTLB MMU Demap events
void SimicsHypervisor::dtlb_demap_callback(void * desc, void * mmu, integer_t tag_reg, integer_t data_reg){
  conf_object_t * current_cpu = SIM_current_processor();
  int current_proc_no = SIM_get_proc_no(current_cpu);
  const char * mmu_name = ((conf_object_t*) mmu)->name;
  Address my_tag_reg = Address(tag_reg);
  Address my_data_reg = Address(data_reg);
  physical_address_t va = my_tag_reg.bitSelect(13, 63);
  physical_address_t context = my_tag_reg.bitSelect(0, 12);
  physical_address_t pa = my_data_reg.bitSelect(13, 42);
  physical_address_t size = my_data_reg.bitSelect(61,62);
  physical_address_t write_bit = my_data_reg.bitSelect(1,1);

  if (0 && XACT_DEBUG && XACT_DEBUG_LEVEL > 1) {
    cout << g_eventQueue_ptr->getTime() << " " << current_proc_no << " [" << current_proc_no / RubyConfig::numberofSMTThreads() << ","
          << current_proc_no % RubyConfig::numberofSMTThreads() << "]  DTLB_DEMAP  " << hex << context << ": " << va << " --> " << pa << dec << endl;
  }

  if(context == 0x0){
    // disregard any mappings from OS
    return;
  }

  PHEntry* proc_entry;
  if (!m_processesMap_ptr->exist(context)) {
    proc_entry = new PHEntry();
    m_processesMap_ptr->add(context, proc_entry);
  } else {
    proc_entry = m_processesMap_ptr->lookup(context);
  }

  if (!proc_entry->m_pageMappings_ptr->exist(va)) {
    proc_entry->m_pageMappings_ptr->add(va, pa);
    if (0 && XACT_DEBUG && XACT_DEBUG_LEVEL > 1) {
      cout << g_eventQueue_ptr->getTime() << " " << current_proc_no << " [" << current_proc_no / RubyConfig::numberofSMTThreads() << ","
            << current_proc_no % RubyConfig::numberofSMTThreads() << "]  PAGE OUT: mapping: " << hex << va << " --> " << pa << dec << endl;
    }
  }

  //optimization: check whether this page could be a transactional page by whether any transactional thread exists currently
  bool existTransThread = false;
  Map<uinteger_t, THEntry*>* threads = proc_entry->m_threadsMap_ptr;
  Vector<uinteger_t> threadIDs = threads->keys();
  int num_threads = threadIDs.size();
  for(int i=0; !existTransThread && (i < num_threads); ++i){
    if((threads->lookup(threadIDs[i]))->m_xact_mgr->getTransactionLevel(0) > 0){
      existTransThread = true;
    }
  }
  // update our transaction pages map
  if(proc_entry->m_xactPages_ptr->exist(va)){
    proc_entry->m_xactPages_ptr->lookup(va) = existTransThread;
    assert(proc_entry->m_xactPages_ptr->lookup(va) == existTransThread);
  }
  else{
    if(existTransThread){
      // only add an entry if we are transactional page
      proc_entry->m_xactPages_ptr->add(va, existTransThread);
    }
  }
}

// Catches DTLB MMU Overwrite events
void SimicsHypervisor::dtlb_overwrite_callback(void * desc, void * mmu, integer_t tag_reg, integer_t data_reg){
  conf_object_t * current_cpu = SIM_current_processor();
  int current_proc_no = SIM_get_proc_no(current_cpu);
  const char * mmu_name = ((conf_object_t*) mmu)->name;
  Address my_tag_reg = Address(tag_reg);
  Address my_data_reg = Address(data_reg);
  physical_address_t va = my_tag_reg.bitSelect(13, 63);
  physical_address_t context = my_tag_reg.bitSelect(0, 12);
  physical_address_t pa = my_data_reg.bitSelect(13, 42);
  physical_address_t size = my_data_reg.bitSelect(61,62);
  physical_address_t write_bit = my_data_reg.bitSelect(1,1);

  if (0 && XACT_DEBUG && XACT_DEBUG_LEVEL > 1) {
    cout << g_eventQueue_ptr->getTime() << " " << current_proc_no << " [" << current_proc_no / RubyConfig::numberofSMTThreads() << ","
          << current_proc_no % RubyConfig::numberofSMTThreads() << "]  DTLB_OVERWRITE  " << hex << context << ": " << va << " --> " << pa << dec << endl;
  }


  if(context == 0x0){
    // disregard any mappings from OS
    return;
  }
  
  dtlb_demap_callback(desc, mmu, tag_reg, data_reg);
}

// Catches DTLB MMU Replace events
void SimicsHypervisor::dtlb_replace_callback(void * desc, void * mmu, integer_t tag_reg, integer_t data_reg){
  conf_object_t * current_cpu = SIM_current_processor();
  int current_proc_no = SIM_get_proc_no(current_cpu);
  const char * mmu_name = ((conf_object_t*) mmu)->name;
  Address my_tag_reg = Address(tag_reg);
  Address my_data_reg = Address(data_reg);
  physical_address_t va = my_tag_reg.bitSelect(13, 63);
  physical_address_t context = my_tag_reg.bitSelect(0, 12);
  physical_address_t pa = my_data_reg.bitSelect(13, 42);
  physical_address_t size = my_data_reg.bitSelect(61,62);
  physical_address_t write_bit = my_data_reg.bitSelect(1,1);

  if (0 && XACT_DEBUG && XACT_DEBUG_LEVEL > 1) {
    cout << g_eventQueue_ptr->getTime() << " " << current_proc_no << " [" << current_proc_no / RubyConfig::numberofSMTThreads() << ","
          << current_proc_no % RubyConfig::numberofSMTThreads() << "]  DTLB_REPLACE  " << hex << context << ": " << va << " --> " << pa << dec << endl;
  }

  if(context == 0x0){
    // disregard any mappings from OS
    return;
  }

  dtlb_demap_callback(desc, mmu, tag_reg, data_reg);
}

void SimicsHypervisor::registerThreadWithHypervisor(int proc_no) {
 	TransactionInterfaceManager* xact_mgr;
 	TransactionInterfaceManager* other_xact_mgr;
 	TransactionIsolationManager* xact_isol_mgr;
 	TransactionIsolationManager* other_xact_isol_mgr;
	uinteger_t tid, pid; 
	
  conf_object_t * cpu_obj = SIM_current_processor();
 	conf_object_t* mmu_obj = SIM_get_attribute( cpu_obj, "mmu" ).u.object;
 	sparc_v9_interface_t *sparc_v9 = (sparc_v9_interface_t*) SIM_get_interface(cpu_obj, "sparc-v9");
  tid = sparc_v9->read_global_register(cpu_obj, 0, 7);
 	pid = SIM_get_attribute(mmu_obj, "ctxt-primary").u.integer & 0x1fff;

  PHEntry* proc_entry;
  if (!m_processesMap_ptr->exist(pid)) {
    proc_entry = new PHEntry();
    m_processesMap_ptr->add(pid, proc_entry);
  } else {
    proc_entry = m_processesMap_ptr->lookup(pid);
  }
  // first time the hypervisor sees that thread
  assert(!proc_entry->m_threadsMap_ptr->exist(tid));  

	xact_mgr = new TransactionInterfaceManager(CHIP, proc_no);
	m_processors[proc_no]->setTransactionInterfaceManager(xact_mgr);
	CHIP->setTransactionInterfaceManager(xact_mgr, proc_no);	
	THEntry* thread_entry = new THEntry(xact_mgr);
	proc_entry->m_threadsMap_ptr->add(tid, thread_entry);
	m_xactmgrToTHEntryMap_ptr->add((uint64) xact_mgr, thread_entry);
	thread_entry->m_processPHEntry = proc_entry;

  thread_entry->m_cpuLastRunOn = proc_no;
 	m_last_user_thread[proc_no] = tid; 
  m_last_primary_context[proc_no] = pid;
	thread_entry->m_isRunning = 1;

	// create a summary signature for me using the virtualized signatures of all other virtualized threads
	calcMySummarySignature(xact_mgr);

  if (XACT_DEBUG && XACT_DEBUG_LEVEL > 1) {
 	   cout << g_eventQueue_ptr->getTime() << " " << proc_no << " [" << proc_no / RubyConfig::numberofSMTThreads() << ","
	         << proc_no % RubyConfig::numberofSMTThreads() << "]  REGISTER THREAD " << hex << pid << ":" << tid << dec << endl;
	}
}


// checks for conflicts with the summary Signature
unsigned int SimicsHypervisor::existSummarySignatureConflict(Address addr, bool isRead, TransactionInterfaceManager *xact_mgr){
  TransactionIsolationManager* xact_isol_mgr = xact_mgr->getXactIsolationManager();
  if (!m_xactmgrToTHEntryMap_ptr->exist((uint64) xact_mgr)) {
    // if there is no such mapping then the thread has not been registered with hypervisor
    return false;
  }
  // use line addresses
  addr.makeLineAddress();

  THEntry * current_thread_entry = m_xactmgrToTHEntryMap_ptr->lookup((uint64) xact_mgr);
  PHEntry * entry = current_thread_entry->m_processPHEntry; 
  assert(current_thread_entry->m_isRunning);

  if(isRead) {
    // check Summary Write signature
		bool perfect_result = xact_isol_mgr->isInSummaryWriteSetPerfectFilter(0, addr);
    bool imperfect_result = perfect_result;
    if(!PERFECT_SUMMARY_FILTER){
			imperfect_result = xact_isol_mgr->isInSummaryWriteSetFilter(0, addr);
      // no false negatives
      assert(!(perfect_result && !imperfect_result));
    }

    if (imperfect_result) {
			return CONFLICT_IS_READER | CONFLICT_WITH_SUMMARY_WRITE;
		} else {
			return 0;
		}
  } else {
    // check Summary Read & Write signature
		bool perfect_result_read  = xact_isol_mgr->isInSummaryReadSetPerfectFilter(0, addr);
		bool perfect_result_write = xact_isol_mgr->isInSummaryWriteSetPerfectFilter(0, addr);
		bool perfect_result = perfect_result_read || perfect_result_write;
		bool imperfect_result_read = perfect_result_read;
		bool imperfect_result_write = perfect_result_write;
    bool imperfect_result = perfect_result;
    if(!PERFECT_SUMMARY_FILTER){
			bool imperfect_result_read  = xact_isol_mgr->isInSummaryReadSetFilter(0, addr);
			bool imperfect_result_write =	xact_isol_mgr->isInSummaryWriteSetFilter(0, addr);
			imperfect_result = imperfect_result_read || imperfect_result_write;
      // no false negatives
      assert(!(perfect_result && !imperfect_result));
    }
	
		if (imperfect_result_read && imperfect_result_write) {	
			return CONFLICT_IS_WRITER | CONFLICT_WITH_SUMMARY_WRITE | CONFLICT_WITH_SUMMARY_READ ;
		} else if (imperfect_result_read) {	
			return CONFLICT_IS_WRITER | CONFLICT_WITH_SUMMARY_READ;
		} else if (imperfect_result_write) {
			return CONFLICT_IS_WRITER | CONFLICT_WITH_SUMMARY_WRITE;
		} else {
			return 0;
		}
	}
}

// resolves conflicts with the summary Signature
void SimicsHypervisor::resolveSummarySignatureConflict(int proc_no){
	PHEntry* proc_entry;
	THEntry* local_thread_entry;
 	TransactionInterfaceManager* xact_mgr;
	uinteger_t pid, tid, conflictType;
	Address conflictAddr;

  conf_object_t * cpu_obj = SIM_current_processor();
 	conf_object_t* mmu_obj = SIM_get_attribute( cpu_obj, "mmu" ).u.object;
 	sparc_v9_interface_t *sparc_v9 = (sparc_v9_interface_t*) SIM_get_interface(cpu_obj, "sparc-v9");
  tid = sparc_v9->read_global_register(cpu_obj, 0, 7);
 	pid = SIM_get_attribute(mmu_obj, "ctxt-primary").u.integer & 0x1fff;

 	conflictAddr = Address(SIMICS_read_register(proc_no, SIMICS_get_register_number(proc_no, "i2")));
  conflictType = SIMICS_read_register(proc_no, SIMICS_get_register_number(proc_no, "i3"));

	assert(m_processesMap_ptr->exist(pid));
	proc_entry = m_processesMap_ptr->lookup(pid);
	assert(proc_entry->m_threadsMap_ptr->exist(tid));
	local_thread_entry = proc_entry->m_threadsMap_ptr->lookup(tid);
	xact_mgr = local_thread_entry->m_xact_mgr;
 
  assert(local_thread_entry->m_isRunning);

  // If Summary signature is stale then update it 
	if (local_thread_entry->m_SummaryIsStale) {
		calcMySummarySignature(xact_mgr);		
		local_thread_entry->m_SummaryIsStale = false;
	}

  TransactionIsolationManager* xact_isol_mgr = xact_mgr->getXactIsolationManager();

  // use line addresses
  conflictAddr.makeLineAddress();

  // Now set the NACKing threads' possible cycle flags
  if(conflictType & CONFLICT_WITH_SUMMARY_READ) {
	  Vector<uinteger_t> keys = proc_entry->m_threadsMap_ptr->keys();
    for(int i=0; i < keys.size(); ++i) {
      THEntry * thread_entry = proc_entry->m_threadsMap_ptr->lookup(keys[i]);
    	if(thread_entry != local_thread_entry && thread_entry->m_isVirtualized) {
					TransactionInterfaceManager *remote_xact_mgr = thread_entry->m_xact_mgr;
					TransactionIsolationManager *remote_xact_isol_mgr = remote_xact_mgr->getXactIsolationManager();
 				bool remote_perfect_result = remote_xact_isol_mgr->isInVirtualReadSetPerfectFilter(0, conflictAddr);
		    bool remote_imperfect_result = remote_perfect_result;
        if(!PERFECT_VIRTUAL_FILTER){
					remote_imperfect_result = remote_xact_isol_mgr->isInVirtualReadSetFilter(0, conflictAddr);
          // no false negatives
          assert(!(remote_perfect_result && !remote_imperfect_result));
        }
        if (remote_imperfect_result) {
				  Address myPC = SIMICS_get_program_counter(xact_mgr->getVersion());
     			if (XACT_DEBUG && XACT_DEBUG_LEVEL > 1) {
  						cout << g_eventQueue_ptr->getTime() << " " << "VIRTUAL READ CONFLICT ADDR = " << conflictAddr << endl 
										<< "  LOCAL  TID = " << xact_mgr->getTID(0) << " processor = " << dec << xact_mgr->getVersion() 
										<< " xact_level = " << xact_mgr->getTransactionLevel(0) << hex 
										<< " escape_depth = " << xact_mgr->getEscapeDepth(0) << hex 
										<< " PC = " << myPC 
										<< " possible_cycle =  " << xact_mgr->possibleCycle(0) << endl 
										<< "  REMOTE TID = " << remote_xact_mgr->getTID(0) << " processor = " << dec << remote_xact_mgr->getVersion() 
										<< " xact_level = " << remote_xact_mgr->getTransactionLevel(0) << hex
										<< " escape_depth = " << remote_xact_mgr->getEscapeDepth(0) << hex 
										<< " possible_cycle =  " << remote_xact_mgr->possibleCycle(0) << dec << endl;
					}	
					if (xact_mgr->possibleCycle(0) && remote_xact_mgr->getTimestamp(0) < xact_mgr->getTimestamp(0)) {
	     			xact_mgr->setAbortFlag(0, conflictAddr);
					} else if (remote_xact_mgr->getTimestamp(0) < xact_mgr->getTimestamp(0)) { 
     				if (XACT_DEBUG && XACT_DEBUG_LEVEL > 1) {
							cout << "RESOLVE SUMMARY CONFLICT: setPossibleCycle" << endl; 
						}
						xact_mgr->setPossibleCycle(0); 
					}
				}
			}
		}
	}

  if(conflictType & CONFLICT_WITH_SUMMARY_WRITE) {
	  Vector<uinteger_t> keys = proc_entry->m_threadsMap_ptr->keys();
    for(int i=0; i < keys.size(); ++i) {
      THEntry * thread_entry = proc_entry->m_threadsMap_ptr->lookup(keys[i]);
    	if(thread_entry != local_thread_entry && thread_entry->m_isVirtualized) {
					TransactionInterfaceManager *remote_xact_mgr = thread_entry->m_xact_mgr;
					TransactionIsolationManager *remote_xact_isol_mgr = remote_xact_mgr->getXactIsolationManager();
 				bool remote_perfect_result = remote_xact_isol_mgr->isInVirtualWriteSetPerfectFilter(0, conflictAddr);
		    bool remote_imperfect_result = remote_perfect_result;
        if(!PERFECT_VIRTUAL_FILTER){
					remote_imperfect_result = remote_xact_isol_mgr->isInVirtualWriteSetFilter(0, conflictAddr);
          // no false negatives
          assert(!(remote_perfect_result && !remote_imperfect_result));
        }
        if (remote_imperfect_result) {
				  Address myPC = SIMICS_get_program_counter(xact_mgr->getVersion());
     			if (XACT_DEBUG && XACT_DEBUG_LEVEL > 1) {
  						cout << g_eventQueue_ptr->getTime() << " " << "VIRTUAL WRITE CONFLICT ADDR = " << conflictAddr << endl 
										<< "  LOCAL  TID = " << xact_mgr->getTID(0) << " processor = " << dec << xact_mgr->getVersion() 
										<< " xact_level = " << xact_mgr->getTransactionLevel(0) << hex 
										<< " escape_depth = " << xact_mgr->getEscapeDepth(0) << hex 
										<< " PC = " << myPC 
										<< " possible_cycle =  " << xact_mgr->possibleCycle(0) << endl 
										<< "  REMOTE TID = " << remote_xact_mgr->getTID(0) << " processor = " << dec << remote_xact_mgr->getVersion() 
										<< " xact_level = " << remote_xact_mgr->getTransactionLevel(0) << hex
										<< " escape_depth = " << remote_xact_mgr->getEscapeDepth(0) << hex 
										<< " possible_cycle =  " << remote_xact_mgr->possibleCycle(0) << dec << endl;
					}	
					if (xact_mgr->possibleCycle(0) && remote_xact_mgr->getTimestamp(0) < xact_mgr->getTimestamp(0)) {
	     			xact_mgr->setAbortFlag(0, conflictAddr);
					} else if (remote_xact_mgr->getTimestamp(0) < xact_mgr->getTimestamp(0)) { 
     				if (XACT_DEBUG && XACT_DEBUG_LEVEL > 1) {
							cout << "RESOLVE SUMMARY CONFLICT: setPossibleCycle" << endl; 
						}
						xact_mgr->setPossibleCycle(0); 
					}
				}
			}
		}
	}

}

void SimicsHypervisor::moveVirtualToSummarySignature(TransactionInterfaceManager *xact_mgr){
  assert(m_xactmgrToTHEntryMap_ptr->exist((uint64) xact_mgr));
  THEntry * thread_entry = m_xactmgrToTHEntryMap_ptr->lookup((uint64) xact_mgr);
  PHEntry * proc_entry = thread_entry->m_processPHEntry; 
	TransactionIsolationManager *xact_isol_mgr = thread_entry->m_xact_mgr->getXactIsolationManager();
	TransactionIsolationManager *other_xact_isol_mgr;

  Vector<uinteger_t> keys = proc_entry->m_threadsMap_ptr->keys();
 
  for(int i=0; i < keys.size(); ++i){
    THEntry * other_thread_entry = proc_entry->m_threadsMap_ptr->lookup(keys[i]);
    // don't update ourselves
    if(other_thread_entry != thread_entry){
			other_xact_isol_mgr = other_thread_entry->m_xact_mgr->getXactIsolationManager();
			other_xact_isol_mgr->mergeVirtualWithSummaryReadSet(0, xact_isol_mgr->getVirtualReadSetPerfectFilter(0));	
			other_xact_isol_mgr->mergeVirtualWithSummaryWriteSet(0, xact_isol_mgr->getVirtualWriteSetPerfectFilter(0));	
      // Update Imperfect Summary if we are using Imperfect Virtual
      if(!PERFECT_VIRTUAL_FILTER && !PERFECT_SUMMARY_FILTER){
        // IMPORTANT: Assumes virtual and summary signatures are the SAME SIZE!
				other_xact_isol_mgr->mergeVirtualWithSummaryReadSet(0, xact_isol_mgr->getVirtualReadSetFilter(0));	
				other_xact_isol_mgr->mergeVirtualWithSummaryWriteSet(0, xact_isol_mgr->getVirtualWriteSetFilter(0));	
      }
    }
  }

	postMagicCrossCalls(xact_mgr);
}

void SimicsHypervisor::calcMySummarySignature(TransactionInterfaceManager *xact_mgr) {
  assert(m_xactmgrToTHEntryMap_ptr->exist((uint64) xact_mgr));
  THEntry * thread_entry = m_xactmgrToTHEntryMap_ptr->lookup((uint64) xact_mgr);
  PHEntry * proc_entry = thread_entry->m_processPHEntry; 
	TransactionIsolationManager *xact_isol_mgr = thread_entry->m_xact_mgr->getXactIsolationManager();
	TransactionIsolationManager *j_xact_isol_mgr;

  Vector<uinteger_t> keys = proc_entry->m_threadsMap_ptr->keys();
	xact_isol_mgr->clearSummaryReadSetPerfectFilter(0); 
	xact_isol_mgr->clearSummaryWriteSetPerfectFilter(0); 
	if (!PERFECT_SUMMARY_FILTER) {
		xact_isol_mgr->clearSummaryReadSetFilter(0); 
		xact_isol_mgr->clearSummaryWriteSetFilter(0); 
	}
 	for(int j=0; j < keys.size(); ++j) {
    THEntry * j_thread_entry = proc_entry->m_threadsMap_ptr->lookup(keys[j]);
  	if(j_thread_entry != thread_entry && j_thread_entry->m_isVirtualized) {
			j_xact_isol_mgr = j_thread_entry->m_xact_mgr->getXactIsolationManager();
 			Vector<Address> read_keys = j_xact_isol_mgr->getVirtualReadSetPerfectFilter(0).keys();
			for(int k = 0; k < j_xact_isol_mgr->getVirtualReadSetPerfectFilter(0).size(); k++) {
				xact_isol_mgr->addToSummaryReadSetPerfectFilter(0, read_keys[k]);
				if (!PERFECT_SUMMARY_FILTER) {
					xact_isol_mgr->addToSummaryReadSetFilter(0, read_keys[k]);		
				}
			}				 
			Vector<Address> write_keys = j_xact_isol_mgr->getVirtualWriteSetPerfectFilter(0).keys();
			for(int k = 0; k < j_xact_isol_mgr->getVirtualWriteSetPerfectFilter(0).size(); k++) {
				xact_isol_mgr->addToSummaryWriteSetPerfectFilter(0, write_keys[k]);
				if (!PERFECT_SUMMARY_FILTER) {
					xact_isol_mgr->addToSummaryWriteSetFilter(0, write_keys[k]);		
				}
 	    }
    } 
	}
}

void SimicsHypervisor::recalcAllSummarySignatures(TransactionInterfaceManager *xact_mgr) {
  assert(m_xactmgrToTHEntryMap_ptr->exist((uint64) xact_mgr));
  THEntry * thread_entry = m_xactmgrToTHEntryMap_ptr->lookup((uint64) xact_mgr);
  PHEntry * proc_entry = thread_entry->m_processPHEntry; 
	recalcAllSummarySignatures(proc_entry);
}

void SimicsHypervisor::recalcAllSummarySignatures(PHEntry * proc_entry) {
	TransactionIsolationManager *i_xact_isol_mgr;
	TransactionIsolationManager *j_xact_isol_mgr;

  Vector<uinteger_t> keys = proc_entry->m_threadsMap_ptr->keys();
  for(int i=0; i < keys.size(); ++i) {
	  THEntry * i_thread_entry = proc_entry->m_threadsMap_ptr->lookup(keys[i]);
		i_xact_isol_mgr = i_thread_entry->m_xact_mgr->getXactIsolationManager();
		i_xact_isol_mgr->clearSummaryReadSetPerfectFilter(0); 
		i_xact_isol_mgr->clearSummaryWriteSetPerfectFilter(0); 
		if (!PERFECT_SUMMARY_FILTER) {
			i_xact_isol_mgr->clearSummaryReadSetFilter(0); 
			i_xact_isol_mgr->clearSummaryWriteSetFilter(0); 
		}
  	for(int j=0; j < keys.size(); ++j) {
   	  THEntry * j_thread_entry = proc_entry->m_threadsMap_ptr->lookup(keys[j]);
   		if(j_thread_entry != i_thread_entry && j_thread_entry->m_isVirtualized) {
				j_xact_isol_mgr = j_thread_entry->m_xact_mgr->getXactIsolationManager();
 				Vector<Address> read_keys = j_xact_isol_mgr->getVirtualReadSetPerfectFilter(0).keys();
				for(int k = 0; k < j_xact_isol_mgr->getVirtualReadSetPerfectFilter(0).size(); k++) {
					i_xact_isol_mgr->addToSummaryReadSetPerfectFilter(0,read_keys[k]);
					if (!PERFECT_SUMMARY_FILTER) {
						i_xact_isol_mgr->addToSummaryReadSetFilter(0,read_keys[k]);
					}
				} 
 				Vector<Address> write_keys = j_xact_isol_mgr->getVirtualWriteSetPerfectFilter(0).keys();
				for(int k = 0; k < j_xact_isol_mgr->getVirtualWriteSetPerfectFilter(0).size(); k++) {
					i_xact_isol_mgr->addToSummaryWriteSetPerfectFilter(0, write_keys[k]);
					if (!PERFECT_SUMMARY_FILTER) {
						i_xact_isol_mgr->addToSummaryWriteSetFilter(0, write_keys[k]);
					}
				} 
 	    }
    } 
	}

	postMagicCrossCalls((TransactionInterfaceManager*) NULL);
}

// occurs when a processor Commits or Aborts a Transaction
void SimicsHypervisor::completeTransaction(TransactionInterfaceManager *xact_mgr){
	int thread = 0;
  if (!m_xactmgrToTHEntryMap_ptr->exist((uint64) xact_mgr)) {
    return;
  }
  THEntry * thread_entry = m_xactmgrToTHEntryMap_ptr->lookup((uint64) xact_mgr);
  PHEntry * proc_entry = thread_entry->m_processPHEntry;
 	TransactionIsolationManager *xact_isol_mgr = xact_mgr->getXactIsolationManager();
	if (thread_entry->m_isVirtualized) {
		thread_entry->m_isVirtualized = 0;
	  xact_isol_mgr->clearVirtualReadSetPerfectFilter(thread);
  	xact_isol_mgr->clearVirtualWriteSetPerfectFilter(thread);
	  if(!PERFECT_VIRTUAL_FILTER) {
  		xact_isol_mgr->clearVirtualReadSetFilter(thread);
  		xact_isol_mgr->clearVirtualWriteSetFilter(thread);
	  }

	  Vector<uinteger_t> keys = proc_entry->m_threadsMap_ptr->keys();
	  for(int i=0; i < keys.size(); ++i){
		   THEntry * i_thread_entry = proc_entry->m_threadsMap_ptr->lookup(keys[i]);
  	  i_thread_entry->m_SummaryIsStale = true;
		}
	}
}

void SimicsHypervisor::pageWalk(TransactionInterfaceManager *xact_mgr, physical_address_t old_phys_page, physical_address_t new_phys_page) {
  int i;
	TransactionIsolationManager* xact_isol_mgr = xact_mgr->getXactIsolationManager();
  if (!m_xactmgrToTHEntryMap_ptr->exist((uint64) xact_mgr)) {
		return;
	}	
	
	for (i=0; i < g_PAGE_SIZE_BYTES; i+=g_DATA_BLOCK_BYTES) {
		if (xact_isol_mgr->isInVirtualReadSetPerfectFilter(0, Address(old_phys_page*g_PAGE_SIZE_BYTES+i))) {
			xact_isol_mgr->addToVirtualReadSetPerfectFilter(0, Address(new_phys_page*g_PAGE_SIZE_BYTES+i));
		}	
		if (xact_isol_mgr->isInVirtualWriteSetPerfectFilter(0, Address(old_phys_page*g_PAGE_SIZE_BYTES+i))) {
			xact_isol_mgr->addToVirtualWriteSetPerfectFilter(0, Address(new_phys_page*g_PAGE_SIZE_BYTES+i));
		}	
		if(!PERFECT_VIRTUAL_FILTER) {
			if (xact_isol_mgr->isInVirtualReadSetFilter(0, Address(old_phys_page*g_PAGE_SIZE_BYTES+i))) {
				xact_isol_mgr->addToVirtualReadSetFilter(0, Address(new_phys_page*g_PAGE_SIZE_BYTES+i));
			}	
			if (xact_isol_mgr->isInVirtualWriteSetFilter(0, Address(old_phys_page*g_PAGE_SIZE_BYTES+i))) {
				xact_isol_mgr->addToVirtualWriteSetFilter(0, Address(new_phys_page*g_PAGE_SIZE_BYTES+i));
			}	
		}
	} 	
}

void SimicsHypervisor::postMagicCrossCalls(TransactionInterfaceManager* xact_mgr) {
	int proc_no;
	if (xact_mgr) {
		proc_no =  xact_mgr->getVersion();
	} else {
		conf_object_t * cpu_obj = SIM_current_processor();
  	proc_no = SIM_get_proc_no(cpu_obj);
	}
	postMagicCrossCalls(proc_no);
}

void SimicsHypervisor::postMagicCrossCalls(int proc_num) {
  int num_of_processors = RubyConfig::numberOfProcessors();
  for (int i = 0; i < num_of_processors; i++) {
		if (i==proc_num) { continue; }
		m_processors[i]->getTransactionSimicsProcessor()->setMagicCrossCall(true);
	}
}	
