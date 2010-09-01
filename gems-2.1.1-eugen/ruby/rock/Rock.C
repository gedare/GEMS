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

#include "Global.h"
#include "System.h"
#include "RubyConfig.h"
#include "interface.h"
#include "Chip.h"
#include "TransactionInterfaceManager.h"
#include "Protocol.h"

extern "C" {
#include "commands.h"
#include "Rock.h"
}

//--------------------------------------------------------------------------- 80
// Helper functions
//--------------------------------------------------------------------------- 80

static int get_thread_no(conf_object_t* cpu){
  int proc_no = SIM_get_proc_no(cpu);
  int core_no = ((proc_no / RubyConfig::numberofSMTThreads()) %
                 RubyConfig::numberOfProcsPerChip());
  int thread_no = core_no % RubyConfig::numberofSMTThreads();
  return thread_no;
}

static TransactionInterfaceManager* get_xact_mgr(conf_object_t* cpu){
  int proc_no = SIM_get_proc_no(cpu);
  int chip_no = ((proc_no / RubyConfig::numberOfProcsPerChip()) /
                 RubyConfig::numberofSMTThreads());
  int core_no = ((proc_no / RubyConfig::numberofSMTThreads()) %
                 RubyConfig::numberOfProcsPerChip());
  AbstractChip* chip = g_system_ptr->getChip(chip_no);
  TransactionInterfaceManager* xact_mgr =
    chip->getTransactionInterfaceManager(core_no);
  return xact_mgr;
}

//--------------------------------------------------------------------------- 80
// Exported functions
//--------------------------------------------------------------------------- 80

extern "C"
int in_transaction(conf_object_t* cpu){
  int thread_no = get_thread_no(cpu);
  TransactionInterfaceManager* xact_mgr = get_xact_mgr(cpu);

  return (int) xact_mgr->inTransaction(thread_no);
}

extern "C"
uint32 rock_read_cps_register(conf_object_t* cpu)
{
  NodeID id = SIM_get_proc_no(cpu);
  Address pc = SIMICS_get_program_counter(id);
  TransactionInterfaceManager* xact_mgr = get_xact_mgr(cpu);
  uint32 cps = xact_mgr->getCPS(get_thread_no(cpu));
  g_system_ptr->getProfiler()->profileGetCPS(id, cps, pc);

  return cps;
}

extern "C"
void rock_trap_instruction_in_xact(conf_object_t* cpu)
{
  assert(in_transaction(cpu));

  if(!ATMTP_ABORT_ON_NON_XACT_INST) {
    ERROR_MSG("Executed trap instruction inside transaction!");
  }
}


//
// [kevin 2007-11-06] This is an approximation of interaction between
// save and restore instructions and Rock transactions.  Our current
// belief is that the execution combination of a save and a later
// restore instruction fails speculation (i.e.  aborts).
//
extern "C"
int rock_save_in_xact_aborted(conf_object_t* cpu)
{
  assert(in_transaction(cpu));

  TransactionInterfaceManager* xact_mgr = get_xact_mgr(cpu);

  if(!ATMTP_ALLOW_SAVE_RESTORE_IN_XACT &&
     xact_mgr->saveInstShouldAbort(get_thread_no(cpu))) {
    // [kevin 2007-11-13] Our current understanding is that Rock
    // should never abort on a save instruction so this path should
    // never execute.
    //
    assert(0);

    xact_mgr->setCPS_inst(get_thread_no(cpu));

    NodeID id = SIM_get_proc_no(cpu);
    Address pc = SIMICS_get_program_counter(id);
    g_system_ptr->getProfiler()->profileTransactionSaveInst(id, pc);

    post_abort_transaction(cpu);
    return true;
  } else {
    return false;
  }
}

extern "C"
int rock_restore_in_xact_aborted(conf_object_t* cpu)
{
  assert(in_transaction(cpu));

  TransactionInterfaceManager* xact_mgr = get_xact_mgr(cpu);

  if(!ATMTP_ALLOW_SAVE_RESTORE_IN_XACT &&
     xact_mgr->restoreInstShouldAbort(get_thread_no(cpu))) {
    xact_mgr->setCPS_inst(get_thread_no(cpu));

    NodeID id = SIM_get_proc_no(cpu);
    Address pc = SIMICS_get_program_counter(id);
    g_system_ptr->getProfiler()->profileTransactionRestoreInst(id, pc);

    post_abort_transaction(cpu);
    return true;
  } else {
    return false;
  }
}


