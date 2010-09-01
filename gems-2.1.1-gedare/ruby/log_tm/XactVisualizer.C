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

#include "XactVisualizer.h"
#include "RubyConfig.h"
#include "CacheRequestType.h"
#include "interface.h"
#include "System.h"
#include "EventQueue.h"
#include "Profiler.h"
#include "XactProfiler.h"

#include <assert.h>
#include <stdlib.h>

XactVisualizer::XactVisualizer() {
  int num_processors = RubyConfig::numberOfProcessors() * RubyConfig::numberofSMTThreads();      
  m_xactState.setSize(num_processors);        
  m_xactLastStateChange.setSize(num_processors);        
  
  m_goodXactPeriod.setSize(num_processors);

  for (int i = 0; i < num_processors; i++){
    m_xactState[i]            = NONXACT;        
    m_xactLastStateChange[i]  = 0;
    m_goodXactPeriod[i]       = 0;
  }
  m_systemXactTime           = 0;  
  m_systemXactBeginTime      = 0;
}

XactVisualizer::~XactVisualizer() {
}

void XactVisualizer::moveToXact(int proc_no){
  bool stateChange = false;      
  if (m_xactState[proc_no] != XACT){
    stateChange = true;            
    profileTransactionStateChange(proc_no);

    if (numXact() == 0)
      m_systemXactBeginTime = g_eventQueue_ptr->getTime();        
  }  
          
  m_xactState[proc_no] = XACT;
  if (stateChange){
    g_system_ptr->getProfiler()->printTransactionState(0);
    m_xactLastStateChange[proc_no] = g_eventQueue_ptr->getTime();
  }  
}

void XactVisualizer::moveToCommiting(int proc_no){
  bool stateChange = false;
  if (m_xactState[proc_no] != COMMITING){
    stateChange = true;
    profileTransactionStateChange(proc_no);
  } 
  m_xactState[proc_no] = COMMITING;
  if (stateChange){
    g_system_ptr->getProfiler()->printTransactionState(0);
    m_xactLastStateChange[proc_no] = g_eventQueue_ptr->getTime();
    g_system_ptr->getProfiler()->profileGoodTransCycles(proc_no, m_goodXactPeriod[proc_no]);
    m_goodXactPeriod[proc_no] = 0;
  }
}                           

void XactVisualizer::moveToNonXact(int proc_no){
  bool stateChange = false;      
  if (m_xactState[proc_no] != NONXACT){      
    stateChange = true;            
    profileTransactionStateChange(proc_no);
  }  
  m_xactState[proc_no] = NONXACT;
  if (stateChange){
    g_system_ptr->getProfiler()->printTransactionState(0);
    m_xactLastStateChange[proc_no] = g_eventQueue_ptr->getTime();
  }  
}                    

void XactVisualizer::moveToStalled(int proc_no){
  bool stateChange = false;      
  if (m_xactState[proc_no] != STALLED){      
    stateChange = true;
    profileTransactionStateChange(proc_no);
  }
   
  m_xactState[proc_no] = STALLED;
  
  if (stateChange){
    g_system_ptr->getProfiler()->printTransactionState(0);
    m_xactLastStateChange[proc_no] = g_eventQueue_ptr->getTime();
  }  
}

void XactVisualizer::moveToAborting(int proc_no){
  bool stateChange = false;      
  if (m_xactState[proc_no] != ABORTING){      
    stateChange = true;            
    profileTransactionStateChange(proc_no);
  }
  
  m_xactState[proc_no] = ABORTING;
  
  if (stateChange){
    g_system_ptr->getProfiler()->printTransactionState(0);
    m_xactLastStateChange[proc_no] = g_eventQueue_ptr->getTime();

    m_goodXactPeriod[proc_no] = 0;
  }
}      

