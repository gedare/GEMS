
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
 * PeekStatementAST.C
 * 
 * Description: See PeekStatementAST.h
 *
 * $Id$
 *
 */

#include "PeekStatementAST.h"
#include "SymbolTable.h"
#include "StatementListAST.h"
#include "TypeAST.h"
#include "VarExprAST.h"

PeekStatementAST::PeekStatementAST(VarExprAST* queue_name_ptr,
                                   TypeAST* type_ptr,
                                   StatementListAST* statementlist_ptr,
                                   string method)
  : StatementAST()
{
  m_queue_name_ptr = queue_name_ptr;
  m_type_ptr = type_ptr;
  m_statementlist_ptr = statementlist_ptr;
  m_method = method;
}

PeekStatementAST::~PeekStatementAST()
{
  delete m_queue_name_ptr;
  delete m_type_ptr;
  delete m_statementlist_ptr;
}

void PeekStatementAST::generate(string& code, Type* return_type_ptr) const
{
  code += indent_str() + "{\n";  // Start scope
  inc_indent();
  g_sym_table.pushFrame();

  Type* msg_type_ptr = m_type_ptr->lookupType();

  // Add new local var to symbol table
  g_sym_table.newSym(new Var("in_msg", getLocation(), msg_type_ptr, "(*in_msg_ptr)", getPairs()));

  // Check the queue type
  m_queue_name_ptr->assertType("InPort");

  // Declare the new "in_msg_ptr" variable
  code += indent_str() + "const " + msg_type_ptr->cIdent() + "* in_msg_ptr;\n";  // Declare message
  //  code += indent_str() + "in_msg_ptr = static_cast<const ";
  code += indent_str() + "in_msg_ptr = dynamic_cast<const ";
  code += msg_type_ptr->cIdent() + "*>(";
  code += "(" + m_queue_name_ptr->getVar()->getCode() + ")";
  code += ".";
  code += m_method;
  code += "());\n";

  code += indent_str() + "assert(in_msg_ptr != NULL);\n";        // Check the cast result
  
  if(CHECK_INVALID_RESOURCE_STALLS) {
    // Declare the "in_buffer_rank" variable
    code += indent_str() + "int in_buffer_rank = ";  // Declare message
    code += "(" + m_queue_name_ptr->getVar()->getCode() + ")";
    code += ".getPriority();\n";
  }

  m_statementlist_ptr->generate(code, return_type_ptr);                // The other statements
  dec_indent();
  g_sym_table.popFrame();
  code += indent_str() + "}\n";  // End scope
}

void PeekStatementAST::findResources(Map<Var*, string>& resource_list) const
{
  m_statementlist_ptr->findResources(resource_list);
}

void PeekStatementAST::print(ostream& out) const
{
  out << "[PeekStatementAST: " << m_method 
      << " queue_name: " << *m_queue_name_ptr
      << " type: " << m_type_ptr->toString()
      << " " << *m_statementlist_ptr
      << "]";
}