extern "C"
void rock_chkpt(conf_object_t* cpu, integer_t offset)
{
  if(in_transaction(cpu) && ATMTP_ABORT_ON_NON_XACT_INST) {
    NodeID id = SIM_get_proc_no(cpu);
    Address pc = SIMICS_get_program_counter(id);
    g_system_ptr->getProfiler()->profileTransactionUnsupInst(id, pc);

    TransactionInterfaceManager* xact_mgr = get_xact_mgr(cpu);
    xact_mgr->setCPS_inst(get_thread_no(cpu));

    post_abort_transaction(cpu);
  } else {
    int proc_no = SIM_get_proc_no(cpu);
    int core_no = ((proc_no / RubyConfig::numberofSMTThreads()) %
                   RubyConfig::numberOfProcsPerChip());
    int thread_no = core_no % RubyConfig::numberofSMTThreads();
    TransactionInterfaceManager* xact_mgr = get_xact_mgr(cpu);
    xact_mgr->beginTransaction(thread_no, proc_no, false);
    xact_mgr->setFailPC(thread_no, Address(offset));
  }
}

extern "C"
void rock_commit(conf_object_t* cpu)
{
  if(in_transaction(cpu)) {
    TransactionInterfaceManager* xact_mgr = get_xact_mgr(cpu);
    xact_mgr->commitTransaction(get_thread_no(cpu), 0, false);
  } else {
    if(ATMTP_ABORT_ON_NON_XACT_INST) {
      NodeID id = SIM_get_proc_no(cpu);
      Address pc = SIMICS_get_program_counter(id);
      printf("  %d Warning: commit (0x%llx) outside transaction!\n",
             id, pc.getAddress());
    } else {
      ERROR_MSG("Executed COMMIT outside a transaction!");
    }
  }
}

extern "C"
void rock_unsup_instruction_in_xact(conf_object_t* cpu)
{
  assert(in_transaction(cpu));

  NodeID id = SIM_get_proc_no(cpu);
  Address pc = SIMICS_get_program_counter(id);
  g_system_ptr->getProfiler()->profileTransactionUnsupInst(id, pc);

  if(ATMTP_ABORT_ON_NON_XACT_INST) {
    TransactionInterfaceManager* xact_mgr = get_xact_mgr(cpu);
    xact_mgr->setCPS_inst(get_thread_no(cpu));
    post_abort_transaction(cpu);
  } else {
    ERROR_MSG("Unsupported Instruction inside a transaction!");
  }
}

extern "C"
int rock_retry_in_xact_aborted(conf_object_t* cpu)
{
  assert(in_transaction(cpu));

  TransactionInterfaceManager* xact_mgr = get_xact_mgr(cpu);

  if(xact_mgr->inRetryHACK(get_thread_no(cpu))) {
    return false;
  } else {
    NodeID id = SIM_get_proc_no(cpu);
    Address pc = SIMICS_get_program_counter(id);
    g_system_ptr->getProfiler()->profileTransactionUnsupInst(id, pc);
    rock_unsup_instruction_in_xact(cpu);
    return true;
  }
}

extern "C"
void rock_fail_transaction(conf_object_t* cpu)
{
  assert(in_transaction(cpu));

  NodeID id = SIM_get_proc_no(cpu);
  Address pc = SIMICS_get_program_counter(id);
  g_system_ptr->getProfiler()->profileTransactionTCC(id, pc);

  TransactionInterfaceManager* xact_mgr = get_xact_mgr(cpu);
  xact_mgr->setCPS_tcc(get_thread_no(cpu));

  post_abort_transaction(cpu);
}

