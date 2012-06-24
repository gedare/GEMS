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
/* I inclued this copyright since we're using Cacti for some stuff */

/*------------------------------------------------------------
 *  Copyright 1994 Digital Equipment Corporation and Steve Wilton
 *                         All Rights Reserved
 *
 * Permission to use, copy, and modify this software and its documentation is
 * hereby granted only under the following terms and conditions.  Both the
 * above copyright notice and this permission notice must appear in all copies
 * of the software, derivative works or modified versions, and any portions
 * thereof, and both notices must appear in supporting documentation.
 *
 * Users of this software agree to the terms and conditions set forth herein,
 * and hereby grant back to Digital a non-exclusive, unrestricted, royalty-
 * free right and license under any changes, enhancements or extensions
 * made to the core functions of the software, including but not limited to
 * those affording compatibility with other hardware or software
 * environments, but excluding applications which incorporate this software.
 * Users further agree to use their best efforts to return to Digital any
 * such changes, enhancements or extensions that they make and inform Digital
 * of noteworthy uses of this software.  Correspondence should be provided
 * to Digital at:
 *
 *                       Director of Licensing
 *                       Western Research Laboratory
 *                       Digital Equipment Corporation
 *                       100 Hamilton Avenue
 *                       Palo Alto, California  94301
 *
 * This software may be distributed (but not offered for sale or transferred
 * for compensation) to third parties, provided such third parties agree to
 * abide by the terms and conditions of this notice.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND DIGITAL EQUIPMENT CORP. DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS.   IN NO EVENT SHALL DIGITAL EQUIPMENT
 * CORPORATION BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 *------------------------------------------------------------*/

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "hfa.h"
#include "hfacore.h"

#include "power.h"

/*----------------------------------------------------------------------*/
power_t::power_t(pseq_t * pseq, int id){
  m_pseq = pseq;

  //create a new power result type struct and zero everything out...
  power = NULL;
  power = new power_result_type;
  memset((void *) power, 0, sizeof(power_result_type));

  //set the output file ptr, the filename is of the form chip_N.p
  m_output_file = NULL;
#if 0
  m_id = id;
  char temp[10];
  sprintf(temp, "%d", m_id);
  char filename[20];
  strcpy(filename, "/tmp/chip_");
  strcat(filename,temp);
  strcat(filename,".p");
  m_output_file = NULL;
  m_output_file = fopen(filename,"w");
  if(!m_output_file){
    cout << "\n********power_t: cannot open file " << filename << endl;
  }
  else{
    //output the header only once
    cout << "\n***power_t: opening file " << filename << endl;
    char header[300];
    //append ID after each name to differentiate for each core:
    strcpy(header, "L2_bottom_");
    strcat(header, temp);
    strcat(header, "\tL2_top_");
    strcat(header, temp);
    strcat(header, "\tL1D_");
    strcat(header, temp);
    strcat(header, "\tL1I_");
    strcat(header, temp);
    strcat(header, "\tRegfile_");
    strcat(header, temp);
    strcat(header, "\tALU_");
    strcat(header, temp);
    strcat(header, "\tIwin_");
    strcat(header, temp);
    strcat(header, "\tLSQ_");
    strcat(header, temp);
    strcat(header, "\tBpred_");
    strcat(header, temp);
    strcat(header, "\tRename_");
    strcat(header, temp);
    strcat(header, "\t\n");
    //    char header[] = "L2_bottom\tL2_top\tL1D\tL1I\tRegfile\tALU\tIwin\tLSQ\tBpred\tRename\t\n";
    fputs(header, m_output_file);
  }
#endif

  //clear out variables:
  /* set scale for crossover (vdd->gnd) currents */
  crossover_scaling = 1.2;
  /* set non-ideal turnoff percentage */
  turnoff_factor = 0.1;

  //set btb_config parameters
  //number of sets in BTB - include both regular and exception BTB sets
  btb_config[0] = CAS_TABLE_SIZE+CAS_EXCEPT_SIZE;
  //assoc of BTB...currently BTB is direct-mapped
  btb_config[1] = 1;

  //set 2-level bp parameters
  // NOTE: currently we use YAGS, which is not a 2level predictor...it is a table based predictor
  //           which uses 2 tables. So we do NOT need a 2level predicotr
  twolev_config[0] = 0;
  twolev_config[1] = 0;
  twolev_config[2] = 0;
  twolev_config[3] = 0;

  // set bimodal predictor parameters..
  //  NOTE: this is closest in configuration to YAGS, since it uses 2 prediction tables.  However we are
  //     fudgeing by NOT taking into account the tag bits for the exception table!
  bimod_config[0] = (1 << BRANCHPRED_PHT_BITS) + (1 << BRANCHPRED_EXCEPTION_BITS);

  //set combining predictor parameters
  // NOTE: not currently simiulated
  comb_config[0] = 0;

  //set cache parameters
  cache_dl1 = NULL;
  cache_il1 = NULL;
  cache_dl2 = NULL;
  cache_dl1 = new cache_params_t;
  cache_il1 = new cache_params_t;
  cache_dl2 = new cache_params_t;
  
  //set L1D params
  cache_dl1->nsets = L1D_NUM_SETS;
  cache_dl1->bsize = L1D_BLOCK_SIZE_BYTES;
  cache_dl1->assoc = L1D_ASSOC;

  //set L1I params
  cache_il1->nsets = L1I_NUM_SETS;
  cache_il1->bsize = L1I_BLOCK_SIZE_BYTES;
  cache_il1->assoc = L1I_ASSOC;

  //set L2 UNIFIED params
  cache_dl2->nsets = L2_NUM_SETS;
  cache_dl2->bsize = L2_BLOCK_SIZE_BYTES;
  cache_dl2->assoc = L2_ASSOC;

  //set Instr and Data TLB params
  dtlb = itlb = NULL;
  dtlb = new cache_params_t;
  itlb = new cache_params_t;

  dtlb->nsets = DTLB_NUM_SETS;
  dtlb->bsize = TLB_BLOCK_SIZE_BYTES;
  dtlb->assoc = DTLB_ASSOC;

  itlb->nsets = ITLB_NUM_SETS;
  itlb->bsize = TLB_BLOCK_SIZE_BYTES;
  itlb->assoc =  ITLB_ASSOC;

  rename_power=0;
  bpred_power=0;
  window_power=0;
  lsq_power=0;
  regfile_power=0;
  icache_power=0;
  dcache_power=0;
  dcache2_power=0;
  alu_power=0;
  falu_power=0;
  resultbus_power=0;
  clock_power=0;
  instr_prefetcher_power = 0;
  data_prefetcher_power = 0;
  compression_power = 0;

  rename_power_cc1=0;
  bpred_power_cc1=0;
  window_power_cc1=0;
  lsq_power_cc1=0;
  regfile_power_cc1=0;
  icache_power_cc1=0;
  dcache_power_cc1=0;
  dcache2_power_cc1=0;
  alu_power_cc1=0;
  resultbus_power_cc1=0;
  clock_power_cc1=0;
  instr_prefetcher_power_cc1 = 0;
  data_prefetcher_power_cc1 = 0;
  compression_power_cc1 = 0;


  rename_power_cc2=0;
  bpred_power_cc2=0;
  window_power_cc2=0;
  lsq_power_cc2=0;
  regfile_power_cc2=0;
  icache_power_cc2=0;
  dcache_power_cc2=0;
  dcache2_power_cc2=0;
  alu_power_cc2=0;
  resultbus_power_cc2=0;
  clock_power_cc2=0;
  instr_prefetcher_power_cc2 = 0;
  data_prefetcher_power_cc2 = 0;
  compression_power_cc2 = 0;


  rename_power_cc3=0;
  bpred_power_cc3=0;
  window_power_cc3=0;
  lsq_power_cc3=0;
  regfile_power_cc3=0;
  icache_power_cc3=0;
  dcache_power_cc3=0;
  dcache2_power_cc3=0;
  alu_power_cc3=0;
  resultbus_power_cc3=0;
  clock_power_cc3=0;
  instr_prefetcher_power_cc3 = 0;
  data_prefetcher_power_cc3 = 0;
  compression_power_cc3 = 0;

  max_rename_power_cc3=0;
  max_bpred_power_cc3=0;
  max_window_power_cc3=0;
  max_lsq_power_cc3=0;
  max_regfile_power_cc3=0;
  max_icache_power_cc3=0;
  max_dcache_power_cc3=0;
  max_dcache2_power_cc3=0;
  max_alu_power_cc3=0;
  max_resultbus_power_cc3=0;
  max_current_clock_power_cc3=0;
  max_instr_prefetcher_power_cc3 = 0;
  max_data_prefetcher_power_cc3 = 0;
  max_compression_power_cc3 = 0;

  
  last_single_total_cycle_power_cc1 = 0.0;
  last_single_total_cycle_power_cc2 = 0.0;
  last_single_total_cycle_power_cc3 = 0.0;

  last_single_total_cycle_power_no_clock_cc1 = 0.0;
  last_single_total_cycle_power_no_clock_cc2 = 0.0;
  last_single_total_cycle_power_no_clock_cc3 = 0.0;

  last_clock_power_cc1 = 0.0;
  last_clock_power_cc2 = 0.0;
  last_clock_power_cc3 = 0.0;

  max_cycle_power = 0.0;
  max_cycle_power_cc1 = 0.0;
  max_cycle_power_cc2 = 0.0;
  max_cycle_power_cc3 = 0.0;
  max_cycle_power_no_clock_cc1 = 0.0;
  max_cycle_power_no_clock_cc2 = 0.0;
  max_cycle_power_no_clock_cc3 = 0.0;


  max_clock_power_cc1 = 0.0;
  max_clock_power_cc2 = 0.0;
  max_clock_power_cc3 = 0.0;

  total_rename_access=0;
  total_bpred_access=0;
  total_window_access=0;
  total_lsq_access=0;
  total_regfile_access=0;
  total_icache_access=0;
  total_dcache_access=0;
  total_dcache2_access=0;
  total_alu_access=0;
  total_resultbus_access=0;
  total_instr_prefetcher_access = 0;
  total_data_prefetcher_access=0;
  total_compression_access=0;

  //reset max accesses
  max_rename_access=0;
  max_bpred_access=0;
  max_window_access=0;
  max_lsq_access=0;
  max_regfile_access=0;
  max_icache_access=0;
  max_dcache_access=0;
  max_dcache2_access=0;
  max_alu_access=0;
  max_resultbus_access=0;
  max_instr_prefetcher_access = 0;
  max_data_prefetcher_access=0;
  max_compression_access=0;

  max_win_preg_access=0;
  max_win_selection_access=0;
  max_win_wakeup_access=0;
  max_lsq_wakeup_access=0;
  max_lsq_preg_access=0;
  max_ialu_access=0;
  max_falu_access=0;

  //those used by the sequencer:
  rename_access=0;
  bpred_access=0;
  window_access=0;
  lsq_access=0;
  regfile_access=0;
  icache_access=0;
  dcache_access=0;
  dcache2_access=0;
  alu_access=0;
  ialu_access=0;
  falu_access=0;
  resultbus_access=0;
  instr_prefetcher_access = 0;
  data_prefetcher_access = 0;
  compression_access=0;

  window_preg_access=0;
  window_selection_access=0;
  window_wakeup_access=0;
  lsq_store_data_access=0;
  lsq_load_data_access=0;
  lsq_preg_access=0;
  lsq_wakeup_access=0;

  window_total_pop_count_cycle=0;
  window_num_pop_count_cycle=0;
  lsq_total_pop_count_cycle=0;
  lsq_num_pop_count_cycle=0;
  regfile_total_pop_count_cycle=0;
  regfile_num_pop_count_cycle=0;
  resultbus_total_pop_count_cycle=0;
  resultbus_num_pop_count_cycle=0;

  m_bank_num.num_l1i_8banks = 0;
  m_bank_num.num_l1d_8banks = 0;
  m_bank_num.num_l1i_4banks = 0;
  m_bank_num.num_l1d_4banks = 0;
  m_bank_num.num_l1i_2banks = 0;
  m_bank_num.num_l1d_2banks = 0;


}

power_t::~power_t(){
  if(power){
    delete power;
  }
  power = NULL;

  if(cache_dl1){
    delete cache_dl1;
  }
  cache_dl1 = NULL;

  if(cache_il1){
    delete cache_il1;
  }
  cache_il1 = NULL;

  if(cache_dl2){
    delete cache_dl2;
  }
  cache_dl2 = NULL;

  if(dtlb){
    delete dtlb;
  }
  dtlb = NULL;

  if(itlb){
    delete itlb;
  }
  itlb = NULL;
}

void power_t::clear_access_stats()
{
  // cout << "***inside CLEAR STATS" << endl;
  rename_access=0;
  bpred_access=0;
  window_access=0;
  lsq_access=0;
  regfile_access=0;
  icache_access=0;
  dcache_access=0;
  dcache2_access=0;
  alu_access=0;
  ialu_access=0;
  falu_access=0;
  resultbus_access=0;
  instr_prefetcher_access = 0;
  data_prefetcher_access = 0;
  compression_access = 0;

  window_preg_access=0;
  window_selection_access=0;
  window_wakeup_access=0;
  lsq_store_data_access=0;
  lsq_load_data_access=0;
  lsq_wakeup_access=0;
  lsq_preg_access=0;

  window_total_pop_count_cycle=0;
  window_num_pop_count_cycle=0;
  lsq_total_pop_count_cycle=0;
  lsq_num_pop_count_cycle=0;
  regfile_total_pop_count_cycle=0;
  regfile_num_pop_count_cycle=0;
  resultbus_total_pop_count_cycle=0;
  resultbus_num_pop_count_cycle=0;

  m_bank_num.num_l1i_8banks = 0;
  m_bank_num.num_l1d_8banks = 0;
  m_bank_num.num_l1i_4banks = 0;
  m_bank_num.num_l1d_4banks = 0;
  m_bank_num.num_l1i_2banks = 0;
  m_bank_num.num_l1d_2banks = 0;
}

/* compute bitline activity factors which we use to scale bitline power 
   Here it is very important whether we assume 0's or 1's are
   responsible for dissipating power in pre-charged stuctures. (since
   most of the bits are 0's, we assume the design is power-efficient
   enough to allow 0's to _not_ discharge 
*/
double power_t::compute_af(counter_t num_pop_count_cycle,counter_t total_pop_count_cycle,int pop_width) {
  double avg_pop_count;
  double af,af_b;

  if(num_pop_count_cycle)
    avg_pop_count = (double)total_pop_count_cycle / (double)num_pop_count_cycle;
  else
    avg_pop_count = 0;

  af = avg_pop_count / (double)pop_width;
  
  af_b = 1.0 - af;

  /*  printf("af == %f%%, af_b == %f%%, total_pop == %d, num_pop == %d\n",100*af,100*af_b,total_pop_count_cycle,num_pop_count_cycle); */

  return(af_b);
}

