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
#ifndef _EXEC_H_
#define _EXEC_H_

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
 * Represents the integer datapath-- allows functional execution of
 * given instructions.
 *
 * @see     dynamic_inst_t
 * @author  cmauer
 * @version $Id: exec.h 1.2 03/07/10 18:07:47-00:00 milo@cottons.cs.wisc.edu $
 */
class dp_int_t {
public:
  /**
   * @name Methods
   */
  //@{
  /** The execute function class: actually implements the functionality
   *  for each instruction. This makes this header file much smaller.
   */
  friend class exec_fn_t;

  /// input source registers
  my_register_t   m_source[2];
  /// result register
  my_register_t   m_dest;

  /// exeception code (if any)
  trap_type_t     m_exception;
  /// source condition code register
  register_cc_t   m_source_cc;
  /// destination condition code register
  register_cc_t   m_dest_cc;
  //@}
};

/**
 * Represents the control datapath-- allows for execution of branch
 * instructions.
 *
 * @see     dp_int_t
 * @author  cmauer
 * @version $Id: exec.h 1.2 03/07/10 18:07:47-00:00 milo@cottons.cs.wisc.edu $
 */
class dp_control_t {
public:
  friend class exec_fn_t;

  void  actualPC_execute( void );
  void  actualPC_branch( void );
  void  actualPC_uncond_branch( void );
  void  actualPC_indirect( void );
  void  actualPC_trap( void );
  void  actualPC_trap_return( my_addr_t tpc, my_addr_t tnpc,
                              ireg_t pstate, ireg_t tl,
                              ireg_t cwp );
  void  actualPC_cwp( void );

  /// The PC (includes NPC and other fetch fields for SPARC)
  /// This PC is both the input and output of the control function:
  ///    on input it contains the state "pre-execution".
  abstract_pc_t  *m_at;
  /// The offset (for branches)
  uint64          m_offset;
  /// true if the annul bit is set
  unsigned char   m_annul;
  /// true if the destination PCs should be used
  unsigned char   m_taken;
};

/**
 * Represents the memory datapath-- allows for memory instruction execution.
 *
 *
 * @see     dp_int_t, dp_control_t
 * @author  cmauer
 * @version $Id: exec.h 1.2 03/07/10 18:07:47-00:00 milo@cottons.cs.wisc.edu $
 */
class dp_memory_t {
public:
  friend class exec_fn_t;

  /// The values from memory
  uint64          m_data;
  /// true if the data is valid
  unsigned char   m_valid;
};

/*------------------------------------------------------------------------*/
/* Global variables                                                       */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Global functions                                                       */
/*------------------------------------------------------------------------*/

/// initialization function of the execute_fn class
void exec_fn_initialize( void );

/// execute function of the execute_fn class
void exec_fn_execute( i_opcode opcode, dp_int_t *data );

#endif  /* _EXEC_H_ */
