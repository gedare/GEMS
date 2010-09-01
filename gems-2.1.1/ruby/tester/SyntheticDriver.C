
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

#include "Global.h"
#include "System.h"
#include "SyntheticDriver.h"
#include "EventQueue.h"
//#ifndef XACT_MEM
#include "RequestGenerator.h"
//#endif
//#include "XactAbortRequestGenerator.h"
//#include "XactRequestGenerator.h"
#include "SubBlock.h"
#include "Chip.h"

SyntheticDriver::SyntheticDriver(System* sys_ptr) 
{
  cout << "SyntheticDriver::SyntheticDriver" << endl;
  if (g_SIMICS) {
    ERROR_MSG("g_SIMICS should not be defined.");
  }

  m_finish_time = 0;
  m_done_counter = 0;

  m_last_progress_vector.setSize(RubyConfig::numberOfProcessors());
  for (int i=0; i<m_last_progress_vector.size(); i++) {
    m_last_progress_vector[i] = 0;
  }

  m_lock_vector.setSize(g_synthetic_locks);
  for (int i=0; i<m_lock_vector.size(); i++) {
    m_lock_vector[i] = -1;  // No processor last held it
  }

  m_request_generator_vector.setSize(RubyConfig::numberOfProcessors());
  for (int i=0; i<m_request_generator_vector.size(); i++) {
    if(XACT_MEMORY){
      //m_request_generator_vector[i] = new XactRequestGenerator(i, *this);
    } else {
      m_request_generator_vector[i] = new RequestGenerator(i, *this);
    }
  }

  // add the tester consumer to the global event queue
  g_eventQueue_ptr->scheduleEvent(this, 1);
}

SyntheticDriver::~SyntheticDriver()
{
  for (int i=0; i<m_last_progress_vector.size(); i++) {
    delete m_request_generator_vector[i];
  }
}

void SyntheticDriver::hitCallback(NodeID proc, SubBlock& data, CacheRequestType type, int thread)
{
  DEBUG_EXPR(TESTER_COMP, MedPrio, data);
  //cout << "  " << proc << " in S.D. hitCallback" << endl;
  if(XACT_MEMORY){
    //XactRequestGenerator* reqGen = static_cast<XactRequestGenerator*>(m_request_generator_vector[proc]);
    //reqGen->performCallback(proc, data);
  } else {
    m_request_generator_vector[proc]->performCallback(proc, data);
  }

  // Mark that we made progress
  m_last_progress_vector[proc] = g_eventQueue_ptr->getTime();
}

void SyntheticDriver::abortCallback(NodeID proc, SubBlock& data, CacheRequestType type, int thread)
{
  //cout << "SyntheticDriver::abortCallback" << endl;
  DEBUG_EXPR(TESTER_COMP, MedPrio, data);

  if(XACT_MEMORY){
    //XactRequestGenerator* reqGen = static_cast<XactRequestGenerator*>(m_request_generator_vector[proc]);
    //reqGen->abortTransaction();
    //reqGen->performCallback(proc, data);
  } else {
    m_request_generator_vector[proc]->performCallback(proc, data);
  }
  
  // Mark that we made progress
  m_last_progress_vector[proc] = g_eventQueue_ptr->getTime();
}

// For Transactional Memory
/*
// called whenever we send a nack
void SyntheticDriver::notifySendNack( int proc, const Address & addr, uint64 remote_timestamp, const MachineID & remote_id ){
  if(XACT_MEMORY){
    //XactRequestGenerator* reqGen = static_cast<XactRequestGenerator*>(m_request_generator_vector[proc]);
    //reqGen->notifySendNack(addr, remote_timestamp, remote_id);
  }
  else{
    cout << "notifySendNack NOT USING TM" << endl;
    ASSERT(0);
  }
}

// called whenever we receive a NACK
//  Either for a demand request or log store
void SyntheticDriver::notifyReceiveNack( int proc, const Address & addr, uint64 remote_timestamp, const MachineID & remote_id ){
  if(XACT_MEMORY){
    //XactRequestGenerator* reqGen = static_cast<XactRequestGenerator*>(m_request_generator_vector[proc]);
    //reqGen->notifyReceiveNack(addr, remote_timestamp, remote_id);
  }
  else{
    cout << "notifyReceiveNack NOT USING TM" << endl;
    ASSERT(0);
  }
}

// called whenever we received ALL the NACKs. Take abort or retry action here
void SyntheticDriver::notifyReceiveNackFinal(int proc, const Address & addr){
  if(XACT_MEMORY){
    //XactRequestGenerator* reqGen = static_cast<XactRequestGenerator*>(m_request_generator_vector[proc]);
    //reqGen->notifyReceiveNackFinal(addr);
  }
  else{
    cout << "notifyReceiveNackFinal NOT USING TM" << endl;
    ASSERT(0);
  }
}

// called during abort handling
// void SyntheticDriver::notifyAbortStart( const Address & handlerPC ){

// }

// void SyntheticDriver::notifyAbortComplete( const Address & newPC ){

// }
*/

