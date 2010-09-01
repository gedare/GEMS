
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
 * Description: The class to implement bandwidth and latency throttle. An
 *              instance of consumer class that can be woke up. It is only used
 *              to control bandwidth at output port of a switch. And the
 *              throttle is added *after* the output port, means the message is
 *              put in the output port of the PerfectSwitch (a
 *              intermediateBuffers) first, then go through the Throttle.
 *
 */

#ifndef THROTTLE_H
#define THROTTLE_H

#include "Global.h"
#include "Vector.h"
#include "Consumer.h"
#include "NodeID.h"
#include "RubyConfig.h"

class MessageBuffer;

class Throttle : public Consumer {
public:
  // Constructors
  Throttle(int sID, NodeID node, int link_latency, int link_bandwidth_multiplier);
  Throttle(NodeID node, int link_latency, int link_bandwidth_multiplier);

  // Destructor
  ~Throttle() {}
  
  // Public Methods
  void addLinks(const Vector<MessageBuffer*>& in_vec, const Vector<MessageBuffer*>& out_vec);
  void wakeup();
  bool broadcastBandwidthAvailable(int rand) const;

  void printStats(ostream& out) const;
  void clearStats();
  void printConfig(ostream& out) const;
  double getUtilization() const; // The average utilization (a percent) since last clearStats()
  int getLinkBandwidth() const { return g_endpoint_bandwidth * m_link_bandwidth_multiplier; }
  int getLatency() const { return m_link_latency; }

  const Vector<Vector<int> >& getCounters() const { return m_message_counters; }

  void clear();

  void print(ostream& out) const;

private:
  // Private Methods
  void init(NodeID node, int link_latency, int link_bandwidth_multiplier);
  void addVirtualNetwork(MessageBuffer* in_ptr, MessageBuffer* out_ptr);
  void linkUtilized(double ratio) { m_links_utilized += ratio; }

  // Private copy constructor and assignment operator
  Throttle(const Throttle& obj);
  Throttle& operator=(const Throttle& obj);
  
  // Data Members (m_ prefix)
  Vector<MessageBuffer*> m_in;
  Vector<MessageBuffer*> m_out;
  Vector<Vector<int> > m_message_counters;
  int m_vnets;
  Vector<int> m_units_remaining;
  int m_sID;
  NodeID m_node;
  int m_bash_counter;
  int m_bandwidth_since_sample;
  Time m_last_bandwidth_sample;
  int m_link_bandwidth_multiplier;
  int m_link_latency;
  int m_wakeups_wo_switch;

  // For tracking utilization
  Time m_ruby_start;
  double m_links_utilized;
};

// Output operator declaration
ostream& operator<<(ostream& out, const Throttle& obj);

// ******************* Definitions *******************

// Output operator definition
extern inline 
ostream& operator<<(ostream& out, const Throttle& obj)
{
  obj.print(out);
  out << flush;
  return out;
}

#endif //THROTTLE_H
