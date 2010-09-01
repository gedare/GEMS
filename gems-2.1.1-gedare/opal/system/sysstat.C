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
 * FileName:  sysstats.C
 * Synopsis:  system wide statistics implementation
 * Author:    cmauer
 * Version:   $Id$
 */

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include "hfa.h"
#include "hfacore.h"
#include "histogram.h"
#include "lockstat.h"
#include "stopwatch.h"
#include "memstat.h"
#include "threadstat.h"
#include "sysstat.h"

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Variable declarations                                                  */
/*------------------------------------------------------------------------*/

/// magic number for thread address
const la_t STAT_IO_THREAD_ADDR = 0x10dec1ce;
/// magic number for thread id
const la_t STAT_IO_THREAD_ID   = (la_t) -1776;

/// estimated memory system latency
const int32 STAT_EST_MEMORY_LATENCY = 10000;

/*------------------------------------------------------------------------*/
/* Forward declarations                                                   */
/*------------------------------------------------------------------------*/

static int lock_compare( const void *el1, const void *el2 );
static void threadTableWalker( histogram_t *h, int PC, int accesses );

// C++ Template: explicit instantiation
template class map<la_t, thread_stat_t *>;
template class map<la_t, lock_stat_t *>;

/*------------------------------------------------------------------------*/
/* Constructor(s) / destructor                                            */
/*------------------------------------------------------------------------*/

//**************************************************************************
sys_stat_t::sys_stat_t( int32 numProcs)
{
  if (STAT_EXPENSIVE_PROFILE) {
    m_mem_stat = new mem_stat_t();
    m_thread_stat_table = new ThreadStatTable();
    m_exclude_lock_table = new PerPCLockTable();
    m_trace_address = 0x1041ad30;
    m_trace_address = (la_t) -1;

    m_trace_phys_address = 0x7f854848;
    m_trace_phys_address = (pa_t) -1;

    // exclude certain PCs from the atomic lock tracking
    // no lock statistics will be gathered for any address in this table
    lock_stat_t *exclude_lock_stat = new lock_stat_t();
    (*m_exclude_lock_table)[0x10035844] = exclude_lock_stat;
    (*m_exclude_lock_table)[0x10007a98] = exclude_lock_stat;
    (*m_exclude_lock_table)[0x10035864] = exclude_lock_stat;
    (*m_exclude_lock_table)[0x10008148] = exclude_lock_stat;

    m_pc_lock_table = new PerPCLockTable();

    // add the "I/O" thread to the stat table
    m_io_thread = new thread_stat_t( (la_t) STAT_IO_THREAD_ID,
                                     (int32) STAT_IO_THREAD_ID, -1 );
    (*m_thread_stat_table)[STAT_IO_THREAD_ADDR] = m_io_thread;
  }
}

//**************************************************************************
sys_stat_t::~sys_stat_t( )
{
  if (STAT_EXPENSIVE_PROFILE) {
    if (m_mem_stat)
      delete m_mem_stat;
    if (m_thread_stat_table)
      delete m_thread_stat_table;
    if (m_exclude_lock_table)
      delete m_exclude_lock_table;
    if (m_pc_lock_table)
      delete m_pc_lock_table;
  }
}

