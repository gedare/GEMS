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
 * FileName:  flatarf.C
 * Synopsis:  implements a flat abstract register file
 * Author:    cmauer
 * Version:   $Id: flatarf.C 1.6 04/08/06 13:27:16-05:00 lyen@opus.cs.wisc.edu $
 */

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include "hfa.h"
#include "hfacore.h"
#include "checkresult.h"            // need declaration of check_result_t
#include "flow.h"
#include "flatarf.h"

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

#define FLATARF_MAX_DISPATCH_SIZE   9*CONFIG_LOGICAL_PER_PHY_PROC      

/*------------------------------------------------------------------------*/
/* Variable declarations                                                  */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Forward declarations                                                   */
/*------------------------------------------------------------------------*/

static void decrement( flow_inst_t *f );
static void increment( flow_inst_t *f );

/*------------------------------------------------------------------------*/
/* Flat (non-renamed) Register File                                       */
/*------------------------------------------------------------------------*/

//***************************************************************************
bool    flat_rf_t::allocateRegister( reg_id_t &rid, uint32 proc ) {
  DEBUG_OUT("error: flat_rf_t: using renaming interfaces on flat register file (allocateRegister).\n");
  return false;
}

//***************************************************************************
bool    flat_rf_t::freeRegister( reg_id_t &rid, uint32 proc ) {
  DEBUG_OUT("error: flat_rf_t: using renaming interfaces on flat register file (freeRegister).\n");
  return false;
}

//***************************************************************************
void    flat_rf_t::writeRetireMap( reg_id_t &rid, uint32 proc ) {
  DEBUG_OUT("error: flat_rf_t: using renaming interfaces on flat register file (writeRetireMap).\n");
}

//***************************************************************************
void    flat_rf_t::writeDecodeMap( reg_id_t &rid, uint32 proc ) {
  DEBUG_OUT("error: flat_rf_t: using renaming interfaces on flat register file (writeDecodeMap).\n");
}
  
//***************************************************************************
void    flat_rf_t::readDecodeMap( reg_id_t &rid, uint32 proc ) {
  DEBUG_OUT("error: flat_rf_t: using renaming interfaces on flat register file (readDecodeMap).\n");
}

//***************************************************************************
bool    flat_rf_t::isReady( reg_id_t &rid, uint32 proc ) {
  // flat registers are always ready
  DEBUG_OUT("error: flat_rf_t: call to isReady implies timing use.\n");
  return true;
}
  
//***************************************************************************
void    flat_rf_t::waitResult( reg_id_t &rid, dynamic_inst_t *d_instr, uint32 proc ) {
  DEBUG_OUT("error: flat_rf_t: using renaming interfaces on flat register file (waitResult).\n");
}

//***************************************************************************
bool    flat_rf_t::regsAvailable( uint32 proc ) {
  DEBUG_OUT("error: flat_rf_t: using renaming interfaces on flat register file (regsAvailable).\n");
  return true;
}

//***************************************************************************
void    flat_rf_t::addFreelist( reg_id_t &rid ) {
  DEBUG_OUT("error: flat_rf_t: using renaming interfaces on flat register file (addFreelist).\n");
}

//***************************************************************************
half_t  flat_rf_t::renameCount( reg_id_t &rid, uint32 proc ) {
  DEBUG_OUT("error: flat_rf_t: using renaming interfaces on flat register file (renameCount).\n");
  return 0;
}

//***************************************************************************
ireg_t  flat_rf_t::readInt64( reg_id_t &rid, uint32 proc )
{
  return (m_rf->getInt( getLogical(rid, proc) ));
}

//***************************************************************************
void    flat_rf_t::writeInt64( reg_id_t &rid, ireg_t value, uint32 proc )
{
  m_rf->setInt( getLogical(rid, proc), value );
}

//***************************************************************************
// IMPORTANT - since flat_rf_t is inherited from abstract_rf_t, if we don't
//          have this version, it will call abstract_rf_t's version, and since its m_last_writer
//          is never set we will SEG_FAULT when calling its getLogical(rid, proc)
void    flat_rf_t::setDependence( reg_id_t &rid, flow_inst_t *inst, uint32 proc )
{
  #ifdef DEBUG_FLATARF
  DEBUG_OUT("flat_rf_t:setDependence (logical proc version) BEGIN\n");
  #endif
  
  half_t  logical = getLogical(rid, proc);              

  // if already equal, return without making modification
  if ( m_last_writer[logical] == inst )
    return;

  increment( inst );
  decrement( m_last_writer[logical] );
  m_last_writer[logical] = inst;

    #ifdef DEBUG_FLATARF
  DEBUG_OUT("flat_rf_t:setDependence (logical proc version) END\n");
  #endif
}

