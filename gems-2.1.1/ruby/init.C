
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
 * init.C
 * 
 * Description: See init.h
 *
 * $Id$
 *
 */

#include "Global.h"
#include "EventQueue.h"
#include "System.h"
#include "Debug.h"
#include "Profiler.h"
#include "Tester.h"
#include "OpalInterface.h"
#include "init.h"
#include "interface.h"

#ifdef CONTIGUOUS_ADDRESSES
#include "ContiguousAddressTranslator.h"

/* Declared in interface.C */
extern ContiguousAddressTranslator * g_p_ca_translator;

#endif // #ifdef CONTIGUOUS_ADDRESSES

using namespace std;
#include <string>
#include <map>
#include <stdlib.h>

extern "C" {
#include "simics/api.h"
};

#include "confio.h"
#include "initvar.h"

// A generated file containing the default parameters in string form
// The defaults are stored in the variable global_default_param
#include "default_param.h"

// get and set interfaces, defined in module/ruby.c
extern "C" attr_value_t ruby_session_get( void *id, conf_object_t *obj,
                                          attr_value_t *idx );
extern "C" set_error_t ruby_session_set( void *id, conf_object_t *obj, 
                                         attr_value_t *val, attr_value_t *idx );
static  initvar_t  *ruby_initvar_obj = NULL;

//***************************************************************************
static void init_generate_values( void )
{
  /* update generated values, based on input configuration */
}

//***************************************************************************
void init_variables( void )
{
  // allocate the "variable initialization" package
  ruby_initvar_obj = new initvar_t( "ruby", "../../../ruby/",
                                    global_default_param,
                                    &init_simulator,
                                    &init_generate_values,
                                    &ruby_session_get,
                                    &ruby_session_set );
}

void init_simulator()
{
  // Set things to NULL to make sure we don't de-reference them
  // without a seg. fault.
  g_system_ptr = NULL;
  g_debug_ptr = NULL;
  g_eventQueue_ptr = NULL;
  
  cout << "Ruby Timing Mode" << endl;
#ifndef MULTIFACET_NO_OPT_WARN
  cerr << "Warning: optimizations not enabled." << endl;
#endif

  if (g_SIMICS) {
    // LUKE - if we don't set the default SMT threads in condor scripts,
    //        set it now
    if(g_NUM_SMT_THREADS == 0){
      g_NUM_SMT_THREADS = 1;
    }
   if(g_NUM_PROCESSORS == 0){   
     //only set to default if value not set in condor scripts  
     //  Account for SMT systems also
      g_NUM_PROCESSORS = SIMICS_number_processors()/g_NUM_SMT_THREADS;
    } 
  }

  RubyConfig::init();

  g_debug_ptr = new Debug( DEBUG_FILTER_STRING,
                           DEBUG_VERBOSITY_STRING,
                           DEBUG_START_TIME,
                           DEBUG_OUTPUT_FILENAME );

  cout << "Creating event queue..." << endl;
  g_eventQueue_ptr = new EventQueue;
  cout << "Creating event queue done" << endl;

  cout << "Creating system..." << endl;
  cout << "  Processors: " << RubyConfig::numberOfProcessors() << endl;

  g_system_ptr = new System;
  cout << "Creating system done" << endl;

  // if opal is loaded, its static interface object (inst) will be non-null,
  // and the opal object needs to be notified that ruby is now loaded.
  // "1" indicates a load and should be replaced with an enumerated type.
  if (OpalInterface::inst != NULL) {
    OpalInterface::inst->notify( 1 );
  }

#ifdef CONTIGUOUS_ADDRESSES
  if(g_SIMICS) {
    cout << "Establishing Contiguous Address Space Mappings..." << flush;
    g_p_ca_translator = new ContiguousAddressTranslator();
    assert(g_p_ca_translator!=NULL);
    if(g_p_ca_translator->AddressesAreContiguous()) {
      cout << "Physical Memory Addresses are already contiguous." << endl;
      delete g_p_ca_translator;
      g_p_ca_translator = NULL;
    } else {
      cout << "Done." << endl;
    }
  } else {
    g_p_ca_translator = NULL;
  }
#endif // #ifdef CONTIGUOUS_ADDRESSES

  cout << "Ruby initialization complete" << endl;
}

void init_opal_interface( mf_ruby_api_t *api )
{
  OpalInterface::installInterface( api );
}

int init_use_snoop()
{
  if (g_SIMICS) {
    // The "snoop interface" defined by simics allows ruby to see store
    // data values (from simics). If DATA_BLOCK is defined, we are tracking
    // data, so we need to install the snoop interface.
    return ((DATA_BLOCK == true) || (XACT_MEMORY));
  } else {
    return (0);
  }
}

void destroy_simulator()
{
  cout << "Deleting system..." << endl;
  delete g_system_ptr;
  cout << "Deleting system done" << endl;

  cout << "Deleting event queue..." << endl;
  delete g_eventQueue_ptr;
  cout << "Deleting event queue done" << endl;

  delete g_debug_ptr;
}
