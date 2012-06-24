/*
    Copyright (C) 1999-2008 by Mark D. Hill and David A. Wood for the
    Wisconsin Multifacet Project.  Contact: gems@cs.wisc.edu
    http://www.cs.wisc.edu/gems/

    --------------------------------------------------------------------

    This file is part of the Opal Timing-First Microarchitectural
    Simulator, a component of the Multifacet GEMS (General 
    Execution-driven Multiprocessor Simulator) software toolset 
    originally developed at the University of Wisconsin-Madison.

    Opal was originally developed by Carl Mauer based upon code by
    Craig Zilles.

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
#ifndef _TARGETMACROS_H_
#define _TARGETMACROS_H_

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

/* maximum trap level for this implementation of the sparc processor */
#define MAXTL          5

/* number of windows defined by this implementation of the sparc processor */
#define NWINDOWS       8

/*------------------------------------------------------------------------*/
/* Constant declaration(s)                                                */
/*------------------------------------------------------------------------*/

/// The number of integer registers
const uint32  MAX_INT_REGS    = (NWINDOWS * 32 + 16);
/// The number of global registers (4 sets of 8 each)
const uint32  MAX_GLOBAL_REGS = (8 * 4);
/// The number of floating point registers
const uint32  MAX_FLOAT_REGS  = 32;
/// The number of control registers
const uint32  MAX_CTL_REGS    = 120;
/// The maximum number of address space identifiers (0-256 are valid)
const uint32  MAX_NUM_ASI     = (256 + 1);

/*
 * @name logical register numbers
 */
//@{
/// normal global register set
const uint32 REG_GLOBAL_NORM =  0;
/// alternate global register set
const uint32 REG_GLOBAL_ALT  =  1;
/// mmu global register set
const uint32 REG_GLOBAL_MMU  =  2;
/// interrupt global register set
const uint32 REG_GLOBAL_INT  =  3;

/// logical register number for: integer condition codes (xcc and icc)
const char   REG_CC_CCR  = 0;
/// logical register number for: register %fcc0
const char   REG_CC_FCC0 = 1;
/// logical register number for: register %fcc1
const char   REG_CC_FCC1 = 2;
/// logical register number for: register %fcc2
const char   REG_CC_FCC2 = 3;
/// logical register number for: register %fcc3
const char   REG_CC_FCC3 = 4;

/// operation to be completed in single precision
const char   REG_FP_SINGLE = 1;
/// operation to be completed in double precision
const char   REG_FP_DOUBLE = 2;
/// operation to be completed in quad   precision
const char   REG_FP_QUAD   = 3;
//@}

/** The number of logical registers:
 *  (Number of windowed regs + 32 globals) currently 160 */
const uint32  CONFIG_IREG_LOGICAL = NWINDOWS*16 + 32;

/** The number of logical (single precision) floating point registers == 64 */
const uint32  CONFIG_FPREG_LOGICAL = 64;

/** The number of logical condition code registers == 5 */
const uint32  CONFIG_CCREG_LOGICAL = 5;

/** "special" instruction signaling "magic", it is special NOP instruction
 *  carry additional information about exception, etc.
 */
const uint32 STATIC_INSTR_MOP     = 0xc1f80000;

/** The maximum number of software traps that can be referenced by tcc instruction */
const uint32 MAX_SOFTWARE_TRAPS = 128;

/*------------------------------------------------------------------------*/
/* Enumerations                                                           */
/*------------------------------------------------------------------------*/

/** classifications of container classes: used to manage renaming of
 *  multiple registers simultaneously.
 */
enum rid_container_t {
  CONTAINER_BLOCK,         // container class for 64-byte block
  CONTAINER_YCC,           // container class for condition codes plus %Y
  CONTAINER_QUAD,          // container class for quad loads
  CONTAINER_NUM_CONTAINER_TYPES
};

