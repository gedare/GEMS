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
 * Vector.h
 * 
 * Description: The Vector class is a generic container which acts
 * much like an array.  The Vector class handles dynamic sizing and
 * resizing as well as performing bounds checking on each access.  An
 * "insertAtBottom" operation is supported to allow adding elements to
 * the Vector much like you would add new elements to a linked list or
 * queue.  
 *
 * $Id: Vector.h,v 3.18 2004/02/01 02:40:50 milo Exp $
 *
 * */

#ifndef VECTOR_H
#define VECTOR_H

#include "std-includes.h"

template <class TYPE>
class Vector
{
public:
  Vector();
  explicit Vector(int initial_size);  // Construct with an initial max size
  ~Vector();
  const TYPE& ref(int index) const;   // Get an element of the vector
  TYPE& ref(int index);               // Get an element of the vector
  void clear();                       // remove all elements of the vector
  void sortVector();                  // sort all elements using < operator
  int size() const { return m_size; } 
  void setSize(int new_size);         // Increase size, reallocates memory as needed
  void expand(int num) { setSize(m_size+num); } // Increase size by num
  void increaseSize(int new_size, const TYPE& reset);    // and adds num of slots at the bottom set to reset value
  void insertAtTop(const TYPE& element);  // Increase size by one and set last element
    // FIXME - WARNING: insertAtTop is currently O(n) and needs to be fixed
  void insertAtBottom(const TYPE& element);  // Increase size by one and set last element
  TYPE sum() const;  // Uses the += operator to sum all the elements of the vector
  void deletePointers(); // Walks the Vector calling delete on all
                         // elements and sets them to NULL, can only
                         // be used when the TYPE is a pointer type.
  void removeFromTop(int num);  // removes elements from top 
  void print(ostream& out) const;


  // Array Reference operator overloading
  const TYPE& operator[](int index) const { return ref(index); }  
  TYPE& operator[](int index) { return ref(index); }

  // Public copy constructor and assignment operator
  Vector(const Vector& vec);  
  Vector<TYPE>& operator=(const Vector& vec);
private:

  void grow(int new_max_size);  // Expands vector to new_max_size
  
  // Data members
  TYPE* m_vec;           // Array to hold the elements
  int m_size;            // Number of elements in use
  int m_max_size;        // Size of allocated array
};

template <class TYPE> 
ostream& operator<<(ostream& out, const Vector<TYPE>& vec);

// *********************

template <class TYPE>
Vector<TYPE>::Vector()
{ 
  m_size = 0; 
  m_max_size = 0;
  m_vec = NULL;
}

template <class TYPE>
Vector<TYPE>::Vector(int initial_size)
{ 
  m_size = 0; 
  m_max_size = initial_size;
  m_vec = NULL;
  grow(initial_size);
}

template <class TYPE>
Vector<TYPE>::~Vector()
{ 
  delete [] m_vec;  
}

template <class TYPE> 
const TYPE& Vector<TYPE>::ref(int index) const
{
#ifndef NO_VECTOR_BOUNDS_CHECKS
  assert(m_size != 0);
  assert(index < m_size);
  assert(index >= 0);
#endif
  return m_vec[index];
}

template <class TYPE> 
TYPE& Vector<TYPE>::ref(int index)
{
#ifndef NO_VECTOR_BOUNDS_CHECKS
  assert(m_size != 0);
  assert(index < m_size);
  assert(index >= 0);
#endif
  return m_vec[index];
}


template <class TYPE> 
void Vector<TYPE>::setSize(int new_size)
{
  // FIXME - this should also decrease or shrink the size of the array at some point.
  if (new_size > m_max_size) {
    grow(max((m_max_size+1)*2, new_size));
  }
  m_size = new_size;
#ifndef NO_VECTOR_BOUNDS_CHECKS
  assert(m_size <= m_max_size);
  assert(m_size >= 0);
#endif
}

template <class TYPE> 
inline 
void Vector<TYPE>::increaseSize(int new_size, const TYPE& reset)
{
  assert(new_size >= m_size);
  if (new_size >= m_max_size) {
    grow(max((m_max_size+1)*2, new_size));
  }
  int old_size = m_size;
  m_size = new_size;
  for (int j = old_size; j < m_size; j++) {
    ref(j) = reset;
  }
  
#ifndef NO_VECTOR_BOUNDS_CHECKS
    assert(m_size <= m_max_size);
    assert(m_size >= 0);
#endif
}

