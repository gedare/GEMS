
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
 * OutPortDeclAST.C
 * 
 * Description: See OutPortDeclAST.h
 *
 * $Id: OutPortDeclAST.C,v 3.3 2004/02/02 22:37:51 milo Exp $
 *
 */

#include "OutPortDeclAST.h"
#include "SymbolTable.h"

OutPortDeclAST::OutPortDeclAST(string* ident_ptr,
                               TypeAST* msg_type_ptr,
                               ExprAST* var_expr_ptr,
                               PairListAST* pairs_ptr)
  : DeclAST(pairs_ptr)
{
  m_ident_ptr = ident_ptr;
  m_msg_type_ptr = msg_type_ptr;
  m_var_expr_ptr = var_expr_ptr;
  m_queue_type_ptr = new TypeAST(new string("OutPort"));
}

OutPortDeclAST::~OutPortDeclAST()
{
  delete m_ident_ptr;
  delete m_msg_type_ptr;
  delete m_var_expr_ptr;
  delete m_queue_type_ptr;
}

void OutPortDeclAST::generate()
{
  string code;
  Type* queue_type_ptr = m_var_expr_ptr->generate(code);
  if (!queue_type_ptr->isOutPort()) {
    error("Outport queues must be of a type that has the 'outport' attribute.  The type '" + 
          queue_type_ptr->toString() + "' does not have this attribute.");
  }

  Type* type_ptr = m_queue_type_ptr->lookupType();
  g_sym_table.newSym(new Var(*m_ident_ptr, getLocation(), type_ptr, code, getPairs()));
}


void OutPortDeclAST::print(ostream& out) const 
{ 
  out << "[OutPortDecl: " << *m_ident_ptr << "]"; 
}