void XactVisualizer::moveToBackoff(int proc_no){
  bool stateChange = false;      
  if (m_xactState[proc_no] != BACKOFF){      
    stateChange = true;            
    profileTransactionStateChange(proc_no);
  }
  m_xactState[proc_no] = BACKOFF;
  if (stateChange){
    g_system_ptr->getProfiler()->printTransactionState(0);
    m_xactLastStateChange[proc_no] = g_eventQueue_ptr->getTime();
  }  
}

void XactVisualizer::moveToBarrier(int proc_no){
  bool stateChange = false;      
  if (m_xactState[proc_no] != BARRIER){      
    stateChange = true;            
    profileTransactionStateChange(proc_no);
  }  
  m_xactState[proc_no] = BARRIER;
  if (stateChange){
    g_system_ptr->getProfiler()->printTransactionState(0);
    m_xactLastStateChange[proc_no] = g_eventQueue_ptr->getTime();
  }  
}

Vector<char> XactVisualizer::getTransactionStateVector(){
  int num_processors = RubyConfig::numberOfProcessors() * RubyConfig::numberofSMTThreads();      
  Vector <char> xactStateVector;
  xactStateVector.setSize(num_processors);        
  for (int i = 0; i < num_processors; i++)
    xactStateVector[i] = s_xactStateStructMap[m_xactState[i]].outp;
  return xactStateVector;
}

void XactVisualizer::profileLastState(){
  int num_processors = RubyConfig::numberOfProcessors() * RubyConfig::numberofSMTThreads();      
  for (int i = 0; i < num_processors; i++){      
    if (m_xactLastStateChange[i] > 0)      
      profileTransactionStateChange(i);
  }          
}        

void XactVisualizer::profileTransactionStateChange(int proc_no){
  Time last_state_time = g_eventQueue_ptr->getTime() - m_xactLastStateChange[proc_no];

  switch(m_xactState[proc_no]){
    case XACT:
         g_system_ptr->getProfiler()->getXactProfiler()->profileTransCycles(proc_no, last_state_time);
         m_goodXactPeriod[proc_no] += last_state_time;
         if (numXact() == 1)
            m_systemXactTime += g_eventQueue_ptr->getTime() - m_systemXactBeginTime;        
         break;
    case STALLED:
         g_system_ptr->getProfiler()->profileStallTransCycles(proc_no, last_state_time);
         break;
    case ABORTING:
         g_system_ptr->getProfiler()->profileAbortingTransCycles(proc_no, last_state_time);
         break;
    case COMMITING:
         g_system_ptr->getProfiler()->profileCommitingTransCycles(proc_no, last_state_time);
         break;     
    case BACKOFF:
         g_system_ptr->getProfiler()->profileBackoffTransCycles(proc_no, last_state_time);
         break;
    case NONXACT:
         g_system_ptr->getProfiler()->profileNonTransCycles(proc_no, last_state_time);
         break;
    case BARRIER:     
         g_system_ptr->getProfiler()->profileBarrierCycles(proc_no, last_state_time);
         break;
    default:     
         ; 
  }         
}

bool XactVisualizer::isStalled(int proc_no){
  return (m_xactState[proc_no] == STALLED);       
}           

Time XactVisualizer::getSystemXactTime(){
  if (numXact() >= 1)
    m_systemXactTime += g_eventQueue_ptr->getTime() - m_systemXactBeginTime;              
  return m_systemXactTime;
}

int XactVisualizer::numXact(){
  int num_processors = RubyConfig::numberOfProcessors() * RubyConfig::numberofSMTThreads();      
  int count = 0;
  for (int i = 0; i < num_processors; i++){
    if (m_xactState[i] == XACT)
      count++;
  }
  return count;
}                              
                    
bool XactVisualizer::existXactActivity(){
  int num_processors = RubyConfig::numberOfProcessors() * RubyConfig::numberofSMTThreads();      
  for (int i = 0; i < num_processors; i++)
    if (m_xactState[i] != NONXACT)
      return true;
  return false;
}              

        
                                          
