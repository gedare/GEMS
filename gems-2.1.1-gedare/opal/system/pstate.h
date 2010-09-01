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
#ifndef _PSTATE_H_
#define _PSTATE_H_

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

/// pstate: successful execution
const uint16  PS_EXEC_SUCCESS  =  0x0;

/// pstate: unsuccessful execution
const uint16  PS_EXEC_FAIL     =  0x1;

/// pstate: no interest in this instruction
const uint16  PS_NO_INTEREST   =  0x2;

/// mapping of logical to physical addresses
typedef map<la_t, pa_t> AddressMapping;

/*------------------------------------------------------------------------*/
/* Class declarations                                                     */
/*------------------------------------------------------------------------*/

/** This object represents the data in an in-order implementation of the
 *  the processors's core.
 *
 * @see     pstate_t
 * @author  cmauer
 * @version $Id: pstate.h,v 3.27 2004/08/03 20:16:34 lyen Exp $
 */

typedef struct core_state {

  /// The global registers (4 sets)
  ireg_t   global_regs[MAX_GLOBAL_REGS];
  /// The windowed (not including global) registers
  ireg_t   int_regs[MAX_INT_REGS];
  /// The floating point registers in the processor
  freg_t   fp_regs[MAX_FLOAT_REGS];
  /// The control registers in the processor
  ireg_t   ctl_regs[MAX_CTL_REGS];

} core_state_t;

/**
* This object represents a single processor's core state.
*
* The basic state of the processor is the integer, floating point,
* and control registers. This class provides an abstract interface to access
* and modify the registers. Additionally, it abstracts reads/writes to
* memory.
*
* The state contained in this object is used by the sequencer which maintains
* timing information for functional units, etc ...
*
* @see     pseq_t
* @author  cmauer
* @version $Id: pstate.h,v 3.27 2004/08/03 20:16:34 lyen Exp $
*/
class pstate_t {

public:

  /**
   * @name Constructor(s) / Destructor
   */
  //@{
  /**
   * Constructor: reads machine state from simics to create processor.
   * @param id The unique integer processor identifier
   */
  pstate_t( int id );

  /**
   * Destructor: frees object.
   */
  ~pstate_t();
  //@}

  /**
   * @name Methods
   */
  //@{

  /// save the current state of the processor
  void checkpointState( );

  /// recover the current state of the processor:
  ///   Warning: this may crash simics. DONT DO THIS!
  void recoverState( );

  /** Step the processor a number of instructions -- This is used to debug
   *  instruction execution.
   */
  void   step( int num_steps );
  /** execute a single instruction on this processor state.
   *  updates the statistical information, just like step
   *  @return bool true if the instruction was supposed to be correct. */
  uint16 execute( static_inst_t *s_instr, pstate_t *gold_state );
  //@}

  /**
   * @name Accessors / Mutators
   */
  //@{
  /// raw accessors directly call SIMICS api (now that its stable) */
  /// read an integer register relative to a cwp
  ireg_t  getIntWp( unsigned int reg, int cwp, unsigned int proc ) {
    assert(proc < CONFIG_LOGICAL_PER_PHY_PROC);

    if (CONFIG_IN_SIMICS) 
      return (m_sparc_intf[proc]->read_window_register( m_cpu[proc], cwp, reg ));
    else
      return (intIntWp(reg, cwp, proc));
  }
  /// read an integer register relative to the global set
  ireg_t  getIntGlobal( unsigned int reg, int global_set, unsigned int proc ) {
    assert(proc < CONFIG_LOGICAL_PER_PHY_PROC);

    if (CONFIG_IN_SIMICS) 
      return (m_sparc_intf[proc]->read_global_register( m_cpu[proc], global_set, reg ));
    else
      return (intIntGlobal(reg, global_set, proc));
  }
  /**
   * read an floating point register.
   * This returns a pointer to a double precision floating point number.
   * This interface is defined for registers 0 .. 62, whereas IntDouble is
   * defined for 0 .. 31 (hence the /2).
   */
  freg_t  getDouble( unsigned int reg, uint32 proc ) {
    assert(CONFIG_LOGICAL_PER_PHY_PROC > 0);
    assert(proc < CONFIG_LOGICAL_PER_PHY_PROC);

    if (CONFIG_IN_SIMICS) 
      return (m_sparc_intf[proc]->read_fp_register_x( m_cpu[proc], reg ));
    else
      return (intDouble(reg/2, proc));
  }
  /** read a control register
   *  Control registers numbers are defined by 
   *      SIM_get_control_register_number( "y" )
   * where "y" is some sparc control register name.
   */
  ireg_t  getControl( unsigned int reg, unsigned int proc ) {
    assert(proc < CONFIG_LOGICAL_PER_PHY_PROC);
   #ifdef DEBUG_PSEQ
     DEBUG_OUT("pstate.h:getControl: Reading from m_id[%d] m_cpu[%0x]\n",m_id,m_cpu[proc]);
   #endif
    if (CONFIG_IN_SIMICS) {
      int32 sim_reg = m_control_map[proc][reg];
      if (sim_reg == -1){
        ERROR_OUT("pstate::getControl() ERROR trying to read ctrl reg not mapped by Opal to Simics reg reg[ %d ]\n", reg);
        return 0;
      }
      return (SIM_read_register( m_cpu[proc], m_control_map[proc][reg] ));
    } else {
      return (intControl( reg, proc ));
    }
  }

