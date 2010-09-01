
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
 * $Id$
 *
 */

#include "StoreCache.h"
#include "System.h"
#include "Driver.h"
#include "Vector.h"
#include "DataBlock.h"
#include "SubBlock.h"
#include "Map.h"

// Helper class
struct StoreCacheEntry {
  StoreCacheEntry() { 
    m_byte_counters.setSize(RubyConfig::dataBlockBytes()); 
    for(int i=0; i<m_byte_counters.size(); i++) {
      m_byte_counters[i] = 0;
    }
    m_line_counter = 0; 
    
  } 
  Address m_addr;
  DataBlock m_datablock;
  Vector<int> m_byte_counters;
  int m_line_counter;
};

StoreCache::StoreCache()
{
  m_internal_cache_ptr = new Map<Address, StoreCacheEntry>;
}

StoreCache::~StoreCache()
{
  delete m_internal_cache_ptr;
}

bool StoreCache::isEmpty() const
{
  return m_internal_cache_ptr->size() == 0;
}

int StoreCache::size() const { return m_internal_cache_ptr->size(); }

void StoreCache::add(const SubBlock& block)
{
  if (m_internal_cache_ptr->exist(line_address(block.getAddress())) == false) {
    m_internal_cache_ptr->allocate(line_address(block.getAddress()));
  }
  
  StoreCacheEntry& entry = m_internal_cache_ptr->lookup(line_address(block.getAddress()));

  // For each byte in entry change the bytes and inc. the counters
  int starting_offset = block.getAddress().getOffset();
  int size = block.getSize();
  for (int index=0; index < size; index++) {
    // Update counter
    entry.m_byte_counters[starting_offset+index]++;

    // Record data
    entry.m_datablock.setByte(starting_offset+index, block.getByte(index));

    DEBUG_EXPR(SEQUENCER_COMP, LowPrio, block.getAddress());
    DEBUG_EXPR(SEQUENCER_COMP, LowPrio, int(block.getByte(index)));
    DEBUG_EXPR(SEQUENCER_COMP, LowPrio, starting_offset+index);
  }

  // Increment the counter
  entry.m_line_counter++;
}

void StoreCache::remove(const SubBlock& block)
{
  assert(m_internal_cache_ptr->exist(line_address(block.getAddress())));

  StoreCacheEntry& entry = m_internal_cache_ptr->lookup(line_address(block.getAddress()));

  // Decrement the byte counters
  int starting_offset = block.getAddress().getOffset();
  int size = block.getSize();
  for (int index=0; index < size; index++) {
    // Update counter
    entry.m_byte_counters[starting_offset+index]--;
  }
  
  // Decrement the line counter
  entry.m_line_counter--;
  assert(entry.m_line_counter >= 0);

  // Check to see if we should de-allocate this entry
  if (entry.m_line_counter == 0) {
    m_internal_cache_ptr->deallocate(line_address(block.getAddress()));
  }
}

bool StoreCache::check(const SubBlock& block) const
{
  if (m_internal_cache_ptr->exist(line_address(block.getAddress())) == false) {
    return false;
  } else {
    // Lookup the entry
    StoreCacheEntry& entry = m_internal_cache_ptr->lookup(line_address(block.getAddress()));
    
    // See if all the bytes are valid
    int starting_offset = block.getAddress().getOffset();
    int size = block.getSize();
    for (int index=0; index < size; index++) {
      if (entry.m_byte_counters[starting_offset+index] > 0) {
        // So far so good
      } else {
        // not all the bytes were valid
        return false;
      }
    }
  }
  return true;
}
    
void StoreCache::update(SubBlock& block) const
{
  if (m_internal_cache_ptr->exist(line_address(block.getAddress()))) {
    // Lookup the entry
    StoreCacheEntry& entry = m_internal_cache_ptr->lookup(line_address(block.getAddress()));
    
    // Copy all appropriate and valid bytes from the store cache to
    // the SubBlock
    int starting_offset = block.getAddress().getOffset();
    int size = block.getSize();
    for (int index=0; index < size; index++) {
      
      DEBUG_EXPR(SEQUENCER_COMP, LowPrio, block.getAddress());
      DEBUG_EXPR(SEQUENCER_COMP, LowPrio, int(entry.m_datablock.getByte(starting_offset+index)));
      DEBUG_EXPR(SEQUENCER_COMP, LowPrio, starting_offset+index);

      // If this byte is valid, copy the data into the sub-block
      if (entry.m_byte_counters[starting_offset+index] > 0) {
        block.setByte(index, entry.m_datablock.getByte(starting_offset+index));
      }
    }
  }
}

void StoreCache::print(ostream& out) const
{
  out << "[StoreCache]";
}

