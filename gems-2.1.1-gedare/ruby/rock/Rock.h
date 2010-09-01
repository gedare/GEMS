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
  A header file for Rock-specific constants and functions.
 */

//--------------------------------------------------------------------------- 80

#ifndef    ROCK_H
#define    ROCK_H


#include "simics/api.h"

// These macros represent the various bits in Rock's CPS register.
// After a transaction fails, one or more of these bits should be set
// to indicate the cause(s) of failure.
//

#define CPS_EXOG    0x00000001 // "Exogenous" code run on  proc
#define CPS_COH     0x00000002 // Conflicting memory request
#define CPS_TCC     0x00000004 // Executed trap instruction
#define CPS_INST    0x00000008 // Executed other 'bad' instruction
#define CPS_PREC    0x00000010 // Other precise trap
#define CPS_ASYNC   0x00000020 // Asynchronous interrupt
#define CPS_SIZ     0x00000040 // Exceeded store queue
#define CPS_LD      0x00000080 // Line in read set evicted or
                               // repeated DTLB miss on load
#define CPS_ST      0x00000100 // Repeated DTLB miss on store
#define CPS_CTI     0x00000200 // Mispredict of defered branch
#define CPS_FP      0x00000400 // Floating point related
#define CPS_ABORT   0x00000000 // Intentional (SW specified) abort


#define MEM_OP_MASK 0xc1f80000
#define OPCODE_LDSB 0xc0480000 // LDSB
#define OPCODE_LDSH 0xc0500000 // LDSH
#define OPCODE_LDSW 0xc0400000 // LDSW
#define OPCODE_LDUB 0xc0080000 // LDUB
#define OPCODE_LDUH 0xc0100000 // LDUH
#define OPCODE_LDUW 0xc0000000 // LDUW
#define OPCODE_LDX  0xc0580000 // LDX

#define OPCODE_STB  0xc0280000 // STB
#define OPCODE_STH  0xc0380000 // STH
#define OPCODE_STW  0xc0200000 // STW
#define OPCODE_STX  0xc0700000 // STX

#define OPCODE_FAIL 0x91d0300f // TA  %xcc, %g0 + 15

#define TRAP_ASYNC_ERROR        0x040
#define TRAP_VECTORED_INT       0x060
#define TRAP_FAST_DATA_MMU_MISS 0x068 // DTLB miss
#define TRAP_SOFTWARE_TRAP      0x100 // tcc 0
#define TRAP_ESOFTWARE_TRAP     0x1ff // tcc 0xff


// SCARY HACK:
// Set this to the address of a retry instruction
// in the kernel trap vector. The abort-on-exception
// code will jump execution to this address, run
// the one retry instruction, then abort the
// transaction.
#define ROCK_HACK_RETRY_ADDRESS 0x1001660


//--------------------------------------------------------------------------- 80
// Rock/SPARC instruction bit-patterns.
//--------------------------------------------------------------------------- 80

#define ATMTP_COMMIT_OPCODE          (0xbdf00000)

#define ATMTP_CHKPT_OPCODE           (0x30500000)
#define ATMTP_CHKPT_OPCODE_MASK      (0xfff80000)
#define ATMTP_CHKPT_OFFSET_MASK      (~(ATMTP_CHKPT_OPCODE_MASK))
#define ATMTP_CHKPT_OFFSET_SIGN_BIT \
  ((ATMTP_CHKPT_OPCODE_MASK >> 1) & ATMTP_CHKPT_OFFSET_MASK)
#define ATMTP_CHKPT_OFFSET_SIGN_EXTEND_MASK \
  (~((integer_t)ATMTP_CHKPT_OFFSET_MASK))

#define ATMTP_WRPR_OPCODE_MASK       (0xc1f80000)
#define ATMTP_WRPR_OPCODE            (0x81900000)

#define ATMTP_WRSTR_OPCODE_MASK      (0xc1f80000)
#define ATMTP_WRSTR_OPCODE           (0x81800000)

#define ATMTP_MEMBAR_OPCODE_MASK     (0xc1f80000)
#define ATMTP_MEMBAR_OPCODE          (0x81400000)

// Bit 24 selects between UDIVX and SDIVX, but since we want the
// same handler for both, we ignore it.
#define ATMTP_DIV_OPCODE_MASK        (0xc0f80000)
#define ATMTP_DIV_OPCODE             (0x80680000)

// Bit 23 selects between LDSTUB and LDSTUBA, but since we want the
// same handler for both, we ignore it.
#define ATMTP_LDSTUB_OPCODE_MASK     (0xc1780000)
#define ATMTP_LDSTUB_OPCODE          (0xc0680000)

#define ATMTP_SAV_REST_OPCODE_MASK   (0xc1f80000)
#define ATMTP_SAV_OPCODE             (0x81e00000)
#define ATMTP_REST_OPCODE            (0x81e80000)

// Bit 20 selects between CASA and CASXA, but since we want the same
// handler for both, we ignore it.
//
#define ATMTP_CASA_CASXA_OPCODE      (0xc1e00000)
#define ATMTP_CASA_CASXA_OPCODE_MASK (0xc1e80000)