// This function is used to output periodic power stats every N cycles, as called by the sequencer
void power_t::output_periodic_power(const uint64 & sim_cycle){
  if(m_output_file){
    //output the current power stats (be sure this matches with header!
    //split the L2 power evenly amongst the bottom and top banks
    //bottom bank gets (100-x)% of the L2 power
    // when running with one SMT core, bottom gets 60%
    //                           two: 73%
    //                           four: 95%
    fprintf( m_output_file, "%.2lf\t", (dcache2_power_cc3/sim_cycle)*0.95);
    //top bank gets x% of the L2 power
    // when running with one SMT core, top gets 40%
    //                            two: 27%
    //                            four: 5%
    fprintf( m_output_file, "%.2lf\t", (dcache2_power_cc3/sim_cycle)*0.05);
    fprintf( m_output_file, "%.2lf\t", dcache_power_cc3/sim_cycle);
    fprintf( m_output_file, "%.2lf\t", icache_power_cc3/sim_cycle);
    fprintf( m_output_file, "%.2lf\t", regfile_power_cc3/sim_cycle);
    fprintf( m_output_file, "%.2lf\t", alu_power_cc3/sim_cycle);
    fprintf( m_output_file, "%.2lf\t", window_power_cc3/sim_cycle);
    fprintf( m_output_file, "%.2lf\t", lsq_power_cc3/sim_cycle);
    fprintf( m_output_file, "%.2lf\t", bpred_power_cc3/sim_cycle);
    fprintf( m_output_file,"%.2lf\t\n", rename_power_cc3/sim_cycle);
  }
}
    
    
/* compute power statistics on each cycle, for each conditional clocking style.  Obviously
most of the speed penalty comes here, so if you don't want per-cycle power estimates
you could post-process 

See README.wattch for details on the various clock gating styles.

*/
void power_t::update_power_stats()
{
  //cout << "update power stats BEGIN" << endl;
  double window_af_b, lsq_af_b, regfile_af_b, resultbus_af_b;

#ifdef DYNAMIC_AF
  window_af_b = compute_af(window_num_pop_count_cycle,window_total_pop_count_cycle,data_width);
  lsq_af_b = compute_af(lsq_num_pop_count_cycle,lsq_total_pop_count_cycle,data_width);
  regfile_af_b = compute_af(regfile_num_pop_count_cycle,regfile_total_pop_count_cycle,data_width);
  resultbus_af_b = compute_af(resultbus_num_pop_count_cycle,resultbus_total_pop_count_cycle,data_width);
#endif
  
  //local variables for the current cc3 components:
  double current_rename_power_cc3 =0.0;
  double current_bpred_power_cc3=0.0;
  double current_window_power_cc3=0.0;
  double current_lsq_power_cc3=0.0;
  double current_regfile_power_cc3=0.0;
  double current_icache_power_cc3=0.0;
  double current_dcache_power_cc3=0.0;
  double current_dcache2_power_cc3=0.0;
  double current_alu_power_cc3=0.0;
  double current_resultbus_power_cc3=0.0;
  double local_current_clock_power_cc3=0.0;
  double current_instr_prefetcher_power_cc3=0.0;
  double current_data_prefetcher_power_cc3=0.0;
  double current_compression_power_cc3=0.0;

  rename_power+=power->rename_power;
  bpred_power+=power->bpred_power;
  window_power+=power->window_power;
  lsq_power+=power->lsq_power;
  regfile_power+=power->regfile_power;
  icache_power+=power->icache_power+power->itlb;
  dcache_power+=power->dcache_power+power->dtlb;
  dcache2_power+=power->dcache2_power;
  alu_power+=power->ialu_power + power->falu_power;
  falu_power+=power->falu_power;
  resultbus_power+=power->resultbus;
  clock_power+=power->clock_power;
  //for prefetcher
  instr_prefetcher_power += power->instr_prefetcher_power;
  data_prefetcher_power += power->data_prefetcher_power;
  //for compression
  compression_power += power->compression_power;

  total_rename_access+=rename_access;
  total_bpred_access+=bpred_access;
  total_window_access+=window_access;
  total_lsq_access+=lsq_access;
  total_regfile_access+=regfile_access;
  total_icache_access+=icache_access;
  total_dcache_access+=dcache_access;
  total_dcache2_access+=dcache2_access;
  total_alu_access+=alu_access;
  total_resultbus_access+=resultbus_access;
  //for prefetcher
  total_instr_prefetcher_access += instr_prefetcher_access;
  total_data_prefetcher_access += data_prefetcher_access;
  //for compression
  total_compression_access += compression_access;

  max_rename_access=MAX(rename_access,max_rename_access);
  max_bpred_access=MAX(bpred_access,max_bpred_access);
  max_window_access=MAX(window_access,max_window_access);
  max_lsq_access=MAX(lsq_access,max_lsq_access);
  max_regfile_access=MAX(regfile_access,max_regfile_access);
  max_icache_access=MAX(icache_access,max_icache_access);
  max_dcache_access=MAX(dcache_access,max_dcache_access);
  max_dcache2_access=MAX(dcache2_access,max_dcache2_access);
  max_alu_access=MAX(alu_access,max_alu_access);
  max_resultbus_access=MAX(resultbus_access,max_resultbus_access);
  //for prefetcher
  max_instr_prefetcher_access = MAX(instr_prefetcher_access, max_instr_prefetcher_access);
  max_data_prefetcher_access = MAX(data_prefetcher_access, max_data_prefetcher_access);
  //for compression
  max_compression_access = MAX(compression_access, max_compression_access);
  //for finer granularity
  max_win_preg_access= MAX(window_preg_access, max_win_preg_access);
  max_win_selection_access = MAX(window_selection_access, max_win_selection_access);
  max_win_wakeup_access = MAX(window_wakeup_access, max_win_wakeup_access);
  max_lsq_wakeup_access = MAX(lsq_wakeup_access, max_lsq_wakeup_access);
  max_lsq_preg_access = MAX(lsq_preg_access, max_lsq_preg_access);
  max_ialu_access = MAX(ialu_access, max_ialu_access);
  max_falu_access = MAX(falu_access, max_falu_access);
      
  if(rename_access) {
    rename_power_cc1+=power->rename_power;
    rename_power_cc2+=((double)rename_access/(double)ruu_decode_width)*power->rename_power;
    rename_power_cc3+=((double)rename_access/(double)ruu_decode_width)*power->rename_power;

    current_rename_power_cc3 = ((double)rename_access/(double)ruu_decode_width)*power->rename_power;
  }
  else{ 
    rename_power_cc3+=turnoff_factor*power->rename_power;

    current_rename_power_cc3 = turnoff_factor*power->rename_power;
  }

  if(bpred_access) {
    if(bpred_access <= 2)
      bpred_power_cc1+=power->bpred_power;
    else
      bpred_power_cc1+=((double)bpred_access/2.0) * power->bpred_power;
    bpred_power_cc2+=((double)bpred_access/2.0) * power->bpred_power;
    bpred_power_cc3+=((double)bpred_access/2.0) * power->bpred_power;

    current_bpred_power_cc3 = ((double)bpred_access/2.0) * power->bpred_power;
  }
  else{
    bpred_power_cc3+=turnoff_factor*power->bpred_power;

    current_bpred_power_cc3 = turnoff_factor*power->bpred_power;
  }

#ifdef STATIC_AF
  if(window_preg_access) {
    if(window_preg_access <= 3*ruu_issue_width)
      window_power_cc1+=power->rs_power;
    else
      window_power_cc1+=((double)window_preg_access/(3.0*(double)ruu_issue_width))*power->rs_power;
    window_power_cc2+=((double)window_preg_access/(3.0*(double)ruu_issue_width))*power->rs_power;
    window_power_cc3+=((double)window_preg_access/(3.0*(double)ruu_issue_width))*power->rs_power;
  }
  else
    window_power_cc3+=turnoff_factor*power->rs_power;
#elif defined(DYNAMIC_AF)
  if(window_preg_access) {
    if(window_preg_access <= 3*ruu_issue_width)
      window_power_cc1+=power->rs_power_nobit + window_af_b*power->rs_bitline;
    else
      window_power_cc1+=((double)window_preg_access/(3.0*(double)ruu_issue_width))*(power->rs_power_nobit + window_af_b*power->rs_bitline);
    window_power_cc2+=((double)window_preg_access/(3.0*(double)ruu_issue_width))*(power->rs_power_nobit + window_af_b*power->rs_bitline);
    window_power_cc3+=((double)window_preg_access/(3.0*(double)ruu_issue_width))*(power->rs_power_nobit + window_af_b*power->rs_bitline);

    current_window_power_cc3 += ((double)window_preg_access/(3.0*(double)ruu_issue_width))*(power->rs_power_nobit + window_af_b*power->rs_bitline);
  }
  else{
    window_power_cc3+=turnoff_factor*power->rs_power;

    current_window_power_cc3 += turnoff_factor*power->rs_power;
  }

#else
  panic("no AF-style defined\n");
#endif

  if(window_selection_access) {
    if(window_selection_access <= ruu_issue_width)
      window_power_cc1+=power->selection;
    else
      window_power_cc1+=((double)window_selection_access/((double)ruu_issue_width))*power->selection;
    window_power_cc2+=((double)window_selection_access/((double)ruu_issue_width))*power->selection;
    window_power_cc3+=((double)window_selection_access/((double)ruu_issue_width))*power->selection;

    current_window_power_cc3 += ((double)window_selection_access/((double)ruu_issue_width))*power->selection;
  }
  else{
    window_power_cc3+=turnoff_factor*power->selection;

    current_window_power_cc3 += turnoff_factor*power->selection;
  }

  if(window_wakeup_access) {
    if(window_wakeup_access <= ruu_issue_width)
      window_power_cc1+=power->wakeup_power;
    else
      window_power_cc1+=((double)window_wakeup_access/((double)ruu_issue_width))*power->wakeup_power;
    window_power_cc2+=((double)window_wakeup_access/((double)ruu_issue_width))*power->wakeup_power;
    window_power_cc3+=((double)window_wakeup_access/((double)ruu_issue_width))*power->wakeup_power;

    current_window_power_cc3 += ((double)window_wakeup_access/((double)ruu_issue_width))*power->wakeup_power;

  }
  else{
    window_power_cc3+=turnoff_factor*power->wakeup_power;

    current_window_power_cc3 += turnoff_factor*power->wakeup_power;
  }

  if(lsq_wakeup_access) {
    if(lsq_wakeup_access <= res_memport)
      lsq_power_cc1+=power->lsq_wakeup_power;
    else
      lsq_power_cc1+=((double)lsq_wakeup_access/((double)res_memport))*power->lsq_wakeup_power;
    lsq_power_cc2+=((double)lsq_wakeup_access/((double)res_memport))*power->lsq_wakeup_power;
    lsq_power_cc3+=((double)lsq_wakeup_access/((double)res_memport))*power->lsq_wakeup_power;

    current_lsq_power_cc3 += ((double)lsq_wakeup_access/((double)res_memport))*power->lsq_wakeup_power;
  }
  else{
    lsq_power_cc3+=turnoff_factor*power->lsq_wakeup_power;

    current_lsq_power_cc3 += turnoff_factor*power->lsq_wakeup_power;
  }

#ifdef STATIC_AF
  if(lsq_preg_access) {
    if(lsq_preg_access <= res_memport)
      lsq_power_cc1+=power->lsq_rs_power;
    else
      lsq_power_cc1+=((double)lsq_preg_access/((double)res_memport))*power->lsq_rs_power;
    lsq_power_cc2+=((double)lsq_preg_access/((double)res_memport))*power->lsq_rs_power;
    lsq_power_cc3+=((double)lsq_preg_access/((double)res_memport))*power->lsq_rs_power;
  }
  else
    lsq_power_cc3+=turnoff_factor*power->lsq_rs_power;
#else
  if(lsq_preg_access) {
    if(lsq_preg_access <= res_memport)
      lsq_power_cc1+=power->lsq_rs_power_nobit + lsq_af_b*power->lsq_rs_bitline;
    else
      lsq_power_cc1+=((double)lsq_preg_access/((double)res_memport))*(power->lsq_rs_power_nobit + lsq_af_b*power->lsq_rs_bitline);
    lsq_power_cc2+=((double)lsq_preg_access/((double)res_memport))*(power->lsq_rs_power_nobit + lsq_af_b*power->lsq_rs_bitline);
    lsq_power_cc3+=((double)lsq_preg_access/((double)res_memport))*(power->lsq_rs_power_nobit + lsq_af_b*power->lsq_rs_bitline);

    current_lsq_power_cc3 += ((double)lsq_preg_access/((double)res_memport))*(power->lsq_rs_power_nobit + lsq_af_b*power->lsq_rs_bitline);
  }
  else{
    lsq_power_cc3+=turnoff_factor*power->lsq_rs_power;

    current_lsq_power_cc3 += turnoff_factor*power->lsq_rs_power;
  }
#endif

  //compute prefetcher power here...
  if(instr_prefetcher_access){
    // cout << "***power, instr prefetcher access " << instr_prefetcher_access << endl;
    if(instr_prefetcher_access <= L1_L2_STARTUP_PFS){
      instr_prefetcher_power_cc1 += power->instr_prefetcher_power;
    }
    else{
      instr_prefetcher_power_cc1 += ((double) instr_prefetcher_access/(1.0*L1_L2_STARTUP_PFS))*power->instr_prefetcher_power;
    }
    instr_prefetcher_power_cc2 += ((double) instr_prefetcher_access/(1.0*L1_L2_STARTUP_PFS))*power->instr_prefetcher_power;
    instr_prefetcher_power_cc3 += ((double) instr_prefetcher_access/(1.0*L1_L2_STARTUP_PFS))*power->instr_prefetcher_power;

    current_instr_prefetcher_power_cc3 = ((double) instr_prefetcher_access/(1.0*L1_L2_STARTUP_PFS))*power->instr_prefetcher_power;
  }
  else{
    instr_prefetcher_power_cc3 +=turnoff_factor*power->instr_prefetcher_power;

    current_instr_prefetcher_power_cc3 = turnoff_factor*power->instr_prefetcher_power;
  }

  if(data_prefetcher_access){
    //    cout << "***power, data prefetcher access " << data_prefetcher_access << endl;
    if(data_prefetcher_access <= L1_L2_STARTUP_PFS){
      data_prefetcher_power_cc1 += power->data_prefetcher_power;
    }
    else{
      data_prefetcher_power_cc1 += ((double) data_prefetcher_access/(1.0*L1_L2_STARTUP_PFS))*power->data_prefetcher_power;
    }
    data_prefetcher_power_cc2 += ((double) data_prefetcher_access/(1.0*L1_L2_STARTUP_PFS))*power->data_prefetcher_power;
    data_prefetcher_power_cc3 += ((double) data_prefetcher_access/(1.0*L1_L2_STARTUP_PFS))*power->data_prefetcher_power;

    current_data_prefetcher_power_cc3 = ((double) data_prefetcher_access/(1.0*L1_L2_STARTUP_PFS))*power->data_prefetcher_power;
  }
  else{
    data_prefetcher_power_cc3 +=turnoff_factor*power->data_prefetcher_power;

    current_data_prefetcher_power_cc3 = turnoff_factor*power->data_prefetcher_power;
  }
#ifdef STATIC_AF
  if(regfile_access) {
    if(regfile_access <= (3.0*ruu_commit_width))
      regfile_power_cc1+=power->regfile_power;
    else
      regfile_power_cc1+=((double)regfile_access/(3.0*(double)ruu_commit_width))*power->regfile_power;
    regfile_power_cc2+=((double)regfile_access/(3.0*(double)ruu_commit_width))*power->regfile_power;
    regfile_power_cc3+=((double)regfile_access/(3.0*(double)ruu_commit_width))*power->regfile_power;
  }
  else
    regfile_power_cc3+=turnoff_factor*power->regfile_power;
#else
  if(regfile_access) {
    if(regfile_access <= (3.0*ruu_commit_width))
      regfile_power_cc1+=power->regfile_power_nobit + regfile_af_b*power->regfile_bitline;
    else
      regfile_power_cc1+=((double)regfile_access/(3.0*(double)ruu_commit_width))*(power->regfile_power_nobit + regfile_af_b*power->regfile_bitline);
    regfile_power_cc2+=((double)regfile_access/(3.0*(double)ruu_commit_width))*(power->regfile_power_nobit + regfile_af_b*power->regfile_bitline);
    regfile_power_cc3+=((double)regfile_access/(3.0*(double)ruu_commit_width))*(power->regfile_power_nobit + regfile_af_b*power->regfile_bitline);

    current_regfile_power_cc3 = ((double)regfile_access/(3.0*(double)ruu_commit_width))*(power->regfile_power_nobit + regfile_af_b*power->regfile_bitline);
  }
else{
    regfile_power_cc3+=turnoff_factor*power->regfile_power;
    
    current_regfile_power_cc3 = turnoff_factor*power->regfile_power;
}
#endif

  if(icache_access) {
    /* don't scale icache because we assume 1 line is fetched, unless fetch stalls */
    //    icache_power_cc1+=power->icache_power+power->itlb;
    //    icache_power_cc2+=power->icache_power+power->itlb;
    //    icache_power_cc3+=power->icache_power+power->itlb;

    //depending on number of cache banks, get appropriate unique bank counter
    int num_banks = 0;
    if(NUM_L1_BANKS == 8){
      num_banks = m_bank_num.num_l1i_8banks;
    }
    else if(NUM_L1_BANKS == 4){
      num_banks = m_bank_num.num_l1i_4banks;
    }
    else if(NUM_L1_BANKS == 2){
      num_banks = m_bank_num.num_l1i_2banks;
    }
    ASSERT(num_banks <= NUM_L1_BANKS);

    //the fraction of banks that are used in this cycle
    double active_bank_ratio = (double) num_banks / (double) NUM_L1_BANKS;

    //compute the power to clock gate the remaining unused banks
    double inactive_bank_power = (1.0 - active_bank_ratio) *turnoff_factor*(power->icache_power+power->itlb);

    //LUKE - changed this to scale!!!!
    if(icache_access <= res_memport){
      icache_power_cc1+=power->icache_power+power->itlb;
    }
    else{
      icache_power_cc1+=((double)icache_access/(double)res_memport)*(power->icache_power +
                                                                     power->itlb);
    }
    icache_power_cc2+=((double)icache_access/(double)res_memport)*(power->icache_power +
                                                                   power->itlb);

    if(NUM_L1_BANKS > 1){
      icache_power_cc3+=( (active_bank_ratio)*((double)icache_access/(double)res_memport)*(power->icache_power +
                                                                                           power->itlb) ) + inactive_bank_power;
      
      current_icache_power_cc3 = ( (active_bank_ratio)*((double)icache_access/(double)res_memport)*(power->icache_power +
                                                                                                    power->itlb) ) + inactive_bank_power;
    }
    else{
      //use original power equations
      icache_power_cc3+=((double)icache_access/(double)res_memport)*(power->icache_power +
                                                                     power->itlb);
      
      current_icache_power_cc3 = ((double)icache_access/(double)res_memport)*(power->icache_power +
                                                                              power->itlb);
    }
  }
  else{
    icache_power_cc3+=turnoff_factor*(power->icache_power+power->itlb);

    current_icache_power_cc3 = turnoff_factor*(power->icache_power+power->itlb);
  }

  if(dcache_access) {
    //depending on number of cache banks, get appropriate unique bank counter
    int num_banks = 0;
    if(NUM_L1_BANKS == 8){
      num_banks = m_bank_num.num_l1d_8banks;
    }
    else if(NUM_L1_BANKS == 4){
      num_banks = m_bank_num.num_l1d_4banks;
    }
    else if(NUM_L1_BANKS == 2){
      num_banks = m_bank_num.num_l1d_2banks;
    }
    ASSERT(num_banks <= NUM_L1_BANKS);
    
    //the fraction of banks that are used in this cycle
    double active_bank_ratio = (double) num_banks / (double) NUM_L1_BANKS;
    
    //compute the power to clock gate the remaining unused banks
    double inactive_bank_power = (1.0 - active_bank_ratio) *turnoff_factor*(power->dcache_power+power->dtlb);

    if(dcache_access <= res_memport)
      dcache_power_cc1+=power->dcache_power+power->dtlb;
    else
      dcache_power_cc1+=((double)dcache_access/(double)res_memport)*(power->dcache_power +
                                                     power->dtlb);
    dcache_power_cc2+=((double)dcache_access/(double)res_memport)*(power->dcache_power +
                                                   power->dtlb);
    
    if(NUM_L1_BANKS > 1){
      dcache_power_cc3+=( (active_bank_ratio)*((double)dcache_access/(double)res_memport)*(power->dcache_power +
                                                                                           power->dtlb)) + inactive_bank_power;
      
      current_dcache_power_cc3 = ( (active_bank_ratio)*((double)dcache_access/(double)res_memport)*(power->dcache_power +
                                                                                                    power->dtlb) ) + inactive_bank_power;
    }
    else{
      //use original power equations, no banking used
       dcache_power_cc3+=((double)dcache_access/(double)res_memport)*(power->dcache_power +
                                                                      power->dtlb);
      
      current_dcache_power_cc3 = ((double)dcache_access/(double)res_memport)*(power->dcache_power +
                                                                              power->dtlb);
    }
  }
else{
    dcache_power_cc3+=turnoff_factor*(power->dcache_power+power->dtlb);

    current_dcache_power_cc3 = turnoff_factor*(power->dcache_power+power->dtlb);
}

  if(dcache2_access) {
    if(dcache2_access <= res_memport)
      dcache2_power_cc1+=power->dcache2_power;
    else
      dcache2_power_cc1+=((double)dcache2_access/(double)res_memport)*power->dcache2_power;
    dcache2_power_cc2+=((double)dcache2_access/(double)res_memport)*power->dcache2_power;
    dcache2_power_cc3+=((double)dcache2_access/(double)res_memport)*power->dcache2_power;

    current_dcache2_power_cc3 = ((double)dcache2_access/(double)res_memport)*power->dcache2_power;
  }
else{
    dcache2_power_cc3+=turnoff_factor*power->dcache2_power;

    current_dcache2_power_cc3 = turnoff_factor*power->dcache2_power;
}

  if(alu_access) {
    if(ialu_access)
      alu_power_cc1+=power->ialu_power;
    else{
      alu_power_cc3+=turnoff_factor*power->ialu_power;
      current_alu_power_cc3 += turnoff_factor*power->ialu_power;
    }
    if(falu_access)
      alu_power_cc1+=power->falu_power;
    else{
      alu_power_cc3+=turnoff_factor*power->falu_power;
      current_alu_power_cc3 += turnoff_factor*power->falu_power;
    }

    alu_power_cc2+=((double)ialu_access/(double)res_ialu)*power->ialu_power +
      ((double)falu_access/(double)res_fpalu)*power->falu_power;
    alu_power_cc3+=((double)ialu_access/(double)res_ialu)*power->ialu_power +
      ((double)falu_access/(double)res_fpalu)*power->falu_power;

    current_alu_power_cc3 += ((double)ialu_access/(double)res_ialu)*power->ialu_power +
      ((double)falu_access/(double)res_fpalu)*power->falu_power;
  }
else{
    alu_power_cc3+=turnoff_factor*(power->ialu_power + power->falu_power);

    current_alu_power_cc3 += turnoff_factor*(power->ialu_power + power->falu_power);
}

  //COMPRESSION power, compute like int alu access
  if(compression_access) {
      compression_power_cc1+=power->compression_power;

      compression_power_cc2+=((double)compression_access/(double)compression_alus)*power->compression_power;
      compression_power_cc3+=((double)compression_access/(double)compression_alus)*power->compression_power;

      current_compression_power_cc3 = ((double)compression_access/(double)compression_alus)*power->compression_power;
  }
  else{
    compression_power_cc3+=turnoff_factor*power->compression_power;

    current_compression_power_cc3 = turnoff_factor*power->compression_power;
  }

#ifdef STATIC_AF
  if(resultbus_access) {
    assert(ruu_issue_width != 0);
    if(resultbus_access <= ruu_issue_width) {
      resultbus_power_cc1+=power->resultbus;
    }
    else {
      resultbus_power_cc1+=((double)resultbus_access/(double)ruu_issue_width)*power->resultbus;
    }
    resultbus_power_cc2+=((double)resultbus_access/(double)ruu_issue_width)*power->resultbus;
    resultbus_power_cc3+=((double)resultbus_access/(double)ruu_issue_width)*power->resultbus;

  }
  else
    resultbus_power_cc3+=turnoff_factor*power->resultbus;
#else
  if(resultbus_access) {
    assert(ruu_issue_width != 0);
    if(resultbus_access <= ruu_issue_width) {
      resultbus_power_cc1+=resultbus_af_b*power->resultbus;
    }
    else {
      resultbus_power_cc1+=((double)resultbus_access/(double)ruu_issue_width)*resultbus_af_b*power->resultbus;
    }
    resultbus_power_cc2+=((double)resultbus_access/(double)ruu_issue_width)*resultbus_af_b*power->resultbus;
    resultbus_power_cc3+=((double)resultbus_access/(double)ruu_issue_width)*resultbus_af_b*power->resultbus;

    current_resultbus_power_cc3 = ((double)resultbus_access/(double)ruu_issue_width)*resultbus_af_b*power->resultbus;
  }
else{
    resultbus_power_cc3+=turnoff_factor*power->resultbus;

    current_resultbus_power_cc3 = turnoff_factor*power->resultbus;
}
#endif

  total_cycle_power = rename_power + bpred_power + window_power + 
    lsq_power + regfile_power + icache_power + dcache_power + dcache2_power +
    alu_power + resultbus_power + instr_prefetcher_power + data_prefetcher_power + compression_power;

  total_cycle_power_cc1 = rename_power_cc1 + bpred_power_cc1 + 
    window_power_cc1 + lsq_power_cc1 + regfile_power_cc1 + 
    icache_power_cc1 + dcache_power_cc1 + + dcache2_power_cc1 + alu_power_cc1 +
    resultbus_power_cc1  + instr_prefetcher_power_cc1 + data_prefetcher_power_cc1 + compression_power_cc1;

  total_cycle_power_cc2 = rename_power_cc2 + bpred_power_cc2 + 
    window_power_cc2 + lsq_power_cc2 + regfile_power_cc2 + 
    icache_power_cc2 + dcache_power_cc2 + dcache2_power_cc2 + alu_power_cc2 + 
    resultbus_power_cc2  + instr_prefetcher_power_cc2 + data_prefetcher_power_cc2 + compression_power_cc2;

  total_cycle_power_cc3 = rename_power_cc3 + bpred_power_cc3 + 
    window_power_cc3 + lsq_power_cc3 + regfile_power_cc3 + 
    icache_power_cc3 + dcache_power_cc3 + dcache2_power_cc3 + alu_power_cc3 + 
    resultbus_power_cc3  + instr_prefetcher_power_cc3 + data_prefetcher_power_cc3 + compression_power_cc3;

  clock_power_cc1+=power->clock_power*(total_cycle_power_cc1/total_cycle_power);
  clock_power_cc2+=power->clock_power*(total_cycle_power_cc2/total_cycle_power);
  clock_power_cc3+=power->clock_power*(total_cycle_power_cc3/total_cycle_power);

  local_current_clock_power_cc3 = power->clock_power*(total_cycle_power_cc3/total_cycle_power);

  //calculate the max power for each cc3 part
  max_rename_power_cc3=MAX(max_rename_power_cc3, current_rename_power_cc3);
  max_bpred_power_cc3=MAX(max_bpred_power_cc3, current_bpred_power_cc3);
  max_window_power_cc3=MAX(max_window_power_cc3, current_window_power_cc3);
  max_lsq_power_cc3=MAX(max_lsq_power_cc3, current_lsq_power_cc3);
  max_regfile_power_cc3=MAX(max_regfile_power_cc3, current_regfile_power_cc3);
  max_icache_power_cc3=MAX(max_icache_power_cc3, current_icache_power_cc3);
  max_dcache_power_cc3=MAX(max_dcache_power_cc3, current_dcache_power_cc3);
  max_dcache2_power_cc3=MAX(max_dcache2_power_cc3, current_dcache2_power_cc3);
  max_alu_power_cc3=MAX(max_alu_power_cc3, current_alu_power_cc3);
  max_resultbus_power_cc3=MAX(max_resultbus_power_cc3, current_resultbus_power_cc3);
  max_current_clock_power_cc3=MAX(max_current_clock_power_cc3, local_current_clock_power_cc3);
  max_instr_prefetcher_power_cc3 = MAX(max_instr_prefetcher_power_cc3, current_instr_prefetcher_power_cc3);
  max_data_prefetcher_power_cc3 = MAX(max_data_prefetcher_power_cc3, current_data_prefetcher_power_cc3);
  max_compression_power_cc3 = MAX(max_compression_power_cc3, current_compression_power_cc3);

  

  current_total_cycle_power_no_clock_cc1 = total_cycle_power_cc1
    -last_single_total_cycle_power_no_clock_cc1;
  current_total_cycle_power_no_clock_cc2 = total_cycle_power_cc2
    -last_single_total_cycle_power_no_clock_cc2;
  current_total_cycle_power_no_clock_cc3 = total_cycle_power_cc3
    -last_single_total_cycle_power_no_clock_cc3;

  last_single_total_cycle_power_no_clock_cc1 = total_cycle_power_cc1;
  last_single_total_cycle_power_no_clock_cc2 = total_cycle_power_cc2;
  last_single_total_cycle_power_no_clock_cc3 = total_cycle_power_cc3;

  total_cycle_power_cc1 += clock_power_cc1;
  total_cycle_power_cc2 += clock_power_cc2;
  total_cycle_power_cc3 += clock_power_cc3;

  current_total_cycle_power_cc1 = total_cycle_power_cc1
    -last_single_total_cycle_power_cc1;
  current_total_cycle_power_cc2 = total_cycle_power_cc2
    -last_single_total_cycle_power_cc2;
  current_total_cycle_power_cc3 = total_cycle_power_cc3
    -last_single_total_cycle_power_cc3;

  //find the current clock power...
  current_clock_power_cc1 = clock_power_cc1 - last_clock_power_cc1;
  current_clock_power_cc2 = clock_power_cc2 - last_clock_power_cc2;
  current_clock_power_cc3 = clock_power_cc3 - last_clock_power_cc3;
  
  max_cycle_power =  MAX(max_cycle_power,total_cycle_power);
  max_cycle_power_cc1 = MAX(max_cycle_power_cc1,current_total_cycle_power_cc1);
  max_cycle_power_cc2 = MAX(max_cycle_power_cc2,current_total_cycle_power_cc2);
  max_cycle_power_cc3 = MAX(max_cycle_power_cc3,current_total_cycle_power_cc3);

  //for no clock max cycle power:
  max_cycle_power_no_clock_cc1 = MAX(max_cycle_power_no_clock_cc1,current_total_cycle_power_no_clock_cc1);
  max_cycle_power_no_clock_cc2 = MAX(max_cycle_power_no_clock_cc2,current_total_cycle_power_no_clock_cc2);
  max_cycle_power_no_clock_cc3 = MAX(max_cycle_power_no_clock_cc3,current_total_cycle_power_no_clock_cc3);

  max_clock_power_cc1 = MAX(max_clock_power_cc1, current_clock_power_cc1);
  max_clock_power_cc2 = MAX(max_clock_power_cc2, current_clock_power_cc2);
  max_clock_power_cc3 = MAX(max_clock_power_cc3, current_clock_power_cc3);

  last_clock_power_cc1 = current_clock_power_cc1;
  last_clock_power_cc2 = current_clock_power_cc2;
  last_clock_power_cc3 = current_clock_power_cc3;


  last_single_total_cycle_power_cc1 = total_cycle_power_cc1;
  last_single_total_cycle_power_cc2 = total_cycle_power_cc2;
  last_single_total_cycle_power_cc3 = total_cycle_power_cc3;

  //cout << "update power stats END" << endl;

}

