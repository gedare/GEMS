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
 * FileName:  confio.C
 * Synopsis:  saves configuration information for later runs
 * Author:    cmauer
 * Version:   $Id$
 */

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#ifdef IS_OPAL
#include "hfa.h"
#endif

#ifdef IS_RUBY
#include "Global.h"
#define SIM_HALT   ASSERT(0)
#endif

#ifdef IS_TOURMALINE
#include "Tourmaline_Global.h"
#endif

using namespace std;
#include <string>
#include <map>
#include <stdlib.h>

extern "C" {
#include "global.h"
#include "simics/api.h"

#ifdef SIMICS22X
#include "sparc_api.h"
#endif
#ifdef SIMICS30
#ifdef SPARC
  #include "sparc.h"
#else
  #include "x86.h"
#endif
#endif
};

#include "confio.h"

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Variable declarations                                                  */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Forward declarations                                                   */
/*------------------------------------------------------------------------*/

// C++ Template: explicit instantiation
template class map<string, confnode_t *>;

// These functions are defined in parser/attrlex.l
extern "C" int parseAttrFile( FILE *inputFile, const char *relative_include_path, attr_value_t *myTable );
extern "C" int parseAttrString( const char *str, attr_value_t *myTable );

/*------------------------------------------------------------------------*/
/* Constructor(s) / destructor                                            */
/*------------------------------------------------------------------------*/

//**************************************************************************
confio_t::confio_t( )
{
  m_verbose = false;
}

//**************************************************************************
confio_t::~confio_t( )
{
  ConfTable::iterator iter;
  
  for ( iter = m_table.begin(); iter != m_table.end(); iter++ ) {
    confnode_t *cfnode = (*iter).second;
    free( cfnode );
  }
}

/*------------------------------------------------------------------------*/
/* Public methods                                                         */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Accessor(s) / mutator(s)                                               */
/*------------------------------------------------------------------------*/

//**************************************************************************
int confio_t::register_attribute( const char *name,
                                  get_confio_t get_attr, void *get_attr_data,
                                  set_confio_t set_attr, void *set_attr_data )
{
  confnode_t *newnode;
  if ( m_table.find(name) == m_table.end() ) {
    if (m_verbose)
      DEBUG_OUT("  registering checkpoint attribute: \"%s\"\n", name);
    
    // add a new entry to the table
    newnode = (confnode_t *) malloc( sizeof( confnode_t ) );
    newnode->get_attr = get_attr;
    newnode->set_attr = set_attr;
    newnode->set_attr_data = set_attr_data;
    newnode->get_attr_data = get_attr_data;
    newnode->attr_is_set = false;
    string key(name);
    m_table[key] = newnode;
  } else {
    ERROR_OUT("  warning: confio: adding existing conf node: %s\n", name);
  }
  return 0;
}

//**************************************************************************
void fprintAttr( FILE *fp, attr_value_t attr )
{
  switch (attr.kind) {
  case Sim_Val_Invalid:
    fprintf(fp, "invalid");
    break;

  case Sim_Val_String:
    fprintf(fp, "%s", attr.u.string);
    break;
    
  case Sim_Val_Integer: 
    fprintf(fp, "0x%llx", attr.u.integer);
    break;
    
  case Sim_Val_Floating: 
    fprintf(fp, "0x%llx", attr.u.integer);
    break;

  case Sim_Val_List:
    fprintf(fp, "(");
    for (uint32 i = 0; i < attr.u.list.size; i++) {
      fprintAttr(fp, attr.u.list.vector[i]);      
      if (i != attr.u.list.size -1) {
        fprintf(fp, ", ");
      }
    }
    fprintf(fp, ")");
    break;

  default:
    ERROR_OUT("fprintAttr: unknown/unimplemented attribute %d\n", attr.kind);
  }
}

