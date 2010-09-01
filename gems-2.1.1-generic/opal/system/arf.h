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
#ifndef _ARF_H_
#define _ARF_H_

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Class declaration(s)                                                   */
/*------------------------------------------------------------------------*/

/**
* Abstract register file super-class - implements a register interface for
* all logical and physical registers, based on which register type is
*
* @see     reg_box_t
* @author  cmauer
* @version $Id$
*/
class abstract_rf_t {
public:
  /**
   * Constructor: creates object
   * NOTE: all three pointers to this class can be NULL.
   */
  abstract_rf_t( physical_file_t *rf, reg_map_t *decode_map, 
                 reg_map_t *retire_map, uint32 reserves ) {
    m_rf = rf;
    m_decode_map = decode_map;
    m_retire_map = retire_map;
    m_reserves = reserves;
    initialize();
  }

  //in order to support arrays of arf objects, need to have an empty constructor
  abstract_rf_t() { }

  virtual ~abstract_rf_t( void ) {
    // not responsible for allocating, or freeing the rfs or maps
  };

  /**
   * @name Register Interfaces
   */
  //@{
  /** allocate a new physical register.
   *  @sideeffect  Modifies the rid field: m_physical.
   *  @return      Boolean: true if allocation succeeds.
   */
  virtual bool    allocateRegister( reg_id_t &rid, uint32 proc );

  /** frees the physical register specified in the m_physical field */
  virtual bool    freeRegister( reg_id_t &rid, uint32 proc );

  /** sets the register as dependent on L2 miss */
  virtual void setL2Miss( reg_id_t & rid, uint32 proc );

  /** sets the sequence number of writer */
  virtual void setWriterSeqnum( reg_id_t & rid, uint64 seqnum, uint32 proc );

  /** returns writer's seqnum */
  virtual uint64 getWriterSeqnum( reg_id_t & rid, uint32 proc);

  /** returns written cycle */
  virtual uint64 getWrittenCycle( reg_id_t & rid, uint32 proc );

  /** returns whether the register is dependent on L2 Miss */
  virtual bool isL2Miss( reg_id_t & rid, uint32 proc );

  /** increments L2 miss dependent counter */
  virtual void incrementL2MissDep( reg_id_t & rid, uint32 proc);

  /** decrements L2 miss dependent counter */
  virtual void decrementL2MissDep( reg_id_t & rid, uint32 proc);

  virtual int getL2MissDep( reg_id_t & rid, uint32 proc);
  
  /** sets the map from virtual register (vanilla) to physical register
   *  @sideeffect  Modifies the register mapping. */
  virtual void    writeRetireMap( reg_id_t &rid, uint32 proc );

  /** sets the map from virtual register (vanilla) to physical register
   *  @sideeffect  Modifies the register mapping. */
  virtual void    writeDecodeMap( reg_id_t &rid, uint32 proc );
  
  /** reads the current map for the vanilla register, 
   *  updating the physical register.
   *  @sideeffect  Modifies the rid field: m_physical. */

  virtual void readDecodeMap(reg_id_t &rid, uint32 proc);

  /** wait until this register is written, then wakeup. */
  virtual void    waitResult( reg_id_t &rid, dynamic_inst_t *d_instr, uint32 proc );   

  /** returns true if registers are still available */
  virtual bool    regsAvailable( uint32 proc );

  /** tests the physical register to see if it is ready.
   *  @return bool  true if ready, false if not. */
  virtual bool    isReady( reg_id_t &rid, uint32 proc );
  
  /** check this register against simics's result */
  virtual void    check( reg_id_t &rid, pstate_t *state,
                         check_result_t *result, uint32 proc );
  
  /**  Given an array of physical registers representing the free registers
   *   in a machine, unset the bit(s) representing any registers that are
   *   used by this reg_id.
   *   Used to validate that there are no register file leaks.
   */
  virtual void    addFreelist( reg_id_t &rid );

