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
 * FileName:  pstate.C
 * Synopsis:  Implements an in-order core's processors state
 * Author:    cmauer
 * Version:   $Id$
 */

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/
#include "hfa.h"
#include "hfacore.h"
#include "pstate.h"

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

#define   REGISTER_BASE  128


/*------------------------------------------------------------------------*/
/* Variable declarations                                                  */
/*------------------------------------------------------------------------*/

/** an array of boolean values that indicate if ASI's are cacheable or not */
char *pstate_t::m_asi_is_cacheable;
/** an array of ints mapping cacheable ASI to context ids */
char *pstate_t::m_asi_ctxt_id;

/*------------------------------------------------------------------------*/
/* Forward declarations                                                   */
/*------------------------------------------------------------------------*/

static void pstate_breakpoint_handler( conf_object_t *cpu, void *parameter );

/*------------------------------------------------------------------------*/
/* Constructor(s) / destructor                                            */
/*------------------------------------------------------------------------*/

//**************************************************************************
pstate_t::pstate_t( int id ) {
#ifdef DEBUG_PSTATE
  DEBUG_OUT("***pstate_t:pstate_t BEGIN\n");
#endif

  m_id  = id;
  m_cpu = (conf_object_t **) malloc(sizeof(conf_object_t *)*CONFIG_LOGICAL_PER_PHY_PROC);
  for(uint i=0; i <  CONFIG_LOGICAL_PER_PHY_PROC; ++i){
    m_cpu[i] =  SIM_proc_no_2_ptr( id + i);             //get pointers to actual Simics procs
     #ifdef DEBUG_PSTATE
        DEBUG_OUT("pstate_t: contstructor, settting m_cpu, addr[%0x] id[%d]\n",m_cpu[i],id+i);
     #endif
    // ASSERT(m_cpu[i] != NULL);              //this is NOT true for tester
  }

  // get the interface associated with this processor
  m_sparc_intf = NULL;
  m_conf_mmu = NULL;
  if (CONFIG_IN_SIMICS) {
    m_sparc_intf = (sparc_v9_interface_t **) malloc(sizeof(sparc_v9_interface_t *) * CONFIG_LOGICAL_PER_PHY_PROC);
    for(uint i=0; i <  CONFIG_LOGICAL_PER_PHY_PROC; ++i){
      //m_sparc_intf = (sparc_v9_interface_t*) SIM_get_interface( m_cpu, SPARC_V9_INTERFACE );
      m_sparc_intf[i] = (sparc_v9_interface_t*) SIM_get_interface( m_cpu[i], SPARC_V9_INTERFACE );
      // ASSERT(m_sparc_intf[i] != NULL);          //this is NOT true for tester
      #ifdef DEBUG_PSTATE
         DEBUG_OUT("pstate_t constructor, setting m_sparc_intf, addr[%0x] id[%d]\n",m_sparc_intf[i],i+id);
      #endif
    }
    
    m_conf_mmu = (conf_object_t **) malloc(sizeof(conf_object_t*) * CONFIG_LOGICAL_PER_PHY_PROC);

    //each logical proc should get its own MMU handle
    for(uint i=0; i <  CONFIG_LOGICAL_PER_PHY_PROC; ++i){
      char mmuname[200];
      pstate_t::getMMUName( m_id + i, mmuname, 200 );
      m_conf_mmu[i] = SIM_get_object( mmuname );
      if (m_conf_mmu[i] == NULL) {
        ERROR_OUT("pstate_t: error: unable to locate object: %s\n", mmuname);
        SIM_HALT;
      }
    }
  }  // end if(CONFIG_IN_SIMICS)
  hfa_checkerr("pstate: getContext: get mmu object");

  // initialize the core state of the processor to all zeros
  m_state = new core_state_t[CONFIG_LOGICAL_PER_PHY_PROC];
  for(uint i=0; i <  CONFIG_LOGICAL_PER_PHY_PROC; ++i){
     //zero the state of the regs:
    for(uint a=0; a< MAX_GLOBAL_REGS; ++a){
      m_state[i].global_regs[a] = 0;
    }
    for(uint b=0; b < MAX_INT_REGS; ++b){
      m_state[i].int_regs[b] = 0;
    }
    for(uint c=0; c < MAX_CTL_REGS; ++c){
      m_state[i].ctl_regs[c] = 0;
    }
     //zero out the fp regs:
    for(uint d=0; d < MAX_FLOAT_REGS; ++d){
      m_state[i].fp_regs[d] = 0;
    }
  
   memset( &m_state[i], 0, sizeof(core_state_t) );
  }  // end for all logical procs

  m_translation_cache = new AddressMapping *[CONFIG_LOGICAL_PER_PHY_PROC];
  for(uint i=0; i <  CONFIG_LOGICAL_PER_PHY_PROC; ++i){
    m_translation_cache[i] = new AddressMapping();
  }

  // read control register mappings
  m_control_map = (int32 **) malloc(sizeof(int32 *) * CONFIG_LOGICAL_PER_PHY_PROC);
  for(uint k=0; k <  CONFIG_LOGICAL_PER_PHY_PROC; ++k){
    m_control_map[k] = (int32 *) malloc( (CONTROL_NUM_CONTROL_TYPES*2) * sizeof(int32) );
    for (int32 i = 0; i < (CONTROL_NUM_CONTROL_TYPES*2); i++) {
      m_control_map[k][i] = -1;
    }
  }
  

  // determine how many of the control registers we actually map
  m_valid_control_reg = (bool **) malloc(sizeof(bool *) * CONFIG_LOGICAL_PER_PHY_PROC);
  int32   max_control_reg   = MAX_CTL_REGS;

  for(uint k=0; k <  CONFIG_LOGICAL_PER_PHY_PROC; ++k){
    m_valid_control_reg[k] = (bool *) malloc( sizeof(bool) * max_control_reg );
    for (int32 i = 0; i < max_control_reg; i++) {
      m_valid_control_reg[k][i] = false;
    }
  }
  
  if ( CONFIG_IN_SIMICS ) {
    for(uint k=0; k <  CONFIG_LOGICAL_PER_PHY_PROC; ++k){
      attr_value_t regs = SIM_get_all_registers(m_cpu[k]);
      if (regs.kind != Sim_Val_List) SIM_HALT;
      int reg_count = regs.u.list.size;
      SIM_free_attribute(regs);

      // verify our count of physical control registers is greater than simics's
      if ((uint64) reg_count > MAX_CTL_REGS) {
        ERROR_OUT("pstate_t: verify simics warning: Opal has %d control regs (MAX_CTL_REGS).\n", MAX_CTL_REGS);
        ERROR_OUT("pstate_t: while simics has %d control regs.\n", reg_count);
        SIM_HALT;
      }
    
      // read the simics control register map
      for ( int i = 0; i < CONTROL_NUM_CONTROL_PHYS; i ++ ) {
        if ( pstate_t::control_reg_menomic[i] == NULL ) {
          ERROR_OUT( "pstate_t: error: control register %d has improper menomic.\n", i );
        }
        int simmap = SIM_get_register_number( m_cpu[k],pstate_t::control_reg_menomic[i]);
        if ( simmap < 0 || simmap > max_control_reg ) {
          ERROR_OUT( "pstate_t: error: control register \"%s\" has improper mapping: %d.\n", pstate_t::control_reg_menomic[i], simmap);
        } else {
          m_valid_control_reg[k][simmap] = true;
        }
        hfa_checkerr( "pstate_t: constructor: unable to get control register number.\n");
        m_control_map[k][i] = simmap;
      }
    
      // compare the control registers that we are defining, versus those
      // that are implemented in simics
      for (int32 i = 0; i < max_control_reg; i++) {
        if (m_valid_control_reg[k][i] == false) {
          const char *regname = SIM_get_register_name(m_cpu[k], i);
          if (regname != NULL) {
            // our name for this control register
            const char* ctl_reg_menomic = "(null)";
            for(int32 j = 0; j < CONTROL_NUM_CONTROL_PHYS; j++) {
              if(m_control_map[k][j] == i) {
                ctl_reg_menomic = pstate_t::control_reg_menomic[j];
                break;
              }
            }
            if(strcmp(regname, ctl_reg_menomic) == 0) {
#ifdef VERIFY_SIMICS
              if (m_id + k == 0) { // only warn once
                DEBUG_OUT("pstate_t: warning: control register #%d == \"%s\" is not modelled.\n", i, regname );
              }
              SIM_HALT;
            } else {
              if (m_id + k == 0) { // only warn once
                DEBUG_OUT("pstate_t: warning: control register #%d == \"%s\" has simics name \"%s\".\n", i, ctl_reg_menomic, regname );
              }
            }
#endif
          } else {
            // else filter out non-existant registers
            SIM_clear_exception();
          }
        }
      }
    
#ifdef VERIFY_SIMICS
    // validate the simics has the same number of register windows as us
      attr_value_t attr = SIM_get_attribute(m_cpu[k], "num-windows");
      if ( attr.kind != Sim_Val_Integer || attr.u.integer != NWINDOWS ) {
        ERROR_OUT("error: pstate_t: number of register windows differ!");
        if (attr.kind == Sim_Val_Integer)
          ERROR_OUT("Simics num-windows == %lld. N-WINDOWS %d\n",
                    attr.u.integer, NWINDOWS );
        SYSTEM_EXIT;
      }
      hfa_checkerr("pstate: get attribute: num-windows");
#endif
      }  // end of for loop over all logical processors 

    } else { // not in simics
      for(uint k=0; k <  CONFIG_LOGICAL_PER_PHY_PROC; ++k){
        for (int32 i = 0; i < max_control_reg; i++) {
          m_valid_control_reg[k][i] = true;
        }
      }
    }

 
  // allocate / initialize the ASI mapping
  m_asi_is_cacheable = (char *) malloc( sizeof(char) * MAX_NUM_ASI );
  // ERROR_OUT("***About to allocate ctxt id array\n");
  m_asi_ctxt_id = (char *) malloc( sizeof(char) * MAX_NUM_ASI );
  //ERROR_OUT("***after allocating ctxt id array\n");
  for ( uint16 i = 0; i < MAX_NUM_ASI; i++ ) {
    m_asi_is_cacheable[i] = 0;
    m_asi_ctxt_id[i] = 0;
  }
  // indicate that certain ASIs _are_ cacheable
  m_asi_is_cacheable[ASI_PRIMARY] = true;
  m_asi_is_cacheable[ASI_NUCLEUS] = true;
  m_asi_is_cacheable[ASI_SECONDARY] = true;
  m_asi_is_cacheable[ASI_AS_IF_USER_PRIMARY] = true;
  m_asi_is_cacheable[ASI_AS_IF_USER_SECONDARY] = true;
  m_asi_is_cacheable[ASI_NUCLEUS_QUAD_LDD] = true;
  m_asi_is_cacheable[ASI_NUCLEUS_QUAD_LDD_L] = true;
  m_asi_is_cacheable[ASI_PHYS_USE_EC] = true;
  m_asi_is_cacheable[ASI_BLK_AIUP] = true;
  m_asi_is_cacheable[ASI_BLK_AIUS] = true;
  m_asi_is_cacheable[ASI_BLK_AIUPL] = true;
  m_asi_is_cacheable[ASI_BLK_AIUSL] = true;
  m_asi_is_cacheable[ASI_BLK_P] = true;
  m_asi_is_cacheable[ASI_BLK_S] = true;
  m_asi_is_cacheable[ASI_BLK_COMMIT_P] = true;
  m_asi_is_cacheable[ASI_BLK_COMMIT_S] = true;

  m_asi_is_cacheable[ASI_FL8_P] = true;
  m_asi_is_cacheable[ASI_FL8_S] = true;
  m_asi_is_cacheable[ASI_FL16_P] = true;
  m_asi_is_cacheable[ASI_FL16_S] = true;
  m_asi_is_cacheable[ASI_FL8_PL] = true;
  m_asi_is_cacheable[ASI_FL8_SL] = true;
  m_asi_is_cacheable[ASI_FL16_PL] = true;
  m_asi_is_cacheable[ASI_FL16_SL] = true;

  // Assign corresponding context ids to the cacheable ASIs...
  m_asi_ctxt_id[ASI_PRIMARY] = 0;
  m_asi_ctxt_id[ASI_NUCLEUS] = 2;
  m_asi_ctxt_id[ASI_SECONDARY] = 1;
  m_asi_ctxt_id[ASI_AS_IF_USER_PRIMARY] = 0;
  m_asi_ctxt_id[ASI_AS_IF_USER_SECONDARY] = 1;
  m_asi_ctxt_id[ASI_NUCLEUS_QUAD_LDD] = 2;
  m_asi_ctxt_id[ASI_NUCLEUS_QUAD_LDD_L] = 2;
  m_asi_ctxt_id[ASI_PHYS_USE_EC] = 2;
  m_asi_ctxt_id[ASI_BLK_AIUP] = 0;
  m_asi_ctxt_id[ASI_BLK_AIUS] = 1;
  m_asi_ctxt_id[ASI_BLK_AIUPL] = 0;
  m_asi_ctxt_id[ASI_BLK_AIUSL] = 1;
  m_asi_ctxt_id[ASI_BLK_P] = 0;
  m_asi_ctxt_id[ASI_BLK_S] = 1;
  m_asi_ctxt_id[ASI_BLK_COMMIT_P] = 0;
  m_asi_ctxt_id[ASI_BLK_COMMIT_S] = 1;

  m_asi_ctxt_id[ASI_FL8_P] = 0;
  m_asi_ctxt_id[ASI_FL8_S] = 1;
  m_asi_ctxt_id[ASI_FL16_P] = 0;
  m_asi_ctxt_id[ASI_FL16_S] = 1;
  m_asi_ctxt_id[ASI_FL8_PL] = 0;
  m_asi_ctxt_id[ASI_FL8_SL] = 1;
  m_asi_ctxt_id[ASI_FL16_PL] = 0;
  m_asi_ctxt_id[ASI_FL16_SL] = 1;

  //  ASSERT(m_sparc_intf != NULL);
#ifdef DEBUG_PSTATE
  DEBUG_OUT("***pstate:pstate_t() END\n");
#endif
}