//***************************************************************************
flow_inst_t *flat_rf_t::getDependence( reg_id_t &rid, uint32 proc )
{
  return (m_last_writer[getLogical(rid, proc)]);       
}

/*------------------------------------------------------------------------*/
/* Windowed Integer Register File                                         */
/*------------------------------------------------------------------------*/

//***************************************************************************
void    flat_int_t::initializeState( pstate_t *state, uint32 proc )
{
  ASSERT(proc < CONFIG_LOGICAL_PER_PHY_PROC);

  // initialize all of the physical registers that are mapped
  for (uint32 cwp = 0; cwp < NWINDOWS; cwp++ ) {
    // compare the IN and LOCAL registers
    for (int reg = 31; reg >= 16; reg --) {
      ireg_t simreg = state->getIntWp( reg, cwp, proc ); 
      half_t flatreg = reg_box_t::iregWindowMap( reg, cwp ); 
      m_rf->setInt( flatreg, simreg );
    }
  }
}

//**************************************************************************
half_t    flat_int_t::getLogical( reg_id_t &rid, uint32 proc )
{
  
    #ifdef DEBUG_FLATARF
  DEBUG_OUT("flat_int_t:getLogical BEGIN\n");
  #endif

  ASSERT(proc < CONFIG_LOGICAL_PER_PHY_PROC);

  half_t logical = reg_box_t::iregWindowMap( rid.getVanilla(),
                                             rid.getVanillaState() );


  #ifdef DEBUG_FLATARF
  DEBUG_OUT("flat_int_t:getLogical END\n");
  #endif

  return (logical);
}

//***************************************************************************
void      flat_int_t::check( reg_id_t &rid, pstate_t *state,
                             check_result_t *result, uint32 proc )
{
  ASSERT(proc < CONFIG_LOGICAL_PER_PHY_PROC);

  // get windowed integer registers
  ireg_t simreg = state->getIntWp( rid.getVanilla(), rid.getVanillaState(), proc );
  half_t logical= getLogical( rid, proc );              
  if (result->update_only) {
    ASSERT( logical != PSEQ_INVALID_REG );
    m_rf->setInt( logical, simreg );
  } else {
    ireg_t myreg = m_rf->getInt( logical );
    if (myreg != simreg) {
      if (result->verbose)
        DEBUG_OUT("patch  local cwp:%d reg:%d flat %d -- 0x%0llx 0x%0llx\n",
                  rid.getVanillaState(), rid.getVanilla(), logical,
                  myreg, simreg);
      result->perfect_check = false;
      m_rf->setInt( logical, simreg );
    }
  }
}

/*------------------------------------------------------------------------*/
/* Global Integer Register File                                           */
/*------------------------------------------------------------------------*/

//**************************************************************************
void      flat_int_global_t::initializeState( pstate_t *state, uint32 proc )
{
  ASSERT(proc < CONFIG_LOGICAL_PER_PHY_PROC);

  half_t flatreg;
  // allocate all 4 global registers sets
  for (uint16 gset = REG_GLOBAL_NORM; gset <= REG_GLOBAL_INT; gset++) {
    for (int reg = 1; reg < 8; reg ++) {
      ireg_t simreg = state->getIntGlobal( reg, gset, proc );
      flatreg = reg_box_t::iregGlobalMap( reg, gset );
      m_rf->setInt( flatreg, simreg );
    }
  }

  // special case for global zero register (%g0)
  flatreg = reg_box_t::iregGlobalMap( 0, 0 );
  m_rf->setInt( flatreg, 0 );
}

//***************************************************************************
half_t    flat_int_global_t::getLogical( reg_id_t &rid, uint32 proc )
{
  ASSERT(proc < CONFIG_LOGICAL_PER_PHY_PROC);

  half_t logical = reg_box_t::iregGlobalMap( rid.getVanilla(),
                                             rid.getVanillaState() );
  return (logical);

}

