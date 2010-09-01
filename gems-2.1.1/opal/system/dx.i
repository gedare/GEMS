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
 * FileName:  dx.i
 * Synopsis:  dynamic execution implementation
 * Author:    cmauer
 * Version:   $Id$
 */

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

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
// for WATTCH power
//***************************************************************************

// To track power when wrtting physical bits to regfile or on result bus
#ifdef DYNAMIC_AF
#define WATTCH_WRITE_DEST( A )                            \
     if(WATTCH_POWER){   \
       m_pseq->getPowerStats()->incrementRegfileTotalPopCount(m_pseq->getPowerStats()->pop_count(A)); \
       m_pseq->getPowerStats()->incrementRegfileNumPopCount();     \
       m_pseq->getPowerStats()->incrementResultBusTotalPopCount(m_pseq->getPowerStats()->pop_count(A));       \
        m_pseq->getPowerStats()->incrementResultBusNumPopCount();  \
    }
 
#endif

//don't do anything if using STATIC_AF
#ifdef STATIC_AF
#define WATTCH_WRITE_DEST( A )       \
           ;
#endif

// To track power when  trasferring a register value stored in Instruction window to Functional unit
#define WATTCH_READ_SOURCE \
      if(WATTCH_POWER){         \
          m_pseq->getPowerStats()->incrementWinPregAccess(); \
     }
      
//***************************************************************************
#define WRITE_DEST(A)         \
    getDestReg(0).getARF()->writeInt64( getDestReg(0), (A), getProc() );    \
    if( getStaticInst()->getFlag( SI_WRITE_CONTROL) ) { \
       WRITE_FPRS;          \
    }  

//***************************************************************************
#define WRITE_DEST2(A)         \
    getDestReg(1).getARF()->writeInt64( getDestReg(1), (A), getProc () )
//***************************************************************************
#define WRITE_DEST_S(A) \
    getDestReg(0).getARF()->writeFloat32( getDestReg(0), (A), getProc () ); \
    if( getStaticInst()->getFlag( SI_WRITE_CONTROL) ){ \
       WRITE_FPRS;          \
    }  
//***************************************************************************
#define WRITE_DEST_D(A) \
    getDestReg(0).getARF()->writeFloat64( getDestReg(0), (A), getProc() );  \
    if( getStaticInst()->getFlag( SI_WRITE_CONTROL) ){ \
       WRITE_FPRS;          \
    }  
//***************************************************************************
#define WRITE_DEST_DI(A) \
    getDestReg(0).getARF()->writeInt64( getDestReg(0), (A), getProc() );  \
   if( getStaticInst()->getFlag( SI_WRITE_CONTROL) ){ \
       WRITE_FPRS;          \
    }  
//***************************************************************************
#define WRITE_CC(A)           \
    getDestReg(1).getARF()->writeInt64( getDestReg(1), (A), getProc() )

//***************************************************************************
#define SOURCE1    getSourceReg(0).getARF()->readInt64( getSourceReg(0), getProc() )

//***************************************************************************
#define SOURCE1_S  getSourceReg(0).getARF()->readFloat32( getSourceReg(0), getProc() )

//***************************************************************************
#define SOURCE1_D  getSourceReg(0).getARF()->readFloat64( getSourceReg(0), getProc() )

//***************************************************************************
#define SOURCE1_DI getSourceReg(0).getARF()->readInt64( getSourceReg(0), getProc() )


//***************************************************************************
#define SOURCE2    getSourceReg(1).getARF()->readInt64( getSourceReg(1), getProc() )

//***************************************************************************
#define SOURCE2_S  getSourceReg(1).getARF()->readFloat32( getSourceReg(1), getProc() )

//***************************************************************************
#define SOURCE2_D  getSourceReg(1).getARF()->readFloat64( getSourceReg(1), getProc() )

//***************************************************************************
#define SOURCE2_DI getSourceReg(1).getARF()->readInt64( getSourceReg(1), getProc() )


//***************************************************************************
#define SOURCE3    getSourceReg(2).getARF()->readInt64( getSourceReg(2), getProc() )


// Source 2 or immediate constant
//***************************************************************************
#define S2ORI     (getStaticInst()->getFlag(SI_ISIMMEDIATE) ? \
                   getStaticInst()->getImmediate() :   \
		   SOURCE2)

//***************************************************************************
#define READ_SELECT_CONTROL( REG, A )            \
  getSourceReg(3).setVanilla( REG );             \
  A = getSourceReg(3).getARF()->readInt64( getSourceReg(3), getProc() );  \
  WATTCH_READ_SOURCE

//***************************************************************************
#define SETCTL_SOURCE( REG )                     \
  getSourceReg(3).setVanilla( REG );

//***************************************************************************
#define GETCTL_SOURCE                            \
  getSourceReg(3).getVanilla( )

//***************************************************************************
#define SOURCE_CTL                               \
  getSourceReg(3).getARF()->readInt64( getSourceReg(3), getProc() )


//***************************************************************************
#define INIT_CONTROL                               \
  if ( getDestReg(1).getRtype() != RID_CONTROL ) { \
    ERROR_OUT( "dx: INIT_CONTROL called with type=%d\n", getDestReg(1).getRtype() ); \
  } else { \
    getDestReg(1).getARF()->initializeControl( getDestReg(1), getProc() ); \
  }

//***************************************************************************
#define SETCTL_DEST( REG )                       \
  getDestReg(1).setVanilla( REG )
//***************************************************************************
#define WRITE_CONTROL(A)                           \
  getDestReg(1).getARF()->writeInt64( getDestReg(1), A , getProc() );
//***************************************************************************
#define WRITE_SELECT_CONTROL(REG, A)               \
  getDestReg(1).setVanilla( REG );                 \
  getDestReg(1).getARF()->writeInt64( getDestReg(1), A , getProc() );
//***************************************************************************
#define FINALIZE_CONTROL                          \
  getDestReg(1).getARF()->finalizeControl( getDestReg(1), getProc() )

//***************************************************************************
// used to update the CONTROL_FPRS register for FP instructions
#define WRITE_FPRS   \
  IREGISTER fprs;      \
  READ_SELECT_CONTROL( CONTROL_FPRS, fprs );   \
  if( getStaticInst()->getFlag( SI_WRITE_LOWER_FP )){   \
    INIT_CONTROL;           \
    WRITE_SELECT_CONTROL( CONTROL_FPRS, fprs | 0x1 );   \
    FINALIZE_CONTROL;   \
  }                       \
  else if( getStaticInst()->getFlag( SI_WRITE_UPPER_FP)){  \
    INIT_CONTROL;           \
    WRITE_SELECT_CONTROL( CONTROL_FPRS, fprs | 0x2 );   \
    FINALIZE_CONTROL;   \
  }            


//***************************************************************************
#define READ_CC     \
  (SOURCE3)

//***************************************************************************
#define COND_CODE_N \
  ((SOURCE3 >> (3 + getStaticInst()->getCCShift())) & 0x1)

//***************************************************************************
#define COND_CODE_Z \
  ((SOURCE3 >> (2 + getStaticInst()->getCCShift())) & 0x1)

//***************************************************************************
#define COND_CODE_V \
  ((SOURCE3 >> (1 + getStaticInst()->getCCShift())) & 0x1)

//***************************************************************************
#define COND_CODE_C \
  ((SOURCE3 >> (0 + getStaticInst()->getCCShift())) & 0x1)

//***************************************************************************
#define ICC_C       \
  ((SOURCE3) & 0x1)

//***************************************************************************
#define COND_CODE_U \
  (SOURCE3 == 3)

//***************************************************************************
#define COND_CODE_G \
  (SOURCE3 == 2)

//***************************************************************************
#define COND_CODE_L \
  (SOURCE3 == 1)

//***************************************************************************
#define COND_CODE_E \
  (SOURCE3 == 0)

//***************************************************************************
#define  NOT_IMPLEMENTED               \
  setTrapType( Trap_Unimplemented );

//***************************************************************************
#define  USE_FUNCTIONAL               \
  setTrapType( Trap_Use_Functional );

/*------------------------------------------------------------------------*/
/* Compare / Move Declarations                                            */
/*------------------------------------------------------------------------*/

//***************************************************************************
#define FCMP( FSDQ )                                          \
  WATTCH_READ_SOURCE                         \
  WATTCH_READ_SOURCE                         \
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
  WATTCH_WRITE_DEST( result )             \
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
  WATTCH_WRITE_DEST( status_reg );          \
  WRITE_SELECT_CONTROL( CONTROL_FSR, status_reg );
#endif

//***************************************************************************
#define MOVECC( COND )          \
  WATTCH_READ_SOURCE     \
  if ( (COND) ) {                                                 \
    WATTCH_WRITE_DEST( S2ORI );                \
    WRITE_DEST( S2ORI );                                          \
  } else {                                                        \
    WATTCH_WRITE_DEST( SOURCE1 );               \
    WRITE_DEST( SOURCE1 );                                        \
  }

//*************************************************************************** 
#define MOVECC_F MOVECC

//***************************************************************************
#define MOVER( CMP )                                 \
  WATTCH_READ_SOURCE    \
  WATTCH_READ_SOURCE    \
  if ( ((int64) SOURCE3) CMP 0 ) {                   \
    WATTCH_WRITE_DEST( S2ORI );            \
    WRITE_DEST( S2ORI );                             \
  } else {                                           \
    WATTCH_WRITE_DEST( SOURCE1 );            \
    WRITE_DEST( SOURCE1 );                           \
  }

//***************************************************************************
#define FMOVER( FSDQ, CMP )                                   \
  WATTCH_READ_SOURCE            \
  WATTCH_READ_SOURCE            \
  if ( ((int64) SOURCE3) CMP 0 ) {                            \
    WATTCH_WRITE_DEST( SOURCE2 );            \
    WRITE_DEST_ ## FSDQ ( SOURCE2_ ## FSDQ );           \
  } else {                                                    \
    WATTCH_WRITE_DEST( SOURCE1 );            \
    WRITE_DEST_ ## FSDQ ( SOURCE1_ ## FSDQ );           \
  }

//***************************************************************************
#define FMOVECC( FSDQ, COND )                                 \
  WATTCH_READ_SOURCE                       \
  if ( (COND) ) {                                             \
    WATTCH_WRITE_DEST( SOURCE2 );            \
    WRITE_DEST_ ## FSDQ ( SOURCE2_ ## FSDQ );           \
  } else {                                                    \
    WATTCH_WRITE_DEST( SOURCE1 );            \
    WRITE_DEST_ ## FSDQ ( SOURCE1_ ## FSDQ );           \
  }

//***************************************************************************
#define MOVE16( CMP )                                         \
  IREGISTER src1 = SOURCE1;                                   \
  IREGISTER src2 = SOURCE2;                                   \
  IREGISTER result = 0;                                       \
  WATTCH_READ_SOURCE                               \
  WATTCH_READ_SOURCE                               \
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
  }         \
  WATTCH_WRITE_DEST( result );            \
  WRITE_DEST( result );

//***************************************************************************
#define MOVE32( CMP )                                         \
  IREGISTER src1 = SOURCE1;                                   \
  IREGISTER src2 = SOURCE2;                                   \
  IREGISTER result = 0;                                       \
  WATTCH_READ_SOURCE                               \
  WATTCH_READ_SOURCE                               \
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
  WATTCH_READ_SOURCE  \
  WATTCH_READ_SOURCE  \
  WATTCH_WRITE_DEST( ((TCAST) SOURCE1) OP S2ORI );            \
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

#define BRANCH_F           BRANCH
#define BRANCH_FP          BRANCH
#define BRANCH_P           BRANCH
#define BRANCH_UNCOND_P    BRANCH_UNCOND

// Note: All these _BRANCH() methods assume the the m_actual.pc & .npc 
// (of the actual target) are set in the constructor to the current PC&NPC.
//***************************************************************************
#define BRANCH( COND )               \
  CONTROL_OP_TYPE *control_inst = CONTROL_OP_GET;          \
  control_inst->setTaken( COND );                          \
  abstract_pc_t *actual = control_inst->getActualTarget(); \
  getStaticInst()->actualPC_branch( COND, actual );      


//***************************************************************************
#define BRANCH_UNCOND( COND )        \
  CONTROL_OP_TYPE *control_inst = CONTROL_OP_GET;          \
  control_inst->setTaken( COND );                          \
  abstract_pc_t *actual = control_inst->getActualTarget(); \
  getStaticInst()->actualPC_uncond_branch( COND, actual );    

//***************************************************************************
#define BRANCH_INDIRECT( DEST )      \
  CONTROL_OP_TYPE *control_inst = CONTROL_OP_GET;          \
  control_inst->setTaken( true );                          \
  abstract_pc_t *actual = control_inst->getActualTarget(); \
  getStaticInst()->actualPC_indirect( DEST, actual );        

