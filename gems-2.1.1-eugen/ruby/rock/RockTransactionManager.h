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

/*
  The RockTransactionManager class maintains Rock-specific transaction state
  including the fail PC and %cps register.
 */

//--------------------------------------------------------------------------- 80

#ifndef    TRANSACTION_ROCK_MANAGER_H
#define    TRANSACTION_ROCK_MANAGER_H


#include "Rock.h"
#include "Address.h"
#include "interface.h"

class AbstractChip;

class RockTransactionManager {
public:
  RockTransactionManager(AbstractChip* chip_ptr, int version, int thread);
  ~RockTransactionManager();

  void beginTransaction();
  void commitTransaction();

  void setCPS(uint32 cps);
  void setCPS_exog();
  void setCPS_coh();
  void setCPS_tcc();
  void setCPS_inst();
  void setCPS_prec();
  void setCPS_async();
  void setCPS_size();
  void setCPS_ld();
  void setCPS_st();
  void setCPS_cti();
  void setCPS_fp();
  uint32 getCPS();
  void clearCPS();

  bool saveInstShouldAbort();
  bool restoreInstShouldAbort();

  bool inRetryHACK();
  void setInRetryHACK();
  void clearInRetryHACK();

  Address restoreFailPC();
  Address getFailPC();
  void setFailPC(Address failPC);

 private:

  void printCopyright();
  static bool s_CopyrightPrinted;

  uint32 m_cps;
  AbstractChip* m_chip_ptr;
  int m_proc_no;
  int m_thread_no;
  uinteger_t m_failPC;
  int m_executedSave;
  bool m_inRetryHACK;
};


#endif // !TRANSACTION_ROCK_MANAGER_H
