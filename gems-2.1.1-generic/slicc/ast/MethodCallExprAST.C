
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
 * MethodCallExprAST.C
 * 
 * Description: See MethodCallExprAST.h
 *
 * $Id$
 *
 */

#include "MethodCallExprAST.h"

MethodCallExprAST::MethodCallExprAST(ExprAST* obj_expr_ptr,
                                     string* proc_name_ptr, 
                                     Vector<ExprAST*>* expr_vec_ptr)
  : ExprAST()
{
  m_obj_expr_ptr = obj_expr_ptr;
  m_type_ptr = NULL;
  m_proc_name_ptr = proc_name_ptr;
  m_expr_vec_ptr = expr_vec_ptr;
}

MethodCallExprAST::MethodCallExprAST(TypeAST* type_ptr,
                                     string* proc_name_ptr, 
                                     Vector<ExprAST*>* expr_vec_ptr)
  : ExprAST()
{
  m_obj_expr_ptr = NULL;
  m_type_ptr = type_ptr;
  m_proc_name_ptr = proc_name_ptr;
  m_expr_vec_ptr = expr_vec_ptr;
}

MethodCallExprAST::~MethodCallExprAST()
{
  delete m_obj_expr_ptr;
  delete m_type_ptr;
  delete m_proc_name_ptr;
  int size = m_expr_vec_ptr->size();
  for(int i=0; i<size; i++) {
    delete (*m_expr_vec_ptr)[i];
  }
  delete m_expr_vec_ptr;
}

Type* MethodCallExprAST::generate(string& code) const
{
  Type* obj_type_ptr = NULL;

  if(m_obj_expr_ptr) {
    // member method call
    code += "((";
    obj_type_ptr = m_obj_expr_ptr->generate(code);
    
    code += ").";
  } else if (m_type_ptr) {
    // class method call
    code += "(" + m_type_ptr->toString() + "::";
    obj_type_ptr = m_type_ptr->lookupType();
  } else {
    // impossible
    assert(0);
  }
  
  Vector <Type*> paramTypes;

  // generate code
  int actual_size = m_expr_vec_ptr->size();
  code += (*m_proc_name_ptr) + "(";
  for(int i=0; i<actual_size; i++) {
    if (i != 0) {
      code += ", ";
    }
    // Check the types of the parameter
    Type* actual_type_ptr = (*m_expr_vec_ptr)[i]->generate(code);
    paramTypes.insertAtBottom(actual_type_ptr);
  }
  code += "))";

  string methodId = obj_type_ptr->methodId(*m_proc_name_ptr, paramTypes);

  // Verify that this is a method of the object
  if (!obj_type_ptr->methodExist(methodId)) {
    error("Invalid method call: Type '" + obj_type_ptr->toString() + "' does not have a method '" + methodId + "'");
  }

  int expected_size = obj_type_ptr->methodParamType(methodId).size();
  if (actual_size != expected_size) {
    // Right number of parameters
    ostringstream err;
    err << "Wrong number of parameters for function name: '" << *m_proc_name_ptr << "'";
    err << ", expected: ";
    err << expected_size;
    err << ", actual: ";
    err << actual_size;
    error(err.str());
  }

  for(int i=0; i<actual_size; i++) {
    // Check the types of the parameter
    Type* actual_type_ptr = paramTypes[i];
    Type* expected_type_ptr = obj_type_ptr->methodParamType(methodId)[i];
    if (actual_type_ptr != expected_type_ptr) {
      (*m_expr_vec_ptr)[i]->error("Type mismatch: expected: " + expected_type_ptr->toString() + 
                                  " actual: " + actual_type_ptr->toString());
    }
  }
  
  // Return the return type of the method
  return obj_type_ptr->methodReturnType(methodId);
}

void MethodCallExprAST::findResources(Map<Var*, string>& resource_list) const
{
  
}

void MethodCallExprAST::print(ostream& out) const
{
  out << "[MethodCallExpr: " << *m_proc_name_ptr << *m_obj_expr_ptr << " " << *m_expr_vec_ptr << "]";
}
