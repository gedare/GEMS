/*
    Copyright (C) 1999-2008 by Mark D. Hill and David A. Wood for the
    Wisconsin Multifacet Project.  Contact: gems@cs.wisc.edu
    http://www.cs.wisc.edu/gems/

    --------------------------------------------------------------------

    This file is part of the Tourmaline Transactional Memory Acclerator, 
    a component of the Multifacet GEMS (General Execution-driven 
    Multiprocessor Simulator) software toolset originally developed at 
    the University of Wisconsin-Madison.

    Tourmaline was originally developed primarily by Dan Gibson, but was
    based on work in the Ruby module performed by Milo Martin and Daniel
    Sorin with contributions from Ross Dickson, Carl Mauer, and Manoj
    Plakal.

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
 * Tourmaline_Global.h
 *
 * Description: 
 * Tourmaline-wide "global" include file. Analagous to Ruby's Global.h. Defines ERROR and
 * WARNING macros, and global variables, as well as standard includes, etc.
 * 
 * $Id: Tourmaline_Global.h 1.00 05/11/09 14:02:03-05:00 gibson@mustardseed.cs.wisc.edu $
 *
 */

#ifndef TOURMALINE_GLOBAL_H
#define TOURMALINE_GLOBAL_H

typedef unsigned char uint8;
typedef unsigned int uint32;
typedef unsigned long long uint64;

typedef signed char int8;
typedef int int32;
typedef long long int64;

typedef long long integer_t;
typedef unsigned long long uinteger_t;

typedef uint64 physical_address_t;
typedef uint64 logical_address_t;

// external includes for all classes
#include "std-includes.h"

// some things may be missed by std-includes.h
#ifndef getpid
#include <sys/types.h>
#endif

class TransactionController;
extern TransactionController *g_trans_control;

class Profiler;
extern Profiler *g_profiler;

// this does error printing as needed by some macros
void tourmalineprintf(const char * format, ... );

// Redefine assert (and define ASSERT) to use Ruby-style asserting
#undef assert
#define assert(EXPR) ASSERT(EXPR)
#undef ASSERT
#define ASSERT(EXPR)\
{\
  if (!(EXPR)) {\
    cerr << "failed assertion '"\
         << #EXPR << "' at fn "\
         << __PRETTY_FUNCTION__ << " in "\
         << __FILE__ << ":"\
         << __LINE__ << endl << flush;\
    cerr << "failed assertion '"\
         << #EXPR << "' at fn "\
         << __PRETTY_FUNCTION__ << " in "\
         << __FILE__ << ":"\
         << __LINE__ << endl << flush;\
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
}

#define ERROR_OUT( rest... ) ERROR( rest )
#define ERROR_MSG( rest... ) ERROR( rest )

#define ERROR( rest... ) \
{\
  cout << "error: in fn "\
       << __PRETTY_FUNCTION__ << " in "\
       << __FILE__ << ":"\
       << __LINE__ << ": ";\
  tourmalineprintf(rest); \
}

#define DEBUG_OUT( rest...) DEBUG( rest )

#define DEBUG( rest... ) \
{\
  cout << "Debug: in fn "\
       << __PRETTY_FUNCTION__\
       << " in " << __FILE__ << ":"\
       << __LINE__ << ": "; \
  tourmalineprintf(rest); \
}

#define WARN_OUT( rest... ) WARNING( rest )
#define WARN_MSG( rest... ) WARNING( rest )
#define WARNING( rest... ) \
{\
  cout << "Warning: in fn "\
       << __PRETTY_FUNCTION__\
       << " in " << __FILE__ << ":"\
       << __LINE__ << ": "; \
  tourmalineprintf(rest); \
}

#endif // TOURMALINE_GLOBAL_H

