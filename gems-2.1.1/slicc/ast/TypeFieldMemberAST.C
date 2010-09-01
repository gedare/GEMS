
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
 * TypeFieldMemberAST.C
 * 
 * Description: See TypeFieldMemberAST.h
 *
 * $Id: TypeFieldMemberAST.C,v 3.1 2003/03/27 22:58:54 xu Exp $
 *
 */

#include "TypeFieldMemberAST.h"

TypeFieldMemberAST::TypeFieldMemberAST(TypeAST* type_ast_ptr,
                                       string* field_id_ptr,
                                       PairListAST* pairs_ptr,
                                       ExprAST* rvalue_ptr)
  : TypeFieldAST(pairs_ptr)
{
  m_type_ast_ptr = type_ast_ptr;
  m_field_id_ptr = field_id_ptr;
  m_rvalue_ptr = rvalue_ptr;
}

TypeFieldMemberAST::~TypeFieldMemberAST()
{
  delete m_type_ast_ptr;
  delete m_field_id_ptr;
  if(m_rvalue_ptr) delete m_rvalue_ptr;
}

void TypeFieldMemberAST::generate(Type *type_ptr)
{
  // Lookup type
  Type* field_type_ptr = m_type_ast_ptr->lookupType();

  // check type if this is a initialization
  string* init_code = NULL;
  if(m_rvalue_ptr) {
    init_code = new string();
    Type* rvalue_type_ptr = m_rvalue_ptr->generate(*init_code);
    if(field_type_ptr != rvalue_type_ptr) {
      error("Initialization type mismatch '" + field_type_ptr->toString() + "' and '" + rvalue_type_ptr->toString() + "'");
    }
  }

  // Add data member to the parent type
  if (!type_ptr->dataMemberAdd(*m_field_id_ptr, field_type_ptr, getPairs(),
                               init_code)) {
    error("Duplicate data member: " + type_ptr->toString() + ":" + *m_field_id_ptr);
  }
}

void TypeFieldMemberAST::print(ostream& out) const 
{ 
  out << "[TypeFieldMember: " << *m_field_id_ptr << "]"; 
}
