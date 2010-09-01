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
   This file has been modified by Kevin Moore and Dan Nussbaum of the
   Scalable Systems Research Group at Sun Microsystems Laboratories
   (http://research.sun.com/scalable/) to support the Adaptive
   Transactional Memory Test Platform (ATMTP).

   Please send email to atmtp-interest@sun.com with feedback, questions, or
   to request future announcements about ATMTP.

   ----------------------------------------------------------------------

   File modification date: 2008-02-23

   ----------------------------------------------------------------------
*/

#include "RegisterStateWindowed.h"
#include "interface.h"

RegisterState::RegisterState(int cpuNumber)
{
  m_proc = cpuNumber;
  if(g_SIMICS){
    //mmu_obj_str[20];
    m_v9_interface = (sparc_v9_interface_t *) SIM_get_interface(SIM_proc_no_2_ptr(m_proc), SPARC_V9_INTERFACE);
    //sprintf(mmu_obj_str, "chmmu%d", m_proc);
    //m_mmu_interface = (mmu_t *) SIM_get_interface(SIM_get_object(m_proc), MMU_INTERFACE);
    int i;
    for(i=0; i < 126 ; i++){
      if((i <= 31) || 
         // Don't save/restore tick/stick OR tick/stick_cmpr 
         //
         //- (i == 39 || (i == 43)) ||  // tick/stick
         (i == 39) || (i == 43) ||  // tick/stick
         (i == 41) || (i == 44) ||  // tick_cmpr/stick_cmpr
         (i == 45) ||               // pstate                 
         (i >= 53 && i <= 57) ||    // invalid
         (i >= 63 && i <= 67) ||    // invalid
         (i >= 73 && i <= 77) ||    // invalid
         (i >= 83 && i <= 87) ||    // invalid
         //(i >= 91 && i <= 95) ||    // window state
         (i == 96) ||               // softint
         (i >= 100 && i <= 110) || // interrupt status (just added)
         (i >= 111 && i <= 119))   // interrupt address
        {
          continue;
        }
      m_controlRegisterNumbers.insertAtBottom(i);
    }

    for(i=0; i < 8; i++) {
      m_iRegisterNumbers.insertAtBottom(i+24);
      m_lRegisterNumbers.insertAtBottom(i+16);
      m_oRegisterNumbers.insertAtBottom(i+8);
    }

    for(i=0; i < 8; i++) {
      m_gRegisterNumbers.insertAtBottom(i);
    }

    for(i=0; i < 64; i=i+2) {
      m_fpRegisterNumbers.insertAtBottom(i);
    }

    for(i=0; i < m_controlRegisterNumbers.size(); i++) {
      m_controlRegisters.allocate(m_controlRegisterNumbers[i]);
    }

    for(int window=0; window < NUM_WINDOWS; window++) {
      for(i=0; i < m_iRegisterNumbers.size(); i++) {
        m_iRegisters[window].allocate(m_iRegisterNumbers[i]);
        m_lRegisters[window].allocate(m_lRegisterNumbers[i]);
        m_oRegisters[window].allocate(m_oRegisterNumbers[i]);
      }
    }

    for(int set=0; set < NUM_GLOBAL_SET; set++) {
      for(i=0; i < m_gRegisterNumbers.size(); i++) {
        m_gRegisters[set].allocate(m_gRegisterNumbers[i]);
      }
    }
  
    for(i=0; i < m_fpRegisterNumbers.size(); i++) {
      m_fpRegisters.allocate(m_fpRegisterNumbers[i]);
    }
  }
}

RegisterState::~RegisterState()
{
  if(g_SIMICS){
    int i;
    for(i=0; i < m_controlRegisterNumbers.size(); i++) {
      m_controlRegisters.deallocate(m_controlRegisterNumbers[i]);
    }

    for(int window=0; window < NUM_WINDOWS; window++) {
      for(i=0; i < m_iRegisterNumbers.size(); i++) {
        m_iRegisters[window].deallocate(m_iRegisterNumbers[i]);
        m_lRegisters[window].deallocate(m_lRegisterNumbers[i]);
        m_oRegisters[window].deallocate(m_oRegisterNumbers[i]);
      }
    }

    for(int set=0; set < NUM_GLOBAL_SET; set++) {
      for(i=0; i < m_gRegisterNumbers.size(); i++) {
        m_gRegisters[set].deallocate(m_gRegisterNumbers[i]);
      }
    }

    for(i=0; i < m_fpRegisterNumbers.size(); i++) {
      m_fpRegisters.deallocate(m_fpRegisterNumbers[i]);
    }

  }
}

void RegisterState::takeCheckpoint(int m_proc)
{
  assert(g_SIMICS);
  
  int i;
  int registerNumber;
  processor_t* cpu = SIM_proc_no_2_ptr(m_proc);
  uinteger_t reg_val;
  const char * reg_name;

  for(int i=0; i < m_controlRegisterNumbers.size(); i++) {
    registerNumber = m_controlRegisterNumbers[i];
    reg_name = SIMICS_get_register_name(m_proc, registerNumber);
    reg_val= SIMICS_read_register(m_proc, registerNumber);
    //if(i >= 90 && i <= 95){
    //  cout << "  " << m_proc << " " << reg_name << " " << hex << reg_val << dec << endl;
    //}
    m_controlRegisters.lookup(registerNumber) = reg_val;
  }

  for(int window=0; window < NUM_WINDOWS; window++) {
    for(i=0; i < m_iRegisterNumbers.size(); i++) {
      registerNumber = m_iRegisterNumbers[i];
      m_iRegisters[window].lookup(registerNumber) = m_v9_interface->read_window_register(cpu, window, registerNumber);
      registerNumber = m_lRegisterNumbers[i];
      m_lRegisters[window].lookup(registerNumber) = m_v9_interface->read_window_register(cpu, window, registerNumber);
      registerNumber = m_oRegisterNumbers[i];
      m_oRegisters[window].lookup(registerNumber) = m_v9_interface->read_window_register(cpu, window, registerNumber);
    }
  }

  for(int set=0; set < NUM_GLOBAL_SET; set++) {
    for(i=0; i < m_gRegisterNumbers.size(); i++) {
      registerNumber = m_gRegisterNumbers[i];
      m_gRegisters[set].lookup(registerNumber) = m_v9_interface->read_global_register(cpu, set, registerNumber);
    }
  }

  for(i=0; i < m_fpRegisterNumbers.size(); i++) {
    registerNumber = m_fpRegisterNumbers[i];
    m_fpRegisters.lookup(registerNumber) = m_v9_interface->read_fp_register_x(cpu, registerNumber);
  }

  /*
  cout << m_proc << " TAKING CHECKPOINT: CWP " << SIMICS_read_register(m_proc, 95)
                                     << " CANSAVE " << SIMICS_read_register(m_proc, 91)
                                     << " CANRESTORE " << SIMICS_read_register(m_proc, 92)
                                     << " OTHERWIN " << SIMICS_read_register(m_proc, 93)
                                     << " CLEANWIN " << SIMICS_read_register(m_proc, 94)
                                     << endl;
   */                                  
}

void RegisterState::restoreCheckpoint(int m_proc)
{
  assert(g_SIMICS);

  int i;
  int registerNumber;
  uinteger_t value, reg_val;
  uint64 fpValue;
  conf_object_t* cpu = SIM_proc_no_2_ptr(m_proc);
  const char* reg_name;

  for(i=0; i < m_controlRegisterNumbers.size(); i++) {
    registerNumber = m_controlRegisterNumbers[i];
    value = m_controlRegisters.lookup(registerNumber);
    /* DONT RESTORE THE REGISTERS EXCLUDED */
    if(		 registerNumber != 32		/* PC */
				&& registerNumber != 33 	/* NPC */ 
				&& registerNumber != 93 	/* OTHERWIN */
				&& registerNumber != 97 	/* SAFARI_CONFIG */	
				&& registerNumber != 123 	/* MID */	) {
      reg_val= SIMICS_read_register(m_proc, registerNumber);
      SIMICS_write_register(m_proc, registerNumber, value);
      reg_name = SIMICS_get_register_name(m_proc, registerNumber);
//			cout << dec << registerNumber << " reg_name: " << reg_name << hex << " new_value: " << reg_val << " old_value: " << value;
//			cout << "  RESTORE ";
//			if (reg_val != value) {	cout << "***"; }
//			cout << endl;
//    } else {
//      reg_val= SIMICS_read_register(m_proc, registerNumber);
//      reg_name = SIMICS_get_register_name(m_proc, registerNumber);
//			cout << dec << registerNumber << " reg_name: " << reg_name << hex << " new_value: " << reg_val << " old_value: " << value;
//			cout << "  NO RESTORE " << endl;
		}
  }

  // KM -- check: cansave + canrestore + otherwin = 6
  // Try: restore canrestore, keep otherwin, cansave = 6 - (otherwin + canrestore) 
  //assert((m_controlRegisters.lookup(91) + 
  //        m_controlRegisters.lookup(92) + 
  //        m_controlRegisters.lookup(93)) == NUM_WINDOWS - 2); 
  uinteger_t canrestore = m_controlRegisters.lookup(92);
  uinteger_t otherwin = SIMICS_read_register(m_proc, 93);
  uinteger_t old_otherwin = m_controlRegisters.lookup(93);
  uinteger_t cansave = (NUM_WINDOWS - 2) - (canrestore + otherwin);
  uinteger_t old_cwp = m_controlRegisters.lookup(95);
  uinteger_t new_cwp = SIMICS_read_register(m_proc, 95);
  SIMICS_write_register(m_proc, 91, cansave);

  if (old_otherwin != otherwin || (otherwin != 0)){
    cout << m_proc << " RESTORING CHECKPOINT: CWP " << SIMICS_read_register(m_proc, 95)
                                     << " CANSAVE " << SIMICS_read_register(m_proc, 91)
                                     << " CANRESTORE " << SIMICS_read_register(m_proc, 92)
                                     << " OTHERWIN " << SIMICS_read_register(m_proc, 93)
                                     << " CLEANWIN " << SIMICS_read_register(m_proc, 94)
                                     << " OLD_CWP " << old_cwp
                                     << " OLD OTHERWIN " << old_otherwin
                                     << endl;
   assert(0);                                  
  }                                   

  // KM -- fix me, only restore valid windows.  Otherwise "other" windows
  // could be inconsistent.
  
  for(int window=0; window < NUM_WINDOWS; window++) {
    for(i=0; i < m_iRegisterNumbers.size(); i++) {
      registerNumber = m_iRegisterNumbers[i];
      value = m_iRegisters[window].lookup(registerNumber);
      m_v9_interface->write_window_register(cpu, window, registerNumber, value);
      registerNumber = m_lRegisterNumbers[i];
      value = m_lRegisters[window].lookup(registerNumber);
      m_v9_interface->write_window_register(cpu, window, registerNumber, value);
      registerNumber = m_oRegisterNumbers[i];
      value = m_oRegisters[window].lookup(registerNumber);
      m_v9_interface->write_window_register(cpu, window, registerNumber, value);
    }
  }
  
  
 /* 
    for(i=0; i < m_iRegisterNumbers.size(); i++) {
      registerNumber = m_iRegisterNumbers[i];
      value = m_iRegisters[old_cwp].lookup(registerNumber);
      m_v9_interface->write_window_register(cpu, new_cwp, registerNumber, value);
      registerNumber = m_lRegisterNumbers[i];
      value = m_lRegisters[old_cwp].lookup(registerNumber);
      m_v9_interface->write_window_register(cpu, new_cwp, registerNumber, value);
      registerNumber = m_oRegisterNumbers[i];
      value = m_oRegisters[old_cwp].lookup(registerNumber);
      m_v9_interface->write_window_register(cpu, new_cwp, registerNumber, value);
    }
   */ 
  
  for(int set=0; set < NUM_GLOBAL_SET; set++) {
    for(i=0; i < m_gRegisterNumbers.size(); i++) {
      registerNumber = m_gRegisterNumbers[i];
      value = m_gRegisters[set].lookup(registerNumber);
      m_v9_interface->write_global_register(cpu, set, registerNumber, value);
    }
  }

  for(i=0; i < m_fpRegisterNumbers.size(); i++) {
    registerNumber = m_fpRegisterNumbers[i];
    fpValue = m_fpRegisters.lookup(registerNumber);
    m_v9_interface->write_fp_register_x(cpu, registerNumber, fpValue);
  }
  /*
  if (old_cwp != new_cwp)
    cout << m_proc << " RESTORING CHECKPOINT: CWP " << SIMICS_read_register(m_proc, 95)
                                     << " CANSAVE " << SIMICS_read_register(m_proc, 91)
                                     << " CANRESTORE " << SIMICS_read_register(m_proc, 92)
                                     << " OTHERWIN " << SIMICS_read_register(m_proc, 93)
                                     << " CLEANWIN " << SIMICS_read_register(m_proc, 94)
                                     << " OLD_CWP " << old_cwp
                                     << endl;
   */                                   
}

void RegisterState::print(ostream& out) const
{
  assert(g_SIMICS);
  
  int j;

  int regNumber;
  int iRegNumber;
  int lRegNumber;
  int oRegNumber;
  uinteger_t regValue;
  uinteger_t iRegValue;
  uinteger_t lRegValue;
  uinteger_t oRegValue;
  const char* regName;

  out << "  " << m_proc << " Control Registers : " << endl;
  for(j=0; j < m_controlRegisterNumbers.size(); j++) {
    regNumber = m_controlRegisterNumbers[j];
    regName = SIMICS_get_register_name(m_proc, regNumber);
    regValue = m_controlRegisters.lookup(regNumber);
    out << "  " << m_proc << " "
        << regName << ": " << regValue << endl;
  }

#if 0
  out << endl << "Window Registers :" << endl;
  for(int window=0; window < NUM_WINDOWS; window++) {
    for(j=0; j < m_iRegisterNumbers.size(); j++) {
      iRegNumber = m_iRegisterNumbers[j];
      lRegNumber = m_lRegisterNumbers[j];
      oRegNumber = m_oRegisterNumbers[j];

      iRegValue = m_iRegisters[window].lookup(iRegNumber);
      lRegValue = m_lRegisters[window].lookup(lRegNumber);
      oRegValue = m_oRegisters[window].lookup(oRegNumber);

      out << "  " << m_proc 
          << " Register Numbers (I L O) : " << iRegNumber << " " << lRegNumber << " " << oRegNumber 
          << " Values (I L O) " << iRegValue << " " << lRegValue << " " << oRegValue << endl;
    }
  }

  out << endl << "  " << m_proc << " Global Registers :" << endl;

  for(int set=0; set < NUM_GLOBAL_SET; set++) {
    for(j=0; j < m_gRegisterNumbers.size(); j++) {
      regNumber = m_gRegisterNumbers[j];
      regValue = m_gRegisters[set].lookup(regNumber);

      out << "  " << m_proc << " Register Number : " << regNumber << " Value : " << regValue << endl;
    }
  }

  out << endl << "  " << m_proc << "Floating Point Registers : (double precision - integer representation" << endl;

  for(j=0; j < m_fpRegisterNumbers.size(); j++) {
    regNumber = m_fpRegisterNumbers[j];
    regValue = m_fpRegisters.lookup(regNumber);

    out << "  " << m_proc << " Register Number : " << regNumber << " Value : " << regValue << endl;
  }
#endif
}

// Restore PC and NPC from checkpoint
Address RegisterState::restorePC(int m_proc){
  assert(g_SIMICS);

  int rn_pc = SIMICS_get_register_number(m_proc, "pc");
  int rn_npc = SIMICS_get_register_number(m_proc, "npc");
  ASSERT(rn_pc == 32);
  ASSERT(rn_npc == 33);
  Address oldPC = SIMICS_get_program_counter(m_proc);
  Address newPC = Address(m_controlRegisters.lookup(rn_pc));  
  Address newNPC = Address(m_controlRegisters.lookup(rn_npc));  
  
  SIMICS_set_pc(m_proc, newPC);
  SIMICS_set_npc(m_proc, newNPC);

  return newPC; 
}

Address RegisterState::getPC(){
  if(g_SIMICS)
    return Address(m_controlRegisters.lookup(32));
  else 
    return Address(0);
}

void RegisterState::setPC(unsigned int new_pc) {
  assert(g_SIMICS);
  m_controlRegisters.lookup(32) = new_pc;
}

void RegisterState::disableInterrupts(int m_proc){
  assert(m_proc == this->m_proc);

  int rn_pstate = SIMICS_get_register_number(m_proc, "pstate");
  uinteger_t pstate = SIMICS_read_register(m_proc, rn_pstate);
  m_savedIE = (pstate & 0x2);
  uinteger_t new_pstate = (pstate & (~0x2));
  SIMICS_write_register(m_proc, rn_pstate, new_pstate);
  pstate = SIMICS_read_register(m_proc, rn_pstate);
  assert((pstate & 0x2) == 0);
  //- int rn_pil = SIMICS_get_register_number(m_proc, "pil");
  //- uinteger_t pil = SIMICS_read_register(m_proc, rn_pil);
  //- m_savedPil = pil;
  //- SIMICS_write_register(m_proc, rn_pil, 15);
  //- pil = SIMICS_read_register(m_proc, rn_pil);
  //- assert(pil == 15);
}

void RegisterState::enableInterrupts(int m_proc){
  assert(m_proc == this->m_proc);

  int rn_pstate = SIMICS_get_register_number(m_proc, "pstate");
  uinteger_t pstate = SIMICS_read_register(m_proc, rn_pstate);
  //- uinteger_t new_pstate = (pstate | 0x2); // turn interrupts back on
  assert((pstate & 0x2) == 0);
  uinteger_t new_pstate = (pstate | m_savedIE);
  SIMICS_write_register(m_proc, rn_pstate, new_pstate);
  //- int rn_pil = SIMICS_get_register_number(m_proc, "pil");
  //- uinteger_t pil = SIMICS_read_register(m_proc, rn_pil);
  //- if(pil != 15){
  //-   WARN_EXPR("WARNING: in enable interrupts");
  //-   WARN_EXPR(pil);
  //- }
  //- SIMICS_write_register(m_proc, rn_pil, m_savedPil);
}
