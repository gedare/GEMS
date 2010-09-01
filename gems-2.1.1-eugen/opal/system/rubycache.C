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
/*
 * FileName:  rubycache.C
 * Synopsis:  interface to ruby (MP cache simulator)
 * Author:    cmauer
 * Version:   $Id$
 */

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include "hfa.h"
#include "hfacore.h"
#include "pipestate.h"
#include "pipepool.h"
#include "lsq.h"
#include "rubycache.h"


/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Variable declarations                                                  */
/*------------------------------------------------------------------------*/

/// memory allocator for rubycache.C
listalloc_t ruby_request_t::m_myalloc;

/*------------------------------------------------------------------------*/
/* Forward declarations                                                   */
/*------------------------------------------------------------------------*/

const char * memop_menomic( OpalMemop_t memop ) {
  switch (memop) {
  case OPAL_LOAD:
    return ("LOAD");
  case OPAL_STORE:
    return ("STORE");
  case OPAL_IFETCH:
    return ("IFETCH");
  case OPAL_ATOMIC:
    return ("ATOMIC");
  default:
    return ("UNKNOWN");
  }
} 

/*------------------------------------------------------------------------*/
/* ruby_request_t                                                         */
/*------------------------------------------------------------------------*/

//**************************************************************************
ruby_request_t::ruby_request_t( waiter_t *waiter, la_t logical_address, pa_t address, 
                                OpalMemop_t requestType, la_t vpc, bool priv,
                                uint64 posttime, int thread) :
  pipestate_t( waiter )
{
  m_logical_address = logical_address;
  m_physical_address = address;
  m_request_type = requestType;
  m_vpc = vpc;
  m_priv = priv;
  m_post_time = posttime;
  m_waiter = waiter;
  m_is_outstanding = false;
  m_thread = thread;
  //initially mark this request as NOT being a L2 miss
  m_l2miss = false;
  //the total number of instrs dependent on this miss
  m_l2dependents = 0;
  //if this request type is LD, set the destination register for this request
  dynamic_inst_t * dinstr = (dynamic_inst_t *) waiter;

  if(requestType == OPAL_LOAD){
    for(int i=0; i < SI_MAX_DEST; ++i){
      reg_id_t & dest = dinstr->getDestReg(i);
      rid_type_t type = dest.getRtype();
      //   byte_t vanillastate = dest.getVanillaState();
      abstract_rf_t * arf = dest.getARF();
      word_t physreg = dest.getPhysical();
      byte_t vanillareg = dest.getVanilla();
      
      m_dest_reg[i].setRtype(type);
      m_dest_reg[i].setVanilla(vanillareg);
      // m_dest_reg[i].setVanillaState(vanillastate);
      m_dest_reg[i].setPhysical(physreg);
      m_dest_reg[i].setARF(arf);
    }
  }
  else{
    //for all other request types, just reset destination register...
    for(int i=0; i < SI_MAX_DEST; ++i){
      m_dest_reg[i].reset();
    }
  }
}

//**************************************************************************
ruby_request_t::~ruby_request_t( void )
{
}

//**************************************************************************
uint64 ruby_request_t::getSortOrder( void )
{
  return (m_post_time);
}

//**************************************************************************
void ruby_request_t::print( void )
{
  DEBUG_OUT("[0x%0x] element: 0x%0x posted: %lld  ", this, getElement(),
            m_post_time);
  getElement()->print();

#if 0
  DEBUG_OUT("rubylink: [0x%0x]\n", this);
  DEBUG_OUT("   element: 0x%x\n", getElement());
  DEBUG_OUT("   address: 0x%llx\n", m_physical_address );
  DEBUG_OUT("   type   : 0x%d\n", m_request_type);
  DEBUG_OUT("   vpc    : 0x%llx\n", m_vpc);
  DEBUG_OUT("   priv   : 0x%d\n", m_priv);
  DEBUG_OUT("   posted :   %lld\n", m_post_time);
  pipestate_t::print();
#endif
}

/*------------------------------------------------------------------------*/
/* Constructor(s) / destructor                                            */
/*------------------------------------------------------------------------*/

//**************************************************************************
rubycache_t::rubycache_t( uint32 id, uint32 block_bits,
                          scheduler_t *eventQueue )
{
  // Rubycache m_id objects are numbered sequentially just like Ruby sequencer objects (but unlike pseq, pstate, chain objects)
  m_id = id/CONFIG_LOGICAL_PER_PHY_PROC;
  m_event_queue = eventQueue;
  m_block_size = 1 << block_bits;
  m_block_bits = block_bits;
  m_block_mask = ~(m_block_size - 1);
  m_timeout_clock = 0;
  m_request_pool = new pipepool_t();
  m_delayed_pool = new pipepool_t();
  m_mshr_count   = 0;
  m_is_scheduled = false;
  m_fastpath_request = 0x0;
  m_fastpath_hit = false;
  m_fastpath_outstanding = false;

  //reset MLP member variables
  total_outstanding = 0;
  last_mshr_change = 0;
  total_mlp_sum = 0;
  max_outstanding = 0;
  max_elapsed_time = 0;
  max_sum = 0;
  //L2 stats
  total_l2_outstanding = 0;
  last_l2_mshr_change = 0;
  total_l2_mlp_sum = 0;
  max_l2_outstanding = 0;
  max_l2_elapsed_time = 0;
  max_l2_sum = 0;
  m_last_l2miss_cycle = 0;
  m_elapsed_l2miss_cycle_sum = 0;
  m_max_elapsed_l2miss_cycles = 0;
  m_l2miss_cycle_samples = 0;

  // for keeping track of addrs that map to both IFETCH and LD/ST/ATOMIC
  m_dual_addrs = new Map<pa_t, dual_info_t>;

  //next line prefetcher
  m_next_line_prefetcher = new Vector< prefetch_entry_t >;
  //initialize stats
  m_stat_nl_prefetcher_not_ready = 0;
  m_stat_nl_prefetcher_accesses = 0;
  m_stat_nl_prefetcher_hits = 0;
  for(int i=0; i < PREFETCH_DISTANCE+1; ++i){
    m_stat_nl_prefetcher_hist[i] = 0;
  }
  m_prev_lineaddr = 0x0;

  m_user_nl_prefetcher = new Vector< prefetch_entry_t >;
  m_kernel_nl_prefetcher = new Vector<prefetch_entry_t>;

  m_stat_instr_page = new Vector<page_access_entry_t>;

  // l2miss stats
  for(int i=0; i < 2; ++i){
    m_load_l2miss[i] = 0;
    m_store_l2miss[i] = 0;
    m_atomic_l2miss[i] = 0;
    m_ifetch_l2miss[i] = 0;
  }
}

//**************************************************************************
rubycache_t::~rubycache_t( void )
{
  if(m_dual_addrs){
    delete m_dual_addrs;
    m_dual_addrs = NULL;
  }
  if(m_next_line_prefetcher){
    delete m_next_line_prefetcher;
    m_next_line_prefetcher = NULL;
  }
  if(m_stat_instr_page){
    delete m_stat_instr_page;
    m_stat_instr_page = NULL;
  }
  if(m_user_nl_prefetcher){
    delete m_user_nl_prefetcher;
    m_user_nl_prefetcher = NULL;
  }
  if(m_kernel_nl_prefetcher){
    delete m_kernel_nl_prefetcher;
    m_kernel_nl_prefetcher = NULL;
  }
}

//*************************************************************************
int rubycache_t::getOpalNumberOutstandingDemand(){
  int total_demand = 0;
  ruby_request_t *miss = static_cast<ruby_request_t*>(m_request_pool->walkList(NULL));
  while (miss != NULL) {
    total_demand++;
    miss = static_cast<ruby_request_t*>(m_request_pool->walkList( miss ));
  }
  return total_demand;
}

//************************************************************************
int rubycache_t::getRubyNumberOutstandingDemand(){
  mf_ruby_api_t *ruby_api = system_t::inst->m_ruby_api;
  if ( ruby_api == NULL || ruby_api->getNumberOutstandingDemand == NULL ) {
    ERROR_OUT("error: getRubyNumberOutstandingDemand ruby api is called but ruby is not installed.\n");
    SYSTEM_EXIT;
  }
  
  return ( (*ruby_api->getNumberOutstandingDemand)( m_id ) );
}

