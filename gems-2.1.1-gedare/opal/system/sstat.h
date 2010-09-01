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

#ifndef _SSTAT_H_
#define _SSTAT_H_

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

/// maximum number of static instructions tracked as 'prior' nodes
const int32     SI_MAX_SUCCESSORS   = 8;
/// maximum number of static instructions tracked as 'successor' nodes
const int32     SI_MAX_PREDECESSORS = 4;

/*------------------------------------------------------------------------*/
/* Class declaration(s)                                                   */
/*------------------------------------------------------------------------*/

/**
* The independent statistics tracking data structure associated with
* The static instruction class.
*
* 
* @see     static_inst_t
* @author  cmauer
* @version $Id: sstat.h 1.2 03/06/01 19:02:59-00:00 cmauer@cottons.cs.wisc.edu $
*/
class static_stat_t {
public:
  /**
   * @name Constructor(s) / destructor
   */
  //@{

  /**
   * Constructor: creates object
   * @param param1 [Description of what param1 is]
   */
  static_stat_t();

  /**
   * Destructor: frees object.
   */
  ~static_stat_t();
  //@}

  /**
   * @name Accessor(s) / mutator(s)
   */
  //@{
  /// add a static instruction to our predecessor list
  void    pushPredecessor( static_inst_t *pred );

  /// add a static instruction to our successor list
  void    pushSuccessor( static_inst_t *pred );

  /// returns the total number of predecessors this node had (-1 if unknown)
  int32   predecessorCount( void );

  /// returns the total number of successors this node had (-1 if unknown)
  int32   successorCount( void );
  //@}

  /** @name profiling statistics */
  //@{
  /// static instructions that depend on this instruction (successors)
  static_inst_t  *m_successors[SI_MAX_SUCCESSORS];
  /// hit count on each successor
  uint32          m_succ_count[SI_MAX_SUCCESSORS];
  /// marks when this instruction's predecessors exceeds the maximum
  bool            m_max_successors;
  /// number of times this instruction has gotten dinged
  uint32          m_total_succs;

  /// static instructions that proceed this instruction (predecessors)
  static_inst_t  *m_predecessors[SI_MAX_PREDECESSORS];
  /// hit count on each predecessor
  uint32          m_pred_count[SI_MAX_SUCCESSORS];
  /// marks when this instruction's successors exceeds the maximum
  bool            m_max_predecessors;
  /// number of times this instruction has gotten dinged
  uint32          m_total_preds;

  /// The physical address of the program counter
  uint64          m_physical_pc;
  /// The total count of the number of misses this instruction caused
  uint32          m_miss_count;
  /// The total count of the number of misses this instruction was critical
  uint32          m_miss_critical_count;
  
#ifdef STATICI_PROFILE
  /// A finite histogram of the number of misses (per window)
  uint16          m_hist_miss[SI_MISS_COUNT];
  /// A count of the number of times this has been seen
  uint16          m_hist_miss_seen[SI_MISS_COUNT];

  /// The number of times this instruction is executed
  uint64    m_count_execute;
  
  /// The number of times this branch is taken
  uint64    m_count_taken;

  /// The number of times this branch is mispredicted by static prediction
  uint32    m_count_static_mispredict;

  /// The number of times this branch is mispredicted
  uint32    m_count_mispredict;

  /// The number of times this branch is mispredicted by the inf predictor
  uint32    m_count_mispredict_inf;
#endif
  //@}
};

/*------------------------------------------------------------------------*/
/* Global variables                                                       */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Global functions                                                       */
/*------------------------------------------------------------------------*/

#endif  /* _SSTAT_H_ */
