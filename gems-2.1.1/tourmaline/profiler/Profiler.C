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
 * Profiler.C
 *
 * Description: 
 * See Profiler.h
 * 
 * $Id: Profiler.C 1.00 05/11/09 14:02:03-05:00 gibson@mustardseed.cs.wisc.edu $
 *
 */

#include "Profiler.h"
#include "Map.h"
#include "interface.h"

// Allows use of times() library call, which determines virtual runtime
#include <sys/times.h>

static double process_memory_total();
static double process_memory_resident();

Profiler::Profiler()
{

  m_real_time_start_time = time(NULL); // Not reset in clearStats()
  
  clearStats();

}

Profiler::~Profiler()
{

}

void Profiler::print(ostream& out) const
{
  out << "[Profiler]";
}

void Profiler::printStats(ostream& out)
{
  out << endl;

  out << "Profiler Stats" << endl;
  out << "--------------" << endl;

  time_t real_time_current = time(NULL);
  double seconds = difftime(real_time_current, m_real_time_start_time);
  double minutes = seconds/60.0;
  double hours = minutes/60.0;
  double days = hours/24.0;
 
  out << "Elapsed_time_in_seconds: " << seconds << endl;
  out << "Elapsed_time_in_minutes: " << minutes << endl;
  out << "Elapsed_time_in_hours: " << hours << endl;
  out << "Elapsed_time_in_days: " << days << endl;
  out << endl;

  // print the virtual runtimes as well
  struct tms vtime;
  times(&vtime);
  seconds = (vtime.tms_utime + vtime.tms_stime) / 100.0;
  minutes = seconds / 60.0;
  hours = minutes / 60.0;
  days = hours / 24.0;
  out << "Virtual_time_in_seconds: " << seconds << endl;
  out << "Virtual_time_in_minutes: " << minutes << endl;
  out << "Virtual_time_in_hours:   " << hours << endl;
  out << "Virtual_time_in_days:    " << hours << endl;
  out << endl;
  
  for(int i=0;i<TourmalineConfig::numberOfProcessors();i++) {
    out << "Total_instructions_proc" << i << ":  " << getInstructionsExecuted(i) << endl;
  }

  out << "Total_instructions_all_procs: " << getTotalInstructionsExecuted() << endl;
  out << endl;
  
  for(int i=0;i<TourmalineConfig::numberOfProcessors();i++) {
    out << "begin_xact_proc" << i << ":  " << m_per_proc_begin_xact_count[i] << endl;
    out << "end_xact_proc" << i << ":  " << m_per_proc_end_xact_count[i] << endl;
  }  
  
  out << endl;
  out << "deepest_nesting_level: " << m_deepest_nest_encountered << endl;
  out << "unmatched_end_xact_calls: " << m_unmatched_end_transactions << endl;
  out << endl;
  
  printResourceUsage(out);
}

void Profiler::printResourceUsage(ostream& out) const
{
  out << "Resource Usage" << endl;
  out << "--------------" << endl;
  
  out << "mbytes_resident: " << process_memory_resident() << endl;
  out << "mbytes_total: " << process_memory_total() << endl;
  if (process_memory_total() > 0) {
    out << "resident_ratio: " << process_memory_resident()/process_memory_total() << endl;
  }
  out << endl;

  integer_t pagesize = getpagesize(); // page size in bytes
  out << "page_size: " << pagesize << endl;
  
  rusage usage;
  getrusage (RUSAGE_SELF, &usage);

  out << "user_time: " << usage.ru_utime.tv_sec << endl;
  out << "system_time: " << usage.ru_stime.tv_sec << endl;
  out << "page_reclaims: " << usage.ru_minflt << endl;
  out << "page_faults: " << usage.ru_majflt << endl;
  out << "swaps: " << usage.ru_nswap << endl;
  out << "block_inputs: " << usage.ru_inblock << endl;
  out << "block_outputs: " << usage.ru_oublock << endl;
}

