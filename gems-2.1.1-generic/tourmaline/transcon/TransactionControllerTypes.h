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
 * TransactionControllerTypes.h
 *
 * Description: 
 * Enumerates the available transaction controller types. Provides an enum, names (as used in 
 * the configuration files), and descriptions of the transaction controllers.
 *
 * Take care when editing here that Serializer is always listed first (for iteration compatibility),
 * and that the Enum's order matches the name's order matches the description's order.
 * 
 * $Id: TransactionControllerTypes.h 1.00 05/11/09 14:02:03-05:00 gibson@mustardseed.cs.wisc.edu $
 *
 */

#ifndef TRANSACTIONCONTROLLERTYPES_H
#define TRANSACTIONCONTROLLERTYPES_H

/*
 * NOTE:
 * Serializer MUST appear first in the enum and name definition
 * for TourmalineConfig::controllerType() to work correctly.
 *
 * Additionally, the order a controller appears in the enum
 * should correspond to the order it appears in the name vector, 
 * and the order it appears in the description vector. 
 *
 * Be careful when adding controllers that the above invariant
 * is satisfied.
 *
 */

// Each transactional controller should have an enum entry here
enum TransactionControllerType {
  Transaction_Controller_Serializer = 0,
  Transaction_Controller_Generic,
/* Transaction_Controller_NewType, */
  Transaction_Controller_COUNT  /* Leave this in -- it is used as an iteration bound */
};

// Each transactional controller should have a name here
const char * const TransactionControllerNames[] = {
        "Serializer",
        "Generic",
/* "MyNewTransactionController", */
        "INVALID_CONTROLLER"
};

// usable for help messages (not yet implemented)
const char * const TransactionControllerDescriptions[] = {
"Serializer: Serializes transactions by disabling all other processors when any processor's \n\
transaction nesting level rises above zero. Re-enables other processors when the transaction\n\
nesting level returns to zero.\n",
"Generic: Observes memory references, dumbly aborting on any conflict.\n",

"INVALID_CONTROLLER: This variable is used as a loop iteration bound.\n"
};

// A little ugly, but it gets the job done
static inline TransactionControllerType operator++(TransactionControllerType& t, int val) {
  if(t!=Transaction_Controller_COUNT) {
    return ((TransactionControllerType) ( (int&) t )++);
  }

  return Transaction_Controller_COUNT;
}

#endif // TRANSACTIONCONTROLLERTYPES_H

