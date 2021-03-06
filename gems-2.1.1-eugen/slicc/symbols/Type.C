
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
 * Type.C
 * 
 * Description: See Type.h
 *
 * $Id$
 * */

#include "Type.h"
#include "fileio.h"
#include "Map.h"
#include "StateMachine.h"

Type::Type(string id, const Location& location, 
           const Map<string, string>& pairs, 
           StateMachine* machine_ptr)
  : Symbol(id, location, pairs)
{
  if (machine_ptr == NULL) {
    m_c_id = id;
  } else if (isExternal() || isPrimitive()) { 
    if (existPair("external_name")) {
      m_c_id = lookupPair("external_name");
    } else {
      m_c_id = id;
    }
  } else {
    m_c_id = machine_ptr->toString() + "_" + id;  // Append with machine name
  }
  
  if(existPair("desc")){
    m_desc = lookupPair("desc");
  } else {
    m_desc = "No description avaliable";
  }

  // check for interface that this Type implements
  if(existPair("interface")) {
    string interface = lookupPair("interface");
    if(interface == "Message" || interface == "NetworkMessage") {
      addPair("message", "yes");
    }
    if(interface == "NetworkMessage") {
      addPair("networkmessage", "yes");
    }
  }

  // FIXME - all of the following id comparisons are fragile hacks
  if ((getIdent() == "CacheMemory") || (getIdent() == "NewCacheMemory") ||
      (getIdent() == "TLCCacheMemory") || (getIdent() == "DNUCACacheMemory") ||
      (getIdent() == "DNUCABankCacheMemory") || (getIdent() == "L2BankCacheMemory") ||
      (getIdent() == "CompressedCacheMemory") || (getIdent() == "PrefetchCacheMemory")) {
    addPair("cache", "yes");
  }

  if ((getIdent() == "TBETable") || (getIdent() == "DNUCATBETable") || (getIdent() == "DNUCAStopTable")) {
    addPair("tbe", "yes");
  }

  if ((getIdent() == "NewTBETable")) {
    addPair("newtbe", "yes");
  }

  if ((getIdent() == "TimerTable")) {
    addPair("timer", "yes");
  }

  if ((getIdent() == "DirectoryMemory")) {
    addPair("dir", "yes");
  }

  if ((getIdent() == "PersistentTable")) {
    addPair("persistent", "yes");
  }

  if ((getIdent() == "Prefetcher")) {
    addPair("prefetcher", "yes");
  }

  if ((getIdent() == "DNUCA_Movement")) {
    addPair("mover", "yes");
  }

  if (id == "MachineType") {
    m_isMachineType = true;
  } else {
    m_isMachineType = false;
  }
}

// Return false on error
bool Type::dataMemberAdd(string id, Type* type_ptr, Map<string, string>& pairs,
                         string* init_code)
{
  if (dataMemberExist(id)) {
    return false; // Error
  } else {
    m_data_member_map.add(id, type_ptr);
    m_data_member_ident_vec.insertAtBottom(id);
    m_data_member_type_vec.insertAtBottom(type_ptr);
    m_data_member_pairs_vec.insertAtBottom(pairs);
    m_data_member_init_code_vec.insertAtBottom(init_code);
  }

  return true;
}

string Type::methodId(string name, 
                      const Vector<Type*>& param_type_vec)
{
  string paramStr = "";
  for (int i = 0; i < param_type_vec.size(); i++) {
    paramStr += "_"+param_type_vec[i]->cIdent();
  }
  return name+paramStr;
}

bool Type::methodAdd(string name, 
                     Type* return_type_ptr, 
                     const Vector<Type*>& param_type_vec)
{
  string id = methodId(name, param_type_vec);
  if (methodExist(id)) {
    return false; // Error
  } else {
    m_method_return_type_map.add(id, return_type_ptr);
    m_method_param_type_map.add(id, param_type_vec);
    return true;
  }  
}

bool Type::enumAdd(string id, Map<string, string> pairs_map)
{
  if (enumExist(id)) {
    return false;
  } else {
    m_enum_map.add(id, true);
    m_enum_vec.insertAtBottom(id);
    m_enum_pairs.insertAtBottom(pairs_map);

    // Add default
    if (!existPair("default")) {
      addPair("default", cIdent()+"_NUM");
    }

    return true;
  }
}

