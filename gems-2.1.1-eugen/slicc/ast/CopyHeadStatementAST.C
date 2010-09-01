
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

#include "CopyHeadStatementAST.h"
#include "SymbolTable.h"
#include "VarExprAST.h"
#include "util.h"

CopyHeadStatementAST::CopyHeadStatementAST(VarExprAST* in_queue_ptr,
                                           VarExprAST* out_queue_ptr,
                                           PairListAST* pairs_ptr)
  : StatementAST(pairs_ptr->getPairs())
{
  m_in_queue_ptr = in_queue_ptr;
  m_out_queue_ptr = out_queue_ptr;
}

CopyHeadStatementAST::~CopyHeadStatementAST()
{
  delete m_in_queue_ptr;
  delete m_out_queue_ptr;
}
  
void CopyHeadStatementAST::generate(string& code, Type* return_type_ptr) const
{
  m_in_queue_ptr->assertType("InPort");
  m_out_queue_ptr->assertType("OutPort");

  code += indent_str();
  code += m_out_queue_ptr->getVar()->getCode() + ".enqueue(" + m_in_queue_ptr->getVar()->getCode() + ".getMsgPtrCopy()";

  if (getPairs().exist("latency")) {
    code += ", " + getPairs().lookup("latency");
  } else {
    code += ", COPY_HEAD_LATENCY";
  }

  code += ");\n";
}

void CopyHeadStatementAST::findResources(Map<Var*, string>& resource_list) const
{
  Var* var_ptr = m_out_queue_ptr->getVar();
  int res_count = 0;
  if (resource_list.exist(var_ptr)) {
    res_count = atoi((resource_list.lookup(var_ptr)).c_str());
  }
  resource_list.add(var_ptr, int_to_string(res_count+1));
}

void CopyHeadStatementAST::print(ostream& out) const
{
  out << "[CopyHeadStatementAst: " << *m_in_queue_ptr << " " << *m_out_queue_ptr << "]";
}
