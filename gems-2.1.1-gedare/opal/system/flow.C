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
 * FileName:  flow.C
 * Synopsis:  Implements an idealized control and data flow graph.
 * Author:    cmauer
 * Version:   $Id$
 */

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include "hfa.h"
#include "hfacore.h"
#include "memop.h"
#include "flow.h"

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Variable declarations                                                  */
/*------------------------------------------------------------------------*/

/// The static jump table for the flow inst class
pmf_flowExecute flow_inst_t::m_jump_table[i_maxcount];

/// memory allocator for dynamic.C
listalloc_t     flow_inst_t::m_myalloc;

/*------------------------------------------------------------------------*/
/* Forward declarations                                                   */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Flow Instruction Class                                                 */
/*------------------------------------------------------------------------*/

//**************************************************************************
flow_inst_t::flow_inst_t( static_inst_t *s, 
                          pseq_t        *pseq,
                          mstate_t      *mstate,
                          mem_dependence_t *mem_dep,
                          uint32 proc )
{
  m_proc = proc;
  m_pseq = pseq;
  m_static_inst = s;
  m_sequence_id = pseq->stepInorderNumber(m_proc);  
  m_mem_depth   = 0;
  
  m_is_taken      = false;
  m_virtual_address = (la_t) -1;
  m_traptype      = (uint16) Trap_NoTrap;
  m_address       = (la_t) -1;
  m_physical_addr = (la_t) -1;
  m_asi           = (uint16) -1;
  m_data_valid    = false;
  m_access_size   = m_static_inst->getAccessSize();
  m_events        = 0;
  m_ref_count     = 0;

  m_transactional_mode = false;
  m_transaction_id = 0;

  // increment the static instruction reference counter
  s->incrementRefCount();
  // increment this instructions ref count
  incrementRefCount();

  if (mstate != NULL) {
    abstract_pc_t &at  = mstate->at[m_proc];
    reg_box_t     &rbox= mstate->regs;

    m_virtual_address = at.pc;
    m_actual_at = at;

    // if this instruction changes the current window pointer... do it!
    uint16 cwp  = at.cwp;
    uint16 gset = at.gset;

  // for each source register, find the corresponding dependence
    for (int i = 0; i < SI_MAX_SOURCE; i++) {
      reg_id_t &id = m_static_inst->getSourceReg(i);
      m_source_reg[i].copy( id, rbox);          
      m_source_reg[i].setVanillaState( cwp, gset );
    }
  
    // see comment in control_inst_t for how CWP updating works
    if (s->getFlag( SI_UPDATE_CWP )) {
      if (s->getOpcode() == i_save) {
        cwp++;
        cwp &= (NWINDOWS - 1);
      } else if (s->getOpcode() == i_restore) {
        cwp--;
        cwp &= (NWINDOWS - 1);
      } else {
        SIM_HALT;
      }
    }

    // for each destination register, add a corresponding dependence
    for (int i = 0; i < SI_MAX_DEST; i++) {
      reg_id_t &id = m_static_inst->getDestReg(i);
      m_dest_reg[i].copy( id, rbox );               
      m_dest_reg[i].setVanillaState( cwp, gset );
    }
  } // end if state != NULL
}

//**************************************************************************
flow_inst_t::~flow_inst_t( void )
{
  // walk the list of nodes in pred, succ; free them all
  m_succ.removeall();
  m_pred.removeall();
  
  // if the static instruction is marked, free it
  m_static_inst->decrementRefCount();
  if (m_static_inst->getFlag( SI_TOFREE )) {
    if (m_static_inst->getRefCount() == 0) {
      delete m_static_inst;
    }
    // CM FIX: no longer trying to figure out if ref counting is a good idea
    //ERROR_OUT("warning: ref counting is a good idea!\n");
  }
}

//**************************************************************************
void flow_inst_t::advancePC( void )
{
  if ( m_static_inst->getType() == DYN_CONTROL) {
    // control instruction has already implemented advancing the PC
    return;
  } else {
    m_static_inst->actualPC_execute( &m_actual_at );
  }
}