//*************************************************************************
bool rubycache_t::checkOutstandingRequests(pa_t physical_address){
  pa_t lineaddr = physical_address & m_block_mask;
  bool found = false;
  ruby_request_t *miss = static_cast<ruby_request_t*>(m_request_pool->walkList(NULL));
  while (miss != NULL) {
    if ( miss->match( lineaddr ) ) {
      found = true;
      break;
    }
    miss = static_cast<ruby_request_t*>(m_request_pool->walkList( miss ));
  }
  return found;
}

//*************************************************************************
bool rubycache_t::checkFastPathOutstanding(pa_t physical_address){
  if ( m_fastpath_outstanding && physical_address == m_fastpath_request ) {
    return true;
  }
  return false;
}

//************************************************************************
void rubycache_t::checkDualAddr(pa_t physical_address, OpalMemop_t type){
  pa_t lineaddr = physical_address & m_block_mask;
  if(type == OPAL_IFETCH || type == OPAL_LOAD || type == OPAL_STORE || type == OPAL_ATOMIC){
    //check table of IFETCH addrs
    bool ifetch = false;
    bool load = false;
    bool store = false;
    bool atomic = false;
    if(type == OPAL_IFETCH){
      ifetch = true;
    }
    else if(type == OPAL_LOAD){
      load = true;
    }
    else if(type == OPAL_STORE){
      store = true;
    }
    else if(type == OPAL_ATOMIC){
      atomic = true;
    }
    if(m_dual_addrs->exist(lineaddr)){
      dual_info_t & entry = m_dual_addrs->lookup(lineaddr);
      if(ifetch){
        entry.ifetch = true;
        entry.count[0]++;
      }
      else if(load){
        entry.load = true;
        entry.count[1]++;
      }
      else if(store){
        entry.store = true;
        entry.count[2]++;
      }
      else if(atomic){
        entry.atomic = true;
        entry.count[3]++;
      }
      else{
        ASSERT(0);
      }
      return;
    }
    else{
      //create a new entry
      dual_info_t entry;
      entry.ifetch = entry.load = entry.store = entry.atomic = false;
      for(int i=0; i < 4; ++i){
        entry.count[i] = 0;
      }
      if(ifetch){
        entry.ifetch = true;
        entry.count[0] = 1;
      }
      else if(load){
        entry.load = true;
        entry.count[1] = 1;
      }
      else if(store){
        entry.store = true;
        entry.count[2] = 1;
      }
      else if(atomic){
        entry.atomic = true;
        entry.count[3] = 1;
      }
      else{
        ASSERT(0);
      }
      m_dual_addrs->insert(lineaddr, entry);
      return;
    }
  }
}

//*************************************************************************
void rubycache_t::printDualAddrs(out_intf_t * out){
  Vector< pa_t > key_array = m_dual_addrs->keys();
  int size = key_array.size();
  out->out_info("\n***DualAddr Stats: Total addrs = %d\n", size);
  out->out_info("============================\n");
  if(size > 0){
    uint64 dual_addrs = 0;
    for(int i=0; i < size; ++i){
      dual_info_t & entry = m_dual_addrs->lookup(key_array[i]);
      uint64 total = 0;
      if(entry.ifetch){
        //check whether used for any data accesses
        if(entry.load || entry.store || entry.atomic){
          dual_addrs++;
          for(int j=0; j < 4; ++j){
            total += entry.count[j];
          }
          //output 
          out->out_info("DualAddr 0x%llx total = %lld [ ", key_array[i], total);
          for(int j=0; j < 4; ++j){
            out->out_info_noid("%lld ", entry.count[j]);
          }
          out->out_info_noid("]\n");
        }
      }
    }
    out->out_info("Total Dual Addrs = %lld ( %6.3f% )\n", dual_addrs, 100.0*dual_addrs/size);
  }
}

//*************************************************************************
// mark an instr as having L2 miss
void rubycache_t::markL2Miss(pa_t physical_address, OpalMemop_t type, int tagexists){
  //  cout << "markL2Miss BEGIN" << endl;
  pa_t lineaddr = physical_address & m_block_mask;
  //walk the list looking for this miss
  bool found = false;
  ruby_request_t *miss = static_cast<ruby_request_t*>(m_request_pool->walkList(NULL));
  while (miss != NULL) {
    if ( miss->match( lineaddr ) ) {
      found = true;
      break;
    }
    miss = static_cast<ruby_request_t*>(m_request_pool->walkList( miss ));
  }
  if(!found){
    ERROR_OUT("\t[%d] markL2Miss, ERROR: Not found addr = 0x%llx\n", m_id, lineaddr);
    print();
  }
  ASSERT(found);

  // do not use m_waiter here, bc instr squashes can lead to pointer getting deleted!
  //set the l2miss flag
  miss->m_l2miss = true;

  //update the stats on which request has L2 miss, and whether the tag exists (in invalid form)
  int tag_index = 0;
  if(tagexists){
    tag_index = 1;
  }
  if(type == OPAL_LOAD){
    m_load_l2miss[tagexists]++;
  }
  else if(type == OPAL_STORE){
    m_store_l2miss[tagexists]++;
  }
  else if(type == OPAL_ATOMIC){
    m_atomic_l2miss[tagexists]++;
  }
  else if(type == OPAL_IFETCH){
    m_ifetch_l2miss[tagexists]++;
  }

  //MLP - calculate MLP change
  uint64 current_cycle = system_t::inst->m_seq[m_id]->getLocalCycle();
  updateL2MLPSum(current_cycle);
  incrementTotalL2Outstanding(current_cycle);

  //uint64 elapsed_l2miss_cycles = current_cycle - m_last_l2miss_cycle;
  //  DEBUG_OUT("markL2Miss: lineaddr[ 0x%llx ] type[ %s ] insertcycle[ %lld ] markL2Misscycle[ %lld ] elapsed_l2miss_cycles[ %lld ]\n", lineaddr, memop_menomic(type), miss->m_post_time, current_cycle, elapsed_l2miss_cycles);

  //also track elapsed cycle time btw last L2 miss
  //don't include first sample
  if(m_last_l2miss_cycle > 0){
    uint64 elapsed_l2miss_cycles = current_cycle - m_last_l2miss_cycle;
    //    if(elapsed_l2miss_cycles <= 0){
    //      ERROR_OUT("ERROR current_cycle[ %lld ] last_l2miss_cycle[ %lld ]\n", current_cycle, m_last_l2miss_cycle);
    //    }
    //    ASSERT(elapsed_l2miss_cycles > 0);
    m_elapsed_l2miss_cycle_sum += elapsed_l2miss_cycles;
    m_l2miss_cycle_samples++;
    if(elapsed_l2miss_cycles > m_max_elapsed_l2miss_cycles){
      m_max_elapsed_l2miss_cycles = elapsed_l2miss_cycles;
    }
  }
  m_last_l2miss_cycle = current_cycle;

  #if 0   
  if(miss->m_request_type != OPAL_IFETCH){
    //check that this instr has not been squashed...
    uint64 max_seq_num = system_t::inst->m_seq[m_id]->getSequenceNumber(miss->m_thread);
    //cout << "BEFORE checking seqnums, miss_seqnum[ " << miss->m_seqnum << " ] seqnum[ " << max_seq_num << " ]" << endl;
    if(miss->m_seqnum <= max_seq_num){
      dynamic_inst_t * dinstr = (dynamic_inst_t *) miss->m_waiter;
      //cout << "Marking an instr as L2 MISS" << endl;
      dinstr->markEvent( EVENT_L2_MISS );
      //set the l2 miss flag
      miss->m_l2miss = true;
      if(miss->m_request_type == OPAL_LOAD){
        //set the destination registers as being dependent on L2 miss
        for(int i=0; i < SI_MAX_DEST; ++i){
          reg_id_t dest_reg = miss->m_dest_reg[i];
          word_t physreg = dest_reg.getPhysical();
          byte_t vanillareg = dest_reg.getVanilla();
          //ASSERT(physreg != PSEQ_INVALID_REG);
        if(physreg != PSEQ_INVALID_REG){
          //IMPORTANT: even if we check for this condition here, it might be the case that we squashed
          //  the instruction initiating this request, so this register might already have been FREED!!
          // printf("markL2Miss [%d] dest[%d] vanilla[%d] phys[%d] BEGIN\n", m_id, i, vanillareg, physreg);
          dest_reg.getARF()->setL2Miss( dest_reg, miss->m_thread );
        }
        }
      }
    }
  }
  #endif
}

