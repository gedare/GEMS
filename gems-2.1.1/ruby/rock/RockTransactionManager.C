/*
   Copyright (C) 2007 Sun Microsystems, Inc.  All rights reserved.
   U.S. Government Rights - Commercial software.  Government users are
   subject to the Sun Microsystems, Inc. standard license agreement and
   applicable provisions of the FAR and its supplements.  Use is subject to
   license terms.  This distribution may include materials developed by
   third parties.Sun, Sun Microsystems and the Sun logo are trademarks or
   registered trademarks of Sun Microsystems, Inc. in the U.S. and other
   countries.  All SPARC trademarks are used under license and are
   trademarks or registered trademarks of SPARC International, Inc. in the
   U.S.  and other countries.

   ----------------------------------------------------------------------

   This file is part of the Adaptive Transactional Memory Test Platform
   (ATMTP) developed and maintained by Kevin Moore and Dan Nussbaum of the
   Scalable Synchronization Research Group at Sun Microsystems Laboratories
   (http://research.sun.com/scalable/).  For information about ATMTP, see
   the GEMS website: http://www.cs.wisc.edu/gems/.

   Please send email to atmtp-interest@sun.com with feedback, questions, or
   to request future announcements about ATMTP.

   ----------------------------------------------------------------------

   ATMTP is distributed as part of the GEMS software toolset and is
   available for use and modification under the terms of version 2 of the
   GNU General Public License.  The GNU General Public License is contained
   in the file $GEMS/LICENSE.

   Multifacet GEMS is free software; you can redistribute it and/or modify
   it under the terms of version 2 of the GNU General Public License as
   published by the Free Software Foundation.
 
   Multifacet GEMS is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.
 
   You should have received a copy of the GNU General Public License along
   with the Multifacet GEMS; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA

   ----------------------------------------------------------------------
*/

#include "RockTransactionManager.h"
#include "AbstractChip.h"
#include "interface.h"
#include "Profiler.h"
#include <assert.h>

bool RockTransactionManager::s_CopyrightPrinted = false;

RockTransactionManager::RockTransactionManager(AbstractChip* chip_ptr, int version, int thread)
{
  assert(ATMTP_ENABLED);
  int procs_per_chip = RubyConfig::numberOfProcsPerChip();
  int smt_threads = RubyConfig::numberofSMTThreads();
  int threads_per_chip = procs_per_chip * smt_threads;
  m_chip_ptr = chip_ptr;
  m_thread_no = thread;
  m_proc_no = chip_ptr->getID() * threads_per_chip + version * smt_threads + thread;

  m_executedSave = false;
  m_inRetryHACK = false;
  m_cps = 0;

  if (!s_CopyrightPrinted){
    s_CopyrightPrinted = true;
    printCopyright();
  }
}

//
// Don't FailPC here -- set it  explictly via setFailPC().
//
void RockTransactionManager::beginTransaction()
{
  m_executedSave = false;
  m_cps = 0;
}

void RockTransactionManager::commitTransaction()
{
}

void RockTransactionManager::setCPS(uint32 cps)
{
  m_cps = m_cps | cps;
}
void RockTransactionManager::setCPS_exog()
{
  m_cps = m_cps | CPS_EXOG;
}
void RockTransactionManager::setCPS_coh()
{
  m_cps = m_cps | CPS_COH;
}
void RockTransactionManager::setCPS_tcc()
{
  m_cps = m_cps | CPS_TCC;
}
void RockTransactionManager::setCPS_inst()
{
  m_cps = m_cps | CPS_INST;
}
void RockTransactionManager::setCPS_prec()
{
  m_cps = m_cps | CPS_PREC;
}
void RockTransactionManager::setCPS_async()
{
  m_cps = m_cps | CPS_ASYNC;
}
void RockTransactionManager::setCPS_size()
{
  m_cps = m_cps | CPS_SIZ;
}
void RockTransactionManager::setCPS_ld()
{
  m_cps = m_cps | CPS_LD;
}
void RockTransactionManager::setCPS_st()
{
  m_cps = m_cps | CPS_ST;
}
void RockTransactionManager::setCPS_cti()
{
  m_cps = m_cps | CPS_CTI;
}
void RockTransactionManager::setCPS_fp()
{
  m_cps = m_cps | CPS_FP;
}
uint32 RockTransactionManager::getCPS()
{
  return m_cps;
}
void RockTransactionManager::clearCPS()
{
  m_cps = 0;
}

