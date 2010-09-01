#ifndef _CCOPS_H_
#define _CCOPS_H_
/*
 * Copyright (c) 1999 by Mark Hill and David Wood for the Wisconsin
 * Multifacet Project.  ALL RIGHTS RESERVED.  
 *
 * ##HEADER##
 *
 * This software is furnished under a license and may be used and
 * copied only in accordance with the terms of such license and the
 * inclusion of the above copyright notice.  This software or any
 * other copies thereof or any derivative works may not be provided or
 * otherwise made available to any other persons.  Title to and
 * ownership of the software is retained by Mark Hill and David Wood.
 * Any use of this software must include the above copyright notice.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS".  THE LICENSOR MAKES NO
 * WARRANTIES ABOUT ITS CORRECTNESS OR PERFORMANCE.
 * */

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Class declaration(s)                                                   */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Global variables                                                       */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Global functions                                                       */
/*------------------------------------------------------------------------*/

/** add with condition codes: used for ISA implementations */
ireg_t us_addcc(ireg_t source1, ireg_t source2, unsigned char *ccode_p);
/** subtract with condition codes: used for ISA implementations */
ireg_t us_subcc(ireg_t source1, ireg_t source2, unsigned char *ccode_p);
/** and with condition codes: used for ISA implementations */
ireg_t us_andcc(ireg_t source1, ireg_t source2, unsigned char *ccode_p);
/** or with condition codes: used for ISA implementations */
ireg_t us_orcc(ireg_t source1, ireg_t source2, unsigned char *ccode_p);
/** xor with condition codes: used for ISA implementations */
ireg_t us_xorcc(ireg_t source1, ireg_t source2, unsigned char *ccode_p);
/** cmp with condition codes: used for ISA implementations */
void   us_cmp(ireg_t source1, ireg_t source2, unsigned char *ccode_p);

/** read the floating point status register, returning its contents */
float64 us_mul_double( float64 source1, float64 source2, uint64 *status );
float64 us_div_double( float64 source1, float64 source2, uint64 *status );

float64 us_add_double( float64 source1, float64 source2, uint64 *status );
float64 us_sub_double( float64 source1, float64 source2, uint64 *status );

void   us_read_fsr( uint64 *status );

#endif  /* _CCOPS_H_ */


