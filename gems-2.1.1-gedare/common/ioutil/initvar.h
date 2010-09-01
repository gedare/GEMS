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


#ifndef _INCLUDE_H_
#define _INCLUDE_H_

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Class declaration(s)                                                   */
/*------------------------------------------------------------------------*/

/**
* This class deals with initializing the global variables in the object,
* setting the varibles (from the command line), printing the configuration,
* and saving it to a file.
*
* Before including this file, you must define the variable CONFIG_VAR_FILENAME
* to define which variables are to be used.
*
* @see     confio_t
* @author  cmauer
* @version $Id$
*/
class initvar_t {
public:
  /**
   * @name Constructor(s) / destructor
   */
  //@{

  /**
   * Constructor: creates object
   * @param name                The name of this object
   * @param relativeIncludePath The relative path to config files
   * @param initializingString  A string (with value pairs) for initialization
   * @param allocate_f        A ptr to the allocate function
   * @param generate_values   A ptr to the generate values function
   * @param my_get_attr       A ptr to the get attribute function
   * @param my_set_attr       A ptr to the set attribute function
   */
  initvar_t( const char *name, const char *relativeIncludePath,
             const char *initializingString,
             void (*allocate_fn)(void),
             void (*my_generate_fn)(void),
             get_attr_t my_get_attr, set_attr_t my_set_attr );

  /**
   * Destructor: frees object.
   */
  ~initvar_t();
  //@}

  /**
   * @name Methods
   */
  //@{
  /// calls the allocation routine explicitly (used by the tester)
  void         allocate( void );

  /// checks to see if all vars have been initialized
  void         checkInitialization( void );

  /// list all parameters: to a file (or stdout if file is NULL)
  void         list_param( FILE *fp );

  /// returns the name of the last config file to be read ("default" is none)
  const char  *get_config_name( void );
  
  /// calls through to the get_attr function, if object is initialized
  attr_value_t dispatch_get( void *id, conf_object_t *obj,
                             attr_value_t *idx );
  
  /** adds initialization attributes, calls through to the set_attr function,
   *  if object is initialized.
   */
  set_error_t  dispatch_set( void *id, conf_object_t *obj, 
                             attr_value_t *val, attr_value_t *idx );
  //@}
  /// (single) instance of the init var object
  static initvar_t *m_inst;
  
protected:
  ///returns true if the variables are initialized
  bool  confirm_init( void ) {
    return m_is_init;
  }

  ///read a configuration file
  void         read_config( const char *parameterFile );

  /// set a parameter to be a particular value
  set_error_t  set_param( const char *name, attr_value_t *value );

  /// initializes the configuration reader
  void         init_config_reader( const char *initString );

  /// bool value (true if initialized)
  bool       m_is_init;

  /// configuration reader
  confio_t  *m_config_reader;
  
  /// a pointer to a string (corresponding to this objects name)
  char      *m_name;

  /// a pointer to a string (representing the last config file read)
  char      *m_config_filename;

  /// the relative include path to the configuration files
  char      *m_rel_include_path;
  
  /// a pointer to the allocation function
  void     (*m_allocate_f)(void);
  
  /// a pointer to the generate values function
  void     (*m_generate_values_f)(void);

  /// a pointer to the session get function
  get_attr_t m_my_get_attr;
  /// a pointer to the session set function
  set_attr_t m_my_set_attr;
};


/*------------------------------------------------------------------------*/
/* Global variables                                                       */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Global functions                                                       */
/*------------------------------------------------------------------------*/

#ifdef __cplusplus
extern "C" {
#endif

///provides a dispatch mechanism that catches a few commands to get variables
attr_value_t initvar_dispatch_get( void *id, conf_object_t *obj,
                                   attr_value_t *idx );

///provides a dispatch mechanism that catches a few commands to set variables
set_error_t  initvar_dispatch_set( void *id, conf_object_t *obj, 
                                   attr_value_t *val, attr_value_t *idx );
  
#ifdef __cplusplus
} // extern "C"
#endif

#endif  /* _INCLUDE_H_ */
