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
 * FileName:  iwindow.C
 * Synopsis:  instruction window (container)
 * Author:    cmauer
 * Version:   $Id$
 */

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include "hfa.h"
#include "hfacore.h"
#include "iwindow.h"

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Variable declarations                                                  */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Forward declarations                                                   */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Constructor(s) / destructor                                            */
/*------------------------------------------------------------------------*/

// parameterless contructor used in allocating an array of iwindow_t inside the pseq class
iwindow_t::iwindow_t()
{

  m_rob_size = 0;
  m_win_size = 0;
  m_window   = NULL;
}

//**************************************************************************
iwindow_t::iwindow_t(uint32 rob_size, uint32 win_size)
{
  ASSERT(rob_size >= win_size && win_size > 0);

  m_rob_size = rob_size;
  m_win_size = win_size;
  m_window   = (dynamic_inst_t **) malloc( sizeof(dynamic_inst_t *) * m_rob_size );

   initialize();
}

void iwindow_t::set(uint32 rob_size, uint32 win_size, uint32 proc)
{
  ASSERT(proc < CONFIG_LOGICAL_PER_PHY_PROC);
  ASSERT(rob_size >= win_size && win_size > 0);

  m_rob_size = rob_size;
  m_win_size = win_size;
  m_window   = (dynamic_inst_t **) malloc( sizeof(dynamic_inst_t *) * m_rob_size );
  m_proc = proc;

   initialize();
}

//**************************************************************************
iwindow_t::~iwindow_t()
{
  if (m_window) {
    free( m_window );
    m_window = NULL;
  }
}

//**************************************************************************
void iwindow_t::initialize( void ) {
  for (uint32 i=0; i < m_rob_size; i++) {
    m_window[i] = NULL;
  }
 
  m_last_fetched   = m_rob_size - 1;
  m_last_decoded   = m_rob_size - 1;
  m_last_scheduled = m_rob_size - 1;
  m_last_retired   = m_rob_size - 1;
}

/*------------------------------------------------------------------------*/
/* Public methods                                                         */
/*------------------------------------------------------------------------*/

//***************************************************************************
uint32 iwindow_t::getInsertIndex( void )
{
  // calculate the next slot in the window (wrap around)
  uint32  nextslot = iwin_increment(m_last_fetched);
  return (nextslot);
}

//**************************************************************************
void  iwindow_t::insertInstruction( dynamic_inst_t *instr)
{
   #ifdef DEBUG_IWIN
      DEBUG_OUT("iwindow_t:insertInstruction BEGIN\n");
  #endif
  // proc indicates which logical proc wants to insert an instr into the buffer

  // calculate the next slot in the window (wrap around)
  uint32  nextslot = iwin_increment(m_last_fetched);

  // check that we insert into an empty slot
  ASSERT( m_window[nextslot] == NULL );
  
  // insert instruction, increment last fetched instruction (wrap around)
  m_window[nextslot]   = instr;
  m_last_fetched       = nextslot;

    #ifdef DEBUG_IWIN
      DEBUG_OUT("iwindow_t:insertInstruction END\n");
  #endif
}

//**************************************************************************
dynamic_inst_t *iwindow_t::decodeInstruction( void )
{
    #ifdef DEBUG_IWIN
      DEBUG_OUT("iwindow_t:decodeInstruction BEGIN\n");
  #endif
  // calculate the next slot in the window (wrap around)
  m_last_decoded = iwin_increment(m_last_decoded);

  // return instruction
    #ifdef DEBUG_IWIN
      DEBUG_OUT("iwindow_t:decodeInstruction END\n");
  #endif
  return (m_window[m_last_decoded]);
}

//**************************************************************************
dynamic_inst_t *iwindow_t::scheduleInstruction( void )
{
  #ifdef DEBUG_IWIN
      DEBUG_OUT("iwindow_t:scheduleInstruction BEGIN\n");
  #endif
  // make sure instruction window is not full
  if( rangeSubtract(m_last_scheduled, m_last_retired) >= (int32) m_win_size){
   #ifdef DEBUG_IWIN
      DEBUG_OUT("iwindow_t:scheduleInstruction END (window full)\n");
  #endif
    return NULL;
  }

  // calculate the next slot in the window (wrap around)
  m_last_scheduled = iwin_increment(m_last_scheduled);

  // return instruction
 #ifdef DEBUG_IWIN
      DEBUG_OUT("iwindow_t:scheduleInstruction END\n");
  #endif
  return (m_window[m_last_scheduled]);
}

