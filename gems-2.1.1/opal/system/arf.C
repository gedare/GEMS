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
 * FileName:  arf.C
 * Synopsis:  Implements all abstract register file classes.
 * Author:    cmauer
 * Version:   $Id$
 */

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include "hfa.h"
#include "hfacore.h"
#include "checkresult.h"            // need declaration of check_result_t
#include "flow.h"
#include "arf.h"

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Variable declarations                                                  */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Forward declarations                                                   */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Abstract Register File                                                 */
/*------------------------------------------------------------------------*/

//***************************************************************************
void    abstract_rf_t::initialize( void )
{
  // initialize this register file: nothing to do!
}

void    abstract_rf_t::initialize(uint32 proc )
{
  // initialize this register file: nothing to do!
}


//***************************************************************************
bool    abstract_rf_t::allocateRegister( reg_id_t &rid, uint32 proc )
{
  #ifdef DEBUG_ARF
  DEBUG_OUT("abstract_rf_t:allocateRegister BEGIN with proc[%d] regtype[%d]\n",proc,rid.getRtype());
  #endif

  // allocate a new physical register ... set it equal
  half_t logical  = getLogical( rid, proc );                
  
  #ifdef DEBUG_REG
     DEBUG_OUT("abstract_rf_t::allocateRegister logical[ %d ]", logical);
  #endif
  half_t physical = m_decode_map->regAllocate(logical, proc);   
  #ifdef DEBUG_REG
      DEBUG_OUT(" physical[ %d ]\n", physical);
  #endif

  if (physical == PSEQ_INVALID_REG) {
    DEBUG_OUT("allocateRegister: unable to allocate register\n");
    print( rid, proc );       
    return (false);
  }
  
  // write the mapping from logical to physical
  rid.setPhysical( physical );
  m_decode_map->setMapping( logical, physical, proc );        

  #ifdef DEBUG_ARF
  DEBUG_OUT("abstract_rf_t:allocateRegister END with proc[%d] logical[%d] physical[%d] regtype[%d]\n",proc,
                logical, physical,rid.getRtype() );
  #endif
  return (true);
}

//***************************************************************************
bool    abstract_rf_t::freeRegister( reg_id_t &rid, uint32 proc )
{
  half_t physical = rid.getPhysical();
  if (physical == PSEQ_INVALID_REG) {
    DEBUG_OUT("freeRegister: error: unable to free INVALID register\n");
    return (false);
  }

  half_t logical = getLogical( rid, proc );             

  #ifdef DEBUG_RETIRE
  DEBUG_OUT("abstract_rf_t:freeRegister BEGIN calling regFree with regtype[%d] logical[%d] physical[%d] proc[%d]\n",
                    rid.getRtype(), logical,physical, proc);
  #endif

  m_decode_map->regFree( logical, physical, proc );      

  #ifdef DEBUG_RETIRE
  DEBUG_OUT("abstract_rf_t:freeRegister END calling regFree with regtype[%d] logical[%d] physical[%d] proc[%d]\n",rid.getRtype(), logical, physical, proc);
  #endif

  rid.setPhysical( PSEQ_INVALID_REG );
  return (true);
}

//***************************************************************************
void    abstract_rf_t::readDecodeMap( reg_id_t &rid, uint32 proc )
{
  DEBUG_OUT("readDecodeMap: called for undefined register type: %d\n",
            rid.getRtype() );
  return;
}

//***************************************************************************
void    abstract_rf_t::writeRetireMap( reg_id_t &rid, uint32 proc )
{
  #ifdef DEBUG_ARF
     DEBUG_OUT("abstract_rf_t:writeRetireMap BEGIN with proc[%d]\n",proc);
  #endif
  half_t logical  = getLogical( rid, proc );      
  half_t physical = rid.getPhysical();
  m_retire_map->setMapping( logical, physical, proc );
  #ifdef DEBUG_ARF
     DEBUG_OUT("abstract_rf_t:writeRetireMap END with proc[%d]\n",proc);
  #endif
}

//***************************************************************************
void    abstract_rf_t::writeDecodeMap( reg_id_t &rid, uint32 proc )
{
  #ifdef DEBUG_ARF
     DEBUG_OUT("abstract_rf_t:writeDecodeMap BEGIN with proc[%d]\n",proc);
  #endif
  half_t logical  = getLogical( rid, proc );             
  half_t physical = rid.getPhysical();
  m_decode_map->setMapping( logical, physical, proc );
  #ifdef DEBUG_ARF
     DEBUG_OUT("abstract_rf_t:writeDecodeMap END with proc[%d]\n",proc);
  #endif
}

//***************************************************************************
bool    abstract_rf_t::isReady( reg_id_t &rid, uint32 proc )
{
   #ifdef DEBUG_REG
        DEBUG_OUT("abstract_rf_t: isReady BEGIN\n");
  #endif
  // zero %g0, and unmapped registers are always ready
  if (rid.isZero())
    return (true);
  
  half_t  physical = rid.getPhysical();
  ASSERT( physical != PSEQ_INVALID_REG );
  #ifdef DEBUG_REG
        DEBUG_OUT("abstract_rf_t: isReady END\n");
  #endif
  return (m_rf->isReady( physical ));
}

//**************************************************************************
void abstract_rf_t::setL2Miss( reg_id_t & rid, uint32 proc ){
  half_t physical = rid.getPhysical();

  if (physical == PSEQ_INVALID_REG) {
      DEBUG_OUT("setL2Miss: error: (abstract_rf) unable to set INVALID register\n");
      ASSERT(0);
  }

  if(physical != PSEQ_INVALID_REG){
    m_rf->setL2Miss(physical, true);
  }
}

//**************************************************************************
void abstract_rf_t::setWriterSeqnum( reg_id_t & rid, uint64 seqnum, uint32 proc ){
  half_t physical = rid.getPhysical();

  if (physical == PSEQ_INVALID_REG) {
    DEBUG_OUT("setWriterSeqnum: error: (abstract_rf) unable to set INVALID register\n");
    ASSERT(0);
  }

  m_rf->setWriterSeqnum( physical, seqnum);
}

//**************************************************************************
uint64 abstract_rf_t::getWriterSeqnum( reg_id_t & rid, uint32 proc ){
  half_t physical = rid.getPhysical();

  if (physical == PSEQ_INVALID_REG) {
    DEBUG_OUT("getWriterSeqnum: error: (abstract_rf) unable to set INVALID register\n");
    ASSERT(0);
  }

  return m_rf->getWriterSeqnum( physical);
}

//**************************************************************************
uint64 abstract_rf_t::getWrittenCycle( reg_id_t & rid, uint32 proc ){
  half_t physical = rid.getPhysical();

  if (physical == PSEQ_INVALID_REG) {
    ERROR_OUT("getWrittenCycle: error: (abstract_rf) unable to set INVALID register\n");
    ASSERT(0);
  }

  return m_rf->getWrittenCycle( physical);
}


//***************************************************************************
bool abstract_rf_t::isL2Miss( reg_id_t & rid, uint32 proc )
{
  half_t  physical = rid.getPhysical();
  ASSERT( physical != PSEQ_INVALID_REG );
  return m_rf->isL2Miss( physical );
}

//***************************************************************************
void abstract_rf_t::incrementL2MissDep( reg_id_t & rid, uint32 proc )
{
   #ifdef DEBUG_REG
     DEBUG_OUT("abstract_rf_t::incrementL2MissDep logical[ %d ] physical[ %d ] cycle[ %lld ]\n", getLogical(rid, proc), rid.getPhysical(), m_rf->getSeq()->getLocalCycle());
       #endif
  half_t  physical = rid.getPhysical();
  ASSERT( physical != PSEQ_INVALID_REG );
  m_rf->incrementL2MissDep( physical );
}

//***************************************************************************
void abstract_rf_t::decrementL2MissDep( reg_id_t & rid, uint32 proc )
{
   #ifdef DEBUG_REG
     DEBUG_OUT("abstract_rf_t::decrementL2MissDep logical[ %d ] physical[ %d ] cycle[ %lld ]\n", getLogical(rid, proc), rid.getPhysical(), m_rf->getSeq()->getLocalCycle());
       #endif
  half_t  physical = rid.getPhysical();
  ASSERT( physical != PSEQ_INVALID_REG );
  m_rf->decrementL2MissDep( physical );
}

//***************************************************************************
int abstract_rf_t::getL2MissDep( reg_id_t & rid, uint32 proc )
{
  half_t  physical = rid.getPhysical();
  ASSERT( physical != PSEQ_INVALID_REG );
  return m_rf->getL2MissDep( physical );
}

//***************************************************************************
void    abstract_rf_t::waitResult( reg_id_t &rid, dynamic_inst_t *d_instr, uint32 proc )
{
  half_t  physical = rid.getPhysical();  
  ASSERT( physical != PSEQ_INVALID_REG );

  // wait for this physical register to wake up!
  #ifdef DEBUG_REG
    DEBUG_OUT("abstract_rf_t::waitResult() logical[ %d ] physical[ %d ] seqnum[ %d ]\n", getLogical(rid, proc), physical, d_instr->getSequenceNumber());
  #endif
  m_rf->waitResult( physical, d_instr );
}

//***************************************************************************
bool    abstract_rf_t::regsAvailable( uint32 proc )
{
  return (m_decode_map->countRegRemaining() > m_reserves);
}

//***************************************************************************
void    abstract_rf_t::check( reg_id_t &rid, pstate_t *state,
                              check_result_t *result, uint32 proc )
{
  // do nothing: default check always succeeds
}

//***************************************************************************
void    abstract_rf_t::addFreelist( reg_id_t &rid )
{
  bool success = m_decode_map->addFreelist( rid.getPhysical() );
  if (!success) {
    SIM_HALT;    
  }
}

/** print out this register mapping */
//***************************************************************************
void    abstract_rf_t::print( reg_id_t &rid, uint32 proc )
{
  const char *rt = reg_id_t::rid_type_menomic( rid.getRtype() );
  DEBUG_OUT( "%.6s (%3.3d : %2.2d) = %3.3d   -->  %3.3d\n",
             rt, rid.getVanilla(), rid.getVanillaState(),
             getLogical(rid, proc), rid.getPhysical() );         
}

//***************************************************************************
half_t abstract_rf_t::renameCount( reg_id_t &rid, uint32 proc )
{
  #ifdef DEBUG_ARF
     DEBUG_OUT("abstract_rf_t:renameCount BEGIN with proc[%d]\n",proc);
  #endif
  half_t logical = getLogical( rid, proc );          

  #ifdef DEBUG_ARF
     DEBUG_OUT("abstract_rf_t:renameCount END with proc[%d]\n",proc);
  #endif
     return ( m_decode_map->countInFlight( logical, proc ) );       
}

//***************************************************************************
// The new getLogical that is used by some register types (RID_INT, RID_INT_GLOBAL)
half_t  abstract_rf_t::getLogical( reg_id_t &rid, uint32 proc )
{
  ERROR_OUT("error: abstract_rf_t: getLogical (logical proc version) called with abstract register!\n");
  SIM_HALT;
  return 0;
}

//***************************************************************************
my_register_t abstract_rf_t::readRegister( reg_id_t &rid, uint32 proc )
{
  // proc arg not used
  // read the register as a 64-bit integer
  my_register_t result;
  result.uint_64 = m_rf->getInt( rid.getPhysical() );
  #ifdef DEBUG_REG
    DEBUG_OUT("\tabstract_rf_t: readRegister vanilla[ %d ] physical[ %d ] value[ 0x%llx ] cycle[ %lld ]\n", rid.getVanilla(), rid.getPhysical(), result.uint_64, m_rf->getSeq()->getLocalCycle());
 #endif
  return result;
}

//***************************************************************************
void abstract_rf_t::writeRegister( reg_id_t &rid, my_register_t value, uint32 proc )
{
  //proc arg not used
  ASSERT( rid.getPhysical() != PSEQ_INVALID_REG );
  m_rf->setWrittenCycle( rid.getPhysical(), m_rf->getSeq()->getLocalCycle() );
  #ifdef DEBUG_REG
    DEBUG_OUT("\tabstract_rf_t: writeRegister vanilla[ %d ] physical[ %d ] value[ 0x%llx ] cycle[ %lld ]\n", rid.getVanilla(), rid.getPhysical(), value.uint_64, m_rf->getSeq()->getLocalCycle());
 #endif
  if(value.uint_64 == -1){
    //    ERROR_OUT("abstract_rf_t: writeRegister vanilla[ %d ] physical[ %d ] value[ %lld ] cycle[ %lld ]\n", rid.getVanilla(), rid.getPhysical(), value.uint_64, m_rf->getSeq()->getLocalCycle());
    //    ASSERT(value.uint_64 != -1);
  }
  m_rf->setInt( rid.getPhysical(), value.uint_64 );  
}

//***************************************************************************
ireg_t  abstract_rf_t::readInt64( reg_id_t &rid, uint32 proc )
{
 #ifdef DEBUG_REG
    DEBUG_OUT("\tabstract_rf_t: readInt64 vanilla[ %d ] physical[ %d ] value[ 0x%llx ] cycle[ %lld ]\n", rid.getVanilla(), rid.getPhysical(), m_rf->getInt( rid.getPhysical() ), m_rf->getSeq()->getLocalCycle());
 #endif
  return (m_rf->getInt( rid.getPhysical() ));
}

/** read this register as a single precision floating point register */
//***************************************************************************
float32 abstract_rf_t::readFloat32( reg_id_t &rid, uint32 proc )
{
  ERROR_OUT("error: trying to read float32 out of a non-32 bit register (type =%d)\n", rid.getRtype() );
  print( rid, proc );         
  return (-1.0);
}

//***************************************************************************
float64 abstract_rf_t::readFloat64( reg_id_t &rid, uint32 proc )
{
  ERROR_OUT("error: trying to read float64 out of non-64 bit register (type=%d) cycle[ %lld ]\n", rid.getRtype(), m_rf->getSeq()->getLocalCycle() );
  print( rid, proc );         
  return (-1.0);
}

//***************************************************************************
void    abstract_rf_t::writeInt64( reg_id_t &rid, ireg_t value, uint32 proc )
{
  m_rf->setWrittenCycle( rid.getPhysical(), m_rf->getSeq()->getLocalCycle() );
  #ifdef DEBUG_REG
     DEBUG_OUT("\tabstract_rf_t: writeInt64 vanilla[ %d ] physical[ %d ] value[ 0x%llx ]\n", rid.getVanilla(), rid.getPhysical(), value);
  #endif
  if(value == -1){
    //    ERROR_OUT("abstract_rf_t: writeInt64 vanilla[ %d ] physical[ %d ] value[ %lld ] cycle[ %lld ]\n", rid.getVanilla(), rid.getPhysical(), value);
    //    ASSERT(value != -1);
  }
  m_rf->setInt( rid.getPhysical(), value );
}