  /** print out this register's mapping */
  virtual void    print( reg_id_t &rid, uint32 proc );

  /** returns the number of times this logical register has been renamed */
  virtual half_t  renameCount( reg_id_t &rid, uint32 proc );

  /** computes the logical register number associated with this register
   *  @returns The logical register number of this register identifier. */

  //the version with the logical proc number
  virtual half_t  getLogical( reg_id_t &rid, uint32 proc );

  //@}

  /**
   * @name Read and write register interfaces
   */
  //@{
  /** read this register -- generic interface */
  virtual my_register_t readRegister( reg_id_t &rid, uint32 proc );

  /** write this register -- generic interface */
  virtual void          writeRegister( reg_id_t &rid, my_register_t value, uint32 proc );

  /** read this register as a integer register */
  virtual ireg_t  readInt64( reg_id_t &rid, uint32 proc );
  
  /** read this register as a single precision floating point register */
  virtual float32 readFloat32( reg_id_t &rid, uint32 proc );
  
  /** read this register as a double precision floating point register */
  virtual float64 readFloat64( reg_id_t &rid, uint32 proc );
  
  /** write this register as a integer register */
  virtual void    writeInt64( reg_id_t &rid, ireg_t value, uint32 proc );

  /** write this register as a single precision floating point register */
  virtual void    writeFloat32( reg_id_t &rid, float32 value, uint32 proc );
  
  /** write this register as a double precision floating point register */
  virtual void    writeFloat64( reg_id_t &rid, float64 value, uint32 proc );

  /** copy the previous control register values into this block */
  virtual void    initializeControl( reg_id_t &rid, uint32 proc );

  /** finalize the control register writes */
  virtual void    finalizeControl( reg_id_t &rid, uint32 proc );
  //@}

  /**
   * @name Flow instruction interfaces
   */
  //@{
  /** Get the last flow instruction(s) if any to write this register.
   *  There can be more than one, for instance, a double register could be
   *  written by 2 separate floating point instructions.
   */
  virtual flow_inst_t *getDependence( reg_id_t &rid, uint32 proc);

  /** This register is written by this flow instruction, set the last
   *  dependence to this flow instruction.
   */
  //  virtual void         setDependence( reg_id_t &rid, flow_inst_t *writer );

  /** some setDependence calls in pseq.C require the getLogical() function call that takes in the 
   *            extra logical proc num, so I have to have a setDependence with an extra logical proc num arg
   *            as well.
   */
    virtual void         setDependence( reg_id_t &rid, flow_inst_t *writer, uint32 proc );
  //@}

  /** returns the decode map associated with this abstract register file
   *  This is used for validating registers in the regbox. */
  reg_map_t           *getDecodeMap( void ) {
    return (m_decode_map);
  }

protected:
  /** Initializes this register file mapping */
    virtual void         initialize(void);

    /** the version that takes in the logical proc number */
   virtual void         initialize(uint32 proc);

  /// physical register file associated with this register box
  physical_file_t   *m_rf;

  /// The number of registers this arf must have in reserve before issuing
  ///      instructions to decode
  uint32             m_reserves;

  /// register mapping (at decode time) associated with this register box
  reg_map_t         *m_decode_map;

  /// register mapping (at retire time) associated with this register box
  reg_map_t         *m_retire_map;
  
  /// A mapping that gives a pointer to the last flow instruction (in program order) to write this register
  flow_inst_t      **m_last_writer;
};

/**
* Integer register file class: implements abstract interface for all
* integer registers. See abstract_rf_t for function descriptions.
*
* @see     abstract_rf_t, reg_box_t
* @author  cmauer
* @version $Id: arf.h,v 3.15 2004/08/03 20:00:46 lyen Exp $
*/

