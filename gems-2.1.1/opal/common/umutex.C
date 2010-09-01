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
 * FileName:  mutex.C
 * Synopsis:  Encapsulates a mutex lock
 * Author:    cmauer
 * Version:   $Id: umutex.C 1.2 03/07/10 18:07:45-00:00 milo@cottons.cs.wisc.edu $
 */

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include "hfa.h"
#include "umutex.h"

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

#define OK         0
#define UNIXERR   -1

/*------------------------------------------------------------------------*/
/* Variable declarations                                                  */
/*------------------------------------------------------------------------*/

/// registry for mutex locks: unique count, map from id -> lock
int                   umutex_t::count = 0;
map<int, umutex_t *> *umutex_t::registry = NULL;
/// mutex for modifying the mutex registry :)
pthread_mutex_t      *umutex_t::reg_mutex = NULL;

/*------------------------------------------------------------------------*/
/* Forward declarations                                                   */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Constructor(s) / destructor                                            */
/*------------------------------------------------------------------------*/

//**************************************************************************
umutex_t::umutex_t( void )
{
  init();
}

//**************************************************************************
umutex_t::umutex_t( const char *name )
{
  init();
  setName( name );
}

//**************************************************************************
umutex_t::~umutex_t( void )
{
}

//**************************************************************************
int32 umutex_t::init( void )
{
  m_name = NULL;
  m_status = OK;

  if (pthread_mutex_init(&m_mutex, NULL) != 0){
    m_status = UNIXERR;
  }
  if ( pthread_mutex_lock( reg_mutex ) != 0 ) {
    ERROR_OUT( "Mutex init fails: reg_mutex not allocated" );
    m_status = UNIXERR;
  }
  m_id = umutex_t::count++;
  (*umutex_t::registry)[m_id] = this;
  if ( pthread_mutex_unlock( reg_mutex ) != 0 ) {
    ERROR_OUT( "Mutex init fails: reg_mutex could not unlock ");
    m_status = UNIXERR;
  }
  return (OK);
}

//**************************************************************************
int    umutex_t::destroy( void )
{
  if (m_name)
    free( m_name );
  pthread_mutex_destroy(&m_mutex);  
  if ( pthread_mutex_lock( reg_mutex ) != 0 ) {
    ERROR_OUT( "Mutex init fails: reg_mutex not allocated" );
    m_status = UNIXERR;
  }
  umutex_t::registry->erase( m_id );
  if ( pthread_mutex_unlock( reg_mutex ) != 0 ) {
    ERROR_OUT( "Mutex init fails: reg_mutex could not unlock ");
    m_status = UNIXERR;
  }
  return (true);
}

//**************************************************************************
void umutex_t::setName( const char *name )
{
  m_name = (char *) malloc( (strlen(name) + 1)*sizeof(char) );
  strcpy( m_name, name );
}

//**************************************************************************
pthread_mutex_t *umutex_t::getMutex( void )
{
  return (&m_mutex);
}

//**************************************************************************
int32  umutex_t::getStatus( void )
{
  return m_status;
}

//**************************************************************************
int    umutex_t::unlock( void )
{
  int lockStatus = pthread_mutex_unlock(&m_mutex);
  if (lockStatus != 0) {
    ERROR_OUT( "mutex error: %d\n", lockStatus );
  }
  return (0);
}

//**************************************************************************
int    umutex_t::lock( void )
{
  int lockStatus = pthread_mutex_lock(&m_mutex);
  if (lockStatus != 0) {
    ERROR_OUT( "mutex error: %d\n", lockStatus );
    pthread_mutex_unlock(&m_mutex);
    return (lockStatus);
  }

  return (0);
}

//**************************************************************************
void   umutex_t::printStats(void)
{
  map<int, umutex_t *>::iterator iter;
  umutex_t   *lock_p;

  cout << endl;
  cout << "Mutex statistics" << endl;
  if ( pthread_mutex_lock( reg_mutex ) != 0 ) {
    ERROR_OUT( "error: could not lock registry mutex" );
  }
  for (iter = umutex_t::registry->begin(); iter != umutex_t::registry->end();
       iter++) {
    lock_p = iter->second;
    if (lock_p != NULL) {
      cout << "Lock: #" << lock_p->m_id << " ";
      if (lock_p->m_name) {
        cout << lock_p->m_name << " ";
      } else {
        cout << "unamed ";
      }
      int trylock = pthread_mutex_trylock( &lock_p->m_mutex );
      if (trylock == 0) {
        cout << "unlocked";
        pthread_mutex_unlock( &lock_p->m_mutex );
      } else {
        cout << "locked";
      }
      cout << endl;      
    } else {
      cout << "deleted mutex" << endl;
    }
  }
  pthread_mutex_unlock( reg_mutex );
}



/*------------------------------------------------------------------------*/
/* Public methods                                                         */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Accessor(s) / mutator(s)                                               */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Private methods                                                        */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Static methods                                                         */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Global functions                                                       */
/*------------------------------------------------------------------------*/

//**************************************************************************
void umutex_init_local( void )
{
  umutex_t::count = 0;
  umutex_t::registry = new map<int, umutex_t *>();
  umutex_t::reg_mutex = new pthread_mutex_t();
  if ( pthread_mutex_init( umutex_t::reg_mutex, NULL ) != 0 ) {
    ERROR_OUT( "Mutex init local fails: reg_mutex could not be created.");
  }
}
