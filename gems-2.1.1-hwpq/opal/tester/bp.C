
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
/*
 * FileName:  bp.C
 * Synopsis:  Branch Predictor Executable
 * Author:    cmauer
 * Version:   $Id$
 */

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include "hfa.h"
#include "hfacore.h"
#include "ipagemap.h"
#include "yags.h"
#include "gshare.h"
#include "igshare.h"
#include "agree.h"
#include "fatpredict.h"
#include "mlpredict.h"
#include "confio.h"
#include "bitdist.h"
#include "branchfile.h"
extern "C" {
#include "hfa_init.h"
}

#include <stdio.h>
#include <sys/types.h>
#include <signal.h>
#define Address uint64_t
#include "statici.h"

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Variable declarations                                                  */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Forward declarations                                                   */
/*------------------------------------------------------------------------*/

extern void simstate_init( void );
extern void simstate_fini( void );

/*------------------------------------------------------------------------*/
/* Constructor(s) / destructor                                            */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Public methods                                                         */
/*------------------------------------------------------------------------*/

/// count of the static branchs in the program
static uint64      m_static_count = 0;

/// count of the dynamic branchs in the program
static uint64      m_dynamic_count = 0;

/// count of the mispredicted branchs
static uint64      m_mispredict_count = 0;

/// count of the mispredicted branchs
static uint64      m_static_mispredict_count = 0;

/// count of the interrupted branches
static uint64      m_interrupt_count = 0;

/// count of static branches agreed
static uint64      m_static_agree = 0;

/// count of static branches ignored
static uint64      m_static_ignore = 0;
/// count of dynamic branches ignored
static uint64      m_dynamic_ignore = 0;

/// count of static branches confident
static uint64      m_static_confident = 0;
/// count of dynamic branches confident
static uint64      m_dynamic_confident = 0;

/// count of static branches frequent
static uint64      m_static_frequent = 0;
/// count of dynamic branches frequent
static uint64      m_dynamic_frequent = 0;

/// count of positive interference
static uint64      m_positive_inf = 0;

/// count of negative interference
static uint64      m_negative_inf = 0;

/// count of the mispredicted branches by inf predictor
static uint64      m_mispredict_count_inf = 0;

/// count of the number of context switches
static uint64      m_context_switches = 0;

/// largest size of ignore list
static uint32      m_max_ignore = 100;

/// largest size of confidence list
static uint32      m_max_confident = 25000;

/// largest size of frequent list
static uint32      m_max_frequent = 32;

/**
 * @name Cache of static (decoded) instructions
 */
//@{
/// current instruction map
ipagemap_t   *m_imap;
//@}
uint32        m_primary_ctx;

uint32        m_ignore_count = 0;
my_addr_t    *m_ignore_array = NULL;

uint32        m_confident_count = 0;
my_addr_t    *m_confident_array = NULL;

uint32        m_frequent_count = 0;
my_addr_t    *m_frequent_array = NULL;

bitdist_t    *m_bitdist = new bitdist_t( 4 );
struct timeval m_start_time;

cond_state_t    m_history;
branchfile_t   *m_branchfp;
branch_record_t m_branch_record;

direct_predictor_t  *m_predict;
fatpredict_t        *m_predict_freq;
igshare_t           *m_predict_inf;
confio_t             m_conf;

/*------------------------------------------------------------------------*/
/* Accessor(s) / mutator(s)                                               */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Private methods                                                        */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Static methods                                                         */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Global functions                                                       */
/*------------------------------------------------------------------------*/


//**************************************************************************
bool staticPredictCallback( static_inst_t *s )
{
  int    predict;
  uint32 inst = s->getInst();

  if ( s->getBranchType() == BRANCH_PCOND ) {
    predict = maskBits32( inst, 19, 19 );
  } else {
    return false;
  }

  // check to see if static predictions are more correct...
  if ( predict ) {
    if ( s->m_count_taken < (s->m_count_execute / 2) ) {
      DEBUG_OUT("0x%012llx  0x%08x %2d %8lld %8lld %8d %8d %8d\n",
                getVPC(), inst, predict,
                s->m_count_execute,
                s->m_count_taken,
                s->m_count_mispredict,
                s->m_count_static_mispredict,
                s->m_count_mispredict_inf );
    }
  } else {
    if ( s->m_count_taken > (s->m_count_execute / 2) ) {
      DEBUG_OUT("0x%012llx  0x%08x %2d %8lld %8lld %8d %8d %8d\n",
                getVPC(), inst, predict,
                s->m_count_execute,
                s->m_count_taken,
                s->m_count_mispredict,
                s->m_count_static_mispredict,
                s->m_count_mispredict_inf );
    }
  }
  return false;
}