//***************************************************************************
void      flat_int_global_t::check( reg_id_t &rid, pstate_t *state,
                                    check_result_t *result, uint32 proc )
{
  ASSERT(proc < CONFIG_LOGICAL_PER_PHY_PROC);

  // if the zero register (%g0) ... don't check
  if ( rid.getVanilla() == 0 ) {
    return;
  }
  
  // get global integer registers
  half_t logical = getLogical( rid, proc );             
  ireg_t simreg  = state->getIntGlobal( rid.getVanilla(),
                                        rid.getVanillaState(), proc );
  if (result->update_only) {
    ASSERT( logical != PSEQ_INVALID_REG );
    m_rf->setInt( logical, simreg );
  } else {
    ireg_t myreg   = m_rf->getInt( logical );
    if (myreg != simreg) {
      if (result->verbose)
        DEBUG_OUT("patch  gset:%d reg:%d flat %d -- 0x%0llx 0x%0llx\n",
                  rid.getVanillaState(), rid.getVanilla(), logical,
                  myreg, simreg);
      m_rf->setInt( logical, simreg );
      result->perfect_check = false;
    }
  }
}

//***************************************************************************
void     flat_int_global_t::writeInt64( reg_id_t &rid, ireg_t value, uint32 proc )
{
  ASSERT(proc < CONFIG_LOGICAL_PER_PHY_PROC);

  // writes to %g0 do nothing
  if (rid.getVanilla() == 0)
    return;
  m_rf->setInt( getLogical( rid, proc ), value );
}

/*------------------------------------------------------------------------*/
/* Single Precision Floating-Point Register File                          */
/*------------------------------------------------------------------------*/

//***************************************************************************
void      flat_single_t::initializeState( pstate_t *state, uint32 proc )
{
  // compare the fp registers
  for (uint32 freg = 0; freg < MAX_FLOAT_REGS; freg++) {
    // get the double value from the floating point rf
    uint32 reg = freg * 2;
    freg_t fsimreg = state->getDouble(reg, proc);          
    uint32 *int_ptr = (uint32 *) &fsimreg;
    ireg_t simupper32 = 0;
    ireg_t simlower32 = 0;
    
    // ENDIAN_MATTERS
    if (ENDIAN_MATCHES) {
      // check target lower 32 bits
      simupper32 = int_ptr[0];
      // check target lower 32 bits
      simlower32 = int_ptr[1];
    } else {
      // check target lower 32 bits
      simupper32 = int_ptr[1];
      // check target lower 32 bits
      simlower32 = int_ptr[0];
    }
    
    // set upper 32 bits
    m_rf->setInt( reg, simupper32 );
    
    // set lower 32 bits
    reg++;
    m_rf->setInt( reg, simlower32 );
  }
}

//***************************************************************************
half_t    flat_single_t::getLogical( reg_id_t &rid, uint32 proc )
{
  //proc arg is not used
  half_t logical = rid.getVanilla();
  return (logical);
}

//***************************************************************************
void      flat_single_t::check( reg_id_t &rid, pstate_t *state,
                               check_result_t *result, uint32 proc )
{
  half_t  reg     = rid.getVanilla();
  half_t  logical = getLogical( rid, proc );             
  // get the double value for the register (rounded to the nearest 2)
  freg_t  fsimreg = state->getDouble( (reg / 2)*2, proc );          
  ireg_t  simreg32;
  uint32 *int_ptr = (uint32 *) &fsimreg;

  // ENDIAN_MATTERS
  if (ENDIAN_MATCHES) {
    if ( reg % 2 == 1 ) {
      // check target upper 32 bits
      simreg32 = int_ptr[1];
    } else {
      // check target lower 32 bits
      simreg32 = int_ptr[0];
    }
  } else {
    if ( reg % 2 == 1 ) {
      // check target upper 32 bits
      simreg32 = int_ptr[0];
    } else {
      // check target lower 32 bits
      simreg32 = int_ptr[1];
    }
  }

  if (result->update_only) {
    ASSERT( logical != PSEQ_INVALID_REG );
    m_rf->setInt( logical, simreg32 );
  } else {
    ireg_t myreg   = m_rf->getInt( logical );
    if (simreg32 != myreg) {
      if (result->verbose) {
        const char *str;
        if (reg % 2 == 1)
          str = "(hi)";
        else
          str = "(lo)";
        DEBUG_OUT("patch  float reg:%d %s -- 0x%0x 0x%0llx 0x%llx\n",
                  reg, str,
                  logical, myreg, simreg32);
      }
      result->perfect_check = false;
      m_rf->setInt( logical, simreg32 );
    }
  }
}