//**************************************************************************
pstate_t::~pstate_t() {
  #ifdef DEBUG_PSTATE
  DEBUG_OUT("***pstate_t:~pstate_t BEGIN\n");
  #endif

  if(m_asi_is_cacheable){
    free(m_asi_is_cacheable);
    m_asi_is_cacheable = NULL;
  }
  if(m_asi_ctxt_id){
    free(m_asi_ctxt_id);
    m_asi_ctxt_id = NULL;
  }
 for(uint k=0; k <  CONFIG_LOGICAL_PER_PHY_PROC; ++k){  
   if(m_translation_cache[k])
     delete m_translation_cache[k];
   if(m_control_map[k])
     free(m_control_map[k]);
   if(m_valid_control_reg[k])
     free(m_valid_control_reg[k]);
 }
  if (m_translation_cache)
    delete [] m_translation_cache;
  if (m_control_map)
    free(m_control_map);

    if(m_valid_control_reg)
     free(m_valid_control_reg);
    if(m_conf_mmu)
     free(m_conf_mmu);
  if(m_sparc_intf)
     free(m_sparc_intf);
   if(m_cpu)
    free(m_cpu);
  
   #ifdef DEBUG_PSTATE
  DEBUG_OUT("***pstate:~pstate_t() END\n");
  #endif
}

/*------------------------------------------------------------------------*/
/* Public methods                                                         */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Private methods                                                        */
/*------------------------------------------------------------------------*/

