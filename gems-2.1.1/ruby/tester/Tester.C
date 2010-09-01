
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

#include "Global.h"
#include "System.h"
#include "Tester.h"
#include "EventQueue.h"
#include "SubBlock.h"
#include "Check.h"
#include "Chip.h"

Tester::Tester(System* sys_ptr)
{
  if (g_SIMICS) {
    ERROR_MSG("g_SIMICS should not be defined.");
  }

  g_callback_counter = 0;

  // add the tester consumer to the global event queue
  g_eventQueue_ptr->scheduleEvent(this, 1);

  m_last_progress_vector.setSize(RubyConfig::numberOfProcessors());
  for (int i=0; i<m_last_progress_vector.size(); i++) {
    m_last_progress_vector[i] = 0;
  }
}

Tester::~Tester()
{
}

void Tester::hitCallback(NodeID proc, SubBlock& data, CacheRequestType type, int thread)
{
  // Mark that we made progress
  m_last_progress_vector[proc] = g_eventQueue_ptr->getTime();
  g_callback_counter++;

  // This tells us our store has 'completed' or for a load gives us
  // back the data to make the check
  DEBUG_EXPR(TESTER_COMP, MedPrio, proc);
  DEBUG_EXPR(TESTER_COMP, MedPrio, data);
  Check* check_ptr = m_checkTable.getCheck(data.getAddress());
  assert(check_ptr != NULL);
  check_ptr->performCallback(proc, data);

}

void Tester::wakeup()
{ 
  if (g_callback_counter < g_tester_length) {
    // Try to perform an action or check
    Check* check_ptr = m_checkTable.getRandomCheck();
    assert(check_ptr != NULL);
    check_ptr->initiate();
    
    checkForDeadlock();
    
    g_eventQueue_ptr->scheduleEvent(this, 2);
  }
}

void Tester::checkForDeadlock()
{
  int size = m_last_progress_vector.size();
  Time current_time = g_eventQueue_ptr->getTime();
  for (int processor=0; processor<size; processor++) {
    if ((current_time - m_last_progress_vector[processor]) > g_DEADLOCK_THRESHOLD) {
      WARN_EXPR(current_time);
      WARN_EXPR(m_last_progress_vector[processor]);
      WARN_EXPR(current_time - m_last_progress_vector[processor]);
      WARN_EXPR(processor);
      Sequencer* seq_ptr = g_system_ptr->getChip(processor/RubyConfig::numberOfProcsPerChip())->getSequencer(processor%RubyConfig::numberOfProcsPerChip());
      assert(seq_ptr != NULL);
      WARN_EXPR(*seq_ptr);
      ERROR_MSG("Deadlock detected.");
    }
  }
}

void Tester::print(ostream& out) const
{
  out << "[Tester]" << endl;
}

