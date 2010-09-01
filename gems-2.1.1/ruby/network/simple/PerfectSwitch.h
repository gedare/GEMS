
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
 * Description: Perfect switch, of course it is perfect and no latency or what
 *              so ever. Every cycle it is woke up and perform all the
 *              necessary routings that must be done. Note, this switch also
 *              has number of input ports/output ports and has a routing table
 *              as well.
 *
 */

#ifndef PerfectSwitch_H
#define PerfectSwitch_H

#include "Global.h"
#include "Vector.h"
#include "Consumer.h"
#include "NodeID.h"

class MessageBuffer;
class NetDest;
class SimpleNetwork;

class LinkOrder {
public:
  int m_link;
  int m_value;
};

class PerfectSwitch : public Consumer {
public:
  // Constructors

  // constructor specifying the number of ports
  PerfectSwitch(SwitchID sid, SimpleNetwork* network_ptr);
  void addInPort(const Vector<MessageBuffer*>& in);
  void addOutPort(const Vector<MessageBuffer*>& out, const NetDest& routing_table_entry);
  void clearRoutingTables();
  void clearBuffers();
  void reconfigureOutPort(const NetDest& routing_table_entry);
  int getInLinks() const { return m_in.size(); }
  int getOutLinks() const { return m_out.size(); }

  // Destructor
  ~PerfectSwitch();
  
  // Public Methods
  void wakeup();

  void printStats(ostream& out) const;
  void clearStats();
  void printConfig(ostream& out) const;

  void print(ostream& out) const;
private:

  // Private copy constructor and assignment operator
  PerfectSwitch(const PerfectSwitch& obj);
  PerfectSwitch& operator=(const PerfectSwitch& obj);
  
  // Data Members (m_ prefix)
  SwitchID m_switch_id;
  
  // vector of queues from the components
  Vector<Vector<MessageBuffer*> > m_in;
  Vector<Vector<MessageBuffer*> > m_out;
  Vector<NetDest> m_routing_table;
  Vector<LinkOrder> m_link_order;
  int m_virtual_networks;
  int m_round_robin_start;
  int m_wakeups_wo_switch;
  SimpleNetwork* m_network_ptr;
};

// Output operator declaration
ostream& operator<<(ostream& out, const PerfectSwitch& obj);

// ******************* Definitions *******************

// Output operator definition
extern inline 
ostream& operator<<(ostream& out, const PerfectSwitch& obj)
{
  obj.print(out);
  out << flush;
  return out;
}

#endif //PerfectSwitch_H