//***************************************************************************
void    abstract_rf_t::writeFloat32( reg_id_t &rid, float32 value, uint32 proc )
{
  ERROR_OUT("P_%d error: trying to write float32 out of non-64 bit register (type=%d)  cycle[ %lld ]\n", m_rf->getSeq()->getID(), rid.getRtype(), m_rf->getSeq()->getLocalCycle() );
}

//***************************************************************************
void    abstract_rf_t::writeFloat64( reg_id_t &rid, float64 value, uint32 proc )
{
  ERROR_OUT("P_%d error: trying to write float64 out of non-64 bit register (type=%d) cycle[ %lld ]\n", m_rf->getSeq()->getID(), rid.getRtype(), m_rf->getSeq()->getLocalCycle() );
}

//***************************************************************************
void    abstract_rf_t::initializeControl( reg_id_t &rid, uint32 proc )
{
  ERROR_OUT("error: trying to initializeControl in a non-control register (type=%d)\n", rid.getRtype() );
}

//***************************************************************************
void    abstract_rf_t::finalizeControl( reg_id_t &rid, uint32 proc )
{
  ERROR_OUT("error: trying to finalizeControl in a non-control register (type=%d)\n", rid.getRtype() );
}

//***************************************************************************
flow_inst_t *abstract_rf_t::getDependence( reg_id_t &rid, uint32 proc )
{
  #ifdef DEBUG_ARF
     DEBUG_OUT("abstract_rf_t:getDependence BEGIN with rid[%d] proc[%d]\n",rid,proc);
  #endif
  // compute a valid logical identifier 
  half_t logical = getLogical( rid, proc );     

  #ifdef DEBUG_ARF
     DEBUG_OUT("abstract_rf_t:getDependence END with rid[%d] proc[%d]\n",rid,proc);
  #endif
  return ( m_last_writer[logical] );
}

//***************************************************************************
void         abstract_rf_t::setDependence( reg_id_t &rid,
                                           flow_inst_t *writer, uint32 proc )
{
 #ifdef DEBUG_ARF
  DEBUG_OUT("abstract_rf_t:setDependence (logical proc version) BEGIN proc[%d]\n", proc);
 #endif

  half_t logical = getLogical( rid, proc );        //call the other version of getLogical()

  assert(m_last_writer != NULL);
  assert(m_last_writer[logical] != NULL);
  assert(writer != NULL);

  // if already equal, return without making modification
  if (m_last_writer[logical] == writer)
    return;
  
  writer->incrementRefCount();

  if (m_last_writer[logical] != NULL) {
     m_last_writer[logical]->decrementRefCount();
    if (m_last_writer[logical]->getRefCount() == 0) {
      delete m_last_writer[logical];
    }
  }
  m_last_writer[logical] = writer;

 #ifdef DEBUG_ARF
  DEBUG_OUT("abstract_rf_t:setDependence (logical proc version) END proc[%d]\n",proc);
 #endif

}


/*------------------------------------------------------------------------*/
/* Int / CC Register File                                                 */
/*------------------------------------------------------------------------*/

//***************************************************************************
void      arf_int_t::initialize( )
{
    #ifdef DEBUG_REG
  DEBUG_OUT("arf_int_t: initialize BEGIN\n");
  #endif

  uint32 reg;
  half_t physreg;
  half_t flatreg;

  // Although we pass in proc, we don't need to use it bc initialize() is called by the arf_int_t object's
  //          constructor.  This means the caller expects us to initialize the m_decode_map for ALL logical
  //           procs, so we need to loop through all logical procs
  for(uint k=0; k < CONFIG_LOGICAL_PER_PHY_PROC; ++k){
    // allocate windowed integer registers
    for (uint16 cwp = 0; cwp < NWINDOWS; cwp++) {
      for (reg = 31; reg >= 8; reg --) {
        flatreg = reg_box_t::iregWindowMap( reg, cwp );   
        physreg = m_decode_map->getMapping( flatreg, k );
        if (physreg == PSEQ_INVALID_REG) {
          // allocate a new physical register for this windowed register
          physreg = m_decode_map->regAllocate( flatreg, k );   
          m_rf->setWrittenCycle( physreg, m_rf->getSeq()->getLocalCycle() );
          //DEBUG_OUT("arf_int_t: initialize vanilla[ %d ] physical[ %d ] value[ %lld ]\n", flatreg, physreg, 0);
          m_rf->setInt( physreg, 0 );
          m_decode_map->setMapping( flatreg, physreg, k );
          m_retire_map->setMapping( flatreg, physreg, k );
        }
        // else already allocated a physical register ... nothing to do
      }
    }
  }   //end for over all logical procs
  #ifdef DEBUG_REG
    DEBUG_OUT("arf_int_t: initialize END\n");
  #endif
}

//***************************************************************************
void      arf_int_t::readDecodeMap( reg_id_t &rid, uint32 proc )
{
  #ifdef DEBUG_REG
    DEBUG_OUT("arf_int_t: readDecodeMap BEGIN\n");
  #endif

  assert(proc < CONFIG_LOGICAL_PER_PHY_PROC);
  half_t logical = reg_box_t::iregWindowMap( rid.getVanilla(),
                                             rid.getVanillaState() );
  rid.setPhysical( m_decode_map->getMapping( logical, proc ) );      
    #ifdef DEBUG_REG
  DEBUG_OUT("arf_int_t: readDecodeMap END\n");
  #endif
}

//***************************************************************************
half_t    arf_int_t::getLogical( reg_id_t &rid, uint32 proc )
{
 #ifdef DEBUG_ARF
  DEBUG_OUT("arf_int_t:getLogical  BEGIN proc[%d]\n",proc);
 #endif

  assert(proc < CONFIG_LOGICAL_PER_PHY_PROC);

  half_t logical = reg_box_t::iregWindowMap( rid.getVanilla(),
                                             rid.getVanillaState() );
  return (logical);

 #ifdef DEBUG_ARF
  DEBUG_OUT("arf_int_t:getLogical END proc[%d]\n",proc);
 #endif
}

//***************************************************************************
void      arf_int_t::check( reg_id_t &rid, pstate_t *state,
                            check_result_t *result, uint32 proc )
{
  assert(proc < CONFIG_LOGICAL_PER_PHY_PROC);

  // get windowed integer registers
  ireg_t simreg = state->getIntWp( rid.getVanilla(), rid.getVanillaState(), proc );
  if (result->update_only) {
    ASSERT( rid.getPhysical() != PSEQ_INVALID_REG );
    m_rf->setWrittenCycle( rid.getPhysical(), m_rf->getSeq()->getLocalCycle() );
    // IMPORTANT: CANNOT get old value here!!, bc may have some unimplemented instr not writing to 
    //     reg
    #ifdef DEBUG_REG
      DEBUG_OUT("\tarf_int_t: check FUNCTIONAL vanilla[ %d ] state[ 0x%x ] physical[ %d ] new_value[ 0x%llx ] \n", rid.getVanilla(), rid.getVanillaState(), rid.getPhysical(), simreg);
  #endif
    if(simreg == -1){
      //DEBUG_OUT("arf_int_t: check vanilla[ %d ] physical[ %d ] value[ %lld ]\n", rid.getVanilla(), rid.getPhysical(), simreg);
      //ASSERT(simreg != -1);
    }
    m_rf->setInt( rid.getPhysical(), simreg );
  } else {
    half_t flatreg = reg_box_t::iregWindowMap( rid.getVanilla(),
                                               rid.getVanillaState() );
    half_t physreg = m_retire_map->getMapping( flatreg, proc );    
    ireg_t myreg   = m_rf->getInt( physreg );
    if (myreg != simreg) {
      if (result->verbose)
        DEBUG_OUT("patch  local cwp:%d reg:%d flat %d -- 0x%0llx 0x%0llx\n",
                  rid.getVanillaState(), rid.getVanilla(), rid.getPhysical(),
                  myreg, simreg);
      result->perfect_check = false;
      m_rf->setWrittenCycle( physreg, m_rf->getSeq()->getLocalCycle() );
      if(myreg == -1 ){
        // ERROR_OUT("arf_int_t: check FAIL vanilla[ %d ] physical[ %d ] new_value[ 0x%llx ] old_value[ 0x%llx ]\n", flatreg, physreg, simreg, myreg);
      }
      if(simreg == -1){
        //DEBUG_OUT("arf_int_t: check vanilla[ %d ] physical[ %d ] value[ %lld ]\n", flatreg, physreg, simreg);
        //        ASSERT( simreg != -1);
      }
 #ifdef DEBUG_REG
      DEBUG_OUT("\tarf_int_t: check FAIL vanilla[ %d ] state[ 0x%x ] physical[ %d ] flatreg[ %d ] physreg[ %d ] new_value[ 0x%llx ] \n", rid.getVanilla(), rid.getVanillaState(), rid.getPhysical(), flatreg, physreg, simreg);
  #endif
      m_rf->setInt( physreg, simreg );
    }
    else{
      //no error
      //DEBUG_OUT("arf_int_t: check PASS vanilla[ %d ] physical[ %d ] new_value[ 0x%llx ] old_value[ 0x%llx ]\n", flatreg, physreg, simreg, myreg);
    }
  }
}

//***************************************************************************
void      arf_int_global_t::initialize( )
{
  uint32 reg;
  half_t physreg;
  half_t flatreg;

  // allocate all 4 global registers sets
  // see arf_int_t's initialize() for comments regarding looping through all logical procs:
  for(uint k=0; k < CONFIG_LOGICAL_PER_PHY_PROC; ++k){
    for (uint16 gset = REG_GLOBAL_NORM; gset <= REG_GLOBAL_INT; gset++) {
      for (reg = 1; reg < 8; reg ++) {
        flatreg = reg_box_t::iregGlobalMap( reg, gset );
        physreg = m_decode_map->getMapping( flatreg, k );
        if (physreg == PSEQ_INVALID_REG) {
          // allocate a new physical register for this global register
          physreg = m_decode_map->regAllocate( flatreg, k );
          m_rf->setWrittenCycle( physreg, m_rf->getSeq()->getLocalCycle() );
          //DEBUG_OUT("arf_int_global_t: initialize vanilla[ %d ] physical[ %d ] value[ %lld ]\n", flatreg, physreg, 0);
          m_rf->setInt( physreg, 0 );
          m_decode_map->setMapping( flatreg, physreg, k );
          m_retire_map->setMapping( flatreg, physreg, k );
        }
      }
    }
  
    // special case for global zero register (%g0)
    flatreg = reg_box_t::iregGlobalMap( 0, 0 );
    physreg = m_decode_map->getMapping( flatreg, k );
    if (physreg == PSEQ_INVALID_REG) {
      physreg = m_decode_map->regAllocate( flatreg, k );
      m_rf->setWrittenCycle( physreg, m_rf->getSeq()->getLocalCycle() );
      //DEBUG_OUT("arf_int_global_t: initialize2 vanilla[ %d ] physical[ %d ] value[ %lld ]\n", flatreg, physreg, 0);
      m_rf->setInt( physreg, 0 );
      flatreg = reg_box_t::iregGlobalMap( 0, 0 );
      m_decode_map->setMapping( flatreg, physreg, k );
      m_retire_map->setMapping( flatreg, physreg, k );
    }
  } //end for loop over all logical procs
}

//***************************************************************************
void     arf_int_global_t::readDecodeMap( reg_id_t &rid, uint32 proc )
{
  assert(proc < CONFIG_LOGICAL_PER_PHY_PROC);

  half_t logical = reg_box_t::iregGlobalMap( rid.getVanilla(),
                                             rid.getVanillaState() );
  rid.setPhysical( m_decode_map->getMapping( logical, proc ) );       
}

//***************************************************************************
half_t   arf_int_global_t::getLogical( reg_id_t &rid, uint32 proc )
{
 #ifdef DEBUG_ARF
  DEBUG_OUT("arf_int_global_t:getLogical BEGIN proc[%d]\n",proc);
 #endif

  half_t logical = reg_box_t::iregGlobalMap( rid.getVanilla(),
                                             rid.getVanillaState() );

 #ifdef DEBUG_ARF
  DEBUG_OUT("arf_int_global_t:getLogical END proc[%d]\n", proc);
 #endif

  return (logical);
}

//***************************************************************************
void     arf_int_global_t::writeInt64( reg_id_t &rid, ireg_t value, uint32 proc )
{
  // writes to %g0 do nothing
  if (rid.getVanilla() == 0)
    return;
  m_rf->setWrittenCycle( rid.getPhysical(), m_rf->getSeq()->getLocalCycle() );
  #ifdef DEBUG_REG
     DEBUG_OUT("\tarf_int_global_t: writeInt64 vanilla[ %d ] physical[ %d ] value[ 0x%llx ]\n", rid.getVanilla(), rid.getPhysical(), value);
  #endif
  if(value == -1){
    //    ERROR_OUT("arf_int_global_t: writeInt64 vanilla[ %d ] physical[ %d ] value[ %lld ]\n", rid.getVanilla(), rid.getPhysical(), value);
    //    ASSERT(value != -1);
  }
  m_rf->setInt( rid.getPhysical(), value );
}

//***************************************************************************
void     arf_int_global_t::check( reg_id_t &rid, pstate_t *state,
                                  check_result_t *result, uint32 proc )
{
  // if the zero register (%g0) ... don't check
  if ( rid.getVanilla() == 0 ) {
    return;
  }

  // get global integer registers
  ireg_t simreg = state->getIntGlobal( rid.getVanilla(), rid.getVanillaState(), proc );
  if (result->update_only) {
    ASSERT( rid.getPhysical() != PSEQ_INVALID_REG );
    m_rf->setWrittenCycle( rid.getPhysical(), m_rf->getSeq()->getLocalCycle() );
    #ifdef DEBUG_REG
       DEBUG_OUT("\tarf_int_global_t: check FUNCTIONAL vanilla[ %d ] state[ 0x%x ] physical[ %d ] new_value[ 0x%llx ]\n", rid.getVanilla(), rid.getVanillaState(), rid.getPhysical(), simreg);
    #endif
    if( simreg == -1){
      //DEBUG_OUT("arf_int_global_t: check vanilla[ %d ] physical[ %d ] value[ %lld ]\n", rid.getVanilla(), rid.getPhysical(), simreg);
      //      ASSERT(simreg != -1);
    }
    m_rf->setInt( rid.getPhysical(), simreg );
  } else {
    half_t flatreg = reg_box_t::iregGlobalMap( rid.getVanilla(),
                                               rid.getVanillaState() );
    half_t physreg = m_retire_map->getMapping( flatreg, proc );     
    ireg_t myreg   = m_rf->getInt( physreg );
    if (myreg != simreg) {
      if (result->verbose)
        DEBUG_OUT("patch  gset:%d reg:%d flat %d -- 0x%0llx 0x%0llx\n",
                  rid.getVanillaState(), rid.getVanilla(), physreg,
                  myreg, simreg);
      m_rf->setWrittenCycle( physreg, m_rf->getSeq()->getLocalCycle() );
      if(myreg == -1){
        // ERROR_OUT("arf_int_global_t: check2 FAIL vanilla[ %d ] physical[ %d ] new_value[ 0x%llx ] old_value[ 0x%llx ]\n", flatreg, physreg, simreg, myreg);
      }
      if( simreg == -1 ){
        //DEBUG_OUT("arf_int_global_t: check2 vanilla[ %d ] physical[ %d ] value[ %lld ]\n", flatreg, physreg, simreg);
        //        ASSERT( simreg != -1 );
      }
   #ifdef DEBUG_REG
       DEBUG_OUT("\tarf_int_global_t: check FAIL vanilla[ %d ] state[ 0x%x ] physical[ %d ] flatreg[ %d ] physreg[ %d ] new_value[ 0x%llx ]\n", rid.getVanilla(), rid.getVanillaState(), rid.getPhysical(), flatreg, physreg, simreg);
    #endif
      m_rf->setInt( physreg, simreg );
      result->perfect_check = false;
    }
    else{
      //check passed
      //DEBUG_OUT("arf_int_global_t: check2 PASSED vanilla[ %d ] physical[ %d ] new_value[ 0x%llx ] old_value[ 0x%llx ]\n", flatreg, physreg, simreg, myreg);
    }
  }
}