  /** check to see if a control register is valid or not.
   * @return bool true if the register is valid, false if not
   */
  bool    isValidControlRegister( uint32 reg, unsigned int proc ) {
    assert(proc < CONFIG_LOGICAL_PER_PHY_PROC);
    //    if(proc >= CONFIG_LOGICAL_PER_PHY_PROC){
    //  ERROR_OUT("p_state_t isValidControlRegister: the proc value isnt between 0 and CONFIG_LOGICAL_PER_PHY_PROC %d\n",proc);
    // return false;
    //}
    if ( reg >= MAX_CTL_REGS ) {
      ERROR_OUT("error: pstate_t: this isn\'t a valid control register\n");
      return false;
    }
    return m_valid_control_reg[proc][reg];
  }

  /** read memory locations
   *
   * @param log_addr The logical address to read from.
   * @param size The number of bytes to read
   * @param sign_extend bool: true if this should be sign extended
   * @param result_reg A pointer to where the result should be written
   * @param proc       The logical processor number (from 0 to CONFIG_LOGICAL_PER_PHY_PROC)
   * @return bool true if the read succeeds, false if TBE access fails
   */
  bool  readMemory( ireg_t log_addr, int size, bool sign_extend,
                    ireg_t *result_reg, unsigned int proc );

  /** read a physical memory location:
   *  same as read memory */
  bool  readPhysicalMemory( pa_t phys_addr, int size,
                            ireg_t *result_reg, unsigned int proc );
  
  /** write memory locations
   *
   * @param log_addr The logical address to write to.
   * @param size The number of bytes to write
   * @param value The value to write
   * @return bool true if the write succeeds, false if TBE access fails
   */
  bool  writeMemory( ireg_t log_addr, int size, ireg_t value );

  /** translate from a instruction virtual address to a physical address
   *
   * @param log_addr The logical address to translate
   * @param size The number of bytes to be read/written
   * @param phys_add The physical address to be returned.
   * @param proc        The logical proc number
   * @returns bool True if the translation succeeds, false if not.
   */
  bool translateInstruction( ireg_t log_addr, int size,
                             pa_t &phys_addr, unsigned int proc );

  /** translate from a logical (virtual) address to a physical address
   *
   * @param log_addr The logical address to translate
   * @param size The number of bytes to be read/written
   * @param phys_add The physical address to be returned.
   * @param proc        The logical proc number
   * @returns bool True if the translation succeeds, false if not.
   */
  bool  translate( ireg_t log_addr, int size, pa_t &phys_addr, unsigned int proc );

  /** dereference a pointer using the target (simulated) system.
   */
  la_t  dereference( la_t ptr, uint32 size, unsigned int proc );
  //@}

  /**
   * @name Package Interfaces: should be used only in sequencer!
   */
  //@{
  /** Multiprocessor mode: enable this processor.
   */
  void    enable( void ) {
    for(uint k=0; k <  CONFIG_LOGICAL_PER_PHY_PROC; ++k){
      SIM_enable_processor( m_cpu[k] );
    }
  }

  /** Multiprocessor mode: disable this processor.
   */
  void    disable( void ) {
    for(uint k=0; k <  CONFIG_LOGICAL_PER_PHY_PROC; ++k){
      SIM_disable_processor( m_cpu[k] );
    }
  }
  
