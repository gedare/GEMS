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
/*
 * FileName:  exec.C
 * Synopsis:  Implements the execute functionality for this target architecture
 * Author:    cmauer
 * Version:   $Id: exec.C 1.3 03/07/10 18:07:47-00:00 milo@cottons.cs.wisc.edu $
 */

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include "hfa.h"
#include "ccops.h"
#include "opcode.h"
#include "regfile.h"
#include "abstractpc.h"
#include "exec.h"

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* exec_fn class declaration                                              */
/*------------------------------------------------------------------------*/

/** Produce execute an instruction on the source data provided */
typedef void (*opcode_execute_t)( dp_int_t *data );

/**
 * Executes SPARC V9 instructions. This class is local to a C file as it
 * has *lots* of member functions that no one can/will ever call directly.
 * (e.g. one for each specific static instruction).
 *
 * @see     exec_box_t
 * @author  cmauer
 * @version $Id: exec.C 1.3 03/07/10 18:07:47-00:00 milo@cottons.cs.wisc.edu $
 */

class exec_fn_t {
public:
/// initialization function of the execute_fn class
static void    initialize( void );

/// jump table of execute functions
static opcode_execute_t m_jump_table[i_maxcount];
};

/*------------------------------------------------------------------------*/
/* Variable declarations                                                  */
/*------------------------------------------------------------------------*/

/// The static jump table for the flow inst class
opcode_execute_t exec_fn_t::m_jump_table[i_maxcount];

//**************************************************************************
void exec_fn_initialize( void )
{
  exec_fn_t::initialize();
  //g_dynamic_debug_table[i_add] = true;
}

//**************************************************************************
void exec_fn_execute( i_opcode opcode, dp_int_t *data )
{
  if (g_dynamic_debug_table[opcode] == true) {
    DEBUG_OUT( "instruction: %s\n", decode_opcode( opcode ) );
    DEBUG_OUT( "    source1: 0x%0llx\n", data->m_source[0].uint_64 );
    DEBUG_OUT( "    source2: 0x%0llx\n", data->m_source[1].uint_64 );
    DEBUG_OUT( "    cc     : 0x%0x\n", data->m_source_cc.ccode );
  }
  exec_fn_t::m_jump_table[opcode]( data );
  if (g_dynamic_debug_table[opcode] == true) {
    DEBUG_OUT( "    result  : 0x%0llx\n", data->m_dest.uint_64 );
    DEBUG_OUT( "    resultcc: 0x%0x\n", data->m_dest_cc.ccode );
  }
}


/*------------------------------------------------------------------------*/
/* Actual PC Methods                                                      */
/*------------------------------------------------------------------------*/

//**************************************************************************
void   dp_control_t::actualPC_execute( void )
{
  // advance the program counter (PC = nPC)
  m_at->pc  = m_at->npc;
  m_at->npc = m_at->npc + sizeof(uint32);
}

/** actualPC: given if a branch is taken, produce the PC, nPC result  */
//**************************************************************************
void   dp_control_t::actualPC_branch( void )
{
  if (m_taken) {
    // (cond, taken, annulled) and (cond, taken, not annulled) have
    // the same behavoir
    la_t targetPC = (int64) m_at->pc + (int64) m_offset;
    m_at->pc  = m_at->npc;
    m_at->npc = targetPC;
  } else {
    if (m_annul) {
      // cond, not taken, annulled
      m_at->pc  = m_at->npc + sizeof(uint32);
      m_at->npc = m_at->npc + (2*sizeof(uint32));
    } else {
      // cond, not taken, not annulled
      m_at->pc  = m_at->npc;
      m_at->npc = m_at->npc + sizeof(uint32);
    }
  }
}

/** given an unconditional branch, produce the PC, nPC result */
//**************************************************************************
void   dp_control_t::actualPC_uncond_branch( void )
{
  if (m_taken) {
    la_t targetPC = (int64) m_at->pc + (int64) m_offset;
    if (m_annul) {
      // (uncond, taken, annulled)
      m_at->pc  = targetPC;
      m_at->npc = m_at->pc + sizeof(uint32);
    } else {
      // (uncond, taken, not annulled)
      m_at->pc  = m_at->npc;
      m_at->npc = targetPC;
    }
  } else {
    if (m_annul) {
      // uncond, not taken, annulled
      m_at->pc  = m_at->npc + sizeof(uint32);
      m_at->npc = m_at->npc + (2*sizeof(uint32));
    } else {
      // uncond, not taken, not annulled
      m_at->pc  = m_at->npc;
      m_at->npc = m_at->npc + sizeof(uint32);
    }
  }
}

/** given the effective addr of a jmp, produce the PC, nPC result */
//**************************************************************************
void   dp_control_t::actualPC_indirect( void )
{
  // m_offset is effective address in this calling convention
  m_at->pc  = m_at->npc;
  m_at->npc = m_offset;
}

/** given the effective addr of a trap, produce the PC, nPC result */
//**************************************************************************
void   dp_control_t::actualPC_trap( void )
{
  // offset is effective address in this calling convention
  if (m_taken) {
    m_at->pc  = m_offset;
    m_at->npc = m_offset + sizeof(uint32);
  } else {
    m_at->pc  = m_at->npc;
    m_at->npc = m_at->npc + sizeof(uint32);
  }
}

/// return from trap (done, retry) instructions
//**************************************************************************
void   dp_control_t::actualPC_trap_return( my_addr_t tpc, my_addr_t tnpc,
                                           ireg_t pstate, ireg_t tl,
                                           ireg_t cwp )
{
  // FIXFIXFIX
  // dp_control is trying to predict the outcome of a trap return
  // without knowing if its a done or retry instruction.
  // in the "less realistic" code static_inst_t::actualPC_trap_return
  // (that is actually used) the opcode is available.

#if 0
  // FIXFIXFIX getOpcode??
  if (getOpcode() == i_done) {
    m_at->pc  = tnpc;
    m_at->npc = tnpc + sizeof(uint32);
  } else if (getOpcode() == i_retry) {
    m_at->pc  = tpc;
    m_at->npc = tnpc;
  } else {
    ERROR_OUT("dx: done/retry: called for unknown opcode\n");
    SIM_HALT;
  }
  m_at->pstate = pstate;
  m_at->tl     = tl;
  m_at->cwp    = cwp;
  m_at->gset   = pstate_t::getGlobalSet( pstate );
#endif
}

//**************************************************************************
void   dp_control_t::actualPC_cwp( void )
{
  // see dp_control_t::actualPC_trap for more details
#if 0
  // advance the program counter (PC = nPC)
  m_at->pc  = m_at->npc;
  m_at->npc = m_at->npc + sizeof(uint32);

  // Task: update the cwp for saves and restores
  // FIXFIXFIX getOpcode??
  if (getOpcode() == i_save) {
    m_at->cwp = (m_at->cwp + 1) & (NWINDOWS - 1);
  } else if (getOpcode() == i_restore) {
    m_at->cwp = (m_at->cwp - 1) & (NWINDOWS - 1);
  } else {
    SIM_HALT;
  }
#endif
}

/*------------------------------------------------------------------------*/
/* Basic Declarations                                                     */
/*------------------------------------------------------------------------*/

//***************************************************************************
#define DX_RETURNT void

//***************************************************************************
#define DX_PLIST dp_int_t *latch

//***************************************************************************
#define IREGISTER ireg_t
//***************************************************************************
#define FREGISTER freg_t

//***************************************************************************
#define MEM_BYTE 1
#define MEM_HALF 2
#define MEM_WORD 4
#define MEM_EXTD 8
#define MEM_QUAD 16

#define MEM_BYTE_MASK 0x00000000000000ffULL
#define MEM_HALF_MASK 0x000000000000ffffULL
#define MEM_WORD_MASK 0x00000000ffffffffULL
#define MEM_EXTD_MASK 0xffffffffffffffffULL
#define MEM_QUAD_MASK 0xffffffffffffffffULL

//***************************************************************************
#define WRITE_DEST(A)         \
    latch->m_dest.uint_64 = (A)

//***************************************************************************
#define WRITE_DEST_S(A) \
    latch->m_dest.float_32 = (A)

//***************************************************************************
#define WRITE_DEST_D(A) \
    latch->m_dest.float_64 = (A)

//***************************************************************************
#define WRITE_DEST_DI(A) \
    latch->m_dest.uint_64 = (A)

//***************************************************************************
#define WRITE_CC(A)           \
    latch->m_dest_cc.ccode = (A);

//***************************************************************************
#define SOURCE1    latch->m_source[0].uint_64
//***************************************************************************
#define SOURCE1_S  latch->m_source[0].float_32
//***************************************************************************
#define SOURCE1_D  latch->m_source[0].float_64
//***************************************************************************
#define SOURCE1_DI latch->m_source[0].uint_64

//***************************************************************************
#define SOURCE2    latch->m_source[1].uint_64
//***************************************************************************
#define SOURCE2_S  latch->m_source[1].float_32
//***************************************************************************
#define SOURCE2_D  latch->m_source[1].float_64
//***************************************************************************
#define SOURCE2_DI latch->m_source[1].uint_64

// Source 2 or immediate constant
//***************************************************************************
#define S2ORI      SOURCE2

//***************************************************************************
#define READ_CC     \
  (latch->m_source_cc.ccode)

//***************************************************************************
#define COND_CODE_N \
  ((READ_CC >> 3) & 0x1)

//***************************************************************************
#define COND_CODE_Z \
  ((READ_CC >> 2) & 0x1)

//***************************************************************************
#define COND_CODE_V \
  ((READ_CC >> 1) & 0x1)

//***************************************************************************
#define COND_CODE_C \
  (READ_CC & 0x1)

//***************************************************************************
#define ICC_C       \
  ((READ_CC) & 0x1)

//***************************************************************************
#define COND_CODE_U \
  (READ_CC == 3)

//***************************************************************************
#define COND_CODE_G \
  (READ_CC == 2)

//***************************************************************************
#define COND_CODE_L \
  (READ_CC == 1)

//***************************************************************************
#define COND_CODE_E \
  (READ_CC == 0)

//***************************************************************************
#define  NOT_IMPLEMENTED               \
  latch->m_exception = Trap_Unimplemented;

//  setTrapType( Trap_Unimplemented );

//***************************************************************************
#define  USE_FUNCTIONAL               \
  latch->m_exception = Trap_Use_Functional;

//  setTrapType( Trap_Use_Functional );

/*------------------------------------------------------------------------*/
/* Compare / Move Declarations                                            */
/*------------------------------------------------------------------------*/

//***************************************************************************
#define FCMP( FSDQ )                                          \
  ireg_t result;                                              \
  if ( SOURCE1_ ## FSDQ == SOURCE2_ ## FSDQ ) {               \
    result = 0;                                               \
  } else if ( SOURCE1_ ## FSDQ < SOURCE2_ ## FSDQ ) {         \
    result = 1;                                               \
  } else if ( SOURCE1_ ## FSDQ > SOURCE2_ ## FSDQ ) {         \
    result = 2;                                               \
  } else {                                                    \
    result = 3;                                               \
  }                                                           \
  WRITE_DEST( result );

#if 0
  /** This commented out region has to do with writing the condition code
      results in to the FSR, instead of just the renamed FCCs.
      Its commented out because it should be handled in the
      abstract register file implementation (hidden). The inorder
      processor is the only one that has a problem w/ this, the OoO doesn't
      check the FSR register.
   */
  uint32 cc_shift = 0;                                        \
  switch ( getDestReg(0).getVanilla() ) {                     \
    case REG_CC_FCC0:                                         \
      cc_shift = 10;                                          \
      break;                                                  \
    case REG_CC_FCC1:                                         \
      cc_shift = 32;                                          \
      break;                                                  \
    case REG_CC_FCC2:                                         \
      cc_shift = 34;                                          \
      break;                                                  \
    case REG_CC_FCC3:                                         \
      cc_shift = 36;                                          \
      break;                                                  \
  }                                                           \
  IREGISTER status_reg;                                       \
  READ_SELECT_CONTROL( CONTROL_FSR, status_reg );             \
  /* step 1: mask off the old FCC bits, and cexc codes */     \
  status_reg  = (status_reg & ~((0x3ULL << cc_shift) | 0x1fULL));\
  /* step 2: put the new FCC bits back on */                  \
  status_reg |= (result << cc_shift);                         \
  us_read_fsr( &status_reg );                                 \
  WRITE_SELECT_CONTROL( CONTROL_FSR, status_reg );
#endif

//***************************************************************************
#define MOVECC( COND )          \
  if ( (COND) ) {                                                 \
    WRITE_DEST( S2ORI );                                          \
  } else {                                                        \
    WRITE_DEST( SOURCE1 );                                        \
  }

//*************************************************************************** 
#define MOVECC_F MOVECC

//***************************************************************************
#define MOVER( CMP )                                 \
  if ( ((int64) SOURCE1) CMP 0 ) {                   \
    WRITE_DEST( S2ORI );                             \
  } else {                                           \
    WRITE_DEST( SOURCE1 );                           \
  }