//***************************************************************************
float32   flat_single_t::readFloat32( reg_id_t &rid, uint32 proc )
{
  return (m_rf->getFloat( getLogical(rid, proc) ));          
}

//***************************************************************************
void      flat_single_t::writeFloat32( reg_id_t &rid, float32 value, uint32 proc )
{
  m_rf->setFloat( getLogical(rid, proc), value );      
}

/*------------------------------------------------------------------------*/
/* Double Precision Floating-Point Register File                          */
/*------------------------------------------------------------------------*/

//***************************************************************************
void    flat_double_t::initializeState( pstate_t *state )
{
  // initialization is handled by the flat_single_t class
}

//***************************************************************************
void    flat_double_t::setDependence( reg_id_t &rid, flow_inst_t *inst, uint32 proc )
{
  half_t logical = getLogical(rid, proc);       

  if ( m_last_writer[logical] != inst ) {
    increment( inst );
    decrement( m_last_writer[logical] );
    m_last_writer[logical] = inst;
  }

  if ( m_last_writer[logical + 1] != inst ) {
    increment( inst );
    decrement( m_last_writer[logical + 1] );
    m_last_writer[logical + 1] = inst;
  }
}

//***************************************************************************
flow_inst_t *flat_double_t::getDependence( reg_id_t &rid, uint32 proc )
{
  // CM FIX: only returns the dependence on the first logical register, 
  // (not both as we should). I'm willing to live with this as it simplifies
  // the interface (a lot).
  // all double    -> double dependences are preserved
  // all single[0] -> double dependences are preserved
  // all single[1] -> double dependences are lost
  return (m_last_writer[getLogical(rid, proc)]);            
}

//***************************************************************************
half_t    flat_double_t::getLogical( reg_id_t &rid, uint32 proc )
{
  //proc arg not used
  half_t logical = rid.getVanilla();
  // and logical + 1
  return (logical);
}


//***************************************************************************
void      flat_double_t::check( reg_id_t &rid, pstate_t *state,
                                check_result_t *result, uint32 proc )
{
  // get the double value from the floating point rf
  freg_t fsimreg = state->getDouble( rid.getVanilla(), proc );          
  uint32 *int_ptr = (uint32 *) &fsimreg;
  ireg_t simupper32 = 0;
  ireg_t simlower32 = 0;

  // ENDIAN_MATTERS
  if (ENDIAN_MATCHES) {
    // check target upper 32 bits
    simupper32 = int_ptr[0];
    // check target lower 32 bits
    simlower32 = int_ptr[1];
  } else {
    // check target upper 32 bits
    simupper32 = int_ptr[1];
    // check target lower 32 bits
    simlower32 = int_ptr[0];
  }

  // get the physical registers
  half_t reg1 = getLogical(rid, proc);           
  half_t reg2 = reg1 + 1;

  if (result->update_only) {
    ASSERT( reg1 != PSEQ_INVALID_REG );
    ASSERT( reg2 != PSEQ_INVALID_REG );
    m_rf->setInt( reg1, simupper32 );
    m_rf->setInt( reg2, simlower32 );
  } else {
    ireg_t myreg   = m_rf->getInt( reg1 );
    if (myreg != simupper32) {
      if (result->verbose) {
        DEBUG_OUT("patch  float reg:%d (hi) -- 0x%0x 0x%0x 0x%llx 0x%x\n",
                  rid.getVanilla(), reg1, myreg, simupper32);
      }
      result->perfect_check = false;
      m_rf->setInt( reg1, simupper32 );
    }
    
    myreg  = m_rf->getInt( reg2 );
    if (myreg != simlower32) {
      if (result->verbose) {
        DEBUG_OUT("patch  float reg:%d (lo) -- 0x%0x 0x%0x 0x%llx 0x%x\n",
                  rid.getVanilla(), reg2, myreg, simlower32);
      }
      result->perfect_check = false;
      m_rf->setInt( reg2, simlower32 );
    }
  }
}