//***************************************************************************
#define BRANCH_REGISTER( OP ) \
  WATTCH_READ_SOURCE               \
  bool  taken = (((int64) SOURCE1) OP 0);                  \
  CONTROL_OP_TYPE *control_inst = CONTROL_OP_GET;          \
  control_inst->setTaken( taken );                          \
  abstract_pc_t *actual = control_inst->getActualTarget(); \
  getStaticInst()->actualPC_branch( taken, actual );           

//***************************************************************************
#define BRANCH_TRAP_RETURN( TPC, TNPC, PSTATE, TL, CWP )   \
  CONTROL_OP_TYPE *control_inst = CONTROL_OP_GET;          \
  control_inst->setTaken( true );                          \
  abstract_pc_t *actual = control_inst->getActualTarget(); \
  getStaticInst()->actualPC_trap_return( actual, tpc, tnpc, pstate, tl, cwp );   

//***************************************************************************
#define BRANCH_CWP            \
  CONTROL_OP_TYPE *control_inst = CONTROL_OP_GET;          \
  abstract_pc_t *actual = control_inst->getActualTarget(); \
  getStaticInst()->actualPC_cwp( actual );                  

//***************************************************************************
#define BRANCH_EXECUTE                                       \
  {                                                          \
    CONTROL_OP_TYPE *control_inst = CONTROL_OP_GET;          \
    abstract_pc_t *actual = control_inst->getActualTarget(); \
    getStaticInst()->actualPC_execute( actual );             \
  }

//***************************************************************************
#define BRANCH_PRIV( PSTATE, TL, CWP )                     \
  CONTROL_OP_TYPE *control_inst = CONTROL_OP_GET;          \
  abstract_pc_t *actual = control_inst->getActualTarget(); \
  actual->pstate = pstate;                                 \
  actual->tl     = tl;                                     \
  actual->cwp    = cwp;                                    \
  actual->gset   = pstate_t::getGlobalSet( pstate );  

//***************************************************************************
#define BRANCH_TRAP( COND )                \
  if (COND) {          \
    WATTCH_READ_SOURCE                     \
    WATTCH_READ_SOURCE                     \
    IREGISTER trapnum = SOURCE1 + S2ORI;   \
    trapnum = 256 + (trapnum & 0x7f);      \
    setTrapType( (trap_type_t) trapnum );  \
  } else {                                 \
    CONTROL_OP_TYPE *control_inst = CONTROL_OP_GET;          \
    abstract_pc_t *actual = control_inst->getActualTarget(); \
    getStaticInst()->actualPC_execute( actual );             \
  }                                                                          

/*------------------------------------------------------------------------*/
/* Memory Declarations                                                    */
/*------------------------------------------------------------------------*/

//***************************************************************************
#define ISADDRESS32( PSTATE ) \
  ( ((PSTATE) & 0x8) == 0x8 )

//***************************************************************************
#define AM64( address )       \
  ( (int32) address & 0xffffffff )

//***************************************************************************
#define STORE_INSTRUCTION \
  if ( getTrapType() == Trap_Unimplemented ) { \
     setTrapType( Trap_NoTrap );               \
  }

// NOTE: can NOT use read_memory for sizes larger than word accesses
//***************************************************************************
#define  READ_MEMORY                                                  \
  MEMORY_OP_TYPE *memop = MEMORY_OP_GET;                              \
  if (memop->isDataValid()) {                                         \
    STAT_INC( m_pseq->m_stat_loads_found[m_proc] );                           \
    WATTCH_WRITE_DEST( memop->getSignedData() );            \
    WRITE_DEST( memop->getSignedData() );                             \
  } else {                                                            \
    STAT_INC( m_pseq->m_stat_loads_notfound[m_proc] );                        \
    USE_FUNCTIONAL;                                                   \
  }

//***************************************************************************
#define  READ_MEMORY_UNSIGNED                                         \
  MEMORY_OP_TYPE *memop = MEMORY_OP_GET;                              \
  if (memop->isDataValid()) {                                         \
    STAT_INC( m_pseq->m_stat_loads_found[m_proc] );                           \
    WATTCH_WRITE_DEST( memop->getUnsignedData() );            \
    WRITE_DEST( memop->getUnsignedData() );                           \
  } else {                                                            \
    STAT_INC( m_pseq->m_stat_loads_notfound[m_proc] );                        \
    USE_FUNCTIONAL;                                                   \
  }

//***************************************************************************
#define  READ_MEMORY_BLOCK                                            \
  MEMORY_OP_TYPE *memop = MEMORY_OP_GET;                              \
  if (memop->isDataValid()) {                                         \
    STAT_INC( m_pseq->m_stat_loads_found[m_proc] );                           \
    int offset = getDestReg(0).getSelector();                         \
    if (memop->getAccessSize() == 64) {                               \
      IREGISTER *data_p = memop->getData();                             \
      for (int i = 0; i < 8; i++) {                                     \
        getDestReg(0).setSelector( i );                                 \
        getDestReg(0).getARF()->writeInt64( getDestReg(0), data_p[i], m_proc ); \
      }                                                                 \
      if( getStaticInst()->getFlag( SI_WRITE_CONTROL) ) { \
       WRITE_FPRS;          \
      }            \
    } else if (memop->getAccessSize() <= 8) {                           \
      IREGISTER data = memop->getUnsignedData();                        \
      getDestReg(0).setSelector( 0 );                                   \
      getDestReg(0).getARF()->writeInt64( getDestReg(0), data, m_proc );        \
      WATTCH_READ_SOURCE                 \
      for (int i = 1; i < 8; i++) {                                     \
        getSourceReg(2).setSelector( i );                               \
        getDestReg(0).setSelector( i );                                 \
        getDestReg(0).getARF()->writeInt64( getDestReg(0), SOURCE3, m_proc );   \
      }                                                                 \
      if( getStaticInst()->getFlag( SI_WRITE_CONTROL) ) { \
       WRITE_FPRS;          \
      }                              \
    } else {                                                            \
      ERROR_OUT("error: READ MEMORY BLOCK invalid size = %d\n",         \
		memop->getAccessSize() );                               \
      getDestReg(0).setSelector( 0 );                                   \
      getDestReg(0).getARF()->writeInt64( getDestReg(0), 0, m_proc );           \
      if( getStaticInst()->getFlag( SI_WRITE_CONTROL) ) { \
       WRITE_FPRS;          \
      }                             \
    }                                                                 \
    /* restore the default selector */                                \
    getDestReg(0).setSelector( offset );                              \
  } else {                                                            \
    STAT_INC( m_pseq->m_stat_loads_notfound[m_proc] );                        \
    NOT_IMPLEMENTED;                                                  \
  }

//***************************************************************************
#define WRITE_MEMORY( VALUE )                                         \
  STORE_OP_TYPE *storeop = STORE_OP_GET;                              \
  storeop->writeValue( VALUE );

#define DX_RETURNT void dynamic_inst_t::
/*------------------------------------------------------------------------*/
/* Global functions                                                       */
/*------------------------------------------------------------------------*/

//***************************************************************************

DX_RETURNT dx_add( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  WATTCH_READ_SOURCE;
  IREGISTER result = (SOURCE1 + S2ORI);
  //ED  m_pseq->out_info("   source1 0x%0llx\n", (SOURCE1));
  //ED  m_pseq->out_info("   source2 0x%0llx\n",  S2ORI);
  //ED  m_pseq->out_info("   result == 0x%0llx\n", result );
  WATTCH_WRITE_DEST(result);            
  WRITE_DEST(result);
}

DX_RETURNT dx_addcc( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  WATTCH_READ_SOURCE;
  unsigned char ccodes;
  // CC_MODIFICATION: changes condition codes
  // uadd64 and sadd64 map to same function call
  IREGISTER result = us_addcc( SOURCE1, S2ORI, &ccodes );
  //m_pseq->out_info(" addcc -- %d\n", ccodes);
  WRITE_CC( ccodes );
  WATTCH_WRITE_DEST( result );            
  WRITE_DEST(result);
}

DX_RETURNT dx_addc( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  WATTCH_READ_SOURCE;
  WATTCH_READ_SOURCE;
  IREGISTER result = (SOURCE1 + S2ORI + ICC_C);
  WATTCH_WRITE_DEST( result );            
  WRITE_DEST(result);
}

DX_RETURNT dx_addccc( DX_PLIST ) {
  unsigned char ccodes;
  // CC_MODIFICATION: changes condition codes
  WATTCH_READ_SOURCE;
  WATTCH_READ_SOURCE;
  WATTCH_READ_SOURCE;
  IREGISTER src_2  = S2ORI + ICC_C;
  IREGISTER result = us_addcc( SOURCE1, src_2, &ccodes );
  WRITE_CC( ccodes );
  WATTCH_WRITE_DEST( result );            
  WRITE_DEST(result);
}

DX_RETURNT dx_and( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  WATTCH_READ_SOURCE;
  IREGISTER result = (SOURCE1 & S2ORI);
  WATTCH_WRITE_DEST( result );            
  WRITE_DEST(result);
}

DX_RETURNT dx_andcc( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  WATTCH_READ_SOURCE;
  unsigned char ccodes;
  // CC_MODIFICATION: changes condition codes
  IREGISTER result = us_andcc( SOURCE1, S2ORI, &ccodes );
  WRITE_CC( ccodes );
  // IREGISTER res2 = SOURCE1 & S2ORI;
  WATTCH_WRITE_DEST( result );            
  WRITE_DEST(result);
}

DX_RETURNT dx_andn( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  WATTCH_READ_SOURCE;
  IREGISTER result = (SOURCE1 & ~S2ORI);
  WATTCH_WRITE_DEST( result );            
  WRITE_DEST(result);
}

DX_RETURNT dx_andncc( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  WATTCH_READ_SOURCE;
  unsigned char ccodes;
  // CC_MODIFICATION: changes condition codes
  IREGISTER result = us_andcc( SOURCE1, ~S2ORI, &ccodes );
  WRITE_CC( ccodes );
  WATTCH_WRITE_DEST( result );            
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
  WATTCH_READ_SOURCE;
  BRANCH_P( !COND_CODE_Z );
}

DX_RETURNT dx_bpe( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  BRANCH_P( COND_CODE_Z );
}

DX_RETURNT dx_bpg( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  BRANCH_P( !( COND_CODE_Z | (COND_CODE_N ^ COND_CODE_V) ) );
}

DX_RETURNT dx_bple( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  BRANCH_P( ( COND_CODE_Z | (COND_CODE_N ^ COND_CODE_V) ) );
}

DX_RETURNT dx_bpge( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  BRANCH_P( !(COND_CODE_N ^ COND_CODE_V) );
}

DX_RETURNT dx_bpl( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  BRANCH_P( (COND_CODE_N ^ COND_CODE_V) );
}

DX_RETURNT dx_bpgu( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  BRANCH_P( !(COND_CODE_C | COND_CODE_Z) );
}

DX_RETURNT dx_bpleu( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  BRANCH_P( (COND_CODE_C | COND_CODE_Z) );
}

DX_RETURNT dx_bpcc( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  BRANCH_P( !COND_CODE_C );
}

DX_RETURNT dx_bpcs( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  BRANCH_P( COND_CODE_C );
}

DX_RETURNT dx_bppos( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  BRANCH_P( !COND_CODE_N );
}

DX_RETURNT dx_bpneg( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  BRANCH_P( COND_CODE_N );
}

DX_RETURNT dx_bpvc( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  BRANCH_P( !COND_CODE_V );
}

DX_RETURNT dx_bpvs( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  BRANCH_P( COND_CODE_V );
}

