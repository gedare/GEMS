
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
 * $Id$
 *
 */

// This Deterministic Generator generates GETX requests for all nodes in the system
// The GETX requests are generated one at a time in round-robin fashion 0...1...2...etc.

#include "DetermGETXGenerator.h"
#include "DetermGETXGeneratorStatus.h"
#include "LockStatus.h"
#include "Sequencer.h"
#include "System.h"
#include "RubyConfig.h"
#include "SubBlock.h"
#include "DeterministicDriver.h"
#include "Chip.h"

DetermGETXGenerator::DetermGETXGenerator(NodeID node, DeterministicDriver& driver) :
  m_driver(driver)
{
  m_status = DetermGETXGeneratorStatus_Thinking;
  m_last_transition = 0;
  m_node = node;
  m_address = Address(9999);  // initialize to null value
  m_counter = 0;

  // don't know exactly when this node needs to request so just guess randomly
  g_eventQueue_ptr->scheduleEvent(this, 1+(random() % 200));
}

DetermGETXGenerator::~DetermGETXGenerator()
{
}

void DetermGETXGenerator::wakeup()
{
  DEBUG_EXPR(TESTER_COMP, MedPrio, m_node);
  DEBUG_EXPR(TESTER_COMP, MedPrio, m_status);

  // determine if this node is next for the GETX round robin request
  if (m_status == DetermGETXGeneratorStatus_Thinking) {
    if (m_driver.isStoreReady(m_node)) {
      pickAddress();
      m_status = DetermGETXGeneratorStatus_Store_Pending;  // Store Pending
      m_last_transition = g_eventQueue_ptr->getTime();
      initiateStore();  // GETX
    } else { // I'll check again later
      g_eventQueue_ptr->scheduleEvent(this, thinkTime());
    }
  } else {
    WARN_EXPR(m_status);
    ERROR_MSG("Invalid status");
  }

}

void DetermGETXGenerator::performCallback(NodeID proc, SubBlock& data)
{
  Address address = data.getAddress();
  assert(proc == m_node);
  assert(address == m_address);  

  DEBUG_EXPR(TESTER_COMP, LowPrio, proc);
  DEBUG_EXPR(TESTER_COMP, LowPrio, m_status);
  DEBUG_EXPR(TESTER_COMP, LowPrio, address);
  DEBUG_EXPR(TESTER_COMP, LowPrio, data);

  if (m_status == DetermGETXGeneratorStatus_Store_Pending) { 
    m_driver.recordStoreLatency(g_eventQueue_ptr->getTime() - m_last_transition);
    data.writeByte(m_node);
    m_driver.storeCompleted(m_node, data.getAddress());  // advance the store queue

    m_counter++;
    if (m_counter < g_tester_length) {
      m_status = DetermGETXGeneratorStatus_Thinking;
      m_last_transition = g_eventQueue_ptr->getTime();
      g_eventQueue_ptr->scheduleEvent(this, waitTime());
    } else {
      m_driver.reportDone();
      m_status = DetermGETXGeneratorStatus_Done;
      m_last_transition = g_eventQueue_ptr->getTime();
    } 

  } else {
    WARN_EXPR(m_status);
    ERROR_MSG("Invalid status");
  }
}

int DetermGETXGenerator::thinkTime() const
{
  return g_think_time;
}

int DetermGETXGenerator::waitTime() const
{
  return g_wait_time;
}

void DetermGETXGenerator::pickAddress()
{
  assert(m_status == DetermGETXGeneratorStatus_Thinking);

  m_address = m_driver.getNextStoreAddr(m_node);
}

void DetermGETXGenerator::initiateStore()
{
  DEBUG_MSG(TESTER_COMP, MedPrio, "initiating Store");
  sequencer()->makeRequest(CacheMsg(m_address, m_address, CacheRequestType_ST, Address(3), AccessModeType_UserMode, 1, PrefetchBit_No, 0, Address(0), 0 /* only 1 SMT thread */, 0, false)); 
}

Sequencer* DetermGETXGenerator::sequencer() const
{
  return g_system_ptr->getChip(m_node/RubyConfig::numberOfProcsPerChip())->getSequencer(m_node%RubyConfig::numberOfProcsPerChip());
}

void DetermGETXGenerator::print(ostream& out) const
{
}

