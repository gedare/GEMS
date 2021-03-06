
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
 *
 */

#include <fstream>
#include <stdarg.h>

#include "Global.h"
#include "Debug.h"
#include "EventQueue.h"

#include "string.h"
class Debug;
extern Debug* g_debug_ptr;
std::ostream * debug_cout_ptr;

// component character list
const char DEFINE_COMP_CHAR[] =
{
#undef DEFINE_COMP
#define DEFINE_COMP(component, character, description) character,
#include "Debug.def"
};

// component description list
const char* DEFINE_COMP_DESCRIPTION[] =
{
#undef DEFINE_COMP
#define DEFINE_COMP(component, character, description) description,
#include "Debug.def"
};

extern "C" void changeDebugVerbosity(VerbosityLevel vb);
extern "C" void changeDebugFilter(int filter);

void changeDebugVerbosity(VerbosityLevel vb)
{
  g_debug_ptr->setVerbosity(vb);
}

void changeDebugFilter(int filter)
{
  g_debug_ptr->setFilter(filter);
}

Debug::Debug( const char *filterString, const char *verboseString,
              Time filterStartTime, const char *filename )
{
  m_verbosityLevel = No_Verb;
  clearFilter();
  debug_cout_ptr = &cout;
  
  m_starting_cycle = filterStartTime;
  setFilterString( filterString );
  setVerbosityString( verboseString );
  setDebugOutputFile( filename );
}

Debug::~Debug()
{
}

void Debug::printVerbosity(ostream& out) const
{
  switch (getVerbosity()) {
  case No_Verb:
    out << "verbosity = No_Verb" << endl;
    break;
  case Low_Verb:
    out << "verbosity = Low_Verb" << endl;
    break;
  case Med_Verb:
    out << "verbosity = Med_Verb" << endl;
    break;
  case High_Verb:
    out << "verbosity = High_Verb" << endl;
    break;
  default:
    out << "verbosity = unknown" << endl;
  }
}

bool Debug::validDebug(int module, PriorityLevel priority)
{
  int local_module = (1 << module);
  if(m_filter & local_module) {
    if (g_eventQueue_ptr == NULL || 
        g_eventQueue_ptr->getTime() >= m_starting_cycle) {
      switch(m_verbosityLevel) {
      case No_Verb:
        return false;
        break;
      case Low_Verb:
        if(priority == HighPrio) {
          return true;
        }else{
          return false;
        }
        break;
      case Med_Verb:
        if(priority == HighPrio || priority == MedPrio ) {
          return true;
        }else{
          return false;
        }
        break;
      case High_Verb:
        return true;
        break;
      }
    }
  }
  return false;
}

void Debug::setDebugTime(Time t)
{
  m_starting_cycle = t;
}

void Debug::setVerbosity(VerbosityLevel vb)
{
  m_verbosityLevel = vb;
}

void Debug::setFilter(int filter)
{
  m_filter = filter;
}

bool Debug::checkVerbosityString(const char *verb_str)
{
  if (verb_str == NULL) {
    cerr << "Error: unrecognized verbosity (use none, low, med, high): NULL" << endl;
    return true; // error
  } else if ( (string(verb_str) == "none") ||
              (string(verb_str) == "low") ||
              (string(verb_str) == "med") ||
              (string(verb_str) == "high") ) {
    return false;
  }
  cerr << "Error: unrecognized verbosity (use none, low, med, high): NULL" << endl;
  return true; // error
}

bool Debug::setVerbosityString(const char *verb_str)
{
  bool check_fails = checkVerbosityString(verb_str);
  if (check_fails) {
    return true; // error
  }
  if (string(verb_str) == "none") {
    setVerbosity(No_Verb);
  } else if (string(verb_str) == "low") {
    setVerbosity(Low_Verb);
  } else if (string(verb_str) == "med") {
    setVerbosity(Med_Verb);
  } else if (string(verb_str) == "high") {
    setVerbosity(High_Verb);
  } else {
    cerr << "Error: unrecognized verbosity (use none, low, med, high): " << verb_str << endl;
    return true; // error
  }
  return false; // no error
}

bool Debug::checkFilter(char ch)
{
  for (int i=0; i<NUMBER_OF_COMPS; i++) {
    // Look at all components to find a character match
    if (DEFINE_COMP_CHAR[i] == ch) {
      // We found a match - return no error
      return false; // no error
    }
  }
  return true; // error
}