//**************************************************************************
void freeAttribute( attr_value_t *attr )
{
  switch (attr->kind) {
  case Sim_Val_Invalid:
    break;

  case Sim_Val_String:
    free( (char *) attr->u.string );
    break;
    
  case Sim_Val_Integer: 
    break;
    
  case Sim_Val_Floating: 
    break;

  case Sim_Val_List:
    for (uint32 i = 0; i < attr->u.list.size; i++) {
      freeAttribute( &(attr->u.list.vector[i]) );
    }
    free( attr->u.list.vector );
    break;

  default:
    ERROR_OUT("freeAttr: unknown/unimplemented attribute %d\n", attr->kind);
  }
}

/**
 * Allocates, and initializes a attribute value.
 * @param number The number of values to allocate.
 * @return       A pointer to the newly allocated structure.
 */
//**************************************************************************
attr_value_t *mallocAttribute( uint32 number )
{
  attr_value_t *newattr = (attr_value_t *) malloc( number *
                                                   sizeof(attr_value_t) );
  if ( newattr == NULL ) {
    ERROR_OUT( "confio: mallocAttribute: out of memory\n" );
    exit(1);
  }
  memset( newattr, 0, number*sizeof(attr_value_t) );
  return (newattr);
}


//**************************************************************************
void fprintMap( FILE *fp, attr_value_t attr )
{
  attr_value_t  name;
  attr_value_t  value;

  if (attr.kind != Sim_Val_List)
    return;

  for (int i = 0; i < attr.u.list.size; i++) {

    if (attr.u.list.vector[i].kind != Sim_Val_List ||
        attr.u.list.vector[i].u.list.size != 2)
      return;

    name  = attr.u.list.vector[i].u.list.vector[0];
    value = attr.u.list.vector[i].u.list.vector[1];
    fprintf( fp, "      %s: ", name.u.string);
    fprintAttr( fp, value );
    fprintf( fp, "\n");
  }
}

/**
 * write a configuration file: e.g. save state
 */
//**************************************************************************
int confio_t::writeConfiguration( const char *outputFilename )
{
  FILE               *fp;
  ConfTable::iterator iter;
  confnode_t         *cfnode;
  attr_value_t        attr;

  memset( &attr, 0, sizeof(attr_value_t) );
  if ( outputFilename == NULL ) {
    fp = stdout;
  } else {
    fp = fopen( outputFilename, "w" );
    if ( fp == NULL ) {
      ERROR_OUT("error: writeConfiguration: unable to open file %s\n",
                outputFilename );
      return (-1);
    }
  }

  for (iter = m_table.begin(); iter != m_table.end(); iter++) {
    fprintf( fp, "      %s: ", (*iter).first.c_str() );
    cfnode = (*iter).second;
    attr   = (*(cfnode->get_attr))( cfnode->get_attr_data, NULL );
    fprintAttr( fp, attr );
    fprintf(fp, "\n");
  }

  if ( outputFilename != NULL ) {
    // wrote to a file: now close it!
    fclose( fp );
  }
  return 0;
}

/**
 * read state from an existing configuration file
 */
//**************************************************************************
int confio_t::readConfiguration( const char *inputFilename,
                                 const char *relativeIncludePath )
{
  // parse the input stream
  FILE               *fp;
  char                relativeFilename[256];
  
  fp = fopen( inputFilename, "r" );
  if ( fp == NULL ) {
    sprintf( relativeFilename, "%s%s", relativeIncludePath, inputFilename );
    fp = fopen( relativeFilename, "r" );
  }
  if ( fp == NULL ) {
    sprintf( relativeFilename, "%s%s%s", relativeIncludePath, "config/",
             inputFilename );
    fp = fopen( relativeFilename, "r" );
  }
  if ( fp == NULL ) {
    ERROR_OUT("error: readConfiguration: unable to open file %s or %s\n",
              inputFilename, relativeFilename);
    return (-1);
  }
  
  attr_value_t *myattr = mallocAttribute(1);
#ifdef MODINIT_VERBOSE
  DEBUG_OUT("confio_t() parsing conf file\n");
#endif
  int rc = parseAttrFile( fp, relativeIncludePath, myattr );
#ifdef MODINIT_VERBOSE
  DEBUG_OUT("confio_t() parse completed\n");
#endif
  if ( rc == 0 ) {
    applyConfiguration( myattr );
    freeAttribute( myattr );
    free(myattr);
  }

  fclose( fp );
#ifdef MODINIT_VERBOSE
  DEBUG_OUT("confio_t() completed\n");
#endif
  return (rc);
}

