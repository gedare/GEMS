
/*
    Copyright (C) 1999-2008 by Mark D. Hill and David A. Wood for the
    Wisconsin Multifacet Project.  Contact: gems@cs.wisc.edu
    http://www.cs.wisc.edu/gems/

    --------------------------------------------------------------------

    This file is part of the Ruby Multiprocessor Memory System Simulator, 
    a component of the Multifacet GEMS (General Execution-driven 
    Multiprocessor Simulator) software toolset originally developed at 
    the University of Wisconsin-Madison.

    Ruby was originally developed primarily by Milo Martin and Daniel
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
 * $Id$
 */

#ifndef DEBUG_H
#define DEBUG_H

#include <unistd.h>
#include <iostream>

extern std::ostream * debug_cout_ptr;

// component enumeration
enum DebugComponents
{
#undef DEFINE_COMP
#define DEFINE_COMP(component, character, description) component,
#include "Debug.def"
  NUMBER_OF_COMPS
};

enum PriorityLevel {HighPrio, MedPrio, LowPrio};
enum VerbosityLevel {No_Verb, Low_Verb, Med_Verb, High_Verb};

class Debug {
public:
  // Constructors
  Debug( const char *filterString, const char *verboseString,
         Time filterStartTime, const char *filename );

  // Destructor
  ~Debug();
  
  // Public Methods
  bool validDebug(int module, PriorityLevel priority);
  void printVerbosity(ostream& out) const;
  void setVerbosity(VerbosityLevel vb);
  static bool checkVerbosityString(const char *verb_str);
  bool setVerbosityString(const char *);
  VerbosityLevel getVerbosity() const { return m_verbosityLevel; }
  void setFilter(int);
  static bool checkFilter( char);
  static bool checkFilterString(const char *);
  bool setFilterString(const char *);
  void setDebugTime(Time);
  Time getDebugTime() const { return m_starting_cycle; }
  bool addFilter(char);
  void clearFilter();
  void allFilter();
  void print(ostream& out) const;
  /* old school debugging "vararg": sends messages to screen and log */
  void debugMsg( const char *fmt, ... );

  void setDebugOutputFile (const char * filename);
  void closeDebugOutputFile ();
  static void usageInstructions(void);

private:
  // Private Methods

  // Private copy constructor and assignment operator
  Debug(const Debug& obj);
  Debug& operator=(const Debug& obj);
  
  // Data Members (m_ prefix)
  VerbosityLevel m_verbosityLevel;
  int m_filter;
  Time m_starting_cycle;

  std::fstream m_fout;
};

// Output operator declaration
ostream& operator<<(ostream& out, const Debug& obj);

// ******************* Definitions *******************

// Output operator definition
extern inline 
ostream& operator<<(ostream& out, const Debug& obj)
{
  obj.print(out);
  out << flush;
  return out;
}

const bool ERROR_MESSAGE_FLAG = true;
const bool WARNING_MESSAGE_FLAG = true;

#ifdef RUBY_NO_ASSERT
const bool ASSERT_FLAG = false;
#else
const bool ASSERT_FLAG = true;
#endif