//***************************************************************************
void      arf_cc_t::initialize( void )
{
  uint32 reg;
  half_t physreg;

  // initialize the condition code registers
  //       i.e.  %ccr, %fcc0, %fcc1, %fcc2, %fcc3
  // do this for all logical procs:
  for(uint k=0; k < CONFIG_LOGICAL_PER_PHY_PROC; ++k){
    for (reg = (uint32) REG_CC_CCR; reg <= (uint32) REG_CC_FCC3; reg ++) {
      physreg = m_decode_map->getMapping( reg, k );
      if (physreg == PSEQ_INVALID_REG) {
        physreg = m_decode_map->regAllocate( reg, k );
        m_rf->setWrittenCycle( physreg, m_rf->getSeq()->getLocalCycle() );
        //DEBUG_OUT("arf_cc: intialize vanilla[ %d ] physical[ %d ] value[ %lld ]\n", reg, physreg, 0);
        m_rf->setInt( physreg, 0 );
        m_decode_map->setMapping( reg, physreg, k );
        m_retire_map->setMapping( reg, physreg, k );      
      }
    }
  }
}

//***************************************************************************
void      arf_cc_t::readDecodeMap( reg_id_t &rid, uint32 proc )
{
  // proc arg not used
  half_t logical = rid.getVanilla();
  rid.setPhysical( m_decode_map->getMapping( logical, proc ) );   
}

//***************************************************************************
half_t    arf_cc_t::getLogical( reg_id_t &rid, uint32 proc )
{
  // proc arg not used
 #ifdef DEBUG_ARF
  DEBUG_OUT("arf_cc_t:getLogical BEGIN proc[%d]\n",proc);
 #endif

  half_t logical = rid.getVanilla();

 #ifdef DEBUG_ARF
  DEBUG_OUT("arf_cc_t:getLogical END proc[%d]\n",proc);
 #endif

  return (logical);
}

//***************************************************************************
void      arf_cc_t::check( reg_id_t &rid, pstate_t *state,
                           check_result_t *result, uint32 proc )
{
  assert(proc < CONFIG_LOGICAL_PER_PHY_PROC);

  ireg_t simreg;
  int32  reg    = rid.getVanilla();
  if (reg == REG_CC_CCR) {
    simreg  = state->getControl( CONTROL_CCR, proc );    
  } else {
    ireg_t fsr = state->getControl( CONTROL_FSR, proc );    
    if (reg == REG_CC_FCC0)
      simreg  = maskBits64( fsr, 11, 10 );
    else if (reg == REG_CC_FCC1)
      simreg  = maskBits64( fsr, 33, 32 );
    else if (reg == REG_CC_FCC2)
      simreg  = maskBits64( fsr, 35, 34 );
    else if (reg == REG_CC_FCC3)
      simreg  = maskBits64( fsr, 37, 36 );
    else {
      DEBUG_OUT( "arf_cc: check fails: unknown rid %d\n", reg);
      return;
    }
  }
  if (result->update_only) {
    ASSERT( rid.getPhysical() != PSEQ_INVALID_REG );
    m_rf->setWrittenCycle( rid.getPhysical(), m_rf->getSeq()->getLocalCycle() );
    #ifdef DEBUG_REG
       DEBUG_OUT("\tarf_cc_t: check FUNCTIONAL vanilla[ %d ] physical[ %d ] new_value[ 0x%llx ]\n", rid.getVanilla(), rid.getPhysical(), simreg);
    #endif
    if( simreg == -1){
      //DEBUG_OUT("arf_cc_t: check vanilla[ %d ] physical[ %d ] value[ %lld ]\n", rid.getVanilla(), rid.getPhysical(), simreg);
      //      ASSERT( simreg != -1);
    }
    m_rf->setInt( rid.getPhysical(), simreg );
  } else {
    half_t physreg = m_retire_map->getMapping( reg, proc );    //each logical proc has its own m_retire_map (due to different class objects)
    ireg_t myreg   = m_rf->getInt( physreg );
    if (simreg != myreg) {
      if (result->verbose)
        DEBUG_OUT("patch  cc:%d: 0x%0llx 0x%0llx\n", reg, myreg, simreg);
      m_rf->setWrittenCycle( physreg, m_rf->getSeq()->getLocalCycle() );
      if(myreg == -1){
        //  ERROR_OUT("arf_cc_t: check2 FAIL vanilla[ %d ] physical[ %d ] new_value[ 0x%llx ] old_val[ 0x%llx ]\n", rid.getVanilla(), physreg, simreg, myreg);
      }
      if(simreg == -1){
        //DEBUG_OUT("arf_cc_t: check2 vanilla[ %d ] physical[ %d ] value[ %lld ]\n", rid.getVanilla(), physreg, simreg);
        //        ASSERT(simreg != -1);
      }
  #ifdef DEBUG_REG
       DEBUG_OUT("\tarf_cc_t: check FAIL vanilla[ %d ] physical[ %d ] physreg[ %d ] new_value[ 0x%llx ]\n", rid.getVanilla(), rid.getPhysical(), physreg, simreg);
    #endif
      m_rf->setInt( physreg, simreg );
      result->perfect_check = false;
    }
    else{
      //check passed
      //DEBUG_OUT("arf_cc_t: check2 PASSED vanilla[ %d ] physical[ %d ] new_value[ 0x%llx ] old_val[ 0x%llx ]\n", rid.getVanilla(), physreg, simreg, myreg);
    }
  }
}


/*------------------------------------------------------------------------*/
/* Single Register File                                                   */
/*------------------------------------------------------------------------*/

//***************************************************************************
void      arf_single_t::initialize( void )
{
  uint32 reg;
  half_t physreg;

  // Floating point register file
  // do this for all logical procs:
  for(uint k=0; k < CONFIG_LOGICAL_PER_PHY_PROC; ++k){
    for (reg = 0; reg < MAX_FLOAT_REGS; reg++) {
      physreg = m_decode_map->getMapping( reg*2, k );
      if (physreg == PSEQ_INVALID_REG) {
        // map the upper 32 bits
        physreg = m_decode_map->regAllocate( reg*2, k );
        m_rf->setWrittenCycle( physreg, m_rf->getSeq()->getLocalCycle() );
        //DEBUG_OUT("arf_single_t: intialize vanilla[ %d ] physical[ %d ] value[ %lld ]\n", reg*2, physreg, 0);
        m_rf->setInt( physreg, 0 );
        m_decode_map->setMapping( reg*2, physreg, k );
        m_retire_map->setMapping( reg*2, physreg, k );
      }

      physreg = m_decode_map->getMapping( reg*2 + 1, k );
      if (physreg == PSEQ_INVALID_REG) {
        // map the lower 32 bits
        physreg = m_decode_map->regAllocate( reg*2 + 1, k );
        m_rf->setWrittenCycle( physreg, m_rf->getSeq()->getLocalCycle() );
        //DEBUG_OUT("arf_single_t: intialize2 vanilla[ %d ] physical[ %d ] value[ %lld ]\n", reg*2+1, physreg, 0);
        m_rf->setInt( physreg, 0 );
        m_decode_map->setMapping( reg*2 + 1, physreg, k );
        m_retire_map->setMapping( reg*2 + 1, physreg, k );
      }
    }
  } //end for loop over all logical procs
}

//***************************************************************************
void      arf_single_t::readDecodeMap( reg_id_t &rid, uint32 proc )
{

   #ifdef DEBUG_RET
  DEBUG_OUT("arf_single_t:readDecodeMap logical[%d] physical[%d] proc[%d]\n",
            rid.getVanilla(), m_decode_map->getMapping(rid.getVanilla(), proc), proc);
   #endif
  // proc arg not used
  half_t logical = rid.getVanilla();
  rid.setPhysical( m_decode_map->getMapping( logical, proc ) );
}

//***************************************************************************
half_t    arf_single_t::getLogical( reg_id_t &rid, uint32 proc )
{
  //proc arg not used

 #ifdef DEBUG_ARF
  DEBUG_OUT("arf_single_t:getLogical BEGIN proc[%d]\n",proc);
 #endif

  half_t logical = rid.getVanilla();

 #ifdef DEBUG_ARF
  DEBUG_OUT("arf_single_t:getLogical END proc[%d]\n",proc);
 #endif
  return (logical);
}

/** read this register as a single precision floating point register */
//***************************************************************************
float32 arf_single_t::readFloat32( reg_id_t &rid, uint32 proc )
{
  #ifdef DEBUG_RET
  DEBUG_OUT("arf_single_t:readFloat32 proc[%d] physical[%d]\n",proc,rid.getPhysical());
  #endif
  return (m_rf->getFloat( rid.getPhysical() ));
}

//***************************************************************************
void    arf_single_t::writeFloat32( reg_id_t &rid, float32 value, uint32 proc )
{
  m_rf->setWrittenCycle( rid.getPhysical(), m_rf->getSeq()->getLocalCycle() );
  m_rf->setFloat( rid.getPhysical(), value );
}

//***************************************************************************
void    arf_single_t::check( reg_id_t &rid, pstate_t *state,
                             check_result_t *result, uint32 proc )
{

  // proc arg not used
  half_t reg = rid.getVanilla();
  // get the double value for the register (rounded to the nearest 2)
  freg_t fsimreg = state->getDouble( (reg / 2)*2, proc );         
  ireg_t simreg32;
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
    ASSERT( rid.getPhysical() != PSEQ_INVALID_REG );
    m_rf->setWrittenCycle( rid.getPhysical(), m_rf->getSeq()->getLocalCycle() );
    #ifdef DEBUG_REG
       DEBUG_OUT("arf_single_t: check FUNCTIONAL vanilla[ %d ] physical[ %d ] new_value[ 0x%llx ]\n", reg, rid.getPhysical(), simreg32);
    #endif
    if( simreg32 == -1){
      //DEBUG_OUT("arf_single_t: check vanilla[ %d ] physical[ %d ] value[ %lld ]\n", reg, rid.getPhysical(), simreg32);
      //      ASSERT( simreg32 != -1);
    }
    m_rf->setInt( rid.getPhysical(), simreg32 );
  } else {
    half_t physreg = m_retire_map->getMapping( reg, proc );        
    ireg_t myreg   = m_rf->getInt( physreg );
    if (simreg32 != myreg) {
      if (result->verbose) {
        const char *str;
        if (reg % 2 == 1)
          str = "(hi)";
        else
          str = "(lo)";
        DEBUG_OUT("patch  float reg:%d %s -- 0x%0x 0x%0llx 0x%llx\n",
                  reg, str,
                  physreg, myreg, simreg32);
      }
      result->perfect_check = false;
      m_rf->setWrittenCycle( physreg, m_rf->getSeq()->getLocalCycle() );
      if(myreg == -1){
        //  ERROR_OUT("arf_single_t: check2 FAIL vanilla[ %d ] physical[ %d ] new_value[ 0x%llx ] old_value[ 0x%llx ]\n", reg, physreg, simreg32, myreg);
      }
      if(simreg32 == -1){
        //DEBUG_OUT("arf_single_t: check2 vanilla[ %d ] physical[ %d ] value[ %lld ]\n", reg, physreg, simreg32);
        //ASSERT(simreg32 != -1);
      }
  #ifdef DEBUG_REG
       DEBUG_OUT("arf_single_t: check FAIL vanilla[ %d ] physical[ %d ] physreg[ %d ] new_value[ 0x%llx ]\n", reg, rid.getPhysical(), physreg, simreg32);
  #endif
      m_rf->setInt( physreg, simreg32 );
    }
    else{
      //check passed
      //DEBUG_OUT("arf_single_t: check2 PASSED vanilla[ %d ] physical[ %d ] new_value[ 0x%llx ] old_value[ 0x%llx ]\n", reg, physreg, simreg32, myreg);
    }
  }
}

/*------------------------------------------------------------------------*/
/* Double Register File                                                   */
/*------------------------------------------------------------------------*/

//***************************************************************************
void    arf_double_t::initialize( void )
{
  // initialization is handled by single registers
}

//***************************************************************************
bool    arf_double_t::allocateRegister( reg_id_t &rid, uint32 proc )
{

  #ifdef DEBUG_RET
  DEBUG_OUT("arf_double_t:allocateRegister proc[%d] regtype[%d]\n",proc, rid.getRtype());
  #endif

  half_t logical = getLogical( rid, proc );

  #ifdef DEBUG_REG
     DEBUG_OUT("arf_double_t::allocateRegister logical[ %d ]", logical);
  #endif

  //   allocate 2 physical registers and concatenate the fields
  //   changed to allocate full 16 bit register specifiers
  word_t reg1 = m_decode_map->regAllocate(logical, proc);       
 #ifdef DEBUG_REG
     DEBUG_OUT(" reg1[ %d ]", reg1);;
  #endif
  word_t reg2 = m_decode_map->regAllocate(logical + 1, proc);
  #ifdef DEBUG_REG
     DEBUG_OUT(" reg2[ %d ]", reg2);
  #endif
  if (reg1 == PSEQ_INVALID_REG || reg2 == PSEQ_INVALID_REG) {
    DEBUG_OUT("error allocating register\n");
    print( rid, proc );       
    return (false);
  }

  // check that regs will fit in a 16 bit quantity (see constructor)
  ASSERT( reg1 < (1 << 16) );
  ASSERT( reg2 < (1 << 16) );
  rid.setPhysical( (reg1 << 16) | reg2 );

  #ifdef DEBUG_REG
     DEBUG_OUT(" physical[ %d ]", rid.getPhysical());
  #endif

  // write the mapping from logical to physical
  m_decode_map->setMapping( logical, (half_t) reg1, proc );
  m_decode_map->setMapping( logical + 1, (half_t) reg2, proc );

  #ifdef DEBUG_RET
  DEBUG_OUT("arf_double_t:allocateRegister END logical1[%d] physical1[%d] logical2[%d] physical2[%d] proc[%d] regtype[%d]\n", logical, reg1, logical+1, reg2, proc, rid.getRtype());
  #endif
  return (true);
}

