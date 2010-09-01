
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
 */

#ifndef MESSAGE_H
#define MESSAGE_H

#include "Global.h"
#include "RefCnt.h"
#include "RefCountable.h"
#include "EventQueue.h"

class Message;
typedef RefCnt<Message> MsgPtr;

class Message : public RefCountable {
public:
  // Constructors
  Message() : RefCountable() { m_time = g_eventQueue_ptr->getTime();  m_LastEnqueueTime = g_eventQueue_ptr->getTime(); m_DelayedCycles = 0;}

  // Destructor
  virtual ~Message() { }
  
  // Public Methods
  virtual Message* clone() const = 0;
  virtual void destroy() = 0;
  virtual void print(ostream& out) const = 0;

  void setDelayedCycles(const int& cycles) { m_DelayedCycles = cycles; }
  const int& getDelayedCycles() const {return m_DelayedCycles;}
  int& getDelayedCycles() {return m_DelayedCycles;}
  void setLastEnqueueTime(const Time& time) { m_LastEnqueueTime = time; }
  const Time& getLastEnqueueTime() const {return m_LastEnqueueTime;}
  Time& getLastEnqueueTime() {return m_LastEnqueueTime;}

  const Time& getTime() const { return m_time; }
  void setTime(const Time& new_time) { m_time = new_time; }
private:
  // Private Methods

  // Data Members (m_ prefix)
  Time m_time;
  Time m_LastEnqueueTime; // my last enqueue time
  int m_DelayedCycles; // my delayed cycles

};

// Output operator declaration
ostream& operator<<(ostream& out, const Message& obj);

// ******************* Definitions *******************

// Output operator definition
extern inline 
ostream& operator<<(ostream& out, const Message& obj)
{
  obj.print(out);
  out << flush;
  return out;
}

#endif //MESSAGE_H