void Type::writeCFiles(string path) const
{
  if (isExternal()) {
    // Do nothing
  } else if (isEnumeration()) {
    printEnumH(path);
    printEnumC(path);
  } else { // User defined structs and messages
    printTypeH(path);
    printTypeC(path);
  }
}

void Type::printTypeH(string path) const
{
  ostringstream out;
  int size = m_data_member_type_vec.size();
  string type_name = cIdent();  // Identifier for the type in C

  // Header
  out << "/** \\file " << type_name << ".h" << endl;
  out << "  * " << endl;
  out << "  * Auto generated C++ code started by "<<__FILE__<<":"<<__LINE__<< endl;
  out << "  */" << endl;
  out << endl;
  out << "#ifndef " << type_name << "_H" << endl;
  out << "#define " << type_name << "_H" << endl;
  out << endl;

  // Include all of the #includes needed
  out << "#include \"Global.h\"" << endl;
  out << "#include \"Allocator.h\"" << endl;
  for (int i=0; i < size; i++) {
    Type* type = m_data_member_type_vec[i];
    if (!type->isPrimitive()) {
      out << "#include \"" << type->cIdent() << ".h" << "\"" << endl;
    }
  }
  string interface = "";
  if(existPair("interface")) {
    interface = lookupPair("interface");
    out << "#include \"" << interface << ".h\"" << endl;
  }

  // Class definition
  out << "class " << type_name;

  if(interface != "") {
    out << " :  public " << interface ;
  }

  out << " {" << endl;
  out << "public:" << endl;

  // ******** Default constructor ********

  out << "  " << type_name << "() " << endl;

  // Call superclass constructor
  if (interface != "") {
      out << "  : " << interface << "()" << endl;
  }

  out << "  {" << endl;
  
  if(!isGlobal()) {
    for (int i=0; i < size; i++) {
      
      Type* type_ptr = m_data_member_type_vec[i];
      string id = m_data_member_ident_vec[i];
      if (m_data_member_pairs_vec[i].exist("default")) {
        // look for default value
        string default_value = m_data_member_pairs_vec[i].lookup("default");
        out << "    m_" << id << " = " << default_value << "; // default for this field " << endl;
      } else if (type_ptr->hasDefault()) {
        // Look for the type default
        string default_value = type_ptr->getDefault();
        out << "    m_" << id << " = " << default_value << "; // default value of " << type_ptr->cIdent() << endl;
      } else {
        out << "    // m_" << id << " has no default" << endl;
      }
    }
  } // end of if(!isGlobal())
  out << "  }" << endl;
  
  // ******** Default destructor ********
  out << "  ";
  out << "~" << type_name << "() { };" << endl;
  
  // ******** Full init constructor ********
  if(! isGlobal()) {
    out << "  " << type_name << "(";
    
    for (int i=0; i < size; i++) {
      if (i != 0) {
        out << ", ";
      }
      Type* type = m_data_member_type_vec[i];
      string id = m_data_member_ident_vec[i];
      out << "const " << type->cIdent() << "& local_" << id;
    }
    out << ")" << endl;
  
    // Call superclass constructor
    if (interface != "") {
      out << "  : " << interface << "()" << endl;
    }

    out << "  {" << endl;
    for (int i=0; i < size; i++) {
      Type* type_ptr = m_data_member_type_vec[i];
      string id = m_data_member_ident_vec[i];
      out << "    m_" << id << " = local_" << id << ";" << endl;
      if (m_data_member_pairs_vec[i].exist("nextLineCallHack")) {
        string next_line_value = m_data_member_pairs_vec[i].lookup("nextLineCallHack");        
        out << "    m_" << id << next_line_value << ";" << endl;
      }

    }
    out << "  }" << endl;
  } // end of if(!isGlobal())

  // bobba - 
  //******** Partial init constructor ********
  //**  Constructor needs only the first n-1 data members for init
  //** HACK to create objects with partially specified data members
  //** Need to get rid of this and use hierarchy instead
//   if(! isGlobal()) {
//     out << "  " << type_name << "(";
    
//     for (int i=0; i < size-1; i++) {
//       if (i != 0) {
//         out << ", ";
//       }
//       Type* type = m_data_member_type_vec[i];
//       string id = m_data_member_ident_vec[i];
//       out << "const " << type->cIdent() << "& local_" << id;
//     }
//     out << ")" << endl;
  
//     // Call superclass constructor
//     if (interface != "") {
//       out << "  : " << interface << "()" << endl;
//     }

//     out << "  {" << endl;
//     for (int i=0; i < size-1; i++) {
//       Type* type_ptr = m_data_member_type_vec[i];
//       string id = m_data_member_ident_vec[i];
//       out << "    m_" << id << " = local_" << id << ";" << endl;
//       if (m_data_member_pairs_vec[i].exist("nextLineCallHack")) {
//         string next_line_value = m_data_member_pairs_vec[i].lookup("nextLineCallHack");        
//         out << "    m_" << id << next_line_value << ";" << endl;
//       }

//     }
//     out << "  }" << endl;
//   } // end of if(!isGlobal())
  
  // ******** Message member functions ********
  // FIXME: those should be moved into slicc file, slicc should support more of
  // the c++ class inheritance

  if (isMessage()) {
    out << "  Message* clone() const { checkAllocator(); return s_allocator_ptr->allocate(*this); }" << endl;
    out << "  void destroy() { checkAllocator(); s_allocator_ptr->deallocate(this); }" << endl;
    out << "  static Allocator<" << type_name << ">* s_allocator_ptr;" << endl;
    out << "  static void checkAllocator() { if (s_allocator_ptr == NULL) { s_allocator_ptr = new Allocator<" << type_name << ">; }}" << endl;
  }

  if(!isGlobal()) {
    // const Get methods for each field
    out << "  // Const accessors methods for each field" << endl;
    for (int i=0; i < size; i++) {
      Type* type_ptr = m_data_member_type_vec[i];
      string type = type_ptr->cIdent();
      string id = m_data_member_ident_vec[i];
      out << "/** \\brief Const accessor method for " << id << " field." << endl;
      out << "  * \\return " << id << " field" << endl;
      out << "  */" << endl;
      out << "  const " << type << "& get" << id 
          << "() const { return m_" << id << "; }" << endl;
    }
    
    out << endl;
    
    // Non-const Get methods for each field
    out << "  // Non const Accessors methods for each field" << endl;
    for (int i=0; i < size; i++) {
      Type* type_ptr = m_data_member_type_vec[i];
      string type = type_ptr->cIdent();
      string id = m_data_member_ident_vec[i];
      out << "/** \\brief Non-const accessor method for " << id << " field." << endl;
      out << "  * \\return " << id << " field" << endl;
      out << "  */" << endl;
      out << "  " << type << "& get" << id 
          << "() { return m_" << id << "; }" << endl;
    }
    
    out << endl;
    
    // Set methods for each field
    out << "  // Mutator methods for each field" << endl;
    for (int i=0; i < size; i++) {
      Type* type_ptr = m_data_member_type_vec[i];
      string type = type_ptr->cIdent();
      string id = m_data_member_ident_vec[i];
      out << "/** \\brief Mutator method for " << id << " field */" << endl;
      out << "  void set" << id << "(const " << type << "& local_" 
          << id << ") { m_" << id << " = local_" << id << "; }" << endl;
    }

    out << endl;
  } // end of if(!isGlobal())
  
  out << "  void print(ostream& out) const;" << endl;
  out << "//private:" << endl;

  // Data members for each field
  for (int i=0; i < size; i++) {
    if (!m_data_member_pairs_vec[i].exist("abstract")) {
      out << "  ";
      // global structure
      if(isGlobal()) out << "static const ";
      
      Type* type = m_data_member_type_vec[i];
      string id = m_data_member_ident_vec[i];
      out << type->cIdent() << " m_" << id;
      
      // init value
      string* init_code = m_data_member_init_code_vec[i];
      if(init_code) {
        // only global structure can have init value here
        assert(isGlobal());
        out << " = " << *init_code << " ";
      }
      out << ";";
      if (m_data_member_pairs_vec[i].exist("desc")) {
        string desc = m_data_member_pairs_vec[i].lookup("desc");
        out << " /**< " << desc << "*/";
      }
      out << endl;
    }
  }

  out << "};" << endl;  // End class 

  out << "// Output operator declaration" << endl;
  out << "ostream& operator<<(ostream& out, const " << type_name << "& obj);" << endl;
  out << endl;
  out << "// Output operator definition" << endl;
  out << "extern inline" << endl;
  out << "ostream& operator<<(ostream& out, const " << type_name << "& obj)" << endl;
  out << "{" << endl;
  out << "  obj.print(out);" << endl;
  out << "  out << flush;" << endl;
  out << "  return out;" << endl;
  out << "}" << endl;
  out << endl;
  out << "#endif // " << type_name << "_H" << endl;
  
  // Write it out
  conditionally_write_file(path + type_name + ".h", out);
}

