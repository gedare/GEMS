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
/*------------------------------------------------------------------------*/
/* Includes                                                                             */
/*------------------------------------------------------------------------*/

#include "hfa.h"
#include "hfacore.h"
#include "pipestate.h"
#include "pipepool.h"
#include "rubycache.h"
#include "writebuffer.h"

//*****************************************************************************************
writebuffer_t::writebuffer_t(uint32 id, uint32 block_bits, scheduler_t * eventQ){
  ASSERT(WRITE_BUFFER_SIZE >= 0);
  m_id = id/CONFIG_LOGICAL_PER_PHY_PROC;
  m_block_size = 1 << block_bits;
  m_block_mask = ~(m_block_size - 1);
  m_buffer_size = 0;
  m_outstanding_stores = 0;
  m_use_write_buffer = false;
  m_event_queue = eventQ;
  m_is_scheduled = false;
  m_write_buffer_full = false;
  if(WRITE_BUFFER_SIZE > 0){
    m_request_pool = new pipepool_t();
    m_use_write_buffer = true;
  }
  else{
    m_request_pool = NULL;
  }

  #ifdef DEBUG_WRITE_BUFFER
      DEBUG_OUT("*******write_buffer_t::Using Write Buffer? %d\n",m_use_write_buffer);
  #endif
}

//******************************************************************************************
writebuffer_t::~writebuffer_t(){
  if(m_request_pool){
    delete m_request_pool;
    m_request_pool = NULL;
  }
}

//******************************************************************************************
writebuffer_status_t writebuffer_t::addToWriteBuffer(waiter_t * store, la_t logical_address, pa_t physical_address, la_t vpc, bool priv, uint64 posttime, int thread){
  // Be careful - since we can have LDs/STs to the same address issued to the MSHR and WB at any
  //                   time, before we insert we HAVE to check that it isn't already queued in the MSHR.
  //                   Note that all LDs must check the WB first, and will be queued in the WB wait list for
  //                   the matching ST.
  if(system_t::inst->m_seq[m_id]->getRubyCache()->checkOutstandingRequests(physical_address)){
    //if there's an outstanding LD in the MSHR, let the ST know this (so it can enqueue itself in MSHR instead)
    return WB_MATCHING_MSHR_ENTRY;
  }

  writebuffer_status_t status = WB_OK;
  if(m_use_write_buffer){
      #ifdef DEBUG_WRITE_BUFFER
         DEBUG_OUT("\n***WriteBuffer: addToWriteBuffer BEGIN, contents:\n");
         print();
         DEBUG_OUT("\n");
      #endif
    //first, check if we already have a store to this cache line:
    pa_t            lineaddr = physical_address & m_block_mask;
    
    //Note: TSO does not allow coalescing writes to the same cache line!  Therefore we always allocate
    //        a new write buffer entry for each incoming request
    //see if we have room to insert it into our write buffer:
    
  // We will enqueue an entry in the WB under 2 conditions:
  //   1) Ruby was ready and we didn't have a fastpath hit
  //   2) Ruby was NOT ready 
  bool wb_full = false;
  if(m_buffer_size >= WRITE_BUFFER_SIZE){
    wb_full = true;
    if(!m_write_buffer_full){
      //Stall the front-end:
      system_t::inst->m_seq[m_id]->WBStallFetch();
      //indicate we have stalled:
      m_write_buffer_full = true;
    }
  }
  ruby_request_t * new_store_request = NULL;
  if(!wb_full){
    //insert a store request with the store waiter, since we can retire the store immediately and
    //  can view this as a "cache HIT".  Note that we don't associate this store waiter with any request!
    dummy_waiter_t * dummy_store = new dummy_waiter_t();
    new_store_request =  new ruby_request_t(dummy_store, logical_address, lineaddr, OPAL_STORE, vpc, priv, posttime, thread);
    #ifdef DEBUG_WRITE_BUFFER
        DEBUG_OUT("\t INSERTING new request, wait_list = 0x%x\n",&new_store_request->m_wait_list);
    #endif
      m_request_pool->insertOrdered( new_store_request  );
      //insert this store into the miss' waiting list:
      ASSERT(dummy_store->Disconnected());
#ifdef DEBUG_WRITE_BUFFER
      DEBUG_OUT("\t***addToWriteBuffer, wait list contents BEFORE add: \n");
      new_store_request->m_wait_list.print(cout);
      DEBUG_OUT("\n");
#endif
      dummy_store->InsertWaitQueue( new_store_request->m_wait_list );
#ifdef DEBUG_WRITE_BUFFER
      DEBUG_OUT("\t***addToWriteBuffer, wait list contents AFTER add: \n");
      new_store_request->m_wait_list.print(cout);
      DEBUG_OUT("\n");
#endif
      m_buffer_size++;
      status = WB_OK;            //instrs view this as a "cache HIT", can retire immediately
  }
  else{
    ASSERT(m_buffer_size == m_request_pool->getCount());
    status = WB_FULL;         //insts see this WB_FULL and proceed to poll this function until
                                       // a space is freed up
  }

   #ifdef DEBUG_WRITE_BUFFER
     DEBUG_OUT("***WriteBuffer: addToWriteBuffer END, contents:\n");
     print();
     DEBUG_OUT("\n");
   #endif
  }  //end if(m_use_write_buffer)

  return status;
}

