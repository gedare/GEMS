
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
 * StatementListAST.C
 * 
 * Description: See StatementListAST.h
 *
 * $Id$
 *
 */

#include "StatementListAST.h"

StatementListAST::StatementListAST(Vector<StatementAST*>* vec_ptr)
  : AST()
{
  assert(vec_ptr != NULL);
  m_vec_ptr = vec_ptr;
}

// Singleton constructor.
StatementListAST::StatementListAST(StatementAST* statement_ptr)
  : AST()
{
  assert(statement_ptr != NULL);
  m_vec_ptr = new Vector<StatementAST*>;
  m_vec_ptr->insertAtTop(statement_ptr);
}

StatementListAST::~StatementListAST()
{
  int size = m_vec_ptr->size();
  for(int i=0; i<size; i++) {
    delete (*m_vec_ptr)[i];
  }
  delete m_vec_ptr;
}

void StatementListAST::generate(string& code, Type* return_type_ptr) const
{
  int size = m_vec_ptr->size();
  for(int i=0; i<size; i++) {
    (*m_vec_ptr)[i]->generate(code, return_type_ptr);
  }
}

void StatementListAST::findResources(Map<Var*, string>& resource_list) const
{ 
  int size = m_vec_ptr->size();
  for(int i=0; i<size; i++) {
    (*m_vec_ptr)[i]->findResources(resource_list);
  }
}

void StatementListAST::print(ostream& out) const
{
  assert(m_vec_ptr != NULL);
  out << "[StatementListAST: " << *m_vec_ptr << "]";
}
