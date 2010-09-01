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
 * FileName:  ccops.c
 * Synopsis:  portatble condition code operations
 * Author:    cmauer
 * Version:   $Id: ccops.c 1.3 03/07/10 18:07:47-00:00 milo@cottons.cs.wisc.edu $
 */

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include "hfa.h"
#include "bitlib.h"
#include "ccops.h"
#ifdef USE_FSR
#include <fenv.h>
#endif

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Variable declarations                                                  */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Forward declarations                                                   */
/*------------------------------------------------------------------------*/

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


//***************************************************************************
static void  us_logic_cc(ireg_t source1, ireg_t source2, ireg_t result,
                         unsigned char *ccode_p)
{
  unsigned char ccode = 0;
  uint32 result32;

#ifdef DEBUG_CC
  printf("src1  0x%0llx\n", source1);
  printf("src2  0x%0llx\n", source2);
  printf("resu  0x%0llx\n", result);
#endif
  // set 64 bit condition codes
  // set the n bit
  ccode |= ( (result >> 63) == 1 ) << 7;
  // set the z bit
  ccode |= ( result == 0 )  << 6;
  // set the v bit to zero
  // set the c bit to zero
  
  // set 32 bit condition codes
  result32 = (uint32) result;
  // set the n bit
  ccode |= ( (result32 >> 31) == 1 ) << 3;
  // set the z bit
  ccode |= ( result32 == 0 ) << 2;
  // set the v bit to zero
  // set the c bit to zero
  *ccode_p = ccode;
}


//**************************************************************************
ireg_t us_andcc(ireg_t source1, ireg_t source2, unsigned char *ccode_p)
{
  ireg_t        result;

  // do a normal and
  result   = source1 & source2;
  us_logic_cc( source1, source2, result, ccode_p );

#ifdef DEBUG_CC
  printf("ANDCC\n");
  //  unsigned char testcc = 0;
  //  uand64_cc(source1, source2, &testcc);
  //  printf("test    0x%0x\n", testcc );
  printf("ccodes  0x%0x\n", *ccode_p);
#endif
  return ( result );
}

//**************************************************************************
ireg_t us_orcc(ireg_t source1, ireg_t source2, unsigned char *ccode_p)
{
  ireg_t        result;

  // do a normal or
  result   = source1 | source2;
  us_logic_cc( source1, source2, result, ccode_p );

#ifdef DEBUG_CC
  printf("ORCC\n");
  // unsigned char testcc = 0;
  // uor64_cc(source1, source2, &testcc);
  // printf("test    0x%0x\n", testcc );
  printf("ccodes  0x%0x\n", *ccode_p);
#endif
  return ( result );
}

//**************************************************************************
ireg_t us_xorcc(ireg_t source1, ireg_t source2, unsigned char *ccode_p)
{
  ireg_t        result;

  // do a normal xor
  result   = source1 ^ source2;
  us_logic_cc( source1, source2, result, ccode_p );

#ifdef DEBUG_CC
  printf("XORCC\n");
  //  unsigned char testcc = 0;
  //  uxor64_cc(source1, source2, &testcc);
  //  printf("test  0x%0x\n", testcc );
  printf("ccodes  0x%0x\n", *ccode_p);
#endif
  return ( result );
}

//**************************************************************************
void   us_cmp(ireg_t source1, ireg_t source2, unsigned char *ccode_p)
{
  us_subcc( source1, source2, ccode_p );
}

//**************************************************************************
ireg_t us_subcc(ireg_t source1, ireg_t source2, unsigned char *ccode_p)
{
  unsigned char    ccode = 0;
  // unsigned "diff"
  ireg_t           diff;
  uint32 diff32;
  uint32 source1_32;
  uint32 source2_32;

  // do a signed subtract
  diff   = (uint64) ((int64) source1 - (int64) source2);

  // set 64 bit condition codes
  // set the n bit
  ccode |= ( diff >> 63 == 1 ) << 7;
  // set the z bit
  ccode |= ( source1 == source2 )  << 6;
  // set the v bit
  ccode |= ( (int64) ((source1^source2)&(diff^source1)) < 0 ) << 5;
  // set the c bit
  ccode |= ( source1 < source2 ) << 4;

  // set 32 bit condition codes
  diff32 = (uint32) diff;
  source1_32 = (uint32) source1;
  source2_32 = (uint32) source2;
  // set the n bit
  ccode |= ( diff32 >> 31 == 1 ) << 3;
  // set the z bit
  ccode |= ( source1_32 == source2_32 ) << 2;
  // set the v bit
  ccode |= ( (int32) ((source1_32^source2_32)&(diff32^source1_32)) < 0 ) << 1;
  // set the c bit
  ccode |= ( source1_32 < source2_32 );

#ifdef DEBUG_CC
  //  unsigned char    testcc = 0;
  printf("CMP\n");
  printf("src1  0x%0llx\n", source1);
  printf("src2  0x%0llx\n", source2);
  printf("resu  0x%0llx\n", diff);
  //  ssub64_cc(source1, source2, &testcc);
  //  printf("test  0x%0x\n", testcc );
  printf("CMP ccodes  0x%0x\n", ccode);  
#endif

  *ccode_p = ccode;
  return (diff);
}

