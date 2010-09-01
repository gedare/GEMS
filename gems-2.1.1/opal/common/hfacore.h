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
#ifndef _HFACORE_H_
#define _HFACORE_H_

/* 
 * Global include file for entire project.
 * Should be included first in all ".C" project files
 */

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

//  hfa utilities functions
#include "hfa_init.h"

//  includes which are independent of the system
#include "bitlib.h"
#include "finitecycle.h"
#include "debugio.h"
#include "listalloc.h"
#include "utimer.h"

//  first level includes: waiters, register handling, abstract pc
#include "wait.h"
#include "regfile.h"
#include "regmap.h"
#include "arf.h"
#include "regbox.h"
#include "abstractpc.h"
#include "checkresult.h"

//  second level: branch predictors
#include "directbp.h"
#include "indirect.h"
#include "ras.h"
#include "tlstack.h"

/** universal predictor state: saves a copy of historical values
 *  so sequencers can roll back on mispredicts.
 **/
typedef struct _predictor_state_t {
  cond_state_t     cond_state;
  indirect_state_t indirect_state;
  ras_state_t      ras_state;
} predictor_state_t;

//  dependency between memop->pstate for is_cacheable function
#include "pstate.h"

//  third level includes: instructions & sub-classes
#include "opcode.h"
#include "statici.h"
#include "dynamic.h"
#include "controlop.h"
#include "memop.h"

//  fourth level includes: other core u-architectural features
#include "decode.h"
#include "iwindow.h"
#include "scheduler.h"
#include "cache.h"
#include "pseq.h"
#include "system.h"
#include "power.h"
#include "storeSet.h"
#include "regstate.h"

#endif  /* _HFACORE_H_ */