//******************************************************************************************
void writebuffer_t::flushWriteBuffer(){
  if(m_use_write_buffer){
     #ifdef DEBUG_WRITE_BUFFER
         DEBUG_OUT("\n***WriteBuffer: flushWriteBuffer BEGIN, contents:\n");
         print();
         DEBUG_OUT("\n");
     #endif

         if(m_buffer_size > 0){
           mf_ruby_api_t *ruby_api = system_t::inst->m_ruby_api;
           // issue a miss to ruby
           if ( ruby_api == NULL || ruby_api->makeRequest == NULL ) {
             ERROR_OUT("error: ruby api is called but ruby is not installed.\n");
             SYSTEM_EXIT;
           }
           
           //issue as many writes as Ruby allows...there are 2 cases why Ruby is not ready:
           //      1) There is an outstanding miss to the same addr - either from WB or MSHR
           //      2) Ruby has too many outstanding requests
           
           ruby_request_t *next_ruby_request = static_cast<ruby_request_t*>(m_request_pool->walkList(NULL));
           while (next_ruby_request != NULL) {
             // if ruby does not accept requests, immediately return (since we issue writes from the buffer in 
             //    FIFO order)
             //  Note: Opal currently executes Stores OOO - which violates TSO!
             //Only issue requests that are already non-outstanding to Ruby:
             if(!next_ruby_request->m_is_outstanding){
               if ((*ruby_api->isReady)( m_id, next_ruby_request->m_logical_address, next_ruby_request->getAddress(), next_ruby_request->m_request_type, next_ruby_request->m_thread)) {
                 
                 //ORDER IS IMPORTANT - we MUST update m_oustanding_stores before makeRequest() bc we could
                 //   have a fastpath HIT, which calls complete() to decrement m_outstanding_stores
                 m_outstanding_stores++;
                 //mark this store as being serviced by Ruby
                 next_ruby_request->m_is_outstanding = true;

                 /* WATTCH power */
                 if(WATTCH_POWER){
                   system_t::inst->m_seq[m_id]->getPowerStats()->incrementDCacheAccess();
                 }

                 (*ruby_api->makeRequest)( m_id, next_ruby_request->m_logical_address, next_ruby_request->getAddress(), m_block_size,
                                           next_ruby_request->m_request_type, next_ruby_request->m_vpc, next_ruby_request->m_priv, next_ruby_request->m_thread);
               
                 
#ifdef DEBUG_WRITE_BUFFER
                 DEBUG_OUT("\tissuing request 0x%x to Ruby...total outstanding = %d\n",next_ruby_request->getAddress(), m_outstanding_stores);
#endif
              
               }
               else{
                 // Ruby is not ready, so we immediately return
#ifdef DEBUG_WRITE_BUFFER
                 DEBUG_OUT("\tRuby is NOT READY for request 0x%x\n", next_ruby_request->getAddress());
#endif
                 break;
               }
             }  //end !m_is_outstanding
             // get next write buffer entry
             next_ruby_request = static_cast<ruby_request_t*>(m_request_pool->walkList( next_ruby_request ));
           }
         }  // end buffer_size > 0

     #ifdef DEBUG_WRITE_BUFFER
         DEBUG_OUT("***WriteBuffer: flushWriteBuffer END, contents:\n");
         print();
         DEBUG_OUT("\n");
   #endif
  } // end if(m_use_write_buffer)
}

