
/*
    Copyright (C) 1999-2008 by Mark D. Hill and David A. Wood for the
    Wisconsin Multifacet Project.  Contact: gems@cs.wisc.edu
    http://www.cs.wisc.edu/gems/

    --------------------------------------------------------------------

    This file is part of the Ruby Multiprocessor Memory System Simulator, 
    a component of the Multifacet GEMS (General Execution-driven 
    Multiprocessor Simulator) software toolset originally developed at 
    the University of Wisconsin-Madison.

    Ruby was originally developed primarily by Milo Martin and Daniel
    Sorin with contributions from Ross Dickson, Carl Mauer, and Manoj
    Plakal.

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
 * $Id$
 *
 * Description: The histogram class implements a simple histogram
 * 
 */

#ifndef HISTOGRAM_H
#define HISTOGRAM_H

#include "Global.h"
#include "Vector.h"

class Histogram {
public:
  // Constructors
  Histogram(int binsize = 1, int bins = 50);

  // Destructor
  ~Histogram();
  
  // Public Methods

  void add(int64 value);
  void add(const Histogram& hist);
  void clear() { clear(m_bins); }
  void clear(int bins);
  void clear(int binsize, int bins);
  int64 size() const { return m_count; }
  int getBins() const { return m_bins; }
  int getBinSize() const { return m_binsize; }
  int64 getTotal() const { return m_sumSamples; }
  int64 getData(int index) const { return m_data[index]; }

  void printWithMultiplier(ostream& out, double multiplier) const;
  void printPercent(ostream& out) const;
  void print(ostream& out) const;
private:
  // Private Methods

  // Private copy constructor and assignment operator
  //  Histogram(const Histogram& obj);
  //  Histogram& operator=(const Histogram& obj);
  
  // Data Members (m_ prefix)
  Vector<int64> m_data;
  int64 m_max;          // the maximum value seen so far
  int64 m_count;                // the number of elements added
  int m_binsize;                // the size of each bucket
  int m_bins;           // the number of buckets
  int m_largest_bin;      // the largest bin used

  int64 m_sumSamples;   // the sum of all samples
  int64 m_sumSquaredSamples; // the sum of the square of all samples

  double getStandardDeviation() const;
};

bool node_less_then_eq(const Histogram* n1, const Histogram* n2);

// Output operator declaration
ostream& operator<<(ostream& out, const Histogram& obj);

// ******************* Definitions *******************

// Output operator definition
extern inline 
ostream& operator<<(ostream& out, const Histogram& obj)
{
  obj.print(out);
  out << flush;
  return out;
}

#endif //HISTOGRAM_H