//***************************************************************************
ireg_t    flat_double_t::readInt64( reg_id_t &rid, uint32 proc )
{
  half_t reg1 = getLogical(rid, proc);                
  half_t reg2 = reg1 + 1;
  
  // read from 2 logical registers (both 32 bits)
  ireg_t  value = ( (m_rf->getInt( reg1 ) << 32) |
                    (m_rf->getInt( reg2 ) & 0xffffffff) );
  return (value);
}
//***************************************************************************
void      flat_double_t::writeInt64( reg_id_t &rid, ireg_t value, uint32 proc )
{
  half_t reg1 = getLogical(rid, proc);              
  half_t reg2 = reg1 + 1;
  uint32 *ival = (uint32 *) &value;
    
  // ENDIAN_MATTERS
  if (ENDIAN_MATCHES) {
    // reg1 is high order bits
    m_rf->setInt( reg1, ival[0] );
    // reg2 is low  order bits
    m_rf->setInt( reg2, ival[1] );
  } else {
    // reg1 is high order bits
    m_rf->setInt( reg1, ival[1] );
    // reg2 is low  order bits
    m_rf->setInt( reg2, ival[0] );
  }
}

//***************************************************************************
float64   flat_double_t::readFloat64( reg_id_t &rid, uint32 proc )
{
  half_t reg1 = getLogical( rid, proc );             
  half_t reg2 = reg1 + 1;
  // read from 2 32-bit registers
  ireg_t  value = ( (m_rf->getInt( reg1 ) << 32) |
                    (m_rf->getInt( reg2 ) & 0xffffffff) );
  
  /*
  DEBUG_OUT("rf64 0x%0llx 0x%0llx = 0x%0llx\n",
         m_rf->getInt( reg1 ),
         m_rf->getInt( reg2 ),
         value);
  */

  // change a 64 bit integer into a float, with out converting it ...
  float64 fval  = *((float64 *) &value);
  return (fval);
}

//***************************************************************************
void      flat_double_t::writeFloat64( reg_id_t &rid, float64 value, uint32 proc )
{
  half_t reg1 = getLogical( rid, proc );             
  half_t reg2 = reg1 + 1;
  uint32 *ival = (uint32 *) &value;

  // ENDIAN_MATTERS
  if (ENDIAN_MATCHES) {
    // reg1 is high order bits
    m_rf->setInt( reg1, ival[0] );
    // reg2 is low  order bits
    m_rf->setInt( reg2, ival[1] );
  } else {
    // reg1 is high order bits
    m_rf->setInt( reg1, ival[1] );
    // reg2 is low  order bits
    m_rf->setInt( reg2, ival[0] );
  }
}

/*------------------------------------------------------------------------*/
/* Control Register File                                                  */
/*------------------------------------------------------------------------*/

//***************************************************************************

//**************************************************************************
void      flat_control_t::initializeState( pstate_t *state, uint32 proc )
{
  ASSERT(proc < CONFIG_LOGICAL_PER_PHY_PROC);

  // read and update all the control registers (that are valid)
  for (uint32 reg = 0; reg < CONTROL_NUM_CONTROL_PHYS; reg++) {
    // read simics's register
    ireg_t simreg = state->getControl( reg, proc );
    // copy into our registers
    m_rf->setInt( reg, simreg );
  }
}

//***************************************************************************
half_t    flat_control_t::getLogical( reg_id_t &rid, uint32 proc )
{
  //proc arg not used
  if ( rid.getRtype() == RID_CC ) {
    switch( rid.getVanilla() ) {
    case REG_CC_CCR:
      return CONTROL_CCR;
      // FIXFIXFIX
      // FSR has 4 distinct fields which represent FCC0-FCC3
    case REG_CC_FCC0:
      return CONTROL_FSR;
    case REG_CC_FCC1:
      return CONTROL_FSR;
    case REG_CC_FCC2:
      return CONTROL_FSR;
    case REG_CC_FCC3:
      return CONTROL_FSR;
    default:
      ERROR_OUT("error: flat_control_t: getLogical: unknown RID_CONTROL type (%d)\n", rid.getVanilla());
    }
  }
  half_t logical = rid.getVanilla();
  return (logical);
}

//***************************************************************************
ireg_t  flat_control_t::readInt64( reg_id_t &rid, uint32 proc )
{
  if ( rid.getRtype() == RID_CC ) {
    uint64 fsr; 
    switch( rid.getVanilla() ) {
    case REG_CC_CCR:
      // CC is normally implemented
      break;

    case REG_CC_FCC0:
      fsr = m_rf->getInt( CONTROL_FSR );
      return ( maskBits64(fsr, 10, 11) );

    case REG_CC_FCC1:
      fsr = m_rf->getInt( CONTROL_FSR );
      return ( maskBits64(fsr, 32, 33) );

    case REG_CC_FCC2:
      fsr = m_rf->getInt( CONTROL_FSR );
      return ( maskBits64(fsr, 34, 35) );

    case REG_CC_FCC3:
      fsr = m_rf->getInt( CONTROL_FSR );
      return ( maskBits64(fsr, 36, 37) );

    default:
      ERROR_OUT("error: flat_control_t: getLogical: unknown RID_CONTROL type (%d)\n", rid.getVanilla());
    }
  }
  return (flat_rf_t::readInt64( rid, proc ));       
}