//***************************************************************************
bool    arf_double_t::freeRegister( reg_id_t &rid, uint32 proc )
{
  word_t physical = rid.getPhysical();
  if (physical == PSEQ_INVALID_REG) {
    DEBUG_OUT("freeRegister: error: (double) unable to free INVALID register\n");
    return (false);
  }
  //   free 2 physical registers by unconcatenating the fields
  half_t logical = getLogical( rid, proc );         
  half_t reg1    = (physical >> 16) & 0xffff;
  half_t reg2    =  physical & 0xffff;

  assert(reg1 < (1 << 16));
  assert(reg2 < (1 << 16));

  #ifdef DEBUG_RETIRE
  DEBUG_OUT("abstract_double_t:freeRegister calling regFree (1) with logical[%d] physical[%d] physical1[%d] proc[%d]\n",logical,physical, reg1,proc);
  #endif
  m_decode_map->regFree( logical, reg1, proc );         

  #ifdef DEBUG_RETIRE
  DEBUG_OUT("abstract_double_t:freeRegister calling regFree (2) with logical[%d] physical[%d] physical1[%d] proc[%d]\n",logical+1,physical, reg2,proc);
  #endif
  m_decode_map->regFree( logical + 1, reg2, proc );        

  #ifdef DEBUG_RETIRE
  DEBUG_OUT("abstract_double_t:freeRegister END regFree with proc[%d]\n",proc);
  #endif

  rid.setPhysical( PSEQ_INVALID_REG );
  return (true);
}

//***************************************************************************
void    arf_double_t::readDecodeMap( reg_id_t &rid, uint32 proc )
{
   #ifdef DEBUG_REG
  DEBUG_OUT("arf_double_t: readDecodeMap BEGIN\n");
  #endif
  // proc arg not used
  half_t logical = rid.getVanilla();
  word_t reg     = m_decode_map->getMapping( logical, proc );       
  if ( reg == PSEQ_INVALID_REG ) {
    ERROR_OUT( "error: readDecode: (double) invalid mapping for reg:%d map:%d\n",
               logical, reg );
  }
  ASSERT( reg < (1 << 16) );
  word_t physical = reg << 16;
  
  reg = m_decode_map->getMapping( logical + 1, proc );     
  if ( reg == PSEQ_INVALID_REG ) {
    ERROR_OUT( "error: readDecode: (double) invalid mapping for reg:%d map:%d\n",
               (logical + 1), reg );
  }
  ASSERT( reg < (1 << 16) );

  #ifdef DEBUG_RET
     DEBUG_OUT("arf_double_t:readDecodeMap logical[%d] physical1[%d] physical2[%d] proc[%d]\n",
               logical, physical >> 16, reg, proc);
  #endif

  physical = physical | reg;
  rid.setPhysical( physical );
    #ifdef DEBUG_REG
  DEBUG_OUT("arf_double_t: readDecodeMap END\n");
  #endif

}

//***************************************************************************
void    arf_double_t::writeRetireMap( reg_id_t &rid, uint32 proc )
{
   #ifdef DEBUG_REG
  DEBUG_OUT("arf_double_t: writeRetireMap BEGIN\n");
  #endif
  half_t logical = getLogical( rid, proc );        
  word_t physical= rid.getPhysical();
  half_t reg1    = (physical >> 16) & 0xffff;
  assert(reg1 < (1 << 16));
  m_retire_map->setMapping( logical, reg1, proc );
  half_t reg2    =  physical & 0xffff;
  assert(reg2 < (1 << 16));
  m_retire_map->setMapping( logical + 1, reg2, proc );
    #ifdef DEBUG_REG
  DEBUG_OUT("arf_double_t: writeRetireMap END physical1[%d] physical2[%d] proc[%d]\n",reg1,reg2,proc);
  #endif
}

//***************************************************************************
void    arf_double_t::writeDecodeMap( reg_id_t &rid, uint32 proc )
{
  #ifdef DEBUG_REG
  DEBUG_OUT("arf_double_t: writeDecodeMap BEGIN\n");
  #endif
  half_t logical = getLogical( rid, proc );          
  word_t physical= rid.getPhysical();
  half_t reg1    = (physical >> 16) & 0xffff;
  assert(reg1 < ( 1 << 16));
  m_decode_map->setMapping( logical, reg1, proc );
  half_t reg2    = physical & 0xffff;
  assert(reg2 < ( 1 << 16));
  m_decode_map->setMapping( logical + 1, reg2, proc );
  #ifdef DEBUG_REG
  DEBUG_OUT("arf_double_t: writeDecodeMap END physical1[%d] physical2[%d] proc[%d]\n",reg1,reg2,proc);
  #endif
  
}

//***************************************************************************
bool    arf_double_t::isReady( reg_id_t &rid, uint32 proc )
{
  #ifdef DEBUG_REG
  //  DEBUG_OUT("arf_double_t: isReady BEGIN\n");
  #endif
  // zero %g0, and unmapped registers are always ready
  if (rid.isZero()) {
    return (true);
  }

  word_t  physical = rid.getPhysical();
  ASSERT( physical != PSEQ_INVALID_REG );
  half_t reg1 = (physical >> 16) & 0xffff;
  assert(reg1 < (1 << 16));
  half_t reg2 = physical & 0xffff;
 
  #ifdef DEBUG_REG
  DEBUG_OUT("arf_double_t: isReady BEGIN physical1[%d] physical2[%d]\n",reg1,reg2);
 #endif
  assert(reg2 < ( 1 << 16));

  bool b1 = m_rf->isReady(reg1);
  bool b2 = m_rf->isReady(reg2);
  #ifdef DEBUG_REG
  DEBUG_OUT("arf_double_t: isReady END physical1[%d] physical2[%d] b1[ %d ] b2[ %d ]\n",reg1,reg2, b1, b2);
 #endif
  return (b1 && b2);
  //  return ( m_rf->isReady( reg1 ) &&
  //       m_rf->isReady( reg2 ) );
}

//***************************************************************************
void arf_double_t::setL2Miss( reg_id_t & rid, uint32 proc)
{
  word_t physical = rid.getPhysical();
  if (physical == PSEQ_INVALID_REG) {
    DEBUG_OUT("setL2Miss: error: (double) unable to set INVALID register\n");
    ASSERT(0);
  }
  if(physical != PSEQ_INVALID_REG){
    half_t reg1    = (physical >> 16) & 0xffff;
    half_t reg2    =  physical & 0xffff;
    
    assert(reg1 < (1 << 16));
    assert(reg2 < (1 << 16)); 
    
    m_rf->setL2Miss(reg1, true);
    m_rf->setL2Miss(reg2, true);
  }
}

//***************************************************************************
void arf_double_t::setWriterSeqnum( reg_id_t & rid, uint64 seqnum, uint32 proc)
{
  word_t physical = rid.getPhysical();
  if (physical == PSEQ_INVALID_REG) {
    DEBUG_OUT("setWriterSeqnum: error: (double) unable to set INVALID register\n");
    ASSERT(0);
  }

  half_t reg1    = (physical >> 16) & 0xffff;
  half_t reg2    =  physical & 0xffff;
  
  assert(reg1 < (1 << 16));
  assert(reg2 < (1 << 16)); 
  
  m_rf->setWriterSeqnum(reg1, seqnum);
  m_rf->setWriterSeqnum(reg2, seqnum);
}

//***************************************************************************
uint64 arf_double_t::getWriterSeqnum( reg_id_t & rid, uint32 proc)
{
  word_t physical = rid.getPhysical();
  if (physical == PSEQ_INVALID_REG) {
    DEBUG_OUT("getWriterSeqnum: error: (double) unable to set INVALID register\n");
    ASSERT(0);
  }

  half_t reg1    = (physical >> 16) & 0xffff;
  half_t reg2    =  physical & 0xffff;
  
  assert(reg1 < (1 << 16));
  assert(reg2 < (1 << 16)); 
  
  uint64 s1 = m_rf->getWriterSeqnum(reg1);
  uint64 s2 = m_rf->getWriterSeqnum(reg2);
  //assertion is NOT TRUE bc can have multiple instr write SINGLE regs!
  //  ASSERT(s1 == s2);
  //for now, return max seq number
  if(s1 > s2){
    return s1;
  }
  return s2;
}

//***************************************************************************
uint64 arf_double_t::getWrittenCycle( reg_id_t & rid, uint32 proc)
{
  word_t physical = rid.getPhysical();
  if (physical == PSEQ_INVALID_REG) {
    DEBUG_OUT("getWrittenCycle: error: (double) unable to set INVALID register\n");
    ASSERT(0);
  }

  half_t reg1    = (physical >> 16) & 0xffff;
  half_t reg2    =  physical & 0xffff;
  
  assert(reg1 < (1 << 16));
  assert(reg2 < (1 << 16)); 
  
  uint64 s1 = m_rf->getWrittenCycle(reg1);
  uint64 s2 = m_rf->getWrittenCycle(reg2);
  word_t vanillareg = (word_t) rid.getVanilla();
  // if(s1 != s2){
  //    printf("ERROR: vanilla[ %d ] physical [ %d ] reg1[ %d ] (seqnum: %lld written: %lld) reg2[ %d ] (seqnum: %lld written: %lld)\n", vanillareg, physical, reg1, m_rf->getWriterSeqnum(reg1), s1, reg2, m_rf->getWriterSeqnum(reg2), s2);
  //  }
 
  //assertion is NOT TRUE bc can have multiple instr write SINGLE reg
  //  ASSERT(s1 == s2);
  //for now, return max written cycle
  if(s1 > s2){
    return s1;
  }
  return s2;
}

//***************************************************************************
bool arf_double_t::isL2Miss( reg_id_t & rid, uint32 proc )
{
  word_t  physical = rid.getPhysical();

  ASSERT( physical != PSEQ_INVALID_REG );
  half_t reg1 = (physical >> 16) & 0xffff;
  assert(reg1 < (1 << 16));
  half_t reg2 = physical & 0xffff;
  assert(reg2 < ( 1 << 16));

  bool b1 = m_rf->isL2Miss(reg1);
  bool b2 = m_rf->isL2Miss(reg2);

  return (b1 || b2);
}  

//***************************************************************************
void arf_double_t::incrementL2MissDep( reg_id_t & rid, uint32 proc )
{
  word_t  physical = rid.getPhysical();

  ASSERT( physical != PSEQ_INVALID_REG );
  half_t reg1 = (physical >> 16) & 0xffff;
  assert(reg1 < (1 << 16));
  half_t reg2 = physical & 0xffff;
  assert(reg2 < ( 1 << 16));

   #ifdef DEBUG_REG
     DEBUG_OUT("arf_double_t::incrementL2MissDep logical[ %d ] physical[ %d ] reg1[ %d] reg2[ %d ]cycle[ %lld ]\n", getLogical(rid, proc), rid.getPhysical(), reg1, reg2, m_rf->getSeq()->getLocalCycle());
       #endif

  //update both halves
  m_rf->incrementL2MissDep(reg1);
  m_rf->incrementL2MissDep(reg2);
}

//***************************************************************************
void arf_double_t::decrementL2MissDep( reg_id_t & rid, uint32 proc )
{
  word_t  physical = rid.getPhysical();

  ASSERT( physical != PSEQ_INVALID_REG );
  half_t reg1 = (physical >> 16) & 0xffff;
  assert(reg1 < (1 << 16));
  half_t reg2 = physical & 0xffff;
  assert(reg2 < ( 1 << 16));

   #ifdef DEBUG_REG
     DEBUG_OUT("arf_double_t::decrementL2MissDep logical[ %d ] physical[ %d ] reg1[ %d] reg2[ %d ]cycle[ %lld ]\n", getLogical(rid, proc), rid.getPhysical(), reg1, reg2, m_rf->getSeq()->getLocalCycle());
       #endif

  //update both halves
  m_rf->decrementL2MissDep(reg1);
  m_rf->decrementL2MissDep(reg2);
}

//***************************************************************************
int arf_double_t::getL2MissDep( reg_id_t & rid, uint32 proc )
{
  word_t  physical = rid.getPhysical();

  ASSERT( physical != PSEQ_INVALID_REG );
  half_t reg1 = (physical >> 16) & 0xffff;
  assert(reg1 < (1 << 16));
  half_t reg2 = physical & 0xffff;
  assert(reg2 < ( 1 << 16));
  // return the bigger miss dep number
  int s1 = m_rf->getL2MissDep(reg1);
  int s2 = m_rf->getL2MissDep(reg2);
  if(s1 > s2){
    return s1;
  }
  return s2;
}

//***************************************************************************
void    arf_double_t::waitResult( reg_id_t &rid, dynamic_inst_t *d_instr, uint32 proc )
{
  word_t  physical = rid.getPhysical();  
  ASSERT( physical != PSEQ_INVALID_REG );
  
  // case RID_DOUBLE:
  half_t reg1 = (physical >> 16) & 0xffff;
  half_t reg2 = physical & 0xffff;
  assert(reg1 < (1 << 16));
  assert(reg2 < (1 << 16));

   #ifdef DEBUG_REG
  DEBUG_OUT("arf_double_t: waitResult: calling isReady\n");
  #endif
  if ( !m_rf->isReady( reg1 ) ) {
    // only can wait on one result at a time --
    //    check first register -- if not ready wait on it
    //    (we will check again at wakeup (dynamic_inst_t::Schedule)
    m_rf->waitResult( reg1, d_instr );
    #ifdef DEBUG_REG
    DEBUG_OUT("arf_double_t:waitResult instr seqnum[%d] proc[%d] logical[ %d ] physical[ %d ] reg1[%d]\n",
              d_instr->getSequenceNumber(), d_instr->getProc(), getLogical(rid, proc), physical, reg1);
    #endif
  } else if ( !m_rf->isReady( reg2 ) ) {
    //    check second register -- if not ready wait on it
    m_rf->waitResult( reg2, d_instr );
    #ifdef DEBUG_REG
    DEBUG_OUT("arf_double_t:waitResult instr seqnum[%d] proc[%d] logical[ %d ] physical[ %d ] reg2[%d]\n",
              d_instr->getSequenceNumber(), d_instr->getProc(), getLogical(rid, proc), physical, reg2);
    #endif
  }

    #ifdef DEBUG_REG
  DEBUG_OUT("arf_double_t: waitResult END\n");
  #endif
}

//***************************************************************************
half_t    arf_double_t::getLogical( reg_id_t &rid, uint32 proc )
{
  // proc arg not used

 #ifdef DEBUG_ARF
  DEBUG_OUT("arf_double_t:getLogical BEGIN\n");
 #endif

  half_t logical = rid.getVanilla();
  // and logical + 1

 #ifdef DEBUG_ARF
  DEBUG_OUT("arf_double_t:getLogical END\n");
 #endif

  return (logical);
}