void
power_t::power_reg_stats(const tick_t & sim_cycle, const uint64 & sim_total_insn)        
{

#if 1
  m_pseq->out_info(" total power usage of rename unit: %6.2lf\n", rename_power);

  m_pseq->out_info(" total power usage of bpred unit: %6.2lf\n", bpred_power);

  m_pseq->out_info(" total power usage of instruction window: %6.2lf\n", window_power);

  m_pseq->out_info(" total power usage of LSQ: %6.2lf\n", lsq_power);

  m_pseq->out_info(" total power usage of arch. regfile: %6.2lf\n", regfile_power);

  m_pseq->out_info(" total power usage of L1Icache: %6.2lf\n", icache_power);

  m_pseq->out_info(" total power usage of L1Dcache: %6.2lf\n", dcache_power);

  m_pseq->out_info(" total power usage of L2cache: %6.2lf\n", dcache2_power);

  m_pseq->out_info( " total power usage of alu: %6.2lf\n", alu_power);

  m_pseq->out_info(" total power usage of falu: %6.2lf\n", falu_power);

  m_pseq->out_info(" total power usage of resultbus: %6.2lf\n", resultbus_power);

  m_pseq->out_info(" total power usage of clock: %6.2lf\n", clock_power);

  m_pseq->out_info(" total power usage of instr prefetcher: %6.2lf\n", instr_prefetcher_power);

  m_pseq->out_info(" total power usage of data prefetcher: %6.2lf\n", data_prefetcher_power);

  m_pseq->out_info(" total power usage of compression alus: %6.2lf\n", compression_power);

  m_pseq->out_info( " avg power usage of rename unit: %6.2lf\n", rename_power/sim_cycle);

  m_pseq->out_info( " avg power usage of bpred unit: %6.2lf\n", bpred_power/sim_cycle);

  m_pseq->out_info(" avg power usage of instruction window: %6.2lf\n", window_power/sim_cycle);

  m_pseq->out_info( " avg power usage of lsq: %6.2lf\n", lsq_power/sim_cycle);

  m_pseq->out_info( " avg power usage of arch. regfile: %6.2lf\n", regfile_power/sim_cycle);

  m_pseq->out_info( " avg power usage of L1Icache: %6.2lf\n", icache_power/sim_cycle);

  m_pseq->out_info( " avg power usage of L1Dcache: %6.2lf\n", dcache_power/sim_cycle);

  m_pseq->out_info( " avg power usage of L2cache: %6.2lf\n", dcache2_power/sim_cycle);

  m_pseq->out_info( " avg power usage of alu: %6.2lf\n", alu_power/sim_cycle);

  m_pseq->out_info( " avg power usage of falu: %6.2lf\n", falu_power/sim_cycle);

  m_pseq->out_info( " avg power usage of resultbus: %6.2lf\n", resultbus_power/sim_cycle);

  m_pseq->out_info( " avg power usage of clock: %6.2lf\n", clock_power/sim_cycle);

  m_pseq->out_info( " avg power usage of instr prefetcher: %6.2lf\n", instr_prefetcher_power/sim_cycle);

  m_pseq->out_info( " avg power usage of data prefetcher: %6.2lf\n", data_prefetcher_power/sim_cycle);

  m_pseq->out_info( " avg power usage of compression alus: %6.2lf\n", compression_power/sim_cycle);

  m_pseq->out_info( " total power usage of fetch stage: %6.2lf\n", icache_power + bpred_power);

  m_pseq->out_info( " total power usage of dispatch stage: %6.2lf\n", rename_power);

  m_pseq->out_info( " total power usage of issue stage: %6.2lf\n", resultbus_power + alu_power + dcache_power + dcache2_power + window_power + lsq_power + instr_prefetcher_power + data_prefetcher_power +
     compression_power);

  m_pseq->out_info(" average power of fetch unit per cycle: %6.2lf\n", (icache_power + bpred_power)/ sim_cycle);

  m_pseq->out_info( " average power of dispatch unit per cycle: %6.2lf\n", (rename_power)/ sim_cycle);

  m_pseq->out_info( " average power of issue unit per cycle: %6.2lf\n", (resultbus_power + alu_power + dcache_power + dcache2_power + window_power + lsq_power + instr_prefetcher_power + data_prefetcher_power + compression_power)/ sim_cycle);

  m_pseq->out_info( " total power per cycle: %6.2lf\n",(rename_power + bpred_power + window_power + lsq_power + regfile_power + icache_power  + resultbus_power + clock_power + alu_power + dcache_power + dcache2_power + instr_prefetcher_power + data_prefetcher_power + compression_power));

  m_pseq->out_info( " average total power per cycle: %6.2lf\n",(rename_power + bpred_power + window_power + lsq_power + regfile_power + icache_power + resultbus_power + clock_power + alu_power + dcache_power + dcache2_power + instr_prefetcher_power + data_prefetcher_power + compression_power)/sim_cycle);

  m_pseq->out_info( " average total power per cycle w/o FP and L2 power: %6.2lf\n",(rename_power + bpred_power + window_power + lsq_power + regfile_power + icache_power + resultbus_power + clock_power + alu_power + dcache_power + instr_prefetcher_power + data_prefetcher_power + compression_power - falu_power )/sim_cycle);

  m_pseq->out_info( " average total power per insn: %6.2lf\n",(rename_power + bpred_power + window_power + lsq_power + regfile_power + icache_power + resultbus_power + clock_power + alu_power + dcache_power + dcache2_power + instr_prefetcher_power + data_prefetcher_power + compression_power)/sim_total_insn);

  m_pseq->out_info( " average total power per insn w/o FP and L2 power: %6.2lf\n",(rename_power + bpred_power + window_power + lsq_power + regfile_power + icache_power + resultbus_power + clock_power + alu_power + dcache_power + instr_prefetcher_power + data_prefetcher_power + compression_power - falu_power )/sim_total_insn);

  m_pseq->out_info( " total power usage of rename unit_cc1: %6.2lf\n", rename_power_cc1);

  m_pseq->out_info( " total power usage of bpred unit_cc1: %6.2lf\n", bpred_power_cc1);

  m_pseq->out_info( " total power usage of instruction window_cc1: %6.2lf\n", window_power_cc1);

  m_pseq->out_info( " total power usage of lsq_cc1: %6.2lf\n", lsq_power_cc1);

  m_pseq->out_info( " total power usage of arch. regfile_cc1: %6.2lf\n", regfile_power_cc1);

  m_pseq->out_info( " total power usage of L1Icache_cc1: %6.2lf\n", icache_power_cc1);

  m_pseq->out_info( " total power usage of L1Dcache_cc1: %6.2lf\n", dcache_power_cc1);

  m_pseq->out_info( " total power usage of L2cache_cc1: %6.2lf\n", dcache2_power_cc1);

  m_pseq->out_info( " total power usage of alu_cc1: %6.2lf\n", alu_power_cc1);

  m_pseq->out_info( " total power usage of resultbus_cc1: %6.2lf\n", resultbus_power_cc1);

  m_pseq->out_info( " total power usage of clock_cc1: %6.2lf\n", clock_power_cc1);

  m_pseq->out_info( " total power usage of instr_prefetcher_cc1: %6.2lf\n", instr_prefetcher_power_cc1);

  m_pseq->out_info( " total power usage of data_prefetcher_cc1: %6.2lf\n", data_prefetcher_power_cc1);

  m_pseq->out_info(" total power usage of compression alus_cc1: %6.2lf\n", compression_power_cc1);

  m_pseq->out_info( " avg power usage of rename unit_cc1: %6.2lf\n", rename_power_cc1/sim_cycle);

  m_pseq->out_info( " avg power usage of bpred unit_cc1: %6.2lf\n", bpred_power_cc1/sim_cycle);

  m_pseq->out_info( " avg power usage of instruction window_cc1: %6.2lf\n", window_power_cc1/sim_cycle);

  m_pseq->out_info( " avg power usage of lsq_cc1: %6.2lf\n", lsq_power_cc1/sim_cycle);

  m_pseq->out_info( " avg power usage of arch. regfile_cc1: %6.2lf\n", regfile_power_cc1/sim_cycle);

  m_pseq->out_info( " avg power usage of L1Icache_cc1: %6.2lf\n", icache_power_cc1/sim_cycle);

  m_pseq->out_info( " avg power usage of L1Dcache_cc1: %6.2lf\n", dcache_power_cc1/sim_cycle);

  m_pseq->out_info( " avg power usage of L2cache_cc1: %6.2lf\n", dcache2_power_cc1/sim_cycle);

  m_pseq->out_info( " avg power usage of alu_cc1: %6.2lf\n", alu_power_cc1/sim_cycle);

  m_pseq->out_info( " avg power usage of resultbus_cc1: %6.2lf\n", resultbus_power_cc1/sim_cycle);

  m_pseq->out_info( " avg power usage of clock_cc1: %6.2lf\n", clock_power_cc1/sim_cycle);

  m_pseq->out_info( " avg power usage of instr_prefetcher_cc1: %6.2lf\n", instr_prefetcher_power_cc1/sim_cycle);

  m_pseq->out_info( " avg power usage of data_prefetcher_cc1: %6.2lf\n", data_prefetcher_power_cc1/sim_cycle);

  m_pseq->out_info( " avg power usage of compression alus_cc1: %6.2lf\n", compression_power_cc1/sim_cycle);

  m_pseq->out_info( " total power usage of fetch stage_cc1: %6.2lf\n", icache_power_cc1 + bpred_power_cc1);

  m_pseq->out_info( " total power usage of dispatch stage_cc1: %6.2lf\n", rename_power_cc1);

  m_pseq->out_info( " total power usage of issue stage_cc1: %6.2lf\n", resultbus_power_cc1 + alu_power_cc1 + dcache_power_cc1 + dcache2_power_cc1 + lsq_power_cc1 + window_power_cc1 + instr_prefetcher_power_cc1 + data_prefetcher_power_cc1 + compression_power_cc1);

  m_pseq->out_info( " average power of fetch unit per cycle_cc1: %6.2lf\n", (icache_power_cc1 + bpred_power_cc1)/ sim_cycle);

  m_pseq->out_info( " average power of dispatch unit per cycle_cc1: %6.2lf\n", (rename_power_cc1)/ sim_cycle);

  m_pseq->out_info( " average power of issue unit per cycle_cc1: %6.2lf\n", (resultbus_power_cc1 + alu_power_cc1 + dcache_power_cc1 + dcache2_power_cc1 + lsq_power_cc1 + window_power_cc1 + instr_prefetcher_power_cc1 + data_prefetcher_power_cc1 + compression_power_cc1)/ sim_cycle);

  m_pseq->out_info( " total power per cycle_cc1: %6.2lf\n",(rename_power_cc1 + bpred_power_cc1 + lsq_power_cc1 + window_power_cc1 + regfile_power_cc1 + icache_power_cc1 + resultbus_power_cc1 + clock_power_cc1 + alu_power_cc1 + dcache_power_cc1 + dcache2_power_cc1 + instr_prefetcher_power_cc1 + data_prefetcher_power_cc1 + compression_power_cc1));

  m_pseq->out_info( " average total power per cycle_cc1: %6.2lf\n",(rename_power_cc1 + bpred_power_cc1 + lsq_power_cc1 + window_power_cc1 + regfile_power_cc1 + icache_power_cc1 + resultbus_power_cc1 + clock_power_cc1 + alu_power_cc1 + dcache_power_cc1 +dcache2_power_cc1+ instr_prefetcher_power_cc1 + data_prefetcher_power_cc1 + compression_power_cc1)/sim_cycle);

  m_pseq->out_info( " average total power per insn_cc1: %6.2lf\n",(rename_power_cc1 + bpred_power_cc1 + lsq_power_cc1 + window_power_cc1 + regfile_power_cc1 + icache_power_cc1 + resultbus_power_cc1 + clock_power_cc1 +  alu_power_cc1 + dcache_power_cc1 + dcache2_power_cc1+ instr_prefetcher_power_cc1 + data_prefetcher_power_cc1 + compression_power_cc1)/sim_total_insn);

  m_pseq->out_info( " total power usage of rename unit_cc2: %6.2lf\n", rename_power_cc2);

  m_pseq->out_info( " total power usage of bpred unit_cc2: %6.2lf\n", bpred_power_cc2);

  m_pseq->out_info( " total power usage of instruction window_cc2: %6.2lf\n", window_power_cc2);

  m_pseq->out_info( " total power usage of lsq_cc2: %6.2lf\n", lsq_power_cc2);

  m_pseq->out_info( " total power usage of arch. regfile_cc2: %6.2lf\n", regfile_power_cc2);

  m_pseq->out_info( " total power usage of L1Icache_cc2: %6.2lf\n", icache_power_cc2);

  m_pseq->out_info( " total power usage of L1Dcache_cc2: %6.2lf\n", dcache_power_cc2);

  m_pseq->out_info( " total power usage of L2cache_cc2: %6.2lf\n", dcache2_power_cc2);

  m_pseq->out_info( " total power usage of alu_cc2: %6.2lf\n", alu_power_cc2);

  m_pseq->out_info( " total power usage of resultbus_cc2: %6.2lf\n", resultbus_power_cc2);

  m_pseq->out_info( " total power usage of clock_cc2: %6.2lf\n", clock_power_cc2);

  m_pseq->out_info( " total power usage of instr_prefetcher_cc2: %6.2lf\n", instr_prefetcher_power_cc2);

  m_pseq->out_info( " total power usage of data_prefetcher_cc2: %6.2lf\n", data_prefetcher_power_cc2);

  m_pseq->out_info(" total power usage of compression alus_cc2: %6.2lf\n", compression_power_cc2);

  m_pseq->out_info( " avg power usage of rename unit_cc2: %6.2lf\n", rename_power_cc2/sim_cycle);

  m_pseq->out_info( " avg power usage of bpred unit_cc2: %6.2lf\n", bpred_power_cc2/sim_cycle);

  m_pseq->out_info( " avg power usage of instruction window_cc2: %6.2lf\n", window_power_cc2/sim_cycle);

  m_pseq->out_info( " avg power usage of instruction lsq_cc2: %6.2lf\n", lsq_power_cc2/sim_cycle);

  m_pseq->out_info( " avg power usage of arch. regfile_cc2: %6.2lf\n", regfile_power_cc2/sim_cycle);

  m_pseq->out_info( " avg power usage of L1Icache_cc2: %6.2lf\n", icache_power_cc2/sim_cycle);

  m_pseq->out_info( " avg power usage of L1Dcache_cc2: %6.2lf\n", dcache_power_cc2/sim_cycle);

  m_pseq->out_info( " avg power usage of L2cache_cc2: %6.2lf\n", dcache2_power_cc2/sim_cycle);

  m_pseq->out_info(" avg power usage of alu_cc2: %6.2lf\n", alu_power_cc2/sim_cycle);

  m_pseq->out_info( " avg power usage of resultbus_cc2: %6.2lf\n", resultbus_power_cc2/sim_cycle);

  m_pseq->out_info( " avg power usage of clock_cc2: %6.2lf\n", clock_power_cc2/sim_cycle);

  m_pseq->out_info( " avg power usage of instr_prefetcher_cc2: %6.2lf\n", instr_prefetcher_power_cc2/sim_cycle);

  m_pseq->out_info( " avg power usage of data_prefetcher_cc2: %6.2lf\n", data_prefetcher_power_cc2/sim_cycle);

  m_pseq->out_info( " avg power usage of compression alus_cc2: %6.2lf\n", compression_power_cc2/sim_cycle);

  m_pseq->out_info( " total power usage of fetch stage_cc2: %6.2lf\n", icache_power_cc2 + bpred_power_cc2);

  m_pseq->out_info( " total power usage of dispatch stage_cc2: %6.2lf\n", rename_power_cc2);

  m_pseq->out_info( " total power usage of issue stage_cc2: %6.2lf\n", resultbus_power_cc2 + alu_power_cc2 + dcache_power_cc2 + dcache2_power_cc2 + lsq_power_cc2 + window_power_cc2 + instr_prefetcher_power_cc2 + data_prefetcher_power_cc2 + compression_power_cc2);

  m_pseq->out_info( " average power of fetch unit per cycle_cc2: %6.2lf\n", (icache_power_cc2 + bpred_power_cc2)/ sim_cycle);

  m_pseq->out_info( " average power of dispatch unit per cycle_cc2: %6.2lf\n", (rename_power_cc2)/ sim_cycle);

  m_pseq->out_info(" average power of issue unit per cycle_cc2: %6.2lf\n", (resultbus_power_cc2 + alu_power_cc2 + dcache_power_cc2 + dcache2_power_cc2 + lsq_power_cc2 + window_power_cc2 + instr_prefetcher_power_cc2 + data_prefetcher_power_cc2 + compression_power_cc2)/ sim_cycle);

  m_pseq->out_info( " total power per cycle_cc2: %6.2lf\n",(rename_power_cc2 + bpred_power_cc2 + lsq_power_cc2 + window_power_cc2 + regfile_power_cc2 + icache_power_cc2 + resultbus_power_cc2 + clock_power_cc2 + alu_power_cc2 + dcache_power_cc2 + dcache2_power_cc2 + instr_prefetcher_power_cc2 + data_prefetcher_power_cc2 + compression_power_cc2));

  m_pseq->out_info(" average total power per cycle_cc2: %6.2lf\n",(rename_power_cc2 + bpred_power_cc2 + lsq_power_cc2 + window_power_cc2 + regfile_power_cc2 + icache_power_cc2 + resultbus_power_cc2 + clock_power_cc2 + alu_power_cc2 + dcache_power_cc2 + dcache2_power_cc2 + instr_prefetcher_power_cc2 + data_prefetcher_power_cc2 + compression_power_cc2)/sim_cycle);

  m_pseq->out_info( " average total power per insn_cc2: %6.2lf\n",(rename_power_cc2 + bpred_power_cc2 + lsq_power_cc2 + window_power_cc2 + regfile_power_cc2 + icache_power_cc2 + resultbus_power_cc2 + clock_power_cc2 + alu_power_cc2 + dcache_power_cc2 + dcache2_power_cc2 + instr_prefetcher_power_cc2 + data_prefetcher_power_cc2 + compression_power_cc2)/sim_total_insn);

  m_pseq->out_info(" total power usage of rename unit_cc3: %6.2lf\n", rename_power_cc3);

  m_pseq->out_info( " total power usage of bpred unit_cc3: %6.2lf\n", bpred_power_cc3);

  m_pseq->out_info(" total power usage of instruction window_cc3: %6.2lf\n", window_power_cc3);

  m_pseq->out_info( " total power usage of lsq_cc3: %6.2lf\n", lsq_power_cc3);

  m_pseq->out_info(" total power usage of arch. regfile_cc3: %6.2lf\n", regfile_power_cc3);

  m_pseq->out_info( " total power usage of L1Icache_cc3: %6.2lf\n", icache_power_cc3);

  m_pseq->out_info( " total power usage of L1Dcache_cc3: %6.2lf\n", dcache_power_cc3);

  m_pseq->out_info( " total power usage of L2cache_cc3: %6.2lf\n", dcache2_power_cc3);

  m_pseq->out_info( " total power usage of alu_cc3: %6.2lf\n", alu_power_cc3);

  m_pseq->out_info( " total power usage of resultbus_cc3: %6.2lf\n", resultbus_power_cc3);

  m_pseq->out_info( " total power usage of clock_cc3: %6.2lf\n", clock_power_cc3);

  m_pseq->out_info( " total power usage of instr_prefetcher_cc3: %6.2lf\n", instr_prefetcher_power_cc3);
  
  m_pseq->out_info( " total power usage of data_prefetcher_cc3: %6.2lf\n", data_prefetcher_power_cc3);

  m_pseq->out_info(" total power usage of compression alus_cc3: %6.2lf\n", compression_power_cc3);

  m_pseq->out_info(" avg power usage of rename unit_cc3: %6.2lf\n", rename_power_cc3/sim_cycle);

  m_pseq->out_info( " avg power usage of bpred unit_cc3: %6.2lf\n", bpred_power_cc3/sim_cycle);

  m_pseq->out_info( " avg power usage of instruction window_cc3: %6.2lf\n", window_power_cc3/sim_cycle);

  m_pseq->out_info( " avg power usage of instruction lsq_cc3: %6.2lf\n", lsq_power_cc3/sim_cycle);

  m_pseq->out_info( " avg power usage of arch. regfile_cc3: %6.2lf\n", regfile_power_cc3/sim_cycle);

  m_pseq->out_info( " avg power usage of L1Icache_cc3: %6.2lf\n", icache_power_cc3/sim_cycle);

  m_pseq->out_info( " avg power usage of L1Dcache_cc3: %6.2lf\n", dcache_power_cc3/sim_cycle);

  m_pseq->out_info( " avg power usage of L2cache_cc3: %6.2lf\n", dcache2_power_cc3/sim_cycle);

  m_pseq->out_info( " avg power usage of alu_cc3: %6.2lf\n", alu_power_cc3/sim_cycle);

  m_pseq->out_info( " avg power usage of resultbus_cc3: %6.2lf\n", resultbus_power_cc3/sim_cycle);

  m_pseq->out_info( " avg power usage of clock_cc3: %6.2lf\n", clock_power_cc3/sim_cycle);

  m_pseq->out_info( " avg power usage of instr_prefetcher_cc3: %6.2lf\n", instr_prefetcher_power_cc3/sim_cycle);

  m_pseq->out_info( " avg power usage of data_prefetcher_cc3: %6.2lf\n", data_prefetcher_power_cc3/sim_cycle);

  m_pseq->out_info( " avg power usage of compression alus_cc3: %6.2lf\n", compression_power_cc3/sim_cycle);

  m_pseq->out_info( " total power usage of fetch stage_cc3: %6.2lf\n", icache_power_cc3 + bpred_power_cc3);

  m_pseq->out_info( " total power usage of dispatch stage_cc3: %6.2lf\n", rename_power_cc3);

  m_pseq->out_info( " total power usage of issue stage_cc3: %6.2lf\n", resultbus_power_cc3 + alu_power_cc3 + dcache_power_cc3 + dcache2_power_cc3 + lsq_power_cc3 + window_power_cc3 + instr_prefetcher_power_cc3 + data_prefetcher_power_cc3 + compression_power_cc3);

  m_pseq->out_info( " average power of fetch unit per cycle_cc3: %6.2lf\n", (icache_power_cc3 + bpred_power_cc3)/ sim_cycle);

  m_pseq->out_info( " average power of dispatch unit per cycle_cc3: %6.2lf\n", (rename_power_cc3)/ sim_cycle);

  m_pseq->out_info( " average power of issue unit per cycle_cc3: %6.2lf\n", (resultbus_power_cc3 + alu_power_cc3 + dcache_power_cc3 + dcache2_power_cc3 + lsq_power_cc3 + window_power_cc3 + instr_prefetcher_power_cc3 + data_prefetcher_power_cc3 + compression_power_cc3)/ sim_cycle);

  m_pseq->out_info( " total power per cycle_cc3: %6.2lf\n",(rename_power_cc3 + bpred_power_cc3 + lsq_power_cc3 + window_power_cc3 + regfile_power_cc3 + icache_power_cc3 + resultbus_power_cc3 + clock_power_cc3 + alu_power_cc3 + dcache_power_cc3 + dcache2_power_cc3 + instr_prefetcher_power_cc3 + data_prefetcher_power_cc3 + compression_power_cc3));

  m_pseq->out_info( " average total power per cycle_cc3: %6.2lf\n",(rename_power_cc3 + bpred_power_cc3 + lsq_power_cc3 + window_power_cc3 + regfile_power_cc3 + icache_power_cc3 + resultbus_power_cc3 + clock_power_cc3 + alu_power_cc3 + dcache_power_cc3 + dcache2_power_cc3 + instr_prefetcher_power_cc3 + data_prefetcher_power_cc3 + compression_power_cc3)/sim_cycle);

  m_pseq->out_info( " average total power per insn_cc3: %6.2lf\n",(rename_power_cc3 + bpred_power_cc3 + lsq_power_cc3 + window_power_cc3 + regfile_power_cc3 + icache_power_cc3 + resultbus_power_cc3 + clock_power_cc3 + alu_power_cc3 + dcache_power_cc3 + dcache2_power_cc3 + instr_prefetcher_power_cc3 + data_prefetcher_power_cc3 + compression_power_cc3)/sim_total_insn);

  m_pseq->out_info( " total number accesses of rename unit: %lld\n", total_rename_access);

  m_pseq->out_info( " total number accesses of bpred unit: %lld\n", total_bpred_access);

  m_pseq->out_info( " total number accesses of instruction window: %lld\n", total_window_access);

  m_pseq->out_info( " total number accesses of LSQ: %lld\n", total_lsq_access);

  m_pseq->out_info( " total number accesses of arch. regfile: %lld\n", total_regfile_access);

  m_pseq->out_info( " total number accesses of L1Icache: %lld\n", total_icache_access);

  m_pseq->out_info( " total number accesses of L1Dcache: %lld\n", total_dcache_access);

  m_pseq->out_info( " total number accesses of L2cache: %lld\n", total_dcache2_access);

  m_pseq->out_info( " total number accesses of alu: %lld\n", total_alu_access);

  m_pseq->out_info( " total number accesses of resultbus: %lld\n", total_resultbus_access);

  m_pseq->out_info( " total number accesses of instr prefetcher: %lld\n", total_instr_prefetcher_access);

  m_pseq->out_info( " total number accesses of data prefetcher: %lld\n", total_data_prefetcher_access);

  m_pseq->out_info( " total number accesses of compression ALUs: %lld\n", total_compression_access);

  m_pseq->out_info( " avg number accesses of rename unit: %6.2lf\n", (total_rename_access*1.0)/sim_cycle);

  m_pseq->out_info( " avg number accesses of bpred unit: %6.2lf\n", (total_bpred_access*1.0)/sim_cycle);

  m_pseq->out_info(" avg number accesses of instruction window: %6.2lf\n", (total_window_access*1.0)/sim_cycle);

  m_pseq->out_info( " avg number accesses of lsq: %6.2lf\n", (total_lsq_access*1.0)/sim_cycle);

  m_pseq->out_info( " avg number accesses of arch. regfile: %6.2lf\n", (total_regfile_access*1.0)/sim_cycle);

  m_pseq->out_info( " avg number accesses of L1Icache: %6.2lf\n", (total_icache_access*1.0)/sim_cycle);

  m_pseq->out_info( " avg number accesses of L1Dcache: %6.2lf\n", (total_dcache_access*1.0)/sim_cycle);

  m_pseq->out_info( " avg number accesses of L2cache: %6.2lf\n", (total_dcache2_access*1.0)/sim_cycle);

  m_pseq->out_info( " avg number accesses of alu: %6.2lf\n", (total_alu_access*1.0)/sim_cycle);

  m_pseq->out_info( " avg number accesses of resultbus: %6.2lf\n", (total_resultbus_access*1.0)/sim_cycle);

  m_pseq->out_info( " avg number accesses of instr prefetcher: %6.2lf\n", (total_instr_prefetcher_access*1.0)/sim_cycle);

  m_pseq->out_info( " avg number accesses of data prefetcher: %6.2lf\n", (total_data_prefetcher_access*1.0)/sim_cycle);

  m_pseq->out_info( " avg number accesses of compression ALUs: %6.2lf\n", (total_compression_access*1.0)/sim_cycle);

  m_pseq->out_info( " max number accesses of rename unit: %lld\n", max_rename_access);

  m_pseq->out_info( " max number accesses of bpred unit: %lld\n", max_bpred_access);

  m_pseq->out_info( " max number accesses of instruction window: %lld\n", max_window_access);

  m_pseq->out_info( " max number accesses of window preg: %lld\n", max_win_preg_access);

  m_pseq->out_info( " max number accesses of window selection: %lld\n", max_win_selection_access);

  m_pseq->out_info( " max number accesses of window wakeup: %lld\n", max_win_wakeup_access);

  m_pseq->out_info( " max number accesses of load/store queue: %lld\n", max_lsq_access);

  m_pseq->out_info( " max number accesses of load/store wakeup: %lld\n", max_lsq_wakeup_access);

  m_pseq->out_info( " max number accesses of load/store preg: %lld\n", max_lsq_preg_access);

  m_pseq->out_info( " max number accesses of arch. regfile: %lld\n", max_regfile_access);

  m_pseq->out_info( " max number accesses of L1Icache: %lld\n", max_icache_access);

  m_pseq->out_info( " max number accesses of L1Dcache: %lld\n", max_dcache_access);

  m_pseq->out_info( " max number accesses of L2cache: %lld\n", max_dcache2_access);

  m_pseq->out_info( " max number accesses of alu: %lld\n", max_alu_access);

  m_pseq->out_info( " max number accesses of INT alu: %lld\n", max_ialu_access);

   m_pseq->out_info( " max number accesses of FP alu: %lld\n", max_falu_access);

  m_pseq->out_info( " max number accesses of resultbus: %lld\n", max_resultbus_access);

  m_pseq->out_info( " max number accesses of instr prefetcher: %lld\n", max_instr_prefetcher_access);

  m_pseq->out_info( " max number accesses of data prefetcher: %lld\n", max_data_prefetcher_access);

  m_pseq->out_info( " max number accesses of compression ALUs: %lld\n", max_compression_access);

  m_pseq->out_info( " max rename power_cc3: %6.2lf\n", max_rename_power_cc3);
  m_pseq->out_info( " max bpred power_cc3: %6.2lf\n", max_bpred_power_cc3);
  m_pseq->out_info( " max window power_cc3: %6.2lf\n", max_window_power_cc3);
  m_pseq->out_info( " max lsq power_cc3: %6.2lf\n", max_lsq_power_cc3);
  m_pseq->out_info( " max regfile power_cc3: %6.2lf\n", max_regfile_power_cc3);
  m_pseq->out_info( " max icache power_cc3: %6.2lf\n", max_icache_power_cc3);
  m_pseq->out_info( " max dcache power_cc3: %6.2lf\n", max_dcache_power_cc3);
  m_pseq->out_info( " max L2 power_cc3: %6.2lf\n", max_dcache2_power_cc3);
  m_pseq->out_info( " max alu power_cc3: %6.2lf\n", max_alu_power_cc3);
  m_pseq->out_info( " max resultbus power_cc3: %6.2lf\n", max_resultbus_power_cc3);
  m_pseq->out_info( " max current clock power_cc3: %6.2lf\n", max_current_clock_power_cc3);
  m_pseq->out_info( " max instr prefetcher power_cc3: %6.2lf\n", max_instr_prefetcher_power_cc3);
  m_pseq->out_info( " max data prefetcher power_cc3: %6.2lf\n", max_data_prefetcher_power_cc3);
  m_pseq->out_info( " max compression power_cc3: %6.2lf\n", max_compression_power_cc3);
  m_pseq->out_info( " max cycle power_cc3: %6.2lf\n", max_rename_power_cc3+max_bpred_power_cc3+
                  max_window_power_cc3+max_lsq_power_cc3+max_regfile_power_cc3+max_icache_power_cc3+
                  max_dcache_power_cc3+max_dcache2_power_cc3+max_alu_power_cc3+max_resultbus_power_cc3+
                  max_current_clock_power_cc3+max_instr_prefetcher_power_cc3+max_data_prefetcher_power_cc3+
                  max_compression_power_cc3);
  
  
  m_pseq->out_info( " maximum clock power usage of cc1: %6.2lf\n", max_clock_power_cc1);

  m_pseq->out_info( " maximum clock power usage of cc2: %6.2lf\n", max_clock_power_cc2);

  m_pseq->out_info( " maximum clock power usage of cc3: %6.2lf\n", max_clock_power_cc3);

  m_pseq->out_info( " maximum cycle power usage: %6.2lf\n", max_cycle_power);

  m_pseq->out_info( " maximum cycle power usage of cc1: %6.2lf\n", max_cycle_power_cc1);

  m_pseq->out_info(" maximum cycle power usage of cc2: %6.2lf\n", max_cycle_power_cc2);

  m_pseq->out_info( " maximum cycle power usage of cc3: %6.2lf\n", max_cycle_power_cc3);

  m_pseq->out_info( " maximum cycle power usage of cc1 no clock: %6.2lf\n", max_cycle_power_no_clock_cc1);
  
  m_pseq->out_info(" maximum cycle power usage of cc2 no clock: %6.2lf\n", max_cycle_power_no_clock_cc2);

  m_pseq->out_info( " maximum cycle power usage of cc3 no clock: %6.2lf\n", max_cycle_power_no_clock_cc3);


  //********print the WATTCH static power
  dump_power_stats();

  //close the output file
  if(m_output_file){
    fclose(m_output_file);
  }

#endif
}

