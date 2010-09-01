
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
 * TourmalineConfig.C
 *
 * Description: 
 * See TourmalineConfig.h
 * 
 * $Id: TourmalineConfig.C 1.00 05/11/09 14:02:03-05:00 gibson@mustardseed.cs.wisc.edu $
 *
 */

#include "TourmalineConfig.h"
#include "interface.h"

#define CHECK_ZERO(N) { if (N != 0) { ERROR_MSG(#N " must be zero at initialization."); }}
#define CHECK_NON_ZERO(N) { if (N == 0) { ERROR_MSG(#N " must be non-zero."); }}

void TourmalineConfig::init()
{
  // Randomize the execution
  if( g_RANDOM_SEED == 0) {
    srandom(time(NULL));
  } else {
    srandom(g_RANDOM_SEED);
  }

  // Figure out the number of processors
  g_NUMBER_OF_PROCESSORS = SIMICS_number_processors();
  CHECK_NON_ZERO(g_NUMBER_OF_PROCESSORS);
  
  // Check that the controller string is valid
  if( TourmalineConfig::controllerType() == Transaction_Controller_COUNT ) {
    ERROR(" Unrecognized transaction controller: [%s]\n", TourmalineConfig::controllerString() );
    cout << "Possible candidates are:" << endl;
    TransactionControllerType type = Transaction_Controller_Serializer;
    for(type; type < Transaction_Controller_COUNT-1; type++ ) {
      cout << TransactionControllerNames[type] << endl;
    }
    
    assert( TourmalineConfig::controllerType() != Transaction_Controller_COUNT );
  }
}

void TourmalineConfig::print_parameters(ostream& out)
{

#define PARAM(NAME) { out << #NAME << ": " << NAME << endl; }
#define PARAM_UINT(NAME) { out << #NAME << ": " << NAME << endl; }
#define PARAM_ULONG(NAME) { out << #NAME << ": " << NAME << endl; }
#define PARAM_BOOL(NAME) { out << #NAME << ": " << bool_to_string(NAME) << endl; }
#define PARAM_DOUBLE(NAME) { out << #NAME << ": " << NAME << endl; }
#define PARAM_STRING(NAME) { assert(NAME != NULL); out << #NAME << ": " << string(NAME) << endl; }
#define PARAM_ARRAY(PTYPE, NAME, ARRAY_SIZE)   \
  {                                            \
    out << #NAME << ": (";                     \
    for (int i = 0; i < ARRAY_SIZE; i++) {     \
      if (i != 0) {                            \
        out << ", ";                           \
      }                                        \
      out << NAME[i];                          \
    }                                          \
    out << ")" << endl;                        \
  }                                            \

    
#include CONFIG_VAR_FILENAME
#undef PARAM
#undef PARAM_UINT
#undef PARAM_ULONG
#undef PARAM_BOOL
#undef PARAM_DOUBLE
#undef PARAM_STRING
#undef PARAM_ARRAY
}

void TourmalineConfig::printConfiguration(ostream& out) {
  out << "Tourmaline Configuration" << endl;
  out << "------------------" << endl;

  SIMICS_print_version(out);
  out << "compiled_at: " << __TIME__ << ", " << __DATE__ << endl;

  char buffer[100];
  gethostname(buffer, 50);
  out << "hostname: " << buffer << endl;

  print_parameters(out);
}

TransactionControllerType TourmalineConfig::controllerType() {
  TransactionControllerType type = Transaction_Controller_Serializer;
  for(type; type < Transaction_Controller_COUNT; type++ ) {
    if( strcmp( TransactionControllerNames[type] ,
                TourmalineConfig::controllerString()) == 0 ) {
      return type;
    }
  }

  return Transaction_Controller_COUNT;
}