//***************************************************************************
void    flat_control_t::writeInt64( reg_id_t &rid, ireg_t value, uint32 proc )
{
  if ( rid.getRtype() == RID_CC ) {
    uint64 fsr; 
    switch( rid.getVanilla() ) {
    case REG_CC_CCR:
      // CC is normally implemented
      break;

    case REG_CC_FCC0:
      fsr  = m_rf->getInt( CONTROL_FSR ) & ~(0x3ULL << 10);
      fsr |= (value & 0x3) << 10;
      return;

    case REG_CC_FCC1:
      fsr  = m_rf->getInt( CONTROL_FSR ) & ~(0x3ULL << 32);
      fsr |= (value & 0x3) << 32;
      return;

    case REG_CC_FCC2:
      fsr  = m_rf->getInt( CONTROL_FSR ) & ~(0x3ULL << 34);
      fsr |= (value & 0x3) << 34;
      return;

    case REG_CC_FCC3:
      fsr  = m_rf->getInt( CONTROL_FSR ) & ~(0x3ULL << 36);
      fsr |= (value & 0x3) << 36;
      return;

    default:
      ERROR_OUT("error: flat_control_t: getLogical: unknown RID_CONTROL type (%d)\n", rid.getVanilla());
    }
  }
  flat_rf_t::writeInt64( rid, value, proc );        
}

//***************************************************************************
void      flat_control_t::check( reg_id_t &rid, pstate_t *state,
                                 check_result_t *result, uint32 proc )
{
  ASSERT(proc < CONFIG_LOGICAL_PER_PHY_PROC);

  int    reg    = getLogical( rid, proc);       
  ireg_t simreg = state->getControl( reg, proc );
  ireg_t myreg  = m_rf->getInt( reg );
  if ( simreg != myreg ) {
    // certain registers are not maintained, or are register renamed
    if ( !(reg == CONTROL_PC ||
           reg == CONTROL_NPC ||
           reg == CONTROL_TICK ||
           reg == CONTROL_TICK_CMPR) ) {
      if (result->verbose)
        DEBUG_OUT( "patch %%control %s -- 0x%0llx 0x%0llx\n",
                   pstate_t::control_reg_menomic[reg],
                   myreg, simreg );
      result->perfect_check = false;
    }
    m_rf->setInt( reg, simreg );
  }
}

//***************************************************************************
void flat_control_t::initializeControl( reg_id_t &rid, uint32 proc )
{
  // do nothing: the default routine in abstract_rf_t prints an error
  //             message. It is OK to call this routine on a flat control RF.
  //             As flat RF doesn't have to do any renaming, do nothing.
}

//***************************************************************************
void flat_control_t::finalizeControl( reg_id_t &rid, uint32 proc )
{
  // do nothing: see initializeControl()
}


/*------------------------------------------------------------------------*/
/* Container Register File                                                */
/*------------------------------------------------------------------------*/

//***************************************************************************
half_t  flat_container_t::getLogical( reg_id_t &rid, uint32 proc ) {
  //proc arg not used
  DEBUG_OUT("warning: flat_container_t getLogical() doesn't really apply to container class\n");
  half_t logical = rid.getPhysical();
  return (logical);
}

/** This function copies the dispatch registers */
//***************************************************************************
void    flat_container_t::copyDispatchRids( reg_id_t &container_rid,
                                            reg_id_t *to_rid )
{
  rid_container_t rtype = (rid_container_t) container_rid.getVanillaState();
  uint32 basevanilla    = container_rid.getVanilla();

  // check each register identifier in the array
  for (uint32 i = 0; i < m_dispatch_size[rtype]; i++) {
    // DEBUG_OUT("check: %d %d %d\n", rtype, index, i);
    reg_id_t &dispatch = m_dispatch_map[rtype][i];
    abstract_rf_t *arf = dispatch.getARF();
    reg_id_t &subid    = to_rid[i];
    
    subid.setRtype( dispatch.getRtype() );
    subid.setVanilla( basevanilla + dispatch.getVanilla() );
    subid.setARF( arf );
  }
}

