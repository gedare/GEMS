
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
 * InPortDeclAST.C
 * 
 * Description: See InPortDeclAST.h
 *
 * $Id$
 *
 */

#include "InPortDeclAST.h"
#include "SymbolTable.h"
#include "Var.h"

InPortDeclAST::InPortDeclAST(string* ident_ptr,
                             TypeAST* msg_type_ptr,
                             ExprAST* var_expr_ptr,
                             PairListAST* pairs_ptr,
                             StatementListAST* statement_list_ptr)
  : DeclAST(pairs_ptr)
{
  m_ident_ptr = ident_ptr;
  m_msg_type_ptr = msg_type_ptr;
  m_var_expr_ptr = var_expr_ptr;
  m_statement_list_ptr = statement_list_ptr;
  m_queue_type_ptr = new TypeAST(new string("InPort"));
}

InPortDeclAST::~InPortDeclAST()
{
  delete m_ident_ptr;
  delete m_msg_type_ptr;
  delete m_var_expr_ptr;
  delete m_statement_list_ptr;
  delete m_queue_type_ptr;
}

void InPortDeclAST::generate()
{
  string code;
  Type* queue_type_ptr = m_var_expr_ptr->generate(code);
  if (!queue_type_ptr->isInPort()) {
    error("Inport queues must be of a type that has the 'inport' attribute.  The type '" + 
          queue_type_ptr->toString() + "' does not have this attribute.");
  }

  Type* type_ptr = m_queue_type_ptr->lookupType();
  Var* in_port_ptr = new Var(*m_ident_ptr, getLocation(), type_ptr, code, getPairs());
  g_sym_table.newSym(in_port_ptr);

  g_sym_table.pushFrame();
  Vector<Type*> param_type_vec;

  // Check for Event
  type_ptr = g_sym_table.getType("Event");
  if (type_ptr == NULL) {
    error("in_port declarations require 'Event' enumeration to be defined");
  }
  param_type_vec.insertAtBottom(type_ptr);

  // Check for Address
  type_ptr = g_sym_table.getType("Address");  
  if (type_ptr == NULL) {
    error("in_port declarations require 'Address' type to be defined");
  }
  param_type_vec.insertAtBottom(type_ptr);

  // Add the trigger method - FIXME, this is a bit dirty
  Map<string, string> pairs;
  pairs.add("external", "yes");
  Vector<string> string_vec;
  g_sym_table.newSym(new Func("trigger", getLocation(), g_sym_table.getType("void"), param_type_vec, string_vec, string(""), pairs, NULL));

  // Check for Event2
  type_ptr = g_sym_table.getType("Event");
  if (type_ptr == NULL) {
    error("in_port declarations require 'Event' enumeration to be defined");
  }
  param_type_vec.insertAtBottom(type_ptr);

  // Check for Address2
  type_ptr = g_sym_table.getType("Address");  
  if (type_ptr == NULL) {
    error("in_port declarations require 'Address' type to be defined");
  }
  param_type_vec.insertAtBottom(type_ptr);

  // Add the doubleTrigger method - this hack supports tiggering two simulateous events
  // The key is that the second transistion cannot fail because the first event cannot be undone
  // therefore you must do some checks before calling double trigger to ensure that won't happen
  g_sym_table.newSym(new Func("doubleTrigger", getLocation(), g_sym_table.getType("void"), param_type_vec, string_vec, string(""), pairs, NULL));

  // Add the continueProcessing method - this hack supports messages that don't trigger events
  Vector<Type*> empty_param_type_vec;
  Vector<string> empty_string_vec;
  g_sym_table.newSym(new Func("continueProcessing", getLocation(), g_sym_table.getType("void"), empty_param_type_vec, empty_string_vec, string(""), pairs, NULL));

  if (m_statement_list_ptr != NULL) {
    inc_indent();
    inc_indent();
    string code;
    m_statement_list_ptr->generate(code, NULL);
    in_port_ptr->addPair("c_code_in_port", code);
    dec_indent();
    dec_indent();
  }
  g_sym_table.popFrame();

  // Add port to state machine
  StateMachine* machine_ptr = g_sym_table.getStateMachine();
  if (machine_ptr == NULL) {
    error("InPort declaration not part of a machine.");
  }
  machine_ptr->addInPort(in_port_ptr);
}


void InPortDeclAST::print(ostream& out) const 
{ 
  out << "[InPortDecl: " << *m_ident_ptr << "]"; 
}
