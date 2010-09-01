/*
    Copyright (C) 1999-2008 by Mark D. Hill and David A. Wood for the
    Wisconsin Multifacet Project.  Contact: gems@cs.wisc.edu
    http://www.cs.wisc.edu/gems/

    --------------------------------------------------------------------

    This file is part of the Tourmaline Transactional Memory Acclerator, 
    a component of the Multifacet GEMS (General Execution-driven 
    Multiprocessor Simulator) software toolset originally developed at 
    the University of Wisconsin-Madison.

    Tourmaline was originally developed primarily by Dan Gibson, but was
    based on work in the Ruby module performed by Milo Martin and Daniel
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
 * init.C
 *
 * Description: 
 * See init.h
 * 
 * $Id: init.C 1.00 05/11/09 14:02:03-05:00 gibson@mustardseed.cs.wisc.edu $
 *
 */

#include "Tourmaline_Global.h"
#include "TourmalineConfig.h"
#include "TransactionControl.h"
#include "Profiler.h"
#include "init.h"
#include "interface.h"

using namespace std;
#include <string>
#include <map>
#include <stdlib.h>

extern "C" {
#include "simics/api.h"
};

#include "confio.h"
#include "initvar.h"

// The parser (attrlex, attrparse) should generate this file,
// which contain the default values for TourmalineConfig parameters
#include "default_param.h"

// get and set interfaces, defined in module/tourmaline.c
extern "C" attr_value_t tourmaline_session_get( void *id, conf_object_t *obj,
                                               attr_value_t *idx );
extern "C" set_error_t tourmaline_session_set( void *id, conf_object_t *obj, 
                                               attr_value_t *val, attr_value_t *idx );

static  initvar_t  *tourmaline_initvar_obj = NULL;

//***************************************************************************
static void init_generate_values( void )
{
  /* update generated values, based on input configuration */
}

//***************************************************************************
void init_variables( void )
{
  tourmaline_initvar_obj = new initvar_t( "tourmaline","../../../tourmaline/",
                                          global_default_param,
                                          &init_simulator,
                                          &init_generate_values,
                                          &tourmaline_session_get,
                                          &tourmaline_session_set );
}

void init_simulator() 
{
  cout << "Initializing Tourmaline Module" << endl;
  cout << "------------------------------" << endl;
  
  cout << "Initializing TourmalineConfig... " << flush;
  TourmalineConfig::init();
  cout << "Done." << endl;
  
  cout << "Creating Transaction Controller: " << endl << flush;
  g_trans_control = allocateTransactionController();
  assert(g_trans_control!=NULL); 
  
  cout << "Creating Profiler... " << flush;
  g_profiler = new Profiler();
  assert(g_profiler!=NULL);
  cout << "Done." << endl << flush;
  
  cout << endl << "Tourmaline Module Initialized." << endl;
  TourmalineConfig::printConfiguration(cout);
  
  cout << flush;
}

void destroy_simulator() {

}