template <class TYPE> 
inline
void Vector<TYPE>::clear()
{
  m_size = 0;
  m_max_size = 0;
  delete [] m_vec;
  m_vec = NULL;
}

template <class TYPE> 
inline
void Vector<TYPE>::sortVector()
{
  sort(&m_vec[0], &m_vec[m_size]);
}

template <class TYPE> 
inline
void Vector<TYPE>::insertAtTop(const TYPE& element) 
{
  setSize(m_size+1);
  for (int i = m_size-1; i >= 1; i--) {
    ref(i) = ref(i-1);
  }
  ref(0) = element;
} 

template <class TYPE>
inline
void Vector<TYPE>::removeFromTop(int num)
{
  if (num > m_size) {
    num = m_size;
  }
  for (int i = 0; i < m_size - num; i++) {
    m_vec[i] = m_vec[i+num];
  }
  m_size = m_size - num;
  
}

template <class TYPE> 
void Vector<TYPE>::insertAtBottom(const TYPE& element) 
{ 
  setSize(m_size+1);
  ref(m_size-1) = element;
}

template <class TYPE> 
TYPE Vector<TYPE>::sum() const
{ 
  assert(m_size > 0);
  TYPE sum = ref(0);
  for(int i=1; i<m_size; i++) {
    sum += ref(i);
  }
  return sum;
}

template <class TYPE> 
void Vector<TYPE>::deletePointers()
{ 
  assert(m_size >= 0);
  for(int i=0; i<m_size; i++) {
    // FIXME this function should be non-member function, otherwise this
    // prevent template instantiation for non-pointer types
    //
    // Also, there is warning of Switch.C which use void* here
    delete ref(i);
    ref(i) = NULL;
  }
}

template <class TYPE> 
void Vector<TYPE>::print(ostream& out) const
{
  out << "[ ";
  for(int i=0; i<m_size; i++) {
    if (i != 0) {
      out << " ";
    }
    out << ref(i);
  }
  out << " ]";
  out << flush;
}

// Copy constructor
template <class TYPE> 
Vector<TYPE>::Vector(const Vector& vec)
{ 
  // Setup the new memory
  m_size = vec.m_size;
  m_max_size = vec.m_max_size;
  if (m_max_size != 0) {
    m_vec = new TYPE[m_max_size];
    assert(m_vec != NULL);
  } else { 
    m_vec = NULL;
  }
  
  // Copy the elements of the array
  for(int i=0; i<m_size; i++) {
    m_vec[i] = vec.m_vec[i];  // Element copy
  }
}

template <class TYPE> 
Vector<TYPE>& Vector<TYPE>::operator=(const Vector& vec)
{
  if (this == &vec) {
    //    assert(0);
  } else {
    // Free the old memory
    delete [] m_vec;

    // Setup the new memory
    m_size = vec.m_size;
    m_max_size = vec.m_max_size;

    if (m_max_size != 0) {
      m_vec = new TYPE[m_max_size];
      assert(m_vec != NULL);
    } else {
      m_vec = NULL;
    }

    // Copy the elements of the array
    for(int i=0; i<m_size; i++) {
      m_vec[i] = vec.m_vec[i];  // Element copy
    }
  }
  return *this;
}

template <class TYPE> 
void Vector<TYPE>::grow(int new_max_size)
{
  TYPE* temp_vec;
  m_max_size = new_max_size;
  if (new_max_size != 0) {
    temp_vec = new TYPE[new_max_size];
    assert(temp_vec != NULL);
  } else {
    temp_vec = NULL;
  }

  // Copy the elements of the array
  for(int i=0; i<m_size; i++) {
    temp_vec[i] = m_vec[i];  // Element copy
  }
  delete [] m_vec;
  m_vec = temp_vec;
}

template <class TYPE> 
ostream& operator<<(ostream& out, const Vector<TYPE>& vec)
{
  vec.print(out);
  return out;
}

#endif //VECTOR_H
