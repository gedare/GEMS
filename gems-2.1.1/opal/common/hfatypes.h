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
#ifndef _HFATYPES_H_
#define _HFATYPES_H_

/* 
 * Global include file for entire project.
 * Should be included first in all ".C" project files
 */

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* SimIcs Includes                                                        */
/*------------------------------------------------------------------------*/

/* import C functions */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef SIMICS22X
#include "global.h"
#endif

#include "simics/api.h"
#include "mf_api.h"
#include "simics/alloc.h"              /* needed for "malloc()" interface */

#ifdef __cplusplus
}
#endif

#ifdef SIMICS30
typedef v9_memory_transaction_t memory_transaction_t;
#endif

/*------------------------------------------------------------------------*/
/* Forward class declaration(s)                                           */
/*------------------------------------------------------------------------*/

/// SPARC integer registers are 64bit
typedef uint64 ireg_t;

/// define floating point registers to be the same as integer registers ireg_t
typedef uint64 freg_t;

/// internal type representing addresses (currently 64-bits)
typedef uint64 my_addr_t;

/// 32-bit floating point number
typedef float  float32;

/// 64-bit floating point number
typedef double float64;

/// 64-bit counter type (used by power analysis)
typedef uint64 counter_t;

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

// definitions of MAX / MIN (if needed)
#ifndef MAX
#define MAX(a, b)       ((a) > (b) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a, b)       ((a) < (b) ? (a) : (b))
#endif

/* Statistics tracking definition */
#define STAT_INC(A) (A)++

/*------------------------------------------------------------------------*/
/* Enumerations                                                           */
/*------------------------------------------------------------------------*/

/** classifications of different types of instructions.
 *  this def'n should migrate here if definitions.h ever dies!
 */
enum dyn_execute_type_t {
  DYN_NONE = 0,              // not identifyable
  DYN_EXECUTE,               // integer operation
  DYN_CONTROL,               // branch, call, jump
  DYN_LOAD,                  // load
  DYN_STORE,                 // store
  DYN_PREFETCH,              // prefetch memory operation
  DYN_ATOMIC,                // atomic memory operation (atomic swap, etc)
  DYN_NUM_EXECUTE_TYPES
};

/** classifications of different types of functional unit dependencies,
 *  intalu, floatmult, etc.
 */
enum fu_type_t {
  FU_NONE = 0,             // inst does not use a functional unit
  FU_INTALU,               // integer ALU
  FU_INTMULT,              // integer multiplier
  FU_INTDIV,               // integer divider
  FU_BRANCH,               // compare / branch unit
  FU_FLOATADD,             // floating point adder/subtractor
  FU_FLOATCMP,             // floating point comparator
  FU_FLOATCVT,             // floating point<->integer converter
  FU_FLOATMULT,            // floating point multiplier
  FU_FLOATDIV,             // floating point divider
  FU_FLOATSQRT,            // floating point square root
  FU_RDPORT,               // memory read port
  FU_WRPORT,               // memory write port
  FU_NUM_FU_TYPES          // total functional unit classes
};

/** classifications of different types of register identifiers in the system
 *  each are associated with a style of register access (file + size) */
enum rid_type_t {
  RID_NONE = 0,            // no register identifier
  RID_INT,                 // integer identifier (windowed)
  RID_INT_GLOBAL,          // integer identifier (global)
  RID_CC,                  // condition code identifier
  RID_CONTROL,             // control register
  RID_SINGLE,              // floating point, single precision
  RID_DOUBLE,              // floating point, double precision
  RID_QUAD,                // floating point, quad   precision
  RID_CONTAINER,           // container class representing > 1 register
  RID_NUM_RID_TYPES
};

/// classifications of different types of branch instructions:
enum branch_type_t {
  BRANCH_NONE = 0,           // not a branch
  BRANCH_UNCOND,             // unconditional branch
  BRANCH_COND,               // conditional branch
  BRANCH_PCOND,              // predicated conditional branch
  BRANCH_CALL,               // call
  BRANCH_RETURN,             // return from call (jmp addr, %g0)
  BRANCH_INDIRECT,           // indirect call    (jmpl)
  BRANCH_CWP,                // current window pointer update
  BRANCH_TRAP_RETURN,        // return from trap
  BRANCH_TRAP,               // trap ? indirect jump ??? incorrect?
  BRANCH_PRIV,               // explicit privilege  level change
  BRANCH_NUM_BRANCH_TYPES
};

/// classifications of different types of direct branch predictors
enum branch_predictor_type_t {
  BRANCHPRED_GSHARE = 0,
  BRANCHPRED_AGREE,
  BRANCHPRED_YAGS,
  BRANCHPRED_IGSHARE,
  BRANCHPRED_MLPRED,
  BRANCHPRED_EXTREME,
  BRANCHPRED_NUM_BRANCH_TYPES
};

/// classsification of different types of exceptions
enum exception_t {
  EXCEPT_NONE = 0,
  EXCEPT_MISPREDICT,
  EXCEPT_VALUE_MISPREDICT, 
  EXCEPT_MEMORY_REPLAY_TRAP, 
  EXCEPT_NUM_EXCEPT_TYPES
};

/*------------------------------------------------------------------------*/
/* Project Includes                                                       */
/*------------------------------------------------------------------------*/

typedef unsigned char  byte_t;          /* byte - 8 bits */
typedef unsigned short half_t;          /* half - 16 bits */
typedef unsigned int   word_t;          /* word - 32 bits */
typedef uint64         tick_t;          /* time - 64 bit */
typedef uint16         context_id_t;    /* context identifier */

#endif  /* _HFATYPES_H_ */
