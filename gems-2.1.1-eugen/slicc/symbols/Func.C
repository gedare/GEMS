
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
 * Func.C
 * 
 * Description: See Func.h
 *
 * $Id$
 *
 */

#include "Func.h"
#include "SymbolTable.h"
#include "fileio.h"
#include "StateMachine.h"

Func::Func(string id, const Location& location, 
           Type* type_ptr, const Vector<Type*>& param_type_vec,
           const Vector<string>& param_string_vec, string body,
           const Map<string, string>& pairs, StateMachine* machine_ptr)
  : Symbol(id, location, pairs)
{ 
  m_type_ptr = type_ptr; 
  m_param_type_vec = param_type_vec;
  m_param_string_vec = param_string_vec;
  m_body = body;
  m_isInternalMachineFunc = false;

  if (machine_ptr == NULL) {
    m_c_ident = id;
  } else if (existPair("external") || existPair("primitive")) { 
    m_c_ident = id;
  } else {
    m_machineStr = machine_ptr->toString();
    m_c_ident = m_machineStr + "_" + id;  // Append with machine name
    m_isInternalMachineFunc = true;
  }
}

void Func::funcPrototype(string& code) const
{
  if (isExternal()) {
    // Do nothing
  } else {
    string return_type = m_type_ptr->cIdent(); 
    Type* void_type_ptr = g_sym_table.getType("void");
    if (existPair("return_by_ref") && (m_type_ptr != void_type_ptr)) {
      return_type += "&";
    }
    code += return_type + " " + cIdent() + "(";
    int size = m_param_string_vec.size();
    for(int i=0; i<size; i++) {
      // Generate code
      if (i != 0) {
        code += ", ";
      }
      code += m_param_string_vec[i];
    }
    code += ");\n";
  }
}

// This write a function of object Chip
void Func::writeCFiles(string path) const
{
  if (isExternal()) {
    // Do nothing
  } else {
    ostringstream out;

    // Header
    out << "/** Auto generated C++ code started by "<<__FILE__<<":"<<__LINE__<< " */" << endl;
    out << endl;
    out << "#include \"Types.h\"" << endl;
    out << "#include \"Chip.h\"" << endl;
    if (m_isInternalMachineFunc) {
      out << "#include \"" << m_machineStr << "_Controller.h\"" << endl;
    }
    out << endl;

    // Generate function header
    string code;
    Type* void_type_ptr = g_sym_table.getType("void");
    string return_type = m_type_ptr->cIdent(); 
    code += return_type;
    if (existPair("return_by_ref") && m_type_ptr != void_type_ptr) {
      code += "&";
    }
    if (!m_isInternalMachineFunc) {   
      code += " Chip::" + cIdent() + "(";
    } else {
      code += " " + m_machineStr + "_Controller::" + cIdent() + "(";
    }
    int size = m_param_type_vec.size();
    for(int i=0; i<size; i++) {
      // Generate code
      if (i != 0) {
        code += ", ";
      }
      code += m_param_string_vec[i];
    }
    code += ")";
    
    // Function body
    code += "\n{\n";
    code += m_body;
    code += "}\n";
    out << code << endl;
    
    // Write it out
    conditionally_write_file(path + cIdent() + ".C", out);
  }
}

void Func::print(ostream& out) const
{
}