class arf_int_t : public abstract_rf_t {
public:
  /// constructor
  arf_int_t( physical_file_t *rf, reg_map_t *decode_map, 
             reg_map_t *retire_map, uint32 reserves ) :
    abstract_rf_t( rf, decode_map, retire_map, reserves ) {
    initialize();
  }
  /// destructor
  ~arf_int_t( void ) { };

  /**
   * @name Register Interfaces
   */
  //@{
  void    readDecodeMap( reg_id_t &rid, uint32 proc );  
  half_t  getLogical( reg_id_t &rid, uint32 proc );
  void    check( reg_id_t &rid, pstate_t *state, check_result_t *result, uint32 proc );
  //@}
protected:
  void    initialize( );
};

/**
* Integer register file class: implements abstract interface for all
* integer registers. See abstract_rf_t for function descriptions.
*
* @see     abstract_rf_t, reg_box_t
* @author  cmauer
* @version $Id: arf.h,v 3.15 2004/08/03 20:00:46 lyen Exp $
*/

class arf_int_global_t : public abstract_rf_t {
public:
  /// constructor
  arf_int_global_t( physical_file_t *rf, reg_map_t *decode_map, 
                    reg_map_t *retire_map ) :
    abstract_rf_t( rf, decode_map, retire_map, 0 ) {
    initialize();
  }

  /// destructor
  ~arf_int_global_t( void ) { };

  /**
   * @name Register Interfaces
   */
  //@{
  void    readDecodeMap( reg_id_t &rid, uint32 proc );  
  half_t  getLogical( reg_id_t &rid, uint32 proc );
  void    check( reg_id_t &rid, pstate_t *state, check_result_t *result, uint32 proc );
  //@}

  /**
   * @name Read and write register interfaces
   */
  //@{
  void    writeInt64( reg_id_t &rid, ireg_t value, uint32 proc );
  //@}
protected:
  void    initialize( );
};

/**
* Floating point register file class: implements abstract interface for 
* single precision floating point registers.
* See abstract_rf_t for function descriptions.
* @see     abstract_rf_t, reg_box_t
* @author  cmauer
* @version $Id: arf.h,v 3.15 2004/08/03 20:00:46 lyen Exp $
*/

class arf_single_t : public abstract_rf_t {
public:
  /// constructor
  arf_single_t( physical_file_t *rf, reg_map_t *decode_map, 
                reg_map_t *retire_map, uint32 reserves ) :
    abstract_rf_t( rf, decode_map, retire_map, reserves ) {
    initialize();
  }
  /// destructor
  ~arf_single_t( void ) { };

  /**
   * @name Register Interfaces
   */
  //@{
  void    readDecodeMap( reg_id_t &rid, uint32 proc );  
  half_t  getLogical( reg_id_t &rid, uint32 proc );
  void    check( reg_id_t &rid, pstate_t *state, check_result_t *result, uint32 proc );
  //@}

  /**
   * @name Read and write register interfaces
   */
  //@{
  float32 readFloat32( reg_id_t &rid, uint32 proc );
  void    writeFloat32( reg_id_t &rid, float32 value, uint32 proc );
  //@}
protected:
  void    initialize( void );
};

/**
* Floating point register file class: implements abstract interface for 
* double precision floating point registers.
* See abstract_rf_t for function descriptions.
* @see     abstract_rf_t, reg_box_t
* @author  cmauer
* @version $Id: arf.h,v 3.15 2004/08/03 20:00:46 lyen Exp $
*/

class arf_double_t : public abstract_rf_t {
public:
  /// constructor
  arf_double_t( physical_file_t *rf, reg_map_t *decode_map, 
                reg_map_t *retire_map ) :
    abstract_rf_t( rf, decode_map, retire_map, 0 ) {
    initialize();
  }
  /// destructor
  ~arf_double_t( void ) { };