//**************************************************************************
void sys_stat_t::observeInstruction( pseq_t *pseq, dynamic_inst_t *dinstr )
{
  tick_t  fetchTime    = dinstr->getFetchTime();
  tick_t decodeTime = fetchTime + dinstr->getEventTime( EVENT_TIME_DECODE );
  tick_t operandsReadyTime = fetchTime + dinstr->getEventTime( EVENT_TIME_OPERANDS_READY );
  tick_t  issueTime    = fetchTime + dinstr->getEventTime( EVENT_TIME_EXECUTE );
  tick_t continueExecute = fetchTime + dinstr->getEventTime( EVENT_TIME_CONTINUE_EXECUTE );
  tick_t  completeTime = fetchTime + dinstr->getEventTime( EVENT_TIME_EXECUTE_DONE );
  tick_t  retireTime   = fetchTime + dinstr->getEventTime( EVENT_TIME_RETIRE );
  // latency for execution
  int32   latency      = 0;
  // latency from fetch to decode
  int32 decode_latency = 0;
  //latency from decode to operand ready
  int32 operandsready_latency = 0;
  //latency from operands ready to beginning execution
  int32 scheduler_latency = 0;
  //latency from starting execution and beginning 2nd part of execution (if stalled)
  int32 cont_execution_latency = 0;
  //latency from completion to retiring
  int32 retire_latency = 0;

  enum i_opcode op     = (enum i_opcode) dinstr->getStaticInst()->getOpcode();
  decode_stat_t *opstat= pseq->getOpstat(dinstr->getProc());  //Luke
  static_inst_t *s = dinstr->getStaticInst();

  //***************print out the retiring instruction
  // IMPORTANT: the registers in use might NOT be valid due to squashes, so do NOT print out regs here! Print them out
  // in Retire() of each instruction type!
  #if 0
  char buf[128];
  s->printDisassemble(buf);
  if(s->getType() == DYN_LOAD || s->getType() == DYN_STORE || s->getType() == DYN_ATOMIC || s->getType() == DYN_PREFETCH){
    //print out memory instructions
    DEBUG_OUT("%s seqnum[ %d ] VPC[ 0x%llx ] PC[ 0x%llx ] physical_addr[ 0x%llx ] fetched[ %lld ]  ", buf,dinstr->getSequenceNumber(), dinstr->getVPC(), dinstr->getPC(), (static_cast<memory_inst_t *> (dinstr)->getPhysicalAddress()) & ~( (1 << 6) - 1), fetchTime);
    if(dinstr->getEvent(EVENT_L2_MISS)){
      DEBUG_OUT("  L2 MISS");
    }
    else if(dinstr->getEvent(EVENT_DCACHE_MISS)){
      DEBUG_OUT("  L1 MISS");
    }
    else{
      DEBUG_OUT("  L1 HIT");
    }
  }
  else{
    //print out non-mem instructions
    DEBUG_OUT("%s seqnum[ %d ] VPC[ 0x%llx ] PC[ 0x%llx ] fetched[ %lld ]", buf,dinstr->getSequenceNumber(), dinstr->getVPC(), dinstr->getPC(), fetchTime);
  }
  //print source and dest regs
  DEBUG_OUT(" SOURCES: ");
  for(int i=0; i < SI_MAX_SOURCE; ++i){
    reg_id_t & source = dinstr->getSourceReg(i);
    if(!source.isZero()){
      DEBUG_OUT("( [%d] V: %d P: %d Arf: %s )", i,source.getVanilla(), source.getPhysical(), source.rid_type_menomic( source.getRtype() ) );
    }
  }
  DEBUG_OUT(" DESTS: ");
  for(int i=0; i < SI_MAX_DEST; ++i){
    reg_id_t & dest = dinstr->getDestReg(i);
    if(!dest.isZero()){
      DEBUG_OUT("( [%d] V: %d P: %d Arf: %s )", i,dest.getVanilla(), dest.getPhysical(), dest.rid_type_menomic( dest.getRtype() ) );
    }
  }
  DEBUG_OUT("\n");
  #endif
  //*****************END print

  int32 proc = dinstr->getProc();

  //update L2 icache miss 
  if(dinstr->getEvent( EVENT_L2_INSTR_MISS)){
    opstat->l2instrmissOp( op, dinstr->getTrapGroup(), dinstr->getStaticInst()->getType() );
  }
  //update L2 data miss, dep L2 miss stats for opcode  
  if(dinstr->getEvent( EVENT_L2_MISS )){
    opstat->l2missOp( op, dinstr->getTrapGroup(), dinstr->getStaticInst()->getType() );
  }
  if(dinstr->getEvent( EVENT_DEP_L2_MISS )){
    opstat->depl2missOp( op );
  }

  opstat->seenOp( op, dinstr->getTrapGroup(), dinstr->getStaticInst()->getType() );

  pseq->m_stat_fu_util_retired[proc][s->getFuType()]++;   //Luke
  if ( completeTime >= issueTime ) {
    latency = (completeTime - issueTime);
    if(decodeTime < fetchTime){
      char buf[128];
      s->printDisassemble(buf);
      printf("***ERROR: decodetime < fetchtime: %ld %ld %s\n",decodeTime, fetchTime, buf);
      dinstr->printDetail();
      pseq->print();
    }
    ASSERT(decodeTime >= fetchTime);
    decode_latency = decodeTime - fetchTime;
    if(operandsReadyTime < decodeTime){
      char buf[128];
      s->printDisassemble(buf);
      printf("***ERROR: operandsreadytime < decodetime: %ld %ld %s\n",operandsReadyTime, decodeTime, buf);
      dinstr->printDetail();
      pseq->print();
    }
    ASSERT(operandsReadyTime >= decodeTime);
    operandsready_latency = operandsReadyTime - decodeTime;
    if(issueTime < operandsReadyTime){
      char buf[128];
      s->printDisassemble(buf);
      printf("***ERROR: issuetime < operandsreadytime: %ld %ld %s\n",issueTime, operandsReadyTime, buf);
      dinstr->printDetail();
      pseq->print();
    }
    ASSERT(issueTime >= operandsReadyTime);
    scheduler_latency = issueTime - operandsReadyTime;
    if(continueExecute > issueTime){
      //we were stalled before we could continue execute
      cont_execution_latency = continueExecute - issueTime;
    }
    //IMPORTANT: retireTime could be < completeTime bc of SQUASHES..
    if(retireTime >= completeTime){
      retire_latency = retireTime - completeTime;
      opstat->opRetireLatency(op, retire_latency);
    }
    opstat->opDecodeLatency( op, decode_latency);
    opstat->opOperandsReadyLatency(op, operandsready_latency);
    opstat->opSchedulerLatency(op, scheduler_latency);
    opstat->opContExecutionLatency(op, cont_execution_latency);
    opstat->opExecuteLatency( op, latency );

  } else {
    latency = 0;
  }

  // increment the count of instructions seen between branch mispredicts
  //    any branch mispredicts
  
  //    just predicated/non-predictated branches

#if 0  
  pseq->out_log( "XM 0x%0x %lld %d %d %d\n",
                 dinstr->getStaticInst()->getInst(),
                 fetchTime,
                 dinstr->getEventTime( EVENT_TIME_EXECUTE ),
                 dinstr->getEventTime( EVENT_TIME_EXECUTE_DONE ),
                 dinstr->getEventTime( EVENT_TIME_RETIRE ) );
#endif
  if (dinstr->getEvent( EVENT_DCACHE_MISS )) {
    //pseq->out_info("RETIRED CACHE MISS:\n");
    //dinstr->printDetail();
    STAT_INC( pseq->m_stat_retired_dcache_miss[proc] );   //Luke

    if (dinstr->getEvent( EVENT_MSHR_HIT )) {
      STAT_INC( pseq->m_stat_retired_mshr_hits[proc] );       //Luke
    } else {

      if (latency > STAT_EST_MEMORY_LATENCY) {
        // push a count of #memory refs that are part of a dependent chain
        int dependent = latency / STAT_EST_MEMORY_LATENCY;
        pseq->m_stat_hist_dep_ops->touch( dependent, 1 );
        
        // if this main memory miss is closer to the last one, aggregate it
        int64 issue_delta = issueTime - pseq->m_stat_last_miss_issue[proc];    //Luke
        // pseq->out_info("  delta: %lld [%d]\n", issue_delta, pseq->m_stat_miss_count );
        if (issue_delta > STAT_EST_MEMORY_LATENCY) {
          //    push the current distribution (MLP, interarrival time)
          pseq->m_stat_hist_misses->touch( pseq->m_stat_miss_count[proc], 1 );  //Luke
          
          // compute the number of instructions between this memory operation
          //    and the previous memory operation
          int64 seq_diff = dinstr->getSequenceNumber() - pseq->m_stat_last_miss_seq[proc];
          pseq->m_stat_hist_inter_cluster->touch( seq_diff, 1 );
          pseq->m_stat_hist_effective_ind->touch( pseq->m_stat_miss_effective_ind[proc], 1 );
          pseq->m_stat_hist_effective_dep->touch( pseq->m_stat_miss_effective_dep[proc], 1 );
          pseq->m_stat_hist_inter_cluster->touch( pseq->m_stat_miss_inter_cluster[proc], 1 );
          
          //    start an new distribution
          pseq->m_stat_miss_count[proc]       = 1;
          pseq->m_stat_last_miss_seq[proc]    = dinstr->getSequenceNumber();
          pseq->m_stat_last_miss_fetch[proc]  = fetchTime;
          pseq->m_stat_last_miss_issue[proc]  = issueTime;
          pseq->m_stat_last_miss_retire[proc] = retireTime;
          
          pseq->m_stat_miss_effective_ind[proc] = 0;
          pseq->m_stat_miss_effective_dep[proc] = 0;
          pseq->m_stat_miss_inter_cluster[proc] = 0;
        } else {
          // issue_delta < MEMORY_LATENCY
          pseq->m_stat_miss_count[proc]++;
        }

        // post this instructions memory latency to a distribution
        STAT_INC( pseq->m_stat_retired_memory_miss[proc] );        //Luke
        opstat->opMemoryLatency( op, latency );
      }
    }
  } else {
    // else: not a memory operation
    // classify this instruction as effective and/or dependent
    
    // effective test
    if ( (dinstr->getFetchTime() > pseq->m_stat_last_miss_fetch[proc]) &&
         (dinstr->getFetchTime() < pseq->m_stat_last_miss_retire[proc]) ) {

      // dependent test: did this instruction execute before the memory miss
      if ( completeTime < pseq->m_stat_last_miss_retire[proc] ) {
        pseq->m_stat_miss_effective_ind[proc]++;
      } else {
        pseq->m_stat_miss_effective_dep[proc]++;
      }
      
    } else {
      // increment inter-cluster count
      pseq->m_stat_miss_inter_cluster[proc]++;
    }
  }
  
  if (STAT_EXPENSIVE_PROFILE) {
    // ATOMICS
    //**************************************************************************
    if (s->getType() == DYN_ATOMIC) {
      atomic_inst_t *atomicop = (atomic_inst_t *)dinstr;

      // exclude certain PCs from the lock analysis
      if (m_exclude_lock_table->find(dinstr->getVPC()) ==
          m_exclude_lock_table->end()) {
      
        // track the number of atomic instructions that occur
#if 0
        pa_t addr               = atomicop->getAddress();
        if (m_pc_lock_table->find( addr ) == m_pc_lock_table->end() ) {
          (*m_pc_lock_table)[addr] = new lock_stat_t();
        }
        lock_stat_t *ls = (*m_pc_lock_table)[addr];
        ls->lock_atomic( pseq, atomicop, atomicop->getProc() );
#endif
      } else {
        pseq->m_exclude_count++;
      }
    
      pa_t pa = atomicop->getPhysicalAddress();
      if ( (pa != (pa_t) -1) &&
           atomicop->isCacheable() ) {
        if (pa == 0x0) {
          pseq->out_info( "0x%0llx %d %d\n",
                          pa, atomicop->getASI(),
                          atomicop->isCacheable() );
          atomicop->printDetail();
        }
        la_t thread_p = pseq->getCurrentThread(atomicop->getProc());  //Luke
        thread_stat_t *ts = (*m_thread_stat_table)[thread_p];
        if (ts == NULL) {
          ts = new thread_stat_t( (la_t) -1, -1, pseq->getID() );
          (*m_thread_stat_table)[thread_p] = ts;
        }
        m_mem_stat->atomic( ts->m_internal_id, atomicop->getAddress(), pa,
                            atomicop->getAccessSize() );
      }
    }

    // STORES
    //**************************************************************************
    if (s->getType() == DYN_STORE) {
      store_inst_t *storeop = (store_inst_t *) dinstr;
#if 0
      pa_t addr               = storeop->getAddress();
      if (m_pc_lock_table->find( addr ) != m_pc_lock_table->end() ) {
        lock_stat_t *ls = (*m_pc_lock_table)[addr];
        ls->lock_write( pseq, storeop, storeop->getProc() );       //Luke
      }
#endif
      pa_t pa = storeop->getPhysicalAddress();
      if ( (pa != (pa_t) -1) &&
           storeop->isCacheable() ) {
        if (pa == 0x0) {
          pseq->out_info( "0x%0llx %d %d\n",
                          pa, storeop->getASI(), storeop->isCacheable());
          storeop->printDetail();
        }
        la_t thread_p = pseq->getCurrentThread(storeop->getProc());  //Luke
        thread_stat_t *ts = (*m_thread_stat_table)[thread_p];
        if (ts == NULL) {
          ts = new thread_stat_t( (la_t) -1, -1, pseq->getID() );
          (*m_thread_stat_table)[thread_p] = ts;
        }
        m_mem_stat->write( ts->m_internal_id, storeop->getAddress(), pa,
                           storeop->getAccessSize() );
      }
    }
  
    // LOADS
    //**************************************************************************
    if (s->getType() == DYN_LOAD) {
      load_inst_t *loadop = (load_inst_t *) dinstr;
      pa_t pa = loadop->getPhysicalAddress();
      if (loadop->isCacheable() && (pa != (pa_t) -1)) {
        if (pa == 0x0) {
          pseq->out_info( "0x%0llx %d %d\n",
                          pa, loadop->getASI(), loadop->isCacheable());
          loadop->printDetail();
        }
        la_t thread_p = pseq->getCurrentThread(loadop->getProc());  //Luke
        thread_stat_t *ts = (*m_thread_stat_table)[thread_p];
        if (ts == NULL) {
          ts = new thread_stat_t( (la_t) -1, -1, pseq->getID() );
          (*m_thread_stat_table)[thread_p] = ts;
        }
        m_mem_stat->read( ts->m_internal_id, loadop->getAddress(), pa,
                          loadop->getAccessSize() );
      }
    }

    // memory tracing facility
    //**************************************************************************
#if 0
    if (s->getType() == DYN_LOAD ||
        s->getType() == DYN_STORE ||
        s->getType() == DYN_ATOMIC) {
      memory_inst_t *memop = (memory_inst_t *) dinstr;
      if (memop->getAddress() == m_trace_address) {
        pseq->out_info("TRACE [0x%0llx]: 0x%0llx %s 0x%x (%d) 0x%0llx\n",
                       m_trace_address,
                       memop->getVPC(),
                       decode_opcode( s->getOpcode() ),
                       memop->getASI(),
                       memop->isDataValid(),
                       memop->getUnsignedData());
      }
      if (memop->getPhysicalAddress() == m_trace_phys_address) {
        pseq->out_info("TRACE [0x%0llx]: 0x%0llx %s 0x%x (%d) 0x%0llx\n",
                       m_trace_phys_address,
                       memop->getVPC(),
                       decode_opcode( s->getOpcode() ),
                       memop->getASI(),
                       memop->isDataValid(),
                       memop->getUnsignedData());
      }
    }
#endif
    
    if ( dinstr->getVPC() >= 0x10042978 && dinstr->getVPC() <= 0x10042a70 ) {
      pseq->m_thread_count_idle++;
    }
    pseq->m_thread_count++;
    // pseq->m_thread_histogram->touch( dinstr->getVPC(), 1 );
  }
}

