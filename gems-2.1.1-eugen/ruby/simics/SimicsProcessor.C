
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


    The GNU General Public License is contained in the file LICENSE.

### END HEADER ###
*/

/*
 * $Id: SimicsProcessor.C 1.57 2006/11/06 17:41:01-06:00 bobba@gratiano.cs.wisc.edu $
 *
 * Description: See SimicsProcessor.h for document.
 */

#include "interface.h"
#include "System.h"
#include "SubBlock.h"
#include "Profiler.h"
#include "AddressProfiler.h"
#include "Chip.h"
#include "SimicsDriver.h"
#include "SimicsProcessor.h"
#include "SimicsHypervisor.h"
#include "TransactionInterfaceManager.h"
#include "TransactionIsolationManager.h"
#include "TransactionVersionManager.h"
#include "TransactionConflictManager.h"
#include "LazyTransactionVersionManager.h"
#include "TransactionSimicsProcessor.h"
#include "OpalInterface.h"
#include "XactIsolationChecker.h"
#include "XactCommitArbiter.h"

// Simics includes
extern "C" {
#include "simics/api.h"
}

// Macros to get privileged mode bit
#ifdef SPARC
  #define PRIV_MODE (mem_trans->priv)
#else
// x86
  #define PRIV_MODE (mem_trans->mode)
#endif

// Constructor
SimicsProcessor::SimicsProcessor(System* sys_ptr, int proc) {
  m_proc = proc;

  // get corresponding sequencer
  m_sequencer = sys_ptr->getChip(m_proc/RubyConfig::numberOfProcsPerChip())->getSequencer(m_proc%RubyConfig::numberOfProcsPerChip());
  assert(m_sequencer);

  m_xact_mgr = NULL;

  // get corresponding transaction manager
  if (XACT_MEMORY) {
    m_xact_proc = new TransactionSimicsProcessor(sys_ptr, proc); 
    m_xact_mgr = sys_ptr->getChip(m_proc/RubyConfig::numberOfProcsPerChip())->getTransactionInterfaceManager(m_proc%RubyConfig::numberOfProcsPerChip());
    assert(m_xact_mgr);
  }
  
  // This is the unique IC for a instruction from simics
  m_current_instruction_count = 0;

  // This is the unique cycle count for a instruction from simics
  m_current_cycle_count = 0;

  // initiate the request vector
  clearActiveRequestVector();

  // get the initial simics step counter
  m_initial_instruction_count = SIMICS_get_insn_count(m_proc);

  // get the initial cycle count
  m_initial_cycle_count = SIMICS_get_cycle_count(m_proc);

  // no lingering request initially
  m_lingering_request_address = Address(0-1);

  // sequence number for memory requests
  m_seqnum = 0;

  // Unstall the processors : a processor could be stalled at the begining of
  // the simulation if we load a checkpoint created with ruby running
  // In other words, there could be outstanding memory transactions that we didn't
  // unstalled before we took the checkpoint
  SIMICS_unstall_proc(m_proc);
}

TransactionSimicsProcessor* SimicsProcessor::getTransactionSimicsProcessor() {
  return m_xact_proc;
}

TransactionInterfaceManager* SimicsProcessor::getTransactionInterfaceManager() {
  return m_xact_mgr;
}

void SimicsProcessor::setTransactionInterfaceManager(TransactionInterfaceManager* xact_mgr) {
	m_xact_proc->setTransactionInterfaceManager(xact_mgr);
  m_xact_mgr = xact_mgr;
  assert(m_xact_mgr);
}

// private methods

// linear search is used in the following methods, since the request vector is
// mostly 1 or 2 entries long. Performance of linear search should be OK.
//
// The vector class is so good that even we constant change the size of the
// vector, it doesn't really allocate/de-allocate the memory for its entries.

