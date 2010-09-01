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
// RockTM test.
//

// We communicate with Ruby/LogTM via MAGIC(n) instructions.
//
#include "magic-instruction.h"

#include <iostream.h>
#include <sys/processor.h>
#include <pthread.h>
#include <assert.h>
#include "sync.h"
#include "rock_tm.h"

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


//
// For communicating arguments to threads.
//
struct ThreadArgs
{
  int itsTid;
};


///////////////////////////////////////
//
// Globals.
//

#pragma align CACHE_LINE_SIZE (g_Threads)
static pthread_t* g_Threads;
static int g_NThreads;
static int g_NIters;
static int g_MachineConfig;

#pragma align CACHE_LINE_SIZE (g_Counter1)
int g_Counter1;

#pragma align CACHE_LINE_SIZE (g_Counter2)
int g_Counter2;

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

#define ARRAY_SIZE 4096
static int big_array[ARRAY_SIZE];

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

extern "C"
static int addThree(int arg){
  return 3 + arg;
}

//
// comparison function for qsort.
//
extern "C"
static int
compare_ints(const void* l, const void* r)
{
  return (*(int*)l - *(int*)r);
}

//
// The thread body.  Synchronize threads upon entry, and then once per
// iteration.
//
extern "C"
{
  typedef void* (*threadBodyPtr) (void*);
}


#define WRITE_BIG_ARRAY(id, newval)         \
  { int chunk = ARRAY_SIZE/16*(2*id + 1);   \
    int base = chunk + 20;                  \
    for(int i=base; i<base+34; ++i){        \
      big_array[i] += newval;               \
    }                                       \
  }                                         \

#define WRITE_SMALL_ARRAY(newval)               \
  { int i;                                      \
    for(i=0; i<10; ++i){                        \
      big_array[i] = newval;                    \
    }                                           \
  }

