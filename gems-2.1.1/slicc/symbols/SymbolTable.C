
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
 * SymbolTable.C
 * 
 * Description: See SymbolTable.h
 *
 * $Id$
 *
 * */

#include "SymbolTable.h"
#include "fileio.h"
#include "html_gen.h"
#include "mif_gen.h"
#include "Action.h"
  
SymbolTable g_sym_table;

SymbolTable::SymbolTable()
{
  m_sym_map_vec.setSize(1);
  m_depth = 0;

  {
    Map<string, string> pairs;
    pairs.add("enumeration", "yes");
    newSym(new Type("MachineType", Location(), pairs));
  }

  {
    Map<string, string> pairs;
    pairs.add("primitive", "yes");
    pairs.add("external", "yes");
    newSym(new Type("void", Location(), pairs));
  }
}

SymbolTable::~SymbolTable()
{
  int size = m_sym_vec.size();
  for(int i=0; i<size; i++) {
    delete m_sym_vec[i];
  }
}

void SymbolTable::newSym(Symbol* sym_ptr)
{
  registerSym(sym_ptr->toString(), sym_ptr);
  m_sym_vec.insertAtBottom(sym_ptr);  // Holder for the allocated Sym objects.
}

void SymbolTable::newMachComponentSym(Symbol* sym_ptr)
{
  // used to cheat-- that is, access components in other machines
  StateMachine* mach_ptr = getStateMachine("current_machine");
  if (mach_ptr != NULL) {
    m_machine_component_map_vec.lookup(mach_ptr->toString()).add(sym_ptr->toString(), sym_ptr); 
  }
}

Var* SymbolTable::getMachComponentVar(string mach, string ident) 
{
  Symbol* s = m_machine_component_map_vec.lookup(mach).lookup(ident);
  return dynamic_cast<Var*>(s);
}


void SymbolTable::registerSym(string id, Symbol* sym_ptr)
{

  // Check for redeclaration (in the current frame only)
  if (m_sym_map_vec[m_depth].exist(id)) {
    sym_ptr->error("Symbol '" + id + "' redeclared in same scope.");
  }
  // FIXME - warn on masking of a declaration in a previous frame
  m_sym_map_vec[m_depth].add(id, sym_ptr);
}

void SymbolTable::registerGlobalSym(string id, Symbol* sym_ptr)
{
  // Check for redeclaration (global frame only)
  if (m_sym_map_vec[0].exist(id)) {
    sym_ptr->error("Global symbol '" + id + "' redeclared in global scope.");
  }
  m_sym_map_vec[0].add(id, sym_ptr);
}

Symbol* SymbolTable::getSym(string ident) const
{
  for (int i=m_depth; i>=0; i--) {
    if (m_sym_map_vec[i].exist(ident)) {
      return m_sym_map_vec[i].lookup(ident);
    }
  }
  return NULL;
}

void SymbolTable::newCurrentMachine(StateMachine* sym_ptr)
{
  registerGlobalSym(sym_ptr->toString(), sym_ptr);
  registerSym("current_machine", sym_ptr);
  m_sym_vec.insertAtBottom(sym_ptr);  // Holder for the allocated Sym objects.
   
  Map<string, Symbol*> m;
  m_machine_component_map_vec.add(sym_ptr->toString(),m);
  
}

Type* SymbolTable::getType(string ident) const
{
  return dynamic_cast<Type*>(getSym(ident)); 
}

Var* SymbolTable::getVar(string ident) const
{
  return dynamic_cast<Var*>(getSym(ident));
}

Func* SymbolTable::getFunc(string ident) const
{
  return dynamic_cast<Func*>(getSym(ident));
}

StateMachine* SymbolTable::getStateMachine(string ident) const
{
  return dynamic_cast<StateMachine*>(getSym(ident));
}

void SymbolTable::pushFrame()
{
  m_depth++;
  m_sym_map_vec.expand(1);
  m_sym_map_vec[m_depth].clear();
}

void SymbolTable::popFrame()
{
  m_depth--;
  assert(m_depth >= 0);
  m_sym_map_vec.expand(-1);
}

