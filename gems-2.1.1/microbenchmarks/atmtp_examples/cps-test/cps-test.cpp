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
// RLS test.
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
#include "inst_test.h"

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

//#define ARRAY_SIZE 65536
#define ARRAY_SIZE 16384
static int big_array[ARRAY_SIZE];
//static int *big_array = new int[ARRAY_SIZE];

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
  MAGIC(100);
}

//
// Function for signalling simics that we're done
//
static void
ruby_dump_registers()
{
  //
  MAGIC(0x50000);
}


extern "C"
static int addNine(int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7, int arg8, int arg9){
  return arg1 + arg2 + arg3 + arg4 + arg5 + arg6 + arg7 + arg8 + arg9;
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

#define READ_BIG_ARRAY(id, newval)          \
  { int chunk = ARRAY_SIZE/16*(2*id + 1);   \
    int base = chunk + 20;                  \
    int tmp = 0;                            \
    for(int i=base; i<ARRAY_SIZE; ++i){     \
      tmp += big_array[i];                  \
    }                                       \
    *newval = tmp;                          \
  }                                         \

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


// -----------------------------------------------
// Test overflow
// -----------------------------------------------

static void test_overflow(ThreadArgs* myArgsP, int opsNum){
  int iterations = opsNum;
  int success = 0;
  int cps_inst = 0;
  int cps_overflow = 0;
  int cps_coherence = 0;
  int cps_st = 0;
  int cps_ld = 0;
  int cps_prec = 0;
  int value = 0;

  //printf("thread %d starting overflow-test.\n", myArgsP->itsTid);
  for (unsigned int iter=0; iter<iterations; iter++) {
    if (begin_transaction()) {
      WRITE_BIG_ARRAY(myArgsP->itsTid, iter);
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
        int chunk = ARRAY_SIZE/16;
        int base = chunk*(2*myArgsP->itsTid + 1) + 20;
        value += big_array[base];
      } else if (cps == 0x00000090){ // CPS_LD | CPS_PREC
        cps_ld++;
        int chunk = ARRAY_SIZE/16;
        int base = chunk*(2*myArgsP->itsTid + 1) + 20;
        value += big_array[base];
      } else if (cps == 0x00000010){
        cps_prec++;
        WRITE_BIG_ARRAY(myArgsP->itsTid, iter);
      } else {
        printf("%d - ovfl transaction %d failed, cps = 0x%x\n", myArgsP->itsTid, iter, cps);
      }

    }
  }
  printf("%d - overfow test: %d/%d coh: %d, ovfl: %d, st: %d, ld: %d, prec: %d, inst: %d\n",
         myArgsP->itsTid, success, iterations, cps_coherence, cps_overflow, cps_st, cps_ld,
         cps_prec, cps_inst);
  printf("dummy value: %d\n", value);
}


// -----------------------------------------------
// Test eviction
// -----------------------------------------------

static void test_eviction(ThreadArgs* myArgsP, int opsNum){
  int iterations = opsNum;
  int success = 0;
  int cps_inst = 0;
  int cps_overflow = 0;
  int cps_coherence = 0;
  int cps_st = 0;
  int cps_ld_tlb = 0;
  int cps_ld_ev = 0;
  int cps_prec = 0;
  int value = 0;

  //printf("thread %d starting evict-test.\n", myArgsP->itsTid);
  for (unsigned int iter=0; iter<iterations; iter++) {
    if (begin_transaction()) {
      READ_BIG_ARRAY(myArgsP->itsTid, &value);
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
        int chunk = ARRAY_SIZE/16;
        int base = chunk*(2*myArgsP->itsTid + 1) + 20;
        value += big_array[base];
      } else if (cps == 0x00000090){ // CPS_LD | CPS_PREC
        cps_ld_tlb++;
        int chunk = ARRAY_SIZE/16;
        int base = chunk*(2*myArgsP->itsTid + 1) + 20;
        value += big_array[base];
      } else if (cps == 0x00000080){ // CPS_LD
        cps_ld_ev++;
      } else if (cps == 0x00000010){
        cps_prec++;
        WRITE_BIG_ARRAY(myArgsP->itsTid, iter);
      } else {
        printf("%d - evict transaction %d failed, cps = 0x%x\n", myArgsP->itsTid, iter, cps);
      }

    }
  }
  printf("%d - eviction test: %d/%d coh: %d, ovfl: %d, st: %d, ld_tlb: %d, ld_ev: %d, prec: %d, inst: %d\n",
         myArgsP->itsTid, success, iterations, cps_coherence, cps_overflow, cps_st, cps_ld_tlb,
         cps_ld_ev, cps_prec, cps_inst);
  printf("dummy value: %d\n", value);
}