// 0b 10 ddddd 101000 11100 0 xxxxxxxxxxxxx  =  0x81470000
//
#define ATMTP_RDCPS_OPCODE           (0x81470000)
#define ATMTP_RDCPS_OPCODE_MASK      (0xc1ffe000)
#define ATMTP_RDCPS_RD_MASK          (0x3e000000)

#define ATMTP_DONE_RETRY_OPCODE_MASK (0xfff80000)
#define ATMTP_DONE_OPCODE            (0x81f00000)
#define ATMTP_RETRY_OPCODE           (0x83f00000)

#define ATMTP_FLUSH_OPCODE_MASK      (0xc1f80000)
#define ATMTP_FLUSH_OPCODE           (0x81d80000)


/*
   Returns true if the indicated cpu is currently running a transaction.
*/
int in_transaction(conf_object_t* cpu);

/*
   Returns the current value of the CPS register (see above).
*/
uint32 rock_read_cps_register(conf_object_t* cpu);

/*
  Execution of a chkpt instruction. If the processor is not already in a
  transaction, it starts a transaction. If there is an active transaction
  and ATMTP_ABORT_ON_NON_XACT_INST is set, rock_chkpt causes an abort.
  Otherwise, the transactions are nested (via flattening).
 */
void rock_chkpt(conf_object_t* cpu, integer_t offset);

/*
  Execution of a commit instruction. If the processor is already in a
  transaction, it commits the transaction. Otherwise, if
  ATMTP_ABORT_ON_NON_XACT_INST is set, commit is a noop, and if not set,
  the commit triggers an assertion violation.
 */
void rock_commit(conf_object_t* cpu);

/*
   Executed a trap instruction in a transaction. Abort transaction (like
   Rock), or fail simulator based on ATMTP_ABORT_ON_NON_XACT_INST and set
   CPS_TCC.
*/
void rock_trap_instruction_in_xact(conf_object_t* cpu);

/*
   Executed an unsupported instruction (other than a trap) in a
   transaction. Abort transaction (like Rock), or fail simulator based on
   ATMTP_ABORT_ON_NON_XACT_INST and set CPS_INST.
*/
void rock_unsup_instruction_in_xact(conf_object_t* cpu);

/*
   Executed a retry instruction (unsupported) inside a transaction. Returns
   true if the retry caused the transaction to abort.  First check to see
   if this retry is a result of the retry/trap HACK (see below), if not,
   abort the transaction or fail simulator based on
   ATMTP_ABORT_ON_NON_XACT_INST and set CPS_INST like with other
   unsupported instructions.
*/
int rock_retry_in_xact_aborted(conf_object_t* cpu);

/*
   SW specified abort.  Abort transaction regardless of
   ATMTP_ABORT_ON_NON_XACT_INST.
*/
void rock_fail_transaction(conf_object_t* cpu);

/*
  Called at the start of every trap or exception. Causes active
  transactions to fail and sets the appropriate bits in the CPS
  register.
 */
void rock_exception_start(void* desc, conf_object_t* cpu, integer_t val);

/*
  Performs the transaction abort after trap processing is complete.
 */
void rock_exception_done(void* desc, conf_object_t* cpu, integer_t val);

/*
  Sets the PC to a location known to contain a retry instruction.
  Executing the retry causes the processor to complete its trap
  processing and return to user mode and triggers a Simics core
  exception done event, which is caught by ctrl_exception_done,
  which calls rock_exception_done.
 */
void run_retry_HACK(conf_object_t* cpu, lang_void *parameter);

/*
  Called when a save instruction is executed in a transaction.
  Returns true if the transaction aborts due to the save.
  [kevin 2007-11-12] We approximate the behavior of Rock by
  recording whether any save instructions have been executed
  in the current transaction.  The transaction aborts if a
  restore instruction is executed in any transaction that has
  already executed a save.
 */
int rock_save_in_xact_aborted(conf_object_t* cpu);

/*
  Called when a restore instruction is executed in a transaction.
  returns true if the restore causes the transaction to abort.
  (see above).
 */
int rock_restore_in_xact_aborted(conf_object_t* cpu);

/*
  Called via SIM_stacked_post by post_abort_transaction to
  perform the actual abort (see below).
 */
void abort_transaction_command(conf_object_t* cpu, lang_void *parameter);

/*
  Calls abort_transaction_command (above) via SIM_stacked_post.
 */
void post_abort_transaction(conf_object_t* cpu);

//
// Simics instruction handlers and related functions.  These both
// return a pointer to the decoder object used by calls to
// SIM_{,un}register_arch_decoder(), or NULL, when ATMTP_DISABLED is
// false.
//
// Note that the actual calls to SIM_hap_add_callback(),
// SIM_{,un}register_arch_decoder() still live in ruby.c.  The
// functionality is split up this way because Rock.C is linked with
// the tester, which doesn't link with the Simics library.
//
decoder_t* ATMTP_create_instruction_decoder();
decoder_t* ATMTP_get_instruction_decoder();

#endif // !ROCK_H
