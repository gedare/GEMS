
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
 * FileName:  printf.c
 * Synopsis:  
 *     Microbenchmark to test the number of instructions in output
 *     to the screen.
 * Author:    cmauer
 * Version:   $Id$
 */

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include "timelib.h"

/*------------------------------------------------------------------------*/
/* Main function                                                          */
/*------------------------------------------------------------------------*/

/** 64   lines, 496 words, and 3295 characters. */
const char *global_lyric[] = {

"GENERAL: ", 
"      I am the very model of a modern Major-General, ", 
"      I've information vegetable, animal, and mineral, ", 
"      I know the kings of England, and I quote the fights historical ", 
"      From Marathon to Waterloo, in order categorical; ", 
"      I'm very well acquainted, too, with matters mathematical, ", 
"      I understand equations, both the simple and quadratical, ", 
"      About binomial theorem I'm teeming with a lot o' news, ", 
"      With many cheerful facts about the square of the hypotenuse.", 
"ALL: ", 
"      With many cheerful facts about the square of the hypotenuse. ", 
"      With many cheerful facts about the square of the hypotenuse. ", 
"      With many cheerful facts about the square of the hypotenuse.", 
"GENERAL: ", 
"      I'm very good at integral and differential calculus; ", 
"      I know the scientific names of beings animalculous: ", 
"      In short, in matters vegetable, animal, and mineral, ", 
"      I am the very model of a modern Major-General.", 
"ALL: ", 
"      In short, in matters vegetable, animal, and mineral, ", 
"      He is the very model of a modern Major-General.", 
"GENERAL: ", 
"      I know our mythic history, King Arthur's and Sir Caradoc's; ", 
"      I answer hard acrostics, I've a pretty taste for paradox, ", 
"      I quote in elegiacs all the crimes of Heliogabalus, ", 
"      In conics I can floor peculiarities parabolous;", 
"", 
"      I can tell undoubted Raphaels from Gerard Dows and Zoffanies, ", 
"      I know the croaking chorus from the Frogs of Aristophanes! ", 
"      Then I can hum a fugue of which I've heard the music's din afore, ", 
"      And whistle all the airs from that infernal nonsense Pinafore.", 
"ALL: ", 
"      And whistle all the airs from that infernal nonsense Pinafore. ", 
"      And whistle all the airs from that infernal nonsense Pinafore. ", 
"      And whistle all the airs from that infernal nonsense Pinafore.", 
"GENERAL: ", 
"      Then I can write a washing bill in Babylonic cuneiform, ", 
"      And tell you ev'ry detail of Caractacus's uniform: ", 
"      In short, in matters vegetable, animal, and mineral, ", 
"      I am the very model of a modern Major-General.", 
"ALL: ", 
"      In short, in matters vegetable, animal, and mineral, ", 
"      He is the very model of a modern Major-General.", 
"GENERAL: ", 
"      In fact, when I know what is meant by \"mamelon\" and \"ravelin\", ", 
"      When I can tell at sight a Mauser rifle from a javelin, ", 
"      When such affairs as sorties and surprises I'm more wary at, ", 
"      And when I know precisely what is meant by \"commissariat\", ", 
"      When I have learnt what progress has been made in modern gunnery, ", 
"      When I know more of tactics than a novice in a nunnery-- ", 
"      In short, when I've a smattering of elemental strategy, ", 
"      You'll say a better Major-General has never sat a gee.", 
"ALL: ", 
"      You'll say a better Major-General has never sat a gee. ", 
"      You'll say a better Major-General has never sat a gee. ", 
"      You'll say a better Major-General has never sat a gee.", 
"GENERAL: ", 
"      For my military knowledge, though I'm plucky and adventury, ", 
"      Has only been brought down to the beginning of the century; ", 
"      But still, in matters vegetable, animal, and mineral, ", 
"      I am the very model of a modern Major-General.", 
"ALL: ", 
"      But still, in matters vegetable, animal, and mineral, ", 
"      He is the very model of a modern Major-General."
};

int main(int argc, char* argv[])
{

  if ( argc != 2 ) {
    printf("usage: %s number-of-trials\n", argv[0]);
    exit(1);
  }

  int     numTrials = atoi( argv[1] );
  timelib timer;
  
  timer.start();
  for (int i = 0; i < numTrials; i++) {

    for (int j = 0; j < 64; j++) {
      printf( "%s\n", global_lyric[j]);
    }
    
  }
  timer.stop();

  timer.report();
  return 0;
}
