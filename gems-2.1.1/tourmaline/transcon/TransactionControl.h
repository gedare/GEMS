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
 * TransactionControl.h
 *
 * Description: 
 * Defines the common interface that all Transaction Controllers must implement. 
 * Specifically, each Transaction controller must have the following functions:
 *  --> Destructor
 *  --> void beginTransaction(int proc, int xactno=0)
 *      This will be called when a processor executes a magic instruction corresponding
 *      to the beginning of a transaction. The transaction number and processor number
 *      will be provided.
 *  --> void endTransaction(int proc, int xactno=0)
 *      This function will be called in response to a processor ending a transaction.
 *  --> void makeRequest(const MemoryAccess& access)
 *      If the transactional controller has enabled the timing interface via
 *      SIMICS_install_timing_model(), Memory requests will be passed to the 
 *      Transaction Controller BEFORE they execute. The access cannot be modified,
 *      though store values may be read if desired. This function will not be called
 *      if the timing interface has not been installed.
 *  --> void finishRequest(MemoryAccess& access)
 *      If the transaction controller has enabled the snoop interface via the
 *      SIMICS_install_snoop_model() call, Memory requests will be passed to the
 *      transaction controller AFTER they complete. This gives the transactional 
 *      controller an opportunity to 1) observe memory requests, if desired. 2) Modify
 *      the values returned by loads, if desired.
 *      This function will not be called if the snoop interface has not been installed.
 * 
 * $Id: TransactionControl.h 1.00 05/11/09 14:02:03-05:00 gibson@mustardseed.cs.wisc.edu $
 *
 */

#ifndef TRANSACTIONCONTROL_H
#define TRANSACTIONCONTROL_H

// defines types and names of transaction controllers
#include "TransactionControllerTypes.h"
#include "Vector.h"
#include "MemoryAccess.h"

class TransactionController {
public:
  TransactionController();
  virtual ~TransactionController()=0;

  /* BEGIN Required TransactionController interface */

  // Called when a transaction begins
  virtual void beginTransaction(int proc, int xactno=0) = 0;

  // Called when a transaction ends
  virtual void endTransaction(int proc, int xactno=0) = 0;

  // Called when simics initiates a memory access--this is 
  // BEFORE the access actually occurs. Data cannot be
  // written to parameter (cannot change the value of
  // a store), but if request is of type ST or ATOMIC_W, 
  // the data may be read
  // Note: Called only if timing interface is enabled
  // via SIMICS_install_timing_model()
  virtual void makeRequest(const MemoryAccess& access) = 0;

  // Called when simics completes a memory access--this is
  // AFTER the access actually occurs. Data can be read or
  // written to parameter (that is, can modify the value
  // returned by a load, if desired)
  // Note: Called only if snoop interface is enabled
  // via SIMICS_install_snoop_model()
  virtual void finishRequest(MemoryAccess& access) = 0;

  /* END Required TransactionController interface */

  void printConfiguration(ostream& out);

protected:

  int m_nProcs;
  TransactionControllerType m_type;
  Vector<int> transaction_depth;

};

// does allocation of transaction controller, creating the type specified by
// the configuration, so init.C doesn't need to know about the different types
// of TransactionController
TransactionController* allocateTransactionController();

#endif

