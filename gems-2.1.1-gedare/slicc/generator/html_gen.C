
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
 * html_gen.C
 * 
 * Description: See html_gen.h
 *
 * $Id: html_gen.C,v 3.4 2004/01/31 20:46:50 milo Exp $
 *
 * */

#include "html_gen.h"
#include "fileio.h"
#include "SymbolTable.h"

string formatHTMLShorthand(const string shorthand);


void createHTMLSymbol(const Symbol& sym, string title, ostream& out)
{
  out << "<HTML><BODY><BIG>" << endl;
  out << title << ": " << endl;
  out << formatHTMLShorthand(sym.getShorthand()) << " - ";
  out << sym.getDescription();
  out << "</BIG></BODY></HTML>" << endl;
}

void createHTMLindex(string title, ostream& out)
{
  out << "<html>" << endl;
  out << "<head>" << endl;
  out << "<title>" << title << "</title>" << endl;
  out << "</head>" << endl;
  out << "<frameset rows=\"*,30\">" << endl;
  Vector<StateMachine*> machine_vec = g_sym_table.getStateMachines();
  if (machine_vec.size() > 1) {
    string machine = machine_vec[0]->getIdent();
    out << "  <frame name=\"Table\" src=\"" << machine << "_table.html\">" << endl;
  } else {
    out << "  <frame name=\"Table\" src=\"empty.html\">" << endl;
  }
  
  out << "  <frame name=\"Status\" src=\"empty.html\">" << endl;
  out << "</frameset>" << endl;
  out << "</html>" << endl;
}

string formatHTMLShorthand(const string shorthand)
{
  string munged_shorthand = "";
  bool mode_is_normal = true;

  // -- Walk over the string, processing superscript directives
  for(unsigned int i = 0; i < shorthand.length(); i++) {
    if(shorthand[i] == '!') {
      // -- Reached logical end of shorthand name
      break;
    } else if( shorthand[i] == '_') {
      munged_shorthand += " ";      
    } else if( shorthand[i] == '^') {
      // -- Process super/subscript formatting
      mode_is_normal = !mode_is_normal;
      if(mode_is_normal) {
        // -- Back to normal mode
        munged_shorthand += "</SUP>";
      } else {
        // -- Going to superscript mode
        munged_shorthand += "<SUP>";
      }
    } else if(shorthand[i] == '\\') {
      // -- Process Symbol character set
      if((i + 1) < shorthand.length()) {
        i++;  // -- Proceed to next char. Yes I know that changing the loop var is ugly!
        munged_shorthand += "<B><FONT size=+1>";
        munged_shorthand += shorthand[i];
        munged_shorthand += "</FONT></B>";
      } else {
        // -- FIXME: Add line number info later
        cerr << "Encountered a `\\` without anything following it!" << endl;
        exit( -1 );
      }
    } else {
      // -- Pass on un-munged
      munged_shorthand += shorthand[i];
    }
  } // -- end for all characters in shorthand

  // -- Do any other munging 
  if(!mode_is_normal) {
    // -- Back to normal mode
    munged_shorthand += "</SUP>";
  }

  // -- Return the formatted shorthand name
  return munged_shorthand;
}


