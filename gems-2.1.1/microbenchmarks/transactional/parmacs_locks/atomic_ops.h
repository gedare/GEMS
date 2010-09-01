
/*
 * Originally from Michael C. Scott
 * http://www.cs.rochester.edu/u/scott/
 *
 */

#ifndef _ATOMIC_OPS_H
#define _ATOMIC_OPS_H

#include "main.h"
#include "inline.h"
#include "atomic_op_bodies.h"

/********
    Someone who ought to know tells me that almost all Sun systems ever
    built implement TSO, and that those that don't have to be run in TSO
    mode in order for important software (like Solaris) to run correctly.
    This means in practice that the only time a memory barrier is needed
    is between a store and a subsequent load which must be ordered wrt
    one another.
 ********/
INLINE void membar()
BODY({MEMBAR_BODY})

INLINE unsigned long
    swap(volatile unsigned long *ptr, unsigned long val)
BODY({SWAP_BODY})

INLINE unsigned long
    cas(volatile unsigned long *ptr, unsigned long old, unsigned long new)
BODY({CAS_BODY})

INLINE unsigned int
    casx(volatile unsigned long long *ptr,
         unsigned long expected_high,
         unsigned long expected_low,
         unsigned long new_high,
         unsigned long new_low)
BODY({CASX_BODY})

INLINE void
    swapx(volatile unsigned long long *ptr,
          unsigned long new_high,
          unsigned long new_low,
          unsigned long long *old)
BODY({SWAPX_BODY})

INLINE void
    mvx(volatile unsigned long long *from,
        volatile unsigned long long *to)
BODY({MVX_BODY})

INLINE unsigned long
    tas(volatile unsigned long *ptr)
BODY({TAS_BODY})

INLINE unsigned long
    fai(volatile unsigned long *ptr)
BODY({FAI_BODY})

#endif /* _ATOMIC_OPS_H */
