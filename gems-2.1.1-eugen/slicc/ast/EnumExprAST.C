
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
 * EnumExprAST.C
 * 
 * Description: See EnumExprAST.h
 *
 * $Id: EnumExprAST.C,v 3.1 2003/07/10 18:08:06 milo Exp $
 *
 */

#include "EnumExprAST.h"

EnumExprAST::EnumExprAST(TypeAST* type_ast_ptr,
                         string* value_ptr)
  : ExprAST()
{
  assert(value_ptr != NULL);
  assert(type_ast_ptr != NULL);
  m_type_ast_ptr = type_ast_ptr;
  m_value_ptr = value_ptr;
}

EnumExprAST::~EnumExprAST()
{
  delete m_type_ast_ptr;
  delete m_value_ptr;
}
  
Type* EnumExprAST::generate(string& code) const
{
  Type* type_ptr = m_type_ast_ptr->lookupType();
  code += type_ptr->cIdent() + "_" + (*m_value_ptr);
  
  // Make sure the enumeration value exists
  if (!type_ptr->enumExist(*m_value_ptr)) {
    error("Type '" + m_type_ast_ptr->toString() + "' does not have enumeration '" + *m_value_ptr + "'");
  }
  
  // Return the proper type
  return type_ptr;
}

void EnumExprAST::print(ostream& out) const
{
  string str;
  str += m_type_ast_ptr->toString()+":"+(*m_value_ptr);
  out << "[EnumExpr: " << str << "]";
}
