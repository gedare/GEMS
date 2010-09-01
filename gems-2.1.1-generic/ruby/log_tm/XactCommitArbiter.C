/*
    Copyright (C) 1999-2008 by Mark D. Hill and David A. Wood for the
    Wisconsin Multifacet Project.  Contact: gems@cs.wisc.edu
    http://www.cs.wisc.edu/gems/

    --------------------------------------------------------------------

    This file is part of the Ruby Multiprocessor Memory System Simulator, 
    a component of the Multifacet GEMS (General Execution-driven 
    Multiprocessor Simulator) software toolset originally developed at 
    the University of Wisconsin-Madison.

    Ruby was originally developed primarily by Milo Martin and Daniel
    Sorin with contributions from Ross Dickson, Carl Mauer, and Manoj
    Plakal.

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

#include "XactCommitArbiter.h"
#include "RubyConfig.h"
#include "CacheRequestType.h"
#include "interface.h"
#include "System.h"
#include "EventQueue.h"

#include <assert.h>
#include <stdlib.h>

XactCommitArbiter::XactCommitArbiter() {
  m_commitTokenRequestList.clear();
}

XactCommitArbiter::~XactCommitArbiter() {
}

void XactCommitArbiter::requestCommitToken(int proc_no){
  assert(XACT_LAZY_VM);      
  for (int i = 0 ; i < m_commitTokenRequestList.size(); i++)
    assert(m_commitTokenRequestList[i] != proc_no);

  m_commitTokenRequestList.insertAtBottom(proc_no);    

  if (!XACT_EAGER_CD){
    if (getTokenOwner() != proc_no){
      if (XACT_DEBUG && XACT_DEBUG_LEVEL > 0)
        cout << g_eventQueue_ptr->getTime() << " " << proc_no << " [" << proc_no / RubyConfig::numberofSMTThreads() << "," << proc_no % RubyConfig::numberofSMTThreads() << "] STALLING FOR COMMIT TOKEN" << endl;              
      SIMICS_post_disable_processor(proc_no);
    } else {
      if (XACT_DEBUG && XACT_DEBUG_LEVEL > 0)
        cout << g_eventQueue_ptr->getTime() << " " << proc_no << " [" << proc_no / RubyConfig::numberofSMTThreads() << "," << proc_no % RubyConfig::numberofSMTThreads() << "] ACQUIRED COMMIT TOKEN" << endl;              
      if (XACT_COMMIT_TOKEN_LATENCY != 0)
        SIMICS_post_stall_proc(proc_no, XACT_COMMIT_TOKEN_LATENCY);          
    }
  }          
}

void XactCommitArbiter::releaseCommitToken(int proc_no){
  assert(m_commitTokenRequestList.size() > 0);      
  assert(proc_no == m_commitTokenRequestList[0]);
  assert(XACT_LAZY_VM && !XACT_EAGER_CD);
    
  if (XACT_DEBUG && XACT_DEBUG_LEVEL > 0)
    cout << g_eventQueue_ptr->getTime() << " " << proc_no << " [" << proc_no / RubyConfig::numberofSMTThreads() << "," << proc_no % RubyConfig::numberofSMTThreads() << "] RELEASING COMMIT TOKEN" << endl;              
  
  m_commitTokenRequestList.removeFromTop(1);
  if (m_commitTokenRequestList.size() >  0){
    int owner = getTokenOwner();      
    if (XACT_DEBUG && XACT_DEBUG_LEVEL > 0)
      cout << g_eventQueue_ptr->getTime() << " " << owner << " [" << owner / RubyConfig::numberofSMTThreads() << "," << owner % RubyConfig::numberofSMTThreads() << "] ACQUIRED COMMIT TOKEN" << endl;              
    if (!SIMICS_processor_enabled(owner)){
      if (XACT_COMMIT_TOKEN_LATENCY != 0){
        SIMICS_enable_processor(owner);
        SIMICS_stall_proc(owner, XACT_COMMIT_TOKEN_LATENCY);
      } else                  
        SIMICS_enable_processor(owner);
    }    
  }           
}

void XactCommitArbiter::removeCommitTokenRequest(int proc_no){
  bool found = false;
  int  index = -1;
  int  curr_size = m_commitTokenRequestList.size();
  
  if ( !XACT_EAGER_CD ){
    if (getTokenOwner() == proc_no){
      releaseCommitToken(proc_no);
      return;
    }
  }            
  
  for(int i = 0; (i < curr_size) && !found; i++){
    if (m_commitTokenRequestList[i] == proc_no){
      found = true;
      index = i;
    }
  }                        
  assert(found);
  for (int i = index; i < (curr_size - 1); i++){
    m_commitTokenRequestList[i] = m_commitTokenRequestList[i + 1];
  }          
  m_commitTokenRequestList.setSize(curr_size - 1);
  if (!SIMICS_processor_enabled(proc_no)){
    SIMICS_enable_processor(proc_no);
  }  
}          

bool XactCommitArbiter::existCommitTokenRequest(int proc_no){
  bool found = false;         
  int  curr_size = m_commitTokenRequestList.size();
  for(int i = 0; (i < curr_size) && !found; i++){
    if (m_commitTokenRequestList[i] == proc_no){
      found = true;
    }
  }  
  return found;
}                        

int XactCommitArbiter::getTokenOwner(){
  assert(XACT_LAZY_VM && !XACT_EAGER_CD);      
  int owner = -1;
  if (m_commitTokenRequestList.size() > 0)
   owner = m_commitTokenRequestList[0];
   
  return owner;
}           

int XactCommitArbiter::getNumTokenRequests(){
 return m_commitTokenRequestList.size();
}          
              
