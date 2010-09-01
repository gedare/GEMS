
/*
 * Originally from Michael C. Scott
 * http://www.cs.rochester.edu/u/scott/
 *
 */

#ifndef _CLH_H
#define _CLH_H

#include <sys/time.h>
#include "atomic_ops.h"
#include "main.h"
#include "inline.h"

/********
     data structures for locks for cache-coherent machines */

typedef struct clh_cc_qnode {
    volatile qnode_status status;
    struct clh_cc_qnode *volatile prev;
} clh_cc_qnode;
typedef clh_cc_qnode *volatile clh_cc_qnode_ptr;
typedef clh_cc_qnode_ptr clh_cc_lock;

/********
    data structures for locks for non-cache-coherent machines */

typedef struct clh_numa_qnode {
    volatile qnode_status *volatile status_ptr;
    struct clh_numa_qnode *volatile prev;
} clh_numa_qnode;
#define waiting_p       ((qnode_status *) 0x0)    /* nil */
#define available_p     ((qnode_status *) 0x1)
typedef clh_numa_qnode *volatile clh_numa_qnode_ptr;
typedef clh_numa_qnode_ptr clh_numa_lock;

/**********************************************************************/

#define clh_qn_swap(p,v) \
    (clh_cc_qnode_ptr) swap((volatile unsigned long *) (p), \
        (unsigned long) (v))

#define numa_qn_swap(p,v) \
    (clh_numa_qnode_ptr) swap((volatile unsigned long *) (p), \
        (unsigned long) (v))

#define sp_swap(p,v) \
    (qnode_status *) swap((volatile unsigned long *) (p), \
        (unsigned long) (v))

/**********************************************************************/

/* Except where noted below, all locks require cache coherence, employ a
non-standard interface, and require cas in addition to swap. */

/* Parameter I, below, points to a pointer to a qnode record.
    The qnode "belongs" to the calling process, but may be in main
    memory anywhere in the system, and will generally change identity as
    a result of releasing a lock. */

/******** no timeout, swap only: ********/

#define CLH_PLAIN_ACQUIRE_BODY \
    clh_cc_qnode *pred; \
    I->status = waiting; \
    I->prev = pred = clh_qn_swap(L, I); \
    while (pred->status != available);  /* spin */

INLINE void clh_plain_acquire(clh_cc_lock *L, clh_cc_qnode *I)
BODY({CLH_PLAIN_ACQUIRE_BODY})

#define CLH_PLAIN_RELEASE_BODY \
    clh_cc_qnode *pred = (*I)->prev; \
    (*I)->status = available; \
    *I = pred;

INLINE void clh_plain_release(clh_cc_qnode **I)
BODY({CLH_PLAIN_RELEASE_BODY})

/******** no timeout, ncc-numa machine, swap only: ********/

#define CLH_PLAIN_NUMA_ACQUIRE_BODY \
    clh_numa_qnode *pred; \
    qnode_status *p; \
    volatile qnode_status stat = waiting; \
    I->status_ptr = waiting_p; \
    I->prev = pred = numa_qn_swap(L, I); \
    p = sp_swap(&pred->status_ptr, &stat); \
    if (p == available_p) return; \
    while (stat != available);  /* spin */

INLINE void clh_plain_numa_acquire(clh_numa_lock *L, clh_numa_qnode *I)
BODY({CLH_PLAIN_NUMA_ACQUIRE_BODY})

#define CLH_PLAIN_NUMA_RELEASE_BODY \
    clh_numa_qnode *pred = (*I)->prev; \
    qnode_status *p = sp_swap(&((*I)->status_ptr), available_p); \
    if (p) *p = available; \
    *I = pred;

INLINE void clh_plain_numa_release(clh_numa_qnode **I)
BODY({CLH_PLAIN_NUMA_RELEASE_BODY})

/******** blocking timeout, bounded space: ********/

#define CLH_TRY_ACQUIRE_BODY \
    clh_cc_qnode *pred; \
    I->status = waiting; \
    pred = clh_qn_swap(L, I); \
    while (1) { \
        qnode_status stat = pred->status; \
        if (stat == available) { \
            I->prev = pred; \
            return; \
        } \
        if (stat == leaving) { \
            clh_cc_qnode *temp = pred->prev; \
            /* tell predecessor that no one will ever look at its node \
            again: */ \
            pred->status = recycled; \
            pred = temp; \
        } \
        /* stat might also be transient, if somebody left the queue just \
        before I entered.  I simply wait for them. */ \
    }

INLINE void clh_try_acquire(clh_cc_lock *L, clh_cc_qnode *I)
BODY({CLH_TRY_ACQUIRE_BODY})

extern unsigned int clh_try_timed_acquire_slowpath(clh_cc_lock *L,
    clh_cc_qnode *I, clh_cc_qnode *pred, hrtime_t T);

#define CLH_TRY_TIMED_ACQUIRE_BODY \
    clh_cc_qnode *pred; \
    I->status = waiting; \
    pred = clh_qn_swap(L, I); \
    if (pred->status == available) { \
        I->prev = pred; \
        return true; \
    } else { \
        return clh_try_timed_acquire_slowpath(L, I, pred, T); \
    }

INLINE unsigned int clh_try_timed_acquire(clh_cc_lock *L, clh_cc_qnode *I, hrtime_t T)
BODY({CLH_TRY_TIMED_ACQUIRE_BODY})

#define CLH_TRY_RELEASE_BODY \
    qnode_status stat; \
    clh_cc_qnode *pred = (*I)->prev; \
    /* can't set my node to available if it's currently transient */ \
    while (1) { \
        if ((stat = s_compare_and_swap(&(*I)->status, waiting, \
                available)) == waiting) \
            break; \
        while ((*I)->status == transient);  /* spin */ \
            /* PREEMPTION VULNERABILITY: I'm stuck if successor is \
                preempted while trying to leave */ \
    } \
    *I = pred;

INLINE void clh_try_release(clh_cc_qnode **I)
BODY({CLH_TRY_RELEASE_BODY})

#endif /* _CLH_H */
