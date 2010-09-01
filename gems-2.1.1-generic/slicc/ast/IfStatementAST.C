
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
 * IfStatementAST.C
 * 
 * Description: See IfStatementAST.h
 *
 * $Id$
 *
 */

#include "IfStatementAST.h"

IfStatementAST::IfStatementAST(ExprAST* cond_ptr,
                               StatementListAST* then_ptr,
                               StatementListAST* else_ptr)
  : StatementAST()
{
  assert(cond_ptr != NULL);
  assert(then_ptr != NULL);
  m_cond_ptr = cond_ptr;
  m_then_ptr = then_ptr;
  m_else_ptr = else_ptr;
}

IfStatementAST::~IfStatementAST()
{
  delete m_cond_ptr;
  delete m_then_ptr;
  delete m_else_ptr;
}


void IfStatementAST::generate(string& code, Type* return_type_ptr) const
{
  Type* type_ptr;

  // Conditional
  code += indent_str() + "if (";
  type_ptr = m_cond_ptr->generate(code);
  if (type_ptr != g_sym_table.getType("bool")) {
    m_cond_ptr->error("Condition of if statement must be boolean, type was '" + type_ptr->toString() + "'");
  }
  code += ") {\n";
  // Then part
  inc_indent();
  m_then_ptr->generate(code, return_type_ptr);
  dec_indent();
  // Else part
  if (m_else_ptr != NULL) {
    code += indent_str() + "} else {\n";  
    inc_indent();
    m_else_ptr->generate(code, return_type_ptr);
    dec_indent();
  }
  code += indent_str() + "}\n";  // End scope
}

void IfStatementAST::findResources(Map<Var*, string>& resource_list) const
{
  // Take a worse case look at both paths
  m_then_ptr->findResources(resource_list);
  if (m_else_ptr != NULL) {
    m_else_ptr->findResources(resource_list);
  }
}

void IfStatementAST::print(ostream& out) const
{
  out << "[IfStatement: " << *m_cond_ptr << *m_then_ptr << *m_else_ptr << "]";
}