//**************************************************************************
void
rubycache_t::updateInstrPage( pa_t physical_address, bool priv){
  int size = m_stat_instr_page->size();
  bool found = false;
  pa_t            lineaddr = physical_address & m_block_mask;
  pa_t page_addr = physical_address & ~(( 1 << PAGE_SIZE_BITS) - 1);
  // mask off the page size bits, and retrieve cache line number (uppermost bits)
  int cache_line = (lineaddr & (( 1 << PAGE_SIZE_BITS) - 1)) >> BLOCK_SIZE_BITS;
  ASSERT(0 <= cache_line && cache_line < ((1 << PAGE_SIZE_BITS)/(1 << BLOCK_SIZE_BITS)) );
  for(int i=0; !found && i < size; ++i){
    pa_t entry_address = (*m_stat_instr_page)[i].page_address;
    int prev_line = (*m_stat_instr_page)[i].prev_line;
    if( entry_address == page_addr){
      found = true;
      (*m_stat_instr_page)[i].num_accesses++;
      (*m_stat_instr_page)[i].line_accesses[cache_line]++;
      if(priv){
        (*m_stat_instr_page)[i].priv[1] = true;
        (*m_stat_instr_page)[i].line_priv[cache_line][1] = true;
      }
      else{
        (*m_stat_instr_page)[i].priv[0] = true;
        (*m_stat_instr_page)[i].line_priv[cache_line][0] = true;
      }
      //update dependency matrix
      if(cache_line != prev_line){
        //update the prev_line's dependency arc
        (*m_stat_instr_page)[i].next_line_accesses[prev_line][cache_line]++;
        //update prev_line to point to current line
        (*m_stat_instr_page)[i].prev_line = cache_line;
      }
    }
  }
  if( !found ){
    //add a new entry
    page_access_entry_t new_entry;
    new_entry.page_address = page_addr;
    new_entry.num_accesses = 1;
    new_entry.prev_line = cache_line;
    //clear the dependency matrix
    for(int i=0; i < 128; ++i){
      new_entry.line_accesses[i] = 0;
      for(int p=0; p < 2; ++p){
        new_entry.line_priv[i][p] = false;
      }
      for(int j=0; j < 128; ++j){
        new_entry.next_line_accesses[i][j] = 0;
      }
    }
    for(int p=0; p < 2; ++p){
      new_entry.priv[p] = false;
    }
    new_entry.line_accesses[cache_line] = 1;
    if(priv){
      new_entry.priv[1] = true;
      new_entry.line_priv[cache_line][1] = true;
    }
    else{
      new_entry.priv[0] = true;
      new_entry.line_priv[cache_line][0] = true;
    }

    m_stat_instr_page->insertAtBottom( new_entry );
  }
}

//**************************************************************************
void
rubycache_t::printInstrPageStats(out_intf_t * out){
  int size = m_stat_instr_page->size();
  out->out_info("\n***Instruction Page Access Stats: total = %d\n", size);
  out->out_info("===============================\n");
  uint64 user_pages = 0;
  uint64 kernel_pages = 0;
  uint64 both_pages = 0;
  double pct;
  for(int i=0; i < size; ++i){
    page_access_entry_t * entry = &(*m_stat_instr_page)[i];
    char priv;
    if(entry->priv[0] && entry->priv[1]){
      priv = 'B';
      both_pages++;
    }
    else if(entry->priv[0]){
      priv = 'U';
      user_pages++;
    }
    else{
      priv = 'K';
      kernel_pages++;
    }
    out->out_info("\t0x%llx total_accesses[ %lld ] priv[ %c ]", entry->page_address, entry->num_accesses, priv);
    //print out per line access count
    out->out_info_noid("  [ ");
    uint64 user_lines = 0;
    uint64 kernel_lines = 0;
    uint64 both_lines = 0;
    for(int line=0; line < 128; ++line){
      if(entry->line_accesses[line] > 0){
        if(entry->line_priv[line][0] && entry->line_priv[line][1]){
          priv = 'B';
          both_lines++;
        }
        else if(entry->line_priv[line][0]){
          priv = 'U';
          user_lines++;
        }
        else{
          priv = 'K';
          kernel_lines++;
        }
        out->out_info_noid(" %lld ( %c )| ", entry->line_accesses[line], priv);
      }
      else{
        out->out_info_noid(" %lld | ", entry->line_accesses[line]);
      }
    }
    out->out_info_noid("]  user_lines[ %d ] kernel_lines[ %d ] both_lines[ %d ]\n", user_lines, kernel_lines, both_lines);
    //now print dependency arc info
    for(int line=0; line < 128; ++line){
      if(entry->line_accesses[line] > 0){
        out->out_info_noid("\t\tPage 0x%llx Line %d dep matrix: [ ", entry->page_address, line);
        for(int i=0; i < 128; ++i){
          if(entry->next_line_accesses[line][i] > 0){
            out->out_info_noid("%d ( %d ), ", i, entry->next_line_accesses[line][i]);
          }
        }
        out->out_info_noid("]\n");
      }
    }
  }  //end for all pages
  pct = 0.0;
  if(size > 0){
    pct = 100.0*user_pages/size;
  }
  out->out_info("\tTotal user pages = %lld ( %6.3f% )\n", user_pages, pct);
  pct = 0.0;
  if(size > 0){
    pct = 100.0*kernel_pages/size;
  }
  out->out_info("\tTotal kernel pages = %lld ( %6.3f% )\n", kernel_pages, pct);
  pct = 0.0;
  if(size > 0){
    pct = 100.0*both_pages/size;
  }
  out->out_info("\tTotal user+kernel pages = %lld ( %6.3f% )\n", both_pages, pct);
}
    
//**************************************************************************
// Version of NL prefetcher using generic table (with aliases)
void
rubycache_t::doGenericPrefetch( pa_t physical_address, bool priv ){
  int size = m_next_line_prefetcher->size();
  pa_t            lineaddr = physical_address & m_block_mask;
  bool found = false;
  m_stat_nl_prefetcher_accesses++;
  //  DEBUG_OUT("\ncheckNLPrefetcher: demand_miss[ 0x%llx ] cycle[ %lld ]\n", lineaddr, system_t::inst->m_seq[m_id]->getLocalCycle());
  //  if(lineaddr != m_prev_lineaddr){
  //    DEBUG_OUT("0x%llx cycle[ %lld ]\n", lineaddr, system_t::inst->m_seq[m_id]->getLocalCycle());
  //    m_prev_lineaddr = lineaddr;
  //  }
  #if 1
  for(int i=0; !found && i < size; ++i){
    pa_t prefetch_address = (*m_next_line_prefetcher)[i].address;
    int num_pf_lines = (*m_next_line_prefetcher)[i].num_pf_lines;
    uint64 timestamp = (*m_next_line_prefetcher)[i].timestamp;
    //check whether current request fits range of prefetch addresses
    //DEBUG_OUT("\tchecking entry[ %d] pf_addr[ 0x%llx ] num_pf_lines[ %d ] timestamp[ %lld ]\n", i, prefetch_address, num_pf_lines, timestamp);
    if( ( (num_pf_lines == 0) && (prefetch_address == lineaddr) ) ||
        ( (num_pf_lines >= 1) && 
      ((lineaddr >= ( prefetch_address - (num_pf_lines*64) ) ) && 
          (lineaddr <= prefetch_address ) ) ) ){
      m_stat_nl_prefetcher_hits++;
      found = true;
      //only launch prefetches if we hit a certain address
      // 2 cases: 1) we didn't issue any prefetches but entry was created
      //             2) we issued at least one prefetch 
      // distance is how many lines away we are from current prefetch_address
      int distance = (prefetch_address - lineaddr)/64;
      ASSERT(distance >= 0);
      //DEBUG_OUT("\tdistance[ %d ]\n", distance);
       if( distance <= 4)  {
         ASSERT( (*m_next_line_prefetcher)[i].valid );
            
         //try to prefetch next X lines
         // offset = which line to prefetch next
         int line = 0;
         int offset = 4;
         for(line =0; line < PREFETCH_DISTANCE; ++line){
           prefetch_address += 64;
           //prefetch_address = prefetch_address + offset;
           ruby_status_t status = prefetch( 0x0, prefetch_address, OPAL_IFETCH, 0x0, priv, 0 /*dummy thread*/);
           if(status == NOT_READY){
             m_stat_nl_prefetcher_not_ready++;
             //unwind last update
             prefetch_address -= 64;
             //prefetch_address -= offset;
             break;
           }
           offset += 4;
           //   DEBUG_OUT("\tlaunching prefetch[ 0x%llx ]\n", prefetch_address);
         }
         //DEBUG_OUT("\tlaunched %d prefetches\n", line);
         //either prefetched X lines or not
         m_stat_nl_prefetcher_hist[line]++;
         //update our entry's prefetch address
         //only update our num_pf_lines if we prefetched something
         if(line > 0){
           (*m_next_line_prefetcher)[i].num_pf_lines = line;
         }
         (*m_next_line_prefetcher)[i].address = prefetch_address;
         (*m_next_line_prefetcher)[i].timestamp = system_t::inst->m_seq[m_id]->getLocalCycle();
       }
    }
  }
  if( !found ){
    //create a new entry, if there is room, otherwise do LRU replacement
    int next_index = 0;
    int line = 0;
    pa_t prefetch_address = lineaddr;

    // start prefetching
    int offset = 4;
    for(line =0; line < PREFETCH_DISTANCE; ++line){
      prefetch_address += 64;
      //prefetch_address = prefetch_address + offset;      
      ruby_status_t status = prefetch( 0x0, prefetch_address, OPAL_IFETCH, 0x0, priv, 0 /*dummy thread*/);
      if(status == NOT_READY){
        m_stat_nl_prefetcher_not_ready++;
        //our last effective prefetch was at prefetch_address - 4;
        prefetch_address -= 64;
        //prefetch_address -= offset;
        break;
      }
      offset += 4;
      // DEBUG_OUT("\tlaunching prefetch[ 0x%llx ]\n", prefetch_address);
    }
    //either prefetched X lines or not
    //DEBUG_OUT("\tlaunched %d prefetches\n", line);
    m_stat_nl_prefetcher_hist[line]++;

    // IMPORTANT: only insert or overwrite an entry if we prefetched some line
    if(line > 0){
      if(size == MAX_PREFETCH_ENTRIES){
        //do LRU replacement
        uint64 lru_timestamp = (*m_next_line_prefetcher)[0].timestamp;
        for(int index = 1; index < size; ++index){
          if( (*m_next_line_prefetcher)[index].timestamp < lru_timestamp){
            next_index = index;
            lru_timestamp = (*m_next_line_prefetcher)[index].timestamp;
          }
        }
        // next_index should now point to LRU entry
        //DEBUG_OUT("REPLACING entry[ %d ]\n", next_index);
      }
      else{
        //create a new entry
        prefetch_entry_t new_entry;
        new_entry.address = prefetch_address;
        new_entry.valid = true;
        m_next_line_prefetcher->insertAtBottom( new_entry );
        // next_index should point to size-1
        next_index = m_next_line_prefetcher->size() - 1;
        //DEBUG_OUT("CREATING NEW ENTRY index[ %d ]\n", next_index);
    }
      ASSERT(0<= next_index && next_index < MAX_PREFETCH_ENTRIES);
   
      //update our entry's prefetch address
      (*m_next_line_prefetcher)[next_index].num_pf_lines = line;
      (*m_next_line_prefetcher)[next_index].address = prefetch_address;
      (*m_next_line_prefetcher)[next_index].timestamp = system_t::inst->m_seq[m_id]->getLocalCycle();
    }
  }
#endif
}

