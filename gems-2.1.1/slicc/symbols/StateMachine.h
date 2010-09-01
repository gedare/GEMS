
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

#ifndef STATEMACHINE_H
#define STATEMACHINE_H

#include "slicc_global.h"
#include "Vector.h"
#include "Map.h"
#include "Symbol.h"

class Transition;
class Event;
class State;
class Action;
class Var;
class Func;

class StateMachine : public Symbol {
public:
  // Constructors
  StateMachine(string ident, const Location& location, const Map<string, string>& pairs);

  // Destructor
  ~StateMachine();
  
  // Public Methods

  // Add items to the state machine
  //  void setMachine(string ident, const Map<string, string>& pairs);
  void addState(State* state_ptr);
  void addEvent(Event* event_ptr);
  void addAction(Action* action_ptr);
  void addTransition(Transition* trans_ptr);
  void addInPort(Var* var) { m_in_ports.insertAtBottom(var); }
  void addFunc(Func* func);

  // Accessors to vectors
  const State& getState(int index) const { return *m_states[index]; }
  const Event& getEvent(int index) const { return *m_events[index]; }
  const Action& getAction(int index) const { return *m_actions[index]; }
  const Transition& getTransition(int index) const { return *m_transitions[index]; }
  const Transition* getTransPtr(int stateIndex, int eventIndex) const;

  // Accessors for size of vectors
  int numStates() const { return m_states.size(); }
  int numEvents() const { return m_events.size(); }
  int numActions() const { return m_actions.size(); }
  int numTransitions() const { return m_transitions.size(); }

  void buildTable();  // Needs to be called before accessing the table

  // Code generator methods
  void writeCFiles(string path) const;
  void writeHTMLFiles(string path) const;

  void print(ostream& out) const { out << "[StateMachine: " << toString() << "]" << endl; }
private:
  // Private Methods
  void checkForDuplicate(const Symbol& sym) const;

  int getStateIndex(State* state_ptr) const { return m_state_map.lookup(state_ptr); }
  int getEventIndex(Event* event_ptr) const { return m_event_map.lookup(event_ptr); }

  // Private copy constructor and assignment operator 
  //  StateMachine(const StateMachine& obj);
  //  StateMachine& operator=(const StateMachine& obj);
  
  void printControllerH(ostream& out, string component) const;
  void printControllerC(ostream& out, string component) const;
  void printCWakeup(ostream& out, string component) const;
  void printCSwitch(ostream& out, string component) const;
  void printProfilerH(ostream& out, string component) const;
  void printProfilerC(ostream& out, string component) const;

  void printHTMLTransitions(ostream& out, int active_state) const;

  // Data Members (m_ prefix)
  Vector<State*> m_states;
  Vector<Event*> m_events;
  Vector<Action*> m_actions;
  Vector<Transition*> m_transitions;
  Vector<Func*> m_internal_func_vec;

  Map<State*, int> m_state_map;
  Map<Event*, int> m_event_map;

  Vector<Var*> m_in_ports;

  // Table variables
  bool m_table_built;
  Vector<Vector<Transition*> > m_table;

};

// Output operator declaration
ostream& operator<<(ostream& out, const StateMachine& obj);

// ******************* Definitions *******************

// Output operator definition
extern inline 
ostream& operator<<(ostream& out, const StateMachine& obj)
{
  obj.print(out);
  out << flush;
  return out;
}

#endif //STATEMACHINE_H