void SymbolTable::writeCFiles(string path) const
{
  int size = m_sym_vec.size();
  {
    // Write the Types.h include file for the types
    ostringstream sstr;
    sstr << "/** Auto generated C++ code started by "<<__FILE__<<":"<<__LINE__<< " */" << endl;
    sstr << endl;
    sstr << "#include \"RubySlicc_includes.h\"" << endl;
    for(int i=0; i<size; i++) {
      Type* type = dynamic_cast<Type*>(m_sym_vec[i]);
      if (type != NULL && !type->isPrimitive()) {
        sstr << "#include \"" << type->cIdent() << ".h" << "\"" << endl;
      }
    }
    conditionally_write_file(path + "/Types.h", sstr);
  }
  
  // Write all the symbols
  for(int i=0; i<size; i++) { 
    m_sym_vec[i]->writeCFiles(path + '/');
  }
  
  writeChipFiles(path);
}

void SymbolTable::writeChipFiles(string path) const
{
  // Create Chip.C and Chip.h

  // FIXME - Note: this method is _really_ ugly.  Most of this
  // functionality should be pushed into each type of symbol and use
  // virtual methods to get the right behavior for each type of
  // symbol.  This is also more flexible, and much cleaner.
 
  int size = m_sym_vec.size();

  // Create Chip.h
  {
    ostringstream sstr;
    sstr << "/** \\file Chip.h " << endl;
    sstr << "  * Auto generated C++ code started by "<<__FILE__<<":"<<__LINE__<<endl;
    sstr << "  */ " <<endl<<endl;
    
    sstr << "#ifndef CHIP_H" << endl;
    sstr << "#define CHIP_H" << endl;
    sstr << endl;

    // Includes
    sstr << "#include \"Global.h\"" << endl;
    sstr << "#include \"Types.h\"" << endl;
    sstr << "#include \"AbstractChip.h\"" << endl;
    sstr << "class Network;" << endl;
    sstr << endl;

    // Class declarations for all Machines/Controllers
    for(int i=0; i<size; i++) { 
      StateMachine* machine = dynamic_cast<StateMachine*>(m_sym_vec[i]);
      if (machine != NULL) {
        sstr << "class " << machine->getIdent() << "_Controller;" << endl;
      }
    }  

    sstr << "class Chip : public AbstractChip {" << endl;
    sstr << "public:" << endl;
    sstr << endl;
    sstr << "  // Constructors" << endl;
    sstr << "  Chip(NodeID chip_number, Network* net_ptr);" << endl;
    sstr << endl;
    sstr << "  // Destructor" << endl;
    sstr << "  ~Chip();" << endl;
    sstr << endl;
    sstr << "  // Public Methods" << endl;
    sstr << "  void recordCacheContents(CacheRecorder& tr) const;" << endl;
    sstr << "  void dumpCaches(ostream& out) const;" << endl;
    sstr << "  void dumpCacheData(ostream& out) const;" << endl;
    sstr << "  static void printStats(ostream& out);" << endl;
    sstr << "  static void clearStats();" << endl;
    sstr << "  void printConfig(ostream& out);" << endl;
    sstr << "  void print(ostream& out) const;" << endl;

    // Used by coherence checker
    sstr << "#ifdef CHECK_COHERENCE" << endl;
    sstr << "  bool isBlockShared(const Address& addr) const;" << endl;
    sstr << "  bool isBlockExclusive(const Address& addr) const;" << endl;
    sstr << "#endif /* CHECK_COHERENCE */" << endl;

    sstr << endl;
    sstr << "private:" << endl;
    sstr << "  // Private copy constructor and assignment operator" << endl;
    sstr << "  Chip(const Chip& obj);" << endl;
    sstr << "  Chip& operator=(const Chip& obj);" << endl;
    sstr << endl;
    sstr << "public: // FIXME - these should not be public" << endl;
    sstr << "  // Data Members (m_ prefix)" << endl;
    sstr << endl;
    sstr << "  Chip* m_chip_ptr;" << endl;
    sstr << endl;
    sstr << "  // SLICC object variables" << endl;
    sstr << endl;

    // Look at all 'Vars'
    for(int i=0; i<size; i++) { 
      Var* var = dynamic_cast<Var*>(m_sym_vec[i]);
      if (var != NULL) {
        if (var->existPair("chip_object")) {
          if (var->existPair("no_chip_object")) {
            // Do nothing
          } else {
            string template_hack = "";
            if (var->existPair("template_hack")) {
              template_hack = var->lookupPair("template_hack");
            }
            if (// var->existPair("network") || var->getType()->existPair("cache") ||
//                 var->getType()->existPair("tbe") || var->getType()->existPair("newtbe") || 
//                 var->getType()->existPair("dir") || var->getType()->existPair("persistent") || 
//                 var->getType()->existPair("filter") || var->getType()->existPair("timer") ||
//                 var->existPair("trigger_queue")
                var->existPair("no_vector")
                ) {
              sstr << "  " << var->getType()->cIdent() << template_hack << "* m_" 
                   << var->cIdent() << "_ptr;" << endl;
            } else {
              // create pointer except those created in AbstractChip
              if (!(var->existPair("abstract_chip_ptr"))) {
                sstr << "  Vector < " << var->getType()->cIdent() << template_hack 
                     << "* >  m_" << var->cIdent() << "_vec;" << endl;
              }
            }
          }
        }
      }
    }
    
    sstr << endl;
    sstr << "  // SLICC machine/controller variables" << endl;

    // Look at all 'Machines'
    for(int i=0; i<size; i++) { 
      StateMachine* machine = dynamic_cast<StateMachine*>(m_sym_vec[i]);
      if (machine != NULL) {
        string ident = machine->getIdent() + "_Controller";
        sstr << "  Vector < " << ident << "* > m_" << ident << "_vec;\n";
      }
    }  

    sstr << endl;

    sstr << "  // machine external SLICC function decls\n";
    
    // Look at all 'Functions'
    for(int i=0; i<size; i++) { 
      Func* func = dynamic_cast<Func*>(m_sym_vec[i]);
      if (func != NULL) {
        string proto;
        func->funcPrototype(proto);
        if (proto != "") {
          sstr << "  " << proto;
        }
      }
    }  

    sstr << "};" << endl;
    sstr << endl;
    sstr << "#endif // CHIP_H" << endl;

    conditionally_write_file(path + "/Chip.h", sstr);
  }
  // Create Chip.C
  {
    ostringstream sstr;
    sstr << "// Auto generated C++ code started by "<<__FILE__<<":"<<__LINE__<<endl<<endl;
    sstr << "#include \"Chip.h\"" << endl;
    sstr << "#include \"Network.h\"" << endl;
    sstr << "#include \"CacheRecorder.h\"" << endl;
    sstr << "" << endl;

    sstr << "// Includes for controllers" << endl;
    for(int i=0; i<size; i++) { 
      StateMachine* machine = dynamic_cast<StateMachine*>(m_sym_vec[i]);
      if (machine != NULL) {
        sstr << "#include \"" << machine->getIdent() << "_Controller.h\"" << endl;
      }
    }  

    sstr << "" << endl;
    sstr << "Chip::Chip(NodeID id, Network* net_ptr):AbstractChip(id, net_ptr)" << endl;
    sstr << "{" << endl;
    sstr << "  m_chip_ptr = this;" << endl;

    // FIXME - WHY IS THIS LOOP HERE?
    // WE SEEM TO BE CREATING A SEQUENCER HERE THEN OVERWRITTING THAT INSTANITATION
    // IN THE NEXT LOOP
//     // find sequencer's type
//     for(int i=0; i<size; i++) { 
//       Var* var = dynamic_cast<Var*>(m_sym_vec[i]);
//       if(var && var->cIdent() == "sequencer")
//         sstr << "  m_sequencer_ptr = new " << var->getType()->cIdent() << "(this);\n";
//     }

    // Look at all 'Vars'
    for(int i=0; i<size; i++) { 
      Var* var = dynamic_cast<Var*>(m_sym_vec[i]);
      if (var != NULL && var->existPair("chip_object") && !var->existPair("no_chip_object")) {

        sstr << "  // " << var->cIdent() << endl;
        if (!var->existPair("network")) {
          // Not a network port object
          if (var->getType()->existPair("primitive")) {
            // Normal non-object
            // sstr << "  m_" << var->cIdent() << "_ptr = new " << var->getType()->cIdent() << ";\n";

              sstr << "  m_" << var->cIdent(); 
              sstr << "_vec.setSize(RubyConfig::numberOf";
              sstr << var->getMachine()->getIdent() << "PerChip(m_id));" << endl;
              sstr << "  for (int i = 0; i < RubyConfig::numberOf" << var->getMachine()->getIdent() 
                   << "PerChip(m_id); i++)  {" << endl;
              sstr << "    m_" << var->cIdent() << "_vec[i] = new " << var->getType()->cIdent() << ";\n";
              if (var->existPair("default")) {
                sstr << "    *(m_" << var->cIdent() << "_vec[i]) = " << var->lookupPair("default") << ";\n"; 
              }  
              sstr << "  }\n";

          } else {

            // Normal Object
            string template_hack = "";
            if (var->existPair("template_hack")) {
              template_hack = var->lookupPair("template_hack");
            }
            if (// var->getType()->existPair("cache") || var->getType()->existPair("tbe") || 
//                 var->getType()->existPair("newtbe") || var->getType()->existPair("timer") ||
//                 var->getType()->existPair("dir") || var->getType()->existPair("persistent") || 
//                 var->getType()->existPair("filter") || var->existPair("trigger_queue")
                var->existPair("no_vector")) {
              sstr << "  m_" << var->cIdent() << "_ptr = new " << var->getType()->cIdent() << template_hack;
              if (!var->getType()->existPair("non_obj") && (!var->getType()->isEnumeration())) {
                if (var->existPair("constructor_hack")) {
                  string constructor_hack = var->lookupPair("constructor_hack");
                  sstr << "(this, " << constructor_hack << ")";
                } else {
                  sstr << "(this)";
                }
              }
              sstr << ";\n";
              sstr << "  assert(m_" << var->cIdent() << "_ptr != NULL);" << endl;

              if (var->existPair("default")) {
                sstr << "  (*m_" << var->cIdent() << "_ptr) = " << var->lookupPair("default") 
                     << "; // Object default" << endl;
              } else if (var->getType()->hasDefault()) {
                sstr << "  (*m_" << var->cIdent() << "_ptr) = " << var->getType()->getDefault() 
                     << "; // Type " << var->getType()->getIdent() << " default" << endl;
              }
              
              // Set ordering
              if (var->existPair("ordered") && !var->existPair("trigger_queue")) {
                // A buffer
                string ordered =  var->lookupPair("ordered");
                sstr << "  m_" << var->cIdent() << "_ptr->setOrdering(" << ordered << ");\n";
              }
              
              // Set randomization
              if (var->existPair("random")) {
                // A buffer
                string value =  var->lookupPair("random");
                sstr << "  m_" << var->cIdent() << "_ptr->setRandomization(" << value << ");\n";
              }
              
              // Set Priority
              if (var->getType()->isBuffer() && var->existPair("rank") && !var->existPair("trigger_queue")) {
                string rank =  var->lookupPair("rank");
                sstr << "  m_" << var->cIdent() << "_ptr->setPriority(" << rank << ");\n";
              }
            } else if ((var->getType()->existPair("mover")) && (var->getMachine()->getIdent() == "L2Cache")) {
              // FIXME - dnuca mover is a special case
              sstr << "  m_" << var->cIdent() << "_ptr = NULL;" << endl;
              sstr << "  if (RubyConfig::isL2CacheDNUCAMoverChip(m_id))  {" << endl;
              sstr << "    m_" << var->cIdent() << "_ptr = new " << var->getType()->cIdent() << template_hack;
              if (!var->getType()->existPair("non_obj") && (!var->getType()->isEnumeration())) {
                if (var->existPair("constructor_hack")) {
                  string constructor_hack = var->lookupPair("constructor_hack");
                  sstr << "(this, " << constructor_hack << ")";
                } else {
                  sstr << "(this)";
                }
              }
              sstr << ";\n";
              sstr << "  }\n";
            } else if (var->getType()->existPair("mover") && ((var->getMachine()->getIdent() == "L1Cache") || (var->getMachine()->getIdent() == "Collector"))) {
              sstr << "  m_" << var->cIdent() << "_ptr = NULL;" << endl;
              sstr << "  \n";
            } else {
              sstr << "  m_" << var->cIdent(); 
              sstr << "_vec.setSize(RubyConfig::numberOf";
              sstr << var->getMachine()->getIdent() << "PerChip(m_id));" << endl;
              sstr << "  for (int i = 0; i < RubyConfig::numberOf" << var->getMachine()->getIdent() 
                   << "PerChip(m_id); i++)  {" << endl;

              
              ostringstream tail;
              tail << template_hack;
              if (!var->getType()->existPair("non_obj") && (!var->getType()->isEnumeration())) {
                if (var->existPair("constructor_hack")) {
                  string constructor_hack = var->lookupPair("constructor_hack");
                  tail << "(this, " << constructor_hack << ")";
                } else {
                  tail << "(this)";
                }
              }
              tail << ";\n";
              
              
              if(var->existPair("child_selector")){
                string child_selector = var->lookupPair("child_selector");
                string child_types = var->lookupPair("child_types");
                string::iterator it = child_types.begin();

                uint num_types = 0;
                for(uint t=0;t<child_types.size();t++){
                  if(child_types.at(t) == '<'){
                    num_types++;
                  }
                }

                string* types = new string[num_types];
                string* ids = new string[num_types];
                int type_idx = 0;
                bool id_done = false;
                for(uint t=0;t<child_types.size();t++){
                  if(child_types[t] == '<'){
                    id_done = false;
                    uint r;
                    for(r=t+1;child_types.at(r)!='>';r++){
                      if(r == child_types.size()){
                        cerr << "Parse error in child_types" << endl;
                        exit(EXIT_FAILURE);
                      }
                      if(child_types.at(r) == ' ') continue;  //ignore whitespace
                      if(child_types.at(r) == ',') {id_done = true;continue;}
                      if(id_done == true)
                        types[type_idx].push_back(child_types.at(r));
                      else
                        ids[type_idx].push_back(child_types.at(r));
                    }
                    type_idx++;
                    t = r;
                  }
                }
                
                for(uint t=0;t<num_types;t++){
                  if(t==0)
                    sstr << "    if(strcmp(" << child_selector << ", \"" << ids[t] << "\") == 0)" << endl;
                  else
                    sstr << "    else if(strcmp(" << child_selector << ", \"" << ids[t] << "\") == 0)" << endl;
                  sstr << "      m_" << var->cIdent() << "_vec[i] = new " << types[t] << tail.str() << endl;
                }
              } 
              else {
                sstr << "    m_" << var->cIdent() << "_vec[i] = new " << var->getType()->cIdent() << tail.str() << endl; 
              }
              
              sstr << "    assert(m_" << var->cIdent() << "_vec[i] != NULL);" << endl;
              if (var->existPair("ordered")) {
                string ordered =  var->lookupPair("ordered");
                sstr << "    m_" << var->cIdent() << "_vec[i]->setOrdering(" << ordered << ");\n";
              }
              if (var->existPair("rank")) {
                string rank =  var->lookupPair("rank");
                sstr << "    m_" << var->cIdent() << "_vec[i]->setPriority(" << rank << ");\n";
              }

              // Set buffer size
              if (var->getType()->isBuffer() && !var->existPair("infinite")) {
                sstr << "    if (FINITE_BUFFERING) {\n";
                sstr << "      m_" << var->cIdent() << "_vec[i]->setSize(PROCESSOR_BUFFER_SIZE);\n";
                sstr << "    }\n";
              }              

              sstr << "  }\n";
            }
          }

          sstr << endl;

        } else {
          // Network port object
          string network = var->lookupPair("network");
          string ordered =  var->lookupPair("ordered");
          string vnet =  var->lookupPair("virtual_network");
          
          if (var->getMachine() != NULL) {
            sstr << "  m_" << var->cIdent() << "_vec.setSize(RubyConfig::numberOf" 
                 << var->getMachine()->getIdent() << "PerChip(m_id));" << endl;
            sstr << "  for (int i = 0; i < RubyConfig::numberOf" << var->getMachine()->getIdent()
                 << "PerChip(m_id); i++)  {" << endl;
            sstr << "    m_" << var->cIdent() << "_vec[i] = m_net_ptr->get" 
                 << network << "NetQueue(i+m_id*RubyConfig::numberOf" <<var->getMachine()->getIdent()
                 << "PerChip()+MachineType_base_number(string_to_MachineType(\""
                 << var->getMachine()->getIdent() << "\")), " 
                 << ordered << ", " << vnet << ");\n";
            sstr << "    assert(m_" << var->cIdent() << "_vec[i] != NULL);" << endl;
          } else { // old protocol
            sstr << "  m_" << var->cIdent() << "_vec.setSize(1);" << endl;
            sstr << "  for (int i = 0; i < 1; i++)  {" << endl;
            sstr << "    m_" << var->cIdent() << "_vec[i] = m_net_ptr->get" 
                 << network << "NetQueue(m_id, " 
                 << ordered << ", " << vnet << ");\n";
            sstr << "    assert(m_" << var->cIdent() << "_vec[i] != NULL);" << endl;
          }          

          // Set ordering
          if (var->existPair("ordered")) {
            // A buffer
            string ordered =  var->lookupPair("ordered");
            sstr << "    m_" << var->cIdent() << "_vec[i]->setOrdering(" << ordered << ");\n";
          }
          
          // Set randomization
          if (var->existPair("random")) {
            // A buffer
            string value =  var->lookupPair("random");
            sstr << "    m_" << var->cIdent() << "_vec[i]->setRandomization(" << value << ");\n";
          }
          
          // Set Priority
          if (var->existPair("rank")) {
            string rank =  var->lookupPair("rank");
            sstr << "    m_" << var->cIdent() << "_vec[i]->setPriority(" << rank << ");\n";
          }

          // Set buffer size
          if (var->getType()->isBuffer()) {
            sstr << "    if (FINITE_BUFFERING) {\n";
            sstr << "      m_" << var->cIdent() << "_vec[i]->setSize(PROTOCOL_BUFFER_SIZE);\n";
            sstr << "    }\n";
          }

          sstr << "  }\n";
        }
      }
    }
    // Look at all 'Machines'
    for(int i=0; i<size; i++) { 
      StateMachine* machine = dynamic_cast<StateMachine*>(m_sym_vec[i]);
      if (machine != NULL) {
        string ident = machine->getIdent() + "_Controller";
        sstr << "  m_" << ident << "_vec.setSize(RubyConfig::numberOf" << machine->getIdent() 
             << "PerChip(m_id));" << endl;
        sstr << "  for (int i = 0; i < RubyConfig::numberOf" << machine->getIdent() 
             << "PerChip(m_id); i++)  {" << endl;
        sstr << "    m_" << ident << "_vec[i] = new " << ident << "(this, i);\n";
        sstr << "    assert(m_" << ident << "_vec[i] != NULL);" << endl;
        sstr << "  }\n";
        sstr << endl;
      }
    }  

    sstr << "}" << endl;
    sstr << endl;
    sstr << "Chip::~Chip()\n";
    sstr << "{\n";
    
//     // FIXME: sequencer shouldn' be manually handled
//     sstr << "  delete m_sequencer_ptr;" << endl;

    // Look at all 'Vars'
    for(int i=0; i<size; i++) { 
      Var* var = dynamic_cast<Var*>(m_sym_vec[i]);
      if (var != NULL) {
        if (var->existPair("chip_object")) {
          if (var->existPair("no_chip_object")) {
            // Do nothing
          } else {
            string template_hack = "";
            if (var->existPair("template_hack")) {
              template_hack = var->lookupPair("template_hack");
            }
            if (// var->getType()->existPair("cache") || var->getType()->existPair("tbe") ||
//                 var->getType()->existPair("newtbe") || var->getType()->existPair("timer") ||
//                 var->getType()->existPair("dir") || var->getType()->existPair("persistent") ||
//                 var->getType()->existPair("filter") || var->existPair("trigger_queue")
                var->existPair("no_vector")) {
              sstr << "  delete m_" << var->cIdent() << "_ptr;\n";
            } else if ((var->getType()->existPair("mover")) && (var->getMachine()->getIdent() == "L2Cache")) {
              sstr << "  if (RubyConfig::isL2CacheDNUCAMoverChip(m_id))  {" << endl;
              sstr << "    delete m_" << var->cIdent() << "_ptr;\n";
              sstr << "  }\n";                         
            } else if (var->getType()->existPair("mover") && ((var->getMachine()->getIdent() == "L1Cache") || (var->getMachine()->getIdent() == "Collector"))) {
              sstr << "  m_" << var->cIdent() << "_ptr = NULL;" << endl;
            } else if (!var->existPair("network")) {
              // Normal Object
              sstr << "  for (int i = 0; i < RubyConfig::numberOf" << var->getMachine()->getIdent() 
                   << "PerChip(m_id); i++)  {" << endl;          
              sstr << "    delete m_" << var->cIdent() << "_vec[i];\n";
              sstr << "  }\n";            
            }
          }
        }
      }
    }

    // Look at all 'Machines'
    for(int i=0; i<size; i++) { 
      StateMachine* machine = dynamic_cast<StateMachine*>(m_sym_vec[i]);
      if (machine != NULL) {
        string ident = machine->getIdent() + "_Controller";
        sstr << "  for (int i = 0; i < RubyConfig::numberOf" << machine->getIdent() 
             << "PerChip(m_id); i++)  {" << endl;          
        sstr << "    delete m_" << ident << "_vec[i];\n";
        sstr << "  }\n";
      }
    }  
    sstr << "}\n";

    sstr << "\n";
    sstr << "void Chip::clearStats()\n";
    sstr << "{\n";


    // Look at all 'Machines'
    for(int i=0; i<size; i++) { 
      StateMachine* machine = dynamic_cast<StateMachine*>(m_sym_vec[i]);
      if (machine != NULL) {
        string ident = machine->getIdent() + "_Controller";
        sstr << "  " << ident << "::clearStats();\n";
      }
    }  

    sstr << "}\n";

    sstr << "\n";
    sstr << "void Chip::printStats(ostream& out)\n";
    sstr << "{\n";
    sstr << "  out << endl;\n";
    sstr << "  out << \"Chip Stats\" << endl;\n";
    sstr << "  out << \"----------\" << endl << endl;\n";

    // Look at all 'Machines'
    for(int i=0; i<size; i++) { 
      StateMachine* machine = dynamic_cast<StateMachine*>(m_sym_vec[i]);
      if (machine != NULL) {
        string ident = machine->getIdent() + "_Controller";
        sstr << "  " << ident << "::dumpStats(out);\n";
      }
    }  
          
    sstr << "}" << endl;
    sstr << endl;
    sstr << "void Chip::printConfig(ostream& out)\n";
    sstr << "{\n";
    sstr << "  out << \"Chip Config\" << endl;\n";
    sstr << "  out << \"-----------\" << endl;\n";
    sstr << "  out << \"Total_Chips: \" << RubyConfig::numberOfChips() << endl;\n";

    // Look at all 'Vars'
    for(int i=0; i<size; i++) { 
      Var* var = dynamic_cast<Var*>(m_sym_vec[i]);
      if (var != NULL) {
        if (var->existPair("chip_object")) {
          if (var->existPair("no_chip_object")) {
            // Do nothing
          } else {
            string template_hack = "";
            if (var->existPair("template_hack")) {
              template_hack = var->lookupPair("template_hack");
            }

            if (!var->existPair("network") && (!var->getType()->existPair("primitive"))) {
              // Normal Object
              if (!var->getType()->existPair("non_obj") && (!var->getType()->isEnumeration())) {
                if (var->existPair("no_vector")) {
                  sstr << "  m_" << var->cIdent() << "_ptr->printConfig(out);\n";
                } else {
                  sstr << "  out << \"\\n" << var->cIdent() << " numberPerChip: \" << RubyConfig::numberOf" << var->getMachine()->getIdent()
                       << "PerChip() << endl;\n";
                  sstr << "  m_" << var->cIdent() << "_vec[0]->printConfig(out);\n";
//                   sstr << "  for (int i = 0; i < RubyConfig::numberOf" << var->getMachine()->getIdent()
//                        << "PerChip(m_id); i++)  {" << endl;
//                   sstr << "    m_" << var->cIdent() << "_vec[i]->printConfig(out);\n";
//                   sstr << "  }\n";
                }
              }
            } 
          }
        }
      }
    }

    sstr << "  out << endl;\n";
    sstr << "}" << endl;

    sstr << endl;
    sstr << "void Chip::print(ostream& out) const\n";
    sstr << "{\n";
    sstr << "  out << \"Ruby Chip\" << endl;\n";
    sstr << "}" << endl;

    sstr << "#ifdef CHECK_COHERENCE" << endl;
    sstr << endl;
    sstr << "bool Chip::isBlockShared(const Address& addr) const" << endl;
    sstr << "{" << endl;

    // Look at all 'Machines'
    for(int i=0; i<size; i++) { 
      StateMachine* machine = dynamic_cast<StateMachine*>(m_sym_vec[i]);
      if (machine != NULL) {
        string ident = machine->getIdent() + "_Controller";
        sstr << "  for (int i = 0; i < RubyConfig::numberOf" << machine->getIdent() 
             << "PerChip(m_id); i++)  {" << endl;          
        sstr << "    if (m_" << ident << "_vec[i]->" << machine->getIdent() << "_isBlockShared(addr)) {\n";
        sstr << "      return true; \n";
        sstr << "    }\n";
        sstr << "  }\n";
      }
    }
    sstr << "  return false;" << endl;
    sstr << "}" << endl;
    sstr << endl;

    sstr << endl;
    sstr << "bool Chip::isBlockExclusive(const Address& addr) const" << endl;
    sstr << "{" << endl;

    // Look at all 'Machines'
    for(int i=0; i<size; i++) { 
      StateMachine* machine = dynamic_cast<StateMachine*>(m_sym_vec[i]);
      if (machine != NULL) {
        string ident = machine->getIdent() + "_Controller";
        sstr << "  for (int i = 0; i < RubyConfig::numberOf" << machine->getIdent() 
             << "PerChip(m_id); i++)  {" << endl;          
        sstr << "    if (m_" << ident << "_vec[i]->" << machine->getIdent() << "_isBlockExclusive(addr)) {\n";
        sstr << "      return true; \n";
        sstr << "    }\n";
        sstr << "  }\n";
      }
    }

    sstr << "  return false;" << endl;
    sstr << "}" << endl;
    sstr << endl;

    sstr << "#endif /* CHECK_COHERENCE */ " << endl;


    sstr << endl;
    sstr << "void Chip::dumpCaches(ostream& out) const" << endl;
    sstr << "{" << endl;

    // Look at all 'Vars'
    for(int i=0; i<size; i++) { 
      Var* var = dynamic_cast<Var*>(m_sym_vec[i]);
      if (var != NULL) {
        if (var->getType()->existPair("cache")){  // caches are partitioned one per controller instaniation
          sstr << "  for (int i = 0; i < RubyConfig::numberOf" << var->getMachine()->getIdent() 
               << "PerChip(m_id); i++)  {" << endl;          
          sstr << "    m_" << var->cIdent() << "_vec[i]->print(out);\n";
          sstr << "  }\n";
        }
      }
    }
    sstr << "}" << endl;
    sstr << endl;

    // Function to dump cache tag and data information
    sstr << "void Chip::dumpCacheData(ostream& out) const" << endl;
    sstr << "{" << endl;

    // Look at all 'Vars'
    for(int i=0; i<size; i++) { 
      Var* var = dynamic_cast<Var*>(m_sym_vec[i]);
      if (var != NULL) {
        if (var->getType()->existPair("cache")){  // caches are partitioned one per controller instaniation
          sstr << "  for (int i = 0; i < RubyConfig::numberOf" << var->getMachine()->getIdent() 
               << "PerChip(m_id); i++)  {" << endl;          
          sstr << "    m_" << var->cIdent() << "_vec[i]->printData(out);\n";
          sstr << "  }\n";
        }
      }
    }
    sstr << "}" << endl;
    sstr << endl;

    sstr << "void Chip::recordCacheContents(CacheRecorder& tr) const" << endl;
    sstr << "{" << endl;

    // Look at all 'Vars'
    for(int i=0; i<size; i++) { 
      Var* var = dynamic_cast<Var*>(m_sym_vec[i]);
      if (var != NULL) {
        if (var->getType()->existPair("cache")){  // caches are partitioned one per controller instaniation
          sstr << "  for (int i = 0; i < RubyConfig::numberOf" << var->getMachine()->getIdent() 
               << "PerChip(m_id); i++)  {" << endl;          
          sstr << "    m_" << var->cIdent() << "_vec[i]->recordCacheContents(tr);\n";
          sstr << "  }\n";
        }
      }
    }
    sstr << "}" << endl;

    conditionally_write_file(path + "/Chip.C", sstr);
  }
}

