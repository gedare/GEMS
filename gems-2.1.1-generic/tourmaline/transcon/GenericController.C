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
 * GenericController.C
 *
 * Description: 
 * See GenericController.h
 * 
 * $Id: GenericController.C 1.00 05/11/09 14:02:03-05:00 gibson@mustardseed.cs.wisc.edu $
 *
 */



/*                              ____                 ________             _______
 * ||   ||   ||        /\       ||  \\     |\    ||  |__ ___|  |\    ||  | _____|
 * ||   ||   ||       /  \      ||   ||    ||\   ||     ||     ||\   ||  | |
 * \\   ||   //      / /\ \     ||__//     || \  ||     ||     || \  ||  | |  __
 *  \\  ||  //      / /__\ \    ||  \\     ||  \ ||   __||__   ||  \ ||  | |_|  |        
 *   \\_||_//      /_/    \_\   ||   \\    ||   \||  |______|  ||   \||  |______|
 *
 * This module is not yet complete. It does *NOT* implement any transactional guarantees.
 * It *DOES* show how to enable and disable the timing interface, and is included as an
 * example.
 */

#include "Tourmaline_Global.h"
#include "GenericController.h"
#include "Profiler.h"
#include "interface.h"

// intended constructor
GenericController::GenericController() : TransactionController() {

  /* All Transaction Controllers Should Execute the following code */
  /* [Begin] */
        
  m_type = Transaction_Controller_Generic;  // Substitute proper enum here (define in TransactionControllerTypes.h)
  printConfiguration(cout);
  
  /* [End] */
  
  // The timing interface is *NOT* installed on startup
  m_timing_interface_installed = false;
}

GenericController::~GenericController() {
  
}

void GenericController::beginTransaction(int proc, int xactno) {

  // sanity checks
  assert(proc>=0);
  assert(proc<m_nProcs);
  
  /* All Transaction Controllers Should Execute the following code */
  /* [Begin] */
  
  g_profiler->beginTransaction(proc);
  transaction_depth[proc]++;
  
  /* [End] */
  
  WARNING(" The Generic module is not complete: It does not ensure transactional properties. ");
  
  // if this is the only processor in a transaction, we need to enable
  // the timing_model interface
  if(!m_timing_interface_installed) {

    // there is something wrong if another processor is in a transaction
    for(int i=0;i<m_nProcs;i++) {
      if(i==proc) continue;
      assert(transaction_depth[i]==0);
    }
          
    SIMICS_install_timing_model();
    m_timing_interface_installed = true;
  }
  
}

void GenericController::endTransaction(int proc, int xactno) {
  
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

  // if no processors are in a transaction anymore, uninstall the
  // timing model for speed!
  bool still_transactional = false;
  for(int i=0;i<m_nProcs;i++) {
    if(transaction_depth[i]>0) {
      still_transactional = true;
      break;
    }
  }

  if(!still_transactional && m_timing_interface_installed) {
    SIMICS_remove_timing_model();
    m_timing_interface_installed = false;
  }

}


void GenericController::makeRequest(const MemoryAccess& access) {
  assert(m_timing_interface_installed);
}

// This Transaction controller does not use the finishRequest() interface
void GenericController::finishRequest(MemoryAccess& access) {
  assert(0);
}