//**************************************************************************
void pstate_t::checkpointState( core_state_t *a_state)
{
  #ifdef DEBUG_PSTATE
    DEBUG_OUT("***pstate:checkpointState BEGIN\n");
 #endif

  // Note: this function should be safe to call multiple times on same SMT chip
  ASSERT(CONFIG_LOGICAL_PER_PHY_PROC > 0);
  ASSERT(a_state != NULL);
  
  // copies the current processors state

  // hfa_checkerr("pstate updatestate: begin\n");
  // zero the state
     //memset( (void*) a_state, 0, sizeof( core_state_t ) );

  //
  // copy the global  registers using the read_register api.
  //
  // There are 4 sets of globals: normal, alt, mmu, and interrupt
  // do this for all logical procs
 
 for(uint k=0; k <  CONFIG_LOGICAL_PER_PHY_PROC; ++k){
    for ( uint32 gset = REG_GLOBAL_NORM; gset <= REG_GLOBAL_INT; gset ++ ) {
      // registers 0...7 are global
      for ( uint32 i = 0; i < 8; i ++ ) {
          #ifdef DEBUG_PSTATE
                DEBUG_OUT("\t setting global regs iter[%d] gset[%d] reg[%d] m_cpu[%0x] m_sparc_intf[%0x]\n",k,gset,i, m_cpu[k], m_sparc_intf[k]);
           #endif
        a_state[k].global_regs[8*gset + i] = m_sparc_intf[k]->read_global_register(
          m_cpu[k], gset, i );
      }
    }
    // hfa_checkerr("pstate: end global\n");

    //
    // copy the integer registers using the read_register api.
    //
    for ( int cwp = 0; cwp < NWINDOWS; cwp++ ) {
      // just update the IN and LOCAL registers for ALL windows
      for (uint32 reg = 31; reg >= 16; reg --) {
           #ifdef DEBUG_PSTATE
             DEBUG_OUT("\t setting int regs iter[%d] cwp[%d] reg[%d]\n",k,cwp,reg);
            #endif
        a_state[k].int_regs[pstateWindowMap(cwp, reg)] =
          m_sparc_intf[k]->read_window_register( m_cpu[k], cwp, reg );
      }
    }
    hfa_checkerr("pstate: end iregister\n");


    //
    // copy the floating point registers using the register api
    //
    for ( uint32 reg = 0; reg < MAX_FLOAT_REGS; reg ++ ) {
      // simics registers are numbered 0 .. 62
       #ifdef DEBUG_PSTATE
         DEBUG_OUT("\tsetting fp regs reg[%d]\n",reg);
        #endif
      a_state[k].fp_regs[reg] = m_sparc_intf[k]->read_fp_register_x( m_cpu[k], (reg*2) );
#if 0
    // CM had a bug here earlier using the wrong register numbers...
    char str[200];
    sprintf(str, "fp register %d", reg);
    hfa_checkerr(str);
#endif
  }
  hfa_checkerr("all fp registers");
 }  //end for loop over all logical procs

  //
  // copy the control registers using register api
  //
 for(uint k=0; k < CONFIG_LOGICAL_PER_PHY_PROC; ++k ){
    for (uint32 i = 0; i < MAX_CTL_REGS; i++) {
      if (m_valid_control_reg[k][i]) {
        // only those valid ones (i.e. Those that Opal really modelled)
        // copy all of EVERY logical procs' control registers
              #ifdef DEBUG_PSTATE
                     DEBUG_OUT("\tsetting ctrl regs iter[%d] reg[%d]\n",k,i);
                #endif
                     a_state[k].ctl_regs[i] = SIM_read_register( m_cpu[k], i );
                     //sprintf(str, "control update state: %d.", i );
                     // hfa_checkerr(str);
      }
    }
  }

#ifdef DEBUG_PSTATE
  DEBUG_OUT("***pstate:checkpointState END\n");
#endif
}

//**************************************************************************
void pstate_t::recoverState( core_state_t *a_state )
{

  #ifdef DEBUG_PSTATE
  DEBUG_OUT("***pstate:recoverState BEGIN\n");
#endif

  ASSERT(CONFIG_LOGICAL_PER_PHY_PROC > 0);

  // recovers the current processors state

  //
  // copy the global registers using the read_register api.
  //
  // There are 4 sets of globals: normal, alt, mmu, and interrupt
  // do this for all logical procs:
for(uint k=0; k < CONFIG_LOGICAL_PER_PHY_PROC; ++k ){
    for ( uint32 gset = REG_GLOBAL_NORM; gset <= REG_GLOBAL_INT; gset ++ ) {
      // registers 0...7 are global
      for ( uint32 i = 0; i < 8; i ++ ) {
         m_sparc_intf[k]->write_global_register( m_cpu[k], gset, i,
                                                 a_state[k].global_regs[8*gset + i] );
      }
    }
    hfa_checkerr("pstate_t: recoverState: end global\n");

    //
    // copy the integer registers using the read_register api.
    //
    for ( int cwp = 0; cwp < NWINDOWS; cwp++ ) {
      // just update the IN and LOCAL registers for ALL windows
      for (uint32 reg = 31; reg >= 16; reg --) {
        m_sparc_intf[k]->write_window_register( m_cpu[k], cwp, reg,
                                                a_state[k].int_regs[pstateWindowMap(cwp, reg)] );
      }
    }
    hfa_checkerr("pstate_t: recoverState: end iregister\n");


  //
  // copy the floating point registers using the register api
  //
  for ( uint32 reg = 0; reg < MAX_FLOAT_REGS; reg ++ ) {
    // simics registers are numbered 0 .. 62
      m_sparc_intf[k]->write_fp_register_x( m_cpu[k], (reg*2), 
                                            a_state[k].fp_regs[reg] );
    }
 
  hfa_checkerr("pstate_t: recoverState: end fp registers");

  //
  // copy the control registers using register api
  //
  for (uint32 i = 0; i < MAX_CTL_REGS; i++) {
      if (m_valid_control_reg[k][i]) {
        SIM_write_register( m_cpu[k], i, a_state[k].ctl_regs[i] );
        //sprintf(str,` "write checkpoint control state: %d.", i );
        // hfa_checkerr(str);
      }
  }
  hfa_checkerr("pstate_t: recoverState: all state updated");

}  //end for loop over all logical procs
  #ifdef DEBUG_PSTATE
  DEBUG_OUT("***pstate:recoverState END\n");
#endif
}

//**************************************************************************
void pstate_t::checkpointState( )
{
  // Note: This function should be called only ONCE per SMT chip, as it loops through all the 
  //          logical processors
  //checkpointState( &m_state );
  checkpointState(m_state);
}

//**************************************************************************
void pstate_t::recoverState( )
{
  // Note: This function should be called only ONCE per SMT chip, as it loops through all the 
  //          logical processors
  recoverState(m_state);
  //recoverState( &m_state );
}

/*------------------------------------------------------------------------*/
/* Accessor(s) / mutator(s)                                               */
/*------------------------------------------------------------------------*/

/// read an integer register relative to the global set
//**************************************************************************
ireg_t  pstate_t::intIntGlobal( unsigned int reg, int global_set, uint32 proc )
{
#ifdef DEBUG_PSTATE
  DEBUG_OUT("***pstate_t:intIntGlobal BEGIN\n");
#endif
  ASSERT(proc < CONFIG_LOGICAL_PER_PHY_PROC);

  ireg_t value;

  if (reg > 8) {
    ERROR_OUT("pstate_t: error: invalid int global register %d\n", reg);
    return (0);
  }

  if ( reg == 0 ) {
    // global register zero always reads as zero
    value = 0;
  } else {
    //int real_reg = pstateGlobalMap( global_set, reg );
    value = m_state[proc].global_regs[ global_set*8 + reg ];  
  }

#ifdef DEBUG_PSTATE
  DEBUG_OUT("***pstate:intIntGlobal END\n");
#endif

  return (value);
}

/// read an integer register relative to a cwp
//**************************************************************************
ireg_t  pstate_t::intIntWp( unsigned int reg, int cwp, uint32 proc )
{
#ifdef DEBUG_PSTATE
  DEBUG_OUT("***pstate_t:intIntWp BEGIN\n");
#endif

  ASSERT(proc < CONFIG_LOGICAL_PER_PHY_PROC);

  ireg_t value;

  if (reg < 8 || reg > 32) {
    ERROR_OUT("pstate_t: error: invalid int windowed register %d\n", reg);
    return (0);
  }
  
  /* offset into the register window */
  int real_reg = pstateWindowMap( cwp, reg );

  //DEBUG_OUT("reg %d (cwp 0x%0llx) ==  %d\n", reg, cwp, real_reg);
  value = m_state[proc].int_regs[real_reg];

#ifdef DEBUG_PSTATE
  DEBUG_OUT("***pstate:intIntWp END\n");
#endif

  return (value);
}