  /**
   * @name Register Interfaces
   */
  //@{
  bool    allocateRegister( reg_id_t &rid, uint32 proc );
  bool    freeRegister( reg_id_t &rid, uint32 proc );
  void    setL2Miss( reg_id_t & rid, uint32 proc );
  void    setWriterSeqnum( reg_id_t & rid, uint64 seqnum, uint32 proc );
  uint64  getWriterSeqnum( reg_id_t & rid, uint32 proc );
  uint64  getWrittenCycle( reg_id_t & rid, uint32 proc );
  bool    isL2Miss( reg_id_t & rid, uint32 proc );
  void    incrementL2MissDep( reg_id_t & rid, uint32 proc );
  void    decrementL2MissDep( reg_id_t & rid, uint32 proc );
  int      getL2MissDep( reg_id_t & rid, uint32 proc );
  void    readDecodeMap( reg_id_t &rid, uint32 proc ); 
  void    writeRetireMap( reg_id_t &rid, uint32 proc );
  void    writeDecodeMap( reg_id_t &rid, uint32 proc );

  bool    isReady( reg_id_t &rid, uint32 proc );
  void    waitResult( reg_id_t &rid, dynamic_inst_t *d_instr, uint32 proc );
  half_t  getLogical( reg_id_t &rid, uint32 proc );
  void    check( reg_id_t &rid, pstate_t *state, check_result_t *result, uint32 proc );
  void    addFreelist( reg_id_t &rid );
  void    print( reg_id_t &rid, uint32 proc);
  //@}
  /**
   * @name Read and write register interfaces
   */
  //@{
  my_register_t readRegister( reg_id_t &rid, uint32 proc );
  void          writeRegister( reg_id_t &rid, my_register_t value, uint32 proc );

  ireg_t  readInt64( reg_id_t &rid, uint32 proc );
  void    writeInt64( reg_id_t &rid, ireg_t value, uint32 proc );
  float64 readFloat64( reg_id_t &rid, uint32 proc );
  void    writeFloat64( reg_id_t &rid, float64 value, uint32 proc );
  //@}
protected:
  void    initialize( void );
};

/**
* A container class -- when you have multiple registers to rename
* like for 64-byte blocks (8 fp registers) or really ugly instructions
* which modify more than N registers, where N is the maximum number of
* destinations (SI_MAX_DEST), you need to use this container class.
* It allows you to construct a class dynamically which contains an
* arbitrary number of registers.
*
* Ideally you'd like to be able to have integer windowed registers, as
* well as integer global registers in the container class. However,
* as the container register can't store both the "cwp" and "gset" registers
* at the same time, this is currently infeasible. Thankfully, there
* are options as far as this is concerned (e.g. there are 3 other
* source registers)
*
* See abstract_rf_t for function descriptions.
* @see     abstract_rf_t, reg_box_t
* @author  cmauer
* @version $Id: arf.h,v 3.15 2004/08/03 20:00:46 lyen Exp $
*/

class arf_container_t : public abstract_rf_t {
public:
  /// constructor
  arf_container_t( void ) :
    abstract_rf_t( NULL, NULL, NULL, 0 ) {
    m_cur_type     = CONTAINER_NUM_CONTAINER_TYPES;
    m_cur_element  = -1;
    m_max_elements = -1;

    /* for safty sake, make it big enough */
    m_size = IWINDOW_ROB_SIZE * 4;
    for (int32 i = 0; i < CONTAINER_NUM_CONTAINER_TYPES; i++) {
      m_dispatch_size[i]= 0;
      m_dispatch_map[i] = NULL;
      m_rename_map[i]   = (reg_id_t **) malloc( sizeof(reg_id_t *) * m_size );
      m_rename_index[i] = 0;

      //clear m_rename_map pointers
      for(int j=0; j < m_size; ++j){
        m_rename_map[i][j] = NULL;
      }
    }
    initialize();
  }
  
  /// destructor
  ~arf_container_t( void );
  