DX_RETURNT dx_call( DX_PLIST ) {

  // set r[15] is always the dest of the call (set r[15] to the current PC)
  WATTCH_WRITE_DEST( getVPC() );            
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

DX_RETURNT dx_casa( DX_PLIST ) {
  // read word from memory (already done in atomic_inst_t)
  MEMORY_OP_TYPE *memop = MEMORY_OP_GET;

  if (memop->isDataValid()) {
    IREGISTER data = memop->getUnsignedData();

    // compare with rs2
#if 0
    m_pseq->out_info("asi     0x%0x\n", memop->getASI());
    m_pseq->out_info("source1 0x%0llx\n", (SOURCE1 & MEM_WORD_MASK));
    m_pseq->out_info("source2 0x%0llx\n", (SOURCE2 & MEM_WORD_MASK));
    m_pseq->out_info("data    0x%0llx\n", data );
#endif
    if ( data == (SOURCE2 & MEM_WORD_MASK) ) {
      WATTCH_READ_SOURCE;
      WATTCH_READ_SOURCE;
      // if equal, write rd to memory ... copy rs2 to rd
        WATTCH_WRITE_DEST( (SOURCE2 & MEM_WORD_MASK) );            
      WRITE_DEST( (SOURCE2 & MEM_WORD_MASK) );
      // rd (the destination) is also source3
      WRITE_MEMORY( (SOURCE3 & MEM_WORD_MASK) );
    } else {
      WATTCH_READ_SOURCE;
      // else not equal, copy UPPER 32 bits from memory to rd
      WATTCH_WRITE_DEST( data );            
      WRITE_DEST( data );
    }
  } else {
    NOT_IMPLEMENTED;
  }
}

DX_RETURNT dx_casxa( DX_PLIST ) {
  // read word from memory (already done in atomic_inst_t)
  MEMORY_OP_TYPE *memop = MEMORY_OP_GET;

  if (memop->isDataValid()) {
    IREGISTER data = memop->getUnsignedData();
    // compare with rs2
    if ( data == SOURCE2 ) {
      WATTCH_READ_SOURCE;
      WATTCH_READ_SOURCE;
      // if equal, write rd to memory ... copy rs2 to rd
      WATTCH_WRITE_DEST( SOURCE2 );            
      WRITE_DEST( SOURCE2 );
      // rd (the destination) is also source3
      WRITE_MEMORY( SOURCE3 );
    } else {
      WATTCH_READ_SOURCE;
      // else not equal, copy 64-bits from memory to rd
      WATTCH_WRITE_DEST( data );            
      WRITE_DEST( data );
    }
  } else {
    NOT_IMPLEMENTED;
  }
}

DX_RETURNT dx_jmpl( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  WATTCH_READ_SOURCE;

  IREGISTER dest = (int64) SOURCE1 + (int64) S2ORI;

  //DEBUG_OUT("dx.i: dx_jmpl  SOURCE1[ 0x%llx ] S2ORI[ 0x%llx ] dest[ 0x%llx ]\n", SOURCE1, S2ORI, dest);

  // CM AddressMask64 AM64
  IREGISTER pstate;
  READ_SELECT_CONTROL( CONTROL_PSTATE, pstate );
  if ( ISADDRESS32(pstate) ) {
    dest = AM64(dest);
  }
  
#if 0
  m_pseq->out_info("jmpl pstate: 0x%0x\n", pstate);
  m_pseq->out_info("jmpl s1: 0x%0llx\n", SOURCE1);
  m_pseq->out_info("jmpl s2: 0x%0llx\n", S2ORI);
  m_pseq->out_info("jmpl d : 0x%0llx\n", dest);
#endif
  WATTCH_WRITE_DEST( getVPC() );            
  WRITE_DEST( getVPC() );
  BRANCH_INDIRECT( dest );
}

DX_RETURNT dx_fabss( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  float d = SOURCE2_S;
  d = fabs( d );
  WATTCH_WRITE_DEST( d );            
  WRITE_DEST_S( d );
}

DX_RETURNT dx_fabsd( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  WATTCH_WRITE_DEST( fabs(SOURCE2_D) );            
  WRITE_DEST_D( fabs(SOURCE2_D) );
}

DX_RETURNT dx_fabsq( DX_PLIST ) {
  NOT_IMPLEMENTED;
  // WRITE_DEST_Q( fabs(SOURCE2_Q) );
}

DX_RETURNT dx_fadds( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  WATTCH_READ_SOURCE;
  WATTCH_WRITE_DEST( SOURCE1_S + SOURCE2_S );            
  WRITE_DEST_S( SOURCE1_S + SOURCE2_S );
  //UPDATE_FSR;
}

DX_RETURNT dx_faddd( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  WATTCH_READ_SOURCE;
  WATTCH_WRITE_DEST( SOURCE1_D + SOURCE2_D );            
  WRITE_DEST_D( SOURCE1_D + SOURCE2_D );
  //UPDATE_FSR;
}

DX_RETURNT dx_faddq( DX_PLIST ) {
  NOT_IMPLEMENTED;
  // WRITE_DEST_Q( SOURCE1_Q + SOURCE2_Q );
}

DX_RETURNT dx_fsubs( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  WATTCH_READ_SOURCE;
  WATTCH_WRITE_DEST( SOURCE1_S - SOURCE2_S );            
  WRITE_DEST_S( SOURCE1_S - SOURCE2_S );
  //UPDATE_FSR;
}

DX_RETURNT dx_fsubd( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  WATTCH_READ_SOURCE;
  WATTCH_WRITE_DEST( SOURCE1_D - SOURCE2_D );            
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
  WATTCH_READ_SOURCE;
  WATTCH_WRITE_DEST( SOURCE2 );            
  WRITE_DEST( SOURCE2 );
}

DX_RETURNT dx_fmovd( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  WATTCH_WRITE_DEST( SOURCE2_DI );            
  WRITE_DEST_DI( SOURCE2_DI );
}

DX_RETURNT dx_fmovq( DX_PLIST ) {
  NOT_IMPLEMENTED;
  // WRITE_DEST_Q( SOURCE2_Q );
}

DX_RETURNT dx_fnegs( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  WATTCH_WRITE_DEST( -SOURCE2_S );            
  WRITE_DEST_S( -SOURCE2_S );
}

DX_RETURNT dx_fnegd( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  WATTCH_WRITE_DEST( -SOURCE2_D );            
  WRITE_DEST_D( -SOURCE2_D );
}

DX_RETURNT dx_fnegq( DX_PLIST ) {
  NOT_IMPLEMENTED;
  // WRITE_DEST_Q( -SOURCE2_Q );
}

DX_RETURNT dx_fmuls( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  WATTCH_READ_SOURCE;
  WATTCH_WRITE_DEST( SOURCE1_S * SOURCE2_S );            
  WRITE_DEST_S( SOURCE1_S * SOURCE2_S );
  //UPDATE_FSR;
}

DX_RETURNT dx_fmuld( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  WATTCH_READ_SOURCE;
  WATTCH_WRITE_DEST( SOURCE1_D * SOURCE2_D );            
  WRITE_DEST_D( SOURCE1_D * SOURCE2_D );
  //UPDATE_FSR;
}

DX_RETURNT dx_fmulq( DX_PLIST ) {
  NOT_IMPLEMENTED;
  // WRITE_DEST_Q( SOURCE1_Q * SOURCE2_Q );
}

DX_RETURNT dx_fdivs( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  WATTCH_READ_SOURCE;
  WATTCH_WRITE_DEST( SOURCE1_S / SOURCE2_S );            
  WRITE_DEST_S( SOURCE1_S / SOURCE2_S );
  //UPDATE_FSR;
}

DX_RETURNT dx_fdivd( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  WATTCH_READ_SOURCE;
  WATTCH_WRITE_DEST( SOURCE1_D / SOURCE2_D );            
  WRITE_DEST_D( SOURCE1_D / SOURCE2_D );
  //UPDATE_FSR;
}

DX_RETURNT dx_fdivq( DX_PLIST ) {
  NOT_IMPLEMENTED;
  // WRITE_DEST_Q( SOURCE1_Q / SOURCE2_Q );
}

DX_RETURNT dx_fsmuld( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  WATTCH_READ_SOURCE;
  WATTCH_WRITE_DEST( SOURCE1_S * SOURCE2_S );            
  WRITE_DEST_D( SOURCE1_S * SOURCE2_S );
}

DX_RETURNT dx_fdmulq( DX_PLIST ) {
  NOT_IMPLEMENTED;
  // WRITE_DEST_Q( SOURCE1_D * SOURCE2_D );
}

DX_RETURNT dx_fsqrts( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  WATTCH_WRITE_DEST( sqrt(SOURCE2_S) );            
  WRITE_DEST_S( sqrt( SOURCE2_S ) );
}

DX_RETURNT dx_fsqrtd( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  WATTCH_WRITE_DEST( sqrt(SOURCE2_D) );            
  WRITE_DEST_D( sqrt( SOURCE2_D ) );
}

DX_RETURNT dx_fsqrtq( DX_PLIST ) {
  NOT_IMPLEMENTED;
  // WRITE_DEST_Q( sqrt( SOURCE2_Q ) );
}

DX_RETURNT dx_retrn( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  WATTCH_READ_SOURCE;
  // DX_DISABLED
  // CM imprecise retrn has restore semantics as well!
  IREGISTER dest = SOURCE1 + S2ORI;
  BRANCH_INDIRECT( dest );
}

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
  WATTCH_READ_SOURCE;
  BRANCH_F( COND_CODE_U );
}

DX_RETURNT dx_fbg( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  BRANCH_F( COND_CODE_G );
}

DX_RETURNT dx_fbug( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  BRANCH_F( COND_CODE_G | COND_CODE_U );
}

DX_RETURNT dx_fbl( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  BRANCH_F( COND_CODE_L );
}

DX_RETURNT dx_fbul( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  BRANCH_F( COND_CODE_L | COND_CODE_U );
}

DX_RETURNT dx_fblg( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  BRANCH_F( COND_CODE_L | COND_CODE_G );
}

DX_RETURNT dx_fbne( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  BRANCH_F( COND_CODE_L | COND_CODE_G | COND_CODE_U );
}

DX_RETURNT dx_fbe( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  BRANCH_F( COND_CODE_E );
}

DX_RETURNT dx_fbue( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  BRANCH_F( COND_CODE_E | COND_CODE_U );
}

DX_RETURNT dx_fbge( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  BRANCH_F( COND_CODE_E | COND_CODE_G );
}

DX_RETURNT dx_fbuge( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  BRANCH_F( COND_CODE_E | COND_CODE_G | COND_CODE_U );
}

DX_RETURNT dx_fble( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  BRANCH_F( COND_CODE_E | COND_CODE_L );
}

DX_RETURNT dx_fbule( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  BRANCH_F( COND_CODE_E | COND_CODE_L | COND_CODE_U );
}

DX_RETURNT dx_fbo( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  BRANCH_F( COND_CODE_E | COND_CODE_L | COND_CODE_G );
}

DX_RETURNT dx_fbpu( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  BRANCH_FP( COND_CODE_U );
}

DX_RETURNT dx_fbpg( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  BRANCH_FP( COND_CODE_G );
}

DX_RETURNT dx_fbpug( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  BRANCH_FP( COND_CODE_G | COND_CODE_U );
}

DX_RETURNT dx_fbpl( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  BRANCH_FP( COND_CODE_L );
}

DX_RETURNT dx_fbpul( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  BRANCH_FP( COND_CODE_L | COND_CODE_U );
}

DX_RETURNT dx_fbplg( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  BRANCH_FP( COND_CODE_L | COND_CODE_G );
}

DX_RETURNT dx_fbpne( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  BRANCH_FP( COND_CODE_L | COND_CODE_G | COND_CODE_U );
}

DX_RETURNT dx_fbpe( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  BRANCH_FP( COND_CODE_E );
}

DX_RETURNT dx_fbpue( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  BRANCH_FP( COND_CODE_E | COND_CODE_U );
}

DX_RETURNT dx_fbpge( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  BRANCH_FP( COND_CODE_E | COND_CODE_G );
}

DX_RETURNT dx_fbpuge( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  BRANCH_FP( COND_CODE_E | COND_CODE_G | COND_CODE_U );
}

DX_RETURNT dx_fbple( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  BRANCH_FP( COND_CODE_E | COND_CODE_L );
}

DX_RETURNT dx_fbpule( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  BRANCH_FP( COND_CODE_E | COND_CODE_L | COND_CODE_U );
}

DX_RETURNT dx_fbpo( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  BRANCH_FP( COND_CODE_E | COND_CODE_L | COND_CODE_G );
}

DX_RETURNT dx_bne( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  BRANCH( !COND_CODE_Z );
}

DX_RETURNT dx_be( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  BRANCH( COND_CODE_Z );
}

DX_RETURNT dx_bg( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  BRANCH( !( COND_CODE_Z | (COND_CODE_N ^ COND_CODE_V) ) );
}

DX_RETURNT dx_ble( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  BRANCH( ( COND_CODE_Z | (COND_CODE_N ^ COND_CODE_V) ) );
}

DX_RETURNT dx_bge( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  BRANCH( !(COND_CODE_N ^ COND_CODE_V) );
}

DX_RETURNT dx_bl( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  BRANCH( (COND_CODE_N ^ COND_CODE_V) );
}

DX_RETURNT dx_bgu( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  BRANCH( !(COND_CODE_C | COND_CODE_Z) );
}

DX_RETURNT dx_bleu( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  BRANCH( (COND_CODE_C | COND_CODE_Z) );
}

DX_RETURNT dx_bcc( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  BRANCH( !COND_CODE_C );
}

DX_RETURNT dx_bcs( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  BRANCH( COND_CODE_C );
}

DX_RETURNT dx_bpos( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  BRANCH( !COND_CODE_N );
}

DX_RETURNT dx_bneg( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  BRANCH( COND_CODE_N );
}

DX_RETURNT dx_bvc( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  BRANCH( !COND_CODE_V );
}

DX_RETURNT dx_bvs( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  BRANCH( COND_CODE_V );
}