//
// Called when a save instruction is executed within a
// transaction. Returns true if the processor should abort.  As of
// 2007-11-12, we hear rock never aborts on a save (only on a restore
// that follows a save).
//
bool RockTransactionManager::saveInstShouldAbort()
{
  Address pc = SIMICS_get_program_counter(m_proc_no);
  physical_address_t myPhysPC = SIMICS_translate_address(m_proc_no, pc);
  integer_t myInst = SIMICS_read_physical_memory(m_proc_no, myPhysPC, 4);
  const char *myInstStr = SIMICS_disassemble_physical(m_proc_no, myPhysPC);
  m_executedSave = true;
  return false;
}

//
// Called when a restore instruction is executed within a transaction.
// Returns true if the processor should abort its transaction. As of
// 2007-11-12, we hear that Rock aborts on a restore if it will
// restore values saved during the transaction.
//
bool RockTransactionManager::restoreInstShouldAbort()
{
  return m_executedSave;
}

bool RockTransactionManager::inRetryHACK()
{
  return m_inRetryHACK;
}

void RockTransactionManager::setInRetryHACK()
{
  m_inRetryHACK = true;
}

void RockTransactionManager::clearInRetryHACK()
{
  m_inRetryHACK = false;
}

Address RockTransactionManager::restoreFailPC()
{
  assert(g_SIMICS);

  Address newPC = Address(m_failPC);
  SIMICS_set_pc(m_proc_no, newPC);

  return newPC;
}

Address RockTransactionManager::getFailPC()
{
  if(g_SIMICS)
    return Address(m_failPC);
  else
    return Address(0);
}

void RockTransactionManager::setFailPC(Address failPC)
{
  assert(g_SIMICS);

  m_failPC = failPC.getAddress();
}

void RockTransactionManager::printCopyright(){
  cout << "   Copyright (C) 2007 Sun Microsystems, Inc.  All rights reserved." << endl
       << "   U.S. Government Rights - Commercial software.  Government users are" << endl
       << "   subject to the Sun Microsystems, Inc. standard license agreement and" << endl
       << "   applicable provisions of the FAR and its supplements.  Use is subject to" << endl
       << "   license terms.  This distribution may include materials developed by" << endl
       << "   third parties.Sun, Sun Microsystems and the Sun logo are trademarks or" << endl
       << "   registered trademarks of Sun Microsystems, Inc. in the U.S. and other" << endl
       << "   countries.  All SPARC trademarks are used under license and are" << endl
       << "   trademarks or registered trademarks of SPARC International, Inc. in the" << endl
       << "   U.S.  and other countries." << endl
       << endl
       << "   The Adaptive Transactional Memory Test Platform (ATMTP) is developed and" << endl
       << "   maintained by Kevin Moore and Dan Nussbaum of the Scalable Synchronization" << endl
       << "   Research Group at Sun Microsystems Laboratories" << endl
       << "   (http://research.sun.com/scalable/).  For information about ATMTP, see" << endl
       << "   the GEMS website: http://www.cs.wisc.edu/gems/." << endl
       << endl
       << "   Please send email to atmtp-interest@sun.com with feedback, questions, or" << endl
       << "   to request future announcements about ATMTP." << endl
       << endl
       << "   ATMTP is distributed as part of the GEMS software toolset and is" << endl
       << "   available for use and modification under the terms of version 2 of the" << endl
       << "   GNU General Public License.  The GNU General Public License is contained" << endl
       << "   in the file $GEMS/LICENSE." << endl
       << endl;
}