//**************************************************************************
freg_t pstate_t::intDouble( unsigned int reg, uint32 proc )
{
  ASSERT(proc < CONFIG_LOGICAL_PER_PHY_PROC);
  #ifdef DEBUG_PSTATE
  DEBUG_OUT("***pstate_t:intDouble BEGIN\n");
#endif

  if (reg > (MAX_FLOAT_REGS)) {
    ERROR_OUT("pstate_t: error: invalid floating point register %d\n", reg);
    return 0;
  }
  #ifdef DEBUG_PSTATE
  DEBUG_OUT("***pstate:intDouble END\n");
#endif

  return (m_state[proc].fp_regs[reg]);
}

//**************************************************************************
ireg_t pstate_t::intControl( unsigned int reg, uint32 proc )
{
  #ifdef DEBUG_PSTATE
     DEBUG_OUT("***pstate_t:intControl BEGIN\n");
  #endif

  ASSERT(proc < CONFIG_LOGICAL_PER_PHY_PROC);

  if (reg > MAX_CTL_REGS) {
    ERROR_OUT("pstate_t: intcontrol: invalid ctl register %d\n", reg);
    return (0);
  }

  if (!m_valid_control_reg[proc][reg]) {
    ERROR_OUT("pstate_t: intControl: invalid control register: %d\n",
              reg);
    return (0);
  }
  ireg_t local_res = m_state[proc].ctl_regs[reg];

  #ifdef DEBUG_PSTATE
     DEBUG_OUT("***pstate:intControl END\n");
 #endif

  return (local_res);
}

//**************************************************************************
bool pstate_t::readMemory( ireg_t log_addr, int size, bool sign_extend,
                           ireg_t *result_reg, uint32 proc )
{
  #ifdef DEBUG_PSTATE
  DEBUG_OUT("***pstate_t:readMemory BEGIN\n");
#endif

  // SIM_logical_to_physical assumes word read ...
#if 0
  if (size > 4)
    ERROR_OUT("warning: read memory only works for size <= 4\n");
#endif

  //in order to enable SMT, we need each logical processor to read from its own m_cpu[] pointer, so that Simics can execute correctly
  //          Additionally, for correctness we assume all logical procs have the same global, int, fp, and physical memory values, so we are free to read from any logical proc
  ASSERT(proc < CONFIG_LOGICAL_PER_PHY_PROC);

  pa_t phys_pc = SIM_logical_to_physical( m_cpu[proc], Sim_DI_Data, log_addr );

  // outside of physical memory -- this is an error code
  if (phys_pc == (pa_t) -1) {
    *result_reg = 0x0;
    return (false);
  }

  // strip off less than word read bits && tack on the end
  int  lower_bits = log_addr & 0x3;
  phys_pc        += lower_bits;
  ireg_t reg      = SIM_read_phys_memory( m_cpu[proc], phys_pc, size );
  /*
  ireg_t reg_word  = SIM_read_phys_memory( phys_pc, 4 );
  DEBUG_OUT( "reading memory v:0x%0llx p:0x%0llx -> 0x%0llx 0x%0llx %d\n",
          log_addr, phys_pc, reg_word, reg, size );
  */

  if (sign_extend) {
    //DEBUG_OUT(" extending %d -- %d\n", size, (size * 8) - 1);
    reg = sign_ext64( reg, ((size * 8) - 1) );
  }
  
  *result_reg = reg;

  #ifdef DEBUG_PSTATE
  DEBUG_OUT("***pstate:readMemory END\n");
#endif

  return (true);
}

//**************************************************************************
bool pstate_t::readPhysicalMemory( pa_t phys_addr, int size,
                                   ireg_t *result_reg, uint32 proc )
{
#ifdef DEBUG_PSTATE
  DEBUG_OUT("***pstate_t:readPhysicalMemory BEGIN\n");
#endif

    ASSERT(proc < CONFIG_LOGICAL_PER_PHY_PROC);

  // use phys_mem0.map to determine which physical addresses are in I/O space
  if ( memory_inst_t::isIOAccess( phys_addr ) ) {
    return (false);
  }

  if ( (phys_addr & (size - 1)) != 0 ) {
    ERROR_OUT("error: readPhysicalMemory(): memory access not aligned: 0x%0llx size=%d\n", phys_addr, size );
    return (false);
  }
  
  // if size is greater than 8, split into chunks of 8
  while ( size > 0 ) {
     ireg_t reg = SIM_read_phys_memory( m_cpu[proc], phys_addr, MIN(size, 8) );
    int isexcept = SIM_get_pending_exception();
    if ( isexcept != 0 ) {
      SIM_clear_exception();

      // CM: There never should be exceptions on read physical memory.
      //     ANY that occur are bad signs & should be investigated.
      ERROR_OUT( "warning: pstate_t::readPhysicalMemory: Exception: %s (address: 0x%0llx size: %d)\n",
                 SIM_last_error(), phys_addr, size );
      return (false);
    }
    *result_reg++ = reg;
    size -= 8;
    phys_addr += 8;

  }

 #ifdef DEBUG_PSTATE
  DEBUG_OUT("***pstate:readPhysicalMemory END\n");
#endif
  return (true);
}

//**************************************************************************
bool  pstate_t::writeMemory( ireg_t log_addr, int size, ireg_t value )
{
  // failed to write memory
  return (false);
}

//**************************************************************************
bool pstate_t::translateInstruction( ireg_t log_addr, int size,
                                     pa_t &phys_addr, uint32 proc )
{
  #ifdef DEBUG_PSTATE
    DEBUG_OUT("***pstate_t:translateInstruction BEGIN\n");
  #endif

  ASSERT(proc < CONFIG_LOGICAL_PER_PHY_PROC);  
  

  // SIM_logical_to_physical assumes word read ...
  phys_addr = SIM_logical_to_physical( m_cpu[proc], Sim_DI_Instruction, log_addr );
  /// physical pc == -1 indicates an error
  if (phys_addr == (pa_t) -1) {
    return (false);
  }

  // strip off less than word read bits && tack on the end
  int  lower_bits = log_addr & 0x3;
  phys_addr      += lower_bits;

  #ifdef DEBUG_PSTATE
    DEBUG_OUT("***pstate:translateInstruction END\n");
  #endif

  return (true);
}

//**************************************************************************
bool pstate_t::translate( ireg_t log_addr, int size, pa_t &phys_addr, uint32 proc )
{
#ifdef DEBUG_PSTATE
  DEBUG_OUT("***pstate_t:translate BEGIN\n");
#endif

  ASSERT(proc < CONFIG_LOGICAL_PER_PHY_PROC);  

  // SIM_logical_to_physical assumes word read ...
  phys_addr = SIM_logical_to_physical( m_cpu[proc], Sim_DI_Data, log_addr );
  /// physical pc == -1 indicates an error
  if (phys_addr == (pa_t) -1) {
    return (false);
  }

  // strip off less than word read bits && tack on the end
  int  lower_bits = log_addr & 0x3;
  phys_addr      += lower_bits;
  return (true);
}

//**************************************************************************
la_t pstate_t::dereference( la_t ptr, uint32 size, uint32 proc )
{
#ifdef DEBUG_PSTATE
  DEBUG_OUT("***pstate_t:dereference BEGIN\n");
#endif
  ASSERT(proc < CONFIG_LOGICAL_PER_PHY_PROC);  

  pa_t  physical = (pa_t) -1;
  bool  success = true;

  physical = SIM_logical_to_physical( m_cpu[proc], Sim_DI_Instruction, ptr );
  success = (physical != (pa_t) -1);
  if (!success) {
    // clear translation exception
    SIM_clear_exception();
    physical = SIM_logical_to_physical( m_cpu[proc], Sim_DI_Data, ptr );
    success = (physical != (pa_t) -1);
  }
  if (!success) {
    // clear translation exception
    SIM_clear_exception();
    if ( m_translation_cache[proc]->find( ptr ) == m_translation_cache[proc]->end() ) {
      // DEBUG_OUT("dereference: no translation for: 0x%0llx\n", ptr );
      return (la_t) -1;
    } else {
      physical = (*m_translation_cache[proc])[ptr];
    }
  }

  la_t  value = SIM_read_phys_memory( m_cpu[proc], physical, size );
  if (m_translation_cache[proc]->find( ptr ) == m_translation_cache[proc]->end()) {
    (*m_translation_cache[proc])[ptr] = physical;
  }

  #ifdef DEBUG_PSTATE
  DEBUG_OUT("***pstate:dereference END\n");
#endif

  return value;
}