//***************************************************************************
#define FMOVECC( FSDQ, COND )                                 \
  if ( (COND) ) {                                             \
    WRITE_DEST_ ## FSDQ ( SOURCE2_ ## FSDQ );           \
  } else {                                                    \
    WRITE_DEST_ ## FSDQ ( SOURCE1_ ## FSDQ );           \
  }

//***************************************************************************
#define MOVE16( CMP )                                         \
  IREGISTER src1 = SOURCE1;                                   \
  IREGISTER src2 = SOURCE2;                                   \
  IREGISTER result = 0;                                       \
  if ( ((int) (src1 & MEM_HALF_MASK)) CMP ((int) (src2 & MEM_HALF_MASK)) ) { \
    result |= 0x1;                                            \
  }                                                           \
  if ( ((int) ((src1 >> 16) & MEM_HALF_MASK)) CMP ((int) ((src2 >> 16) & MEM_HALF_MASK)) ) { \
    result |= 0x2;                                            \
  }                                                           \
  if ( ((int) ((src1 >> 32) & MEM_HALF_MASK)) CMP ((int) ((src2 >> 32) & MEM_HALF_MASK)) ) { \
    result |= 0x4;                                            \
  }                                                           \
  if ( ((int) ((src1 >> 48) & MEM_HALF_MASK)) CMP ((int) ((src2 >> 48) & MEM_HALF_MASK)) ) { \
    result |= 0x8;                                            \
  }                                                           \
  WRITE_DEST( result );

//***************************************************************************
#define MOVE32( CMP )                                         \
  IREGISTER src1 = SOURCE1;                                   \
  IREGISTER src2 = SOURCE2;                                   \
  IREGISTER result = 0;                                       \
  if ( ((int) (src1 & MEM_HALF_MASK)) CMP ((int) (src2 & MEM_HALF_MASK)) ) { \
    result |= 0x1;                                            \
  }                                                           \
  if ( ((int) ((src1 >> 32) & MEM_HALF_MASK)) CMP ((int) ((src2 >> 32) & MEM_HALF_MASK)) ) { \
    result |= 0x2;                                            \
  }                                                           \

//***************************************************************************
#define FMOVECC_F FMOVECC

//***************************************************************************
#define LOGICAL_SHIFT( TCAST, OP ) \
  WRITE_DEST( ((TCAST) SOURCE1) OP S2ORI );

/* pre-condition: there must be a variable "result" defined */
//***************************************************************************
#define BUILD_OLD_CCODES                                \
    unsigned char ccode = 0;                            \
    uint32        res32 = (uint32) result;              \
    /* set 64 bit condition codes */                    \
    /* set the n bit */                                 \
    ccode |= ( result >> 63 == 1 ) << 7;                \
    /* set the z bit */                                 \
    ccode |= ( result == 0 ) << 6;                      \
    /* set 32 bit condition codes */                    \
    /* set the n bit */                                 \
    ccode |= ( res32 >> 31 == 1 ) << 3;                 \
    /* set the z bit */                                 \
    ccode |= ( res32 == 0 )  << 2;

/*------------------------------------------------------------------------*/
/* Branch Declarations                                                    */
/*------------------------------------------------------------------------*/

#define BRANCH_P           BRANCH
#define BRANCH_UNCOND_P    BRANCH_UNCOND

// Note: All these _BRANCH() methods assume the the m_actual.pc & .npc 
// (of the actual target) are set in the constructor to the current PC&NPC.
//***************************************************************************
#define BRANCH( COND )               \
  dp_control_t *dpath = (dp_control_t *) latch;            \
  dpath->m_taken = COND;                                   \
  dpath->actualPC_branch();

//***************************************************************************
#define BRANCH_UNCOND( COND )        \
  dp_control_t *dpath = (dp_control_t *) latch;            \
  dpath->m_taken = COND;                                   \
  dpath->actualPC_uncond_branch();

//***************************************************************************
#define BRANCH_INDIRECT( DEST )      \
  dp_control_t *dpath = (dp_control_t *) latch;            \
  dpath->m_taken = COND;                                   \
  dpath->m_offset = DEST;                                  \
  dpath->actualPC_indirect();

//***************************************************************************
#define BRANCH_REGISTER( OP ) \
  dp_control_t *dpath = (dp_control_t *) latch;            \
  bool  taken = (((int64) SOURCE1) OP 0);                  \
  dpath->m_taken = taken;                                  \
  dpath->actualPC_branch();

/*------------------------------------------------------------------------*/
/* Memory Declarations                                                    */
/*------------------------------------------------------------------------*/

//***************************************************************************
#define ISADDRESS32( PSTATE ) \
  ( ((PSTATE) & 0x8) == 0x8 )

//***************************************************************************
#define AM64( address )       \
  ( (int32) address & 0xffffffff )

// FIXFIXFIX
//***************************************************************************
#define STORE_INSTRUCTION

#if 0
#define STORE_INSTRUCTION \
  if ( getTrapType() == Trap_Unimplemented ) { \
     setTrapType( Trap_NoTrap );               \
  }
#endif

#define  READ_MEMORY_UNSIGNED READ_MEMORY

// NOTE: can NOT use read_memory for sizes larger than word accesses
//***************************************************************************
#define  READ_MEMORY                                                  \
  dp_memory_t *dpath = (dp_memory_t *) latch;                         \
  if (dpath->m_valid) {                                               \
    WRITE_DEST( dpath->m_data );                                      \
  } else {                                                            \
    USE_FUNCTIONAL;                                                   \
  }

// do nothing
#define WRITE_MEMORY( VALUE )

// FIXFIXFIX
#if 0
//***************************************************************************
#define WRITE_MEMORY( VALUE )                                         \
  STORE_OP_TYPE *storeop = STORE_OP_GET;                              \
  storeop->writeValue( VALUE );
#endif

/*------------------------------------------------------------------------*/
/* Global functions                                                       */
/*------------------------------------------------------------------------*/

//***************************************************************************
DX_RETURNT dx_add( DX_PLIST ) {
  IREGISTER result = (SOURCE1 + S2ORI);
  //ED  m_pseq->out_info("   source1 0x%0llx\n", (SOURCE1));
  //ED  m_pseq->out_info("   source2 0x%0llx\n",  S2ORI);
  //ED  m_pseq->out_info("   result == 0x%0llx\n", result );
  WRITE_DEST(result);
}

DX_RETURNT dx_addcc( DX_PLIST ) {
  unsigned char ccodes;
  // CC_MODIFICATION: changes condition codes
  // uadd64 and sadd64 map to same function call
  IREGISTER result = us_addcc( SOURCE1, S2ORI, &ccodes );
  //m_pseq->out_info(" addcc -- %d\n", ccodes);
  WRITE_CC( ccodes );
  WRITE_DEST(result);
}

DX_RETURNT dx_addc( DX_PLIST ) {
  IREGISTER result = (SOURCE1 + S2ORI + ICC_C);
  WRITE_DEST(result);
}

DX_RETURNT dx_addccc( DX_PLIST ) {
  unsigned char ccodes;
  // CC_MODIFICATION: changes condition codes
  IREGISTER src_2  = S2ORI + ICC_C;
  IREGISTER result = us_addcc( SOURCE1, src_2, &ccodes );
  WRITE_CC( ccodes );
  WRITE_DEST(result);
}

DX_RETURNT dx_and( DX_PLIST ) {
  IREGISTER result = (SOURCE1 & S2ORI);
  WRITE_DEST(result);
}

DX_RETURNT dx_andcc( DX_PLIST ) {
  unsigned char ccodes;
  // CC_MODIFICATION: changes condition codes
  IREGISTER result = us_andcc( SOURCE1, S2ORI, &ccodes );
  WRITE_CC( ccodes );
  // IREGISTER res2 = SOURCE1 & S2ORI;
  WRITE_DEST(result);
}

DX_RETURNT dx_andn( DX_PLIST ) {
  IREGISTER result = (SOURCE1 & ~S2ORI);
  WRITE_DEST(result);
}

DX_RETURNT dx_andncc( DX_PLIST ) {
  unsigned char ccodes;
  // CC_MODIFICATION: changes condition codes
  IREGISTER result = us_andcc( SOURCE1, ~S2ORI, &ccodes );
  WRITE_CC( ccodes );
  WRITE_DEST(result);
}

DX_RETURNT dx_fba( DX_PLIST ) {
  BRANCH_UNCOND( true );
}

DX_RETURNT dx_ba( DX_PLIST ) {
  BRANCH_UNCOND( true );
}

DX_RETURNT dx_fbpa( DX_PLIST ) {
  BRANCH_UNCOND_P( true );
}

DX_RETURNT dx_bpa( DX_PLIST ) {
  BRANCH_UNCOND_P( true );
}

DX_RETURNT dx_fbn( DX_PLIST ) {
  // Hmm -- branch never -- how does that work
  BRANCH_UNCOND( false );
}

DX_RETURNT dx_fbpn( DX_PLIST ) {
  // Hmm -- branch never -- how does that work
  BRANCH_UNCOND_P( false );
}

DX_RETURNT dx_bn( DX_PLIST ) {
  // Hmm -- branch never -- how does that work
  BRANCH_UNCOND( false );
}

DX_RETURNT dx_bpn( DX_PLIST ) {
  // DX_DISABLED
  // CM imprecise: represents a PREFETCH instruction, but is not implemented
  BRANCH_UNCOND( false );
}

DX_RETURNT dx_bpne( DX_PLIST ) {
  BRANCH_P( !COND_CODE_Z );
}

DX_RETURNT dx_bpe( DX_PLIST ) {
  BRANCH_P( COND_CODE_Z );
}

DX_RETURNT dx_bpg( DX_PLIST ) {
  BRANCH_P( !( COND_CODE_Z | (COND_CODE_N ^ COND_CODE_V) ) );
}

DX_RETURNT dx_bple( DX_PLIST ) {
  BRANCH_P( ( COND_CODE_Z | (COND_CODE_N ^ COND_CODE_V) ) );
}

DX_RETURNT dx_bpge( DX_PLIST ) {
  BRANCH_P( !(COND_CODE_N ^ COND_CODE_V) );
}

DX_RETURNT dx_bpl( DX_PLIST ) {
  BRANCH_P( (COND_CODE_N ^ COND_CODE_V) );
}

DX_RETURNT dx_bpgu( DX_PLIST ) {
  BRANCH_P( !(COND_CODE_C | COND_CODE_Z) );
}

DX_RETURNT dx_bpleu( DX_PLIST ) {
  BRANCH_P( (COND_CODE_C | COND_CODE_Z) );
}

DX_RETURNT dx_bpcc( DX_PLIST ) {
  BRANCH_P( !COND_CODE_C );
}

DX_RETURNT dx_bpcs( DX_PLIST ) {
  BRANCH_P( COND_CODE_C );
}

DX_RETURNT dx_bppos( DX_PLIST ) {
  BRANCH_P( !COND_CODE_N );
}

DX_RETURNT dx_bpneg( DX_PLIST ) {
  BRANCH_P( COND_CODE_N );
}

DX_RETURNT dx_bpvc( DX_PLIST ) {
  BRANCH_P( !COND_CODE_V );
}

DX_RETURNT dx_bpvs( DX_PLIST ) {
  BRANCH_P( COND_CODE_V );
}

// FIXFIXFIX
#if 0
DX_RETURNT dx_call( DX_PLIST ) {

  // set r[15] is always the dest of the call (set r[15] to the current PC)
  WRITE_DEST( getVPC() );

  IREGISTER dest = (int64) getVPC() + (int64) getStaticInst()->getOffset();
#if 0
  m_pseq->out_info("CALLINST\n");
  m_pseq->out_info("VPC    0x%0llx\n", getVPC() );
  m_pseq->out_info("IMM    0x%0llx\n", getStaticInst()->getOffset() );
  m_pseq->out_info("DEST   0x%0llx\n", dest );
#endif
  BRANCH_INDIRECT( dest );
}
#endif

DX_RETURNT dx_fabss( DX_PLIST ) {
  float d = SOURCE2_S;
  d = fabs( d );
  WRITE_DEST_S( d );
}

DX_RETURNT dx_fabsd( DX_PLIST ) {
  WRITE_DEST_D( fabs(SOURCE2_D) );
}

DX_RETURNT dx_fabsq( DX_PLIST ) {
  NOT_IMPLEMENTED;
  // WRITE_DEST_Q( fabs(SOURCE2_Q) );
}

DX_RETURNT dx_fadds( DX_PLIST ) {
  WRITE_DEST_S( SOURCE1_S + SOURCE2_S );
  //UPDATE_FSR;
}

DX_RETURNT dx_faddd( DX_PLIST ) {
  WRITE_DEST_D( SOURCE1_D + SOURCE2_D );
  //UPDATE_FSR;
}

DX_RETURNT dx_faddq( DX_PLIST ) {
  NOT_IMPLEMENTED;
  // WRITE_DEST_Q( SOURCE1_Q + SOURCE2_Q );
}