// -----------------------------------------------
// Test coherence
// -----------------------------------------------
static void test_coherence(ThreadArgs* myArgsP, int opsNum){
  int success = 0;
  int cps_overflow = 0;
  int cps_coherence = 0;
  int cps_st = 0;
  int cps_prec = 0;
  int cps_inst = 0;
  int iterations = opsNum * 5;
  //printf("thread %d starting coherence-test.\n", myArgsP->itsTid);
  for (unsigned int iter=0; iter<iterations; iter++) {
    if (begin_transaction()) {
      WRITE_SMALL_ARRAY(iter);
      commit_transaction();
      success++;
    } else {
      //... if transaction failed, you get here ...
      // read cps reg
      unsigned int cps = read_cps_reg();
      if(cps == 0x2){
        cps_coherence++;
      } else if (cps == 0x8){
        cps_inst++;
      } else if (cps == 0x40){
        cps_overflow++;
      } else if (cps == 0x110){
        cps_st++;
      } else if (cps == 0x10){
        cps_prec++;
      } else {
        printf("%d - coh trans %d failed, cps = 0x%x\n",
               myArgsP->itsTid, iter, cps);
      }
    }
  }
  printf("%d - coherence test: %d/%d coh: %d, ovfl: %d, inst: %d\n", myArgsP->itsTid,
         success, iterations, cps_coherence, cps_overflow, cps_inst);
}

// -----------------------------------------------
// Test tcc
// -----------------------------------------------

static void test_tcc(ThreadArgs* myArgsP, int opsNum){
  int success = 0;
  int cps_overflow = 0;
  int cps_coherence = 0;
  int cps_tcc = 0;
  int cps_prec = 0;
  int cps_inst = 0;
  int iterations = 5;
  //printf("thread %d starting coherence-test.\n", myArgsP->itsTid);
  if (myArgsP->itsTid == 0){
    for (unsigned int iter=0; iter<iterations; iter++) {
      if (begin_transaction()) {
        force_tcc();
        commit_transaction();
        success++;
        //printf("transaction %d success!\n", iter);
      } else {
        //... if transaction failed, you get here ...
        // read cps reg
        unsigned int cps = read_cps_reg();
        if(cps == 0x02){
          cps_coherence++;
        } else if (cps == 0x04){
          cps_tcc++;
        } else if (cps == 0x40){
          cps_overflow++;
        } else if (cps == 0x08){ // CPS_INST
          cps_inst++;
        } else if (cps == 0x10){
          cps_prec++;
        } else {
          printf("%d - tcc transaction %d failed, cps = 0x%x\n",
                 myArgsP->itsTid, iter, cps);
        }
      }
    }
    printf("%d - tcc test: %d/%d coh: %d, ovfl: %d, prec: %d, tcc: %d, inst: %d\n",
           myArgsP->itsTid, success, iterations, cps_coherence, cps_overflow,
           cps_prec, cps_tcc, cps_inst);
  }
}

// -----------------------------------------------
// Test fail transaction
// -----------------------------------------------

static void test_fail(ThreadArgs* myArgsP, int opsNum){
  int success = 0;
  int cps_overflow = 0;
  int cps_coherence = 0;
  int cps_tcc = 0;
  int cps_prec = 0;
  int cps_inst = 0;
  int iterations = 5;
  //printf("thread %d starting coherence-test.\n", myArgsP->itsTid);
  if (myArgsP->itsTid == 0){
    for (unsigned int iter=0; iter<iterations; iter++) {
      if (begin_transaction()) {
        abort_transaction();
        commit_transaction();
        success++;
        //printf("transaction %d success!\n", iter);
      } else {
        //... if transaction failed, you get here ...
        // read cps reg
        unsigned int cps = read_cps_reg();
        if(cps == 0x02){
          cps_coherence++;
        } else if (cps == 0x04){
          cps_tcc++;
        } else if (cps == 0x40){
          cps_overflow++;
        } else if (cps == 0x08){ // CPS_INST
          cps_inst++;
        } else if (cps == 0x10){
          cps_prec++;
        } else {
          printf("%d - fail-test transaction %d failed, cps = 0x%x\n",
                 myArgsP->itsTid, iter, cps);
        }
      }
    }
    printf("%d - fail test: %d/%d coh: %d, ovfl: %d, prec: %d, tcc: %d, inst: %d\n",
           myArgsP->itsTid, success, iterations, cps_coherence, cps_overflow,
           cps_prec, cps_tcc, cps_inst);
  }
}


