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
 * TransactionControl.C
 *
 * Description:
 * See TransactionControl.h
 *
 * $Id: TransactionControl.C 1.00 05/11/09 14:02:03-05:00 gibson@mustardseed.cs.wisc.edu $
 *
 */

#include "Tourmaline_Global.h"
#include "TourmalineConfig.h"
#include "TransactionControl.h"
#include "Serializer.h"
#include "GenericController.h"

/* dummy constructor */
TransactionController::TransactionController() {
  m_nProcs = TourmalineConfig::numberOfProcessors();
  m_type   = Transaction_Controller_COUNT; // dummy flag value 
  transaction_depth.setSize(m_nProcs);

  for(int i=0;i<m_nProcs;i++) {
    transaction_depth[i] = 0;
  }
}

/* dummy destructor */
TransactionController::~TransactionController() {
        
}

void TransactionController::printConfiguration(ostream& out) {
  cout << "Controller_type: " << TransactionControllerNames[m_type] << endl;
  cout << "number_of_procs: " << m_nProcs << endl;
}

TransactionController* allocateTransactionController() {
  // make a decision based on configuration as to what type
  // of TransactionController to allocate
  TransactionController * t = NULL; 
  
  switch( TourmalineConfig::controllerType() ) {
  case Transaction_Controller_Serializer:
    t = new Serializer();
    break;
  case Transaction_Controller_Generic:
    t = new GenericController();
    break;
  /* Add other transaction controllers here */
  case Transaction_Controller_COUNT:
    // unrecognized transaction controller
    t = NULL;
    break;
  }
  
  return t;
}

