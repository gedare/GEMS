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

//
// STL Vector Lock-Elision Microbenchmark
//

// We communicate with Ruby/LogTM via MAGIC(n) instructions.
//
#include "magic-instruction.h"

#include <alloca.h>
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream.h>
#include <sys/processor.h>
#include <sys/procset.h>
#include <sys/time.h>
#include <sys/types.h>

#include "sync.h"
#include "rand-seq.h"
#include "TxLockElision.h"
#include "Locks.h"

#include <vector>

inline void* operator new(size_t s, void* location) throw() {
  return location;
}

///////////////////////////////////////
//
// Types and Definitions.
//

#define CACHE_LINE_SIZE 64
#define MAX_THREADS 127

//
// Compiled-in/out printing.
//
#ifdef     DEBUG_PRINTFS
#define DEBUG_PRINTF printf
#else  // !DEBUG_PRINTFS
#define DEBUG_PRINTF while (0) printf
#endif // !DEBUG_PRINTFS
//
// Compiled-in/out assertions.
//
#ifdef     HT_DEBUG
#define DEBUG_ASSERT assert
#else  // !HT_DEBUG
#define DEBUG_ASSERT while (0) assert
#endif // !HT_DEBUG

struct Counter {
  Counter(UINT64 theVal):data(theVal) {
  }
  struct _data {
    _data(UINT64 theVal):counter(theVal) {
    }
    UINT64 counter;
  } data;
  char pad[CACHE_LINE_SIZE-sizeof(Counter::data)];
};

typedef std::vector<Counter> SeqType;

#ifdef    USE_RWLOCK
typedef SimpleRWLock LockType;

#define WLOCK_CMD(TID) g_TxLock.LockForWrite((TID))
#define RLOCK_CMD(TID) g_TxLock.LockForRead()
#define LOCK_FREE_OR_MINE_FOR_READ(owner, TID) \
  ((owner) == LockType::Free || (owner) == LockType::Readers)
#define LOCK_FREE_OR_MINE_FOR_WRITE(owner, TID) \
  ((owner) == LockType::Free || (owner) == (TID))

#else //  !USE_RWLOCK
typedef SimpleLock LockType;

#define WLOCK_CMD(TID)\
g_TxLock.Lock((TID))

#define RLOCK_CMD(TID)\
g_TxLock.Lock((TID))

#define LOCK_FREE_OR_MINE_FOR_READ(owner, TID) \
  ((owner) == LockType::Free || (owner) == (TID))
#define LOCK_FREE_OR_MINE_FOR_WRITE(owner, TID) \
  ((owner) == LockType::Free || (owner) == (TID))

#endif // !USE_RWLOCK


//
// Test constants --- might want to allow passing some of these as parameters.
//

enum OpType {
  Inc,
  Dec,
  Read,
  NumOpTypes
};

enum {
  IncPercent = 10,
  DecPercent = 10
};

//
// For communicating arguments to threads.
//
struct ThreadArgs
{
  int itsTid;
  rand_seq_t* itsRandSeq;
};

//
// For statistics gathered by each thread.
//
struct ThreadStats
{
  ThreadStats():
    itsNumInserts(0), itsNumDeletes(0), itsNumInc(0), itsNumDec(0),
    itsReadValSum(0)
  {
  }

  UINT32 itsNumInserts;
  UINT32 itsNumDeletes;
  UINT32 itsNumInc;
  UINT32 itsNumDec;
  UINT64 itsReadValSum;

  TxLockStats itsTxLockStats[NumOpTypes];
};

struct PaddedThreadStats
{
  UINT8 prePad[CACHE_LINE_SIZE_BYTES];
  ThreadStats stats;
  UINT8 postPad[CACHE_LINE_SIZE_BYTES];
};


///////////////////////////////////////
//
// Globals.
//

#pragma align CACHE_LINE_SIZE (g_Threads)
static pthread_t* g_Threads;
static int g_NThreads;
static int g_NIters;
static int g_TLMemSize;
static int g_InitSeqSize;
static int g_SyncType;
static int g_MachineConfig;
static int g_SuppressTableStats;

#pragma align CACHE_LINE_SIZE (g_ThreadsStats)
ThreadStats* g_ThreadsStats[MAX_THREADS];

#pragma align CACHE_LINE_SIZE (g_RandSeqs)
static rand_seq_t g_RandSeqs[MAX_THREADS];

#pragma align CACHE_LINE_SIZE (g_TxLock)
static LockType g_TxLock;

#pragma align CACHE_LINE_SIZE (g_seqP)
static SeqType* g_seqP;

#pragma align CACHE_LINE_SIZE (g_seqSize)
static UINT64 g_seqSize;

#pragma align CACHE_LINE_SIZE (g_counterRange)
static UINT64 g_counterRange;