Address SyntheticDriver::pickAddress(NodeID node)
{
  // This methods picks a random lock that we were NOT that last
  // processor to acquire.  Why?  Without this change 2 and 4
  // processor runs, the odds of having the lock in your cache in
  // read/write state is 50% or 25%, respectively.  This effect can
  // make our 'throughput per processor' results look too strange.

  Address addr;
  // FIXME - make this a parameter of the workload
  bool done = false;
  int lock_number = 0;
  int counter = 0;
  while (1) {
    // Pick a random lock
    lock_number = random() % m_lock_vector.size();

    // Were we the last to acquire the lock?
    if (m_lock_vector[lock_number] != node) {
      break;
    }

    // Don't keep trying forever, since if there is only one lock, we're always the last to try to obtain the lock
    counter++;
    if (counter > 10) { 
      break;
    }
  }

  // We're going to acquire it soon, so we can update the last
  // processor to hold the lock at this time
  m_lock_vector[lock_number] = node;

  // One lock per cache line
  addr.setAddress(lock_number * RubyConfig::dataBlockBytes());
  return addr;
}

void SyntheticDriver::reportDone()
{
  m_done_counter++;
  if (m_done_counter == RubyConfig::numberOfProcessors()) {
    m_finish_time = g_eventQueue_ptr->getTime();
  }
}

void SyntheticDriver::recordTestLatency(Time time)
{
  m_test_latency.add(time);
}

void SyntheticDriver::recordSwapLatency(Time time)
{
  m_swap_latency.add(time);
}

void SyntheticDriver::recordReleaseLatency(Time time)
{
  m_release_latency.add(time);
}

void SyntheticDriver::wakeup()
{ 
  //  checkForDeadlock();
  if (m_done_counter < RubyConfig::numberOfProcessors()) {
    g_eventQueue_ptr->scheduleEvent(this, g_DEADLOCK_THRESHOLD);
  }
}

void SyntheticDriver::checkForDeadlock()
{
  int size = m_last_progress_vector.size();
  Time current_time = g_eventQueue_ptr->getTime();
  for (int processor=0; processor<size; processor++) {
    if ((current_time - m_last_progress_vector[processor]) > g_DEADLOCK_THRESHOLD) {
      WARN_EXPR(processor);
      Sequencer* seq_ptr = g_system_ptr->getChip(processor/RubyConfig::numberOfProcsPerChip())->getSequencer(processor%RubyConfig::numberOfProcsPerChip());
      assert(seq_ptr != NULL);
      //     if (seq_ptr->isRequestPending()) {
      //       WARN_EXPR(seq_ptr->pendingAddress());
      //      }
      WARN_EXPR(current_time);
      WARN_EXPR(m_last_progress_vector[processor]);
      WARN_EXPR(current_time - m_last_progress_vector[processor]);
      ERROR_MSG("Deadlock detected.");
    }
  }
}

integer_t  SyntheticDriver::readPhysicalMemory(int procID, physical_address_t address,
                                               int len ){
  char buffer[8];
  ASSERT(len <= 8);
  Sequencer* seq = g_system_ptr->getChip(procID/RubyConfig::numberOfProcsPerChip())->getSequencer(procID%RubyConfig::numberOfProcsPerChip());
  assert(seq != NULL);
  bool found = seq->getRubyMemoryValue(Address(address), buffer, len );
  ASSERT(found);
  return *((integer_t *) buffer);
}
  
void  SyntheticDriver::writePhysicalMemory( int procID, physical_address_t address,
                                            integer_t value, int len ){
  char buffer[8];
  ASSERT(len <= 8);

  memcpy(buffer, (const void*) &value, len);
  DEBUG_EXPR(TESTER_COMP, MedPrio, "");
  Sequencer* seq = g_system_ptr->getChip(procID/RubyConfig::numberOfProcsPerChip())->getSequencer(procID%RubyConfig::numberOfProcsPerChip());
  assert(seq != NULL);
  bool found = seq->setRubyMemoryValue(Address(address), buffer, len );
  ASSERT(found);
  //return found;
}

void SyntheticDriver::printStats(ostream& out) const
{
  out << endl;
  out << "SyntheticDriver Stats" << endl;
  out << "---------------------" << endl;

  out << "synthetic_finish_time: " << m_finish_time << endl;
  out << "test_latency: " << m_test_latency << endl;
  out << "swap_latency: " << m_swap_latency << endl;
  out << "release_latency: " << m_release_latency << endl;
}

void SyntheticDriver::print(ostream& out) const
{
}