//***************************************************************************
my_register_t arf_double_t::readRegister( reg_id_t &rid, uint32 proc )
{
    #ifdef DEBUG_REG
  DEBUG_OUT("arf_double_t: readRegister BEGIN\n");
  #endif
  // read the register as a 64-bit integer
  my_register_t result;
  result.uint_64 = readInt64( rid, proc );        
    #ifdef DEBUG_REG
  DEBUG_OUT("arf_double_t: readRegister END\n");
  #endif
   #ifdef DEBUG_REG
    DEBUG_OUT("\tarf_double_t: readRegister vanilla[ %d ] physical[ %d ] value[ 0x%llx ] cycle[ %lld ]\n", rid.getVanilla(), rid.getPhysical(), result.uint_64, m_rf->getSeq()->getLocalCycle());
 #endif
  return result;
}

//***************************************************************************
void arf_double_t::writeRegister( reg_id_t &rid, my_register_t value, uint32 proc )
{
    #ifdef DEBUG_REG
  DEBUG_OUT("arf_double_t: writeRegister BEGIN\n");
  #endif
 #ifdef DEBUG_REG
    DEBUG_OUT("\tarf_double_t: writeRegister vanilla[ %d ] physical[ %d ] value[ 0x%llx ] cycle[ %lld ]\n", rid.getVanilla(), rid.getPhysical(), value.uint_64, m_rf->getSeq()->getLocalCycle());
 #endif
  ASSERT( rid.getPhysical() != PSEQ_INVALID_REG );
  writeInt64( rid, value.uint_64, proc );            
    #ifdef DEBUG_REG
  DEBUG_OUT("arf_double_t: writeRegister END\n");
  #endif
}

//***************************************************************************
ireg_t  arf_double_t::readInt64( reg_id_t &rid, uint32 proc )
{
    #ifdef DEBUG_REG
  DEBUG_OUT("arf_double_t: readInt64 BEGIN\n");
  #endif
  word_t physical = rid.getPhysical();  
  half_t reg1 = (physical >> 16) & 0xffff;
  half_t reg2 =  physical & 0xffff;
  
  assert(reg1 < ( 1 << 16));
  assert(reg2 < ( 1 << 16));
  #ifdef DEBUG_RET
  DEBUG_OUT("arf_double_t:readInt64 proc[%d] physical1[%d] physical2[%d]\n",proc,reg1,reg2);
  #endif
 #ifdef DEBUG_REG
  DEBUG_OUT("\tarf_double_t: readInt64 vanilla[ %d ] physical[ %d ] value1[ 0x%llx ] value2[ 0x%llx ] cycle[ %lld ]\n", rid.getVanilla(), rid.getPhysical(), m_rf->getInt(reg1) << 32, m_rf->getInt(reg2) & 0xffffffff, m_rf->getSeq()->getLocalCycle());
 #endif
  // read from 2 physical registers (both 32 bits)
  ireg_t  value = ( (m_rf->getInt( reg1 ) << 32) |
                    (m_rf->getInt( reg2 ) & 0xffffffff) );
    #ifdef DEBUG_REG
  DEBUG_OUT("arf_double_t: readInt64 END\n");
  #endif
  return (value);
}

//***************************************************************************
void    arf_double_t::writeInt64( reg_id_t &rid, ireg_t value, uint32 proc )
{
    #ifdef DEBUG_REG
  DEBUG_OUT("arf_double_t: writeInt64 BEGIN\n");
  #endif
  word_t physical = rid.getPhysical();
  half_t reg1 = (physical >> 16) & 0xffff;
  half_t reg2 =  physical & 0xffff;
  assert(reg1 < ( 1 << 16));
  assert(reg2 < ( 1 << 16));
  uint32 *ival = (uint32 *) &value;
    
  // ENDIAN_MATTERS
  if (ENDIAN_MATCHES) {
    // reg1 is high order bits
    m_rf->setWrittenCycle( reg1, m_rf->getSeq()->getLocalCycle() );
    #ifdef DEBUG_REG
        DEBUG_OUT("\tarf_double_t: writeInt64 vanilla[ %d ] physical[ %d ] value[ 0x%llx ]\n", rid.getVanilla(), reg1, ival[0]);
    #endif
    if( ival[0] == -1){
      //DEBUG_OUT("arf_double_t: writeInt64 vanilla[ %d ] physical[ %d ] value[ %lld ]\n", rid.getVanilla(), reg1, ival[0]);
      //      ASSERT( ival[0] != -1);
    }
    m_rf->setInt( reg1, ival[0] );
    // reg2 is low  order bits
    m_rf->setWrittenCycle( reg2, m_rf->getSeq()->getLocalCycle() );
    #ifdef DEBUG_REG
      DEBUG_OUT("\tarf_double_t: writeInt64 vanilla[ %d ] physical[ %d ] value[ 0x%llx ]\n", rid.getVanilla(), reg2, ival[1]);
    #endif
    if( ival[1] == -1){
      //DEBUG_OUT("arf_double_t: writeInt64 vanilla[ %d ] physical[ %d ] value[ %lld ]\n", rid.getVanilla(), reg2, ival[1]);
      //      ASSERT( ival[1] != -1);
    }
    m_rf->setInt( reg2, ival[1] );
  } else {
    // reg1 is high order bits
    m_rf->setWrittenCycle( reg1, m_rf->getSeq()->getLocalCycle() );
    #ifdef DEBUG_REG
       DEBUG_OUT("\tarf_double_t: writeInt64 2 vanilla[ %d ] physical[ %d ] value[ 0x%llx ]\n", rid.getVanilla(), reg1, ival[1]);
    #endif
    if( ival[1] == -1){
      //DEBUG_OUT("arf_double_t: writeInt64 2 vanilla[ %d ] physical[ %d ] value[ %lld ]\n", rid.getVanilla(), reg1, ival[1]);
      //      ASSERT( ival[1] != -1 );
    }
    m_rf->setInt( reg1, ival[1] );
    // reg2 is low  order bits
    m_rf->setWrittenCycle( reg2, m_rf->getSeq()->getLocalCycle() );
    #ifdef DEBUG_REG
      DEBUG_OUT("\tarf_double_t: writeInt64 2 vanilla[ %d ] physical[ %d ] value[ 0x%llx ]\n", rid.getVanilla(), reg2, ival[0]);
    #endif
    if( ival[0] == -1 ){
      //DEBUG_OUT("arf_double_t: writeInt64 2 vanilla[ %d ] physical[ %d ] value[ %lld ]\n", rid.getVanilla(), reg2, ival[0]);
      //      ASSERT( ival[0] != -1 );
    }
    m_rf->setInt( reg2, ival[0] );
  }
    #ifdef DEBUG_REG
  DEBUG_OUT("arf_double_t: writeInt64 END\n");
  #endif
}

//***************************************************************************
float64 arf_double_t::readFloat64( reg_id_t &rid, uint32 proc )
{
    #ifdef DEBUG_REG
  DEBUG_OUT("arf_double_t: readFloat64 BEGIN\n");
  #endif
  word_t  physical = rid.getPhysical();  
  half_t reg1 = (physical >> 16) & 0xffff;
  half_t reg2 =  physical & 0xffff;

  assert(reg1 < ( 1 << 16));
  assert(reg2 < ( 1 << 16));

 #ifdef DEBUG_RET
  DEBUG_OUT("arf_double_t:readFloat64 proc[%d] physical1[%d] physical2[%d]\n",proc,reg1,reg2);
  #endif
#ifdef DEBUG_REG
  DEBUG_OUT("\tarf_double_t: readFloat64 vanilla[ %d ] physical[ %d ] value1[ 0x%llx ] value2[ 0x%llx ] cycle[ %lld ]\n", rid.getVanilla(), rid.getPhysical(), m_rf->getInt(reg1) << 32, m_rf->getInt(reg2) & 0xffffffff, m_rf->getSeq()->getLocalCycle());
 #endif
  // read from 2 physical registers (both 32 bits)
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
    #ifdef DEBUG_REG
  DEBUG_OUT("arf_double_t: readFloat64 END\n");
  #endif
  return (fval);
}

/** write this register as a double precision floating point register */
//***************************************************************************
void    arf_double_t::writeFloat64( reg_id_t &rid, float64 value, uint32 proc )
{
    #ifdef DEBUG_REG
  DEBUG_OUT("arf_double_t: writeFloat64 BEGIN\n");
  #endif
  word_t physical = rid.getPhysical();
  half_t reg1 = (physical >> 16) & 0xffff;
  half_t reg2 =  physical & 0xffff;

  assert(reg1 < ( 1 << 16));
  assert(reg2 < ( 1 << 16));

  uint32 *ival = (uint32 *) &value;

  // ENDIAN_MATTERS
  if (ENDIAN_MATCHES) {
    // reg1 is high order bits
    m_rf->setWrittenCycle( reg1, m_rf->getSeq()->getLocalCycle() );
    #ifdef DEBUG_REG
      DEBUG_OUT("\tarf_double_t: writeFloat64 vanilla[ %d ] physical[ %d ] value[ 0x%llx ]\n", rid.getVanilla(), reg1, ival[0]);
    #endif
    if( ival[0] == -1 ){
      //DEBUG_OUT("arf_double_t: writeFloat64 vanilla[ %d ] physical[ %d ] value[ %lld ]\n", rid.getVanilla(), reg1, ival[0]);
      //      ASSERT( ival[0] != -1 );
    }
    m_rf->setInt( reg1, ival[0] );
    // reg2 is low  order bits
    m_rf->setWrittenCycle( reg2, m_rf->getSeq()->getLocalCycle() );
    #ifdef DEBUG_REG
      DEBUG_OUT("\tarf_double_t: writeFloat64 vanilla[ %d ] physical[ %d ] value[ 0x%llx ]\n", rid.getVanilla(), reg2, ival[1]);
    #endif
    if( ival[1] == -1 ){
      //DEBUG_OUT("arf_double_t: writeFloat64 vanilla[ %d ] physical[ %d ] value[ %lld ]\n", rid.getVanilla(), reg2, ival[1]);
      //      ASSERT( ival[1] != -1 );
    }
    m_rf->setInt( reg2, ival[1] );
  } else {
    // reg1 is high order bits
    m_rf->setWrittenCycle( reg1, m_rf->getSeq()->getLocalCycle() );
    #ifdef DEBUG_REG
      DEBUG_OUT("\tarf_double_t: writeFloat64 2 vanilla[ %d ] physical[ %d ] value[ 0x%llx ]\n", rid.getVanilla(), reg1, ival[1]);
    #endif
    if( ival[1] == -1 ){
      //DEBUG_OUT("arf_double_t: writeFloat64 2 vanilla[ %d ] physical[ %d ] value[ %lld ]\n", rid.getVanilla(), reg1, ival[1]);
      //      ASSERT( ival[1] != -1 );
    }
    m_rf->setInt( reg1, ival[1] );
    // reg2 is low  order bits
    m_rf->setWrittenCycle( reg2, m_rf->getSeq()->getLocalCycle() );
    #ifdef DEBUG_REG
      DEBUG_OUT("\tarf_double_t: writeFloat64 2 vanilla[ %d ] physical[ %d ] value[ 0x%llx ]\n", rid.getVanilla(), reg2, ival[0]);
    #endif
    if( ival[0] == -1 ){
      //DEBUG_OUT("arf_double_t: writeFloat64 2 vanilla[ %d ] physical[ %d ] value[ %lld ]\n", rid.getVanilla(), reg2, ival[0]);
      //ASSERT( ival[0] != -1 );
    }
    m_rf->setInt( reg2, ival[0] );
  }
    #ifdef DEBUG_REG
  DEBUG_OUT("arf_double_t: writeFloat64 END\n");
  #endif
}

//***************************************************************************
void    arf_double_t::check( reg_id_t &rid, pstate_t *state,
                             check_result_t *result, uint32 proc )
{
  // proc arg not used
    #ifdef DEBUG_REG
  DEBUG_OUT("arf_double_t: check BEGIN\n");
  #endif
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
  half_t reg1;
  half_t reg2;
  if (result->update_only) {
    word_t phys_regs = rid.getPhysical();
    reg1 = (phys_regs >> 16) & 0xffff;
    reg2 =  phys_regs & 0xffff;
  } else {
    reg1 = m_retire_map->getMapping( rid.getVanilla(), proc );    
    reg2 = m_retire_map->getMapping( rid.getVanilla() + 1, proc );      
  }

  assert(reg1 < ( 1 << 16));
  assert(reg2 < ( 1 << 16));

  if (result->update_only) {
    ASSERT( reg1 != PSEQ_INVALID_REG );
    ASSERT( reg2 != PSEQ_INVALID_REG );
    m_rf->setWrittenCycle( reg1, m_rf->getSeq()->getLocalCycle() );
    #ifdef DEBUG_REG
      DEBUG_OUT("\tarf_double_t: check FUNCTIONAL vanilla[ %d ] physical[ %d ] new_value[ 0x%llx ]\n", rid.getVanilla(), reg1, simupper32);
    #endif
    if( simupper32 == -1 ){
      //DEBUG_OUT("arf_double_t: check vanilla[ %d ] physical[ %d ] value[ %lld ]\n", rid.getVanilla(), reg1, simupper32);
      ASSERT( simupper32 != -1 );
    }
    m_rf->setInt( reg1, simupper32 );
    m_rf->setWrittenCycle( reg2, m_rf->getSeq()->getLocalCycle() );
    #ifdef DEBUG_REG
      DEBUG_OUT("\tarf_double_t: check FUNCTIONAL vanilla[ %d ] physical[ %d ] new_value[ 0x%llx ]\n", rid.getVanilla(), reg2, simlower32);
    #endif
    if( simlower32 == -1 ){
      //DEBUG_OUT("arf_double_t: check vanilla[ %d ] physical[ %d ] value[ %lld ]\n", rid.getVanilla(), reg2, simlower32);
      //ASSERT( simlower32 != -1 );
    }
    m_rf->setInt( reg2, simlower32 );
  } else {
    ireg_t myreg   = m_rf->getInt( reg1 );
    if (myreg != simupper32) {
      if (result->verbose) {
        DEBUG_OUT("patch  float reg:%d (hi) -- 0x%0x 0x%llx 0x%llx\n",
                  rid.getVanilla(), reg1, myreg, simupper32);
      }
      result->perfect_check = false;
      m_rf->setWrittenCycle( reg1, m_rf->getSeq()->getLocalCycle() );
      if(myreg == -1){
        //ERROR_OUT("arf_double_t: check 2 FAIL vanilla[ %d ] physical[ %d ] new_value[ 0x%llx ] old_value[ 0x%llx ]\n", rid.getVanilla(), reg1, simupper32, myreg);
      }
      if(simupper32 == -1 ){
        //DEBUG_OUT("arf_double_t: check 2 vanilla[ %d ] physical[ %d ] value[ %lld ]\n", rid.getVanilla(), reg1, simupper32);
        //ASSERT( simupper32 != -1 );
      }
    #ifdef DEBUG_REG
      DEBUG_OUT("\tarf_double_t: check FAIL 1 vanilla[ %d ] physical[ %d ] new_value[ 0x%llx ]\n", rid.getVanilla(), reg1, simupper32);
     #endif
      m_rf->setInt( reg1, simupper32 );
    }
    else{
      //check passed
      //DEBUG_OUT("arf_double_t: check 2 PASSED vanilla[ %d ] physical[ %d ] new_value[ 0x%llx ] old_value[ 0x%llx ]\n", rid.getVanilla(), reg1, simupper32, myreg);
    }
    
    myreg  = m_rf->getInt( reg2 );
    if (myreg != simlower32) {
      if (result->verbose) {
        DEBUG_OUT("patch  float reg:%d (lo) -- 0x%0x 0x%llx 0x%llx\n",
                  rid.getVanilla(), reg2, myreg, simlower32);
      }
      result->perfect_check = false;
      m_rf->setWrittenCycle( reg2, m_rf->getSeq()->getLocalCycle() );
      if(myreg == -1){
        //ERROR_OUT("arf_double_t: check 2 FAIL vanilla[ %d ] physical[ %d ] new_value[ 0x%llx ] old_value[ 0x%llx ]\n", rid.getVanilla(), reg2, simlower32, myreg);
      }
      if(simlower32 == -1){
        //DEBUG_OUT("arf_double_t: check 2 vanilla[ %d ] physical[ %d ] value[ %lld ]\n", rid.getVanilla(), reg2, simlower32);
        //ASSERT( simlower32 != -1 );
      }
    #ifdef DEBUG_REG
      DEBUG_OUT("\tarf_double_t: check FAIL 2 vanilla[ %d ] physical[ %d ] new_value[ 0x%llx ]\n", rid.getVanilla(), reg2, simupper32);
     #endif
      m_rf->setInt( reg2, simlower32 );
    }
    else{
      //check passed
      //DEBUG_OUT("arf_double_t: check 2 PASSED vanilla[ %d ] physical[ %d ] new_value[ 0x%llx ] old_value[ 0x%llx ]\n", rid.getVanilla(), reg2, simlower32, myreg);
    }
  }

    #ifdef DEBUG_REG
  DEBUG_OUT("arf_double_t: check END\n");
  #endif
}