int SimicsProcessor::lookupRequest(Address addr, CacheRequestType type, int size) {
  for(int i = 0; i < m_active_requests.size(); i++) {
    if (m_active_requests[i].status != Invalid &&
        m_active_requests[i].addr == addr &&
        m_active_requests[i].type == type ) {
      // found one
      assert(m_active_requests[i].size == size);
      return i;
    }
  }

  // not found
  return -1;
}

int SimicsProcessor::getServingRequestSlot() {
  int result = -1;
  int i;
  for(i = 0; i < m_active_requests.size(); i++) {
    if (m_active_requests[i].status == Serving) {
      // At most one serving request in the vector
      if (result != -1)
        cout << g_eventQueue_ptr->getTime() << " " << m_proc << " No Serving Request " << endl;        
      assert(result == -1);
      result = i;
    }
  }

  // not found
  return result;
}

int SimicsProcessor::getFreeSlotInActiveRequestVector() {
  int i;
  for(i = 0; i < m_active_requests.size(); i++) {
    if (m_active_requests[i].status == Invalid) {
      return i;
    }
  }

  // increase the vector length by one
  m_active_requests.expand(1);
  return i;
}

void SimicsProcessor::clearActiveRequestVector() {
  for(int i=0; i < m_active_requests.size(); i++){
    m_active_requests[i].status = Invalid;
  }
  m_active_requests.setSize(0);
}

// public methods

integer_t SimicsProcessor::getInstructionCount() const {
  return SIMICS_get_insn_count(m_proc) - m_initial_instruction_count;
}

integer_t SimicsProcessor::getCycleCount() const {
  return SIMICS_get_cycle_count(m_proc) - m_initial_cycle_count;
}

void SimicsProcessor::hitCallback(CacheRequestType type, SubBlock& data) {

  int idx = getServingRequestSlot();
  
  DEBUG_MSG(SEQUENCER_COMP, MedPrio, "hitCallback");
  DEBUG_EXPR(SEQUENCER_COMP, MedPrio, m_proc);
  DEBUG_EXPR(SEQUENCER_COMP, MedPrio, data.getAddress());
  DEBUG_EXPR(SEQUENCER_COMP, MedPrio, data.getSize());
  DEBUG_EXPR(SEQUENCER_COMP, MedPrio, idx);

  if (idx >= 0 && data.getAddress() == m_active_requests[idx].addr) {
    DEBUG_EXPR(SEQUENCER_COMP, MedPrio, m_active_requests[idx].addr);

    assert(m_active_requests[idx].status == Serving);
    if (m_active_requests[idx].size != data.getSize()){
      cout << " HIT CALLBACK FOR " << data.getAddress() << " size: " << data.getSize() << endl;
      cout << " ACTIVE REQUEST " << m_active_requests[idx].addr << " size: " << m_active_requests[idx].size << endl;
      assert(0);        
    }       

    m_active_requests[idx].status = Done;
    m_active_requests[idx].success = true;
  }          

  if (XACT_MEMORY) {
		if (XACT_ENABLE_VIRTUALIZATION_LOGTM_SE) {
			if (m_xact_proc->getMagicCrossCall()) {
				m_xact_proc->setMagicCrossCall(false);
        m_active_requests[idx].status = Done;
        m_active_requests[idx].success = false;
  			m_xact_proc->completedMemRef(false, type, data.getAddress(), data.getLogicalAddress());
			} else {
	    	m_xact_proc->completedMemRef(true, type, data.getAddress(), data.getLogicalAddress());
			}
		} else {
    	m_xact_proc->completedMemRef(true, type, data.getAddress(), data.getLogicalAddress());
		}
  } else {           
    SIMICS_unstall_proc(m_proc);
  }        
}

