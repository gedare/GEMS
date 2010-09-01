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
 * FileName:  dynamic.C
 * Synopsis:  Represents an in-flight instruction in the processor
 * Description: Contains information related to an instruction being executed,
 *              which is distinct from the generic decoded instruction.
 * Author:    zilles
 * Maintainer:cmauer
 * Version:   $Id$
 */

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/
#include "hfa.h"
#include "hfacore.h"

#include "lsq.h"
#include "exec.h"
#include "scheduler.h"
#include "pipestate.h"
#include "rubycache.h"
#include "dynamic.h"

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Variable declarations                                                  */
/*------------------------------------------------------------------------*/

/// memory allocator for dynamic.C
listalloc_t dynamic_inst_t::m_myalloc;

/// The static jump table for the flow inst class
pmf_dynamicExecute dynamic_inst_t::m_jump_table[i_maxcount];

/*------------------------------------------------------------------------*/
/* Forward declarations                                                   */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Constructor(s) / destructor                                            */
/*------------------------------------------------------------------------*/

/**
 *  Constructor:
 *     assumes the dynamic instruction is in the Fetch state,
 *     resets the counters for the object.
 */
//**************************************************************************
dynamic_inst_t::dynamic_inst_t( static_inst_t *s_inst, 
                                int32 window_index,
                                pseq_t *pseq,
                                abstract_pc_t *at,
                                pa_t physicalPC, 
                                trap_type_t trapgroup,
                                uint32 proc
                                )
{
  #ifdef DEBUG_DYNAMIC
     DEBUG_OUT("dynamic_inst_t:constructor BEGIN\n");
  #endif
  this->m_proc           = proc;
  this->m_pseq           = pseq;
  this->s                = s_inst;
  this->m_virtual_address= at->pc;
  this->m_physical_pc = physicalPC;
  this->m_windex         = window_index;
  this->seq_num          = pseq->stepSequenceNumber(proc);  
  this->m_fetch_cycle    = pseq->getLocalCycle();
  this->m_complete_cycle = 0;
  // for checkpointing purposes, keep a copy of the m_fetch_at
  this->m_checkpoint_fetch_at = *at;
  
  /* instruction was just fetched */
  this->m_stage  = FETCH_STAGE;
  this->m_events = 0;
  this->m_traptype = (uint16) Trap_NoTrap;

  // set the trapgroup for this instruction
  this->m_trapgroup = trapgroup;

  #ifdef DEBUG_DYNAMIC
      DEBUG_OUT("\tassigning the pstate variables...\n");
  #endif
  uint16 pstate  = at->pstate;
  uint16 cwp     = at->cwp;
  uint16 gset    = at->gset;
  
  // get priv info from pstate
  m_priv = ((pstate & 0x4) == 0x4);


  // clear the event times
  for (int i = 0; i < (int) EVENT_TIME_NUM_EVENT_TIMES; i++) {
    m_event_times[i] = 0;
  }

  #ifdef DEBUG_DYNAMIC
    DEBUG_OUT("\tabout to increment ref count...\n");
  #endif
  // increment the static instruction reference counter
  s_inst->incrementRefCount();
  
  // CM It should be possible to optimize the copying of register from
  //       the static instruction here (particularly "tofree")

  if ( !(cwp < NWINDOWS) ) {
    m_pseq->out_info("dynamic_t:: constructor: warning: cwp is %d, set to %d\n",
                     cwp, (cwp & (NWINDOWS - 1)));
    cwp &= (NWINDOWS - 1);
  }
  if ( !(gset <= REG_GLOBAL_INT) ) {
    m_pseq->out_info("dynamic_t:: constructor: warning: gset is %d\n", gset);
    gset = REG_GLOBAL_INT;
  }
  
  #ifdef DEBUG_DYNAMIC
  DEBUG_OUT("\tgetting regbox...\n");
  #endif
  reg_box_t &rbox = pseq->getRegBox();      
  for (int i = 0; i < SI_MAX_SOURCE; i++) {
    #ifdef DEBUG_DYNAMIC
         char b[128];
         int len = s_inst->printDisassemble(b);
         if(len >0){
              DEBUG_OUT("\tconstructor: Disassembled instr = %s\n",b);
          }
        DEBUG_OUT("\tabout to getsourceReg...\n");
    #endif
    reg_id_t &id = s_inst->getSourceReg(i);
    #ifdef DEBUG_DYNAMIC
    DEBUG_OUT("\t(constructor) dynamic instr seq_num[%d] proc[%d] src#%d\n",
              this->seq_num, proc, i);
    #endif
    m_source_reg[i].copy( id, rbox );     
    m_source_reg[i].setVanillaState( cwp, gset );

    //reset l2miss dep flags
    m_source_l2miss_dep[i] = false;

  }

  for (int i = 0; i < SI_MAX_DEST; i++) {
      #ifdef DEBUG_DYNAMIC
        DEBUG_OUT("\tabout to getDestReg...\n");
      #endif
    reg_id_t &id = s_inst->getDestReg(i);
    #ifdef DEBUG_DYNAMIC
    DEBUG_OUT("\t(constructor) dynamic instr seq_num[%d] proc[%d] dest#%d\n",
              this->seq_num, proc, i);
    #endif
    m_dest_reg[i].copy( id, rbox );      
    m_dest_reg[i].setVanillaState( cwp, gset );
    m_tofree_reg[i].copy( id, rbox );      
    m_tofree_reg[i].setVanillaState( cwp, gset );
  }
}

//**************************************************************************
dynamic_inst_t::~dynamic_inst_t()
{
  // if the static instruction is marked, free it
  s->decrementRefCount();
  if (s->getFlag( SI_TOFREE )) {
    if (s->getRefCount() == 0) {
      delete s;
    }
    // CM FIX: no longer trying to figure out if ref counting is a good idea
    //ERROR_OUT("warning: ref counting is a good idea!\n");
  }
}

//**************************************************************************
void 
dynamic_inst_t::Decode( uint64 issueTime )
{
  assert(m_proc < CONFIG_LOGICAL_PER_PHY_PROC);

  #ifdef DEBUG_DYNAMIC
      DEBUG_OUT("dynamic_inst_t:Decode issueTime[%d] seqnum[%d] proc[%d]\n",issueTime,
                         seq_num, m_proc);
  #endif

  // record when decode takes place
  m_event_times[EVENT_TIME_DECODE] = m_pseq->getLocalCycle() - m_fetch_cycle;

  /*
   * for each source, get the mapped register 
   */
  for (int i = 0; i < SI_MAX_SOURCE; i++) {
   #ifdef DEBUG_DYNAMIC
    //  DEBUG_OUT("\tBefore getSourceReg...\n");
  #endif
    reg_id_t &source = getSourceReg(i);
  #ifdef DEBUG_DYNAMIC
    //DEBUG_OUT("\tAfter getSourceReg\n");
  #endif
    source.getARF()->readDecodeMap( source, m_proc );    

    /* WATTCH power */
    if(WATTCH_POWER){
      m_pseq->getPowerStats()->incrementRegfileAccess();
    }
  }

  //allocate destination registers
  allocateDestRegs();

  /* WATTCH power */
  if(WATTCH_POWER){
    m_pseq->getPowerStats()->incrementRenameAccess();
  }

  /* instruction scheduling priority is passed in a decode time
     (this can be updated later, but isn't CM 8/2002). */
  SetPriority( issueTime );
  SetStage(DECODE_STAGE);

 #ifdef DEBUG_DYNAMIC
     DEBUG_OUT("dynamic_inst_t:Decode END issueTime[%d] proc[%d] seqnum[%d]\n",issueTime,
                         m_proc, seq_num);
  #endif
}

//*************************************************************************
void
dynamic_inst_t::allocateDestRegs(){
 /*
   * for each destination, 
   *     - get the currently mapped register (to free later)
   *     - allocate a new register to write into
   */
  for (int i = 0; i < SI_MAX_DEST; i++) {
   #ifdef DEBUG_DYNAMIC
      DEBUG_OUT("\tBefore getDestReg...\n");
   #endif
    reg_id_t &dest    = getDestReg(i);

     #ifdef DEBUG_DYNAMIC
    //     DEBUG_OUT("\tAfter getDestReg...\n");
     #endif
    if (!dest.isZero()) {
     #ifdef DEBUG_DYNAMIC
      //  DEBUG_OUT("\tBefore getToFreeReg...\n");
       #endif
      reg_id_t &tofree = getToFreeReg(i);
        #ifdef DEBUG_DYNAMIC
      //     DEBUG_OUT("\tAfter getToFreeReg tofree\n");
          #endif

#ifdef RENAME_EXPERIMENT
      // get the number of renames for this register (dest)
      half_t count   = dest.getARF()->renameCount(dest, m_proc);    
      half_t logical = dest.getARF()->getLogical(dest, m_proc);        
      m_pseq->logLogicalRenameCount( dest.getRtype(), logical, count );
#endif

         #ifdef DEBUG_DYNAMIC
           // DEBUG_OUT("\tBefore readDecodeMap proc[%d]\n",m_proc);
         #endif
      // get the previous mapping so we can free it later
      tofree.getARF()->readDecodeMap(tofree, m_proc);    


      //all regs should always be mapped to valid physical reg
      if( tofree.getPhysical() == PSEQ_INVALID_REG){
        // this should NEVER OCCUR
        char buf[128];
        s->printDisassemble(buf);
        ERROR_OUT("dynamic_inst_t:: allocateDestRegs ERROR INVALID REG vanilla[ 0x%x ] seqnum[ %lld ] cycle[ %lld ] fetched[ %lld ] TofreeArf[ %s ] DestArf[ %s ] %s\n",
                  tofree.getVanilla(), seq_num, m_pseq->getLocalCycle(), m_fetch_cycle, tofree.rid_type_menomic( tofree.getRtype() ), dest.rid_type_menomic( dest.getRtype() ), buf);
      }
      ASSERT( tofree.getPhysical() != PSEQ_INVALID_REG);
        
         #ifdef DEBUG_DYNAMIC
            // DEBUG_OUT("\tAfter readDecodeMap\n");
          #endif
      // allocate a new physical register, and
      // set the logical mapping to point at it
      bool success = dest.getARF()->allocateRegister( dest, m_proc );   
              
      if (!success) {
        ERROR_OUT("[ %d ] dynamic_t:: decode: unable to allocate register:\n", m_pseq->getID());
        printDetail();
      }

    #ifdef DEBUG_PSEQ
      char buf[128];
      s->printDisassemble(buf);
      DEBUG_OUT("dynamic_inst_t::allocateDestReg  DEST index[ %d ] vanilla[ %d ] physical[ %d ] Arf[ %s ] TOFREE vanilla[ %d ] physical[ %d ] Arf[ %s ] seq_num[ %d ] cycle[ %lld ] %s\n", i, dest.getVanilla(), dest.getPhysical(), dest.rid_type_menomic( dest.getRtype() ), tofree.getVanilla(), tofree.getPhysical(), tofree.rid_type_menomic( tofree.getRtype()), seq_num, m_pseq->getLocalCycle(), buf);
    #endif

      //set the dest register's writer sequence number to our sequence number
      dest.getARF()->setWriterSeqnum( dest, seq_num, m_proc );

      // CM REGISTERRENAME DEBUG
      // DEBUG_OUT(" -- old dest (%d)\n", i);
      // tofree.print();
      // DEBUG_OUT(" -- destination (%d)\n", i);
      // dest.print();
    }
  }
}

//**************************************************************************
void 
dynamic_inst_t::testSourceReadiness() { 
 #ifdef DEBUG_DYNAMIC
     DEBUG_OUT("dynamic_inst_t:testSourceReadiness BEGIN proc[%d] seqnum[%d]\n",m_proc,seq_num);
  #endif
  if (s->getType() == DYN_STORE) {
    // store instrs start waiting on source register 2, instead of 3 --
    //   This allows stores to issue as soon as address is available,
    //   instead of waiting for store data
    SetStage( WAIT_2ND_STAGE );

  } else if (s->getType() == DYN_ATOMIC) {
    /** Some replay traps are triggered by casa and casxa instructions.
     *  casa and casxa only use source1 for address generation, while
     *  swap(a) and ldstub(a) use both source 1 & 2 for a-gen.
     *  You could optimize the hardware this by starting cas(x)a instructions
     *  in WAIT_1ST_STAGE, and double checking that source 2&3 are ready
     *  in the memop function Execute().
     */
    if (s->getOpcode() == i_casa || s->getOpcode() == i_casxa) {
      SetStage( WAIT_1ST_STAGE );
    } else {
      SetStage( WAIT_2ND_STAGE );
    }
  } else {
    SetStage( WAIT_4TH_STAGE );
  }
   #ifdef DEBUG_DYNAMIC
     DEBUG_OUT("dynamic_inst_t:testSourceReadiness END proc[%d] seqnum[%d]\n",m_proc,seq_num);
  #endif
}

