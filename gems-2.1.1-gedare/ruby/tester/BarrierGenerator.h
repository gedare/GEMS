
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
 * Description: 
 *
 */

#ifndef BARRIERGENERATOR_H
#define BARRIERGENERATOR_H

#include "Global.h"
#include "Consumer.h"
#include "NodeID.h"
#include "Address.h"

class Sequencer;
class SubBlock;
class SyntheticDriver;


enum BarrierGeneratorStatus {
  BarrierGeneratorStatus_FIRST,
  BarrierGeneratorStatus_Thinking = BarrierGeneratorStatus_FIRST,
  BarrierGeneratorStatus_Test_Pending,
  BarrierGeneratorStatus_Test_Waiting,
  BarrierGeneratorStatus_Before_Swap,
  BarrierGeneratorStatus_Swap_Pending,
  BarrierGeneratorStatus_Holding,
  BarrierGeneratorStatus_Release_Pending,
  BarrierGeneratorStatus_Release_Waiting,
  BarrierGeneratorStatus_StoreFlag_Waiting,
  BarrierGeneratorStatus_StoreFlag_Pending,
  BarrierGeneratorStatus_Done,
  BarrierGeneratorStatus_SpinFlag_Ready,
  BarrierGeneratorStatus_SpinFlag_Pending,
  BarrierGeneratorStatus_LoadBarrierCounter_Pending,
  BarrierGeneratorStatus_StoreBarrierCounter_Pending,
  BarrierGeneratorStatus_StoreBarrierCounter_Waiting,
  BarrierGeneratorStatus_NUM
};


// UNCOMMENT THIS FOR A SINGLE WORK QUEUE
// static int m_counter;

class BarrierGenerator : public Consumer {
public:
  // Constructors
  BarrierGenerator(NodeID node, SyntheticDriver& driver);

  // Destructor
  ~BarrierGenerator();
  
  // Public Methods
  void wakeup();
  void performCallback(NodeID proc, SubBlock& data);

  void print(ostream& out) const;
private:
  // Private Methods
  int thinkTime() ;
  int waitTime() const;
  void initiateTest();
  void initiateSwap();
  void initiateRelease();
  void initiateLoadCtr();
  void initiateStoreCtr();
  void initiateLoadFlag();
  void initiateStoreFlag();
  Sequencer* sequencer() const;

  // Private copy constructor and assignment operator
  BarrierGenerator(const BarrierGenerator& obj);
  BarrierGenerator& operator=(const BarrierGenerator& obj);
  
  // Data Members (m_ prefix)
  SyntheticDriver& m_driver;
  NodeID m_node;
  BarrierGeneratorStatus m_status;
  int proc_counter;

  int m_counter;

  bool m_local_sense;
  bool m_barrier_done;

  Time m_last_transition;
  Address m_address;

  int m_total_think;
  int m_think_periods;
};

// Output operator declaration
ostream& operator<<(ostream& out, const BarrierGenerator& obj);

// ******************* Definitions *******************

// Output operator definition
extern inline 
ostream& operator<<(ostream& out, const BarrierGenerator& obj)
{
  obj.print(out);
  out << flush;
  return out;
}

#endif //REQUESTGENERATOR_H