/// physical control register numbers 
// LUKE - TESTER relies on the exact order of these when reading in the 
//             processor state trace, so be careful when adding - safe to add new regs
//             after CONTROL_CLEANWIN
enum control_reg_t {
  CONTROL_PC = 0,
  CONTROL_NPC,
  CONTROL_Y,
  CONTROL_CCR,
  CONTROL_FPRS,
  CONTROL_FSR,
  CONTROL_ASI,
  CONTROL_TICK,
  CONTROL_GSR,
  CONTROL_TICK_CMPR,
  CONTROL_DISPATCH_CONTROL,
  CONTROL_PSTATE,
  CONTROL_TL,
  CONTROL_PIL,
  CONTROL_TPC1,
  CONTROL_TPC2,
  CONTROL_TPC3,
  CONTROL_TPC4,
  CONTROL_TPC5,
  CONTROL_TNPC1,
  CONTROL_TNPC2,
  CONTROL_TNPC3,
  CONTROL_TNPC4,
  CONTROL_TNPC5,
  CONTROL_TSTATE1,
  CONTROL_TSTATE2,
  CONTROL_TSTATE3,
  CONTROL_TSTATE4,
  CONTROL_TSTATE5,
  CONTROL_TT1,
  CONTROL_TT2,
  CONTROL_TT3,
  CONTROL_TT4,
  CONTROL_TT5,
  CONTROL_TBA,
  CONTROL_VER,
  CONTROL_CWP,
  CONTROL_CANSAVE,
  CONTROL_CANRESTORE,
  CONTROL_OTHERWIN,
  CONTROL_WSTATE,
  CONTROL_CLEANWIN,
  CONTROL_STICK,
  CONTROL_STICK_CMPR,
  CONTROL_SOFTINT,
  //for the Fireplane config and address
  CONTROL_FIREPLANE_CONFIG,
  CONTROL_FIREPLANE_ADDRESS,
  // for the interrupt vector dispatch and receive status
  CONTROL_INTR_DISPATCH_STATUS,
  CONTROL_INTR_RECEIVE_STATUS,
  // for the incoming interrupt vector data
  CONTROL_IN_INTR_DATA0,
  CONTROL_IN_INTR_DATA1,
  CONTROL_IN_INTR_DATA2,
  CONTROL_IN_INTR_DATA3,
  CONTROL_IN_INTR_DATA4,
  CONTROL_IN_INTR_DATA5,
  CONTROL_IN_INTR_DATA6,
  CONTROL_IN_INTR_DATA7,
  CONTROL_NUM_CONTROL_PHYS,
  CONTROL_UNIMPL,
  CONTROL_ISREADY,
  CONTROL_NUM_CONTROL_TYPES
};

/// enumerations of trap types
enum trap_type_t {
  Trap_Power_On_Reset = 0x01,
  Trap_Watchdog_Reset = 0x02,
  Trap_Externally_Initiated_Reset = 0x03,
  Trap_Software_Initiated_Reset = 0x04,
  Trap_Red_State_Exception = 0x05,
  Trap_Instruction_Access_Exception = 0x08,
  Trap_Instruction_Access_MMU_Miss = 0x09,
  Trap_Instruction_Access_Error = 0x0a,
  Trap_Illegal_Instruction = 0x10,
  Trap_Privileged_Opcode = 0x11,
  Trap_Unimplemented_Ldd = 0x12,
  Trap_Unimplemented_Std = 0x13,
  Trap_Fp_Disabled = 0x20,
  Trap_Fp_Exception_Ieee_754 = 0x21,
  Trap_Fp_Exception_Other = 0x22,
  Trap_Tag_Overflow = 0x23,
  Trap_Clean_Window = 0x24,
  Trap_Division_By_Zero = 0x28,
  Trap_Internal_Processor_Error = 0x29,
  Trap_Data_Access_Exception = 0x30,
  Trap_Data_Access_Mmu_Miss = 0x31,
  Trap_Data_Access_Error = 0x32,
  Trap_Data_Access_Protection = 0x33,
  Trap_Mem_Address_Not_Aligned = 0x34,
  Trap_Lddf_Mem_Address_Not_Aligned = 0x35,
  Trap_Stdf_Mem_Address_Not_Aligned = 0x36,
  Trap_Privileged_Action = 0x37,
  Trap_Ldqf_Mem_Address_Not_Aligned = 0x38,
  Trap_Stqf_Mem_Address_Not_Aligned = 0x39,
  Trap_Async_Data_Error = 0x40,
  Trap_Interrupt_Level_1 = 0x41,
  Trap_Interrupt_Level_2 = 0x42,
  Trap_Interrupt_Level_3 = 0x43,
  Trap_Interrupt_Level_4 = 0x44,
  Trap_Interrupt_Level_5 = 0x45,
  Trap_Interrupt_Level_6 = 0x46,
  Trap_Interrupt_Level_7 = 0x47,
  Trap_Interrupt_Level_8 = 0x48,
  Trap_Interrupt_Level_9 = 0x49,
  Trap_Interrupt_Level_10 = 0x4a,
  Trap_Interrupt_Level_11 = 0x4b,
  Trap_Interrupt_Level_12 = 0x4c,
  Trap_Interrupt_Level_13 = 0x4d,
  Trap_Interrupt_Level_14 = 0x4e,
  Trap_Interrupt_Level_15 = 0x4f,