//************************************************************************************
int power_t::pow2(int x) {
  return((int)pow(2.0,(double)x));
}
  
//************************************************************************************
double power_t::logfour(double x)
{
  if (x<=0) DEBUG_OUT("ERROR: power_t: logfour %e\n",x);
  return( (double) (log(x)/log(4.0)) );
}

//************************************************************************************
  /* safer pop count to validate the fast algorithm */
int power_t::pop_count_slow(uint64 bits)
{
  int count = 0; 
  uint64 tmpbits = bits; 
  while (tmpbits) { 
    if (tmpbits & 1) ++count; 
    tmpbits >>= 1; 
  } 
  return count; 
}
  
//*************************************************************************************
  /* fast pop count */
int power_t::pop_count(uint64 bits)
{
#define T unsigned long long
#define ONES ((T)(-1)) 
#define TWO(k) ((T)1 << (k)) 
#define CYCL(k) (ONES/(1 + (TWO(TWO(k))))) 
#define BSUM(x,k) ((x)+=(x) >> TWO(k), (x) &= CYCL(k)) 

  quad_t x = bits; 
  x = (x & CYCL(0)) + ((x>>TWO(0)) & CYCL(0)); 
  x = (x & CYCL(1)) + ((x>>TWO(1)) & CYCL(1)); 
  BSUM(x,2); 
  BSUM(x,3); 
  BSUM(x,4); 
  BSUM(x,5); 
  return x; 
}

//************************************************************************************

/* this routine takes the number of rows and cols of an array structure
   and attemps to make it make it more of a reasonable circuit structure
   by trying to make the number of rows and cols as close as possible.
   (scaling both by factors of 2 in opposite directions).  it returns
   a scale factor which is the amount that the rows should be divided
   by and the columns should be multiplied by.
*/
int power_t::squarify(int rows, int cols)
{
  int scale_factor = 1;

  if(rows == cols)
    return 1;

  /*
  printf("init rows == %d\n",rows);
  printf("init cols == %d\n",cols);
  */

  while(rows > cols) {
    rows = rows/2;
    cols = cols*2;

    /*
    printf("rows == %d\n",rows);
    printf("cols == %d\n",cols);
    printf("scale_factor == %d (2^ == %d)\n\n",scale_factor,(int)pow(2.0,(double)scale_factor));
    */

    if (rows/2 <= cols)
      return((int)pow(2.0,(double)scale_factor));
    scale_factor++;
  }

  return 1;
}

//************************************************************************************
/* could improve squarify to work when rows < cols */

double power_t::squarify_new(int rows, int cols)
{
  double scale_factor = 0.0;

  if(rows==cols)
    return(pow(2.0,scale_factor));

  while(rows > cols) {
    rows = rows/2;
    cols = cols*2;
    if (rows <= cols)
      return(pow(2.0,scale_factor));
    scale_factor++;
  }

  while(cols > rows) {
    rows = rows*2;
    cols = cols/2;
    if (cols <= rows)
      return(pow(2.0,scale_factor));
    scale_factor--;
  }

  return 1;

}

//************************************************************************************
void power_t::dump_power_stats()
{
  double total_power;
  double bpred_power;
  double rename_power;
  double rat_power;
  double dcl_power;
  double lsq_power;
  double window_power;
  double wakeup_power;
  double rs_power;
  double lsq_wakeup_power;
  double lsq_rs_power;
  double regfile_power;
  double reorder_power;
  double icache_power;
  double dcache_power;
  double dcache2_power;
  double dtlb_power;
  double itlb_power;
  double instr_prefetcher_power;
  double data_prefetcher_power;
  double ambient_power = 2.0;

  instr_prefetcher_power = power->instr_prefetcher_power;
  data_prefetcher_power = power->data_prefetcher_power;

  icache_power = power->icache_power;

  dcache_power = power->dcache_power;

  dcache2_power = power->dcache2_power;

  itlb_power = power->itlb;
  dtlb_power = power->dtlb;

  bpred_power = power->btb + power->local_predict + power->global_predict + 
    power->chooser + power->ras;

  rat_power = power->rat_decoder + 
    power->rat_wordline + power->rat_bitline + power->rat_senseamp;

  dcl_power = power->dcl_compare + power->dcl_pencode;

  rename_power = power->rat_power + power->dcl_power + power->inst_decoder_power;

  wakeup_power = power->wakeup_tagdrive + power->wakeup_tagmatch + 
    power->wakeup_ormatch;
   
  rs_power = power->rs_decoder + 
    power->rs_wordline + power->rs_bitline + power->rs_senseamp;

  window_power = wakeup_power + rs_power + power->selection;

  lsq_rs_power = power->lsq_rs_decoder + 
    power->lsq_rs_wordline + power->lsq_rs_bitline + power->lsq_rs_senseamp;

  lsq_wakeup_power = power->lsq_wakeup_tagdrive + 
    power->lsq_wakeup_tagmatch + power->lsq_wakeup_ormatch;

  lsq_power = lsq_wakeup_power + lsq_rs_power;

  reorder_power = power->reorder_decoder + 
    power->reorder_wordline + power->reorder_bitline + 
    power->reorder_senseamp;

  regfile_power = power->regfile_decoder + 
    power->regfile_wordline + power->regfile_bitline + 
    power->regfile_senseamp;

  total_power = bpred_power + rename_power + window_power + regfile_power +
    power->resultbus + lsq_power + 
    icache_power + dcache_power + dcache2_power + 
    dtlb_power + itlb_power + power->clock_power + power->ialu_power +
    power->falu_power + instr_prefetcher_power + data_prefetcher_power + power->compression_power;

  m_pseq->out_info("\n***WATTCH Static Power Analysis Stats:\n");
  m_pseq->out_info("----------------------------------------\n");
  m_pseq->out_info("Processor Parameters:\n");
  m_pseq->out_info("Issue Width: %d\n",ruu_issue_width);
  m_pseq->out_info("Window Size: %d\n",RUU_size);
  m_pseq->out_info("Number of INT Virtual Registers: %d\n",MD_NUM_IREGS);
  m_pseq->out_info("Number of FP Virtual Registers: %d\n",MD_NUM_FREGS);
  m_pseq->out_info("Number of INT Physical Registers: %d\n",MD_NUM_PHYS_IREGS);
  m_pseq->out_info("Number of FP Physical Registers: %d\n",MD_NUM_PHYS_FREGS);
  m_pseq->out_info("Number of L1 Banks: %d\n",NUM_L1_BANKS);
  m_pseq->out_info("Number of Int ALUS: %d\n", res_ialu);
  m_pseq->out_info("Number of FP ALUS: %d\n", res_fpalu);
  m_pseq->out_info("Number of compression ALUS: %d\n", compression_alus);
  m_pseq->out_info("Number of memory ports: %d\n", res_memport);
  m_pseq->out_info("Datapath Width: %d\n",data_width);
  m_pseq->out_info("COMPRESSION_POWER? %d\n", COMPRESSION_POWER);
  m_pseq->out_info("PREFETCHER_POWER? %d\n", PREFETCHER_POWER);

  m_pseq->out_info("\nTotal Power Consumption: %g\n",total_power+ambient_power);
  m_pseq->out_info("Branch Predictor Power Consumption: %g  (%.3g%%)\n",bpred_power,100*bpred_power/total_power);
  m_pseq->out_info(" branch target buffer power (W): %g\n",power->btb);
  m_pseq->out_info(" local predict power (W): %g\n",power->local_predict);
  m_pseq->out_info(" global predict power (W): %g\n",power->global_predict);
  m_pseq->out_info(" chooser power (W): %g\n",power->chooser);
  m_pseq->out_info(" RAS power (W): %g\n",power->ras);
  m_pseq->out_info(" INSTR prefetcher power (W): %g  (%.3g%%)\n", power->instr_prefetcher_power,
                             100.0* power->instr_prefetcher_power/total_power);
  m_pseq->out_info(" DATA prefetcher power (W): %g  (%.3g%%)\n", power->data_prefetcher_power,
                              100.0* power->data_prefetcher_power/total_power);
  m_pseq->out_info(" COMPRESSION power (W): %g  (%.3g%%)\n", power->compression_power,
                               100.0* power->compression_power/total_power);
  m_pseq->out_info("Rename Logic Power Consumption: %g  (%.3g%%)\n",rename_power,100*rename_power/total_power);
  m_pseq->out_info(" Instruction Decode Power (W): %g\n",power->inst_decoder_power);
  m_pseq->out_info(" RAT decode_power (W): %g\n",power->rat_decoder);
  m_pseq->out_info(" RAT wordline_power (W): %g\n",power->rat_wordline);
  m_pseq->out_info(" RAT bitline_power (W): %g\n",power->rat_bitline);
  m_pseq->out_info(" DCL Comparators (W): %g\n",power->dcl_compare);
  m_pseq->out_info("Instruction Window Power Consumption: %g  (%.3g%%)\n",window_power,100*window_power/total_power);
  m_pseq->out_info(" tagdrive (W): %g\n",power->wakeup_tagdrive);
  m_pseq->out_info(" tagmatch (W): %g\n",power->wakeup_tagmatch);
  m_pseq->out_info(" Selection Logic (W): %g\n",power->selection);
  m_pseq->out_info(" decode_power (W): %g\n",power->rs_decoder);
  m_pseq->out_info(" wordline_power (W): %g\n",power->rs_wordline);
  m_pseq->out_info(" bitline_power (W): %g\n",power->rs_bitline);
  m_pseq->out_info("Load/Store Queue Power Consumption: %g  (%.3g%%)\n",lsq_power,100*lsq_power/total_power);
  m_pseq->out_info(" tagdrive (W): %g\n",power->lsq_wakeup_tagdrive);
  m_pseq->out_info(" tagmatch (W): %g\n",power->lsq_wakeup_tagmatch);
  m_pseq->out_info(" decode_power (W): %g\n",power->lsq_rs_decoder);
  m_pseq->out_info(" wordline_power (W): %g\n",power->lsq_rs_wordline);
  m_pseq->out_info(" bitline_power (W): %g\n",power->lsq_rs_bitline);
  m_pseq->out_info("Arch. Register File Power Consumption: %g  (%.3g%%)\n",regfile_power,100*regfile_power/total_power);
  m_pseq->out_info(" decode_power (W): %g\n",power->regfile_decoder);
  m_pseq->out_info(" wordline_power (W): %g\n",power->regfile_wordline);
  m_pseq->out_info(" bitline_power (W): %g\n",power->regfile_bitline);
  m_pseq->out_info("Result Bus Power Consumption: %g  (%.3g%%)\n",power->resultbus,100*power->resultbus/total_power);
  m_pseq->out_info("Total Clock Power: %g  (%.3g%%)\n",power->clock_power,100*power->clock_power/total_power);
  m_pseq->out_info("Int ALU Power: %g  (%.3g%%)\n",power->ialu_power,100*power->ialu_power/total_power);
  m_pseq->out_info("FP ALU Power: %g  (%.3g%%)\n",power->falu_power,100*power->falu_power/total_power);
  m_pseq->out_info("Instruction Cache Power Consumption: %g  (%.3g%%)\n",icache_power,100*icache_power/total_power);
  m_pseq->out_info(" decode_power (W): %g\n",power->icache_decoder);
  m_pseq->out_info(" wordline_power (W): %g\n",power->icache_wordline);
  m_pseq->out_info(" bitline_power (W): %g\n",power->icache_bitline);
  m_pseq->out_info(" senseamp_power (W): %g\n",power->icache_senseamp);
  m_pseq->out_info(" tagarray_power (W): %g\n",power->icache_tagarray);
  m_pseq->out_info("Itlb_power (W): %g (%.3g%%)\n",power->itlb,100*power->itlb/total_power);
  m_pseq->out_info("Data Cache Power Consumption: %g  (%.3g%%)\n",dcache_power,100*dcache_power/total_power);
  m_pseq->out_info(" decode_power (W): %g\n",power->dcache_decoder);
  m_pseq->out_info(" wordline_power (W): %g\n",power->dcache_wordline);
  m_pseq->out_info(" bitline_power (W): %g\n",power->dcache_bitline);
  m_pseq->out_info(" senseamp_power (W): %g\n",power->dcache_senseamp);
  m_pseq->out_info(" tagarray_power (W): %g\n",power->dcache_tagarray);
  m_pseq->out_info("Dtlb_power (W): %g (%.3g%%)\n",power->dtlb,100*power->dtlb/total_power);
  m_pseq->out_info("Level 2 Cache Power Consumption: %g (%.3g%%)\n",dcache2_power,100*dcache2_power/total_power);
  m_pseq->out_info(" decode_power (W): %g\n",power->dcache2_decoder);
  m_pseq->out_info(" wordline_power (W): %g\n",power->dcache2_wordline);
  m_pseq->out_info(" bitline_power (W): %g\n",power->dcache2_bitline);
  m_pseq->out_info(" senseamp_power (W): %g\n",power->dcache2_senseamp);
  m_pseq->out_info(" tagarray_power (W): %g\n",power->dcache2_tagarray);
}

/*======================================================================*/

/*----------------------------------------------------------------------*/

double power_t::logtwo(double x)
{
if (x<=0) printf("%e\n",x);
        return( (double) (log10(x)/log10(2.0)) );
}

//*********************************************************************************

double power_t::gatecap(double width,double wirelength) /* returns gate capacitance in Farads */
{
  /* gate width in um (length is Leff) */
  /* poly wire length going to gate in lambda */
  double overlapCap;
  double gateCap;
  double l = 0.1525;
  
#if defined(Pdelta_w)
  overlapCap = (width - 2*Pdelta_w) * PCov;
  gateCap  = ((width - 2*Pdelta_w) * (l * LSCALE - 2*Pdelta_l) *
                PCg) + 2.0 * overlapCap;

  return gateCap;
#endif
  return(width*Leff*Cgate+wirelength*Cpolywire*Leff);
  return(width*Leff*Cgate);
  /*        return(width*CgateLeff+wirelength*Cpolywire*Leff);*/
}

//*******************************************************************************
double power_t::gatecappass(double width,double wirelength) /* returns gate capacitance in Farads */
{
  return(gatecap(width,wirelength));
}

//*******************************************************************************
/* Routine for calculating drain capacitances.  The draincap routine
 * folds transistors larger than 10um */

double power_t::draincap(double width,int nchannel,int stack)  /* returns drain cap in Farads */
{
  /* width = um */
  /* nchannel = whether n or p-channel (boolean) */
  /* stack = number of transistors in series that are on */

  double Cdiffside,Cdiffarea,Coverlap,cap;
  
  double overlapCap;
  double swAreaUnderGate;
  double area_peri;
  double diffArea;
  double diffPeri;
  double l = 0.4 * LSCALE;
  
  
  diffArea = l * width;
  diffPeri = 2 * l + 2 * width;

#if defined(Pdelta_w)
 if(nchannel == 0) {
   overlapCap = (width - 2 * Pdelta_w) * PCov;
   swAreaUnderGate = (width - 2 * Pdelta_w) * PCjswA;
   area_peri = ((diffArea * PCja)
                +  (diffPeri * PCjsw));
     
   return(stack*(area_peri + overlapCap + swAreaUnderGate));
 }
 else {
   overlapCap = (width - 2 * Ndelta_w) * NCov;
   swAreaUnderGate = (width - 2 * Ndelta_w) * NCjswA;
   area_peri = ((diffArea * NCja * LSCALE)
                +  (diffPeri * NCjsw * LSCALE));
   
   return(stack*(area_peri + overlapCap + swAreaUnderGate));
 }
#endif


        Cdiffside = (nchannel) ? Cndiffside : Cpdiffside;
        Cdiffarea = (nchannel) ? Cndiffarea : Cpdiffarea;
        Coverlap = (nchannel) ? (Cndiffovlp+Cnoxideovlp) :
                                (Cpdiffovlp+Cpoxideovlp);
        /* calculate directly-connected (non-stacked) capacitance */
        /* then add in capacitance due to stacking */
        if (width >= 10) {
            cap = 3.0*Leff*width/2.0*Cdiffarea + 6.0*Leff*Cdiffside +
                width*Coverlap;
            cap += (double)(stack-1)*(Leff*width*Cdiffarea +
                4.0*Leff*Cdiffside + 2.0*width*Coverlap);
        } else {
            cap = 3.0*Leff*width*Cdiffarea + (6.0*Leff+width)*Cdiffside +
                width*Coverlap;
            cap += (double)(stack-1)*(Leff*width*Cdiffarea +
                2.0*Leff*Cdiffside + 2.0*width*Coverlap);
        }
        return(cap);
}

//**************************************************************************
/* This routine operates in reverse: given a resistance, it finds
 * the transistor width that would have this R.  It is used in the
 * data wordline to estimate the wordline driver size. */

double power_t::restowidth(double res,int nchannel)  /* returns width in um */
{
  /* res = resistance in ohms */
  /* nchannel = whether N-channel or P-channel */
  double restrans;
  
  restrans = (nchannel) ? Rnchannelon : Rpchannelon;
  
  return(restrans/res);
}

//*************************************************************************
double power_t::horowitz(double inputramptime,double tf, double vs1,double vs2,int rise)
{
  /*
   * double inputramptime,    // input rise time 
   *       tf,               // time constant of gate 
   *    vs1,vs2;          // threshold voltages 
   * int rise;                // whether INPUT rise or fall (boolean) 
   */
    double a,b,td;

    a = inputramptime/tf;
    if (rise==RISE) {
       b = 0.5;
       td = tf*sqrt(fabs( log(vs1)*log(vs1)+2*a*b*(1.0-vs1))) +
            tf*(log(vs1)-log(vs2));
    } else {
       b = 0.4;
       td = tf*sqrt(fabs( log(1.0-vs1)*log(1.0-vs1)+2*a*b*(vs1))) +
            tf*(log(1.0-vs1)-log(1.0-vs2));
    }

    return(td);
}


//*************************************************************************
/*----------------------------------------------------------------------*/

/* Decoder delay:  (see section 6.1 of tech report) */