  /**
   * @name Register Interfaces
   */
  //@{
  bool    allocateRegister( reg_id_t &rid, uint32 proc );
  bool    freeRegister( reg_id_t &rid, uint32 proc );
  void    setL2Miss( reg_id_t & rid, uint32 proc );
  void    setWriterSeqnum( reg_id_t & rid, uint64 seqnum, uint32 proc );
  uint64  getWriterSeqnum( reg_id_t & rid, uint32 proc );
  uint64  getWrittenCycle( reg_id_t & rid, uint32 proc );
  bool    isL2Miss( reg_id_t & rid, uint32 proc );
  void    incrementL2MissDep( reg_id_t & rid, uint32 proc );
  void    decrementL2MissDep( reg_id_t & rid, uint32 proc );
  int      getL2MissDep( reg_id_t & rid, uint32 proc );
  void    readDecodeMap( reg_id_t &rid, uint32 proc );      
  void    writeRetireMap( reg_id_t &rid, uint32 proc );
  void    writeDecodeMap( reg_id_t &rid, uint32 proc );

  bool    isReady( reg_id_t &rid, uint32 proc );
  void    waitResult( reg_id_t &rid, dynamic_inst_t *d_instr, uint32 proc );
  half_t  getLogical( reg_id_t &rid, uint32 proc );
  void    check( reg_id_t &rid, pstate_t *state, check_result_t *result, uint32 proc );
  void    addFreelist( reg_id_t &rid );
  void    print( reg_id_t &rid, uint32 proc );
  //@}

  /** defines a register type to have a certain number of elements. */
  void    openRegisterType( rid_container_t rtype, int32 numElements );
  /** Adds a register to the container type. The offset is used to modify
   *  the physical register given in the rid structure ("vanilla").
   */
  void    addRegister( rid_type_t regtype, int32 offset, abstract_rf_t *arf );
  /** closes the definition of a container register type */
  void    closeRegisterType( void );
  /**
   * @name Read and write register interfaces
   */
  //@{
  ireg_t  readInt64( reg_id_t &rid, uint32 proc );
  void    writeInt64( reg_id_t &rid, ireg_t value, uint32 proc );
  void    initializeControl( reg_id_t &rid, uint32 proc );
  void    finalizeControl( reg_id_t &rid, uint32 proc );
  //@}

protected:
  void    initialize( void );

  // current type of element being defined
  rid_container_t  m_cur_type;
  // current element being defined by an open register
  int32            m_cur_element;
  // maximum number of elements which can be defined
  int32            m_max_elements;
  // size of rename map (max num. of registers in flight)
  uint32           m_size;

  // size of element in the dispatch array
  uint32       m_dispatch_size[CONTAINER_NUM_CONTAINER_TYPES];
  // array of arf handlers for a particular container
  reg_id_t    *m_dispatch_map[CONTAINER_NUM_CONTAINER_TYPES];
  // per instruction window array for decode, retire mappings of inflight instructions
  reg_id_t   **m_rename_map[CONTAINER_NUM_CONTAINER_TYPES];
  // array of indicies into the dispatch map
  int32        m_rename_index[CONTAINER_NUM_CONTAINER_TYPES];

};

/**
* Condition code register file class: implements abstract interface for 
* condition code registers.
* See abstract_rf_t for function descriptions.
* @see     abstract_rf_t, reg_box_t
* @author  cmauer
* @version $Id: arf.h,v 3.15 2004/08/03 20:00:46 lyen Exp $
*/
class arf_cc_t : public abstract_rf_t {
public:
  /// constructor
  arf_cc_t( physical_file_t *rf, reg_map_t *decode_map, 
            reg_map_t *retire_map, uint32 reserves ) :
    abstract_rf_t( rf, decode_map, retire_map, reserves ) {
    initialize();
  }

  //in order to enable SMT support, need to have an array of arf_cc_t objects, which 
  //           requires an empty constructor (see the cc_arf variable in pseq.C for details)
  arf_cc_t(){ }

