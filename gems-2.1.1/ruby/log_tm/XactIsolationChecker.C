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

#include "XactIsolationChecker.h"
#include "RubyConfig.h"
#include "CacheRequestType.h"
#include "interface.h"

#include <assert.h>
#include <stdlib.h>

XactIsolationChecker::XactIsolationChecker() {
  int numLogicalProcessors = RubyConfig::numberOfProcessors() * RubyConfig::numberofSMTThreads();   
  m_readSet.setSize(numLogicalProcessors);
  m_writeSet.setSize(numLogicalProcessors);
  m_abortingProcessor.setSize(numLogicalProcessors); 
  for (int i = 0; i < numLogicalProcessors; i++){
    m_readSet[i].setSize(1);
    m_writeSet[i].setSize(1);
    m_abortingProcessor[i] = false;
  }          
}

XactIsolationChecker::~XactIsolationChecker() {
}

bool XactIsolationChecker::existInReadSet(int proc, Address addr){
  int numLevels = m_readSet[proc].size();
  bool found = false;
  
  for (int i = 0; i < numLevels; i++){
    if (m_readSet[proc][i].exist(addr)){ 
      found = true;
      break;
    }
  }
  return found;                
}                             

bool XactIsolationChecker::existInWriteSet(int proc, Address addr){
  int numLevels = m_writeSet[proc].size();
  bool found = false;
  
  for (int i = 0; i < numLevels; i++){
    if (m_writeSet[proc][i].exist(addr)){ 
      found = true;
      break;
    }
  }
  return found;                
}                             

bool XactIsolationChecker::checkXACTConsistency(int proc, Address addr, CacheRequestType accessType){
   int numLogicalProcessors = RubyConfig::numberOfProcessors() * RubyConfig::numberofSMTThreads();     
   addr.makeLineAddress();      
  //cout << proc << " XACT CONSISTENCY CHECKER: CHECK XACT CONSISTENCY 0x" << hex << addr << dec << " ACCESS TYPE: " << accessType <<  endl;         
  bool ok = true;
  for (int i = 0; i < numLogicalProcessors; i++){
    if (i == proc) continue;
    // Processors that are in the process of aborting their transactions.
    // It is ok to access the read/write sets belonging to these transactions.
    if (m_abortingProcessor[i] == true) continue;
    switch(accessType){
      case CacheRequestType_LD:
      case CacheRequestType_LD_XACT:
          if (existInWriteSet(i, addr)){
            cout << proc << " XACT CONSISTENCY CHECKER: FAILED 0x" << hex << addr <<
              " ACCESS TYPE: " << dec << accessType << " IN WRITE SET OF " << i << " PC " << SIMICS_get_program_counter(proc) << " RANDOM_SEED " << g_RANDOM_SEED << endl;              
            ok = false;
          }          
          break;             
      case CacheRequestType_ST:
      case CacheRequestType_ST_XACT:
      case CacheRequestType_ATOMIC:
          if (existInReadSet(i, addr)){
             cout << proc << " XACT CONSISTENCY CHECKER: FAILED 0x" << hex << addr <<
               " ACCESS TYPE: " << dec << accessType << " IN READ SET OF " << i << " PC " << SIMICS_get_program_counter(proc) << " RANDOM_SEED " << g_RANDOM_SEED << endl; 
             printReadWriteSets(i); 
             ok = false;
          }   
          if (existInWriteSet(i, addr)){
             cout << proc << " XACT CONSISTENCY CHECKER: FAILED 0x" << hex << addr <<
               " ACCESS TYPE: " << dec << accessType << " IN WRITE SET OF " << i << " PC " << SIMICS_get_program_counter(proc) << " RANDOM_SEED " << g_RANDOM_SEED  << endl; 
             printReadWriteSets(i); 
             ok = false;
          }   
          break;
       default:
           break;
     }
  }              
  return ok;
}          

void XactIsolationChecker::addToReadSet(int proc, Address addr){
  addToReadSet(proc, addr, 1);
}
          
void XactIsolationChecker::addToReadSet(int proc, Address addr, int xact_level){
  assert(xact_level > 0);
  
  if (xact_level > m_readSet[proc].size()){
    m_readSet[proc].setSize(xact_level);
  }
  
  if (!m_readSet[proc][xact_level - 1].exist(addr)){
    m_readSet[proc][xact_level-1].add(addr, 'y');
  }  

}
        