//**************************************************************************
void sys_stat_t::observeStaticInstruction( pseq_t *pseq, 
                                           static_inst_t *s_instr, uint32 proc )
{
  //Luke
  assert(proc < CONFIG_LOGICAL_PER_PHY_PROC);

  if (STAT_EXPENSIVE_PROFILE) {
#ifdef BRANCH_TRACING
    if (branch_type == BRANCH_COND ||
        branch_type == BRANCH_PCOND) {
      // write changed instruction::
      m_branch_trace->writeChangedBranch( fetchPC, fetchNPC, 
                                          fetchPhysicalPC, traplevel,
                                          s_instr );
    }

    // update static branch prediction (predicated) instruction statistics
    if (s_instr->getType() == DYN_CONTROL) {
      uint32 inst;
      int    op2;
      int    pred;
      switch (s_instr->getBranchType()) {
      case BRANCH_COND:
        // conditional branch
        STAT_INC( pseq->m_nonpred_count_stat[proc] );
        break;
      
      case BRANCH_PCOND:
        // predicted conditional
        inst = s_instr->getInst();
        op2  = maskBits32( inst, 22, 24 );
        pred = maskBits32( inst, 19, 19 );
        if ( op2 == 3 ) {
          // integer register w/ prediction
          STAT_INC( pseq->m_pred_reg_count_stat[proc] );
          if (pred) {
            STAT_INC( pseq->m_pred_reg_taken_stat[proc] );
          } else {
            STAT_INC( pseq->m_pred_reg_nottaken_stat[proc] );
          }
        } else {
          STAT_INC( pseq->m_pred_count_stat[proc] );
          if (pred) {
            STAT_INC( pseq->m_pred_count_taken_stat[proc] );
          } else {
            STAT_INC( pseq->m_pred_count_nottaken_stat[proc] );
          }
        }
        break;

      default:
        ;      // ignore non-predictated branches
      }
    }
#endif //  BRANCH_TRACING
  } //  EXPENSIVE_STATS
}

