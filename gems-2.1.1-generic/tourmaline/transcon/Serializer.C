/*
    Copyright (C) 1999-2008 by Mark D. Hill and David A. Wood for the
    Wisconsin Multifacet Project.  Contact: gems@cs.wisc.edu
    http://www.cs.wisc.edu/gems/

    --------------------------------------------------------------------

    This file is part of the Tourmaline Transactional Memory Acclerator, 
    a component of the Multifacet GEMS (General Execution-driven 
    Multiprocessor Simulator) software toolset originally developed at 
    the University of Wisconsin-Madison.

    Tourmaline was originally developed primarily by Dan Gibson, but was
    based on work in the Ruby module performed by Milo Martin and Daniel
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
 * Serializer.C
 *
 * Description: 
 * See Serializer.h
 * 
 * $Id: Serializer.C 1.00 05/11/09 14:02:03-05:00 gibson@mustardseed.cs.wisc.edu $
 *
 */

#include "Tourmaline_Global.h"
#include "Serializer.h"
#include "Profiler.h"
#include "interface.h"

// intended constructor
Serializer::Serializer() : TransactionController() {

  /* All Transaction Controllers Should Execute the following code */
  /* [Begin] */
        
  m_type = Transaction_Controller_Serializer;  // Substitute proper enum here (define in TransactionControl.h)
  printConfiguration(cout);
  
  /* [End] */
  
}

Serializer::~Serializer() {
  
}

void Serializer::beginTransaction(int proc, int xactno) {

  // sanity checks
  assert(proc>=0);
  assert(proc<m_nProcs);
  
  /* All Transaction Controllers Should Execute the following code */
  /* [Begin] */
  
  g_profiler->beginTransaction(proc);
  transaction_depth[proc]++;
  
  /* [End] */

  if(transaction_depth[proc]==1) {
    // just entered a transaction -- disable all other processors
    for(int i=0; i<m_nProcs;i++) {
      if(i==proc) continue;
      
      // only disable processors that are currently enabled
      if(SIMICS_cpu_enabled(i)) {
        SIMICS_disable_processor(i);
      } else {
        WARNING( "Serializer intended to disable processor %i, but found it was already disabled.\n",i);
      }
      
      // If >1 processor is in a transaction, something is wrong
      assert(transaction_depth[i]==0);
    }
  }
  
}

void Serializer::endTransaction(int proc, int xactno) {
  
  // sanity checks
  assert(proc>=0);
  assert(proc<m_nProcs);

  /* All Transaction Controllers Should Execute the following code */
  /* [Begin] */
  
  g_profiler->endTransaction(proc);
  transaction_depth[proc]--;

  if(transaction_depth[proc]<0) {
    WARNING( "Unmatched \"end_transaction\" encountered, processor %i, SIMICS_cycle_count = %i\n",
                    proc, SIMICS_cycle_count(proc));
    transaction_depth[proc]=0;
    return;
  }  
  
  /* [End] */
  
  if(transaction_depth[proc]==0) {
    // done! wake up the other processors
    for(int i=0; i<m_nProcs; i++) {
      if(i==proc) continue;

      // only enable processors that are currently disabled
      if(!SIMICS_cpu_enabled(i)) {
        SIMICS_enable_processor(i);
      } else {
        WARNING( "Serializer intended to enable processor %i, but found it was already enabled.\n",i);
      }
      
      // If >1 processor is in a transaction, something is wrong
      assert(transaction_depth[i]==0);
    }
  }
}

/*
 * This function does nothing, as this transaction controller
 * never enables the timing model
 */
void Serializer::makeRequest(const MemoryAccess& access) {
 assert(0);
}

/*
 * This function does nothing, as this transaction controller
 * never enables the snoop model
 */
void Serializer::finishRequest(MemoryAccess& access) {
 assert(0);
}