#undef assert
#define assert(EXPR) ASSERT(EXPR)
#undef ASSERT
#define ASSERT(EXPR)\
{\
  if (ASSERT_FLAG) {\
    if (!(EXPR)) {\
      cerr << "failed assertion '"\
           << #EXPR << "' at fn "\
           << __PRETTY_FUNCTION__ << " in "\
           << __FILE__ << ":"\
           << __LINE__ << endl << flush;\
      (* debug_cout_ptr) << "failed assertion '"\
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
  }\
}

#define BREAK(X)\
{\
    cerr << "breakpoint '"\
         << #X << "' reached at fn "\
         << __PRETTY_FUNCTION__ << " in "\
         << __FILE__ << ":"\
         << __LINE__ << endl << flush;\
    if(isatty(STDIN_FILENO)) {\
      cerr << "press enter to continue" << endl;\
      cerr << "PID: " << getpid();\
      cerr << endl << flush; \
      char c; \
      cin.get(c); \
    }\
}

#define ERROR_MSG(MESSAGE)\
{\
  if (ERROR_MESSAGE_FLAG) {\
    cerr << "Fatal Error: in fn "\
         << __PRETTY_FUNCTION__ << " in "\
         << __FILE__ << ":"\
         << __LINE__ << ": "\
         << (MESSAGE) << endl << flush;\
    (* debug_cout_ptr) << "Fatal Error: in fn "\
         << __PRETTY_FUNCTION__ << " in "\
         << __FILE__ << ":"\
         << __LINE__ << ": "\
         << (MESSAGE) << endl << flush;\
    abort();\
  }\
}

#define WARN_MSG(MESSAGE)\
{\
  if (WARNING_MESSAGE_FLAG) {\
    cerr << "Warning: in fn "\
         << __PRETTY_FUNCTION__ << " in "\
         << __FILE__ << ":"\
         << __LINE__ << ": "\
         << (MESSAGE) << endl << flush;\
    (* debug_cout_ptr) << "Warning: in fn "\
         << __PRETTY_FUNCTION__ << " in "\
         << __FILE__ << ":"\
         << __LINE__ << ": "\
         << (MESSAGE) << endl << flush;\
  }\
}

#define WARN_EXPR(EXPR)\
{\
  if (WARNING_MESSAGE_FLAG) {\
    cerr << "Warning: in fn "\
         << __PRETTY_FUNCTION__ << " in "\
         << __FILE__ << ":"\
         << __LINE__ << ": "\
         << #EXPR << " is "\
         << (EXPR) << endl << flush;\
    (* debug_cout_ptr) << "Warning: in fn "\
         << __PRETTY_FUNCTION__ << " in "\
         << __FILE__ << ":"\
         << __LINE__ << ": "\
         << #EXPR << " is "\
         << (EXPR) << endl << flush;\
  }\
}

#define DEBUG_MSG(module, priority, MESSAGE)\
{\
  if (RUBY_DEBUG) {\
    if (g_debug_ptr->validDebug(module, priority)) {\
      (* debug_cout_ptr) << "Debug: in fn "\
           << __PRETTY_FUNCTION__\
           << " in " << __FILE__ << ":"\
           << __LINE__ << ": "\
           << (MESSAGE) << endl << flush;\
    }\
  }\
}

#define DEBUG_EXPR(module, priority, EXPR)\
{\
  if (RUBY_DEBUG) {\
    if (g_debug_ptr->validDebug(module, priority)) {\
      (* debug_cout_ptr) << "Debug: in fn "\
           << __PRETTY_FUNCTION__\
           << " in " << __FILE__ << ":"\
           << __LINE__ << ": "\
           << #EXPR << " is "\
           << (EXPR) << endl << flush;\
    }\
  }\
}

#define DEBUG_NEWLINE(module, priority)\
{\
  if (RUBY_DEBUG) {\
    if (g_debug_ptr->validDebug(module, priority)) {\
      (* debug_cout_ptr) << endl << flush;\
    }\
  }\
}

#define DEBUG_SLICC(priority, LINE, MESSAGE)\
{\
  if (RUBY_DEBUG) {\
    if (g_debug_ptr->validDebug(SLICC_COMP, priority)) {\
      (* debug_cout_ptr) << (LINE) << (MESSAGE) << endl << flush;\
    }\
  }\
}

#define DEBUG_OUT( rest... ) \
{\
  if (RUBY_DEBUG) {\
    cout << "Debug: in fn "\
         << __PRETTY_FUNCTION__\
         << " in " << __FILE__ << ":"\
         << __LINE__ << ": "; \
    g_debug_ptr->debugMsg(rest); \
  }\
}

#define ERROR_OUT( rest... ) \
{\
  if (ERROR_MESSAGE_FLAG) {\
    cout << "error: in fn "\
         << __PRETTY_FUNCTION__ << " in "\
         << __FILE__ << ":"\
         << __LINE__ << ": ";\
    g_debug_ptr->debugMsg(rest); \
  }\
}

#endif //DEBUG_H

