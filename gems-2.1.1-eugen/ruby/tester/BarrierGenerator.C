
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

/*
 * $Id: BarrierGenerator.C 1.3 2005/01/19 13:12:35-06:00 mikem@maya.cs.wisc.edu $
 *
 */

#include "BarrierGenerator.h"
#include "Sequencer.h"
#include "System.h"
#include "RubyConfig.h"
#include "SubBlock.h"
#include "SyntheticDriver.h"
#include "Chip.h"

BarrierGenerator::BarrierGenerator(NodeID node, SyntheticDriver& driver) :
  m_driver(driver)
{
  m_status = BarrierGeneratorStatus_Thinking;
  m_last_transition = 0;
  m_node = node;
  m_counter = 0;
  proc_counter = 0;
  m_local_sense = false;

  m_total_think = 0;
  m_think_periods = 0;

  g_eventQueue_ptr->scheduleEvent(this, 1+(random() % 200));
}

BarrierGenerator::~BarrierGenerator()
{
}

void BarrierGenerator::wakeup()
{
  DEBUG_EXPR(TESTER_COMP, MedPrio, m_node);
  DEBUG_EXPR(TESTER_COMP, MedPrio, m_status);

  if (m_status == BarrierGeneratorStatus_Thinking) {
    m_barrier_done = false;
    m_local_sense = !m_local_sense;
    m_status = BarrierGeneratorStatus_Test_Pending;
    m_last_transition = g_eventQueue_ptr->getTime();
    initiateTest();  // Test
  } else if (m_status == BarrierGeneratorStatus_Test_Waiting) {
    m_status = BarrierGeneratorStatus_Test_Pending;
    m_last_transition = g_eventQueue_ptr->getTime();
    initiateTest();  // Test
  } else if (m_status == BarrierGeneratorStatus_Release_Waiting) {
    m_status = BarrierGeneratorStatus_Release_Pending;
    m_last_transition = g_eventQueue_ptr->getTime();
    initiateRelease();  // Test
  } else if (m_status == BarrierGeneratorStatus_StoreBarrierCounter_Waiting) {
    m_status = BarrierGeneratorStatus_StoreBarrierCounter_Pending;
    m_last_transition = g_eventQueue_ptr->getTime();
    initiateStoreCtr();
  } else if (m_status == BarrierGeneratorStatus_StoreFlag_Waiting) {
    m_status = BarrierGeneratorStatus_StoreFlag_Pending;
    m_last_transition = g_eventQueue_ptr->getTime();
    initiateStoreFlag();
  } else if (m_status == BarrierGeneratorStatus_Holding) {
    m_status = BarrierGeneratorStatus_Release_Pending;
    m_last_transition = g_eventQueue_ptr->getTime();
    initiateRelease();  // Release
  } else if (m_status == BarrierGeneratorStatus_Before_Swap) {
    m_status = BarrierGeneratorStatus_Swap_Pending;
    m_last_transition = g_eventQueue_ptr->getTime();
    initiateSwap();
  } else if (m_status == BarrierGeneratorStatus_SpinFlag_Ready) {
    m_status = BarrierGeneratorStatus_SpinFlag_Pending;
    m_last_transition = g_eventQueue_ptr->getTime();
    initiateLoadFlag();
  } else {
    WARN_EXPR(m_status);
    ERROR_MSG("Invalid status");
  }
}