void XactIsolationChecker::addToWriteSet(int proc, Address addr){
  addToWriteSet(proc, addr, 1);
}
          
void XactIsolationChecker::addToWriteSet(int proc, Address addr, int xact_level){
  assert(xact_level > 0);
  
  if (xact_level > m_writeSet[proc].size()){
    m_writeSet[proc].setSize(xact_level);
  }
  
  if (!m_writeSet[proc][xact_level - 1].exist(addr)){
    m_writeSet[proc][xact_level-1].add(addr, 'y');
  }  
}
        
void XactIsolationChecker::removeFromReadSet(int proc, Address addr, int xact_level){
  if (m_readSet[proc][xact_level-1].exist(addr)){
    if(XACT_DEBUG){
      cout << "XactIsolationChecker REMOVING ADDR " << addr << " from read set of " << proc << " level " << xact_level << endl;
    }
    m_readSet[proc][xact_level-1].erase(addr);
  }
}
        
void XactIsolationChecker::removeFromWriteSet(int proc, Address addr, int xact_level){
  if (m_writeSet[proc][xact_level-1].exist(addr)){
    if(XACT_DEBUG){
      cout << "XactIsolationChecker REMOVING ADDR " << addr << " from write set of " << proc << " level " << xact_level << endl;
    }
    m_writeSet[proc][xact_level-1].erase(addr);
  }
}
        
void XactIsolationChecker::clearWriteSet(int proc, int xact_level){
  if (xact_level > m_writeSet[proc].size())
    return;              
  for (int i = xact_level; i <= m_writeSet[proc].size(); i++){    
    m_writeSet[proc][i-1].clear();
  }
  m_writeSet[proc].setSize(xact_level-1);  
}
        
void XactIsolationChecker::clearReadSet(int proc, int xact_level){
  if (xact_level > m_readSet[proc].size())
    return;              
  for (int i = xact_level; i <= m_readSet[proc].size(); i++){    
    m_readSet[proc][i-1].clear();
  }  
  m_readSet[proc].setSize(xact_level-1);  
}

void XactIsolationChecker::copyReadSet(int proc, int old_xact_level, int new_xact_level){
  if (old_xact_level > m_readSet[proc].size()) return;      
  if (new_xact_level > m_readSet[proc].size())
    m_readSet[proc].setSize(new_xact_level);        

  Vector<Address> readSet = m_readSet[proc][old_xact_level-1].keys();

  for (int i = 0; i < readSet.size(); i++){
    addToReadSet(proc, readSet[i], new_xact_level);
  }
}            

void XactIsolationChecker::copyWriteSet(int proc, int old_xact_level, int new_xact_level){
  if (old_xact_level > m_writeSet[proc].size()) return;      
  if (new_xact_level > m_writeSet[proc].size())
    m_writeSet[proc].setSize(new_xact_level);        

  Vector<Address> writeSet = m_writeSet[proc][old_xact_level-1].keys();

  for (int i = 0; i < writeSet.size(); i++){
    addToWriteSet(proc, writeSet[i], new_xact_level);
  }
}    

void XactIsolationChecker::setAbortingProcessor(int proc) {
  m_abortingProcessor[proc] = true;
} 

void XactIsolationChecker::clearAbortingProcessor(int proc) {          
  m_abortingProcessor[proc] = false;
}          

void XactIsolationChecker::printReadWriteSets(int proc) {
  cout << " PROCESSOR: " << proc << endl;
  cout << " READ SET: ";
  for (int i = 0; i < m_readSet[proc].size(); i++){
    cout << " LEVEL " << i << ": ";
    Vector<Address> readSet = m_readSet[proc][i].keys();
    for (int j = 0; j < readSet.size(); j++)
      cout << readSet[j] << " ";
  } 
  cout << endl;   
  cout << " WRITE SET: ";
  for (int i = 0; i < m_writeSet[proc].size(); i++){
    cout << " LEVEL " << i << ": ";
    Vector<Address> writeSet = m_writeSet[proc][i].keys();
    for (int j = 0; j < writeSet.size(); j++)
      cout << writeSet[j] << " ";
  } 
  cout << endl;   
}  
                     

        