Vector<StateMachine*> SymbolTable::getStateMachines() const
{
  Vector<StateMachine*> machine_vec;
  int size = m_sym_vec.size();
  for(int i=0; i<size; i++) {
    StateMachine* type = dynamic_cast<StateMachine*>(m_sym_vec[i]);
    if (type != NULL) {
      machine_vec.insertAtBottom(type);
    }
  }
  return machine_vec;
}

void SymbolTable::writeHTMLFiles(string path) const
{
  // Create index.html
  {
    ostringstream out;
    createHTMLindex(path, out);
    conditionally_write_file(path + "index.html", out);
  }

  // Create empty.html
  {
    ostringstream out;
    out << "<HTML></HTML>";
    conditionally_write_file(path + "empty.html", out);
  }

  // Write all the symbols
  int size = m_sym_vec.size();
  for(int i=0; i<size; i++) {
    m_sym_vec[i]->writeHTMLFiles(path);
  }
}

void write_file(string filename, ostringstream& sstr) 
{
  ofstream out;

  out.open(filename.c_str());
  out << sstr.str();
  out.close();
}

void SymbolTable::writeMIFFiles(string path) const
{
  int size = m_sym_vec.size();
  for(int i=0; i<size; i++) {
    ostringstream states, events, actions, transitions;
    StateMachine* machine = dynamic_cast<StateMachine*>(m_sym_vec[i]);
    if (machine != NULL) {
      printStateTableMIF(*machine, states);
      write_file(path + machine->getIdent() + "_states.mif", states);
      printEventTableMIF(*machine, events);
      write_file(path + machine->getIdent() + "_events.mif", events);
      printActionTableMIF(*machine, actions);
      write_file(path + machine->getIdent() + "_actions.mif", actions);
      printTransitionTableMIF(*machine, transitions);
      write_file(path + machine->getIdent() + "_transitions.mif", transitions);
    }
  }
}


void SymbolTable::print(ostream& out) const
{
  out << "[SymbolTable]";  // FIXME
}