//**************************************************************************
void
rubycache_t::doSeparatePrefetch( pa_t physical_address, bool priv){
  Vector< prefetch_entry_t > * nl_prefetcher = NULL;
  if(priv){
    //look in kernel prefetch table
    nl_prefetcher = m_kernel_nl_prefetcher;
  }
  else{
    nl_prefetcher = m_user_nl_prefetcher;
  }

  int size = nl_prefetcher->size();
  pa_t            lineaddr = physical_address & m_block_mask;
  pa_t page_addr = physical_address & ~(( 1 << PAGE_SIZE_BITS) - 1);
  // mask off the page size bits, and retrieve cache line number (uppermost bits)
  int cache_line = (lineaddr & (( 1 << PAGE_SIZE_BITS) - 1)) >> BLOCK_SIZE_BITS;
  ASSERT(0 <= cache_line && cache_line < ((1 << PAGE_SIZE_BITS)/(1 << BLOCK_SIZE_BITS)) );
  bool found = false;
  m_stat_nl_prefetcher_accesses++;
  for(int i=0; !found && i < size; ++i){
    pa_t pg_addr = (*nl_prefetcher)[i].address;
    pa_t last_pfaddr = (*nl_prefetcher)[i].last_pfaddr;
    pa_t prefetch_address = last_pfaddr;
    if( pg_addr == page_addr ){
      m_stat_nl_prefetcher_hits++;
      int distance = (last_pfaddr - lineaddr)/64;
      ASSERT(distance >= 0);
      DEBUG_OUT("\tdistance[ %d ]\n", distance);
       if( distance <= 4)  {
         ASSERT( (*nl_prefetcher)[i].valid );
            
         //try to prefetch next X lines
         // offset = which line to prefetch next
         int line = 0;
         for(line =0; line < PREFETCH_DISTANCE; ++line){
           prefetch_address += 64;
           //prefetch_address = prefetch_address + offset;
           ruby_status_t status = prefetch( 0x0, prefetch_address, OPAL_IFETCH, 0x0, priv, 0 /*dummy thread*/);
           if(status == NOT_READY){
             m_stat_nl_prefetcher_not_ready++;
             //unwind last update
             prefetch_address -= 64;
             break;
           }
           DEBUG_OUT("\tlaunching prefetch[ 0x%llx ]\n", prefetch_address);
         }
         DEBUG_OUT("\tlaunched %d prefetches\n", line);
         //either prefetched X lines or not
         m_stat_nl_prefetcher_hist[line]++;
         //update our entry's prefetch address
         //only update our num_pf_lines if we prefetched something
         if(line > 0){
           (*nl_prefetcher)[i].num_pf_lines = line;
         }
         (*nl_prefetcher)[i].last_pfaddr = prefetch_address;
         (*nl_prefetcher)[i].timestamp = system_t::inst->m_seq[m_id]->getLocalCycle();
       }
    }
  }
}
  
//**************************************************************************
void
rubycache_t::checkNLPrefetcher( pa_t physical_address, bool priv ){
  //doSeparatePrefetch( physical_address, priv);
  doGenericPrefetch( physical_address, priv);
}

//**************************************************************************
void
rubycache_t::printNLPrefetcherStats(out_intf_t * out){
  double pct = 0.0;
  out->out_info("\n***Next line prefetcher Stats\n");
  out->out_info("=====================\n");
  out->out_info("\tTotal entries = %d\n", m_next_line_prefetcher->size());
  if(m_stat_nl_prefetcher_accesses > 0){
    pct = 100.0*m_stat_nl_prefetcher_not_ready/m_stat_nl_prefetcher_accesses;
  }
  out->out_info("\tNum times prefetcher NOT READY = %lld ( %6.3f )\n", m_stat_nl_prefetcher_not_ready, pct);
  out->out_info("\tNum prefetcher accesses = %lld\n", m_stat_nl_prefetcher_accesses);
  pct = 0.0;
  if(m_stat_nl_prefetcher_accesses > 0){
    pct = 100.0*m_stat_nl_prefetcher_hits/m_stat_nl_prefetcher_accesses;
  }
  out->out_info("\tNum prefetcher hits = %lld ( %6.3f )\n", m_stat_nl_prefetcher_hits, pct);
  out->out_info("\tPrefetch line histogram:\n");
  for(int i=0; i < PREFETCH_DISTANCE+1; ++i){
    out->out_info("\t\tLines[ %d ] = %lld\n", i, m_stat_nl_prefetcher_hist[i]);
  }
}

//*************************************************************************
void
rubycache_t::printL2MissStats(out_intf_t * out){
  double pct = 0.0;
  uint64 total = 0;
  out->out_info("\n***L2Miss Stats\n");
  out->out_info("==============\n");
  total = m_load_l2miss[0] + m_load_l2miss[1];
  out->out_info("Load misses: %lld\n", m_load_l2miss[0] + m_load_l2miss[1]);
  if( total > 0){
    pct = m_load_l2miss[1]*100.0/total;
  }
  out->out_info("\tLoad misses w/ tag: %lld (%6.3f %%)\n", m_load_l2miss[1], pct);
  total = m_store_l2miss[0] + m_store_l2miss[1];
  out->out_info("Store misses: %lld\n", m_store_l2miss[0] + m_store_l2miss[1]);
  pct = 0.0;
  if( total > 0){
    pct = m_store_l2miss[1]*100.0/total;
  }
  out->out_info("\tStore misses w/ tag: %lld (%6.3f %%)\n", m_store_l2miss[1], pct);
  total = m_atomic_l2miss[0] + m_atomic_l2miss[1];
  out->out_info("Atomic misses: %lld\n", m_atomic_l2miss[0] + m_atomic_l2miss[1]);
  pct = 0.0;
  if( total > 0){
    pct = m_atomic_l2miss[1]*100.0/total;
  }
  out->out_info("\tAtomic misses w/ tag: %lld (%6.3f %%)\n", m_atomic_l2miss[1], pct);
  total = m_ifetch_l2miss[0] + m_ifetch_l2miss[1];
  out->out_info("Ifetch misses: %lld\n", m_ifetch_l2miss[0] + m_ifetch_l2miss[1]);
  pct = 0.0;
  if( total > 0){
    pct = m_ifetch_l2miss[1]*100.0/total;
  }
  out->out_info("\tIfetch misses w/ tag: %lld (%6.3f %%)\n", m_ifetch_l2miss[1], pct);
}
    