//****************************************************************************************
void writebuffer_t::complete(pa_t physical_address, bool abortRequest, OpalMemop_t type, int thread ){
  if(m_use_write_buffer){

         #ifdef DEBUG_WRITE_BUFFER
           DEBUG_OUT("\n***WriteBuffer: complete BEGIN, contents:\n");
           print();
           DEBUG_OUT("\n");
         #endif
    bool        found = false;
    pa_t        lineaddr = physical_address & m_block_mask;

    //Note fastpath hits are handled like regular requests - they must remove the WB entry!
    if ( lineaddr != physical_address ) {
      ERROR_OUT("error: writebuffer: ruby returns pa 0x%0llx which is not a cache line: 0x%0llx\n", physical_address, lineaddr );
    }
    
    // search the linked list for the address
    ruby_request_t *miss = static_cast<ruby_request_t*>(m_request_pool->walkList(NULL));
    while (miss != NULL) {
      // check that request types match as well as thread ID
      if ( miss->match( lineaddr ) && (type == miss->m_request_type) && (thread == miss->m_thread)) {
        found = true;
        break;
      }
      miss = static_cast<ruby_request_t*>(m_request_pool->walkList( miss ));
    }
    
    if (found) {
      // first remove the rubymiss from the linked list
      bool found = m_request_pool->removeElement( miss );
      ASSERT( found == true );
      
      // decrement the number of WB entries
      m_buffer_size--;
      m_outstanding_stores--;
      ASSERT(m_buffer_size >= 0);
      ASSERT(m_outstanding_stores >= 0);

      //if we stalled the front-end, unstall it now (since we now have a free slot in the write buffer):
      if(m_write_buffer_full){
        m_write_buffer_full = false;
        system_t::inst->m_seq[m_id]->WBUnstallFetch();
      }

      //Wakeup all those loads that was waiting for this store to be serviced (put there by checkLoadHit)
      miss->m_wait_list.WakeupChain();
      
#ifdef DEBUG_WRITE_BUFFER
      DEBUG_OUT( "[%d] COMPLETE 0x%0llx 0x%0llx (count:%d)\n",
               m_id, lineaddr, miss->m_physical_address,
               m_buffer_size);
#endif

      //free up our dummy_store waiter 
      delete miss->m_waiter;      
      miss->m_waiter = NULL;
      delete miss;
      miss = NULL;
    } else {
      // if not found, report a warning
      ERROR_OUT("[%d] error: writebuffer: at complete, address 0x%0llx not found.\n", m_id, lineaddr);
      ERROR_OUT("writebuffer:: complete FAILS\n");
      print();
      SIM_HALT;
    }
    
         #ifdef DEBUG_WRITE_BUFFER
         DEBUG_OUT("***WriteBuffer: complete END, contents:\n");
         print();
         DEBUG_OUT("\n");
         #endif
  } // end if(m_use_write_buffer)
}

//**************************************************************************
void writebuffer_t::scheduleWakeup( void )
{
  //FIXME: scheduling with the event queue is currently creating WB inconsistencies...use issueWrites() instead
    #ifdef DEBUG_WRITE_BUFFER
         DEBUG_OUT("\n***WriteBuffer: scheduleWakeup BEGIN, contents:\n");
         print();
         DEBUG_OUT("\n");
   #endif
  if (!m_is_scheduled) {
    // wake up next cycle, and handle the delayed stores
    if (m_event_queue != NULL) {
      int mshr_entry_count = system_t::inst->m_seq[m_id]->getRubyCache()->getMSHRcount();
      if(mshr_entry_count == 0){
        // if we are running using the event queue, schedule ourselves
        // otherwise rely on caller to poll the is_scheduled variable
        tick_t current_cycle = m_event_queue->getCycle();
        m_event_queue->queueInsert( this, current_cycle, 0 );
        //we want to reschedule after every Wakeup(), so set this to true here
        m_is_scheduled = true;
      }
    }
  }
    #ifdef DEBUG_WRITE_BUFFER
         DEBUG_OUT("\n***WriteBuffer: scheduleWakeup END, contents:\n");
         print();
         DEBUG_OUT("\n");
   #endif
}

//****************************************************************************************************
void writebuffer_t::issueWrites(){
  //We issue writes from the WB only if the MSHR does NOT contain any LD entries - MSHR LDs
  //   have priority over us (note that the MSHR does NOT count Ifetch entries!)
  if(m_use_write_buffer){
    ASSERT(m_id < system_t::inst->m_numSMTProcs);
    int mshr_entry_count = system_t::inst->m_seq[m_id]->getRubyCache()->getMSHRcount();
    if(mshr_entry_count == 0){
      flushWriteBuffer();
    }
  }
}


//***************************************************************************************************
void writebuffer_t::Wakeup( void )
{
  #ifdef DEBUG_WRITE_BUFFER
      DEBUG_OUT("\n****writebuffer_t: Wakeup CALLED cycle[%d]!\n",m_event_queue->getCycle());
      print();
  #endif

  //reset is scheduled flag
  m_is_scheduled = false;
  //  DEBUG_OUT("***Wakeup called cycle[%d]\n",m_event_queue->getCycle());
  flushWriteBuffer();
  return;  
}

//***************************************************************************************************
bool writebuffer_t::checkOutstandingRequests(pa_t physical_address){
   if(m_use_write_buffer){
       #ifdef DEBUG_WRITE_BUFFER
         DEBUG_OUT("\n***WriteBuffer: checkOutsandingRequests BEGIN\n");
      #endif
    pa_t lineaddr = physical_address & m_block_mask;
    ruby_request_t * miss = static_cast<ruby_request_t*>(m_request_pool->walkList(NULL));
    while (miss != NULL) {
      if ( miss->match( lineaddr ) ) {
         return true;
      }
      miss = static_cast<ruby_request_t*>(m_request_pool->walkList( miss ));
    }
    //    Not found
    return false;
  }
   else{
     return false;
   }
}

