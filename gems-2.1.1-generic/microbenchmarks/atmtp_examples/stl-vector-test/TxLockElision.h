/*
   Copyright (C) 2007 Sun Microsystems, Inc.  All rights reserved.
   U.S. Government Rights - Commercial software.  Government users are
   subject to the Sun Microsystems, Inc. standard license agreement and
   applicable provisions of the FAR and its supplements.  Use is subject to
   license terms.  This distribution may include materials developed by
   third parties.Sun, Sun Microsystems and the Sun logo are trademarks or
   registered trademarks of Sun Microsystems, Inc. in the U.S. and other
   countries.  All SPARC trademarks are used under license and are
   trademarks or registered trademarks of SPARC International, Inc. in the
   U.S.  and other countries.

   ----------------------------------------------------------------------

   This file is part of the Adaptive Transactional Memory Test Platform
   (ATMTP) developed and maintained by Kevin Moore and Dan Nussbaum of the
   Scalable Synchronization Research Group at Sun Microsystems Laboratories
   (http://research.sun.com/scalable/).  For information about ATMTP, see
   the GEMS website: http://www.cs.wisc.edu/gems/.

   Please send email to atmtp-interest@sun.com with feedback, questions, or
   to request future announcements about ATMTP.

   ----------------------------------------------------------------------

   ATMTP is distributed as part of the GEMS software toolset and is
   available for use and modification under the terms of version 2 of the
   GNU General Public License.  The GNU General Public License is contained
   in the file $GEMS/LICENSE.

   Multifacet GEMS is free software; you can redistribute it and/or modify
   it under the terms of version 2 of the GNU General Public License as
   published by the Free Software Foundation.
 
   Multifacet GEMS is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.
 
   You should have received a copy of the GNU General Public License along
   with the Multifacet GEMS; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA

   ----------------------------------------------------------------------
*/

///////////////////////////////////////////////////////////////////////////////
//
// TxLockElision.h: Define macros and structures associated with the
// transactional lock elision mechanism.
//

#ifndef    _TxLockElision_H
#define    _TxLockElision_H


#include "sync.h"
#include "rock_tm.h"
#include <time.h>
#include <stdio.h>

typedef char               INT8   ;
typedef short              INT16  ;
typedef unsigned char      UINT8  ;
typedef unsigned short     UINT16 ;
typedef unsigned int       UINT32 ;
typedef unsigned long long UINT64 ;

//
// "Constants of Nature".
//
enum
{
  HT_CASABLE_SIZE_BITS       = 64,
  LOG2_BITS_PER_BYTE         =  3,
  BITS_PER_BYTE              = (1 << LOG2_BITS_PER_BYTE),
  LOG2_CACHE_LINE_SIZE_BYTES =  6,
  CACHE_LINE_SIZE_BYTES      = (1 << LOG2_CACHE_LINE_SIZE_BYTES)
};

//
// MaxHTFailures --- Number of hardware transactions that are allowed
// to fail executing a block before we give up and grab the lock.
//
enum {
  MaxHTFailures = 4
};

//
// Assertions and the like.
//
#define HT_ALWAYS_ASSERT(EX) \
    do                       \
    {                        \
        assert(EX) ;         \
    }                        \
    while (0)

#define HT_ALWAYS_ASSERT_STR(EX, Str)             \
  do {                                            \
    if (!(EX)) {                                  \
      printf("HT Assertion failed: %s\n", (Str)); \
      assert(EX);                                 \
    }                                             \
  }                                               \
  while (0)


#ifdef HT_DEBUG

  #define HT_DBG_CMD(CMD)            \
          CMD
  #define HT_ASSERT(EX)              \
          HT_ALWAYS_ASSERT(EX)
  #define HT_ASSERT_STR(EX , Str)    \
          HT_ALWAYS_ASSERT_STR(EX , Str)

#else // !HT_DEBUG

  #define HT_DBG_CMD(CMD)
  #define HT_ASSERT(EX)
  #define HT_ASSERT_STR(EX , Str)

#endif // !HT_DEBUG

//
// TxLockStats: A structure to collect per-thread lock elision related
// statistics.
//
// A thread can share the same TxLockStats between all atomic blocks
// or use different ones for different blocks, depending on the
// granularity of statistics collection required.  However, the object
// is not accessed in a thread-safe manner, so different threads are
// not allowed to share the same object.
//
struct TxLockStats {
  TxLockStats(): itsHTFailures(0),itsWaitingForLock(0),itsGettingLock(0),
    itsWaitForLockTime(0),itsHeldLockTime(0) {
  }
  UINT64 itsHTFailures;
  UINT64 itsWaitingForLock;
  UINT64 itsGettingLock;
  hrtime_t itsWaitForLockTime;
  hrtime_t itsHeldLockTime;

  void Print() {
    printf("HTFailures = %lld , "
           "WaitedForLock = %lld ( %lld us ) , "
           "GotLock = %lld ( %lld us )\n",
           itsHTFailures,
           itsWaitingForLock, itsWaitForLockTime/1000,
           itsGettingLock, itsHeldLockTime/1000);
  }
};


