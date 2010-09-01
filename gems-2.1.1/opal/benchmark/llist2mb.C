
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
/*
 * FileName:  llistmb
 * Synopsis:  
 *     Microbenchmark to test linked list traversal
 *     
 * Author:    cmauer
 * Version:   $Id$
 */

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include "timelib.h"

typedef struct list_element {
  int                  data;
  struct list_element *next;
} list_element_t;

/// size of linked list array
const  int LIST_LENGTH = 1024;

/// size of linked list element in bytes
const  int ELEMENT_SIZE = 256;

/// global array of linked list elements
list_element_t **g_array;
/// current element at the head of the linked list
list_element_t *g_cur_head;

//**************************************************************************
void insertList( int place )
{
  // printf("%d\n", place);
  if ( g_array[place]->data == 1 ) {
    printf("*** bad range: already selected list element %d\n", place);
  }
  g_cur_head->next = g_array[place];
  g_array[place]->data = 1;
  g_cur_head = g_cur_head->next;
}

//**************************************************************************
void binaryTree( int lowrange, int highrange, void (*callback)(int) )
{
  if ( lowrange == highrange - 1 ) {
    return;
  }
  int mid = (lowrange + highrange) / 2;
  // printf( "BT %d <-> %d\n", lowrange, highrange );

  (*callback)( mid );
  binaryTree( lowrange, mid, callback );
  binaryTree( mid, highrange, callback );
}

/*------------------------------------------------------------------------*/
/* Main function                                                          */
/*------------------------------------------------------------------------*/

//**************************************************************************
int main(int argc, char* argv[])
{
  if ( argc != 2 ) {
    printf("usage: %s number-of-trials\n", argv[0]);
    exit(1);
  }
  int     numTrials = atoi( argv[1] );
  /** preamble: setup this microbenchmark */

  list_element_t *head;
  list_element_t *element;

  g_array = (list_element_t **) malloc( sizeof(list_element_t *) * LIST_LENGTH );
  for (int i = 0; i < LIST_LENGTH; i++) {
    g_array[i]       = (list_element_t *) malloc( ELEMENT_SIZE );
    g_array[i]->data = 0;
  }

  head = g_array[0];
  g_cur_head = head;
  g_array[0]->data = 1;

  binaryTree( 0, LIST_LENGTH, insertList );
  
  /** begin trial */
  timelib timer;

  timer.start();
  for (int i = 0; i < numTrials; i++) {

    for (int j = 0; j < 100; j++) {
      element = head;
      while ( element != NULL ) {
        if (element->data == 2) {
          printf("extrodinary data element <> 0 or 1\n");
        }
        element = element->next;
      }
    }

  }
  timer.stop();
  /** end trial */

  /** postamble: free resources used by this microbenchmark */
  timer.report();
  return 0;
}