  /** Multiprocessor continue: enable this processor, continue some steps,
   *  then disable again.
   */
  void    simcontinue( uint32 numsteps, unsigned int proc );

  /** Returns the current context from the MMU from SimIcs */
  int     getContext(uint32 proc );
  int     getSecondaryContext(uint32 proc);
  int     getNucleusContext(uint32 proc);

  int     readContext( uint16 asi, uint32 proc);
  

  /** get the current "time" as defined by simics
   *  @return pc_step_t The current "time" in steps
   */
  pc_step_t      getTime(unsigned int proc ) {
    return ( SIM_step_count( m_cpu[proc] ) ) ;
  }
  
  /** Cache-Timing Mode:
   *      When *not* simulating out-of-order processing, you can "unstall"
   *      a memory request by calling this API. This should be called when
   *      ruby returns a 'HIT' signal.
   */
  void    unstall(unsigned int proc ) {
    SIM_stall_cycle( m_cpu[proc], 0 );
  }

  /** Do absolutely everything you can to determine what simics thinks
   *  a particular instruction is. Providing "0" for any value will cause
   *  it to not try a particular approach.
   */
  void    askSimics( uint32 instr, pa_t paddr, la_t addr, unsigned int proc );

  /** Returns the time (# of simics cycles) for the current processor */
  cycles_t   getSimicsCycleCount( unsigned int proc ) {
    return SIM_cycle_count( m_cpu[proc] );
  }

  /** Returns a pointer to the current state -- use this only for tracing!!  */
  core_state_t  *getProcessorState( uint32 proc );

  /** returns a pointer to this processors conf object.
   *
   *  Offically, all simics related interfaces have to go through the
   *  pstate object -- however, we want to support
   *  memory transactions for the sequencer.
   */
  conf_object_t *getSimicsProcessor(unsigned int proc ) {
    assert(m_cpu[proc] != NULL);
    return m_cpu[proc];
  }

  /** returns a pointer to this processors MMU conf object.
   *
   *  Offically, all simics related interfaces have to go through the
   *  pstate object -- however, we want to support
   *  memory transactions for the sequencer.
   */
  conf_object_t *getSimicsMMU(unsigned int proc ) {
    return m_conf_mmu[proc];
  }

  /** updates this processor's state with a different state.
   *  This is useful for overwriting speculative state with correct non-spec
   *  state.
   */
  void           copyProcessorState( pstate_t *correct_state );
  //@}

  /** @name Static Variables */
  //@{
  /** map windowed integer registers to flat simics registers */
  static int  pstateWindowMap( int cwp, unsigned int reg );

  /** copies the name of the mmu for processor number "id" in to mmuname.
   *  The m_cpu must be initialized before calling this function.
   *  mmuname must be pre-allocated and have enough space 
   *  (typically 100 characters for safety). */
  static void getMMUName( int id, char *mmuname, int len );

  /**
   *  This gives a menomic for each control register defined in hfatypes.h
   */
  static const char  *control_reg_menomic[CONTROL_NUM_CONTROL_TYPES];

  /**
   *  This gives a menomic for the branch types defined hfatypes.h
   */
  static const char  *branch_type_menomic[BRANCH_NUM_BRANCH_TYPES];

  /** This gives a menomic for branch predictors defined in hfatypes.h */
  static const char  *branch_predictor_type_menomic[BRANCHPRED_NUM_BRANCH_TYPES];

  /**
   *  This gives the name of each trap defined by the sparc architecture. */
  static const char *trap_num_menomic( trap_type_t traptype );

  /**
   *  This gives the name of each asynchronous exception in the architecture */
  static const char *async_exception_menomic( exception_t except_type );

  /**
   *  This gives the name of each function unit type in the architecture */
  static const char *fu_type_menomic( fu_type_t fu_type );

  /**
   *  This translates a given PSTATE value into a global set
   */
  static uint16      getGlobalSet( ireg_t reg_pstate ) {
    if ((reg_pstate & 0x1) == 1) {
      // alt globals
      return REG_GLOBAL_ALT;
    } else if ((reg_pstate >> 10) & 0x1 == 1) {
      // MMU globals
      return REG_GLOBAL_MMU;
    } else if ((reg_pstate >> 11) & 0x1 == 1) {
      // Interrupt globals
      return REG_GLOBAL_INT;
    }
    // normal globals
    return REG_GLOBAL_NORM;
  }
  
