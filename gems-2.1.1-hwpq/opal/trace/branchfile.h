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
#ifndef _BRANCHFILE_H_
#define _BRANCHFILE_H_

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include "tracefile.h"

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

/// magic number for processor log files (PC plus data)
const int BRTRACE_MAGIC_NUM = 0x62902200;

//  Trace file control instructions:
///         1st bit is reserved for "taken/not taken" from previous branch
const char BRTRACE_TAKEN         = (char)    1;
///         2nd bit is reserved to indicate trap level at this time
const char BRTRACE_TRAPLEVEL     = (char)    2;
///         3rd bit is reserved to indicate if this branch was interrupted
const char BRTRACE_INTERRUPT     = (char)    4;

/// mask off the taken, traplevel bits.
const char BRTRACE_BITMASK       = (char)   ~7;
/// trace file token for observing branches
const char BRTRACE_BRANCH        = (char)    8;
/// trace file token for observing new branches
const char BRTRACE_NEW_BRANCH    = (char)   16;
/// trace file token for observing context switches
const char BRTRACE_CTX_SWITCH    = (char)   24;
/// switch to the next file
const char BRTRACE_FILE_SWITCH   = (char)   32;


/*------------------------------------------------------------------------*/
/* Class declaration(s)                                                   */
/*------------------------------------------------------------------------*/

/**
 * structure for returning traced branches
 */

typedef struct branch_record {
  /// virtual address of this branch
  my_addr_t        vpc;
  /// physical program counter: filled in only if type == BRTRACE_NEW_BRANCH
  uint64           ppc;
  /// instruction: filled in only if type == BRTRACE_NEW_BRANCH
  uint32           instr;
  /// is this branch taken or not: 1 == taken, 0 == not
  char             result;
  /// was the trap level set or not: 1 == tl set, 0 == not
  char             tlset;
  /// was the this branch interrupted: 1 == interrupted, 0 == not
  char             interrupted;
  /// type of this record
  char             rectype;
} branch_record_t;


/**
* Interface to a binary file which contains a branch trace.
*
* This file currently represents a uniprocessor trace -- only branches
* are represented in the trace. The format is:
*     Magic Number
*     BRTRACE_CTX_SWITCH
*     Initial Context
*     A sequence of control bytes & data:
*
*     (1) BRTRACE_NEW_BRANCH
*     uint32:       branch virtual address
*     uint64:       branch physical address
*     uint32:       branch opcode
*
*     (2) BRTRACE_BRANCH
*     uint32:       branch virtual address
*
*     (3) BRTRACE_CTX_SWITCH
*     uint32:       context
*
* @author  cmauer
* @version $Id: branchfile.h 1.5 03/07/10 18:07:50-00:00 milo@cottons.cs.wisc.edu $
*/

class branchfile_t {

public:

  /**
   * @name Constructor(s) / destructor
   */
  //@{

  /**
   * Constructor: opens a trace file for reading
   * @param branchfileName The name of the file be read
   * @param context_t      A pointer to the current context of this processor
   * @param startIndex     The file index to start at
   */
  branchfile_t( char *traceFileName, uint32 *context_p, uint32 startIndex );

  /**
   * Constructor: opens a trace file for writing
   * @param traceFileName The name of the file be written
   * @param context       The current context of the processor being traced
   */
  branchfile_t( char *traceFileName, uint32 context );
  
  /**
   * Destructor: frees object.
   */
  ~branchfile_t();
  //@}

  /**
   * @name Methods
   */
  //@{

  /** Write a branch into the branch trace
   *  @return true if the write succeeds, false if not
   */
  bool   writeBranch( my_addr_t curPC, my_addr_t curNPC, bool traplevel,
                      static_inst_t *s_instr );

  /** write the PC and any difference between the current state and this state
   *  @return true if the write succeeds, false if not
   */
  bool   writeChangedBranch( my_addr_t curPC, my_addr_t curNPC,
                             pa_t curPPC, bool traplevel,
                             static_inst_t *s_instr );