double power_t::decoder_delay(int C, int B, int A, int Ndwl, int Ndbl, int Nspd, int Ntwl, int Ntbl, int Ntspd, double * Tdecdrive, double * Tdecoder1, double * Tdecoder2, double * outrisetime)
{
        double Ceq,Req,Rwire,rows,tf,nextinputtime,vth,tstep,m,a,b,c;
        int numstack;
        double z = 0.0;

        /* Calculate rise time.  Consider two inverters */

        Ceq = draincap(Wdecdrivep,PCH,1)+draincap(Wdecdriven,NCH,1) +
              gatecap(Wdecdrivep+Wdecdriven,0.0);
        tf = Ceq*transreson(Wdecdriven,NCH,1);
        nextinputtime = horowitz(z,tf,VTHINV100x60,VTHINV100x60,FALL)/
                                  (VTHINV100x60);

        Ceq = draincap(Wdecdrivep,PCH,1)+draincap(Wdecdriven,NCH,1) +
              gatecap(Wdecdrivep+Wdecdriven,0.0);
        tf = Ceq*transreson(Wdecdriven,NCH,1);
        nextinputtime = horowitz(nextinputtime,tf,VTHINV100x60,VTHINV100x60,
                               RISE)/
                                  (1.0-VTHINV100x60);

        /* First stage: driving the decoders */

        rows = C/(8*B*A*Ndbl*Nspd);
        Ceq = draincap(Wdecdrivep,PCH,1)+draincap(Wdecdriven,NCH,1) +
            4*gatecap(Wdec3to8n+Wdec3to8p,10.0)*(Ndwl*Ndbl)+
            Cwordmetal*0.25*8*B*A*Ndbl*Nspd;
        Rwire = Rwordmetal*0.125*8*B*A*Ndbl*Nspd;
        tf = (Rwire + transreson(Wdecdrivep,PCH,1))*Ceq;
        *Tdecdrive = horowitz(nextinputtime,tf,VTHINV100x60,VTHNAND60x90,
                     FALL);
        nextinputtime = *Tdecdrive/VTHNAND60x90;

        /* second stage: driving a bunch of nor gates with a nand */

        numstack =
          ceil((1.0/3.0)*logtwo( (double)((double)C/(double)(B*A*Ndbl*Nspd))));
        if (numstack==0) numstack = 1;
        if (numstack>5) numstack = 5;
        Ceq = 3*draincap(Wdec3to8p,PCH,1) +draincap(Wdec3to8n,NCH,3) +
              gatecap(WdecNORn+WdecNORp,((numstack*40)+20.0))*rows +
              Cbitmetal*rows*8;

        Rwire = Rbitmetal*rows*8/2;
        tf = Ceq*(Rwire+transreson(Wdec3to8n,NCH,3)); 

        /* we only want to charge the output to the threshold of the
           nor gate.  But the threshold depends on the number of inputs
           to the nor.  */

        switch(numstack) {
          case 1: vth = VTHNOR12x4x1; break;
          case 2: vth = VTHNOR12x4x2; break;
          case 3: vth = VTHNOR12x4x3; break;
          case 4: vth = VTHNOR12x4x4; break;
          case 5: vth = VTHNOR12x4x4; break;
          default: printf("error:numstack=%d\n",numstack);
        }
        *Tdecoder1 = horowitz(nextinputtime,tf,VTHNAND60x90,vth,RISE);
        nextinputtime = *Tdecoder1/(1.0-vth);

        /* Final stage: driving an inverter with the nor */

        Req = transreson(WdecNORp,PCH,numstack);
        Ceq = (gatecap(Wdecinvn+Wdecinvp,20.0)+
              numstack*draincap(WdecNORn,NCH,1)+
                     draincap(WdecNORp,PCH,numstack));
        tf = Req*Ceq;
        *Tdecoder2 = horowitz(nextinputtime,tf,vth,VSINV,FALL);
        *outrisetime = *Tdecoder2/(VSINV);
        return(*Tdecdrive+*Tdecoder1+*Tdecoder2);
}

//************************************************************************
/* Decoder delay in the tag array (see section 6.1 of tech report) */


double power_t::decoder_tag_delay(int C, int B, int A, int Ndwl, int Ndbl, int Nspd, int Ntwl, int Ntbl, int Ntspd, double * Tdecdrive, double * Tdecoder1, double * Tdecoder2, double * outrisetime)
{
        double Ceq,Req,Rwire,rows,tf,nextinputtime,vth,tstep,m,a,b,c;
        int numstack;


        /* Calculate rise time.  Consider two inverters */

        Ceq = draincap(Wdecdrivep,PCH,1)+draincap(Wdecdriven,NCH,1) +
              gatecap(Wdecdrivep+Wdecdriven,0.0);
        tf = Ceq*transreson(Wdecdriven,NCH,1);
        nextinputtime = horowitz(0.0,tf,VTHINV100x60,VTHINV100x60,FALL)/
                                  (VTHINV100x60);

        Ceq = draincap(Wdecdrivep,PCH,1)+draincap(Wdecdriven,NCH,1) +
              gatecap(Wdecdrivep+Wdecdriven,0.0);
        tf = Ceq*transreson(Wdecdriven,NCH,1);
        nextinputtime = horowitz(nextinputtime,tf,VTHINV100x60,VTHINV100x60,
                               RISE)/
                                  (1.0-VTHINV100x60);

        /* First stage: driving the decoders */

        rows = C/(8*B*A*Ntbl*Ntspd);
        Ceq = draincap(Wdecdrivep,PCH,1)+draincap(Wdecdriven,NCH,1) +
            4*gatecap(Wdec3to8n+Wdec3to8p,10.0)*(Ntwl*Ntbl)+
            Cwordmetal*0.25*8*B*A*Ntbl*Ntspd;
        Rwire = Rwordmetal*0.125*8*B*A*Ntbl*Ntspd;
        tf = (Rwire + transreson(Wdecdrivep,PCH,1))*Ceq;
        *Tdecdrive = horowitz(nextinputtime,tf,VTHINV100x60,VTHNAND60x90,
                     FALL);
        nextinputtime = *Tdecdrive/VTHNAND60x90;

        /* second stage: driving a bunch of nor gates with a nand */

        numstack =
          ceil((1.0/3.0)*logtwo( (double)((double)C/(double)(B*A*Ntbl*Ntspd))));
        if (numstack==0) numstack = 1;
        if (numstack>5) numstack = 5;

        Ceq = 3*draincap(Wdec3to8p,PCH,1) +draincap(Wdec3to8n,NCH,3) +
              gatecap(WdecNORn+WdecNORp,((numstack*40)+20.0))*rows +
              Cbitmetal*rows*8;

        Rwire = Rbitmetal*rows*8/2;
        tf = Ceq*(Rwire+transreson(Wdec3to8n,NCH,3)); 

        /* we only want to charge the output to the threshold of the
           nor gate.  But the threshold depends on the number of inputs
           to the nor.  */

        switch(numstack) {
          case 1: vth = VTHNOR12x4x1; break;
          case 2: vth = VTHNOR12x4x2; break;
          case 3: vth = VTHNOR12x4x3; break;
          case 4: vth = VTHNOR12x4x4; break;
          case 5: vth = VTHNOR12x4x4; break;
          case 6: vth = VTHNOR12x4x4; break;
          default: printf("error:numstack=%d\n",numstack);
        }
        *Tdecoder1 = horowitz(nextinputtime,tf,VTHNAND60x90,vth,RISE);
        nextinputtime = *Tdecoder1/(1.0-vth);

        /* Final stage: driving an inverter with the nor */

        Req = transreson(WdecNORp,PCH,numstack);
        Ceq = (gatecap(Wdecinvn+Wdecinvp,20.0)+
              numstack*draincap(WdecNORn,NCH,1)+
                     draincap(WdecNORp,PCH,numstack));
        tf = Req*Ceq;
        *Tdecoder2 = horowitz(nextinputtime,tf,vth,VSINV,FALL);
        *outrisetime = *Tdecoder2/(VSINV);
        return(*Tdecdrive+*Tdecoder1+*Tdecoder2);
}


/*----------------------------------------------------------------------*/

/* Data array wordline delay (see section 6.2 of tech report) */


double power_t::wordline_delay(int B,int A,int Ndwl,int Nspd,double inrisetime, double * outrisetime)
{
        double Rpdrive,nextrisetime;
        double desiredrisetime,psize,nsize;
        double tf,nextinputtime,Ceq,Req,Rline,Cline;
        int cols;
        double Tworddrivedel,Twordchargedel;

        cols = 8*B*A*Nspd/Ndwl;

        /* Choose a transistor size that makes sense */
        /* Use a first-order approx */

        desiredrisetime = krise*log((double)(cols))/2.0;
        Cline = (gatecappass(Wmemcella,0.0)+
                 gatecappass(Wmemcella,0.0)+
                 Cwordmetal)*cols;
        Rpdrive = desiredrisetime/(Cline*log(VSINV)*-1.0);
        psize = restowidth(Rpdrive,PCH);
        if (psize > Wworddrivemax) {
           psize = Wworddrivemax;
        }

        /* Now that we have a reasonable psize, do the rest as before */
        /* If we keep the ratio the same as the tag wordline driver,
           the threshold voltage will be close to VSINV */

        nsize = psize * Wdecinvn/Wdecinvp;

        Ceq = draincap(Wdecinvn,NCH,1) + draincap(Wdecinvp,PCH,1) +
              gatecap(nsize+psize,20.0);
        tf = transreson(Wdecinvn,NCH,1)*Ceq;

        Tworddrivedel = horowitz(inrisetime,tf,VSINV,VSINV,RISE);
        nextinputtime = Tworddrivedel/(1.0-VSINV);

        Cline = (gatecappass(Wmemcella,(BitWidth-2*Wmemcella)/2.0)+
                 gatecappass(Wmemcella,(BitWidth-2*Wmemcella)/2.0)+
                 Cwordmetal)*cols+
                draincap(nsize,NCH,1) + draincap(psize,PCH,1);
        Rline = Rwordmetal*cols/2;
        tf = (transreson(psize,PCH,1)+Rline)*Cline;
        Twordchargedel = horowitz(nextinputtime,tf,VSINV,VSINV,FALL);
        *outrisetime = Twordchargedel/VSINV;

        return(Tworddrivedel+Twordchargedel);
}

/*----------------------------------------------------------------------*/

/* Tag array wordline delay (see section 6.3 of tech report) */


double power_t::wordline_tag_delay(int C,int A,int Ntspd,int Ntwl,double inrisetime, double * outrisetime)
{
        double tf,m,a,b,c;
        double Cline,Rline,Ceq,nextinputtime;
        int tagbits;
        double Tworddrivedel,Twordchargedel;

        /* number of tag bits */

        tagbits = ADDRESS_BITS+2-(int)logtwo((double)C)+(int)logtwo((double)A);

        /* first stage */

        Ceq = draincap(Wdecinvn,NCH,1) + draincap(Wdecinvp,PCH,1) +
              gatecap(Wdecinvn+Wdecinvp,20.0);
        tf = transreson(Wdecinvn,NCH,1)*Ceq;

        Tworddrivedel = horowitz(inrisetime,tf,VSINV,VSINV,RISE);
        nextinputtime = Tworddrivedel/(1.0-VSINV);

        /* second stage */
        Cline = (gatecappass(Wmemcella,(BitWidth-2*Wmemcella)/2.0)+
                 gatecappass(Wmemcella,(BitWidth-2*Wmemcella)/2.0)+
                 Cwordmetal)*tagbits*A*Ntspd/Ntwl+
                draincap(Wdecinvn,NCH,1) + draincap(Wdecinvp,PCH,1);
        Rline = Rwordmetal*tagbits*A*Ntspd/(2*Ntwl);
        tf = (transreson(Wdecinvp,PCH,1)+Rline)*Cline;
        Twordchargedel = horowitz(nextinputtime,tf,VSINV,VSINV,FALL);
        *outrisetime = Twordchargedel/VSINV;
        return(Tworddrivedel+Twordchargedel);

}

/*----------------------------------------------------------------------*/

/* Data array bitline: (see section 6.4 in tech report) */


double power_t::bitline_delay(int C,int A,int B,int Ndwl,int Ndbl,int Nspd,double inrisetime, double * outrisetime)
{
        double Tbit,Cline,Ccolmux,Rlineb,r1,r2,c1,c2,a,b,c;
        double m,tstep;
        double Cbitrow;    /* bitline capacitance due to access transistor */
        int rows,cols;

        Cbitrow = draincap(Wmemcella,NCH,1)/2.0; /* due to shared contact */
        rows = C/(B*A*Ndbl*Nspd);
        cols = 8*B*A*Nspd/Ndwl;
        if (Ndbl*Nspd == 1) {
           Cline = rows*(Cbitrow+Cbitmetal)+2*draincap(Wbitpreequ,PCH,1);
           Ccolmux = 2*gatecap(WsenseQ1to4,10.0);
           Rlineb = Rbitmetal*rows/2.0;
           r1 = Rlineb;
        } else { 
           Cline = rows*(Cbitrow+Cbitmetal) + 2*draincap(Wbitpreequ,PCH,1) +
                   draincap(Wbitmuxn,NCH,1);
           Ccolmux = Nspd*Ndbl*(draincap(Wbitmuxn,NCH,1))+2*gatecap(WsenseQ1to4,10.0);
           Rlineb = Rbitmetal*rows/2.0;
           r1 = Rlineb + 
                 transreson(Wbitmuxn,NCH,1);
        }
        r2 = transreson(Wmemcella,NCH,1) +
             transreson(Wmemcella*Wmemcellbscale,NCH,1);
        c1 = Ccolmux;
        c2 = Cline;


        tstep = (r2*c2+(r1+r2)*c1)*log((Vbitpre)/(Vbitpre-Vbitsense));

        /* take input rise time into account */

        m = Vdd/inrisetime;
        if (tstep <= (0.5*(Vdd-Vt)/m)) {
              a = m;
              b = 2*((Vdd*0.5)-Vt);
              c = -2*tstep*(Vdd-Vt)+1/m*((Vdd*0.5)-Vt)*
                  ((Vdd*0.5)-Vt);
              Tbit = (-b+sqrt(b*b-4*a*c))/(2*a);
        } else {
              Tbit = tstep + (Vdd+Vt)/(2*m) - (Vdd*0.5)/m;
        }

        *outrisetime = Tbit/(log((Vbitpre-Vbitsense)/Vdd));
        return(Tbit);
}




/*----------------------------------------------------------------------*/

/* Tag array bitline: (see section 6.4 in tech report) */


double power_t::bitline_tag_delay(int C,int A,int B,int Ntwl,int Ntbl,int Ntspd,double inrisetime, double * outrisetime)
{
        double Tbit,Cline,Ccolmux,Rlineb,r1,r2,c1,c2,a,b,c;
        double m,tstep;
        double Cbitrow;    /* bitline capacitance due to access transistor */
        int rows,cols;

        Cbitrow = draincap(Wmemcella,NCH,1)/2.0; /* due to shared contact */
        rows = C/(B*A*Ntbl*Ntspd);
        cols = 8*B*A*Ntspd/Ntwl;
        if (Ntbl*Ntspd == 1) {
           Cline = rows*(Cbitrow+Cbitmetal)+2*draincap(Wbitpreequ,PCH,1);
           Ccolmux = 2*gatecap(WsenseQ1to4,10.0);
           Rlineb = Rbitmetal*rows/2.0;
           r1 = Rlineb;
        } else { 
           Cline = rows*(Cbitrow+Cbitmetal) + 2*draincap(Wbitpreequ,PCH,1) +
                   draincap(Wbitmuxn,NCH,1);
           Ccolmux = Ntspd*Ntbl*(draincap(Wbitmuxn,NCH,1))+2*gatecap(WsenseQ1to4,10.0);
           Rlineb = Rbitmetal*rows/2.0;
           r1 = Rlineb + 
                 transreson(Wbitmuxn,NCH,1);
        }
        r2 = transreson(Wmemcella,NCH,1) +
             transreson(Wmemcella*Wmemcellbscale,NCH,1);

        c1 = Ccolmux;
        c2 = Cline;

        tstep = (r2*c2+(r1+r2)*c1)*log((Vbitpre)/(Vbitpre-Vbitsense));

        /* take into account input rise time */

        m = Vdd/inrisetime;
        if (tstep <= (0.5*(Vdd-Vt)/m)) {
              a = m;
              b = 2*((Vdd*0.5)-Vt);
              c = -2*tstep*(Vdd-Vt)+1/m*((Vdd*0.5)-Vt)*
                  ((Vdd*0.5)-Vt);
              Tbit = (-b+sqrt(b*b-4*a*c))/(2*a);
        } else {
              Tbit = tstep + (Vdd+Vt)/(2*m) - (Vdd*0.5)/m;
        }

        *outrisetime = Tbit/(log((Vbitpre-Vbitsense)/Vdd));
        return(Tbit);
}



/*----------------------------------------------------------------------*/

/* It is assumed the sense amps have a constant delay
   (see section 6.5) */

double power_t::sense_amp_delay(double inrisetime, double * outrisetime)
{
   *outrisetime = tfalldata;
   return(tsensedata);
}

/*--------------------------------------------------------------*/

double power_t::sense_amp_tag_delay(double inrisetime, double * outrisetime)
{
   *outrisetime = tfalltag;
   return(tsensetag);
}

/*----------------------------------------------------------------------*/

/* Comparator Delay (see section 6.6) */


double power_t::compare_time(int C,int A,int Ntbl,int Ntspd,double inputtime, double * outputtime)
{
   double Req,Ceq,tf,st1del,st2del,st3del,nextinputtime,m;
   double c1,c2,r1,r2,tstep,a,b,c;
   double Tcomparatorni;
   int cols,tagbits;

   /* First Inverter */

   Ceq = gatecap(Wcompinvn2+Wcompinvp2,10.0) +
         draincap(Wcompinvp1,PCH,1) + draincap(Wcompinvn1,NCH,1);
   Req = transreson(Wcompinvp1,PCH,1);
   tf = Req*Ceq;
   st1del = horowitz(inputtime,tf,VTHCOMPINV,VTHCOMPINV,FALL);
   nextinputtime = st1del/VTHCOMPINV;

   /* Second Inverter */

   Ceq = gatecap(Wcompinvn3+Wcompinvp3,10.0) +
         draincap(Wcompinvp2,PCH,1) + draincap(Wcompinvn2,NCH,1);
   Req = transreson(Wcompinvn2,NCH,1);
   tf = Req*Ceq;
   st2del = horowitz(inputtime,tf,VTHCOMPINV,VTHCOMPINV,RISE);
   nextinputtime = st1del/(1.0-VTHCOMPINV);

   /* Third Inverter */

   Ceq = gatecap(Wevalinvn+Wevalinvp,10.0) +
         draincap(Wcompinvp3,PCH,1) + draincap(Wcompinvn3,NCH,1);
   Req = transreson(Wcompinvp3,PCH,1);
   tf = Req*Ceq;
   st3del = horowitz(nextinputtime,tf,VTHCOMPINV,VTHEVALINV,FALL);
   nextinputtime = st1del/(VTHEVALINV);

   /* Final Inverter (virtual ground driver) discharging compare part */
   
   tagbits = ADDRESS_BITS - (int)logtwo((double)C) + (int)logtwo((double)A);
   cols = tagbits*Ntbl*Ntspd;

   r1 = transreson(Wcompn,NCH,2);
   r2 = transresswitch(Wevalinvn,NCH,1);
   c2 = (tagbits)*(draincap(Wcompn,NCH,1)+draincap(Wcompn,NCH,2))+
         draincap(Wevalinvp,PCH,1) + draincap(Wevalinvn,NCH,1);
   c1 = (tagbits)*(draincap(Wcompn,NCH,1)+draincap(Wcompn,NCH,2))
        +draincap(Wcompp,PCH,1) + gatecap(Wmuxdrv12n+Wmuxdrv12p,20.0) +
        cols*Cwordmetal;

   /* time to go to threshold of mux driver */

   tstep = (r2*c2+(r1+r2)*c1)*log(1.0/VTHMUXDRV1);

   /* take into account non-zero input rise time */

   m = Vdd/nextinputtime;

   if ((tstep) <= (0.5*(Vdd-Vt)/m)) {
                a = m;
              b = 2*((Vdd*VTHEVALINV)-Vt);
              c = -2*(tstep)*(Vdd-Vt)+1/m*((Vdd*VTHEVALINV)-Vt)*((Vdd*VTHEVALINV)-Vt);
               Tcomparatorni = (-b+sqrt(b*b-4*a*c))/(2*a);
   } else {
              Tcomparatorni = (tstep) + (Vdd+Vt)/(2*m) - (Vdd*VTHEVALINV)/m;
   }
   *outputtime = Tcomparatorni/(1.0-VTHMUXDRV1);
                        
   return(Tcomparatorni+st1del+st2del+st3del);
}


/*----------------------------------------------------------------------*/

/* Delay of the multiplexor Driver (see section 6.7) */


double power_t::mux_driver_delay(int C,int B,int A,int Ndbl,int Nspd,int Ndwl,int Ntbl,int Ntspd,double inputtime,double * outputtime)
{
   double Ceq,Req,tf,nextinputtime;
   double Tst1,Tst2,Tst3;

   /* first driver stage - Inverte "match" to produce "matchb" */
   /* the critical path is the DESELECTED case, so consider what
      happens when the address bit is true, but match goes low */

   Ceq = gatecap(WmuxdrvNORn+WmuxdrvNORp,15.0)*(8*B/BITOUT) +
         draincap(Wmuxdrv12n,NCH,1) + draincap(Wmuxdrv12p,PCH,1);
   Req = transreson(Wmuxdrv12p,PCH,1);
   tf = Ceq*Req;
   Tst1 = horowitz(inputtime,tf,VTHMUXDRV1,VTHMUXDRV2,FALL);
   nextinputtime = Tst1/VTHMUXDRV2;

   /* second driver stage - NOR "matchb" with address bits to produce sel */

   Ceq = gatecap(Wmuxdrv3n+Wmuxdrv3p,15.0) + 2*draincap(WmuxdrvNORn,NCH,1) +
         draincap(WmuxdrvNORp,PCH,2);
   Req = transreson(WmuxdrvNORn,NCH,1);
   tf = Ceq*Req;
   Tst2 = horowitz(nextinputtime,tf,VTHMUXDRV2,VTHMUXDRV3,RISE);
   nextinputtime = Tst2/(1-VTHMUXDRV3);

   /* third driver stage - invert "select" to produce "select bar" */

   Ceq = BITOUT*gatecap(Woutdrvseln+Woutdrvselp+Woutdrvnorn+Woutdrvnorp,20.0)+
         draincap(Wmuxdrv3p,PCH,1) + draincap(Wmuxdrv3n,NCH,1) +
         Cwordmetal*8*B*A*Nspd*Ndbl/2.0;
   Req = (Rwordmetal*8*B*A*Nspd*Ndbl/2)/2 + transreson(Wmuxdrv3p,PCH,1);
   tf = Ceq*Req;
   Tst3 = horowitz(nextinputtime,tf,VTHMUXDRV3,VTHOUTDRINV,FALL);
   *outputtime = Tst3/(VTHOUTDRINV);

   return(Tst1 + Tst2 + Tst3);

}



/*----------------------------------------------------------------------*/

/* Valid driver (see section 6.9 of tech report)
   Note that this will only be called for a direct mapped cache */

double power_t::valid_driver_delay(int C, int A,int Ntbl,int Ntspd,double inputtime)
{
   double Ceq,Tst1,tf;

   Ceq = draincap(Wmuxdrv12n,NCH,1)+draincap(Wmuxdrv12p,PCH,1)+Cout;
   tf = Ceq*transreson(Wmuxdrv12p,PCH,1);
   Tst1 = horowitz(inputtime,tf,VTHMUXDRV1,0.5,FALL);

   return(Tst1);
}


/*----------------------------------------------------------------------*/

/* Data output delay (data side) -- see section 6.8
   This is the time through the NAND/NOR gate and the final inverter 
   assuming sel is already present */