//**************************************************************************
ruby_status_t rubycache_t::access( la_t logical_address, pa_t physical_address,
                                   OpalMemop_t requestType, la_t vpc,
                                   bool priv, waiter_t *inst, bool &mshr_hit,
                                   bool &mshr_stall, bool & conflicting_access, int thread)
{
  ruby_request_t *matchingmiss = NULL;
  pa_t            lineaddr = physical_address & m_block_mask;
  mshr_hit   = false;
  mshr_stall = false;
  conflicting_access = false;

  // check for dual addresses
  checkDualAddr(physical_address, requestType);

  //get the actual index for the global system seq object:
  int32 seq_num = m_id;
  ASSERT(seq_num >= 0 && seq_num < system_t::inst->m_numSMTProcs);

  // IFETCH - we should NOT merge IFETCH requests with LD/ST/ATOMIC to the same cache line, bc
  //  the request goes to a different cache. (I-cache vs D-cache)
  if(matchingmiss != NULL && requestType == OPAL_IFETCH){
    if(matchingmiss->m_request_type != OPAL_IFETCH){
      //we cannot merge IFETCH with any other outstanding request types
      matchingmiss = NULL;
      conflicting_access = true;
    }
    //otherwise we can merge with outstanding IFETCH to same addr
  }

  // LD - we should not merge LD requests with IFETCH to the same cache line, bc the request goes to
  //  a different cache. (D-cache vs. I-cache)
  if(matchingmiss != NULL && requestType == OPAL_LOAD){
    if(matchingmiss->m_request_type == OPAL_IFETCH){
      //we cannot merge LD with outstanding IFETCH to same addr
      matchingmiss = NULL;
      conflicting_access = true;
    }
    //else it is a LD/ST/ATOMIC to same addr, this is OK
  }

  // if not found, see if ruby can accept more requests
  //       This models a finite number of MSHRs.
  if (matchingmiss == NULL) {

#ifndef FAKE_RUBY
    // get a pointer to the ruby api call
    mf_ruby_api_t *ruby_api = system_t::inst->m_ruby_api;
#endif

#ifndef FAKE_RUBY
    // get a pointer to the ruby api call
  //    mf_ruby_api_t *ruby_api = system_t::inst->m_ruby_api;
    // issue a miss to ruby
    if ( ruby_api == NULL || ruby_api->makeRequest == NULL ) {
      ERROR_OUT("error: ruby api is called but ruby is not installed.\n");
      SYSTEM_EXIT;
    }

    // if ruby does not accept requests, return
    if (!(*ruby_api->isReady)( m_id, logical_address, lineaddr, requestType, thread)) {
      //printf("[ %d ] DEMAND NOT_READY thread[ %d ] addr[ 0x%llx ]\n", m_id, thread, physical_address);
      //print();
      return NOT_READY;
    }
#endif

    if(requestType == OPAL_IFETCH){
      //update instr page stats
      //      updateInstrPage( physical_address, priv);
    }

    // check the number of outstanding misses: if too great, wait
    // one hardware context, the sequencer, has a dedicated MSHR reserved
    // as it impacts fetch
    if ( requestType != OPAL_IFETCH &&
         m_mshr_count >= MEMORY_OUTSTANDING_REQUESTS ) {
      
      // add this waiter to a list waiting on MSHR resources
#ifdef DEBUG_RUBY
      DEBUG_OUT("adding memory waiter %d >= %d...\n",
                m_request_pool->getCount(), MEMORY_OUTSTANDING_REQUESTS );
#endif
      
      // add the 'waiter' to the pipeline pool
      // allocate a new ruby miss object
      matchingmiss = new ruby_request_t( inst, logical_address, lineaddr,
                                         requestType, vpc, priv,
                                         system_t::inst->m_seq[seq_num]->getLocalCycle(),
                                         thread
                                        );

      m_delayed_pool->insertOrdered( matchingmiss );
      mshr_stall = true;
      return MISS;
    }
    
    // if ruby does accept requests, try for a "fast path hit"
    m_fastpath_request     = lineaddr;
    m_fastpath_outstanding = true;
    m_fastpath_hit         = false;

    // call make request
#ifndef FAKE_RUBY
#ifdef  DEBUG_RUBY
    if (requestType != OPAL_IFETCH) {
      DEBUG_OUT("[%d] rubycache:: access %s 0x%0llx\n", m_id,
                memop_menomic(requestType), lineaddr );
      print();
    }
#endif

    //charge power to access the cache, depending on whether it is an Ifetch, LD, or ST
    if(requestType == OPAL_IFETCH){
      //get the Bank Number...
      pa_t shifted_addr = lineaddr >> m_block_bits;
      // 3 bits = 8 banks
      int banknum = shifted_addr & 0x7;
      ASSERT(banknum >= 0 && banknum < 8);
      //update per-cycle bank stat....
      system_t::inst->m_seq[seq_num]->L1IBankNumStat(banknum);

      /* WATTCH power */
      if(WATTCH_POWER){
        system_t::inst->m_seq[seq_num]->getPowerStats()->incrementICacheAccess();
      }
    }
    else{
       //get the Bank Number...
      pa_t shifted_addr = lineaddr >> m_block_bits;
      // 3 bits = 8 banks
      int banknum = shifted_addr & 0x7;
      ASSERT(banknum >= 0 && banknum < 8);
      //update per-cycle bank stat....
      system_t::inst->m_seq[seq_num]->L1DBankNumStat(banknum);

      /* WATTCH power */
      if(WATTCH_POWER){
        system_t::inst->m_seq[seq_num]->getPowerStats()->incrementDCacheAccess();
      }
    }

    // ruby may call opal api to change m_fastpath_hit's value
    (*ruby_api->makeRequest)( m_id, logical_address, lineaddr, m_block_size,
                              requestType, vpc, priv, thread
                              );
#else
    // complete the request about 25% of the time
    if ( random() % 4 == 0 ) {
      complete( lineaddr, false /* no abort */, requestType, thread );
    }
#endif
    
    if ( m_fastpath_hit ) {
      // before we exit, launch off some more prefetches
      // if(requestType == OPAL_IFETCH){
      //  checkNLPrefetcher( physical_address, priv );
      // }
      // hit in fast path
      // DEBUG_OUT("\t[ %d ] FASTPATH HIT request[ %s ] cycle[ %lld ]\n", m_id, memop_menomic(requestType), system_t::inst->m_seq[seq_num]->getLocalCycle());

      return HIT;
    }

    // not a fastpath hit
    m_fastpath_outstanding = false;

 
    // do not increment mshr count until after "fast path hit"
    // only increment count on non-ifetch miss
    if ( requestType != OPAL_IFETCH ) {
      m_mshr_count++;
    }

    // allocate a new miss, add to list
    matchingmiss = new ruby_request_t( inst, logical_address, lineaddr,
                                       requestType, vpc, priv,
                                       system_t::inst->m_seq[seq_num]->getLocalCycle(),
                                       thread
                                       );

    m_request_pool->insertOrdered( matchingmiss );

    // Sanity check: total number of outstanding Opal and Ruby demand requests should match
    if(CONFIG_WITH_RUBY){
      int total_opal_demand = getOpalNumberOutstandingDemand();
      int total_ruby_demand = getRubyNumberOutstandingDemand();
      if( total_opal_demand != total_ruby_demand ){
        ERROR_OUT("rubycache_t: ERROR (access)- number of Opal and Ruby Demand misses differ! opal[ %d ] ruby[ %d ]\n", total_opal_demand, total_ruby_demand);
        //reset debug cycle
        debugio_t::setDebugTime( 0 );
        print();
      }
      ASSERT(total_opal_demand == total_ruby_demand );
    }

    //MLP - calculate MLP change
    uint64 current_cycle = system_t::inst->m_seq[seq_num]->getLocalCycle();
    updateMLPSum(current_cycle);
    incrementTotalOutstanding(current_cycle);

    //print out when requests are added to the MSHR
    //DEBUG_OUT("rubycache_t::access ADDING 0x%llx type[ %s ] thread[ %d ] cycle[ %lld]\n", lineaddr, memop_menomic(requestType), thread, current_cycle);


  } else {
    // found miss entry in MSHR
    mshr_hit = true;

#ifdef DEBUG_RUBY
    DEBUG_OUT("rubycache outstanding request: 0x%0llx\n", lineaddr);
#endif

    if(requestType == OPAL_IFETCH){
      //update instr page stats
      //      updateInstrPage( physical_address, priv);
    }
  }

  // missed in cache (either already outstanding or miss in fastpath)
  // add the waiter to the waiter list of this miss
  inst->InsertWaitQueue( matchingmiss->m_wait_list );

  // before we exit, launch off some more prefetches
  // if(requestType == OPAL_IFETCH){
  //  checkNLPrefetcher( physical_address, priv );
  // }
  return MISS;
}