//
// The lock-elision macros, to be used around an atomic block.
//
// TXLOCK_REGION_BEGIN is used at the beginning of an atomic block,
// and TXLOCK_REGION_END at the end of it.  The macros get the
// following parameters:
//
//   GRAB_LOCK_ST: A *statement* to acquire the lock.  The acquisition
//   is not allowed to fail --- when the statement is done executing,
//   the thread executing it should have the lock. (Hence, any waiting
//   for a busy lock should be handled internally by the statement.)
//
//   LOCK_MINE_OR_FREE_EXP: A boolean *expression* that indicates
//   whether the thread is currently holding the lock, or that the
//   lock is free.  The expression is a condition for a successful
//   execution of the atomic block using a hardware transaction.
//
//   RELEASE_LOCK_ST: A *statement* to release the lock.
//
//   TX_LOCK_STATS_OBJ: An object of type TxLockStats for collecting
//   stats for this block and thread.
//

#ifdef     TXLOCK_NO_ELISION

//
// If lock-elision is disabled, simply grab the lock on
// TXLOCK_REGION_BEGIN and release it on TXLOCK_REGION_END
//

#define TXLOCK_REGION_BEGIN(GRAB_LOCK_ST,                                 \
          LOCK_MINE_OR_FREE_EXP,                                          \
          TX_LOCK_STATS_OBJ)                                              \
  {                                                                       \
    TxLockStats& __stats = TX_LOCK_STATS_OBJ;                             \
    hrtime_t __t = gethrtime();                                           \
    GRAB_LOCK_ST;                                                         \
    hrtime_t __timeLockTaken = gethrtime();                               \
    __stats.itsWaitForLockTime += (__timeLockTaken - __t);                \
    __stats.itsGettingLock++;

#define TXLOCK_REGION_END(RELEASE_LOCK_ST)                                \
    RELEASE_LOCK_ST;                                                      \
    __stats.itsHeldLockTime += (gethrtime() - __timeLockTaken);           \
  }

#else //  !TXLOCK_NO_ELISION

// With lock-elision enabled, we first try to execute the atomic block
// using a transaction, by calling begin_transaction() which begins a
// transaction and returns true; if the transaction is later aborted,
// conrol returns to the begin_transaction() function that then returns
// false.
//
// We use the hardware transaction to atomically execute the block and
// check the LOCK_MINE_OR_FREE_EXP expression, guaranteeing that the
// execution fails (and have no effect) if some other thread acquires
// the lock.
//
// If a transaction fails, we may retry the block using another
// hardware transaction until we have failed MaxHTFailures times. To
// avoid a cascading effect, in which once thread that grabs the lock
// causes all other to fail all their transactions and grab the lock
// as well, we do not retry the block using a transaction until the
// LOCK_MINE_OR_FREE_EXP expression is evaluated to false.
//
// Finally, if we failed too many times we grab the lock and execute
// the block.  We store required information like the number of failed
// hardware transactions and whether we need to release the lock upon
// completion in stack variables declared by the TXLOCK_REGION_BEGIN
// macro.
//

#define TXLOCK_REGION_BEGIN(GRAB_LOCK_ST,                                 \
                            LOCK_MINE_OR_FREE_EXP,                        \
                            TX_LOCK_STATS_OBJ)                            \
  {                                                                       \
    /*HyTMUtils::BackoffBox<> __myBOBox;*/                                \
    UINT64 __HTfails = 0;                                                 \
    bool __IhaveLock = false;                                             \
    hrtime_t timeLockTaken = 0;                                           \
    TxLockStats& __stats = TX_LOCK_STATS_OBJ;                             \
    while (!begin_transaction()) {                                        \
      __HTfails++;                                                        \
      if (__HTfails >= MaxHTFailures) {                                   \
        __IhaveLock = true;                                               \
        hrtime_t t=gethrtime();                                           \
        GRAB_LOCK_ST;                                                     \
        timeLockTaken = gethrtime();                                      \
        __stats.itsWaitForLockTime += (timeLockTaken-t);                  \
        __stats.itsGettingLock++;                                         \
        break;                                                            \
      }                                                                   \
      if (!(LOCK_MINE_OR_FREE_EXP)) {                                     \
        hrtime_t t=gethrtime();                                           \
        while (!(LOCK_MINE_OR_FREE_EXP)) {                                \
          ;                                                               \
        }                                                                 \
        __stats.itsWaitForLockTime += (gethrtime()-t);                    \
        __stats.itsWaitingForLock++;                                      \
      }                                                                   \
      /*else {                                                            \
        __myBOBox.Wait();                                                 \
      }*/                                                                 \
    }                                                                     \
    if (!(LOCK_MINE_OR_FREE_EXP)) {                                       \
      abort_transaction();                                                \
    }


#define TXLOCK_REGION_END(RELEASE_LOCK_ST)                                \
    if (!__IhaveLock) {                                                   \
      commit_transaction();                                               \
    } else {                                                              \
      RELEASE_LOCK_ST;                                                    \
      __stats.itsHeldLockTime += (gethrtime() - timeLockTaken);           \
    }                                                                     \
    __stats.itsHTFailures += __HTfails;                                   \
  }

#endif // !TXLOCK_NO_ELISION


#endif // !_TxLockElision_H