//***************************************************************************
void    arf_double_t::addFreelist( reg_id_t &rid )
{
    #ifdef DEBUG_REG
  DEBUG_OUT("arf_double_t: addFreelist BEGIN\n");
  #endif
  word_t phys = rid.getPhysical();
  half_t reg1 = (phys >> 16) & 0xffff;
  half_t reg2 =  phys & 0xffff;

  assert(reg1 < (1 << 16));
  assert(reg2 < (1 << 16));

  bool success = m_decode_map->addFreelist( reg1 );
  if (!success) {
    SIM_HALT;    
  }
  success = m_decode_map->addFreelist( reg2 );
  if (!success) {
    SIM_HALT;    
  }
    #ifdef DEBUG_REG
  DEBUG_OUT("arf_double_t: addFreelist END\n");
  #endif
}

//***************************************************************************
void    arf_double_t::print( reg_id_t &rid, uint32 proc )
{
  const char *rt = reg_id_t::rid_type_menomic( rid.getRtype() );
  word_t phys = rid.getPhysical();
  half_t reg1 = (phys >> 16) & 0xffff;
  half_t reg2 =  phys & 0xffff;
  assert(reg1 < (1 << 16));
  assert(reg2 < (1 << 16));
  DEBUG_OUT( "%.6s (%3.3d : %2.2d) = %3.3d   -->  %3.3d %3.3d\n",
             rt, rid.getVanilla(), rid.getVanillaState(),
             getLogical(rid, proc), reg1, reg2 );      
}

/*------------------------------------------------------------------------*/
/* Container Register File                                                */
/*------------------------------------------------------------------------*/

//***************************************************************************
arf_container_t::~arf_container_t( void )
{
  for (int32 i = 0; i < CONTAINER_NUM_CONTAINER_TYPES; i++) {
    // if dispatch map != NULL, then this register type has been initialized
    if (m_dispatch_map[i] != NULL) {
      for (uint32 k = 0; k < m_size; k++) {
        if (m_rename_map[i][k] != NULL){
          delete [] m_rename_map[i][k];
        }
      }
      delete [] m_dispatch_map[i];
    }
    if (m_rename_map[i] != NULL){
      free( m_rename_map[i] );
    }
  }
}

//***************************************************************************
void    arf_container_t::initialize( void )
{
  // container class is manually initialized as it contains references
  //      across multiple register files.
}

//***************************************************************************
bool    arf_container_t::allocateRegister( reg_id_t &rid, uint32 proc )
{

  #ifdef DEBUG_RET
  // DEBUG_OUT("arf_container_t:allocateRegister proc[%d] regtype[%d]\n",proc,rid.getRtype());
 #endif

  // DEBUG_OUT("container: alloc reg\n");
  // assign a new register handler for this register instance
  rid_container_t rtype = (rid_container_t) rid.getVanillaState();
  uint32 index          = m_rename_index[rtype];
  m_rename_index[rtype] = (m_rename_index[rtype] + 1) % m_size;
  uint32 basevanilla    = rid.getVanilla();

  // copy the registers in dispatch into the next free register array
  for (uint32 i = 0; i < m_dispatch_size[rtype]; i++) {
    reg_id_t &subid    = m_rename_map[rtype][index][i];
    reg_id_t &dispatch = m_dispatch_map[rtype][i];
    abstract_rf_t *arf = dispatch.getARF();

    subid.setRtype( dispatch.getRtype() );

  #ifdef DEBUG_RET
    DEBUG_OUT("arf_container_t:allocateRegister setVanilla regno[%d]\n",basevanilla+dispatch.getVanilla());
 #endif

    subid.setVanilla( basevanilla + dispatch.getVanilla() );
    subid.setARF( arf );
    bool success = arf->allocateRegister( subid, proc );     
    if (!success) {
      // DEBUG_OUT("container: alloc fails=%d\n", i);
      return (false);
    }
  }

  /* for (uint32 i=0; i < m_dispatch_size[rtype]; i++) {
     m_rename_map[rtype][index][i].print( rid );
     }
  */
  rid.setPhysical( index );

  #ifdef DEBUG_RET
   DEBUG_OUT("arf_container_t:allocateRegister proc[%d] physical[%d]\n", proc,index);
 #endif
  return (true);
}

//***************************************************************************
bool    arf_container_t::freeRegister( reg_id_t &rid, uint32 proc )
{
  #ifdef DEBUG_RETIRE
    DEBUG_OUT("arf_container_t:freeRegister BEGIN proc[%d]\n",proc);
  #endif
  // DEBUG_OUT("container: free reg\n");
  uint32 index = rid.getPhysical();
  if (index == PSEQ_INVALID_REG) {
    DEBUG_OUT("freeRegister: error: (container) unable to free INVALID register\n");
    return (false);
  }
  rid_container_t rtype = (rid_container_t) rid.getVanillaState();

  // free the registers in the rename map
  for (uint32 i = 0; i < m_dispatch_size[rtype]; i++) {
    reg_id_t &subid = m_rename_map[rtype][index][i];
    subid.getARF()->freeRegister( subid, proc );     
  }

  // CM You might think its a good idea to free the physical register here.
  //    However, its not!
  // rid.setPhysical( PSEQ_INVALID_REG );
  // DEBUG_OUT("container: end free=%d\n", index);
  #ifdef DEBUG_RETIRE
    DEBUG_OUT("arf_container_t:freeRegister END proc[%d]\n",proc);
  #endif
  return (true);
}

//***************************************************************************
void    arf_container_t::readDecodeMap( reg_id_t &rid, uint32 proc )
{
  // proc arg not used
  // DEBUG_OUT("container: read decode\n");
  // assign a new register handler for this register instance
  rid_container_t rtype = (rid_container_t) rid.getVanillaState();
  uint32 index          = m_rename_index[rtype];
  m_rename_index[rtype] = (m_rename_index[rtype] + 1) % m_size;
  uint32 basevanilla    = rid.getVanilla();

  // copy the registers in dispatch into the next free register array
  for (uint32 i = 0; i < m_dispatch_size[rtype]; i++) {
    reg_id_t &subid    = m_rename_map[rtype][index][i];
    reg_id_t &dispatch = m_dispatch_map[rtype][i];
    abstract_rf_t *arf = dispatch.getARF();

    subid.setRtype( dispatch.getRtype() );
    subid.setVanilla( basevanilla + dispatch.getVanilla() );
    subid.setARF( arf );
    arf->readDecodeMap( subid, proc );    
  }
  
  rid.setPhysical( index );
  // DEBUG_OUT("container: end alloc=%d\n", index);
}

//***************************************************************************
void    arf_container_t::writeRetireMap( reg_id_t &rid, uint32 proc )
{
  // DEBUG_OUT("container: write retire\n");
  uint32 index          = rid.getPhysical();
  rid_container_t rtype = (rid_container_t) rid.getVanillaState();

  // write the retire maps
  for (uint32 i = 0; i < m_dispatch_size[rtype]; i++) {
    reg_id_t &subid = m_rename_map[rtype][index][i];
    subid.getARF()->writeRetireMap( subid, proc );            
  }
}

//***************************************************************************
void    arf_container_t::writeDecodeMap( reg_id_t &rid, uint32 proc )
{
  uint32 index          = rid.getPhysical();
  rid_container_t rtype = (rid_container_t) rid.getVanillaState();

  // write the decode maps
  for (uint32 i = 0; i < m_dispatch_size[rtype]; i++) {
    reg_id_t &subid = m_rename_map[rtype][index][i];
    subid.getARF()->writeDecodeMap( subid, proc );      
  }
}

//***************************************************************************
bool    arf_container_t::isReady( reg_id_t &rid, uint32 proc )
{
   #ifdef DEBUG_REG
  DEBUG_OUT("arf_container_t: isReady BEGIN\n");
  #endif
  // DEBUG_OUT("container: is ready\n");
  // zero %g0, and unmapped registers are always ready
  if (rid.isZero()){
        #ifdef DEBUG_REG
           DEBUG_OUT("arf_container_t::isReady END\n");
      #endif
    return (true);
  }

  uint32 index          = rid.getPhysical();
  rid_container_t rtype = (rid_container_t) rid.getVanillaState();
  for (uint32 i = 0; i < m_dispatch_size[rtype]; i++) {
    reg_id_t &subid = m_rename_map[rtype][index][i];
    if (!subid.getARF()->isReady( subid, proc )) {
         #ifdef DEBUG_REG
           DEBUG_OUT("arf_container_t::isReady END (returning false)\n");
      #endif
      return (false);
    }
  }
     #ifdef DEBUG_REG
           DEBUG_OUT("arf_container_t::isReady END (returning true)\n");
      #endif
  // DEBUG_OUT("container: ready now\n");
  return (true);
}

//**************************************************************************
void arf_container_t::setL2Miss( reg_id_t & rid, uint32 proc )
{
  uint32 index = rid.getPhysical();
  if (index == PSEQ_INVALID_REG) {
    DEBUG_OUT("setL2Miss: error: (container) unable to set INVALID register\n");
    ASSERT(0);
  }
  if(index != PSEQ_INVALID_REG){
    rid_container_t rtype = (rid_container_t) rid.getVanillaState();
    
    // free the registers in the rename map
    for (uint32 i = 0; i < m_dispatch_size[rtype]; i++) {
      reg_id_t &subid = m_rename_map[rtype][index][i];
      subid.getARF()->setL2Miss( subid, proc );      
    }
  }
}

//**************************************************************************
void arf_container_t::setWriterSeqnum( reg_id_t & rid, uint64 seqnum, uint32 proc )
{
  uint32 index = rid.getPhysical();
  if (index == PSEQ_INVALID_REG) {
    DEBUG_OUT("setWriterSeqnum: error: (container) unable to set INVALID register\n");
    ASSERT(0);
  }
  rid_container_t rtype = (rid_container_t) rid.getVanillaState();
    
  // free the registers in the rename map
  for (uint32 i = 0; i < m_dispatch_size[rtype]; i++) {
    reg_id_t &subid = m_rename_map[rtype][index][i];
    subid.getARF()->setWriterSeqnum( subid, seqnum, proc );     
  }
}

//**************************************************************************
uint64 arf_container_t::getWriterSeqnum( reg_id_t & rid, uint32 proc )
{
  uint32 index = rid.getPhysical();
  if (index == PSEQ_INVALID_REG) {
    DEBUG_OUT("getWriterSeqnum: error: (container) unable to set INVALID register\n");
    ASSERT(0);
  }
  rid_container_t rtype = (rid_container_t) rid.getVanillaState();
    
  // free the registers in the rename map
  for (uint32 i = 0; i < m_dispatch_size[rtype]; i++) {
    reg_id_t &subid = m_rename_map[rtype][index][i];
    //should just return first seqnum, since all are the same
    return subid.getARF()->getWriterSeqnum( subid, proc );     
  }
  //ERROR?
  ASSERT(0);
  return 0;
}

//**************************************************************************
uint64 arf_container_t::getWrittenCycle( reg_id_t & rid, uint32 proc )
{
  uint32 index = rid.getPhysical();
  if (index == PSEQ_INVALID_REG) {
    DEBUG_OUT("getWrittenCycle: error: (container) unable to set INVALID register\n");
    ASSERT(0);
  }
  rid_container_t rtype = (rid_container_t) rid.getVanillaState();
    
  // free the registers in the rename map
  for (uint32 i = 0; i < m_dispatch_size[rtype]; i++) {
    reg_id_t &subid = m_rename_map[rtype][index][i];
    //should just return first seqnum, since all are the same
    return subid.getARF()->getWrittenCycle( subid, proc );     
  }
  //ERROR?
  ASSERT(0);
  return 0;
}

//***************************************************************************
bool    arf_container_t::isL2Miss( reg_id_t &rid, uint32 proc )
{
  uint32 index          = rid.getPhysical();
  rid_container_t rtype = (rid_container_t) rid.getVanillaState();
  for (uint32 i = 0; i < m_dispatch_size[rtype]; i++) {
    reg_id_t &subid = m_rename_map[rtype][index][i];
    if (subid.getARF()->isL2Miss( subid, proc )) {
      return true;
    }
  }
  return false;
}

//***************************************************************************
void    arf_container_t::incrementL2MissDep( reg_id_t &rid, uint32 proc )
{
  uint32 index          = rid.getPhysical();
  rid_container_t rtype = (rid_container_t) rid.getVanillaState();
  for (uint32 i = 0; i < m_dispatch_size[rtype]; i++) {
    reg_id_t &subid = m_rename_map[rtype][index][i];
    if (subid.getARF()->isL2Miss( subid, proc )) {
      subid.getARF()->incrementL2MissDep(subid, proc);
      return;
    }
  }
}

//***************************************************************************
void    arf_container_t::decrementL2MissDep( reg_id_t &rid, uint32 proc )
{
  uint32 index          = rid.getPhysical();
  rid_container_t rtype = (rid_container_t) rid.getVanillaState();
  for (uint32 i = 0; i < m_dispatch_size[rtype]; i++) {
    reg_id_t &subid = m_rename_map[rtype][index][i];
    if (subid.getARF()->isL2Miss( subid, proc )) {
      subid.getARF()->decrementL2MissDep(subid, proc);
      return;
    }
  }
}