void BarrierGenerator::performCallback(NodeID proc, SubBlock& data)
{
  Address address = data.getAddress();
  assert(proc == m_node);

  DEBUG_EXPR(TESTER_COMP, LowPrio, proc);
  DEBUG_EXPR(TESTER_COMP, LowPrio, m_status);
  DEBUG_EXPR(TESTER_COMP, LowPrio, address);
  DEBUG_EXPR(TESTER_COMP, LowPrio, data);

  if (m_status == BarrierGeneratorStatus_Test_Pending) { 
    uint8 dat = data.readByte();
    uint8 lock = dat >> 7;
    if (lock == 1) {
      // Locked - keep spinning
      m_status = BarrierGeneratorStatus_Test_Waiting;
      m_last_transition = g_eventQueue_ptr->getTime();
      g_eventQueue_ptr->scheduleEvent(this, waitTime());
    } else {
      // Unlocked - try the swap
      m_driver.recordTestLatency(g_eventQueue_ptr->getTime() - m_last_transition);
      m_status = BarrierGeneratorStatus_Before_Swap;
      m_last_transition = g_eventQueue_ptr->getTime();
      g_eventQueue_ptr->scheduleEvent(this, waitTime());
    }
  } else if (m_status == BarrierGeneratorStatus_Swap_Pending) {
    m_driver.recordSwapLatency(g_eventQueue_ptr->getTime() - m_last_transition);
    uint8 dat = data.readByte();
    uint8 lock = dat >> 7;
    if (lock == 1) {
      // We failed to aquire the lock
      m_status = BarrierGeneratorStatus_Test_Waiting;
      m_last_transition = g_eventQueue_ptr->getTime();
      g_eventQueue_ptr->scheduleEvent(this, waitTime());
    } else {
      // We acquired the lock
      dat = dat | 0x80;
      data.writeByte(dat);
      m_status = BarrierGeneratorStatus_StoreBarrierCounter_Waiting;
      m_last_transition = g_eventQueue_ptr->getTime();
      DEBUG_MSG(TESTER_COMP, HighPrio, "Acquired");
      DEBUG_EXPR(TESTER_COMP, HighPrio, proc);
      DEBUG_EXPR(TESTER_COMP, HighPrio, g_eventQueue_ptr->getTime());
      // g_eventQueue_ptr->scheduleEvent(this, holdTime());

      g_eventQueue_ptr->scheduleEvent(this, 1);

      // initiateLoadCtr();
    }
  } else if (m_status == BarrierGeneratorStatus_StoreBarrierCounter_Pending) {

    // if value == p, reset counter and set local sense flag
    uint8 ctr = data.readByte();
    //uint8 sense = ctr >> 4;
    ctr = ctr & 0x0F; 

    ctr++;
    data.writeByte( ctr | 0x80);  // store counter and lock

    //cout << m_node << " incremented Barrier_ctr to " << (int)ctr << ", " << data << "\n";

    if (ctr == (uint8) 16) {

       data.writeByte( 0x0 );
       m_status = BarrierGeneratorStatus_StoreFlag_Waiting;
       m_barrier_done = true;

       g_eventQueue_ptr->scheduleEvent(this, 1);
    }
    else {
       
       m_status = BarrierGeneratorStatus_Release_Waiting;
       g_eventQueue_ptr->scheduleEvent(this, 1);
    }
  } else if (m_status == BarrierGeneratorStatus_StoreFlag_Pending) {

     // write flag
     if (m_local_sense) {
         data.writeByte( 0x01 );
     } 
     else {
         data.writeByte( 0x00 );
     }
     
     m_status = BarrierGeneratorStatus_Release_Waiting;
     g_eventQueue_ptr->scheduleEvent(this, 1);
  
  } else if (m_status == BarrierGeneratorStatus_Release_Pending) {
    m_driver.recordReleaseLatency(g_eventQueue_ptr->getTime() - m_last_transition);
    // We're releasing the lock
    uint8 dat = data.readByte();
    dat = dat & 0x7F;
    data.writeByte(dat);

    if (m_barrier_done) {
      m_counter++;
      proc_counter++;
      if (m_counter < g_tester_length) {
        m_status = BarrierGeneratorStatus_Thinking;
        m_last_transition = g_eventQueue_ptr->getTime();
        g_eventQueue_ptr->scheduleEvent(this, thinkTime());
      } else {

        m_driver.reportDone(proc_counter, m_node);
        m_last_transition = g_eventQueue_ptr->getTime();
      } 
    }
    else {
      m_status = BarrierGeneratorStatus_SpinFlag_Ready;
      m_last_transition = g_eventQueue_ptr->getTime();
      g_eventQueue_ptr->scheduleEvent(this, waitTime());
    }
  } else if (m_status == BarrierGeneratorStatus_SpinFlag_Pending) {

    uint8 sense = data.readByte();


    if (sense != m_local_sense) {
      m_status = BarrierGeneratorStatus_SpinFlag_Ready;
      m_last_transition = g_eventQueue_ptr->getTime();
      g_eventQueue_ptr->scheduleEvent(this, waitTime());
    }
    else {
      m_counter++;
      proc_counter++;
      if (m_counter < g_tester_length) {
        m_status = BarrierGeneratorStatus_Thinking;
        m_last_transition = g_eventQueue_ptr->getTime();
        g_eventQueue_ptr->scheduleEvent(this, thinkTime());
      } else {
        m_driver.reportDone(proc_counter, m_node);
        m_status = BarrierGeneratorStatus_Done;
        m_last_transition = g_eventQueue_ptr->getTime();
      } 
    }

  } else {
    WARN_EXPR(m_status);
    ERROR_MSG("Invalid status");
  }
}