//**************************************************************************
void flow_inst_t::readMemory( void )
{
  static_inst_t *s = getStaticInst();

  OpalMemop_t accessType;
  switch (s->getType()) {
  case DYN_LOAD:
    accessType = OPAL_LOAD;
    break;
  case DYN_STORE:
    accessType = OPAL_STORE;
    break;
  case DYN_PREFETCH:
    accessType = OPAL_LOAD;
    break;
  case DYN_ATOMIC:
    accessType = OPAL_ATOMIC;
    break;
  default:
    // not a load, store, atomic or prefetch
    return;
  }
  
  // calculate the virtual address being accessed
  if ( s->getFlag(SI_ISIMMEDIATE) ) {
    m_address = getSourceReg(0).getARF()->readInt64(getSourceReg(0), getProc()) + 
      s->getImmediate();
  } else {
    m_address = getSourceReg(0).getARF()->readInt64(getSourceReg(0), getProc()) + 
      getSourceReg(1).getARF()->readInt64(getSourceReg(1), getProc());
  }
#if DEBUG_IDEAL
  DEBUG_OUT("warning: flow: readMemory: virtual address: 0x%0llx\n",
            m_address);
#endif

  // get the ASI for this access
  if ( s->getFlag( SI_ISASI ) ) {
    bool is_block_load = false;
    m_asi = memop_getASI( s->getInst(), getSourceReg(3), &is_block_load, getProc() );
    if (!is_block_load) {
      // if the asi is not a block load, and we've reserved 64 bytes,
      // the actual load or store size is 8 bytes
      if (s->getOpcode() == i_lddfa || s->getOpcode() == i_stdfa) {
        m_access_size = 8;
        
        // later we'll forward data from the 0th register using readInt64
        if (s->getOpcode() == i_stdfa) {
          getSourceReg(2).setVanilla(0);
        }
      }
    }
  } else {
    int  inverse_endian = (m_actual_at.pstate >> 9) & 0x1;
    if (!inverse_endian) {
      if ( m_actual_at.tl == 0 ) {
        m_asi = ASI_PRIMARY;
      } else {
        m_asi = ASI_NUCLEUS;
      }
    } else {
      if ( m_actual_at.tl == 0 ) {
        m_asi = ASI_PRIMARY_LITTLE;
      } else {
        m_asi = ASI_NUCLEUS_LITTLE;
      }
    }
  }
  ASSERT( m_asi != (uint16) -1 );
  
  // if the ASI is not cacheable, use the functional execution
  if ( !pstate_t::is_cacheable( m_asi ) ) {
    setTrapType( Trap_Use_Functional );
    return;    
  }

  // translate the address
  trap_type_t t = Trap_NoTrap;
  t = m_pseq->mmuAccess( m_address, m_asi, m_access_size, accessType,
                         m_actual_at.pstate, m_actual_at.tl, &m_physical_addr, m_proc, true );
#if DEBUG_IDEAL
  DEBUG_OUT("warning: flow: readMemory: physical address: 0x%0llx\n",
            m_physical_addr);
  DEBUG_OUT("warning: flow: readMemory: PSTATE: 0x%0x\n", m_actual_at.pstate);
  DEBUG_OUT("warning: flow: readMemory: TL: 0x%0x\n", m_actual_at.tl);
  DEBUG_OUT("warning: flow: readMemory: ASI: 0x%0x\n", m_asi);
#endif

  if ( m_physical_addr != (pa_t) -1 &&
       memory_inst_t::isIOAccess( m_physical_addr ) ) {
    setTrapType( Trap_Use_Functional );
    return;
  }

  if (t != Trap_NoTrap) {
    setTrapType( t );
    return;
  }

  if ( pstate_t::is_cacheable( m_asi ) ) {
    // read the physical memory
    m_data_valid = m_pseq->readPhysicalMemory( m_physical_addr,
                                               (int) m_access_size,
                                               getData(), m_proc );
  } else {
    // I/O access
    setTrapType( Trap_Use_Functional );
  }
}

