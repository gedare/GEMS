/*
    Copyright (C) 1999-2008 by Mark D. Hill and David A. Wood for the
    Wisconsin Multifacet Project.  Contact: gems@cs.wisc.edu
    http://www.cs.wisc.edu/gems/

    --------------------------------------------------------------------

    This file is part of Orion (Princeton's interconnect power model),
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
#include "power_static.h"

#if (PARM_TECH_POINT == 18)
double NMOS_TAB[1] = {20.5e-9};
double PMOS_TAB[1] = {9.2e-9};
double NAND2_TAB[4] = {6.4e-10, 20.4e-9, 12.6e-9, 18.4e-9};
double NOR2_TAB[4] ={40.9e-9, 8.32e-9, 9.2e-9, 2.3e-10};
#elif (PARM_TECH_POINT == 10)
double NMOS_TAB[1] = {22.7e-9};
double PMOS_TAB[1] = {18.0e-9};
double NAND2_TAB[4] = {1.2e-9, 22.6e-9, 11.4e-9, 35.9e-9};
double NOR2_TAB[4] ={45.1e-9, 11.5e-9, 17.9e-9, 1.8e-9};
#elif (PARM_TECH_POINT == 7)
double NMOS_TAB[1] = {118.1e-9};
double PMOS_TAB[1] = {135.2e-9};
double NAND2_TAB[4] = {19.7e-9, 115.3e-9, 83.0e-9, 267.6e-9};
double NOR2_TAB[4] ={232.4e-9, 79.6e-9, 127.9e-9, 12.3e-9};
#endif
