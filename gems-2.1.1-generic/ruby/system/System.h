
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
 * System.h
 * 
 * Description: Contains all of the various parts of the system we are
 * simulating.  Performs allocation, deallocation, and setup of all
 * the major components of the system
 *
 * $Id$
 *
 */

#ifndef SYSTEM_H
#define SYSTEM_H

#include "Global.h"
#include "Vector.h"
#include "Address.h"
#include "RubyConfig.h"
#include "MachineType.h"
#include "AbstractChip.h"

class Profiler;
class Network;
class Driver;
class CacheRecorder;
class Tracer;
class Sequencer;
class XactIsolationChecker;
class XactCommitArbiter;
class XactVisualizer;
class TransactionInterfaceManager;

class System {
public:
  // Constructors
  System();

  // Destructor
  ~System();
  
  // Public Methods
  int getNumProcessors() { return RubyConfig::numberOfProcessors(); }
  int getNumMemories() { return RubyConfig::numberOfMemories(); }
  Profiler* getProfiler() { return m_profiler_ptr; }
  Driver* getDriver() { assert(m_driver_ptr != NULL); return m_driver_ptr; }
  Tracer* getTracer() { assert(m_tracer_ptr != NULL); return m_tracer_ptr; }
  Network* getNetwork() { assert(m_network_ptr != NULL); return m_network_ptr; }
  XactIsolationChecker* getXactIsolationChecker() { assert(m_xact_isolation_checker!= NULL); return m_xact_isolation_checker;} 
  XactCommitArbiter* getXactCommitArbiter() { assert(m_xact_commit_arbiter!= NULL); return m_xact_commit_arbiter;} 
  XactVisualizer*    getXactVisualizer() { assert(m_xact_visualizer!= NULL); return m_xact_visualizer;} 

  AbstractChip* getChip(int chipNumber) const { assert(m_chip_vector[chipNumber] != NULL); return m_chip_vector[chipNumber];}
  Sequencer* getSequencer(int procNumber) const { 
    assert(procNumber < RubyConfig::numberOfProcessors()); 
    return m_chip_vector[procNumber/RubyConfig::numberOfProcsPerChip()]->getSequencer(procNumber%RubyConfig::numberOfProcsPerChip());
  }
  TransactionInterfaceManager* getTransactionInterfaceManager(int procNumber) const {
    return m_chip_vector[procNumber/RubyConfig::numberOfProcsPerChip()]->getTransactionInterfaceManager(procNumber%RubyConfig::numberOfProcsPerChip());
  }          
  void recordCacheContents(CacheRecorder& tr) const;
  void printConfig(ostream& out) const;
  void printStats(ostream& out);
  void clearStats() const;

  // called to notify the system when opal is loaded
  void opalLoadNotify();

  void print(ostream& out) const;
#ifdef CHECK_COHERENCE
  void checkGlobalCoherenceInvariant(const Address& addr);
#endif
  
private:
  // Private Methods

  // Private copy constructor and assignment operator
  System(const System& obj);
  System& operator=(const System& obj);
  
  // Data Members (m_ prefix)
  Network* m_network_ptr;
  Vector<AbstractChip*> m_chip_vector;
  Profiler* m_profiler_ptr;
  Driver* m_driver_ptr;
  Tracer* m_tracer_ptr;
  XactIsolationChecker *m_xact_isolation_checker;
  XactCommitArbiter    *m_xact_commit_arbiter;
  XactVisualizer       *m_xact_visualizer;
};

// Output operator declaration
ostream& operator<<(ostream& out, const System& obj);

// ******************* Definitions *******************

// Output operator definition
inline 
ostream& operator<<(ostream& out, const System& obj)
{
//  obj.print(out);
  out << flush;
  return out;
}

#endif //SYSTEM_H