//**************************************************************************
void 
dynamic_inst_t::Schedule() { 
   #ifdef DEBUG_DYNAMIC
     DEBUG_OUT("dynamic_inst_t:Schedule BEGIN proc[%d] seqnum[%d]\n", m_proc,seq_num);
  #endif

     //used to indicate whether to stall scheduler
     bool stall = false;

     #ifdef DEBUG_REG
       char buf[128];
       s->printDisassemble(buf);
     #endif

  switch (m_stage) {
  case WAIT_4TH_STAGE:
    {
        #ifdef DEBUG_REG
             DEBUG_OUT("WAIT_4th_STAGE, cycle[ %lld ] seqnum[ %d ] proc[ %d ] %s\n", m_pseq->getLocalCycle(), seq_num, m_proc, buf);
      #endif
      reg_id_t &source = getSourceReg(3);
      if ( !source.getARF()->isReady( source, m_proc ) ) {
        #ifdef DEBUG_WAIT
            DEBUG_OUT("dynamic_inst_t:Schedule: waiting for source reg (4th):seqnum[%d]\n\n",
                                  seq_num);
            source.print(m_proc);
        #endif

            //check whether waiting on L2 miss reg
            if( source.getARF()->isL2Miss( source, m_proc)){
              markEvent( EVENT_DEP_L2_MISS );
              //increment the number of dependent instrs
              //DEBUG_OUT("WAIT_4th_STAGE seq_num[ %lld ] cycle[ %lld ] incrementL2MissDep\n", seq_num, m_pseq->getLocalCycle());
              source.getARF()->incrementL2MissDep( source, m_proc );
              //mark this source as being l2miss dependent
              m_source_l2miss_dep[3] = true;
            }

        source.getARF()->waitResult( source, this, m_proc );
        break;
      }
        #ifdef DEBUG_WAIT
            DEBUG_OUT("dynamic_inst_t:Schedule: NOT waiting for source reg (4th):seqnum[%d]\n\n",
                                  seq_num);
            source.print(m_proc);
        #endif

      SetStage(WAIT_3RD_STAGE);

      /* WATTCH power */
      if(WATTCH_POWER){
        m_pseq->getPowerStats()->incrementWindowAccess();
      }
    }
    /* fall through */
  case WAIT_3RD_STAGE:
    {
        #ifdef DEBUG_REG
             DEBUG_OUT("WAIT_3rd_STAGE, cycle[ %lld ] seqnum[ %d ] proc[ %d ] %s\n", m_pseq->getLocalCycle(), seq_num, m_proc, buf);
      #endif

      reg_id_t &source = getSourceReg(2);
      if ( !source.getARF()->isReady( source, m_proc ) ) {
          #ifdef DEBUG_WAIT
            DEBUG_OUT("dynamic_inst_t:Schedule: waiting for source reg (3rd): seqnum[%d]\n\n",
                                 seq_num);
            source.print(m_proc);
        #endif

            //check whether waiting on L2 miss reg
            if( source.getARF()->isL2Miss( source, m_proc)){
              markEvent( EVENT_DEP_L2_MISS );
              //increment the number of dependent instrs
              //DEBUG_OUT("WAIT_3rd_STAGE seq_num[ %lld ] cycle[ %lld ] incrementL2MissDep\n", seq_num, m_pseq->getLocalCycle());
              source.getARF()->incrementL2MissDep( source, m_proc );
              //mark this source as being l2miss dependent
              m_source_l2miss_dep[2] = true;
            }

        source.getARF()->waitResult( source, this, m_proc );
        break;
      }
        #ifdef DEBUG_WAIT
            DEBUG_OUT("dynamic_inst_t:Schedule: NOT waiting for source reg (3rd): seqnum[%d]\n\n",
                                 seq_num);
            source.print(m_proc);
        #endif

      SetStage(WAIT_2ND_STAGE);

      /* WATTCH power */
      if(WATTCH_POWER){
        m_pseq->getPowerStats()->incrementWindowAccess();
      }
    }
    /* fall through */
  case WAIT_2ND_STAGE:
    {
        #ifdef DEBUG_REG
             DEBUG_OUT("WAIT_2nd_STAGE, cycle[ %lld ] seqnum[ %d ] proc[ %d ] %s\n", m_pseq->getLocalCycle(), seq_num, m_proc, buf);
      #endif

      reg_id_t &source = getSourceReg(1);
      if ( !source.getARF()->isReady( source, m_proc ) ) {
          #ifdef DEBUG_WAIT
            DEBUG_OUT("dynamic_inst_t:Schedule: waiting for source reg (2nd):seqnum[%d]\n\n",
                                  seq_num);
            source.print(m_proc);
        #endif

            //check whether waiting on L2 miss reg
            if( source.getARF()->isL2Miss( source, m_proc)){
              markEvent( EVENT_DEP_L2_MISS );
              //increment the number of dependent instrs
              //DEBUG_OUT("WAIT_2nd_STAGE seq_num[ %lld ] cycle[ %lld ] incrementL2MissDep\n", seq_num, m_pseq->getLocalCycle());
              source.getARF()->incrementL2MissDep( source, m_proc );
              //mark this source as being l2miss dependent
              m_source_l2miss_dep[1] = true;
            }

        source.getARF()->waitResult( source, this, m_proc );
        break;
      }
        #ifdef DEBUG_WAIT
            DEBUG_OUT("dynamic_inst_t:Schedule: NOT waiting for source reg (2nd):seqnum[%d]\n\n",
             seq_num);
            source.print(m_proc);
        #endif

      SetStage(WAIT_1ST_STAGE);

      /* WATTCH power */
      if(WATTCH_POWER){
        m_pseq->getPowerStats()->incrementWindowAccess();
      }
    }
    /* fall through */
  case WAIT_1ST_STAGE:
    {
        #ifdef DEBUG_REG
             DEBUG_OUT("WAIT_1st_STAGE, cycle[ %lld ] seqnum[ %d ] proc[ %d ] %s\n", m_pseq->getLocalCycle(), seq_num, m_proc, buf);
      #endif

      reg_id_t &source = getSourceReg(0);
      if ( !source.getARF()->isReady( source, m_proc ) ) {
          #ifdef DEBUG_WAIT
            DEBUG_OUT("dynamic_inst_t:Schedule: waiting for source reg (1st): seqnum[%d]\n\n",
                    seq_num);
            source.print(m_proc);
        #endif

            //check whether waiting on L2 miss reg
            if( source.getARF()->isL2Miss( source, m_proc)){
              markEvent( EVENT_DEP_L2_MISS );
              //increment the number of dependent instrs
              //DEBUG_OUT("WAIT_1st_STAGE seq_num[ %lld ] cycle[ %lld ] incrementL2MissDep\n", seq_num, m_pseq->getLocalCycle());
              source.getARF()->incrementL2MissDep( source, m_proc );
              //mark this source as being l2miss dependent
              m_source_l2miss_dep[0] = true;
            }

        source.getARF()->waitResult( source, this, m_proc );
        break;
      }
        #ifdef DEBUG_WAIT
            DEBUG_OUT("dynamic_inst_t:Schedule: NOT waiting for source reg (1st):seqnum[%d]\n\n",
                                seq_num);
            source.print(m_proc);
        #endif

      SetStage(READY_STAGE);

      /* WATTCH power */
      if(WATTCH_POWER){
        m_pseq->getPowerStats()->incrementWindowAccess();
      }
    }
    /* fall through */
    case READY_STAGE:
        #ifdef DEBUG_REG
             DEBUG_OUT("READY_STAGE, cycle[ %lld ] seqnum[ %d ] proc[ %d ] %s\n", m_pseq->getLocalCycle(), seq_num, m_proc, buf);
      #endif

    // record when all operands are ready
    m_event_times[EVENT_TIME_OPERANDS_READY] = m_pseq->getLocalCycle() - m_fetch_cycle;

    /* WATTCH power */
    if(WATTCH_POWER){
      m_pseq->getPowerStats()->incrementWindowAccess();
      m_pseq->getPowerStats()->incrementWinSelectionAccess();
    }

    /* when all registers are ready, tell the scheduler that this
       instruction is ready */
    if(STORESET_PREDICTOR){
      //LUKE - if this instruction is a LD, check StoreSet predictor for conflicts
      if(s->getType() == DYN_LOAD){
        // We will get put onto wait list if StoreSet says LD should stall
        if((static_cast< load_inst_t *> (this))->checkStoreSet()){
          //we are stalled, just exit without calling scheduler
          stall = true;
        }
      }
      else if(s->getType() == DYN_ATOMIC){
        // We will get put onto wait list if StoreSet Atomic should stall
        if((static_cast<atomic_inst_t *> (this))->checkStoreSet()){
          stall = true;
        }
      }
    }
    if(!stall){
      m_pseq->getScheduler()->schedule(this);
    }
    #ifdef DEBUG_WAIT
        DEBUG_OUT("dynamic_inst_t: Schedule after calling the scheduler seqnum[%d] proc[%d]\n",
                            seq_num, m_proc);
    #endif
    break;
  case EXECUTE_STAGE:
      #ifdef DEBUG_REG
             DEBUG_OUT("EXECUTE_STAGE, cycle[ %lld ] seqnum[ %d ] proc[ %d ] %s\n", m_pseq->getLocalCycle(), seq_num, m_proc, buf);
      #endif

    Execute();
    break;
  case LSQ_WAIT_STAGE:
      #ifdef DEBUG_REG
             DEBUG_OUT("LSQ_WAIT_STAGE, cycle[ %lld ] seqnum[ %d ] proc[ %d ] %s\n", m_pseq->getLocalCycle(), seq_num, m_proc, buf);
      #endif

    /* copy the value in */
    (dynamic_cast<load_interface_t *> (this))->lsqBypass();
    break;
  case ADDR_OVERLAP_STALL_STAGE:
    /* Loads enter this stage when it finds an older store in the LSQ with an overlapping address.
     * Load will wait until the store retires (and removes itself from LSQ), so that load can read
     * value from cache.
     * Note we call different instances of continueExecution() based on whether LD or ATOMIC inst
     */
    #ifdef DEBUG_REG
             DEBUG_OUT("ADDR_OVERLAP_STALL_STAGE, cycle[ %lld ] seqnum[ %d ] proc[ %d ] %s\n", m_pseq->getLocalCycle(), seq_num, m_proc, buf);
      #endif

    //mark this event
    markEvent( EVENT_ADDR_OVERLAP_STALL );
    if(s->getType() == DYN_LOAD){
      (static_cast<load_inst_t *> (this))->continueExecution();
    }
    else if(s->getType() == DYN_ATOMIC){
      (static_cast<atomic_inst_t *> (this))->continueExecution();
    }
    else{
      ASSERT(0);
    }
    break;
  case STORESET_STALL_LOAD_STAGE:
    /* Loads enter this stage when StoreSet predictor predicts load will conflict with an in-flight store instr
     * possibly leading to a load replay.  Load gets woken up when Store retires (gets removed from LSQ)
     */
    // tell scheduler we are now ready to proceed
      #ifdef DEBUG_REG
             DEBUG_OUT("STORESET_STALL_LOAD_STAGE, cycle[ %lld ] seqnum[ %d ] proc[ %d ] %s\n", m_pseq->getLocalCycle(), seq_num, m_proc, buf);
      #endif

    m_pseq->getScheduler()->schedule(this);
    break;
  case EARLY_STORE_STAGE:
    // The store data is now available for the write...
    //    a prefetch has already been launched for the block
    //    calls storeDataToCache()
    //    calls store complete() if appropriate
    {
        #ifdef DEBUG_REG
             DEBUG_OUT("EARLY_STORE_STAGE, cycle[ %lld ] seqnum[ %d ] proc[ %d ] %s\n", m_pseq->getLocalCycle(), seq_num, m_proc, buf);
      #endif

      for (int32 i = 0; i < SI_MAX_SOURCE; i++) {
        reg_id_t &source = getSourceReg(i);
        if ( !source.getARF()->isReady( source, m_proc ) ) {
             #ifdef DEBUG_WAIT
              DEBUG_OUT("dynamic_inst_t:Schedule: waiting for source reg (early store): seqnum[%d]\n\n",
                                   seq_num);
              source.print(m_proc);
           #endif

          source.getARF()->waitResult( source, this, m_proc );
          return;
        }
      }
        #ifdef DEBUG_WAIT
            DEBUG_OUT("dynamic_inst_t:Schedule: NOT waiting for source reg (early store): seqnum[%d]\n\n", seq_num);
        #endif

      (static_cast<store_inst_t *> (this))->storeDataWakeup();
      break;
    }

  case EARLY_ATOMIC_STAGE:
    {
      #ifdef DEBUG_REG
             DEBUG_OUT("EARLY_ATOMIC_STAGE, cycle[ %lld ] seqnum[ %d ] proc[ %d ] %s\n", m_pseq->getLocalCycle(), seq_num, m_proc, buf);
      #endif

      // The atomic memory operation either got its memory value from LSQ or not, but
      // The value to write is not ready yet... make sure all sources are ready
      // IMPORTANT: LIke EARLY_STORE_STAGE, need to make sure we have store permissions
      //     before calling Complete(), so need to call storeDataWakeup() instead of Complete() at
      //     this point!
      for (int32 i = 0; i < SI_MAX_SOURCE; i++) {
        reg_id_t &source = getSourceReg(i);
        if ( !source.getARF()->isReady( source, m_proc ) ) {
            #ifdef DEBUG_WAIT
            DEBUG_OUT("dynamic_inst_t:Schedule: waiting for source reg (early atomic): seqnum[%d]\n\n",
                                 seq_num);
            source.print(m_proc);
        #endif

          source.getARF()->waitResult( source, this, m_proc );
          return;
        }
      }
        #ifdef DEBUG_WAIT
            DEBUG_OUT("dynamic_inst_t:Schedule: NOT waiting for source reg (early atomic): seqnum[%d]\n\n", seq_num);
               #endif
            (static_cast<store_inst_t *> (this))->storeDataWakeup();
            //(static_cast<atomic_inst_t *> (this))->Complete();
    }
    break;

  case CACHE_MISS_STAGE:
     #ifdef DEBUG_REG
             DEBUG_OUT("CACHE_MISS_STAGE, cycle[ %lld ] seqnum[ %d ] proc[ %d ] %s\n", m_pseq->getLocalCycle(), seq_num, m_proc, buf);
      #endif

    // the cache line was not present in the cache ... we await its return
    //      (for stores, the data to be written is ready)
    //    calls load complete if it was a load
    //    calls store complete if it was a store ...
    static_cast<memory_inst_t *> (this)->Complete(); 
    break;

  case CACHE_NOTREADY_STAGE:
      #ifdef DEBUG_REG
             DEBUG_OUT("CACHE_NOTREADY_STAGE, cycle[ %lld ] seqnum[ %d ] proc[ %d ] %s\n", m_pseq->getLocalCycle(), seq_num, m_proc, buf);
      #endif

    // the cache's resources are full, the access hasn't issued yet.
    //    poll to re-issue the request.
    //    calls load complete if it was a load
    //    calls store complete if it was a store ...
    if (static_cast<memory_inst_t *>(this)->accessCache() == true) {
      // cache access succeed: now move to new state: complete
      static_cast<memory_inst_t *>(this)->Complete(); 
    }
    // default: retry the request until it completes
    break;

  case CACHE_MISS_RETIREMENT_STAGE:
     #ifdef DEBUG_REG
             DEBUG_OUT("CACHE_MISS_RETIREMENT_STAGE, cycle[ %lld ] seqnum[ %d ] proc[ %d ] %s\n", m_pseq->getLocalCycle(), seq_num, m_proc, buf);
      #endif

    // Just set the m_retirement_flag to be true.  Should only be here if we are already complete!
    static_cast<memory_inst_t *> (this)->setRetirementPermission( true );
    // do NOT call SetStage() bc we don't want to recompute m_complete_cycle. 
    m_stage = COMPLETE_STAGE;
    break;

  case CACHE_NOTREADY_RETIREMENT_STAGE:
      #ifdef DEBUG_REG
             DEBUG_OUT("CACHE_NOTREADY_RETIREMENT_STAGE, cycle[ %lld ] seqnum[ %d ] proc[ %d ] %s\n", m_pseq->getLocalCycle(), seq_num, m_proc, buf);
      #endif

    // continue to poll rubycache.  If true, this means we have a HIT, so set retirement permission flag
    //  Otherwise we have a MISS or NOT_READY, so don't set flag.
    if(static_cast<memory_inst_t *>(this)->accessCacheRetirement() == true){
      static_cast<memory_inst_t *>(this)->setRetirementPermission( true );
      // do NOT call SetStage() bc we don't want to recompute m_complete_cycle
      m_stage = COMPLETE_STAGE;
    }
    //NOT_READY or MISS, so continue to stall
    break;
    
  case WB_FULL_STAGE:
      #ifdef DEBUG_REG
             DEBUG_OUT("WB_FULL_STAGE, cycle[ %lld ] seqnum[ %d ] proc[ %d ] %s\n", m_pseq->getLocalCycle(), seq_num, m_proc, buf);
      #endif

    // Initated when the Write Buffer is FULL and the store instruction is executing.  Thus
    //   we continually poll the addToWriteBuffer() return value until an entry is freed:
    if(static_cast<store_inst_t *>(this)->addToWriteBuffer() == true){
      // Write buffer insertion succeeded - now move to complete stage
      static_cast<store_inst_t *>(this)->Complete(); 
    }
    // default: retry the store request until it completes
    break;

  case RETIRE_STAGE:
    ERROR_OUT("error: instruction woken up in retire stage.\n");
    printDetail();
    SIM_HALT;

  default:
    /* should never reach this stage */
    SIM_HALT;
  }

 #ifdef DEBUG_DYNAMIC
     DEBUG_OUT("dynamic_inst_t:Schedule END proc[%d] seqnum[%d]\n",m_proc,seq_num);
  #endif
  // DEBUG_OUT("instruction: %s stage: %d\n", print(), m_stage);
}

