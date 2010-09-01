
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
 * Description: The actual modelled switch. It use the perfect switch and a
 *              Throttle object to control and bandwidth and timing *only for
 *              the output port*. So here we have un-realistic modelling, 
 *              since the order of PerfectSwitch and Throttle objects get
 *              woke up affect the message timing. A more accurate model would
 *              be having two set of system states, one for this cycle, one for
 *              next cycle. And on the cycle boundary swap the two set of
 *              states.
 *
 */

#ifndef Switch_H
#define Switch_H

#include "Global.h"
#include "Vector.h"

class MessageBuffer;
class PerfectSwitch;
class NetDest;
class SimpleNetwork;
class Throttle;

class Switch {
public:
  // Constructors

  // constructor specifying the number of ports
  Switch(SwitchID sid, SimpleNetwork* network_ptr);
  void addInPort(const Vector<MessageBuffer*>& in);
  void addOutPort(const Vector<MessageBuffer*>& out, const NetDest& routing_table_entry, int link_latency, int bw_multiplier);
  const Throttle* getThrottle(LinkID link_number) const;
  const Vector<Throttle*>* getThrottles() const;
  void clearRoutingTables();
  void clearBuffers();
  void reconfigureOutPort(const NetDest& routing_table_entry);

  void printStats(ostream& out) const;
  void clearStats();
  void printConfig(ostream& out) const;

  // Destructor
  ~Switch();
  
  void print(ostream& out) const;
private:

  // Private copy constructor and assignment operator
  Switch(const Switch& obj);
  Switch& operator=(const Switch& obj);
  
  // Data Members (m_ prefix)
  PerfectSwitch* m_perfect_switch_ptr;
  Vector<Throttle*> m_throttles;
  Vector<MessageBuffer*> m_buffers_to_free;
  SwitchID m_switch_id;
};

// Output operator declaration
ostream& operator<<(ostream& out, const Switch& obj);

// ******************* Definitions *******************

// Output operator definition
extern inline 
ostream& operator<<(ostream& out, const Switch& obj)
{
  obj.print(out);
  out << flush;
  return out;
}

#endif //Switch_H