double power_t::dataoutput_delay(int C, int B, int A, int Ndbl, int Nspd,int Ndwl,
              double  inrisetime, double * outrisetime)
{
        double Ceq,Rwire,Rline;
        double aspectRatio;     /* as height over width */
        double ramBlocks;       /* number of RAM blocks */
        double tf;
        double nordel,outdel,nextinputtime;       
        double hstack,vstack;

        /* calculate some layout info */

        aspectRatio = (2.0*C)/(8.0*B*B*A*A*Ndbl*Ndbl*Nspd*Nspd);
        hstack = (aspectRatio > 1.0) ? aspectRatio : 1.0/aspectRatio;
        ramBlocks = Ndwl*Ndbl;
        hstack = hstack * sqrt(ramBlocks/ hstack);
        vstack = ramBlocks/ hstack;

        /* Delay of NOR gate */

        Ceq = 2*draincap(Woutdrvnorn,NCH,1)+draincap(Woutdrvnorp,PCH,2)+
              gatecap(Woutdrivern,10.0);
        tf = Ceq*transreson(Woutdrvnorp,PCH,2);
        nordel = horowitz(inrisetime,tf,VTHOUTDRNOR,VTHOUTDRIVE,FALL);
        nextinputtime = nordel/(VTHOUTDRIVE);

        /* Delay of final output driver */

        Ceq = (draincap(Woutdrivern,NCH,1)+draincap(Woutdriverp,PCH,1))*
              ((8*B*A)/BITOUT) +
              Cwordmetal*(8*B*A*Nspd* (vstack)) + Cout;
        Rwire = Rwordmetal*(8*B*A*Nspd* (vstack))/2;

        tf = Ceq*(transreson(Woutdriverp,PCH,1)+Rwire);
        outdel = horowitz(nextinputtime,tf,VTHOUTDRIVE,0.5,RISE);
        *outrisetime = outdel/0.5;
        return(outdel+nordel);
}

/*----------------------------------------------------------------------*/

/* Sel inverter delay (part of the output driver)  see section 6.8 */

double power_t::selb_delay_tag_path(double inrisetime, double * outrisetime)
{
   double Ceq,Tst1,tf;

   Ceq = draincap(Woutdrvseln,NCH,1)+draincap(Woutdrvselp,PCH,1)+
         gatecap(Woutdrvnandn+Woutdrvnandp,10.0);
   tf = Ceq*transreson(Woutdrvseln,NCH,1);
   Tst1 = horowitz(inrisetime,tf,VTHOUTDRINV,VTHOUTDRNAND,RISE);
   *outrisetime = Tst1/(1.0-VTHOUTDRNAND);

   return(Tst1);
}


/*----------------------------------------------------------------------*/

/* This routine calculates the extra time required after an access before
 * the next access can occur [ie.  it returns (cycle time-access time)].
 */

double power_t::precharge_delay(double worddata)
{
   double Ceq,tf,pretime;

   /* as discussed in the tech report, the delay is the delay of
      4 inverter delays (each with fanout of 4) plus the delay of
      the wordline */

   Ceq = draincap(Wdecinvn,NCH,1)+draincap(Wdecinvp,PCH,1)+
         4*gatecap(Wdecinvn+Wdecinvp,0.0);
   tf = Ceq*transreson(Wdecinvn,NCH,1);
   pretime = 4*horowitz(0.0,tf,0.5,0.5,RISE) + worddata;

   return(pretime);
}


/*======================================================================*/


/* returns TRUE if the parameters make up a valid organization */
/* Layout concerns drive any restrictions you might add here */
 
int power_t::organizational_parameters_valid(int rows,int cols,int Ndwl,int Ndbl,int Nspd,int Ntwl,int Ntbl,int Ntspd)
{
   /* don't want more than 8 subarrays for each of data/tag */

   if (Ndwl*Ndbl>MAXSUBARRAYS) return(FALSE);
   if (Ntwl*Ntbl>MAXSUBARRAYS) return(FALSE);

   /* add more constraints here as necessary */

   return(TRUE);
}

//*************************************************************************
/* The following routines estimate the effective resistance of an
   on transistor as described in the tech report.  The first routine
   gives the "switching" resistance, and the second gives the 
   "full-on" resistance */

double power_t::transresswitch(double width, int nchannel, int stack)  /* returns resistance in ohms */
{
  /*
   * double width;                // um 
  * int nchannel;                // whether n or p-channel (boolean) 
  * int stack;                // number of transistors in series 
  */
  double restrans;
  restrans = (nchannel) ? (Rnchannelstatic) : (Rpchannelstatic);
  /* calculate resistance of stack - assume all but switching trans
     have 0.8X the resistance since they are on throughout switching */
  return((1.0+((stack-1.0)*0.8))*restrans/width);        
}

//*************************************************************************
double power_t::transreson(double width, int nchannel,int stack)  /* returns resistance in ohms */
{
  /*
   * double width;           // um 
  * int nchannel;           // whether n or p-channel (boolean) 
  * int stack;              // number of transistors in series 
  */
        double restrans;
        restrans = (nchannel) ? Rnchannelon : Rpchannelon;

      /* calculate resistance of stack.  Unlike transres, we don't
           multiply the stacked transistors by 0.8 */
        return(stack*restrans/width);

}

//*************************************************************************
void power_t::calculate_time(time_result_type * result, time_parameter_type * parameters)
{
   int Ndwl,Ndbl,Nspd,Ntwl,Ntbl,Ntspd,rows,columns,tag_driver_size1,tag_driver_size2;
   double access_time;
   double before_mux,after_mux;
   double decoder_data_driver,decoder_data_3to8,decoder_data_inv;
   double decoder_data,decoder_tag,wordline_data,wordline_tag;
   double decoder_tag_driver,decoder_tag_3to8,decoder_tag_inv;
   double bitline_data,bitline_tag,sense_amp_data,sense_amp_tag;
   double compare_tag,mux_driver,data_output,selb;
   double time_till_compare,time_till_select,driver_cap,valid_driver;
   double cycle_time, precharge_del;
   double outrisetime,inrisetime;   

   rows = parameters->number_of_sets;
   columns = 8*parameters->block_size*parameters->associativity;

   /* go through possible Ndbl,Ndwl and find the smallest */
   /* Because of area considerations, I don't think it makes sense
      to break either dimension up larger than MAXN */

   result->cycle_time = BIGNUM;
   result->access_time = BIGNUM;
   for (Nspd=1;Nspd<=MAXSPD;Nspd=Nspd*2) {
    for (Ndwl=1;Ndwl<=MAXN;Ndwl=Ndwl*2) {
     for (Ndbl=1;Ndbl<=MAXN;Ndbl=Ndbl*2) {
      for (Ntspd=1;Ntspd<=MAXSPD;Ntspd=Ntspd*2) {
       for (Ntwl=1;Ntwl<=1;Ntwl=Ntwl*2) {
        for (Ntbl=1;Ntbl<=MAXN;Ntbl=Ntbl*2) {

        if (organizational_parameters_valid
            (rows,columns,Ndwl,Ndbl,Nspd,Ntwl,Ntbl,Ntspd)) {


            /* Calculate data side of cache */


            decoder_data = decoder_delay(parameters->cache_size,parameters->block_size,
                           parameters->associativity,Ndwl,Ndbl,Nspd,Ntwl,Ntbl,Ntspd,
                           &decoder_data_driver,&decoder_data_3to8,
                           &decoder_data_inv,&outrisetime);


            inrisetime = outrisetime;
            wordline_data = wordline_delay(parameters->block_size,
                            parameters->associativity,Ndwl,Nspd,
                            inrisetime,&outrisetime);

            inrisetime = outrisetime;
            bitline_data = bitline_delay(parameters->cache_size,parameters->associativity,
                           parameters->block_size,Ndwl,Ndbl,Nspd,
                           inrisetime,&outrisetime);
            inrisetime = outrisetime;
            sense_amp_data = sense_amp_delay(inrisetime,&outrisetime);
            inrisetime = outrisetime;
            data_output = dataoutput_delay(parameters->cache_size,parameters->block_size,
                          parameters->associativity,Ndbl,Nspd,Ndwl,
                          inrisetime,&outrisetime);
            inrisetime = outrisetime;


            /* if the associativity is 1, the data output can come right
               after the sense amp.   Otherwise, it has to wait until 
               the data access has been done. */

            if (parameters->associativity==1) {
               before_mux = decoder_data + wordline_data + bitline_data +
                            sense_amp_data + data_output;
               after_mux = 0;
            } else {   
                  before_mux = decoder_data + wordline_data + bitline_data +
                               sense_amp_data;
                  after_mux = data_output;
            }


            /*
             * Now worry about the tag side.
             */


            decoder_tag = decoder_tag_delay(parameters->cache_size,
                          parameters->block_size,parameters->associativity,
                          Ndwl,Ndbl,Nspd,Ntwl,Ntbl,Ntspd,
                          &decoder_tag_driver,&decoder_tag_3to8,
                          &decoder_tag_inv,&outrisetime);
            inrisetime = outrisetime;

            wordline_tag = wordline_tag_delay(parameters->cache_size,
                           parameters->associativity,Ntspd,Ntwl,
                           inrisetime,&outrisetime);
            inrisetime = outrisetime;

            bitline_tag = bitline_tag_delay(parameters->cache_size,parameters->associativity,
                          parameters->block_size,Ntwl,Ntbl,Ntspd,
                          inrisetime,&outrisetime);
            inrisetime = outrisetime;

            sense_amp_tag = sense_amp_tag_delay(inrisetime,&outrisetime);
            inrisetime = outrisetime;

            compare_tag = compare_time(parameters->cache_size,parameters->associativity,
                          Ntbl,Ntspd,
                          inrisetime,&outrisetime);
            inrisetime = outrisetime;

            if (parameters->associativity == 1) {
               mux_driver = 0;
               valid_driver = valid_driver_delay(parameters->cache_size,
                              parameters->associativity,Ntbl,Ntspd,inrisetime);
               time_till_compare = decoder_tag + wordline_tag + bitline_tag +
                                    sense_amp_tag;
 
               time_till_select = time_till_compare+ compare_tag + valid_driver;
        
         
               /*
                * From the above info, calculate the total access time
                */
          
               access_time = MAX(before_mux+after_mux,time_till_select);


            } else {
               mux_driver = mux_driver_delay(parameters->cache_size,parameters->block_size,
                            parameters->associativity,Ndbl,Nspd,Ndwl,Ntbl,Ntspd,
                            inrisetime,&outrisetime);

               selb = selb_delay_tag_path(inrisetime,&outrisetime);

               valid_driver = 0;

               time_till_compare = decoder_tag + wordline_tag + bitline_tag +
                                 sense_amp_tag;

               time_till_select = time_till_compare+ compare_tag + mux_driver
                                  + selb;

               access_time = MAX(before_mux,time_till_select) +after_mux;
            }

            /*
             * Calcuate the cycle time
             */

            precharge_del = precharge_delay(wordline_data);
             
            cycle_time = access_time + precharge_del;
         
            /*
             * The parameters are for a 0.8um process.  A quick way to
             * scale the results to another process is to divide all
             * the results by FUDGEFACTOR.  Normally, FUDGEFACTOR is 1.
             */

            if (result->cycle_time+1e-11*(result->best_Ndwl+result->best_Ndbl+result->best_Nspd+result->best_Ntwl+result->best_Ntbl+result->best_Ntspd) > cycle_time/FUDGEFACTOR+1e-11*(Ndwl+Ndbl+Nspd+Ntwl+Ntbl+Ntspd)) {
               result->cycle_time = cycle_time/FUDGEFACTOR;
               result->access_time = access_time/FUDGEFACTOR;
               result->best_Ndwl = Ndwl;
               result->best_Ndbl = Ndbl;
               result->best_Nspd = Nspd;
               result->best_Ntwl = Ntwl;
               result->best_Ntbl = Ntbl;
               result->best_Ntspd = Ntspd;
               result->decoder_delay_data = decoder_data/FUDGEFACTOR;
               result->decoder_delay_tag = decoder_tag/FUDGEFACTOR;
               result->dec_tag_driver = decoder_tag_driver/FUDGEFACTOR;
               result->dec_tag_3to8 = decoder_tag_3to8/FUDGEFACTOR;
               result->dec_tag_inv = decoder_tag_inv/FUDGEFACTOR;
               result->dec_data_driver = decoder_data_driver/FUDGEFACTOR;
               result->dec_data_3to8 = decoder_data_3to8/FUDGEFACTOR;
               result->dec_data_inv = decoder_data_inv/FUDGEFACTOR;
               result->wordline_delay_data = wordline_data/FUDGEFACTOR;
               result->wordline_delay_tag = wordline_tag/FUDGEFACTOR;
               result->bitline_delay_data = bitline_data/FUDGEFACTOR;
               result->bitline_delay_tag = bitline_tag/FUDGEFACTOR;
               result->sense_amp_delay_data = sense_amp_data/FUDGEFACTOR;
               result->sense_amp_delay_tag = sense_amp_tag/FUDGEFACTOR;
               result->compare_part_delay = compare_tag/FUDGEFACTOR;
               result->drive_mux_delay = mux_driver/FUDGEFACTOR;
               result->selb_delay = selb/FUDGEFACTOR;
               result->drive_valid_delay = valid_driver/FUDGEFACTOR;
               result->data_output_delay = data_output/FUDGEFACTOR;
               result->precharge_delay = precharge_del/FUDGEFACTOR;
            }
        }
        }
       }
      }
     }
    }
   }
}

/* 
 * This part of the code contains routines for each section as
 * described in the tech report.  See the tech report for more details
 * and explanations */

/*----------------------------------------------------------------------*/

double power_t::driver_size(double driving_cap, double desiredrisetime) 
{
  double nsize, psize;
  double Rpdrive; 

  Rpdrive = desiredrisetime/(driving_cap*log(VSINV)*-1.0);
  psize = restowidth(Rpdrive,PCH);
  nsize = restowidth(Rpdrive,NCH);
  if (psize > Wworddrivemax) {
    psize = Wworddrivemax;
  }
  if (psize < 4.0 * LSCALE)
    psize = 4.0 * LSCALE;

  return (psize);

}

//************************************************************************************
/* Decoder delay:  (see section 6.1 of tech report) */

double power_t::array_decoder_power(int rows,int cols,double predeclength,int rports,int wports,int cache)
{
  double Ctotal=0;
  double Ceq=0;
  int numstack;
  int decode_bits=0;
  int ports;
  double rowsb;

  /* read and write ports are the same here */
  ports = rports + wports;

  rowsb = (double)rows;

  /* number of input bits to be decoded */
  decode_bits=ceil((logtwo(rowsb)));

  /* First stage: driving the decoders */

  /* This is the capacitance for driving one bit (and its complement).
     -There are #rowsb 3->8 decoders contributing gatecap.
     - 2.0 factor from 2 identical sets of drivers in parallel
  */
  Ceq = 2.0*(draincap(Wdecdrivep,PCH,1)+draincap(Wdecdriven,NCH,1)) +
    gatecap(Wdec3to8n+Wdec3to8p,10.0)*rowsb;

  /* There are ports * #decode_bits total */
  Ctotal+=ports*decode_bits*Ceq;

  if(verbose)
    fprintf(stderr,"Decoder -- Driving decoders            == %g\n",.3*Ctotal*Powerfactor);

  /* second stage: driving a bunch of nor gates with a nand 
     numstack is the size of the nor gates -- ie. a 7-128 decoder has
     3-input NAND followed by 3-input NOR  */

  numstack = ceil((1.0/3.0)*logtwo(rows));

  if (numstack<=0) numstack = 1;
  if (numstack>5) numstack = 5;

  /* There are #rowsb NOR gates being driven*/
  Ceq = (3.0*draincap(Wdec3to8p,PCH,1) +draincap(Wdec3to8n,NCH,3) +
         gatecap(WdecNORn+WdecNORp,((numstack*40)+20.0)))*rowsb;

  Ctotal+=ports*Ceq;

  if(verbose)
    fprintf(stderr,"Decoder -- Driving nor w/ nand         == %g\n",.3*ports*Ceq*Powerfactor);

  /* Final stage: driving an inverter with the nor 
     (inverter preceding wordline driver) -- wordline driver is in the next section*/

  Ceq = (gatecap(Wdecinvn+Wdecinvp,20.0)+
         numstack*draincap(WdecNORn,NCH,1)+
         draincap(WdecNORp,PCH,numstack));

  if(verbose)
    fprintf(stderr,"Decoder -- Driving inverter w/ nor     == %g\n",.3*ports*Ceq*Powerfactor);

  Ctotal+=ports*Ceq;

  /* assume Activity Factor == .3  */

  return(.3*Ctotal*Powerfactor);
}

//************************************************************************************
double power_t::simple_array_decoder_power(int rows,int cols,int rports,int wports,int cache)
{
  double predeclength=0.0;
  return(array_decoder_power(rows,cols,predeclength,rports,wports,cache));
}

//************************************************************************************
double power_t::array_wordline_power(int rows,int cols,double wordlinelength,int rports,int wports,int cache)
{
  double Ctotal=0;
  double Ceq=0;
  double Cline=0;
  double Cliner, Clinew=0;
  double desiredrisetime,psize,nsize;
  int ports;
  double colsb;

  ports = rports+wports;

  colsb = (double)cols;

  /* Calculate size of wordline drivers assuming rise time == Period / 8 
     - estimate cap on line 
     - compute min resistance to achieve this with RC 
     - compute width needed to achieve this resistance */

  desiredrisetime = Period/16;
  Cline = (gatecappass(Wmemcellr,1.0))*colsb + wordlinelength*CM3metal;
  psize = driver_size(Cline,desiredrisetime);
  
  /* how do we want to do p-n ratioing? -- here we just assume the same ratio 
     from an inverter pair  */
  nsize = psize * Wdecinvn/Wdecinvp; 
  
  if(verbose)
    fprintf(stderr,"Wordline Driver Sizes -- nsize == %f, psize == %f\n",nsize,psize);

  Ceq = draincap(Wdecinvn,NCH,1) + draincap(Wdecinvp,PCH,1) +
    gatecap(nsize+psize,20.0);

  Ctotal+=ports*Ceq;

  if(verbose)
    fprintf(stderr,"Wordline -- Inverter -> Driver         == %g\n",ports*Ceq*Powerfactor);

  /* Compute caps of read wordline and write wordlines 
     - wordline driver caps, given computed width from above
     - read wordlines have 1 nmos access tx, size ~4
     - write wordlines have 2 nmos access tx, size ~2
     - metal line cap
  */

  Cliner = (gatecappass(Wmemcellr,(BitWidth-2*Wmemcellr)/2.0))*colsb+
    wordlinelength*CM3metal+
    2.0*(draincap(nsize,NCH,1) + draincap(psize,PCH,1));
  Clinew = (2.0*gatecappass(Wmemcellw,(BitWidth-2*Wmemcellw)/2.0))*colsb+
    wordlinelength*CM3metal+
    2.0*(draincap(nsize,NCH,1) + draincap(psize,PCH,1));

  if(verbose) {
    fprintf(stderr,"Wordline -- Line                       == %g\n",1e12*Cline);
    fprintf(stderr,"Wordline -- Line -- access -- gatecap  == %g\n",1e12*colsb*2*gatecappass(Wmemcella,(BitWidth-2*Wmemcella)/2.0));
    fprintf(stderr,"Wordline -- Line -- driver -- draincap == %g\n",1e12*draincap(nsize,NCH,1) + draincap(psize,PCH,1));
    fprintf(stderr,"Wordline -- Line -- metal              == %g\n",1e12*wordlinelength*CM3metal);
  }
  Ctotal+=rports*Cliner+wports*Clinew;

  /* AF == 1 assuming a different wordline is charged each cycle, but only
     1 wordline (per port) is actually used */

  return(Ctotal*Powerfactor);
}

//************************************************************************************
double power_t::simple_array_wordline_power(int rows,int cols,int rports,int wports,int cache)
{
  double wordlinelength;
  int ports = rports + wports;
  wordlinelength = cols *  (RegCellWidth + 2 * ports * BitlineSpacing);
  return(array_wordline_power(rows,cols,wordlinelength,rports,wports,cache));
}


//************************************************************************************
double power_t::array_bitline_power(int rows,int cols, double bitlinelength,int rports,int wports,int cache)
{
  double Ctotal=0;
  double Ccolmux=0;
  double Cbitrowr=0;
  double Cbitroww=0;
  double Cprerow=0;
  double Cwritebitdrive=0;
  double Cpregate=0;
  double Cliner=0;
  double Clinew=0;
  int ports;
  double rowsb;
  double colsb;

  double desiredrisetime, Cline, psize, nsize;

  ports = rports + wports;

  rowsb = (double)rows;
  colsb = (double)cols;

  /* Draincaps of access tx's */

  Cbitrowr = draincap(Wmemcellr,NCH,1);
  Cbitroww = draincap(Wmemcellw,NCH,1);

  /* Cprerow -- precharge cap on the bitline
     -simple scheme to estimate size of pre-charge tx's in a similar fashion
      to wordline driver size estimation.
     -FIXME: it would be better to use precharge/keeper pairs, i've omitted this
      from this version because it couldn't autosize as easily.
  */

  desiredrisetime = Period/8;

  Cline = rowsb*Cbitrowr+CM2metal*bitlinelength;
  psize = driver_size(Cline,desiredrisetime);

  /* compensate for not having an nmos pre-charging */
  psize = psize + psize * Wdecinvn/Wdecinvp; 

  if(verbose)
    printf("Cprerow auto   == %g (psize == %g)\n",draincap(psize,PCH,1),psize);

  Cprerow = draincap(psize,PCH,1);

  /* Cpregate -- cap due to gatecap of precharge transistors -- tack this
     onto bitline cap, again this could have a keeper */
  Cpregate = 4.0*gatecap(psize,10.0);
  global_clockcap+=rports*cols*2.0*Cpregate;

  /* Cwritebitdrive -- write bitline drivers are used instead of the precharge
     stuff for write bitlines
     - 2 inverter drivers within each driver pair */

  Cline = rowsb*Cbitroww+CM2metal*bitlinelength;

  psize = driver_size(Cline,desiredrisetime);
  nsize = psize * Wdecinvn/Wdecinvp; 

  Cwritebitdrive = 2.0*(draincap(psize,PCH,1)+draincap(nsize,NCH,1));

  /* 
     reg files (cache==0) 
     => single ended bitlines (1 bitline/col)
     => AFs from pop_count
     caches (cache ==1)
     => double-ended bitlines (2 bitlines/col)
     => AFs = .5 (since one of the two bitlines is always charging/discharging)
  */

#ifdef STATIC_AF
  if (cache == 0) {
    /* compute the total line cap for read/write bitlines */
    Cliner = rowsb*Cbitrowr+CM2metal*bitlinelength+Cprerow;
    Clinew = rowsb*Cbitroww+CM2metal*bitlinelength+Cwritebitdrive;

    /* Bitline inverters at the end of the bitlines (replaced w/ sense amps
       in cache styles) */
    Ccolmux = gatecap(MSCALE*(29.9+7.8),0.0)+gatecap(MSCALE*(47.0+12.0),0.0);
    Ctotal+=(1.0-POPCOUNT_AF)*rports*cols*(Cliner+Ccolmux+2.0*Cpregate);
    Ctotal+=.3*wports*cols*(Clinew+Cwritebitdrive);
  } 
  else { 
    Cliner = rowsb*Cbitrowr+CM2metal*bitlinelength+Cprerow + draincap(Wbitmuxn,NCH,1);
    Clinew = rowsb*Cbitroww+CM2metal*bitlinelength+Cwritebitdrive;
    Ccolmux = (draincap(Wbitmuxn,NCH,1))+2.0*gatecap(WsenseQ1to4,10.0);
    Ctotal+=.5*rports*2.0*cols*(Cliner+Ccolmux+2.0*Cpregate);
    Ctotal+=.5*wports*2.0*cols*(Clinew+Cwritebitdrive);
  }
#else
  if (cache == 0) {
    /* compute the total line cap for read/write bitlines */
    Cliner = rowsb*Cbitrowr+CM2metal*bitlinelength+Cprerow;
    Clinew = rowsb*Cbitroww+CM2metal*bitlinelength+Cwritebitdrive;

    /* Bitline inverters at the end of the bitlines (replaced w/ sense amps
       in cache styles) */
    Ccolmux = gatecap(MSCALE*(29.9+7.8),0.0)+gatecap(MSCALE*(47.0+12.0),0.0);
    Ctotal += rports*cols*(Cliner+Ccolmux+2.0*Cpregate);
    Ctotal += .3*wports*cols*(Clinew+Cwritebitdrive);
  } 
  else { 
    Cliner = rowsb*Cbitrowr+CM2metal*bitlinelength+Cprerow + draincap(Wbitmuxn,NCH,1);
    Clinew = rowsb*Cbitroww+CM2metal*bitlinelength+Cwritebitdrive;
    Ccolmux = (draincap(Wbitmuxn,NCH,1))+2.0*gatecap(WsenseQ1to4,10.0);
    Ctotal+=.5*rports*2.0*cols*(Cliner+Ccolmux+2.0*Cpregate);
    Ctotal+=.5*wports*2.0*cols*(Clinew+Cwritebitdrive);
  }
#endif

  if(verbose) {
    fprintf(stderr,"Bitline -- Precharge                   == %g\n",1e12*Cpregate);
    fprintf(stderr,"Bitline -- Line                        == %g\n",1e12*(Cliner+Clinew));
    fprintf(stderr,"Bitline -- Line -- access draincap     == %g\n",1e12*rowsb*Cbitrowr);
    fprintf(stderr,"Bitline -- Line -- precharge draincap  == %g\n",1e12*Cprerow);
    fprintf(stderr,"Bitline -- Line -- metal               == %g\n",1e12*bitlinelength*CM2metal);
    fprintf(stderr,"Bitline -- Colmux                      == %g\n",1e12*Ccolmux);

    fprintf(stderr,"\n");
  }


  if(cache==0)
    return(Ctotal*Powerfactor);
  else
    return(Ctotal*SensePowerfactor*.4);
  
}

