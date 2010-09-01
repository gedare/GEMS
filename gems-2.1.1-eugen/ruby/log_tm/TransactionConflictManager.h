
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

#ifndef TRANSACTIONCONFLICTMANAGER_H
#define TRANSACTIONCONFLICTMANAGER_H

#include "Global.h"
#include "System.h"
#include "Profiler.h"
#include "Vector.h"
#include "Map.h"
#include "Address.h"
#include "GenericBloomFilter.h"
#include "Sequencer.h"

class TransactionConflictManager {
public:
  TransactionConflictManager(AbstractChip* chip_ptr, int version);
  ~TransactionConflictManager();
  
  void beginTransaction(int thread);
  void commitTransaction(int thread);
  void restartTransaction(int thread);

  
  bool shouldNackLoad(Address addr, uint64 remote_timestamp, MachineID remote_id);
  bool shouldNackStore(Address addr, uint64 remote_timestamp, MachineID remote_id);
  void notifyReceiveNack(int thread, Address addr, uint64 local_timestamp, uint64 remote_timestamp, MachineID remote_id);
  void notifyReceiveNackFinal(int thread, Address addr);
  void notifySendNack(Address addr, uint64 remote_timestamp, MachineID remote_id);
  uint64 getTimestamp(int thread);
  uint64 getOldestTimestamp();
  int getOldestThreadExcludingThread(int thread);
  int getOldestThread();

  bool possibleCycle(int thread);
  void setPossibleCycle(int thread);
  void clearPossibleCycle(int thread);
  bool nacked(int thread);

  bool magicWait(int thread);
  void setMagicWait(int thread);
  void setEnemyProcessor(int thread, int remote_proc);

  int getNumRetries(int thread);
  int getLowestConflictLevel(int thread);
  void setLowestConflictLevel(int thread, Address addr, bool nackedStore);
  int  getEnemyProc(int thread);
  
  void setVersion(int version);
  int getVersion() const;
private:
  int getProcID() const;
  int getLogicalProcID(int thread) const;
  bool isRemoteOlder(int thread, int remote_thread, uint64 local_timestamp, uint64 remote_timestamp, MachineID remote_id);
  
  AbstractChip * m_chip_ptr;
  int m_version;

  uint64 *m_timestamp;
  bool   *m_possible_cycle;
  bool   *m_lock_timestamp;
  int    *m_lowestConflictLevel;

  /*
  uint64 *m_max_waiting_ts;
  int    *m_waiting_proc;
  int    *m_waiting_thread;
  bool   *m_delay_thread;
  */

  bool   *m_shouldTrap;
  int    *m_enemyProc; 

  bool   *m_needMagicWait;
  bool   *m_enableMagicWait;
  int    *m_magicWaitingOn;
  Time   *m_magicWaitingTime;
  
  int    *m_numRetries;
  bool   *m_nacked;
  bool   **m_nackedBy;
  uint64 **m_nackedByTimestamp;

};  
 

#endif