DX_RETURNT dx_fstox( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  IREGISTER result = (IREGISTER) SOURCE2_S;
  WATTCH_WRITE_DEST( result );            
  //LUKE - BUGFIX - should write as 64bit Integer in FP regfile
  WRITE_DEST( result );
}

DX_RETURNT dx_fdtox( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  IREGISTER result = (IREGISTER) SOURCE2_D;
  WATTCH_WRITE_DEST( result );            
  //LUKE - BUGFIX - should write as 64bit integer double in FP regfile
  WRITE_DEST( result );
}

DX_RETURNT dx_fqtox( DX_PLIST ) {
  NOT_IMPLEMENTED;
}

DX_RETURNT dx_fstoi( DX_PLIST ) {
  WATTCH_READ_SOURCE
  IREGISTER result = (int) SOURCE2_S;
  WATTCH_WRITE_DEST( result );            
  WRITE_DEST( result );
}

DX_RETURNT dx_fdtoi( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  IREGISTER result = (int) SOURCE2_D;
  WATTCH_WRITE_DEST( result );            
  WRITE_DEST( result );
}

DX_RETURNT dx_fqtoi( DX_PLIST ) {
  NOT_IMPLEMENTED;
}

DX_RETURNT dx_fstod( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  WATTCH_WRITE_DEST( SOURCE2_S );            
  WRITE_DEST_D( SOURCE2_S );
}

DX_RETURNT dx_fstoq( DX_PLIST ) {
  NOT_IMPLEMENTED;
}

DX_RETURNT dx_fdtos( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  WATTCH_WRITE_DEST( SOURCE2_D );            
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
  WATTCH_READ_SOURCE;
  IREGISTER result = SOURCE2_DI;
  WATTCH_WRITE_DEST( (int64) result );            
  WRITE_DEST_S( (int64) result );
}

DX_RETURNT dx_fxtod( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  IREGISTER result = SOURCE2_DI;
  WATTCH_WRITE_DEST( (int64) result );            
  WRITE_DEST_D( (int64) result );
}

DX_RETURNT dx_fxtoq( DX_PLIST ) {
  NOT_IMPLEMENTED;
}

DX_RETURNT dx_fitos( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  IREGISTER result = SOURCE2;
  WATTCH_WRITE_DEST( (int32) result );            
  WRITE_DEST_S( (int32) result );
}

DX_RETURNT dx_fitod( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  WATTCH_WRITE_DEST( (int32) SOURCE2 );            
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
  WATTCH_READ_SOURCE;
  FMOVECC_F( S, COND_CODE_L | COND_CODE_G | COND_CODE_U );
}

DX_RETURNT dx_fmovfslg( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  FMOVECC_F( S, COND_CODE_L | COND_CODE_G );
}

DX_RETURNT dx_fmovfsul( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  FMOVECC_F( S, COND_CODE_L | COND_CODE_U );
}

DX_RETURNT dx_fmovfsl( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  FMOVECC_F( S, COND_CODE_L );
}

DX_RETURNT dx_fmovfsug( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  FMOVECC_F( S, COND_CODE_G | COND_CODE_U );
}

DX_RETURNT dx_fmovfsg( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  FMOVECC_F( S, COND_CODE_G );
}

DX_RETURNT dx_fmovfsu( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  FMOVECC_F( S, COND_CODE_U );
}

DX_RETURNT dx_fmovfsa( DX_PLIST ) {
  FMOVECC_F( S, true );
}

DX_RETURNT dx_fmovfse( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  FMOVECC_F( S, COND_CODE_E );
}

DX_RETURNT dx_fmovfsue( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  FMOVECC_F( S, COND_CODE_E | COND_CODE_U );
}

DX_RETURNT dx_fmovfsge( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  FMOVECC_F( S, COND_CODE_E | COND_CODE_G );
}

DX_RETURNT dx_fmovfsuge( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  FMOVECC_F( S, COND_CODE_E | COND_CODE_G | COND_CODE_U );
}

DX_RETURNT dx_fmovfsle( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  FMOVECC_F( S, COND_CODE_E | COND_CODE_L );
}

DX_RETURNT dx_fmovfsule( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  FMOVECC_F( S, COND_CODE_E | COND_CODE_L | COND_CODE_U );
}

DX_RETURNT dx_fmovfso( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  FMOVECC_F( S, COND_CODE_E | COND_CODE_L | COND_CODE_G );
}

//
// Begin FMOVF {double} on {fp} condition codes
//

DX_RETURNT dx_fmovfdn( DX_PLIST ) {
  FMOVECC_F( D, false );
}

DX_RETURNT dx_fmovfdne( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  FMOVECC_F( D, COND_CODE_L | COND_CODE_G | COND_CODE_U );
}

DX_RETURNT dx_fmovfdlg( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  FMOVECC_F( D, COND_CODE_L | COND_CODE_G );
}

DX_RETURNT dx_fmovfdul( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  FMOVECC_F( D, COND_CODE_L | COND_CODE_U );
}

DX_RETURNT dx_fmovfdl( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  FMOVECC_F( D, COND_CODE_L );
}

DX_RETURNT dx_fmovfdug( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  FMOVECC_F( D, COND_CODE_G | COND_CODE_U );
}

DX_RETURNT dx_fmovfdg( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  FMOVECC_F( D, COND_CODE_G );
}

DX_RETURNT dx_fmovfdu( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  FMOVECC_F( D, COND_CODE_U );
}

DX_RETURNT dx_fmovfda( DX_PLIST ) {
  FMOVECC_F( D, true );
}

DX_RETURNT dx_fmovfde( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  FMOVECC_F( D, COND_CODE_E );
}

DX_RETURNT dx_fmovfdue( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  FMOVECC_F( D, COND_CODE_E | COND_CODE_U );
}

DX_RETURNT dx_fmovfdge( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  FMOVECC_F( D, COND_CODE_E | COND_CODE_G );
}

DX_RETURNT dx_fmovfduge( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  FMOVECC_F( D, COND_CODE_E | COND_CODE_G | COND_CODE_U );
}

DX_RETURNT dx_fmovfdle( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  FMOVECC_F( D, COND_CODE_E | COND_CODE_L );
}

DX_RETURNT dx_fmovfdule( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  FMOVECC_F( D, COND_CODE_E | COND_CODE_L | COND_CODE_U );
}

DX_RETURNT dx_fmovfdo( DX_PLIST ) {
  WATTCH_READ_SOURCE;
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
  WATTCH_READ_SOURCE;
  FMOVECC( S, COND_CODE_Z );
}

DX_RETURNT dx_fmovsle( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  FMOVECC( S, ( COND_CODE_Z | (COND_CODE_N ^ COND_CODE_V) ) );
}

DX_RETURNT dx_fmovsl( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  FMOVECC( S, (COND_CODE_N ^ COND_CODE_V) );
}

DX_RETURNT dx_fmovsleu( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  FMOVECC( S, (COND_CODE_C | COND_CODE_Z) );
}

DX_RETURNT dx_fmovscs( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  FMOVECC( S, COND_CODE_C );
}

DX_RETURNT dx_fmovsneg( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  FMOVECC( S, COND_CODE_N );
}

DX_RETURNT dx_fmovsvs( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  FMOVECC( S, COND_CODE_V );
}

DX_RETURNT dx_fmovsa( DX_PLIST ) {
  FMOVECC( S, true );
}

DX_RETURNT dx_fmovsne( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  FMOVECC( S, !COND_CODE_Z );
}

DX_RETURNT dx_fmovsg( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  FMOVECC( S, !( COND_CODE_Z | (COND_CODE_N ^ COND_CODE_V) ) );
}

DX_RETURNT dx_fmovsge( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  FMOVECC( S, !(COND_CODE_N ^ COND_CODE_V) );
}

DX_RETURNT dx_fmovsgu( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  FMOVECC( S, !(COND_CODE_C | COND_CODE_Z) );
}

DX_RETURNT dx_fmovscc( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  FMOVECC( S, !COND_CODE_C );
}

DX_RETURNT dx_fmovspos( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  FMOVECC( S, !COND_CODE_N );
}

DX_RETURNT dx_fmovsvc( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  FMOVECC( S, !COND_CODE_V );
}

//
// Begin FMOVF {double} on {integer} condition codes
//

DX_RETURNT dx_fmovdn( DX_PLIST ) {
  FMOVECC( D, false );
}

DX_RETURNT dx_fmovde( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  FMOVECC( D, COND_CODE_Z );
}

DX_RETURNT dx_fmovdle( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  FMOVECC( D, ( COND_CODE_Z | (COND_CODE_N ^ COND_CODE_V) ) );
}

DX_RETURNT dx_fmovdl( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  FMOVECC( D, (COND_CODE_N ^ COND_CODE_V) );
}

DX_RETURNT dx_fmovdleu( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  FMOVECC( D, (COND_CODE_C | COND_CODE_Z) );
}

DX_RETURNT dx_fmovdcs( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  FMOVECC( D, COND_CODE_C );
}

DX_RETURNT dx_fmovdneg( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  FMOVECC( D, COND_CODE_N );
}

DX_RETURNT dx_fmovdvs( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  FMOVECC( D, COND_CODE_V );
}

DX_RETURNT dx_fmovda( DX_PLIST ) {
  FMOVECC( D, true );
}

DX_RETURNT dx_fmovdne( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  FMOVECC( D, !COND_CODE_Z );
}

DX_RETURNT dx_fmovdg( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  FMOVECC( D, !( COND_CODE_Z | (COND_CODE_N ^ COND_CODE_V) ) );
}

DX_RETURNT dx_fmovdge( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  FMOVECC( D, !(COND_CODE_N ^ COND_CODE_V) );
}

DX_RETURNT dx_fmovdgu( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  FMOVECC( D, !(COND_CODE_C | COND_CODE_Z) );
}

DX_RETURNT dx_fmovdcc( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  FMOVECC( D, !COND_CODE_C );
}

DX_RETURNT dx_fmovdpos( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  FMOVECC( D, !COND_CODE_N );
}

