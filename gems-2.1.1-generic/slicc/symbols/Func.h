
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
 * Func.h
 * 
 * Description: 
 *
 * $Id$
 *
 */

#ifndef FUNC_H
#define FUNC_H

#include "slicc_global.h"
#include "Type.h"
class StateMachine;

class Func : public Symbol {
public:
  // Constructors
  Func(string id, const Location& location,
       Type* type_ptr, const Vector<Type*>& param_type_vec, const Vector<string>& param_string_vec,
       string body, const Map<string, string>& pairs, StateMachine* machine_ptr);

  // Destructor
  ~Func() {}
  
  // Public Methods
  string cIdent() const { return m_c_ident; }
  const Vector<Type*>& getParamTypes() const { return m_param_type_vec; } 
  Type* getReturnType() const { return m_type_ptr; }
  void writeCFiles(string path) const;
  void funcPrototype(string& code) const;
  bool isExternal() const { return existPair("external"); }
  bool isInternalMachineFunc() const { return m_isInternalMachineFunc; }
  void print(ostream& out) const;
private:
  // Private Methods

  // Private copy constructor and assignment operator
  Func(const Func& obj);
  Func& operator=(const Func& obj);
  
  // Data Members (m_ prefix)
  Type* m_type_ptr;
  Vector<Type*> m_param_type_vec;
  Vector<string> m_param_string_vec;
  string m_body;
  string m_c_ident;  
  string m_machineStr;
  bool m_isInternalMachineFunc;
};

// Output operator declaration
ostream& operator<<(ostream& out, const Func& obj);

// ******************* Definitions *******************

// Output operator definition
extern inline 
ostream& operator<<(ostream& out, const Func& obj)
{
  obj.print(out);
  out << flush;
  return out;
}

#endif //FUNC_H
