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
 * NetworkInterface.h
 *
 * Niket Agarwal, Princeton University
 *
 * */
#ifndef NET_INTERFACE_H
#define NET_INTERFACE_H

#include "NetworkHeader.h"
#include "GarnetNetwork.h"
#include "Vector.h"
#include "FlexibleConsumer.h"
#include "Message.h"
#include "NetworkLink.h"
#include "OutVcState.h"

class NetworkMessage;
class MessageBuffer;
class flitBuffer;

class NetworkInterface : public FlexibleConsumer {
public:
	NetworkInterface(int id, int virtual_networks, GarnetNetwork* network_ptr);

	~NetworkInterface();

	void addInPort(NetworkLink *in_link);
	void addOutPort(NetworkLink *out_link);

	void wakeup();
	void addNode(Vector<MessageBuffer *> &inNode, Vector<MessageBuffer *> &outNode);
	void grant_vc(int out_port, int vc, Time grant_time);
	void release_vc(int out_port, int vc, Time release_time);
	bool isBufferNotFull(int vc, int inport)
	{
		return true;
	}
	void request_vc(int in_vc, int in_port, NetDest destination, Time request_time);

	void printConfig(ostream& out) const;
        void print(ostream& out) const;

private:
/**************Data Members*************/
	GarnetNetwork *m_net_ptr;
	int m_virtual_networks, m_num_vcs, m_vc_per_vnet;
	NodeID m_id;
	
	Vector<OutVcState *> m_out_vc_state;
	Vector<int > m_vc_allocator;
	int m_vc_round_robin; // For round robin scheduling
	flitBuffer *outSrcQueue; // For modelling link contention
	
	NetworkLink *inNetLink;
	NetworkLink *outNetLink;

	// Input Flit Buffers
	Vector<flitBuffer *>   m_ni_buffers; // The flit buffers which will serve the Consumer

	Vector<MessageBuffer *> inNode_ptr; // The Message buffers that takes messages from the protocol
	Vector<MessageBuffer *> outNode_ptr; // The Message buffers that provides messages to the protocol 

	bool flitisizeMessage(MsgPtr msg_ptr, int vnet);
	int calculateVC(int vnet);
	void scheduleOutputLink();
	void checkReschedule();
};

#endif
