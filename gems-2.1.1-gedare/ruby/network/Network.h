
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
 * Network.h
 * 
 * Description: The Network class is the base class for classes that
 * implement the interconnection network between components
 * (processor/cache components and memory/directory components).  The
 * interconnection network as described here is not a physical
 * network, but a programming concept used to implement all
 * communication between components.  Thus parts of this 'network'
 * will model the on-chip connections between cache controllers and
 * directory controllers as well as the links between chip and network
 * switches.
 *
 * $Id$
 * */

#ifndef NETWORK_H
#define NETWORK_H

#include "Global.h"
#include "NodeID.h"
#include "MessageSizeType.h"

class NetDest;
class MessageBuffer;
class Throttle;

class Network {
public:
  // Constructors
  Network() {}

  // Destructor
  virtual ~Network() {}

  // Public Methods

  static Network* createNetwork(int nodes);

  // returns the queue requested for the given component
  virtual MessageBuffer* getToNetQueue(NodeID id, bool ordered, int netNumber) = 0;
  virtual MessageBuffer* getFromNetQueue(NodeID id, bool ordered, int netNumber) = 0;
  virtual const Vector<Throttle*>* getThrottles(NodeID id) const { return NULL; }

  virtual int getNumNodes() {return 1;}

  virtual void makeOutLink(SwitchID src, NodeID dest, const NetDest& routing_table_entry, int link_latency, int link_weight, int bw_multiplier, bool isReconfiguration) = 0;
  virtual void makeInLink(SwitchID src, NodeID dest, const NetDest& routing_table_entry, int link_latency, int bw_multiplier, bool isReconfiguration) = 0;
  virtual void makeInternalLink(SwitchID src, NodeID dest, const NetDest& routing_table_entry, int link_latency, int link_weight, int bw_multiplier, bool isReconfiguration) = 0;

  virtual void reset() = 0;

  virtual void printStats(ostream& out) const = 0;
  virtual void clearStats() = 0;
  virtual void printConfig(ostream& out) const = 0;
  virtual void print(ostream& out) const = 0;

private:

  // Private Methods
  // Private copy constructor and assignment operator
  Network(const Network& obj);
  Network& operator=(const Network& obj);

  // Data Members (m_ prefix)
};

// Output operator declaration
ostream& operator<<(ostream& out, const Network& obj);

// ******************* Definitions *******************

// Output operator definition
extern inline 
ostream& operator<<(ostream& out, const Network& obj)
{
  obj.print(out);
  out << flush;
  return out;
}

// Code to map network message size types to an integer number of bytes
const int CONTROL_MESSAGE_SIZE = 8;
const int DATA_MESSAGE_SIZE = (64+8);

extern inline
int MessageSizeType_to_int(MessageSizeType size_type)
{
  switch(size_type) {
  case MessageSizeType_Undefined:
    ERROR_MSG("Can't convert Undefined MessageSizeType to integer");
    break;
  case MessageSizeType_Control:
  case MessageSizeType_Request_Control:
  case MessageSizeType_Reissue_Control:
  case MessageSizeType_Response_Control:
  case MessageSizeType_Writeback_Control:
  case MessageSizeType_Forwarded_Control:
  case MessageSizeType_Invalidate_Control:
  case MessageSizeType_Unblock_Control:
  case MessageSizeType_Persistent_Control:
  case MessageSizeType_Completion_Control:
    return CONTROL_MESSAGE_SIZE;
    break;
  case MessageSizeType_Data:
  case MessageSizeType_Response_Data:
  case MessageSizeType_ResponseLocal_Data:
  case MessageSizeType_ResponseL2hit_Data:
  case MessageSizeType_Writeback_Data:
    return DATA_MESSAGE_SIZE;
    break;
  default:
    ERROR_MSG("Invalid range for type MessageSizeType");
    break;
  }
  return 0;
}

#endif //NETWORK_H
