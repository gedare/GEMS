
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
 * EnumDeclAST.C
 * 
 * Description: See EnumDeclAST.h
 *
 * $Id$
 *
 */

#include "EnumDeclAST.h"
#include "main.h"
#include "SymbolTable.h"

EnumDeclAST::EnumDeclAST(TypeAST* type_ast_ptr, 
                         PairListAST* pairs_ptr,
                         Vector<TypeFieldAST*>* field_vec_ptr)
  : DeclAST(pairs_ptr)
{
  m_type_ast_ptr = type_ast_ptr;
  m_field_vec_ptr = field_vec_ptr;
}

EnumDeclAST::~EnumDeclAST()
{
  delete m_type_ast_ptr;
  if (m_field_vec_ptr != NULL) {
    int size = m_field_vec_ptr->size();
    for(int i=0; i<size; i++) {
      delete (*m_field_vec_ptr)[i];
    }
    delete m_field_vec_ptr;
  }
}

void EnumDeclAST::generate()
{
  string machine_name;
  string id = m_type_ast_ptr->toString();

  Vector<Type*> param_type_vec;  // used by to_string func call

  // Make the new type
  Type* new_type_ptr = new Type(id, getLocation(), getPairs(), 
                                g_sym_table.getStateMachine());
  g_sym_table.newSym(new_type_ptr);

  // Add all of the fields of the type to it
  if (m_field_vec_ptr != NULL) {
    int size = m_field_vec_ptr->size();
    for(int i=0; i<size; i++) {
      (*m_field_vec_ptr)[i]->generate(new_type_ptr);
    }
  }

  // Add the implicit State_to_string method - FIXME, this is a bit dirty
  param_type_vec.insertAtBottom(new_type_ptr);  // add state to param vector
  string func_id = new_type_ptr->cIdent()+"_to_string";

  Map<string, string> pairs;
  pairs.add("external", "yes");
  Vector<string> string_vec;
  g_sym_table.newSym(new Func(func_id, getLocation(), g_sym_table.getType("string"), param_type_vec, string_vec, string(""), pairs, NULL));
}

void EnumDeclAST::print(ostream& out) const 
{ 
  out << "[EnumDecl: " << m_type_ast_ptr->toString() << "]"; 
}

