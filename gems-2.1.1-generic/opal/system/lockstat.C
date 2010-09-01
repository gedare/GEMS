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
 * FileName:  lockstat.C
 * Synopsis:  implements the lock statistics class
 * Author:    cmauer
 * Version:   $Id: lockstat.C 1.5 04/08/06 13:31:42-05:00 lyen@opus.cs.wisc.edu $
 */

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include "hfa.h"
#include "hfacore.h"
#include "stopwatch.h"
#include "histogram.h"
#include "lockstat.h"

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


/*------------------------------------------------------------------------*/
/* Lock stat structure....                                                */
/*------------------------------------------------------------------------*/

//**************************************************************************
lock_stat_t::lock_stat_t( void )
{
  m_virtual_address = 0;
  m_length_held = new histogram_t( "lock_stat", 64 );
  m_atomic_pc = new histogram_t( "atomic_pc", 64 );
  m_store_pc = new histogram_t( "store_pc", 64 );
  m_acquire_time = 0;
  m_acquires = 0;
  m_releases = 0;

  m_atomic_fails = 0;
  m_contended = 0;
  m_cpu_transfer = 0;
  m_held = 0;
  m_last_touched = -1;
  m_last_thread  = 0;
}

//**************************************************************************
lock_stat_t::~lock_stat_t()
{
  delete m_length_held;
  delete m_atomic_pc;
  delete m_store_pc;
}

//**************************************************************************
void lock_stat_t::lock_read( pseq_t *pseq, load_inst_t *loadop )
{
}

//**************************************************************************
void lock_stat_t::lock_write( pseq_t *pseq, store_inst_t *storeop )
{
  //it seems this function is only called in sysstat.C, but is ifdefed out...

#if 0
  pseq->out_info("%s: 0x%0llx 0x%0llx STORE\n",
                 decode_opcode( s->getOpcode() ),
                 addr, storeop->getVPC());
#endif
  if (storeop->isDataValid()) {
    m_store_pc->touch( storeop->getVPC(), 1 );
    uint32 len_held = pseq->m_overall_timer[storeop->getProc()]->relative_time() - 
      m_acquire_time;
    m_releases++;
    m_held = 0;
    m_length_held->touch( len_held, 1 );
  }
}

//**************************************************************************
void lock_stat_t::lock_atomic( pseq_t *pseq, atomic_inst_t *atomicop )
{
  m_physical_address = atomicop->getPhysicalAddress();
  m_virtual_address = atomicop->getAddress();
  m_atomic_pc->touch( atomicop->getVPC(), 1 );
#if 0
  pseq->out_info("%s: 0x%0llx  data: 0x%0llx  masked: 0x%0llx\n",
                 decode_opcode( s->getOpcode() ), atomicop->getAddress(),
                 atomicop->isDataValid(), *atomicop->getData(),
                 atomicop->getUnsignedData() );
#endif
  if (!atomicop->atomicMemoryWrite()) {
    m_atomic_fails++;
  }
  if (atomicop->isDataValid() && (atomicop->getUnsignedData() != 0x0)) {
    if (m_held) {
      m_contended++;
    } else {
      if ( m_last_touched != -1 ) {
        if ( m_last_touched != pseq->getID() ) {
          m_cpu_transfer++;
        }
      }
      m_held = 1;
      m_acquires++;
      m_acquire_time = pseq->m_overall_timer[atomicop->getProc()]->relative_time();
      m_last_touched = pseq->getID();
    }
  } else {
    uint32 len_held = pseq->m_overall_timer[atomicop->getProc()]->relative_time() - 
      m_acquire_time;
    m_releases++;
    m_held = 0;
    m_length_held->touch( len_held, 1 );
  }
}

//**************************************************************************
void lock_stat_t::print( out_intf_t *stream )
{
  stream->out_info( "LOCK: VA:0x%0llx PA:0x%0llx\n",
                    m_virtual_address, m_physical_address );
  stream->out_info( "\tacquires: %d\n", m_acquires );
  stream->out_info( "\treleases: %d\n", m_releases );
  stream->out_info( "\tfailed atomics: %d\n", m_atomic_fails );
  stream->out_info( "\tfailed acquire: %d\n", m_contended );
  stream->out_info( "\ttransfers: %d\n", m_cpu_transfer );
  stream->out_info( "\tatomic PCs:\n");
  m_atomic_pc->print( stream );
  stream->out_info( "\tstore PCs:\n");
  m_store_pc->print( stream );
  stream->out_info( "\theld histogram:\n");
  m_length_held->print( stream );
}

/*------------------------------------------------------------------------*/
/* Public methods                                                         */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Accessor(s) / mutator(s)                                               */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Private methods                                                        */
/*------------------------------------------------------------------------*/

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