//***************************************************************************
int    arf_container_t::getL2MissDep( reg_id_t &rid, uint32 proc )
{
  uint32 index          = rid.getPhysical();
  int total = 0;
  rid_container_t rtype = (rid_container_t) rid.getVanillaState();
  for (uint32 i = 0; i < m_dispatch_size[rtype]; i++) {
    reg_id_t &subid = m_rename_map[rtype][index][i];
    if (subid.getARF()->isL2Miss( subid, proc )) {
      total += subid.getARF()->getL2MissDep(subid, proc);
    }
  }
  return total;
}

//***************************************************************************
void    arf_container_t::waitResult( reg_id_t &rid, dynamic_inst_t *d_instr, uint32 proc )
{
  // DEBUG_OUT("container: wait\n");
  uint32 index          = rid.getPhysical();
  rid_container_t rtype = (rid_container_t) rid.getVanillaState();

  for (uint32 i = 0; i < m_dispatch_size[rtype]; i++) {
    reg_id_t &subid = m_rename_map[rtype][index][i];
     #ifdef DEBUG_REG
           DEBUG_OUT("arf_container_t::waitResult calling isReady\n");
      #endif
    if (!subid.getARF()->isReady( subid, proc )) {
      // only wait on at most one register
      subid.getARF()->waitResult( subid, d_instr, proc );
        #ifdef DEBUG_REG
           DEBUG_OUT("arf_container_t::waitResult isReady END\n");
      #endif
      return;
    }
  }
   #ifdef DEBUG_REG
           DEBUG_OUT("arf_container_t::waitResult isReady END\n");
      #endif
}

//***************************************************************************
half_t    arf_container_t::getLogical( reg_id_t &rid, uint32 proc )
{

  // proc arg not used
 #ifdef DEBUG_ARF
  DEBUG_OUT("arf_container_t:getLogical BEGIN proc[%d]\n",proc);
 #endif

  // warning: getlogical() does not really apply to container class
  half_t logical = rid.getPhysical();

 #ifdef DEBUG_ARF
  DEBUG_OUT("arf_container_t:getLogical END proc[%d]\n",proc);
 #endif
  return (logical);
}

//***************************************************************************
ireg_t    arf_container_t::readInt64( reg_id_t &rid, uint32 proc )
{
  // DEBUG_OUT("container: readint\n");
  // index contains which register array in m_rename_map to use
  uint32 index          = rid.getPhysical();
  // Vanilla is used in at retirement to select which register to write to
  uint32 selector       = rid.getSelector();
  rid_container_t rtype = (rid_container_t) rid.getVanillaState();

  /*
  DEBUG_OUT("index=%d\n", index);
  DEBUG_OUT("rtype=%d\n", rtype);
  DEBUG_OUT("selector=%d\n", selector);
  DEBUG_OUT("size=%d\n", m_dispatch_size[rtype] );
  */

  if ( selector >= m_dispatch_size[rtype] ) {
    DEBUG_OUT( "warning: readInt64 selector out of range: %d\n",
               selector );
    return (0);
  }

  reg_id_t &subid = m_rename_map[rtype][index][selector];
  return (subid.getARF()->readInt64( subid, proc ));          
}

//***************************************************************************
void      arf_container_t::writeInt64( reg_id_t &rid, ireg_t value, uint32 proc )
{
  // DEBUG_OUT("container: writeint\n");
  // index contains which register array in m_rename_map to use
  uint32 index          = rid.getPhysical();
  // Vanilla is used in at retirement to select which register to write to
  uint32 selector       = rid.getSelector();
  rid_container_t rtype = (rid_container_t) rid.getVanillaState();

  if ( selector > m_dispatch_size[rtype] ) {
    ERROR_OUT( "warning: writeInt64 selector out of range: %d\n",
               selector );
    return;
  }

  /*
  DEBUG_OUT("index=%d\n", index);
  DEBUG_OUT("rtype=%d\n", rtype);
  DEBUG_OUT("selector=%d\n", selector);
  */

  reg_id_t &subid = m_rename_map[rtype][index][selector];
  subid.getARF()->writeInt64( subid, value, proc );        
}

//***************************************************************************
void    arf_container_t::initializeControl( reg_id_t &rid, uint32 proc )
{
  // proc arg is not used
  // DEBUG_OUT("container: init control\n");
  // index contains which register array in m_rename_map to use
  uint32 index          = rid.getPhysical();
  // Vanilla is used in at retirement to select which register to write to
  uint32 selector       = rid.getSelector();
  rid_container_t rtype = (rid_container_t) rid.getVanillaState();

  /*
  DEBUG_OUT("index=%d\n", index);
  DEBUG_OUT("rtype=%d\n", rtype);
  DEBUG_OUT("selector=%d\n", selector);
  DEBUG_OUT("size=%d\n", m_dispatch_size[rtype] );
  */

  if ( selector >= m_dispatch_size[rtype] ) {
    DEBUG_OUT( "warning: initializeControl selector out of range: %d\n",
               selector );
    return;
  }

  reg_id_t &subid = m_rename_map[rtype][index][selector];
  subid.getARF()->initializeControl( subid, proc );      
}

//**************************************************************************
void    arf_container_t::finalizeControl( reg_id_t &rid, uint32 proc )
{
  // DEBUG_OUT("container: finalize control\n");
  // index contains which register array in m_rename_map to use
  uint32 index          = rid.getPhysical();
  // Vanilla is used in at retirement to select which register to write to
  uint32 selector       = rid.getSelector();
  rid_container_t rtype = (rid_container_t) rid.getVanillaState();

  /*
  DEBUG_OUT("index=%d\n", index);
  DEBUG_OUT("rtype=%d\n", rtype);
  DEBUG_OUT("selector=%d\n", selector);
  DEBUG_OUT("size=%d\n", m_dispatch_size[rtype] );
  */

  if ( selector >= m_dispatch_size[rtype] ) {
    DEBUG_OUT( "warning: finalize control selector out of range: %d\n",
               selector );
    return;
  }

  reg_id_t &subid = m_rename_map[rtype][index][selector];
  subid.getARF()->finalizeControl( subid, proc );
  // DEBUG_OUT("container: end finalize control\n");
}

//***************************************************************************
void      arf_container_t::check( reg_id_t &rid, pstate_t *state,
                                  check_result_t *result, uint32 proc )
{
  // DEBUG_OUT("container: checkerino\n");
  // index contains which register array in m_rename_map to use
  uint32 index          = rid.getPhysical();
  ASSERT( index < m_size );
  rid_container_t rtype = (rid_container_t) rid.getVanillaState();
  
  // check each register identifier in the array
  for (uint32 i = 0; i < m_dispatch_size[rtype]; i++) {
    // DEBUG_OUT("check: %d %d %d\n", rtype, index, i);
    reg_id_t &subid = m_rename_map[rtype][index][i];
    subid.getARF()->check( subid, state, result, proc );     
  }
  // DEBUG_OUT("endth\n");
}

//***************************************************************************
void    arf_container_t::addFreelist( reg_id_t &rid )
{
  // index contains which register array in m_rename_map to use
  uint32 index          = rid.getPhysical();
  rid_container_t rtype = (rid_container_t) rid.getVanillaState();

  // check each register identifier in the array
  for (uint32 i = 0; i < m_dispatch_size[rtype]; i++) {
    // DEBUG_OUT("print: %d %d %d\n", rtype, index, i);
    reg_id_t &subid = m_rename_map[rtype][index][i];
    subid.getARF()->addFreelist( subid );
  }
}

//***************************************************************************
void    arf_container_t::print( reg_id_t &rid, uint32 proc )
{
  // index contains which register array in m_rename_map to use
  uint32 index          = rid.getPhysical();
  rid_container_t rtype = (rid_container_t) rid.getVanillaState();

  // print each register identifier in the array
  // DEBUG_OUT( "container:%d type:%d\n", index, rtype );
  for (uint32 i = 0; i < m_dispatch_size[rtype]; i++) {
    // DEBUG_OUT("print: %d %d %d\n", rtype, index, i);
    reg_id_t &subid = m_rename_map[rtype][index][i];
    subid.getARF()->print( subid, proc );                
  }
}

//***************************************************************************
void  arf_container_t::openRegisterType( rid_container_t rtype,
                                         int32 numElements )
{
  //   DEBUG_OUT("open reg\n");
  if (m_cur_type != CONTAINER_NUM_CONTAINER_TYPES) {
    ERROR_OUT("container_t: open register called while register already open.\n");
    SIM_HALT;
  }
  m_cur_type             = rtype;
  m_cur_element          = 0;
  m_max_elements         = numElements;
  m_dispatch_size[rtype] = numElements;
  m_dispatch_map[m_cur_type] = new reg_id_t[numElements];

  for ( uint32 i = 0; i < m_size; i++ ) {
    m_rename_map[m_cur_type][i] = new reg_id_t[numElements];
  }
}