DX_RETURNT dx_fsubs( DX_PLIST ) {
  WRITE_DEST_S( SOURCE1_S - SOURCE2_S );
  //UPDATE_FSR;
}

DX_RETURNT dx_fsubd( DX_PLIST ) {
  WRITE_DEST_D( SOURCE1_D - SOURCE2_D );
  //UPDATE_FSR;
}

DX_RETURNT dx_fsubq( DX_PLIST ) {
  NOT_IMPLEMENTED;
  // WRITE_DEST_Q( SOURCE1_Q - SOURCE2_Q );
}

DX_RETURNT dx_fcmps( DX_PLIST ) {
  FCMP( S );
}

DX_RETURNT dx_fcmpd( DX_PLIST ) {
  FCMP( D );
}

DX_RETURNT dx_fcmpq( DX_PLIST ) {
  NOT_IMPLEMENTED;
}

DX_RETURNT dx_fcmpes( DX_PLIST ) {
  FCMP( S );
}

DX_RETURNT dx_fcmped( DX_PLIST ) {
  FCMP( D );
}

DX_RETURNT dx_fcmpeq( DX_PLIST ) {
  NOT_IMPLEMENTED;
}

DX_RETURNT dx_fmovs( DX_PLIST ) {
  // safer to move FPs as integers: conversions can happen if moved as floats
  WRITE_DEST( SOURCE2 );
}

DX_RETURNT dx_fmovd( DX_PLIST ) {
  WRITE_DEST_DI( SOURCE2_DI );
}

DX_RETURNT dx_fmovq( DX_PLIST ) {
  NOT_IMPLEMENTED;
  // WRITE_DEST_Q( SOURCE2_Q );
}

DX_RETURNT dx_fnegs( DX_PLIST ) {
  WRITE_DEST_S( -SOURCE2_S );
}

DX_RETURNT dx_fnegd( DX_PLIST ) {
  WRITE_DEST_D( -SOURCE2_D );
}

DX_RETURNT dx_fnegq( DX_PLIST ) {
  NOT_IMPLEMENTED;
  // WRITE_DEST_Q( -SOURCE2_Q );
}

DX_RETURNT dx_fmuls( DX_PLIST ) {
  WRITE_DEST_S( SOURCE1_S * SOURCE2_S );
  //UPDATE_FSR;
}

DX_RETURNT dx_fmuld( DX_PLIST ) {
  WRITE_DEST_D( SOURCE1_D * SOURCE2_D );
  //UPDATE_FSR;
}

DX_RETURNT dx_fmulq( DX_PLIST ) {
  NOT_IMPLEMENTED;
  // WRITE_DEST_Q( SOURCE1_Q * SOURCE2_Q );
}

DX_RETURNT dx_fdivs( DX_PLIST ) {
  WRITE_DEST_S( SOURCE1_S / SOURCE2_S );
  //UPDATE_FSR;
}

DX_RETURNT dx_fdivd( DX_PLIST ) {
  WRITE_DEST_D( SOURCE1_D / SOURCE2_D );
  //UPDATE_FSR;
}

DX_RETURNT dx_fdivq( DX_PLIST ) {
  NOT_IMPLEMENTED;
  // WRITE_DEST_Q( SOURCE1_Q / SOURCE2_Q );
}

DX_RETURNT dx_fsmuld( DX_PLIST ) {
  WRITE_DEST_D( SOURCE1_S * SOURCE2_S );
}

DX_RETURNT dx_fdmulq( DX_PLIST ) {
  NOT_IMPLEMENTED;
  // WRITE_DEST_Q( SOURCE1_D * SOURCE2_D );
}

DX_RETURNT dx_fsqrts( DX_PLIST ) {
  WRITE_DEST_S( sqrt( SOURCE2_S ) );
}

DX_RETURNT dx_fsqrtd( DX_PLIST ) {
  WRITE_DEST_D( sqrt( SOURCE2_D ) );
}

DX_RETURNT dx_fsqrtq( DX_PLIST ) {
  NOT_IMPLEMENTED;
  // WRITE_DEST_Q( sqrt( SOURCE2_Q ) );
}

// FIXFIXFIX
#if 0
DX_RETURNT dx_retrn( DX_PLIST ) {
  // DX_DISABLED
  // CM imprecise retrn has restore semantics as well!
  IREGISTER dest = SOURCE1 + S2ORI;
  BRANCH_INDIRECT( dest );
}
#endif

DX_RETURNT dx_brz( DX_PLIST ) {
  BRANCH_REGISTER( == );
}

DX_RETURNT dx_brlez( DX_PLIST ) {
  BRANCH_REGISTER( <= );
}

DX_RETURNT dx_brlz( DX_PLIST ) {
  BRANCH_REGISTER( < );
}

DX_RETURNT dx_brnz( DX_PLIST ) {
  BRANCH_REGISTER( != );
}

DX_RETURNT dx_brgz( DX_PLIST ) {
  BRANCH_REGISTER( > );
}

DX_RETURNT dx_brgez( DX_PLIST ) {
  BRANCH_REGISTER( >= );
}

DX_RETURNT dx_fbu( DX_PLIST ) {
  BRANCH( COND_CODE_U );
}

DX_RETURNT dx_fbg( DX_PLIST ) {
  BRANCH( COND_CODE_G );
}

DX_RETURNT dx_fbug( DX_PLIST ) {
  BRANCH( COND_CODE_G | COND_CODE_U );
}

DX_RETURNT dx_fbl( DX_PLIST ) {
  BRANCH( COND_CODE_L );
}

DX_RETURNT dx_fbul( DX_PLIST ) {
  BRANCH( COND_CODE_L | COND_CODE_U );
}

DX_RETURNT dx_fblg( DX_PLIST ) {
  BRANCH( COND_CODE_L | COND_CODE_G );
}

DX_RETURNT dx_fbne( DX_PLIST ) {
  BRANCH( COND_CODE_L | COND_CODE_G | COND_CODE_U );
}

DX_RETURNT dx_fbe( DX_PLIST ) {
  BRANCH( COND_CODE_E );
}

DX_RETURNT dx_fbue( DX_PLIST ) {
  BRANCH( COND_CODE_E | COND_CODE_U );
}

DX_RETURNT dx_fbge( DX_PLIST ) {
  BRANCH( COND_CODE_E | COND_CODE_G );
}

DX_RETURNT dx_fbuge( DX_PLIST ) {
  BRANCH( COND_CODE_E | COND_CODE_G | COND_CODE_U );
}

DX_RETURNT dx_fble( DX_PLIST ) {
  BRANCH( COND_CODE_E | COND_CODE_L );
}

DX_RETURNT dx_fbule( DX_PLIST ) {
  BRANCH( COND_CODE_E | COND_CODE_L | COND_CODE_U );
}

DX_RETURNT dx_fbo( DX_PLIST ) {
  BRANCH( COND_CODE_E | COND_CODE_L | COND_CODE_G );
}

DX_RETURNT dx_fbpu( DX_PLIST ) {
  BRANCH( COND_CODE_U );
}

DX_RETURNT dx_fbpg( DX_PLIST ) {
  BRANCH( COND_CODE_G );
}

DX_RETURNT dx_fbpug( DX_PLIST ) {
  BRANCH( COND_CODE_G | COND_CODE_U );
}

DX_RETURNT dx_fbpl( DX_PLIST ) {
  BRANCH( COND_CODE_L );
}

DX_RETURNT dx_fbpul( DX_PLIST ) {
  BRANCH( COND_CODE_L | COND_CODE_U );
}

DX_RETURNT dx_fbplg( DX_PLIST ) {
  BRANCH( COND_CODE_L | COND_CODE_G );
}

DX_RETURNT dx_fbpne( DX_PLIST ) {
  BRANCH( COND_CODE_L | COND_CODE_G | COND_CODE_U );
}

DX_RETURNT dx_fbpe( DX_PLIST ) {
  BRANCH( COND_CODE_E );
}

DX_RETURNT dx_fbpue( DX_PLIST ) {
  BRANCH( COND_CODE_E | COND_CODE_U );
}

DX_RETURNT dx_fbpge( DX_PLIST ) {
  BRANCH( COND_CODE_E | COND_CODE_G );
}

DX_RETURNT dx_fbpuge( DX_PLIST ) {
  BRANCH( COND_CODE_E | COND_CODE_G | COND_CODE_U );
}

DX_RETURNT dx_fbple( DX_PLIST ) {
  BRANCH( COND_CODE_E | COND_CODE_L );
}

DX_RETURNT dx_fbpule( DX_PLIST ) {
  BRANCH( COND_CODE_E | COND_CODE_L | COND_CODE_U );
}

DX_RETURNT dx_fbpo( DX_PLIST ) {
  BRANCH( COND_CODE_E | COND_CODE_L | COND_CODE_G );
}

DX_RETURNT dx_bne( DX_PLIST ) {
  BRANCH( !COND_CODE_Z );
}

DX_RETURNT dx_be( DX_PLIST ) {
  BRANCH( COND_CODE_Z );
}

DX_RETURNT dx_bg( DX_PLIST ) {
  BRANCH( !( COND_CODE_Z | (COND_CODE_N ^ COND_CODE_V) ) );
}

DX_RETURNT dx_ble( DX_PLIST ) {
  BRANCH( ( COND_CODE_Z | (COND_CODE_N ^ COND_CODE_V) ) );
}

DX_RETURNT dx_bge( DX_PLIST ) {
  BRANCH( !(COND_CODE_N ^ COND_CODE_V) );
}

DX_RETURNT dx_bl( DX_PLIST ) {
  BRANCH( (COND_CODE_N ^ COND_CODE_V) );
}

DX_RETURNT dx_bgu( DX_PLIST ) {
  BRANCH( !(COND_CODE_C | COND_CODE_Z) );
}

DX_RETURNT dx_bleu( DX_PLIST ) {
  BRANCH( (COND_CODE_C | COND_CODE_Z) );
}

DX_RETURNT dx_bcc( DX_PLIST ) {
  BRANCH( !COND_CODE_C );
}

DX_RETURNT dx_bcs( DX_PLIST ) {
  BRANCH( COND_CODE_C );
}

DX_RETURNT dx_bpos( DX_PLIST ) {
  BRANCH( !COND_CODE_N );
}

DX_RETURNT dx_bneg( DX_PLIST ) {
  BRANCH( COND_CODE_N );
}

DX_RETURNT dx_bvc( DX_PLIST ) {
  BRANCH( !COND_CODE_V );
}

DX_RETURNT dx_bvs( DX_PLIST ) {
  BRANCH( COND_CODE_V );
}

DX_RETURNT dx_fstox( DX_PLIST ) {
  IREGISTER result = (IREGISTER) SOURCE2_S;
  WRITE_DEST_S( result );
}

DX_RETURNT dx_fdtox( DX_PLIST ) {
  IREGISTER result = (IREGISTER) SOURCE2_D;
  WRITE_DEST_S( result );
}

DX_RETURNT dx_fqtox( DX_PLIST ) {
  NOT_IMPLEMENTED;
}

DX_RETURNT dx_fstoi( DX_PLIST ) {
  IREGISTER result = (int) SOURCE2_S;
  WRITE_DEST( result );
}

DX_RETURNT dx_fdtoi( DX_PLIST ) {
  IREGISTER result = (int) SOURCE2_D;
  WRITE_DEST( result );
}

DX_RETURNT dx_fqtoi( DX_PLIST ) {
  NOT_IMPLEMENTED;
}

DX_RETURNT dx_fstod( DX_PLIST ) {
  WRITE_DEST_D( SOURCE2_S );
}

DX_RETURNT dx_fstoq( DX_PLIST ) {
  NOT_IMPLEMENTED;
}

DX_RETURNT dx_fdtos( DX_PLIST ) {
  WRITE_DEST_S( SOURCE2_D );
}

DX_RETURNT dx_fdtoq( DX_PLIST ) {
  NOT_IMPLEMENTED;
}

DX_RETURNT dx_fqtos( DX_PLIST ) {
  NOT_IMPLEMENTED;
}

DX_RETURNT dx_fqtod( DX_PLIST ) {
  NOT_IMPLEMENTED;
}

DX_RETURNT dx_fxtos( DX_PLIST ) {
  IREGISTER result = SOURCE2_DI;
  WRITE_DEST_S( (int64) result );
}

DX_RETURNT dx_fxtod( DX_PLIST ) {
  IREGISTER result = SOURCE2_DI;
  WRITE_DEST_D( (int64) result );
}

DX_RETURNT dx_fxtoq( DX_PLIST ) {
  NOT_IMPLEMENTED;
}

DX_RETURNT dx_fitos( DX_PLIST ) {
  IREGISTER result = SOURCE2;
  WRITE_DEST_S( (int32) result );
}

DX_RETURNT dx_fitod( DX_PLIST ) {
  WRITE_DEST_D( (int32) SOURCE2 );
}

DX_RETURNT dx_fitoq( DX_PLIST ) {
  NOT_IMPLEMENTED;
}

