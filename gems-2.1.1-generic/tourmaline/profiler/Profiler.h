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
 * Profiler.h
 *
 * Description: 
 * Used to maintain statistics within the Tourmaline module. Analagous to 
 * Ruby's Profiler.h. There is a single global profiler object, g_profiler, which
 * is invoked to modify statistics as needed.
 * 
 * $Id: Profiler.h 1.00 05/11/09 14:02:03-05:00 gibson@mustardseed.cs.wisc.edu $
 *
 */

#ifndef PROFILER_H
#define PROFILER_H

#include "Tourmaline_Global.h"
#include "TourmalineConfig.h"
#include "Histogram.h"

template <class KEY_TYPE, class VALUE_TYPE> class Map;

class Profiler {
public:
  // Constructors
  Profiler();

  // Destructor
  ~Profiler();  
  
  void printStats(ostream& out);
  void clearStats();
  void printResourceUsage(ostream& out) const;

  void beginTransaction(int cpu, int xactno=0);
  void endTransaction(int cpu, int xactno=0);

  void print(ostream& out) const;

  integer_t getInstructionsExecuted( int cpu );
  integer_t getTotalInstructionsExecuted();
  
private:
  Profiler(const Profiler& obj);
  Profiler& operator=(const Profiler& obj);

  time_t m_real_time_start_time;

  Vector<integer_t> m_per_proc_instructions_at_start;

  Vector<integer_t> m_per_proc_begin_xact_count;
  Vector<integer_t> m_per_proc_end_xact_count;
  Vector<integer_t> m_per_proc_xact_nesting_level;
  Vector<integer_t> m_per_proc_xact_highest_nesting;

  integer_t m_deepest_nest_encountered;

  int m_unmatched_end_transactions;
};

// Output operator declaration
ostream& operator<<(ostream& out, const Profiler& obj);

// ******************* Definitions *******************

// Output operator definition
extern inline 
ostream& operator<<(ostream& out, const Profiler& obj)
{
  obj.print(out);
  out << flush;
  return out;
}

#endif // PROFILER_H


