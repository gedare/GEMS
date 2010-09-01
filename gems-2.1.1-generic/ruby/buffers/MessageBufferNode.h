
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
 * EventQueueNode.h
 * 
 * Description: 
 *
 * $Id: MessageBufferNode.h,v 3.3 2003/12/04 15:01:34 xu Exp $
 *
 */

#ifndef MESSAGEBUFFERNODE_H
#define MESSAGEBUFFERNODE_H

#include "Global.h"
#include "Message.h"

class MessageBufferNode {
public:
  // Constructors
  MessageBufferNode() { m_time = 0; m_msg_counter = 0; }
  MessageBufferNode(const Time& time, int counter, const MsgPtr& msgptr)
    { m_time = time; m_msgptr = msgptr; m_msg_counter = counter; }
  // Destructor
  //~MessageBufferNode();
  
  // Public Methods
  void print(ostream& out) const;
private:
  // Private Methods

  // Default copy constructor and assignment operator
  // MessageBufferNode(const MessageBufferNode& obj);
  // MessageBufferNode& operator=(const MessageBufferNode& obj);
  
  // Data Members (m_ prefix)
public:
  Time m_time;
  int m_msg_counter; // FIXME, should this be a 64-bit value?
  MsgPtr m_msgptr;  
};

// Output operator declaration
ostream& operator<<(ostream& out, const MessageBufferNode& obj);

// ******************* Definitions *******************

inline extern bool node_less_then_eq(const MessageBufferNode& n1, const MessageBufferNode& n2);

inline extern
bool node_less_then_eq(const MessageBufferNode& n1, const MessageBufferNode& n2)
{
  if (n1.m_time == n2.m_time) {
    assert(n1.m_msg_counter != n2.m_msg_counter);
    return (n1.m_msg_counter <= n2.m_msg_counter);
  } else {
    return (n1.m_time <= n2.m_time);
  }
}

// Output operator definition
extern inline 
ostream& operator<<(ostream& out, const MessageBufferNode& obj)
{
  obj.print(out);
  out << flush;
  return out;
}

#endif //MESSAGEBUFFERNODE_H
