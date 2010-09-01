
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

#ifndef ACCESSTRACEFORADDRESS_H
#define ACCESSTRACEFORADDRESS_H

#include "Global.h"
#include "RubyConfig.h"
#include "Address.h"
#include "CacheRequestType.h"
#include "AccessModeType.h"
#include "NodeID.h"
#include "Set.h"
class Histogram;

class AccessTraceForAddress {
public:
  // Constructors
  AccessTraceForAddress();
  explicit AccessTraceForAddress(const Address& addr);

  // Destructor
  ~AccessTraceForAddress();
  
  // Public Methods

  void update(CacheRequestType type, AccessModeType access_mode, NodeID cpu, bool sharing_miss);
  int getTotal() const;
  int getSharing() const { return m_sharing; } 
  int getTouchedBy() const { return m_touched_by.count(); } 
  const Address& getAddress() const { return m_addr; }
  void addSample(int value);

  void print(ostream& out) const;
private:
  // Private Methods

  // Private copy constructor and assignment operator
  // AccessTraceForAddress(const AccessTraceForAddress& obj);
  // AccessTraceForAddress& operator=(const AccessTraceForAddress& obj);
  
  // Data Members (m_ prefix)

  Address m_addr;
  uint64 m_loads;
  uint64 m_stores;
  uint64 m_atomics;
  uint64 m_total;
  uint64 m_user;
  uint64 m_sharing;
  Set m_touched_by;
  Histogram* m_histogram_ptr;
};

bool node_less_then_eq(const AccessTraceForAddress* n1, const AccessTraceForAddress* n2);

// Output operator declaration
ostream& operator<<(ostream& out, const AccessTraceForAddress& obj);

// ******************* Definitions *******************

// Output operator definition
extern inline 
ostream& operator<<(ostream& out, const AccessTraceForAddress& obj)
{
  obj.print(out);
  out << flush;
  return out;
}

#endif //ACCESSTRACEFORADDRESS_H