//**************************************************************************
void sys_stat_t::observeThreadSwitch( int32 id, uint32 proc )
{

  //Luke - id is the index number for the m_seq and m_state arrays, proc is the logical processor
  //           number inside a SMT processor 
  ASSERT(id >= 0 && id < system_t::inst->m_numSMTProcs);
  ASSERT(proc < CONFIG_LOGICAL_PER_PHY_PROC);

  if (STAT_EXPENSIVE_PROFILE) {
    pseq_t   *pseq   = system_t::inst->m_seq[id];
    pstate_t *pstate = system_t::inst->m_state[id];
    int64     ttime  = pseq->m_thread_timer[proc]->relative_time();   //Luke
    int64     tcycle = pseq->m_thread_timer[proc]->relative_cycle();
    la_t      address= pseq->getThreadPhysAddress(proc);
    int32     pid = 0;
    // Luke - simics_proc_num is the actual Simics processor number, given the array index
    //            id (SMT proc number) and the logical processor number proc within the SMT proc
    int32 simics_proc_num = (id * CONFIG_LOGICAL_PER_PHY_PROC) + proc;

    // previously: read thread from cpu_list offset
    // la_t   thread   = dereference( cpu_list + 0x10, 8 );

    // now: "address" is the physical address of the thread pointer--
    //    : read as physical memory instead
    la_t threadp = 0;
    bool success = pstate->readPhysicalMemory( address, 8, (ireg_t *) &threadp, proc );  //Luke
    if (!success) {
      DEBUG_OUT( "read phys mem failed: 0x%0llx\n", address );
    }    

    if (m_thread_stat_table->find(threadp) == m_thread_stat_table->end()) {
      (*m_thread_stat_table)[threadp] = new thread_stat_t( (la_t) -1, pid, simics_proc_num );
    }
    thread_stat_t *ts = (*m_thread_stat_table)[threadp];
    ts->runOnCpu(simics_proc_num);  //Luke
    // if the pid isn't discovered, try to find it out
    pid = ts->m_pid;
    if (pid == -1) {
      pid = pseq->getPid(proc);  // pid is -1 if read fails
      //Luke - altered bc we should have all the info we need...
      //pid = pseq->getPid(threadp);
      ts->m_pid = pid;
    }
    ts->m_execution_count += ttime;

    DEBUG_OUT("cpu: %2.2d  pid: %4.4d 0x%0llx IPC: %4.4f abstime: %lld time: %lld cycle: %lld idle: %4.4f\n",
              id, pid, threadp,
              (float) ttime / (float) tcycle,
              pseq->getLocalCycle(), ttime, tcycle,
              ((float) pseq->m_thread_count_idle[proc] / (float) ttime) * 100.0
              );
    pseq->m_thread_timer[proc]->reset();   //Luke
    // pseq->m_thread_histogram->walk( threadTableWalker );

    // reset all the statistics
    // Luke
    pseq->m_thread_count[proc] = 0;
    pseq->m_thread_count_idle[proc] = 0;
    // delete pseq->m_thread_histogram;
    // pseq->m_thread_histogram = new histogram_t( "Thread PC", 2048 );
  }
}