//***************************************************************************************************
void writebuffer_t::checkOutstandingRequestsDebug(pa_t physical_address){
   if(m_use_write_buffer){
       #ifdef DEBUG_WRITE_BUFFER
         DEBUG_OUT("\n***WriteBuffer: checkOutsandingRequests BEGIN\n");
      #endif
    pa_t lineaddr = physical_address & m_block_mask;
    ruby_request_t * miss = static_cast<ruby_request_t*>(m_request_pool->walkList(NULL));
    while (miss != NULL) {
      DEBUG_OUT("miss addr = 0x%x ",physical_address);
      DEBUG_OUT("entry addr = 0x%x\n",miss->getAddress());
      if ( miss->match( lineaddr ) ) {
        DEBUG_OUT("\tMATCH FOUND\n");
        return;
      }
      miss = static_cast<ruby_request_t*>(m_request_pool->walkList( miss ));
    }
    #ifdef DEBUG_WRITE_BUFFER
        DEBUG_OUT("***Writebuffer: checkOutstandingRequests END, found? %d\n", found);
    #endif
  }
}

//****************************************************************************************************
bool writebuffer_t::checkWriteBuffer(pa_t physical_address, ruby_request_t * match){
  if(m_use_write_buffer){
      #ifdef DEBUG_WRITE_BUFFER
         DEBUG_OUT("\n***WriteBuffer: checkWriteBuffer BEGIN\n");
      #endif
    pa_t lineaddr = physical_address & m_block_mask;
    bool found = false;
    ruby_request_t * miss = static_cast<ruby_request_t*>(m_request_pool->walkList(NULL));
    while (miss != NULL) {
      if ( miss->match( lineaddr ) ) {
        found = true;
        match = miss;
        return true;
      }
      miss = static_cast<ruby_request_t*>(m_request_pool->walkList( miss ));
    }
    return false;
  }
  else{
    // if !m_use_write_buffer
    match = NULL;
    return false;
  }
}

//*****************************************************************************************************
bool writebuffer_t::checkForLoadHit(waiter_t * waiter, pa_t physical_address){
  //this is a pessimistic algorithm: if any load address hits to the WB, we flush the WB:
  ruby_request_t * match = NULL;     //contains the pointer to the matching entry
  pa_t lineaddr = physical_address & m_block_mask;
  bool found = false;
  ruby_request_t * miss = static_cast<ruby_request_t*>(m_request_pool->walkList(NULL));
  while (miss != NULL) {
    if ( miss->match( lineaddr ) ) {
      found = true;
      match = miss;
      break;
    }
    miss = static_cast<ruby_request_t*>(m_request_pool->walkList( miss ));
  }
  if(found){
    ASSERT(match != NULL);
    ASSERT(m_buffer_size > 0); 
    ASSERT(waiter != NULL);
    // The wait list should always have a dummy store waiter on it!
    ASSERT(!match->m_wait_list.Empty());
    //Note we need to perform this check bc the load might be linked to this store in the LSQ!
    //   TODO - isn't the Load always disconnected when it reaches here?
    if(waiter->Disconnected()){
      //make the LD wait on the matching ST
      waiter->InsertWaitQueue( match->m_wait_list );
    }
    //DO NOT call flushWriteBuffer() here, bc the LD has not yet finished Execute(), and we 
    //     might wakeup the load prematurely due to fastpath HITS!!
    // The WB will be flushed during the regular time - when the MSHR has no outstanding requests
  }
  return found;
}

//***************************************************************************************************
void writebuffer_t::print( void )
{
  DEBUG_OUT("[%d] writebuffer_t: Total entries: %d Outstanding: %d\n", m_id,m_buffer_size,m_outstanding_stores);

  if(m_use_write_buffer){
    ruby_request_t *miss = static_cast<ruby_request_t*>(m_request_pool->walkList(NULL));
    if (miss == NULL) {
      DEBUG_OUT("[%d] writebuffer_t: no outstanding\n", m_id);
    } else {
      DEBUG_OUT("[%d] writebuffer_t: Write Buffer Entries.\n", m_id);
    }
    while (miss != NULL) {
      DEBUG_OUT("[%d]    writebuffer_t 0x%0llx (Is Outstanding? %d) (posted: %d)\n", m_id, miss->getAddress(), miss->m_is_outstanding, miss->m_post_time);
      miss->m_wait_list.print(cout);
      miss = static_cast<ruby_request_t*>(m_request_pool->walkList( miss ));
    }
  }
  else{
    DEBUG_OUT("\t WRITE BUFFER NOT USED\n");
  }
}


      
    
