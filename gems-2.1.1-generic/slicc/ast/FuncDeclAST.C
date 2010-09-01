
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
 * FuncDeclAST.C
 * 
 * Description: See FuncDeclAST.h
 *
 * $Id: FuncDeclAST.C,v 3.4 2003/08/22 18:19:34 beckmann Exp $
 *
 */

#include "FuncDeclAST.h"
#include "SymbolTable.h"
#include "main.h"

FuncDeclAST::FuncDeclAST(TypeAST* return_type_ast_ptr, 
                         string* ident_ptr, 
                         Vector<FormalParamAST*>* formal_vec_ptr,
                         PairListAST* pairs_ptr,
                         StatementListAST* statement_list_ptr)
  : DeclAST(pairs_ptr)
{
  m_return_type_ast_ptr = return_type_ast_ptr;
  m_ident_ptr = ident_ptr;
  m_formal_vec_ptr = formal_vec_ptr;
  m_statement_list_ptr = statement_list_ptr;
}

FuncDeclAST::~FuncDeclAST()
{
  delete m_return_type_ast_ptr;
  delete m_ident_ptr;

  int size = m_formal_vec_ptr->size();
  for(int i=0; i<size; i++) {
    delete (*m_formal_vec_ptr)[i];
  }
  delete m_formal_vec_ptr;
  delete m_statement_list_ptr;
}

void FuncDeclAST::generate()
{
  Vector<Type*> type_vec;
  Vector<string> param_vec;
  Type* void_type_ptr = g_sym_table.getType("void");

  // Generate definition code
  g_sym_table.pushFrame();

  // Lookup return type
  Type* return_type_ptr = m_return_type_ast_ptr->lookupType();
  
  // Generate function header
  int size = m_formal_vec_ptr->size();
  for(int i=0; i<size; i++) {
    // Lookup parameter types
    string ident;
    Type* type_ptr = (*m_formal_vec_ptr)[i]->generate(ident);
    type_vec.insertAtBottom(type_ptr);
    param_vec.insertAtBottom(ident);
  }

  string body;
  if (m_statement_list_ptr == NULL) {
    getPairs().add("external", "yes");
  } else {
    m_statement_list_ptr->generate(body, return_type_ptr);
  }
  g_sym_table.popFrame();
  
  StateMachine* machine_ptr = g_sym_table.getStateMachine();
  if (machine_ptr != NULL) {
    machine_ptr->addFunc(new Func(*m_ident_ptr, getLocation(), return_type_ptr, type_vec, param_vec, body, getPairs(), machine_ptr));
  } else {
    g_sym_table.newSym(new Func(*m_ident_ptr, getLocation(), return_type_ptr, type_vec, param_vec, body, getPairs(), machine_ptr));
  }

}

void FuncDeclAST::print(ostream& out) const 
{ 
  out << "[FuncDecl: " << *m_ident_ptr << "]"; 
}
