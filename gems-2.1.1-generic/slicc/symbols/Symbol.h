
/*
    Copyright (C) 1999-2008 by Mark D. Hill and David A. Wood for the
    Wisconsin Multifacet Project.  Contact: gems@cs.wisc.edu
    http://www.cs.wisc.edu/gems/

    --------------------------------------------------------------------

    This file is part of the SLICC (Specification Language for 
    Implementing Cache Coherence), a component of the Multifacet GEMS 
    (General Execution-driven Multiprocessor Simulator) software 
    toolset originally developed at the University of Wisconsin-Madison.

    SLICC was originally developed by Milo Martin with substantial
    contributions from Daniel Sorin.

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
 */

#ifndef SYMBOL_H
#define SYMBOL_H

#include "slicc_global.h"
#include "Map.h"
#include "Location.h"

class Symbol {
public:
  // Constructors
  Symbol(string id, const Location& location, const Map<string, string>& pairs);
  Symbol(string id, const Location& location);
  // Destructor
  virtual ~Symbol() { }
  
  // Public Methods
  void error(string err_msg) const { m_location.error(err_msg); }
  void warning(string err_msg) const { m_location.warning(err_msg); }
  const Location& getLocation() const { return m_location; }

  const string& toString() const { return m_id; }

  const string& getIdent() const { return m_id; }
  const string& getShorthand() const { return lookupPair("short"); }
  const string& getDescription() const { return lookupPair("desc"); }

  void markUsed() { m_used = true; }
  bool wasUsed() { return m_used; }

  bool existPair(const string& key) const { return m_pairs.exist(key); }
  const string& lookupPair(const string& key) const;
  void addPair(const string& key, const string& value);

  //  virtual string getCode() const = 0;
  virtual void writeCFiles(string path) const {}
  virtual void writeHTMLFiles(string path) const {}
  virtual void print(ostream& out) const { out << "[Symbol: " << getIdent() << "]"; }

private:
  // Private Methods

  // Private copy constructor and assignment operator
  // Symbol(const Symbol& obj);
  // Symbol& operator=(const Symbol& obj);
  
  // Data Members (m_ prefix)
  string m_id;
  Map<string, string> m_pairs;
  Location m_location;
  bool m_used;
};

// Output operator declaration
ostream& operator<<(ostream& out, const Symbol& obj);

// ******************* Definitions *******************

// Output operator definition
extern inline 
ostream& operator<<(ostream& out, const Symbol& obj)
{
  obj.print(out);
  out << flush;
  return out;
}

#endif //SYMBOL_H
