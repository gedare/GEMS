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
#ifndef _REGFILE_H_
#define _REGFILE_H_

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include "wait.h"

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Class declarations                                                     */
/*------------------------------------------------------------------------*/

/**
 * Implements a 64-bit physical register, able to be read as an
 * integer, float, or double.
 */
union my_register_u {
  uint64   uint_64;
  uint32   uint_32;
  int64    int_64;
  int32    int_32;
  double   float_64;
  float    float_32;
};

/// typedef of a register union type
typedef union my_register_u my_register_t;

union register_cc_u {
  unsigned char   ccode;
};

typedef union register_cc_u register_cc_t;

/*****************************************************************/
/********************** Physical Register ************************/
/*****************************************************************/

/** 
*  A physical register holds one of the inflight values associated
*  with an architectural register. 
*
*  The free bit indicates whether the register has been allocated, 
*  and the ready bit tracks whether the producer has produced yet.
*
* @author  zilles
* @version $Id$
*/

class physical_reg_t {

public:
  /** anonymous union: accesss this register as a FP or INT register */
  union {
         ireg_t   as_int;
         float32  as_float;
  };

  /** pointer to a list of instructions waiting on this value */
  wait_list_t wait_list;

  /** indicates if this register is ready to be consumed yet */
  bool        isReady;

  /** indicates if this register is free or not */
  bool        isFree;

  /** indicates whether this register is the destination of a load that had L2 miss */
  bool l2Miss;

  /** indicates how many instructions are dependent on this register during L2 miss */
  int l2MissDependents;

  /** indicates the seqnum of the instr that wrote this register */
  uint64 writerSeqnum;

  /** indicates the cycle when register was written */
  uint64 writtenCycle;
};

/*****************************************************************/
/******************** Physical Register File *********************/
/*****************************************************************/

/**
* The physical register file in the microprocessor.
*
* @author  zilles
* @version $Id$
*/
class physical_file_t {

public:

  /**
   * @name Constructor(s) / Destructor
   */
  //@{
  /** Constructor: creates an empty physical register file */
  physical_file_t( uint16 num_physical, pseq_t *seq);

  //in order to enable SMT support need to be able to declare array of physical_file_t 
  //           objects, which requires an empty constructor...see variable m_control_rf in pseq.C
  //           for details
  physical_file_t() { }

  /** Destructor: frees the object */
  ~physical_file_t();
  //@}

  /**
   * @name Accessors / Mutators
   */
  //@{
  /** tests if a register has its results "ready" for the next dependent
   *  operation.
   *  @param  i    The physical register number to test.
   *               Limit: i must be less than DEV_NULL_DEST
   *  implicit     The PSEQ_ZERO_REG is %g0 (alpha r31) == always 0 && ready
   *  @return bool True if the operation is ready or not.
   */
  bool isReady(uint16 reg_no) const {
      if(reg_no >= m_num_physical){
      #ifdef DEBUG_REG
          DEBUG_OUT("regfile.h: isReady: ERROR reg_no[%d] max_physical[%d]\n",
                reg_no, m_num_physical);
      #endif
        }
    ASSERT(reg_no < m_num_physical);
    return ( m_reg[reg_no].isReady );
  }

  //returns whether this register is object of L2 miss...
  bool isL2Miss(uint16 reg_no) const {
    ASSERT(reg_no < m_num_physical);
    return (m_reg[reg_no].l2Miss);
  }

  // increment the number of L2 miss dependents
  void incrementL2MissDep(uint16 reg_no) {
    ASSERT(reg_no < m_num_physical);
    m_reg[reg_no].l2MissDependents++;
  }

  // decrement the number of L2 miss dependents
  void decrementL2MissDep(uint16 reg_no) {
    ASSERT(reg_no < m_num_physical);
    if(m_reg[reg_no].l2MissDependents == 0){
      ERROR_OUT("decrementL2MissDep, counter is already zero, reg_no[ %d ]\n", reg_no);
    }
    ASSERT(m_reg[reg_no].l2MissDependents > 0);
    #ifdef DEBUG_REG
       DEBUG_OUT("decrementL2MissDep, reg_no[ %d ] l2missDepcount[ %d ]\n", reg_no, m_reg[reg_no].l2MissDependents);
    #endif
    m_reg[reg_no].l2MissDependents--;
  }

  //return the number of L2 miss dependents
  int getL2MissDep(uint16 reg_no) const {
    ASSERT(reg_no < m_num_physical);
    return m_reg[reg_no].l2MissDependents;
  }

  void setL2Miss(uint16 reg_no, bool value) {
    ASSERT(reg_no < m_num_physical);
    m_reg[reg_no].l2Miss = value;
  }

  void setWriterSeqnum( uint16 reg_no, uint64 seqnum) {
    ASSERT(reg_no < m_num_physical);
    m_reg[reg_no].writerSeqnum = seqnum;
  }

  uint64 getWriterSeqnum( uint16 reg_no) const {
    ASSERT(reg_no < m_num_physical);
    return m_reg[reg_no].writerSeqnum;
  }

  void setWrittenCycle( uint16 reg_no, uint64 cycle) {
    ASSERT(reg_no < m_num_physical);
    m_reg[reg_no].writtenCycle = cycle;
  }

  uint64 getWrittenCycle( uint16 reg_no) const {
    ASSERT(reg_no < m_num_physical);
    return m_reg[reg_no].writtenCycle;
  }