/*------------------------------------------------------------------------*/
/* Package Interfaces                                                     */
/*------------------------------------------------------------------------*/

//***************************************************************************
int pstate_t::getContext(uint32 proc )
{
  #ifdef DEBUG_PSTATE
  DEBUG_OUT("***pstate_t:getContext BEGIN\n");
#endif

  ASSERT(proc < CONFIG_LOGICAL_PER_PHY_PROC);  

  // Note that for SMT there exists multiple contexts, one for each thread
  attr_value_t   attr_reg = SIM_get_attribute( m_conf_mmu[proc], "ctxt_primary" );
  ireg_t         pctx_reg = 0;
  if (attr_reg.kind == Sim_Val_Integer) {
    pctx_reg = attr_reg.u.integer;
  } else {
    ERROR_OUT("error: unable to read attribute on sfmmu: ctxt_primary\n");
  }

  int      context  = (int) pctx_reg;

#ifdef DEBUG_PSTATE
  DEBUG_OUT("***pstate:getContext END\n");
#endif

  return (context);
}

int pstate_t::getSecondaryContext(uint32 proc )
{
  #ifdef DEBUG_PSTATE
  DEBUG_OUT("***pstate_t:getContext BEGIN\n");
#endif

  ASSERT(proc < CONFIG_LOGICAL_PER_PHY_PROC);  

  // Note that for SMT there exists multiple contexts, one for each thread
  attr_value_t   attr_reg = SIM_get_attribute( m_conf_mmu[proc], "ctxt_secondary" );
  ireg_t         pctx_reg = 0;
  if (attr_reg.kind == Sim_Val_Integer) {
    pctx_reg = attr_reg.u.integer;
  } else {
    ERROR_OUT("error: unable to read attribute on sfmmu: ctxt_secondary\n");
  }

  int      context  = (int) pctx_reg;

#ifdef DEBUG_PSTATE
  DEBUG_OUT("***pstate:getContext END\n");
#endif

  return (context);
}

int pstate_t::getNucleusContext(uint32 proc )
{
  #ifdef DEBUG_PSTATE
  DEBUG_OUT("***pstate_t:getContext BEGIN\n");
#endif

  ASSERT(proc < CONFIG_LOGICAL_PER_PHY_PROC);  

  // Note that for SMT there exists multiple contexts, one for each thread
  attr_value_t   attr_reg = SIM_get_attribute( m_conf_mmu[proc], "ctxt_nucleus" );
  ireg_t         pctx_reg = 0;
  if (attr_reg.kind == Sim_Val_Integer) {
    pctx_reg = attr_reg.u.integer;
  } else {
    ERROR_OUT("error: unable to read attribute on sfmmu: ctxt_nucleus\n");
  }

  int      context  = (int) pctx_reg;

#ifdef DEBUG_PSTATE
  DEBUG_OUT("***pstate:getContext END\n");
#endif

  return (context);
}

//************************************************************************
//   returns the contents of the context register
int pstate_t::readContext( uint16 asi, uint32 proc){
  if(m_asi_ctxt_id[asi] == 0){
    //this is the primary context
    return getContext(proc);
  }
  else if(m_asi_ctxt_id[asi] == 1){
    //this is the secondary context
    return getSecondaryContext(proc);
  }
  else{
    //this is the nucleus context
    return getNucleusContext(proc);
  }
}

//*************************************************************************
// Translates a ASI + VA into an MMU register attribute name
char * pstate_t::getMMUAttributeName( uint16 asi, la_t va){
  switch( asi ){
  case 0x4:
    return "ctxt_nucleus";
  case 0x50:
    switch( va ){
    case 0x0:
      return "itag_target";
    case 0x18:
      return "isfsr";
    case 0x28:
      return "itsb";
    case 0x30:
      return "itag_access";
    case 0x48:
      return "itsb_px";
    case 0x50:
      // Should always return value 0
      return "hw_zero";
    case 0x58:
      return "itsb_nx";
    default:
      return "Unknown";
    }
  case 0x51:
    return "itsbp8k";
  case 0x52:
    return "itsbp64k";
  case 0x58:
    switch( va ){
    case 0x0:
      return "dtag_target";
    case 0x8:
      return "ctxt_primary";
    case 0x10:
      return "ctxt_secondary";
    case 0x18:
      return "dsfsr";
    case 0x20:
      return "dsfar";
    case 0x28:
      return "dtsb";
    case 0x30:
      return "dtag_access";
    case 0x38:
      return "va_watchpoint";
    case 0x40:
      return "pa_watchpoint";
    case 0x48:
      return "dtsb_px";
    case 0x50:
      return "dtsb_sx";
    case 0x58:
      return "dtsb_nx";
    default:
      return "Unknown";
    }
  case 0x59:
    return "dtsbp8k";
  case 0x5a:
    return "dtsbp64k";
  case 0x5b:
    return "dtsbpd";
  //if not in our MMU ASI range, return Unknown
  default:
    return "Unknown";
  }
}

//*************************************************************************
// Used to return the values of MMU registers
bool pstate_t::getMMURegisterValue( uint16 asi, la_t va, uint32 proc, ireg_t & val){
  char * regname = getMMUAttributeName( asi, va );
  val = 0;
  bool error = false;
  if(regname == "Unknown"){
    ERROR_OUT("pstate_t::getMMURegisterValue ERROR unknown ASI VA asi[ 0x%x ] VA[ 0x%llx ]\n", asi, va);
    return true;
  }
  if(regname == "hw_zero"){
    // val is already set to 0
    return false;
  }
   // Note that for SMT there exists multiple contexts, one for each thread
  // Query Simics for register value
  attr_value_t   attr_reg = SIM_get_attribute( m_conf_mmu[proc], regname);
  if (attr_reg.kind == Sim_Val_Integer) {
    val = attr_reg.u.integer;
  } else {
    ERROR_OUT("pstate_t::getMMURegisterValue ERROR cannot read register value!\n");
    error = true;
  }

  return error;
}

//*************************************************************************
//used to translate between ASI, VA pairs and Opal's mapped Control registers
control_reg_t pstate_t::getOpalControlReg( uint16 asi, la_t va){
  switch( asi ){
  case 0x4a:
  // fireplane specific
    switch( va ){
    case 0x0:
      return CONTROL_FIREPLANE_CONFIG;
      break;
    case 0x8:
      return CONTROL_FIREPLANE_ADDRESS;
      break;
    default:
      return CONTROL_NUM_CONTROL_PHYS;
      break;
    }
  case 0x48:
    // interrupt vector dispatch status
    return CONTROL_INTR_DISPATCH_STATUS;
    break;
  case 0x49:
    // interrupt vector receive status
    return CONTROL_INTR_RECEIVE_STATUS;
    break;
  case 0x7f:
    //incoming interrupt vector data
    switch( va ){
    case 0x40:
      return CONTROL_IN_INTR_DATA0;
      break;
    case 0x48:
      return CONTROL_IN_INTR_DATA1;
      break;
    case 0x50:
      return CONTROL_IN_INTR_DATA2;
      break;
    case 0x58:
      return CONTROL_IN_INTR_DATA3;
      break;
    case 0x60:
      return CONTROL_IN_INTR_DATA4;
      break;
    case 0x68:
      return CONTROL_IN_INTR_DATA5;
      break;
    case 0x80:
      return CONTROL_IN_INTR_DATA6;
      break;
    case 0x88:
      return CONTROL_IN_INTR_DATA7;
      break;
    default:
      return  CONTROL_NUM_CONTROL_PHYS;
      break;
    }

    //for all other ASI, VA pairs
  default:
    return  CONTROL_NUM_CONTROL_PHYS;
    break;
  }
}

//*************************************************************************
bool pstate_t::readControlReg( uint16 asi, la_t va, uint32 proc, ireg_t & val ){
  val = 0;
  control_reg_t opal_reg = getOpalControlReg( asi, va );
  if( opal_reg == CONTROL_NUM_CONTROL_PHYS){
    ERROR_OUT("pstate_t::readControlReg(), ERROR, unmapped ASI[ 0x%x ] VA[ 0x%x ]\n", asi, va);
    return true;
  }
  val = getControl( opal_reg, proc );
  return false;
}

