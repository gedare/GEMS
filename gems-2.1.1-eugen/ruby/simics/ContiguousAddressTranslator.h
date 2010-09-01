
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
 * ContiguousAddressTranslator.h
 *
 * This object is used to translate non-contiguous physical addresses used by Simics to
 * contiguous physical addresses for Ruby. Ruby proper is not aware of the translation.
 * Ruby requires physical addresses to be contiguous for a variety of assumptions, many
 * of which are unknown at the time of this writing.
 *
 * Address translation is ONLY ENABLED if Ruby is compiled with CONTIGUOUS_ADDRESSES defined.
 * An option exits in the Makefile's SPECIAL_FLAGS to turn this option on.
 *
 * Translation occurs:
 * commands.C, which will use a shadow memory_transaction_t. Simics -> Ruby
 * interface.C, whenever an address is passed to Simics, Ruby -> Simics
 * interface.C, whenever an address is passed to Ruby, Simics -> Ruby
 *
 */

#ifndef CONTIGUOUSADDRESSTRANSLATOR_H
#define CONTIGUOUSADDRESSTRANSLATOR_H

#include "Global.h"
#include "Address.h"
#include "Vector.h"

class ContiguousAddressTranslator {
public:
  ContiguousAddressTranslator();
  ~ContiguousAddressTranslator();

  uint64 TranslateSimicsToRuby( uint64 addr ) const;
  uint64 TranslateRubyToSimics( uint64 addr ) const;

  bool AddressesAreContiguous() const { return m_b_contiguous; }

  /* Aliases */
  uint64 TranslateSimicsToRuby( const Address& addr ) const { return TranslateSimicsToRuby( addr.getAddress() ); }
  uint64 TranslateRubyToSimics( const Address& addr ) const { return TranslateRubyToSimics( addr.getAddress() ); }

private:

  int m_nNumMaps;
  bool m_b_contiguous;

  Vector<uint64> m_startAddrs;
  Vector<uint64> m_endAddrs;
  Vector<uint64> m_sizes;
  Vector<uint64> m_offsets; /* Def, m_offsets[n] = Sum, vary k from 0 to n-1 of offset[k], aka the sum of all previous sizes */
  Vector<uint64> m_translations;

};

#endif // #ifndef CONTIGUOUSADDRESSTRANSLATOR_H
