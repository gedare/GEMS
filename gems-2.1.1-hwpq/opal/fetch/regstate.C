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
#include "hfa.h"
#include "hfacore.h"
#include "Map.h"
#include "Vector.h"
#include "regstate.h"

//*********************************************************
regstate_predictor_t::regstate_predictor_t(pseq_t * seq){
  m_pseq = seq;
  m_pstate_predictor = new Map<la_t, regstate_entry_t >;
  m_tl_predictor = new Map<la_t, regstate_entry_t >;
  m_cwp_predictor = new Map<la_t, regstate_entry_t >;
}

//********************************************************
regstate_predictor_t::~regstate_predictor_t(){
  if(m_pstate_predictor){
    delete m_pstate_predictor;
    m_pstate_predictor = NULL;
  }
  if(m_tl_predictor){
    delete m_tl_predictor;
    m_tl_predictor = NULL;
  }
  if(m_cwp_predictor){
    delete m_cwp_predictor;
    m_cwp_predictor = NULL;
  }
}

//*******************************************************
// updates the StoreSet predictor table
void 
regstate_predictor_t::update(const la_t & vpc, const byte_t & type, const ireg_t & actual_value, const ireg_t & pred_value){
#if 1
  if( type == CONTROL_PSTATE){
    if(m_pstate_predictor->exist( vpc ) ){
      regstate_entry_t & entry = m_pstate_predictor->lookup(vpc);
      entry.predicted_value = actual_value;
    }
    else{
      regstate_entry_t new_entry;
      new_entry.predicted_value = actual_value;
      m_pstate_predictor->insert( vpc, new_entry );
    }
  }
  else if(type == CONTROL_TL){
     if(m_tl_predictor->exist( vpc ) ){
      regstate_entry_t & entry = m_tl_predictor->lookup(vpc);
      entry.predicted_value = actual_value;
    }
    else{
      regstate_entry_t new_entry;
      new_entry.predicted_value = actual_value;
      m_tl_predictor->insert( vpc, new_entry );
    }
  } 
  else if( type == CONTROL_CWP){
    bool value = false;
    // a predicted CWP value of 0 is a special case
    if(pred_value == 0 && (actual_value == NWINDOWS-1)){
      value = false;
    }
    else{
      int difference = pred_value - actual_value;
      ASSERT(difference != 0);   //this should be the default prediction
      if(difference < 0){
        //means we should have incremented
        value = true;
      }
      else{
        value = false;
      }
    }

    if(m_cwp_predictor->exist(vpc)){
      regstate_entry_t & entry = m_cwp_predictor->lookup(vpc);
      entry.cwp_increment = value;
    }
    else{
      regstate_entry_t new_entry;
      new_entry.cwp_increment = value;
      m_cwp_predictor->insert( vpc, new_entry);
    }
  }
  else{
    //we should not modify any other types
    ASSERT(0);
  }

#endif
}

//*****************************************************
ireg_t
regstate_predictor_t::Predict(const la_t & vpc, abstract_pc_t * a, const byte_t & type){
  if( type == CONTROL_PSTATE){
    if(m_pstate_predictor->exist(vpc)){
      regstate_entry_t & entry = m_pstate_predictor->lookup(vpc);
      return entry.predicted_value;
    }
    else{
      //return the current pstate
      return a->pstate;
    }
  }
  else if( type == CONTROL_TL){
    if(m_tl_predictor->exist(vpc)){
      regstate_entry_t & entry = m_tl_predictor->lookup(vpc);
      return entry.predicted_value;
    }
    else{
      //return the current tl
      return a->tl;
    }
  }
  else if( type == CONTROL_CWP ){
    if( m_cwp_predictor->exist(vpc)){
      regstate_entry_t & entry = m_cwp_predictor->lookup(vpc);
      if(entry.cwp_increment){
        return  (a->cwp + 1) & (NWINDOWS - 1);
      }
      else{
         return  (a->cwp - 1) & (NWINDOWS - 1);
      }
    }
    else{
      //return the current cwp
      return a->cwp;
    }
  }
}

//*******************************************************
void 
regstate_predictor_t::printConfig(out_intf_t * out)
{
  Vector< la_t > pstate_keys = m_pstate_predictor->keys();
  Vector< la_t > tl_keys = m_tl_predictor->keys();
  Vector< la_t > cwp_keys = m_cwp_predictor->keys();
  int pstate_size = pstate_keys.size();
  int tl_size = tl_keys.size();
  int cwp_size = cwp_keys.size();
  out->out_info("\n***Regstate Predictor (for BRANCH_PRIV)\n");
  out->out_info("===============================\n");
  out->out_info("\tPSTATE predictor, total size = %d entries\n", pstate_size);
  for(int i=0; i < pstate_size; ++i){
    regstate_entry_t & entry = m_pstate_predictor->lookup(pstate_keys[i]);
    out->out_info("\t\tVPC[ 0x%llx ] prediction[ 0x%llx ]\n", pstate_keys[i], entry.predicted_value);
  }
  out->out_info("\tTL predictor, total size = %d entries\n", tl_size);
  for(int i=0; i < tl_size; ++i){
    regstate_entry_t & entry = m_tl_predictor->lookup(tl_keys[i]);
    out->out_info("\t\tVPC[ 0x%llx ] prediction[ 0x%llx ]\n", tl_keys[i], entry.predicted_value);
  }
  out->out_info("\tCWP predictor, total size = %d entries\n", cwp_size);
  for(int i=0; i < cwp_size; ++i){
    regstate_entry_t & entry = m_cwp_predictor->lookup(cwp_keys[i]);
    out->out_info("\t\tVPC[ 0x%llx ] prediction[ %d ]\n", cwp_keys[i], entry.cwp_increment);
  }
}

  