//***************************************************************************
dynamic_inst_t *iwindow_t::retireInstruction( void )
{
    #ifdef DEBUG_IWIN
      DEBUG_OUT("iwindow_t:retireInstruction BEGIN\n");
  #endif
  // calculate the next retired slot
  uint32  nextslot      = iwin_increment(m_last_retired);
  dynamic_inst_t *dinst = m_window[nextslot];

  // check if this dynamic instruction is ready to retire...
  // DEBUG_RETIRE
  /*
  if (dinst) {
    dinst->m_pseq->out_info("## retire check %d %d (%d)\n", m_last_retired,
                            dinst->isRetireReady(), (int) dinst );
    dinst->printDetail();
  }
  */

  if (dinst && dinst->isRetireReady()) {
    m_last_retired = nextslot;
    m_window[m_last_retired] = NULL;
    // mark "d" as removed from the iwin, so lsq->verify doesn't get confused
    dinst->markEvent( EVENT_IWINDOW_REMOVED );
    #ifdef DEBUG_IWIN
      DEBUG_OUT("iwindow_t:retireInstruction END\n");
  #endif
    return (dinst);
  }
    #ifdef DEBUG_IWIN
      DEBUG_OUT("iwindow_t:retireInstruction END\n");
  #endif
  return (NULL);
}

//**************************************************************************
bool   iwindow_t::isInWindow( uint32 testNum )
{
  bool result;

  result = rangeOverlap( m_last_retired, m_last_decoded, testNum );
  return ( result );
}

