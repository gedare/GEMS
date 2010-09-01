
/*
    Copyright (C) 1999-2008 by Mark D. Hill and David A. Wood for the
    Wisconsin Multifacet Project.  Contact: gems@cs.wisc.edu
    http://www.cs.wisc.edu/gems/

    --------------------------------------------------------------------

    This file is part of the SLICC (Specification Language for 
    Implementing Cache Coherence), a component of the Multifacet GEMS 
    (General Execution-driven Multiprocessor Simulator) software 
    toolset originally developed at the University of Wisconsin-Madison.

    SLICC was originally developed by Milo Martin with substantial
    contributions from Daniel Sorin.

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
 * Type.h
 * 
 * Description: 
 *
 * $Id$
 *
 * */

#ifndef TYPE_H
#define TYPE_H

#include "slicc_global.h"
#include "Map.h"
#include "Symbol.h"

class StateMachine;

class Type : public Symbol {
public:
  // Constructors
  Type(string id, const Location& location, 
       const Map<string, string>& pairs, 
       StateMachine* machine_ptr = NULL);

  // Destructor
  ~Type() {}
  
  // Public Methods
  string cIdent() const { return m_c_id; }
  string desc() const { return m_desc; }

  bool isPrimitive() const { return existPair("primitive"); }
  bool isNetworkMessage() const { return existPair("networkmessage"); }
  bool isMessage() const { return existPair("message"); }
  bool isBuffer() const { return existPair("buffer"); }
  bool isInPort() const { return existPair("inport"); }
  bool isOutPort() const { return existPair("outport"); }
  bool isEnumeration() const { return existPair("enumeration"); }
  bool isExternal() const { return existPair("external"); }
  bool isGlobal() const { return existPair("global"); }
  
  // The data members of this type - only valid for messages and SLICC
  // declared structures
  // Return false on error
  bool dataMemberAdd(string id, Type* type_ptr, Map<string, string>& pairs,
                     string* init_code);
  bool dataMemberExist(string id) const { return m_data_member_map.exist(id); }
  Type* dataMemberType(string id) const { return m_data_member_map.lookup(id); }

  // The methods of this type - only valid for external types
  // Return false on error
  bool methodAdd(string name, Type* return_type_ptr, const Vector<Type*>& param_type_vec);
  bool methodExist(string id) const { return m_method_return_type_map.exist(id); }

  string methodId(string name, const Vector<Type*>& param_type_vec);
  Type* methodReturnType(string id) const { return m_method_return_type_map.lookup(id); }
  const Vector<Type*>& methodParamType(string id) const { return m_method_param_type_map.lookup(id); }
  
  // The enumeration idents of this type - only valid for enums
  // Return false on error
  bool enumAdd(string id, Map<string, string> pairs);
  bool enumExist(string id) const { return m_enum_map.exist(id); }

  // Write the C output files
  void writeCFiles(string path) const;

  bool hasDefault() const { return existPair("default"); }
  string getDefault() const { return lookupPair("default"); }

  void print(ostream& out) const {}
private:
  // Private Methods

  void printTypeH(string path) const;
  void printTypeC(string path) const;
  void printEnumC(string path) const;
  void printEnumH(string path) const;

  // Private copy constructor and assignment operator
  Type(const Type& obj);
  Type& operator=(const Type& obj);
  
  // Data Members (m_ prefix)
  string m_c_id;
  string m_desc;
  
  // Data Members
  Map<string, Type*> m_data_member_map;
  Vector<string> m_data_member_ident_vec;
  Vector<Type*> m_data_member_type_vec;
  Vector<Map<string, string> > m_data_member_pairs_vec;
  Vector<string*> m_data_member_init_code_vec;
  // Needs pairs here

  // Methods
  Map<string, Type*> m_method_return_type_map;
  Map<string, Vector<Type*> > m_method_param_type_map;
  // Needs pairs here

  // Enum
  Map<string, bool> m_enum_map;
  Vector<string> m_enum_vec;
  Vector< Map < string, string > > m_enum_pairs;
  
  // MachineType Hack
  bool m_isMachineType;
  
};

// Output operator declaration
ostream& operator<<(ostream& out, const Type& obj);

// ******************* Definitions *******************

// Output operator definition
extern inline 
ostream& operator<<(ostream& out, const Type& obj)
{
  obj.print(out);
  out << flush;
  return out;
}

#endif //TYPE_H
