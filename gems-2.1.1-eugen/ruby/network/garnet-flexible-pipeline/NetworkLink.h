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
 * NetworkLink.h
 *
 * Niket Agarwal, Princeton University
 *
 * */
#ifndef NETWORK_LINK_H
#define NETWORK_LINK_H

#include "NetworkHeader.h"
#include "FlexibleConsumer.h"
#include "flitBuffer.h"
#include "PrioHeap.h"
#include "NetDest.h"

class GarnetNetwork;

class NetworkLink : public FlexibleConsumer {
public:
	NetworkLink();
	NetworkLink(int id, int latency, GarnetNetwork *net_ptr);
	~NetworkLink();

	void setLinkConsumer(FlexibleConsumer *consumer);
	void setSourceQueue(flitBuffer *srcQueue);
	flit* peekLink();
	flit* consumeLink();

	void print(ostream& out) const {}

	bool is_vc_ready(flit *t_flit);

	int get_id();
	void setInPort(int port);
	void setOutPort(int port);
	void wakeup();
	bool isReady();
	void grant_vc_link(int vc, Time grant_time);
	void release_vc_link(int vc, Time release_time);
	void request_vc_link(int vc, NetDest destination, Time request_time);
	bool isBufferNotFull_link(int vc);
	void setSource(FlexibleConsumer *source);
	double getLinkUtilization();
	Vector<int> getVcLoad();

protected:
	int m_id, m_latency;
	int m_in_port, m_out_port;
	int m_link_utilized;
	Vector<int > m_vc_load;
	GarnetNetwork *m_net_ptr;

	flitBuffer *linkBuffer;
	FlexibleConsumer *link_consumer;
	FlexibleConsumer *link_source;
	flitBuffer *link_srcQueue;
};

#endif

