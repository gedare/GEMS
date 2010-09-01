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
 * NetworkLink_d.C
 *
 * Niket Agarwal, Princeton University
 *
 * */

#include "NetworkLink_d.h"
#include "NetworkConfig.h"
#include "GarnetNetwork_d.h"

NetworkLink_d::NetworkLink_d(int id)
{
	m_id = id;
	m_latency = 1;
	m_flit_width = NetworkConfig::getFlitSize();

	linkBuffer = new flitBuffer_d();
	m_link_utilized = 0;
	m_vc_load.setSize(NetworkConfig::getVCsPerClass()*NUMBER_OF_VIRTUAL_NETWORKS);

	for(int i = 0; i < NetworkConfig::getVCsPerClass()*NUMBER_OF_VIRTUAL_NETWORKS; i++)
		m_vc_load[i] = 0;
}

NetworkLink_d::NetworkLink_d(int id, int link_latency, GarnetNetwork_d *net_ptr)
{
	m_net_ptr = net_ptr;
	m_id = id;
	m_latency = link_latency;
	linkBuffer = new flitBuffer_d();
	m_link_utilized = 0;
	m_vc_load.setSize(NetworkConfig::getVCsPerClass()*NUMBER_OF_VIRTUAL_NETWORKS);

	for(int i = 0; i < NetworkConfig::getVCsPerClass()*NUMBER_OF_VIRTUAL_NETWORKS; i++)
		m_vc_load[i] = 0;
}

NetworkLink_d::~NetworkLink_d()
{
	delete linkBuffer;
}

void NetworkLink_d::setLinkConsumer(Consumer *consumer)
{
	link_consumer = consumer;
}

void NetworkLink_d::setSourceQueue(flitBuffer_d *srcQueue)
{
	link_srcQueue = srcQueue;
}

void NetworkLink_d::wakeup()
{
	if(link_srcQueue->isReady())
	{
		flit_d *t_flit = link_srcQueue->getTopFlit();
		t_flit->set_time(g_eventQueue_ptr->getTime() + m_latency);
		linkBuffer->insert(t_flit);
		g_eventQueue_ptr->scheduleEvent(link_consumer, m_latency);
		m_link_utilized++;
		m_vc_load[t_flit->get_vc()]++;
	}
}

Vector<int> NetworkLink_d::getVcLoad()
{
	return m_vc_load;
}

int NetworkLink_d::getLinkUtilization()
{
	return m_link_utilized;
}
