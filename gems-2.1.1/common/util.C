/*
    Copyright (C) 1999-2005 by Mark D. Hill and David A. Wood for the
    Wisconsin Multifacet Project.  Contact: gems@cs.wisc.edu
    http://www.cs.wisc.edu/gems/

    --------------------------------------------------------------------

    This file a component of the Multifacet GEMS (General 
    Execution-driven Multiprocessor Simulator) software toolset 
    originally developed at the University of Wisconsin-Madison.

    Ruby was originally developed primarily by Milo Martin and Daniel
    Sorin with contributions from Ross Dickson, Carl Mauer, and Manoj
    Plakal.
 
    SLICC was originally developed by Milo Martin with substantial
    contributions from Daniel Sorin.

    Opal was originally developed by Carl Mauer based upon code by
    Craig Zilles.

    Substantial further development of Multifacet GEMS at the
    University of Wisconsin was performed by Alaa Alameldeen, Brad
    Beckmann, Jayaram Bobba, Ross Dickson, Dan Gibson, Pacia Harper,
    Derek Hower, Milo Martin, Michael Marty, Carl Mauer, Michelle Moravan,
    Kevin Moore, Manoj Plakal, Daniel Sorin, Haris Volos, Min Xu, and Luke Yen.

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
 * $Id$
 */

#include "assert.h"
#include "util.h"

// Split a string into a head and tail strings on the specified
// character.  Return the head and the string passed in is modified by
// removing the head, leaving just the tail.

string string_split(string& str, char split_character)
{
  string head = "";
  string tail = "";

  uint counter = 0;
  while(counter < str.size()) {
    if (str[counter] == split_character) {
      counter++;
      break;
    } else {
      head += str[counter];
    }
    counter++;
  }
  
  while(counter < str.size()) {
    tail += str[counter];
    counter++;
  }
  str = tail;
  return head;
}

string bool_to_string(bool value)
{
  if (value) {
    return "true";
  } else {
    return "false";
  }
}

string int_to_string(int n, bool zero_fill, int width)
{
  ostringstream sstr;
  if(zero_fill) {
    sstr << setw(width) << setfill('0') << n;
  } else {
    sstr << n;
  }
  string str = sstr.str();
  return str;
}

float string_to_float(string& str) 
{
  stringstream sstr(str);
  float ret;
  sstr >> ret;
  return ret;
}

// Log functions
int log_int(long long n)
{
  assert(n > 0);
  int counter = 0;
  while (n >= 2) {
    counter++;
    n = n>>(long long)(1);
  }
  return counter;
}

bool is_power_of_2(long long n)
{
  return (n == ((long long)(1) << log_int(n)));
}