//**************************************************************************
void iwindow_t::squash( pseq_t* the_pseq, int32 last_good, int32 &num_decoded)
{
  #ifdef DEBUG_IWIN
      DEBUG_OUT("iwindow_t:squash lastgood[%d] num_decoded[%d]\n",last_good,num_decoded);
  #endif
  // window index in the m_window
  uint32      windex;

  // check that last good is between the last_retired && the last_fetched
  if ( !rangeOverlap( m_last_retired, m_last_fetched, last_good ) ) {
    DEBUG_OUT( "squash: warning: last_good = %d. last_fetched = %d. last_retired = %d. [%0llx]\n",
               last_good, m_last_fetched, m_last_retired,
               the_pseq->getLocalCycle() );
  }

    #ifdef DEBUG_IWIN
      DEBUG_OUT("\tafter calling rangeOverlap\n",last_good,num_decoded);
  #endif
  if ( ( iwin_increment(last_good) == m_last_retired ) &&
       m_window[last_good] == NULL ) {   
    DEBUG_OUT( "squash: warning: last_good = %d. last_retired = %d. [%0llx]\n",
               last_good, m_last_retired, the_pseq->getLocalCycle() );
  }

  #ifdef DEBUG_IWIN
      DEBUG_OUT("\tbegin setting stage_squash \n",last_good,num_decoded);
  #endif
  uint32 stage_squash[dynamic_inst_t::MAX_INST_STAGE];
  for (uint32 i = 0; i < dynamic_inst_t::MAX_INST_STAGE; i++) {
    stage_squash[i] = 0;
  }
  windex = m_last_fetched;
  // squash all the fetched instructions (in reverse order)
  while (windex != m_last_decoded) {

    if (m_window[windex]) {
      // FetchSquash double checks this is OK to squash
      stage_squash[m_window[windex]->getStage()]++;
      #ifdef DEBUG_IWIN
          DEBUG_OUT("\tbefore calling FetchSquash\n",last_good,num_decoded);
       #endif
      m_window[windex]->FetchSquash();
        #ifdef DEBUG_IWIN
              DEBUG_OUT("\tafter calling FetchSquash\n",last_good,num_decoded);
       #endif
      delete m_window[windex];
        #ifdef DEBUG_IWIN
             DEBUG_OUT("\tsetting m_window[windex]=NULL\n",last_good,num_decoded);
        #endif
      m_window[windex] = NULL;
    } else {
      ERROR_OUT("error: iwindow: fetchsquash(): index %d is NULL\n", windex);
      SIM_HALT;
    }
    windex = iwin_decrement(windex);
  }

  // we squash instructions in the opposite order they were fetched,
  // the process of squashing restores the decode register map.
  windex = m_last_decoded;
  // squash all the decoded instructions (in reverse order)
  while (windex != (uint32) last_good) {
    
    // squash modifies the decode map to restore the original mapping
    if (m_window[windex]) {
      // if last_good is last_retired, windex may point to NULL entry
      stage_squash[m_window[windex]->getStage()]++;
        #ifdef DEBUG_IWIN
            DEBUG_OUT("\tbefore calling Squash\n",last_good,num_decoded);
        #endif
            assert(m_window[windex] != NULL);
          
      m_window[windex]->Squash();

        #ifdef DEBUG_IWIN
            DEBUG_OUT("\tafter calling Squash\n",last_good,num_decoded);
       #endif
      delete m_window[windex];
        #ifdef DEBUG_IWIN
           DEBUG_OUT("\tsetting m_window[windex]=NULL\n",last_good,num_decoded);
         #endif
      m_window[windex] = NULL;
    } else {
      ERROR_OUT("error: iwindow: squash(): index %d is NULL\n", windex);
      SIM_HALT;
    }
    windex = iwin_decrement(windex);
  }
  
  // assess stage-based statistics on what was squashed
  for (uint32 i = 0; i < dynamic_inst_t::MAX_INST_STAGE; i++) {
    if (stage_squash[i] >= (uint32) IWINDOW_ROB_SIZE) {
      ERROR_OUT("lots of instructions (%d) squashed from stage: %s\n",
                stage_squash[i],
                dynamic_inst_t::printStage( (dynamic_inst_t::stage_t) i ));
      stage_squash[i] = IWINDOW_ROB_SIZE - 1;
    }
    STAT_INC( the_pseq->m_hist_squash_stage[i][stage_squash[i]] );
  }
  
  // look back through the in-flight instructions,
  // restoring the most recent "good" branch predictors state
  windex = last_good;
  while (windex != m_last_retired) {
    if (m_window[windex] == NULL) {
      ERROR_OUT("error: iwindow: inflight: index %d is NULL\n", windex);
      SIM_HALT;
    } else {
      if (m_window[windex]->getStaticInst()->getType() == DYN_CONTROL) {
        predictor_state_t good_spec_state = (static_cast<control_inst_t*> (m_window[windex]))->getPredictorState();
         #ifdef DEBUG_IWIN
               DEBUG_OUT("\tbefore calling setSpecBPS\n",last_good,num_decoded);
         #endif
               the_pseq->setSpecBPS(good_spec_state);  
          #ifdef DEBUG_IWIN
               DEBUG_OUT("\tafter calling setSpecBPS\n",last_good,num_decoded);
           #endif
        break;
      }
    }
    windex = iwin_decrement(windex);
  }
  if (windex == m_last_retired) {
    /* no inflight branch, restore from retired "architectural" state */
    #ifdef DEBUG_IWIN
      DEBUG_OUT("\tbefore calling setSpecBPS (2)\n",last_good,num_decoded);
    #endif
    the_pseq->setSpecBPS(*(the_pseq->getArchBPS()) );
    #ifdef DEBUG_IWIN
      DEBUG_OUT("\tafter calling setSpecBPS (2)\n",last_good,num_decoded);
    #endif
  }

  m_last_fetched   = last_good;
  m_last_decoded   = last_good;

  // if we squash in execute, we can flush the decode pipeline, with no trouble
  // check if logically (m_last_scheduled > last_good)
 #ifdef DEBUG_IWIN
      DEBUG_OUT("\tbefore rangeOverlap\n",last_good,num_decoded);
  #endif
  if ( rangeOverlap( m_last_retired, m_last_scheduled, last_good ) ) {

    m_last_scheduled = last_good;
    num_decoded = 0;
    ASSERT( rangeSubtract( m_last_decoded, m_last_scheduled ) <= 0 );

  } else {

   #ifdef DEBUG_IWIN
      DEBUG_OUT("\tbefore calling rangeSubtract\n",last_good,num_decoded);
  #endif
    // else, last_good instruction is in decode ... so we need to pass
    // the number of currently decoded instructions back...
    num_decoded = (uint32) rangeSubtract( m_last_decoded, m_last_scheduled );
   #ifdef DEBUG_IWIN
      DEBUG_OUT("\tafter calling rangeSubtract\n",last_good,num_decoded);
  #endif
    
  }

  if (num_decoded > 8) {
    SIM_HALT;
  }
    #ifdef DEBUG_IWIN
      DEBUG_OUT("iwindow_t::squash END\n",last_good,num_decoded);
  #endif
}