//**************************************************************************
void pstate_t::askSimics( uint32 instr, pa_t paddr, la_t addr, uint32 proc )
{

  #ifdef DEBUG_PSTATE
  DEBUG_OUT("***pstate_t:askSimics BEGIN\n");
#endif

  ASSERT(proc < CONFIG_LOGICAL_PER_PHY_PROC);  

  if (instr != 0) {
    // The proposed methodology would be to write a physical address, then disassemble it there
    DEBUG_OUT("askSimics: current not authorized to disassemble instructions\n");
  }
  if (paddr != 0) {
    tuple_int_string_t *result = SIM_disassemble( m_cpu[proc], paddr, /* phy */ 0 );
    if ( result == NULL ) {
      DEBUG_OUT("   pa: 0x%0llx NULL\n", paddr );
    } else {
      DEBUG_OUT("   pa: 0x%0llx %s\n", paddr, result->string );
    }
  }
  if (addr != 0) {
    tuple_int_string_t *result = SIM_disassemble( m_cpu[proc], addr, /* log */ 1 );
    if (result == NULL) {
      DEBUG_OUT("   la: 0x%0llx NULL\n", addr );
    } else {
      DEBUG_OUT("   la: 0x%0llx %s\n", addr, result->string );
    }
  }
  // may (likely) cause a translation exception while trying to do this.
  SIM_clear_exception();

  #ifdef DEBUG_PSTATE
  DEBUG_OUT("***pstate:askSimics END\n");
#endif
}

/** See the related documentation in the '.h' file before you get any idea
 *  about calling this function -- you shouldn't have to !
 */
//**************************************************************************
core_state_t  *pstate_t::getProcessorState( uint32 proc ) {
  return ( &m_state[proc] );
}

//***************************************************************************
void pstate_t::copyProcessorState( pstate_t *correct_state )
{
  DEBUG_OUT("copyProcessorState BEGIN\n");
  for(uint k=0; k < CONFIG_LOGICAL_PER_PHY_PROC; ++k ){

   core_state_t *ps = correct_state->getProcessorState(k);

    //zero the state of the regs:
    for(uint a=0; a< MAX_GLOBAL_REGS; ++a){
      m_state[k].global_regs[a] = ps->global_regs[a];
    }
    for(uint b=0; b < MAX_INT_REGS; ++b){
      m_state[k].int_regs[b] = ps->int_regs[b];
    }
    for(uint c=0; c < MAX_CTL_REGS; ++c){
      m_state[k].ctl_regs[c] = ps->ctl_regs[c];
    }
     //zero out the fp regs:
    for(uint d=0; d < MAX_FLOAT_REGS; ++d){
      m_state[k].fp_regs[d] = ps->fp_regs[d];
    }
 }

 DEBUG_OUT("copyProcessorState END\n");
}

/*------------------------------------------------------------------------*/
/* Static methods                                                         */
/*------------------------------------------------------------------------*/

//***************************************************************************
void pstate_breakpoint_handler( conf_object_t *cpu, void *parameter )
{
  // int id = ((pstate_t *) parameter)->getID();
  // DEBUG_OUT( "bp: %d\n", id );
  SIM_break_simulation( NULL );
}

// new style window mapping: always return zero oriented register
//***************************************************************************
int pstate_t::pstateWindowMap( int cwp, unsigned int reg )
{
  int real_reg;
  if (cwp == 7 && reg < 16) {
    real_reg = REGISTER_BASE + reg;
  } else {
    real_reg = REGISTER_BASE - ((cwp + 1) * 16) + reg;
  }
  return (real_reg);
} 

// copies the name of the mmu for processor number "id" in to mmuname.
//***************************************************************************
void pstate_t::getMMUName( int id, char *mmuname, int len )
{
  conf_object_t *cpu = SIM_proc_no_2_ptr( id );

  // use the current cpu to get the attribute 
  attr_value_t mmu_obj = SIM_get_attribute( cpu, "mmu" );
  if (mmu_obj.kind != Sim_Val_Object) {
    ERROR_OUT("pstate_t: getMMUName: unable to get mmu name. not an object\n");
    SIM_HALT;
  }
  attr_value_t mmu_name = SIM_get_attribute( mmu_obj.u.object, "name" );
  if (mmu_name.kind != Sim_Val_String) {
    ERROR_OUT("pstate_t: getMMUName: unable to get mmu name. not a string\n");
    ERROR_OUT("pstate_t: getMMUName: kind = %d\n", mmu_name.kind );
    SIM_HALT;
  }
  strncpy( mmuname, mmu_name.u.string, len );
}

/*------------------------------------------------------------------------*/
/* Static Member Variables                                                */
/*------------------------------------------------------------------------*/

//**************************************************************************
const char *pstate_t::control_reg_menomic[CONTROL_NUM_CONTROL_TYPES] = {
  /* CONTROL_PC */               "pc",
  /* CONTROL_NPC */              "npc",
  /* CONTROL_Y */                "y",
  /* CONTROL_CCR */              "ccr",
  /* CONTROL_FPRS */             "fprs",
  /* CONTROL_FSR */              "fsr",
  /* CONTROL_ASI */              "asi",
  /* CONTROL_TICK */             "tick",
  /* CONTROL_GSR */              "gsr",
  /* CONTROL_TICK_CMPR */        "tick_cmpr",
  /* CONTROL_DISPATCH_CONTROL */ "dcr",
  /* CONTROL_PSTATE */           "pstate",
  /* CONTROL_TL */               "tl",
  /* CONTROL_PIL */              "pil",
  /* CONTROL_TPC1 */             "tpc1",
  /* CONTROL_TPC2 */             "tpc2",
  /* CONTROL_TPC3 */             "tpc3",
  /* CONTROL_TPC4 */             "tpc4",
  /* CONTROL_TPC5 */             "tpc5",
  /* CONTROL_TNPC1 */            "tnpc1",
  /* CONTROL_TNPC2 */            "tnpc2",
  /* CONTROL_TNPC3 */            "tnpc3",
  /* CONTROL_TNPC4 */            "tnpc4",
  /* CONTROL_TNPC5 */            "tnpc5",
  /* CONTROL_TSTATE1 */          "tstate1",
  /* CONTROL_TSTATE2 */          "tstate2",
  /* CONTROL_TSTATE3 */          "tstate3",
  /* CONTROL_TSTATE4 */          "tstate4",
  /* CONTROL_TSTATE5 */          "tstate5",
  /* CONTROL_TT1 */              "tt1",
  /* CONTROL_TT2 */              "tt2",
  /* CONTROL_TT3 */              "tt3",
  /* CONTROL_TT4 */              "tt4",
  /* CONTROL_TT5 */              "tt5",
  /* CONTROL_TBA */              "tba",
  /* CONTROL_VER */              "ver",
  /* CONTROL_CWP */              "cwp",
  /* CONTROL_CANSAVE */          "cansave",
  /* CONTROL_CANRESTORE */       "canrestore",
  /* CONTROL_OTHERWIN */         "otherwin",
  /* CONTROL_WSTATE */           "wstate",
  /* CONTROL_CLEANWIN */         "cleanwin",
  /* CONTROL_STICK */           "stick",
  /* CONTROL_STICK_CMPR */      "stick_cmpr",
  /* CONTROL_SOFTINT */            "softint",
  /*  CONTROL_FIREPLANE_CONFIG */    "safari_config",
  /* CONTROL_FIREPLANE_ADDRESS */   "safari_address",
  /* CONTROL_INTR_DISPATCH_STATUS */  "intr_dispatch_status",
  /* CONTROL_INTR_RECEIVE_STATUS */   "intr_receive",
  /* CONTROL_IN_INTR_DATA0 */  "in_intr_data0",
  /* CONTROL_IN_INTR_DATA1 */  "in_intr_data1",
  /* CONTROL_IN_INTR_DATA2 */  "in_intr_data2",
  /* CONTROL_IN_INTR_DATA3 */  "in_intr_data3",
  /* CONTROL_IN_INTR_DATA4 */  "in_intr_data4",
  /* CONTROL_IN_INTR_DATA5 */  "in_intr_data5",
  /* CONTROL_IN_INTR_DATA6 */  "in_intr_data6",
  /* CONTROL_IN_INTR_DATA7 */  "in_intr_data7",
  /* CONTROL_NUM_CONTROL_PHYS*/  "physcnt",
  /* CONTROL_UNIMPL */           "unimpl"
};

