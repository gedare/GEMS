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
 * FileName:  urwlock.C
 * Synopsis:  Implementation of a reader / writer lock
 * Author:    cmauer
 * Version:   $Id: urwlock.C 1.2 03/07/10 18:07:45-00:00 milo@cottons.cs.wisc.edu $
 */

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include "hfa.h"
#include "urwlock.h"

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

#define DEBUG_MSG DEBUG_OUT
#define OK                       0
#define UNIXERR                 -1
#define URWLOCK_INTERNAL_ERROR  -2
#define URWLOCK_INVALID_UPGRADE -3

/*------------------------------------------------------------------------*/
/* Variable declarations                                                  */
/*------------------------------------------------------------------------*/

/// registry for urwlocks locks: unique count, map from id -> lock
int                   urwlock_t::count = 0;
map<int, urwlock_t *> *urwlock_t::registry = NULL;
/// mutex for modifying the urwlock registry :)
pthread_mutex_t      *urwlock_t::reg_mutex = NULL;

/*------------------------------------------------------------------------*/
/* Forward declarations                                                   */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Constructor(s) / destructor                                            */
/*------------------------------------------------------------------------*/

//**************************************************************************
urwlock_t::urwlock_t() {
  m_status = OK;
  m_reader_count = 0;
  m_writer_count = 0;
  m_writer_waiting = false;
  m_name = NULL;

  m_status = m_mutex.getStatus();
  if (m_status != OK) {
    goto end;
  }
  
  if (pthread_cond_init(&m_lock_free_cv, NULL) != 0){
    m_status = UNIXERR;
    goto end;
  }
  
  m_id = urwlock_t::count++;
  (*urwlock_t::registry)[m_id] = this;

 end: {}
}

//**************************************************************************
urwlock_t::~urwlock_t(){
  if (m_name)
    delete []m_name;
  pthread_cond_destroy(&m_lock_free_cv);
}

//**************************************************************************
int32 urwlock_t::setName( const char *name )
{
  m_name  = new char[ strlen(name) + 1 ];
  strcpy( m_name, name );

  // +1 for NULL terminator, +6 for adding " mtx" to end
  char *namebuf = new char[ strlen(name) + 1 + 6 ]; 
  strcpy( namebuf, name );
  strcat( namebuf, " mtx" );

  m_mutex.setName( namebuf );
  delete [] namebuf;
  return (OK);
}

//**************************************************************************
int32 urwlock_t::getStatus(){
  return m_status;
}

//**************************************************************************
int32 urwlock_t::read_lock(){
  if (m_mutex.lock() != 0){
    m_mutex.unlock();
    return UNIXERR;
  }
  
  // if a writer holds the lock, or writers are waiting, wait until
  // they are done (give priority to writers)
  while (m_writer_waiting || m_writer_count > 0){
    DEBUG_MSG("URWLOCK reader \"%s\" waiting...", m_name);
    if (pthread_cond_wait(&m_lock_free_cv, m_mutex.getMutex()) != 0){
      m_mutex.unlock();
      return UNIXERR;
    }
    DEBUG_MSG("URWLOCK reader \"%s\" woke up", m_name);
  }
  m_reader_count++;

  if (m_mutex.unlock() != 0)
    return UNIXERR;
  
  return OK;
}

//**************************************************************************
int32 urwlock_t::read_unlock(){
  if (m_mutex.lock() != 0){
    m_mutex.unlock();
    return UNIXERR;
  }

  if (m_reader_count <= 0){
    m_mutex.unlock();
    return URWLOCK_INTERNAL_ERROR;
  }

  m_reader_count--;

  // if this is the last reader unlocking and there is a writer
  // waiting to get in, signal so that a writer can get in
  if (m_reader_count == 0 && m_writer_waiting){ 
    pthread_cond_signal(&m_lock_free_cv);
  }
  
  if (m_mutex.unlock() != 0)
    return UNIXERR;
  
  return OK;
}

