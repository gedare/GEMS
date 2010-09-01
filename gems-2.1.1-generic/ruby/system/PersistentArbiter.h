
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
 * PersistentArbiter.h
 * 
 * Description: 
 *
 * Used for hierarchical distributed persistent request scheme
 *
 */

#ifndef PERSISTENTARBITER_H
#define PERSISTENTARBITER_H

#include "Global.h"
#include "Vector.h"
#include "AbstractChip.h"
#include "AccessPermission.h"
#include "AccessType.h"
#include "RubyConfig.h"
#include "Address.h"
#include "interface.h"

struct ArbiterEntry {
  bool valid;
  Address address;
  AccessType type;
  NodeID localId;
};

class PersistentArbiter {
public:

  // Constructors
  PersistentArbiter(AbstractChip* chip_ptr);

  // Destructor
  ~PersistentArbiter();
  
  // Public Methods

  void addLocker(NodeID id, Address addr, AccessType type);
  void removeLocker(NodeID id);
  bool successorRequestPresent(Address addr, NodeID id);
  bool lockersExist();
  void advanceActiveLock();
  Address getActiveLockAddress();
  NodeID getArbiterId();
  bool isBusy();

  void setIssuedAddress(Address addr);
  bool isIssuedAddress(Address addr);


  Address getIssuedAddress() { return m_issued_address; }

  static void printConfig(ostream& out) {}
  void print(ostream& out) const;

  NodeID getActiveLocalId();

private:
 
  Address m_issued_address;
  AbstractChip* m_chip_ptr;
  int m_locker;
  bool m_busy;
  Vector<ArbiterEntry> m_entries;
};

// Output operator definition
extern inline
ostream& operator<<(ostream& out, const PersistentArbiter& obj)
{
  obj.print(out);
  out << flush;
  return out;
}


#endif //PERFECTCACHEMEMORY_H