//
// Begin FMOVF {single} on {fp} condition codes
//

DX_RETURNT dx_fmovfsn( DX_PLIST ) {
  FMOVECC_F( S, false );
}

DX_RETURNT dx_fmovfsne( DX_PLIST ) {
  FMOVECC_F( S, COND_CODE_L | COND_CODE_G | COND_CODE_U );
}

DX_RETURNT dx_fmovfslg( DX_PLIST ) {
  FMOVECC_F( S, COND_CODE_L | COND_CODE_G );
}

DX_RETURNT dx_fmovfsul( DX_PLIST ) {
  FMOVECC_F( S, COND_CODE_L | COND_CODE_U );
}

DX_RETURNT dx_fmovfsl( DX_PLIST ) {
  FMOVECC_F( S, COND_CODE_L );
}

DX_RETURNT dx_fmovfsug( DX_PLIST ) {
  FMOVECC_F( S, COND_CODE_G | COND_CODE_U );
}

DX_RETURNT dx_fmovfsg( DX_PLIST ) {
  FMOVECC_F( S, COND_CODE_G );
}

DX_RETURNT dx_fmovfsu( DX_PLIST ) {
  FMOVECC_F( S, COND_CODE_U );
}

DX_RETURNT dx_fmovfsa( DX_PLIST ) {
  FMOVECC_F( S, true );
}

DX_RETURNT dx_fmovfse( DX_PLIST ) {
  FMOVECC_F( S, COND_CODE_E );
}

DX_RETURNT dx_fmovfsue( DX_PLIST ) {
  FMOVECC_F( S, COND_CODE_E | COND_CODE_U );
}

DX_RETURNT dx_fmovfsge( DX_PLIST ) {
  FMOVECC_F( S, COND_CODE_E | COND_CODE_G );
}

DX_RETURNT dx_fmovfsuge( DX_PLIST ) {
  FMOVECC_F( S, COND_CODE_E | COND_CODE_G | COND_CODE_U );
}

DX_RETURNT dx_fmovfsle( DX_PLIST ) {
  FMOVECC_F( S, COND_CODE_E | COND_CODE_L );
}

DX_RETURNT dx_fmovfsule( DX_PLIST ) {
  FMOVECC_F( S, COND_CODE_E | COND_CODE_L | COND_CODE_U );
}

DX_RETURNT dx_fmovfso( DX_PLIST ) {
  FMOVECC_F( S, COND_CODE_E | COND_CODE_L | COND_CODE_G );
}

//
// Begin FMOVF {double} on {fp} condition codes
//

DX_RETURNT dx_fmovfdn( DX_PLIST ) {
  FMOVECC_F( D, false );
}

DX_RETURNT dx_fmovfdne( DX_PLIST ) {
  FMOVECC_F( D, COND_CODE_L | COND_CODE_G | COND_CODE_U );
}

DX_RETURNT dx_fmovfdlg( DX_PLIST ) {
  FMOVECC_F( D, COND_CODE_L | COND_CODE_G );
}

DX_RETURNT dx_fmovfdul( DX_PLIST ) {
  FMOVECC_F( D, COND_CODE_L | COND_CODE_U );
}

DX_RETURNT dx_fmovfdl( DX_PLIST ) {
  FMOVECC_F( D, COND_CODE_L );
}

DX_RETURNT dx_fmovfdug( DX_PLIST ) {
  FMOVECC_F( D, COND_CODE_G | COND_CODE_U );
}

DX_RETURNT dx_fmovfdg( DX_PLIST ) {
  FMOVECC_F( D, COND_CODE_G );
}

DX_RETURNT dx_fmovfdu( DX_PLIST ) {
  FMOVECC_F( D, COND_CODE_U );
}

DX_RETURNT dx_fmovfda( DX_PLIST ) {
  FMOVECC_F( D, true );
}

DX_RETURNT dx_fmovfde( DX_PLIST ) {
  FMOVECC_F( D, COND_CODE_E );
}

DX_RETURNT dx_fmovfdue( DX_PLIST ) {
  FMOVECC_F( D, COND_CODE_E | COND_CODE_U );
}

DX_RETURNT dx_fmovfdge( DX_PLIST ) {
  FMOVECC_F( D, COND_CODE_E | COND_CODE_G );
}

DX_RETURNT dx_fmovfduge( DX_PLIST ) {
  FMOVECC_F( D, COND_CODE_E | COND_CODE_G | COND_CODE_U );
}

DX_RETURNT dx_fmovfdle( DX_PLIST ) {
  FMOVECC_F( D, COND_CODE_E | COND_CODE_L );
}

DX_RETURNT dx_fmovfdule( DX_PLIST ) {
  FMOVECC_F( D, COND_CODE_E | COND_CODE_L | COND_CODE_U );
}

DX_RETURNT dx_fmovfdo( DX_PLIST ) {
  FMOVECC_F( D, COND_CODE_E | COND_CODE_L | COND_CODE_G );
}

//
// Begin FMOVF {quad} on {fp} condition codes
//

DX_RETURNT dx_fmovfqn( DX_PLIST ) {
  // FMOVECC_F( Q, false );
  NOT_IMPLEMENTED;
}

DX_RETURNT dx_fmovfqne( DX_PLIST ) {
  // FMOVECC_F( Q, COND_CODE_L | COND_CODE_G | COND_CODE_U );
  NOT_IMPLEMENTED;
}

DX_RETURNT dx_fmovfqlg( DX_PLIST ) {
  // FMOVECC_F( Q, COND_CODE_L | COND_CODE_G );
  NOT_IMPLEMENTED;
}

DX_RETURNT dx_fmovfqul( DX_PLIST ) {
  // FMOVECC_F( Q, COND_CODE_L | COND_CODE_U );
  NOT_IMPLEMENTED;
}

DX_RETURNT dx_fmovfql( DX_PLIST ) {
  // FMOVECC_F( Q, COND_CODE_L );
  NOT_IMPLEMENTED;
}

DX_RETURNT dx_fmovfqug( DX_PLIST ) {
  // FMOVECC_F( Q, COND_CODE_G | COND_CODE_U );
  NOT_IMPLEMENTED;
}

DX_RETURNT dx_fmovfqg( DX_PLIST ) {
  // FMOVECC_F( Q, COND_CODE_G );
  NOT_IMPLEMENTED;
}

DX_RETURNT dx_fmovfqu( DX_PLIST ) {
  // FMOVECC_F( Q, COND_CODE_U );
  NOT_IMPLEMENTED;
}

DX_RETURNT dx_fmovfqa( DX_PLIST ) {
  // FMOVECC_F( Q, true );
  NOT_IMPLEMENTED;
}

DX_RETURNT dx_fmovfqe( DX_PLIST ) {
  // FMOVECC_F( Q, COND_CODE_E );
  NOT_IMPLEMENTED;
}

DX_RETURNT dx_fmovfque( DX_PLIST ) {
  // FMOVECC_F( Q, COND_CODE_E | COND_CODE_U );
  NOT_IMPLEMENTED;
}

DX_RETURNT dx_fmovfqge( DX_PLIST ) {
  // FMOVECC_F( Q, COND_CODE_E | COND_CODE_G );
  NOT_IMPLEMENTED;
}

DX_RETURNT dx_fmovfquge( DX_PLIST ) {
  // FMOVECC_F( Q, COND_CODE_E | COND_CODE_G | COND_CODE_U );
  NOT_IMPLEMENTED;
}

DX_RETURNT dx_fmovfqle( DX_PLIST ) {
  // FMOVECC_F( Q, COND_CODE_E | COND_CODE_L );
  NOT_IMPLEMENTED;
}

DX_RETURNT dx_fmovfqule( DX_PLIST ) {
  // FMOVECC_F( Q, COND_CODE_E | COND_CODE_L | COND_CODE_U );
  NOT_IMPLEMENTED;
}

DX_RETURNT dx_fmovfqo( DX_PLIST ) {
  // FMOVECC_F( Q, COND_CODE_E | COND_CODE_L | COND_CODE_G );
  NOT_IMPLEMENTED;
}


//
// Begin FMOVF {single} on {integer} condition codes
//

DX_RETURNT dx_fmovsn( DX_PLIST ) {
  FMOVECC( S, false );
}

DX_RETURNT dx_fmovse( DX_PLIST ) {
  FMOVECC( S, COND_CODE_Z );
}

DX_RETURNT dx_fmovsle( DX_PLIST ) {
  FMOVECC( S, ( COND_CODE_Z | (COND_CODE_N ^ COND_CODE_V) ) );
}

DX_RETURNT dx_fmovsl( DX_PLIST ) {
  FMOVECC( S, (COND_CODE_N ^ COND_CODE_V) );
}

DX_RETURNT dx_fmovsleu( DX_PLIST ) {
  FMOVECC( S, (COND_CODE_C | COND_CODE_Z) );
}

DX_RETURNT dx_fmovscs( DX_PLIST ) {
  FMOVECC( S, COND_CODE_C );
}

DX_RETURNT dx_fmovsneg( DX_PLIST ) {
  FMOVECC( S, COND_CODE_N );
}

DX_RETURNT dx_fmovsvs( DX_PLIST ) {
  FMOVECC( S, COND_CODE_V );
}

DX_RETURNT dx_fmovsa( DX_PLIST ) {
  FMOVECC( S, true );
}

DX_RETURNT dx_fmovsne( DX_PLIST ) {
  FMOVECC( S, !COND_CODE_Z );
}

DX_RETURNT dx_fmovsg( DX_PLIST ) {
  FMOVECC( S, !( COND_CODE_Z | (COND_CODE_N ^ COND_CODE_V) ) );
}

DX_RETURNT dx_fmovsge( DX_PLIST ) {
  FMOVECC( S, !(COND_CODE_N ^ COND_CODE_V) );
}

DX_RETURNT dx_fmovsgu( DX_PLIST ) {
  FMOVECC( S, !(COND_CODE_C | COND_CODE_Z) );
}

DX_RETURNT dx_fmovscc( DX_PLIST ) {
  FMOVECC( S, !COND_CODE_C );
}

DX_RETURNT dx_fmovspos( DX_PLIST ) {
  FMOVECC( S, !COND_CODE_N );
}

DX_RETURNT dx_fmovsvc( DX_PLIST ) {
  FMOVECC( S, !COND_CODE_V );
}

//
// Begin FMOVF {double} on {integer} condition codes
//

DX_RETURNT dx_fmovdn( DX_PLIST ) {
  FMOVECC( D, false );
}

DX_RETURNT dx_fmovde( DX_PLIST ) {
  FMOVECC( D, COND_CODE_Z );
}

DX_RETURNT dx_fmovdle( DX_PLIST ) {
  FMOVECC( D, ( COND_CODE_Z | (COND_CODE_N ^ COND_CODE_V) ) );
}

DX_RETURNT dx_fmovdl( DX_PLIST ) {
  FMOVECC( D, (COND_CODE_N ^ COND_CODE_V) );
}

DX_RETURNT dx_fmovdleu( DX_PLIST ) {
  FMOVECC( D, (COND_CODE_C | COND_CODE_Z) );
}

DX_RETURNT dx_fmovdcs( DX_PLIST ) {
  FMOVECC( D, COND_CODE_C );
}

DX_RETURNT dx_fmovdneg( DX_PLIST ) {
  FMOVECC( D, COND_CODE_N );
}

DX_RETURNT dx_fmovdvs( DX_PLIST ) {
  FMOVECC( D, COND_CODE_V );
}

DX_RETURNT dx_fmovda( DX_PLIST ) {
  FMOVECC( D, true );
}

DX_RETURNT dx_fmovdne( DX_PLIST ) {
  FMOVECC( D, !COND_CODE_Z );
}

DX_RETURNT dx_fmovdg( DX_PLIST ) {
  FMOVECC( D, !( COND_CODE_Z | (COND_CODE_N ^ COND_CODE_V) ) );
}

DX_RETURNT dx_fmovdge( DX_PLIST ) {
  FMOVECC( D, !(COND_CODE_N ^ COND_CODE_V) );
}

DX_RETURNT dx_fmovdgu( DX_PLIST ) {
  FMOVECC( D, !(COND_CODE_C | COND_CODE_Z) );
}

DX_RETURNT dx_fmovdcc( DX_PLIST ) {
  FMOVECC( D, !COND_CODE_C );
}

DX_RETURNT dx_fmovdpos( DX_PLIST ) {
  FMOVECC( D, !COND_CODE_N );
}

DX_RETURNT dx_fmovdvc( DX_PLIST ) {
  FMOVECC( D, !COND_CODE_V );
}

//
// Begin FMOVF {quad} on {integer} condition codes
//

DX_RETURNT dx_fmovqn( DX_PLIST ) {
  // FMOVECC( Q, false );
  NOT_IMPLEMENTED;
}

DX_RETURNT dx_fmovqe( DX_PLIST ) {
  // FMOVECC( Q, COND_CODE_Z );
  NOT_IMPLEMENTED;
}