//**************************************************************************
const char *pstate_t::branch_type_menomic[BRANCH_NUM_BRANCH_TYPES] = {
  "BRANCH_NONE",               // not a branch
  "BRANCH_UNCOND",             // unconditional branch
  "BRANCH_COND",               // conditional branch
  "BRANCH_PCOND",              // predicated conditional branch
  "BRANCH_CALL",               // call
  "BRANCH_RETURN",             // return from call (jmp addr, %g0)
  "BRANCH_INDIRECT",           // indirect call    (jmpl)
  "BRANCH_CWP",                // current window pointer update
  "BRANCH_TRAP_RETURN",        // return from trap
  "BRANCH_TRAP",               // trap ? indirect jump ??? incorrect?
  "BRANCH_PRIV"                // explicit privilege  level change
};

//**************************************************************************
const char *pstate_t::branch_predictor_type_menomic[BRANCHPRED_NUM_BRANCH_TYPES] = {
  "BRANCHPRED_GSHARE",         // gshare predictor
  "BRANCHPRED_AGREE",          // agree predictor
  "BRANCHPRED_YAGS",           // YAGS predictor
  "BRANCHPRED_IGSHARE",        // infinite (ideal) gshare predictor
  "BRANCHPRED_MLPRED",         // multi-level predictor
  "BRANCHPRED_EXTREME"         // not implemented
};

//**************************************************************************
const char *pstate_t::trap_num_menomic( trap_type_t traptype ) {

  switch (traptype) {

  case (0x00):
    return ("NoTrap");
  case (0x01):
    return ("Power_On_Reset");
  case (0x02):
    return ("Watchdog_Reset");
  case (0x03):
    return ("Externally_Initiated_Reset");
  case (0x04):
    return ("Software_Initiated_Reset");
  case (0x05):
    return ("Red_State_Exception");
  case (0x08):
    return ("Instruction_Access_Exception");
  case (0x09):
    return ("Instruction_Access_Mmu_Miss");
  case (0x0a):
    return ("Instruction_Access_Error");
  case (0x10):
    return ("Illegal_Instruction");
  case (0x11):
    return ("Privileged_Opcode");
  case (0x12):
    return ("Unimplemented_Ldd");
  case (0x13):
    return ("Unimplemented_Std");
  case (0x20):
    return ("Fp_Disabled");
  case (0x21):
    return ("Fp_Exception_Ieee_754");
  case (0x22):
    return ("Fp_Exception_Other");
  case (0x23):
    return ("Tag_Overflow");
  case (0x24):
    return ("Clean_Window");
  case (0x28):
    return ("Division_By_Zero");
  case (0x29):
    return ("Internal_Processor_Error");
  case (0x30):
    return ("Data_Access_Exception");
  case (0x31):
    return ("Data_Access_Mmu_Miss");
  case (0x32):
    return ("Data_Access_Error");
  case (0x33):
    return ("Data_Access_Protection");
  case (0x34):
    return ("Mem_Address_Not_Aligned");
  case (0x35):
    return ("Lddf_Mem_Address_Not_Aligned");
  case (0x36):
    return ("Stdf_Mem_Address_Not_Aligned");
  case (0x37):
    return ("Privileged_Action");
  case (0x38):
    return ("Ldqf_Mem_Address_Not_Aligned");
  case (0x39):
    return ("Stqf_Mem_Address_Not_Aligned");
  case (0x40):
    return ("Async_Data_Error");
  case (0x41):
    return ("Interrupt_Level_1");
  case (0x42):
    return ("Interrupt_Level_2");
  case (0x43):
    return ("Interrupt_Level_3");
  case (0x44):
    return ("Interrupt_Level_4");
  case (0x45):
    return ("Interrupt_Level_5");
  case (0x46):
    return ("Interrupt_Level_6");
  case (0x47):
    return ("Interrupt_Level_7");
  case (0x48):
    return ("Interrupt_Level_8");
  case (0x49):
    return ("Interrupt_Level_9");
  case (0x4a):
    return ("Interrupt_Level_10");
  case (0x4b):
    return ("Interrupt_Level_11");
  case (0x4c):
    return ("Interrupt_Level_12");
  case (0x4d):
    return ("Interrupt_Level_13");
  case (0x4e):
    return ("Interrupt_Level_14");
  case (0x4f):
    return ("Interrupt_Level_15");
  case (0x60):
    return ("Interrupt_Vector");
  case (0x61):
    return ("PA_Watchpoint");
  case (0x62):
    return ("VA_Watchpoint");
  case (0x63):
    return ("Corrected_ECC_Error");
  case (0x64):
    return ("Fast_Instruction_Access_MMU_Miss");
  case (0x68):
    return ("Fast_Data_Access_MMU_Miss");
  case (0x6c):
    return ("Fast_Data_Access_Protection");
  case (0x80):
    return ("Spill_0_Normal");
  case (0x84):
    return ("Spill_1_Normal");
  case (0x88):
    return ("Spill_2_Normal");
  case (0x8c):
    return ("Spill_3_Normal");
  case (0x90):
    return ("Spill_4_Normal");
  case (0x94):
    return ("Spill_5_Normal");
  case (0x98):
    return ("Spill_6_Normal");
  case (0x9c):
    return ("Spill_7_Normal");
  case (0xa0):
    return ("Spill_0_Other");
  case (0xa4):
    return ("Spill_1_Other");
  case (0xa8):
    return ("Spill_2_Other");
  case (0xac):
    return ("Spill_3_Other");
  case (0xb0):
    return ("Spill_4_Other");
  case (0xb4):
    return ("Spill_5_Other");
  case (0xb8):
    return ("Spill_6_Other");
  case (0xbc):
    return ("Spill_7_Other");
  case (0xc0):
    return ("Fill_0_Normal");
  case (0xc4):
    return ("Fill_1_Normal");
  case (0xc8):
    return ("Fill_2_Normal");
  case (0xcc):
    return ("Fill_3_Normal");
  case (0xd0):
    return ("Fill_4_Normal");
  case (0xd4):
    return ("Fill_5_Normal");
  case (0xd8):
    return ("Fill_6_Normal");
  case (0xdc):
    return ("Fill_7_Normal");
  case (0xe0):
    return ("Fill_0_Other");
  case (0xe4):
    return ("Fill_1_Other");
  case (0xe8):
    return ("Fill_2_Other");
  case (0xec):
    return ("Fill_3_Other");
  case (0xf0):
    return ("Fill_4_Other");
  case (0xf4):
    return ("Fill_5_Other");
  case (0xf8):
    return ("Fill_6_Other");
  case (0xfc):
    return ("Fill_7_Other");
  default:
    //for software traps
    if( (0x100 <= traptype) && (traptype <= (0x100+0x17f)) ){
      return "Software_Trap";
    }
  }
  return ("Unknown_Trap");
}

//**************************************************************************
const char *pstate_t::async_exception_menomic( exception_t except_type ) {

  switch (except_type) {
  case EXCEPT_NONE:
    return("EXCEPT_NONE");
    break;
  case EXCEPT_MISPREDICT:
    return("EXCEPT_MISPREDICT");
    break;
  case EXCEPT_VALUE_MISPREDICT: 
    return("EXCEPT_VALUE_MISPREDICT");
    break;
  case EXCEPT_MEMORY_REPLAY_TRAP: 
    return("EXCEPT_MEMORY_REPLAY_TRAP");
    break;
  case EXCEPT_NUM_EXCEPT_TYPES:
    return("EXCEPT_NUM_EXCEPT_TYPES");
    break;
  }
  return ("EXCEPT_UNKNOWN");
}