int BarrierGenerator::thinkTime() 
{
  int ret;
  float ratio = g_think_fudge_factor;

  // return 400;

  if (ratio == 0) { 
    return g_think_time;
  }

  int r = random();
  int x = (int) ( (float)g_think_time*ratio*2.0);
  int mod = r % x; 


  int rand = ( mod+1 -  ((float)g_think_time*ratio) );

  ret = (g_think_time + rand);

  m_total_think += ret;
  m_think_periods++;

  return ret;
}

int BarrierGenerator::waitTime() const
{
  return g_wait_time;
}


void BarrierGenerator::initiateTest()
{
  DEBUG_MSG(TESTER_COMP, MedPrio, "initiating Test");
  sequencer()->makeRequest(CacheMsg(Address(0x40), CacheRequestType_LD, Address(1), AccessModeType_UserMode, 1, PrefetchBit_No, 0, false)); 
}

void BarrierGenerator::initiateSwap()
{
  DEBUG_MSG(TESTER_COMP, MedPrio, "initiating Swap");
  sequencer()->makeRequest(CacheMsg(Address(0x40), CacheRequestType_ATOMIC, Address(2), AccessModeType_UserMode, 1, PrefetchBit_No, 0, false)); 
}

void BarrierGenerator::initiateRelease()
{
  DEBUG_MSG(TESTER_COMP, MedPrio, "initiating Release");
  sequencer()->makeRequest(CacheMsg(Address(0x40), CacheRequestType_ST, Address(3), AccessModeType_UserMode, 1, PrefetchBit_No, 0, false)); 
}

void BarrierGenerator::initiateLoadCtr()
{
  DEBUG_MSG(TESTER_COMP, MedPrio, "initiating load of barrier counter");
  sequencer()->makeRequest(CacheMsg(Address(0x40), CacheRequestType_LD, Address(3), AccessModeType_UserMode, 1, PrefetchBit_No, 0, false)); 
}

void BarrierGenerator::initiateStoreCtr()
{
  DEBUG_MSG(TESTER_COMP, MedPrio, "initiating load of barrier counter");
  sequencer()->makeRequest(CacheMsg(Address(0x40), CacheRequestType_ST, Address(3), AccessModeType_UserMode, 1, PrefetchBit_No, 0, false)); 
}

void BarrierGenerator::initiateStoreFlag()
{
  DEBUG_MSG(TESTER_COMP, MedPrio, "initiating load of barrier counter");
  sequencer()->makeRequest(CacheMsg(Address(0x00), CacheRequestType_ST, Address(3), AccessModeType_UserMode, 1, PrefetchBit_No, 0, false)); 
}

void BarrierGenerator::initiateLoadFlag()
{
  DEBUG_MSG(TESTER_COMP, MedPrio, "initiating load of barrier counter");
  sequencer()->makeRequest(CacheMsg(Address(0x00), CacheRequestType_LD, Address(3), AccessModeType_UserMode, 1, PrefetchBit_No, 0, false)); 
}


Sequencer* BarrierGenerator::sequencer() const
{
  return g_system_ptr->getChip(m_node/RubyConfig::numberOfProcsPerChip())->getSequencer(m_node%RubyConfig::numberOfProcsPerChip());
}

void BarrierGenerator::print(ostream& out) const
{
}

