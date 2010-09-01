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


#ifndef _CONFIO_H_
#define _CONFIO_H_

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

/// constant for attribute parsing: a (x) single value
const  attr_kind_t   CONF_ATTR_SINGLE = (attr_kind_t) (Sim_Val_Object + 1);
/// constant for attribute parsing: a (x,y) pair of values
const  attr_kind_t   CONF_ATTR_PAIR   = (attr_kind_t) (Sim_Val_Object + 2);

/*------------------------------------------------------------------------*/
/* Class declaration(s)                                                   */
/*------------------------------------------------------------------------*/

/*
 *  Functions for modifying the micro-architectural configuation of
 *  a class.
 */

/// function for getting the configuration value
typedef  attr_value_t (*get_confio_t)( void *ptr, void *obj );
/// function for setting the configuration value
typedef  set_error_t  (*set_confio_t)( void *ptr, void *obj,
                                       attr_value_t *value );

/// a struture containing the functional callbacks for each conf node
typedef struct confnode {
  get_confio_t get_attr;
  set_confio_t set_attr;
  void        *set_attr_data;
  void        *get_attr_data;
  bool         attr_is_set;
} confnode_t;

/// a mapping from a string to a configuration structure
typedef map<string, confnode_t *> ConfTable;

/**
* Configuration state saving: allows the user to save the micro-architectural
* state in a text file for later runs. This file is also used to set
* globals during simulation.
*
* @author  cmauer
* @version $Id$
*/
class confio_t {

public:


  /**
   * @name Constructor(s) / destructor
   */
  //@{

  /**
   * Constructor: creates object
   */
  confio_t();

  /**
   * Destructor: frees object.
   */
  ~confio_t();
  //@}

  /**
   * @name Methods
   */
  //@{
  //@}

  /**
   * @name Accessor(s) / mutator(s)
   */
  //@{
  /**
   * register a configuration variable with the configuration manager.
   * @param get_attr   A function to get the attribute value
   * @param get_attr_data Void pointer, available to get_attr
   * @param set_attr   A function to set the attribute value
   * @param set_attr_data Void pointer, available to set_attr
   * @return [Description of return value]
   */
  int  register_attribute( const char *name,
                           get_confio_t get_attr, void *get_attr_data,
                           set_confio_t set_attr, void *set_attr_data );

  /**
   * Set verbosity of the configuration
   * @param verbose     True causes more info to be printed out, False doesn't
   */
  void setVerbosity( bool verbose ) {
    m_verbose = verbose;
  }

  /**
   * write a configuration file: e.g. save state
   */
  int  writeConfiguration( const char *outputFilename );
  
  /**
   * read state from an existing configuration file
   * @param inputFilename          The file to read
   * @param relativeIncludePath    The path to search on 'include' statements
   */
  int  readConfiguration( const char *inputFilename,
                          const char *relativeIncludePath );

  /**
   * read state from a string
   */
  int  readConfigurationString( const char *inputBuffer );

  /**
   * check that each registered configuration is set (reports a warning if
   * they are not.)
   */
  void checkInitialization( void );
  //@}

private:
  /**
   * Apply an attribute list to the configuration table
   */
  int  applyConfiguration( attr_value_t *attr );

  /// configuration table: contains a map from a string -> conf node
  ConfTable     m_table;

  /// if false, nothing is printed under normal operation
  bool          m_verbose;
};

/*------------------------------------------------------------------------*/
/* Global variables                                                       */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Global functions                                                       */
/*------------------------------------------------------------------------*/

/**
 * Allocates an array of attributes.
 */
attr_value_t *mallocAttribute( uint32 number );

/**
 * Walks an attribute tree, freeing all memory under attr. Does not free
 * attr itself.
 */
void          freeAttribute( attr_value_t *attr );

#endif  /* _CONFIO_H_ */