  Trap_Interrupt_Vector = 0x60,
  Trap_PA_Watchpoint = 0x61,
  Trap_VA_Watchpoint = 0x62,
  Trap_Corrected_ECC_Error = 0x63,

  Trap_Fast_Instruction_Access_MMU_Miss = 0x64,
  Trap_Fast_Data_Access_MMU_Miss = 0x68,
  Trap_Fast_Data_Access_Protection = 0x6c,


  Trap_Spill_0_Normal = 0x80,
  Trap_Spill_1_Normal = 0x84,
  Trap_Spill_2_Normal = 0x88,
  Trap_Spill_3_Normal = 0x8c,
  Trap_Spill_4_Normal = 0x90,
  Trap_Spill_5_Normal = 0x94,
  Trap_Spill_6_Normal = 0x98,
  Trap_Spill_7_Normal = 0x9c,
  Trap_Spill_0_Other = 0xa0,
  Trap_Spill_1_Other = 0xa4,
  Trap_Spill_2_Other = 0xa8,
  Trap_Spill_3_Other = 0xac,
  Trap_Spill_4_Other = 0xb0,
  Trap_Spill_5_Other = 0xb4,
  Trap_Spill_6_Other = 0xb8,
  Trap_Spill_7_Other = 0xbc,
  Trap_Fill_0_Normal = 0xc0,
  Trap_Fill_1_Normal = 0xc4,
  Trap_Fill_2_Normal = 0xc8,
  Trap_Fill_3_Normal = 0xcc,
  Trap_Fill_4_Normal = 0xd0,
  Trap_Fill_5_Normal = 0xd4,
  Trap_Fill_6_Normal = 0xd8,
  Trap_Fill_7_Normal = 0xdc,
  Trap_Fill_0_Other = 0xe0,
  Trap_Fill_1_Other = 0xe4,
  Trap_Fill_2_Other = 0xe8,
  Trap_Fill_3_Other = 0xec,
  Trap_Fill_4_Other = 0xf0,
  Trap_Fill_5_Other = 0xf4,
  Trap_Fill_6_Other = 0xf8,
  Trap_Fill_7_Other = 0xfc,
  /* LUKE - note that software initiated traps (using tcc instr) resides between 0x100 and (0x100+0x17f) */
  /* MAX is 1 more than max software trap */
  TRAP_NUM_TRAP_TYPES = 0x280,
  /** pseudo trap to squash pipeline for "unimplemented" instructions */
  Trap_Unimplemented = 0x416,
  /** pseudo trap to read value from simics */
  Trap_Use_Functional,
  /** pseudo trap to indicate no problems: must be highest # (last) trap */
  Trap_NoTrap
};

/*------------------------------------------------------------------------*/
/* Global variables                                                       */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Global functions                                                       */
/*------------------------------------------------------------------------*/

#endif  /* _TARGETMACROS_H_ */