void SimicsProcessor::conflictCallback(CacheRequestType type, SubBlock& data) {
  assert(XACT_MEMORY);        

  int idx = getServingRequestSlot();

  DEBUG_MSG(SEQUENCER_COMP, MedPrio, "conflictCallback");
  DEBUG_EXPR(SEQUENCER_COMP, MedPrio, m_proc);
  DEBUG_EXPR(SEQUENCER_COMP, MedPrio, data.getAddress());
  DEBUG_EXPR(SEQUENCER_COMP, MedPrio, data.getSize());
  DEBUG_EXPR(SEQUENCER_COMP, MedPrio, idx);

  if (idx >= 0 && data.getAddress() == m_active_requests[idx].addr) {
    DEBUG_EXPR(SEQUENCER_COMP, MedPrio, m_active_requests[idx].addr);
    assert(m_active_requests[idx].status == Serving);
    assert(m_active_requests[idx].size == data.getSize());

    m_active_requests[idx].status = Done;
    m_active_requests[idx].success = false;
  }          

  m_xact_proc->completedMemRef(false, type, data.getAddress(), data.getLogicalAddress());
}
        
void SimicsProcessor::abortCallback(SubBlock& data) {
  assert(0);      
}

MemoryTransactionResult SimicsProcessor::makeRequest(
  memory_transaction_t* mem_trans) {

  DEBUG_MSG(SEQUENCER_COMP, MedPrio, "\n\nstart of makeRequest");
  DEBUG_EXPR(SEQUENCER_COMP, MedPrio, m_proc);

  DEBUG_EXPR(SEQUENCER_COMP, MedPrio, Address(mem_trans->s.physical_address));
  DEBUG_EXPR(SEQUENCER_COMP, MedPrio, mem_trans->s.size);
  DEBUG_EXPR(SEQUENCER_COMP, MedPrio, getRequestType(mem_trans));
  DEBUG_EXPR(SEQUENCER_COMP, MedPrio, mem_trans->s.may_stall);
  DEBUG_EXPR(SEQUENCER_COMP, MedPrio, m_current_instruction_count);
  DEBUG_EXPR(SEQUENCER_COMP, MedPrio, SIMICS_get_insn_count(m_proc));

  // All instruction fetches are stallable requests
  if(mem_trans->s.type == Sim_Trans_Instr_Fetch){
    if (PRINT_INSTRUCTION_TRACE && (g_eventQueue_ptr->getTime() >= g_DEBUG_CYCLE )){
      const char * instruction = SIMICS_disassemble_physical( m_proc, mem_trans->s.physical_address );
      // write instruction address
      cout << g_eventQueue_ptr->getTime() << " #" << m_proc << "\t" << getInstructionCount() << "\tVPC: " << Address(mem_trans->s.logical_address)
           << " PC: " << Address(mem_trans->s.physical_address)  << " " << instruction << dec << endl;
    }
    
    if (PROFILE_ALL_INSTRUCTIONS){   
      // call profiler with PC
      g_system_ptr->getProfiler()->getInstructionProfiler()->addTraceSample
        (Address(mem_trans->s.physical_address),
         Address(mem_trans->s.physical_address),
         CacheRequestType_IFETCH,
         PRIV_MODE ? AccessModeType_SupervisorMode :
                           AccessModeType_UserMode,
         m_proc,
         false);
    }
    assert(mem_trans->s.may_stall);
  }
  
  // blow away the request vector if we are serving a new instruction
  // presumablely, the lingering request has been saved.
  if (m_current_instruction_count != SIMICS_get_insn_count(m_proc)) {
    clearActiveRequestVector();
    m_current_instruction_count = SIMICS_get_insn_count(m_proc);
  }
  
  
  // Check to see if this is a new request or a duplicated instruction, data,
  // or atomic request.
  int idx = lookupRequest(Address(mem_trans->s.physical_address),
                          getRequestType(mem_trans), mem_trans->s.size);
  
    
  if (idx == -1) {
    // new request
    idx = getFreeSlotInActiveRequestVector();
    m_active_requests[idx].status = Queued;
    m_active_requests[idx].addr = Address(mem_trans->s.physical_address);
    m_active_requests[idx].size = mem_trans->s.size;
    m_active_requests[idx].type = getRequestType(mem_trans);
    m_active_requests[idx].success = true;
  } else {
    // dup request
    DEBUG_MSG(SEQUENCER_COMP, MedPrio, "Duplicate Request");    
    
    if (XACT_MEMORY){
      if (m_active_requests[idx].status == Done) {      
        bool success = m_active_requests[idx].success;
        m_xact_proc->readyToRetireMemRef(success, mem_trans, getRequestType(mem_trans));
        int outcome = m_xact_proc->canRetireMemRef(mem_trans, getRequestType(mem_trans), success);
        if (outcome == 1) {
          m_active_requests[idx].status = Trap;          
        } else if (outcome == 2) {
          clearActiveRequestVector();
          m_active_requests[idx].status = Retry;        
        } else {
          m_active_requests[idx].status = Done;
        }  
      }            
    }          
    switch(m_active_requests[idx].status) {
    case Queued:
      // haven't been handled, continue
      break;
    case Serving:
      ERROR_MSG("Error: Simics get unstalled without ruby asking?");
      break;
    case Done:
      switch(getRequestType(mem_trans)){
      case CacheRequestType_LD_XACT:
      case CacheRequestType_LD:
      case CacheRequestType_ST:
      case CacheRequestType_ST_XACT:
      case CacheRequestType_LDX_XACT:
        return D_Replay;
        break;
      case CacheRequestType_ATOMIC:
        return A_Replay;
        break;
      case CacheRequestType_IFETCH:
        return I_Replay;
        break;
      default:
        ERROR_MSG("Error: Strange request type?");
      }
      break;
    case Trap:
        assert(XACT_MEMORY);
        mem_trans->s.exception = 0x122;
        return D_Replay;  
    case Retry:
        assert(XACT_MEMORY);
        return Conflict_Retry;    
    default:
      ERROR_MSG("Error: Strange request status?");
    }
  } // dup request

  assert(m_active_requests[idx].status == Queued);

  CacheMsg request = memoryTransactionToCacheMsg(mem_trans);
  
  if (XACT_MEMORY){
    unsigned int readyToStart = m_xact_proc->startingMemRef(mem_trans, getRequestType(mem_trans));
    if (readyToStart == 0){
      assert(mem_trans->s.may_stall);      
      return Not_Ready;        
    } else if (readyToStart == 1) {
			m_active_requests[idx].status = Done;
			m_active_requests[idx].success = false;
			return Not_Ready;
		}  
    
    if (mem_trans->s.may_stall){
      if (XACT_LAZY_VM && !XACT_EAGER_CD && request.getType() == CacheRequestType_ST_XACT){
        m_active_requests[idx].status = Done;
        m_active_requests[idx].success = true;
      } else {
        if (XACT_ENABLE_TOURMALINE) {
          if (m_xact_proc->tourmalineExistGlobalConflict(request.getType(), Address(mem_trans->s.physical_address))){
            m_active_requests[idx].status = Done;
            m_active_requests[idx].success = false;
          } else {
            m_active_requests[idx].status = Done;        
            m_active_requests[idx].success = true;
          }                        
        } else {
          // ISSUE REQUEST TO MEMORY SUBSYSTEM      
          if (m_sequencer->isReady(request) == false) {
            DEBUG_MSG(SEQUENCER_COMP,MedPrio, "Sequencer was NOT READY");
            return Not_Ready;
          }
          m_active_requests[idx].status = Serving;
          DEBUG_MSG(SEQUENCER_COMP,MedPrio,"Making memory request");
          m_sequencer->makeRequest(request);                
        }
      }  
    } else {
      m_active_requests[idx].status = Done;
      m_active_requests[idx].success = true;
    }             
    
    if (XACT_VISUALIZER && mem_trans->s.type == Sim_Trans_Instr_Fetch)
      g_system_ptr->getProfiler()->printTransactionState(1);
  
    //increment the sequence number 
    m_seqnum++;

    if (m_active_requests[idx].status == Done) {
      bool success = m_active_requests[idx].success;
      if (success) 
        m_xact_proc->addToReadWriteSets(mem_trans, request.getType());      
      m_xact_proc->readyToRetireMemRef(success, mem_trans, getRequestType(mem_trans));
      int outcome = m_xact_proc->canRetireMemRef(mem_trans, getRequestType(mem_trans), success);
      if (outcome == 1) {
        mem_trans->s.exception = 0x122;
        return D_Replay;  
      } else if (outcome == 2) {
        clearActiveRequestVector();
        return Conflict_Retry;  
      } else {
        return Hit;
      }  
    } else {
      return Miss;
    }          
  } else { // XACT_MEMORY
    if (mem_trans->s.may_stall){  
      if (m_sequencer->isReady(request) == false) {
        DEBUG_MSG(SEQUENCER_COMP,MedPrio, "Sequencer was NOT READY");
        return Not_Ready;
      }

      // update the request status
      m_active_requests[idx].status = Serving;

      // Send request to sequencer
      DEBUG_MSG(SEQUENCER_COMP,MedPrio,"Making memory request");
      m_sequencer->makeRequest(request);
    } else {
      m_active_requests[idx].status = Done;
    }
    
    //increment the sequence number 
    m_seqnum++;

    if (m_active_requests[idx].status == Done) {
      // This case is for the fast path.  That is, the data is in our
      // cache with the correct permissions.
      DEBUG_MSG(SEQUENCER_COMP,MedPrio, "Request was satisfied\n");
      return Hit;

    }

    DEBUG_MSG(SEQUENCER_COMP,MedPrio, "Request was stalled\n");
    return Miss;
  }
}