//************************************************************************************
double power_t::simple_array_bitline_power(int rows,int cols,int rports,int wports,int cache)
{
  double bitlinelength;

  int ports = rports + wports;

  bitlinelength = rows * (RegCellHeight + ports * WordlineSpacing);

  return (array_bitline_power(rows,cols,bitlinelength,rports,wports,cache));

}

//************************************************************************************
/* estimate senseamp power dissipation in cache structures (Zyuban's method) */
double power_t::senseamp_power(int cols)
{
  return((double)cols * Vdd/8 * .5e-3);
}

//************************************************************************************
/* estimate comparator power consumption (this comparator is similar
   to the tag-match structure in a CAM */
double power_t::compare_cap(int compare_bits)
{
  double c1, c2;
  /* bottom part of comparator */
  c2 = (compare_bits)*(draincap(Wcompn,NCH,1)+draincap(Wcompn,NCH,2))+
    draincap(Wevalinvp,PCH,1) + draincap(Wevalinvn,NCH,1);

  /* top part of comparator */
  c1 = (compare_bits)*(draincap(Wcompn,NCH,1)+draincap(Wcompn,NCH,2)+
                       draincap(Wcomppreequ,NCH,1)) +
    gatecap(WdecNORn,1.0)+
    gatecap(WdecNORp,3.0);

  return(c1 + c2);
}

//************************************************************************************
/* power of depency check logic */
double power_t::dcl_compare_power(int compare_bits)
{
  double Ctotal;
  int num_comparators;
  
  num_comparators = (ruu_decode_width - 1) * (ruu_decode_width);

  Ctotal = num_comparators * compare_cap(compare_bits);

  return(Ctotal*Powerfactor*AF);
}

//************************************************************************************
double power_t::simple_array_power(int rows,int cols,int rports,int wports,int cache)
{
  if(cache==0)
    return( simple_array_decoder_power(rows,cols,rports,wports,cache)+
            simple_array_wordline_power(rows,cols,rports,wports,cache)+
            simple_array_bitline_power(rows,cols,rports,wports,cache));
  else
    return( simple_array_decoder_power(rows,cols,rports,wports,cache)+
            simple_array_wordline_power(rows,cols,rports,wports,cache)+
            simple_array_bitline_power(rows,cols,rports,wports,cache)+
            senseamp_power(cols));
}


//************************************************************************************
double power_t::cam_tagdrive(int rows,int cols,int rports,int wports)
{
  double Ctotal, Ctlcap, Cblcap, Cwlcap;
  double taglinelength;
  double wordlinelength;
  double nsize, psize;
  int ports;
  Ctotal=0;

  ports = rports + wports;

  taglinelength = rows * 
    (CamCellHeight + ports * MatchlineSpacing);

  wordlinelength = cols * 
    (CamCellWidth + ports * TaglineSpacing);

  /* Compute tagline cap */
  Ctlcap = Cmetal * taglinelength + 
    rows * gatecappass(Wcomparen2,2.0) +
    draincap(Wcompdrivern,NCH,1)+draincap(Wcompdriverp,PCH,1);

  /* Compute bitline cap (for writing new tags) */
  Cblcap = Cmetal * taglinelength +
    rows * draincap(Wmemcellr,NCH,2);

  /* autosize wordline driver */
  psize = driver_size(Cmetal * wordlinelength + 2 * cols * gatecap(Wmemcellr,2.0),Period/8);
  nsize = psize * Wdecinvn/Wdecinvp; 

  /* Compute wordline cap (for writing new tags) */
  Cwlcap = Cmetal * wordlinelength + 
    draincap(nsize,NCH,1)+draincap(psize,PCH,1) +
    2 * cols * gatecap(Wmemcellr,2.0);
    
  Ctotal += (rports * cols * 2 * Ctlcap) + 
    (wports * ((cols * 2 * Cblcap) + (rows * Cwlcap)));

  return(Ctotal*Powerfactor*AF);
}

//************************************************************************************
double power_t::cam_tagmatch(int rows,int cols,int rports,int wports)
{
  double Ctotal, Cmlcap;
  double matchlinelength;
  int ports;
  Ctotal=0;

  ports = rports + wports;

  matchlinelength = cols * 
    (CamCellWidth + ports * TaglineSpacing);

  Cmlcap = 2 * cols * draincap(Wcomparen1,NCH,2) + 
    Cmetal * matchlinelength + draincap(Wmatchpchg,NCH,1) +
    gatecap(Wmatchinvn+Wmatchinvp,10.0) +
    gatecap(Wmatchnandn+Wmatchnandp,10.0);

  Ctotal += rports * rows * Cmlcap;

  global_clockcap += rports * rows * gatecap(Wmatchpchg,5.0);
  
  /* noring the nanded match lines */
  if(ruu_issue_width >= 8)
    Ctotal += 2 * gatecap(Wmatchnorn+Wmatchnorp,10.0);

  return(Ctotal*Powerfactor*AF);
}

//************************************************************************************
double power_t::cam_array(int rows,int cols,int rports,int wports)
{
  return(cam_tagdrive(rows,cols,rports,wports) +
         cam_tagmatch(rows,cols,rports,wports));
}

//************************************************************************************
double power_t::selection_power(int win_entries)
{
  double Ctotal, Cor, Cpencode;
  int num_arbiter=1;

  Ctotal=0;

  while(win_entries > 4)
    {
      win_entries = (int)ceil((double)win_entries / 4.0);
      num_arbiter += win_entries;
    }

  Cor = 4 * draincap(WSelORn,NCH,1) + draincap(WSelORprequ,PCH,1);

  Cpencode = draincap(WSelPn,NCH,1) + draincap(WSelPp,PCH,1) + 
    2*draincap(WSelPn,NCH,1) + draincap(WSelPp,PCH,2) + 
    3*draincap(WSelPn,NCH,1) + draincap(WSelPp,PCH,3) + 
    4*draincap(WSelPn,NCH,1) + draincap(WSelPp,PCH,4) + 
    4*gatecap(WSelEnn+WSelEnp,20.0) + 
    4*draincap(WSelEnn,NCH,1) + 4*draincap(WSelEnp,PCH,1);

  Ctotal += ruu_issue_width * num_arbiter*(Cor+Cpencode);

  return(Ctotal*Powerfactor*AF);
}

//************************************************************************************
/* very rough clock power estimates */
double power_t::total_clockpower(double die_length)
{
  double clocklinelength;
  double Cline,Cline2,Ctotal;
  double pipereg_clockcap=0;
  double global_buffercap = 0;
  double Clockpower;

  double num_piperegs;

  int npreg_width = (int)ceil(logtwo((double)MD_NUM_PHYS_IREGS+(double)MD_NUM_PHYS_FREGS));

  /* Assume say 8 stages (kinda low now).
     FIXME: this could be a lot better; user could input
     number of pipestages, etc  */

  /* assume 8 pipe stages and try to estimate bits per pipe stage */
  /* pipe stage 0/1 */
  num_piperegs = ruu_issue_width*inst_length + data_width;
  /* pipe stage 1/2 */
  num_piperegs += ruu_issue_width*(inst_length + 3 * RUU_size);
  /* pipe stage 2/3 */
  num_piperegs += ruu_issue_width*(inst_length + 3 * RUU_size);
  /* pipe stage 3/4 */
  num_piperegs += ruu_issue_width*(3 * npreg_width + pow2(opcode_length));
  /* pipe stage 4/5 */
  num_piperegs += ruu_issue_width*(2*data_width + pow2(opcode_length));
  /* pipe stage 5/6 */
  num_piperegs += ruu_issue_width*(data_width + pow2(opcode_length));
  /* pipe stage 6/7 */
  num_piperegs += ruu_issue_width*(data_width + pow2(opcode_length));
  /* pipe stage 7/8 */
  num_piperegs += ruu_issue_width*(data_width + pow2(opcode_length));

  /* assume 50% extra in control signals (rule of thumb) */
  num_piperegs = num_piperegs * 1.5;

  pipereg_clockcap = num_piperegs * 4*gatecap(10.0,0);

  /* estimate based on 3% of die being in clock metal */
  Cline2 = Cmetal * (.03 * die_length * die_length/BitlineSpacing) * 1e6 * 1e6;

  /* another estimate */
  clocklinelength = die_length*(.5 + 4 * (.25 + 2*(.25) + 4 * (.125)));
  Cline = 20 * Cmetal * (clocklinelength) * 1e6;
  global_buffercap = 12*gatecap(1000.0,10.0)+16*gatecap(200,10.0)+16*8*2*gatecap(100.0,10.00) + 2*gatecap(.29*1e6,10.0);
  /* global_clockcap is computed within each array structure for pre-charge tx's*/
  Ctotal = Cline+global_clockcap+pipereg_clockcap+global_buffercap;

  if(verbose)
    fprintf(stderr,"num_piperegs == %f\n",num_piperegs);

  /* add I_ADD Clockcap and F_ADD Clockcap */
  Clockpower = Ctotal*Powerfactor + res_ialu*I_ADD_CLOCK + res_fpalu*F_ADD_CLOCK;

  if(verbose) {
    fprintf(stderr,"Global Clock Power: %g\n",Clockpower);
    fprintf(stderr," Global Metal Lines   (W): %g\n",Cline*Powerfactor);
    fprintf(stderr," Global Metal Lines (3%%) (W): %g\n",Cline2*Powerfactor);
    fprintf(stderr," Global Clock Buffers (W): %g\n",global_buffercap*Powerfactor);
    fprintf(stderr," Global Clock Cap (Explicit) (W): %g\n",global_clockcap*Powerfactor+I_ADD_CLOCK+F_ADD_CLOCK);
    fprintf(stderr," Global Clock Cap (Implicit) (W): %g\n",pipereg_clockcap*Powerfactor);
  }
  return(Clockpower);

}

//************************************************************************************
/* very rough global clock power estimates */
double power_t::global_clockpower(double die_length)
{
  double clocklinelength;
  double Cline,Cline2,Ctotal;
  double global_buffercap = 0;

  Cline2 = Cmetal * (.03 * die_length * die_length/BitlineSpacing) * 1e6 * 1e6;

  clocklinelength = die_length*(.5 + 4 * (.25 + 2*(.25) + 4 * (.125)));
  Cline = 20 * Cmetal * (clocklinelength) * 1e6;
  global_buffercap = 12*gatecap(1000.0,10.0)+16*gatecap(200,10.0)+16*8*2*gatecap(100.0,10.00) + 2*gatecap(.29*1e6,10.0);
  Ctotal = Cline+global_buffercap;

  if(verbose) {
    fprintf(stderr,"Global Clock Power: %g\n",Ctotal*Powerfactor);
    fprintf(stderr," Global Metal Lines   (W): %g\n",Cline*Powerfactor);
    fprintf(stderr," Global Metal Lines (3%%) (W): %g\n",Cline2*Powerfactor);
    fprintf(stderr," Global Clock Buffers (W): %g\n",global_buffercap*Powerfactor);
  }

  return(Ctotal*Powerfactor);

}

//************************************************************************************
double power_t::compute_resultbus_power()
{
  double Ctotal, Cline;

  double regfile_height;

  /* compute size of result bus tags */
  int npreg_width = (int)ceil(logtwo((double)MD_NUM_PHYS_IREGS+(double)MD_NUM_PHYS_FREGS));

  Ctotal=0;

  regfile_height = RUU_size * (RegCellHeight + 
                               WordlineSpacing * 3 * ruu_issue_width); 

  /* assume num alu's == ialu  (FIXME: generate a more detailed result bus network model*/
  Cline = Cmetal * (regfile_height + .5 * res_ialu * 3200.0 * LSCALE);

  /* or use result bus length measured from 21264 die photo */
  /*  Cline = Cmetal * 3.3*1000;*/

  /* Assume ruu_issue_width result busses -- power can be scaled linearly
     for number of result busses (scale by writeback_access) */
  Ctotal += 2.0 * (data_width + npreg_width) * (ruu_issue_width)* Cline;

#ifdef STATIC_AF
  return(Ctotal*Powerfactor*AF);
#else
  return(Ctotal*Powerfactor);
#endif
  
}

