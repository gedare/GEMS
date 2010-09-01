
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
 * Description: see Throttle.h
 *
 */

#include "Throttle.h"
#include "MessageBuffer.h"
#include "Network.h"
#include "System.h"
#include "NetworkMessage.h"
#include "Protocol.h"

const int HIGH_RANGE = 256;
const int ADJUST_INTERVAL = 50000;
const int MESSAGE_SIZE_MULTIPLIER = 1000;
//const int BROADCAST_SCALING = 4; // Have a 16p system act like a 64p systems
const int BROADCAST_SCALING = 1;
const int PRIORITY_SWITCH_LIMIT = 128;

static int network_message_to_size(NetworkMessage* net_msg_ptr);

extern std::ostream * debug_cout_ptr;

Throttle::Throttle(int sID, NodeID node, int link_latency, int link_bandwidth_multiplier)
{
  init(node, link_latency, link_bandwidth_multiplier);
  m_sID = sID;
}

Throttle::Throttle(NodeID node, int link_latency, int link_bandwidth_multiplier)
{
  init(node, link_latency, link_bandwidth_multiplier);
  m_sID = 0;
}

void Throttle::init(NodeID node, int link_latency, int link_bandwidth_multiplier)
{
  m_node = node;
  m_vnets = 0;

  ASSERT(link_bandwidth_multiplier > 0);
  m_link_bandwidth_multiplier = link_bandwidth_multiplier;
  m_link_latency = link_latency;

  m_bash_counter = HIGH_RANGE;
  m_bandwidth_since_sample = 0;
  m_last_bandwidth_sample = 0;
  m_wakeups_wo_switch = 0;
  clearStats();
}

void Throttle::clear()
{
  for (int counter = 0; counter < m_vnets; counter++) {
    m_in[counter]->clear();
    m_out[counter]->clear();
  }
}

void Throttle::addLinks(const Vector<MessageBuffer*>& in_vec, const Vector<MessageBuffer*>& out_vec)
{
  assert(in_vec.size() == out_vec.size());
  for (int i=0; i<in_vec.size(); i++) {
    addVirtualNetwork(in_vec[i], out_vec[i]);
  }

  m_message_counters.setSize(MessageSizeType_NUM);
  for (int i=0; i<MessageSizeType_NUM; i++) {
    m_message_counters[i].setSize(in_vec.size());
    for (int j=0; j<m_message_counters[i].size(); j++) {
      m_message_counters[i][j] = 0;
    }
  }

  if (g_PRINT_TOPOLOGY) {
    m_out_link_vec.insertAtBottom(out_vec);  
  }
}

void Throttle::addVirtualNetwork(MessageBuffer* in_ptr, MessageBuffer* out_ptr)
{
  m_units_remaining.insertAtBottom(0);
  m_in.insertAtBottom(in_ptr);
  m_out.insertAtBottom(out_ptr);
  
  // Set consumer and description
  m_in[m_vnets]->setConsumer(this);
  string desc = "[Queue to Throttle " + NodeIDToString(m_sID) + " " + NodeIDToString(m_node) + "]";
  m_in[m_vnets]->setDescription(desc);
  m_vnets++;
}