// -----------------------------------------------
// Test wrpr
// -----------------------------------------------

static void test_wrpr(ThreadArgs* myArgsP, int opsNum){
  int success = 0;
  int cps_overflow = 0;
  int cps_coherence = 0;
  int cps_inst = 0;
  int iterations = 5;
  //printf("thread %d starting coherence-test.\n", myArgsP->itsTid);
  if (myArgsP->itsTid == 0){
    for (unsigned int iter=0; iter<iterations; iter++) {
      if (begin_transaction()) {
        wrpr();
        commit_transaction();
        success++;
        //printf("transaction %d success!\n", iter);
      } else {
        //... if transaction failed, you get here ...
        // read cps reg
        unsigned int cps = read_cps_reg();
        if(cps == 0x2){
          cps_coherence++;
        } else if (cps == 0x40){
          cps_overflow++;
        } else if (cps == 0x08){ // CPS_INST
          cps_inst++;
        } else {
          printf("transaction %d failed, cps = 0x%x\n", iter, cps);
        }
      }
    }
    printf("%d - wrpr test: %d/%d coh: %d, ovfl: %d, inst: %d\n", myArgsP->itsTid,
           success, iterations, cps_coherence, cps_overflow, cps_inst);
  }
}

// -----------------------------------------------
// Test done
// -----------------------------------------------

static void test_done(ThreadArgs* myArgsP, int opsNum){
  int success = 0;
  int cps_overflow = 0;
  int cps_coherence = 0;
  int cps_inst = 0;
  int cps_st = 0;
  int cps_prec = 0;

  if (myArgsP->itsTid == 0){
    if (begin_transaction()) {
      done();
      commit_transaction();
      success++;
    } else {
      //... if transaction failed, you get here ...
      // read cps reg
      unsigned int cps = read_cps_reg();
      if(cps == 0x2){
        cps_coherence++;
      } else if (cps == 0x40){
        cps_overflow++;
      } else if (cps == 0x08){ // CPS_INST
        cps_inst++;
      } else if (cps == 0x100){
        cps_st++;
      } else if (cps == 0x10){
        cps_prec++;
      } else {
        printf("done transaction failed, cps = 0x%x\n", cps);
      }
    }

    printf("%d - done test: %d/1 coh: %d, ovfl: %d, inst: %d\n", myArgsP->itsTid,
           success, cps_coherence, cps_overflow, cps_inst);
  }
}

// -----------------------------------------------
// Test retry
// -----------------------------------------------

static void test_retry(ThreadArgs* myArgsP, int opsNum){
  int success = 0;
  int cps_overflow = 0;
  int cps_coherence = 0;
  int cps_inst = 0;
  int cps_st = 0;
  int cps_prec = 0;

  if (myArgsP->itsTid == 0){
    if (begin_transaction()) {
      retry();
      commit_transaction();
      success++;
    } else {
      //... if transaction failed, you get here ...
      // read cps reg
      unsigned int cps = read_cps_reg();
      if(cps == 0x2){
        cps_coherence++;
      } else if (cps == 0x40){
        cps_overflow++;
      } else if (cps == 0x08){ // CPS_INST
        cps_inst++;
      } else if (cps == 0x100){
        cps_st++;
      } else if (cps == 0x10){
        cps_prec++;
      } else {
        printf("retry transaction failed, cps = 0x%x\n", cps);
      }
    }

    printf("%d - retry test: %d/1 coh: %d, ovfl: %d, inst: %d, prec: %d\n", myArgsP->itsTid,
           success, cps_coherence, cps_overflow, cps_inst, cps_prec);
  }
}

// -----------------------------------------------
// Test flush
// -----------------------------------------------

static void test_flush(ThreadArgs* myArgsP, int opsNum){
  int success = 0;
  int cps_overflow = 0;
  int cps_coherence = 0;
  int cps_inst = 0;
  int cps_st = 0;
  int cps_prec = 0;

  if (myArgsP->itsTid == 0){
    if (begin_transaction()) {
      flush();
      commit_transaction();
      success++;
    } else {
      //... if transaction failed, you get here ...
      // read cps reg
      unsigned int cps = read_cps_reg();
      if(cps == 0x2){
        cps_coherence++;
      } else if (cps == 0x40){
        cps_overflow++;
      } else if (cps == 0x08){ // CPS_INST
        cps_inst++;
      } else if (cps == 0x100){
        cps_st++;
      } else if (cps == 0x10){
        cps_prec++;
      } else {
        printf("flush transaction failed, cps = 0x%x\n", cps);
      }
    }

    printf("%d - flush test: %d/1 coh: %d, ovfl: %d, inst: %d\n", myArgsP->itsTid,
           success, cps_coherence, cps_overflow, cps_inst);
  }
}

