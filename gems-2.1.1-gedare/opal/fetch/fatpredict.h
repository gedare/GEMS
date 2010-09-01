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
#ifndef _FATPREDICT_H_
#define _FATPREDICT_H_

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include "bitfield.h"
#include "directbp.h"

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Class declaration(s)                                                   */
/*------------------------------------------------------------------------*/

/** entries in the fat prediction table */
class fatentry_t {

public:
  /// constructor
  fatentry_t( uint32 history_size ) {
    alloc( history_size );
  }
  // array constructor
  fatentry_t( void ) { }
  // allocates memory
  void alloc( uint32 history_size ) {
    m_bitfield.alloc( 1 << (history_size + 1) );
    clearPredict(history_size);
  }

  /// destructor
  ~fatentry_t( void ) {
  }
  
  char getPredict( uint32 history ) {
    return ( m_bitfield.getBit( 2*history ) << 1 |
             m_bitfield.getBit( 2*history + 1 ) );
  }
  
  void setPredict( uint32 history, char tableentry ) {
    // update the 2 bit counter
    m_bitfield.setBit( 2*history, ((tableentry >> 1) & 0x1) );
    m_bitfield.setBit( 2*history + 1, (tableentry & 0x1) );
  }

  void clearPredict( uint32 history_bits ) {
    // update each bitfield to weakly taken
    for (int32 i = 0; i < (1 << history_bits); i++) {
      // binary "1 0" == 2 which is weakly taken
      m_bitfield.setBit( i * 2, 1 );
    }
  }

  /// the tags (based on logical address)
  my_addr_t   m_tag;

  /// the number of times this entries has been accessed, since last time
  uint64      m_accesses;

private:
  /// Two bit counters based on history bits
  bitfield_t  m_bitfield;
};

/**
 * Tagged, Fully-associative predictor, using 2-bit counters
 * @author  cmauer
 * @version $Id: fatpredict.h 1.2 03/07/10 18:07:46-00:00 milo@cottons.cs.wisc.edu $
 */
class fatpredict_t {
public:
  /**
   * @name Constructor(s) / destructor
   */
  //@{

  /**
   * Constructor: creates object
   * @param num_entries    The number of entries in the table
   * @param history_bits   The number of history bits
   * @param tag_bits       The number of bits per tag in the table
   * @param
   */
  fatpredict_t( int32 num_entries, uint32 history_bits, uint32 tag_bits );

  /**
   * Destructor: frees object.
   */
  ~fatpredict_t();
  //@}

  /// @name directbp interfaces
  //@{
  /** Prints out identifing information about this branch predictor */
  void printInformation( FILE *fp );
  //@}
  
  /** search the table for a hit.
   *  @return  bool   True == its in the table
   */
  bool   inTable( my_addr_t vpc );
  
  /** make a prediction based on a vpc
   *  @return  bool   The prediction (true == taken).
   */
  bool   predict( int32 entry, uint32 history, bool staticPred );

  /** clear the access counters in the whole table */
  void   clearAccessCounter( void );

  /** search the table for a hit.
   *  @return  uint32   The table index, or -1 if not found.x
   */
  int32  searchTable( my_addr_t vpc );

  /** find an entry to replace */
  int32  chooseReplacement( void );

  /** find an entry to promote */
  int32  choosePromotion( void );

  /** replace something in the table */
  void   overwrite( int32 entry, my_addr_t vpc, uint64 accesses ); 

  /** update the 2 bit counter, and access counter. */
  void   update( int32 entry, uint32 history, bool staticPred, bool taken );

  /** get the entry: used for debugging purposes... */
  fatentry_t *getEntry( int32 entry ) {
    ASSERT( entry < m_num_entries );
    ASSERT( entry >= 0 );
    return ( &m_fatarray[entry] );
  }

  /** get the size of this table: # of entries */
  int32       getSize( void ) {
    return m_num_entries;
  }

private:
  /// make a tag from a virtual program counter
  my_addr_t   makeTag( my_addr_t vpc ) {
    return vpc;
  }

  /// The number of entries
  int32         m_num_entries;

  /// The size of the history
  uint32        m_history_bits;

  /// A mask for accessing history entries
  uint32        m_history_mask;

  /// The size of the tag bits
  uint32        m_tag_bits;

  /// The clock algorithm for tie breaking
  int32         m_clock_index;
  
  /// The array of predictions
  fatentry_t    *m_fatarray;

};

/*------------------------------------------------------------------------*/
/* Global variables                                                       */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Global functions                                                       */
/*------------------------------------------------------------------------*/

#endif  /* _FATPREDICT_H_ */