void Throttle::wakeup()
{
  // Limits the number of message sent to a limited number of bytes/cycle.
  assert(getLinkBandwidth() > 0);
  int bw_remaining = getLinkBandwidth();

  // Give the highest numbered link priority most of the time
  m_wakeups_wo_switch++;
  int highest_prio_vnet = m_vnets-1;
  int lowest_prio_vnet = 0;
  int counter = 1;
  bool schedule_wakeup = false;

  // invert priorities to avoid starvation seen in the component network
  if (m_wakeups_wo_switch > PRIORITY_SWITCH_LIMIT) { 
    m_wakeups_wo_switch = 0;
    highest_prio_vnet = 0;
    lowest_prio_vnet = m_vnets-1;
    counter = -1;
  } 

  for (int vnet = highest_prio_vnet; (vnet*counter) >= (counter*lowest_prio_vnet); vnet -= counter) {

    assert(m_out[vnet] != NULL);
    assert(m_in[vnet] != NULL);
    assert(m_units_remaining[vnet] >= 0);
    
    while ((bw_remaining > 0) && ((m_in[vnet]->isReady()) || (m_units_remaining[vnet] > 0)) && m_out[vnet]->areNSlotsAvailable(1)) {
      
      // See if we are done transferring the previous message on this virtual network
      if (m_units_remaining[vnet] == 0 && m_in[vnet]->isReady()) {
        
        // Find the size of the message we are moving
        MsgPtr msg_ptr = m_in[vnet]->peekMsgPtr();
        NetworkMessage* net_msg_ptr = dynamic_cast<NetworkMessage*>(msg_ptr.ref());
        m_units_remaining[vnet] += network_message_to_size(net_msg_ptr);
        
        DEBUG_NEWLINE(NETWORK_COMP,HighPrio);
        DEBUG_MSG(NETWORK_COMP,HighPrio,"throttle: " + int_to_string(m_node)
                  + " my bw " + int_to_string(getLinkBandwidth())
                  + " bw spent enqueueing net msg " + int_to_string(m_units_remaining[vnet])
                  + " time: " + int_to_string(g_eventQueue_ptr->getTime()) + ".");

        // Move the message
        m_out[vnet]->enqueue(m_in[vnet]->peekMsgPtr(), m_link_latency);
        m_in[vnet]->pop();

        // Count the message
        m_message_counters[net_msg_ptr->getMessageSize()][vnet]++;

        DEBUG_MSG(NETWORK_COMP,LowPrio,*m_out[vnet]);
        DEBUG_NEWLINE(NETWORK_COMP,HighPrio);
      }

      // Calculate the amount of bandwidth we spent on this message
      int diff = m_units_remaining[vnet] - bw_remaining;
      m_units_remaining[vnet] = max(0, diff);
      bw_remaining = max(0, -diff);
    }

    if ((bw_remaining > 0) && ((m_in[vnet]->isReady()) || (m_units_remaining[vnet] > 0)) && !m_out[vnet]->areNSlotsAvailable(1)) {
      DEBUG_MSG(NETWORK_COMP,LowPrio,vnet);
      schedule_wakeup = true; // schedule me to wakeup again because I'm waiting for my output queue to become available
    }
  }
  
  // We should only wake up when we use the bandwidth
  //  assert(bw_remaining != getLinkBandwidth());  // This is only mostly true

  // Record that we used some or all of the link bandwidth this cycle
  double ratio = 1.0-(double(bw_remaining)/double(getLinkBandwidth()));
  // If ratio = 0, we used no bandwidth, if ratio = 1, we used all
  linkUtilized(ratio);

  // Sample the link bandwidth utilization over a number of cycles
  int bw_used = getLinkBandwidth()-bw_remaining;
  m_bandwidth_since_sample += bw_used;

  // FIXME - comment out the bash specific code for faster performance
  // Start Bash code
  // Update the predictor
  Time current_time = g_eventQueue_ptr->getTime();
  while ((current_time - m_last_bandwidth_sample) > ADJUST_INTERVAL) {
    double utilization = m_bandwidth_since_sample/double(ADJUST_INTERVAL * getLinkBandwidth());

    if (utilization > g_bash_bandwidth_adaptive_threshold) {
      // Used more bandwidth
      m_bash_counter++;
    } else {
      // Used less bandwidth
      m_bash_counter--;
    }

    // Make sure we don't overflow
    m_bash_counter = min(HIGH_RANGE, m_bash_counter);
    m_bash_counter = max(0, m_bash_counter);

    // Reset samples
    m_last_bandwidth_sample += ADJUST_INTERVAL;
    m_bandwidth_since_sample = 0;
  }
  // End Bash code

  if ((bw_remaining > 0) && !schedule_wakeup) {
    // We have extra bandwidth and our output buffer was available, so we must not have anything else to do until another message arrives.
    DEBUG_MSG(NETWORK_COMP,LowPrio,*this);
    DEBUG_MSG(NETWORK_COMP,LowPrio,"not scheduled again");
  } else {
    DEBUG_MSG(NETWORK_COMP,LowPrio,*this);
    DEBUG_MSG(NETWORK_COMP,LowPrio,"scheduled again");
    // We are out of bandwidth for this cycle, so wakeup next cycle and continue
    g_eventQueue_ptr->scheduleEvent(this, 1);
  }
}

bool Throttle::broadcastBandwidthAvailable(int rand) const
{
  bool result =  !(m_bash_counter > ((HIGH_RANGE/4) + (rand % (HIGH_RANGE/2))));
  return result;
}

void Throttle::printStats(ostream& out) const
{
  out << "utilized_percent: " << getUtilization() << endl;  
}

void Throttle::clearStats()
{
  m_ruby_start = g_eventQueue_ptr->getTime();
  m_links_utilized = 0.0;

  for (int i=0; i<m_message_counters.size(); i++) {
    for (int j=0; j<m_message_counters[i].size(); j++) {
      m_message_counters[i][j] = 0;
    }
  }
}

void Throttle::printConfig(ostream& out) const
{
  
}

double Throttle::getUtilization() const
{
  return (100.0 * double(m_links_utilized)) / (double(g_eventQueue_ptr->getTime()-m_ruby_start));
}

void Throttle::print(ostream& out) const
{
  out << "[Throttle: " << m_sID << " " << m_node << " bw: " << getLinkBandwidth() << "]";
}

// Helper function

static 
int network_message_to_size(NetworkMessage* net_msg_ptr)
{
  assert(net_msg_ptr != NULL);

  // Artificially increase the size of broadcast messages
  if (BROADCAST_SCALING > 1) {
    if (net_msg_ptr->getDestination().isBroadcast()) {
      return (MessageSizeType_to_int(net_msg_ptr->getMessageSize()) * MESSAGE_SIZE_MULTIPLIER * BROADCAST_SCALING);
    }
  }
  return (MessageSizeType_to_int(net_msg_ptr->getMessageSize()) * MESSAGE_SIZE_MULTIPLIER);
}
