
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
 * DirectoryMemory.C
 * 
 * Description: See DirectoryMemory.h
 *
 * $Id$
 *
 */

#include "System.h"
#include "Driver.h"
#include "DirectoryMemory.h"
#include "RubySlicc_Util.h"
#include "RubyConfig.h" 
#include "Chip.h" 
#include "interface.h"

DirectoryMemory::DirectoryMemory(Chip* chip_ptr, int version)
{
  m_chip_ptr = chip_ptr;
  m_version = version;
  // THIS DOESN'T SEEM TO WORK -- MRM
  // m_size = RubyConfig::memoryModuleBlocks()/RubyConfig::numberOfDirectory();
  m_size = RubyConfig::memoryModuleBlocks();
  assert(m_size > 0);
  // allocates an array of directory entry pointers & sets them to NULL
  m_entries = new Directory_Entry*[m_size];  
  if (m_entries == NULL) {
    ERROR_MSG("Directory Memory: unable to allocate memory.");
  }

  for (int i=0; i < m_size; i++) {
    m_entries[i] = NULL;
  }
}

DirectoryMemory::~DirectoryMemory()
{
  // free up all the directory entries
  for (int i=0; i < m_size; i++) {
    if (m_entries[i] != NULL) {
      delete m_entries[i];
      m_entries[i] = NULL;
    }
  }

  // free up the array of directory entries
  delete[] m_entries;
}

// Static method
void DirectoryMemory::printConfig(ostream& out)
{
  out << "Memory config:" << endl;
  out << "  memory_bits: " << RubyConfig::memorySizeBits() << endl;
  out << "  memory_size_bytes: " << RubyConfig::memorySizeBytes() << endl; 
  out << "  memory_size_Kbytes: " << double(RubyConfig::memorySizeBytes()) / (1<<10) << endl;
  out << "  memory_size_Mbytes: " << double(RubyConfig::memorySizeBytes()) / (1<<20) << endl;
  out << "  memory_size_Gbytes: " << double(RubyConfig::memorySizeBytes()) / (1<<30) << endl;

  out << "  module_bits: " << RubyConfig::memoryModuleBits() << endl;
  out << "  module_size_lines: " << RubyConfig::memoryModuleBlocks() << endl; 
  out << "  module_size_bytes: " << RubyConfig::memoryModuleBlocks() * RubyConfig::dataBlockBytes() << endl; 
  out << "  module_size_Kbytes: " << double(RubyConfig::memoryModuleBlocks() * RubyConfig::dataBlockBytes()) / (1<<10) << endl; 
  out << "  module_size_Mbytes: " << double(RubyConfig::memoryModuleBlocks() * RubyConfig::dataBlockBytes()) / (1<<20) << endl; 
}

// Public method
bool DirectoryMemory::isPresent(PhysAddress address)
{
  return (map_Address_to_DirectoryNode(address) == m_chip_ptr->getID()*RubyConfig::numberOfDirectoryPerChip()+m_version);
}

Directory_Entry& DirectoryMemory::lookup(PhysAddress address)
{
  assert(isPresent(address));
  Index index = address.memoryModuleIndex();

  if (index < 0 || index > m_size) {
    WARN_EXPR(m_chip_ptr->getID()); 
    WARN_EXPR(address.getAddress());
    WARN_EXPR(index);
    WARN_EXPR(m_size);
    ERROR_MSG("Directory Memory Assertion: accessing memory out of range.");
  }
  Directory_Entry* entry = m_entries[index];

  // allocate the directory entry on demand.
  if (entry == NULL) {
    entry = new Directory_Entry;

    //    entry->getProcOwner() = m_chip_ptr->getID(); // FIXME - This should not be hard coded
    //    entry->getDirOwner() = true;        // FIXME - This should not be hard-coded

    // load the data from SimICS when first initalizing
    if (g_SIMICS) {
      if (DATA_BLOCK) {
        physical_address_t physAddr = address.getAddress();
        
        for(int j=0; j < RubyConfig::dataBlockBytes(); j++) {
          int8 data_byte = (int8) SIMICS_read_physical_memory( m_chip_ptr->getID(),
                                                               physAddr + j, 1 );
          //printf("SimICS, byte %d: %lld\n", j, data_byte );
          entry->getDataBlk().setByte(j, data_byte);
        }
        DEBUG_EXPR(NODE_COMP, MedPrio,entry->getDataBlk());
      }
    }

    // store entry to the table
    m_entries[index] = entry;    
  }

  return (*entry);
}

/*
void DirectoryMemory::invalidateBlock(PhysAddress address)
{
  assert(isPresent(address));

  Index index = address.memoryModuleIndex();

  if (index < 0 || index > m_size) {
    ERROR_MSG("Directory Memory Assertion: accessing memory out of range.");
  }

  if(m_entries[index] != NULL){
    delete m_entries[index];
    m_entries[index] = NULL;
  }

}
*/

void DirectoryMemory::print(ostream& out) const
{
  out << "Directory dump: " << endl;
  for (int i=0; i < m_size; i++) {
    if (m_entries[i] != NULL) {
      out << i << ": ";
      out << *m_entries[i] << endl;
    }
  }
}