//************************************************************************************
void power_t::calculate_power()
{
  double clockpower;
  double predeclength, wordlinelength, bitlinelength;
  int ndwl, ndbl, nspd, ntwl, ntbl, ntspd, c,b,a,cache, rowsb, colsb;
  int trowsb, tcolsb, tagsize;
  int va_size = 48;

  int npreg_width = (int)ceil(logtwo((double)MD_NUM_PHYS_IREGS+(double)MD_NUM_PHYS_FREGS));

  /* these variables are needed to use Cacti to auto-size cache arrays 
     (for optimal delay) */
  time_result_type time_result;
  time_parameter_type time_parameters;

  /* used to autosize other structures, like bpred tables */
  int scale_factor;

  global_clockcap = 0;

  cache=0;


  /* FIXME: ALU power is a simple constant, it would be better
     to include bit AFs and have different numbers for different
     types of operations */
  power->ialu_power = res_ialu * I_ADD;
  power->falu_power = res_fpalu * F_ADD;

  /* LUKE - COMPRESSION POWER */
  // compression alus are simple int ALUS
  if(COMPRESSION_POWER){
    ASSERT(compression_alus != 0);
    power->compression_power = compression_alus * I_ADD;
  }
  else{
    power->compression_power = 0;
  }
 


  nvreg_width = (int)ceil(logtwo((double)MD_NUM_IREGS+(double) MD_NUM_FREGS));
  ASSERT(MD_NUM_PHYS_IREGS > 0);
  ASSERT(MD_NUM_PHYS_FREGS > 0);
  npreg_width = (int)ceil(logtwo((double)MD_NUM_PHYS_IREGS+(double)MD_NUM_PHYS_FREGS));


  /* RAT has shadow bits stored in each cell, this makes the
     cell size larger than normal array structures, so we must
     compute it here */

  predeclength = (MD_NUM_IREGS+MD_NUM_FREGS) * 
    (RatCellHeight + 3 * ruu_decode_width * WordlineSpacing);

  wordlinelength = npreg_width * 
    (RatCellWidth + 
     6 * ruu_decode_width * BitlineSpacing + 
     RatShiftRegWidth*RatNumShift);

  bitlinelength = (MD_NUM_IREGS+MD_NUM_FREGS) * (RatCellHeight + 3 * ruu_decode_width * WordlineSpacing);

  if(verbose)
    fprintf(stderr,"rat power stats\n");
  power->rat_decoder = array_decoder_power(MD_NUM_IREGS+MD_NUM_FREGS,npreg_width,predeclength,2*ruu_decode_width,ruu_decode_width,cache);
  power->rat_wordline = array_wordline_power(MD_NUM_IREGS+MD_NUM_FREGS,npreg_width,wordlinelength,2*ruu_decode_width,ruu_decode_width,cache);
  power->rat_bitline = array_bitline_power(MD_NUM_IREGS+MD_NUM_FREGS,npreg_width,bitlinelength,2*ruu_decode_width,ruu_decode_width,cache);
  power->rat_senseamp = 0;

  power->dcl_compare = dcl_compare_power(nvreg_width);
  power->dcl_pencode = 0;
  power->inst_decoder_power = ruu_decode_width * simple_array_decoder_power(opcode_length,1,1,1,cache);
  power->wakeup_tagdrive =cam_tagdrive(RUU_size,npreg_width,ruu_issue_width,ruu_issue_width);
  power->wakeup_tagmatch =cam_tagmatch(RUU_size,npreg_width,ruu_issue_width,ruu_issue_width);
  power->wakeup_ormatch =0; 

  power->selection = selection_power(RUU_size);


  predeclength = (MD_NUM_IREGS+MD_NUM_FREGS) * (RegCellHeight + 3 * ruu_issue_width * WordlineSpacing);

  wordlinelength = data_width * 
    (RegCellWidth + 
     6 * ruu_issue_width * BitlineSpacing);

  bitlinelength = (MD_NUM_IREGS+MD_NUM_FREGS) * (RegCellHeight + 3 * ruu_issue_width * WordlineSpacing);

  if(verbose)
    fprintf(stderr,"regfile power stats\n");

  power->regfile_decoder = array_decoder_power(MD_NUM_IREGS+MD_NUM_FREGS,data_width,predeclength,2*ruu_issue_width,ruu_issue_width,cache);
  power->regfile_wordline = array_wordline_power(MD_NUM_IREGS+MD_NUM_FREGS,data_width,wordlinelength,2*ruu_issue_width,ruu_issue_width,cache);
  power->regfile_bitline = array_bitline_power(MD_NUM_IREGS+MD_NUM_FREGS,data_width,bitlinelength,2*ruu_issue_width,ruu_issue_width,cache);
  power->regfile_senseamp =0;

  predeclength = RUU_size * (RegCellHeight + 3 * ruu_issue_width * WordlineSpacing);

  wordlinelength = data_width * 
    (RegCellWidth + 
     6 * ruu_issue_width * BitlineSpacing);

  bitlinelength = RUU_size * (RegCellHeight + 3 * ruu_issue_width * WordlineSpacing);

  if(verbose)
    fprintf(stderr,"res station power stats\n");
  power->rs_decoder = array_decoder_power(RUU_size,data_width,predeclength,2*ruu_issue_width,ruu_issue_width,cache);
  power->rs_wordline = array_wordline_power(RUU_size,data_width,wordlinelength,2*ruu_issue_width,ruu_issue_width,cache);
  power->rs_bitline = array_bitline_power(RUU_size,data_width,bitlinelength,2*ruu_issue_width,ruu_issue_width,cache);
  /* no senseamps in reg file structures (only caches) */
  power->rs_senseamp =0;

  /* addresses go into lsq tag's */
  power->lsq_wakeup_tagdrive =cam_tagdrive(LSQ_size,data_width,res_memport,res_memport);
  power->lsq_wakeup_tagmatch =cam_tagmatch(LSQ_size,data_width,res_memport,res_memport);
  power->lsq_wakeup_ormatch =0; 

  wordlinelength = data_width * 
    (RegCellWidth + 
     4 * res_memport * BitlineSpacing);

  bitlinelength = RUU_size * (RegCellHeight + 4 * res_memport * WordlineSpacing);

  /* rs's hold data */
  if(verbose)
    fprintf(stderr,"lsq station power stats\n");
  power->lsq_rs_decoder = array_decoder_power(LSQ_size,data_width,predeclength,res_memport,res_memport,cache);
  power->lsq_rs_wordline = array_wordline_power(LSQ_size,data_width,wordlinelength,res_memport,res_memport,cache);
  power->lsq_rs_bitline = array_bitline_power(LSQ_size,data_width,bitlinelength,res_memport,res_memport,cache);
  power->lsq_rs_senseamp =0;

  power->resultbus = compute_resultbus_power();

  /* Load cache values into what cacti is expecting */
  time_parameters.cache_size = btb_config[0] * (data_width/8) * btb_config[1]; /* C */
  time_parameters.block_size = (data_width/8); /* B */
  time_parameters.associativity = btb_config[1]; /* A */
  time_parameters.number_of_sets = btb_config[0]; /* C/(B*A) */

  /* have Cacti compute optimal cache config */
  calculate_time(&time_result,&time_parameters);
  output_data(&time_result,&time_parameters);

  /* extract Cacti results */
  ndwl=time_result.best_Ndwl;
  ndbl=time_result.best_Ndbl;
  nspd=time_result.best_Nspd;
  ntwl=time_result.best_Ntwl;
  ntbl=time_result.best_Ntbl;
  ntspd=time_result.best_Ntspd;
  c = time_parameters.cache_size;
  b = time_parameters.block_size;
  a = time_parameters.associativity; 

  cache=1;

  /* Figure out how many rows/cols there are now */
  rowsb = c/(b*a*ndbl*nspd);
  colsb = 8*b*a*nspd/ndwl;

  if(verbose) {
    fprintf(stderr,"%d KB %d-way btb (%d-byte block size):\n",c,a,b);
    fprintf(stderr,"ndwl == %d, ndbl == %d, nspd == %d\n",ndwl,ndbl,nspd);
    fprintf(stderr,"%d sets of %d rows x %d cols\n",ndwl*ndbl,rowsb,colsb);
  }

  predeclength = rowsb * (RegCellHeight + WordlineSpacing);
  wordlinelength = colsb *  (RegCellWidth + BitlineSpacing);
  bitlinelength = rowsb * (RegCellHeight + WordlineSpacing);

  if(verbose)
    fprintf(stderr,"btb power stats\n");
  power->btb = ndwl*ndbl*(array_decoder_power(rowsb,colsb,predeclength,1,1,cache) + array_wordline_power(rowsb,colsb,wordlinelength,1,1,cache) + array_bitline_power(rowsb,colsb,bitlinelength,1,1,cache) + senseamp_power(colsb));

  cache=1;

  scale_factor = squarify(twolev_config[0],twolev_config[2]);
  predeclength = (twolev_config[0] / scale_factor)* (RegCellHeight + WordlineSpacing);
  wordlinelength = twolev_config[2] * scale_factor *  (RegCellWidth + BitlineSpacing);
  bitlinelength = (twolev_config[0] / scale_factor) * (RegCellHeight + WordlineSpacing);

  if(verbose)
    fprintf(stderr,"local predict power stats\n");

  power->local_predict = array_decoder_power(twolev_config[0]/scale_factor,twolev_config[2]*scale_factor,predeclength,1,1,cache) + array_wordline_power(twolev_config[0]/scale_factor,twolev_config[2]*scale_factor,wordlinelength,1,1,cache) + array_bitline_power(twolev_config[0]/scale_factor,twolev_config[2]*scale_factor,bitlinelength,1,1,cache) + senseamp_power(twolev_config[2]*scale_factor);

  scale_factor = squarify(twolev_config[1],3);

  predeclength = (twolev_config[1] / scale_factor)* (RegCellHeight + WordlineSpacing);
  wordlinelength = 3 * scale_factor *  (RegCellWidth + BitlineSpacing);
  bitlinelength = (twolev_config[1] / scale_factor) * (RegCellHeight + WordlineSpacing);


  if(verbose)
    fprintf(stderr,"local predict power stats\n");
  power->local_predict += array_decoder_power(twolev_config[1]/scale_factor,3*scale_factor,predeclength,1,1,cache) + array_wordline_power(twolev_config[1]/scale_factor,3*scale_factor,wordlinelength,1,1,cache) + array_bitline_power(twolev_config[1]/scale_factor,3*scale_factor,bitlinelength,1,1,cache) + senseamp_power(3*scale_factor);

  if(verbose)
    fprintf(stderr,"bimod_config[0] == %d\n",bimod_config[0]);

  scale_factor = squarify(bimod_config[0],2);

  predeclength = bimod_config[0]/scale_factor * (RegCellHeight + WordlineSpacing);
  wordlinelength = 2*scale_factor *  (RegCellWidth + BitlineSpacing);
  bitlinelength = bimod_config[0]/scale_factor * (RegCellHeight + WordlineSpacing);


  if(verbose)
    fprintf(stderr,"global predict power stats\n");
  power->global_predict = array_decoder_power(bimod_config[0]/scale_factor,2*scale_factor,predeclength,1,1,cache) + array_wordline_power(bimod_config[0]/scale_factor,2*scale_factor,wordlinelength,1,1,cache) + array_bitline_power(bimod_config[0]/scale_factor,2*scale_factor,bitlinelength,1,1,cache) + senseamp_power(2*scale_factor);

  scale_factor = squarify(comb_config[0],2);

  predeclength = comb_config[0]/scale_factor * (RegCellHeight + WordlineSpacing);
  wordlinelength = 2*scale_factor *  (RegCellWidth + BitlineSpacing);
  bitlinelength = comb_config[0]/scale_factor * (RegCellHeight + WordlineSpacing);

  if(verbose)
    fprintf(stderr,"chooser predict power stats\n");
  power->chooser = array_decoder_power(comb_config[0]/scale_factor,2*scale_factor,predeclength,1,1,cache) + array_wordline_power(comb_config[0]/scale_factor,2*scale_factor,wordlinelength,1,1,cache) + array_bitline_power(comb_config[0]/scale_factor,2*scale_factor,bitlinelength,1,1,cache) + senseamp_power(2*scale_factor);

  if(verbose)
    fprintf(stderr,"RAS predict power stats\n");
  power->ras = simple_array_power(ras_size,data_width,1,1,0);

  tagsize = va_size - ((int)logtwo(cache_dl1->nsets) + (int)logtwo(cache_dl1->bsize));

  if(verbose)
    fprintf(stderr,"dtlb predict power stats\n");
  power->dtlb = res_memport*(cam_array(dtlb->nsets, va_size - (int)logtwo((double)dtlb->bsize),1,1) + simple_array_power(dtlb->nsets,tagsize,1,1,cache));

  tagsize = va_size - ((int)logtwo(cache_il1->nsets) + (int)logtwo(cache_il1->bsize));

  predeclength = itlb->nsets * (RegCellHeight + WordlineSpacing);
  wordlinelength = logtwo((double)itlb->bsize) * (RegCellWidth + BitlineSpacing);
  bitlinelength = itlb->nsets * (RegCellHeight + WordlineSpacing);

  if(verbose)
    fprintf(stderr,"itlb predict power stats\n");
  power->itlb = cam_array(itlb->nsets, va_size - (int)logtwo((double)itlb->bsize),1,1) + simple_array_power(itlb->nsets,tagsize,1,1,cache);

  //LUKE - calculate prefetcher power...
  // Each prefetcher consists of 3 filter tables (simple arrays) and 1 stream table (modeled as cam_array)
  if(PREFETCHER_POWER){
    int prefetch_instr_tagsize = va_size - (int)logtwo(cache_il1->bsize);
    int prefetch_data_tagsize = va_size - (int)logtwo(cache_dl1->bsize);
    power->instr_prefetcher_power = 2*simple_array_power(UNIT_PF_FILTERSIZE, prefetch_instr_tagsize, 1, 1, cache) +
      simple_array_power(NONUNIT_PF_FILTERSIZE, prefetch_instr_tagsize, 1, 1, cache) +
      cam_array(L1I_PF_STREAMS, prefetch_instr_tagsize+10, 1, 1) + simple_array_power(L1I_PF_STREAMS, prefetch_instr_tagsize,1,1,cache);
    power->data_prefetcher_power = 2*simple_array_power(UNIT_PF_FILTERSIZE, prefetch_data_tagsize, 1, 1, cache) +
      simple_array_power(NONUNIT_PF_FILTERSIZE, prefetch_data_tagsize, 1, 1, cache) +
      cam_array(L1D_PF_STREAMS, prefetch_data_tagsize+10, 1, 1) + simple_array_power(L1D_PF_STREAMS, prefetch_data_tagsize,1,1,cache);
  }
  else{
    //prefetcher not used..
    power->instr_prefetcher_power = 0;
    power->data_prefetcher_power = 0;
  }

  cache=1;

  time_parameters.cache_size = cache_il1->nsets * cache_il1->bsize * cache_il1->assoc; /* C */
  time_parameters.block_size = cache_il1->bsize; /* B */
  time_parameters.associativity = cache_il1->assoc; /* A */
  time_parameters.number_of_sets = cache_il1->nsets; /* C/(B*A) */

  calculate_time(&time_result,&time_parameters);
  output_data(&time_result,&time_parameters);

  ndwl=time_result.best_Ndwl;
  ndbl=time_result.best_Ndbl;
  nspd=time_result.best_Nspd;
  ntwl=time_result.best_Ntwl;
  ntbl=time_result.best_Ntbl;
  ntspd=time_result.best_Ntspd;

  c = time_parameters.cache_size;
  b = time_parameters.block_size;
  a = time_parameters.associativity;

  rowsb = c/(b*a*ndbl*nspd);
  colsb = 8*b*a*nspd/ndwl;

  tagsize = va_size - ((int)logtwo(cache_il1->nsets) + (int)logtwo(cache_il1->bsize));
  trowsb = c/(b*a*ntbl*ntspd);
  tcolsb = a * (tagsize + 1 + 6) * ntspd/ntwl;
 
  if(verbose) {
    fprintf(stderr,"%d KB %d-way cache (%d-byte block size):\n",c,a,b);
    fprintf(stderr,"ndwl == %d, ndbl == %d, nspd == %d\n",ndwl,ndbl,nspd);
    fprintf(stderr,"%d sets of %d rows x %d cols\n",ndwl*ndbl,rowsb,colsb);
    fprintf(stderr,"tagsize == %d\n",tagsize);
  }

  predeclength = rowsb * (RegCellHeight + WordlineSpacing);
  wordlinelength = colsb *  (RegCellWidth + BitlineSpacing);
  bitlinelength = rowsb * (RegCellHeight + WordlineSpacing);

  if(verbose)
    fprintf(stderr,"icache power stats\n");
  power->icache_decoder = ndwl*ndbl*array_decoder_power(rowsb,colsb,predeclength,1,1,cache);
  power->icache_wordline = ndwl*ndbl*array_wordline_power(rowsb,colsb,wordlinelength,1,1,cache);
  power->icache_bitline = ndwl*ndbl*array_bitline_power(rowsb,colsb,bitlinelength,1,1,cache);
  power->icache_senseamp = ndwl*ndbl*senseamp_power(colsb);
  power->icache_tagarray = ntwl*ntbl*(simple_array_power(trowsb,tcolsb,1,1,cache));

  power->icache_power = power->icache_decoder + power->icache_wordline + power->icache_bitline + power->icache_senseamp + power->icache_tagarray;

  time_parameters.cache_size = cache_dl1->nsets * cache_dl1->bsize * cache_dl1->assoc; /* C */
  time_parameters.block_size = cache_dl1->bsize; /* B */
  time_parameters.associativity = cache_dl1->assoc; /* A */
  time_parameters.number_of_sets = cache_dl1->nsets; /* C/(B*A) */

  calculate_time(&time_result,&time_parameters);
  output_data(&time_result,&time_parameters);

  ndwl=time_result.best_Ndwl;
  ndbl=time_result.best_Ndbl;
  nspd=time_result.best_Nspd;
  ntwl=time_result.best_Ntwl;
  ntbl=time_result.best_Ntbl;
  ntspd=time_result.best_Ntspd;
  c = time_parameters.cache_size;
  b = time_parameters.block_size;
  a = time_parameters.associativity; 

  cache=1;

  rowsb = c/(b*a*ndbl*nspd);
  colsb = 8*b*a*nspd/ndwl;

  tagsize = va_size - ((int)logtwo(cache_dl1->nsets) + (int)logtwo(cache_dl1->bsize));
  trowsb = c/(b*a*ntbl*ntspd);
  tcolsb = a * (tagsize + 1 + 6) * ntspd/ntwl;

  if(verbose) {
    fprintf(stderr,"%d KB %d-way cache (%d-byte block size):\n",c,a,b);
    fprintf(stderr,"ndwl == %d, ndbl == %d, nspd == %d\n",ndwl,ndbl,nspd);
    fprintf(stderr,"%d sets of %d rows x %d cols\n",ndwl*ndbl,rowsb,colsb);
    fprintf(stderr,"tagsize == %d\n",tagsize);

    fprintf(stderr,"\nntwl == %d, ntbl == %d, ntspd == %d\n",ntwl,ntbl,ntspd);
    fprintf(stderr,"%d sets of %d rows x %d cols\n",ntwl*ntbl,trowsb,tcolsb);
  }

  predeclength = rowsb * (RegCellHeight + WordlineSpacing);
  wordlinelength = colsb *  (RegCellWidth + BitlineSpacing);
  bitlinelength = rowsb * (RegCellHeight + WordlineSpacing);

  if(verbose)
    fprintf(stderr,"dcache power stats\n");
  power->dcache_decoder = res_memport*ndwl*ndbl*array_decoder_power(rowsb,colsb,predeclength,1,1,cache);
  power->dcache_wordline = res_memport*ndwl*ndbl*array_wordline_power(rowsb,colsb,wordlinelength,1,1,cache);
  power->dcache_bitline = res_memport*ndwl*ndbl*array_bitline_power(rowsb,colsb,bitlinelength,1,1,cache);
  power->dcache_senseamp = res_memport*ndwl*ndbl*senseamp_power(colsb);
  power->dcache_tagarray = res_memport*ntwl*ntbl*(simple_array_power(trowsb,tcolsb,1,1,cache));

  power->dcache_power = power->dcache_decoder + power->dcache_wordline + power->dcache_bitline + power->dcache_senseamp + power->dcache_tagarray;

  clockpower = total_clockpower(.018);
  power->clock_power = clockpower;
  if(verbose) {
    fprintf(stderr,"result bus power == %f\n",power->resultbus);
    fprintf(stderr,"global clock power == %f\n",clockpower);
  }

  time_parameters.cache_size = cache_dl2->nsets * cache_dl2->bsize * cache_dl2->assoc; /* C */
  time_parameters.block_size = cache_dl2->bsize; /* B */
  time_parameters.associativity = cache_dl2->assoc; /* A */
  time_parameters.number_of_sets = cache_dl2->nsets; /* C/(B*A) */

  calculate_time(&time_result,&time_parameters);
  output_data(&time_result,&time_parameters);

  ndwl=time_result.best_Ndwl;
  ndbl=time_result.best_Ndbl;
  nspd=time_result.best_Nspd;
  ntwl=time_result.best_Ntwl;
  ntbl=time_result.best_Ntbl;
  ntspd=time_result.best_Ntspd;
  c = time_parameters.cache_size;
  b = time_parameters.block_size;
  a = time_parameters.associativity;

  rowsb = c/(b*a*ndbl*nspd);
  colsb = 8*b*a*nspd/ndwl;

  tagsize = va_size - ((int)logtwo(cache_dl2->nsets) + (int)logtwo(cache_dl2->bsize));
  trowsb = c/(b*a*ntbl*ntspd);

  /* COMPRESSION POWER */
  //LUKE: in order to account for compressed L2 cache we need to double the tag array size
  if(COMPRESSION_POWER){
     tcolsb = (2*a) * (tagsize + 1 + 6) * ntspd/ntwl;
  }
  else{
    tcolsb = a * (tagsize + 1 + 6) * ntspd/ntwl;
  }

  if(verbose) {
    fprintf(stderr,"%d KB %d-way cache (%d-byte block size):\n",c,a,b);
    fprintf(stderr,"ndwl == %d, ndbl == %d, nspd == %d\n",ndwl,ndbl,nspd);
    fprintf(stderr,"%d sets of %d rows x %d cols\n",ndwl*ndbl,rowsb,colsb);
    fprintf(stderr,"tagsize == %d\n",tagsize);
  }

  predeclength = rowsb * (RegCellHeight + WordlineSpacing);
  wordlinelength = colsb *  (RegCellWidth + BitlineSpacing);
  bitlinelength = rowsb * (RegCellHeight + WordlineSpacing);

  if(verbose)
    fprintf(stderr,"dcache2 power stats\n");
  power->dcache2_decoder = array_decoder_power(rowsb,colsb,predeclength,1,1,cache);
  power->dcache2_wordline = array_wordline_power(rowsb,colsb,wordlinelength,1,1,cache);
  power->dcache2_bitline = array_bitline_power(rowsb,colsb,bitlinelength,1,1,cache);
  power->dcache2_senseamp = senseamp_power(colsb);
  power->dcache2_tagarray = simple_array_power(trowsb,tcolsb,1,1,cache);

  power->dcache2_power = power->dcache2_decoder + power->dcache2_wordline + power->dcache2_bitline + power->dcache2_senseamp + power->dcache2_tagarray;

  power->rat_decoder *= crossover_scaling;
  power->rat_wordline *= crossover_scaling;
  power->rat_bitline *= crossover_scaling;

  power->dcl_compare *= crossover_scaling;
  power->dcl_pencode *= crossover_scaling;
  power->inst_decoder_power *= crossover_scaling;
  power->wakeup_tagdrive *= crossover_scaling;
  power->wakeup_tagmatch *= crossover_scaling;
  power->wakeup_ormatch *= crossover_scaling;

  power->selection *= crossover_scaling;

  power->regfile_decoder *= crossover_scaling;
  power->regfile_wordline *= crossover_scaling;
  power->regfile_bitline *= crossover_scaling;
  power->regfile_senseamp *= crossover_scaling;

  power->rs_decoder *= crossover_scaling;
  power->rs_wordline *= crossover_scaling;
  power->rs_bitline *= crossover_scaling;
  power->rs_senseamp *= crossover_scaling;

  power->lsq_wakeup_tagdrive *= crossover_scaling;
  power->lsq_wakeup_tagmatch *= crossover_scaling;

  power->lsq_rs_decoder *= crossover_scaling;
  power->lsq_rs_wordline *= crossover_scaling;
  power->lsq_rs_bitline *= crossover_scaling;
  power->lsq_rs_senseamp *= crossover_scaling;
 
  power->resultbus *= crossover_scaling;

  power->btb *= crossover_scaling;
  power->local_predict *= crossover_scaling;
  power->global_predict *= crossover_scaling;
  power->chooser *= crossover_scaling;

  power->dtlb *= crossover_scaling;

  power->itlb *= crossover_scaling;

  power->icache_decoder *= crossover_scaling;
  power->icache_wordline*= crossover_scaling;
  power->icache_bitline *= crossover_scaling;
  power->icache_senseamp*= crossover_scaling;
  power->icache_tagarray*= crossover_scaling;

  power->icache_power *= crossover_scaling;

  power->dcache_decoder *= crossover_scaling;
  power->dcache_wordline *= crossover_scaling;
  power->dcache_bitline *= crossover_scaling;
  power->dcache_senseamp *= crossover_scaling;
  power->dcache_tagarray *= crossover_scaling;

  power->dcache_power *= crossover_scaling;
  
  power->clock_power *= crossover_scaling;

  power->dcache2_decoder *= crossover_scaling;
  power->dcache2_wordline *= crossover_scaling;
  power->dcache2_bitline *= crossover_scaling;
  power->dcache2_senseamp *= crossover_scaling;
  power->dcache2_tagarray *= crossover_scaling;

  power->dcache2_power *= crossover_scaling;

  power->total_power = power->local_predict + power->global_predict + 
    power->chooser + power->btb +
    power->rat_decoder + power->rat_wordline + 
    power->rat_bitline + power->rat_senseamp + 
    power->dcl_compare + power->dcl_pencode + 
    power->inst_decoder_power +
    power->wakeup_tagdrive + power->wakeup_tagmatch + 
    power->selection +
    power->regfile_decoder + power->regfile_wordline + 
    power->regfile_bitline + power->regfile_senseamp +  
    power->rs_decoder + power->rs_wordline +
    power->rs_bitline + power->rs_senseamp + 
    power->lsq_wakeup_tagdrive + power->lsq_wakeup_tagmatch +
    power->lsq_rs_decoder + power->lsq_rs_wordline +
    power->lsq_rs_bitline + power->lsq_rs_senseamp +
    power->resultbus +
    power->clock_power +
    power->icache_power + 
    power->itlb + 
    power->dcache_power + 
    power->dtlb + 
    power->dcache2_power +
    power->instr_prefetcher_power +
    power->data_prefetcher_power +
    power->compression_power;

  power->total_power_nodcache2 =power->local_predict + power->global_predict + 
    power->chooser + power->btb +
    power->rat_decoder + power->rat_wordline + 
    power->rat_bitline + power->rat_senseamp + 
    power->dcl_compare + power->dcl_pencode + 
    power->inst_decoder_power +
    power->wakeup_tagdrive + power->wakeup_tagmatch + 
    power->selection +
    power->regfile_decoder + power->regfile_wordline + 
    power->regfile_bitline + power->regfile_senseamp +  
    power->rs_decoder + power->rs_wordline +
    power->rs_bitline + power->rs_senseamp + 
    power->lsq_wakeup_tagdrive + power->lsq_wakeup_tagmatch +
    power->lsq_rs_decoder + power->lsq_rs_wordline +
    power->lsq_rs_bitline + power->lsq_rs_senseamp +
    power->resultbus +
    power->clock_power +
    power->icache_power + 
    power->itlb + 
    power->dcache_power + 
    power->dtlb + 
    power->dcache2_power +
    power->instr_prefetcher_power +
    power->data_prefetcher_power +
    power->compression_power;

  power->bpred_power = power->btb + power->local_predict + power->global_predict + power->chooser + power->ras;

  power->rat_power = power->rat_decoder + 
    power->rat_wordline + power->rat_bitline + power->rat_senseamp;

  power->dcl_power = power->dcl_compare + power->dcl_pencode;

  power->rename_power = power->rat_power + 
    power->dcl_power + 
    power->inst_decoder_power;

  power->wakeup_power = power->wakeup_tagdrive + power->wakeup_tagmatch + 
    power->wakeup_ormatch;

  power->rs_power = power->rs_decoder + 
    power->rs_wordline + power->rs_bitline + power->rs_senseamp;

  power->rs_power_nobit = power->rs_decoder + 
    power->rs_wordline + power->rs_senseamp;

  power->window_power = power->wakeup_power + power->rs_power + 
    power->selection;

  power->lsq_rs_power = power->lsq_rs_decoder + 
    power->lsq_rs_wordline + power->lsq_rs_bitline + 
    power->lsq_rs_senseamp;

  power->lsq_rs_power_nobit = power->lsq_rs_decoder + 
    power->lsq_rs_wordline + power->lsq_rs_senseamp;
   
  power->lsq_wakeup_power = power->lsq_wakeup_tagdrive + power->lsq_wakeup_tagmatch;

  power->lsq_power = power->lsq_wakeup_power + power->lsq_rs_power;

  power->regfile_power = power->regfile_decoder + 
    power->regfile_wordline + power->regfile_bitline + 
    power->regfile_senseamp;

  power->regfile_power_nobit = power->regfile_decoder + 
    power->regfile_wordline + power->regfile_senseamp;

  dump_power_stats();

}

//*********************************************************************************
//* The following was pulled from Cacti, io.c
//********************************************************************************
void power_t::output_time_components(int A, time_result_type * result)
{
   m_pseq->out_info(" decode_data (ns): %g\n",result->decoder_delay_data/1e-9);
   m_pseq->out_info(" wordline_data (ns): %g\n",result->wordline_delay_data/1e-9);
   m_pseq->out_info(" bitline_data (ns): %g\n",result->bitline_delay_data/1e-9);
   m_pseq->out_info(" sense_amp_data (ns): %g\n",result->sense_amp_delay_data/1e-9);
   m_pseq->out_info(" decode_tag (ns): %g\n",result->decoder_delay_tag/1e-9);
   m_pseq->out_info(" wordline_tag (ns): %g\n",result->wordline_delay_tag/1e-9);
   m_pseq->out_info(" bitline_tag (ns): %g\n",result->bitline_delay_tag/1e-9);
   m_pseq->out_info(" sense_amp_tag (ns): %g\n",result->sense_amp_delay_tag/1e-9);
   m_pseq->out_info(" compare (ns): %g\n",result->compare_part_delay/1e-9);
   if (A == 1)
      m_pseq->out_info(" valid signal driver (ns): %g\n",result->drive_valid_delay/1e-9);
   else {
      m_pseq->out_info(" mux driver (ns): %g\n",result->drive_mux_delay/1e-9);
      m_pseq->out_info(" sel inverter (ns): %g\n",result->selb_delay/1e-9);
   }
   m_pseq->out_info(" data output driver (ns): %g\n",result->data_output_delay/1e-9);
   m_pseq->out_info(" total data path (with output driver) (ns): %g\n",result->decoder_delay_data/1e-9+result->wordline_delay_data/1e-9+result->bitline_delay_data/1e-9+result->sense_amp_delay_data/1e-9);
   if (A==1)
      m_pseq->out_info(" total tag path is dm (ns): %g\n", result->decoder_delay_tag/1e-9+result->wordline_delay_tag/1e-9+result->bitline_delay_tag/1e-9+result->sense_amp_delay_tag/1e-9+result->compare_part_delay/1e-9+result->drive_valid_delay/1e-9);
   else 
      m_pseq->out_info(" total tag path is set assoc (ns): %g\n", result->decoder_delay_tag/1e-9+result->wordline_delay_tag/1e-9+result->bitline_delay_tag/1e-9+result->sense_amp_delay_tag/1e-9+result->compare_part_delay/1e-9+result->drive_mux_delay/1e-9+result->selb_delay/1e-9);
   m_pseq->out_info(" precharge time (ns): %g\n",result->precharge_delay/1e-9);
}

//*************************************************************************************************
void power_t::output_data(time_result_type * result, time_parameter_type * parameters)
{
   double datapath,tagpath;

   datapath = result->decoder_delay_data+result->wordline_delay_data+result->bitline_delay_data+result->sense_amp_delay_data+result->data_output_delay;
   if (parameters->associativity == 1) {
         tagpath = result->decoder_delay_tag+result->wordline_delay_tag+result->bitline_delay_tag+result->sense_amp_delay_tag+result->compare_part_delay+result->drive_valid_delay;
   } else {
         tagpath = result->decoder_delay_tag+result->wordline_delay_tag+result->bitline_delay_tag+result->sense_amp_delay_tag+result->compare_part_delay+result->drive_mux_delay+result->selb_delay;
   }

#  if OUTPUTTYPE == LONG
      m_pseq->out_info("\nCache Parameters:\n");
      m_pseq->out_info("  Size in bytes: %d\n",parameters->cache_size);
      m_pseq->out_info("  Number of sets: %d\n",parameters->number_of_sets);
      m_pseq->out_info("  Associativity: %d\n",parameters->associativity);
      m_pseq->out_info("  Block Size (bytes): %d\n",parameters->block_size);

      m_pseq->out_info("\nAccess Time: %g\n",result->access_time);
      m_pseq->out_info("Cycle Time:  %g\n",result->cycle_time);
      m_pseq->out_info("\nBest Ndwl (L1): %d\n",result->best_Ndwl);
      m_pseq->out_info("Best Ndbl (L1): %d\n",result->best_Ndbl);
      m_pseq->out_info("Best Nspd (L1): %d\n",result->best_Nspd);
      m_pseq->out_info("Best Ntwl (L1): %d\n",result->best_Ntwl);
      m_pseq->out_info("Best Ntbl (L1): %d\n",result->best_Ntbl);
      m_pseq->out_info("Best Ntspd (L1): %d\n",result->best_Ntspd);
      m_pseq->out_info("\nTime Components:\n");
      m_pseq->out_info(" data side (with Output driver) (ns): %g\n",datapath/1e-9);
      m_pseq->out_info(" tag side (ns): %g\n",tagpath/1e-9);
      output_time_components(parameters->associativity,result);

#  else
      m_pseq->out_info("%d %d %d  %d %d %d %d %d %d  %e %e %e %e  %e %e %e %e  %e %e %e %e  %e %e %e %e  %e %e\n",
               parameters->cache_size,
               parameters->block_size,
               parameters->associativity,
               result->best_Ndwl,
               result->best_Ndbl,
               result->best_Nspd,
               result->best_Ntwl,
               result->best_Ntbl,
               result->best_Ntspd,
               result->access_time,
               result->cycle_time,
               datapath,
               tagpath,
               result->decoder_delay_data,
               result->wordline_delay_data,
               result->bitline_delay_data,
               result->sense_amp_delay_data,
               result->decoder_delay_tag,
               result->wordline_delay_tag,
               result->bitline_delay_tag,
               result->sense_amp_delay_tag,
               result->compare_part_delay,
               result->drive_mux_delay,
               result->selb_delay,
               result->drive_valid_delay,
               result->data_output_delay,
               result->precharge_delay);



# endif

}