// -----------------------------------------------
// Test nesting
// -----------------------------------------------

static void test_nesting(ThreadArgs* myArgsP, int opsNum){
  int success = 0;
  int cps_overflow = 0;
  int cps_coherence = 0;
  int cps_inst = 0;
  int cps_st = 0;
  int cps_prec = 0;

  if (myArgsP->itsTid == 0){
    printf("%d - starting nesting test.\n", myArgsP->itsTid);
    if (begin_transaction()) {
      begin_transaction();
      commit_transaction();
      commit_transaction(); // two commits for nesting
      success++;
    } else {
      //... if transaction failed, you get here ...
      // read cps reg
      unsigned int cps = read_cps_reg();
      if(cps == 0x2){
        cps_coherence++;
      } else if (cps == 0x40){
        cps_overflow++;
      } else if (cps == 0x08){ // CPS_INST
        cps_inst++;
      } else if (cps == 0x100){
        cps_st++;
      } else if (cps == 0x10){
        cps_prec++;
      } else {
        printf("nested transaction failed, cps = 0x%x\n", cps);
      }
    }

    printf("%d - nesting test: %d/1 coh: %d, ovfl: %d, inst: %d\n", myArgsP->itsTid,
           success, cps_coherence, cps_overflow, cps_inst);
  }
}

// -----------------------------------------------
// Test spurious commit
// -----------------------------------------------

static void test_spurious_commit(ThreadArgs* myArgsP, int opsNum){
  int success = 0;
  int cps_overflow = 0;
  int cps_coherence = 0;
  int cps_inst = 0;
  int cps_st = 0;
  int cps_prec = 0;

  if (myArgsP->itsTid == 0){
    printf("%d - starting extra commit test\n", myArgsP->itsTid);
    commit_transaction();
    printf("%d - finished extra commit test\n", myArgsP->itsTid);
  }
}

// -----------------------------------------------
// Test save/restore
// -----------------------------------------------

static void test_save_restore(ThreadArgs* myArgsP, int opsNum){
  int success = 0;
  int cps_overflow = 0;
  int cps_coherence = 0;
  int cps_inst = 0;
  int cps_st = 0;
  int cps_prec = 0;
  int dummy = 17;

  if (myArgsP->itsTid == 0){
    if (begin_transaction()) {
      save_restore();
      commit_transaction();
      success++;
    } else {
      //... if transaction failed, you get here ...
      // read cps reg
      unsigned int cps = read_cps_reg();
      if(cps == 0x2){
        cps_coherence++;
      } else if (cps == 0x40){
        cps_overflow++;
      } else if (cps == 0x08){ // CPS_INST
        cps_inst++;
      } else if (cps == 0x100){
        cps_st++;
      } else if (cps == 0x10){
        cps_prec++;
      } else {
        printf("save/restore transaction failed, cps = 0x%x\n", cps);
      }
    }

    printf("%d - save/restore test: %d/1 coh: %d, ovfl: %d, inst: %d, dummy: %d\n", myArgsP->itsTid,
           success, cps_coherence, cps_overflow, cps_inst, dummy);
  }
}

// -----------------------------------------------
// Test dump registers
// -----------------------------------------------

static void test_dump_registers(ThreadArgs* myArgsP, int opsNum){
  if(myArgsP->itsTid == 0){
    ruby_dump_registers();
  }
}

// -----------------------------------------------
// Test async interrupt
// -----------------------------------------------

static void test_async_int(ThreadArgs* myArgsP, int opsNum){
  int success = 0;

  fprintf(stderr, "%d - starting async test...\n", myArgsP->itsTid);
  if (begin_transaction()) {
    while(1);
    commit_transaction();
    success++;
  } else {
    //... if transaction failed, you get here ...
    // read cps reg
    unsigned int cps = read_cps_reg();

    fprintf(stderr, "%d - async transaction failed, cps = 0x%x\n", myArgsP->itsTid, cps);
  }

  if(success > 0){
    fprintf(stderr, "%d - Error!  Async Int Transaction succeeded!\n", myArgsP->itsTid);
  }
}