//**************************************************************************
//   Used to blindly squash all instructions in window.  Used to squash
//     pipe whenever we don't care about restoring register mappings using Squash()
void iwindow_t::squash_all( pseq_t* the_pseq)
{
  for (uint32 i=0; i < m_rob_size; i++) {
    if(m_window[i]){
      delete m_window[i];
    }
    m_window[i] = NULL;
  }
 
  m_last_fetched   = m_rob_size - 1;
  m_last_decoded   = m_rob_size - 1;
  m_last_scheduled = m_rob_size - 1;
  m_last_retired   = m_rob_size - 1;
}

//**************************************************************************
void  iwindow_t::print( void )
{
  DEBUG_OUT("|");
  for (uint32 i = 0; i < m_rob_size; i++) {
    if (m_window[i] != NULL) {
      DEBUG_OUT("X|");
    } else {
      DEBUG_OUT(" |");
    }
  }
  DEBUG_OUT("\n");

  DEBUG_OUT("|");
  for (uint32 i = 0; i < m_rob_size; i++) {
    int  overlap = 0;
    char output  = ' ';
    if ( i == m_last_retired ) {
      overlap++;
      output = 'R';
    }
    if ( i == m_last_scheduled ) {
      overlap++;
      output = 'E';
    }
    if ( i == m_last_decoded ) {
      overlap++;
      output = 'D';
    }
    if ( i == m_last_fetched ) {
      overlap++;
      output = 'F';
    }
    if (overlap > 1) {
      DEBUG_OUT("%.1d|", overlap);
    } else {
      DEBUG_OUT("%c|", output);
    }
  }
  DEBUG_OUT("\n");
}

//***************************************************************************
void  iwindow_t::printDetail( void )
{
  DEBUG_OUT("Num slots taken: %d\n", getNumSlotsTaken());
  DEBUG_OUT("last fetched:    %d\n", m_last_fetched);
  DEBUG_OUT("last decoded:    %d\n", m_last_decoded);
  DEBUG_OUT("last scheduled:  %d\n", m_last_scheduled);
  DEBUG_OUT("last retired:    %d\n", m_last_retired);
  uint32  loc = iwin_increment(m_last_retired);
  while (loc != m_last_retired) {
    if (m_window[loc] != NULL) {
      DEBUG_OUT("window: %d\n", loc);
      m_window[loc]->printDetail();
    }
    loc = iwin_increment(loc);
  }

  if (m_window[m_last_retired] != NULL) {
    DEBUG_OUT("window: %d\n", m_last_retired);
    m_window[m_last_retired]->printDetail();
  }
}

//***************************************************************************
dynamic_inst_t *iwindow_t::peekWindow( int &index )
{
   #ifdef DEBUG_IWIN
      DEBUG_OUT("iwindow_t:peekWindow BEGIN\n");
  #endif
  // return the current dynamic instr
  dynamic_inst_t *d = m_window[index];

  // calculate the next slot in the window (wrap around)
  index = iwin_increment(index);
    #ifdef DEBUG_IWIN
      DEBUG_OUT("iwindow_t:peekWindow END index[%d]\n",index);
  #endif
  return (d);
}

/*------------------------------------------------------------------------*/
/* Accessor(s) / mutator(s)                                               */
/*------------------------------------------------------------------------*/

//***************************************************************************
uint32  iwindow_t::getLastFetched( void )
{
  return (m_last_fetched);
}

//***************************************************************************
uint32  iwindow_t::getLastDecoded( void )
{
  return (m_last_decoded);
}

//***************************************************************************
uint32  iwindow_t::getLastScheduled( void )
{
  return (m_last_scheduled);
}

//***************************************************************************
uint32  iwindow_t::getLastRetired( void )
{
  return (m_last_retired);
}

/*------------------------------------------------------------------------*/
/* Private methods                                                        */
/*------------------------------------------------------------------------*/

