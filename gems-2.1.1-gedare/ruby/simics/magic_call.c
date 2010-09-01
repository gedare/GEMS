
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
 * magic_call.c
 * 
 * Description: Call Ruby magic services from within the SimICS. This
 *              is the Ruby/SimICS equivalent of the rotary/cell phones in the Matrix
 *
 * $Id$
 *
 * $Log$
 * Revision 1.1.1.1  2000/03/09 10:18:40  milo
 * Initial import
 *
 * Revision 2.0  2000/01/19 07:21:12  milo
 * Version 2.0
 *
 * Revision 1.1  1999/10/28 07:41:32  plakal
 * Added a magic_call client which can make one of 11 magic service calls
 * (defined in magic_services.def and magic_services.h). In the process of
 * adding handlers for these services in the Ruby module.
 *
 *
 * 
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "magic_services.h"

static int num_services;

void Usage( void )
{
  int i;
  
  printf( "\nUsage: magic_call <service-name> [<parameter>]\n" );
  printf( " where service-name = " );
  for( i = 0; i < num_services; i++ )
  {
    printf( "%s%s", ((i>0) ? ", " : ""), magic_service_mnemonics[i].mnemonic );
  }
  printf( "\n" );
  printf( " and parameter is an unsigned 32-bit integer (default zero)\n" );
  printf( "\n" );
}


int main(int argc, char* argv[] )
{
  int service;
  register unsigned int parameter asm (RUBY_MAGIC_REGISTER_NAME);
  int i;
  
  num_services = sizeof( magic_service_mnemonics ) / sizeof( Ruby_Magic_Service_Entry ) - 1;

  /* -- Check usage */
  if( (argc < 2) || (argc > 3) )
  {
    Usage();
    exit( 1 );
  }

  /* -- Figure out which service to call */
  service = -1;
  for( i = 0; i < num_services; i++ )
  {
    if( strcasecmp( argv[1], magic_service_mnemonics[i].mnemonic ) == 0 )
    {
      /* -- Bingo! */
      service = magic_service_mnemonics[i].service;
      break;
    }
  }
  if( service == -1 )
  {
    Usage();
    exit( 2 );
  }
  
  /* -- Get parameter */
  parameter = 0;
  if( argc == 3 )
  {
    unsigned long param;
    
    param = atoi( argv[2] );
    if( param < 0 )
    {
      Usage();
      exit( 3 );
    }
    parameter = (unsigned int) param;
  }
  
  /* -- Make magic service call */
  printf( 
          "\n *** Making magic call to service %s with parameter %d ***\n",
          argv[1],
          parameter
        );

  switch( service )
  {

#define SERVICE( service_num, mnemonic )        \
    case (service_num):                         \
      RUBY_MAGIC_CALL( service_num );           \
      break;

#include "magic_services.def"      
#undef SERVICE

    default:
      printf( " *** Error! This should never get executed!!\n" );
      break;
  }
  
  printf( " *** Returned from magic call ***\n\n" );
  
  return 0;
}