//**************************************************************************
bool staticCallback( static_inst_t *s )
{
  int    predict;
  uint32 inst = s->getInst();

  if ( s->getBranchType() == BRANCH_PCOND ) {
    predict = maskBits32( inst, 19, 19 );
  } else {
    predict = -1;
  }
  
  double pct_of_total = ((double) s->m_count_mispredict /
                         (double) m_mispredict_count) * 100;
  double pct_of_static = ((double) s->m_count_static_mispredict /
                          (double) m_static_mispredict_count) * 100;
  if ( pct_of_total < 0.01 ) {
    pct_of_total = 0.0;
  }
  if ( pct_of_static < 0.01 ) {
    pct_of_static = 0.0;
  }

  //     pstate_t::branch_type_menomic[ s->getBranchType() ] + 7,
  // %4.1f %4.1f
  //     pct_of_total,
  //     pct_of_static,
  DEBUG_OUT("0x%08llx  0x%08x %2d %8lld %8lld %8d %8d %8d\n",
            getVPC(), inst, predict,
            //pct
            s->m_count_execute,
            s->m_count_taken,
            s->m_count_mispredict,
            s->m_count_static_mispredict,
            s->m_count_mispredict_inf );
  m_static_count++;
  return (false);
}

//**************************************************************************
void printStats( void )
{
  struct timeval now;

#if 1
  // print out statistics related to this branch predictor
  // %%dyn %%stat
  DEBUG_OUT("VPC         INSTR     TYPE   EXECUTE    TAKEN   MISPRED  STATICMIS INFMIS\n");
  m_imap->walkMap( staticCallback );
#endif

  DEBUG_OUT("\n");
  DEBUG_OUT("Infinite Predictor:\n");
  m_predict_inf->printInformation( stdout );
  DEBUG_OUT("\n");
  DEBUG_OUT("Real Predictor:\n");
  m_predict->printInformation( stdout );
  DEBUG_OUT("\n");
  if (m_predict_freq) {
    DEBUG_OUT("Frequent Predictor:\n");
    m_predict_freq->printInformation( stdout );
    DEBUG_OUT("\n");
  }
  DEBUG_OUT("static count of branches         : %lld\n", m_static_count);
  DEBUG_OUT("dynamic count of branches        : %lld\n", m_dynamic_count);
  DEBUG_OUT("number of branches interruped    : %lld\n", m_interrupt_count );
  DEBUG_OUT("static ignored branches          : %lld\n", m_static_ignore);
  DEBUG_OUT("dynamic ignored branches         : %lld\n", m_dynamic_ignore);
  DEBUG_OUT("count of context switches        : %lld\n", m_context_switches);
  DEBUG_OUT("\n");
  DEBUG_OUT("static confident branches        : %lld\n", m_static_confident);
  DEBUG_OUT("dynamic confident branches       : %lld\n", m_dynamic_confident);
  DEBUG_OUT("\n");
  DEBUG_OUT("static frequent branches        : %lld\n", m_static_frequent);
  DEBUG_OUT("dynamic frequent branches       : %lld\n", m_dynamic_frequent);
  DEBUG_OUT("\n");
  DEBUG_OUT("number of static agreed          : %lld\n", m_static_agree);
  DEBUG_OUT("agree percentage                 : %g%%\n",
            ((double) (m_static_agree) /
             (double) m_dynamic_count) * 100.0);
  DEBUG_OUT("\n");
  DEBUG_OUT("branch predictor correct         : %lld\n",
            (m_dynamic_count - m_mispredict_count));
  DEBUG_OUT("branch predictor mispredict      : %lld\n",
            m_mispredict_count);
  DEBUG_OUT("branch predictor accuracy        : %g%%\n",
            ((double) (m_dynamic_count - m_mispredict_count) /
             (double) m_dynamic_count) * 100.0);
  DEBUG_OUT("\n");
  DEBUG_OUT("inf predictor predictor correct  : %lld\n",
            (m_dynamic_count - m_mispredict_count_inf));
  DEBUG_OUT("inf predictor mispredict         : %lld\n",
            m_mispredict_count_inf);
  DEBUG_OUT("inf predictor accuracy           : %g%%\n",
            ((double) (m_dynamic_count - m_mispredict_count_inf) /
             (double) m_dynamic_count) * 100.0);
  DEBUG_OUT("\n");
  DEBUG_OUT("static predictor correct         : %lld\n",
            (m_dynamic_count - m_static_mispredict_count));
  DEBUG_OUT("static predictor mispredict      : %lld\n",
            m_static_mispredict_count);
  DEBUG_OUT("static predictor static accuracy : %g%%\n",
            ((double) (m_dynamic_count - m_static_mispredict_count) /
             (double) m_dynamic_count) * 100.0);
  DEBUG_OUT("\n");
  DEBUG_OUT("number of positive interference  : %lld\n", m_positive_inf);
  DEBUG_OUT("number of negative interference  : %lld\n", m_negative_inf);
  DEBUG_OUT("percent of mispredictions due to interference: %g%%\n",
            ((double) m_negative_inf / (double) m_mispredict_count) * 100.0);
  DEBUG_OUT("\n");
  m_bitdist->printStats();

  if ( gettimeofday( &now, NULL ) != 0 ) {
    DEBUG_OUT("error: unable to call gettimeofday()\n");
  }
  long  sec_expired  = now.tv_sec - m_start_time.tv_sec;
  long  usec_expired = now.tv_usec - m_start_time.tv_usec;
  if ( usec_expired < 0 ) {
    usec_expired += 1000000;
    sec_expired--;
  }

  DEBUG_OUT("  %-50.50s %lu sec %lu usec\n", "Total Elapsed Time:",
            sec_expired, usec_expired );
  double dsec = (double) sec_expired + ((double) usec_expired / (double) 1000000);
  DEBUG_OUT("  %-50.50s %g\n", "Approximate branch per sec:",
            (double) m_dynamic_count / dsec );

  exit(1);
}

