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
#ifndef _REG_MAP_H_
#define _REG_MAP_H_
/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include "bitfield.h"
#include "regfile.h"

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Class declarations                                                     */
/*------------------------------------------------------------------------*/

/**
* The logical register file in the microprocessor.
*
* @author  zilles
* @version $Id$
*/
class reg_map_t {

public:

  /**
   * @name Constructor(s) / Destructor
   */
  //@{
  /** Constructor: creates an empty logical register file */
  reg_map_t(physical_file_t *p, int num_logical);

  // in order to enable SMT support, need to have an empty constructor to create
  // arrays of reg_map_t objects...see pseq.C (the cc_decode_map variable) for details
  //  Note this constructor doesn't do anything
  reg_map_t(){ }   
  
  /** Destructor: frees the object */
  virtual ~reg_map_t();
  //@}

  /// Convert a logical register to a physical register
  uint16 getMapping(uint16 logical_reg, uint32 proc) const;

  /// explicitly set a logical->physical mapping
  void   setMapping(uint16 logical_reg, uint16 phys_reg, uint32 proc);

  /**
   *  @name Allocation of Registers
   */
  //@{
  /// allocate a new physical register (returns index)
  uint16 regAllocate(uint16 logical_reg, uint32 proc);
  
  /// frees a physical register (param index The index to be freed)
  void   regFree(uint16 logical_reg, uint16 physical_reg, uint32 proc);

  /// returns the number of logical registers in flight
  uint16 countInFlight(uint16 logical_reg, uint32 proc);

  /// returns the number of physical registers remaining for allocation
  uint16 countRegRemaining(void);

  /// returns a count of the number of logical registers in this mapping
  uint32 getNumLogical( void ) {
    return m_num_logical;
  }
  //@}

  /// verify that all free'd registers are actually free
  bool  *buildFreelist( ) const;
  
  /// add a register to the freelist
  bool   addFreelist( uint16 physical_reg );

  /// verify that no registers are "leaked"
  void   leakCheck( uint32 id, const char *map_name ) const;

  /** compare if two register mappings are equal
   *  @return bool true if match, false if not.
   */
  bool   equals(reg_map_t *other, uint32 proc);

  /** print out the register mapping */
  void   print( ) const;


protected:
  /** pointer to the physical register file */
  physical_file_t *m_physical;

  /** number of logical registers in the mapping */
  uint32          m_num_logical;
  /** number of phyiscal registers in the register file */
  uint32          m_num_physical;

  /** index from logical register to physical register
   *  length: # of logical registers */
  /// in order for all logical procs to share the same physical reg file, we ONLY need separate
  ///           rename maps (ie all logical procs share the same m_freelist variable)
  uint16         ** m_logical_reg;

  /** number of times a given logical register has been renamed.
   *  length: # of logical registers */
  /// for the same reason as above, each logical proc should have its own rename counter array
  uint16         ** m_logical_rename_count;

#ifdef CHECK_REGFILE
  /** a bit vector of allocated physical registers:
   *  bits is true if the register is free!
   *  length: # of physical registers */
  bitfield_t     *m_bits;
#endif

  /** an array of indicies of currently unallocated physical registers
   *  length: # of physical registers */
  uint16         *m_freelist;

  /** index in the freelist array to the next free register */
  int16          m_freelist_index;

  /** A bitmap of free registers, used to validate there are no register leaks. */
  bool           *m_validate_map;

};

#endif /* _REG_MAP_H_ */
