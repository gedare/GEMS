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
 * NetworkInterface_d.h
 *
 * Niket Agarwal, Princeton University
 *
 * */

#ifndef NET_INTERFACE_D_H
#define NET_INTERFACE_D_H

#include "NetworkHeader.h"
#include "GarnetNetwork_d.h"
#include "Vector.h"
#include "Consumer.h"
#include "Message.h"
#include "NetworkLink_d.h"
#include "CreditLink_d.h"
#include "OutVcState_d.h"

class NetworkMessage;
class MessageBuffer;
class flitBuffer_d;

class NetworkInterface_d : public Consumer {
public:
	NetworkInterface_d(int id, int virtual_networks, GarnetNetwork_d* network_ptr);

	~NetworkInterface_d();

	void addInPort(NetworkLink_d *in_link, CreditLink_d *credit_link);
	void addOutPort(NetworkLink_d *out_link, CreditLink_d *credit_link);

	void wakeup();
	void addNode(Vector<MessageBuffer *> &inNode, Vector<MessageBuffer *> &outNode);
	void printConfig(ostream& out) const;
        void print(ostream& out) const;
	int get_vnet(int vc);

private:
/**************Data Members*************/
	GarnetNetwork_d *m_net_ptr;
	int m_virtual_networks, m_num_vcs, m_vc_per_vnet;
	NodeID m_id;
	Vector<OutVcState_d *> m_out_vc_state;
	Vector<int > m_vc_allocator;
	int m_vc_round_robin; // For round robin scheduling
	flitBuffer_d *outSrcQueue; // For modelling link contention
	flitBuffer_d *creditQueue;

	NetworkLink_d *inNetLink;
	NetworkLink_d *outNetLink;
	CreditLink_d *m_credit_link;
	CreditLink_d *m_ni_credit_link;

	// Input Flit Buffers
	Vector<flitBuffer_d *>   m_ni_buffers; // The flit buffers which will serve the Consumer
	Vector<Time > m_ni_enqueue_time;

	Vector<MessageBuffer *> inNode_ptr; // The Message buffers that takes messages from the protocol
	Vector<MessageBuffer *> outNode_ptr; // The Message buffers that provides messages to the protocol 

	bool flitisizeMessage(MsgPtr msg_ptr, int vnet);
	int calculateVC(int vnet);
	void scheduleOutputLink();
	void checkReschedule();
};

#endif