DX_RETURNT dx_fmovqle( DX_PLIST ) {
  // FMOVECC( Q, ( COND_CODE_Z | (COND_CODE_N ^ COND_CODE_V) ) );
  NOT_IMPLEMENTED;
}

DX_RETURNT dx_fmovql( DX_PLIST ) {
  // FMOVECC( Q, (COND_CODE_N ^ COND_CODE_V) );
  NOT_IMPLEMENTED;
}

DX_RETURNT dx_fmovqleu( DX_PLIST ) {
  // FMOVECC( Q, (COND_CODE_C | COND_CODE_Z) );
  NOT_IMPLEMENTED;
}

DX_RETURNT dx_fmovqcs( DX_PLIST ) {
  // FMOVECC( Q, COND_CODE_C );
  NOT_IMPLEMENTED;
}

DX_RETURNT dx_fmovqneg( DX_PLIST ) {
  // FMOVECC( Q, COND_CODE_N );
  NOT_IMPLEMENTED;
}

DX_RETURNT dx_fmovqvs( DX_PLIST ) {
  // FMOVECC( Q, COND_CODE_V );
  NOT_IMPLEMENTED;
}

DX_RETURNT dx_fmovqa( DX_PLIST ) {
  // FMOVECC( Q, true );
  NOT_IMPLEMENTED;
}

DX_RETURNT dx_fmovqne( DX_PLIST ) {
  // FMOVECC( Q, !COND_CODE_Z );
  NOT_IMPLEMENTED;
}

DX_RETURNT dx_fmovqg( DX_PLIST ) {
  // FMOVECC( Q, !( COND_CODE_Z | (COND_CODE_N ^ COND_CODE_V) ) );
  NOT_IMPLEMENTED;
}

DX_RETURNT dx_fmovqge( DX_PLIST ) {
  // FMOVECC( Q, !(COND_CODE_N ^ COND_CODE_V) );
  NOT_IMPLEMENTED;
}

DX_RETURNT dx_fmovqgu( DX_PLIST ) {
  // FMOVECC( Q, !(COND_CODE_C | COND_CODE_Z) );
  NOT_IMPLEMENTED;
}

DX_RETURNT dx_fmovqcc( DX_PLIST ) {
  // FMOVECC( Q, !COND_CODE_C );
  NOT_IMPLEMENTED;
}

DX_RETURNT dx_fmovqpos( DX_PLIST ) {
  // FMOVECC( Q, !COND_CODE_N );
  NOT_IMPLEMENTED;
}

DX_RETURNT dx_fmovqvc( DX_PLIST ) {
  // FMOVECC( Q, !COND_CODE_V );
  NOT_IMPLEMENTED;
}

//
// move {integer} registers based on {integer} cond codes
//

DX_RETURNT dx_mova( DX_PLIST ) {
  MOVECC( true );
}

DX_RETURNT dx_movfa( DX_PLIST ) {
  MOVECC_F( true );
}

DX_RETURNT dx_movn( DX_PLIST ) {
  MOVECC( false );
}

DX_RETURNT dx_movfn( DX_PLIST ) {
  MOVECC_F( false );
}

DX_RETURNT dx_movne( DX_PLIST ) {
  MOVECC( !COND_CODE_Z );
}

DX_RETURNT dx_move( DX_PLIST ) {
  MOVECC( COND_CODE_Z );
}

DX_RETURNT dx_movg( DX_PLIST ) {
  MOVECC( !( COND_CODE_Z | (COND_CODE_N ^ COND_CODE_V) ) );
}

DX_RETURNT dx_movle( DX_PLIST ) {
  MOVECC( ( COND_CODE_Z | (COND_CODE_N ^ COND_CODE_V) ) );
}

DX_RETURNT dx_movge( DX_PLIST ) {
  MOVECC( !(COND_CODE_N ^ COND_CODE_V) );
}

DX_RETURNT dx_movl( DX_PLIST ) {
  MOVECC( (COND_CODE_N ^ COND_CODE_V) );
}

DX_RETURNT dx_movgu( DX_PLIST ) {
  MOVECC( !(COND_CODE_C | COND_CODE_Z) );
}

DX_RETURNT dx_movleu( DX_PLIST ) {
  MOVECC( (COND_CODE_C | COND_CODE_Z) );
}

DX_RETURNT dx_movcc( DX_PLIST ) {
  MOVECC( !COND_CODE_C );
}

DX_RETURNT dx_movcs( DX_PLIST ) {
  MOVECC( COND_CODE_C );
}

DX_RETURNT dx_movpos( DX_PLIST ) {
  MOVECC( !COND_CODE_N );
}

DX_RETURNT dx_movneg( DX_PLIST ) {
  MOVECC( COND_CODE_N );
}

DX_RETURNT dx_movvc( DX_PLIST ) {
  MOVECC( !COND_CODE_V );
}

DX_RETURNT dx_movvs( DX_PLIST ) {
  MOVECC( COND_CODE_V );
}

//
// move {integer} registers based on {fp} cond codes
//

DX_RETURNT dx_movfu( DX_PLIST ) {
  MOVECC_F( COND_CODE_U );
}

DX_RETURNT dx_movfg( DX_PLIST ) {
  MOVECC_F( COND_CODE_G );
}

DX_RETURNT dx_movfug( DX_PLIST ) {
  MOVECC_F( COND_CODE_G | COND_CODE_U );
}

DX_RETURNT dx_movfl( DX_PLIST ) {
  MOVECC_F( COND_CODE_L );
}

DX_RETURNT dx_movful( DX_PLIST ) {
  MOVECC_F( COND_CODE_L | COND_CODE_U );
}

DX_RETURNT dx_movflg( DX_PLIST ) {
  MOVECC_F( COND_CODE_L | COND_CODE_G );
}

DX_RETURNT dx_movfne( DX_PLIST ) {
  MOVECC_F( COND_CODE_L | COND_CODE_G | COND_CODE_U );
}

DX_RETURNT dx_movfe( DX_PLIST ) {
  MOVECC_F( COND_CODE_E );
}

DX_RETURNT dx_movfue( DX_PLIST ) {
  MOVECC_F( COND_CODE_E | COND_CODE_U );
}

DX_RETURNT dx_movfge( DX_PLIST ) {
  MOVECC_F( COND_CODE_E | COND_CODE_G );
}

DX_RETURNT dx_movfuge( DX_PLIST ) {
  MOVECC_F( COND_CODE_E | COND_CODE_G | COND_CODE_U );
}

DX_RETURNT dx_movfle( DX_PLIST ) {
  MOVECC_F( COND_CODE_E | COND_CODE_L );
}

DX_RETURNT dx_movfule( DX_PLIST ) {
  MOVECC_F( COND_CODE_E | COND_CODE_L | COND_CODE_U );
}

DX_RETURNT dx_movfo( DX_PLIST ) {
  MOVECC_F( COND_CODE_E | COND_CODE_L | COND_CODE_G );
}

DX_RETURNT dx_movrz( DX_PLIST ) {
  MOVER( == );
}

DX_RETURNT dx_movrlez( DX_PLIST ) {
  MOVER( <= );
}

DX_RETURNT dx_movrlz( DX_PLIST ) {
  MOVER( < );
}

DX_RETURNT dx_movrnz( DX_PLIST ) {
  MOVER( != );
}

DX_RETURNT dx_movrgz( DX_PLIST ) {
  MOVER( > );
}

DX_RETURNT dx_movrgez( DX_PLIST ) {
  MOVER( >= );
}

//
//
//

DX_RETURNT dx_sub( DX_PLIST ) {
  IREGISTER result = (SOURCE1 - S2ORI);
  WRITE_DEST(result);
}

DX_RETURNT dx_subcc( DX_PLIST ) {
  unsigned char ccodes;
  // CC_MODIFICATION: changes condition codes
  // usub64 and ssub64 map to same function call
  IREGISTER result = us_subcc( SOURCE1, S2ORI, &ccodes );
  //m_pseq->out_info(" subcc -- %d\n", ccodes);
  WRITE_CC( ccodes );
  WRITE_DEST(result);
}

DX_RETURNT dx_subc( DX_PLIST ) {
  IREGISTER result = (SOURCE1 - S2ORI - ICC_C);
  WRITE_DEST(result);
}

DX_RETURNT dx_subccc( DX_PLIST ) {
  unsigned char ccodes;

  // CC_MODIFICATION: changes condition codes
  // usub64 and ssub64 map to same function call
  IREGISTER result = us_subcc( SOURCE1, (S2ORI + ICC_C), &ccodes );
  //m_pseq->out_info(" subccc -- %d\n", ccodes);
  WRITE_CC( ccodes );
  WRITE_DEST(result);
}

DX_RETURNT dx_or( DX_PLIST ) {
  IREGISTER result = (SOURCE1 | S2ORI);
  WRITE_DEST(result);
}

DX_RETURNT dx_orcc( DX_PLIST ) {
  unsigned char ccodes;
  // CC_MODIFICATION: changes condition codes
  IREGISTER result = us_orcc( SOURCE1, S2ORI, &ccodes );
  WRITE_CC( ccodes );
  WRITE_DEST(result);
}

DX_RETURNT dx_orn( DX_PLIST ) {
  IREGISTER result = (SOURCE1 | ~S2ORI);
  WRITE_DEST(result);
}

DX_RETURNT dx_orncc( DX_PLIST ) {
  unsigned char ccodes;
  // CC_MODIFICATION: changes condition codes
  IREGISTER result = us_orcc( SOURCE1, ~S2ORI, &ccodes );
  WRITE_CC( ccodes );
  WRITE_DEST(result);
}

DX_RETURNT dx_xor( DX_PLIST ) {
  IREGISTER result = (SOURCE1 ^ S2ORI);
  WRITE_DEST(result);
}

DX_RETURNT dx_xorcc( DX_PLIST ) {
  unsigned char ccodes;
  // CC_MODIFICATION: changes condition codes
  IREGISTER result = us_xorcc( SOURCE1, S2ORI, &ccodes );
  WRITE_CC( ccodes );
  WRITE_DEST(result);
}

DX_RETURNT dx_xnor( DX_PLIST ) {
  IREGISTER result = (SOURCE1 ^ ~S2ORI);
  WRITE_DEST(result);
}

DX_RETURNT dx_xnorcc( DX_PLIST ) {
  unsigned char ccodes;
  // CC_MODIFICATION: changes condition codes
  IREGISTER result = us_xorcc( SOURCE1, ~S2ORI, &ccodes );
  WRITE_CC( ccodes );
  WRITE_DEST(result);
}

DX_RETURNT dx_mulx( DX_PLIST ) {
  WRITE_DEST( SOURCE1 * S2ORI );
}

DX_RETURNT dx_sdivx( DX_PLIST ) {
  if ((int64) S2ORI == 0) {
    latch->m_exception = Trap_Division_By_Zero;
  } else {
    WRITE_DEST( (int64) SOURCE1 / (int64) S2ORI );
  }
}

DX_RETURNT dx_udivx( DX_PLIST ) {
  if (S2ORI == 0) {
    latch->m_exception = Trap_Division_By_Zero;
  } else {
    WRITE_DEST( (uint64) SOURCE1 / (uint64) S2ORI );
  }
}

DX_RETURNT dx_sll( DX_PLIST ) {
  // shifting is done modulo 32-bits
  WRITE_DEST( ((uint64) SOURCE1) << (S2ORI % 32) );
}

DX_RETURNT dx_srl( DX_PLIST ) {
  LOGICAL_SHIFT( uint32, >> );
}

DX_RETURNT dx_sra( DX_PLIST ) {
  LOGICAL_SHIFT( int, >> );
}

DX_RETURNT dx_sllx( DX_PLIST ) {
  LOGICAL_SHIFT( uint64, << );
}

DX_RETURNT dx_srlx( DX_PLIST ) {
  LOGICAL_SHIFT( uint64, >>  );
}

DX_RETURNT dx_srax( DX_PLIST ) {
  LOGICAL_SHIFT( int64, >>  );
}

DX_RETURNT dx_taddcc( DX_PLIST ) {
  NOT_IMPLEMENTED;
  // CC_MODIFICATION: changes condition codes
}

DX_RETURNT dx_taddcctv( DX_PLIST ) {
  NOT_IMPLEMENTED;
  // CC_MODIFICATION: changes condition codes
}

DX_RETURNT dx_tsubcc( DX_PLIST ) {
  NOT_IMPLEMENTED;
  // CC_MODIFICATION: changes condition codes
}

DX_RETURNT dx_tsubcctv( DX_PLIST ) {
  NOT_IMPLEMENTED;
  // CC_MODIFICATION: changes condition codes
}

DX_RETURNT dx_popc( DX_PLIST ) {
  NOT_IMPLEMENTED;
}

DX_RETURNT dx_flush( DX_PLIST ) {
}

DX_RETURNT dx_rdcc( DX_PLIST ) {
  IREGISTER  result = READ_CC;
  WRITE_DEST( result );
}

DX_RETURNT dx_wrcc( DX_PLIST ) {
  WRITE_CC( SOURCE1 ^ S2ORI );
}