void SimicsProcessor::observeMemoryAccess(memory_transaction_t *mem_trans) {

  DEBUG_MSG(SEQUENCER_COMP, MedPrio, "\n\nstart of observeMemoryAccess\n");
  DEBUG_EXPR(SEQUENCER_COMP, MedPrio, m_proc);
  
  DEBUG_EXPR(SEQUENCER_COMP, MedPrio, Address(mem_trans->s.physical_address));
  DEBUG_EXPR(SEQUENCER_COMP, MedPrio, mem_trans->s.size);
  DEBUG_EXPR(SEQUENCER_COMP, MedPrio, getRequestType(mem_trans));
  DEBUG_EXPR(SEQUENCER_COMP, MedPrio, mem_trans->s.may_stall);

  // You should NOT do any filtering in this function!
  // See observeMemoryAccess() of SimicsDriver for details!
  char  buffer[256];
  ASSERT (mem_trans->s.size <= 256);

  if (XACT_MEMORY){
    m_xact_proc->retiredMemRef(mem_trans);
  } else { // XACT_MEMORY
    if (mem_trans->s.type == Sim_Trans_Load || mem_trans->s.type == Sim_Trans_Instr_Fetch) {
      // NOT IMPLEMENTED
      // m_sequencer->getRubyMemoryValue( Address(mem_trans->s.physical_address),
      //                                 buffer, mem_trans->s.size);
  
      // check against simics memory
      assert(SIMICS_check_memory_value(m_proc, mem_trans->s.physical_address,
                                       buffer, mem_trans->s.size));

      // supply our loaded value
      SIM_c_set_mem_op_value_buf( &(mem_trans->s), buffer );
    } else if (mem_trans->s.type == Sim_Trans_Store) {
      // update the value in the cache, if any
      SIM_c_get_mem_op_value_buf( &(mem_trans->s), buffer );

      //m_sequencer->setRubyMemoryValue( Address(mem_trans->s.physical_address),
      //                                 buffer, mem_trans->s.size);
    } else {
      ERROR_MSG("Error: not a read or a write?");
    }
  }  
}