DX_RETURNT dx_fmovdvc( DX_PLIST ) {
  WATTCH_READ_SOURCE;
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
// Begin FMOVR {single} on {integer registers}
//

DX_RETURNT dx_fmovrsz( DX_PLIST ) {
  FMOVER( S, == );
}

DX_RETURNT dx_fmovrslez( DX_PLIST ) {
  FMOVER( S, <= );
}

DX_RETURNT dx_fmovrslz( DX_PLIST ) {
  FMOVER( S, < );
}

DX_RETURNT dx_fmovrsnz( DX_PLIST ) {
  FMOVER( S, != );
}

DX_RETURNT dx_fmovrsgz( DX_PLIST ) {
  FMOVER( S, > );
}

DX_RETURNT dx_fmovrsgez( DX_PLIST ) {
  FMOVER( S, >= );
}

// Begin FMOVR {double} on {integer registers}

DX_RETURNT dx_fmovrdz( DX_PLIST ) {
  FMOVER( D, == );
}

DX_RETURNT dx_fmovrdlez( DX_PLIST ) {
  FMOVER( D, <= );
}

DX_RETURNT dx_fmovrdlz( DX_PLIST ) {
  FMOVER( D, < );
}

DX_RETURNT dx_fmovrdnz( DX_PLIST ) {
  FMOVER( D, != );
}

DX_RETURNT dx_fmovrdgz( DX_PLIST ) {
  FMOVER( D, > );
}

DX_RETURNT dx_fmovrdgez( DX_PLIST ) {
  FMOVER( D, >= );
}

// Begin FMOVR {quad} on {integer registers}

DX_RETURNT dx_fmovrqz( DX_PLIST ) {
  // FMOVER( Q, == );
  NOT_IMPLEMENTED;
}

DX_RETURNT dx_fmovrqlez( DX_PLIST ) {
  // FMOVER( Q, <= );
  NOT_IMPLEMENTED;
}

DX_RETURNT dx_fmovrqlz( DX_PLIST ) {
  // FMOVER( Q, < );
  NOT_IMPLEMENTED;
}

DX_RETURNT dx_fmovrqnz( DX_PLIST ) {
  // FMOVER( Q, != );
  NOT_IMPLEMENTED;
}

DX_RETURNT dx_fmovrqgz( DX_PLIST ) {
  // FMOVER( Q, > );
  NOT_IMPLEMENTED;
}

DX_RETURNT dx_fmovrqgez( DX_PLIST ) {
  // FMOVER( Q, >= );
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
  WATTCH_READ_SOURCE;
  MOVECC( !COND_CODE_Z );
}

DX_RETURNT dx_move( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  MOVECC( COND_CODE_Z );
}

DX_RETURNT dx_movg( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  MOVECC( !( COND_CODE_Z | (COND_CODE_N ^ COND_CODE_V) ) );
}

DX_RETURNT dx_movle( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  MOVECC( ( COND_CODE_Z | (COND_CODE_N ^ COND_CODE_V) ) );
}

DX_RETURNT dx_movge( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  MOVECC( !(COND_CODE_N ^ COND_CODE_V) );
}

DX_RETURNT dx_movl( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  MOVECC( (COND_CODE_N ^ COND_CODE_V) );
}

DX_RETURNT dx_movgu( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  MOVECC( !(COND_CODE_C | COND_CODE_Z) );
}

DX_RETURNT dx_movleu( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  MOVECC( (COND_CODE_C | COND_CODE_Z) );
}

DX_RETURNT dx_movcc( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  MOVECC( !COND_CODE_C );
}

DX_RETURNT dx_movcs( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  MOVECC( COND_CODE_C );
}

DX_RETURNT dx_movpos( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  MOVECC( !COND_CODE_N );
}

DX_RETURNT dx_movneg( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  MOVECC( COND_CODE_N );
}

DX_RETURNT dx_movvc( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  MOVECC( !COND_CODE_V );
}

DX_RETURNT dx_movvs( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  MOVECC( COND_CODE_V );
}

//
// move {integer} registers based on {fp} cond codes
//

DX_RETURNT dx_movfu( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  MOVECC_F( COND_CODE_U );
}

DX_RETURNT dx_movfg( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  MOVECC_F( COND_CODE_G );
}

DX_RETURNT dx_movfug( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  MOVECC_F( COND_CODE_G | COND_CODE_U );
}

DX_RETURNT dx_movfl( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  MOVECC_F( COND_CODE_L );
}

DX_RETURNT dx_movful( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  MOVECC_F( COND_CODE_L | COND_CODE_U );
}

DX_RETURNT dx_movflg( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  MOVECC_F( COND_CODE_L | COND_CODE_G );
}

DX_RETURNT dx_movfne( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  MOVECC_F( COND_CODE_L | COND_CODE_G | COND_CODE_U );
}

DX_RETURNT dx_movfe( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  MOVECC_F( COND_CODE_E );
}

DX_RETURNT dx_movfue( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  MOVECC_F( COND_CODE_E | COND_CODE_U );
}

DX_RETURNT dx_movfge( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  MOVECC_F( COND_CODE_E | COND_CODE_G );
}

DX_RETURNT dx_movfuge( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  MOVECC_F( COND_CODE_E | COND_CODE_G | COND_CODE_U );
}

DX_RETURNT dx_movfle( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  MOVECC_F( COND_CODE_E | COND_CODE_L );
}

DX_RETURNT dx_movfule( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  MOVECC_F( COND_CODE_E | COND_CODE_L | COND_CODE_U );
}

DX_RETURNT dx_movfo( DX_PLIST ) {
  WATTCH_READ_SOURCE;
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
// Trap on {integer} condition codes
//

DX_RETURNT dx_ta( DX_PLIST ) {
  BRANCH_TRAP( true );
}

DX_RETURNT dx_tn( DX_PLIST ) {
  BRANCH_TRAP( false );
}

DX_RETURNT dx_tne( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  BRANCH_TRAP( !COND_CODE_Z );
}

DX_RETURNT dx_te( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  BRANCH_TRAP( COND_CODE_Z );
}

DX_RETURNT dx_tg( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  BRANCH_TRAP( !( COND_CODE_Z | (COND_CODE_N ^ COND_CODE_V) ) );
}

DX_RETURNT dx_tle( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  BRANCH_TRAP( ( COND_CODE_Z | (COND_CODE_N ^ COND_CODE_V) ) );
}

DX_RETURNT dx_tge( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  BRANCH_TRAP( !(COND_CODE_N ^ COND_CODE_V) );
}

DX_RETURNT dx_tl( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  BRANCH_TRAP( (COND_CODE_N ^ COND_CODE_V) );
}

DX_RETURNT dx_tgu( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  BRANCH_TRAP( !(COND_CODE_C | COND_CODE_Z) );
}

DX_RETURNT dx_tleu( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  BRANCH_TRAP( (COND_CODE_C | COND_CODE_Z) );
}

DX_RETURNT dx_tcc( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  BRANCH_TRAP( !COND_CODE_C );
}

DX_RETURNT dx_tcs( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  BRANCH_TRAP( COND_CODE_C );
}

DX_RETURNT dx_tpos( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  BRANCH_TRAP( !COND_CODE_N );
}

DX_RETURNT dx_tneg( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  BRANCH_TRAP( COND_CODE_N );
}

DX_RETURNT dx_tvc( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  BRANCH_TRAP( !COND_CODE_V );
}

DX_RETURNT dx_tvs( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  BRANCH_TRAP( COND_CODE_V );
}

//
//
//

DX_RETURNT dx_sub( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  WATTCH_READ_SOURCE;
  IREGISTER result = (SOURCE1 - S2ORI);
  WATTCH_WRITE_DEST( result );            
  WRITE_DEST(result);
}

DX_RETURNT dx_subcc( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  WATTCH_READ_SOURCE;
  unsigned char ccodes;
  // CC_MODIFICATION: changes condition codes
  // usub64 and ssub64 map to same function call
  IREGISTER result = us_subcc( SOURCE1, S2ORI, &ccodes );
  //m_pseq->out_info(" subcc -- %d\n", ccodes);
  WRITE_CC( ccodes );
  WATTCH_WRITE_DEST( result );            
  WRITE_DEST(result);
}

DX_RETURNT dx_subc( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  WATTCH_READ_SOURCE;
  WATTCH_READ_SOURCE;
  IREGISTER result = (SOURCE1 - S2ORI - ICC_C);
  WATTCH_WRITE_DEST( result );            
  WRITE_DEST(result);
}

DX_RETURNT dx_subccc( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  WATTCH_READ_SOURCE;
  WATTCH_READ_SOURCE;
  unsigned char ccodes;

  // CC_MODIFICATION: changes condition codes
  // usub64 and ssub64 map to same function call
  IREGISTER result = us_subcc( SOURCE1, (S2ORI + ICC_C), &ccodes );
  //m_pseq->out_info(" subccc -- %d\n", ccodes);
  WRITE_CC( ccodes );
  WATTCH_WRITE_DEST( result );            
  WRITE_DEST(result);
}

DX_RETURNT dx_or( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  WATTCH_READ_SOURCE;
  IREGISTER result = (SOURCE1 | S2ORI);
  WATTCH_WRITE_DEST( result );            
  WRITE_DEST(result);
}

DX_RETURNT dx_orcc( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  WATTCH_READ_SOURCE;
  unsigned char ccodes;
  // CC_MODIFICATION: changes condition codes
  IREGISTER result = us_orcc( SOURCE1, S2ORI, &ccodes );
  WRITE_CC( ccodes );
  WATTCH_WRITE_DEST( result );            
  WRITE_DEST(result);
}

DX_RETURNT dx_orn( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  WATTCH_READ_SOURCE;
  IREGISTER result = (SOURCE1 | ~S2ORI);
  WATTCH_WRITE_DEST( result );            
  WRITE_DEST(result);
}

DX_RETURNT dx_orncc( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  WATTCH_READ_SOURCE;
  unsigned char ccodes;
  // CC_MODIFICATION: changes condition codes
  IREGISTER result = us_orcc( SOURCE1, ~S2ORI, &ccodes );
  WRITE_CC( ccodes );
  WATTCH_WRITE_DEST( result );            
  WRITE_DEST(result);
}

DX_RETURNT dx_xor( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  WATTCH_READ_SOURCE;
  IREGISTER result = (SOURCE1 ^ S2ORI);
  WATTCH_WRITE_DEST( result );            
  WRITE_DEST(result);
}

DX_RETURNT dx_xorcc( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  WATTCH_READ_SOURCE;
  unsigned char ccodes;
  // CC_MODIFICATION: changes condition codes
  IREGISTER result = us_xorcc( SOURCE1, S2ORI, &ccodes );
  WRITE_CC( ccodes );
  WATTCH_WRITE_DEST( result );            
  WRITE_DEST(result);
}

DX_RETURNT dx_xnor( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  WATTCH_READ_SOURCE;
  IREGISTER result = (SOURCE1 ^ ~S2ORI);
  WATTCH_WRITE_DEST( result );            
  WRITE_DEST(result);
}

DX_RETURNT dx_xnorcc( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  WATTCH_READ_SOURCE;
  unsigned char ccodes;
  // CC_MODIFICATION: changes condition codes
  IREGISTER result = us_xorcc( SOURCE1, ~S2ORI, &ccodes );
  WRITE_CC( ccodes );
  WATTCH_WRITE_DEST( result );            
  WRITE_DEST(result);
}

DX_RETURNT dx_mulx( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  WATTCH_READ_SOURCE;
  WATTCH_WRITE_DEST( SOURCE1 * S2ORI );            
  WRITE_DEST( SOURCE1 * S2ORI );
}

DX_RETURNT dx_sdivx( DX_PLIST ) {
  if ((int64) S2ORI == 0) {
    WATTCH_READ_SOURCE;
    setTrapType( Trap_Division_By_Zero );
  } else {
    WATTCH_READ_SOURCE;
    WATTCH_READ_SOURCE;
    WATTCH_WRITE_DEST( (int64) SOURCE1 / (int64) S2ORI );            
    WRITE_DEST( (int64) SOURCE1 / (int64) S2ORI );
  }
}

DX_RETURNT dx_udivx( DX_PLIST ) {
  if (S2ORI == 0) {
    WATTCH_READ_SOURCE;
    setTrapType( Trap_Division_By_Zero );
  } else {
    WATTCH_READ_SOURCE;
    WATTCH_READ_SOURCE;
    WATTCH_WRITE_DEST( (uint64) SOURCE1 / (uint64) S2ORI );            
    WRITE_DEST( (uint64) SOURCE1 / (uint64) S2ORI );
  }
}

DX_RETURNT dx_sll( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  WATTCH_READ_SOURCE;
  // shifting is done modulo 32-bits
  WATTCH_WRITE_DEST( ((uint64) SOURCE1) << (S2ORI % 32) );            
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

DX_RETURNT dx_udiv( DX_PLIST ) {
  if (S2ORI == 0) {
    WATTCH_READ_SOURCE;
    setTrapType( Trap_Division_By_Zero );
  } else {
    WATTCH_READ_SOURCE;
    WATTCH_READ_SOURCE;
    WATTCH_READ_SOURCE;
    IREGISTER dividend = (SOURCE_CTL << 32) | (SOURCE1 & MEM_WORD_MASK);
    IREGISTER result   = (uint64) dividend / (uint64) S2ORI;
    WATTCH_WRITE_DEST( result );            
    WRITE_DEST( result );
  }
}

DX_RETURNT dx_sdiv( DX_PLIST ) {
  if ((int) S2ORI == 0) {
    WATTCH_READ_SOURCE;
    setTrapType( Trap_Division_By_Zero );
  } else {
    WATTCH_READ_SOURCE;
    WATTCH_READ_SOURCE;
    WATTCH_READ_SOURCE;
    IREGISTER dividend = (SOURCE_CTL << 32) | (SOURCE1 & MEM_WORD_MASK);
    IREGISTER result   = (int64) dividend / (int64) S2ORI;
    WATTCH_WRITE_DEST( result );            
    WRITE_DEST( result );
  }
}

DX_RETURNT dx_umul( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  WATTCH_READ_SOURCE;
  uint32    reg1 = (SOURCE1 & MEM_WORD_MASK);
  uint32    reg2 = (S2ORI & MEM_WORD_MASK);
  IREGISTER result = (uint64) reg1 * (uint64) reg2;
  WATTCH_WRITE_DEST( result );            
  WRITE_DEST( result );

  IREGISTER y    = result >> 32;
  INIT_CONTROL;
  WATTCH_WRITE_DEST( y );   
  WRITE_CONTROL( y );
  FINALIZE_CONTROL;
}

DX_RETURNT dx_smul( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  WATTCH_READ_SOURCE;
  int       reg1 = (SOURCE1 & MEM_WORD_MASK);
  int       reg2 = (S2ORI & MEM_WORD_MASK);
  IREGISTER result = (int64) reg1 * (int64) reg2;
  WATTCH_WRITE_DEST( result );            
  WRITE_DEST( result );

  IREGISTER y    = result >> 32;
  INIT_CONTROL;
  WATTCH_WRITE_DEST( y );   
  WRITE_CONTROL( y );
  FINALIZE_CONTROL;
}

DX_RETURNT dx_udivcc( DX_PLIST ) {
  if ( (uint32) S2ORI == 0) {
    WATTCH_READ_SOURCE;
    setTrapType( Trap_Division_By_Zero );
  } else {
    WATTCH_READ_SOURCE;
    WATTCH_READ_SOURCE;
    WATTCH_READ_SOURCE;
    IREGISTER dividend = (SOURCE_CTL << 32) | (SOURCE1 & MEM_WORD_MASK);
    IREGISTER result   = (uint64) dividend / (uint64) S2ORI;

    BUILD_OLD_CCODES;
    // modify the V ("overflow") bit
    // greater than or equal to 2^32
    if ( result >= 0x100000000ULL ) {
      ccode |= (1 << 2);
      result = 0xffffffffULL;
    }
    WRITE_CC( ccode );
    WATTCH_WRITE_DEST( result );            
    WRITE_DEST( result );
  }
  // CC_MODIFICATION: changes condition codes
}

DX_RETURNT dx_sdivcc( DX_PLIST ) {
  if ( (int) S2ORI == 0) {
    WATTCH_READ_SOURCE;
    setTrapType( Trap_Division_By_Zero );
  } else {
    WATTCH_READ_SOURCE;
    WATTCH_READ_SOURCE;
    WATTCH_READ_SOURCE;
    IREGISTER dividend = (SOURCE_CTL << 32) | (SOURCE1 & MEM_WORD_MASK);
    IREGISTER result   = (int64) dividend / (int64) S2ORI;

    BUILD_OLD_CCODES;
    // modify the V ("overflow") bit
    // greater than or equal to 2^31
    if ( (int64) result >= 0x80000000LL ) {
      ccode |= (1 << 2);
      result = 0x7fffffffULL;
      // less than or equal to -2^31 - 1
    } else if ( (int64) result <= -2147483649LL ) {
      ccode |= (1 << 2);
      result = 0xffffffff80000000ULL;
    }
    WRITE_CC( ccode );
    WATTCH_WRITE_DEST( result );            
    WRITE_DEST( result );
  }
  // CC_MODIFICATION: changes condition codes
}

DX_RETURNT dx_umulcc( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  WATTCH_READ_SOURCE;
  uint32    reg1 = (SOURCE1 & MEM_WORD_MASK);
  uint32    reg2 = (S2ORI & MEM_WORD_MASK);
  IREGISTER result = (uint64) reg1 * (uint64) reg2;
  WATTCH_WRITE_DEST( result );            
  WRITE_DEST( result );
  
  IREGISTER y    = result >> 32;
  // destReg(1) is container class, in which selector 1 == %Y register
  getDestReg(1).setSelector( 1 );
  getDestReg(1).getARF()->initializeControl( getDestReg(1), getProc() );
  getDestReg(1).getARF()->writeInt64( getDestReg(1), (y), getProc() );
  getDestReg(1).getARF()->finalizeControl( getDestReg(1), getProc() );
  
  unsigned char ccode = 0;
  uint32        res32 = (uint32) result;
  // set 64 bit condition codes
  // set the n bit
  ccode |= ( result >> 63 == 1 ) << 7;
  // set the z bit
  ccode |= ( result == 0 ) << 6;
  
  // set 32 bit condition codes
  // set the n bit
  ccode |= ( res32 >> 31 == 1 ) << 3;
  // set the z bit
  ccode |= ( res32 == 0 )  << 2;

  // CC_MODIFICATION: changes condition codes

  // selector 0 == (renamed) %ccr register
  getDestReg(1).setSelector( 0 );
  getDestReg(1).getARF()->writeInt64( getDestReg(1), ccode, getProc() );
}

DX_RETURNT dx_smulcc( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  WATTCH_READ_SOURCE;
  int       reg1 = (SOURCE1 & MEM_WORD_MASK);
  int       reg2 = (S2ORI & MEM_WORD_MASK);
  IREGISTER result = (int64) reg1 * (int64) reg2;
  WATTCH_WRITE_DEST( result );            
  WRITE_DEST( result );

  IREGISTER y    = result >> 32;
  // destReg(1) is container class, in which selector 1 == %Y register
  getDestReg(1).setSelector( 1 );
  getDestReg(1).getARF()->initializeControl( getDestReg(1), getProc() );
  getDestReg(1).getARF()->writeInt64( getDestReg(1), (y), getProc() );
  getDestReg(1).getARF()->finalizeControl( getDestReg(1), getProc() );

  unsigned char ccode = 0;
  uint32        res32 = (uint32) result;
  // set 64 bit condition codes
  // set the n bit
  ccode |= ( result >> 63 == 1 ) << 7;
  // set the z bit
  ccode |= ( result == 0 ) << 6;
  
  // set 32 bit condition codes
  // set the n bit
  ccode |= ( res32 >> 31 == 1 ) << 3;
  // set the z bit
  ccode |= ( res32 == 0 )  << 2;

  // CC_MODIFICATION: changes condition codes
  // selector 0 == (renamed) %ccr register
  getDestReg(1).setSelector( 0 );
  getDestReg(1).getARF()->writeInt64( getDestReg(1), ccode, getProc() );
}

DX_RETURNT dx_mulscc( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  WATTCH_READ_SOURCE;
  uint32    reg1 = (SOURCE1 & MEM_WORD_MASK);
  uint32    reg2 = (S2ORI & MEM_WORD_MASK);
  IREGISTER result;
  unsigned char ccodes;

  // least significant bit of register 2
  int reg2_lsb = (reg2 & 0x1);
  // soon to be most significant bit of register 1
  WATTCH_READ_SOURCE;
  int reg1_msb = COND_CODE_N ^ COND_CODE_V;

  reg1  = ((reg1 >> 1) | (reg1_msb << 31));
  WATTCH_READ_SOURCE;
  IREGISTER y_reg = SOURCE_CTL;
  y_reg = ((y_reg >> 1) | (reg2_lsb << 31));
  if ( (y_reg & 0x1) ) {
    result = us_addcc( reg1, reg2, &ccodes );
  } else {
    result = us_addcc( reg1, 0, &ccodes );
  }

  // destReg(1) is container class, in which selector 1 == %Y register
  getDestReg(1).setSelector( 1 );
  getDestReg(1).getARF()->initializeControl( getDestReg(1), getProc() );
  getDestReg(1).getARF()->writeInt64( getDestReg(1), (y_reg), getProc() );
  getDestReg(1).getARF()->finalizeControl( getDestReg(1), getProc() );

  unsigned char ccode = 0;
  uint32        res32 = (uint32) result;
  // set 64 bit condition codes
  // set the n bit
  ccode |= ( result >> 63 == 1 ) << 7;
  // set the z bit
  ccode |= ( result == 0 ) << 6;
  
  // set 32 bit condition codes
  // set the n bit
  ccode |= ( res32 >> 31 == 1 ) << 3;
  // set the z bit
  ccode |= ( res32 == 0 )  << 2;

  // CC_MODIFICATION: changes condition codes
  // selector 0 == (renamed) %ccr register
  getDestReg(1).setSelector( 0 );
  getDestReg(1).getARF()->writeInt64( getDestReg(1), ccode, getProc() );
  WATTCH_WRITE_DEST( (result & MEM_WORD_MASK) );            
  WRITE_DEST( (result & MEM_WORD_MASK) );
}

DX_RETURNT dx_popc( DX_PLIST ) {
  NOT_IMPLEMENTED;
}

DX_RETURNT dx_flush( DX_PLIST ) {
}

DX_RETURNT dx_flushw( DX_PLIST ) {
  ireg_t cansave;
  READ_SELECT_CONTROL( CONTROL_CANSAVE, cansave );

  if (cansave != NWINDOWS - 2) {

    // event spill trap
    ireg_t otherwin;
    ireg_t wstate;
    READ_SELECT_CONTROL( CONTROL_OTHERWIN, otherwin );
    READ_SELECT_CONTROL( CONTROL_WSTATE, wstate );
    // 010 (base 2) for spill traps
    int traptype  = 2;
      
    // cause a spill trap
    STAT_INC( m_pseq->m_stat_spill[m_proc] );

    ///    Task: form mask for spill trap type
    if (otherwin != 0) {
      traptype = (traptype << 1) | 0x1;
      // use other wstate register
      traptype = (traptype << 3) | (wstate >> 3 & 0x7);
    } else {
      traptype = traptype << 1;
      // use normal wstate register
      traptype = (traptype << 3) | (wstate & 0x7);
    }
    traptype = (traptype << 2);
    setTrapType( (trap_type_t) traptype );
    // m_pseq->out_info("SPILL TL 0x%0x\n", traptype);
  }
}

DX_RETURNT dx_rd( DX_PLIST ) {
  int16 src = GETCTL_SOURCE;
  if ( (src == CONTROL_UNIMPL) ) {
    USE_FUNCTIONAL;
  } else {
    WATTCH_READ_SOURCE;
    WATTCH_WRITE_DEST( SOURCE_CTL );            
    WRITE_DEST( SOURCE_CTL );
  }
}

DX_RETURNT dx_rdcc( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  IREGISTER  result = READ_CC;
  WATTCH_WRITE_DEST( result );            
  WRITE_DEST( result );
}

DX_RETURNT dx_wr( DX_PLIST ) {
  int16     dest = getDestReg(1).getVanilla();
  if ((dest == CONTROL_UNIMPL) ) {
    // do nothing: unimplemented registers are always ready,
    //             so check will succeed
    INIT_CONTROL;
    //indicate that we have unimplemented control reg
    USE_FUNCTIONAL;
    FINALIZE_CONTROL;
  } else {
    WATTCH_READ_SOURCE;
    WATTCH_READ_SOURCE;
    IREGISTER result = (SOURCE1 ^ S2ORI);
    // truncate result ( if necessary ) for register size
    // " - 1" is necessary as 10 bit PSTATE is bits "0 to 9"
    result = result & makeMask64( reg_box_t::controlRegSize((control_reg_t)dest) - 1, 0 );
    INIT_CONTROL;
    WATTCH_WRITE_DEST( result );   
    WRITE_CONTROL( result );
    FINALIZE_CONTROL;
  }
}

DX_RETURNT dx_wrcc( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  WATTCH_READ_SOURCE;
  WRITE_CC( SOURCE1 ^ S2ORI );
}

DX_RETURNT dx_save( DX_PLIST ) {
  ireg_t cwp;
  ireg_t cansave;
  ireg_t canrestore;
  ireg_t cleanwin;

  INIT_CONTROL;
  READ_SELECT_CONTROL( CONTROL_CWP, cwp );
  READ_SELECT_CONTROL( CONTROL_CANSAVE, cansave );
  READ_SELECT_CONTROL( CONTROL_CANRESTORE, canrestore );
  READ_SELECT_CONTROL( CONTROL_CLEANWIN, cleanwin );

  if (cansave == 0) {
    // event spill trap
    ireg_t otherwin;
    ireg_t wstate;
    READ_SELECT_CONTROL( CONTROL_OTHERWIN, otherwin );
    READ_SELECT_CONTROL( CONTROL_WSTATE, wstate );
    // 010 (base 2) for spill traps
    int traptype  = 2;
      
    // cause a spill trap
    STAT_INC( m_pseq->m_stat_spill[m_proc] );

    ///    Task: form mask for spill trap type

    if (otherwin != 0) {
      traptype = (traptype << 1) | 0x1;
      // use other wstate register
      traptype = (traptype << 3) | (wstate >> 3 & 0x7);
    } else {
      traptype = traptype << 1;
      // use normal wstate register
      traptype = (traptype << 3) | (wstate & 0x7);
    }
    traptype = (traptype << 2);
    setTrapType( (trap_type_t) traptype );

    #ifdef DEBUG_DYNAMIC
       DEBUG_OUT("%%SAVE: SPILL TL 0x%0x\n", traptype);
     #endif
    
  } else if (cleanwin - canrestore == 0) {

    // event clean window trap
    setTrapType( Trap_Clean_Window );
    #ifdef DEBUG_DYNAMIC
      DEBUG_OUT("%%SAVE: CLEANWIN TL 0x%0x\n", Trap_Clean_Window );
    #endif
    
  } else {
    // normal save/restore
    BRANCH_CWP;

    cwp = (cwp + 1) % NWINDOWS;
    WATTCH_WRITE_DEST( cwp );   
    WRITE_SELECT_CONTROL( CONTROL_CWP, cwp );
    canrestore++;
    WATTCH_WRITE_DEST( canrestore );   
    WRITE_SELECT_CONTROL( CONTROL_CANRESTORE, canrestore );
    #ifdef DEBUG_DYNAMIC
      DEBUG_OUT("%%SAVE: canrestore: %d\n", canrestore);
    #endif
    cansave--;
    WATTCH_WRITE_DEST( cansave );   
    WRITE_SELECT_CONTROL( CONTROL_CANSAVE, cansave );
    #ifdef DEBUG_DYNAMIC
      DEBUG_OUT("%%SAVE: cansave: %d\n", cansave);
    #endif

    // if a spill/fill trap is not generated, perform an 'ADD' instruction
    //     store result in new windows destination register
    WATTCH_READ_SOURCE;
    WATTCH_READ_SOURCE;
    WATTCH_WRITE_DEST( (SOURCE1 + S2ORI) );            
    WRITE_DEST( (SOURCE1 + S2ORI) );
  }
  FINALIZE_CONTROL;
}

DX_RETURNT dx_restore( DX_PLIST ) {
  ireg_t cwp;
  ireg_t cansave;
  ireg_t canrestore;

  INIT_CONTROL;
  READ_SELECT_CONTROL( CONTROL_CWP, cwp );
  READ_SELECT_CONTROL( CONTROL_CANSAVE, cansave );
  READ_SELECT_CONTROL( CONTROL_CANRESTORE, canrestore );

  // m_pseq->out_info("### restore\n");
  // m_pseq->out_info("cansave    0x%0x\n", cansave);
  // m_pseq->out_info("canrestore 0x%0x\n", canrestore); 
  if (canrestore == 0) {
    // cause a fill trap
    ireg_t otherwin;
    ireg_t wstate;
    READ_SELECT_CONTROL( CONTROL_OTHERWIN, otherwin );
    READ_SELECT_CONTROL( CONTROL_WSTATE, wstate );
    // m_pseq->out_info("otherwin    0x%0x\n", otherwin);
    // m_pseq->out_info("wstate    0x%0x\n", wstate);
    
    // 011 (base 2) for fill traps
    int traptype  = 3;
    STAT_INC( m_pseq->m_stat_fill[m_proc] );      
    
    /*
     *  Task: form mask for fill trap type
     */
    if (otherwin != 0) {
      traptype = (traptype << 1) | 0x1;
      // use other wstate register
      traptype = (traptype << 3) | (wstate >> 3 & 0x7);
    } else {
      traptype = traptype << 1;
      // use normal wstate register
      traptype = (traptype << 3) | (wstate & 0x7);
    }
    traptype = (traptype << 2);
    setTrapType( (trap_type_t) traptype );
    // m_pseq->out_info("FILL 0x%0x\n", traptype);
  } else {
    // normal save/restore
    BRANCH_CWP;

    cwp = (cwp - 1) % NWINDOWS;
    WATTCH_WRITE_DEST( cwp );   
    WRITE_SELECT_CONTROL( CONTROL_CWP, cwp );
    canrestore--;
    WATTCH_WRITE_DEST( canrestore );   
    WRITE_SELECT_CONTROL( CONTROL_CANRESTORE, canrestore );
    cansave++;
    WATTCH_WRITE_DEST( cansave );   
    WRITE_SELECT_CONTROL( CONTROL_CANSAVE, cansave );

    // if a spill/fill trap is not generated, perform an 'ADD' instruction
    //     store result in new windows destination register
    WATTCH_READ_SOURCE;
    WATTCH_READ_SOURCE;
    WATTCH_WRITE_DEST( (SOURCE1 + S2ORI) );            
    WRITE_DEST( (SOURCE1 + S2ORI) );
  }
  FINALIZE_CONTROL;
}

DX_RETURNT dx_saved( DX_PLIST ) {

  ireg_t cansave;
  ireg_t canrestore;
  ireg_t otherwin;
  READ_SELECT_CONTROL( CONTROL_CANSAVE, cansave );
  READ_SELECT_CONTROL( CONTROL_CANRESTORE, canrestore );
  READ_SELECT_CONTROL( CONTROL_OTHERWIN, otherwin );
  cansave++;
  if (otherwin == 0) {
    canrestore--;
  } else {
    otherwin--;
  }

  INIT_CONTROL;
  WATTCH_WRITE_DEST( cansave );   
  WATTCH_WRITE_DEST( canrestore );   
  WATTCH_WRITE_DEST( otherwin );   
  WRITE_SELECT_CONTROL( CONTROL_CANSAVE, cansave );
  WRITE_SELECT_CONTROL( CONTROL_CANRESTORE, canrestore );
  WRITE_SELECT_CONTROL( CONTROL_OTHERWIN, otherwin );
  FINALIZE_CONTROL;
}

DX_RETURNT dx_restored( DX_PLIST ) {
  ireg_t cansave;
  ireg_t canrestore;
  ireg_t otherwin;
  ireg_t cleanwin;
  READ_SELECT_CONTROL( CONTROL_CANSAVE, cansave );
  READ_SELECT_CONTROL( CONTROL_CANRESTORE, canrestore );
  READ_SELECT_CONTROL( CONTROL_OTHERWIN, otherwin );
  READ_SELECT_CONTROL( CONTROL_CLEANWIN, cleanwin );
  canrestore++;
  if (otherwin == 0) {
    cansave--;
  } else {
    otherwin--;
  }
  // CM FIX: according to the SPARC V9 architecture manual, cleanwin should
  //         be incremented when it is not == to NWINDOWS. Simics however
  //         does not increment it above 7 (w/ NWINDOWS == 8).
  //         Its possible this should be "NWINDOWS" (not "- 1") here.
  if (cleanwin != (NWINDOWS - 1)) {
    cleanwin++;
  }

  INIT_CONTROL;
  WATTCH_WRITE_DEST( cansave );   
  WATTCH_WRITE_DEST( canrestore );   
  WATTCH_WRITE_DEST( otherwin );   
  WATTCH_WRITE_DEST( cleanwin );   
  WRITE_SELECT_CONTROL( CONTROL_CANSAVE, cansave );
  WRITE_SELECT_CONTROL( CONTROL_CANRESTORE, canrestore );
  WRITE_SELECT_CONTROL( CONTROL_OTHERWIN, otherwin );
  WRITE_SELECT_CONTROL( CONTROL_CLEANWIN, cleanwin );
  FINALIZE_CONTROL;
}

DX_RETURNT dx_sethi( DX_PLIST ) {
  WATTCH_WRITE_DEST( getStaticInst()->getImmediate() );            
  WRITE_DEST( getStaticInst()->getImmediate() );
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

DX_RETURNT dx_lddfa( DX_PLIST ) {
  READ_MEMORY_BLOCK;
}

DX_RETURNT dx_ldblk( DX_PLIST ) {
  // DX_DISABLED
  NOT_IMPLEMENTED;
#if 0
  // write to 8 double precision fp registers->..
  MEMORY_OP_TYPE *memop = MEMORY_OP_GET;

  if (memop->isDataValid()) {
    IREGISTER   *data  = memop->getData();
    STAT_INC( m_pseq->m_stat_loads_found[m_proc] );
    int              reg   = getDestReg(0).getVanilla();
    half_t           preg;
    uint32          *ival = (uint32 *) data;
    reg_map_t       *mymap = m_pseq->getFPMap();
    physical_file_t *myrf  = m_pseq->getFPFile();

    // write the renamed register, hence advance 2 single precision regs.
    WATTCH_WRITE_DEST( *data );            
    WRITE_DEST_DI( *data );
    reg  += 2;
    ival += 2;
    for ( int i = 2; i < 8; i ++ ) {
      // write 16 single precision registers with the 64-bit value
      preg = mymap->getMapping( reg++, m_proc );
      myrf->setInt( preg, *ival++ );
      
      preg = mymap->getMapping( reg++, m_proc );
      myrf->setInt( preg, *ival++ );
    }
  } else {
    STAT_INC( m_pseq->m_stat_loads_notfound[m_proc] );
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

DX_RETURNT dx_ldd( DX_PLIST ) {
  // double word into contiguous registers
  MEMORY_OP_TYPE *memop = MEMORY_OP_GET;

  if (memop->isDataValid()) {
    IREGISTER data = memop->getUnsignedData();
    STAT_INC( m_pseq->m_stat_loads_found[m_proc] );
    // does ENDIAN_MATTERS?
    // write lower 32-bits
    WATTCH_WRITE_DEST( (data >> 32) & MEM_WORD_MASK );            
    WRITE_DEST( (data >> 32) & MEM_WORD_MASK );
  
    // write upper 32-bits
    WATTCH_WRITE_DEST( (data & MEM_WORD_MASK) );            
    WRITE_DEST2( (data & MEM_WORD_MASK) );
  } else {
    STAT_INC( m_pseq->m_stat_loads_notfound[m_proc] );
    NOT_IMPLEMENTED;
  }
}

DX_RETURNT dx_ldsba( DX_PLIST ) {
  READ_MEMORY;
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

DX_RETURNT dx_ldda( DX_PLIST ) {
  // doubleword asi space:
  MEMORY_OP_TYPE *memop = MEMORY_OP_GET;

  if (memop->isDataValid()) {
    IREGISTER data = memop->getUnsignedData();
    
    STAT_INC( m_pseq->m_stat_loads_found[m_proc] );
    // does ENDIAN_MATTERS?
    // write lower 32-bits
    WATTCH_WRITE_DEST( (data >> 32) & MEM_WORD_MASK );            
    WRITE_DEST( (data >> 32) & MEM_WORD_MASK );
			      
    // write upper 32-bits
    WATTCH_WRITE_DEST( data & MEM_WORD_MASK );            
    WRITE_DEST2( data & MEM_WORD_MASK );
  } else {
    STAT_INC( m_pseq->m_stat_loads_notfound[m_proc] );
    NOT_IMPLEMENTED;
  }
}

DX_RETURNT dx_ldqa( DX_PLIST ) {
  // doubleword asi space:
  MEMORY_OP_TYPE *memop = MEMORY_OP_GET;

  if (memop->isDataValid()) {
    IREGISTER *data = memop->getData();
    STAT_INC( m_pseq->m_stat_loads_found[m_proc] );
    // does ENDIAN_MATTERS?
    // write lower 64-bits
    WATTCH_WRITE_DEST( data[0] );            
    WRITE_DEST( data[0] );

    // write upper 64-bits
    WATTCH_WRITE_DEST( data[1] );            
    WRITE_DEST2( data[1] );
  } else {
    STAT_INC( m_pseq->m_stat_loads_notfound[m_proc] );
    NOT_IMPLEMENTED;
  }
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

DX_RETURNT dx_swap( DX_PLIST ) {
  // read word from memory (already done in atomic_inst_t)
  MEMORY_OP_TYPE *memop = MEMORY_OP_GET;

  if (memop->isDataValid()) {
    WATTCH_READ_SOURCE;
    IREGISTER    data = memop->getUnsignedData();
    IREGISTER    tmp  = (SOURCE3 & MEM_WORD_MASK);

    // modify rd
    WATTCH_WRITE_DEST( data );            
    WRITE_DEST( data );

    // write the old value of rd to memory
    WRITE_MEMORY( tmp );
  } else {
    NOT_IMPLEMENTED;
  }
}

DX_RETURNT dx_swapa( DX_PLIST ) {
  // read word from memory (already done in atomic_inst_t)
  MEMORY_OP_TYPE *memop = MEMORY_OP_GET;

  if (memop->isDataValid()) {
    WATTCH_READ_SOURCE;
    IREGISTER    data = memop->getUnsignedData();
    IREGISTER    tmp  = (SOURCE3 & MEM_WORD_MASK);

    // modify rd
    WATTCH_WRITE_DEST( data );            
    WRITE_DEST( data );

    // write the old value of rd to memory
    WRITE_MEMORY( tmp );
  } else {
    NOT_IMPLEMENTED;
  }
}

DX_RETURNT dx_ldfsr( DX_PLIST ) {
  MEMORY_OP_TYPE *memop = MEMORY_OP_GET;
  if (memop->isDataValid()) {

    IREGISTER fsr_value;
    READ_SELECT_CONTROL( CONTROL_FSR, fsr_value );
    fsr_value = ((fsr_value & ~MEM_WORD_MASK) | 
		 memop->getUnsignedData());
    INIT_CONTROL;
    WATTCH_WRITE_DEST( fsr_value );   
    WRITE_CONTROL( fsr_value );
    FINALIZE_CONTROL;
  } else {
    // m_pseq->out_info("LDFSR NOT FOUND\n");
    // control file is not renamed ... just ignore the results
    NOT_IMPLEMENTED;
  }
}

DX_RETURNT dx_ldxfsr( DX_PLIST ) {
  MEMORY_OP_TYPE *memop = MEMORY_OP_GET;
  if (memop->isDataValid()) {
    INIT_CONTROL;
    WATTCH_WRITE_DEST( memop->getUnsignedData() );   
    WRITE_CONTROL( memop->getUnsignedData() );
    FINALIZE_CONTROL;
  } else {
    // m_pseq->out_info("LDXFSR NOT FOUND\n");
    // control file is not renamed ... just ignore the results
    NOT_IMPLEMENTED;
  }
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
  WATTCH_READ_SOURCE;
  WATTCH_READ_SOURCE;
  us_cmp( SOURCE1, S2ORI, &ccodes );
  WRITE_CC( ccodes );
}

DX_RETURNT dx_jmp( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  WATTCH_READ_SOURCE;
  IREGISTER dest = SOURCE1 + S2ORI;

  // CM AddressMask64 AM64
  IREGISTER pstate;
  READ_SELECT_CONTROL( CONTROL_PSTATE, pstate );
  if ( ISADDRESS32(pstate) ) {
    dest = AM64(dest);
  }
#if 0
  m_pseq->out_info("jmpl s1: 0x%0llx\n", SOURCE1);
  m_pseq->out_info("jmpl s2: 0x%0llx\n", S2ORI);
  m_pseq->out_info("jmpl d : 0x%0llx\n", dest);
#endif
  BRANCH_INDIRECT( dest );
}

DX_RETURNT dx_mov( DX_PLIST ) {
  IREGISTER result = S2ORI;
  WATTCH_WRITE_DEST( result );            
  WRITE_DEST(result);
}

DX_RETURNT dx_nop( DX_PLIST ) {
  /* do nothing */
}

DX_RETURNT dx_not( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  IREGISTER result = ~SOURCE1;
  WATTCH_WRITE_DEST( result );            
  WRITE_DEST(result);
}

DX_RETURNT dx_rdpr( DX_PLIST ) {
  int16 src = GETCTL_SOURCE;
  if ( (src == CONTROL_UNIMPL) ) {
    USE_FUNCTIONAL;
  } else {
    if (src == CONTROL_TPC1 ||
	src == CONTROL_TNPC1 ||
	src == CONTROL_TSTATE1 ||
	src == CONTROL_TT1 ) {
      IREGISTER tl;
      READ_SELECT_CONTROL( CONTROL_TL, tl );
      
      // TL is based on 1, our array is based on zero; hence '- 1'
      src = src + (tl - 1);
    }

    IREGISTER value;
    READ_SELECT_CONTROL( src, value );
    IREGISTER result = value & makeMask64( reg_box_t::controlRegSize((control_reg_t) src) - 1, 0 );
    WATTCH_WRITE_DEST( result );            
    WRITE_DEST( result );
  }
}

DX_RETURNT dx_wrpr( DX_PLIST ) {
  int16     dest   = getDestReg(1).getVanilla();
  if ( (dest == CONTROL_UNIMPL) ) {
    // do nothing: unimplemented registers are always ready,
    //             so check will succeed
    INIT_CONTROL;
    //indicate we don't know how to handle this instr
    USE_FUNCTIONAL;
    FINALIZE_CONTROL;
  } else {
    WATTCH_READ_SOURCE;
    WATTCH_READ_SOURCE;
    IREGISTER result = (SOURCE1 ^ S2ORI);
    // truncate result ( if necessary ) for register size
    // " - 1" is necessary as 10 bit PSTATE is bits "0 to 9"
    result = result & makeMask64( reg_box_t::controlRegSize((control_reg_t)dest) - 1, 0 );

    // if this instruction reads from one of a set of registers
    // use the tl (traplevel) to select which register is read
    if (dest == CONTROL_TPC1 ||
	dest == CONTROL_TNPC1 ||
	dest == CONTROL_TSTATE1 ||
	dest == CONTROL_TT1 ) {
      IREGISTER tl;
      READ_SELECT_CONTROL( CONTROL_TL, tl );
      // TL is based on 1, our array is based on zero; hence '- 1'
      dest = dest + (tl - 1);
      SETCTL_DEST( dest );
    }

    // if this instruction speculated on its pstate, tl or cwp
    // find the actual value to compare with the speculation
    if (dest == CONTROL_PSTATE ||
	dest == CONTROL_TL ||
	dest == CONTROL_CWP) {
      // ONLY wrpr instructions that modify the pstate, tl, or cwp are
      //      control instructions... all others are normal instructions!
      BRANCH_EXECUTE;

      IREGISTER  pstate;
      IREGISTER  tl;
      IREGISTER  cwp;
      if (dest == CONTROL_PSTATE) {
	pstate = result;
      } else {
	READ_SELECT_CONTROL( CONTROL_PSTATE, pstate );
      }
      if (dest == CONTROL_TL) {
	tl = result;
      } else {
	READ_SELECT_CONTROL( CONTROL_TL, tl );
      }
      if (dest == CONTROL_CWP) {
	cwp = result;
      } else {
	READ_SELECT_CONTROL( CONTROL_CWP, cwp );
      }
      BRANCH_PRIV( pstate, tl, cwp );
    }
    
    // write the privileged register
    INIT_CONTROL;
    WATTCH_WRITE_DEST( result );   
    WRITE_CONTROL( result );
    FINALIZE_CONTROL;
  }
}

DX_RETURNT dx_faligndata( DX_PLIST ) {

  // read the lower 3 bits of the gsr register
  SETCTL_SOURCE( CONTROL_GSR );
  WATTCH_READ_SOURCE;
  IREGISTER offset = SOURCE_CTL & 0x7;
  WATTCH_READ_SOURCE;
  WATTCH_READ_SOURCE;
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
  WATTCH_WRITE_DEST( rd );            
  WRITE_DEST_DI( rd );
}

DX_RETURNT dx_alignaddr( DX_PLIST ) {

  SETCTL_SOURCE( CONTROL_GSR );
  WATTCH_READ_SOURCE;
  WATTCH_READ_SOURCE;
  WATTCH_READ_SOURCE;
  IREGISTER gsr    = SOURCE_CTL;
  IREGISTER sum    = SOURCE1 + SOURCE2;

#if 0
  // mask off 3 lsb store in gsr
  m_pseq->out_info("rs1  0x%0llx\n", SOURCE1 );
  m_pseq->out_info("rs2  0x%0llx\n", SOURCE2 );
  m_pseq->out_info("res  0x%0llx\n", sum );
  m_pseq->out_info("alignaddr gsr = 0x%0x\n", sum & 0x7 );
#endif

  INIT_CONTROL;
  WATTCH_WRITE_DEST( (gsr & ~0x7) | (sum & 0x7) );   
  WRITE_CONTROL( (gsr & ~0x7) | (sum & 0x7) );
  FINALIZE_CONTROL;

  // store result, minus the 3 least sig bits
  WATTCH_WRITE_DEST( sum & ~0x7 );            
  WRITE_DEST( sum & ~0x7 );
}

DX_RETURNT dx_alignaddrl( DX_PLIST ) {

  SETCTL_SOURCE( CONTROL_GSR );
  WATTCH_READ_SOURCE;
  WATTCH_READ_SOURCE;
  WATTCH_READ_SOURCE;
  IREGISTER gsr    = SOURCE_CTL;
  IREGISTER sum    = SOURCE1 + SOURCE2;

  // mask off 3 lsb store in gsr -- compute 2's complement
  IREGISTER offset = ~(sum & 0x7) + 1;

  INIT_CONTROL;
  WATTCH_WRITE_DEST( (gsr & ~0x7) | (offset & 0x7) );   
  WRITE_CONTROL( (gsr & ~0x7) | (offset & 0x7) );
  FINALIZE_CONTROL;

  // store result, minus the 3 least sig bits
  WATTCH_WRITE_DEST( sum & ~0x7 );            
  WRITE_DEST( sum & ~0x7 );
}

DX_RETURNT dx_fzero( DX_PLIST ) {
  WATTCH_WRITE_DEST( 0 );            
  WRITE_DEST_D( 0 );
}

DX_RETURNT dx_fzeros( DX_PLIST ) {
  WATTCH_WRITE_DEST( 0 );            
  WRITE_DEST_S( 0 );
}

DX_RETURNT dx_fsrc1( DX_PLIST ) {
  WATTCH_READ_SOURCE;
  WATTCH_WRITE_DEST( SOURCE1_D );            
  WRITE_DEST_D( SOURCE1_D );
}

DX_RETURNT dx_doneretry( DX_PLIST ) {
  INIT_CONTROL;

  // read the trap state 
  SETCTL_SOURCE( CONTROL_TL );
  WATTCH_READ_SOURCE;
  IREGISTER tl = SOURCE_CTL;
  if ( tl == 0 || tl > MAXTL) {
    NOT_IMPLEMENTED;
    return;
  }

  // NOTE: normally TL is decremented at end of retry. Our trap state array
  //       has trap level 1 at array index 0, so we decrement it here.
  tl = (tl - 1);
  WATTCH_WRITE_DEST( tl );   
  WRITE_SELECT_CONTROL( CONTROL_TL, tl );

  SETCTL_SOURCE( (CONTROL_TSTATE1 + tl) );
  IREGISTER trapstate = SOURCE_CTL;

  IREGISTER cwp    = maskBits64( trapstate, 4, 0 );
  cwp &= (NWINDOWS - 1);
  WATTCH_WRITE_DEST( cwp );   
  WRITE_SELECT_CONTROL( CONTROL_CWP, cwp );
  
  IREGISTER pstate = maskBits64( trapstate, 19, 8 );
  WATTCH_WRITE_DEST( pstate );   
  WRITE_SELECT_CONTROL( CONTROL_PSTATE, pstate );
  
  IREGISTER asi    = maskBits64( trapstate, 31, 24 );
  WATTCH_WRITE_DEST( asi );   
  WRITE_SELECT_CONTROL( CONTROL_ASI, asi );

  IREGISTER ccr    = maskBits64( trapstate, 39, 32 );
  // write condition codes to dest1
  WATTCH_WRITE_DEST( ccr );            
  WRITE_DEST( ccr );
  
  // set PC and NPC (of the control instruction)
 
  SETCTL_SOURCE( CONTROL_TPC1 + tl );
  IREGISTER tpc = SOURCE_CTL;
  SETCTL_SOURCE( CONTROL_TNPC1 + tl );
  IREGISTER tnpc = SOURCE_CTL;

  #ifdef DEBUG_DYNAMIC
    DEBUG_OUT("dx.i: retry TL[ %d ] CWP[ 0x%x ] PSTATE[ 0x%x ] ASI[ 0x%x ] TPC[ 0x%llx ] TNPC[ 0x%llx ]\n", tl, cwp, pstate, asi, tpc, tnpc);
  #endif

  BRANCH_TRAP_RETURN( tpc, tnpc, pstate, tl, cwp );

  FINALIZE_CONTROL;
}

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

DX_RETURNT dx_bshuffle( DX_PLIST ) {
  // read the lower 3 bits of the gsr register
  SETCTL_SOURCE( CONTROL_GSR );
  WATTCH_READ_SOURCE;
  IREGISTER mask = (SOURCE_CTL >> 32) & MEM_WORD_MASK;
  WATTCH_READ_SOURCE;
  WATTCH_READ_SOURCE;
  IREGISTER rs1 = SOURCE1_DI;
  IREGISTER rs2 = SOURCE2_DI;
  IREGISTER rd  = 0;
  char     *src1_ptr = (char *) &rs1;
  char     *src2_ptr = (char *) &rs2;
  char     *result_ptr = (char *) &rd;

  // ENDIAN_MATTERS
  for (int index = 0; index < 8; index++) {
    char byte_select = mask & 0xf;
    char offset      = 7 - (byte_select & 0x7);
    mask = mask >> 4;
    // select the rs1|rs2[select] byte
    
    if (byte_select < 8) {
      result_ptr[index] = src1_ptr[offset];
    } else {
      result_ptr[index] = src2_ptr[offset];
    }
  }
  
#if 0
      m_pseq->out_info(" select byte 0x%x 0x%x src1: 0x%x\n",
		       byte_select, offset, result_ptr[index]);
      m_pseq->out_info(" select byte 0x%x 0x%0x src2: 0x%x\n",
		       byte_select, offset, result_ptr[index]);
  m_pseq->out_info("bshuffle\n");
  m_pseq->out_info("rs1  0x%0llx\n", rs1 );
  m_pseq->out_info("rs2  0x%0llx\n", rs2 );
  mask = (SOURCE_CTL >> 32) & MEM_WORD_MASK;
  m_pseq->out_info("gsr  0x%0llx\n", mask);
  m_pseq->out_info("result  0x%0llx\n", rd );
#endif
  WATTCH_WRITE_DEST( rd );            
  WRITE_DEST( rd );
}

DX_RETURNT dx_bmask( DX_PLIST ) {
  SETCTL_SOURCE( CONTROL_GSR );
  WATTCH_READ_SOURCE;
  IREGISTER gsr = SOURCE_CTL & makeMask64( reg_box_t::controlRegSize((control_reg_t)CONTROL_GSR) - 1, 0 );

  WATTCH_READ_SOURCE;
  WATTCH_READ_SOURCE;
  IREGISTER result = SOURCE1 + SOURCE2;
  WATTCH_WRITE_DEST( result );            
  WRITE_DEST(result);

  INIT_CONTROL;
  WATTCH_WRITE_DEST(  ((result & MEM_WORD_MASK) << 32) | (gsr & MEM_WORD_MASK) );   
  WRITE_CONTROL( ((result & MEM_WORD_MASK) << 32) | (gsr & MEM_WORD_MASK) );
  FINALIZE_CONTROL;
}

DX_RETURNT dx_idealwr( DX_PLIST ) {
  if ( !getDestReg(0).isZero() ) {
    WATTCH_WRITE_DEST( 0 );            
    WRITE_DEST( 0 );
  }
  if ( !getDestReg(1).isZero() ) {
    WATTCH_WRITE_DEST( 0 );            
    WRITE_DEST2( 0 );
  }
}

DX_RETURNT dx_ill( DX_PLIST ) {
}