//**************************************************************************
const char *pstate_t::fu_type_menomic( fu_type_t fu_type ) {
  switch (fu_type) {
  case FU_NONE:
    return("FU_NONE");
    break;
  case FU_INTALU:
    return("FU_INTALU");
    break;
  case FU_INTMULT:
    return("FU_INTMULT");
    break;
  case FU_INTDIV:
    return("FU_INTDIV");
    break;
  case FU_BRANCH:
    return("FU_BRANCH");
    break;
  case FU_FLOATADD:
    return("FU_FLOATADD");
    break;
  case FU_FLOATCMP:
    return("FU_FLOATCMP");
    break;
  case FU_FLOATCVT:
    return("FU_FLOATCVT");
    break;
  case FU_FLOATMULT:
    return("FU_FLOATMULT");
    break;
  case FU_FLOATDIV:
    return("FU_FLOATDIV");
    break;
  case FU_FLOATSQRT:
    return("FU_FLOATSQRT");
    break;
  case FU_RDPORT:
    return("FU_RDPORT");
    break;
  case FU_WRPORT:
    return("FU_WRPORT");
    break;
  default:
    return ("UNKNOWN FU_TYPE");
  }
  return ("UNKNOWN FU_TYPE");
}

/*------------------------------------------------------------------------*/
/* Global functions                                                       */
/*------------------------------------------------------------------------*/

//***************************************************************************
void  pstate_t::simcontinue( uint32 numsteps, uint32 proc)
{
  #ifdef DEBUG_PSTATE
  DEBUG_OUT("***pstate_t:simcontinue BEGIN\n");
#endif

   ASSERT(proc < CONFIG_LOGICAL_PER_PHY_PROC);  

  sim_exception_t except_code;

  SIM_enable_processor( m_cpu[proc] );

#if 0
  // new simics 1.0.1
  SIM_processor_break( m_cpu[proc], numsteps );
  SIM_set_quiet(1);
  except_code = SIM_get_pending_exception();
  if (SimExc_No_Exception == except_code) {
    cout << "called get pendingSimExc_no" << endl;
  }
  except_code = SIM_clear_exception();
  pc_step_t rc = SIM_continue( 0 );
  except_code = SIM_clear_exception();
  // DEBUG_OUT( "pstate: stepped processor %d\n", m_id );
#endif

#if 1
  // new simics 1.2.5
  SIM_time_post_cycle( m_cpu[proc], numsteps, Sim_Sync_Processor,
                       pstate_breakpoint_handler, (void *) this );
  SIM_continue( 0 );
  //DEBUG_OUT( "pstate: stepped processor %d\n", m_id );
#endif

  // ignore the message: time breakpoint
  except_code = SIM_clear_exception();
  // SimExc_General is raised when a run-time breakpoint is reached
  if (except_code != SimExc_No_Exception &&
      except_code != SimExc_General) {
    if ( (!strncmp(SIM_last_error(), "Received control-c", 17)) ||
         (!strncmp(SIM_last_error(), "control C", 8)) ) 
    {
      DEBUG_OUT("control-C: ending simulation...\n");
      system_t::inst->breakSimulation();
    }
    DEBUG_OUT( "Exception error message: %s\n",
               SIM_last_error() );
    DEBUG_OUT( "pstate_t::simcontinue: caught exception (#%d).\n",
               (int) except_code );
  }

  SIM_disable_processor( m_cpu[proc] );

  #ifdef DEBUG_PSTATE
  DEBUG_OUT("***pstate:simcontinue END\n");
#endif
}

/**
 * pseudo-static package interface:
 *   pstate_step_callback: callback function after a single step on the cpu.
 */
//**************************************************************************
void pstate_step_callback( conf_object_t *obj, void *mypstate )
{
  // currently unused
  //   pstate_t *ps = (pstate_t *) mypstate;
}

/** compares two processors states, returns true if the same. */
//**************************************************************************
bool ps_silent_compare( core_state_t * ps_1, core_state_t * ps_2 )
{
 #ifdef DEBUG_PSTATE
  DEBUG_OUT("***pstate_t:silent_compare BEGIN\n");
#endif

  bool  are_same = true;
  
  for ( uint32 i=0; i < MAX_GLOBAL_REGS; i++ )
    {
      if ( ps_1->global_regs[i] != ps_2->global_regs[i] ) {
        are_same = false;
      }
    }

  for ( uint32 i=0; i < MAX_INT_REGS; i++ )
    {
      if ( ps_1->int_regs[i] != ps_2->int_regs[i] ) {
        are_same = false;
      }
    }

  for ( uint32 i=0; i < MAX_FLOAT_REGS; i++ )
    { 
      if ( ps_1->fp_regs[i] != ps_2->fp_regs[i] ) {
        are_same = false;
      }
    }

  // compare control registers
  for ( uint32 i=0; i < MAX_CTL_REGS; i++ )
    {
      if ( ps_1->ctl_regs[i] != ps_2->ctl_regs[i] ) {
        are_same = false;
      }
    }

#ifdef DEBUG_PSTATE
  DEBUG_OUT("***pstate:silent_compare END\n");
#endif

  return (are_same);
}

/** compares two processors states, returns true if the same. */
//**************************************************************************
bool ps_compare( core_state_t * ps_1, core_state_t * ps_2 )
{
  #ifdef DEBUG_PSTATE
  DEBUG_OUT("***pstate_t:ps_compare BEGIN\n");
#endif

  bool  are_same = true;
  
  for ( uint32 i=0; i < MAX_GLOBAL_REGS; i++ )
    {
      if ( ps_1->global_regs[i] != ps_2->global_regs[i] ) {
        are_same = false;
        DEBUG_OUT( "greg %d: 0x%0llx 0x%0llx\n", i, 
                   ps_1->global_regs[i], ps_2->global_regs[i] );
      }
    }

  for ( uint32 i=0; i < MAX_INT_REGS; i++ )
    {
      if ( ps_1->int_regs[i] != ps_2->int_regs[i] ) {
        are_same = false;
        DEBUG_OUT( "ireg %d: 0x%0llx 0x%0llx\n", i, 
                   ps_1->int_regs[i], ps_2->int_regs[i] );
      }
    }

  for ( uint32 i=0; i < MAX_FLOAT_REGS; i++ )
    {
      if ( ps_1->fp_regs[i] != ps_2->fp_regs[i] ) {
        are_same = false;
        DEBUG_OUT( "fpreg %d: [0] 0x%0x 0x%0x [1] 0x%0x 0x%0x\n", i,
                   ((uint32 *) &ps_1->fp_regs[i])[0],
                   ((uint32 *) &ps_2->fp_regs[i])[0],
                   ((uint32 *) &ps_1->fp_regs[i])[1],
                   ((uint32 *) &ps_2->fp_regs[i])[1] );
      }
    }

  // compare control registers
  for ( uint32 i=0; i < MAX_CTL_REGS; i++ )
    {
      if ( ps_1->ctl_regs[i] != ps_2->ctl_regs[i] ) {
        are_same = false;
        DEBUG_OUT( "cntl %d: 0x%0llx 0x%0llx\n", i,
                   ps_1->ctl_regs[i], ps_2->ctl_regs[i] );
      }
    }

#ifdef DEBUG_PSTATE
  DEBUG_OUT("***pstate:ps_compare END\n");
#endif

  return (are_same);
}

/** Prints out the entire core processor state structure. */
//**************************************************************************
void  ps_print( core_state_t * ps )
{
  #ifdef DEBUG_PSTATE
    DEBUG_OUT("***pstate_t:ps_print BEGIN\n");
  #endif

  for ( uint32 i=0; i < MAX_GLOBAL_REGS; i++ ) {
    DEBUG_OUT( "greg %d: 0x%016llx\n", i, 
               ps->global_regs[i]);
  }
 
  for ( uint32 i=0; i < MAX_INT_REGS; i++ ) {
    DEBUG_OUT( "ireg %d: 0x%016llx\n", i, 
               ps->int_regs[i]);
  }

  for ( uint32 i=0; i < MAX_FLOAT_REGS; i++ ) {
    DEBUG_OUT( "fpreg %d: %016llx\n", i,
               ps->fp_regs[i]);
  }

  for ( uint32 i=0; i < MAX_CTL_REGS; i++ ) {
    DEBUG_OUT( "ctl %d: %016llx\n", i,
               ps->ctl_regs[i]);
  }

  #ifdef DEBUG_PSTATE
    DEBUG_OUT("***pstate:ps_print END\n");
  #endif

}