  /// destructor
  ~arf_cc_t( void ) { };

  /**
   * @name Register Interfaces
   */
  //@{
  void    readDecodeMap( reg_id_t &rid, uint32 proc );          
  half_t  getLogical( reg_id_t &rid, uint32 proc );

  void    check( reg_id_t &rid, pstate_t *state, check_result_t *result, uint32 proc );

  //@}
protected:
  void    initialize( void );
};

/**
* Control register file class. Each control rid represents an entire
* set of control registers (%pc through %cleanwin). Having a source
* control register file means you can read from all registers in the set.
* Having a "destination" control register file means you can write 
* ANY register in the set.
*
* See abstract_rf_t for function descriptions.
* @see     abstract_rf_t, reg_box_t
* @author  cmauer
* @version $Id: arf.h,v 3.15 2004/08/03 20:00:46 lyen Exp $
*/

class arf_control_t : public abstract_rf_t {
public:
  /// constructor
  arf_control_t( physical_file_t *** rf_array, uint32 count ) :
    abstract_rf_t( NULL, NULL, NULL, 0 ) {
    m_rf_count = count;
    if ( m_rf_count == 0 ) {
      DEBUG_OUT( "Error: ARF Control: NO control sets. Expect a crash.\n" );
    }
    m_rf_array = rf_array;

    //do this for all logical processors:
    m_retire_rf = new uint32[CONFIG_LOGICAL_PER_PHY_PROC];
    m_decode_rf = new uint32[CONFIG_LOGICAL_PER_PHY_PROC];
    for(uint k=0; k < CONFIG_LOGICAL_PER_PHY_PROC; ++k){
       m_rf_array[k][0]->setInt( CONTROL_ISREADY, 1 );
       m_retire_rf[k] = 0;
       m_decode_rf[k] = 0;
    }
    initialize();
  }

  //in order to enable SMT we need to be able to declare an array of arf_control_t objects
  //           , which requires an empty constructor (see m_control_arf variable in pseq.C for details)
  arf_control_t() { }

  /// destructor
  ~arf_control_t( void ) { 
    if(m_retire_rf){
      delete [] m_retire_rf;
    }
    if(m_decode_rf){
      delete [] m_decode_rf;
    }
  };

  /**
   * @name Register Interfaces
   */
  //@{
  bool    allocateRegister( reg_id_t &rid, uint32 proc );
  bool    freeRegister( reg_id_t &rid, uint32 proc );
  void    setL2Miss( reg_id_t & rid, uint32 proc );
  void    setWriterSeqnum( reg_id_t & rid, uint64 seqnum, uint32 proc );
  uint64  getWriterSeqnum( reg_id_t & rid, uint32 proc );
  uint64  getWrittenCycle( reg_id_t & rid, uint32 proc );
  bool    isL2Miss( reg_id_t & rid, uint32 proc );
  void    incrementL2MissDep( reg_id_t & rid, uint32 proc );
  void    decrementL2MissDep( reg_id_t & rid, uint32 proc );
  int      getL2MissDep( reg_id_t & rid, uint32 proc );
  void    readDecodeMap( reg_id_t &rid, uint32 proc );       
  void    writeRetireMap( reg_id_t &rid, uint32 proc );
  void    writeDecodeMap( reg_id_t &rid, uint32 proc );

  bool    isReady( reg_id_t &rid, uint32 proc );
  void    waitResult( reg_id_t &rid, dynamic_inst_t *d_instr, uint32 proc );
  half_t  renameCount( reg_id_t &rid, uint32 proc );
  half_t  getLogical( reg_id_t &rid, uint32 proc );

  void    check( reg_id_t &rid, pstate_t *state, check_result_t *result, uint32 proc );