//**************************************************************************
ruby_status_t rubycache_t::prefetch( la_t logical_address, pa_t physical_address,
                                     OpalMemop_t requestType, la_t vpc,
                                     bool priv, int thread) {
#ifdef FAKE_RUBY
  return HIT;
#endif
  // get a pointer to the ruby api call
  pa_t           lineaddr = physical_address & m_block_mask;  
  mf_ruby_api_t *ruby_api = system_t::inst->m_ruby_api;
  
  // issue a miss to ruby
  if ( ruby_api == NULL || ruby_api->makePrefetch == NULL ) {
    ERROR_OUT("error: prefetch ruby is not installed.\n");
    SYSTEM_EXIT;
  }

  // if ruby does not accept requests, drop prefetch and return
  if (!(*ruby_api->isReady)( m_id, logical_address, lineaddr, requestType, thread)) {
    return HIT;
  }

//charge power to access the cache, depending on whether it is an Ifetch, LD, or ST
  if(requestType == OPAL_IFETCH){
    /* WATTCH power */
    if(WATTCH_POWER){
      system_t::inst->m_seq[m_id]->getPowerStats()->incrementICacheAccess();
    }
  }
  else{
    /* WATTCH power */
    if(WATTCH_POWER){
      system_t::inst->m_seq[m_id]->getPowerStats()->incrementDCacheAccess();
    }
  }

  (*ruby_api->makePrefetch)( m_id, logical_address, lineaddr, m_block_size,
                             requestType, vpc, priv, thread );

  // Sanity check: total number of outstanding Opal and Ruby demand requests should match
  if(CONFIG_WITH_RUBY){
    int total_opal_demand = getOpalNumberOutstandingDemand();
    int total_ruby_demand = getRubyNumberOutstandingDemand();
    if( total_opal_demand != total_ruby_demand ){
      ERROR_OUT("rubycache_t: ERROR - (prefetch) number of Opal and Ruby Demand misses differ! opal[ %d ] ruby[ %d ]\n", total_opal_demand, total_ruby_demand);
      //reset debug cycle
      debugio_t::setDebugTime( 0 );
      print();
    }
    ASSERT(total_opal_demand == total_ruby_demand );
  }

  // always hit for prefetch
  return HIT;
}

//*************************************************************************
bool rubycache_t::isLineInCache( pa_t physical_address,
                                     OpalMemop_t requestType, la_t vpc,
                                     bool priv )
{
  #if 0
  // get a pointer to the ruby api call
  pa_t           lineaddr = physical_address & m_block_mask;  
  mf_ruby_api_t *ruby_api = system_t::inst->m_ruby_api;
  bool hit;
  
  // issue a miss to ruby
  if ( ruby_api == NULL || ruby_api->isLineInCache == NULL ) {
    ERROR_OUT("error: isLineInCache ruby is not installed.\n");
    SYSTEM_EXIT;
  }
  
  // checks for writable permissions:
  hit = (*ruby_api->isLineInCache)( m_id, lineaddr, m_block_size,
                             requestType, vpc, priv );
  return hit;
  #endif

  return true;
}

//*************************************************************************
bool rubycache_t::isLineInCacheReadable( pa_t physical_address,
                                     OpalMemop_t requestType, la_t vpc,
                                     bool priv )
{
  #if 0
  // get a pointer to the ruby api call
  pa_t           lineaddr = physical_address & m_block_mask;  
  mf_ruby_api_t *ruby_api = system_t::inst->m_ruby_api;
  bool hit;
  
  // issue a miss to ruby
  if ( ruby_api == NULL || ruby_api->isLineInCacheReadable == NULL ) {
    ERROR_OUT("error: ruby is not installed.\n");
    SYSTEM_EXIT;
  }
  
  // checks for read permissions:
  hit = (*ruby_api->isLineInCacheReadable)( m_id, lineaddr, m_block_size,
                             requestType, vpc, priv );
  return hit;
  #endif

  return true;
}

//**************************************************************************
bool rubycache_t::staleDataRequest( pa_t physical_address, char accessSize,
                                    ireg_t *prediction )
{
#ifdef DEBUG_RUBY
  DEBUG_OUT("rubycache: stale data request 0x%0llx\n", m_id, physical_address);
#endif

  if (accessSize == 0)
    return false;

  mf_ruby_api_t *ruby_api = system_t::inst->m_ruby_api;
  int8 *buffer = new int8[m_block_size];
  int   found = (*ruby_api->staleDataRequest)( m_id, physical_address,
                                               (int) accessSize,
                                               buffer );
  
  // if there is no data... return immediately (no formatting to do)
  if (!found) {
    delete [] buffer;
    return found;
  }

  // copy bytes from cache line into correct locations
  // NOTE: the "predicted_storage" (prediction) array is in 'target endianess'
  //       We need to value in 'host endianess' so we do byte swaps

  //   a) calculate the byte offset in the array
  uint32  offset  = physical_address & ~m_block_mask;
  ASSERT( offset < m_block_size );

  //   b) read the memory, possibly correcting the endianess
  if (accessSize == 4) {
    read_array_as_memory32( buffer, prediction, offset );
  } else if (accessSize == 2) {
    read_array_as_memory16( buffer, prediction, offset );
  } else if (accessSize == 1) {
    prediction[0] = buffer[offset];
  } else if (accessSize == 8) {
    read_array_as_memory64( buffer, prediction, offset, 0 );
  } else if (accessSize == 16) {
    read_array_as_memory64( buffer, prediction, offset, 0 );
    read_array_as_memory64( buffer, prediction, offset + 8, 1 );
  } else if (accessSize == 64) {
    read_array_as_memory64( buffer, prediction, offset,     0 );
    read_array_as_memory64( buffer, prediction, offset +  8, 1 );
    read_array_as_memory64( buffer, prediction, offset + 16, 2 );
    read_array_as_memory64( buffer, prediction, offset + 24, 3 );
    read_array_as_memory64( buffer, prediction, offset + 32, 4 );
    read_array_as_memory64( buffer, prediction, offset + 40, 5 );
    read_array_as_memory64( buffer, prediction, offset + 48, 6 );
    read_array_as_memory64( buffer, prediction, offset + 56, 7 );
  } else {
    ERROR_OUT("error: rubycache: unknown access size == %d\n", accessSize );
    delete [] buffer;
    return false;
  }
  
  /* DEBUG_OUT("prediction (%d): 0x%0llx 0x%llx\n",
     accessSize, physical_address, prediction[0]); */
#ifdef DEBUG_RUBY
  DEBUG_OUT("... hit = %d\n", found);
#endif
  delete [] buffer;
  return found;
}