void SimicsProcessor::exceptionCallback(integer_t exc) {

  DEBUG_MSG(SEQUENCER_COMP, MedPrio, "\n\nnew exception in exceptionCallback");

  DEBUG_EXPR(SEQUENCER_COMP, MedPrio, m_proc);
  DEBUG_EXPR(SEQUENCER_COMP, MedPrio, exc);

  // make the current serving request a lingering request
  if (getServingRequestSlot() != -1) {
    m_lingering_request_address =
      m_active_requests[getServingRequestSlot()].addr;
  }

  // we don't need to blow away the current instruction request vector
  // since simics should come back with a new instruction count, and we
  // would be able to detect this change in makeRequest().
}

#ifdef SPARC
exception_type_t SimicsProcessor::asiCallback(memory_transaction_t *mem_trans) {
  DEBUG_MSG(SEQUENCER_COMP, MedPrio, "\n\nnew asi mem_trans in asiCallback");
  DEBUG_EXPR(SEQUENCER_COMP, MedPrio, m_proc);

  // ignore instruction fetches
  if (mem_trans->s.type == Sim_Trans_Instr_Fetch) {
    #ifdef SIMICS22X
    return Sim_PE_Continue;
    #endif

    #ifdef SIMICS30
    return Sim_PE_No_Exception;
    #endif
  }

  DEBUG_EXPR(SEQUENCER_COMP, MedPrio, mem_trans->address_space);
  DEBUG_EXPR(SEQUENCER_COMP, MedPrio, Address(mem_trans->s.physical_address));
  DEBUG_EXPR(SEQUENCER_COMP, MedPrio, getRequestType(mem_trans));
  DEBUG_EXPR(SEQUENCER_COMP, MedPrio, mem_trans->s.size);

  // do something here with this ASI access, if you want 
  //int translated_asi = translateASI(mem_trans);
  #ifdef SIMICS22X
  return Sim_PE_Continue;
  #endif

  #ifdef SIMICS30
  return Sim_PE_No_Exception;
  #endif
}
#endif