static void
RLSTestThreadBody(void* theArgsP)
{
  ThreadArgs* myArgsP = (ThreadArgs*)theArgsP;

  DEBUG_PRINTF("%d:  starting test...\n", myArgsP->itsTid);

  if(0 > myArgsP->itsTid){
    printf("%d Error: thread id less than 0\n", myArgsP->itsTid);
  }

  if(myArgsP->itsTid >= g_NThreads){
    printf("%d Error: thread id more than g_NThreads\n", myArgsP->itsTid);
  }

  // Bind this thread to a processor, using the specified processor
  // binding map.
  //
  processorid_t destination =
    IdProcidMaps[g_MachineConfig].map[myArgsP->itsTid];
  DEBUG_PRINTF("thread %d: binding to processor %d (running on %d)\n",
         myArgsP->itsTid, destination, getcpuid());
  if (processor_bind(P_LWPID, P_MYID, destination, NULL) != 0) {
    fprintf(stderr, "processor_bind(%d) failed for thread %d: ",
            destination, myArgsP->itsTid);
    perror("");
  } else {
    DEBUG_PRINTF("%d:  bound to processor %d.\n",
           myArgsP->itsTid, getcpuid());
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
  int tmp = 0;
  READ_BIG_ARRAY(myArgsP->itsTid, &tmp);
  test_overflow(myArgsP, opsNum);
  test_eviction(myArgsP, opsNum);
  test_coherence(myArgsP, opsNum);
  test_fail(myArgsP, opsNum);
  test_tcc(myArgsP, opsNum);
  test_wrpr(myArgsP, opsNum);
  test_done(myArgsP, opsNum);
  test_retry(myArgsP, opsNum);
  test_flush(myArgsP, opsNum);
  test_nesting(myArgsP, opsNum);
  test_spurious_commit(myArgsP, opsNum);
  test_save_restore(myArgsP, opsNum);
  test_dump_registers(myArgsP, opsNum);
  //test_async_int(myArgsP, opsNum);

  // Record the end time for this thread.
  //
  g_EndTimes[myArgsP->itsTid] = gethrtime();

  // Use CAS to increment g_ThreadsFinished counter.
  //
  while (true) {
    int read_ThreadsFinished = g_ThreadsFinished;
    // If I am the last thread to finish, break so that we can take
    // ruby statistics
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
  if (argc != 4) {
    fprintf(stderr,
            "Usage: %s <N-threads> <N-iters> <machine-config>\n"
            "       On each of <N-iters> iterations, increment\n"
            "       a single global CAS counter.  When finished,\n"
            "       test that the counter has the expected value.\n\n"
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
            "               17=sarek-64p\n\n",
            argv[0]);
    return -1;
  }
  g_NThreads          = atoi(argv[1]);
  g_NIters            = atoi(argv[2]);
  g_MachineConfig     = atoi(argv[3]);

  // Validate arguments (somewhat).
  //
  assert(g_NThreads > 0 && g_NThreads <= MAX_THREADS);
  assert(g_NIters >= 0);
  assert(g_MachineConfig >= 0 && g_MachineConfig < FIRST_INVALID_MACHINE_CONFIG);

  // Make sure g_NThreads won't index out of range for the selected
  // id-procid map.
  //
  assert(g_NThreads <= IdProcidMaps[g_MachineConfig].map_size);

  // Herald.
  //
  DEBUG_PRINTF("%s g_Nthreads=%d g_NIters=%d g_Machine_Config=%d\n",
         argv[0], g_NThreads, g_NIters, g_MachineConfig);

  // Create and run threads.
  //
  pthread_attr_t thread_attr;
  pthread_attr_init(&thread_attr);

  g_Threads = new pthread_t[g_NThreads];
  assert(g_Threads != NULL);

  ThreadArgs* threadsArgs = new ThreadArgs[g_NThreads];
  assert(threadsArgs != NULL);

  // Spawn all threads (except 0).
  //
  for (int tid = 0; tid < g_NThreads; tid++) {
    DEBUG_PRINTF("master: creating thread %d...\n", tid);
    threadsArgs[tid].itsTid = tid;
    if (tid != 0) {
      pthread_create(&g_Threads[tid],
                     &thread_attr,
                     (threadBodyPtr)RLSTestThreadBody,
                     (void*)&threadsArgs[tid]);
    }
  }

  // Run thread 0 directly.
  //
  RLSTestThreadBody(&threadsArgs[0]);

  // Wait for other threads to finish the interesting part of the test.
  //
  DEBUG_PRINTF("master: waiting for other threads to finish...\n");
  while (g_ThreadsFinished < g_NThreads) ;
  DEBUG_PRINTF("master: threads have finished.\n");

  ruby_breakpoint();

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
  DEBUG_PRINTF("\nRLS test finished\n\n");

  return 0;
}