static void
RockTMTestThreadBody(void* theArgsP)
{
  ThreadArgs* myArgsP = (ThreadArgs*)theArgsP;

  DEBUG_PRINTF("%d:  starting test...\n", myArgsP->itsTid);

  if(0 > myArgsP->itsTid){
    printf("%d Error: thread id less than 0\n", myArgsP->itsTid);
  }

  if(myArgsP->itsTid >= g_NThreads){
    printf("%d Error: thread id more than g_NThreads\n", myArgsP->itsTid);
  }

  // Initialize local structures.
  //

  unsigned int opsNum = g_NIters;

  /////////////////////////////////////////////////////////////////
  //
  // First barrier.  Once we get past this barrier, ruby has been
  // installed.
  //

  // Use CAS to increment g_ThreadsAlive counter.
  //
  while (true) {
    printf("%d trying to increment threads alive.\n", myArgsP->itsTid);
    int read_ThreadsAlive = g_ThreadsAlive;
    // If I am about to start everything going, break to allow for
    // turning on the accurate timing model.
    //
    if (read_ThreadsAlive + 1 == g_NThreads) {
      DEBUG_PRINTF("%d:  about to trigger breakpoint...\n",
                   myArgsP->itsTid);
      //printf("starting ruby\n");
      ruby_breakpoint();
    }
    if (CAS32(&g_ThreadsAlive, read_ThreadsAlive, read_ThreadsAlive + 1)) {
      break;
    }
  }
  DEBUG_PRINTF("%d:  successfully incremented g_ThreadsAlive...\n",
               myArgsP->itsTid);
  // Wait for all threads to show up.
  //
  if (g_ThreadsAlive < g_NThreads) {
    DEBUG_PRINTF("%d:  waiting for others...\n", myArgsP->itsTid);
    while (g_ThreadsAlive < g_NThreads) {
      ;//printf("%d waiting on threads alive %d/%d.\n", myArgsP->itsTid, g_ThreadsAlive, g_NThreads);
    }
  }
  //DEBUG_ASSERT(g_ThreadsAlive == g_NThreads);
  if(g_ThreadsAlive != g_NThreads){
    printf("%d error: %d/%d\n", myArgsP->itsTid, g_ThreadsAlive, g_NThreads);
  } else {
    printf("%d past barrier 1 %d/%d\n", myArgsP->itsTid, g_ThreadsAlive, g_NThreads);
  }
  //
  // Once we get past this point, ruby has been installed.


  // Record the start time for this thread.
  //
  g_StartTimes[myArgsP->itsTid] = gethrtime();


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
                   myArgsP->itsTid);
      ruby_finish_unit();
    }
    if (CAS32(&g_ThreadsReady, read_ThreadsReady, read_ThreadsReady + 1)) {
      break;
    }
  }
  DEBUG_PRINTF("%d:  successfully incremented g_ThreadsReady to %d...\n",
         myArgsP->itsTid, g_ThreadsReady);
  // Wait for all threads to show up.
  //
  if (g_ThreadsReady < g_NThreads) {
    DEBUG_PRINTF("%d:  waiting for others...\n", myArgsP->itsTid);
    while (g_ThreadsReady < g_NThreads){
      ;//printf("%d waiting on threads ready %d/%d.\n", myArgsP->itsTid, g_ThreadsReady, g_NThreads);
    };
  }
  //DEBUG_ASSERT(g_ThreadsReady == g_NThreads);
  //
  // Once we get past this point, everybody can start the test.

  DEBUG_PRINTF("%d:  running test...\n", myArgsP->itsTid);

  //
  // OK -- everybody is here now.  The test can start.
  //

  // -----------------------------------------------
  // Test Rock Transactions
  // -----------------------------------------------
  int cps_overflow = 0;
  int cps_coherence = 0;
  int cps_st = 0;
  int cps_ld = 0;
  int cps_prec = 0;
  int cps_inst = 0;
  int iterations = opsNum;
  int value = 0;
  int success = 0;
  printf("thread %d starting rock-tm-test.\n", myArgsP->itsTid);

  for (unsigned int iter=0; iter<iterations; iter++) {
    int index = iter + myArgsP->itsTid % g_NThreads;
    if(index >= ARRAY_SIZE){
      index = index % ARRAY_SIZE;
    }

    if (begin_transaction()) {
      big_array[index] = big_array[index] + 1;
      commit_transaction();
      success++;
    } else {
      //... if transaction failed, you get here ...
      // read cps reg
      unsigned int cps = read_cps_reg();
      if(cps == 0x2){
        cps_coherence++;
      } else if (cps == 0x00000008){ // CPS_INST
        cps_inst++;
      } else if (cps == 0x40){
        cps_overflow++;
      } else if (cps == 0x00000110){ // CPS_ST | CPS_PREC
        cps_st++;
        cps_prec++;
        int chunk = ARRAY_SIZE/16;
        int base = chunk*(2*myArgsP->itsTid + 1) + 20;
        value += big_array[base];
      } else if (cps == 0x00000090){ // CPS_LD | CPS_PREC
        cps_ld++;
        cps_prec++;
        int chunk = ARRAY_SIZE/16;
        int base = chunk*(2*myArgsP->itsTid + 1) + 20;
        value += big_array[base];
      } else if (cps == 0x00000010){
        cps_prec++;
      } else {
        printf("thread %d - failed transaction %d, cps=%x\n", myArgsP->itsTid, iter, cps);
      }
      // warm up tlbs
      for(int i=0; i<ARRAY_SIZE; i++){
        big_array[i] = myArgsP->itsTid;
      }
    }
  }

  printf("thread %d: (%d/%d) ovfl[%d], coh[%d], st[%d], prec[%d], inst[%d], ld[%d], st[%d]\n",
         myArgsP->itsTid, success, iterations, cps_overflow, cps_coherence,
         cps_st, cps_prec, cps_inst, cps_ld, cps_st);

  // -----------------------------------------------
  // ------------       END TESTS      -------------
  // -----------------------------------------------


  // Record the end time for this thread.
  //
  g_EndTimes[myArgsP->itsTid] = gethrtime();

  // Use CAS to increment g_ThreadsFinished counter.
  //
  while (true) {
    int read_ThreadsFinished = g_ThreadsFinished;
    // If I am the last thread to finish, break so that we can take
    // ruby statistics before dumping out our HyTM statistics.
    //
    if (read_ThreadsFinished+1 == g_NThreads) {
      DEBUG_PRINTF("%d:  last thread incrementing g_ThreadsFinished\n",
                   myArgsP->itsTid);
      ruby_finish_unit();
    }
    if (CAS32(&g_ThreadsFinished,
              read_ThreadsFinished,
              read_ThreadsFinished + 1)) {
      break;
    }
  }
  DEBUG_PRINTF("%d:  successfully incremented g_ThreadsFinished...\n",
               myArgsP->itsTid);

  // Wait for other threads to finish the interesting part of the
  // test, so that no thread starts dumping stats until all threads
  // have finished the operative part of the test.
  //
  DEBUG_PRINTF("%d: waiting for other threads to finish...\n", myArgsP->itsTid);
  while (g_ThreadsFinished < g_NThreads) ;
  DEBUG_PRINTF("%d: other threads have finished.\n", myArgsP->itsTid);

}