#ifdef SPARC
int SimicsProcessor::translateASI(const memory_transaction_t* mem_trans) {
  int translated_asi = -1;
  if(mem_trans->address_space > 0xff /* ASI ranges from 0 to 0xff */) {
    translated_asi = (PRIV_MODE)?  ASI_NUCLEUS:ASI_PRIMARY;
  } else {
    // simics defined ASI?
    translated_asi = mem_trans->address_space;
  }
  return translated_asi;
}
#endif

// ******** Non-method helper functions ********

CacheMsg SimicsProcessor::memoryTransactionToCacheMsg(const memory_transaction_t *mem_trans) {
  // set access type (user/supervisor)
  AccessModeType access_mode;
  if (PRIV_MODE) {
    access_mode = AccessModeType_SupervisorMode;
  } else {
    access_mode = AccessModeType_UserMode;
  }

  int transactionLevel = XACT_MEMORY ? m_xact_mgr->getTransactionLevel(0) : 0;
  uint64 timestamp     = XACT_MEMORY ? m_xact_mgr->getXactConflictManager()->getTimestamp(0) : 0;
  bool shouldEscape    = XACT_MEMORY ? ((m_xact_mgr->getEscapeDepth(0) > 0)|| mem_trans->s.type==Sim_Trans_Instr_Fetch || PRIV_MODE) : false;

  return CacheMsg(Address(mem_trans->s.physical_address),
                  Address(mem_trans->s.physical_address),
                  getRequestType(mem_trans),
                  SIMICS_get_program_counter(mem_trans->s.ini_ptr),
                  access_mode,
                  mem_trans->s.size,
                  PrefetchBit_No,
                  0,                       // Version number
                  Address((physical_address_t)mem_trans->s.logical_address),
                  0  /* only 1 SMT thread */,
                  timestamp,
                  shouldEscape
                  );
}

