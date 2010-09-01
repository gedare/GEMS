
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

#ifndef PersistentTable_H
#define PersistentTable_H

#include "Global.h"
#include "MachineID.h"
#include "AccessType.h"

class AbstractChip;

template <class KEY_TYPE, class VALUE_TYPE> class Map;
class Address;
class PersistentTableEntry;

class PersistentTable {
public:
  // Constructors
  PersistentTable(AbstractChip* chip_ptr, int version);

  // Destructor
  ~PersistentTable();
  
  // Public Methods
  void persistentRequestLock(const Address& address, MachineID locker, AccessType type);
  void persistentRequestUnlock(const Address& address, MachineID unlocker);
  bool okToIssueStarving(const Address& address) const;
  MachineID findSmallest(const Address& address) const;
  AccessType typeOfSmallest(const Address& address) const;
  void markEntries(const Address& address);
  bool isLocked(const Address& addr) const;
  int countStarvingForAddress(const Address& addr) const;
  int countReadStarvingForAddress(const Address& addr) const;

  static void printConfig(ostream& out) {}

  void print(ostream& out) const;
private:
  // Private Methods

  // Private copy constructor and assignment operator
  PersistentTable(const PersistentTable& obj);
  PersistentTable& operator=(const PersistentTable& obj);

  // Data Members (m_prefix)
  Map<Address, PersistentTableEntry>* m_map_ptr;
  AbstractChip* m_chip_ptr;
  int m_version;
};

// Output operator declaration
ostream& operator<<(ostream& out, const PersistentTable& obj);

// ******************* Definitions *******************

// Output operator definition
extern inline 
ostream& operator<<(ostream& out, const PersistentTable& obj)
{
  obj.print(out);
  out << flush;
  return out;
}

#endif //PersistentTable_H