DX_RETURNT dx_sethi( DX_PLIST ) {
  WRITE_DEST( S2ORI );
}

DX_RETURNT dx_magic( DX_PLIST ) {
  // DX_DISABLED
#if 0
  // throwing this exception will interrupt execution ...
  //     * set stage to complete, so it looks like Execution is complete
  SetStage(COMPLETE_STAGE);
  //     * now throw a magic breakpoint exception
  m_pseq->out_info("magic breakpoint\n");
  fflush(stdout);
  SIM_raise_general_exception( "hfa: magic breakpoint reached." );
#endif
  /* do nothing --
   * using magic breaks for simulating end-of-transaction conditions */
}

DX_RETURNT dx_ldf( DX_PLIST ) {
  READ_MEMORY_UNSIGNED;
}

DX_RETURNT dx_lddf( DX_PLIST ) {
  READ_MEMORY_UNSIGNED;
}

DX_RETURNT dx_ldqf( DX_PLIST ) {
  NOT_IMPLEMENTED;
}

DX_RETURNT dx_ldfa( DX_PLIST ) {
  READ_MEMORY_UNSIGNED;
}

DX_RETURNT dx_ldblk( DX_PLIST ) {
  // DX_DISABLED
  NOT_IMPLEMENTED;
#if 0
  // write to 8 double precision fp registers->..
  MEMORY_OP_TYPE *memop = MEMORY_OP_GET;

  if (memop->isDataValid()) {
    IREGISTER   *data  = memop->getData();
    STAT_INC( m_pseq->m_stat_loads_found );
    int              reg   = getDestReg(0).getVanilla();
    half_t           preg;
    uint32          *ival = (uint32 *) data;
    reg_map_t       *mymap = m_pseq->getFPMap();
    physical_file_t *myrf  = m_pseq->getFPFile();

    // write the renamed register, hence advance 2 single precision regs.
    WRITE_DEST_DI( *data );
    reg  += 2;
    ival += 2;
    for ( int i = 2; i < 8; i ++ ) {
      // write 16 single precision registers with the 64-bit value
      preg = mymap->getMapping( reg++ );
      myrf->setInt( preg, *ival++ );
      
      preg = mymap->getMapping( reg++ );
      myrf->setInt( preg, *ival++ );
    }
  } else {
    STAT_INC( m_pseq->m_stat_loads_notfound );
    NOT_IMPLEMENTED;
  }
#endif
}

DX_RETURNT dx_ldqfa( DX_PLIST ) {
  NOT_IMPLEMENTED;
}

DX_RETURNT dx_ldsb( DX_PLIST ) {
  READ_MEMORY;
}

DX_RETURNT dx_ldsh( DX_PLIST ) {
  READ_MEMORY;
}

DX_RETURNT dx_ldsw( DX_PLIST ) {
  READ_MEMORY;
}

DX_RETURNT dx_ldub( DX_PLIST ) {
  READ_MEMORY_UNSIGNED;
}

DX_RETURNT dx_lduh( DX_PLIST ) {
  READ_MEMORY_UNSIGNED;
}

DX_RETURNT dx_lduw( DX_PLIST ) {
  READ_MEMORY_UNSIGNED;
}

DX_RETURNT dx_ldx( DX_PLIST ) {
  READ_MEMORY_UNSIGNED;
}

DX_RETURNT dx_ldsha( DX_PLIST ) {
  READ_MEMORY;
}

DX_RETURNT dx_ldswa( DX_PLIST ) {
  READ_MEMORY;
}

DX_RETURNT dx_lduba( DX_PLIST ) {
  READ_MEMORY_UNSIGNED;
}

DX_RETURNT dx_lduha( DX_PLIST ) {
  READ_MEMORY_UNSIGNED;
}

DX_RETURNT dx_lduwa( DX_PLIST ) {
  READ_MEMORY_UNSIGNED;
}

DX_RETURNT dx_ldxa( DX_PLIST ) {
  READ_MEMORY_UNSIGNED;
}

DX_RETURNT dx_stf( DX_PLIST ) {
  STORE_INSTRUCTION;
}

DX_RETURNT dx_stdf( DX_PLIST ) {
  STORE_INSTRUCTION;
}

DX_RETURNT dx_stqf( DX_PLIST ) {
  STORE_INSTRUCTION;
}

DX_RETURNT dx_stb( DX_PLIST ) {
  STORE_INSTRUCTION;
}

DX_RETURNT dx_sth( DX_PLIST ) {
  STORE_INSTRUCTION;
}

DX_RETURNT dx_stw( DX_PLIST ) {
  STORE_INSTRUCTION;
}

DX_RETURNT dx_stx( DX_PLIST ) {
  STORE_INSTRUCTION;
}

DX_RETURNT dx_std( DX_PLIST ) {
  STORE_INSTRUCTION;
}

DX_RETURNT dx_stfa( DX_PLIST ) {
  STORE_INSTRUCTION;
}

DX_RETURNT dx_stdfa( DX_PLIST ) {
  STORE_INSTRUCTION;
}

DX_RETURNT dx_stblk( DX_PLIST ) {
  STORE_INSTRUCTION;
}

DX_RETURNT dx_stqfa( DX_PLIST ) {
  STORE_INSTRUCTION;
}

DX_RETURNT dx_stba( DX_PLIST ) {
  STORE_INSTRUCTION;
}

DX_RETURNT dx_stha( DX_PLIST ) {
  STORE_INSTRUCTION;
}

DX_RETURNT dx_stwa( DX_PLIST ) {
  STORE_INSTRUCTION;
}

DX_RETURNT dx_stxa( DX_PLIST ) {
  STORE_INSTRUCTION;
}

DX_RETURNT dx_stda( DX_PLIST ) {
  STORE_INSTRUCTION;
}

DX_RETURNT dx_ldstub( DX_PLIST ) {
  READ_MEMORY_UNSIGNED;

  // store all ones to this byte
  WRITE_MEMORY( 0xff );
}

DX_RETURNT dx_ldstuba( DX_PLIST ) {
  READ_MEMORY_UNSIGNED;

  // store all ones to this byte
  WRITE_MEMORY( 0xff );
}

DX_RETURNT dx_prefetch( DX_PLIST ) {
}

DX_RETURNT dx_prefetcha( DX_PLIST ) {
}

DX_RETURNT dx_stfsr( DX_PLIST ) {
}

DX_RETURNT dx_stxfsr( DX_PLIST ) {
}

DX_RETURNT dx__trap( DX_PLIST ) {
}

DX_RETURNT dx_impdep1( DX_PLIST ) {
}

DX_RETURNT dx_impdep2( DX_PLIST ) {
}

DX_RETURNT dx_membar( DX_PLIST ) {
}

DX_RETURNT dx_stbar( DX_PLIST ) {
}

DX_RETURNT dx_cmp( DX_PLIST ) {
  unsigned char ccodes;
#if 0
  if (getStaticInst()->getFlag(SI_ISIMMEDIATE)) {
    m_pseq->out_info("IMMEDIATE: 0x%0llx\n", getStaticInst()->getImmediate());
    m_pseq->out_info("         : 0x%0x\n", getStaticInst()->getInst());
  }
#endif
  // CC_MODIFICATION: changes condition codes
  us_cmp( SOURCE1, S2ORI, &ccodes );
  WRITE_CC( ccodes );
}

DX_RETURNT dx_mov( DX_PLIST ) {
  IREGISTER result = S2ORI;
  WRITE_DEST(result);
}

DX_RETURNT dx_nop( DX_PLIST ) {
  /* do nothing */
}

DX_RETURNT dx_not( DX_PLIST ) {
  IREGISTER result = ~SOURCE1;
  WRITE_DEST(result);
}

   // FIXFIXFIX
#if 0
DX_RETURNT dx_faligndata( DX_PLIST ) {

  // read the lower 3 bits of the gsr register
  SETCTL_SOURCE( CONTROL_GSR );
  IREGISTER offset = SOURCE_CTL & 0x7;
  IREGISTER rs1 = SOURCE1_DI;
  IREGISTER rs2 = SOURCE2_DI;

  ASSERT( offset < 8 );
  // something like this
  IREGISTER rd;
  
  if ( offset == 0 ) {
    rd = rs1;
  } else if ( offset == 8 ) {
    rd = rs2;
  } else {
    rd = (rs1 << (offset * 8)) | (rs2 >> ((8 - offset) * 8));
  }
  
#if 0
  m_pseq->out_info("rs1  0x%0llx\n", rs1 );
  m_pseq->out_info("rs2  0x%0llx\n", rs2 );
  m_pseq->out_info("gsr %lld\n", offset);

  m_pseq->out_info("suba  0x%0llx\n", rs1 << (offset * 8));
  m_pseq->out_info("subb  0x%0llx\n", (rs2 >> ((8 - offset)*8)));
  m_pseq->out_info("rd    0x%0llx\n", rd );
#endif
  WRITE_DEST_DI( rd );
}
#endif

DX_RETURNT dx_fcmple16( DX_PLIST ) {
  MOVE16( <= );
}

DX_RETURNT dx_fcmpne16( DX_PLIST ) {
  MOVE16( != );
}

DX_RETURNT dx_fcmple32( DX_PLIST ) {
  MOVE32( <= );
}

DX_RETURNT dx_fcmpne32( DX_PLIST ) {
  MOVE32( != );
}

DX_RETURNT dx_fcmpgt16( DX_PLIST ) {
  MOVE16( > );
}

DX_RETURNT dx_fcmpeq16( DX_PLIST ) {
  MOVE16( == );
}

DX_RETURNT dx_fcmpgt32( DX_PLIST ) {
  MOVE32( > );
}

DX_RETURNT dx_fcmpeq32( DX_PLIST ) {
  MOVE32( == );
}

DX_RETURNT dx_ill( DX_PLIST ) {
}