#pragma align CACHE_LINE_SIZE (g_ThreadsAlive)
static volatile int g_ThreadsAlive;

#pragma align CACHE_LINE_SIZE (g_ThreadsReady)
static volatile int g_ThreadsReady;

#pragma align CACHE_LINE_SIZE (g_ThreadsFinished)
static volatile int g_ThreadsFinished;

#pragma align CACHE_LINE_SIZE (g_ThreadsIterCounter)
static volatile int g_ThreadsIterCounter;

#pragma align CACHE_LINE_SIZE (g_StartTimes)
hrtime_t g_StartTimes[MAX_THREADS];

#pragma align CACHE_LINE_SIZE (g_EndTimes)
hrtime_t g_EndTimes[MAX_THREADS];

//
// The arrays below map thread_ids (0..N_threads-1) to processor_ids.  This
// mapping varies from machine to machine.
//
// This enum serves as an index into an array of the arrays below,
// also serving as a way to specify a map by index on the command
// line.
//

enum MachineConfigurationClasses {
  PROCMAP_GENERIC_64 = 0,
  PROCMAP_NIAGARA_ROCK,
  PROCMAP_GARBAGE,
  PROCMAP_MDE69003A,
  PROCMAP_BOBCAT,
  PROCMAP_SUMOCATB,
  PROCMAP_SAREK_1P,
  PROCMAP_SAREK_2P,
  PROCMAP_SAREK_3P,
  PROCMAP_SAREK_4P,
  PROCMAP_SAREK_6P,
  PROCMAP_SAREK_8P,
  PROCMAP_SAREK_12P,
  PROCMAP_SAREK_16P,
  PROCMAP_SAREK_24P,
  PROCMAP_SAREK_32P,
  PROCMAP_SAREK_48P,
  PROCMAP_SAREK_64P,
  FIRST_INVALID_MACHINE_CONFIG
};

static processorid_t
generic_64_IdProcidMap[] = {
  // Use this if you don't know any better, but don't assume reasonable results
  //
  0,   1,   2,   3,   4,   5,   6,   7,
  8,   9,   10,  11,  12,  13,  14,  15,
  16,  17,  18,  19,  20,  21,  22,  23,
  24,  25,  26,  27,  28,  29,  30,  31,
  32,  33,  34,  35,  36,  37,  38,  39,
  40,  41,  42,  43,  44,  45,  46,  47,
  48,  49,  50,  51,  52,  53,  54,  55,
  56,  57,  58,  59,  60,  61,  62,  63
};

static processorid_t
niagara_rock_IdProcidMap[] = {
  // niagara, rock
  //
  0,   1,   2,   3,   4,   5,   6,   7,
  8,   9,   10,  11,  12,  13,  14,  15,
  16,  17,  18,  19,  20,  21,  22,  23,
  24,  25,  26,  27,  28,  29,  30,  31
};


static processorid_t
garbage_IdProcidMap[] = {
  // garbage
  //
  0,   1,             4,   5,   6,   7,
  8,   9,   10,  11,  12,  13,  14,  15,
  16,  17,  18,  19,  20,  21,  22,  23,
  24,  25,  26,  27,  28,  29,  30,  31
};

static processorid_t
mde69003a_IdProcidMap[] = {
  // mde69003a
  //
  0,   512, 1,   513, 2,   514, 3,   515,
  4,   516, 5,   517, 6,   518, 7,   519,
  8,   520, 9,   521, 10,  522, 11,  523,
  12,  524, 13,  525, 14,  526, 15,  527,
  16,  528, 17,  529, 18,  530, 19,  531,
  20,  532, 21,  533, 22,  534, 23,  535
};

static processorid_t
bobcat_IdProcidMap[] = {
  // bobcat, pseudocat, sumocat
  //
  0,   4,   1,   5,   2,   6,   3,   7,
  32,  36,  33,  37,  34,  38,  35,  39,
  64,  68,  65,  69,  66,  70,  67,  71,
  96,  100, 97,  101, 98,  102, 99,  103,
  128, 132, 129, 133, 130, 134, 131, 135,
  160, 164, 161, 165, 162, 166, 163, 167,
  192, 196, 193, 197, 194, 198, 195, 199,
  224, 228, 225, 229, 226, 230, 227, 231,
  256, 260, 257, 261, 258, 262, 259, 263,
  288, 292, 289, 293, 290, 294, 291, 295,
  320, 324, 321, 325, 322, 326, 323, 327,
  352, 356, 353, 357, 354, 358, 355, 359,
  384, 388, 385, 389, 386, 390, 387, 391,
  416, 420, 417, 421, 418, 422, 419, 423,
  448, 452, 449, 453, 450, 454, 451, 455,
  480, 484, 481, 485, 482, 486, 483, 487,
  512, 516, 513, 517, 514, 518, 515, 519,
  544, 548, 545, 549, 546, 550, 547, 551
};

