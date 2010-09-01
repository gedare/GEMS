
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
 * Description: A entry in the cache request record. It is aware of
 *              the ruby time and can issue the request back to the
 *              cache.
 *
 */

#ifndef TRACERECORD_H
#define TRACERECORD_H

#include "Global.h"
#include "Address.h"
#include "NodeID.h"
#include "CacheRequestType.h"
class CacheMsg;

class TraceRecord {
public:
  // Constructors
  TraceRecord(NodeID id, const Address& data_addr, const Address& pc_addr, CacheRequestType type, Time time);
  TraceRecord() { m_node_num = 0; m_time = 0; m_type = CacheRequestType_NULL; }

  // Destructor
  //  ~TraceRecord();
  
  // Public copy constructor and assignment operator
  TraceRecord(const TraceRecord& obj);
  TraceRecord& operator=(const TraceRecord& obj);

  // Public Methods
  bool node_less_then_eq(const TraceRecord& rec) const { return (this->m_time <= rec.m_time); }
  void issueRequest() const;

  void print(ostream& out) const;
  void output(ostream& out) const;
  bool input(istream& in);
private:
  // Private Methods
  
  // Data Members (m_ prefix)
  NodeID m_node_num;
  Time m_time;
  Address m_data_address;
  Address m_pc_address;
  CacheRequestType m_type;
};

inline extern bool node_less_then_eq(const TraceRecord& n1, const TraceRecord& n2);

// Output operator declaration
ostream& operator<<(ostream& out, const TraceRecord& obj);

// ******************* Definitions *******************

inline extern
bool node_less_then_eq(const TraceRecord& n1, const TraceRecord& n2)
{
  return n1.node_less_then_eq(n2);
}

// Output operator definition
extern inline 
ostream& operator<<(ostream& out, const TraceRecord& obj)
{
  obj.print(out);
  out << flush;
  return out;
}

#endif //TRACERECORD_H