/* operators on signed/unsigned long values that set condition codes */
//**************************************************************************
ireg_t us_addcc(ireg_t source1, ireg_t source2, unsigned char *ccode_p)
{
  unsigned char ccode = 0;
  // unsigned "diff"
  ireg_t        sum;
  uint32 sum32;
  uint32 source1_32;
  uint32 source2_32;

  // do a normal add
  sum   = (uint64) ((int64) source1 + (int64) source2);

  // set 64 bit condition codes
  // set the n bit
  ccode |= ( sum >> 63 == 1 ) << 7;
  // set the z bit
  ccode |= ( sum == 0 ) << 6;
  // set the v bit
  ccode |= ( (int64) ((source1^~source2)&(sum^source1)) < 0 ) << 5;
  // set the c bit
  ccode |= ( sum < source1 || sum < source2 ) << 4;
  
  // set 32 bit condition codes
  sum32 = (uint32) sum;
  source1_32 = (uint32) source1;
  source2_32 = (uint32) source2;
  // set the n bit
  ccode |= ( sum32 >> 31 == 1 ) << 3;
  // set the z bit
  ccode |= ( sum32 == 0 )  << 2;
  // set the v bit
  ccode |= ( (int32) ((source1_32^~source2_32)&(sum32^source1_32)) < 0 ) << 1;
  // set the c bit
  ccode |= ( sum32 < source1_32 || sum32 < source2_32 ) << 0;

  *ccode_p = ccode;
  return ( (ireg_t) sum );
}

/*------------------------------------------------------------------------*/
/* Floating point global methods                                          */
/*------------------------------------------------------------------------*/

#ifdef USE_FSR
//***************************************************************************
static void   us_make_fsr( uint64 *status, fexcept_t fp_flags )
{
  uint64 fsr            = *status;
  uint32 current_except = 0;
  uint64 tem            = maskBits64( *status, 23, 27 );
  if ( fp_flags & FE_INVALID ) {
    current_except |= (1 << 4);
  }
  if ( fp_flags & FE_OVERFLOW ) {
    current_except |= (1 << 3);
  }
  if ( fp_flags & FE_UNDERFLOW ) {
    current_except |= (1 << 2);
  }
  if ( fp_flags & FE_DIVBYZERO ) {
    current_except |= (1 << 1);
  }
  if ( fp_flags & FE_INEXACT ) {
    current_except |=  1;
  }

  if ((tem & current_except) != 0) {
    // generate IEEE_754_exception, set current execptions
    fsr = (fsr & ~0x1f) | current_except;
  } else {
    // add to accrued exceptions by OR'ing cxec with accrued exceptions
    fsr = (fsr & ~0x3e0) | (current_except << 5);
    // fsr = (fsr & ~0x3ff) | (current_except << 5) | current_except;
  }
  
  // current exception bits of fsr
#if 0
  DEBUG_OUT("fsr_is       : 0x%0llx\n", *status );
  DEBUG_OUT("TEM          : 0x%0llx\n", tem );
  DEBUG_OUT("current_flags: 0x%0x\n", current_except );
  DEBUG_OUT("fsr          : 0x%0llx\n", fsr );
#endif
  *status = fsr;
}
#endif

//***************************************************************************
float64 us_mul_double( float64 source1, float64 source2, uint64 *status )
{
#ifdef USE_FSR
  fexcept_t fp_flags;
#endif

  float64   result = source1 * source2;
#ifdef USE_FSR
  fegetexceptflag( &fp_flags, FE_ALL_EXCEPT );
  us_make_fsr( status, fp_flags );
#endif
  return result;
}

//***************************************************************************
float64 us_div_double( float64 source1, float64 source2, uint64 *status )
{
#ifdef USE_FSR
  fexcept_t fp_flags;
#endif

  float64   result = source1 / source2;
#ifdef USE_FSR
  fegetexceptflag( &fp_flags, FE_ALL_EXCEPT );
  us_make_fsr( status, fp_flags );
#endif
  return result;
}

//***************************************************************************
float64 us_add_double( float64 source1, float64 source2, uint64 *status )
{
#ifdef USE_FSR
  fexcept_t fp_flags;
#endif

  float64   result = source1 + source2;
#ifdef USE_FSR
  fegetexceptflag( &fp_flags, FE_ALL_EXCEPT );
  us_make_fsr( status, fp_flags );
#endif
  return result;
}

//***************************************************************************
float64 us_sub_double( float64 source1, float64 source2, uint64 *status )
{
#ifdef USE_FSR
  fexcept_t fp_flags;
#endif

  float64   result = source1 - source2;
#ifdef USE_FSR
  fegetexceptflag( &fp_flags, FE_ALL_EXCEPT );
  us_make_fsr( status, fp_flags );
#endif
  return result;
}

//***************************************************************************
void   us_read_fsr( uint64 *status )
{
#ifdef USE_FSR
  fexcept_t fp_flags;
  fegetexceptflag( &fp_flags, FE_ALL_EXCEPT );
  us_make_fsr( status, fp_flags );
#endif
}