static processorid_t
sumocatb_IdProcidMap[] = {
  // sumocatb
  //
  32,  36,  33,  37,  34,  38,  35,  39,
  64,  68,  65,  69,  66,  70,  67,  71,
  96,  100, 97,  101, 98,  102, 99,  103,
  128, 132, 129, 133, 130, 134, 131, 135,
  160, 164, 161, 165, 162, 166, 163, 167,
  192, 196, 193, 197, 194, 198, 195, 199,
  224, 228, 225, 229, 226, 230, 227, 231,
  256, 260, 257, 261, 258, 262, 259, 263,
  288, 292, 289, 293, 290, 294, 291, 295,
  320, 324, 321, 325, 322, 326, 323, 327,
  352, 356, 353, 357, 354, 358, 355, 359,
  384, 388, 385, 389, 386, 390, 387, 391,
  416, 420, 417, 421, 418, 422, 419, 423,
  448, 452, 449, 453, 450, 454, 451, 455,
  480, 484, 481, 485, 482, 486, 483, 487,
  512, 516, 513, 517, 514, 518, 515, 519,
  544, 548, 545, 549, 546, 550, 547, 551
};

static processorid_t
sarek_1p_IdProcidMap[] = {
  // sarek 1p
  //
  0
};

static processorid_t
sarek_2p_IdProcidMap[] = {
  // sarek 2p
  //
  0,  1
};

static processorid_t
sarek_3p_IdProcidMap[] = {
  // sarek 3p
  //
  0,  1,  2
};

static processorid_t
sarek_4p_IdProcidMap[] = {
  // sarek 4p
  //
  0,  1,  2,  3
};

static processorid_t
sarek_6p_IdProcidMap[] = {
  // sarek 6p
  //
  0,  1,  2,      4,  5,  6
};

static processorid_t
sarek_8p_IdProcidMap[] = {
  // sarek 8p
  //
  0,  1,  2,  3,  4,  5,  6,  7
};

static processorid_t
sarek_12p_IdProcidMap[] = {
  // sarek 12p
  //
  0,  1,  2,      4,  5,  6,
  8,  9, 10,     12, 13, 14
};

static processorid_t
sarek_16p_IdProcidMap[] = {
  // sarek 16p
  //
  0,  1,  2,  3,  4,  5,  6,  7,
  8,  9, 10, 11, 12, 13, 14, 15
};

static processorid_t
sarek_24p_IdProcidMap[] = {
  // sarek 24p
  //
  0,  1,  2,  3,  4,  5,  6,  7,
  8,  9, 10, 11, 12, 13, 14, 15,
  16, 17, 18, 19, 20, 21, 22, 23
};

static processorid_t
sarek_32p_IdProcidMap[] = {
  // sarek 32p
  //
  0,  1,  2,  3,  4,  5,  6,  7,
  8,  9, 10, 11, 12, 13, 14, 15,

  32, 33, 34, 35, 36, 37, 38, 39,
  40, 41, 42, 43, 44, 45, 46, 47
};

static processorid_t
sarek_48p_IdProcidMap[] = {
  // sarek 48p
  //
  0,  1,  2,  3,  4,  5,  6,  7,
  8,  9, 10, 11, 12, 13, 14, 15,
  16, 17, 18, 19, 20, 21, 22, 23,

  32, 33, 34, 35, 36, 37, 38, 39,
  40, 41, 42, 43, 44, 45, 46, 47,
  48, 49, 50, 51, 52, 53, 54, 55
};

static processorid_t
sarek_64p_IdProcidMap[] = {
  // sarek 64p
  //
  0,  1,  2,  3,  4,  5,  6,  7,
  8,  9, 10, 11, 12, 13, 14, 15,
  16, 17, 18, 19, 20, 21, 22, 23,

  32, 33, 34, 35, 36, 37, 38, 39,
  40, 41, 42, 43, 44, 45, 46, 47,
  48, 49, 50, 51, 52, 53, 54, 55,

  64, 65, 66, 67, 78, 79, 70, 71,
  72, 73, 74, 75, 76, 77, 78, 79
};


typedef struct {
  int map_size;
  processorid_t *map;
} SizedMap;

