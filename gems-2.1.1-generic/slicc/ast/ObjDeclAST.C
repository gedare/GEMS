
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
 * ObjDeclAST.C
 * 
 * Description: See ObjDeclAST.h
 *
 * $Id: ObjDeclAST.C,v 3.13 2004/06/24 15:56:14 beckmann Exp $
 *
 */

#include "ObjDeclAST.h"
#include "SymbolTable.h"
#include "main.h"

ObjDeclAST::ObjDeclAST(TypeAST* type_ptr, 
                       string* ident_ptr, 
                       PairListAST* pairs_ptr)
  : DeclAST(pairs_ptr)
{
  m_type_ptr = type_ptr;
  m_ident_ptr = ident_ptr;
}

ObjDeclAST::~ObjDeclAST()
{
  delete m_type_ptr;
  delete m_ident_ptr;
}

void ObjDeclAST::generate()
{

  bool machineComponentSym = false;

  getPairs().add("chip_object", "yes");

  string c_code;


  if (getPairs().exist("hack")) {
    warning("'hack=' is now deprecated");
  } 

  if (getPairs().exist("network")) {
    if (!getPairs().exist("virtual_network")) {
      error("Network queues require a 'virtual_network' attribute.");
    }
  }

  Type* type_ptr = m_type_ptr->lookupType();  
  if (type_ptr->isBuffer()) {
    if (!getPairs().exist("ordered")) {
      error("Buffer object declarations require an 'ordered' attribute.");
    }
  }

  if (getPairs().exist("ordered")) {
    string value = getPairs().lookup("ordered");
    if (value != "true" && value != "false") {
      error("The 'ordered' attribute must be 'true' or 'false'.");
    }
  }

  if (getPairs().exist("random")) {
    string value = getPairs().lookup("random");
    if (value != "true" && value != "false") {
      error("The 'random' attribute must be 'true' or 'false'.");
    }
  }

  string machine;
  if (g_sym_table.getStateMachine() != NULL) {
    machine = g_sym_table.getStateMachine()->getIdent() + "_";
  }

  // FIXME : should all use accessors here to avoid public member variables
  if (*m_ident_ptr == "id") {
    c_code = "m_chip_ptr->getID()";
  } else if (*m_ident_ptr == "version") {
    c_code = "m_version";
  } else if (*m_ident_ptr == "machineID") {
    c_code = "m_machineID";
  } else if (*m_ident_ptr == "sequencer") {
    c_code = "*(dynamic_cast<"+m_type_ptr->toString()+"*>(m_chip_ptr->getSequencer(m_version)))";
    machineComponentSym = true;
  } /*else if (*m_ident_ptr == "xfdr_record_mgr") {
    c_code = "*(dynamic_cast<"+m_type_ptr->toString()+"*>(m_chip_ptr->getXfdrManager(m_version)))";
    machineComponentSym = true;
    } */else if (// getPairs().exist("network") || (m_type_ptr->lookupType()->existPair("cache")) 
//              || (m_type_ptr->lookupType()->existPair("tbe")) ||  
//              (m_type_ptr->lookupType()->existPair("newtbe")) ||  
//              (m_type_ptr->lookupType()->existPair("timer")) ||  
//              (m_type_ptr->lookupType()->existPair("dir")) || 
//              (m_type_ptr->lookupType()->existPair("persistent")) || 
//              (m_type_ptr->lookupType()->existPair("filter")) ||
//              (getPairs().exist("trigger_queue"))
             getPairs().exist("no_vector")) {
    c_code = "(*(m_chip_ptr->m_" + machine + *m_ident_ptr + "_ptr))";
    machineComponentSym = true;
  } else {
    c_code = "(*(m_chip_ptr->m_" + machine + *m_ident_ptr + "_vec[m_version]))";
    machineComponentSym = true;
  }

  Var* v = new Var(*m_ident_ptr, getLocation(), type_ptr, c_code, 
                             getPairs(), g_sym_table.getStateMachine());

  g_sym_table.newSym(v);

  // used to cheat-- that is, access components in other machines
  if (machineComponentSym) {
    g_sym_table.newMachComponentSym(v);
  }
                    
}

void ObjDeclAST::print(ostream& out) const 
{ 
  out << "[ObjDecl: " << *m_ident_ptr << "]"; 
}
