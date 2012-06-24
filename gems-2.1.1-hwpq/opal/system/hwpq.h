/*
 * hwpq.h
 *
 * A priority queue supporting:
 *  enqueue
 *  extract
 *  first (peek)
 * For modelling a HW PQ.  
 */

#ifndef _HWPQ_H_
#define _HWPQ_H_

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include <vector>
#include <stdio.h>

#include "dynamic.h"

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

//#define GAB_DEBUG
//#define GAB_HWDS_DEBUG

/*------------------------------------------------------------------------*/
/* Class declaration(s)                                                   */
/*------------------------------------------------------------------------*/

// types of hwpq instructions
enum hwds_inst_type_t {
  HWDS_INST_UNUSED = 0,
  HWDS_INST_FIRST,
  HWDS_INST_ENQUEUE,
  HWDS_INST_EXTRACT,
  HWDS_INST_GET_CURRENT_SIZE,
  HWDS_INST_GET_CONTEXT,
  HWDS_INST_EXTRACT_LAST,
  HWDS_INST_SPILL,
  HWDS_INST_FILL,
  HWDS_INST_LAST_PRIO,
  HWDS_INST_GET_POINTER,
  HWDS_INST_ADJUST_SPILL_COUNT,
  HWDS_INST_GET_SIZE,
  HWDS_INST_SET_CURRENT_ID,
  HWDS_INST_GET_CURRENT_ID,
  HWDS_INST_SET_SIZE,
  HWDS_INST_INVALIDATE,
  HWDS_INST_SEARCH,
  HWDS_INST_SET_PAYLOAD,
  HWDS_INST_GET_TRAP_PAYLOAD,
  HWDS_INST_SET_TRAP_PAYLOAD,
  HWDS_NUM_INST_TYPES
};

/**
 * An "in-flight" hwpq instruction in the processor model.
 * This will probably change to a hwds interface and hwpq implementation.
 *
 * @see     dynamic_inst_t, waiter_t
 * @author  gedare
 * @version $Id$
 */
//**************************************************************************
class hwpq_inst_t : public dynamic_inst_t {
public:
  /** @name Constructor(s) / Destructor */
  //@{
  //  /** Constructor: uses default constructor */
  hwpq_inst_t( static_inst_t *s_inst,
      int32 window_index,
      pseq_t *pseq,
      abstract_pc_t *at,
      pa_t physicalPC,
      trap_type_t trapgroup,
      uint32 proc
      );

  /** Destructor: uses default destructor. */
  ~hwpq_inst_t();
  //@}

  /** Allocates object throuh allocator interface */
  void *operator new( size_t size ) {
    return ( m_myalloc.memalloc( size ) );
  }
  
  /** frees object through allocator interface */
  void operator delete( void *obj ) {
    m_myalloc.memfree( obj );
  }

  /** dynamic_inst_t virtual functions */
  /** squash an instruction in the pipe, after it has been renamed.
   *  As it rolls back the state it is implemented differently by control,
   *  exec, mem, hwds instrs. */
  void Squash();
  /** retire: change the machines in-order architected state according
   *  to an instruction.
   *  (implemented differently by control, exec, mem, hwds instrs).
   *  @param a   The next in-order pc, when this instruction is retired.
   */
  void Retire( abstract_pc_t *a );

  /** do the generic execution stuff for hwds operations */
  void Execute();

  /** @name memory allocation:: */
  //@{
  static listalloc_t m_myalloc;
  //@}

private:
  // the hwpq for this instruction
  int m_queue_idx;

  // the type of this instruction
  hwds_inst_type_t m_type;
};

/*------------------------------------------------------------------------*/
/* Global variables                                                       */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Global functions                                                       */
/*------------------------------------------------------------------------*/

#endif /* _HWPQ_H_ */