/**
 * read state from a configuration string
 */
//**************************************************************************
int confio_t::readConfigurationString( const char *inputBuffer )
{
  if ( inputBuffer == NULL ) {
    ERROR_OUT( "error: readConfiguration: NULL inputBuffer\n" );
    return (-1);
  }
  
  attr_value_t *myattr = mallocAttribute(1);
#ifdef MODINIT_VERBOSE
  DEBUG_OUT("confio_t() parsing conf string\n");
#endif
  
  int rc = parseAttrString( inputBuffer, myattr );
#ifdef MODINIT_VERBOSE
  DEBUG_OUT("confio_t() parse completed\n");
#endif
  if ( rc == 0 ) {
    applyConfiguration( myattr );
    freeAttribute( myattr );
    free(myattr);
  }
  return (rc);
}

//**************************************************************************
void confio_t::checkInitialization( void )
{
  ConfTable::iterator iter;
  confnode_t         *cfnode;

  for (iter = m_table.begin(); iter != m_table.end(); iter++) {
    cfnode = (*iter).second;
    if ( !cfnode->attr_is_set ) {
      DEBUG_OUT("    warning: %s is not set in configuration file.\n", (*iter).first.c_str() );
    }
  }
}

/*------------------------------------------------------------------------*/
/* Private methods                                                        */
/*------------------------------------------------------------------------*/

//**************************************************************************
int confio_t::applyConfiguration( attr_value_t *attr )
{
  confnode_t         *cfnode;
  attr_value_t        name;
  attr_value_t        value;
  set_error_t         seterr;

#ifdef MODINIT_VERBOSE
  DEBUG_OUT("confio_t() data in memory\n");
  fprintMap( stdout, *attr );
#endif

  // apply the configuration the the m_table
  if (attr->kind != Sim_Val_List ||
      attr->u.list.size <= 0) {
    ERROR_OUT("readconfiguration: internal error #1\n");
    return -1;
  }

  for (int i = 0; i < attr->u.list.size; i++) {
      
    if (attr->u.list.vector[i].kind != Sim_Val_List ||
        attr->u.list.vector[i].u.list.size != 2) {
      ERROR_OUT("readconfiguration: illegal configuration kind:%d size:%lld\n",
                attr->u.list.vector[i].kind,
                attr->u.list.vector[i].u.list.size);
      continue;
    }

    name   = attr->u.list.vector[i].u.list.vector[0];
    value  = attr->u.list.vector[i].u.list.vector[1];
    string newstr((char *) name.u.string);
    if ( m_table.find(newstr) != m_table.end()) {
        
      // set the value found in the configuration
      cfnode = m_table[newstr];
      seterr = (*cfnode->set_attr)( cfnode->set_attr_data, NULL,
                                    &(value) );
      if ( seterr == Sim_Set_Ok ) {
        cfnode->attr_is_set = true;
        if (m_verbose)
          DEBUG_OUT("configuration set for: %s\n", name.u.string);
      } else {
        ERROR_OUT("error: \"%s\" unable to set value: %d\n",
                  name.u.string, (int) seterr);
      }
    } else {
      ERROR_OUT("error: \"%s\" not found. unable to set value.\n",
                name.u.string);
    }
  }
  return 0;
}

/*------------------------------------------------------------------------*/
/* Static methods                                                         */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Global functions                                                       */
/*------------------------------------------------------------------------*/


/** [Memo].
 *  [Internal Documentation]
 */
//**************************************************************************