//**************************************************************************
void sys_stat_t::observeIOAction( memory_transaction_t *mem_op )
{
  if (STAT_EXPENSIVE_PROFILE) {
    attr_value name_val = SIM_get_attribute( mem_op->s.ini_ptr, "name" );
    const char *name = "unknown";
    if (name_val.kind == Sim_Val_String) {
      name = name_val.u.string;
    }

    const char *action = "UNKNOWN";
    if ( SIM_mem_op_is_write( &mem_op->s ) ) {
      action = "WRITE";
    } else if ( SIM_mem_op_is_read( &mem_op->s ) ) {
      action = "READ";
    }

    DEBUG_OUT( "I/O %s: [0x%0llx] %s\n",
               action, mem_op->s.physical_address, name );

    if ( SIM_mem_op_is_write( &mem_op->s ) ) {
      m_mem_stat->write( m_io_thread->m_internal_id,
                         mem_op->s.logical_address,
                         mem_op->s.physical_address,
                         mem_op->s.size );
    } else if ( SIM_mem_op_is_read( &mem_op->s ) ) {
      m_mem_stat->read( m_io_thread->m_internal_id,
                        mem_op->s.logical_address,
                        mem_op->s.physical_address,
                        mem_op->s.size );
    } else {
      ERROR_OUT( "error: sys_stat_t: observe I/O action: unknown type!\n" );
      SIM_HALT;
    }
  }
}