static SizedMap
IdProcidMaps[] = {
  { sizeof(generic_64_IdProcidMap),       generic_64_IdProcidMap },
  { sizeof(niagara_rock_IdProcidMap),     niagara_rock_IdProcidMap },
  { sizeof(garbage_IdProcidMap),          garbage_IdProcidMap },
  { sizeof(mde69003a_IdProcidMap),        mde69003a_IdProcidMap },
  { sizeof(bobcat_IdProcidMap),           bobcat_IdProcidMap },
  { sizeof(sumocatb_IdProcidMap),         sumocatb_IdProcidMap },
  { sizeof(sarek_1p_IdProcidMap),         sarek_1p_IdProcidMap },
  { sizeof(sarek_2p_IdProcidMap),         sarek_2p_IdProcidMap },
  { sizeof(sarek_3p_IdProcidMap),         sarek_3p_IdProcidMap },
  { sizeof(sarek_4p_IdProcidMap),         sarek_4p_IdProcidMap },
  { sizeof(sarek_6p_IdProcidMap),         sarek_6p_IdProcidMap },
  { sizeof(sarek_8p_IdProcidMap),         sarek_8p_IdProcidMap },
  { sizeof(sarek_12p_IdProcidMap),        sarek_12p_IdProcidMap },
  { sizeof(sarek_16p_IdProcidMap),        sarek_16p_IdProcidMap },
  { sizeof(sarek_24p_IdProcidMap),        sarek_24p_IdProcidMap },
  { sizeof(sarek_32p_IdProcidMap),        sarek_32p_IdProcidMap },
  { sizeof(sarek_48p_IdProcidMap),        sarek_48p_IdProcidMap },
  { sizeof(sarek_64p_IdProcidMap),        sarek_64p_IdProcidMap }
};


////////////////////////////////////////////////
//
// Code starts here.
//

//
// Function for signalling simics via breakpoint
//
static void
ruby_breakpoint()
{
  // With Ruby/LogTM, this is how you get a breakpoint.
  //
  MAGIC(0x40000);
}

//
// Function for signalling simics that we're done
//
static void
ruby_finish_unit()
{
  // With Ruby/LogTM, this is how you signify completion of a "unit of work".
  //
  MAGIC(0x50000);
}

//
// The thread body.  Synchronize threads upon entry, and then once per
// iteration.
//
extern "C"
{
  typedef void* (*threadBodyPtr) (void*);
}

// Empty "markers" functions used for tracing inserts/deletes during
// executions.  Note --- direct the compiler not to inline them!
//
extern "C" {
  void deleting() {
  }
  void inserting() {
  }
}

inline void
OneIter(SeqType& theSeq,
        OpType theOp,
        unsigned int theKey,
        int theTid,
        ThreadStats* theStatsP)
{
//   printf("%d: Executing %s operation with key %lld\n",
//          theTid, theOp==Inc ? "Increment" : "Decrement", theKey);

  UINT64 owner;
  switch (theOp) {
    case Inc:
#ifdef    USE_RWLOCK
      TXLOCK_REGION_BEGIN(g_TxLock.LockForWrite(theTid),
                          (owner = g_TxLock.GetOwner(),
                           owner == LockType::Free || owner == theTid),
                          theStatsP->itsTxLockStats[Inc]);
#else//  !USE_RWLOCK
      TXLOCK_REGION_BEGIN(g_TxLock.Lock(theTid),
                          (owner = g_TxLock.GetOwner(),
                           owner == LockType::Free || owner == theTid),
                          theStatsP->itsTxLockStats[Inc]);
#endif// !USE_RWLOCK
      {
        theStatsP->itsNumInc++;

        SeqType::size_type s = theSeq.size();
        while (theKey >= s) {
          theKey -= s;
        }
        theSeq[theKey].data.counter++;

        if (theSeq[theKey].data.counter == g_counterRange) {
          // Split element into two elements with g_counterRange/2 counters.
          //
          theSeq[theKey].data.counter = g_counterRange/2;
          theSeq.insert(theSeq.begin()+theKey, Counter(g_counterRange/2));
          g_seqSize++;

//          inserting();
          theStatsP->itsNumInserts++;
        }
      }
      TXLOCK_REGION_END(g_TxLock.UnLock());

      break;

    case Dec:
#ifdef    USE_RWLOCK
      TXLOCK_REGION_BEGIN(g_TxLock.LockForWrite(theTid),
                          (owner = g_TxLock.GetOwner(),
                           owner == LockType::Free || owner == theTid),
                          theStatsP->itsTxLockStats[Dec]);
#else//  !USE_RWLOCK
      TXLOCK_REGION_BEGIN(g_TxLock.Lock(theTid),
                          (owner = g_TxLock.GetOwner(),
                           owner == LockType::Free || owner == theTid),
                          theStatsP->itsTxLockStats[Dec]);
#endif// !USE_RWLOCK
      {
        theStatsP->itsNumDec++;

        SeqType::size_type s = theSeq.size();
        while (theKey >= s) {
          theKey -= s;
        }
        theSeq[theKey].data.counter--;
        if (theSeq[theKey].data.counter == 0) {
          // Remove the element.
          //
          theSeq.erase(theSeq.begin()+theKey);

          if(g_seqSize <= 1){
            cerr << "Error --- all counters of the vector were removed!" << endl;
            cerr << "Try picking a c-range larger than: " << g_counterRange << endl;
            assert(g_seqSize > 0);
          }

          g_seqSize--;

//          deleting();
          theStatsP->itsNumDeletes++;
        }
      }
      TXLOCK_REGION_END(g_TxLock.UnLock());

      break;

    case Read:
#ifdef    USE_RWLOCK
      TXLOCK_REGION_BEGIN(g_TxLock.LockForRead(),
                          !g_TxLock.IsOwnedForWrite(),
                          theStatsP->itsTxLockStats[Read]);
#else//  !USE_RWLOCK
      TXLOCK_REGION_BEGIN(g_TxLock.Lock(theTid),
                          (owner = g_TxLock.GetOwner(),
                           owner == LockType::Free || owner == theTid),
                          theStatsP->itsTxLockStats[Read]);
#endif// !USE_RWLOCK
      {
        SeqType::size_type s = theSeq.size();
        while (theKey >= s) {
          theKey -= s;
        }
        theStatsP->itsReadValSum += theSeq[theKey].data.counter;
      }
      TXLOCK_REGION_END(g_TxLock.UnLock());
  }
}