  /** Unsets the ready bit for a register --
   *  implicit     No operation must be waiting on this register.
   *  @param  i    The physical register number to test.
   *               Limit: i must be less than DEV_NULL_DEST
   */
  void setUnready(uint16 reg_no) {
    ASSERT(reg_no < m_num_physical);
    ASSERT( m_reg[reg_no].wait_list.Empty() );
    m_reg[reg_no].isReady = false;
    m_reg[reg_no].isFree  = true;
    //reset L2 miss flag
    m_reg[reg_no].l2Miss = false;
    //reset number of dependents
    m_reg[reg_no].l2MissDependents = 0;
    //reset writer info
    m_reg[reg_no].writerSeqnum = 0;
    m_reg[reg_no].writtenCycle = 0;
  }

  /** Gives the size of the physical register file.
   *  @return uint16 the size of the physical register file */
  uint16 getNumRegisters( void ) const {
    return (m_num_physical);
  }

  /** Get an integer value.
   *  The register must be ready to be read when this function is called.
   *  @param reg_no The register to read. Must be < REG_NUM_ACTUAL.
   *  @return uint64 The value in the register.
   */
  uint64 getInt(uint16 reg_no) const {
    ASSERT (reg_no < m_num_physical);
    #ifdef DEBUG_REG
    DEBUG_OUT("regfile.h:getInt isReady BEGIN\n");
    #endif
    if (!isReady(reg_no)) {
      dependenceError( reg_no );
      DEBUG_OUT("regfile.h:getInt ERROR: reg NOT READY\n");
      return 0;
    }
     #ifdef DEBUG_REG
    DEBUG_OUT("regfile.h:getInt isReady END\n");
    #endif
    return m_reg[reg_no].as_int;
  }
  
  /** Set an integer value.
   *  @param reg_no The register to read. Must be < REG_NUM_ACTUAL.
   *  @param value The value to write in the register 
   */
  void   setInt(uint16 reg_no, uint64 value) {
    if( reg_no >= m_num_physical ){
      ERROR_OUT("setInt() ERROR exceeding physical regfile reg_no[ %d ] max_physical[ %d ]\n", reg_no, m_num_physical);
    }
    ASSERT (reg_no < m_num_physical);
#if 0
    if ( m_watch == reg_no ) {
      DEBUG_OUT("WATCHPOINT: writing register: %d value: 0x%0llx\n",
                reg_no, value);
    }
#endif
    m_reg[reg_no].as_int = value;
    setReady(reg_no);
  }

  /** Get an single floating point value.
   *  The register must be ready to be read when this function is called.
   *  @param  reg_no  The register to read. Must be < REG_NUM_ACTUAL.
   *  @return float32 The value in the register.
   */
  float32 getFloat(uint16 reg_no) const {
    ASSERT (reg_no < m_num_physical);
     #ifdef DEBUG_REG
    DEBUG_OUT("regfile.h:getFloat isReady BEGIN\n");
    #endif
    if (!isReady(reg_no)) {
      dependenceError( reg_no );
      ERROR_OUT("regfile.h:getFloat ERROR reg NOT READY\n");
      return 0.0;
    }
     #ifdef DEBUG_REG
    DEBUG_OUT("regfile.h:getFloat isReady END\n");
    #endif
    return m_reg[reg_no].as_float;
  }

  /** Set an single floating point value.
   *  @param reg_no The register to read. Must be < REG_NUM_ACTUAL.
   *  @param value The value to set in the register.
   */
  void    setFloat(uint16 reg_no, float32 value) {
    if( reg_no >= m_num_physical ){
      ERROR_OUT("setFloat() ERROR exceeding physical regfile reg_no[ %d ] max_physical[ %d ]\n", reg_no, m_num_physical);
    }
    ASSERT (reg_no < m_num_physical);
    m_reg[reg_no].as_float = value;
    setReady(reg_no);
  }
  //@}

  /** @name Instructions waiting for registers */
  //@{  
  /// wait on this register to be written (i.e. to become ready)
  void    waitResult( uint16 reg_no, dynamic_inst_t *d_instr );
  
  /// is the wait list empty
  bool    isWaitEmpty( uint16 reg_no );

  /// set the physical register free bit to "value"
  void    setFree(uint16 reg_no, bool value) {
    ASSERT(reg_no < m_num_physical);
    m_reg[reg_no].isFree  = value;
  }

  /** sets a watch point on a physical register. used for debugging. */
  void   setWatchpoint( uint16 reg_no ) {
    m_watch = reg_no;
  }

  /// print out a particular physical register
  void    print( uint16 reg_no );
  //@}

  pseq_t * getSeq(){
    return m_pseq;
  }

protected:

  /** sets a register has having its results "ready" for the next dependent
   *  operation. Implicitly called when setInt() is called -- 
   *  should not be called anywhere else.
   *
   *  @param  i    The physical register number to test.
   *               Limit: i must be less than DEV_NULL_DEST
   *  implicit     The PSEQ_ZERO_REG is %g0 (alpha r31) == always 0 && ready
   */
  void setReady(uint16 reg_no) {
    // already have checked this: ASSERT(reg_no < m_num_physical);
    m_reg[reg_no].isReady = true;
    // wakeup all instructions waiting for this register to be written
    m_reg[reg_no].wait_list.WakeupChain();
  }

  void dependenceError( uint16 reg_no ) const;

  /** number of physical registers in this file */
  uint16          m_num_physical;

  /** number of actual (allocated) registers -- to account for "virtual regs"*/
  uint16          m_num_actual;

  /** watch point on a physical register in the machine */
  uint16          m_watch;

  /** physical registers in machine */
  physical_reg_t *m_reg;

  /** owning sequencer: used for debugging  */
  pseq_t         *m_pseq;

};

#endif /* _REGFILE_H_ */