//**************************************************************************
void exec_fn_t::initialize( void )
{
  for (uint32 i = 0; i < i_maxcount; i++) {
    m_jump_table[i] = &dx_ill;
    g_dynamic_debug_table[i] = false;
  }

  m_jump_table[i_add] =         &dx_add;
  m_jump_table[i_addcc] =       &dx_addcc;
  m_jump_table[i_addc] =        &dx_addc;
  m_jump_table[i_addccc] =      &dx_addccc;
  m_jump_table[i_and] =         &dx_and;
  m_jump_table[i_andcc] =       &dx_andcc;
  m_jump_table[i_andn] =        &dx_andn;
  m_jump_table[i_andncc] =      &dx_andncc;
  m_jump_table[i_fba] =         &dx_fba;
  m_jump_table[i_ba] =  &dx_ba;
  m_jump_table[i_fbpa] =        &dx_fbpa;
  m_jump_table[i_bpa] =         &dx_bpa;
  m_jump_table[i_fbn] =         &dx_fbn;
  m_jump_table[i_fbpn] =        &dx_fbpn;
  m_jump_table[i_bn] =  &dx_bn;
  m_jump_table[i_bpn] =         &dx_bpn;
  m_jump_table[i_bpne] =        &dx_bpne;
  m_jump_table[i_bpe] =         &dx_bpe;
  m_jump_table[i_bpg] =         &dx_bpg;
  m_jump_table[i_bple] =        &dx_bple;
  m_jump_table[i_bpge] =        &dx_bpge;
  m_jump_table[i_bpl] =         &dx_bpl;
  m_jump_table[i_bpgu] =        &dx_bpgu;
  m_jump_table[i_bpleu] =       &dx_bpleu;
  m_jump_table[i_bpcc] =        &dx_bpcc;
  m_jump_table[i_bpcs] =        &dx_bpcs;
  m_jump_table[i_bppos] =       &dx_bppos;
  m_jump_table[i_bpneg] =       &dx_bpneg;
  m_jump_table[i_bpvc] =        &dx_bpvc;
  m_jump_table[i_bpvs] =        &dx_bpvs;
  //  m_jump_table[i_call] =    &dx_call;
  //  m_jump_table[i_casa] =    &dx_casa;
  //  m_jump_table[i_casxa] =   &dx_casxa;
  //  m_jump_table[i_done] =    &dx_doneretry;
  //  m_jump_table[i_retry] =   &dx_doneretry;
  //  m_jump_table[i_jmpl] =    &dx_jmpl;
  m_jump_table[i_fabss] =       &dx_fabss;
  m_jump_table[i_fabsd] =       &dx_fabsd;
  m_jump_table[i_fabsq] =       &dx_fabsq;
  m_jump_table[i_fadds] =       &dx_fadds;
  m_jump_table[i_faddd] =       &dx_faddd;
  m_jump_table[i_faddq] =       &dx_faddq;
  m_jump_table[i_fsubs] =       &dx_fsubs;
  m_jump_table[i_fsubd] =       &dx_fsubd;
  m_jump_table[i_fsubq] =       &dx_fsubq;
  m_jump_table[i_fcmps] =       &dx_fcmps;
  m_jump_table[i_fcmpd] =       &dx_fcmpd;
  m_jump_table[i_fcmpq] =       &dx_fcmpq;
  m_jump_table[i_fcmpes] =      &dx_fcmpes;
  m_jump_table[i_fcmped] =      &dx_fcmped;
  m_jump_table[i_fcmpeq] =      &dx_fcmpeq;
  m_jump_table[i_fmovs] =       &dx_fmovs;
  m_jump_table[i_fmovd] =       &dx_fmovd;
  m_jump_table[i_fmovq] =       &dx_fmovq;
  m_jump_table[i_fnegs] =       &dx_fnegs;
  m_jump_table[i_fnegd] =       &dx_fnegd;
  m_jump_table[i_fnegq] =       &dx_fnegq;
  m_jump_table[i_fmuls] =       &dx_fmuls;
  m_jump_table[i_fmuld] =       &dx_fmuld;
  m_jump_table[i_fmulq] =       &dx_fmulq;
  m_jump_table[i_fdivs] =       &dx_fdivs;
  m_jump_table[i_fdivd] =       &dx_fdivd;
  m_jump_table[i_fdivq] =       &dx_fdivq;
  m_jump_table[i_fsmuld] =      &dx_fsmuld;
  m_jump_table[i_fdmulq] =      &dx_fdmulq;
  m_jump_table[i_fsqrts] =      &dx_fsqrts;
  m_jump_table[i_fsqrtd] =      &dx_fsqrtd;
  m_jump_table[i_fsqrtq] =      &dx_fsqrtq;
  //  m_jump_table[i_retrn] =   &dx_retrn;
  m_jump_table[i_brz] =         &dx_brz;
  m_jump_table[i_brlez] =       &dx_brlez;
  m_jump_table[i_brlz] =        &dx_brlz;
  m_jump_table[i_brnz] =        &dx_brnz;
  m_jump_table[i_brgz] =        &dx_brgz;
  m_jump_table[i_brgez] =       &dx_brgez;
  m_jump_table[i_fbu] =         &dx_fbu;
  m_jump_table[i_fbg] =         &dx_fbg;
  m_jump_table[i_fbug] =        &dx_fbug;
  m_jump_table[i_fbl] =         &dx_fbl;
  m_jump_table[i_fbul] =        &dx_fbul;
  m_jump_table[i_fblg] =        &dx_fblg;
  m_jump_table[i_fbne] =        &dx_fbne;
  m_jump_table[i_fbe] =         &dx_fbe;
  m_jump_table[i_fbue] =        &dx_fbue;
  m_jump_table[i_fbge] =        &dx_fbge;
  m_jump_table[i_fbuge] =       &dx_fbuge;
  m_jump_table[i_fble] =        &dx_fble;
  m_jump_table[i_fbule] =       &dx_fbule;
  m_jump_table[i_fbo] =         &dx_fbo;
  m_jump_table[i_fbpu] =        &dx_fbpu;
  m_jump_table[i_fbpg] =        &dx_fbpg;
  m_jump_table[i_fbpug] =       &dx_fbpug;
  m_jump_table[i_fbpl] =        &dx_fbpl;
  m_jump_table[i_fbpul] =       &dx_fbpul;
  m_jump_table[i_fbplg] =       &dx_fbplg;
  m_jump_table[i_fbpne] =       &dx_fbpne;
  m_jump_table[i_fbpe] =        &dx_fbpe;
  m_jump_table[i_fbpue] =       &dx_fbpue;
  m_jump_table[i_fbpge] =       &dx_fbpge;
  m_jump_table[i_fbpuge] =      &dx_fbpuge;
  m_jump_table[i_fbple] =       &dx_fbple;
  m_jump_table[i_fbpule] =      &dx_fbpule;
  m_jump_table[i_fbpo] =        &dx_fbpo;
  m_jump_table[i_bne] =         &dx_bne;
  m_jump_table[i_be] =  &dx_be;
  m_jump_table[i_bg] =  &dx_bg;
  m_jump_table[i_ble] =         &dx_ble;
  m_jump_table[i_bge] =         &dx_bge;
  m_jump_table[i_bl] =  &dx_bl;
  m_jump_table[i_bgu] =         &dx_bgu;
  m_jump_table[i_bleu] =        &dx_bleu;
  m_jump_table[i_bcc] =         &dx_bcc;
  m_jump_table[i_bcs] =         &dx_bcs;
  m_jump_table[i_bpos] =        &dx_bpos;
  m_jump_table[i_bneg] =        &dx_bneg;
  m_jump_table[i_bvc] =         &dx_bvc;
  m_jump_table[i_bvs] =         &dx_bvs;
  m_jump_table[i_fstox] =       &dx_fstox;
  m_jump_table[i_fdtox] =       &dx_fdtox;
  m_jump_table[i_fqtox] =       &dx_fqtox;
  m_jump_table[i_fstoi] =       &dx_fstoi;
  m_jump_table[i_fdtoi] =       &dx_fdtoi;
  m_jump_table[i_fqtoi] =       &dx_fqtoi;
  m_jump_table[i_fstod] =       &dx_fstod;
  m_jump_table[i_fstoq] =       &dx_fstoq;
  m_jump_table[i_fdtos] =       &dx_fdtos;
  m_jump_table[i_fdtoq] =       &dx_fdtoq;
  m_jump_table[i_fqtos] =       &dx_fqtos;
  m_jump_table[i_fqtod] =       &dx_fqtod;
  m_jump_table[i_fxtos] =       &dx_fxtos;
  m_jump_table[i_fxtod] =       &dx_fxtod;
  m_jump_table[i_fxtoq] =       &dx_fxtoq;
  m_jump_table[i_fitos] =       &dx_fitos;
  m_jump_table[i_fitod] =       &dx_fitod;
  m_jump_table[i_fitoq] =       &dx_fitoq;
  m_jump_table[i_fmovfsn] =     &dx_fmovfsn;
  m_jump_table[i_fmovfsne] =    &dx_fmovfsne;
  m_jump_table[i_fmovfslg] =    &dx_fmovfslg;
  m_jump_table[i_fmovfsul] =    &dx_fmovfsul;
  m_jump_table[i_fmovfsl] =     &dx_fmovfsl;
  m_jump_table[i_fmovfsug] =    &dx_fmovfsug;
  m_jump_table[i_fmovfsg] =     &dx_fmovfsg;
  m_jump_table[i_fmovfsu] =     &dx_fmovfsu;
  m_jump_table[i_fmovfsa] =     &dx_fmovfsa;
  m_jump_table[i_fmovfse] =     &dx_fmovfse;
  m_jump_table[i_fmovfsue] =    &dx_fmovfsue;
  m_jump_table[i_fmovfsge] =    &dx_fmovfsge;
  m_jump_table[i_fmovfsuge] =   &dx_fmovfsuge;
  m_jump_table[i_fmovfsle] =    &dx_fmovfsle;
  m_jump_table[i_fmovfsule] =   &dx_fmovfsule;
  m_jump_table[i_fmovfso] =     &dx_fmovfso;
  m_jump_table[i_fmovfdn] =     &dx_fmovfdn;
  m_jump_table[i_fmovfdne] =    &dx_fmovfdne;
  m_jump_table[i_fmovfdlg] =    &dx_fmovfdlg;
  m_jump_table[i_fmovfdul] =    &dx_fmovfdul;
  m_jump_table[i_fmovfdl] =     &dx_fmovfdl;
  m_jump_table[i_fmovfdug] =    &dx_fmovfdug;
  m_jump_table[i_fmovfdg] =     &dx_fmovfdg;
  m_jump_table[i_fmovfdu] =     &dx_fmovfdu;
  m_jump_table[i_fmovfda] =     &dx_fmovfda;
  m_jump_table[i_fmovfde] =     &dx_fmovfde;
  m_jump_table[i_fmovfdue] =    &dx_fmovfdue;
  m_jump_table[i_fmovfdge] =    &dx_fmovfdge;
  m_jump_table[i_fmovfduge] =   &dx_fmovfduge;
  m_jump_table[i_fmovfdle] =    &dx_fmovfdle;
  m_jump_table[i_fmovfdule] =   &dx_fmovfdule;
  m_jump_table[i_fmovfdo] =     &dx_fmovfdo;
  m_jump_table[i_fmovfqn] =     &dx_fmovfqn;
  m_jump_table[i_fmovfqne] =    &dx_fmovfqne;
  m_jump_table[i_fmovfqlg] =    &dx_fmovfqlg;
  m_jump_table[i_fmovfqul] =    &dx_fmovfqul;
  m_jump_table[i_fmovfql] =     &dx_fmovfql;
  m_jump_table[i_fmovfqug] =    &dx_fmovfqug;
  m_jump_table[i_fmovfqg] =     &dx_fmovfqg;
  m_jump_table[i_fmovfqu] =     &dx_fmovfqu;
  m_jump_table[i_fmovfqa] =     &dx_fmovfqa;
  m_jump_table[i_fmovfqe] =     &dx_fmovfqe;
  m_jump_table[i_fmovfque] =    &dx_fmovfque;
  m_jump_table[i_fmovfqge] =    &dx_fmovfqge;
  m_jump_table[i_fmovfquge] =   &dx_fmovfquge;
  m_jump_table[i_fmovfqle] =    &dx_fmovfqle;
  m_jump_table[i_fmovfqule] =   &dx_fmovfqule;
  m_jump_table[i_fmovfqo] =     &dx_fmovfqo;
  m_jump_table[i_fmovsn] =      &dx_fmovsn;
  m_jump_table[i_fmovse] =      &dx_fmovse;
  m_jump_table[i_fmovsle] =     &dx_fmovsle;
  m_jump_table[i_fmovsl] =      &dx_fmovsl;
  m_jump_table[i_fmovsleu] =    &dx_fmovsleu;
  m_jump_table[i_fmovscs] =     &dx_fmovscs;
  m_jump_table[i_fmovsneg] =    &dx_fmovsneg;
  m_jump_table[i_fmovsvs] =     &dx_fmovsvs;
  m_jump_table[i_fmovsa] =      &dx_fmovsa;
  m_jump_table[i_fmovsne] =     &dx_fmovsne;
  m_jump_table[i_fmovsg] =      &dx_fmovsg;
  m_jump_table[i_fmovsge] =     &dx_fmovsge;
  m_jump_table[i_fmovsgu] =     &dx_fmovsgu;
  m_jump_table[i_fmovscc] =     &dx_fmovscc;
  m_jump_table[i_fmovspos] =    &dx_fmovspos;
  m_jump_table[i_fmovsvc] =     &dx_fmovsvc;
  m_jump_table[i_fmovdn] =      &dx_fmovdn;
  m_jump_table[i_fmovde] =      &dx_fmovde;
  m_jump_table[i_fmovdle] =     &dx_fmovdle;
  m_jump_table[i_fmovdl] =      &dx_fmovdl;
  m_jump_table[i_fmovdleu] =    &dx_fmovdleu;
  m_jump_table[i_fmovdcs] =     &dx_fmovdcs;
  m_jump_table[i_fmovdneg] =    &dx_fmovdneg;
  m_jump_table[i_fmovdvs] =     &dx_fmovdvs;
  m_jump_table[i_fmovda] =      &dx_fmovda;
  m_jump_table[i_fmovdne] =     &dx_fmovdne;
  m_jump_table[i_fmovdg] =      &dx_fmovdg;
  m_jump_table[i_fmovdge] =     &dx_fmovdge;
  m_jump_table[i_fmovdgu] =     &dx_fmovdgu;
  m_jump_table[i_fmovdcc] =     &dx_fmovdcc;
  m_jump_table[i_fmovdpos] =    &dx_fmovdpos;
  m_jump_table[i_fmovdvc] =     &dx_fmovdvc;
  m_jump_table[i_fmovqn] =      &dx_fmovqn;
  m_jump_table[i_fmovqe] =      &dx_fmovqe;
  m_jump_table[i_fmovqle] =     &dx_fmovqle;
  m_jump_table[i_fmovql] =      &dx_fmovql;
  m_jump_table[i_fmovqleu] =    &dx_fmovqleu;
  m_jump_table[i_fmovqcs] =     &dx_fmovqcs;
  m_jump_table[i_fmovqneg] =    &dx_fmovqneg;
  m_jump_table[i_fmovqvs] =     &dx_fmovqvs;
  m_jump_table[i_fmovqa] =      &dx_fmovqa;
  m_jump_table[i_fmovqne] =     &dx_fmovqne;
  m_jump_table[i_fmovqg] =      &dx_fmovqg;
  m_jump_table[i_fmovqge] =     &dx_fmovqge;
  m_jump_table[i_fmovqgu] =     &dx_fmovqgu;
  m_jump_table[i_fmovqcc] =     &dx_fmovqcc;
  m_jump_table[i_fmovqpos] =    &dx_fmovqpos;
  m_jump_table[i_fmovqvc] =     &dx_fmovqvc;
  //  m_jump_table[i_fmovrsz] =         &dx_fmovrsz;
  //  m_jump_table[i_fmovrslez] =       &dx_fmovrslez;
  //  m_jump_table[i_fmovrslz] =        &dx_fmovrslz;
  //  m_jump_table[i_fmovrsnz] =        &dx_fmovrsnz;
  //  m_jump_table[i_fmovrsgz] =        &dx_fmovrsgz;
  //  m_jump_table[i_fmovrsgez] =       &dx_fmovrsgez;
  //  m_jump_table[i_fmovrdz] =         &dx_fmovrdz;
  //  m_jump_table[i_fmovrdlez] =       &dx_fmovrdlez;
  //  m_jump_table[i_fmovrdlz] =        &dx_fmovrdlz;
  //  m_jump_table[i_fmovrdnz] =        &dx_fmovrdnz;
  //  m_jump_table[i_fmovrdgz] =        &dx_fmovrdgz;
  //  m_jump_table[i_fmovrdgez] =       &dx_fmovrdgez;
  //  m_jump_table[i_fmovrqz] =         &dx_fmovrqz;
  //  m_jump_table[i_fmovrqlez] =       &dx_fmovrqlez;
  //  m_jump_table[i_fmovrqlz] =        &dx_fmovrqlz;
  //  m_jump_table[i_fmovrqnz] =        &dx_fmovrqnz;
  //  m_jump_table[i_fmovrqgz] =        &dx_fmovrqgz;
  //  m_jump_table[i_fmovrqgez] =       &dx_fmovrqgez;
  m_jump_table[i_mova] =        &dx_mova;
  m_jump_table[i_movfa] =       &dx_movfa;
  m_jump_table[i_movn] =        &dx_movn;
  m_jump_table[i_movfn] =       &dx_movfn;
  m_jump_table[i_movne] =       &dx_movne;
  m_jump_table[i_move] =        &dx_move;
  m_jump_table[i_movg] =        &dx_movg;
  m_jump_table[i_movle] =       &dx_movle;
  m_jump_table[i_movge] =       &dx_movge;
  m_jump_table[i_movl] =        &dx_movl;
  m_jump_table[i_movgu] =       &dx_movgu;
  m_jump_table[i_movleu] =      &dx_movleu;
  m_jump_table[i_movcc] =       &dx_movcc;
  m_jump_table[i_movcs] =       &dx_movcs;
  m_jump_table[i_movpos] =      &dx_movpos;
  m_jump_table[i_movneg] =      &dx_movneg;
  m_jump_table[i_movvc] =       &dx_movvc;
  m_jump_table[i_movvs] =       &dx_movvs;
  m_jump_table[i_movfu] =       &dx_movfu;
  m_jump_table[i_movfg] =       &dx_movfg;
  m_jump_table[i_movfug] =      &dx_movfug;
  m_jump_table[i_movfl] =       &dx_movfl;
  m_jump_table[i_movful] =      &dx_movful;
  m_jump_table[i_movflg] =      &dx_movflg;
  m_jump_table[i_movfne] =      &dx_movfne;
  m_jump_table[i_movfe] =       &dx_movfe;
  m_jump_table[i_movfue] =      &dx_movfue;
  m_jump_table[i_movfge] =      &dx_movfge;
  m_jump_table[i_movfuge] =     &dx_movfuge;
  m_jump_table[i_movfle] =      &dx_movfle;
  m_jump_table[i_movfule] =     &dx_movfule;
  m_jump_table[i_movfo] =       &dx_movfo;
  m_jump_table[i_movrz] =       &dx_movrz;
  m_jump_table[i_movrlez] =     &dx_movrlez;
  m_jump_table[i_movrlz] =      &dx_movrlz;
  m_jump_table[i_movrnz] =      &dx_movrnz;
  m_jump_table[i_movrgz] =      &dx_movrgz;
  m_jump_table[i_movrgez] =     &dx_movrgez;
  //  m_jump_table[i_ta] =      &dx_ta;
  //  m_jump_table[i_tn] =      &dx_tn;
  //  m_jump_table[i_tne] =     &dx_tne;
  //  m_jump_table[i_te] =      &dx_te;
  //  m_jump_table[i_tg] =      &dx_tg;
  //  m_jump_table[i_tle] =     &dx_tle;
  //  m_jump_table[i_tge] =     &dx_tge;
  //  m_jump_table[i_tl] =      &dx_tl;
  //  m_jump_table[i_tgu] =     &dx_tgu;
  //  m_jump_table[i_tleu] =    &dx_tleu;
  //  m_jump_table[i_tcc] =     &dx_tcc;
  //  m_jump_table[i_tcs] =     &dx_tcs;
  //  m_jump_table[i_tpos] =    &dx_tpos;
  //  m_jump_table[i_tneg] =    &dx_tneg;
  //  m_jump_table[i_tvc] =     &dx_tvc;
  //  m_jump_table[i_tvs] =     &dx_tvs;
  m_jump_table[i_sub] =         &dx_sub;
  m_jump_table[i_subcc] =       &dx_subcc;
  m_jump_table[i_subc] =        &dx_subc;
  m_jump_table[i_subccc] =      &dx_subccc;
  m_jump_table[i_or] =  &dx_or;
  m_jump_table[i_orcc] =        &dx_orcc;
  m_jump_table[i_orn] =         &dx_orn;
  m_jump_table[i_orncc] =       &dx_orncc;
  m_jump_table[i_xor] =         &dx_xor;
  m_jump_table[i_xorcc] =       &dx_xorcc;
  m_jump_table[i_xnor] =        &dx_xnor;
  m_jump_table[i_xnorcc] =      &dx_xnorcc;
  m_jump_table[i_mulx] =        &dx_mulx;
  m_jump_table[i_sdivx] =       &dx_sdivx;
  m_jump_table[i_udivx] =       &dx_udivx;
  m_jump_table[i_sll] =         &dx_sll;
  m_jump_table[i_srl] =         &dx_srl;
  m_jump_table[i_sra] =         &dx_sra;
  m_jump_table[i_sllx] =        &dx_sllx;
  m_jump_table[i_srlx] =        &dx_srlx;
  m_jump_table[i_srax] =        &dx_srax;
  m_jump_table[i_taddcc] =      &dx_taddcc;
  m_jump_table[i_taddcctv] =    &dx_taddcctv;
  m_jump_table[i_tsubcc] =      &dx_tsubcc;
  m_jump_table[i_tsubcctv] =    &dx_tsubcctv;
  //  m_jump_table[i_udiv] =    &dx_udiv;
  //  m_jump_table[i_sdiv] =    &dx_sdiv;
  //  m_jump_table[i_umul] =    &dx_umul;
  //  m_jump_table[i_smul] =    &dx_smul;
  //  m_jump_table[i_udivcc] =  &dx_udivcc;
  //  m_jump_table[i_sdivcc] =  &dx_sdivcc;
  //  m_jump_table[i_umulcc] =  &dx_umulcc;
  //  m_jump_table[i_smulcc] =  &dx_smulcc;
  //  m_jump_table[i_mulscc] =  &dx_mulscc;
  m_jump_table[i_popc] =        &dx_popc;
  m_jump_table[i_flush] =       &dx_flush;
  //  m_jump_table[i_flushw] =  &dx_flushw;
  //  m_jump_table[i_rd] =      &dx_rd;
  m_jump_table[i_rdcc] =        &dx_rdcc;
  //  m_jump_table[i_wr] =      &dx_wr;
  m_jump_table[i_wrcc] =        &dx_wrcc;
  //  m_jump_table[i_save] =    &dx_save;
  //  m_jump_table[i_restore] =         &dx_restore;
  //  m_jump_table[i_saved] =   &dx_saved;
  //  m_jump_table[i_restored] =        &dx_restored;
  m_jump_table[i_sethi] =       &dx_sethi;
  m_jump_table[i_ldf] =         &dx_ldf;
  m_jump_table[i_lddf] =        &dx_lddf;
  m_jump_table[i_ldqf] =        &dx_ldqf;
  m_jump_table[i_ldfa] =        &dx_ldfa;
  //  m_jump_table[i_lddfa] =   &dx_lddfa;
  m_jump_table[i_ldblk] =       &dx_ldblk;
  m_jump_table[i_ldqfa] =       &dx_ldqfa;
  m_jump_table[i_ldsb] =        &dx_ldsb;
  m_jump_table[i_ldsh] =        &dx_ldsh;
  m_jump_table[i_ldsw] =        &dx_ldsw;
  m_jump_table[i_ldub] =        &dx_ldub;
  m_jump_table[i_lduh] =        &dx_lduh;
  m_jump_table[i_lduw] =        &dx_lduw;
  m_jump_table[i_ldx] =         &dx_ldx;
  //  m_jump_table[i_ldd] =     &dx_ldd;
  //  m_jump_table[i_ldsba] =   &dx_ldsba;
  m_jump_table[i_ldsha] =       &dx_ldsha;
  m_jump_table[i_ldswa] =       &dx_ldswa;
  m_jump_table[i_lduba] =       &dx_lduba;
  m_jump_table[i_lduha] =       &dx_lduha;
  m_jump_table[i_lduwa] =       &dx_lduwa;
  m_jump_table[i_ldxa] =        &dx_ldxa;
  //  m_jump_table[i_ldda] =    &dx_ldda;
  //  m_jump_table[i_ldqa] =    &dx_ldqa;
  m_jump_table[i_stf] =         &dx_stf;
  m_jump_table[i_stdf] =        &dx_stdf;
  m_jump_table[i_stqf] =        &dx_stqf;
  m_jump_table[i_stb] =         &dx_stb;
  m_jump_table[i_sth] =         &dx_sth;
  m_jump_table[i_stw] =         &dx_stw;
  m_jump_table[i_stx] =         &dx_stx;
  m_jump_table[i_std] =         &dx_std;
  m_jump_table[i_stfa] =        &dx_stfa;
  m_jump_table[i_stdfa] =       &dx_stdfa;
  m_jump_table[i_stblk] =       &dx_stblk;
  m_jump_table[i_stqfa] =       &dx_stqfa;
  m_jump_table[i_stba] =        &dx_stba;
  m_jump_table[i_stha] =        &dx_stha;
  m_jump_table[i_stwa] =        &dx_stwa;
  m_jump_table[i_stxa] =        &dx_stxa;
  m_jump_table[i_stda] =        &dx_stda;
  m_jump_table[i_ldstub] =      &dx_ldstub;
  m_jump_table[i_ldstuba] =     &dx_ldstuba;
  m_jump_table[i_prefetch] =    &dx_prefetch;
  m_jump_table[i_prefetcha] =   &dx_prefetcha;
  //  m_jump_table[i_swap] =    &dx_swap;
  //  m_jump_table[i_swapa] =   &dx_swapa;
  //  m_jump_table[i_ldfsr] =   &dx_ldfsr;
  //  m_jump_table[i_ldxfsr] =  &dx_ldxfsr;
  m_jump_table[i_stfsr] =       &dx_stfsr;
  m_jump_table[i_stxfsr] =      &dx_stxfsr;
  m_jump_table[i__trap] =       &dx__trap;
  m_jump_table[i_impdep1] =     &dx_impdep1;
  m_jump_table[i_impdep2] =     &dx_impdep2;
  m_jump_table[i_membar] =      &dx_membar;
  m_jump_table[i_stbar] =       &dx_stbar;
  m_jump_table[i_cmp] =         &dx_cmp;
  //  m_jump_table[i_jmp] =     &dx_jmp;
  m_jump_table[i_mov] =         &dx_mov;
  m_jump_table[i_nop] =         &dx_nop;
  m_jump_table[i_not] =         &dx_not;
  //  m_jump_table[i_rdpr] =    &dx_rdpr;
  //  m_jump_table[i_wrpr] =    &dx_wrpr;
  //  m_jump_table[i_faligndata] =      &dx_faligndata;
  //  m_jump_table[i_alignaddr] =       &dx_alignaddr;
  //  m_jump_table[i_alignaddrl] =      &dx_alignaddrl;
  //  m_jump_table[i_fzero] =   &dx_fzero;
  //  m_jump_table[i_fzeros] =  &dx_fzeros;
  //  m_jump_table[i_fsrc1] =   &dx_fsrc1;
  m_jump_table[i_fcmple16] =    &dx_fcmple16;
  m_jump_table[i_fcmpne16] =    &dx_fcmpne16;
  m_jump_table[i_fcmple32] =    &dx_fcmple32;
  m_jump_table[i_fcmpne32] =    &dx_fcmpne32;
  m_jump_table[i_fcmpgt16] =    &dx_fcmpgt16;
  m_jump_table[i_fcmpeq16] =    &dx_fcmpeq16;
  m_jump_table[i_fcmpgt32] =    &dx_fcmpgt32;
  m_jump_table[i_fcmpeq32] =    &dx_fcmpeq32;
  //  m_jump_table[i_bshuffle] =        &dx_bshuffle;
  //  m_jump_table[i_bmask] =   &dx_bmask;
  m_jump_table[i_ill] =         &dx_ill;
}

/*------------------------------------------------------------------------*/
/* Global functions                                                       */
/*------------------------------------------------------------------------*/