extern "C"
void rock_exception_start(void* desc, conf_object_t* cpu, integer_t val)
{
  if(ATMTP_ENABLED && in_transaction(cpu)) {
    TransactionInterfaceManager* xact_mgr = get_xact_mgr(cpu);

    int proc_no = SIM_get_proc_no(cpu);
    // This translation will not fail -- to get to a DTLB miss, the
    // instruction must have been fetched, so a valid translation
    // for the PC must exist.
    //
    Address pc = SIMICS_get_program_counter(proc_no);
    physical_address_t p_addr = SIMICS_translate_address(proc_no, pc);
    assert(SIM_get_pending_exception() == SimExc_No_Exception);

    uint32 instruction = SIMICS_read_physical_memory(proc_no, p_addr, 4);

    if ((val >= TRAP_ASYNC_ERROR) && (val <= TRAP_VECTORED_INT)) {
      //
      // Async Interrupts

      xact_mgr->setCPS_async(get_thread_no(cpu));
      // [kevin 2007-11-06]: not sure how to handle this. Fail loudly for now!
      //
      ERROR_MSG("ASYNC INTERRUPT in XACT!");

    } else if (val == TRAP_FAST_DATA_MMU_MISS) {
      //
      // DTLB miss 0x68

      if ((instruction & MEM_OP_MASK) == OPCODE_LDSB || // LDSB;
          (instruction & MEM_OP_MASK) == OPCODE_LDSH || // LDSH
          (instruction & MEM_OP_MASK) == OPCODE_LDSW || // LDSW
          (instruction & MEM_OP_MASK) == OPCODE_LDUB || // LDUB
          (instruction & MEM_OP_MASK) == OPCODE_LDUH || // LDUH
          (instruction & MEM_OP_MASK) == OPCODE_LDUW || // LDUW
          (instruction & MEM_OP_MASK) == OPCODE_LDX     // LDX
          ) {
        xact_mgr->setCPS_ld(get_thread_no(cpu));
      } else if ((instruction & MEM_OP_MASK) == OPCODE_STB || // STB
                 (instruction & MEM_OP_MASK) == OPCODE_STH || // STH
                 (instruction & MEM_OP_MASK) == OPCODE_STW || // STW
                 (instruction & MEM_OP_MASK) == OPCODE_STX    // STX
                 ) {
        xact_mgr->setCPS_st(get_thread_no(cpu));
      } else {
        cerr << "  " << proc_no
             << " ERROR!!! tlb miss on unrecognized inst: "
             << hex << instruction << dec
             << " CPS bits not set." << endl;
        ASSERT(0);
      }
      xact_mgr->setCPS_prec(get_thread_no(cpu));

    } else if (val >= TRAP_SOFTWARE_TRAP && val <= TRAP_ESOFTWARE_TRAP) {
      //
      // Software trap, caused by tcc instruction.

      Address pc = SIMICS_get_program_counter(proc_no);
      g_system_ptr->getProfiler()->profileTransactionTCC(proc_no, pc);
      xact_mgr->setCPS_tcc(get_thread_no(cpu));

      if(!ATMTP_ABORT_ON_NON_XACT_INST){
        if(instruction == (uint32) OPCODE_FAIL){
          WARN_MSG("Executed FAIL TRANSACTION (ta) inside a transaction.");
        } else {
          ERROR_MSG("Executed trap instruction (tcc) inside a transaction!");
        }
      }

    } else {
      //
      // Other Precise traps

      xact_mgr->setCPS_prec(get_thread_no(cpu));

    }

    // Set a flag in the XactMgr to record the start of the trap HACK,
    // and then arrange to Run a retry instruction to exit the trap
    // handler (see below).
    //
    xact_mgr->setInRetryHACK(get_thread_no(cpu));
    SIM_stacked_post(cpu, run_retry_HACK, (lang_void*) NULL);
  }
}

extern "C"
void rock_exception_done(void* desc, conf_object_t* cpu, integer_t val)
{
  // Set CPS register bits according to trap type (Prec. v. Async.)
  //
  if(ATMTP_ENABLED && in_transaction(cpu)) {
    TransactionInterfaceManager* xact_mgr = get_xact_mgr(cpu);
    assert(xact_mgr->inRetryHACK(get_thread_no(cpu)));
    post_abort_transaction(cpu);
    xact_mgr->clearInRetryHACK(get_thread_no(cpu));
  }
}

//--------------------------------------------------------------------------- 80

//
// Jumps the PC to a location known to contain a retry instruction.
// Executing retry causes the processor to end trap handling and
// return to user mode. In addtion, executing retry causes an
// exception_return hap, which triggers a call to rock_exception_done
// (above), which executes the transaction abort after the processor
// returns to user mode.
//
extern "C"
void run_retry_HACK(conf_object_t* cpu, lang_void* parameter)
{
  int proc_no = SIM_get_proc_no(cpu);
  SIMICS_set_pc(proc_no, Address(ROCK_HACK_RETRY_ADDRESS));
}

extern "C"
void abort_transaction_command(conf_object_t* cpu, lang_void* parameter){
  TransactionInterfaceManager* xact_mgr = get_xact_mgr(cpu);
  xact_mgr->abortLazyTransaction(get_thread_no(cpu));
  xact_mgr->restoreFailPC(get_thread_no(cpu));
}

extern "C"
void post_abort_transaction(conf_object_t* cpu){
  SIM_stacked_post(cpu, abort_transaction_command, (lang_void*) NULL);
}