//**************************************************************************
void
dynamic_inst_t::printInstruction(){
   char buf[128];
  s->printDisassemble(buf);
  DEBUG_OUT("\tdynamic_inst_t: printInstruction() %s seqnum[ %lld ] fetched[ %lld ] cycle[ %lld ]", buf, seq_num, m_fetch_cycle, m_pseq->getLocalCycle());
  //print source and dest regs
  DEBUG_OUT(" SOURCES: ");
  for(int i=0; i < SI_MAX_SOURCE; ++i){
    reg_id_t & source = getSourceReg(i);
    if(!source.isZero()){
      DEBUG_OUT("( [%d] V: %d P: %d Arf: %s Written: %lld Writerseqnum: %lld)", i,source.getVanilla(), source.getPhysical(), source.rid_type_menomic( source.getRtype() ), source.getARF()->getWrittenCycle( source, m_proc ), source.getARF()->getWriterSeqnum( source, m_proc) );
    }
  }
  DEBUG_OUT(" DESTS: ");
  for(int i=0; i < SI_MAX_DEST; ++i){
    reg_id_t & dest = getDestReg(i);
    if(!dest.isZero()){
      DEBUG_OUT("( [%d] V: %d P: %d Arf: %s)", i,dest.getVanilla(), dest.getPhysical(), dest.rid_type_menomic( dest.getRtype() ));
    }
  }
  DEBUG_OUT("\n");
}

//**************************************************************************
void 
dynamic_inst_t::beginExecution() { 
   #ifdef DEBUG_DYNAMIC
     DEBUG_OUT("dynamic_inst_t:beginExecution BEGIN\n");
  #endif
  // record when execution takes place
  m_event_times[EVENT_TIME_EXECUTE] = m_pseq->getLocalCycle() - m_fetch_cycle;
  
  SetStage(EXECUTE_STAGE);
  STAT_INC(m_pseq->m_stat_total_insts[m_proc]);

 #ifdef DEBUG_DYNAMIC
     DEBUG_OUT("dynamic_inst_t:beginExecution END\n");
  #endif
}

//***************************************************************************
void
dynamic_inst_t::Execute( void ) {
  #ifdef DEBUG_DYNAMIC
     DEBUG_OUT("dynamic_inst_t:Execute BEGIN proc[%d] seqnum[%d]\n",m_proc,seq_num);
  #endif
  // record when execution takes place
  m_event_times[EVENT_TIME_EXECUTE_DONE] = m_pseq->getLocalCycle() - m_fetch_cycle;

  // call the appropriate function
  static_inst_t *si = getStaticInst();

  #ifdef DEBUG_DYNAMIC
  char buf[128];
  s->printDisassemble(buf);
  DEBUG_OUT("[ %d ] dynamic_inst_t: EXECUTE %s seqnum[ %lld ] fetched[ %lld ] cycle[ %lld ]", m_pseq->getID(), buf, seq_num, m_fetch_cycle, m_pseq->getLocalCycle());
  //print source and dest regs
  DEBUG_OUT(" SOURCES: ");
  for(int i=0; i < SI_MAX_SOURCE; ++i){
    reg_id_t & source = getSourceReg(i);
    if(!source.isZero()){
      DEBUG_OUT("( [%d] V: %d P: %d Arf: %s WriterSN: %lld WrittenCycle: %lld State: 0x%x)", i,source.getVanilla(), source.getPhysical(), source.rid_type_menomic( source.getRtype() ), source.getARF()->getWriterSeqnum( source, m_proc ), source.getARF()->getWrittenCycle( source, m_proc), source.getVanillaState() );
    }
  }
  DEBUG_OUT(" DESTS: ");
  for(int i=0; i < SI_MAX_DEST; ++i){
    reg_id_t & dest = getDestReg(i);
    if(!dest.isZero()){
      DEBUG_OUT("( [%d] V: %d P: %d Arf: %s)", i,dest.getVanilla(), dest.getPhysical(), dest.rid_type_menomic( dest.getRtype() ));
    }
  }
  DEBUG_OUT("\n");
#endif


  #if 0
  if(m_pseq->getLocalCycle() >= 0){
    char buf[128];
    getStaticInst()->printDisassemble(buf);
    cout << "P_" << m_pseq->getID() << " " << buf << " Cycle[ " << m_pseq->getLocalCycle() << " ] Seqnum[ " << seq_num << " ] ";
    for(int i=0; i < SI_MAX_SOURCE; ++i){
    reg_id_t & source = getSourceReg(i);
    if(!source.isZero()){
      word_t physreg = source.getPhysical();
      word_t vanillareg = (word_t) source.getVanilla();
      abstract_rf_t * arf = source.getARF();
      //get timestamp and writer info
      uint64 written_cycle = source.getARF()->getWrittenCycle( source, m_proc );
      uint64 writer_seqnum = source.getARF()->getWriterSeqnum( source, m_proc );
      cout << "source_" << i << " :( type: " << reg_id_t::rid_type_menomic( source.getRtype() ) << " arf: 0x" << hex << arf << " v: " << dec << vanillareg << " | p: " << dec << physreg << " ) [ " << writer_seqnum << " " << written_cycle << " ] ";
    }
  }
  //now print out dest regs
  for(int i=0; i < SI_MAX_DEST; ++i){
    reg_id_t & dest = getDestReg(i);
    if(!dest.isZero()){
      bool l2miss = dest.getARF()->isL2Miss( dest, m_proc );
      word_t physreg = dest.getPhysical();
      word_t vanillareg = (word_t ) dest.getVanilla();
      abstract_rf_t * arf = dest.getARF();
      cout << "dest_" << i << " :( type: " << reg_id_t::rid_type_menomic( dest.getRtype() ) << " arf: 0x" << hex << arf << " v: " << dec << vanillareg << " | p: " << physreg << " ) [ l2miss: ";
      if(l2miss){
        cout << "TRUE";
      }
      else{
        cout << "FALSE";
      }
      cout << " ] ";
    }
  }
  cout << endl;
  }
#endif

  //print out the register that are written to
  #if 0
  char buf[128];
  s->printDisassemble(buf);
  DEBUG_OUT("dynamic_inst_t:: Execute, about to write to regs, cycle[ %lld ] seqnum[ %lld ] %s\n", m_pseq->getLocalCycle(), seq_num, buf);
  for(int i=0; i < SI_MAX_DEST; ++i){
    reg_id_t & dest = getDestReg(i);
    if( !dest.isZero() ){
      DEBUG_OUT("\tDest[ %d ] Vanilla[ %d ] Physical[ %d ] Arf[ %s ]\n", i, dest.getVanilla(), dest.getPhysical(), dest.rid_type_menomic( dest.getRtype() ) );
    }
  }
 #endif
    
  // execute the instruction using the jump table
  pmf_dynamicExecute pmf = m_jump_table[si->getOpcode()];
  (this->*pmf)();

#if 0
  if (!si->getFlag( SI_COMPLEX_OP )) {
    // execute this instruction
    //   call the appropriate jump table function
    dp_int_t *dp_value = new dp_int_t();
    if (!getSourceReg(0).isZero()) {
      dp_value->m_source[0] = getSourceReg(0).getARF()->readRegister( getSourceReg(0), m_proc );
    }
    
    if (si->getFlag(SI_ISIMMEDIATE)) {
      dp_value->m_source[1].uint_64 = si->getImmediate();
    } else {
      if (!getSourceReg(1).isZero()) {
        dp_value->m_source[1] = getSourceReg(1).getARF()->readRegister( getSourceReg(1), m_proc );
      }
    }
  
    // read the input condition code register (shifted if necessary)
    if (getSourceReg(2).getRtype() == RID_CC) {
      dp_value->m_source_cc.ccode = getSourceReg(2).getARF()->readInt64( getSourceReg(2), m_proc ) >> si->getCCShift();
    }
    
    dp_value->m_exception = Trap_NoTrap;
    
    // do the operation
    exec_fn_execute( (i_opcode) si->getOpcode(), dp_value );
  
    // store the result back to the register file
    getDestReg(0).getARF()->writeRegister( getDestReg(0), dp_value->m_dest, m_proc );
    if (getDestReg(1).getRtype() == RID_CC) {
      getDestReg(1).getARF()->writeInt64( getDestReg(1),
                                          dp_value->m_dest_cc.ccode, m_proc ); 
    }
  }
#endif

  // check that this instruction has written its destination registers
  if ( getTrapType() == Trap_NoTrap ) {
    ASSERT( getDestReg(0).getARF()->isReady( getDestReg(0), m_proc ));
    
    // stxa's write the control registers at retire time (when they become 
    // non-speculative). So they need a conditional check.
    // See memory_inst_t::Retire() for more information (9/12/2002)
    if (getStaticInst()->getOpcode() != i_stxa) {
      if( getDestReg(1).getARF()->isReady( getDestReg(1), m_proc ) == false){
        printInstruction();
      }
      ASSERT( getDestReg(1).getARF()->isReady( getDestReg(1), m_proc ));
    }
  }
  SetStage(COMPLETE_STAGE);

 #ifdef DEBUG_DYNAMIC
     DEBUG_OUT("dynamic_inst_t:Execute END proc[%d] seqnum[%d]\n",m_proc,seq_num);
  #endif
}

//**************************************************************************
void
dynamic_inst_t::Retire( abstract_pc_t *a )
{

  #ifdef DEBUG_RETIRE
  if(Waiting() != false || Disconnected() == false){
  DEBUG_OUT("dynamic_inst_t:Retire BEGIN with m_id[%d] proc[%d] seqnum[%d]\n",m_pseq->getID(),m_proc,seq_num);
  DEBUG_OUT("\ta.pc (begin)= %0x\n",a->pc);
  DEBUG_OUT("\ta.npc (begin)= %0x\n", a->npc);
  DEBUG_OUT("\tWaiting = %d\n",Waiting());
  DEBUG_OUT("\tDisconnected =  %d\n",Disconnected());
  }
  #endif

  // record when execution takes place
  m_event_times[EVENT_TIME_RETIRE] = m_pseq->getLocalCycle() - m_fetch_cycle;

  ASSERT( !getEvent( EVENT_FINALIZED ) );
  ASSERT( !s->getFlag( SI_FETCH_BARRIER ) );
#ifdef DEBUG_RETIRE
  if(Waiting() != false || Disconnected() == false){
  m_pseq->out_info("## Integer Retire Stage\n");
  printDetail();
  }
#endif

  SetStage(RETIRE_STAGE);
  retireRegisters();

  // NOTE: you could just call "nextPC_execute" here, if it wasn't for mops

  // advance the program counter by calling member function "nextPC"
  pmf_nextPC pmf = s->nextPC;
  (this->*pmf)( a );
  markEvent( EVENT_FINALIZED );

  #ifdef DEBUG_RETIRE
  if(Waiting() != false || Disconnected() == false){
    DEBUG_OUT("\ta.pc (end)= %0x\n",a->pc);
    DEBUG_OUT("\ta.npc (end)= %0x\n", a->npc);
    DEBUG_OUT("dynamic_inst_t:Retire END with m_id[%d] proc[%d] seqnum[%d]\n",m_pseq->getID(),m_proc,seq_num);
  }
  #endif
}

//**************************************************************************
void 
dynamic_inst_t::retireRegisters() {
  #ifdef DEBUG_DYNAMIC_RET
     DEBUG_OUT("dynamic_inst_t:retireRegisters BEGIN with proc[%d] seqnum[%d]\n",m_proc,seq_num);
  #endif

  //print out this instruction and source operand info (written cycle and seq. num of writer)
  # if 0
  char buf[128];
  getStaticInst()->printDisassemble(buf);
  cout << "P_" << m_pseq->getID() << " " << buf << " Cycle[ " << m_pseq->getLocalCycle() << " ] Seqnum[ " << seq_num << " ] ";
  for(int i=0; i < SI_MAX_SOURCE; ++i){
    reg_id_t & source = getSourceReg(i);
    if(!source.isZero()){
      word_t physreg = source.getPhysical();
      word_t vanillareg = (word_t) source.getVanilla();
      //get timestamp and writer info
      uint64 written_cycle = source.getARF()->getWrittenCycle( source, m_proc );
      uint64 writer_seqnum = source.getARF()->getWriterSeqnum( source, m_proc );
      cout << "source_" << i << " :( v: " << dec << vanillareg << " | p: " << dec << physreg << " ) [ " << writer_seqnum << " " << written_cycle << " ] ";
    }
  }
  //now print out dest regs
  for(int i=0; i < SI_MAX_DEST; ++i){
    reg_id_t & dest = getDestReg(i);
    if(!dest.isZero()){
      bool l2miss = dest.getARF()->isL2Miss( dest, m_proc );
      word_t physreg = dest.getPhysical();
      word_t vanillareg = (word_t ) dest.getVanilla();
      cout << "dest_" << i << " :( v: " << vanillareg << " | p: " << physreg << " ) [ l2miss: ";
      if(l2miss){
        cout << "TRUE";
      }
      else{
        cout << "FALSE";
      }
      cout << " ] ";
    }
  }
  //now print out L2 miss specific info:
  if(getEvent( EVENT_L2_MISS )){
    cout << "L2_MISS";
  }
  if(getEvent( EVENT_DEP_L2_MISS )){
    cout << "L2_MISS_DEP";
  }
  cout << endl;
#endif

  for (int i = 0; i < SI_MAX_DEST; i++) {
    reg_id_t &dest    = getDestReg(i);

    // if the register isn't the %g0 register --
    if (!dest.isZero()) {

      // NOTE: for the control registers, you must first writeRetireMap then
      //       free the registers. writeRetire does a copy of unused registers
      //       at this point.

      /* WATTCH power */
      if(WATTCH_POWER){
        m_pseq->getPowerStats()->incrementRegfileAccess();
        m_pseq->getPowerStats()->incrementResultBusAccess();
        m_pseq->getPowerStats()->incrementWinWakeupAccess();
      }

      // dest should always be valid
      ASSERT( dest.getPhysical() != PSEQ_INVALID_REG);

      // update the logical->physical mapping in the "retired" map
      dest.getARF()->writeRetireMap( dest, m_proc );      

      // free the old physical register: old_reg      
      reg_id_t &tofree = getToFreeReg(i);

      #ifdef DEBUG_DYNAMIC_RET
      DEBUG_OUT("dynamic_inst_t:retireRegisters() about to call freeRegister, proc[%d] seqnum[%d]\n",
                m_proc,seq_num);
       #endif

      //check to see whether there exists any instructions that had L2 depdendencies on this reg
      if( (tofree.getPhysical() == PSEQ_INVALID_REG)){
        // this should NEVER OCCUR, as all logical regs should have been mapped to physical reg
        char buf[128];
        s->printDisassemble(buf);
        ERROR_OUT("dynamic_inst_t:: retireRegs ERROR INVALID REG Vanilla[ 0x%x ] seqnum[ %lld ] cycle[ %lld ] fetched[ %lld ] TofreeArf[ %s ] DestArf[ %s ] %s\n",
                  tofree.getVanilla(), seq_num, m_pseq->getLocalCycle(), m_fetch_cycle, tofree.rid_type_menomic( tofree.getRtype() ), dest.rid_type_menomic( dest.getRtype() ), buf);
      }
      // tofree should never be invalid
      ASSERT( tofree.getPhysical() != PSEQ_INVALID_REG);

      //only get the miss dep on mapped tofree regs
      int total_dep = tofree.getARF()->getL2MissDep( tofree, m_proc);
      bool l2miss = tofree.getARF()->isL2Miss( tofree, m_proc );
      if(l2miss){
        m_pseq->updateL2MissDep(total_dep);
      }

      tofree.getARF()->freeRegister( tofree, m_proc );  

       #ifdef DEBUG_DYNAMIC_RET
         DEBUG_OUT("dynamic_inst_t:retireRegisters() END freeRegister, proc[%d] seqnum[%d]\n",
                m_proc, seq_num);
       #endif
    }
  }
  #ifdef DEBUG_DYNAMIC_RET
  DEBUG_OUT("dynamic_inst_t:retireRegisters END with proc[%d]\n",m_proc);
  #endif
}