void Profiler::clearStats()
{
  m_per_proc_instructions_at_start.setSize(TourmalineConfig::numberOfProcessors()); 
  m_per_proc_begin_xact_count.setSize(TourmalineConfig::numberOfProcessors());
  m_per_proc_end_xact_count.setSize(TourmalineConfig::numberOfProcessors());
  m_per_proc_xact_nesting_level.setSize(TourmalineConfig::numberOfProcessors());
  m_per_proc_xact_highest_nesting.setSize(TourmalineConfig::numberOfProcessors());

  for(int i=0;i<TourmalineConfig::numberOfProcessors();i++) {
    m_per_proc_instructions_at_start[i] = SIMICS_get_insn_count(i);
    m_per_proc_begin_xact_count[i] = 0;
    m_per_proc_end_xact_count[i] = 0;
    m_per_proc_xact_nesting_level[i] = 0;
    m_per_proc_xact_highest_nesting[i] = 0;
  }

  m_deepest_nest_encountered = 0;
  m_unmatched_end_transactions = 0;
}

void Profiler::beginTransaction( int cpu, int xactno) {
  m_per_proc_begin_xact_count[cpu]++;
  m_per_proc_xact_nesting_level[cpu]++;

  cout << "  " << cpu << " BEGIN " << xactno 
       << " level " << m_per_proc_xact_nesting_level[cpu] 
       << endl;

  if(m_per_proc_xact_nesting_level[cpu] > m_per_proc_xact_highest_nesting[cpu]) {
    m_per_proc_xact_highest_nesting[cpu] = m_per_proc_xact_nesting_level[cpu];
  }
  
}

void Profiler::endTransaction( int cpu, int xactno ) {

  m_per_proc_end_xact_count[cpu]++;
  m_per_proc_xact_nesting_level[cpu]--;

  cout << "  " << cpu << " COMMIT " << xactno 
       << " level " << m_per_proc_xact_nesting_level[cpu] 
       << endl;

  if(m_per_proc_xact_nesting_level[cpu] == -1) {
    m_per_proc_xact_nesting_level[cpu] = 0;
    m_unmatched_end_transactions++;
  }
  
  if(m_per_proc_xact_nesting_level[cpu] == 0) {
    if(m_per_proc_xact_highest_nesting[cpu] > m_deepest_nest_encountered) {
      m_deepest_nest_encountered = m_per_proc_xact_highest_nesting[cpu];
    }

    m_per_proc_xact_highest_nesting[cpu] = 0;
  }

}

integer_t Profiler::getInstructionsExecuted(int cpu) {
  return SIMICS_get_insn_count(cpu) - m_per_proc_instructions_at_start[cpu];
}

integer_t Profiler::getTotalInstructionsExecuted() {
  integer_t total = 0;
  for(int i=0;i<TourmalineConfig::numberOfProcessors();i++) {
    total += getInstructionsExecuted(i);
  }
  return total;
}

// Helper function
static double process_memory_total()
{
  const double MULTIPLIER = 4096.0/(1024.0*1024.0); // 4kB page size, 1024*1024 bytes per MB, 
  ifstream proc_file;
  proc_file.open("/proc/self/statm");
  int total_size_in_pages = 0;
  int res_size_in_pages = 0;
  proc_file >> total_size_in_pages;
  proc_file >> res_size_in_pages;
  return double(total_size_in_pages)*MULTIPLIER; // size in megabytes
}

static double process_memory_resident()
{
  const double MULTIPLIER = 4096.0/(1024.0*1024.0); // 4kB page size, 1024*1024 bytes per MB, 
  ifstream proc_file;
  proc_file.open("/proc/self/statm");
  int total_size_in_pages = 0;
  int res_size_in_pages = 0;
  proc_file >> total_size_in_pages;
  proc_file >> res_size_in_pages;
  return double(res_size_in_pages)*MULTIPLIER; // size in megabytes
}


