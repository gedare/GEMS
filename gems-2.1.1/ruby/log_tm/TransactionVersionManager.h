
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

#
*/

#ifndef TRANSACTIONVERSIONMANAGER_H
#define TRANSACTIONVERSIONMANAGER_H

#include "Vector.h"
#include "Map.h"
#include "Global.h"
#include "Address.h"
#include "CacheRequestType.h"
#include "Chip.h"
#include "RegisterStateWindowed.h"


class TransactionVersionManager {
public:
  TransactionVersionManager(AbstractChip* chip_ptr, int version);
  ~TransactionVersionManager();
  
  bool beginTransaction(int thread);
  void commitTransaction(int thread, bool isOpen);
  void restartTransaction(int thread, int xact_level);

  void setRestorePC(int thread, int xact_level, unsigned int pc);
  void setLogBase(int thread);
  
  Address getLogPointerAddress(int thread);
  Address getLogPointerAddressPhysical(int thread);
  Address getLogBase(int thread);
  Address getLogBasePhysical(int thread);
  Address getLogFramePointer(int thread);
  Address getLogCommitActionBase(int thread);
  int     getLogSize(int thread);
  int     getNumCommitActions(int thread);
  
  void addUndoLogEntry(int thread, Address logical_address, Address address);
  void allocateLogEntry(int thread, Address address, int transactionLevel);

  void registerCompensatingAction(int thread);
  void registerCommitAction(int thread);
  void xmalloc(int thread);

  void updateLogPointers(int thread);

  bool shouldLog(int thread, Address addr, int transactionLevel);
  void processUndoLogEntry(int thread, physical_address_t log_entry, physical_address_t dest_address);
  void hardwareRollback(int thread);

  void setVersion(int version);
  int getVersion() const;
private:
  int getProcID() const;
  int getLogicalProcID(int thread) const;
  physical_address_t getDataTranslation(physical_address_t virtual_address, int thread) ;
  bool addLogEntry(int thread, int log_entry_size, Vector<uint32> &entry);
  void incrementLogPointer(int thread, int size_in_bytes);
  
  void takeCheckpoint(int thread);
  bool addNewLogFrame(int thread);
  
  AbstractChip * m_chip_ptr;
  int m_version;
  
  int *     m_logSize;   // logSize in bytes 
  int *     m_tid;    
  int *     m_maxLogSize; // max log size in kilo bytes

  int *     m_numCommitActions; 

  Address * m_logBase;  // Virtual Address of Log Base
  Address * m_logPointer;  // Virtual Address of LogPointer 
  Address * m_logFramePointer; // Virtual Address of TopMost frame header
  Address * m_logCommitActionBase; // VA of head of CA list 
  Address * m_logCommitActionPointer; // VA of last CA
  Address * m_tm_contextAddress; // Virtual Address of Software TM struct 

  Address * m_logPointer_physical;  // Virtual Address of LogPointer 
  Address * m_tm_contextAddress_physical; // Virtual Address of Software TM struct 
  Address * m_logFramePointer_physical; // Virtual Address of TopMost frame header
  
  Map<Address, Address> ** m_logAddressFilter_ptr;
  Map<physical_address_t, physical_address_t> *  m_logAddressTLB;
  
  Vector< Vector<RegisterState*> > m_registers;
};  
 

#endif

