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
#ifndef _IPAGEMAP_H_
#define _IPAGEMAP_H_
/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include "ipage.h"

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

/// file token indicating the start of a new table entry
const la_t IPAGEMAP_INDEX_TOKEN = ~0x0;
/// file token indicating the end of a number of pages (in one bucket)
const la_t IPAGEMAP_DONE_TOKEN  = ~0x1;

/*------------------------------------------------------------------------*/
/* Class declaration(s)                                                   */
/*------------------------------------------------------------------------*/

/**
 * A hash table entry for an open addressing hash table used to store
 * instruction pages.
 *
 * @see     ipagemap_t
 * @author  cmauer
 * @version $Id: ipagemap.h 1.17 04/08/09 11:27:57-05:00 lyen@opus.cs.wisc.edu $
 */
class imapentry_t {

public:
  /**
   * @name Constructor(s) / destructor
   */
  //@{

  /**
   * Constructor: creates object
   */
  imapentry_t( ipage_t *entry, uint32 tag ) {
    m_entry = entry;
    m_tag   = tag;
    m_next  = NULL;
  }

  /**
   * Destructor: frees object.
   */
  ~imapentry_t() { };
  //@}

  /// A pointer to this ipage
  ipage_t       *m_entry;

  /** The virtual address tag of this instruction page
   *  The tag is defined to be (address >> IPAGE_PAGE_BITS)
   */
  pa_t          m_tag;

  /// The next entry in the hash table
  imapentry_t   *m_next;

};


/**
 * A hash table for instruction pages.
 *
 * @see     ipage_t, pseq_t
 * @author  cmauer
 * @version $Id: ipagemap.h 1.17 04/08/09 11:27:57-05:00 lyen@opus.cs.wisc.edu $
 */
class ipagemap_t {

public:

  /**
   * @name Constructor(s) / destructor
   */
  //@{

  /**
   * Constructor: creates object
   * @param tablesize The number of entrys in the (open-addressed) hash table.
   */
  ipagemap_t( uint32 tablesize );

  //need an empty constructor to support arrays of ipagemap_t objects
  ipagemap_t(){ }

  /**
   * Destructor: frees object.
   */
  ~ipagemap_t();
  //@}

  /**
   * @name Methods
   */
  //@{

  /**
   * Hash function for instruction pages
   * @param  address  Virtual program counter of the instruction
   * @return int32    The "bucket" to search for this instruction's ipage
   */
  inline uint32  hash_function( pa_t address ) {

    // strip off the low order (page offset) bits in the address
    pa_t hash_index = address >> IPAGE_PAGE_BITS;

    // xor on high order bits
    hash_index = (hash_index ^ (hash_index >> m_tablebits)) & m_hashmask;
    return ( (uint32) hash_index );
  }

  /**
   * Query for an instruction in the page map.
   * return the location where it should be inserted if its not
   * already present.
   * @param  address     Virtual program counter of the instruction
   * @param  hash_index  The "bucket" to search for this instruction's ipage
   * @return imapentry_t NULL if not found, otherwise pointer to the entry
   */
  imapentry_t *queryLocation( pa_t address, uint32 &hash_index );

  /**
   * Reads the instruction map from a file.
   */
  bool   readFile( char *infilename );
  /**
   * Save the instruction map to a file.
   * This provides traces with a instruction map to run with.
   */
  bool   saveFile( char *outfilename );
  //@}

  /**
   * @name Accessor(s) / mutator(s)
   */
  //@{

  /**
   * Insert a new instruction into the page map.
   * @param  address   Virtual program counter of the instruction
   * @param  instr     The 32-bit instruction to be inserted in the table
   * @param  s_instr   A pointer to the new static instruction at this address
   * @return uint16 status result: IPAGE_SEEN, IPAGE_NEW, IPAGE_CONFLICT
   */
  uint16  insertInstr( pa_t phys_addr,
                       uint32 instr, static_inst_t * &s_instr );

  /**
   * Query if an instruction exists in the page map.
   * Returns true if the instruction is found, false if not.
   
   * @param  address Physical program counter of the instruction
   * @param  s_instr A pointer to the static instruction at this address
   * @return uint16  IPAGEMAP_FOUND, _NOTFOUND or _NOPAGE
   */
  bool    queryInstr( pa_t address, static_inst_t * &s_instr );

  /**
   * Invalides a instruction in the page map.
   * @param  address   Physical program counter of the instruction
   */
  void    invalidateInstr( pa_t address );

  /**
   * Traverses all of the instructions in the ipagemap, calling the
   * callback function "static_inst_callback()" on each. This function
   * @param user_data  A void pointer for your convenience
   * @param _callback  The function to call on each node
   * returns true to stop traversing, false to continue.
   * @return true if traversing is interrupted, false if not. 
   */
  bool    walkMap( void *user_data, ipage_callback_t si_callback );
  
  /**
   * Prints statistics about this instruction map.
   */
  void    printStats( void );

  /**
   * Print out memory usage stats for this structure.
   */
  void    printMemoryStats( out_intf_t *log );

  /**
   * Prints a dump of all the instructions in the ipagemap
   *  @param  verbose False if instructions should only be counted not printed
   *  @return uint32  The number of instructions printed
   */
  uint32  print( bool verbose );
  //@}

private:

  /** An open addressing hash table pointing (through the 'entry' in the table)
   *  To instruction pages objects (ipage.h)
   */
  imapentry_t  **m_table;

  /** 
   *  masks off to leave the most significant bits in the table after the
   *  ipage bits (which are the lower bits up to IPAGE_PAGE_BITS).
   *  
   *  <p>|---------------------|--------------|---------------|
   *  <p>  Upper Bits           Hash Mask bits IPAGE_PAGE_BITS
   */
  uint32   m_hashmask;

  /// size of the table
  uint32   m_tablesize;

  /// the number of bits in the table:: log_base_two(m_tablesize)
  uint32   m_tablebits;

  /// Total number of unique instructions present in the table
  uint32   m_num_instr;

  /// Total number of instruction pages in the ipagemap
  uint32   m_num_totalpages;

  /// Number of collisions in the hash table
  uint32   m_num_collisions;

};

/*------------------------------------------------------------------------*/
/* Global variables                                                       */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Global functions                                                       */
/*------------------------------------------------------------------------*/

#endif  /* _IPAGEMAP_H_ */
