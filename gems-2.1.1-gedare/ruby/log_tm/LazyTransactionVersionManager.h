
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

/*
   This file has been modified by Kevin Moore and Dan Nussbaum of the
   Scalable Systems Research Group at Sun Microsystems Laboratories
   (http://research.sun.com/scalable/) to support the Adaptive
   Transactional Memory Test Platform (ATMTP).

   Please send email to atmtp-interest@sun.com with feedback, questions, or
   to request future announcements about ATMTP.

   ----------------------------------------------------------------------

   File modification date: 2008-02-23

   ----------------------------------------------------------------------
*/

#ifndef LAZYTRANSACTIONVERSIONMANAGER_H
#define LAZYTRANSACTIONVERSIONMANAGER_H

#include "Vector.h"
#include "Map.h"
#include "Global.h"
#include "Address.h"
#include "CacheRequestType.h"
#include "Chip.h"
#include "RegisterStateWindowed.h"


class LazyTransactionVersionManager {
public:
  LazyTransactionVersionManager(AbstractChip* chip_ptr, int version);
  ~LazyTransactionVersionManager();

  void addToWriteBuffer(int thread, int transactionLevel, Address physical_address, int size, Vector<uint8> data);
  Vector<uint8> forwardData(int thread, int transactionLevel, Address physical_address, int size); 
  void flushWriteBuffer(int thread, int transactionLevel);
  void discardWriteBuffer(int thread, int transactionLevel);
  
  void beginTransaction(int thread);
  void commitTransaction(int thread, bool isOpen);
  void restartTransaction(int thread, int xact_level);

  void hitCallback(int thread, Address physicalAddress);
private:
  int getProcID() const;
  int getLogicalProcID(int thread) const;
 
  void takeCheckpoint(int thread);
  bool existInWriteBuffer(int thread, physical_address_t addr);
  uint8 getDataFromWriteBuffer(int thread, physical_address_t addr);

  void issueWriteRequests(int thread);
  
  AbstractChip * m_chip_ptr;
  int m_version;

  Vector< Vector < Map<physical_address_t, uint8> > > m_writeBuffer;
  Vector< Vector < Map<physical_address_t, uint8> > > m_writeBufferBlocks;
  Vector< int > m_writeBufferSizes;
  Vector< Vector<RegisterState*> > m_registers;

  Address m_pendingWriteBufferRequest;
};  
 

#endif