//**************************************************************************
void 
dynamic_inst_t::FetchSquash() { 
   #ifdef DEBUG_DYNAMIC
     DEBUG_OUT("dynamic_inst_t:FetchSquash BEGIN proc[%d]\n",m_proc);
  #endif
  ASSERT(!Waiting());
  ASSERT(m_stage <= FETCH_STAGE);
#ifdef PIPELINE_VIS
  m_pseq->out_log("squash %d\n", getWindowIndex());
#endif
  m_pseq->decrementSequenceNumber(m_proc); 
  #ifdef DEBUG_DYNAMIC
     DEBUG_OUT("dynamic_inst_t:FetchSquash END proc[%d]\n",m_proc);
  #endif
}

//**************************************************************************
void 
dynamic_inst_t::Squash() { 
   #ifdef DEBUG_DYNAMIC
     DEBUG_OUT("dynamic_inst_t:Squash BEGIN proc[%d]\n",m_proc);
  #endif
  ASSERT( !getEvent( EVENT_FINALIZED ) );
  ASSERT(m_stage != RETIRE_STAGE);
  if (Waiting()) {
    RemoveWaitQueue();
  }
  UnwindRegisters( );
  m_pseq->decrementSequenceNumber(m_proc);  

#ifdef PIPELINE_VIS
  m_pseq->out_log("squash %d\n", getWindowIndex());
#endif
  markEvent( EVENT_FINALIZED );
   #ifdef DEBUG_DYNAMIC
     DEBUG_OUT("dynamic_inst_t:Squash END proc[%d]\n", m_proc);
  #endif
}

//**************************************************************************
void dynamic_inst_t::Wakeup( void )
{
  #ifdef DEBUG_WAIT
  DEBUG_OUT("dynamic_inst_t: Wakeup BEGIN seqnum[%d] proc[%d]\n",seq_num,m_proc);
  #endif
  Schedule();
   #ifdef DEBUG_WAIT
  DEBUG_OUT("dynamic_inst_t: Wakeup END seqnum[%d] proc[%d]\n",seq_num,m_proc);
  #endif
}

/// sets the stage of execution
//**************************************************************************
void dynamic_inst_t::SetStage(enum stage_t stage)
{
#ifdef PIPELINE_VIS
  if (stage != WAIT_2ND_STAGE && stage != WAIT_3RD_STAGE)
    m_pseq->out_log("stage %d %s\n", getWindowIndex(),
                    printStageAbbr(stage) );
#endif
  this->m_stage = stage;

  /* if we are finished this instruction: */
  if (m_stage == COMPLETE_STAGE) {
    /* ready to retire RETIRE_STAGES cycles later */
    m_complete_cycle = m_pseq->getLocalCycle() + RETIRE_STAGES;
  }
}

//***************************************************************************
const char *dynamic_inst_t::print( void ) const
{
  return ( decode_opcode( (enum i_opcode) s->getOpcode() ) );
}

//***************************************************************************
bool dynamic_inst_t::isRetireReady()  {
  /* make sure we have set m_complete_cycle */
  if (m_stage == COMPLETE_STAGE){
    if(m_complete_cycle == 0){
      DEBUG_OUT("dynamic_inst_t::isRetireReady, m_complete_cycle is 0! cycle[ %lld ] seqnum[ %d ] proc[ %d ]\n", m_pseq->getLocalCycle(), seq_num, m_proc);
    }
    ASSERT(m_complete_cycle != 0);
  }
  
  // This check is used for non-memory instructions
  bool ready =  (m_stage == COMPLETE_STAGE) &&
         (m_pseq->getLocalCycle() >= m_complete_cycle);

  // LUKE - Memory instructions need to have correct retirement permissions before retiring them.
  //  If not, issue cache request to obtain correct permissions:
  if((s->getType() == DYN_LOAD) || (s->getType() == DYN_STORE) || (s->getType() == DYN_ATOMIC) ||
      (s->getType() == DYN_PREFETCH)){

    bool retire_permission = static_cast<memory_inst_t *>(this)->getRetirementPermission(); 

    //Issue a retirement permission request only if we are complete and don't have permissions
    // RETIREMENT_CACHE_ACCESS indicates whether to perform this check
    if( RETIREMENT_CACHE_ACCESS && (m_stage == COMPLETE_STAGE) && (retire_permission == false) ){
      if(static_cast<memory_inst_t *>(this)->accessCacheRetirement()){
        //if we have HIT, make sure retire permission is set
        retire_permission = static_cast<memory_inst_t *>(this)->getRetirementPermission();
        ASSERT(retire_permission == true);
      }
      //else we are already placed on wait list
    }
    else if( !RETIREMENT_CACHE_ACCESS 
             && ( m_stage == COMPLETE_STAGE) )  {
      //we should already be in the complete stage
      if( m_stage != COMPLETE_STAGE ){
        printDetail();
      }
      ASSERT( m_stage == COMPLETE_STAGE );
      //skip obtaining retirement permissions
      if(retire_permission == false){
        static_cast<memory_inst_t *>(this)->setRetirementPermission(true);
        retire_permission = true;
      }
    }

    // if we do not have permissions m_stage is set to RETIREMENT_STAGE
    ready = ((retire_permission == true) && 
             (m_stage == COMPLETE_STAGE) && (m_pseq->getLocalCycle() >= m_complete_cycle) );
  }

  if(!ready){
    // prints out the instruction at head of ROB
      #ifdef DEBUG_PSEQ
    //  if(m_pseq->getLocalCycle() > debugio_t::getDebugTime()){
    //    printDetail();
    //  }
     #endif

    //record stage of instruction that is causing it to be not ready
    STAT_INC(m_pseq->m_stat_retire_notready_stage[m_stage]);

    if(CONFIG_WITH_RUBY){
      rubycache_t * rcache = m_pseq->getRubyCache();
      // if not ready then check whether request is L2Miss or not
      memory_inst_t * meminst = static_cast<memory_inst_t *>(this);
      if((m_stage == CACHE_MISS_STAGE) && (rcache->isRequestL2Miss(meminst->getPhysicalAddress())) ){
        //mark this instr as experience L2 Miss
        meminst->markL2Miss();
      }
      
    } //CONFIG_WITH_RUBY 
  }
  return ready;
}

//***************************************************************************
void dynamic_inst_t::printDetail( void )
{
  char buf[128];
  s->printDisassemble(buf);
  DEBUG_OUT(" dynamic instruction: [%s]: %s\n", 
         decode_opcode( (enum i_opcode) s->getOpcode() ), buf);
  DEBUG_OUT("   proc: %d\n", m_pseq->getID());
  DEBUG_OUT("   thread: %d\n", m_proc);
  DEBUG_OUT("   window: %d\n", m_windex);
  DEBUG_OUT("   stage : %s\n", printStage(m_stage) );
  DEBUG_OUT("   priv:   %d\n", m_priv);
  DEBUG_OUT("   VPC    : 0x%0llx\n", getVPC());
  DEBUG_OUT("   PC    : 0x%0llx\n", getPC());
  DEBUG_OUT("   inst  : 0x%0x\n", s->getInst());
  DEBUG_OUT("   fetched : %d\n", m_fetch_cycle);
  DEBUG_OUT("   executed : %d\n",  m_event_times[EVENT_TIME_EXECUTE] + m_fetch_cycle);
  DEBUG_OUT("   retired : %d\n", m_event_times[EVENT_TIME_RETIRE] + m_fetch_cycle);
  DEBUG_OUT("   complete_cycle : %d\n", m_complete_cycle);
  DEBUG_OUT("   seq # : %lld\n", seq_num);
  if (getTrapType() != Trap_NoTrap) {
    DEBUG_OUT("   TRAP: 0x%0x\n", getTrapType() );
  }
  if (m_events != 0) {
    DEBUG_OUT("   EVENTS: 0x%0x\n", m_events);
  }
  if (s->getFlag(SI_ISIMMEDIATE)) {
    DEBUG_OUT("   Immediate: 0x%0llx\n", s->getImmediate());
  } else {
    DEBUG_OUT("   No Immediate\n");
  }
  DEBUG_OUT("   Offset: 0x%0llx\n", s->getOffset());

  printRegs( false );
}

//***************************************************************************
void dynamic_inst_t::printRegs( bool print_values )
{
  DEBUG_OUT(" dynamic source regs:   \n");
  for (int i = 0; i < SI_MAX_SOURCE; i ++) {
    reg_id_t &reg = getSourceReg(i);
    if (!reg.isZero()) {
      DEBUG_OUT("  %2.2d ", i);
      abstract_rf_t *arf = reg.getARF();
      if (arf != NULL) {
        arf->print( reg, m_proc );    
      } else {
        DEBUG_OUT("  NULL ARF ");
      }
      if (print_values) {
        if (reg.getPhysical() == PSEQ_INVALID_REG)
          DEBUG_OUT("not decoded\n");
        else if (reg.getARF()->isReady( reg, m_proc ))
          DEBUG_OUT("value = 0x%0llx\n",
                           reg.getARF()->readInt64( reg, m_proc ));  
        else
          DEBUG_OUT("not ready\n");
      }
    }
  }

  DEBUG_OUT(" dynamic dest regs:   \n");
  for (int i = 0; i < SI_MAX_DEST; i ++) {
    reg_id_t &reg = getDestReg(i);
    if (!reg.isZero()) {
      DEBUG_OUT("  %2.2d ", i);
      abstract_rf_t *arf = reg.getARF();
      if (arf != NULL) {
        arf->print( reg, m_proc );      
      } else {
        DEBUG_OUT("  NULL ARF ");
      }
      if (print_values) {
        if (reg.getPhysical() == PSEQ_INVALID_REG)
          DEBUG_OUT("not decoded\n");
        else if (reg.getARF()->isReady( reg, m_proc ))
          DEBUG_OUT("value = 0x%0llx\n",
                           reg.getARF()->readInt64( reg, m_proc ));   
        else
          DEBUG_OUT("not ready\n");
      }
    }
  }

  DEBUG_OUT(" to free dest regs:   \n");
  for (int i = 0; i < SI_MAX_DEST; i ++) {
    reg_id_t &reg = getToFreeReg(i);
    if (!reg.isZero()) {
      DEBUG_OUT("  %2.2d ", i);
      abstract_rf_t *arf = reg.getARF();
      if (arf != NULL) {
        arf->print( reg, m_proc );        
      } else {
        DEBUG_OUT("  NULL ARF ");
      }
      if (print_values) {
        if (reg.getPhysical() == PSEQ_INVALID_REG)
          DEBUG_OUT("not decoded\n");
        else if (reg.getARF()->isReady( reg, m_proc ))
          DEBUG_OUT("value = 0x%0llx\n",
                           reg.getARF()->readInt64( reg, m_proc ));    
        else
          DEBUG_OUT("not ready\n");
      }
    }
  }
  DEBUG_OUT("\n");
}

//**************************************************************************
void
dynamic_inst_t::UnwindRegisters( void )
{
 #ifdef DEBUG_DYNAMIC_RET
     DEBUG_OUT("dynamic_inst_t:UnwindRegisters BEGIN proc[%d]\n",m_proc);
  #endif

  for (int i = 0; i < SI_MAX_DEST; i++) {
    reg_id_t &dest    = getDestReg(i);
    // if the register isn't the %g0 register --
    if (!dest.isZero()) {

      reg_id_t &tofree = getToFreeReg(i);
#if 0
      m_pseq->out_info("Old destination:\n");
      tofree.print();
      if (tofree.getRtype() == RID_INT &&
          tofree.isReady( box )) {
        m_pseq->out_info("Revert to value: 0x%0llx\n", tofree.readInt64( box ));
      }
      m_pseq->out_info("Current Dest:   \n");
      dest.print();
#endif
      #ifdef DEBUG_DYNAMIC_RET
         DEBUG_OUT("dynamic_inst_t:UnwindRegisters calling writeDecodeMap with index[%d] logical[ %d ] physical[ %d ] proc[%d] seqnum[%d]\n",i, tofree.getVanilla(), tofree.getPhysical(), m_proc, seq_num);
      #endif

      // revert the logical->physical mapping in the "decode" map
      tofree.getARF()->writeDecodeMap( tofree, m_proc );     

      // free the physical register
      #ifdef DEBUG_DYNAMIC_RET
        DEBUG_OUT("dynamic_inst_t:UnwindRegisters calling freeRegister with index[%d] logical[ %d ] physical[%d] proc[%d] seqnum[%d]\n",i, dest.getVanilla(), dest.getPhysical(), m_proc, seq_num);
      #endif

      dest.getARF()->freeRegister( dest, m_proc );  

#ifdef DEBUG_DYNAMIC_RET
      DEBUG_OUT("dynamic_inst_t:UnwindRegisters END freeRegister with logical[%d] physical[%d] proc[%d] seqnum[%d]\n",i, dest.getPhysical(), m_proc, seq_num);
  #endif
    }
  }

   #ifdef DEBUG_DYNAMIC_RET
     DEBUG_OUT("dynamic_inst_t:UnwindRegisters END proc[%d]\n", m_proc);
  #endif
}

