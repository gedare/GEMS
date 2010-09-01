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
#ifndef _DTLB_H_
#define _DTLB_H_
/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

const word_t DTLB_BITS = 8; // 64 entry TLB
const word_t DTLB_SIZE = 1 << DTLB_BITS;

/*------------------------------------------------------------------------*/
/* Class declaration(s)                                                   */
/*------------------------------------------------------------------------*/

/// Entry in the tlb
class dtlb_entry_t {
public:
  dtlb_entry_t( la_t va, pa_t pa, context_id_t context, uint32 pagesize ) {
    this->v_address = va;
    this->p_address = pa;
    this->m_context = context;
    this->m_pagesize = pagesize;
    this->m_valid   = true;
    this->m_next    = false;
  }

  la_t               v_address;
  pa_t               p_address;
  context_id_t       m_context;
  uint32             m_pagesize;
  bool               m_valid;
  dtlb_entry_t      *m_next;
};

/** Table for mapping virtual addresses to physical addresses */
typedef map<la_t, dtlb_entry_t *>  AddressTranslationMap;

/**
* Translation lookaside buffer implmementation-- This is called a "D"-TLB
* for historical reasons only.
*
* @see     pseq_t
* @author  cmauer
* @version $Id: dtlb.h,v 3.4 2003/07/10 18:07:47 milo Exp $
*/
class dtlb_t {
public:

  /**
   * @name Constructor(s) / destructor
   */
  //@{

  /**
   * Constructor: creates object
   *  @param dtlb_size      The size of the TLB object
   *  @param num_page_sizes The number of different page sizes supported.
   *  @param page_sizes     An enumeration of page sizes
   */
  dtlb_t( uint32 dtlb_size, uint32 num_page_sizes, uint32 *page_sizes );

  /**
   * Destructor: frees object.
   */
  ~dtlb_t( void );
  //@}

  /**
   * @name Methods
   */
  //@{

  /** Translate a virtual address --
   *  @param va          The virtual address to translate
   *  @param context     The context to translate it in
   *  @param accessSize  The size of the access
   *  @param isWrite     True if this requires exclusive permission
   *  @param pa          The translated physical address
   *  @return bool       True if a translation is found, false otherwise
   */
  bool    translateAddress( la_t va, context_id_t context, 
                            uint32 accessSize, bool isWrite,
                            pa_t *pa);

  /** Add an entry to the TLB --
   *  @param va       The virtual address to translate
   *  @param context  The context to translate it in
   *  @param pa       The physical translated memory address
   *  @param pagesize The size of the page
   */
  void    addTranslation( la_t va, context_id_t context, 
                          pa_t pa, uint32 page_size );

  /** Read translations from a file.
   *  @param fp  The file containing the translation information.
   */
  void    readTranslation( FILE *fp );

  /** Write translations to a file.
   *  @param fp  The file containing the translation information.
   */
  void    writeTranslation( FILE *fp );
  //@}

private:
  /** Insert an entry in to the TLB.
   */
  void    insertEntry( dtlb_entry_t *entry );

  /** An array of hash table for address translation.
   *  Each table corresponds to a different page size--
   */
  AddressTranslationMap *m_table;

  /** An array of page masks for address translation-- correlated with m_table
   */
  uint64         *m_page_mask;

  /** An array of page sizes for address translation-- correlated with m_table
   */
  uint32         *m_page_size;

  /** The number of page sizes supported by this TLB.
   */
  uint32          m_num_pages;

  /// count of the address translations in the TLB
  uint32          m_count;

  /// size of the translation lookaside buffer
  uint32          m_size;
};

/*------------------------------------------------------------------------*/
/* Global variables                                                       */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Global functions                                                       */
/*------------------------------------------------------------------------*/

#endif  /* _DTLB_H_ */
