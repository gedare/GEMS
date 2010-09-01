
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
 * Transition.h
 * 
 * Description: 
 *
 * $Id$
 *
 * */

#ifndef TRANSITION_H
#define TRANSITION_H

#include "slicc_global.h"
#include "Vector.h"
#include "Symbol.h"

class State;
class Event;
class Action;
class Var;

class Transition : public Symbol {
public:
  // Constructors
  Transition(string state, string event, string nextState,
             const Vector<string>& actionList,
             const Location& location,
             const Map<string, string>& pairMap);
  // Destructor
  ~Transition() { }
  
  // Public Methods
  State* getStatePtr() const { assert(m_statePtr != NULL); return m_statePtr; }
  Event* getEventPtr() const { assert(m_eventPtr != NULL); return m_eventPtr; }
  State* getNextStatePtr() const { assert(m_nextStatePtr != NULL); return m_nextStatePtr; }

  //  int getStateIndex() const { assert(m_statePtr != NULL); return m_statePtr->getIndex(); }
  //  int getEventIndex() const { assert(m_eventPtr != NULL); return m_eventPtr->getIndex(); }
  //  int getNextStateIndex() const { assert(m_nextStatePtr != NULL); return m_nextStatePtr->getIndex(); }
  void checkIdents(const Vector<State*>& states, 
                   const Vector<Event*>& events, 
                   const Vector<Action*>& actions);

  const string& getStateShorthand() const;
  const string& getEventShorthand() const;
  const string& getNextStateShorthand() const;
  string getActionShorthands() const;
  const Vector<Action*>& getActions() const { assert(m_actionPtrsValid); return m_actionPtrs; }
  const Map<Var*, string>& getResources() const { assert(m_actionPtrsValid); return m_resources; }

  void print(ostream& out) const;

  // Default copy constructor and assignment operator
  // Transition(const Transition& obj);
  // Transition& operator=(const Transition& obj);
private:
  // Private Methods
  Event* findIndex(const Vector<Event*>& vec, string ident);
  State* findIndex(const Vector<State*>& vec, string ident);
  Action* findIndex(const Vector<Action*>& vec, string ident);
  
  // Data Members (m_ prefix)
  string m_state;
  string m_event;
  string m_nextState;

  State* m_statePtr;
  Event* m_eventPtr;
  State* m_nextStatePtr;

  Vector<string> m_actionList;
  Vector<Action*> m_actionPtrs;
  Map<Var*, string> m_resources;
  bool m_actionPtrsValid;
};

// Output operator declaration
ostream& operator<<(ostream& out, const Transition& obj);

// ******************* Definitions *******************

// Output operator definition
extern inline 
ostream& operator<<(ostream& out, const Transition& obj)
{
  obj.print(out);
  out << flush;
  return out;
}

#endif //TRANSITION_H