//***************************************************************************
const char *dynamic_inst_t::printStage( stage_t stage )
{
  switch (stage) {

  case FETCH_STAGE:
    return ("FETCH_STAGE");
  case DECODE_STAGE:
    return ("DECODE_STAGE");
  case WAIT_4TH_STAGE:
    return ("WAIT_4TH_STAGE");
  case WAIT_3RD_STAGE:
    return ("WAIT_3RD_STAGE");
  case WAIT_2ND_STAGE:
    return ("WAIT_2ND_STAGE");
  case WAIT_1ST_STAGE:
    return ("WAIT_1ST_STAGE");
  case READY_STAGE:
    return ("READY_STAGE");
  case EXECUTE_STAGE:
    return ("EXECUTE_STAGE");
  case LSQ_WAIT_STAGE:
    return ("LSQ_WAIT_STAGE");
  case EARLY_STORE_STAGE:
    return ("EARLY_STORE_STAGE");
  case EARLY_ATOMIC_STAGE:
    return ("EARLY_ATOMIC_STAGE");
  case CACHE_MISS_STAGE:
    return ("CACHE_MISS_STAGE");
  case CACHE_NOTREADY_STAGE:
    return ("CACHE_NOTREADY_STAGE");
  case CACHE_MISS_RETIREMENT_STAGE:
    return ("CACHE_MISS_RETIREMENT_STAGE");
  case CACHE_NOTREADY_RETIREMENT_STAGE:
    return ("CACHE_NOTREADY_RETIREMENT_STAGE");
  case WB_FULL_STAGE:
    return ("WB_FULL_STAGE");
  case ADDR_OVERLAP_STALL_STAGE:
    return ("ADDR_OVERLAP_STALL_STAGE");
  case STORESET_STALL_LOAD_STAGE:
    return ("STORESET_STALL_LOAD_STAGE");
  case COMPLETE_STAGE:
    return ("COMPLETE_STAGE");
  case RETIRE_STAGE:
    return ("RETIRE_STAGE");
  default:
    return ("Unknown");    
  }
  return ("Unknown");    
}

//***************************************************************************
const char *dynamic_inst_t::printStageAbbr( stage_t stage )
{
  switch (stage) {

  case FETCH_STAGE:
    return ("F");
  case DECODE_STAGE:
    return ("D");
  case WAIT_4TH_STAGE:
  case WAIT_3RD_STAGE:
  case WAIT_2ND_STAGE:
  case WAIT_1ST_STAGE:
  case LSQ_WAIT_STAGE:
  case EARLY_STORE_STAGE:
  case EARLY_ATOMIC_STAGE:
  case CACHE_MISS_STAGE:
  case CACHE_NOTREADY_STAGE:
  case CACHE_MISS_RETIREMENT_STAGE:
  case CACHE_NOTREADY_RETIREMENT_STAGE:
  case WB_FULL_STAGE:
  case ADDR_OVERLAP_STALL_STAGE:
  case STORESET_STALL_LOAD_STAGE:
    return ("S");
  case READY_STAGE:
    return ("Y");
  case EXECUTE_STAGE:
    return ("E");
  case COMPLETE_STAGE:
    return ("C");
  case RETIRE_STAGE:
    return ("R");
  default:
    return ("Unknown");    
  }
  return ("Unknown");    
}


/*------------------------------------------------------------------------*/
/* Next PC Methods                                                        */
/*------------------------------------------------------------------------*/

/**  nextPC: Given the current PC, nPC, these functions
 **  compute the next PC and next nPC. This modifies the state of
 **  the branch predictors, so they must be used accordingly.
 */

/// non-CTI instructions: PC = nPC, nPC = nPC + 4
//**************************************************************************
void   dynamic_inst_t::nextPC_execute( abstract_pc_t *a )
{
  // NOTE: NO prediction is allowed here. setTarget_uncond calls this
  //char buf[128];
  //s->printDisassemble(buf);
  //DEBUG_OUT("dynamic_inst_t:: nextPC_execute %s cycle[ %lld ] seqnum[ %lld ]\n", buf, m_pseq->getLocalCycle(), seq_num);

  // advance the program counter (PC = nPC)
  a->pc  = a->npc;
  a->npc = a->npc + sizeof(uint32);

  /* Our "prediction" should never be incorrect */ 
  //control_inst_t *control_p = static_cast<control_inst_t *>( this );
  //control_p->setPredictedTarget(a);
  //control_p->setPredictorState(*specBPS);

  /** Note: 11/6/01 CM
   *        No speculation is permitted in this function. see controlop.h */
}

/// taken non annulled CTI: PC = nPC, nPC = EA
//**************************************************************************
void   dynamic_inst_t::nextPC_taken( abstract_pc_t *a )
{
  predictor_state_t *specBPS = m_pseq->getSpecBPS();

  // In the future, this could be speculative, given BTB design
  la_t targetPC = (int64) a->pc + (int64) s->getOffset();
  a->pc  = a->npc;
  a->npc = targetPC;

  /* store the predicted target (a->pc after the jump) with the 
   * dynamic instruction, so we can later verify the prediction */ 
  control_inst_t *control_p = static_cast<control_inst_t *>( this );
  control_p->setPredictedTarget(a);
  control_p->setPredictorState(*specBPS);
}

/// taken annulled CTI: PC = EA, nPC = EA + 4
//**************************************************************************
void   dynamic_inst_t::nextPC_taken_a( abstract_pc_t *a )
{
  predictor_state_t *specBPS = m_pseq->getSpecBPS();

  // In the future, this could be speculative, given BTB design
  a->pc  = (int64) a->pc + (int64) s->getOffset();
  a->npc = a->pc + sizeof(uint32);

  /* store the predicted target (a->pc after the jump) with the 
   * dynamic instruction, so we can later verify the prediction */ 
  control_inst_t *control_p = static_cast<control_inst_t *>( this );
  control_p->setPredictedTarget(a);
  control_p->setPredictorState(*specBPS);
}

/// untaken annulled CTI: PC = nPC + 4, nPC = nPC + 8
//**************************************************************************
void   dynamic_inst_t::nextPC_untaken_a( abstract_pc_t *a )
{
  //  char buf[128];
  //s->printDisassemble(buf);
  //DEBUG_OUT("dynamic_inst_t:: nextPC_untaken_a %s cycle[ %lld ] seqnum[ %lld ]\n", buf, m_pseq->getLocalCycle(), seq_num);

  predictor_state_t *specBPS = m_pseq->getSpecBPS();

  // In the future, this could be speculative, given BTB design
  a->pc  = a->npc + sizeof(uint32);
  a->npc = a->npc + (2*sizeof(uint32));

  /* store the predicted target (a->pc after the jump) with the 
   * dynamic instruction, so we can later verify the prediction */ 
  control_inst_t *control_p = static_cast<control_inst_t *>( this );
  control_p->setPredictedTarget(a);
  control_p->setPredictorState(*specBPS);
}

/// untaken not annulled CTI: PC = nPC, nPC = nPC + 4
//**************************************************************************
void   dynamic_inst_t::nextPC_untaken( abstract_pc_t *a )
{
  //  char buf[128];
  //s->printDisassemble(buf);
  //DEBUG_OUT("dynamic_inst_t:: nextPC_untaken_a %s cycle[ %lld ] seqnum[ %lld ]\n", buf, m_pseq->getLocalCycle(), seq_num);

  predictor_state_t *specBPS = m_pseq->getSpecBPS();

  // advance the program counter (PC = nPC)
  a->pc  = a->npc;
  a->npc = a->npc + sizeof(uint32); 

  /* store the predicted target (a->pc after the jump) with the 
   * dynamic instruction, so we can later verify the prediction */ 
  control_inst_t *control_p = static_cast<control_inst_t *>( this );
  control_p->setPredictedTarget(a);
  control_p->setPredictorState(*specBPS);
}

/// return from trap (done, retry) instructions
//**************************************************************************
void   dynamic_inst_t::nextPC_trap_return( abstract_pc_t *a )
{
  predictor_state_t *specBPS = m_pseq->getSpecBPS();
  tlstack_t         *tlstack = m_pseq->getTLstack(m_proc);  

  // read the future tl off of the stack
  ireg_t tpc  = 0;
  ireg_t tnpc = 0;
  // NOTE: normally TL is decremented at end of retry. Our trap state array
  //       has trap level 1 at array index 0, so we decrement it here.
  
  // decrement tl
  if ( a->tl <= 0 ) {
    a->tl = 0;
  } else {
    a->tl = a->tl - 1;
  }
  tlstack->pop( tpc, tnpc, a->pstate, a->cwp, a->tl );
  a->gset = pstate_t::getGlobalSet( a->pstate );

  // Trap return speculation can cause the fetch unit to retry instructions
  // that miss in the I-TLB, before the new translation is placed in the I-TLB.
  // Now that the instruction executed by the trap handler (stxa) 
  // that updated the I-TLB stalls fetch, this isn't a problem.
  if (s->getOpcode() == i_done) {
    a->pc  = tnpc;
    a->npc = tnpc + sizeof(uint32);
  } else if (s->getOpcode() == i_retry) {
    a->pc  = tpc;
    a->npc = tnpc;
  } else {
    ERROR_OUT("dx: nextPC_trap_return: called for unknown opcode\n");
    SIM_HALT;
  }
  
  /* store the predicted target (a->pc after the jump) with the 
   * dynamic instruction, so we can later verify the prediction */ 
  control_inst_t *control_p = static_cast<control_inst_t *>( this );
  control_p->setPredictedTarget(a);
  control_p->setPredictorState(*specBPS);
}

/** CALL control transfer: functionally similar to nextPC_indirect, but
 *       * updates the RAS predictor as well.
 *  Now use indirect predictor to predict call target.
 */
//**************************************************************************
void   dynamic_inst_t::nextPC_call( abstract_pc_t *a )
{
  cascaded_indirect_t   *bpred          = m_pseq->getIndirectBP();
  predictor_state_t     *specBPS        = m_pseq->getSpecBPS();
  ras_t                 *ras            = m_pseq->getRAS(m_proc);    

  // make a branch prediction
  STAT_INC( m_pseq->m_branch_pred_stat[s->getBranchType()][2] );
  STAT_INC( m_pseq->m_branch_pred_stat[s->getBranchType()][m_priv? 1:0] );

  my_addr_t predNPC = bpred->Predict( a->pc, &specBPS->indirect_state,
                                      &m_pseq->m_branch_except_stat[s->getBranchType()], m_proc);

  /* WATTCH power */
  if(WATTCH_POWER){
    m_pseq->getPowerStats()->incrementBpredAccess();
  }

  la_t targetPC = (int64) a->pc + (int64) s->getOffset();
  a->pc = a->npc;
  if(s->getOpcode() == i_call) {
    // call instruction: get the target address directly
    a->npc = targetPC;
  } else {
    // jmpl instruction: using indirect predictor
    #ifdef DEBUG_DYNAMIC
    DEBUG_OUT("dynamic_inst_t::nextPC_call()   predicting VNPC[ 0x%llx ] cycle[ %lld ]\n", predNPC, m_pseq->getLocalCycle());
   #endif
    a->npc = predNPC;
  }

  // push the return address on the RAS
  //          + 4 is the delay slot
  //          + 8 is the return address
  if(ras_t::PRECISE_CALL) {
    // exclude call to <pc+8>
    if( (getVPC() + 8) != a->npc ) {
      ras->push( getVPC() + 8, a->npc, &specBPS->ras_state );
    }
  } else {
    ras->push( getVPC() + 8, a->npc, &specBPS->ras_state );
  }

  /* store the predicted target (a->pc after the jump) with the 
   * dynamic instruction, so we can later verify the prediction */ 
  control_inst_t *control_p = static_cast<control_inst_t *>( this );
  control_p->setPredictedTarget(a);
  control_p->setPredictorState(*specBPS);
}

/** RET  control transfer: functionally similar to nextPC_indirect, but
 *       * uses the RAS predictor as well.
 *       * taken non annulled CTI: PC = nPC, nPC = EA */
//**************************************************************************
void   dynamic_inst_t::nextPC_return( abstract_pc_t *a )
{
  predictor_state_t *specBPS = m_pseq->getSpecBPS();
  ras_t *ras = m_pseq->getRAS(m_proc);    

  // make a return address prediction
  STAT_INC( m_pseq->m_branch_pred_stat[s->getBranchType()][2] );
  STAT_INC( m_pseq->m_branch_pred_stat[s->getBranchType()][m_priv? 1:0] );

  a->pc  = a->npc;
  a->npc = ras->pop( &specBPS->ras_state );

  /* store the predicted target (a->pc after the jump) with the 
   * dynamic instruction, so we can later verify the prediction */ 
  control_inst_t *control_p = static_cast<control_inst_t *>( this );
  control_p->setPredictedTarget(a);
  control_p->setPredictorState(*specBPS);
}

//**************************************************************************
void   dynamic_inst_t::nextPC_cwp( abstract_pc_t *a )
{
  predictor_state_t *specBPS = m_pseq->getSpecBPS();

  // advance the program counter (PC = nPC)
  a->pc  = a->npc;
  a->npc = a->npc + sizeof(uint32);

  // Task: update the cwp for saves and restores

  /* 
   *  Save/Restores (1) read from sources in old windows
   *                (2) write to dest in new windows
   *  The control_inst_t constructor helps us out in this case.
   */
  if (s->getOpcode() == i_save) {
    a->cwp = (a->cwp + 1) & (NWINDOWS - 1);
  } else if (s->getOpcode() == i_restore) {
    a->cwp = (a->cwp - 1) & (NWINDOWS - 1);
  } else {
    SIM_HALT;
  }

  // Note: you could speculatively enter the trap handler here too

  /* store the predicted target (a->pc after the jump) with the 
   * dynamic instruction, so we can later verify the prediction */ 
  control_inst_t *control_p = static_cast<control_inst_t *>( this );
  control_p->setPredictedTarget(a);
  control_p->setPredictorState(*specBPS);
}

//**************************************************************************
void   dynamic_inst_t::nextPC_nop( abstract_pc_t *a )
{
  // special case handler which does nothing to pc&npc
}

/** predicted conditional: 
 **    use predicated result to determine PC, nPC result
 **    Currently predicated branches are treated as a normal, unpredicated br
 */
//**************************************************************************
void   dynamic_inst_t::nextPC_predicated_branch( abstract_pc_t *a )
{
  direct_predictor_t  *bpred   = m_pseq->getDirectBP();
  predictor_state_t   *specBPS = m_pseq->getSpecBPS();
  bool prediction;

  // make a branch prediction
  STAT_INC( m_pseq->m_branch_pred_stat[s->getBranchType()][2] );
  STAT_INC( m_pseq->m_branch_pred_stat[s->getBranchType()][m_priv? 1:0] );
 
  prediction = bpred->Predict(a->pc, specBPS->cond_state,
                              s->getFlag(SI_STATICPRED) , m_proc );

  /* WATTCH power */
  if(WATTCH_POWER){
    m_pseq->getPowerStats()->incrementBpredAccess();
  }

  if (prediction) {
    // (cond, taken, annulled) and (cond, taken, not annulled) have
    // the same behavoir
    la_t targetPC = (int64) a->pc + (int64) s->getOffset();
    a->pc  = a->npc;
    a->npc = targetPC;
  } else {
    
    if (s->getAnnul()) {
      // cond, not taken, annulled
      a->pc  = a->npc + sizeof(uint32);
      a->npc = a->npc + (2*sizeof(uint32));
    } else {
      // cond, not taken, not annulled
      a->pc  = a->npc;
      a->npc = a->npc + sizeof(uint32);
    }
  }

  /* speculatively update the branch predictor's history */
  specBPS->cond_state = ((specBPS->cond_state << 1) |
                         (prediction & 0x1));
  
  /* store the predicted target (a->pc after the branch) with the 
   * dynamic instruction, so we can later verify the prediction */ 
  control_inst_t *control_p = static_cast<control_inst_t *>( this );
  control_p->setPredictedTarget(a);
  control_p->setPredictorState(*specBPS);
}