//**************************************************************************
void rubycache_t::complete( pa_t physical_address, bool abortRequest, OpalMemop_t type, int thread )
{
  bool        found = false;
  pa_t        lineaddr = physical_address & m_block_mask;

  // check for fast path completion
  if ( m_fastpath_outstanding && physical_address == m_fastpath_request ) {
    #ifdef DEBUG_RUBY
    DEBUG_OUT("\n[%d] rubycache:: fastpath complete 0x%0llx 0x%llx\n", m_id, physical_address, lineaddr);
    #endif

    m_fastpath_hit = true;
    // reset the oustanding flag so that other non-fastpath outstanding requests to same line can be
    //           removed from MSHR - used only for SC
     m_fastpath_outstanding = false;
    return;
  }

  if ( lineaddr != physical_address ) {
    ERROR_OUT("error: rubycache: ruby returns pa 0x%0llx which is not a cache line: 0x%0llx\n", physical_address, lineaddr );
  }
  
  // search the linked list for the address
  ruby_request_t *miss = static_cast<ruby_request_t*>(m_request_pool->walkList(NULL));
  while (miss != NULL) {
    if ( miss->match( lineaddr ) && type == miss->m_request_type ) {
      if(miss->m_thread == thread){
        found = true;
        break;
      }
      // else non-mergible request with non-matching id 
    }
    miss = static_cast<ruby_request_t*>(m_request_pool->walkList( miss ));
  }

  if (found) {

    //print();
    //DEBUG_OUT("[ %d ] rubycache::complete() REMOVING REQUEST %s lineaddr[ 0x%llx ] cycle[ %lld ]\n", m_id, memop_menomic(miss->m_request_type), lineaddr, system_t::inst->m_seq[m_id]->getLocalCycle());
   

    // first remove the rubymiss from the linked list
    bool found = m_request_pool->removeElement( miss );
    ASSERT( found == true );

    // Sanity check: total number of outstanding Opal and Ruby demand requests should match
    if(CONFIG_WITH_RUBY){
      int total_opal_demand = getOpalNumberOutstandingDemand();
      int total_ruby_demand = getRubyNumberOutstandingDemand();
      if( total_opal_demand != total_ruby_demand ){
        ERROR_OUT("rubycache_t: ERROR (complete)- number of Opal and Ruby Demand misses differ! opal[ %d ] ruby[ %d ]\n", total_opal_demand, total_ruby_demand);
        //reset debug cycle
        debugio_t::setDebugTime( 0 );
        print();
      }
      ASSERT(total_opal_demand == total_ruby_demand );
    }

    //DEBUG
    //print();

    //MLP - calculate MLP change
    // Note all requests (L2 or not) update the base counter value!
    uint64 current_cycle = system_t::inst->m_seq[m_id]->getLocalCycle();
    updateMLPSum(current_cycle);
    decrementTotalOutstanding(current_cycle);
    if( miss->m_l2miss){
      //also update the L2 miss counter stats
      updateL2MLPSum(current_cycle);
      decrementTotalL2Outstanding(current_cycle);
    }
    
    // only decrement count on non-ifetch miss
    if ( miss->m_request_type != OPAL_IFETCH ) {
      // decrement the number of (non-icache) outstanding misses
      m_mshr_count--;
    }

    if ( m_delayed_pool->getCount() > 0 ) {
      scheduleWakeup();
    }

    uint64 latency =   system_t::inst->m_seq[m_id]->getLocalCycle() - miss->m_post_time;
    assert(latency > 0);

    if(miss->m_request_type == OPAL_IFETCH){
      system_t::inst->m_seq[m_id]->addIfetchLatencyStat(latency, miss->m_thread);
    }
    else if(miss->m_request_type == OPAL_LOAD){
      system_t::inst->m_seq[m_id]->addLoadLatencyStat(latency, miss->m_thread);
    }
    else if(miss->m_request_type == OPAL_STORE){
      system_t::inst->m_seq[m_id]->addStoreLatencyStat(latency, miss->m_thread);
    }
    else if(miss->m_request_type == OPAL_ATOMIC){
       system_t::inst->m_seq[m_id]->addAtomicLatencyStat(latency, miss->m_thread);
    }

    // If using a write buffer, then schedule a Wakeup only if we don't have any outstanding MSHR requests
    /*
    if(m_mshr_count == 0){
      system_t::inst->m_seq[m_id]->scheduleWBWakeup();
    }
    */

    miss->m_wait_list.WakeupChain();

    
#ifdef DEBUG_RUBY
    DEBUG_OUT( "\n[%d] COMPLETE 0x%0llx 0x%0llx (count:%d) (delay:%d)\n",
               m_id, lineaddr, miss->m_physical_address,
               m_mshr_count, m_delayed_pool->getCount() );
#endif

    delete miss;
  } else {
    // if not found, report a warning
    ERROR_OUT("[%d] error: rubycache: at complete, address 0x%0llx %s not found.\n", m_id, lineaddr, memop_menomic(type));
    ERROR_OUT("rubycache:: complete FAILS\n");
    print();
    SIM_HALT;
  }
}

//**************************************************************************
void rubycache_t::scheduleWakeup( void )
{
  if (!m_is_scheduled) {
    // wake up next cycle, and handle outstanding miss
    if (m_event_queue != NULL) {
      // if we are running using the event queue, schedule ourselves
      // otherwise rely on caller to poll the is_scheduled variable
      tick_t current_cycle = m_event_queue->getCycle();
      m_event_queue->queueInsert( this, current_cycle, 1 );
    }
    m_is_scheduled = true;
  }
}

//**************************************************************************
void rubycache_t::Wakeup( void )
{
  // The event queue must not be NULL: and will not be for timing simulation
  ASSERT( m_event_queue != NULL );
  m_is_scheduled = false;
  if ( m_mshr_count < MEMORY_OUTSTANDING_REQUESTS &&
       m_delayed_pool->getCount() > 0 ) {

    // select the oldest miss (from decending ordered list)
    ruby_request_t   *miss = (ruby_request_t *) m_delayed_pool->removeHead();
    if (miss != NULL) {
      // re-issue the oldest memory reference in the list
#ifdef DEBUG_RUBY
      DEBUG_OUT("*** issuing [0x%0x] 0x%0x\n", miss, miss->getElement());
#endif
      mf_ruby_api_t *ruby_api    = system_t::inst->m_ruby_api;
      pa_t           re_lineaddr = miss->m_physical_address & m_block_mask;  
        
      // if ruby does not accept requests, halt simulation
      if (!(*ruby_api->isReady)( m_id, miss->m_logical_address, re_lineaddr, miss->m_request_type, miss->m_thread)) {
#ifdef DEBUG_RUBY
        DEBUG_OUT( "warning: rubycache_t: wakeup: unable to reissue request\n");
#endif
        // repost miss to the mshr list, reschedule for next cycle
        m_delayed_pool->insertOrdered( miss );
        scheduleWakeup();
        return;
      }

      bool mshr_hit = false;
      bool mshr_stall = false;
      bool conflicting_access = false;
      ruby_status_t status = access(miss->m_logical_address, 
                                    miss->m_physical_address,
                                     miss->m_request_type,
                                     miss->m_vpc,
                                     miss->m_priv,
                                     miss->getElement(),
                                     mshr_hit,
                                     mshr_stall,
                                     conflicting_access,
                                     miss->m_thread
                                     );
      // we just freed up resources, so this shouldn't stall
      ASSERT( !mshr_stall );

      if (status == HIT) {
#ifdef DEBUG_RUBY
        DEBUG_OUT( "*** HIT [0x%0x] 0x%0x\n", miss, miss->getElement());
#endif
        tick_t current_cycle = m_event_queue->getCycle();
        m_event_queue->queueInsert( miss->getElement(), current_cycle, 1 );

      } else if (status == MISS) {
#ifdef DEBUG_RUBY
        DEBUG_OUT( "*** MISS [0x%0x] 0x%0x\n", miss, miss->getElement());
#endif
      } else if (status == NOT_READY) {
        DEBUG_OUT( "*** NOTREADY [0x%0x] 0x%0x\n", miss, miss->getElement());
        SIM_HALT;
      } else {
        DEBUG_OUT( "rubycache:wakeup: error: unknown status return type in wakeup.\n");
        SIM_HALT;
      }
      
      delete miss;
    } else {
      ERROR_OUT( "rubycache:wakeup: error: delayed == %d, but none found on list.\n",
                 m_delayed_pool->getCount() );
      SIM_HALT;
    }
    
    // if there are guys remaining, reschsedule for next cycle
    if (m_delayed_pool->getCount() > 0)
      scheduleWakeup();
  }
}