void Type::printTypeC(string path) const
{
  ostringstream out;
  int size = m_data_member_type_vec.size();
  string type_name = cIdent();  // Identifier for the type in C

  // Header
  out << "/** \\file " << type_name << ".C" << endl;
  out << "  * " << endl;
  out << "  * Auto generated C++ code started by "<<__FILE__<<":"<<__LINE__<< endl;
  out << "  */" << endl;
  out << endl;
  out << "#include \"" << type_name << ".h\"" << endl;
  out << endl;
  if (isMessage()) {
    out << "Allocator<" << type_name << ">* " << type_name << "::s_allocator_ptr = NULL;" << endl;
  }
  out << "/** \\brief Print the state of this object */" << endl;
  out << "void " << type_name << "::print(ostream& out) const" << endl;
  out << "{" << endl;
  out << "  out << \"[" << type_name << ": \";" << endl;

  // For each field
  for (int i=0; i < size; i++) {
    string id = m_data_member_ident_vec[i];
    out << "  out << \"" << id << "=\" << m_" << id << " << \" \";" << endl;
  }

  if (isMessage()) {
    out << "  out << \"" << "Time" << "=\" << getTime()" <<  " << \" \";" << endl;
  }

  // Trailer
  out << "  out << \"]\";" << endl;
  out << "}" << endl;

  // Write it out
  conditionally_write_file(path + type_name + ".C", out);
}

