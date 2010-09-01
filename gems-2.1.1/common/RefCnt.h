/*
    Copyright (C) 1999-2005 by Mark D. Hill and David A. Wood for the
    Wisconsin Multifacet Project.  Contact: gems@cs.wisc.edu
    http://www.cs.wisc.edu/gems/

    --------------------------------------------------------------------

    This file a component of the Multifacet GEMS (General 
    Execution-driven Multiprocessor Simulator) software toolset 
    originally developed at the University of Wisconsin-Madison.

    Ruby was originally developed primarily by Milo Martin and Daniel
    Sorin with contributions from Ross Dickson, Carl Mauer, and Manoj
    Plakal.
 
    SLICC was originally developed by Milo Martin with substantial
    contributions from Daniel Sorin.

    Opal was originally developed by Carl Mauer based upon code by
    Craig Zilles.

    Substantial further development of Multifacet GEMS at the
    University of Wisconsin was performed by Alaa Alameldeen, Brad
    Beckmann, Jayaram Bobba, Ross Dickson, Dan Gibson, Pacia Harper,
    Derek Hower, Milo Martin, Michael Marty, Carl Mauer, Michelle Moravan,
    Kevin Moore, Manoj Plakal, Daniel Sorin, Haris Volos, Min Xu, and Luke Yen.

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
 * RefCnt.h
 * 
 * Description: 
 *
 * $Id$
 *
 */

#ifndef REFCNT_H
#define REFCNT_H

template <class TYPE>
class RefCnt {
public:
  // Constructors
  RefCnt();
  RefCnt(const TYPE& data);

  // Destructor
  ~RefCnt();
  
  // Public Methods
  const TYPE* ref() const { return m_data_ptr; }
  TYPE* ref() { return m_data_ptr; }
  TYPE* mod_ref() const { return m_data_ptr; }
  void freeRef();
  void print(ostream& out) const;

  // Public copy constructor and assignment operator
  RefCnt(const RefCnt& obj);
  RefCnt& operator=(const RefCnt& obj);

private:
  // Private Methods

  // Data Members (m_ prefix)
  TYPE* m_data_ptr;
  //  int* m_count_ptr; // Not used yet
};

// Output operator declaration
template <class TYPE> 
inline 
ostream& operator<<(ostream& out, const RefCnt<TYPE>& obj);

// ******************* Definitions *******************

// Constructors
template <class TYPE> 
inline
RefCnt<TYPE>::RefCnt()
{
  m_data_ptr = NULL;
}

template <class TYPE> 
inline
RefCnt<TYPE>::RefCnt(const TYPE& data) 
{
  m_data_ptr = data.clone(); 
  m_data_ptr->setRefCnt(1);
}

template <class TYPE> 
inline
RefCnt<TYPE>::~RefCnt() 
{ 
  freeRef();
}

template <class TYPE> 
inline
void RefCnt<TYPE>::freeRef() 
{ 
  if (m_data_ptr != NULL) {
    m_data_ptr->decRefCnt();
    if (m_data_ptr->getRefCnt() == 0) {
      m_data_ptr->destroy();
    }
    m_data_ptr = NULL; 
  }
}

template <class TYPE> 
inline
void RefCnt<TYPE>::print(ostream& out) const 
{
  if (m_data_ptr == NULL) {
    out << "[RefCnt: Null]";
  } else {
    out << "[RefCnt: ";
    m_data_ptr->print(out);
    out << "]";
  }
}

// Copy constructor
template <class TYPE> 
inline
RefCnt<TYPE>::RefCnt(const RefCnt<TYPE>& obj) 
{ 
  //  m_data_ptr = obj.m_data_ptr->clone();  
  m_data_ptr = obj.m_data_ptr;

  // Increment the reference count
  if (m_data_ptr != NULL) {
    m_data_ptr->incRefCnt();
  }
}

// Assignment operator
template <class TYPE> 
inline
RefCnt<TYPE>& RefCnt<TYPE>::operator=(const RefCnt<TYPE>& obj) 
{ 
  if (this == &obj) {
    // If this is the case, do nothing
    //    assert(false);
  } else {
    freeRef();
    m_data_ptr = obj.m_data_ptr;
    if (m_data_ptr != NULL) {
      m_data_ptr->incRefCnt();
    }
  }
  return *this;
}


// Output operator definition
template <class TYPE> 
inline 
ostream& operator<<(ostream& out, const RefCnt<TYPE>& obj)
{
  obj.print(out);
  out << flush;
  return out;
}



#endif //REFCNT_H