//**************************************************************************
void signalHandler( int signal )
{
  if (signal == SIGINT) {
    DEBUG_OUT("User interrupted: ^C exiting...\n");
  } else {
    DEBUG_OUT("handling signal: %d\n", signal);
  }
  printStats();
}

//**************************************************************************
static int compareAddress( const void *addr1, const void *addr2 )
{
  my_addr_t *a1 = (my_addr_t *) addr1;
  my_addr_t *a2 = (my_addr_t *) addr2;
  if ( (*a1 < *a2) )
    return -1;
  else if ( (*a1 > *a2) )
    return 1;
  return 0;
}

//**************************************************************************
my_addr_t *readExceptionFile( char *filename, uint32 *exception_count,
                              uint32 max_excepts )
{
  FILE            *exception_fp = NULL;
  char             buffer[200];
  uint32           count = 0;
  
  exception_fp = fopen( filename, "r" );
  if (exception_fp == NULL) {
    *exception_count = 0;
    DEBUG_OUT("warning: unable to open exception file: %s\n",
              filename);
    return NULL;
  }
  
  my_addr_t       *exception_array = (my_addr_t *) malloc( sizeof(my_addr_t) *
                                                           max_excepts );
  while ( fgets( buffer, 200, exception_fp ) != NULL ) {
    if (count >= max_excepts) {
      DEBUG_OUT("warning: maximum exception size exceeded. file: %s. list >= %d.\n",
                filename,
                max_excepts);
      exit(1);
    }
    exception_array[count++] = strtoull( buffer, NULL, 0 );
  }
  *exception_count = count;
#if 0
  for (uint32 i = 0; i < count; i++) {
    DEBUG_OUT("0x%0llx\n", exception_array[i]);
  }
#endif
  DEBUG_OUT("read %d branches from %s\n", *exception_count, filename );
  fflush(stdout);

  // sort the exception array
  qsort( exception_array, count, sizeof(my_addr_t), &compareAddress );
  return exception_array;
}

/** [Memo].
 *  [Internal Documentation]
 */
