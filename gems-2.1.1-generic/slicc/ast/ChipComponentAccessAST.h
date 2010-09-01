
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
 *
 * 
 * Description: 
 *
 * $Id: ChipComponentAccessAST.h 1.8 04/06/18 21:00:08-00:00 beckmann@cottons.cs.wisc.edu $
 *
 */

#ifndef ChipComponentAccessAST_H
#define ChipComponentAccessAST_H

#include "slicc_global.h"
#include "StatementAST.h"
#include "ExprAST.h"
#include "VarExprAST.h"
#include "TypeAST.h"

class ChipComponentAccessAST : public ExprAST {
public:
  // Constructors

  // method call from local chip
  ChipComponentAccessAST(VarExprAST* machine, ExprAST* mach_version, VarExprAST* component, string* proc_name, Vector<ExprAST*>* expr_vec_ptr);
  // member access from local chip
  ChipComponentAccessAST(VarExprAST* machine, ExprAST* mach_version, VarExprAST* component, string* field_name);

  // method call from specified chip
  ChipComponentAccessAST(ExprAST* chip_version, VarExprAST* machine, ExprAST* mach_version, VarExprAST* component, string* proc_name, Vector<ExprAST*>* expr_vec_ptr);

  // member access from specified chip
  ChipComponentAccessAST(ExprAST* chip_version, VarExprAST* machine, ExprAST* mach_version, VarExprAST* component, string* field_name);

  // Destructor
  ~ChipComponentAccessAST();
  
  // Public Methods
  Type* generate(string& code) const;
  void findResources(Map<Var*, string>& resource_list) const;
  void print(ostream& out) const;
private:
  // Private Methods

  // Private copy constructor and assignment operator
  ChipComponentAccessAST(const ChipComponentAccessAST& obj);
  ChipComponentAccessAST& operator=(const ChipComponentAccessAST& obj);
  
  // Data Members (m_ prefix)
  VarExprAST* m_mach_var_ptr;
  VarExprAST* m_comp_var_ptr;
  ExprAST* m_mach_ver_expr_ptr;
  ExprAST* m_chip_ver_expr_ptr;
  Vector<ExprAST*>* m_expr_vec_ptr;
  string* m_proc_name_ptr;
  string* m_field_name_ptr;
};

// Output operator declaration
ostream& operator<<(ostream& out, const ChipComponentAccessAST& obj);

// ******************* Definitions *******************

// Output operator definition
extern inline 
ostream& operator<<(ostream& out, const ChipComponentAccessAST& obj)
{
  obj.print(out);
  out << flush;
  return out;
}

#endif // ChipComponentAccessAST_H
