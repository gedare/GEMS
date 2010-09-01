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
#ifndef _HISTOGRAM_H_
#define _HISTOGRAM_H_

typedef int32 histo_x_t; /* 32 bits */
typedef int32 histo_y_t; /* 32 bits */

struct hash_ent {
  histo_x_t x;           /* X value */
  histo_y_t y;           /* Y value */
  struct hash_ent *prev, *next; /* Link pointers */
};

/*------------------------------------------------------------------------*/
/* Class declaration(s)                                                   */
/*------------------------------------------------------------------------*/

/**
* This is the API interface for a histogram like
* statistic information tracker.
*
* @author  mxu
* @version $Id: histogram.h,v 3.5 2003/07/10 18:07:48 milo Exp $
*/

class histogram_t {
public:
  /// constructor
  histogram_t(const char* name, int size);

  /// destructor
  ~histogram_t();

  /** walk the histogram table, calling back F() on each entry */
  int walk(void (*f)(struct histogram_t *, int, int) );

  /** increment/decrement X by Y */
  int touch(histo_x_t X, histo_y_t Y);
  
  /** calculate and print out a cumulative distribution (possibly weighted)
   *  of the data. */
  int printDistribution( out_intf_t *stream, bool is_weighted );
  
  /** print out the data from the histogram */
  int print( out_intf_t *stream );

private:
  /** allocates the array, sorts & returns the total count of data values.
   */
  void  sortData( struct hash_ent **array, int *total );

  const char* m_name;              /* name of the this histogram */
  int m_size;                      /* length of the hash_table, it is
                                      also the hash number of the hash
                                      function. */
  int m_sizemask;                  /* mask derived from the size */

  int m_totalcount;                /* total count of Y entries in the table */
  
  /* this must be at the end of the struct, cause it is a variable
     length array. */
  struct hash_ent **m_hash_table;  /* the array of the hash_table with
                                      size "size" */
};

#endif  /* _HISTOGRAM_H_ */
