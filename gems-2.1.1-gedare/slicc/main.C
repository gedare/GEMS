
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
 *
 * */

#include "main.h"
#include "StateMachine.h"
#include "mif_gen.h"
#include "html_gen.h"
#include "fileio.h"
#include "DeclListAST.h"
#include "Type.h"
#include "SymbolTable.h"
#include "Event.h"
#include "State.h"
#include "Action.h"
#include "Transition.h"

// -- Main conversion functions

void printDotty(const StateMachine& sm, ostream& out);
void printTexTable(const StateMachine& sm, ostream& out);

DeclListAST* g_decl_list_ptr;
DeclListAST* parse(string filename);

int main(int argc, char *argv[])
{
  cerr << "SLICC v0.3" << endl;

  if (argc < 5) {
    cerr << "  Usage: generator.exec <code path> <html path> <ident> <html direction> files ... " << endl;
    exit(1);
  }

  // The path we should place the generated code
  string code_path(argv[1]);
  code_path += "/";

  // The path we should place the generated html
  string html_path(argv[2]);
  html_path += "/";

  string ident(argv[3]);

  string html_generate(argv[4]);

  Vector<DeclListAST*> decl_list_vec;

  // Parse
  cerr << "Parsing..." << endl;
  for(int i=5; i<argc; i++) {
    cerr << "  " << argv[i] << endl;
    DeclListAST* decl_list_ptr = parse(argv[i]);
    decl_list_vec.insertAtBottom(decl_list_ptr);
  }

  // Find machines
  cerr << "Generator pass 1..." << endl;
  int size = decl_list_vec.size();
  for(int i=0; i<size; i++) {
    DeclListAST* decl_list_ptr = decl_list_vec[i];
    decl_list_ptr->findMachines();    
  }

  // Generate Code
  cerr << "Generator pass 2..." << endl;
  for(int i=0; i<size; i++) {
    DeclListAST* decl_list_ptr = decl_list_vec[i];
    decl_list_ptr->generate();    
    delete decl_list_ptr;
  }

  // Generate C/C++ files
  cerr << "Writing C files..." << endl;

  {
    // Generate the name of the protocol
    ostringstream sstr;
    sstr << "// Auto generated C++ code started by "<<__FILE__<<":"<<__LINE__<<endl;
    sstr << endl;
    sstr << "#ifndef PROTOCOL_NAME_H" << endl;
    sstr << "#define PROTOCOL_NAME_H" << endl;
    sstr << endl;
    sstr << "const char CURRENT_PROTOCOL[] = \"";
    sstr << ident << "\";\n";
    sstr << "#endif // PROTOCOL_NAME_H" << endl;
    conditionally_write_file(code_path + "/protocol_name.h", sstr);
  }

  g_sym_table.writeCFiles(code_path);

  // Generate HTML files
  if (html_generate == "html") {
    cerr << "Writing HTML files..." << endl;
    g_sym_table.writeHTMLFiles(html_path);
  } else if (html_generate == "no_html") {
    cerr << "No HTML files generated" << endl;
  } else {
    cerr << "ERROR, unidentified html direction" << endl;
  }

  cerr << "Done..." << endl;

  // Generate MIF files
  cerr << "Writing MIF files..." << endl;
  g_sym_table.writeMIFFiles(html_path);

  cerr << "Done..." << endl;

}
  /*  
  if(!strcmp(argv[2], "parse")) {
    // Parse only
  } else if(!strcmp(argv[2], "state")) {
    printStateTableMIF(s, cout);
  } else if(!strcmp( argv[2], "event")) {
    printEventTableMIF(s, cout);
  } else if(!strcmp( argv[2], "action")) {
    printActionTableMIF(s, cout);
  } else if(!strcmp( argv[2], "transition")) {
    printTransitionTableMIF(s, cout);
  } else if(!strcmp( argv[2], "tbe")) {
    for(int i=0; i<s.numTypes(); i++) {
      if (s.getType(i).getIdent() == "TBE") {
        printTBETableMIF(s, s.getTypeFields(i), cout);
      }
    }
  } else if(!strcmp( argv[2], "dot")) {
    printDotty(s, cout);
  } else if(!strcmp( argv[2], "latex")) {
    printTexTable(s, cout);
  } else if (!strcmp( argv[2], "murphi")) {
    printMurphi(s, cout);
  } else if (!strcmp( argv[2], "html")) {
    printHTML(s);
  } else if(!strcmp( argv[2], "code")) {
    if (argc < 4) {
      cerr << "Error: Wrong number of command line parameters!" << endl;
      exit(1);
    }
  */    


void printDotty(const StateMachine& sm, ostream& out)
{
  out << "digraph " << sm.getIdent() << " {" << endl;
  for(int i=0; i<sm.numTransitions(); i++) {
    const Transition& t = sm.getTransition(i);
    // Don't print ignored transitions
    if ((t.getActionShorthands() != "--") && (t.getActionShorthands() != "z")) {
    //    if (t.getStateShorthand() != t.getNextStateShorthand()) {
      out << "  " << t.getStateShorthand() << " -> ";
      out << t.getNextStateShorthand() << "[label=\"";
      out << t.getEventShorthand() << "/" 
          << t.getActionShorthands() << "\"]" << endl;
    }
  }
  out << "}" << endl;
}

void printTexTable(const StateMachine& sm, ostream& out)
{
  const Transition* trans_ptr;
  int stateIndex, eventIndex;
  string actions;
  string nextState;

  out << "%& latex" << endl;
  out << "\\documentclass[12pt]{article}" << endl;
  out << "\\usepackage{graphics}" << endl;
  out << "\\begin{document}" << endl;
  //  out << "{\\large" << endl;
  out << "\\begin{tabular}{|l||";
  for(eventIndex=0; eventIndex < sm.numEvents(); eventIndex++) {
    out << "l";
  }
  out << "|} \\hline" << endl;

  for(eventIndex=0; eventIndex < sm.numEvents(); eventIndex++) {
    out << " & \\rotatebox{90}{";
    out << sm.getEvent(eventIndex).getShorthand();
    out << "}";
  }
  out << "\\\\ \\hline  \\hline" << endl;

  for(stateIndex=0; stateIndex < sm.numStates(); stateIndex++) {
    out << sm.getState(stateIndex).getShorthand();
    for(eventIndex=0; eventIndex < sm.numEvents(); eventIndex++) {
      out << " & ";
      trans_ptr = sm.getTransPtr(stateIndex, eventIndex);
      if (trans_ptr == NULL) {
      } else {
        actions = trans_ptr->getActionShorthands();
        // FIXME: should compare index, not the string
        if (trans_ptr->getNextStateShorthand() != 
            sm.getState(stateIndex).getShorthand() ) { 
          nextState = trans_ptr->getNextStateShorthand();
        } else {
          nextState = "";
        }

        out << actions;
        if ((nextState.length() != 0) && (actions.length() != 0)) {
          out << "/";
        }
        out << nextState;
      }
    }
    out << "\\\\" << endl;
  }
  out << "\\hline" << endl;
  out << "\\end{tabular}" << endl;
  //  out << "}" << endl;
  out << "\\end{document}" << endl;
}