//**************************************************************************
void rubycache_t::squashInstruction( dynamic_inst_t *inst )
{
  ruby_request_t *miss  = static_cast<ruby_request_t*>(m_delayed_pool->walkList(NULL));
  ruby_request_t *match = NULL;
  while (miss != NULL) {
    // FIXME: This seems to be a bug. Shouldn't we be checking the m_wait_list
    // for the instruction that are waiting? Right now, I think, the
    // m_wait_list always has only one element which is what miss->getElement()
    // returns. Therefore, no problem is observed. However, if we ever have
    // more than one element in m_wait_list, what we should do here is:
    //   1. check the m_wait_list for a match
    //   2. remove the matching instruction from the m_wait_list
    //   3. if the matchin instruction is what miss->getElement returns, update
    //      *miss object so getElement() returns one instruction that is still
    //      on the m_wait_list
    //   4. remove the miss structure is all instructions are squashed.
    if ( miss->getElement() == inst ) {
      if (match != NULL) {
        ERROR_OUT("error: rubycache: squashInstruction: inst 0x%0x has multiple instances in list!\n", inst);
        SIM_HALT;
      }
      match = miss;
    }
    miss = static_cast<ruby_request_t*>(m_delayed_pool->walkList( miss ));
  }
  
  if (match != NULL) {
    m_delayed_pool->removeElement( match );
  }
  // match may be NULL as delayed instructions can be issued, or completed
}

//**************************************************************************
void rubycache_t::advanceTimeout( void )
{
  m_timeout_clock++;
  if ( m_timeout_clock % 20 == 0 ) {
    // complete the load or store at the head of the queue
    ruby_request_t *cur = (ruby_request_t *) m_request_pool->removeHead();

    // DEBUG_OUT("    advanceTime completing: 0x%0llx\n", cur->getAddress());
    complete( cur->getAddress(), false /* no abort */, cur->m_request_type, cur->m_thread );
  }
}

//**************************************************************************
void rubycache_t::print( void )
{
  DEBUG_OUT("[%d] rubycache_t: delayed: %d outstanding: %d (%d).\n", m_id, 
            m_delayed_pool->getCount(), m_mshr_count, m_is_scheduled);
    
  ruby_request_t *miss = static_cast<ruby_request_t*>(m_request_pool->walkList(NULL));
  if (miss == NULL) {
    DEBUG_OUT("[%d] rubycache_t: no outstanding\n", m_id);
  } else {
    DEBUG_OUT("[%d] rubycache_t: outstanding memory transactions.\n", m_id);
  }
  while (miss != NULL) {
    DEBUG_OUT("[%d]    rubycache_t %s: 0x%0llx (posted: %d) thread[ %d ] \n", m_id,
              memop_menomic( miss->m_request_type ), miss->getAddress(), miss->m_post_time, miss->m_thread);
    miss = static_cast<ruby_request_t*>(m_request_pool->walkList( miss ));
  }
    
  // walk the pipeline pool, printing out all elements
  ruby_request_t *link = NULL;
  while (link != NULL) {
    link->print();
    link = (ruby_request_t *) m_delayed_pool->walkList( link );
  }
    
// now print Ruby's outstannding request table
if(CONFIG_WITH_RUBY){
  DEBUG_OUT("\nRuby's Outstanding Request Table:\n");
  mf_ruby_api_t *ruby_api = system_t::inst->m_ruby_api;
  
  if ( ruby_api == NULL || ruby_api->printProgress == NULL ) {
    ERROR_OUT("error: print ruby is not installed.\n");
    SYSTEM_EXIT;
  }
  (*ruby_api->printProgress)(m_id);
}
  
}

/*------------------------------------------------------------------------*/
/* Public methods                                                         */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Accessor(s) / mutator(s)                                               */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Private methods                                                        */
/*------------------------------------------------------------------------*/

//**************************************************************************
void rubycache_t::read_array_as_memory16( int8 *buffer, uint64 *result,
                                           uint32 offset )
{
  if (!ENDIAN_MATCHES) {
    result[0]   =  buffer[offset] & 0xff;
    result[0] <<= 8;
    offset++;
    result[0]  |= buffer[offset] & 0xff;
  } else {
    result[0]  = (buffer[offset] & 0xff);
    offset++;
    result[0] |= (buffer[offset] & 0xff) << 8;
  }
  if ( !(offset < m_block_size) ) {
    ERROR_OUT( "error: read_array_as_memory: final offset = %d\n", offset );
  }
  ASSERT( offset < m_block_size );
}

//**************************************************************************
void rubycache_t::read_array_as_memory32( int8 *buffer, uint64 *result,
                                           uint32 offset )
{
  if (!ENDIAN_MATCHES) {
    result[0]   =  buffer[offset] & 0xff;
    result[0] <<= 8;
    offset++;
    result[0]  |= buffer[offset] & 0xff;
    result[0] <<= 8;
    offset++;
    result[0]  |= buffer[offset] & 0xff;
    result[0] <<= 8;
    offset++;
    result[0]  |= buffer[offset] & 0xff;
  } else {
    result[0]  = ((uint64) buffer[offset] & 0x00ff);
    offset++;
    result[0] |= ((uint64) buffer[offset] & 0x00ff) << 8;
    offset++;
    result[0] |= ((uint64) buffer[offset] & 0x00ff) << 16;
    offset++;
    result[0] |= ((uint64) buffer[offset] & 0x00ff) << 24;
  }
  if ( !(offset < m_block_size) ) {
    ERROR_OUT( "error: read_array_as_memory: final offset = %d\n", offset );
  }
  ASSERT( offset < m_block_size );
}

//**************************************************************************
void rubycache_t::read_array_as_memory64( int8 *buffer, uint64 *result,
                                          uint32 offset, uint32 rindex )
{
  if (!ENDIAN_MATCHES) {
    result[rindex]   =  buffer[offset] & 0xff;
    result[rindex] <<= 8;
    offset++;
    result[rindex]  |= buffer[offset] & 0xff;
    result[rindex] <<= 8;
    offset++;
    result[rindex]  |= buffer[offset] & 0xff;
    result[rindex] <<= 8;
    offset++;
    result[rindex]  |= buffer[offset] & 0xff;
    result[rindex] <<= 8;
    offset++;
    result[rindex]  |= buffer[offset] & 0xff;
    result[rindex] <<= 8;
    offset++;
    result[rindex]  |= buffer[offset] & 0xff;
    result[rindex] <<= 8;
    offset++;
    result[rindex]  |= buffer[offset] & 0xff;
    result[rindex] <<= 8;
    offset++;
    result[rindex]  |= buffer[offset] & 0xff;

#if 0
    result[rindex]  = ((uint64) buffer[offset] & 0x00ff) << 56;
    offset++;
    result[rindex] |= ((uint64) buffer[offset] & 0x00ff) << 48;
    offset++;
    result[rindex] |= ((uint64) buffer[offset] & 0x00ff) << 40;
    offset++;
    result[rindex] |= ((uint64) buffer[offset] & 0x00ff) << 32;
    offset++;
    result[rindex] |= ((uint64) buffer[offset] & 0x00ff) << 24;
    offset++;
    result[rindex] |= ((uint64) buffer[offset] & 0x00ff) << 16;
    offset++;
    result[rindex] |= ((uint64) buffer[offset] & 0x00ff) << 8;
    offset++;
    result[rindex] |= ((uint64) buffer[offset] & 0x00ff);
#endif
  } else {
    result[rindex]  = ((uint64) buffer[offset] & 0x00ff);
    offset++;
    result[rindex] |= ((uint64) buffer[offset] & 0x00ff) << 8;
    offset++;
    result[rindex] |= ((uint64) buffer[offset] & 0x00ff) << 16;
    offset++;
    result[rindex] |= ((uint64) buffer[offset] & 0x00ff) << 24;
    offset++;
    result[rindex] |= ((uint64) buffer[offset] & 0x00ff) << 32;
    offset++;
    result[rindex] |= ((uint64) buffer[offset] & 0x00ff) << 40;
    offset++;
    result[rindex] |= ((uint64) buffer[offset] & 0x00ff) << 48;
    offset++;
    result[rindex]  = ((uint64) buffer[offset] & 0x00ff) << 56;
  }
  if ( !(offset < m_block_size) ) {
    ERROR_OUT( "error: read_array_as_memory: final offset = %d\n", offset );
  }
  ASSERT( offset < m_block_size );
}

//**************************************************************************
// Returns true if the outstanding request is an L2 miss
bool rubycache_t::isRequestL2Miss(pa_t physical_address){
  pa_t lineaddr = physical_address & m_block_mask;
  ruby_request_t *miss = static_cast<ruby_request_t*>(m_request_pool->walkList(NULL));
  while (miss != NULL) {
    if ( miss->match( lineaddr ) ) {
      //check l2 miss flag
      return miss->m_l2miss;
    }
    miss = static_cast<ruby_request_t*>(m_request_pool->walkList( miss ));
  }
  // request not found
  return false;
}

/*------------------------------------------------------------------------*/
/* Static methods                                                         */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Global functions                                                       */
/*------------------------------------------------------------------------*/


/** [Memo].
 *  [Internal Documentation]
 */
//**************************************************************************