int
main(int argc, char* argv[])
{
  // Check and parse arguments.
  //
  if (argc != 3) {
    fprintf(stderr,
            "Usage: %s <N-threads> <N-iters> <machine-config>\n",
            argv[0]);
    return -1;
  }
  g_NThreads          = atoi(argv[1]);
  g_NIters            = atoi(argv[2]);

#ifdef STATS
  HyTMCompilerIf__DoDumpStats = 0;
#endif // STATS

  // Validate arguments (somewhat).
  //
  assert(g_NThreads > 0 && g_NThreads <= MAX_THREADS);
  assert(g_NIters >= 0);

  DEBUG_PRINTF("%s g_Nthreads=%d g_NIters=%d\n",
         argv[0], g_NThreads, g_NIters);

  // Create and run threads.
  //
  pthread_attr_t thread_attr;
  pthread_attr_init(&thread_attr);

  g_Threads = new pthread_t[g_NThreads];
  assert(g_Threads != NULL);

  ThreadArgs* threadsArgs = new ThreadArgs[g_NThreads];
  assert(threadsArgs != NULL);

  for(int i=0; i<ARRAY_SIZE; ++i){
    big_array[i] = 0;
  }

  // Spawn all threads (except 0).
  //
  for (int tid = 0; tid < g_NThreads; tid++) {
    DEBUG_PRINTF("master: creating thread %d...\n", tid);
    threadsArgs[tid].itsTid = tid;
    if (tid != 0) {
      pthread_create(&g_Threads[tid],
                     &thread_attr,
                     (threadBodyPtr)RockTMTestThreadBody,
                     (void*)&threadsArgs[tid]);
    }
  }

  // Run thread 0 directly.
  //
  RockTMTestThreadBody(&threadsArgs[0]);

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
  DEBUG_PRINTF("\n"
         "    start(us)   end(us)\n");
  DEBUG_PRINTF("    ---------   -------\n");
  for (int i = 0; i < g_NThreads; i++) {
    DEBUG_PRINTF("%2d:%9lld %9lld\n",
           i,
           (g_StartTimes[i] - LatestStart) / 1000,
           (g_EndTimes[i] - LatestStart) / 1000);
  }
  DEBUG_PRINTF("\ntotal running time: %10lldus\n\n",
         (LatestEnd - LatestStart) / 1000);

  // Wait for other threads to terminate.
  //
  DEBUG_PRINTF("master: waiting for threads to shut down...\n");
  for (int tid = 1; tid < g_NThreads; tid++) {
    pthread_join(g_Threads[tid], NULL);
  }
  DEBUG_PRINTF("master: all threads have shut down.\n");

  // Footer.
  //
  printf("\nRockTM test finished\n\n");
  ruby_breakpoint();
  return 0;
}