bool Debug::checkFilterString(const char *filter_str)
{
  if (filter_str == NULL) {
    cerr << "Error: unrecognized component filter: NULL" << endl;
    return true; // error
  }

  // check for default filter ("none") before reporting RUBY_DEBUG error
  if ( (string(filter_str) == "none") ) {
    return false; // no error
  }
  
  if (RUBY_DEBUG == false) {
    cerr << "Error: User specified set of debug components, but the RUBY_DEBUG compile-time flag is false." << endl;
    cerr << "Solution: Re-compile with RUBY_DEBUG set to true." << endl;
    return true; // error
  }
  
  if ( (string(filter_str) == "all") ) {
    return false; // no error
  }

  // scan string checking each character
  for (unsigned int i = 0; i < strlen(filter_str); i++) {
    bool unrecognized = checkFilter( filter_str[i] );
    if (unrecognized == true) {
      return true; // error
    }
  }
  return false; // no error
}

bool Debug::setFilterString(const char *filter_str)
{
  if (checkFilterString(filter_str)) {
    return true; // error
  }

  if (string(filter_str) == "all" ) {
    allFilter();
  } else if (string(filter_str) == "none") {
    clearFilter();
  } else {
    // scan string adding to bit mask for each component which is present
    for (unsigned int i = 0; i < strlen(filter_str); i++) {
      bool error = addFilter( filter_str[i] );
      if (error) {
        return true; // error
      }
    }
  }
  return false; // no error
}

bool Debug::addFilter(char ch)
{
  for (int i=0; i<NUMBER_OF_COMPS; i++) {
    // Look at all components to find a character match
    if (DEFINE_COMP_CHAR[i] == ch) {
      // We found a match - update the filter bit mask
      cout << "  Debug: Adding to filter: '" << ch << "' (" << DEFINE_COMP_DESCRIPTION[i] << ")" << endl;
      m_filter |= (1 << i);
      return false; // no error
    }
  }

  // We didn't find the character
  cerr << "Error: unrecognized component filter: " << ch << endl;
  usageInstructions();
  return true; // error
}

void Debug::clearFilter()
{
  m_filter = 0;
}

void Debug::allFilter()
{
  m_filter = ~0;
}

void Debug::usageInstructions(void)
{
  cerr << "Debug components: " << endl;
  for (int i=0; i<NUMBER_OF_COMPS; i++) {
    cerr << "  " << DEFINE_COMP_CHAR[i] << ": " << DEFINE_COMP_DESCRIPTION[i] << endl;
  }
}

void Debug::print(ostream& out) const
{
  out << "[Debug]" << endl;
}

void Debug::setDebugOutputFile (const char * filename)
{
  if ( (filename == NULL) ||
       (!strcmp(filename, "none")) ) {
    debug_cout_ptr = &cout;
    return;
  }

  if (m_fout.is_open() ) {
    m_fout.close ();
  }
  m_fout.open (filename, std::ios::out);
  if (! m_fout.is_open() ) {
    cerr << "setDebugOutputFile: can't open file " << filename << endl;
  }
  else {
    debug_cout_ptr = &m_fout;
  }
}

void Debug::closeDebugOutputFile ()
{
  if (m_fout.is_open() ) {
    m_fout.close ();
    debug_cout_ptr = &cout;
  }
}

void Debug::debugMsg( const char *fmt, ... )
{
  va_list  args;

  // you could check validDebug() here before printing the message
  va_start(args, fmt);
  vfprintf(stdout, fmt, args);
  va_end(args);
}

/*
void DEBUG_OUT( const char* fmt, ...) {
  if (RUBY_DEBUG) {
    cout << "Debug: in fn "
         << __PRETTY_FUNCTION__
         << " in " << __FILE__ << ":"
         << __LINE__ << ": ";
    va_list  args;
    va_start(args, fmt);
    vfprintf(stdout, fmt, args);
    va_end(args);
  }
}

void ERROR_OUT( const char* fmt, ... ) {
  if (ERROR_MESSAGE_FLAG) {
    cout << "error: in fn "
         << __PRETTY_FUNCTION__ << " in "
         << __FILE__ << ":"
         << __LINE__ << ": ";
    va_list  args;
    va_start(args, fmt);
    vfprintf(stdout, fmt, args);
    va_end(args);
  }
  assert(0);
}
*/

