
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
 * Topology.h
 *
 * Description: The topology here is configurable; it can be a hierachical
 *              (default one) or a 2D torus or a 2D torus with half switches
 *              killed. I think all input port has a
 *              one-input-one-output switch connected just to control and
 *              bandwidth, since we don't control bandwidth on input ports.
 *              Basically, the class has a vector of nodes and edges. First
 *              2*m_nodes elements in the node vector are input and output
 *              ports. Edges are represented in two vectors of src and dest
 *              nodes. All edges have latency.
 *
 * $Id$
 *
 * */

#ifndef TOPOLOGY_H
#define TOPOLOGY_H

#include "Global.h"
#include "Vector.h"
#include "NodeID.h"

class Network;
class NetDest;

typedef Vector < Vector <int> > Matrix;

class Topology {
public:
  // Constructors
  Topology(Network* network_ptr, int number_of_nodes);

  // Destructor
  ~Topology() {}

  // Public Methods
  int numSwitches() const { return m_number_of_switches; }
  void createLinks(bool isReconfiguration);

  void printStats(ostream& out) const {}
  void clearStats() {}
  void printConfig(ostream& out) const;
  void print(ostream& out) const { out << "[Topology]"; }

private:
  // Private Methods
  void init();
  SwitchID newSwitchID();
  void addLink(SwitchID src, SwitchID dest, int link_latency);
  void addLink(SwitchID src, SwitchID dest, int link_latency, int bw_multiplier);
  void addLink(SwitchID src, SwitchID dest, int link_latency, int bw_multiplier, int link_weight);
  void makeLink(SwitchID src, SwitchID dest, const NetDest& routing_table_entry, int link_latency, int weight, int bw_multiplier, bool isReconfiguration);

  void makeHierarchicalSwitch(int fan_out_degree);
  void make2DTorus();
  void makePtToPt();
  void makeFileSpecified();

  void makeSwitchesPerChip(Vector< Vector < SwitchID > > &nodePairs, Vector<int> &latencies, Vector<int> &bw_multis, int numberOfChips);

  string getDesignStr();
  // Private copy constructor and assignment operator
  Topology(const Topology& obj);
  Topology& operator=(const Topology& obj);

  // Data Members (m_ prefix)
  Network* m_network_ptr;
  NodeID m_nodes;
  int m_number_of_switches;

  Vector<SwitchID> m_links_src_vector;
  Vector<SwitchID> m_links_dest_vector;
  Vector<int> m_links_latency_vector;
  Vector<int> m_links_weight_vector;
  Vector<int> m_bw_multiplier_vector;

  Matrix m_component_latencies;
  Matrix m_component_inter_switches;
};

// Output operator declaration
ostream& operator<<(ostream& out, const Topology& obj);

// ******************* Definitions *******************

// Output operator definition
extern inline
ostream& operator<<(ostream& out, const Topology& obj)
{
  obj.print(out);
  out << flush;
  return out;
}

#endif
