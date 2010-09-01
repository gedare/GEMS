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
#ifndef _ACTOR_H_
#define _ACTOR_H_

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
 * The Actor represents a structure (and its associated state) in a
 * microarchitecture. A scheduling queue and its associated data is
 * one example.
 *
 * @see     dynamic_inst_t
 * @author  cmauer
 * @version $Id: actor.h 1.2 04/08/09 10:31:01-05:00 lyen@opus.cs.wisc.edu $
*/
class actor_t {
public:
  /**
   * @name Constructor(s) / destructor
   */
  //@{
  /**
   * Constructor: creates object
   * @param seq        The owning sequencer for this actor
   * @param queue_size The size of the FIFO queue leading to this actor
   */
  actor_t( pseq_t *seq, int32 queue_size );

  /**
   * Destructor: frees object.
   */
  virtual ~actor_t();
  //@}

  /**
   * @name Methods
   */
  //@{
  /// When the local time advances, advance this local queue's time
  virtual void act_tick( void );

  /// When an event happens to dynamic instruction, it calls the actor for the next step
  virtual void act_wakeup( dynamic_inst_t *inst );

  // insert this item into this queue
  void   queueInsert( dynamic_inst_t *inst );
  // FIFO queue waiting to be scheduled
  bool   queueFull( void );
  /// Print out the occupancy of the queue
  void   queuePrint( void );
  //@}

protected:
  /// A pointer to the sequencer that owns this queue
  pseq_t     *m_seq;

  /// The maximum number of queued instructions
  int32       m_queue_size;

  /// A FIFO queue of instructions waiting to be acted upon
  //  queue<dynamic_inst_t *> m_queue;
};

/**
 *  The scheduling actor: waits for all inputs to become ready for
 *  a given instruction, then issues it to the subsequent execute actor.
 *
 *  
 * @see     actor_t
 * @author  cmauer
 * @version $Id: actor.h 1.2 04/08/09 10:31:01-05:00 lyen@opus.cs.wisc.edu $
 */
class act_schedule_t : public actor_t {
public:
  /// Constructor: creates object
  act_schedule_t( pseq_t *seq, int32 queue_size );
  /// Destructor: frees object.  
  virtual ~act_schedule_t( void );

  void   act_tick( void );
  void   act_wakeup( dynamic_inst_t *inst );

  // every instruct that wakes up in stored in this scheduler
private:
  
  

  /// count of instructions that are in the issue queue
  int32   m_issue_count;
};

/*------------------------------------------------------------------------*/
/* Global variables                                                       */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Global functions                                                       */
/*------------------------------------------------------------------------*/

#endif  /* _ACTOR_H_ */