/** unpredicted conditional: use branch prediction to determine
 **    PC, nPC result
 */
//**************************************************************************
void   dynamic_inst_t::nextPC_predict_branch( abstract_pc_t *a )
{
  // get the branch predictor from the sequencer
  direct_predictor_t  *bpred   = m_pseq->getDirectBP();
  predictor_state_t   *specBPS = m_pseq->getSpecBPS();

  // make a branch prediction
  STAT_INC( m_pseq->m_branch_pred_stat[s->getBranchType()][2] );
  STAT_INC( m_pseq->m_branch_pred_stat[s->getBranchType()][m_priv? 1:0] );
 
  bool prediction = bpred->Predict(a->pc, specBPS->cond_state,
                                   s->getFlag(SI_STATICPRED), m_proc );

  /* WATTCH power */
  if(WATTCH_POWER){
    m_pseq->getPowerStats()->incrementBpredAccess();
  }

  if (prediction) {
    // (cond, taken, annulled) and (cond, taken, not annulled) have
    // the same behavoir
    la_t targetPC = (int64) a->pc + (int64) s->getOffset();
    a->pc  = a->npc;
    a->npc = targetPC;
  } else {
    
    if (s->getAnnul()) {
      // cond, not taken, annulled
      a->pc  = a->npc + sizeof(uint32);
      a->npc = a->npc + (2*sizeof(uint32));
    } else {
      // cond, not taken, not annulled
      a->pc  = a->npc;
      a->npc = a->npc + sizeof(uint32);
    }
  }

  /* speculatively update the branch predictor's history */
  specBPS->cond_state = ((specBPS->cond_state << 1) |
                         (prediction & 0x1));
  
  /* store the predicted target (a->pc after the branch) with the 
   * dynamic instruction, so we can later verify the prediction */ 
  control_inst_t *control_p = static_cast<control_inst_t *>( this );
  control_p->setPredictedTarget(a);
  control_p->setPredictorState(*specBPS);
}

/** register indirect delayed control transfer to effective address */
//**************************************************************************
void   dynamic_inst_t::nextPC_indirect( abstract_pc_t *a )
{
  // get the branch predictor from the sequencer
  cascaded_indirect_t     *bpred     = m_pseq->getIndirectBP();
  predictor_state_t       *specBPS   = m_pseq->getSpecBPS();

  // make a branch prediction
  STAT_INC( m_pseq->m_branch_pred_stat[s->getBranchType()][2] );
  STAT_INC( m_pseq->m_branch_pred_stat[s->getBranchType()][m_priv? 1:0] );
 
  my_addr_t predNPC = bpred->Predict( a->pc, &specBPS->indirect_state,
                                      &m_pseq->m_branch_except_stat[s->getBranchType()] , m_proc);

  /* WATTCH power */
    if(WATTCH_POWER){
      m_pseq->getPowerStats()->incrementBpredAccess();
    }

  a->pc  = a->npc;
  a->npc = predNPC;

 #ifdef DEBUG_DYNAMIC
    DEBUG_OUT("dynamic_inst_t::nextPC_indirect()   predicting VNPC[ 0x%llx ] cycle[ %lld ]\n", predNPC, m_pseq->getLocalCycle());
   #endif
  
  /* store the predicted target (a->pc after the jump) with the 
   * dynamic instruction, so we can later verify the prediction */ 
  control_inst_t *control_p = static_cast<control_inst_t *>( this );
  control_p->setPredictedTarget(a);
  control_p->setPredictorState(*specBPS);
}

/// take a trap instruction (Tcc)
//**************************************************************************
void   dynamic_inst_t::nextPC_trap( abstract_pc_t *a )
{
  // get the branch predictor state from the sequencer
  // NOTE: this is not restored ever, it just enable us to call setpredtarget
  predictor_state_t *specBPS = m_pseq->getSpecBPS();

  // CM FIX should insert mechanism to speculate into traps successfully
  // However, since trap instructions are so _rare_ (almost never occurring)
  // this isn't a high priority.
#if 0
  ireg_t trapnum = getSourceReg(0).readInt64( m_pseq->getRegBox() );   
  if ( s->getFlag(SI_ISIMMEDIATE) ) {
    trapnum += s->getImmediate();
  } else {
    trapnum += getSourceReg(1).readInt64( m_pseq->getRegBox() ); 
  }
#endif

  // predict the trap is NOT taken
  a->pc  = a->npc;
  a->npc = a->npc + sizeof(uint32);

  /* store the predicted target (a->pc after the branch) with the 
   * dynamic instruction, so we can later verify the prediction */ 
  control_inst_t *control_p = static_cast<control_inst_t *>( this );
  control_p->setPredictedTarget(a);
  control_p->setPredictorState(*specBPS);
}

//**************************************************************************
void   dynamic_inst_t::nextPC_priv( abstract_pc_t *a )
{
  // get the branch predictor state from the sequencer
  // NOTE: this is not restored ever, it just enable us to call setpredtarget
  predictor_state_t *specBPS = m_pseq->getSpecBPS();

  // advance the program counter (PC = nPC)
  a->pc  = a->npc;
  a->npc = a->npc + sizeof(uint32);

  // Three types of predictions are made here :PSTATE, TL, and CWP
  ireg_t value;
  if ( getDestReg(1).getVanilla() == CONTROL_PSTATE ) {
    //   if ( s->getFlag(SI_ISIMMEDIATE) ) {
    //   value = s->getImmediate();
    //  } else {
    //   value = 0x1a;
    //  }
    value = m_pseq->getRegstatePred()->Predict( getVPC(), a, CONTROL_PSTATE);
    a->pstate = value;
    a->gset = pstate_t::getGlobalSet( a->pstate );
  } else if ( getDestReg(1).getVanilla() == CONTROL_TL ) {
    //    if ( s->getFlag(SI_ISIMMEDIATE) ) {
    //      value = s->getImmediate();
    //    } else {
    //      if (a->tl == 0) {
    //        value = 1;
    //      } else {
    //        value = a->tl - 1;
    //      }
    //}
    value = m_pseq->getRegstatePred()->Predict( getVPC(), a, CONTROL_TL);
    a->tl = value;
  } else if ( getDestReg(1).getVanilla() == CONTROL_CWP ) {
    //    if ( s->getFlag(SI_ISIMMEDIATE) ) {
    //      value = s->getImmediate();
    //    } else {
    //value = (a->cwp - 1) & (NWINDOWS - 1);
    //    }
    value = m_pseq->getRegstatePred()->Predict( getVPC(), a, CONTROL_CWP);
    a->cwp = value;
  } else {
    ERROR_OUT("dynamic_inst: nextPC_priv: unknown instruction type\n");
  }
  
  /* store the predicted target (a->pc after the branch) with the 
   * dynamic instruction, so we can later verify the prediction */ 
  control_inst_t *control_p = static_cast<control_inst_t *>( this );
  control_p->setPredictedTarget(a);
  control_p->setPredictorState(*specBPS);
}