//***************************************************************************
void  arf_container_t::addRegister( rid_type_t regtype, int32 offset,
                                    abstract_rf_t *arf )
{
  // DEBUG_OUT("add reg\n");
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
void  arf_container_t::closeRegisterType( void )
{
  //  DEBUG_OUT("close reg\n");
  if (m_cur_element != m_max_elements) {
    ERROR_OUT("container_t: addRegister called too few times %d (%d).\n",
              m_cur_element, m_max_elements);
    SIM_HALT;
  }

  /*
  for (uint32 i=0; i < m_dispatch_size[m_cur_type]; i++) {
    reg_id_t &rid = m_dispatch_map[m_cur_type][i];
    rid.getARF->print( rid );
  }
  */
  m_cur_type     = CONTAINER_NUM_CONTAINER_TYPES;
  m_cur_element  = -1;
  m_max_elements = -1;
}

/*------------------------------------------------------------------------*/
/* Control Register File                                                  */
/*------------------------------------------------------------------------*/

//***************************************************************************
void    arf_control_t::initialize( void )
{
  uint32 reg;
  
  // Control Register File:
  //    no mappings to set, but initialize some values.
  // do this for all logical processors:
 for(uint k=0; k < CONFIG_LOGICAL_PER_PHY_PROC; ++k){
     for ( uint32 i = 0; i < CONFIG_NUM_CONTROL_SETS; i++ ) {
       for (reg = CONTROL_PC; reg < CONTROL_NUM_CONTROL_TYPES; reg ++) {
         m_rf_array[k][i]->setWrittenCycle( reg, m_rf_array[k][i]->getSeq()->getLocalCycle() );
         //DEBUG_OUT("arf_control_t: initialize vanilla[ %d ] physical[ %d ] value[ %lld ]\n", i, reg, 0);
         m_rf_array[k][i]->setInt( reg, 0 );
         m_rf_array[k][i]->setFree( reg, false );
         if (reg == CONTROL_ISREADY) {
           // control isready indicates if this group is able to be used yet.
           // the set 0 is ok at the beginning of the program, others are not.
           if (i != 0) {
             m_rf_array[k][i]->setUnready( reg );
             m_rf_array[k][i]->setFree( reg, false );
           }
         }
       }
     }
 }   // end for all logical procs
}

//***************************************************************************
bool    arf_control_t::allocateRegister( reg_id_t &rid, uint32 proc )
{
  assert(proc < CONFIG_LOGICAL_PER_PHY_PROC); 
  // allocate a whole set of control registers
  uint32  nextset = (m_decode_rf[proc] + 1) % m_rf_count;
  if (nextset == m_retire_rf[proc]) {
    ERROR_OUT("error: arf_control: too few control register sets to continue\n");
    ERROR_OUT("     : you may need to increase the CONFIG_NUM_CONTROL_SETS variable\n");
    return false;
  }
  
   #ifdef DEBUG_REG
     DEBUG_OUT("arf_control_t:allocateRegister: calling isReady\n");
  #endif
  if ( m_rf_array[proc][nextset]->isReady( CONTROL_ISREADY ) ) {
    ERROR_OUT("error: arf_control: allocating set which is already active.  set[ %d ] retire_rf[ %d ] maxsets[ %d ] lasttimestamp[ %lld ] currenttimestamp[ %lld ]\n", nextset, m_retire_rf[proc], m_rf_count, m_rf_array[proc][nextset]->getWrittenCycle( CONTROL_ISREADY ), m_rf_array[proc][nextset]->getSeq()->getLocalCycle() );
    return false;
  }

  #ifdef DEBUG_REG
        DEBUG_OUT("arf_control_t:allocateRegister isReady END\n");
  #endif
  rid.setPhysical( nextset );
  physical_file_t *rf = m_rf_array[proc][nextset];
  rf->setUnready( CONTROL_ISREADY );
  rf->setFree( CONTROL_ISREADY, false );
  // update this register's timestamp
  rf->setWrittenCycle( CONTROL_ISREADY, rf->getSeq()->getLocalCycle() );

  m_decode_rf[proc] = nextset;

  //DEBUG_OUT("\narf_control_t allocateRegister set[ %d ] thread[ %d ]\n", nextset, proc);
 
  #ifdef DEBUG_RET
  DEBUG_OUT("arf_control_t:allocateRegister END proc[%d] physical[%d]\n",proc,nextset);
  #endif
  return true;
}

//***************************************************************************
bool    arf_control_t::freeRegister( reg_id_t &rid, uint32 proc )
{
 #ifdef DEBUG_RETIRE
    DEBUG_OUT("arf_control_t:freeRegister BEGIN proc[%d]\n",proc);
  #endif
  //proc arg is not used

  half_t  physical = rid.getPhysical();
  half_t prev = (physical -1) % m_rf_count;
  ASSERT(0 <= prev && prev < m_rf_count);

  /* DEBUG_OUT("arf_control: freeRegister: physical=%d decode=%d retire=%d\n",
     physical, m_decode_rf, m_retire_rf );
  */
  ASSERT( physical < m_rf_count );
  physical_file_t *rf = m_rf_array[proc][physical];
  physical_file_t *prev_rf = m_rf_array[proc][prev];
  rf->setUnready( CONTROL_ISREADY );
  rf->setFree( CONTROL_ISREADY, false );
  // update this register's timestamp
  rf->setWrittenCycle( CONTROL_ISREADY, rf->getSeq()->getLocalCycle() );

  //DEBUG_OUT("\narf_control_t freeRegister set[ %d ] thread[ %d ]\n", physical, proc);

    #ifdef DEBUG_RETIRE
    DEBUG_OUT("arf_control_t:freeRegister END proc[%d]\n",proc);
     #endif
  return (true);
}

//***************************************************************************
void    arf_control_t::readDecodeMap( reg_id_t &rid, uint32 proc )
{
  //proc arg not used
  rid.setPhysical( m_decode_rf[proc] );
}

//***************************************************************************
void    arf_control_t::writeRetireMap( reg_id_t &rid, uint32 proc )
{
  //proc arg not used
  half_t physical= rid.getPhysical();
  half_t next    = (m_retire_rf[proc] + 1) % m_rf_count;
  if ( physical != next ) {
    DEBUG_OUT( "warning: arf_control: retire map does not advance sequentially %d != %d\n", next, physical );
  }
  m_retire_rf[proc] = physical;
}

//***************************************************************************
void    arf_control_t::writeDecodeMap( reg_id_t &rid, uint32 proc )
{
  //proc arg not used
  half_t physical= rid.getPhysical();
  half_t prev    = (m_decode_rf[proc] - 1) % m_rf_count;
  if ( physical != prev ) {
    DEBUG_OUT( "warning: arf_control: decode map does not decrease sequentially %d != %d\n", prev, physical );
  }
  m_decode_rf[proc] = physical;
}

//***************************************************************************
bool    arf_control_t::isReady( reg_id_t &rid, uint32 proc )
{
   #ifdef DEBUG_REG
       DEBUG_OUT("arf_control_t: isReady BEGIN\n");
  #endif
  half_t physical = rid.getPhysical();
  ASSERT( physical < m_rf_count );
  /*  DEBUG_OUT( "arf_control: isready: phys=%d %d\n", physical,
      m_rf_array[physical]->isReady( CONTROL_ISREADY ) ); */
  bool ret = m_rf_array[proc][physical]->isReady(CONTROL_ISREADY);
  #ifdef DEBUG_REG
        DEBUG_OUT("arf_control_t: isReady END\n");
  #endif
  return ( m_rf_array[proc][physical]->isReady( CONTROL_ISREADY ) );
}


//**************************************************************************
void arf_control_t::setL2Miss( reg_id_t & rid, uint32 proc )
{
  half_t  physical = rid.getPhysical();
  
  if (physical == PSEQ_INVALID_REG) {
    DEBUG_OUT("setL2Miss: error: (control) unable to set INVALID register\n");
    ASSERT(0);
  }
  if(physical != PSEQ_INVALID_REG){
    ASSERT( physical < m_rf_count );
    half_t  reg  = rid.getVanilla();
    physical_file_t *rf = m_rf_array[proc][physical];
    rf->setL2Miss( reg, true );
  } 
}

//**************************************************************************
void arf_control_t::setWriterSeqnum( reg_id_t & rid, uint64 seqnum, uint32 proc )
{
  half_t  physical = rid.getPhysical();
  
  if (physical == PSEQ_INVALID_REG) {
    DEBUG_OUT("setWriterSeqnum: error: (control) unable to set INVALID register\n");
    ASSERT(0);
  }
  ASSERT( physical < m_rf_count );
  physical_file_t *rf = m_rf_array[proc][physical];
  half_t  reg  = rid.getVanilla();
  rf->setWriterSeqnum( reg, seqnum );
}

//**************************************************************************
uint64 arf_control_t::getWriterSeqnum( reg_id_t & rid, uint32 proc )
{
  half_t  physical = rid.getPhysical();
  
  if (physical == PSEQ_INVALID_REG) {
    DEBUG_OUT("getWriterSeqnum: error: (control) unable to set INVALID register\n");
    ASSERT(0);
  }
  ASSERT( physical < m_rf_count );
  physical_file_t *rf = m_rf_array[proc][physical];
  half_t  reg  = rid.getVanilla();
  return rf->getWriterSeqnum( reg );
}

//**************************************************************************
uint64 arf_control_t::getWrittenCycle( reg_id_t & rid, uint32 proc )
{
  half_t  physical = rid.getPhysical();
  
  if (physical == PSEQ_INVALID_REG) {
    DEBUG_OUT("getWrittenCycle: error: (control) unable to set INVALID register\n");
    ASSERT(0);
  }
  ASSERT( physical < m_rf_count );
  physical_file_t *rf = m_rf_array[proc][physical];
  half_t  reg  = rid.getVanilla();
  return rf->getWrittenCycle( reg );
}

//***************************************************************************
bool    arf_control_t::isL2Miss( reg_id_t &rid, uint32 proc )
{
  half_t physical = rid.getPhysical();
  ASSERT( physical < m_rf_count );
  half_t  reg  = rid.getVanilla();
  return ( m_rf_array[proc][physical]->isL2Miss( reg ) );
}

//***************************************************************************
void    arf_control_t::incrementL2MissDep( reg_id_t &rid, uint32 proc )
{
  half_t physical = rid.getPhysical();
  ASSERT( physical < m_rf_count );
  half_t  reg  = rid.getVanilla();

   #ifdef DEBUG_REG
     DEBUG_OUT("arf_control_t::incrementL2MissDep logical[ %d ] physical[ %d ] cycle[ %lld ]\n", physical, reg, m_rf_array[proc][physical]->getSeq()->getLocalCycle() );
       #endif

  m_rf_array[proc][physical]->incrementL2MissDep( reg );
}

//***************************************************************************
void    arf_control_t::decrementL2MissDep( reg_id_t &rid, uint32 proc )
{
  half_t physical = rid.getPhysical();
  ASSERT( physical < m_rf_count );
  half_t  reg  = rid.getVanilla();

     #ifdef DEBUG_REG
     DEBUG_OUT("arf_control_t::decrementL2MissDep logical[ %d ] physical[ %d ] cycle[ %lld ]\n", physical, reg, m_rf_array[proc][physical]->getSeq()->getLocalCycle() );
     #endif

  m_rf_array[proc][physical]->decrementL2MissDep( reg );
}

//***************************************************************************
int    arf_control_t::getL2MissDep( reg_id_t &rid, uint32 proc )
{
  half_t physical = rid.getPhysical();
  ASSERT( physical < m_rf_count );
  half_t  reg  = rid.getVanilla();
  return m_rf_array[proc][physical]->getL2MissDep( reg );
}

//***************************************************************************
void    arf_control_t::waitResult( reg_id_t &rid, dynamic_inst_t *d_instr, uint32 proc )
{
  half_t physical = rid.getPhysical();
  ASSERT( physical < m_rf_count );
  #ifdef DEBUG_REG
    DEBUG_OUT("arf_control_t::waitResult() control_set[ %d ] seq_num[ %d ]\n", physical, d_instr->getSequenceNumber());
  #endif
  m_rf_array[proc][physical]->waitResult( CONTROL_ISREADY, d_instr );
}

//***************************************************************************
half_t  arf_control_t::renameCount( reg_id_t &rid, uint32 proc )
{
  //proc arg not used
  // FIX: rename count not implemented for control registers.
  return 0;
}


//***************************************************************************
half_t    arf_control_t::getLogical( reg_id_t &rid, uint32 proc )
{
  //proc arg not used
 #ifdef DEBUG_ARF
  DEBUG_OUT("arf_control_t:getLogical BEGIN proc[%d]\n",proc);
 #endif

  half_t logical = rid.getVanilla();
 #ifdef DEBUG_ARF
  DEBUG_OUT("arf_control_t:getLogical END proc[%d]\n",proc);
 #endif

  return (logical);
}

//***************************************************************************
ireg_t  arf_control_t::readInt64( reg_id_t &rid, uint32 proc )
{
  half_t  rset = rid.getPhysical();
  half_t  reg  = rid.getVanilla();
  #ifdef DEBUG_REG
    DEBUG_OUT("\tarf_control_t: readInt64 vanilla[ %d ] physical[ %d ] value[ 0x%llx ]\n", rset, reg, m_rf_array[proc][rset]->getInt(reg));
  #endif
  return (m_rf_array[proc][rset]->getInt( reg ));
}

//***************************************************************************
void    arf_control_t::writeInt64( reg_id_t &rid, ireg_t value, uint32 proc )
{
  half_t  rset = rid.getPhysical();
  half_t  reg  = rid.getVanilla();

  m_rf_array[proc][rset]->setWrittenCycle( reg, m_rf_array[proc][rset]->getSeq()->getLocalCycle() );
  #ifdef DEBUG_REG
    DEBUG_OUT("\tarf_control_t: writeInt64 vanilla[ %d ] physical[ %d ] value[ 0x%llx ]\n", rset, reg, value);
  #endif
  if( value == -1 ){
    //DEBUG_OUT("arf_control_t: writeInt64 vanilla[ %d ] physical[ %d ] value[ %lld ]\n", rset, reg, value);
    //    ASSERT( value != -1 );
  }
  return (m_rf_array[proc][rset]->setInt( reg, value ));
}

//***************************************************************************
void    arf_control_t::initializeControl( reg_id_t &rid, uint32 proc )
{
  half_t  rset = rid.getPhysical();
  // copy all of the control registers from source to dest
  half_t  prev = (rset - 1) % m_rf_count;
  if ( rid.getPhysical() == PSEQ_INVALID_REG ) {
    ERROR_OUT("error: arf_control: initialize control fails- invalid register.\n");
    print( rid, proc );          
    return;
  }
   #ifdef DEBUG_REG
        DEBUG_OUT("arf_control_t: initializeControl: calling isReady\n");
  #endif
  if ( !m_rf_array[proc][prev]->isReady( CONTROL_ISREADY ) ) {
    ERROR_OUT("error: arf_control: initialize control fails- prev is uninitialized.\n");
    print( rid, proc );     
    return;
  }
 #ifdef DEBUG_REG
        DEBUG_OUT("arf_control_t: isReady END\n");
  #endif
  physical_file_t *source = m_rf_array[proc][prev];
  physical_file_t *dest   = m_rf_array[proc][rset];
  for (uint32 i = 0; i < (uint32) CONTROL_NUM_CONTROL_PHYS; i++) {
    dest->setWrittenCycle( i, dest->getSeq()->getLocalCycle() );
    #ifdef DEBUG_REG
      DEBUG_OUT("\tarf_control_t: initializeControl vanilla[ %d ] physical[ %d ] value[ 0x%llx ]\n", rset, i, source->getInt(i));
    #endif
    if( source->getInt(i) == -1 ){
      //DEBUG_OUT("arf_control_t: initializeControl vanilla[ %d ] physical[ %d ] value[ %lld ]\n", rset, i, source->getInt(i));
      //ASSERT( source->getInt(i) != -1 );
    }
    dest->setInt( i, source->getInt(i) );
  }  
}

//**************************************************************************
void    arf_control_t::finalizeControl( reg_id_t &rid, uint32 proc )
{
  // now flag the register set as 'ready'
  physical_file_t *dest   = m_rf_array[proc][rid.getPhysical()];
  dest->setWrittenCycle( CONTROL_ISREADY, dest->getSeq()->getLocalCycle() );
  #ifdef DEBUG_REG
    DEBUG_OUT("\tarf_control_t: finalizeControl vanilla[ %d ] physical[ %d ] value[ 0x%x ]\n", rid.getPhysical(), CONTROL_ISREADY, 1);
  #endif
  dest->setInt( CONTROL_ISREADY, 1 );  
}

//***************************************************************************
void    arf_control_t::check( reg_id_t &rid, pstate_t *state,
                              check_result_t *result, uint32 proc )
{
  assert(proc < CONFIG_LOGICAL_PER_PHY_PROC);

  // invalid control register: unable to check
  if ( rid.getVanilla() == CONTROL_UNIMPL )
    return;

  half_t  reg    = rid.getVanilla();
  ireg_t  simreg = state->getControl( reg,proc );
  if (result->update_only) {
    ASSERT( rid.getPhysical() != PSEQ_INVALID_REG );
    m_rf_array[proc][rid.getPhysical()]->setWrittenCycle( reg, m_rf_array[proc][rid.getPhysical()]->getSeq()->getLocalCycle() );
    #ifdef DEBUG_REG
      DEBUG_OUT("\tarf_control_t: check FUNCTIONAL vanilla[ %d ] physical[ %d ] new_value[ 0x%llx ]\n", rid.getPhysical(), reg, simreg);
    #endif
    if( simreg == -1 ){
      //DEBUG_OUT("arf_control_t: check vanilla[ %d ] physical[ %d ] value[ %lld ]\n", rid.getPhysical(), reg, simreg);
      //     ASSERT( simreg != -1 );
    }
    m_rf_array[proc][rid.getPhysical()]->setInt( reg, simreg );
    finalizeControl( rid, proc );
  } else {
    ireg_t  myreg  = m_rf_array[proc][m_retire_rf[proc]]->getInt( reg );
    // The condition code bits (FCC0-3) are renamed, so mask them off
    //   for the comparison and update if any
    // FIXFIXFIX
#if 0
    if (rid.getVanilla() == CONTROL_FSR ) {
      simreg = simreg & 0xfffff9ff;
      myreg  = myreg  & 0xfffff9ff;
    }
#endif
    if ( simreg != myreg ) {
      // certain registers are not maintained, or are register renamed
      if ( !(reg == CONTROL_PC ||
             reg == CONTROL_NPC ||
             reg == CONTROL_CCR ||
             reg == CONTROL_TICK ||
             reg == CONTROL_TICK_CMPR) ) {
        if (result->verbose)
          DEBUG_OUT( "patch %%control %s (%d )-- 0x%0llx 0x%0llx\n",
                     pstate_t::control_reg_menomic[reg], reg,
                     myreg, simreg );
        result->perfect_check = false;
      }
      m_rf_array[proc][m_retire_rf[proc]]->setWrittenCycle( reg, m_rf_array[proc][m_retire_rf[proc]]->getSeq()->getLocalCycle() );
      if(myreg == -1){
        //ERROR_OUT("arf_control_t: check 2 FAIL vanilla[ %d ] physical[ %d ] new_value[ 0x%llx ] old_value[ 0x%llx ]\n", m_retire_rf[proc], reg, simreg, myreg);
      }
      if( simreg == -1 ){
        //DEBUG_OUT("arf_control_t: check 2 vanilla[ %d ] physical[ %d ] value[ %lld ]\n", m_retire_rf[proc], reg, simreg);
        //        ASSERT( simreg != -1 );
      }
 #ifdef DEBUG_REG
      DEBUG_OUT("\tarf_control_t: check FAIL vanilla[ %d ] physical[ %d ] new_value[ 0x%llx ]\n", rid.getPhysical(), reg, simreg);
 #endif
      m_rf_array[proc][m_retire_rf[proc]]->setInt( reg, simreg );
    }
    else{
      //check passed
      //DEBUG_OUT("arf_control_t: check 2 PASSED vanilla[ %d ] physical[ %d ] new_value[ 0x%llx ] old_value[ 0x%llx ]\n", m_retire_rf[proc], reg, simreg, myreg);
    }
  }
}

//***************************************************************************
bool    arf_control_t::regsAvailable( uint32 proc )
{
  uint32  nextset = (m_decode_rf[proc] + 1) % m_rf_count;
  // at least one control register remaining
  return (nextset != m_retire_rf[proc]);
}

//***************************************************************************
void    arf_control_t::addFreelist( reg_id_t &rid )
{
  // control register files are unable to be leaked -- they are not
  // handled using the register map.
}

/*------------------------------------------------------------------------*/
/* Accessor(s) / mutator(s)                                               */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Private methods                                                        */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Static methods                                                         */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Global functions                                                       */
/*------------------------------------------------------------------------*/