void Type::printEnumH(string path) const
{
  ostringstream out;
  int size = m_enum_vec.size();
  string type_name = cIdent();  // Identifier for the type in C
  string type_desc = desc();

  // Header
  out << "/** \\file " << type_name << ".h" << endl;
  out << "  * " << endl;
  out << "  * Auto generated C++ code started by "<<__FILE__<<":"<<__LINE__<< endl;
  out << "  */" << endl;

  out << "#ifndef " << type_name << "_H" << endl;
  out << "#define " << type_name << "_H" << endl;
  out << endl;
  // Include all of the #includes needed
  out << "#include \"Global.h\"" << endl;
  if (m_isMachineType) {
    out << "#include \"RubyConfig.h\"" << endl << endl;
  }
  out << endl;

  // Class definition
  out << "/** \\enum " << type_name << endl;
  out << "  * \\brief " << type_desc << endl;
  out << "  */" << endl;
  out << "enum " << type_name << " {" << endl;

  out << "  " << type_name << "_FIRST," << endl;

  // For each field
  for(int i = 0; i < size; i++ ) {
    string id = m_enum_vec[i];
    string description;
    if(m_enum_pairs[i].exist("desc")){
      description = m_enum_pairs[i].lookup("desc");
    } else {
      description = "No description avaliable";
    }
    if (i == 0) {
      out << "  " << type_name << "_" << id << " = " << type_name << "_FIRST,  /**< " << description << " */" << endl;
    }
    else {
      out << "  " << type_name << "_" << id << ",  /**< " << description << " */" << endl;
    }
  }
  out << "  " << type_name << "_NUM" << endl;
  out << "};" << endl;

  // Code to convert from a string to the enumeration
  out << type_name << " string_to_" << type_name << "(const string& str);" << endl;

  // Code to convert state to a string
  out << "string " << type_name << "_to_string(const " << type_name << "& obj);" << endl;

  // Code to increment an enumeration type
  out << type_name << " &operator++( " << type_name << " &e);" << endl;

  // MachineType hack used to set the base component id for each Machine
  if (m_isMachineType) {
    out << "int " << type_name << "_base_level(const " << type_name << "& obj);" << endl;
    out << "int " << type_name << "_base_number(const " << type_name << "& obj);" << endl;
    out << "int " << type_name << "_base_count(const " << type_name << "& obj);" << endl;
    out << "int " << type_name << "_chip_count(const " << type_name << "& obj, NodeID chipID);" << endl;

    for(int i = 0; i < size; i++ ) {
      string id = m_enum_vec[i];
      out << "#define MACHINETYPE_" << id << " 1" << endl;
    }
    cout << endl;
  }

  // Trailer
  out << "ostream& operator<<(ostream& out, const " << type_name << "& obj);" << endl;
  out << endl;
  out << "#endif // " << type_name << "_H" << endl;

  // Write the file
  conditionally_write_file(path + type_name + ".h", out);
}