//**************************************************************************
void sys_stat_t::observeTransactionComplete( int32 id, uint32 proc )
{
  //Luke - id is the sequencer index (SMT chip number), and proc is the logical processor
  //          number within the SMT chip
  ASSERT(id >= 0 && id < system_t::inst->m_numSMTProcs);
  ASSERT(proc < CONFIG_LOGICAL_PER_PHY_PROC);

  if (STAT_EXPENSIVE_PROFILE) {
    pseq_t   *pseq   = system_t::inst->m_seq[id];
    pstate_t *pstate = system_t::inst->m_state[id];
    la_t      threadp = 0;
    pstate->readPhysicalMemory( pseq->getThreadPhysAddress(proc), 8,
                                (ireg_t *) &threadp, proc );    //Luke
    int32 pid;
    //Luke - this is the actual Simics processor number
    int32 simics_proc_num = (id * CONFIG_LOGICAL_PER_PHY_PROC) + proc;
    
    if (m_thread_stat_table->find(threadp) != m_thread_stat_table->end()) {
      thread_stat_t *ts = (*m_thread_stat_table)[threadp];
      ts->runOnCpu(simics_proc_num);
      pid = ts->m_pid;
      if (pid == -1) {
        pid = pseq->getPid(proc);
         //Luke - altered bc we should have all the info we need
        //pid = pseq->getPid(threadp);     
        ts->m_pid = pid;
      }
    } else {
      pid = pseq->getPid(proc);
      //pid = pseq->getPid(threadp);
      if (pid != -1) {
        (*m_thread_stat_table)[threadp] = new thread_stat_t( (la_t) -1, pid, simics_proc_num ); //Luke
      }
    }

    DEBUG_OUT("complete cpu: %2.2d  pid: %4.4d abstime: %lld 0x%0llx\n",
              simics_proc_num, pid, pseq->getLocalCycle(),
              threadp );
  }
}

