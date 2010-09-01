
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

/*
 * ContiguousAddressTranslator.C
 *
 * See ContiguousAddressTranslator.h
 *
 */

#include "ContiguousAddressTranslator.h"
#include "interface.h"

ContiguousAddressTranslator::ContiguousAddressTranslator() {

  m_b_contiguous = false;

  /*
   * Ask Simics for the maps
   */
  conf_object_t *phys_mem;
  phys_mem = SIM_get_object("phys_mem0");
  
  attr_value_t map = SIM_get_attribute( phys_mem, "map" );
  if(map.kind!=Sim_Val_List) {
    ERROR_MSG("Danger, Will Robinson! Physical Memory Object Map is not a list??? \n");
  } else {
    attr_list_t map_list = map.u.list;
    integer_t list_size = map_list.size;
    m_nNumMaps = (int) list_size;

    m_startAddrs.setSize(m_nNumMaps);
    m_endAddrs.setSize(m_nNumMaps);
    m_sizes.setSize(m_nNumMaps);
    m_offsets.setSize(m_nNumMaps);
    m_translations.setSize(m_nNumMaps);

    attr_list_t list;
    
    for(int i=0;i<list_size;i++) {
      list = map_list.vector[i].u.list;
      if(map_list.vector[i].kind != Sim_Val_List ) {
        ERROR_MSG("Danger, Will Robinson! Physical Memory Object Map List Entries Aren't Lists??? \n");
      } else {
        integer_t inner_list_size = list.size;
        assert(inner_list_size == 9);

        attr_value_t inner_list_elem = list.vector[0]; // base address
        m_startAddrs[i] = (uint64) inner_list_elem.u.integer;

        inner_list_elem = list.vector[4]; // size
        m_sizes[i] = (uint64) inner_list_elem.u.integer;
       
      }

    }

    SIM_free_attribute(map);
  }

  /*
   * start addresses and sizes are now known, compute end addresses and offsets
   * ignore the last member of the map, however, since it appears to be some 
   * kind of seldom-used ROM
   */

  for(int i=0;i<(m_nNumMaps-1);i++) {
    m_endAddrs[i] = m_startAddrs[i] + m_sizes[i];
    m_offsets[i] = 0;
    for(int j=0;j<i;j++) {
      m_offsets[i] += m_sizes[j];
    }

    m_translations[i] = m_startAddrs[i] - m_offsets[i];
  }

  /*
   * Check if addresses are contiguous or not
   */

  m_b_contiguous = true;
  for(int i=0;i<(m_nNumMaps-2);i++) {
    if( m_endAddrs[i] != m_startAddrs[i+1] ) {
      m_b_contiguous = false;
      break;
    }
  }
}

ContiguousAddressTranslator::~ContiguousAddressTranslator() {

}

uint64 ContiguousAddressTranslator::TranslateSimicsToRuby( uint64 addr ) const {
  uint64 translated = 0;

  for(int i=0;i<m_nNumMaps-1;i++) {
    if(m_startAddrs[i+1] > addr) {
      translated = addr - m_translations[i];
      break;
    }
  }
  /*
   * If the loop falls through, then addr is part of an untranslated region anyway,
   * like that silly little ROM that sits at the end of memory 
   */
  return translated;
}

uint64 ContiguousAddressTranslator::TranslateRubyToSimics( uint64 addr ) const {
  uint64 translated = 0;
  for(int i=0;i<m_nNumMaps-1;i++) {
    if( addr < m_offsets[i] ) {
      translated = addr + m_translations[i];
      break;
    }
  }
  return translated;
}