void Type::printEnumC(string path) const
{
  ostringstream out;
  int size = m_enum_vec.size();
  string type_name = cIdent();  // Identifier for the type in C

  // Header
  out << "/** \\file " << type_name << ".h" << endl;
  out << "  * " << endl;
  out << "  * Auto generated C++ code started by "<<__FILE__<<":"<<__LINE__<< endl;
  out << "  */" << endl;

  out << endl;
  out << "#include \"" << type_name << ".h\"" << endl;
  out << endl;

  // Code for output operator
  out << "ostream& operator<<(ostream& out, const " << type_name << "& obj)" << endl;
  out << "{" << endl;
  out << "  out << " << type_name << "_to_string(obj);" << endl;
  out << "  out << flush;" << endl;
  out << "  return out;" << endl;
  out << "}" << endl;

  // Code to convert state to a string  
  out << endl;
  out << "string " << type_name << "_to_string(const " << type_name << "& obj)" << endl;
  out << "{" << endl;
  out << "  switch(obj) {" << endl;
  
  // For each field
  for( int i = 0; i<size; i++ ) {
    out << "  case " << type_name << "_" << m_enum_vec[i] << ":" << endl;
    out << "    return \"" << m_enum_vec[i] << "\";" << endl;
  }

  // Trailer
  out << "  default:" << endl;
  out << "    ERROR_MSG(\"Invalid range for type " << type_name << "\");" << endl;
  out << "    return \"\";" << endl;
  out << "  }" << endl;
  out << "}" << endl;

  // Code to convert from a string to the enumeration
  out << endl;
  out << type_name << " string_to_" << type_name << "(const string& str)" << endl;
  out << "{" << endl;
  out << "  if (false) {" << endl;

  // For each field
  for( int i = 0; i<size; i++ ) {
    out << "  } else if (str == \"" << m_enum_vec[i] << "\") {" << endl;
    out << "    return " << type_name << "_" << m_enum_vec[i] << ";" << endl;
  }

  out << "  } else {" << endl;
  out << "    WARN_EXPR(str);" << endl;
  out << "    ERROR_MSG(\"Invalid string conversion for type " << type_name << "\");" << endl;
  out << "  }" << endl;
  out << "}" << endl;

  // Code to increment an enumeration type
  out << endl;
  out << type_name << "& operator++( " << type_name << "& e) {" << endl;
  out << "  assert(e < " << type_name << "_NUM);" << endl;
  out << "  return e = " << type_name << "(e+1);" << endl; 
  out << "}" << endl;

  // MachineType hack used to set the base level and number of components for each Machine
  if (m_isMachineType) {
    out << endl;
    out << "/** \\brief returns the base vector index for each machine type to be used by NetDest " << endl;
    out << "  * " << endl;
    out << "  * \\return the base vector index for each machine type to be used by NetDest" << endl;
    out << "  * \\see NetDest.h" << endl;
    out << "  */" << endl;
    out << "int " << type_name << "_base_level(const " << type_name << "& obj)" << endl;
    out << "{" << endl;
    out << "  switch(obj) {" << endl;
    
    // For each field
    Vector < string > MachineNames;
    for( int i = 0; i<size; i++ ) {
      out << "  case " << type_name << "_" << m_enum_vec[i] << ":" << endl;
      out << "    return " << MachineNames.size() << ";" << endl;
      MachineNames.insertAtBottom(m_enum_vec[i]);
    }
    
    // total num
    out << "  case " << type_name << "_NUM:" << endl;
    out << "    return " << MachineNames.size() << ";" << endl;
    
    // Trailer
    out << "  default:" << endl;
    out << "    ERROR_MSG(\"Invalid range for type " << type_name << "\");" << endl;
    out << "    return -1;" << endl;
    out << "  }" << endl;
    out << "}" << endl;
    
    out << endl;
    out << "/** \\brief The return value indicates the number of components created" << endl;
    out << "  * before a particular machine's components" << endl;
    out << "  * " << endl;
    out << "  * \\return the base number of components for each machine" << endl;
    out << "  */" << endl;
    out << "int " << type_name << "_base_number(const " << type_name << "& obj)" << endl;
    out << "{" << endl;
    out << "  switch(obj) {" << endl;
    
    // For each field
    MachineNames.clear();
    for( int i = 0; i<size; i++ ) {
      out << "  case " << type_name << "_" << m_enum_vec[i] << ":" << endl;
      out << "    return 0";
      for ( int m = 0; m<MachineNames.size(); m++) {
        out << "+RubyConfig::numberOf" << MachineNames[m] << "()";
      }
      out << ";" << endl;
      MachineNames.insertAtBottom(m_enum_vec[i]);
    }
    
    // total num
    out << "  case " << type_name << "_NUM:" << endl;
    out << "    return 0";
    for ( int m = 0; m<MachineNames.size(); m++) {
      out << "+RubyConfig::numberOf" << MachineNames[m] << "()";
    }
    out << ";" << endl;
    
    // Trailer
    out << "  default:" << endl;
    out << "    ERROR_MSG(\"Invalid range for type " << type_name << "\");" << endl;
    out << "    return -1;" << endl;
    out << "  }" << endl;
    out << "}" << endl;


    out << endl;
    out << "/** \\brief returns the total number of components for each machine" << endl;
    out << "  * \\return the total number of components for each machine" << endl;
    out << "  */" << endl;
    out << "int " << type_name << "_base_count(const " << type_name << "& obj)" << endl;
    out << "{" << endl;
    out << "  switch(obj) {" << endl;
    
    // For each field
    for( int i = 0; i<size; i++ ) {
      out << "  case " << type_name << "_" << m_enum_vec[i] << ":" << endl;
      out << "    return RubyConfig::numberOf" << m_enum_vec[i] << "();" << endl;
    }
    
    // total num
    out << "  case " << type_name << "_NUM:" << endl;
    // Trailer
    out << "  default:" << endl;
    out << "    ERROR_MSG(\"Invalid range for type " << type_name << "\");" << endl;
    out << "    return -1;" << endl;
    out << "  }" << endl;
    out << "}" << endl;

    out << endl;
    out << "/** \\brief returns the total number of components for each machine" << endl;
    out << "  * \\return the total number of components for each machine" << endl;
    out << "  */" << endl;
    out << "int " << type_name << "_chip_count(const " << type_name << "& obj, NodeID chipID)" << endl;
    out << "{" << endl;
    out << "  switch(obj) {" << endl;
    
    // For each field
    for( int i = 0; i<size; i++ ) {
      out << "  case " << type_name << "_" << m_enum_vec[i] << ":" << endl;
      out << "    return RubyConfig::numberOf" << m_enum_vec[i] << "PerChip(chipID);" << endl;
    }
    
    // total num
    out << "  case " << type_name << "_NUM:" << endl;
    // Trailer
    out << "  default:" << endl;
    out << "    ERROR_MSG(\"Invalid range for type " << type_name << "\");" << endl;
    out << "    return -1;" << endl;
    out << "  }" << endl;
    out << "}" << endl;


  }

  // Write the file
  conditionally_write_file(path + type_name + ".C", out);
}