//***************************************************************************
void    flat_container_t::check( reg_id_t &rid, pstate_t *state, check_result_t *result, uint32 proc )
{
  //DEBUG_OUT("container: checkerino\n");

  // index contains which register array in m_rename_map to use
  rid_container_t rtype = (rid_container_t) rid.getVanillaState();
  reg_id_t        subid[FLATARF_MAX_DISPATCH_SIZE];
  copyDispatchRids( rid, subid );

  // check each register identifier in the array
  for (uint32 i = 0; i < m_dispatch_size[rtype]; i++) {
    // DEBUG_OUT("check: %d %d %d\n", rtype, index, i);
    subid[i].getARF()->check( subid[i], state, result, proc );       
  }
  //DEBUG_OUT("end check\n");
}

//***************************************************************************
void    flat_container_t::print( reg_id_t &rid, uint32 proc )
{
  rid_container_t rtype = (rid_container_t) rid.getVanillaState();
  reg_id_t        subid[FLATARF_MAX_DISPATCH_SIZE];
  copyDispatchRids( rid, subid );

  // print each register identifier in the array
  //DEBUG_OUT( "container:%d type:%d\n", index, rtype );
  for (uint32 i = 0; i < m_dispatch_size[rtype]; i++) {
    // DEBUG_OUT("print: %d %d %d\n", rtype, index, i);
    subid[i].getARF()->print( subid[i], proc );          
  }
}

//***************************************************************************
void    flat_container_t::openRegisterType( rid_container_t rtype, int32 numElements )
{
  //DEBUG_OUT("open reg\n");
  if (m_cur_type != CONTAINER_NUM_CONTAINER_TYPES) {
    ERROR_OUT("container_t: open register called while register already open.\n");
    SIM_HALT;
  }
  m_cur_type             = rtype;
  m_cur_element          = 0;
  m_max_elements         = numElements;
  m_dispatch_size[rtype] = numElements;
  if (numElements > m_max_dispatch_size) {
    m_max_dispatch_size = numElements;
    if (m_max_dispatch_size >= FLATARF_MAX_DISPATCH_SIZE) {
      ERROR_OUT("error: flat_container: openRegisterType: please increase the constant MAX_DISPATCH_SIZE to be %d\n", m_max_dispatch_size );
      SYSTEM_EXIT;
    }
  }
  m_dispatch_map[m_cur_type] = new reg_id_t[numElements];
}

//***************************************************************************
void    flat_container_t::addRegister( rid_type_t regtype, int32 offset, abstract_rf_t *arf )
{
  //DEBUG_OUT("add reg\n");
  if (m_cur_element == -1) {
    ERROR_OUT("container_t: addRegister called before open register.\n");
    SIM_HALT;
  }
  if (m_cur_element >= m_max_elements) {
    ERROR_OUT("container_t: addRegister called too many times %d (%d).\n",
              m_cur_element, m_max_elements);
    SIM_HALT;
  }
  m_dispatch_map[m_cur_type][m_cur_element].setRtype( regtype );
  m_dispatch_map[m_cur_type][m_cur_element].setVanilla( offset );
  m_dispatch_map[m_cur_type][m_cur_element].setARF( arf );
  
  m_cur_element = m_cur_element + 1;
}

//***************************************************************************
void    flat_container_t::closeRegisterType( void )
{
  //DEBUG_OUT("close reg\n");
  if (m_cur_element != m_max_elements) {
    ERROR_OUT("container_t: addRegister called too few times %d (%d).\n",
              m_cur_element, m_max_elements);
    SIM_HALT;
  }

#if 0
  for (uint32 i=0; i < m_dispatch_size[m_cur_type]; i++) {
    reg_id_t &rid = m_dispatch_map[m_cur_type][i];
    rid.getARF()->print( rid );
  }
#endif
  m_cur_type     = CONTAINER_NUM_CONTAINER_TYPES;
  m_cur_element  = -1;
  m_max_elements = -1;
}