// KM -- I'm putting the code that sets the trans/non-trans aspect of incoming
// requests here because this interface isn't used if we run with opal.  In that
// case, opal determines which requests are part of a transaction.
CacheRequestType SimicsProcessor::getRequestType(const memory_transaction_t *mem_trans) {
  CacheRequestType type;

  bool transaction;
  if (XACT_MEMORY){ 
    transaction = (m_xact_mgr->getTransactionLevel(0) > 0) && (!PRIV_MODE) && (m_xact_mgr->getEscapeDepth(0) == 0);
  } else {
    transaction = false;
  }          

  // is it an IFetch?
  if(mem_trans->s.type == Sim_Trans_Instr_Fetch) {
    type = CacheRequestType_IFETCH;
  } else if(mem_trans->s.type == Sim_Trans_Load) {
    if(transaction){
      if(m_xact_mgr->shouldUpgradeLoad(SIMICS_get_program_counter(mem_trans->s.ini_ptr).getAddress())){
        type = CacheRequestType_LDX_XACT;
      }else {
        type = CacheRequestType_LD_XACT;
      }
    } else {
      type = CacheRequestType_LD;
    }
  } else if(mem_trans->s.type == Sim_Trans_Store){
    if(transaction){
      type = CacheRequestType_ST_XACT;
    } else {
      type = CacheRequestType_ST;
    }
  } else {
    WARN_EXPR(mem_trans->s.type);
    ERROR_MSG("Error: Strange memory transaction type: not a IF, LD or ST");
  }
        
  // is it atomic?
  if (mem_trans->s.atomic) {
    if(transaction){
      type = CacheRequestType_ST_XACT; // Check this out
    } else {
#ifdef SPARC            
      if(SIMICS_is_ldda(mem_trans)) {
        type = CacheRequestType_LD;
      } else {
        type = CacheRequestType_ATOMIC;
      }
#else
      type = CacheRequestType_ATOMIC;
#endif            
    }
  }

  // Be careful changing this assertion. Only these six types are allowed
  // since they are what a instruction can generate. Read the comments of the
  // active request vector in SimicsProcessor.h to understand why allow a
  // CacheRequestType_REPLACEMENT here is a very bad idea!
  assert( type == CacheRequestType_LD ||
          type == CacheRequestType_ST ||
          type == CacheRequestType_ATOMIC ||
          type == CacheRequestType_LD_XACT ||
          type == CacheRequestType_ST_XACT ||
          type == CacheRequestType_LDX_XACT ||
          type == CacheRequestType_IFETCH );

  return type;
}

SimicsProcessor::CacheRequestStatus SimicsProcessor::string_to_CacheRequestStatus(const string& str) {
  if (str == "Invalid") {
    return SimicsProcessor::Invalid;
  } else if (str == "Queued") {
    return SimicsProcessor::Queued;
  } else if (str == "Serving") {
    return SimicsProcessor::Serving;
  } else if (str == "Done") {
    return SimicsProcessor::Done;
  } else {
    WARN_EXPR(str);
    ERROR_MSG("Invalid string conversion for type SimicsProcessor::CacheRequestStatus");
  }
}

// called by Driver to calculate the abort delay
int SimicsProcessor::getAbortDelay(){
  assert(0);      
  return 0;        
}

// called during trap handling
void SimicsProcessor::notifyTrapStart( const Address & handlerPC ){

}

void SimicsProcessor::notifyTrapComplete( const Address & newPC ){

}

void SimicsProcessor::restoreCheckpoint(){
}

int SimicsProcessor::getTransactionLevel(){
  return m_xact_mgr->getTransactionLevel(0);
}

ostream& operator<<(ostream& out, const SimicsProcessor::CacheRequestStatus& obj) {
  switch(obj) {
    case SimicsProcessor::Invalid:
      out << "Invalid";
      break;
    case SimicsProcessor::Queued:
      out << "Queued";
      break;
    case SimicsProcessor::Serving:
      out << "Serving";
      break;
    case SimicsProcessor::Done:
      out << "Done";
      break;
    default:
      ERROR_MSG("Invalid range for type SimicsProcessor::CacheRequestStatus");
      break;
  }
  out << flush;
  return out;
}

