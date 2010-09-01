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
#ifndef _HFA_INIT_H_
#define _HFA_INIT_H_

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Interface between opal loadable module and simulator.
 * @version $Id: hfa_init.h,v 3.13 2003/12/06 22:08:05 xu Exp $
 */

/* External interfaces to system.C extension module */

/** print an automatically generated list of parameters to file 'fp' */
extern void hfa_list_param( FILE *fp );

/* External interfaces to opal.C extension module */

/** Check for any error conditions */
extern void hfa_checkerr( const char *location );

/** Check that simics will work with this version of opal */
extern void hfa_simcheck( void );

/** 'constructor' for simics: allocates memory for opal */
extern void hfa_allocate( void );

/** 'destructor' for simics: frees opal's memory */
extern void hfa_deallocate( void );

/** initialize the global variable handler */
extern void hfa_construct_initvar( void );

/** get and set opal parameters */
attr_value_t hfa_dispatch_get( void *id, conf_object_t *obj,
                               attr_value_t *idx );
set_error_t hfa_dispatch_set( void *id, conf_object_t *obj, 
                              attr_value_t *val, attr_value_t *idx );

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* _HFA_INIT_H_ */