//***************************************************************************
ireg_t  flat_container_t::readInt64( reg_id_t &rid, uint32 proc )
{
  //DEBUG_OUT("container: readint\n");
  // Vanilla is used in at retirement to select which register to write to
  uint32 selector       = rid.getSelector();
  rid_container_t rtype = (rid_container_t) rid.getVanillaState();
  reg_id_t        subid[FLATARF_MAX_DISPATCH_SIZE];
  copyDispatchRids( rid, subid );
  
#if 0
  DEBUG_OUT("index=%d\n", index);
  DEBUG_OUT("rtype=%d\n", rtype);
  DEBUG_OUT("selector=%d\n", selector);
  DEBUG_OUT("size=%d\n", m_dispatch_size[rtype] );
#endif
  if ( selector >= m_dispatch_size[rtype] ) {
    DEBUG_OUT( "warning: readInt64 selector out of range: %d\n",
               selector );
    return (0);
  }

  return subid[selector].getARF()->readInt64( subid[selector], proc );     
}

//***************************************************************************
void    flat_container_t::writeInt64( reg_id_t &rid, ireg_t value, uint32 proc )
{
  //DEBUG_OUT("container: writeint\n");
  // Vanilla is used in at retirement to select which register to write to
  uint32 selector       = rid.getSelector();
  rid_container_t rtype = (rid_container_t) rid.getVanillaState();
  reg_id_t        subid[FLATARF_MAX_DISPATCH_SIZE];
  copyDispatchRids( rid, subid );

  if ( selector >= m_dispatch_size[rtype] ) {
    ERROR_OUT( "warning: writeInt64 selector out of range: %d\n",
               selector );
    return;
  }

  subid[selector].getARF()->writeInt64( subid[selector], value, proc );      
}

//***************************************************************************
void    flat_container_t::initializeControl( reg_id_t &rid, uint32 proc )
{
  //DEBUG_OUT("container: init control\n");
  // Vanilla is used in at retirement to select which register to write to
  uint32 selector       = rid.getSelector();
  rid_container_t rtype = (rid_container_t) rid.getVanillaState();
  reg_id_t        subid[FLATARF_MAX_DISPATCH_SIZE];
  copyDispatchRids( rid, subid );

  if ( selector >= m_dispatch_size[rtype] ) {
    DEBUG_OUT( "warning: initializeControl selector out of range: %d\n",
               selector );
    return;
  }

  subid[selector].getARF()->initializeControl( subid[selector], proc );
}

//***************************************************************************
void    flat_container_t::finalizeControl( reg_id_t &rid, uint32 proc )
{
  //DEBUG_OUT("container: finalize control\n");
  // Vanilla is used in at retirement to select which register to write to
  uint32 selector       = rid.getSelector();
  rid_container_t rtype = (rid_container_t) rid.getVanillaState();
  reg_id_t        subid[FLATARF_MAX_DISPATCH_SIZE];
  copyDispatchRids( rid, subid );

  if ( selector >= m_dispatch_size[rtype] ) {
    DEBUG_OUT( "warning: finalize control selector out of range: %d\n",
               selector );
    return;
  }

  subid[selector].getARF()->finalizeControl( subid[selector], proc );
  //DEBUG_OUT("container: end finalize control\n");
}

//***************************************************************************
void         flat_container_t::setDependence( reg_id_t &rid,
                                              flow_inst_t *inst, uint32 proc )
{
  // for each register, set the dependence
  rid_container_t rtype = (rid_container_t) rid.getVanillaState();
  reg_id_t        subid[FLATARF_MAX_DISPATCH_SIZE];
  copyDispatchRids( rid, subid );
  
  for (uint32 i = 0; i < m_dispatch_size[rtype]; i++) {
    subid[i].getARF()->setDependence( subid[i], inst, proc );     
  }
}

//***************************************************************************
flow_inst_t *flat_container_t::getDependence( reg_id_t &rid, uint32 proc )
{
  // since there is no way to return multiple dependencies, just
  // pick one. I have no problem with this approximation as most
  // container stuff is a big hack anyway.
  reg_id_t        subid[FLATARF_MAX_DISPATCH_SIZE];
  copyDispatchRids( rid, subid );
  
  return (subid[0].getARF()->getDependence( subid[0], proc ) );     
}

/*------------------------------------------------------------------------*/
/* Private methods                                                        */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Static methods                                                         */
/*------------------------------------------------------------------------*/

static void decrement( flow_inst_t *f ) 
{
  if (f == NULL)
    return;
  f->decrementRefCount();
  if (f->getRefCount() == 0) {
    delete f;
  }
}

static void increment( flow_inst_t *f ) 
{
  if (f == NULL)
    return;
  f->incrementRefCount();
}

/*------------------------------------------------------------------------*/
/* Global functions                                                       */
/*------------------------------------------------------------------------*/


/** [Memo].
 *  [Internal Documentation]
 */
//**************************************************************************
