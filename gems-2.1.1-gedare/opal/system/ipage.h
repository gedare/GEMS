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
#ifndef _IPAGE_H_
#define _IPAGE_H_
/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include "statici.h"

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

// CM: reduced size of ipages to improve memory performance (3-4-2002)
//const uint32 IPAGE_MAX_INSTR =   1024;
//const int    IPAGE_PAGE_BITS =     10;

/** defines the number of static instruction per page */
const uint32 IPAGE_MAX_INSTR =    128;
/** defines the log-base2(IPAGE_MAX_INSTR) */ 
const int    IPAGE_PAGE_BITS =      7;

/** used to make a tag from an address: 
 *  defines a mask to remove low order bits from instruction,
 */
const int    IPAGE_PAGE_MASK = IPAGE_MAX_INSTR-1;

/// instruction has been seen before
const uint16 IPAGE_SEEN      =     0x0;
/// instruction has not been seen before
const uint16 IPAGE_NEW       =     0x1;
/** virtual program counter has been seen, but the instruction has not!
 *  This implies a conflict in the vpc
 */
const uint16 IPAGE_CONFLICT  =     0x2;

/*------------------------------------------------------------------------*/
/* Class declaration(s)                                                   */
/*------------------------------------------------------------------------*/

/// The callback function type for walking ipages
typedef bool (*ipage_callback_t)( void *user_data, static_inst_t *s );

/**
* A decodeded instruction page in memory.
*
* @see     static_inst_t, bitfield_t, pseq_t
* @author  cmauer
* @version $Id: ipage.h 1.21 03/07/10 18:07:48-00:00 milo@cottons.cs.wisc.edu $
*/
class ipage_t {
  
public:

  /**
   * @name Constructor(s) / destructor
   */
  //@{
  /**
   * Constructor: creates object
   */
  ipage_t();

  /**
   * Destructor: frees object.
   */
  ~ipage_t( void );
  //@}

  /** inserts an instruction into the ipage.
   *  @param  address The virtual address of the instruction to insert.
   *  @param  instr   The instruction to insert in the page
   *  @param  s_instr A pointer to the new (or old) static instruction
   *  @return uint16  status result: IPAGE_SEEN, IPAGE_NEW, IPAGE_CONFLICT
   */
  uint16           insertInstruction( pa_t address, uint32 instr,
                                      static_inst_t * &s_instr );

  /** given an offset (word aligned) into a page, returns the static
   *  instruction at that location
   */
  static_inst_t   *getInstruction( pa_t address );

  /** invalidates an instruction into the ipage.
   *  @param  address The virtual address of the instruction to invalidate.
   *  @return uint16  status result: IPAGE_SEEN, IPAGE_NEW, IPAGE_CONFLICT
   */
  void     invalidateInstruction( pa_t address );

  /** sets the physical address of the instruction page. */
  void     setPageAddress( pa_t address );

  /** Prints a dump of all the instructions in this page
   *  @return uint32 The number of instructions printed
   *  @param  verbose False if instructions should only be counted not printed
   */
  uint32   print( bool verbose );

  /** Walk this instruction page, calling static_inst_callback() on
   *  every valid instruction.
   * @param user_data  A void pointer for your convenience
   * @param _callback  The function to call on each node
   *  returns true to stop traversing, false to continue.
   *  @return true if traversing is interrupted, false if not. 
   */
  bool     walkMap( void *user_data, ipage_callback_t si_callback );
  
  /** Save the decoded instructions to a file.
   *  @param  fp   The file pointer to read from
   *  @return bool true if successful, false if not
   */
  bool     writePage( FILE *fp );

  /** Load the decoded instructions from a file.
   *  @param fp  The file pointer to read from
   *  @param tag The virtual program counter (minus the least sig bits)
   *             which the fp is currently pointing to.
   *  @return bool true if successful, false if not
   */
  bool     readPage( FILE *fp, pa_t tag, uint32 &totalread );
  
private:
  /** page header for this instruction block */
  si_page_header_t  m_header;

  /** array of statically decoded instructions.
   *  instruction pointer can be NULL, indicating that the
   *  instruction still needs to be decoded.
   */
  static_inst_t    *m_instr[ IPAGE_MAX_INSTR ];
};

/*------------------------------------------------------------------------*/
/* Global variables                                                       */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Global functions                                                       */
/*------------------------------------------------------------------------*/

#endif  /* _IPAGE_H_ */
