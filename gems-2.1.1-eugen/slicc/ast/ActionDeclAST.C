
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
 * ActionDeclAST.C
 * 
 * Description: See ActionDeclAST.h
 *
 * $Id$
 *
 */

#include "ActionDeclAST.h"
#include "Action.h"

ActionDeclAST::ActionDeclAST(string* ident_ptr, 
                             PairListAST* pairs_ptr,
                             StatementListAST* statement_list_ptr)
  : DeclAST(pairs_ptr)
{
  m_ident_ptr = ident_ptr;
  m_statement_list_ptr = statement_list_ptr;
}

ActionDeclAST::~ActionDeclAST()
{
  delete m_ident_ptr;
  delete m_statement_list_ptr;
}

void ActionDeclAST::generate()
{
  Map<Var*, string> resource_list;
  if (m_statement_list_ptr != NULL) {
    string code;

    // Add new local vars
    g_sym_table.pushFrame();
    
    Type* type_ptr = g_sym_table.getType("Address");

    if (type_ptr == NULL) {
      error("Type 'Address' not declared.");
    }

    g_sym_table.newSym(new Var("address", getLocation(), type_ptr, "addr", getPairs()));

    // Don't allows returns in actions
    m_statement_list_ptr->generate(code, NULL);

    getPairs().add("c_code", code);

    m_statement_list_ptr->findResources(resource_list);

    g_sym_table.popFrame();
  }
  
  StateMachine* machine_ptr = g_sym_table.getStateMachine();
  if (machine_ptr == NULL) {
    error("Action declaration not part of a machine.");
  } else {
    machine_ptr->addAction(new Action(*m_ident_ptr, resource_list, getLocation(), getPairs()));
  }

}

void ActionDeclAST::print(ostream& out) const 
{ 
  out << "[ActionDecl: " << *m_ident_ptr << "]"; 
}