//**************************************************************************
int32 urwlock_t::write_lock(){
  if (m_mutex.lock() != 0){
    m_mutex.unlock();
    return UNIXERR;
  }

  m_writer_waiting = true;

  while (m_writer_count > 0 || m_reader_count > 0){
    DEBUG_MSG("URWLOCK writer \"%s\" waiting...", m_name);
    if (pthread_cond_wait(&m_lock_free_cv, m_mutex.getMutex()) != 0) {
      m_mutex.unlock();
      return UNIXERR;
    }
    DEBUG_MSG("URWLOCK writer \"%s\" woke up", m_name);
  }

  m_writer_waiting = false;
  m_writer_count = 1;

  if (m_mutex.unlock() != 0)
    return UNIXERR;
  
  return OK;
}


//**************************************************************************
int32 urwlock_t::write_unlock(){
  if (m_mutex.lock() != 0){
    m_mutex.unlock();
    return UNIXERR;
  }

  if (m_writer_count <= 0 || m_reader_count != 0){
    m_mutex.unlock();
    return URWLOCK_INTERNAL_ERROR;
  }

  m_writer_count = 0;
  
  // broadcast to everyone who's waiting
  if (pthread_cond_broadcast(&m_lock_free_cv) != 0){
    m_mutex.unlock();
    return UNIXERR;
  }
  
  if (m_mutex.unlock() != 0)
    return UNIXERR;
  
  return OK;
} 

// WARNING: THIS FUNCTION HAS NOT YET BEEN FULLY TESTED - IT IS NOT
// CURRENTLY CONSIDERED SAFE TO USE!!!
//**************************************************************************
int32 urwlock_t::upgrade_read_lock(){
  if (m_mutex.lock() != 0){
    m_mutex.unlock();
    return UNIXERR;
  }

  if (m_reader_count < 1){
    m_mutex.unlock();
    return URWLOCK_INVALID_UPGRADE;
  }

  m_writer_waiting = true;
  m_reader_count--;

  while (m_writer_count > 0 || m_reader_count > 0){
    
    if (pthread_cond_wait(&m_lock_free_cv, m_mutex.getMutex()) != 0){
      m_mutex.unlock();
      return UNIXERR;
    }
    
  }

  m_writer_waiting = false;
  m_writer_count = 1;
  
  if (m_mutex.unlock() != 0)
    return UNIXERR;
  
  return OK;
}

// WARNING: THIS FUNCTION HAS NOT YET BEEN FULLY TESTED - IT IS NOT
// CURRENTLY CONSIDERED SAFE TO USE!!!
//**************************************************************************
int32 urwlock_t::downgrade_write_lock(){
  if (m_mutex.lock() != 0){
    m_mutex.unlock();
    return UNIXERR;
  }

  if (m_writer_count <= 0 || m_reader_count != 0){
    m_mutex.unlock();
    return URWLOCK_INTERNAL_ERROR;
  }
  
  m_writer_count = 0;
  m_reader_count = 1;

  // broadcast to everyone who's waiting
  if (pthread_cond_broadcast(&m_lock_free_cv) != 0){
    m_mutex.unlock();
    return UNIXERR;
  }
  
  
  if (m_mutex.unlock() != 0)
    return UNIXERR;
  
  return OK;
}

//**************************************************************************
void   urwlock_t::printStats(void) {
  map<int, urwlock_t *>::iterator iter;
  urwlock_t   *lock_p;
  
  cout << "Urwlock statistics" << endl;
  for (iter = urwlock_t::registry->begin(); iter != urwlock_t::registry->end();
       iter++) {
    lock_p = iter->second;
    cout << "Lock: #" << lock_p->m_id << " ";
    if (lock_p->m_name) {
      cout << lock_p->m_name << " ";
    } else {
      cout << "unamed ";
    }
    cout << " " << lock_p->m_reader_count << " " << lock_p->m_writer_count
         << " " << lock_p->m_writer_waiting << " " << lock_p->m_status << endl;
  }
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
void urwlock_init_local( void )
{
  urwlock_t::count = 0;
  urwlock_t::registry = new map<int, urwlock_t *>();
  urwlock_t::reg_mutex = new pthread_mutex_t();
  if ( pthread_mutex_init( urwlock_t::reg_mutex, NULL ) != 0 ) {
    ERROR_OUT( "URWLOCK init local fails: reg_mutex could not be created.");
  }
}