//**************************************************************************
void flow_inst_t::print( void )
{
  DEBUG_OUT(" flow instruction: [0x%0x] [%s]\n", 
                   this,
                   decode_opcode( (enum i_opcode) getStaticInst()->getOpcode() ));
  DEBUG_OUT("   PC    : 0x%0llx\n", getVPC());
  DEBUG_OUT("   inst  : 0x%0x\n", getStaticInst()->getInst());
  if (getTrapType() != Trap_NoTrap) {
    DEBUG_OUT("   TRAP: 0x%0x\n", getTrapType() );
  }

  if (m_data_valid) {
    if (m_access_size <= 8) {
      DEBUG_OUT("   DATA: 0x%0llx\n", getUnsignedData());
    }
  } else {
    DEBUG_OUT("   NO DATA.\n");
  }
  
  DEBUG_OUT(" dynamic source regs:   \n");
  for (int i = 0; i < SI_MAX_SOURCE; i ++) {
    reg_id_t &reg = getSourceReg(i);
    if (!reg.isZero()) {
      DEBUG_OUT("  %2.2d ", i);
      reg.getARF()->print( reg, getProc() );        
    }
  }

  DEBUG_OUT(" dynamic dest regs:   \n");
  for (int i = 0; i < SI_MAX_DEST; i ++) {
    reg_id_t &reg = getDestReg(i);
    if (!reg.isZero()) {
      DEBUG_OUT("  %2.2d ", i);
      reg.getARF()->print( reg, getProc() );       
    }
  }
}

//**************************************************************************
void    flow_inst_t::printGraph( uint32 depth )
{
  char  str[255];
  char  dot_str[255];

  // print this node
  int32 count = m_static_inst->printDisassemble( str );
  strcpy( dot_str, "" );
  for (uint32 i = 0; i < depth; i++) {
    strcat( dot_str, "." );
  }
  DEBUG_OUT("%s%s [%lld]\n", dot_str, str, getID() );

  // walk the list of successors, printing each in a breadth first manner
  depth = depth + 1;
  cfg_list_t     *succ = &m_succ;
  while (succ != NULL) {
    flow_inst_t  *node = succ->getNode();
    if (node != NULL) {
      node->printGraph( depth );
    }
    succ = succ->getNext();
  }
}

//**************************************************************************
void flow_inst_t::debugInputs( pstate_t *state )
{
  check_result_t   result;
  
  result.verbose = true;
  // for each (non-null) source, check it

  for (int i = 0; i < SI_MAX_SOURCE; i++) {
    reg_id_t &id = m_source_reg[i];
    if (!id.isZero()) {
      // check this source register (essentially printing it out)
      id.getARF()->check( id, state, &result, getProc() );       
    }
  }

}

/*------------------------------------------------------------------------*/
/* Control Flow Graph List                                                */
/*------------------------------------------------------------------------*/

//**************************************************************************
cfg_list_t::cfg_list_t()
{
  m_node = NULL;
  m_next = NULL;
}

//**************************************************************************
cfg_list_t::~cfg_list_t( )
{
}

//**************************************************************************
void cfg_list_t::add( flow_inst_t *node )
{
  cfg_list_t *newnode = new cfg_list_t();
  newnode->m_node = node;
  newnode->m_next = this->m_next;
  this->m_next = newnode;
}

//**************************************************************************
bool cfg_list_t::contains( flow_inst_t *node )
{
  cfg_list_t *scannode;
  
  // scan list to see if this node is already on the list
  scannode = this;
  while (scannode != NULL) {
    if (scannode->m_node == node)
      return true;
    scannode = scannode->m_next;
  }
  return false;
}

//**************************************************************************
void cfg_list_t::add_unique( flow_inst_t *node )
{
  if (this->contains(node))
    return;
  
  // add the node to the head of this list
  add( node );
}

//**************************************************************************
void cfg_list_t::print( void )
{
  cfg_list_t *l = this->m_next;

  while (l != NULL) {
    printf("%d ", l->m_node->getID());
    l = l->m_next;
  }
}

//**************************************************************************
void cfg_list_t::removeall( void )
{
  cfg_list_t *l;
  cfg_list_t *next;
  
  l = this->m_next;

  // free all nodes in a particular list
  while (l != NULL) {
    next = l->m_next;
    delete l;
    l = next;
  }
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

