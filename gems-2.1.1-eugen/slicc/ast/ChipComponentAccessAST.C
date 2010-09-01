
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
 * ChipComponentAccessAST.C
 * 
 * Description: See ChipComponentAccessAST.h
 *
 * $Id: ChipComponentAccessAST.C 1.9 04/06/18 21:00:08-00:00 beckmann@cottons.cs.wisc.edu $
 *
 */

#include "ChipComponentAccessAST.h"

ChipComponentAccessAST::ChipComponentAccessAST(VarExprAST* machine, ExprAST* mach_version, VarExprAST* component, string* proc_name, Vector<ExprAST*>* expr_vec_ptr)

  : ExprAST()
{
  m_chip_ver_expr_ptr = NULL;
  m_mach_var_ptr = machine;
  m_comp_var_ptr = component;
  m_mach_ver_expr_ptr = mach_version;
  m_expr_vec_ptr = expr_vec_ptr;
  m_proc_name_ptr = proc_name;
  m_field_name_ptr = NULL;
}

ChipComponentAccessAST::ChipComponentAccessAST(VarExprAST* machine, ExprAST* mach_version, VarExprAST* component, string* field_name)

  : ExprAST()
{
  m_chip_ver_expr_ptr = NULL;
  m_mach_var_ptr = machine;
  m_comp_var_ptr = component;
  m_mach_ver_expr_ptr = mach_version;
  m_expr_vec_ptr = NULL;
  m_proc_name_ptr = NULL;
  m_field_name_ptr = field_name;
}

ChipComponentAccessAST::ChipComponentAccessAST(ExprAST* chip_version, VarExprAST* machine, ExprAST* mach_version, VarExprAST* component, string* proc_name, Vector<ExprAST*>* expr_vec_ptr)

  : ExprAST()
{
  m_chip_ver_expr_ptr = chip_version;
  m_mach_var_ptr = machine;
  m_comp_var_ptr = component;
  m_mach_ver_expr_ptr = mach_version;
  m_expr_vec_ptr = expr_vec_ptr;
  m_proc_name_ptr = proc_name;
  m_field_name_ptr = NULL;
}

ChipComponentAccessAST::ChipComponentAccessAST(ExprAST* chip_version, VarExprAST* machine, ExprAST* mach_version, VarExprAST* component, string* field_name)

  : ExprAST()
{
  m_chip_ver_expr_ptr = chip_version;
  m_mach_var_ptr = machine;
  m_comp_var_ptr = component;
  m_mach_ver_expr_ptr = mach_version;
  m_expr_vec_ptr = NULL;
  m_proc_name_ptr = NULL;
  m_field_name_ptr = field_name;
}



ChipComponentAccessAST::~ChipComponentAccessAST()
{
  if (m_expr_vec_ptr != NULL) {
    int size = m_expr_vec_ptr->size();
    for(int i=0; i<size; i++) {
      delete (*m_expr_vec_ptr)[i];
    }
  }

  delete m_mach_var_ptr;
  delete m_comp_var_ptr;
  delete m_mach_ver_expr_ptr;

  if (m_proc_name_ptr != NULL) {
    delete m_proc_name_ptr;
  }

  if (m_field_name_ptr != NULL) {
    delete m_field_name_ptr;
  }

  if (m_chip_ver_expr_ptr != NULL) {
    delete m_chip_ver_expr_ptr;
  }
}

Type* ChipComponentAccessAST::generate(string& code) const
{
  Type* void_type_ptr = g_sym_table.getType("void");
  Type* ret_type_ptr;


  code += "(";
  
  Var* v = g_sym_table.getMachComponentVar(m_mach_var_ptr->getName(), m_comp_var_ptr->getName());

  string orig_code = v->getCode();
  string working_code;

  if (m_chip_ver_expr_ptr != NULL) {
    // replace m_chip_ptr with specified chip

    unsigned int t = orig_code.find("m_chip_ptr");
    assert(t != string::npos);
    string code_temp0 = orig_code.substr(0, t);
    string code_temp1 = orig_code.substr(t+10);

    working_code += code_temp0;
    working_code += "g_system_ptr->getChip(";
    m_chip_ver_expr_ptr->generate(working_code);
    working_code += ")";
    working_code += code_temp1;
  }
  else {
    working_code += orig_code;
  }

  // replace default "m_version" with the version we really want
  unsigned int tmp_uint = working_code.find("m_version");
  assert(tmp_uint != string::npos);
  string code_temp2 = working_code.substr(0, tmp_uint);
  string code_temp3 = working_code.substr(tmp_uint+9);

  code += code_temp2;
  code += "(";
  m_mach_ver_expr_ptr->generate(code);
  code += ")";
  code += code_temp3;
  code += ")";
  
  if (m_proc_name_ptr != NULL) {
    // method call
    code += ".";
 
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
    code += ")";

    Type* obj_type_ptr = v->getType();
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
    ret_type_ptr = obj_type_ptr->methodReturnType(methodId);
  }
  else if (m_field_name_ptr != NULL) {
    Type* obj_type_ptr = v->getType();
    code += ").m_" + (*m_field_name_ptr);

    // Verify that this is a valid field name for this type
    if (!obj_type_ptr->dataMemberExist(*m_field_name_ptr)) {
      error("Invalid object field: Type '" + obj_type_ptr->toString() + "' does not have data member " + *m_field_name_ptr);
    }

    // Return the type of the field
    ret_type_ptr = obj_type_ptr->dataMemberType(*m_field_name_ptr);
  }
  else {
    assert(0);
  }

  return ret_type_ptr;
}

void ChipComponentAccessAST::findResources(Map<Var*, string>& resource_list) const
{
  
}

void ChipComponentAccessAST::print(ostream& out) const
{
  out << "[ChipAccessExpr: " << *m_expr_vec_ptr << "]";
}
