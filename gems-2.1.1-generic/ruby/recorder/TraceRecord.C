
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

#include "TraceRecord.h"
#include "Sequencer.h"
#include "System.h"
#include "AbstractChip.h"
#include "CacheMsg.h"

TraceRecord::TraceRecord(NodeID id, const Address& data_addr, const Address& pc_addr, CacheRequestType type, Time time) 
{
  m_node_num = id;
  m_data_address = data_addr;
  m_pc_address = pc_addr;
  m_time = time;
  m_type = type;

  // Don't differentiate between store misses and atomic requests in
  // the trace
  if (m_type == CacheRequestType_ATOMIC) {
    m_type = CacheRequestType_ST;  
  }
}

// Public copy constructor and assignment operator
TraceRecord::TraceRecord(const TraceRecord& obj)
{
  *this = obj;  // Call assignment operator
}

TraceRecord& TraceRecord::operator=(const TraceRecord& obj)
{
  m_node_num = obj.m_node_num;
  m_time = obj.m_time;
  m_data_address = obj.m_data_address;
  m_pc_address = obj.m_pc_address;
  m_type = obj.m_type;
  return *this;
}

void TraceRecord::issueRequest() const
{
  // Lookup sequencer pointer from system
  // Note that the chip index also needs to take into account SMT configurations
  AbstractChip* chip_ptr = g_system_ptr->getChip(m_node_num/RubyConfig::numberOfProcsPerChip()/RubyConfig::numberofSMTThreads());
  assert(chip_ptr != NULL);
  Sequencer* sequencer_ptr = chip_ptr->getSequencer((m_node_num/RubyConfig::numberofSMTThreads())%RubyConfig::numberOfProcsPerChip());
  assert(sequencer_ptr != NULL);
    
  CacheMsg request(m_data_address, m_data_address, m_type, m_pc_address, AccessModeType_UserMode, 0, PrefetchBit_Yes, 0, Address(0), 0 /* only 1 SMT thread */, 0, false);

  // Clear out the sequencer
  while (!sequencer_ptr->empty()) {
    g_eventQueue_ptr->triggerEvents(g_eventQueue_ptr->getTime() + 100);
  }

  sequencer_ptr->makeRequest(request);

  // Clear out the sequencer
  while (!sequencer_ptr->empty()) {
    g_eventQueue_ptr->triggerEvents(g_eventQueue_ptr->getTime() + 100);
  }
}

void TraceRecord::print(ostream& out) const
{
  out << "[TraceRecord: Node, " << m_node_num << ", " << m_data_address << ", " << m_pc_address << ", " << m_type << ", Time: " << m_time << "]";
}

void TraceRecord::output(ostream& out) const
{
  out << m_node_num << " ";
  m_data_address.output(out);
  out << " ";
  m_pc_address.output(out);
  out << " ";
  out << m_type;
  out << endl;
}

bool TraceRecord::input(istream& in)
{
  in >> m_node_num;
  m_data_address.input(in);
  m_pc_address.input(in);
  string type;
  if (!in.eof()) {
    in >> type;
    m_type = string_to_CacheRequestType(type);
    
    // Ignore the rest of the line
    char c = '\0';
    while ((!in.eof()) && (c != '\n')) {
      in.get(c);
    }
    
    return true;
  } else {
    return false;
  }
}