//--------------------------------------------------------------------------- 80
// Simics instruction handlers and related functions.
//--------------------------------------------------------------------------- 80

static decoder_t ATMTP_decoder;

static integer_t
get_pc_plus_offset(conf_object_t *cpu, int offset)
{
  integer_t ll_offset = offset;
  if (ll_offset & ATMTP_CHKPT_OFFSET_SIGN_BIT) {
    ll_offset |= ATMTP_CHKPT_OFFSET_SIGN_EXTEND_MASK;
  }
  ll_offset <<= 2;

  return read_reg(cpu, "pc") + ll_offset;
}


extern "C"
exception_type_t
ATMTP_chkpt(conf_object_t *cpu, unsigned int arg, lang_void *user_data)
{
  assert(ATMTP_ENABLED);
  rock_chkpt(cpu, get_pc_plus_offset(cpu, arg));
  return Sim_PE_No_Exception;
}

extern "C"
exception_type_t
ATMTP_commit(conf_object_t *cpu, unsigned int arg, lang_void *user_data)
{
  assert(ATMTP_ENABLED);
  rock_commit(cpu);
  return Sim_PE_No_Exception;
}

extern "C"
exception_type_t
ATMTP_unsup_inst(conf_object_t *cpu, unsigned int arg, lang_void *user_data)
{
  if (ATMTP_ENABLED && in_transaction(cpu)) {
    rock_unsup_instruction_in_xact(cpu);
    return Sim_PE_No_Exception;
  } else {
    return Sim_PE_Default_Semantics;
  }
}

extern "C"
exception_type_t
ATMTP_retry(conf_object_t *cpu, unsigned int arg, lang_void *user_data)
{
  if (ATMTP_ENABLED && in_transaction(cpu) && rock_retry_in_xact_aborted(cpu)) {
    return Sim_PE_No_Exception;
  } else {
    return Sim_PE_Default_Semantics;
  }
}

extern "C"
exception_type_t
ATMTP_save(conf_object_t *cpu, unsigned int arg, lang_void *user_data)
{
  if (ATMTP_ENABLED && in_transaction(cpu)){
    if(rock_save_in_xact_aborted(cpu)){
      return Sim_PE_No_Exception;
    } else {
      return Sim_PE_Default_Semantics;
    }
  } else {
    return Sim_PE_Default_Semantics;
  }
}

extern "C"
exception_type_t
ATMTP_restore(conf_object_t *cpu, unsigned int arg, lang_void *user_data)
{
  if (ATMTP_ENABLED && in_transaction(cpu)){
    if (rock_restore_in_xact_aborted(cpu)) {
      return Sim_PE_No_Exception;
    } else {
      return Sim_PE_Default_Semantics;
    }
  } else {
    return Sim_PE_Default_Semantics;
  }
}

//
// arg contains register number for rd.
//
extern "C"
exception_type_t
ATMTP_rdcps(conf_object_t *cpu, unsigned int arg, lang_void *user_data)
{
  assert(ATMTP_ENABLED);

  int rd;
  rd = (arg >> 25);

  uinteger_t value = rock_read_cps_register(cpu);

  SIM_write_register(cpu, rd, value);
  if (SIM_clear_exception()) {
    fprintf(stderr,
            "ATMTP_rdcps: SIM_write_register(%s, 0x%x, 0x%llx) failed!\n",
            cpu->name, rd, value);
    assert(0);
  }

  return Sim_PE_No_Exception;
}

