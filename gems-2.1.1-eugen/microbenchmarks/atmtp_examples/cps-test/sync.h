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

#ifndef    _sync_H
#define    _sync_H

#include <unistd.h>

#ifdef    __cplusplus
extern "C" {
#endif // __cplusplus
  int32_t sysCompareAndSwap(int32_t o0, int32_t o1, int32_t *o2);
  int64_t sysCompareAndSwap64(int64_t o0, int64_t o1, int64_t *o2);

  int64_t sysPopc(int64_t o0);

  void membarstoreload();

  void* getCurrentCFrame();

  void failHardwareTransaction();

  // Simics "magic" instructions for debugging.  In a non-simics
  // environment, these are just noops.
  //
  void emit_magic_instruction_1();
  void emit_magic_instruction_2();

  void emit_magic_instruction_6();
  void emit_magic_instruction_7();
  void emit_magic_instruction_8();
  void emit_magic_instruction_9();
  void emit_magic_instruction_10();
  void emit_magic_instruction_11();
  void emit_magic_instruction_12();
  void emit_magic_instruction_13();
  void emit_magic_instruction_14();

  void emit_magic_instruction_16();
  void emit_magic_instruction_17();
  void emit_magic_instruction_18();
  void emit_magic_instruction_19();
  void emit_magic_instruction_20();
  void emit_magic_instruction_21();
  void emit_magic_instruction_22();
  void emit_magic_instruction_23();
  void emit_magic_instruction_24();
  void emit_magic_instruction_25();
  void emit_magic_instruction_26();
  void emit_magic_instruction_27();
  void emit_magic_instruction_28();
  void emit_magic_instruction_29();

  extern uint64_t rdtick();
  extern uint64_t rdstick();

  // Non-faulting LD.  Can easily be emulated with trap handlers on
  // SIGSEGV, SIGBUS, SIGILL.
  //
  intptr_t LDNF   (intptr_t * a) ;

  // We use PrefetchW in LD...CAS and LD...ST circumstances to force the
  // $line directly into M-state, avoiding RTS->RTO upgrade txns.
  //
  void PrefetchW  (void * a) ;
#ifdef    __cplusplus
}
#endif // __cplusplus

#define CAS64(A,O,N) \
  (sysCompareAndSwap64((int64_t)N,(int64_t)O,(int64_t *)A)==(int64_t)O)
#define CAS32(A,O,N) \
  (sysCompareAndSwap((int32_t)N,(int32_t)O,(int32_t *)A)==(int32_t)O)

#endif // !_sync_H