  /** check the last branch to see if it was taken or not taken */
  void   checkLastBranch( my_addr_t curPC, my_addr_t curNPC ) {
    if (m_check_branch) {
      if ( curPC == m_check_taken_pc &&
           curNPC == m_check_taken_npc ) {
        m_last_taken = true;
      } else if ( curPC == m_check_nottaken_pc &&
                  curNPC == m_check_nottaken_npc ) {
        m_last_taken = false;
      } else if ( curPC == 0x10000c00 ||
                  (curPC >= 0x10000820 && curPC <= 0x100009e0) ||
                  (curPC >= 0x10004820 && curPC <= 0x100049e0) ) {
        m_last_interrupt = true;
      } else {
        DEBUG_OUT("  checklastbranch: unknown resolution:\n");
        DEBUG_OUT("  expected taken:  PC: 0x%0llx  NPC: 0x%0llx\n",
                  m_check_taken_pc, m_check_taken_npc );
        DEBUG_OUT("  expected nttn :  PC: 0x%0llx  NPC: 0x%0llx\n",
                  m_check_nottaken_pc, m_check_nottaken_npc );
        DEBUG_OUT("  actual        :  PC: 0x%0llx  NPC: 0x%0llx\n",
                  curPC, curNPC );
      }
      m_check_branch = false;
    }
  }
  
  /** write out the new context to the file
   *  @param new_context The context identifier being switched to.
   */
  bool   writeContextSwitch( uint32 new_context );

  /** read one branch record from the branch trace */
  bool   readBranch( branch_record_t *rec );

  /** switch from writing one file to the next file in a sequence of files */
  bool   writeNextFile( void );

  /** debugging: validate the contents of a trace file
   */
  bool   validate( void );

  /** print out the branch record */
  void   print( branch_record_t *record );

  /** returns true if the trace is being read
   *  @return bool true if the trace is being read
   */
  bool   isReading( void ) {
    return (m_isReading);
  }

  /** returns false if the trace is being written
   *  @return bool false if the trace is being writting
   */
  bool   isWriting( void ) {
    return (!m_isReading);
  }

  /** returns true when the trace is finished
   *  @return bool true if the trace is finished reading
   */
  bool   isFinished( void ) {
    return (feof(m_fp) != 0);
  }

  //@}

private:

  /// open the next file in a sequence of files for writing
  void    writeFile( void );

  /// open the next file in a sequence of files for reading
  void    readFile( void );

  /// close the currently open file
  void    closeFile( void );
  
  /// The current trace file (being read or written)
  FILE                    *m_fp;

  /// The index in a sequential set of files to be read/written
  int32                    m_fp_index;

  /// The name of the trace file
  char                    *m_filename;

  /// The base filename for a sequence of files
  char                    *m_base_filename;

  /// The current context: used only for writes
  uint32                   m_current_context;
  /// A pointer to the current context (on read, we will modify this context)
  uint32                  *m_context;

  /// True if the last instruction was a branch
  bool                     m_check_branch;
  /// True the the last branch was taken
  bool                     m_last_taken;
  /// True the the last branch was interrupted
  bool                     m_last_interrupt;
  /// The PC, NPC if the last branch is taken
  my_addr_t                m_check_taken_pc;
  my_addr_t                m_check_taken_npc;
  /// The PC, NPC if the last branch is not taken
  my_addr_t                m_check_nottaken_pc;
  my_addr_t                m_check_nottaken_npc;
  
  /// The state of the last instruction read from the trace: BRTRACE_ constants
  char                     m_last_token;

  /// Indicates if this trace is being read or written
  bool                     m_isReading;
};

/*------------------------------------------------------------------------*/
/* Global variables                                                       */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Global functions                                                       */
/*------------------------------------------------------------------------*/

#endif  /* _BRANCHFILE_H_ */