void
TestBody(UINT32 theIterNum,
         SeqType& theSeq,
         rand_seq_t* theRandomSeqP,
         int theTid,
         ThreadStats* theStatsP)
{
  for (UINT32 iter=0; iter<theIterNum; iter++) {

    // Run this iteration of the test.
    //
    DEBUG_PRINTF("%d:    running iteration %d...\n", theTid, iter);

    UINT32 opCode = rand_seq_next(theRandomSeqP) % 100;
    OpType op = (opCode < IncPercent ?
                 Inc : (opCode >= 100 - DecPercent ? Dec : Read));

    assert(g_seqSize > 0);
    unsigned int key = rand_seq_next(theRandomSeqP) % g_seqSize;

    OneIter(theSeq, op, key, theTid, theStatsP);

    // Finished with this iteration of the test.  Back around once
    // more.
    //
    DEBUG_PRINTF("%d:    iteration %d finished.\n", theTid, iter);
  }
}

static void
VectorThreadBody(void* theArgsP)
{
  ThreadArgs* myArgsP = (ThreadArgs*)theArgsP;
  int myTid = myArgsP->itsTid;

//   // Allocate local memory pool:
//   //
//   MemAllocsStatic::ThreadInit(myTid);

  DEBUG_PRINTF("%d:  starting test...\n", myTid);

  DEBUG_ASSERT(0 <= myTid && myTid < g_NThreads);

  // Bind this thread to a processor, using the specified processor
  // binding map.
  //
  processorid_t destination =
    IdProcidMaps[g_MachineConfig].map[myTid];
  printf("thread %d: binding to processor %d (running on %d)\n",
         myTid, destination, getcpuid());
  if (processor_bind(P_LWPID, P_MYID, destination, NULL) != 0) {
    fprintf(stderr, "processor_bind(%d) failed for thread %d: ",
            destination, myTid);
    perror("");
  } else {
    printf("%d:  bound to processor %d.\n",
           myTid, getcpuid());
  }

  // Read global vars:
  //

  UINT32 opsNum = g_NIters;
  SeqType& sharedSeq = *g_seqP;

  // Allocate Stats Structure.
  //
  ThreadStats* myStatsP = &(new PaddedThreadStats)->stats;

  // Walk vector to pre-populate the TSB.
  //

  UINT64 sumOfElems = 0;
  for (SeqType::iterator i = sharedSeq.begin(); i != sharedSeq.end(); i++) {
    sumOfElems += (*i).data.counter;
  }
  printf("%d:  Finished walking through all elements of the sequence. "
         "Their sum is %lld\n",
         myTid, sumOfElems);

  /////////////////////////////////////////////////////////////////
  //
  // First barrier.  Once we get past this barrier, ruby has been
  // installed.
  //

  // Use CAS to increment g_ThreadsAlive counter.
  //
  while (true) {
    int read_ThreadsAlive = g_ThreadsAlive;
    // If I am about to start everything going, break to allow for
    // turning on the accurate timing model.
    //
    if (read_ThreadsAlive + 1 == g_NThreads) {
      DEBUG_PRINTF("%d:  about to trigger breakpoint...\n",
                   myTid);
      ruby_breakpoint();
    }
    if (CAS32(&g_ThreadsAlive, read_ThreadsAlive, read_ThreadsAlive + 1)) {
      break;
    }
    //yield();
  }
  DEBUG_PRINTF("%d:  successfully incremented g_ThreadsAlive...\n",
               myTid);
  // Wait for all threads to show up.
  //
  if (g_ThreadsAlive < g_NThreads) {
    DEBUG_PRINTF("%d:  waiting for others...\n", myTid);
    while (g_ThreadsAlive < g_NThreads) {
      //yield();
    }
  }
  DEBUG_ASSERT(g_ThreadsAlive == g_NThreads);
  //
  // Once we get past this point, ruby has been installed.


  // Walk table to warm up cache.
  //
  UINT64 sumOfElems2 = 0;
  for (SeqType::iterator i = sharedSeq.begin(); i != sharedSeq.end(); i++) {
    UINT64 val = (*i).data.counter;
    sumOfElems2 += val;
    CAS64(&((*i).data.counter), val, val);
  }
  printf("%d:  Finished second (post ruby) walking through and dummy-CASing "
         "all elements of the sequence. Their sum is %lld\n",
         myTid, sumOfElems2);

  // Record the start time for this thread.
  //
  g_StartTimes[myTid] = gethrtime();


  /////////////////////////////////////////////////////////////////
  //
  // Second barrier.  Once we get past this barrier, we can start the
  // test.
  //

  // Use CAS to increment g_ThreadsReady counter.
  //
  while (true) {
    int read_ThreadsReady = g_ThreadsReady;
    // If I am about to start everything going, break to allow for
    // turning on the accurate timing model.
    //
    if (read_ThreadsReady + 1 == g_NThreads) {
      DEBUG_PRINTF("%d:  about to trigger breakpoint...\n",
                   myTid);
      ruby_finish_unit();
    }
    if (CAS32(&g_ThreadsReady, read_ThreadsReady, read_ThreadsReady + 1)) {
      break;
    }
    //yield();
  }
  DEBUG_PRINTF("%d:  successfully incremented g_ThreadsReady...\n",
               myTid);
  // Wait for all threads to show up.
  //
  if (g_ThreadsReady < g_NThreads) {
    DEBUG_PRINTF("%d:  waiting for others...\n", myTid);
    while (g_ThreadsReady < g_NThreads) {
      //yield();
    }
  }
  DEBUG_ASSERT(g_ThreadsReady == g_NThreads);
  //
  // Once we get past this point, everybody can start the test.

  DEBUG_PRINTF("%d:  running test...\n", myTid);

  //
  // OK -- everybody is here now.  The test can start.
  //

  TestBody(opsNum, sharedSeq, myArgsP->itsRandSeq, myTid, myStatsP);

  // Record the end time for this thread.
  //
  g_EndTimes[myTid] = gethrtime();

  // Use CAS to increment g_ThreadsFinished counter.
  //
  while (true) {
    int read_ThreadsFinished = g_ThreadsFinished;
    // If I am the last thread to finish, break so that we can take
    // ruby statistics before dumping out our HyTM statistics.
    //
    if (read_ThreadsFinished+1 == g_NThreads) {
      DEBUG_PRINTF("%d:  last thread incrementing g_ThreadsFinished\n",
                   myTid);
      ruby_finish_unit();
    }
    if (CAS32(&g_ThreadsFinished,
              read_ThreadsFinished,
              read_ThreadsFinished + 1)) {
      break;
    }
    //yield();
  }
  DEBUG_PRINTF("%d:  successfully incremented g_ThreadsFinished...\n",
               myTid);

  // Wait for other threads to finish the interesting part of the
  // test, so that no thread starts dumping stats until all threads
  // have finished the operative part of the test.
  //
  DEBUG_PRINTF("%d: waiting for other threads to finish...\n", myTid);
  while (g_ThreadsFinished < g_NThreads) {
    //yield();
  }
  DEBUG_PRINTF("%d: other threads have finished.\n", myTid);

  // Make my statistics available.
  //
  g_ThreadsStats[myTid] = myStatsP;
}


