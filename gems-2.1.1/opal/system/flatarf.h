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
#ifndef _FLATARF_H_
#define _FLATARF_H_

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
* Flat (non-renamed) register file class: Implements a non-renamed 
* register file interface.
*
* @see     abstract_rf_t, reg_box_t
* @author  cmauer
* @version $Id: flatarf.h 1.7 04/08/06 13:27:53-05:00 lyen@opus.cs.wisc.edu $
*/

class flat_rf_t : public abstract_rf_t {
public:
  /// constructor
  flat_rf_t( physical_file_t *rf ) :
    abstract_rf_t( rf, NULL, NULL, 0 ) {
    
    // we maintain a separate 'ideal' (in-order) register state
    // keep track of the last flow instruction to write each register
    m_last_writer = NULL;
    if (m_rf != NULL) {
      m_last_writer = (flow_inst_t **) malloc( sizeof(flow_inst_t *) *
                                               m_rf->getNumRegisters() );
      for( uint16 i = 0; i < m_rf->getNumRegisters(); i++ ) {
        m_last_writer[i] = NULL;
      }
    }
  }
  
  /// destructor
  ~flat_rf_t( void ) { };

  /**
   * @name Register Interfaces
   */
  //@{
  bool    allocateRegister( reg_id_t &rid, uint32 proc );
  bool    freeRegister( reg_id_t &rid, uint32 proc );
  void    writeRetireMap( reg_id_t &rid, uint32 proc );
  void    writeDecodeMap( reg_id_t &rid, uint32 proc );
  void    readDecodeMap( reg_id_t &rid, uint32 proc );
  bool    isReady( reg_id_t &rid, uint32 proc );
  void    waitResult( reg_id_t &rid, dynamic_inst_t *d_instr, uint32 proc );
  bool    regsAvailable( uint32 proc );
  void    addFreelist( reg_id_t &rid );
  half_t  renameCount( reg_id_t &rid, uint32 proc );
  //@}

  /**
   * @name read and write register interfaces
   */
  //@{
  /** read this register as a integer register */
  ireg_t  readInt64( reg_id_t &rid, uint32 proc );
  void    writeInt64( reg_id_t &rid, ireg_t value, uint32 proc );

  /** Set and get the last writer for a particular register. */
  void         setDependence( reg_id_t &rid, flow_inst_t *inst, uint32 proc );

  flow_inst_t *getDependence( reg_id_t &rid, uint32 proc );
  //@}

protected:
  /// A mapping that gives a pointer to the last flow instruction (in program order) to write this register
  flow_inst_t      **m_last_writer;
};

/**
* Flat integer register file class: Implements a non-renamed 
* flat register file interface.
*
* @see     flat_rf_t, abstract_rf_t
* @author  cmauer
* @version $Id: flatarf.h 1.7 04/08/06 13:27:53-05:00 lyen@opus.cs.wisc.edu $
*/

class flat_int_t : public flat_rf_t {
public:
  flat_int_t( physical_file_t *rf, pstate_t *state, uint32 proc ) :
    flat_rf_t( rf ) {
    initializeState( state, proc );
  }
  /**
   * @name Register Interfaces
   */
  //@{
  half_t  getLogical( reg_id_t &rid, uint32 proc );
  void    check( reg_id_t &rid, pstate_t *state, check_result_t *result, uint32 proc );
  //@}

protected:
  void    initializeState( pstate_t *state, uint32 proc );
};

/**
* Global integer register file class: Implements a non-renamed 
* flat register file interface.
*
* @see     flat_rf_t, abstract_rf_t
* @author  cmauer
* @version $Id: flatarf.h 1.7 04/08/06 13:27:53-05:00 lyen@opus.cs.wisc.edu $
*/

class flat_int_global_t : public flat_rf_t {
public:
  flat_int_global_t( physical_file_t *rf, pstate_t *state, unsigned int proc ) :
    flat_rf_t( rf ) {
    initializeState( state, proc ); 
  }
  /**
   * @name Register Interfaces
   */
  //@{
  half_t  getLogical( reg_id_t &rid, uint32 proc );
  void    check( reg_id_t &rid, pstate_t *state, check_result_t *result, uint32 proc );
  void    writeInt64( reg_id_t &rid, ireg_t value, uint32 proc );
  //@}
  
protected:
  void    initializeState( pstate_t *state, uint32 proc );
};

/**
* Global single precision register file class.
*
* @see     flat_rf_t, abstract_rf_t
* @author  cmauer
* @version $Id: flatarf.h 1.7 04/08/06 13:27:53-05:00 lyen@opus.cs.wisc.edu $
*/

class flat_single_t : public flat_rf_t {
public:
  flat_single_t( physical_file_t *rf, pstate_t *state, uint32 proc ) :
    flat_rf_t( rf ) {
    initializeState( state, proc );             
  }
  /**
   * @name Register Interfaces
   */
  //@{
  half_t  getLogical( reg_id_t &rid, uint32 proc );
  void    check( reg_id_t &rid, pstate_t *state, check_result_t *result, uint32 proc );

  float32 readFloat32( reg_id_t &rid, uint32 proc );
  void    writeFloat32( reg_id_t &rid, float32 value, uint32 proc );

  /// This interface is necessary for double & single regs to share last writer
  flow_inst_t **getLastWriter( void ) {
    return m_last_writer;
  }
  //@}
  
protected:
  void    initializeState( pstate_t *state, uint32 proc );      
};

