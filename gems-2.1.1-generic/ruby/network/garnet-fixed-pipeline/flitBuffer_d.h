/*
    Copyright (C) 1999-2008 by Mark D. Hill and David A. Wood for the
    Wisconsin Multifacet Project.  Contact: gems@cs.wisc.edu
    http://www.cs.wisc.edu/gems/

    --------------------------------------------------------------------

    This file is part of Garnet (Princeton's interconnect model),
    a component of the Multifacet GEMS (General Execution-driven 
    Multiprocessor Simulator) software toolset originally developed at 
    the University of Wisconsin-Madison.

    Garnet was developed by Niket Agarwal at Princeton University. Orion was
    developed by Princeton University.

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
 * flitBuffer_d.h
 *
 * Niket Agarwal, Princeton University
 *
 * */

#ifndef FLIT_BUFFER_D_H
#define FLIT_BUFFER_D_H

#include "NetworkHeader.h"
#include "PrioHeap.h"
#include "flit_d.h"

class flitBuffer_d {
public:
	flitBuffer_d();
	flitBuffer_d(int maximum_size);

	bool isReady();
	bool isReadyForNext();
	bool isEmpty();
	void print(ostream& out) const;
	bool isFull();
	void setMaxSize(int maximum);

	inline flit_d* getTopFlit()
	{
		return m_buffer.extractMin();
	}
	inline flit_d* peekTopFlit()
	{
		return m_buffer.peekMin();
	}
	inline void insert(flit_d *flt)
	{
		m_buffer.insert(flt);
	}
	/**********Data Members*********/
private:
	PrioHeap <flit_d *> m_buffer;
	int size, max_size;
};

ostream& operator<<(ostream& out, const flitBuffer_d& obj);

// ******************* Definitions *******************

// Output operator definition
extern inline 
ostream& operator<<(ostream& out, const flitBuffer_d& obj)
{
  obj.print(out);
  out << flush;
  return out;
}

#endif

