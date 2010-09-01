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
 * NetworkLink.C
 *
 * Niket Agarwal, Princeton University
 *
 * */

#include "NetworkLink.h"
#include "NetworkConfig.h"
#include "GarnetNetwork.h"

NetworkLink::NetworkLink(int id, int latency, GarnetNetwork *net_ptr)
{
	m_id = id;
	linkBuffer = new flitBuffer();
	m_in_port = 0;
	m_out_port = 0;
	m_link_utilized = 0;
	m_net_ptr = net_ptr;
	m_latency = latency;
	int num_net = NUMBER_OF_VIRTUAL_NETWORKS;
	int num_vc = NetworkConfig::getVCsPerClass();
	m_vc_load.setSize(num_net*num_vc);

	for(int i = 0; i < num_net*num_vc; i++)
		m_vc_load[i] = 0;
}

NetworkLink::~NetworkLink()
{
	delete linkBuffer;
}

int NetworkLink::get_id()
{	
	return m_id;
}

void NetworkLink::setLinkConsumer(FlexibleConsumer *consumer)
{
	link_consumer = consumer;
}

void NetworkLink::setSourceQueue(flitBuffer *srcQueue)
{
	link_srcQueue = srcQueue;
}

void NetworkLink::setSource(FlexibleConsumer *source)
{
	link_source = source;
}
void NetworkLink::request_vc_link(int vc, NetDest destination, Time request_time)
{
	link_consumer->request_vc(vc, m_in_port, destination, request_time);
}
bool NetworkLink::isBufferNotFull_link(int vc)
{
	return link_consumer->isBufferNotFull(vc, m_in_port);
}

void NetworkLink::grant_vc_link(int vc, Time grant_time)
{
	link_source->grant_vc(m_out_port, vc, grant_time);
}

void NetworkLink::release_vc_link(int vc, Time release_time)
{
	link_source->release_vc(m_out_port, vc, release_time);
}

Vector<int> NetworkLink::getVcLoad()
{
	return m_vc_load;
}

double NetworkLink::getLinkUtilization()
{
	Time m_ruby_start = m_net_ptr->getRubyStartTime();
	return (double(m_link_utilized)) / (double(g_eventQueue_ptr->getTime()-m_ruby_start));
}

bool NetworkLink::isReady()
{
	return linkBuffer->isReady();
}

void NetworkLink::setInPort(int port)
{
	m_in_port = port;
}

void NetworkLink::setOutPort(int port)
{
	m_out_port = port;
}

void NetworkLink::wakeup()
{
	if(link_srcQueue->isReady())
	{
		flit *t_flit = link_srcQueue->getTopFlit();
		t_flit->set_time(g_eventQueue_ptr->getTime() + m_latency);
		linkBuffer->insert(t_flit);
		g_eventQueue_ptr->scheduleEvent(link_consumer, m_latency);
		m_link_utilized++;
		m_vc_load[t_flit->get_vc()]++;
	}
}

flit* NetworkLink::peekLink()
{
	return linkBuffer->peekTopFlit();
}

flit* NetworkLink::consumeLink()
{
	return linkBuffer->getTopFlit();
}