  bool    regsAvailable( uint32 proc );
  void    addFreelist( reg_id_t &rid );
  //@}
  /**
   * @name Read and write register interfaces
   */
  //@{
  ireg_t  readInt64( reg_id_t &rid, uint32 proc );
  void    writeInt64( reg_id_t &rid, ireg_t value, uint32 proc );
  void    initializeControl( reg_id_t &rid, uint32 proc);
  void    finalizeControl( reg_id_t &rid, uint32 proc );
  //@}

  /// returns the retirement register file
  physical_file_t *getRetireRF( uint32 proc ) {
    return (m_rf_array[proc][m_retire_rf[proc]]);
  }
  /// returns the decode register file
  physical_file_t *getDecodeRF( uint32 proc ) {
    return (m_rf_array[proc][m_decode_rf[proc]]);
  }

protected:
  void    initialize( void );

  /// the following should be dupllicated for EACH logical processor:
  /// number of control registers files which exist in the system
  uint32             m_rf_count;
  /// Oldest set of registers which is still alive
  uint32      *       m_retire_rf;
  /// Most recent set of registers allocated
  uint32      *       m_decode_rf;

  /// each logical proc should have its own array:
  /// control physical register files 
  physical_file_t  *** m_rf_array;
};

/**
* Empty register file class. Does not implement any actions.
*
* See abstract_rf_t for function descriptions.
* @see     abstract_rf_t, reg_box_t
* @author  cmauer
* @version $Id: arf.h,v 3.15 2004/08/03 20:00:46 lyen Exp $
*/
class arf_none_t : public abstract_rf_t {
public:
  /// constructor
  arf_none_t( physical_file_t *rf, reg_map_t *decode_map, 
             reg_map_t *retire_map ) :
    abstract_rf_t( rf, decode_map, retire_map, 0 ) {
  }
  /// destructor
  ~arf_none_t( void ) { };

  /**
   * @name Register Interfaces
   */
  //@{
  bool    allocateRegister( reg_id_t &rid, uint32 proc ) { return (true); }
  bool    freeRegister( reg_id_t &rid, uint32 proc ) { return (true); }
  void    setL2Miss( reg_id_t & rid, uint32 proc ) {}
  void    setWriterSeqnum( reg_id_t & rid, uint64 seqnum, uint32 proc ) {}
  uint64  getWriterSeqnum( reg_id_t & rid, uint32 proc ){ return 0; }
  uint64  getWrittenCycle( reg_id_t & rid, uint32 proc ) { return 0; }
  bool    isL2Miss( reg_id_t & rid, uint32 proc ) { return false;}
  void    incrementL2MissDep( reg_id_t & rid, uint32 proc ) {}
  void    decrementL2MissDep( reg_id_t & rid, uint32 proc ) {}
  int      getL2MissDep( reg_id_t & rid, uint32 proc ) { return 0; }
  void    writeRetireMap( reg_id_t &rid, uint32 proc ) {}
  void    writeDecodeMap( reg_id_t &rid, uint32 proc ) {}
  void    readDecodeMap( reg_id_t &rid, uint32 proc ) {}   
  bool    isReady( reg_id_t &rid, uint32 proc ) { return (true); }
  void    waitResult( reg_id_t &rid, dynamic_inst_t *d_instr, uint32 proc ) {}
  half_t  renameCount( reg_id_t &rid, uint32 proc ) { return (0); }
  half_t  getLogical( reg_id_t &rid, uint32 proc ) { return (0); }
  void    check( reg_id_t &rid, pstate_t *state, check_result_t *result, uint32 proc ) {}

  /**
   * @name Read and write register interfaces
   */
  //@{
  ireg_t  readInt64( reg_id_t &rid, uint32 proc ) { return (0); }
  void    writeInt64( reg_id_t &rid, ireg_t value, uint32 proc ) {}
  //@}
protected:
  void    initialize( void ) { }
};

/*------------------------------------------------------------------------*/
/* Global variables                                                       */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Global functions                                                       */
/*------------------------------------------------------------------------*/

#endif  /* _ARF_H_ */
