
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


#ifndef SLICC_GLOBAL_H
#define SLICC_GLOBAL_H

#include <assert.h> /* slicc needs to include this in order to use classes in
                     * ../common directory.
                     */

#include "std-includes.h"
#include "Map.h"

typedef unsigned char uint8;
typedef unsigned int uint32;
typedef unsigned long long uint64;

typedef signed char int8;
typedef int int32;
typedef long long int64;

typedef long long integer_t;
typedef unsigned long long uinteger_t;

const bool ASSERT_FLAG = true;

// when CHECK_RESOURCE_DEADLOCK is enabled, slicc will generate additional code
// that works in conjuction with the resources rank value specified in the protocol
// to detect invalid resource stalls as soon as they occur.
const bool CHECK_INVALID_RESOURCE_STALLS = false;

#undef assert
#define assert(EXPR) ASSERT(EXPR)

#define ASSERT(EXPR)\
{\
  if (ASSERT_FLAG) {\
    if (!(EXPR)) {\
      cerr << "failed assertion '"\
           << #EXPR << "' at fn "\
           << __PRETTY_FUNCTION__ << " in "\
           << __FILE__ << ":"\
           << __LINE__ << endl;\
      if(isatty(STDIN_FILENO)) {\
        cerr << "At this point you might want to attach a debug to ";\
        cerr << "the running and get to the" << endl;\
        cerr << "crash site; otherwise press enter to continue" << endl;\
        cerr << "PID: " << getpid();\
        cerr << endl << flush; \
        char c; \
        cin.get(c); \
      }\
      abort();\
    }\
  }\
}

class State;
class Event;
class Symbol;
class Var;

namespace __gnu_cxx {
  template <> struct hash<State*>
  {
    size_t operator()(State* s) const { return (size_t) s; }
  };
  template <> struct hash<Event*>
  {
    size_t operator()(Event* s) const { return (size_t) s; }
  };
  template <> struct hash<Symbol*>
  {
    size_t operator()(Symbol* s) const { return (size_t) s; }
  };
  template <> struct hash<Var*>
  {
    size_t operator()(Var* s) const { return (size_t) s; }
  };
} // namespace __gnu_cxx

namespace std {
  template <> struct equal_to<Event*>
  {
    bool operator()(Event* s1, Event* s2) const { return s1 == s2; }
  };
  template <> struct equal_to<State*>
  {
    bool operator()(State* s1, State* s2) const { return s1 == s2; }
  };
  template <> struct equal_to<Symbol*>
  {
    bool operator()(Symbol* s1, Symbol* s2) const { return s1 == s2; }
  };
  template <> struct equal_to<Var*>
  {
    bool operator()(Var* s1, Var* s2) const { return s1 == s2; }
  };
} // namespace std

#endif //SLICC_GLOBAL_H
