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
   This file has been modified by Kevin Moore and Dan Nussbaum of the
   Scalable Systems Research Group at Sun Microsystems Laboratories
   (http://research.sun.com/scalable/) to support the Adaptive
   Transactional Memory Test Platform (ATMTP).

   Please send email to atmtp-interest@sun.com with feedback, questions, or
   to request future announcements about ATMTP.

   ----------------------------------------------------------------------

   File modification date: 2008-02-23

   ----------------------------------------------------------------------
*/

/*
 * FileName:  initvar.C
 * Synopsis:  implementation of global variable initialization in simics
 * Author:    cmauer
 * Version:   $Id$
 */

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

using namespace std;
#include <string>
#include <map>
#include <stdlib.h>

extern "C" {
#include "global.h"
#include "simics/api.h"
#ifdef SIMICS22X
#include "configuration_api.h"
#endif
#ifdef SIMICS30
#include "configuration.h"
#endif
};

#ifdef IS_OPAL
#include "hfatypes.h"
#include "debugio.h"
#endif

#ifdef IS_RUBY
#include "Global.h"
#endif

#ifdef IS_TOURMALINE
#include "Tourmaline_Global.h"
#endif

#include "confio.h"
#include "initvar.h"

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

#define  CONFIG_VAR_FILENAME "config.include"

/*------------------------------------------------------------------------*/
/* Variable declarations                                                  */
/*------------------------------------------------------------------------*/

// define global "constants" using centralized file
#define PARAM( NAME ) \
   int32  NAME;
#define PARAM_UINT( NAME ) \
   uint32 NAME;
#define PARAM_ULONG( NAME ) \
   uint64 NAME;
#define PARAM_BOOL( NAME ) \
   bool   NAME;
#define PARAM_DOUBLE( NAME ) \
   double NAME;
#define PARAM_STRING( NAME ) \
   char  *NAME;
#define PARAM_ARRAY( PTYPE, NAME, ARRAY_SIZE ) \
   PTYPE  NAME[ARRAY_SIZE];
#include CONFIG_VAR_FILENAME
#undef PARAM
#undef PARAM_UINT
#undef PARAM_ULONG
#undef PARAM_BOOL
#undef PARAM_DOUBLE
#undef PARAM_STRING
#undef PARAM_ARRAY

/** global initvar object */
initvar_t  *initvar_t::m_inst = NULL;

/*------------------------------------------------------------------------*/
/* Forward declarations                                                   */
/*------------------------------------------------------------------------*/

static attr_value_t initvar_get_attr( void *ptr, void *obj );
static set_error_t  initvar_set_attr( void *ptr, void *obj,
                                      attr_value_t *value );

/*------------------------------------------------------------------------*/
/* Constructor(s) / destructor                                            */
/*------------------------------------------------------------------------*/

//**************************************************************************
initvar_t::initvar_t( const char *name, const char *relativeIncludePath,
                      const char *initializingString,
                      void (*allocate_fn)(void),
                      void (*my_generate_fn)(void),
                      get_attr_t my_get_attr, set_attr_t my_set_attr )
{
  m_is_init           = false;
  m_name              = (char *) malloc( sizeof(char)*(strlen( name ) + 2) );
  m_rel_include_path  = (char *) malloc( sizeof(char)*(strlen( relativeIncludePath ) + 2) );
  m_config_filename   = NULL;
  strcpy( m_name, name );
  strcpy( m_rel_include_path, relativeIncludePath );
  m_allocate_f        = allocate_fn;
  m_generate_values_f = my_generate_fn;
  m_my_get_attr       = my_get_attr;
  m_my_set_attr       = my_set_attr;

  initvar_t::m_inst = this;
  init_config_reader( initializingString );
}

//**************************************************************************
initvar_t::~initvar_t( )
{
#define PARAM( NAME )
#define PARAM_UINT( NAME )
#define PARAM_ULONG( NAME )
#define PARAM_BOOL( NAME )
#define PARAM_DOUBLE( NAME )
#define PARAM_STRING( NAME ) \
   if (NAME != NULL) {       \
      free( NAME );          \
      NAME = NULL;           \
   }
#define PARAM_ARRAY( PTYPE, NAME, ARRAY_SIZE )
#include CONFIG_VAR_FILENAME
#undef PARAM
#undef PARAM_UINT
#undef PARAM_ULONG
#undef PARAM_BOOL
#undef PARAM_DOUBLE
#undef PARAM_STRING
#undef PARAM_ARRAY
  if (m_name) {
    free( m_name );
  }
  if (m_rel_include_path) {
    free( m_rel_include_path );
  }
  if (m_config_reader) {
    delete m_config_reader;
  }
  if (m_config_filename) {
    delete m_config_filename;
  }
}

//**************************************************************************
void initvar_t::init_config_reader( const char *initString )
{
  int         rc;
  const char *name;

  m_config_reader = new confio_t();
  m_config_reader->setVerbosity( false );

  // Initialize the config reader object to identify each parameter
#define PARAM_UINT   PARAM
#define PARAM_ULONG  PARAM
#define PARAM_BOOL   PARAM
#define PARAM_DOUBLE PARAM
#define PARAM( NAME )                                         \
  name = #NAME;                                               \
  rc = m_config_reader->register_attribute( name,             \
                                     initvar_get_attr, (void *) name,   \
                                     initvar_set_attr, (void *) name );
#define PARAM_STRING( NAME )                                  \
  NAME = NULL;                                                \
  name = #NAME;                                               \
  rc = m_config_reader->register_attribute( name,             \
                                     initvar_get_attr, (void *) name,   \
                                     initvar_set_attr, (void *) name );
#define PARAM_ARRAY( PTYPE, NAME, ARRAY_SIZE )                \
  name = #NAME;                                               \
  rc = m_config_reader->register_attribute( name,             \
                                     initvar_get_attr, (void *) name,   \
                                     initvar_set_attr, (void *) name );

#include CONFIG_VAR_FILENAME
#undef PARAM
#undef PARAM_UINT
#undef PARAM_ULONG
#undef PARAM_BOOL
#undef PARAM_DOUBLE
#undef PARAM_STRING
#undef PARAM_ARRAY

  // read the default configuration from the embedded text file
  rc = m_config_reader->readConfigurationString( initString );
  (*m_generate_values_f)();
}

/*------------------------------------------------------------------------*/
/* Public methods                                                         */
/*------------------------------------------------------------------------*/

//**************************************************************************
void initvar_t::allocate( void )
{
  if ( confirm_init() ) {
    DEBUG_OUT("error: %s initvar::allocate() called twice\n", m_name);
    return;
  }
  
  (*m_generate_values_f)();
  (*m_allocate_f)();
  m_is_init = true;
}

//**************************************************************************
void initvar_t::checkInitialization( void )
{
  m_config_reader->checkInitialization();
}

//**************************************************************************
attr_value_t initvar_t::dispatch_get( void *id, conf_object_t *obj,
                                      attr_value_t *idx )
{
  const char *command = (const char *) id;
  if ( !confirm_init() ) {
    DEBUG_OUT("error: %s is uninitialized. unable to get \'%s\'\n", m_name, command);
    DEBUG_OUT("     : you must initialize %s with a configuration file first.\n", m_name);
    DEBUG_OUT("     : use the command \'%s0.init\'\n", m_name);

    attr_value_t ret;
    ret.kind      = Sim_Val_Invalid;
    ret.u.integer = 0;
    return ret;
  }
  
  return ((*m_my_get_attr)(id, obj, idx));
}


//**************************************************************************
set_error_t initvar_t::dispatch_set( void *id, conf_object_t *obj, 
                                     attr_value_t *val, attr_value_t *idx )
{
  const char *command = (const char *) id;
  
  // DEBUG_OUT("set attribute: %s\n", command);
  if (!strcmp(command, "init")) {
    if (val->kind == Sim_Val_String) {
      if (!strcmp( val->u.string, "" )) {
        //       update generated values, then allocate
        allocate();
      } else {
        read_config( val->u.string );
        allocate();
      }
      return Sim_Set_Ok;
    } else {
      return Sim_Set_Need_String;
    }
  } else if (!strcmp(command, "readparam")) {
    if (val->kind == Sim_Val_String) {
      read_config( val->u.string );
      return Sim_Set_Ok;
    } else {
      return Sim_Set_Need_String;
    }
  } else if (!strcmp(command, "saveparam")) {
    if (val->kind == Sim_Val_String) {
      FILE *fp = fopen( val->u.string, "w" );
      if (fp == NULL) {
        ERROR_OUT("error: unable to open file: %s\n", val->u.string);
        return Sim_Set_Illegal_Value;
      }
      list_param( fp );
      if (fp != NULL) {
        fclose( fp );
      }
      return Sim_Set_Ok;
    } else {
      ERROR_OUT("error: saveparam given wrong type.\n");
      return Sim_Set_Illegal_Value;
    }
  } else if (!strcmp(command, "param")) {
    if (val->kind == Sim_Val_Integer) {
      list_param( stdout );
      return Sim_Set_Ok;
    } else if ( val->kind == Sim_Val_List &&
                val->u.list.size == 2 &&
                val->u.list.vector[0].kind == Sim_Val_String ) {
      return (set_param( val->u.list.vector[0].u.string,
                         &val->u.list.vector[1] ));
    } else {
      DEBUG_OUT("error: set parameter given wrong type.\n");
      return Sim_Set_Illegal_Value;
    }
  }
  
  if ( !confirm_init() ) {
    DEBUG_OUT("error: %s is uninitialized. unable to set \'%s\'\n", m_name, id);
    DEBUG_OUT("     : you must initialize %s with a configuration file first.\n", m_name);
    DEBUG_OUT("     : use the command \'%s0.init\'\n", m_name);
    return Sim_Set_Illegal_Value;
  }
  
  return (*m_my_set_attr)( id, obj, val, idx );
}

/*------------------------------------------------------------------------*/
/* Accessor(s) / mutator(s)                                               */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Private methods                                                        */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Static methods                                                         */
/*------------------------------------------------------------------------*/

//**************************************************************************
static attr_value_t initvar_get_attr( void *ptr, void *obj )
{
  const char *name = (const char *) ptr;
  attr_value_t  ret;
  memset( &ret, 0, sizeof(attr_value_t) );

#define PARAM_UINT   PARAM
#define PARAM_ULONG  PARAM
#define PARAM_BOOL   PARAM
#define PARAM( NAME )                       \
  if (!strcmp(name, #NAME)) {               \
    ret.kind = Sim_Val_Integer;             \
    ret.u.integer = NAME;                   \
    return (ret);                           \
  }
#define PARAM_DOUBLE( NAME )                \
  if (!strcmp(name, #NAME)) {               \
    ret.kind = Sim_Val_Floating;            \
    ret.u.floating = NAME;                  \
    return (ret);                           \
  }
#define PARAM_STRING( NAME )                \
  if (!strcmp(name, #NAME)) {               \
    ret.kind = Sim_Val_String;              \
    ret.u.string = NAME;                    \
    return (ret);                           \
  }
#define PARAM_ARRAY( PTYPE, NAME, ARRAY_SIZE )            \
  if (!strcmp(name, #NAME)) {                             \
    ret.kind = Sim_Val_List;                                  \
    ret.u.list.size = ARRAY_SIZE;                         \
    ret.u.list.vector = mallocAttribute( ARRAY_SIZE );    \
    for (int i = 0; i < ARRAY_SIZE; i++) {                \
      ret.u.list.vector[i].u.integer = NAME[i];           \
    }                                                     \
    return (ret);                                         \
  }

#include CONFIG_VAR_FILENAME
#undef PARAM
#undef PARAM_UINT
#undef PARAM_ULONG
#undef PARAM_BOOL
#undef PARAM_DOUBLE
#undef PARAM_STRING
#undef PARAM_ARRAY

  DEBUG_OUT("error: %s not found.\n", name);
  ret.kind = Sim_Val_Invalid;
  return (ret);
}

//***************************************************************************
static set_error_t initvar_set_attr( void *ptr, void *obj,
                                     attr_value_t *value )
{
  const char *name = (const char *) ptr;

#define PARAM_UINT   PARAM
#define PARAM_ULONG  PARAM
#define PARAM( NAME )                           \
  if (!strcmp(name, #NAME)) {                   \
    if ( value->kind != Sim_Val_Integer ) {     \
      ERROR_OUT("error: %s is not an integer\n", name );\
      return Sim_Set_Need_Integer;              \
    }                                           \
    NAME = value->u.integer;                    \
    return Sim_Set_Ok;                          \
  }
#define PARAM_BOOL( NAME )                     \
  if (!strcmp(name, #NAME)) {                  \
    if ( value->kind != Sim_Val_String ) {     \
      ERROR_OUT("error: %s is not an bool string\n", name );\
      return Sim_Set_Need_String;              \
    }                                          \
    if (!strcmp(value->u.string, "true")) {    \
      NAME = true;                             \
    } else if (!strcmp(value->u.string, "false")) { \
      NAME = false;                            \
    } else {                                   \
      ERROR_OUT("error: value %s for %s is not an bool string (set to false)\n", value->u.string, name );\
      NAME = false;                            \
    }                                          \
    return Sim_Set_Ok;                         \
  }
#define PARAM_DOUBLE( NAME )                     \
  if (!strcmp(name, #NAME)) {                    \
    if ( value->kind != Sim_Val_String ) {     \
      ERROR_OUT("error: %s is not a float\n", name );\
      return Sim_Set_Need_Floating;              \
    }                                            \
    NAME = atof( value->u.string );              \
    return Sim_Set_Ok;                           \
  }
#define PARAM_STRING( NAME )                    \
  if (!strcmp(name, #NAME)) {                   \
    if ( value->kind != Sim_Val_String ) {      \
      ERROR_OUT("error: %s is not an string\n", name ); \
      return Sim_Set_Need_String;               \
    }                                           \
    if (NAME != NULL) {                         \
      free( NAME );                             \
    }                                           \
    NAME = strdup( value->u.string );           \
    return Sim_Set_Ok;                          \
  }
#define PARAM_ARRAY( PTYPE, NAME, ARRAY_SIZE )            \
  if (!strcmp(name, #NAME)) {                             \
    if ( value->kind != Sim_Val_List ) {                      \
      ERROR_OUT("error: %s is not an list\n", name );     \
      return Sim_Set_Need_List;                               \
    }                                                     \
    if ( value->u.list.size != ARRAY_SIZE ) {             \
      ERROR_OUT("error: %s has %lld elements (should be %d)\n", name, value->u.list.size, ARRAY_SIZE); \
      return Sim_Set_Illegal_Value;                           \
    }                                                     \
    for (int i = 0; i < ARRAY_SIZE; i++) {                \
      NAME[i] = value->u.list.vector[i].u.integer;        \
    }                                                     \
    return Sim_Set_Ok;                                        \
  }

#include CONFIG_VAR_FILENAME
#undef PARAM
#undef PARAM_UINT
#undef PARAM_ULONG
#undef PARAM_BOOL
#undef PARAM_DOUBLE
#undef PARAM_STRING
#undef PARAM_ARRAY

  ERROR_OUT("error: %s not a parameter\n", name);
  return Sim_Set_Illegal_Value;
}

//***************************************************************************
void initvar_t::read_config( const char *parameterFile )
{
  DEBUG_OUT("read configuration: %s\n", parameterFile );
  m_config_filename = strdup( parameterFile );
  int rc = m_config_reader->readConfiguration( parameterFile,
                                               m_rel_include_path );
  if ( rc < 0 ) {
    ERROR_OUT("fatal error in read configuration: unable to continue.\n");
    exit(1);
  }
  // update generated values
  (*m_generate_values_f)();
}

/** sets one of the parameters */
//**************************************************************************
set_error_t initvar_t::set_param( const char *name, attr_value_t *value )
{
  
  // [dann 2007-04-04] ATMTP VV
  //
  // HACK ALERT: allow setting REMOVE_SINGLE_CYCLE_DCACHE_FAST_PATH,
  // PROFILE_EXCEPTIONS, PROFILE_XACT, ATMTP_DEBUG_LEVEL and
  // ATMTP_ENABLED after initialization.  This works is because ruby's
  // m_generate_values_f() does nothing -- more particularly, nothing
  // that depends on any of these parameters is pre-calculated
  // anywhere.
  //
  if (strcmp(name, "REMOVE_SINGLE_CYCLE_DCACHE_FAST_PATH") != 0 &&
      strcmp(name, "PROFILE_EXCEPTIONS") != 0 &&
      strcmp(name, "PROFILE_XACT") != 0 &&
      strcmp(name, "ATMTP_DEBUG_LEVEL") != 0 &&
      strcmp(name, "ATMTP_ENABLED") != 0) {
    //
    // [dann 2007-04-04] ATMTP ^^
    if ( confirm_init() ) {
      DEBUG_OUT("error: %s is already initialized.\n", m_name);
      DEBUG_OUT("     : setting parameters after initialization is unsupported\n");
      return (Sim_Set_Illegal_Value);
    }
    // [dann 2007-04-04] ATMTP VV
    //
  }
  //
  // [dann 2007-04-04] ATMTP ^^
  
  set_error_t result = initvar_set_attr( (void *) name, NULL, value );
  (*m_generate_values_f)();
  return (result);
}

/** print out a list of valid parameters */
//**************************************************************************
void initvar_t::list_param( FILE *fp )
{
    if (!fp)
        fp = stdout;

#define PARAM( NAME )                            \
  fprintf( fp, "%-44.44s: %26d\n", #NAME, NAME );     
#define PARAM_UINT( NAME )                              \
  fprintf( fp, "%-44.44s: %26u\n", #NAME, NAME );     
#define PARAM_ULONG( NAME )                             \
  fprintf( fp, "%-44.44s: %26llu\n", #NAME, NAME );
#define PARAM_BOOL( NAME )                              \
  if (NAME == true) {                                 \
    fprintf( fp, "%-44.44s: %26.26s\n", #NAME, "true" ); \
  } else {                                            \
    fprintf( fp, "%-44.44s: %26.26s\n", #NAME, "false" );\
  }                                                   
#define PARAM_DOUBLE( NAME )                            \
  fprintf( fp, "%-44.44s: %26f\n", #NAME, NAME );
#define PARAM_STRING( NAME )                              \
  if ( NAME != NULL ) {                                   \
    fprintf( fp, "%-44.44s: %26.26s\n", #NAME, NAME );  \
  }
#define PARAM_ARRAY( PTYPE, NAME, ARRAY_SIZE )          \
  fprintf( fp, "%-44.44s: (", #NAME );                \
  for (int i = 0; i < ARRAY_SIZE; i++) {                \
    if ( i != 0 ) {                                     \
      fprintf( fp, ", " );                            \
    }                                                   \
    fprintf( fp, "%d", NAME[i] );                     \
  }                                                     \
  fprintf( fp, ")\n" );

#include CONFIG_VAR_FILENAME
#undef PARAM
#undef PARAM_UINT
#undef PARAM_ULONG
#undef PARAM_BOOL
#undef PARAM_DOUBLE
#undef PARAM_STRING
#undef PARAM_ARRAY
}

//**************************************************************************
const char *initvar_t::get_config_name( void )
{
  if (m_config_filename == NULL) {
    return "default";
  }
  return m_config_filename;
}
  
/*------------------------------------------------------------------------*/
/* Global functions                                                       */
/*------------------------------------------------------------------------*/

//**************************************************************************
attr_value_t initvar_dispatch_get( void *id, conf_object_t *obj,
                                   attr_value_t *idx )
{
  initvar_t  *init_obj = initvar_t::m_inst;
  return (init_obj->dispatch_get( id, obj, idx ));
}

//**************************************************************************
set_error_t initvar_dispatch_set( void *id, conf_object_t *obj, 
                                  attr_value_t *val, attr_value_t *idx )
{
  initvar_t  *init_obj = initvar_t::m_inst;
  return (init_obj->dispatch_set( id, obj, val, idx ));
}
