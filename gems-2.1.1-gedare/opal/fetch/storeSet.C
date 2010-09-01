/*
    Copyright (C) 1999-2008 by Mark D. Hill and David A. Wood for the
    Wisconsin Multifacet Project.  Contact: gems@cs.wisc.edu
    http://www.cs.wisc.edu/gems/

    --------------------------------------------------------------------

    This file is part of the Opal Timing-First Microarchitectural
    Simulator, a component of the Multifacet GEMS (General 
    Execution-driven Multiprocessor Simulator) software toolset 
    originally developed at the University of Wisconsin-Madison.

    Opal was originally developed by Carl Mauer based upon code by
    Craig Zilles.

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

#include "hfa.h"
#include "hfacore.h"
#include "Map.h"
#include "Vector.h"
#include "storeSet.h"

//*********************************************************
storeset_predictor_t::storeset_predictor_t(pseq_t * seq){
  m_pseq = seq;
  m_violating_stores = new Map<la_t, Vector< violation_info_t> >;
}

//********************************************************
storeset_predictor_t::~storeset_predictor_t(){
  if(m_violating_stores){
    delete m_violating_stores;
    m_violating_stores = NULL;
  }
}

//*******************************************************
// updates the StoreSet predictor table
void 
storeset_predictor_t::update(const la_t & store_vpc, const la_t & load_vpc){
  if(m_violating_stores->exist(load_vpc)){
    // Load PC exists, now check whether Store PC exists
    Vector< violation_info_t> & info = m_violating_stores->lookup(load_vpc);
    int size = info.size();
    bool found = false;
    for(int i=0; !found && i < size; ++i){
      if(info[i].store_pc == store_vpc){
        info[i].accesses++;
        found = true;
      }
    }
    if(!found){
      // add the ST PC
      violation_info_t new_entry;
      new_entry.store_pc = store_vpc;
      // Our first access to this ST PC
      new_entry.accesses = 1;
      info.insertAtBottom(new_entry);
    }
  }
  else{
    Vector< violation_info_t> new_info;
    violation_info_t new_entry;
    new_entry.store_pc = store_vpc;
    // Our first access to this ST PC
    new_entry.accesses = 1;
    new_info.insertAtBottom(new_entry);
    m_violating_stores->insert( load_vpc, new_info );
  }
}

//*****************************************************
// Searches the StoreSet predictor table for possible LD-ST pair conflicts
//   The following are conditions under which predictor will return TRUE or FALSE
//     1) A ST PC exists in the instruction window and the store (which should be older than load) has NOT
//         executed (ie inserted itself in the LSQ so that load might possibly forward data from it)
//                  Returns TRUE
//     2) A ST PC exists in the instruction window but it has already executed (ie LSQ will correctly 
//           forward data to this load when it is possible to do so)
//                  Returns FALSE
//     3) No ST PC in instruction window is found (either bc ST has already retired on no conflict with
//         this dynamic instance of load --> mark as false positive bc searched instr window??
//                  Returns FALSE
//     4) No entry in predictor corresponding to load PC (ie predictor has never seen this load PC before)
//                  Returns FALSE
bool
storeset_predictor_t::checkForConflict(memory_inst_t * load){
  //Get the relevant info
  la_t load_vpc = load->getVPC();
  int32 load_index = load->getWindowIndex();
  uint32 threadid = load->getProc();
  //get the specific instruction window
  iwindow_t * iwin = m_pseq->getIwindow( threadid );
  ASSERT(iwin != NULL);
  if(m_violating_stores->exist(load_vpc)){
    Vector< violation_info_t> & info = m_violating_stores->lookup(load_vpc);
    int size = info.size();
    dynamic_inst_t * youngest_match = NULL;
    uint64 youngest_seqnum = 0;
    for(int i=0; i < size; ++i){
      la_t store_pc = info[i].store_pc;
      //find this store_pc in the instruction window by looking backwards from current load_index
      //  (ie look at all older non-retired instructions)
      dynamic_inst_t * next_match = iwin->lookupOlder( load_index, store_pc );
      //find the youngest store which load is predicted to depend on...
      if(next_match != NULL){
        if((youngest_match == NULL) || (youngest_seqnum < next_match->getSequenceNumber()) ){
          youngest_match = next_match;
          youngest_seqnum = next_match->getSequenceNumber();
        }
      }
    }
    if(youngest_match != NULL){
      //check whether this ST has executed or not
      if( youngest_match->getStage() < dynamic_inst_t::EXECUTE_STAGE){
          // make load wait until this Store executes
        store_inst_t * store = static_cast< store_inst_t *> (youngest_match);
        load_interface_t * waitInterface;
        if (load->getStaticInst()->getType() == DYN_LOAD) {
          waitInterface = static_cast<load_interface_t *>(static_cast<load_inst_t *>(load));
        } else if (load->getStaticInst()->getType() == DYN_ATOMIC) {
          waitInterface = static_cast<load_interface_t *>(static_cast<atomic_inst_t *>(load));
        } else {
          SIM_HALT;
        }
        waitInterface->stallLoad(store);
        return true;
      }
    }
    // Otherwise no ST PC found in window or ST has already executed
  }
  return false;
}

//*******************************************************
void storeset_predictor_t::printConfig(out_intf_t * out)
{
  Vector< la_t > keys = m_violating_stores->keys();
  int size = keys.size();
  int total_store_pcs = 0;
  int total_violations = 0;
  out->out_info("\n*** StoreSet predictor:\n");
  out->out_info("\tTotal entries = %d\n", size);
  for(int i=0; i < size; ++i){
    out->out_info("\t\tEntry[ %d ] LD_PC = 0x%llx\n", i, keys[i]);
    Vector< violation_info_t > info = m_violating_stores->lookup(keys[i]);
    int size2 = info.size();
    int local_violations = 0;
    total_store_pcs += size2;
    for(int j=0; j < size2; ++j){
      local_violations += info[j].accesses;
      total_violations += info[j].accesses;
      out->out_info("\t\t\t ST_PC = 0x%llx  = %lld violations\n", info[j].store_pc, info[j].accesses);
    }
    out->out_info("\t\t %d ST PCs causing %d violations\n", size2, local_violations);
  }
  out->out_info("\t%d Total ST PCs causing %d Total Violations\n", total_store_pcs, total_violations);
}

  