static void
PrintStats()
{

  UINT32 totalDeleted=0;
  UINT32 totalInserted=0;
  UINT32 totalInc=0;
  UINT32 totalDec=0;
  for (UINT32 tid=0; tid<g_NThreads; tid++) {
    ThreadStats* statsP = g_ThreadsStats[tid];
    totalInserted += statsP->itsNumInserts;
    totalDeleted += statsP->itsNumDeletes;
    totalInc += statsP->itsNumInc;
    totalDec += statsP->itsNumDec;
    UINT32 readsNum = g_NIters - (statsP->itsNumInc +  statsP->itsNumDec);

    cout << "Stats for thread " << tid << endl << endl;
    cout << "Increments: " << statsP->itsNumInc << endl;
    cout << "Decrements: " << statsP->itsNumDec << endl;
    cout << "Reads: " << readsNum << endl;
    if(readsNum > 0){
      cout << "Average read val: " << statsP->itsReadValSum/readsNum << endl;
    } else {
      cout << "Average read val is undefined." << endl;
    }

    cout << "Inserts: " << statsP->itsNumInserts ;
    if (statsP->itsNumInc > 0) {
      cout << " (" << 100*statsP->itsNumInserts/statsP->itsNumInc<<"% of increments)" << endl;
    } else {
      cout << endl;
    }

    cout << "Deletes: " << statsP->itsNumDeletes ;
    if(statsP->itsNumDec > 0){
      cout << " (" << 100*statsP->itsNumDeletes/statsP->itsNumDec<<"% of decrements)" << endl;
    } else {
      cout << endl;
    }

    cout << "Lock Elision Stats:\n";
    printf("\t%-15s ","Increments:") ;
    statsP->itsTxLockStats[Inc].Print();
    printf("\t%-15s ","Decrements:") ;
    statsP->itsTxLockStats[Dec].Print();
    printf("\t%-15s ","Reads:") ;
    statsP->itsTxLockStats[Read].Print();

    cout << endl;
  }


  UINT32 seqSize = g_seqP->size();
  cout <<
    "Totals:\n  Initial size: " << g_InitSeqSize <<
    " \n  Inserted: " << totalInserted <<
    " \n  Deleted: " << totalDeleted <<
    " \n  Final: " << seqSize << endl;

  // Since we only remove a counter from the array when its value is 0, and
  // since splitting a counter doesn't change the total sum of the
  // counters in the array, the sum of all counters should be equal to
  // the initial sum of all counters, plus the difference between the
  // total number of increments to the total number of decrements by
  // all threads:
  //
  int balance = totalInc - totalDec;
  SeqType &seq = *g_seqP;
  UINT64 countersSum = 0;
  for (UINT32 i = 0; i<seqSize; i++) {
    countersSum += seq[i].data.counter;
  }
  if (g_InitSeqSize*(g_counterRange/2) + balance != countersSum) {
    printf("\nError: Failed integrity check! "
           "CountersSum = %lld, Balance = %d\n", countersSum, balance);
  } else {
    printf("\nPassed integrity check!\n");
  }

  DEBUG_ASSERT(seqSize == g_InitSeqSize+totalInserted-totalDeleted);
}