//**************************************************************************
int main( int argc, char *argv[] ) {

  if ( !(argc == 3 || argc == 4) ) {
    DEBUG_OUT("usage: %s conffile tracefile startindex\n", argv[0]);
    exit(1);
  }

  // initialize the state
  simstate_init();

  // initialize our module
  hfa_init_local();

  static_inst_t  *s_instr = NULL;
  uint32          startindex = 0;
  if (argc == 4) {
    startindex = atoi( argv[3] );
  }

  // create all global objects
  system_t::inst = new system_t( argv[1] );

  // infinite predictor
  m_predict_inf = new igshare_t( 12, true );
  // register the predictor's configuration checkpoint
  m_predict_inf->registerCheckpoint( &m_conf );
  // m_conf.readConfiguration( "igshare-gcc.conf" );

  // initialize branch predictor
  switch (BRANCHPRED_TYPE) {
  case BRANCHPRED_GSHARE:
    m_predict = new gshare_t( BRANCHPRED_PHT_BITS );
    break;
  case BRANCHPRED_AGREE:
    m_predict = new agree_t( BRANCHPRED_PHT_BITS );
    break;
  case BRANCHPRED_YAGS:
    m_predict = new yags_t( BRANCHPRED_PHT_BITS, BRANCHPRED_EXCEPTION_BITS,
                            BRANCHPRED_TAG_BITS );
    break;
  case BRANCHPRED_IGSHARE:
    m_predict = new igshare_t( BRANCHPRED_PHT_BITS, BRANCHPRED_USE_GLOBAL );
    break;
  case BRANCHPRED_MLPRED:
    m_predict = new mlpredict_t( BRANCHPRED_PHT_BITS, 32, 32 );
    ((mlpredict_t *) m_predict)->readTags( NULL );
    break;
  case BRANCHPRED_EXTREME:
    break;
  default:
    ERROR_OUT("Unknown branch predictor type: %d\n", BRANCHPRED_TYPE);
    ASSERT(0);
  }

  m_predict->InitializeState( &m_history );
  m_static_count = 0;
  memset( &m_branch_record, 0, sizeof(branch_record_t) );
  m_branchfp = new branchfile_t( argv[2], &m_primary_ctx, startindex );
  DEBUG_OUT("context = %d\n", m_primary_ctx);
  
  /** cf:: pseq_t::installInterfaces() */

  m_imap = new ipagemap_t(PSEQ_IPAGE_TABLESIZE);
  if ( gettimeofday( &m_start_time, NULL ) != 0 ) {
    ERROR_OUT("error: unable to call gettimeofday()\n");
  }

  // install signal handler
  signal( SIGINT, signalHandler );

  // read the branches to ignore from a file
  if ( BRANCHPRED_IGNORE_FILE ) {
    m_ignore_array = readExceptionFile( BRANCHPRED_IGNORE_FILE,
                                        &m_ignore_count, m_max_ignore );
  }
  
  // read the branches to treat as "high confidence" branches
  if ( BRANCHPRED_CONFIDENT_FILE ) {
    m_confident_array = readExceptionFile( BRANCHPRED_CONFIDENT_FILE,
                                           &m_confident_count, m_max_confident );
  }

  // read the branches which are the highest probability
  if ( BRANCHPRED_FREQUENT_FILE ) {
    m_frequent_array = readExceptionFile( BRANCHPRED_FREQUENT_FILE,
                                          &m_frequent_count, m_max_frequent );
    if (m_frequent_count != 32) {
      DEBUG_OUT("warning: number of frequent branches hard coded at %d\n",
                m_max_frequent);
    }
    m_predict_freq = new fatpredict_t( m_max_frequent, 5, 32 );
    for (uint32 i = 0; i < m_max_frequent; i++) {
      m_predict_freq->overwrite(i, m_frequent_array[i], 0);
    }
  }

  // read a branch from the branch file
  bool readOK = true;
  bool isfound;
  while (readOK) {

    readOK = m_branchfp->readBranch( &m_branch_record );
    if (m_branch_record.rectype == BRTRACE_BRANCH) {
      isfound = m_imap_os->queryInstr( m_branch_record.ppc, s_instr );
      if (!isfound) {
        DEBUG_OUT("warning: ppc: 0x%0llx not found!\n", m_branch_record.ppc );
        m_branchfp->print( &m_branch_record );
        continue;
      }
    } else if (m_branch_record.rectype == BRTRACE_NEW_BRANCH) {

      // update the static instruction tables
      m_imap->insertInstr( m_branch_record.vpc, 
                           m_branch_record.ppc,
                           m_branch_record.instr,
                           s_instr );

      // search a list of suggest "static" list
      my_addr_t searchPC = s_instr->getVPC();
      void     *foundmatch;
      if ( m_ignore_count > 0 ) {
        foundmatch = bsearch( &searchPC, m_ignore_array, m_ignore_count,
                              sizeof(my_addr_t), &compareAddress );
        if (foundmatch != NULL) {
          m_static_ignore++;
          s_instr->setFlag( SI_WRITE_CONTROL, 1 );
        }
      }
      if ( m_confident_count > 0 ) {
        foundmatch = bsearch( &searchPC, m_confident_array, m_confident_count,
                              sizeof(my_addr_t), &compareAddress );
        if (foundmatch != NULL) {
          m_static_confident++;
          s_instr->setFlag( SI_TOFREE, 1 );
        }
      }
      if ( m_frequent_count > 0 ) {
        foundmatch = bsearch( &searchPC, m_frequent_array, m_frequent_count,
                              sizeof(my_addr_t), &compareAddress );
        if (foundmatch != NULL) {
          m_static_frequent++;
          s_instr->setFlag( SI_BIT6, 1 );
        }
      }

    } else if (m_branch_record.rectype == BRTRACE_CTX_SWITCH) {

      m_context_switches++;
      //      DEBUG_OUT("changed primary ctx = %d\n", m_primary_ctx);
      continue;
    }

    if ( m_branch_record.interrupted ) {
      m_interrupt_count++;
      // don't train this branch, we don't know its result
      continue;
    }

    ASSERT( s_instr != NULL );

    // ignoring this branch: don't predict, update the counters or anything...
    // if ignoring this branch ... don't predict, don't update stats
    if ( s_instr->getFlag( SI_WRITE_CONTROL ) ) {
      m_dynamic_ignore++;
      continue;
    }

    bool infpred;
    bool mypred;
    bool mispredict;
    int  static_predict = s_instr->getFlag( SI_STATICPRED );

    // predict using the inf predictor
    infpred = m_predict_inf->Predict( m_branch_record.vpc, m_history,
                                      static_predict );

    // high confidence branch : treat differently.
    // if (true) {
    if ( s_instr->getFlag( SI_TOFREE ) ) {
      m_dynamic_confident++;
      // predict last result of this branch
      mypred = static_predict;

      // frequent branch: treat differently
    } else if ( s_instr->getFlag( SI_BIT6 ) ) {
      
      m_dynamic_frequent++;
      int32 entry = m_predict_freq->searchTable( m_branch_record.vpc );
      if (entry >= 0) {
        mypred = m_predict_freq->predict( entry, m_history, static_predict );
      } else {
        DEBUG_OUT("warn: can't find branch 0x%0llx in fat predict.\n",
                  m_branch_record.vpc );
        mypred = m_predict->Predict( m_branch_record.vpc, m_history,
                                     static_predict );
      }
      
    } else {
      // Predict this branch
      mypred = m_predict->Predict( m_branch_record.vpc, m_history,
                                   static_predict );
    }
    
    if (mypred == static_predict) {
      m_static_agree++;
    }
    mispredict = ( mypred != m_branch_record.result );
    if (mispredict) {
      // we got it wrong, but inf got it right
      if (infpred != mypred)
        m_negative_inf++;
    } else {
      // we got it right, but inf got it wrong
      if (infpred != mypred)
        m_positive_inf++;
    }
    if ( infpred != m_branch_record.result ) {
      m_mispredict_count_inf++;
      s_instr->m_count_mispredict_inf++;
    }

    //
    // Train
    //

    // Train the infinite prediction
    m_predict_inf->Update( m_branch_record.vpc, m_history,
                           static_predict, m_branch_record.result );
      
    bool history_update = true;
    if ( s_instr->getFlag( SI_TOFREE ) ) {
      // update this branches static prediction
      s_instr->setFlag( SI_STATICPRED, m_branch_record.result );
      if (BRANCHPRED_HISTORY_FILTER)
        history_update = false;
      
    } else if ( s_instr->getFlag( SI_BIT6 ) ) {
      int32 entry = m_predict_freq->searchTable( m_branch_record.vpc );
      if (entry >= 0) {
        m_predict_freq->update( entry, m_history, static_predict,
                                m_branch_record.result );
      } else {
        DEBUG_OUT("warn: can't update branch 0x%0llx in fat predict.\n",
                  m_branch_record.vpc );
      }
    } else {
      m_predict->Update( m_branch_record.vpc, m_history,
                         static_predict, m_branch_record.result );
      // update the architected history ONLY WHEN branch is not biased!
    }
    
    //
    // History
    //

    // update the architected history AFTER updating the branch predictors
    if (history_update) {
      m_history = m_history << 1;
      m_history = m_history | (m_branch_record.result & 0x1);
      // agree (m_branch_record.result == static_predict) );
    }

    //
    // Statistics
    //

    // increment statistics for static instruction
    m_dynamic_count++;
    s_instr->m_count_execute++;
    if (m_branch_record.result)
      s_instr->m_count_taken++;
    
    if (mispredict) {
      m_mispredict_count++;
      s_instr->m_count_mispredict++;
    }
    
    if (m_branch_record.result != static_predict) {
      m_static_mispredict_count++;
      s_instr->m_count_static_mispredict++;
    }

    // add this result to the "global" history counter.
    m_bitdist->pushHistory( m_branch_record.result, mispredict );
  }
  
  // m_conf.writeConfiguration("igshare-gcc.conf");
  printStats();

  // close down our module
  hfa_fini_local();
  // close down the state
  simstate_fini();
}