//**************************************************************************
void sys_stat_t::printStats( void )
{
  if (STAT_EXPENSIVE_PROFILE) {
    // print out the status of the locks
    PerPCLockTable::iterator pc_iter;
    lock_stat_t            **lock_array;
    lock_array = (lock_stat_t **) malloc( sizeof(lock_stat_t *) * 
                                          m_pc_lock_table->size() );
    pc_iter = m_pc_lock_table->begin();
    for (uint32 i = 0; i < m_pc_lock_table->size(); i++) {
      ASSERT ( pc_iter != m_pc_lock_table->end() );
      lock_array[i] = pc_iter->second;
      pc_iter++;
    }

    qsort( lock_array, m_pc_lock_table->size(),
           sizeof(lock_stat_t *), lock_compare );

    // print out the last 10 elements in the array
    for (uint32 i = MAX(0, m_pc_lock_table->size() - 10);
         i < m_pc_lock_table->size(); i++) {
      lock_stat_t *lock_stat = lock_array[i];
      // CM FIX: should introduce static system wide output stream
      lock_stat->print( system_t::inst->m_seq[0] );
    }

    //Luke
    for ( int32 i = 0; i < system_t::inst->m_numSMTProcs; i++ ) {  
      for(uint k =0; k < CONFIG_LOGICAL_PER_PHY_PROC; ++k){
        pseq_t   *pseq   = system_t::inst->m_seq[i];    
        pseq->out_info("total number of atomics / excluded atomics: %lld / %lld\n",
                       pseq->m_stat_atomics_retired[k],
                       pseq->m_exclude_count[k]);
        if ( pseq->m_stat_atomics_retired[k] > 0 ) {
          pseq->out_info("percent excluded atomics: %4.2f\n",
                         ((float) pseq->m_exclude_count[k] /
                          (float) pseq->m_stat_atomics_retired[k]) * 100.0 );
        }
      }
    }

    DEBUG_OUT( "ID Thread Ptr    Retired PID (if any)\n");
    int32 max_internal_id  = -1;
    ThreadStatTable::iterator iter;
    for ( iter = m_thread_stat_table->begin();
          iter != m_thread_stat_table->end();
          iter++ ) {
      thread_stat_t *ts = iter->second;
      if (ts->m_internal_id > max_internal_id) {
        max_internal_id = ts->m_internal_id;
      }
      DEBUG_OUT( "%2d 0x%0llx %0lld %4d\n", ts->m_internal_id, iter->first, 
                 ts->m_execution_count, ts->m_pid );
    }

    // print out thread statistics
    DEBUG_OUT( "Thread Migration Stats\n");
    for ( iter = m_thread_stat_table->begin();
          iter != m_thread_stat_table->end();
          iter++ ) {
      thread_stat_t *ts = iter->second;
      if (ts->m_migrations > 0) {
        DEBUG_OUT( "MIGRATIONS 0x%0llx %d\n", iter->first, ts->m_migrations );
      }
    }
    
    // zeroth bit is not used so
    m_mem_stat->printStats( max_internal_id );
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

/*------------------------------------------------------------------------*/
/* Static methods                                                         */
/*------------------------------------------------------------------------*/

//***************************************************************************
static int lock_compare( const void *el1, const void *el2 )
{
  const lock_stat_t **l1 = (const lock_stat_t **) el1;
  const lock_stat_t **l2 = (const lock_stat_t **) el2;

  if ( (*l1)->m_acquires > (*l2)->m_acquires ) {
    return 1;
  } else if ( (*l1)->m_acquires == (*l2)->m_acquires ) {
    return 0;
  }
  // else if ( l1->m_acquires < l2->m_acquires )
  return -1;
}

//**************************************************************************
static void threadTableWalker( histogram_t *h, int PC, int accesses )
{
  if (accesses > 1000) {
    printf("\t0x%0x %d\n", PC, accesses );
  }
}

/*------------------------------------------------------------------------*/
/* Global functions                                                       */
/*------------------------------------------------------------------------*/


/** [Memo].
 *  [Internal Documentation]
 */
//**************************************************************************


