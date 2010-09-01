
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
 * TypeDeclAST.h
 * 
 * Description: 
 *
 * $Id: TypeDeclAST.h,v 3.2 2003/03/17 01:55:28 xu Exp $
 *
 */

#ifndef TypeDeclAST_H
#define TypeDeclAST_H

#include "slicc_global.h"
#include "DeclAST.h"
#include "TypeAST.h"
#include "TypeFieldAST.h"

class TypeDeclAST : public DeclAST {
public:
  // Constructors
  TypeDeclAST(TypeAST* type_ast_ptr, 
              PairListAST* pairs_ptr,
              Vector<TypeFieldAST*>* field_vec_ptr);

  // Destructor
  ~TypeDeclAST();
  
  // Public Methods
  virtual void generate();
  void print(ostream& out) const;
private:
  // Private Methods

  // Private copy constructor and assignment operator
  TypeDeclAST(const TypeDeclAST& obj);
  TypeDeclAST& operator=(const TypeDeclAST& obj);
  
  // Data Members (m_ prefix)
  TypeAST* m_type_ast_ptr;
  Vector<TypeFieldAST*>* m_field_vec_ptr;
};

// Output operator declaration
ostream& operator<<(ostream& out, const TypeDeclAST& obj);

// ******************* Definitions *******************

// Output operator definition
extern inline 
ostream& operator<<(ostream& out, const TypeDeclAST& obj)
{
  obj.print(out);
  out << flush;
  return out;
}

#endif //TypeDeclAST_H