int
main(int argc, char* argv[])
{
  // Check and parse arguments.
  //
  if (argc != 7) {
    fprintf(stderr,
            "Usage: %s <N-threads> <N-iters> \\\n"
            "          <init-vec-size> <c-range> \\\n"
            "          <suppress-vecStats-dump> <machine-config> \n\n"
            "       On each of <N-iters> iterations, each of a set of\n"
            "       <N-threads> threads randomly selects to increment,\n"
            "       decrement, or just read a random counter in a vector.\n\n"
            "       The vector is initialize with <init-size> counters of\n"
            "       value <c-range>/2. If the counter is updated to 0,\n"
            "       then it is removed from the vector; if it reaches the \n"
            "       value <c-range>, it is split into two counters of value\n"
            "       <c-range>/2 (which adds a new counter to the vector). \n\n"
            "       <machine-config> specifies which machine configuration\n"
            "                        to use for processor bindings:\n"
            "               0=default identity bindings up to 64 processors\n"
            "               1=Rock/Niagara\n"
            "               2=garbage\n"
            "               3=mde69003a\n"
            "               4=bobcat/pseudocat/sumocat\n"
            "               5=sumocatb\n"
            "               6=sarek-1p\n"
            "               7=sarek-2p\n"
            "               8=sarek-3p\n"
            "               9=sarek-4p\n"
            "               10=sarek-6p\n"
            "               11=sarek-8p\n"
            "               12=sarek-12p\n"
            "               13=sarek-16p\n"
            "               14=sarek-24p\n"
            "               15=sarek-32p\n"
            "               16=sarek-48p\n"
            "               17=sarek-64p\n\n"
            "       <suppress-vecStats-dump> directs the test to suppress\n"
            "               vector-statistics-dumping\n\n",
            argv[0]);
    return -1;
  }
  g_NThreads            = atoi(argv[1]);
  g_NIters              = atoi(argv[2]);
  g_InitSeqSize         = atoi(argv[3]);
  g_counterRange        = atoi(argv[4]);
  g_SuppressTableStats  = atoi(argv[5]);
  g_MachineConfig       = atoi(argv[6]);

  // Validate arguments (somewhat).
  //
  assert(g_NThreads > 0 && g_NThreads <= MAX_THREADS);
  assert(g_NIters >= 0);
  assert(g_InitSeqSize >= 0);
  assert(g_MachineConfig >= 0 && g_MachineConfig < FIRST_INVALID_MACHINE_CONFIG);

  // Make sure g_NThreads won't index out of range for the selected
  // id-procid map.
  //
  assert(g_NThreads <= IdProcidMaps[g_MachineConfig].map_size);

  //-----------------------------------------------------------
  // Build a table.
  //-----------------------------------------------------------
//   MemAllocsStatic::SetAllocPoolSize(g_InitSeqSize+g_TLMemSize);
//   MemAllocsStatic::SetNumOfThreas(g_NThreads);
//   MemAllocsStatic::ThreadInit(0);

  fprintf(stderr, "Building vector");

  g_seqP = new SeqType();
  g_seqP->reserve(2*g_InitSeqSize);

  SeqType& sharedSeq = *g_seqP;

  sharedSeq._M_fill_insert(sharedSeq.begin(),
                           g_InitSeqSize,
                           Counter(g_counterRange/2));

  UINT32 vecSize = sharedSeq.size();
  fprintf(stderr, "done. Vector size is %d\n",vecSize);
  DEBUG_ASSERT(vecSize == g_InitSeqSize);

  g_seqSize = vecSize;

  // Herald.
  //
  printf("%s g_Nthreads=%d g_NIters=%d g_InitSeqSize=%d g_counterRange=%d "
         "g_Machine_Config=%d\n",
         argv[0], g_NThreads, g_NIters, g_InitSeqSize, g_counterRange,
         g_MachineConfig);

  printf("\n\nVector test starting with vector of size: %d, "
         "and operations distribution of %d%% increments, %d%% decrements, "
         "and %d%% lookups, with counter range of 0 - %d\n\n",
         vecSize, IncPercent, DecPercent,
         100-IncPercent-DecPercent, g_counterRange-1);

  double noCollissionChance = 1;
  for (int i=1; i<g_NThreads; i++) {
    noCollissionChance *= (((double)g_InitSeqSize - i)/g_InitSeqSize);
  }
  printf("Collission change: %3.2f%% * %d%% = %3.2f%%\n\n",
         100*(1-noCollissionChance),
         IncPercent+DecPercent,
         (1-noCollissionChance)*(IncPercent+DecPercent));

  // Create and run threads.
  //
  pthread_attr_t thread_attr;
  pthread_attr_init(&thread_attr);
  pthread_attr_setschedpolicy(&thread_attr, SCHED_RR);

  struct sched_param schedparam;
  schedparam.sched_priority = 1;
  pthread_setschedparam(pthread_self(), SCHED_OTHER, &schedparam);

  g_Threads = new pthread_t[g_NThreads];
  assert(g_Threads != NULL);

  ThreadArgs* threadsArgs = new ThreadArgs[g_NThreads];
  assert(threadsArgs != NULL);

  // Spawn all threads (except 0).
  //
  for (int tid = 0; tid < g_NThreads; tid++) {
    DEBUG_PRINTF("master: creating thread %d...\n", tid);
    threadsArgs[tid].itsTid = tid;
    threadsArgs[tid].itsRandSeq = &g_RandSeqs[tid];
    rand_seq_init(threadsArgs[tid].itsRandSeq, RAND_SEQ_LEN);
    if (tid != 0) {
      pthread_create(&g_Threads[tid],
                     &thread_attr,
                     (threadBodyPtr)VectorThreadBody,
                     (void*)&threadsArgs[tid]);
      printf("spawned thread: %d\n", tid);
    }
  }

  // Run thread 0 directly.
  //
  VectorThreadBody(&threadsArgs[0]);

  // Wait for other threads to finish the interesting part of the test.
  //
  DEBUG_PRINTF("master: waiting for other threads to finish...\n");
  while (g_ThreadsFinished < g_NThreads) ;
  DEBUG_PRINTF("master: threads have finished.\n");

  // Find max start and end times.
  //
  hrtime_t LatestStart = 0, LatestEnd = 0;
  for (int i = 0; i < g_NThreads; i++) {
    // Note: subtraction/0-comparison avoids issues with clock wraparound.
    //
    if (i == 0 || g_StartTimes[i] - LatestStart > 0) {
      LatestStart = g_StartTimes[i];
    }
    if (i == 0 || g_EndTimes[i] - LatestEnd > 0) {
      LatestEnd = g_EndTimes[i];
    }
  }

  // Print start and end times
  //
  printf("\n"
         "    start(us)   end(us)\n");
  printf("    ---------   -------\n");
  for (int i = 0; i < g_NThreads; i++) {
    printf("%2d:%9lld %9lld\n",
           i,
           (g_StartTimes[i] - LatestStart) / 1000,
           (g_EndTimes[i] - LatestStart) / 1000);
  }
  printf("\ntotal running time: %10lldus\n\n",
         (LatestEnd - LatestStart) / 1000);

  // Wait for other threads to terminate.
  //
  DEBUG_PRINTF("master: waiting for threads to shut down...\n");
  for (int tid = 1; tid < g_NThreads; tid++) {
    pthread_join(g_Threads[tid], NULL);
  }
  DEBUG_PRINTF("master: all threads have shut down.\n");

  // Dump RedBlackTree stats for this thread.
  //
  if (!g_SuppressTableStats) {
    PrintStats();
  }

  // Footer.
  //
  printf("\nVector test finished\n\n");
  ruby_breakpoint();

  return 0;
}
