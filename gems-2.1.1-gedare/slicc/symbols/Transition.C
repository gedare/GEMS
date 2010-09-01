
/*
    Copyright (C) 1999-2008 by Mark D. Hill and David A. Wood for the
    Wisconsin Multifacet Project.  Contact: gems@cs.wisc.edu
    http://www.cs.wisc.edu/gems/

    --------------------------------------------------------------------

    This file is part of the SLICC (Specification Language for 
    Implementing Cache Coherence), a component of the Multifacet GEMS 
    (General Execution-driven Multiprocessor Simulator) software 
    toolset originally developed at the University of Wisconsin-Madison.

    SLICC was originally developed by Milo Martin with substantial
    contributions from Daniel Sorin.

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
 * */

#include "Transition.h"
#include "State.h"
#include "Event.h"
#include "Action.h"
#include "util.h"
#include "Var.h"

Transition::Transition(string state, string event, string nextState,
                       const Vector<string>& actionList,
                       const Location& location,
                       const Map<string, string>& pairMap)
  : Symbol(state + "|" + event, location, pairMap)
{
  m_state = state;
  m_event = event;
  m_nextState = nextState;
  m_actionList = actionList;

  // Ptrs are undefined at this point
  m_statePtr = NULL;
  m_eventPtr = NULL;
  m_nextStatePtr = NULL;
  m_actionPtrsValid = false;
}

void Transition::checkIdents(const Vector<State*>& states, 
                             const Vector<Event*>& events, 
                             const Vector<Action*>& actions)
{
  m_statePtr = findIndex(states, m_state);
  m_eventPtr = findIndex(events, m_event);
  m_nextStatePtr = findIndex(states, m_nextState);

  for(int i=0; i < m_actionList.size(); i++) {
    Action* action_ptr = findIndex(actions, m_actionList[i]);
    int size = action_ptr->getResources().keys().size();
    for (int j=0; j < size; j++) {
      Var* var_ptr = action_ptr->getResources().keys()[j];
      if (var_ptr->getType()->cIdent() != "DNUCAStopTable") {
        int num = atoi((action_ptr->getResources().lookup(var_ptr)).c_str());
        if (m_resources.exist(var_ptr)) {
          num += atoi((m_resources.lookup(var_ptr)).c_str());
        }
        m_resources.add(var_ptr, int_to_string(num));
      } else {
        m_resources.add(var_ptr, action_ptr->getResources().lookup(var_ptr));
      }
    }
    m_actionPtrs.insertAtBottom(action_ptr);
  }
  m_actionPtrsValid = true;
}

const string& Transition::getStateShorthand() const
{
  assert(m_statePtr != NULL);
  return m_statePtr->getShorthand();
}

const string& Transition::getEventShorthand() const
{
  assert(m_eventPtr != NULL);
  return m_eventPtr->getShorthand();
}

const string& Transition::getNextStateShorthand() const
{
  assert(m_nextStatePtr != NULL);
  return m_nextStatePtr->getShorthand();
}

string Transition::getActionShorthands() const
{
  assert(m_actionPtrsValid);
  string str;
  int numActions = m_actionPtrs.size();
  for (int i=0; i<numActions; i++) {
    str += m_actionPtrs[i]->getShorthand();
  }
  return str;
}

void Transition::print(ostream& out) const
{
  out << "[Transition: ";
  out << "(" << m_state;
  if (m_statePtr != NULL) {
    out << ":" << *m_statePtr;
  }
  out << ", " << m_event;
  if (m_eventPtr != NULL) {
    out << ":" << *m_eventPtr;
  }
  out << ") -> ";
  out << m_nextState;
  if (m_nextStatePtr != NULL) {
    out << ":" << *m_nextStatePtr;
  }
  out << ", ";
  out << m_actionList;
  out << "]";
}

Event* Transition::findIndex(const Vector<Event*>& vec, string ident)
{
  int size = vec.size();
  for(int i=0; i<size; i++) {
    if (ident == vec[i]->getIdent()) {
      return vec[i];
    }
  }
  error("Event not found: " + ident);
  return NULL;
}

State* Transition::findIndex(const Vector<State*>& vec, string ident)
{
  int size = vec.size();
  for(int i=0; i<size; i++) {
    if (ident == vec[i]->getIdent()) {
      return vec[i];
    }
  }
  error("State not found: " + ident);
  return NULL;
}

Action* Transition::findIndex(const Vector<Action*>& vec, string ident)
{
  int size = vec.size();
  for(int i=0; i<size; i++) {
    if (ident == vec[i]->getIdent()) {
      return vec[i];
    }
  }
  error("Action not found: " + ident);
  return NULL;
}
  