/**
* Global double precision register file class.
*
* @see     flat_rf_t, abstract_rf_t
* @author  cmauer
* @version $Id: flatarf.h 1.7 04/08/06 13:27:53-05:00 lyen@opus.cs.wisc.edu $
*/

class flat_double_t : public flat_rf_t {
public:
  flat_double_t( physical_file_t *rf, pstate_t *state,
                 flow_inst_t **last_writer ) :
    flat_rf_t( rf ) {

    initializeState( state );
    // free the allocated last writier array
    free( m_last_writer );
    // and use the same last_writer as the single precison fp file
    m_last_writer = last_writer;
  }

  ~flat_double_t( void ) {
    // don't free last_writer (single allocated it)
    m_last_writer = NULL;
  };

  /**
   * @name Register Interfaces
   */
  //@{
  half_t  getLogical( reg_id_t &rid, uint32 proc );
  void    check( reg_id_t &rid, pstate_t *state, check_result_t *result, uint32 proc );

  ireg_t  readInt64( reg_id_t &rid, uint32 proc );
  void    writeInt64( reg_id_t &rid, ireg_t value, uint32 proc );
  float64 readFloat64( reg_id_t &rid, uint32 proc );
  void    writeFloat64( reg_id_t &rid, float64 value, uint32 proc );

  void         setDependence( reg_id_t &rid, flow_inst_t *inst, uint32 proc );
  flow_inst_t *getDependence( reg_id_t &rid, uint32 proc );
  //@}
  
protected:
  void    initializeState( pstate_t *state );
};

/**
* Control register file class.
*
* @see     flat_rf_t, abstract_rf_t
* @author  cmauer
* @version $Id: flatarf.h 1.7 04/08/06 13:27:53-05:00 lyen@opus.cs.wisc.edu $
*/

class flat_control_t : public flat_rf_t {
public:
  flat_control_t( physical_file_t *rf, pstate_t *state, uint32 proc ) :
    flat_rf_t( rf ) {
    initializeState( state, proc );
  }
  /**
   * @name Register Interfaces
   */
  //@{
  half_t  getLogical( reg_id_t &rid, uint32 proc );
  void    check( reg_id_t &rid, pstate_t *state, check_result_t *result, uint32 proc );
  void    initializeControl( reg_id_t &rid, uint32 proc );
  void    finalizeControl( reg_id_t &rid, uint32 proc );
  //@}

  ireg_t  readInt64( reg_id_t &rid, uint32 proc );
  void    writeInt64( reg_id_t &rid, ireg_t value, uint32 proc );
  
protected:
  void    initializeState( pstate_t *state, uint32 proc );
};

/**
* Container register file class.
*
* @see     flat_rf_t, abstract_rf_t
* @author  cmauer
* @version $Id: flatarf.h 1.7 04/08/06 13:27:53-05:00 lyen@opus.cs.wisc.edu $
*/

class flat_container_t : public flat_rf_t {
public:
  flat_container_t( void ) :
    flat_rf_t( NULL ) {
    m_cur_type     = CONTAINER_NUM_CONTAINER_TYPES;
    m_cur_element  = -1;
    m_max_elements = -1;
    m_max_dispatch_size = -1;

    for (int32 i = 0; i < CONTAINER_NUM_CONTAINER_TYPES; i++) {
      m_dispatch_size[i] = 0;
      m_dispatch_map[i]  = NULL;
    }
  }
  /**
   * @name Register Interfaces
   */
  //@{
  half_t  getLogical( reg_id_t &rid, uint32 proc );
  void    check( reg_id_t &rid, pstate_t *state, check_result_t *result, uint32 proc );
  void    print( reg_id_t &rid, uint32 proc );

  /** defines a register type to have a certain number of elements. */
  void    openRegisterType( rid_container_t rtype, int32 numElements );
  /** Adds a register to the container type. The offset is used to modify
   *  the physical register given in the rid structure ("vanilla").
   */
  void    addRegister( rid_type_t regtype, int32 offset, abstract_rf_t *arf );
  /** closes the definition of a container register type */
  void    closeRegisterType( void );

  ireg_t  readInt64( reg_id_t &rid, uint32 proc );
  void    writeInt64( reg_id_t &rid, ireg_t value, uint32 proc );
  void    initializeControl( reg_id_t &rid, uint32 proc );
  void    finalizeControl( reg_id_t &rid, uint32 proc );

  /** Set and get the last writer for a particular register. */
  void         setDependence( reg_id_t &rid, flow_inst_t *inst, uint32 proc );
  flow_inst_t *getDependence( reg_id_t &rid, uint32 proc );
  //@}

protected:
  /** This function copies the dispatch registers */
  void    copyDispatchRids( reg_id_t &container_rid, reg_id_t *to_rid );
  
  /// current type of element being defined
  rid_container_t  m_cur_type;
  /// current element being defined by an open register
  int32            m_cur_element;
  /// maximum number of elements which can be defined
  int32            m_max_elements;
  /// largest # of elements defined by the dispatch array
  /// (defined to do a stack based array allocation)
  int32            m_max_dispatch_size;

  /// size of element in the dispatch array
  uint32       m_dispatch_size[CONTAINER_NUM_CONTAINER_TYPES];
  /// array of arf handlers for a particular container
  reg_id_t    *m_dispatch_map[CONTAINER_NUM_CONTAINER_TYPES];
};


/*------------------------------------------------------------------------*/
/* Global variables                                                       */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Global functions                                                       */
/*------------------------------------------------------------------------*/

#endif  /* _FLATARF_H_ */
