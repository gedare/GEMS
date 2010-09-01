
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
/*
 * FileName:  regtest.C
 * Synopsis:  tests the register file
 * Author:    cmauer
 * Version:   $Id$
 */

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/


#include "hfa.h"
#include "hfacore.h"
extern "C" {
#include "hfa_init.h"
}
#include "opal.h"

#include <stdio.h>
#include <sys/types.h>

#include "pstate.h"
#include "regmap.h"

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Variable declarations                                                  */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Forward declarations                                                   */
/*------------------------------------------------------------------------*/

extern void simstate_init( void );
extern void simstate_fini( void );

/*------------------------------------------------------------------------*/
/* Constructor(s) / destructor                                            */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Public methods                                                         */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Accessor(s) / mutator(s)                                               */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Private methods                                                        */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Static methods                                                         */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Global functions                                                       */
/*------------------------------------------------------------------------*/

/** [Memo].
 *  [Internal Documentation]
 */
//**************************************************************************
int main( int argc, char *argv[] ) {

  // initialize the state
  simstate_init();

  // TEMP
#if 0
  // initialize our module
  hfa_init_local();

  physical_file_t *m_integer_rf = new physical_file_t( CONFIG_IREG_PHYSICAL );
  reg_map_t *m_idecode_map = new reg_map_t( m_integer_rf,
                                            CONFIG_IREG_LOGICAL );
#endif

#if 0
  for (int i = 0; i < 63; i += 32) {
    for (int j = 0; j < 63; j += 8) {
      printf("i = %d j = %d\n", i, j);
      printBits64( makeMask64(i, j) );
    }
  }
#endif

#if 1
  for (uint16 cwp = 0; cwp < NWINDOWS; cwp++) {

    for (int reg = 31; reg >= 8; reg --) {
      
      int flatreg = pstate_t::pstateWindowMap( cwp, reg );
      if ( (reg / 8) == 1 ) {
        printf("o");
      } else if ( (reg / 8) == 2 ) {
        printf("l");
      } else if ( (reg / 8) == 3 ) {
        printf("i");
      }
      printf("%d ", reg % 8);
      printf("cwp %d reg %2.2d = %d\n", cwp, reg, flatreg);

    }
  }
#endif

#if 0
  for (uint16 cwp = 0; cwp < NWINDOWS; cwp++) {
    for (int reg = 0; reg < 8; reg ++) {
      half_t flatreg = pstate_t::pstateGlobalMap( reg, cwp, 0 );
      printf("cwp %2.2d reg %2.2d = %d\n", cwp, reg, flatreg);
    }
  }
#endif

#if 0
  for (uint16 gset = 0; gset < 4; gset++) {
    for (int reg = 0; reg < 8; reg ++) {
      half_t flatreg = reg_box_t::iregGlobalMap( reg, gset );
      printf("gset %d reg %2.2d = %d\n", gset, reg, flatreg);
    }
  }
#endif

#if 0
  // allocate and free a bunch of registers
  int rmap[1000];
  for (uint32 i = 0; i < CONFIG_IREG_LOGICAL + 10; i++) {
    rmap[i] = m_idecode_map->regAllocate( i );
  }
  for (uint32 i = 0; i < CONFIG_IREG_LOGICAL; i++) {
    m_idecode_map->regFree( i, rmap[i] );
  }
  for (uint32 i = 0; i < CONFIG_IREG_LOGICAL; i++) {
    rmap[i] = m_idecode_map->regAllocate( i );
  }
  for (uint32 i = 0; i < CONFIG_IREG_LOGICAL + 10; i++) {
    m_idecode_map->regFree( i, rmap[i] );
  }
#endif

#if 0
  // test the floating point registers
  reg_box_t m_regbox;
  /// physical floating-point register file
  physical_file_t *m_fp_rf;
  /// floating point map at decode stage
  reg_map_t       *m_fp_decode_map;
  /// floating point map at retire stage
  reg_map_t       *m_fp_retire_map;

  m_fp_rf = new physical_file_t( CONFIG_FPREG_PHYSICAL );
  m_fp_decode_map = new reg_map_t( m_fp_rf, CONFIG_FPREG_LOGICAL );
  m_fp_retire_map = new reg_map_t( m_fp_rf, CONFIG_FPREG_LOGICAL );
  m_regbox.addRegisterHandler( RID_SINGLE, m_fp_rf, 
                                 m_fp_decode_map, m_fp_retire_map );
  m_regbox.addRegisterHandler( RID_DOUBLE, m_fp_rf, 
                                 m_fp_decode_map, m_fp_retire_map );
  m_regbox.addRegisterHandler( RID_QUAD, m_fp_rf, 
                                 m_fp_decode_map, m_fp_retire_map );

  int physreg;
  for (int reg = 0; reg < (int) MAX_FLOAT_REGS; reg++) {
    // upper 32 bits (including exponent)
    physreg = m_fp_decode_map->regAllocate( reg*2 );
    m_fp_rf->setInt( physreg, 0 );
    m_fp_decode_map->setMapping( reg*2, physreg );
    m_fp_retire_map->setMapping( reg*2, physreg );

    // lower 32 bits
    physreg = m_fp_decode_map->regAllocate( reg*2 + 1 );
    m_fp_rf->setInt( physreg, 1 );
    m_fp_decode_map->setMapping( reg*2 + 1, physreg );
    m_fp_retire_map->setMapping( reg*2 + 1, physreg );    
  }

  // write two single values, read out the double
  reg_id_t fp0_single;
  reg_id_t fp1_single;
  reg_id_t fp0_double;

  fp0_single.setRtype( RID_SINGLE );
  fp0_single.setVanilla( 0 );
  fp0_single.readDecodeMap( m_regbox );

  fp1_single.setRtype( RID_SINGLE );
  fp1_single.setVanilla( 1 );
  fp1_single.readDecodeMap( m_regbox );

  fp0_double.setRtype( RID_DOUBLE );
  fp0_double.setVanilla( 0 );
  fp0_double.readDecodeMap( m_regbox );

  fp0_single.print();
  fp1_single.print();
  fp0_double.print();

  float64 value = 1.25e+25;
  printf("double value  : %g\n", value );
  printf("double as int : 0x%0llx\n", value );
  printf("int    values: 0x%0x 0x%0x\n",
         ((int *) &value)[0], ((int *) &value)[1] );

  fp0_double.writeFloat64( m_regbox, 1.25e+25 );

  value = fp0_double.readFloat64( m_regbox );
  printf("double value : %g\n", value );
  printf("int    values: 0x%0x 0x%0x\n",
         ((int *) &value)[0], ((int *) &value)[1] );
  ireg_t fp0i = fp0_single.readInt64( m_regbox );
  ireg_t fp1i = fp1_single.readInt64( m_regbox );
  printf("single values: 0x%0llx 0x%0llx\n", fp0i, fp1i );
#endif

  // TEMP
#if 0
  // close down our module
  hfa_fini_local();
#endif

  // close down the state
  simstate_fini();
}