  /**
   *  This gives if a given ASI is cacheable or not
   */
  static bool        is_cacheable( uint16 asi ) {
    return ( (bool) m_asi_is_cacheable[asi] );
  }

  /**
   *  showme_op specifies a information should be printed for a
   *  single instruction of interest.
   *  It is used for debugging implementations of specific instructions.
   *  This should only be used in pstate_t.C and hfa.C.
   */
  static int showme_op;
  //@}

  void checkSparcIntf(){
    ASSERT(m_sparc_intf != NULL);
  }

  /**
   *    MMU register read interface
   */
  bool getMMURegisterValue( uint16 asi, la_t va, uint32 proc, ireg_t & val);
  
  /** Used to read certain control registers, given an ASI and VA pair */
  bool readControlReg( uint16 asi, la_t va, uint32 proc, ireg_t & val );
private:
  // helper function to convert ASI + VA into a Simics MMU register attribute name
  char * getMMUAttributeName( uint16 asi, la_t va);

  /** converts a ASI, VA pair into the Opal control reg number */
  control_reg_t getOpalControlReg( uint16 asi, la_t va );

  /** internal versions of get_ functions */
  /// read an integer register relative to a cwp
  ireg_t  intIntWp( unsigned int reg, int cwp, unsigned int proc );

  /// read an integer register relative to the global set
  ireg_t  intIntGlobal( unsigned int reg, int global_set, unsigned int proc );
  /**
   * read an floating point register.
   * This returns a pointer to a double precision floating point number.
   */
  freg_t  intDouble( unsigned int reg, uint32 proc );

  /** read a control register
   *  Control registers numbers are defined by
   *      SIM_get_control_register_number( "y" )
   * where "y" is some sparc control register name.
   */
  ireg_t  intControl( unsigned int reg, unsigned int proc );

  /** save the current state of the processor
   *  @param a_state Writes the current state into this pointer.
   */
  void checkpointState( core_state_t *a_state );

  /// recover the current state of the processor:
  void recoverState( core_state_t *a_state );

  /// The unique id of this processor
  int                m_id;

  /// A pointer to the processor conf object
  conf_object_t ** m_cpu;

  /// A pointer to the sparc v9 interface
  sparc_v9_interface_t ** m_sparc_intf;

  /// A pointer to the Simics data structure for this processor's mmu
  conf_object_t ** m_conf_mmu;

  /// The current registers in this processor
  /// all logical procs should have their own core_state_t (because TESTER depends on static
  ///            size of core_state_t struct!)
  core_state_t    *   m_state;
  
  /// A cached copy of common memory translations
  // AddressMapping is of type map<la_t, pa_t>
  AddressMapping ** m_translation_cache;

  /// A map from our control register type -> simics's control register type
  int32 ** m_control_map;

  /** An array of boolean values, this provides a "valid" bit for all
   *  control registers from 0 to MAX_CTL_REGS, indicating which ones are
   *  in fact used by simics.
   */
  bool ** m_valid_control_reg;

  /// A boolean array that indicates if an ASI is cacheable or not
  static char       *m_asi_is_cacheable;

  /// maps the cacheable ASIs to the context ids
  // The ids are assigned as follows:
  //   0 = PRIMARY
  //   1 = SECONDARY
  //   2 = NUCLEUS
  static char     * m_asi_ctxt_id;
};

/*------------------------------------------------------------------------*/
/* Global variables                                                       */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Global functions                                                       */
/*------------------------------------------------------------------------*/

/**
 * pseudo-static package interface:
 *   pstate_step_callback: callback function after a single step on the cpu.
 * @param obj      The cpu object
 * @param mypstate The processor state.
 */
extern void  pstate_step_callback( conf_object_t *obj, void *mypstate );

/**
 * compares two processor states.
 * @param state_1 The first processor state to compare
 * @param state_2 The second processor state to compare
 * @return bool: true if the two are equivalent, false if not
 */
extern bool  ps_compare( core_state_t * ps_1, core_state_t * ps_2 );
/**
 * same as ps_compare, but instead of printing out differences, it just
 * returns true for perfect match, and false for not.
 * @param state_1 The first processor state to compare
 * @param state_2 The second processor state to compare
 * @return bool: true if the two are equivalent, false if not
 */
extern bool  ps_silent_compare( core_state_t * ps_1, core_state_t * ps_2 );

/** prints out the processor state */
extern void  ps_print( core_state_t *ps );

#endif /* _PSTATE_H_ */