//**************************************************************************
void dynamic_inst_t::initialize( void )
{
  for (uint32 i = 0; i < i_maxcount; i++) {
    m_jump_table[i] = &dynamic_inst_t::dx_ill;
  }

  m_jump_table[i_add] = &dynamic_inst_t::dx_add;
  m_jump_table[i_addcc] = &dynamic_inst_t::dx_addcc;
  m_jump_table[i_addc] = &dynamic_inst_t::dx_addc;
  m_jump_table[i_addccc] = &dynamic_inst_t::dx_addccc;
  m_jump_table[i_and] = &dynamic_inst_t::dx_and;
  m_jump_table[i_andcc] = &dynamic_inst_t::dx_andcc;
  m_jump_table[i_andn] = &dynamic_inst_t::dx_andn;
  m_jump_table[i_andncc] = &dynamic_inst_t::dx_andncc;
  m_jump_table[i_fba] = &dynamic_inst_t::dx_fba;
  m_jump_table[i_ba] = &dynamic_inst_t::dx_ba;
  m_jump_table[i_fbpa] = &dynamic_inst_t::dx_fbpa;
  m_jump_table[i_bpa] = &dynamic_inst_t::dx_bpa;
  m_jump_table[i_fbn] = &dynamic_inst_t::dx_fbn;
  m_jump_table[i_fbpn] = &dynamic_inst_t::dx_fbpn;
  m_jump_table[i_bn] = &dynamic_inst_t::dx_bn;
  m_jump_table[i_bpn] = &dynamic_inst_t::dx_bpn;
  m_jump_table[i_bpne] = &dynamic_inst_t::dx_bpne;
  m_jump_table[i_bpe] = &dynamic_inst_t::dx_bpe;
  m_jump_table[i_bpg] = &dynamic_inst_t::dx_bpg;
  m_jump_table[i_bple] = &dynamic_inst_t::dx_bple;
  m_jump_table[i_bpge] = &dynamic_inst_t::dx_bpge;
  m_jump_table[i_bpl] = &dynamic_inst_t::dx_bpl;
  m_jump_table[i_bpgu] = &dynamic_inst_t::dx_bpgu;
  m_jump_table[i_bpleu] = &dynamic_inst_t::dx_bpleu;
  m_jump_table[i_bpcc] = &dynamic_inst_t::dx_bpcc;
  m_jump_table[i_bpcs] = &dynamic_inst_t::dx_bpcs;
  m_jump_table[i_bppos] = &dynamic_inst_t::dx_bppos;
  m_jump_table[i_bpneg] = &dynamic_inst_t::dx_bpneg;
  m_jump_table[i_bpvc] = &dynamic_inst_t::dx_bpvc;
  m_jump_table[i_bpvs] = &dynamic_inst_t::dx_bpvs;
  m_jump_table[i_call] = &dynamic_inst_t::dx_call;
  m_jump_table[i_casa] = &dynamic_inst_t::dx_casa;
  m_jump_table[i_casxa] = &dynamic_inst_t::dx_casxa;
  m_jump_table[i_done] = &dynamic_inst_t::dx_doneretry;
  m_jump_table[i_retry] = &dynamic_inst_t::dx_doneretry;
  m_jump_table[i_jmpl] = &dynamic_inst_t::dx_jmpl;
  m_jump_table[i_fabss] = &dynamic_inst_t::dx_fabss;
  m_jump_table[i_fabsd] = &dynamic_inst_t::dx_fabsd;
  m_jump_table[i_fabsq] = &dynamic_inst_t::dx_fabsq;
  m_jump_table[i_fadds] = &dynamic_inst_t::dx_fadds;
  m_jump_table[i_faddd] = &dynamic_inst_t::dx_faddd;
  m_jump_table[i_faddq] = &dynamic_inst_t::dx_faddq;
  m_jump_table[i_fsubs] = &dynamic_inst_t::dx_fsubs;
  m_jump_table[i_fsubd] = &dynamic_inst_t::dx_fsubd;
  m_jump_table[i_fsubq] = &dynamic_inst_t::dx_fsubq;
  m_jump_table[i_fcmps] = &dynamic_inst_t::dx_fcmps;
  m_jump_table[i_fcmpd] = &dynamic_inst_t::dx_fcmpd;
  m_jump_table[i_fcmpq] = &dynamic_inst_t::dx_fcmpq;
  m_jump_table[i_fcmpes] = &dynamic_inst_t::dx_fcmpes;
  m_jump_table[i_fcmped] = &dynamic_inst_t::dx_fcmped;
  m_jump_table[i_fcmpeq] = &dynamic_inst_t::dx_fcmpeq;
  m_jump_table[i_fmovs] = &dynamic_inst_t::dx_fmovs;
  m_jump_table[i_fmovd] = &dynamic_inst_t::dx_fmovd;
  m_jump_table[i_fmovq] = &dynamic_inst_t::dx_fmovq;
  m_jump_table[i_fnegs] = &dynamic_inst_t::dx_fnegs;
  m_jump_table[i_fnegd] = &dynamic_inst_t::dx_fnegd;
  m_jump_table[i_fnegq] = &dynamic_inst_t::dx_fnegq;
  m_jump_table[i_fmuls] = &dynamic_inst_t::dx_fmuls;
  m_jump_table[i_fmuld] = &dynamic_inst_t::dx_fmuld;
  m_jump_table[i_fmulq] = &dynamic_inst_t::dx_fmulq;
  m_jump_table[i_fdivs] = &dynamic_inst_t::dx_fdivs;
  m_jump_table[i_fdivd] = &dynamic_inst_t::dx_fdivd;
  m_jump_table[i_fdivq] = &dynamic_inst_t::dx_fdivq;
  m_jump_table[i_fsmuld] = &dynamic_inst_t::dx_fsmuld;
  m_jump_table[i_fdmulq] = &dynamic_inst_t::dx_fdmulq;
  m_jump_table[i_fsqrts] = &dynamic_inst_t::dx_fsqrts;
  m_jump_table[i_fsqrtd] = &dynamic_inst_t::dx_fsqrtd;
  m_jump_table[i_fsqrtq] = &dynamic_inst_t::dx_fsqrtq;
  m_jump_table[i_retrn] = &dynamic_inst_t::dx_retrn;
  m_jump_table[i_brz] = &dynamic_inst_t::dx_brz;
  m_jump_table[i_brlez] = &dynamic_inst_t::dx_brlez;
  m_jump_table[i_brlz] = &dynamic_inst_t::dx_brlz;
  m_jump_table[i_brnz] = &dynamic_inst_t::dx_brnz;
  m_jump_table[i_brgz] = &dynamic_inst_t::dx_brgz;
  m_jump_table[i_brgez] = &dynamic_inst_t::dx_brgez;
  m_jump_table[i_fbu] = &dynamic_inst_t::dx_fbu;
  m_jump_table[i_fbg] = &dynamic_inst_t::dx_fbg;
  m_jump_table[i_fbug] = &dynamic_inst_t::dx_fbug;
  m_jump_table[i_fbl] = &dynamic_inst_t::dx_fbl;
  m_jump_table[i_fbul] = &dynamic_inst_t::dx_fbul;
  m_jump_table[i_fblg] = &dynamic_inst_t::dx_fblg;
  m_jump_table[i_fbne] = &dynamic_inst_t::dx_fbne;
  m_jump_table[i_fbe] = &dynamic_inst_t::dx_fbe;
  m_jump_table[i_fbue] = &dynamic_inst_t::dx_fbue;
  m_jump_table[i_fbge] = &dynamic_inst_t::dx_fbge;
  m_jump_table[i_fbuge] = &dynamic_inst_t::dx_fbuge;
  m_jump_table[i_fble] = &dynamic_inst_t::dx_fble;
  m_jump_table[i_fbule] = &dynamic_inst_t::dx_fbule;
  m_jump_table[i_fbo] = &dynamic_inst_t::dx_fbo;
  m_jump_table[i_fbpu] = &dynamic_inst_t::dx_fbpu;
  m_jump_table[i_fbpg] = &dynamic_inst_t::dx_fbpg;
  m_jump_table[i_fbpug] = &dynamic_inst_t::dx_fbpug;
  m_jump_table[i_fbpl] = &dynamic_inst_t::dx_fbpl;
  m_jump_table[i_fbpul] = &dynamic_inst_t::dx_fbpul;
  m_jump_table[i_fbplg] = &dynamic_inst_t::dx_fbplg;
  m_jump_table[i_fbpne] = &dynamic_inst_t::dx_fbpne;
  m_jump_table[i_fbpe] = &dynamic_inst_t::dx_fbpe;
  m_jump_table[i_fbpue] = &dynamic_inst_t::dx_fbpue;
  m_jump_table[i_fbpge] = &dynamic_inst_t::dx_fbpge;
  m_jump_table[i_fbpuge] = &dynamic_inst_t::dx_fbpuge;
  m_jump_table[i_fbple] = &dynamic_inst_t::dx_fbple;
  m_jump_table[i_fbpule] = &dynamic_inst_t::dx_fbpule;
  m_jump_table[i_fbpo] = &dynamic_inst_t::dx_fbpo;
  m_jump_table[i_bne] = &dynamic_inst_t::dx_bne;
  m_jump_table[i_be] = &dynamic_inst_t::dx_be;
  m_jump_table[i_bg] = &dynamic_inst_t::dx_bg;
  m_jump_table[i_ble] = &dynamic_inst_t::dx_ble;
  m_jump_table[i_bge] = &dynamic_inst_t::dx_bge;
  m_jump_table[i_bl] = &dynamic_inst_t::dx_bl;
  m_jump_table[i_bgu] = &dynamic_inst_t::dx_bgu;
  m_jump_table[i_bleu] = &dynamic_inst_t::dx_bleu;
  m_jump_table[i_bcc] = &dynamic_inst_t::dx_bcc;
  m_jump_table[i_bcs] = &dynamic_inst_t::dx_bcs;
  m_jump_table[i_bpos] = &dynamic_inst_t::dx_bpos;
  m_jump_table[i_bneg] = &dynamic_inst_t::dx_bneg;
  m_jump_table[i_bvc] = &dynamic_inst_t::dx_bvc;
  m_jump_table[i_bvs] = &dynamic_inst_t::dx_bvs;
  m_jump_table[i_fstox] = &dynamic_inst_t::dx_fstox;
  m_jump_table[i_fdtox] = &dynamic_inst_t::dx_fdtox;
  m_jump_table[i_fqtox] = &dynamic_inst_t::dx_fqtox;
  m_jump_table[i_fstoi] = &dynamic_inst_t::dx_fstoi;
  m_jump_table[i_fdtoi] = &dynamic_inst_t::dx_fdtoi;
  m_jump_table[i_fqtoi] = &dynamic_inst_t::dx_fqtoi;
  m_jump_table[i_fstod] = &dynamic_inst_t::dx_fstod;
  m_jump_table[i_fstoq] = &dynamic_inst_t::dx_fstoq;
  m_jump_table[i_fdtos] = &dynamic_inst_t::dx_fdtos;
  m_jump_table[i_fdtoq] = &dynamic_inst_t::dx_fdtoq;
  m_jump_table[i_fqtos] = &dynamic_inst_t::dx_fqtos;
  m_jump_table[i_fqtod] = &dynamic_inst_t::dx_fqtod;
  m_jump_table[i_fxtos] = &dynamic_inst_t::dx_fxtos;
  m_jump_table[i_fxtod] = &dynamic_inst_t::dx_fxtod;
  m_jump_table[i_fxtoq] = &dynamic_inst_t::dx_fxtoq;
  m_jump_table[i_fitos] = &dynamic_inst_t::dx_fitos;
  m_jump_table[i_fitod] = &dynamic_inst_t::dx_fitod;
  m_jump_table[i_fitoq] = &dynamic_inst_t::dx_fitoq;
  m_jump_table[i_fmovfsn] = &dynamic_inst_t::dx_fmovfsn;
  m_jump_table[i_fmovfsne] = &dynamic_inst_t::dx_fmovfsne;
  m_jump_table[i_fmovfslg] = &dynamic_inst_t::dx_fmovfslg;
  m_jump_table[i_fmovfsul] = &dynamic_inst_t::dx_fmovfsul;
  m_jump_table[i_fmovfsl] = &dynamic_inst_t::dx_fmovfsl;
  m_jump_table[i_fmovfsug] = &dynamic_inst_t::dx_fmovfsug;
  m_jump_table[i_fmovfsg] = &dynamic_inst_t::dx_fmovfsg;
  m_jump_table[i_fmovfsu] = &dynamic_inst_t::dx_fmovfsu;
  m_jump_table[i_fmovfsa] = &dynamic_inst_t::dx_fmovfsa;
  m_jump_table[i_fmovfse] = &dynamic_inst_t::dx_fmovfse;
  m_jump_table[i_fmovfsue] = &dynamic_inst_t::dx_fmovfsue;
  m_jump_table[i_fmovfsge] = &dynamic_inst_t::dx_fmovfsge;
  m_jump_table[i_fmovfsuge] = &dynamic_inst_t::dx_fmovfsuge;
  m_jump_table[i_fmovfsle] = &dynamic_inst_t::dx_fmovfsle;
  m_jump_table[i_fmovfsule] = &dynamic_inst_t::dx_fmovfsule;
  m_jump_table[i_fmovfso] = &dynamic_inst_t::dx_fmovfso;
  m_jump_table[i_fmovfdn] = &dynamic_inst_t::dx_fmovfdn;
  m_jump_table[i_fmovfdne] = &dynamic_inst_t::dx_fmovfdne;
  m_jump_table[i_fmovfdlg] = &dynamic_inst_t::dx_fmovfdlg;
  m_jump_table[i_fmovfdul] = &dynamic_inst_t::dx_fmovfdul;
  m_jump_table[i_fmovfdl] = &dynamic_inst_t::dx_fmovfdl;
  m_jump_table[i_fmovfdug] = &dynamic_inst_t::dx_fmovfdug;
  m_jump_table[i_fmovfdg] = &dynamic_inst_t::dx_fmovfdg;
  m_jump_table[i_fmovfdu] = &dynamic_inst_t::dx_fmovfdu;
  m_jump_table[i_fmovfda] = &dynamic_inst_t::dx_fmovfda;
  m_jump_table[i_fmovfde] = &dynamic_inst_t::dx_fmovfde;
  m_jump_table[i_fmovfdue] = &dynamic_inst_t::dx_fmovfdue;
  m_jump_table[i_fmovfdge] = &dynamic_inst_t::dx_fmovfdge;
  m_jump_table[i_fmovfduge] = &dynamic_inst_t::dx_fmovfduge;
  m_jump_table[i_fmovfdle] = &dynamic_inst_t::dx_fmovfdle;
  m_jump_table[i_fmovfdule] = &dynamic_inst_t::dx_fmovfdule;
  m_jump_table[i_fmovfdo] = &dynamic_inst_t::dx_fmovfdo;
  m_jump_table[i_fmovfqn] = &dynamic_inst_t::dx_fmovfqn;
  m_jump_table[i_fmovfqne] = &dynamic_inst_t::dx_fmovfqne;
  m_jump_table[i_fmovfqlg] = &dynamic_inst_t::dx_fmovfqlg;
  m_jump_table[i_fmovfqul] = &dynamic_inst_t::dx_fmovfqul;
  m_jump_table[i_fmovfql] = &dynamic_inst_t::dx_fmovfql;
  m_jump_table[i_fmovfqug] = &dynamic_inst_t::dx_fmovfqug;
  m_jump_table[i_fmovfqg] = &dynamic_inst_t::dx_fmovfqg;
  m_jump_table[i_fmovfqu] = &dynamic_inst_t::dx_fmovfqu;
  m_jump_table[i_fmovfqa] = &dynamic_inst_t::dx_fmovfqa;
  m_jump_table[i_fmovfqe] = &dynamic_inst_t::dx_fmovfqe;
  m_jump_table[i_fmovfque] = &dynamic_inst_t::dx_fmovfque;
  m_jump_table[i_fmovfqge] = &dynamic_inst_t::dx_fmovfqge;
  m_jump_table[i_fmovfquge] = &dynamic_inst_t::dx_fmovfquge;
  m_jump_table[i_fmovfqle] = &dynamic_inst_t::dx_fmovfqle;
  m_jump_table[i_fmovfqule] = &dynamic_inst_t::dx_fmovfqule;
  m_jump_table[i_fmovfqo] = &dynamic_inst_t::dx_fmovfqo;
  m_jump_table[i_fmovsn] = &dynamic_inst_t::dx_fmovsn;
  m_jump_table[i_fmovse] = &dynamic_inst_t::dx_fmovse;
  m_jump_table[i_fmovsle] = &dynamic_inst_t::dx_fmovsle;
  m_jump_table[i_fmovsl] = &dynamic_inst_t::dx_fmovsl;
  m_jump_table[i_fmovsleu] = &dynamic_inst_t::dx_fmovsleu;
  m_jump_table[i_fmovscs] = &dynamic_inst_t::dx_fmovscs;
  m_jump_table[i_fmovsneg] = &dynamic_inst_t::dx_fmovsneg;
  m_jump_table[i_fmovsvs] = &dynamic_inst_t::dx_fmovsvs;
  m_jump_table[i_fmovsa] = &dynamic_inst_t::dx_fmovsa;
  m_jump_table[i_fmovsne] = &dynamic_inst_t::dx_fmovsne;
  m_jump_table[i_fmovsg] = &dynamic_inst_t::dx_fmovsg;
  m_jump_table[i_fmovsge] = &dynamic_inst_t::dx_fmovsge;
  m_jump_table[i_fmovsgu] = &dynamic_inst_t::dx_fmovsgu;
  m_jump_table[i_fmovscc] = &dynamic_inst_t::dx_fmovscc;
  m_jump_table[i_fmovspos] = &dynamic_inst_t::dx_fmovspos;
  m_jump_table[i_fmovsvc] = &dynamic_inst_t::dx_fmovsvc;
  m_jump_table[i_fmovdn] = &dynamic_inst_t::dx_fmovdn;
  m_jump_table[i_fmovde] = &dynamic_inst_t::dx_fmovde;
  m_jump_table[i_fmovdle] = &dynamic_inst_t::dx_fmovdle;
  m_jump_table[i_fmovdl] = &dynamic_inst_t::dx_fmovdl;
  m_jump_table[i_fmovdleu] = &dynamic_inst_t::dx_fmovdleu;
  m_jump_table[i_fmovdcs] = &dynamic_inst_t::dx_fmovdcs;
  m_jump_table[i_fmovdneg] = &dynamic_inst_t::dx_fmovdneg;
  m_jump_table[i_fmovdvs] = &dynamic_inst_t::dx_fmovdvs;
  m_jump_table[i_fmovda] = &dynamic_inst_t::dx_fmovda;
  m_jump_table[i_fmovdne] = &dynamic_inst_t::dx_fmovdne;
  m_jump_table[i_fmovdg] = &dynamic_inst_t::dx_fmovdg;
  m_jump_table[i_fmovdge] = &dynamic_inst_t::dx_fmovdge;
  m_jump_table[i_fmovdgu] = &dynamic_inst_t::dx_fmovdgu;
  m_jump_table[i_fmovdcc] = &dynamic_inst_t::dx_fmovdcc;
  m_jump_table[i_fmovdpos] = &dynamic_inst_t::dx_fmovdpos;
  m_jump_table[i_fmovdvc] = &dynamic_inst_t::dx_fmovdvc;
  m_jump_table[i_fmovqn] = &dynamic_inst_t::dx_fmovqn;
  m_jump_table[i_fmovqe] = &dynamic_inst_t::dx_fmovqe;
  m_jump_table[i_fmovqle] = &dynamic_inst_t::dx_fmovqle;
  m_jump_table[i_fmovql] = &dynamic_inst_t::dx_fmovql;
  m_jump_table[i_fmovqleu] = &dynamic_inst_t::dx_fmovqleu;
  m_jump_table[i_fmovqcs] = &dynamic_inst_t::dx_fmovqcs;
  m_jump_table[i_fmovqneg] = &dynamic_inst_t::dx_fmovqneg;
  m_jump_table[i_fmovqvs] = &dynamic_inst_t::dx_fmovqvs;
  m_jump_table[i_fmovqa] = &dynamic_inst_t::dx_fmovqa;
  m_jump_table[i_fmovqne] = &dynamic_inst_t::dx_fmovqne;
  m_jump_table[i_fmovqg] = &dynamic_inst_t::dx_fmovqg;
  m_jump_table[i_fmovqge] = &dynamic_inst_t::dx_fmovqge;
  m_jump_table[i_fmovqgu] = &dynamic_inst_t::dx_fmovqgu;
  m_jump_table[i_fmovqcc] = &dynamic_inst_t::dx_fmovqcc;
  m_jump_table[i_fmovqpos] = &dynamic_inst_t::dx_fmovqpos;
  m_jump_table[i_fmovqvc] = &dynamic_inst_t::dx_fmovqvc;
  m_jump_table[i_fmovrsz] = &dynamic_inst_t::dx_fmovrsz;
  m_jump_table[i_fmovrslez] = &dynamic_inst_t::dx_fmovrslez;
  m_jump_table[i_fmovrslz] = &dynamic_inst_t::dx_fmovrslz;
  m_jump_table[i_fmovrsnz] = &dynamic_inst_t::dx_fmovrsnz;
  m_jump_table[i_fmovrsgz] = &dynamic_inst_t::dx_fmovrsgz;
  m_jump_table[i_fmovrsgez] = &dynamic_inst_t::dx_fmovrsgez;
  m_jump_table[i_fmovrdz] = &dynamic_inst_t::dx_fmovrdz;
  m_jump_table[i_fmovrdlez] = &dynamic_inst_t::dx_fmovrdlez;
  m_jump_table[i_fmovrdlz] = &dynamic_inst_t::dx_fmovrdlz;
  m_jump_table[i_fmovrdnz] = &dynamic_inst_t::dx_fmovrdnz;
  m_jump_table[i_fmovrdgz] = &dynamic_inst_t::dx_fmovrdgz;
  m_jump_table[i_fmovrdgez] = &dynamic_inst_t::dx_fmovrdgez;
  m_jump_table[i_fmovrqz] = &dynamic_inst_t::dx_fmovrqz;
  m_jump_table[i_fmovrqlez] = &dynamic_inst_t::dx_fmovrqlez;
  m_jump_table[i_fmovrqlz] = &dynamic_inst_t::dx_fmovrqlz;
  m_jump_table[i_fmovrqnz] = &dynamic_inst_t::dx_fmovrqnz;
  m_jump_table[i_fmovrqgz] = &dynamic_inst_t::dx_fmovrqgz;
  m_jump_table[i_fmovrqgez] = &dynamic_inst_t::dx_fmovrqgez;
  m_jump_table[i_mova] = &dynamic_inst_t::dx_mova;
  m_jump_table[i_movfa] = &dynamic_inst_t::dx_movfa;
  m_jump_table[i_movn] = &dynamic_inst_t::dx_movn;
  m_jump_table[i_movfn] = &dynamic_inst_t::dx_movfn;
  m_jump_table[i_movne] = &dynamic_inst_t::dx_movne;
  m_jump_table[i_move] = &dynamic_inst_t::dx_move;
  m_jump_table[i_movg] = &dynamic_inst_t::dx_movg;
  m_jump_table[i_movle] = &dynamic_inst_t::dx_movle;
  m_jump_table[i_movge] = &dynamic_inst_t::dx_movge;
  m_jump_table[i_movl] = &dynamic_inst_t::dx_movl;
  m_jump_table[i_movgu] = &dynamic_inst_t::dx_movgu;
  m_jump_table[i_movleu] = &dynamic_inst_t::dx_movleu;
  m_jump_table[i_movcc] = &dynamic_inst_t::dx_movcc;
  m_jump_table[i_movcs] = &dynamic_inst_t::dx_movcs;
  m_jump_table[i_movpos] = &dynamic_inst_t::dx_movpos;
  m_jump_table[i_movneg] = &dynamic_inst_t::dx_movneg;
  m_jump_table[i_movvc] = &dynamic_inst_t::dx_movvc;
  m_jump_table[i_movvs] = &dynamic_inst_t::dx_movvs;
  m_jump_table[i_movfu] = &dynamic_inst_t::dx_movfu;
  m_jump_table[i_movfg] = &dynamic_inst_t::dx_movfg;
  m_jump_table[i_movfug] = &dynamic_inst_t::dx_movfug;
  m_jump_table[i_movfl] = &dynamic_inst_t::dx_movfl;
  m_jump_table[i_movful] = &dynamic_inst_t::dx_movful;
  m_jump_table[i_movflg] = &dynamic_inst_t::dx_movflg;
  m_jump_table[i_movfne] = &dynamic_inst_t::dx_movfne;
  m_jump_table[i_movfe] = &dynamic_inst_t::dx_movfe;
  m_jump_table[i_movfue] = &dynamic_inst_t::dx_movfue;
  m_jump_table[i_movfge] = &dynamic_inst_t::dx_movfge;
  m_jump_table[i_movfuge] = &dynamic_inst_t::dx_movfuge;
  m_jump_table[i_movfle] = &dynamic_inst_t::dx_movfle;
  m_jump_table[i_movfule] = &dynamic_inst_t::dx_movfule;
  m_jump_table[i_movfo] = &dynamic_inst_t::dx_movfo;
  m_jump_table[i_movrz] = &dynamic_inst_t::dx_movrz;
  m_jump_table[i_movrlez] = &dynamic_inst_t::dx_movrlez;
  m_jump_table[i_movrlz] = &dynamic_inst_t::dx_movrlz;
  m_jump_table[i_movrnz] = &dynamic_inst_t::dx_movrnz;
  m_jump_table[i_movrgz] = &dynamic_inst_t::dx_movrgz;
  m_jump_table[i_movrgez] = &dynamic_inst_t::dx_movrgez;
  m_jump_table[i_ta] = &dynamic_inst_t::dx_ta;
  m_jump_table[i_tn] = &dynamic_inst_t::dx_tn;
  m_jump_table[i_tne] = &dynamic_inst_t::dx_tne;
  m_jump_table[i_te] = &dynamic_inst_t::dx_te;
  m_jump_table[i_tg] = &dynamic_inst_t::dx_tg;
  m_jump_table[i_tle] = &dynamic_inst_t::dx_tle;
  m_jump_table[i_tge] = &dynamic_inst_t::dx_tge;
  m_jump_table[i_tl] = &dynamic_inst_t::dx_tl;
  m_jump_table[i_tgu] = &dynamic_inst_t::dx_tgu;
  m_jump_table[i_tleu] = &dynamic_inst_t::dx_tleu;
  m_jump_table[i_tcc] = &dynamic_inst_t::dx_tcc;
  m_jump_table[i_tcs] = &dynamic_inst_t::dx_tcs;
  m_jump_table[i_tpos] = &dynamic_inst_t::dx_tpos;
  m_jump_table[i_tneg] = &dynamic_inst_t::dx_tneg;
  m_jump_table[i_tvc] = &dynamic_inst_t::dx_tvc;
  m_jump_table[i_tvs] = &dynamic_inst_t::dx_tvs;
  m_jump_table[i_sub] = &dynamic_inst_t::dx_sub;
  m_jump_table[i_subcc] = &dynamic_inst_t::dx_subcc;
  m_jump_table[i_subc] = &dynamic_inst_t::dx_subc;
  m_jump_table[i_subccc] = &dynamic_inst_t::dx_subccc;
  m_jump_table[i_or] = &dynamic_inst_t::dx_or;
  m_jump_table[i_orcc] = &dynamic_inst_t::dx_orcc;
  m_jump_table[i_orn] = &dynamic_inst_t::dx_orn;
  m_jump_table[i_orncc] = &dynamic_inst_t::dx_orncc;
  m_jump_table[i_xor] = &dynamic_inst_t::dx_xor;
  m_jump_table[i_xorcc] = &dynamic_inst_t::dx_xorcc;
  m_jump_table[i_xnor] = &dynamic_inst_t::dx_xnor;
  m_jump_table[i_xnorcc] = &dynamic_inst_t::dx_xnorcc;
  m_jump_table[i_mulx] = &dynamic_inst_t::dx_mulx;
  m_jump_table[i_sdivx] = &dynamic_inst_t::dx_sdivx;
  m_jump_table[i_udivx] = &dynamic_inst_t::dx_udivx;
  m_jump_table[i_sll] = &dynamic_inst_t::dx_sll;
  m_jump_table[i_srl] = &dynamic_inst_t::dx_srl;
  m_jump_table[i_sra] = &dynamic_inst_t::dx_sra;
  m_jump_table[i_sllx] = &dynamic_inst_t::dx_sllx;
  m_jump_table[i_srlx] = &dynamic_inst_t::dx_srlx;
  m_jump_table[i_srax] = &dynamic_inst_t::dx_srax;
  m_jump_table[i_taddcc] = &dynamic_inst_t::dx_taddcc;
  m_jump_table[i_taddcctv] = &dynamic_inst_t::dx_taddcctv;
  m_jump_table[i_tsubcc] = &dynamic_inst_t::dx_tsubcc;
  m_jump_table[i_tsubcctv] = &dynamic_inst_t::dx_tsubcctv;
  m_jump_table[i_udiv] = &dynamic_inst_t::dx_udiv;
  m_jump_table[i_sdiv] = &dynamic_inst_t::dx_sdiv;
  m_jump_table[i_umul] = &dynamic_inst_t::dx_umul;
  m_jump_table[i_smul] = &dynamic_inst_t::dx_smul;
  m_jump_table[i_udivcc] = &dynamic_inst_t::dx_udivcc;
  m_jump_table[i_sdivcc] = &dynamic_inst_t::dx_sdivcc;
  m_jump_table[i_umulcc] = &dynamic_inst_t::dx_umulcc;
  m_jump_table[i_smulcc] = &dynamic_inst_t::dx_smulcc;
  m_jump_table[i_mulscc] = &dynamic_inst_t::dx_mulscc;
  m_jump_table[i_popc] = &dynamic_inst_t::dx_popc;
  m_jump_table[i_flush] = &dynamic_inst_t::dx_flush;
  m_jump_table[i_flushw] = &dynamic_inst_t::dx_flushw;
  m_jump_table[i_rd] = &dynamic_inst_t::dx_rd;
  m_jump_table[i_rdcc] = &dynamic_inst_t::dx_rdcc;
  m_jump_table[i_wr] = &dynamic_inst_t::dx_wr;
  m_jump_table[i_wrcc] = &dynamic_inst_t::dx_wrcc;
  m_jump_table[i_save] = &dynamic_inst_t::dx_save;
  m_jump_table[i_restore] = &dynamic_inst_t::dx_restore;
  m_jump_table[i_saved] = &dynamic_inst_t::dx_saved;
  m_jump_table[i_restored] = &dynamic_inst_t::dx_restored;
  m_jump_table[i_sethi] = &dynamic_inst_t::dx_sethi;
  m_jump_table[i_ldf] = &dynamic_inst_t::dx_ldf;
  m_jump_table[i_lddf] = &dynamic_inst_t::dx_lddf;
  m_jump_table[i_ldqf] = &dynamic_inst_t::dx_ldqf;
  m_jump_table[i_ldfa] = &dynamic_inst_t::dx_ldfa;
  m_jump_table[i_lddfa] = &dynamic_inst_t::dx_lddfa;
  m_jump_table[i_ldblk] = &dynamic_inst_t::dx_ldblk;
  m_jump_table[i_ldqfa] = &dynamic_inst_t::dx_ldqfa;
  m_jump_table[i_ldsb] = &dynamic_inst_t::dx_ldsb;
  m_jump_table[i_ldsh] = &dynamic_inst_t::dx_ldsh;
  m_jump_table[i_ldsw] = &dynamic_inst_t::dx_ldsw;
  m_jump_table[i_ldub] = &dynamic_inst_t::dx_ldub;
  m_jump_table[i_lduh] = &dynamic_inst_t::dx_lduh;
  m_jump_table[i_lduw] = &dynamic_inst_t::dx_lduw;
  m_jump_table[i_ldx] = &dynamic_inst_t::dx_ldx;
  m_jump_table[i_ldd] = &dynamic_inst_t::dx_ldd;
  m_jump_table[i_ldsba] = &dynamic_inst_t::dx_ldsba;
  m_jump_table[i_ldsha] = &dynamic_inst_t::dx_ldsha;
  m_jump_table[i_ldswa] = &dynamic_inst_t::dx_ldswa;
  m_jump_table[i_lduba] = &dynamic_inst_t::dx_lduba;
  m_jump_table[i_lduha] = &dynamic_inst_t::dx_lduha;
  m_jump_table[i_lduwa] = &dynamic_inst_t::dx_lduwa;
  m_jump_table[i_ldxa] = &dynamic_inst_t::dx_ldxa;
  m_jump_table[i_ldda] = &dynamic_inst_t::dx_ldda;
  m_jump_table[i_ldqa] = &dynamic_inst_t::dx_ldqa;
  m_jump_table[i_stf] = &dynamic_inst_t::dx_stf;
  m_jump_table[i_stdf] = &dynamic_inst_t::dx_stdf;
  m_jump_table[i_stqf] = &dynamic_inst_t::dx_stqf;
  m_jump_table[i_stb] = &dynamic_inst_t::dx_stb;
  m_jump_table[i_sth] = &dynamic_inst_t::dx_sth;
  m_jump_table[i_stw] = &dynamic_inst_t::dx_stw;
  m_jump_table[i_stx] = &dynamic_inst_t::dx_stx;
  m_jump_table[i_std] = &dynamic_inst_t::dx_std;
  m_jump_table[i_stfa] = &dynamic_inst_t::dx_stfa;
  m_jump_table[i_stdfa] = &dynamic_inst_t::dx_stdfa;
  m_jump_table[i_stblk] = &dynamic_inst_t::dx_stblk;
  m_jump_table[i_stqfa] = &dynamic_inst_t::dx_stqfa;
  m_jump_table[i_stba] = &dynamic_inst_t::dx_stba;
  m_jump_table[i_stha] = &dynamic_inst_t::dx_stha;
  m_jump_table[i_stwa] = &dynamic_inst_t::dx_stwa;
  m_jump_table[i_stxa] = &dynamic_inst_t::dx_stxa;
  m_jump_table[i_stda] = &dynamic_inst_t::dx_stda;
  m_jump_table[i_ldstub] = &dynamic_inst_t::dx_ldstub;
  m_jump_table[i_ldstuba] = &dynamic_inst_t::dx_ldstuba;
  m_jump_table[i_prefetch] = &dynamic_inst_t::dx_prefetch;
  m_jump_table[i_prefetcha] = &dynamic_inst_t::dx_prefetcha;
  m_jump_table[i_swap] = &dynamic_inst_t::dx_swap;
  m_jump_table[i_swapa] = &dynamic_inst_t::dx_swapa;
  m_jump_table[i_ldfsr] = &dynamic_inst_t::dx_ldfsr;
  m_jump_table[i_ldxfsr] = &dynamic_inst_t::dx_ldxfsr;
  m_jump_table[i_stfsr] = &dynamic_inst_t::dx_stfsr;
  m_jump_table[i_stxfsr] = &dynamic_inst_t::dx_stxfsr;
  m_jump_table[i__trap] = &dynamic_inst_t::dx__trap;
  m_jump_table[i_impdep1] = &dynamic_inst_t::dx_impdep1;
  m_jump_table[i_impdep2] = &dynamic_inst_t::dx_impdep2;
  m_jump_table[i_membar] = &dynamic_inst_t::dx_membar;
  m_jump_table[i_stbar] = &dynamic_inst_t::dx_stbar;
  m_jump_table[i_cmp] = &dynamic_inst_t::dx_cmp;
  m_jump_table[i_jmp] = &dynamic_inst_t::dx_jmp;
  m_jump_table[i_mov] = &dynamic_inst_t::dx_mov;
  m_jump_table[i_nop] = &dynamic_inst_t::dx_nop;
  m_jump_table[i_not] = &dynamic_inst_t::dx_not;
  m_jump_table[i_rdpr] = &dynamic_inst_t::dx_rdpr;
  m_jump_table[i_wrpr] = &dynamic_inst_t::dx_wrpr;
  m_jump_table[i_faligndata] = &dynamic_inst_t::dx_faligndata;
  m_jump_table[i_alignaddr] = &dynamic_inst_t::dx_alignaddr;
  m_jump_table[i_alignaddrl] = &dynamic_inst_t::dx_alignaddrl;
  m_jump_table[i_fzero] = &dynamic_inst_t::dx_fzero;
  m_jump_table[i_fzeros] = &dynamic_inst_t::dx_fzeros;
  m_jump_table[i_fsrc1] = &dynamic_inst_t::dx_fsrc1;
  m_jump_table[i_fcmple16] = &dynamic_inst_t::dx_fcmple16;
  m_jump_table[i_fcmpne16] = &dynamic_inst_t::dx_fcmpne16;
  m_jump_table[i_fcmple32] = &dynamic_inst_t::dx_fcmple32;
  m_jump_table[i_fcmpne32] = &dynamic_inst_t::dx_fcmpne32;
  m_jump_table[i_fcmpgt16] = &dynamic_inst_t::dx_fcmpgt16;
  m_jump_table[i_fcmpeq16] = &dynamic_inst_t::dx_fcmpeq16;
  m_jump_table[i_fcmpgt32] = &dynamic_inst_t::dx_fcmpgt32;
  m_jump_table[i_fcmpeq32] = &dynamic_inst_t::dx_fcmpeq32;
  m_jump_table[i_bshuffle] = &dynamic_inst_t::dx_bshuffle;
  m_jump_table[i_bmask] = &dynamic_inst_t::dx_bmask;
  m_jump_table[i_ill] = &dynamic_inst_t::dx_ill;
}