//***************************************************************************
bool iwindow_t::rangeOverlap( uint32 lowerNum, uint32 upperNum, uint32 testNum )
{
    #ifdef DEBUG_IWIN
      DEBUG_OUT("iwindow_t:rangeOverlap BEGIN lowernum[%d] uppernum[%d] testnum[%d]\n",
                          lowerNum, upperNum, testNum);
      #endif
  bool result;
  if ( lowerNum <= upperNum ) {
    // regular check
    result = ( (lowerNum <= testNum) && (testNum <= upperNum) );
  } else {
    // wrapped check
    result = !( (upperNum < testNum) && (testNum < lowerNum) );
  }
    #ifdef DEBUG_IWIN
      DEBUG_OUT("iwindow_t:rangeOverlap END result[%d]\n",result);
  #endif
  return (result);
}

//***************************************************************************
int32 iwindow_t::rangeSubtract( uint32 index1, uint32 index2 )
{
    #ifdef DEBUG_IWIN
      DEBUG_OUT("iwindow_t:rangeSubtract BEGIN index1[%d] index2[%d]\n",index1, index2);
  #endif
  // offset of index 1 relative to the last_retired instruction
  uint32  offseti1;

  if ( m_last_retired <= index1 ) {
    offseti1 = index1 - m_last_retired;
  } else {
    offseti1 = (m_rob_size + index1) - m_last_retired;
  }

  // offset of index 2 relative to the last_retired instruction
  uint32  offseti2;

  if ( m_last_retired <= index2 ) {
    offseti2 = index2 - m_last_retired;
  } else {
    offseti2 = (m_rob_size + index2) - m_last_retired;
  }

  #ifdef DEBUG_IWIN
      DEBUG_OUT("iwindow_t:rangeSubtract END result[%d]\n",offseti1-offseti2);
  #endif
  return ( offseti1 - offseti2 );
}

//**************************************************************************
int32 iwindow_t::getNumSlotsTaken(){
  //Note: keep m_last_retired as 2nd argument so that you won't get negative return values
  int32 slotsTaken = rangeSubtract(m_last_fetched, m_last_retired);
  assert(slotsTaken >= 0);
  // this is an offset by one count
  if( isSlotAvailable() == false ){
    // we always have to allocate one more ROB entry than the simulated ROB size, because of the way isSlotAvailable() checks pointers
    assert(slotsTaken == m_rob_size-1);
  }
  return (slotsTaken);
}

//*************************************************************************
int32 iwindow_t::getNumCanRetire(){
   //Note: keep m_last_retired as 2nd argument so that you won't get negative return values
  int32 num_retire = rangeSubtract(m_last_scheduled, m_last_retired);
  assert(num_retire >= 0);
  return num_retire;
}

//*************************************************************************
int32 iwindow_t::getNumCanSched(){
  int32 num_sched = rangeSubtract(m_last_decoded, m_last_scheduled);
  assert(num_sched >= 0);
  return num_sched;
}

//*************************************************************************
int32 iwindow_t::getNumCanDecode(){
  int32 num_decode = rangeSubtract(m_last_fetched, m_last_decoded);
  assert(num_decode >= 0);
  return num_decode;
}

//**************************************************************************
// Used by fetch/storeSet.C to find the dynamic instr matching the store PC
dynamic_inst_t *
iwindow_t::lookupOlder( const uint32 & index, const la_t & pc){
  //  cout << "lookupOlder BEGIN" << endl;
  uint32 cur_index = index;
  ASSERT( cur_index != m_last_retired );
  cur_index = iwin_decrement( cur_index );
  while(cur_index != m_last_retired){
    if( m_window[cur_index] != NULL){
      dynamic_inst_t * instr = m_window[cur_index];
      if( (instr->getStaticInst()->getType() == DYN_STORE) &&
          (instr->getVPC() == pc) ){
        //    cout << "\tlookupolder FOUND" << endl;
        return instr;
      }
    }
    cur_index = iwin_decrement( cur_index );
  }
  // no matching instr found
  //  cout << "lookupOlder END" << endl;
  return NULL;
}

/*------------------------------------------------------------------------*/
/* Static methods                                                         */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Global functions                                                       */
/*------------------------------------------------------------------------*/


/** [Memo].
 *  [Internal Documentation]
 */
//**************************************************************************