extern "C"
int
ATMTP_decode(u_char *code, int valid_bytes, conf_object_t *cpu,
             instruction_info_t *ii, void *data)
{
  assert(valid_bytes == 4);

  uint32 i, instr = 0;
  for (i = 0; i < 4; i++) {
    instr = (instr << 8) | code[i];
  }

  if ((instr & ATMTP_CHKPT_OPCODE_MASK) == ATMTP_CHKPT_OPCODE) {
    ii->ii_ServiceRoutine = ATMTP_chkpt;
    ii->ii_Arg            = instr & ATMTP_CHKPT_OFFSET_MASK;
    ii->ii_Type           = UD_IT_SEQUENTIAL;
    ii->ii_UserData       = NULL;
  } else if (instr == ATMTP_COMMIT_OPCODE) {
    ii->ii_ServiceRoutine = ATMTP_commit;
    ii->ii_Arg            = 0x0badbeef;
    ii->ii_Type           = UD_IT_SEQUENTIAL;
    ii->ii_UserData       = NULL;
  } else if ((instr & ATMTP_RDCPS_OPCODE_MASK) == ATMTP_RDCPS_OPCODE) {
    ii->ii_ServiceRoutine = ATMTP_rdcps;
    ii->ii_Arg            = instr & ATMTP_RDCPS_RD_MASK;
    ii->ii_Type           = UD_IT_SEQUENTIAL;
    ii->ii_UserData       = NULL;
  } else if ((instr & ATMTP_WRPR_OPCODE_MASK) == ATMTP_WRPR_OPCODE ||
             (instr & ATMTP_MEMBAR_OPCODE_MASK) == ATMTP_MEMBAR_OPCODE ||
             (instr & ATMTP_DIV_OPCODE_MASK) == ATMTP_DIV_OPCODE ||
             (instr & ATMTP_WRSTR_OPCODE_MASK) == ATMTP_WRSTR_OPCODE ||
             (instr & ATMTP_CASA_CASXA_OPCODE_MASK) == ATMTP_CASA_CASXA_OPCODE ||
             (instr & ATMTP_DONE_RETRY_OPCODE_MASK) == ATMTP_DONE_OPCODE ||
             (instr & ATMTP_FLUSH_OPCODE_MASK) == ATMTP_FLUSH_OPCODE ||
             (instr & ATMTP_LDSTUB_OPCODE_MASK) == ATMTP_LDSTUB_OPCODE) {
    ii->ii_ServiceRoutine = ATMTP_unsup_inst;
    ii->ii_Arg            = 0x0badfeed;
    ii->ii_Type           = UD_IT_CONTROL_FLOW;
    ii->ii_UserData       = NULL;
  } else if ((instr & ATMTP_DONE_RETRY_OPCODE_MASK) == ATMTP_RETRY_OPCODE) {
    ii->ii_ServiceRoutine = ATMTP_retry;
    ii->ii_Arg            = 0x0badfeed;
    ii->ii_Type           = UD_IT_CONTROL_FLOW;
    ii->ii_UserData       = NULL;
  } else if ((instr & ATMTP_SAV_REST_OPCODE_MASK) == ATMTP_SAV_OPCODE) {
    ii->ii_ServiceRoutine = ATMTP_save;
    ii->ii_Arg            = 0xdeadbeef;
    ii->ii_Type           = UD_IT_CONTROL_FLOW;
    ii->ii_UserData       = NULL;
  } else if ((instr & ATMTP_SAV_REST_OPCODE_MASK) == ATMTP_REST_OPCODE) {
    ii->ii_ServiceRoutine = ATMTP_restore;
    ii->ii_Arg            = 0xdeadbeef;
    ii->ii_Type           = UD_IT_CONTROL_FLOW;
    ii->ii_UserData       = NULL;
  } else {
    return 0;
  }

  return 4;
}

extern "C"
int
ATMTP_disassemble(u_char *code, int valid_bytes, conf_object_t *cpu,
                  char *string_buffer, void *data)
{
  assert(valid_bytes == 4);

  uint32 i, instr = 0;
  for (i = 0; i < 4; i++) {
    instr = (instr << 8) | code[i];
  }

  if ((instr & ATMTP_CHKPT_OPCODE_MASK) == ATMTP_CHKPT_OPCODE) {
    sprintf(string_buffer, "chkpt 0x%llx",
            get_pc_plus_offset(cpu, instr & ATMTP_CHKPT_OFFSET_MASK));
  } else if (instr == ATMTP_COMMIT_OPCODE) {
    strcpy(string_buffer, "commit");
  } else {
    return 0;
  }

  return 4;
}

extern "C"
int
ATMTP_flush(conf_object_t *cpu, instruction_info_t *ii, void *data)
{
  service_routine_t sr = ii->ii_ServiceRoutine;

  return (sr == ATMTP_chkpt || sr == ATMTP_commit ||
          sr == ATMTP_rdcps || sr == ATMTP_unsup_inst ||
          sr == ATMTP_retry || sr == ATMTP_save ||
          sr == ATMTP_restore);
}

extern "C"
decoder_t*
ATMTP_create_instruction_decoder()
{
  ATMTP_decoder.decode = ATMTP_decode;
  ATMTP_decoder.disassemble = ATMTP_disassemble;
  ATMTP_decoder.flush = ATMTP_flush;

  return &ATMTP_decoder;
}

extern "C"
decoder_t*
ATMTP_get_instruction_decoder()
{
  return &ATMTP_decoder;
}
