
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
 * InfixOperatorExprAST.C
 * 
 * Description: See InfixOperatorExprAST.h
 *
 * $Id: InfixOperatorExprAST.C,v 3.2 2004/01/31 20:46:15 milo Exp $
 *
 */

#include "InfixOperatorExprAST.h"

InfixOperatorExprAST::InfixOperatorExprAST(ExprAST* left_ptr, 
                                           string* op_ptr, 
                                           ExprAST* right_ptr)
  : ExprAST()
{
  m_left_ptr = left_ptr;
  m_op_ptr = op_ptr;
  m_right_ptr = right_ptr;
}

InfixOperatorExprAST::~InfixOperatorExprAST()
{
  delete m_left_ptr;
  delete m_op_ptr;
  delete m_right_ptr;
}

Type* InfixOperatorExprAST::generate(string& code) const
{
  code += "(";
  Type* left_type_ptr = m_left_ptr->generate(code);
  code += " " + *m_op_ptr + " ";
  Type* right_type_ptr = m_right_ptr->generate(code);
  code += ")";
  
  string inputs, output;
  // Figure out what the input and output types should be
  if ((*m_op_ptr == "==" || 
       *m_op_ptr == "!=")) {
    output = "bool";
    if (left_type_ptr != right_type_ptr) {
      error("Type mismatch: left & right operand of operator '" + *m_op_ptr + 
            "' must be the same type." + 
            "left: '" + left_type_ptr->toString() + 
            "', right: '" + right_type_ptr->toString() + "'");
    }
  } else {
    if ((*m_op_ptr == "&&" || 
         *m_op_ptr == "||")) {
      // boolean inputs and output
      inputs = "bool";
      output = "bool";
    } else if ((*m_op_ptr == "==" || 
                *m_op_ptr == "!=" || 
                *m_op_ptr == ">=" || 
                *m_op_ptr == "<=" || 
                *m_op_ptr == ">" || 
                *m_op_ptr == "<")) {
      // Integer inputs, boolean output
      inputs = "int";
      output = "bool";
    } else {
      // integer inputs and output
      inputs = "int";
      output = "int";    
    }
    
    Type* inputs_type = g_sym_table.getType(inputs);
    
    if (inputs_type != left_type_ptr) {
      m_left_ptr->error("Type mismatch: left operand of operator '" + *m_op_ptr + 
                        "' expects input type '" + inputs + "', actual was " + left_type_ptr->toString() + "'");
    }
    
    if (inputs_type != right_type_ptr) {
      m_right_ptr->error("Type mismatch: right operand of operator '" + *m_op_ptr + 
                         "' expects input type '" + inputs + "', actual was '" + right_type_ptr->toString() + "'");
    }
  }

  // All is well
  Type* output_type = g_sym_table.getType(output);
  return output_type;
}


void InfixOperatorExprAST::print(ostream& out) const
{
  out << "[InfixExpr: " << *m_left_ptr 
      << *m_op_ptr << *m_right_ptr << "]";
}
