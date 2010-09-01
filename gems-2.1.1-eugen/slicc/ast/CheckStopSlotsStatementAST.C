
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
 * $Id$
 *
 */

#include "CheckStopSlotsStatementAST.h"
#include "SymbolTable.h"
#include "VarExprAST.h"
#include "PairListAST.h"

CheckStopSlotsStatementAST::CheckStopSlotsStatementAST(VarExprAST* variable, string* condStr, string* bankStr)
  : StatementAST()
{
  m_variable = variable;
  m_condStr_ptr = condStr;
  m_bankStr_ptr = bankStr;
}

CheckStopSlotsStatementAST::~CheckStopSlotsStatementAST()
{
  delete m_variable;
  delete m_condStr_ptr;
  delete m_bankStr_ptr;
}
  
void CheckStopSlotsStatementAST::generate(string& code, Type* return_type_ptr) const
{
  
  // Make sure the variable is valid
  m_variable->getVar();  

}

void CheckStopSlotsStatementAST::findResources(Map<Var*, string>& resource_list) const
{
  Type* type_ptr;

  Var* var_ptr = m_variable->getVar();
  string check_code;

  if (*m_condStr_ptr == "((*in_msg_ptr)).m_isOnChipSearch") {
    check_code += "    const Response9Msg* in_msg_ptr;\n";
    check_code += "    in_msg_ptr = dynamic_cast<const Response9Msg*>(((*(m_chip_ptr->m_L2Cache_responseToL2Cache9_vec[m_version]))).peek());\n";
    check_code += "    assert(in_msg_ptr != NULL);\n";
  }

  check_code += "    if (";
  check_code += *m_condStr_ptr;
  check_code += ") {\n";

  check_code += "      if (!";
  type_ptr = m_variable->generate(check_code);
  check_code += ".isDisableSPossible((((*(m_chip_ptr->m_DNUCAmover_ptr))).getBankPos(";
  check_code += *m_bankStr_ptr;
  check_code += ")))) {\n";
  if(CHECK_INVALID_RESOURCE_STALLS) {
    check_code += "        assert(priority >= ";
    type_ptr = m_variable->generate(check_code);
    check_code += ".getPriority());\n";
  }
  check_code += "        return TransitionResult_ResourceStall;\n";
  check_code += "      }\n"; 
  check_code += "    } else {\n";
  check_code += "      if (!";
  type_ptr = m_variable->generate(check_code);
  check_code += ".isDisableFPossible((((*(m_chip_ptr->m_DNUCAmover_ptr))).getBankPos(";
  check_code += *m_bankStr_ptr;
  check_code += ")))) {\n";
  if(CHECK_INVALID_RESOURCE_STALLS) {
    check_code += "        assert(priority >= ";
    type_ptr = m_variable->generate(check_code);
    check_code += ".getPriority());\n";
  }
  check_code += "        return TransitionResult_ResourceStall;\n";
  check_code += "      }\n"; 
  check_code += "    }\n"; 

  assert(!resource_list.exist(var_ptr));
  resource_list.add(var_ptr, check_code);

}

void CheckStopSlotsStatementAST::print(ostream& out) const
{
  out << "[CheckStopSlotsStatementAst: " << *m_variable << "]";
}
