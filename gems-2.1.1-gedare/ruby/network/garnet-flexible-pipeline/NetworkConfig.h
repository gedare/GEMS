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
 * NetworkConfig.h
 *
 * Description: This header file is used to define all configuration parameters required by the interconnection network. 
 * 		
 * Niket Agarwal, Princeton University
 * 
 * */		

#ifndef NETWORKCONFIG_H
#define NETWORKCONFIG_H

#include "NetworkHeader.h"
#include "util.h"
#include "RubyConfig.h"

class NetworkConfig {
	public:
		static bool isGarnetNetwork() {return g_GARNET_NETWORK; }
		static bool isDetailNetwork() {return g_DETAIL_NETWORK; }
		static int isNetworkTesting() {return g_NETWORK_TESTING; }
		static int getFlitSize() {return g_FLIT_SIZE; }
		static int getNumPipeStages() {return g_NUM_PIPE_STAGES; }
		static int getVCsPerClass() {return g_VCS_PER_CLASS; }
		static int getBufferSize() {return g_BUFFER_SIZE; }
  // This is no longer used. See config/rubyconfig.defaults to set Garnet parameters.
		static void readNetConfig()
		{
      /*
			string filename = "network/garnet-flexible-pipeline/";
			filename += NETCONFIG_DEFAULTS;

  		if (g_SIMICS) {
    			filename = "../../../ruby/"+filename;
  		}
			ifstream NetconfigFile( filename.c_str(), ios::in);
			if(!NetconfigFile.is_open())
			{	
				cout << filename << endl;
				cerr << "Network Configuration file cannot be opened\n";
				exit(1);
			}

			string line = "";
      
			while(!NetconfigFile.eof())
			{
				getline(NetconfigFile, line, '\n');
				string var = string_split(line, ':');
	
				if(!var.compare("g_GARNET_NETWORK"))
				{
					if(!line.compare("true"))
						g_GARNET_NETWORK = true;
					else 
						g_GARNET_NETWORK = false;
				}	
				if(!var.compare("g_DETAIL_NETWORK"))
				{
					if(!line.compare("true"))
						g_DETAIL_NETWORK = true;
					else 
						g_DETAIL_NETWORK = false;
				}	
				if(!var.compare("g_NETWORK_TESTING"))
				{
					if(!line.compare("true"))
						g_NETWORK_TESTING = true;
					else 
						g_NETWORK_TESTING = false;
				}	
				if(!var.compare("g_FLIT_SIZE"))
					g_FLIT_SIZE = atoi(line.c_str());
				if(!var.compare("g_NUM_PIPE_STAGES"))
					g_NUM_PIPE_STAGES = atoi(line.c_str());
				if(!var.compare("g_VCS_PER_CLASS"))
					g_VCS_PER_CLASS = atoi(line.c_str());
				if(!var.compare("g_BUFFER_SIZE"))
					g_BUFFER_SIZE = atoi(line.c_str());
			}
			NetconfigFile.close();
      */
      /*
      cout << "g_GARNET_NETWORK = " << g_GARNET_NETWORK << endl;
      cout << "g_DETAIL_NETWORK = " << g_DETAIL_NETWORK << endl;
      cout << "g_NETWORK_TESTING = " << g_NETWORK_TESTING << endl;
      cout << "g_FLIT_SIZE = " << g_FLIT_SIZE << endl;
      cout << "g_NUM_PIPE_STAGES = " << g_NUM_PIPE_STAGES << endl;
      cout << "g_VCS_PER_CLASS= " << g_VCS_PER_CLASS << endl;
      cout << "g_BUFFER_SIZE = " << g_BUFFER_SIZE << endl;
      */
		}
};

#endif
