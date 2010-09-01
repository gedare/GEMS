
/*
    Copyright (C) 1999-2008 by Mark D. Hill and David A. Wood for the
    Wisconsin Multifacet Project.  Contact: gems@cs.wisc.edu
    http://www.cs.wisc.edu/gems/

    --------------------------------------------------------------------

    This file is part of the Opal Timing-First Microarchitectural
    Simulator, a component of the Multifacet GEMS (General 
    Execution-driven Multiprocessor Simulator) software toolset 
    originally developed at the University of Wisconsin-Madison.

    Opal was originally developed by Carl Mauer based upon code by
    Craig Zilles.

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

#include <stdlib.h>
#include "FrontEnd.h"
#include "global.h"

static char *g_pgm_name = NULL;
static char *g_usage_tester_string = "
";

static char *g_usage_ruby_string = "
";

FrontEnd::FrontEnd( int argc, char **argv )
{
 int m_num_opt = parseOptions( argc, argv );
}

void FrontEnd::usageError()
{
  if ( g_pgm_name == NULL )
    cerr << "program usage error" << endl;
  else if (!strcmp(g_pgm_name, "ruby.exec"))
    cerr << g_usage_tester_string << endl;
  else
    cerr << g_usage_ruby_string << endl;
  exit(1);
}

int FrontEnd::parseOptions( int argc, char **argv )
{
  g_pgm_name = argv[0];

  int c;
  int numopt = 0;

  /* Parse command line options.  */
  while ((c = getopt(argc, argv, "a:cds:t:v")) != EOF)
    {
      switch (c)
        {
        case 0:
          break;

        case 'a':
          g_array_size = (int) strtol( optarg, NULL, 0 );
          numopt += 2;
          break;

        case 'c':
          g_continual = true;
          numopt++;
          break;

        case 'd':
          numopt += 1;
          break;

        case 's':
          g_rand_seed = (int) strtol( optarg, NULL, 0 );
          numopt += 2;
          break;

        case 't':
          g_num_trials = (int) strtol( optarg, NULL, 0 );
          numopt += 2;
          break;

        case 'v':
          g_verbose = (int) strtol( optarg, NULL, 0 );
          numopt += 1;
          break;
        }
    }
  return (numopt);
}

void FrontEnd::print(ostream& out) const
{
  out << "[ FrontEnd ]" << endl;
}

