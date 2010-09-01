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

#ifndef _POWER_H_
#define _POWER_H_

/*------------------------------------------------------------
 *  Copyright 1994 Digital Equipment Corporation and Steve Wilton
 *                         All Rights Reserved
 *
 * Permission to use, copy, and modify this software and its documentation is
 * hereby granted only under the following terms and conditions.  Both the
 * above copyright notice and this permission notice must appear in all copies
 * of the software, derivative works or modified versions, and any portions
 * thereof, and both notices must appear in supporting documentation.
 *
 * Users of this software agree to the terms and conditions set forth herein,
 * and hereby grant back to Digital a non-exclusive, unrestricted, royalty-
 * free right and license under any changes, enhancements or extensions
 * made to the core functions of the software, including but not limited to
 * those affording compatibility with other hardware or software
 * environments, but excluding applications which incorporate this software.
 * Users further agree to use their best efforts to return to Digital any
 * such changes, enhancements or extensions that they make and inform Digital
 * of noteworthy uses of this software.  Correspondence should be provided
 * to Digital at:
 *
 *                       Director of Licensing
 *                       Western Research Laboratory
 *                       Digital Equipment Corporation
 *                       100 Hamilton Avenue
 *                       Palo Alto, California  94301
 *
 * This software may be distributed (but not offered for sale or transferred
 * for compensation) to third parties, provided such third parties agree to
 * abide by the terms and conditions of this notice.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND DIGITAL EQUIPMENT CORP. DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS.   IN NO EVENT SHALL DIGITAL EQUIPMENT
 * CORPORATION BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 *------------------------------------------------------------*/

//#include "stats.h"
//#include "pseq.h"

/*  The following are things you might want to change
 *  when compiling
 */

/*
 * The output can be in 'long' format, which shows everything, or
 * 'short' format, which is just what a program like 'grap' would
 * want to see
 */

#define LONG 1
#define SHORT 2

#define OUTPUTTYPE LONG

/* Do we want static AFs (STATIC_AF) or Dynamic AFs (DYNAMIC_AF) */
/* #define DYNAMIC_AF */
#define DYNAMIC_AF

/*
 * Address bits in a word, and number of output bits from the cache 
 */

#define ADDRESS_BITS 64
#define BITOUT 64

/* limits on the various N parameters */

#define MAXN 8            /* Maximum for Ndwl,Ntwl,Ndbl,Ntbl */
#define MAXSUBARRAYS 8    /* Maximum subarrays for data and tag arrays */
#define MAXSPD 8          /* Maximum for Nspd, Ntspd */


/*===================================================================*/

/*
 * The following are things you probably wouldn't want to change.  
 */


#define TRUE 1
#define FALSE 0
#define OK 1
#define ERROR 0
#define BIGNUM 1e30
#define DIVIDE(a,b) ((b)==0)? 0:(a)/(b)
#ifndef MAX
#define MAX(a,b) (((a)>(b))?(a):(b))
#endif

/* Used to communicate with the horowitz model */

#define RISE 1
#define FALL 0
#define NCH  1
#define PCH  0

/*
 * The following scale factor can be used to scale between technologies.
 * To convert from 0.8um to 0.5um, make FUDGEFACTOR = 1.6
 */
 
#define FUDGEFACTOR 1.0

/*===================================================================*/

/*
 * Cache layout parameters and process parameters 
 * Thanks to Glenn Reinman for the technology scaling factors
 */

#define GEN_POWER_FACTOR 1.31

#define TECH_POINT35
#if defined(TECH_POINT10)
#define CSCALE                (84.2172)        /* wire capacitance scaling factor */
                        /* linear: 51.7172, predicted: 84.2172 */
#define RSCALE                (80.0000)        /* wire resistance scaling factor */
#define LSCALE                0.1250                /* length (feature) scaling factor */
#define ASCALE                (LSCALE*LSCALE)        /* area scaling factor */
#define VSCALE                0.38                /* voltage scaling factor */
#define VTSCALE                0.49                /* threshold voltage scaling factor */
#define SSCALE                0.80                /* sense voltage scaling factor */
#define GEN_POWER_SCALE (1/GEN_POWER_FACTOR)
#elif defined(TECH_POINT18)
#define CSCALE                (19.7172)        /* wire capacitance scaling factor */
#define RSCALE                (20.0000)        /* wire resistance scaling factor */
#define LSCALE                0.2250                /* length (feature) scaling factor */
#define ASCALE                (LSCALE*LSCALE)        /* area scaling factor */
#define VSCALE                0.4                /* voltage scaling factor */
#define VTSCALE                0.5046                /* threshold voltage scaling factor */
#define SSCALE                0.85                /* sense voltage scaling factor */
#define GEN_POWER_SCALE 1
#elif defined(TECH_POINT25)
#define CSCALE                (10.2197)        /* wire capacitance scaling factor */
#define RSCALE                (10.2571)        /* wire resistance scaling factor */
#define LSCALE                0.3571                /* length (feature) scaling factor */
#define ASCALE                (LSCALE*LSCALE)        /* area scaling factor */
#define VSCALE                0.45                /* voltage scaling factor */
#define VTSCALE                0.5596                /* threshold voltage scaling factor */
#define SSCALE                0.90                /* sense voltage scaling factor */
#define GEN_POWER_SCALE GEN_POWER_FACTOR
#elif defined(TECH_POINT35a)
#define CSCALE                (5.2197)        /* wire capacitance scaling factor */
#define RSCALE                (5.2571)        /* wire resistance scaling factor */
#define LSCALE                0.4375                /* length (feature) scaling factor */
#define ASCALE                (LSCALE*LSCALE)        /* area scaling factor */
#define VSCALE                0.5                /* voltage scaling factor */
#define VTSCALE                0.6147                /* threshold voltage scaling factor */
#define SSCALE                0.95                /* sense voltage scaling factor */
#define GEN_POWER_SCALE (GEN_POWER_FACTOR*GEN_POWER_FACTOR)
#elif defined(TECH_POINT35)
#define CSCALE                (5.2197)        /* wire capacitance scaling factor */
#define RSCALE                (5.2571)        /* wire resistance scaling factor */
#define LSCALE                0.4375                /* length (feature) scaling factor */
#define ASCALE                (LSCALE*LSCALE)        /* area scaling factor */
#define VSCALE                0.5                /* voltage scaling factor */
#define VTSCALE                0.6147                /* threshold voltage scaling factor */
#define SSCALE                0.95                /* sense voltage scaling factor */
#define GEN_POWER_SCALE (GEN_POWER_FACTOR*GEN_POWER_FACTOR)
#elif defined(TECH_POINT40)
#define CSCALE                1.0                /* wire capacitance scaling factor */
#define RSCALE                1.0                /* wire resistance scaling factor */
#define LSCALE                0.5                /* length (feature) scaling factor */
#define ASCALE                (LSCALE*LSCALE)        /* area scaling factor */
#define VSCALE                1.0                /* voltage scaling factor */
#define VTSCALE                1.0                /* threshold voltage scaling factor */
#define SSCALE                1.0                /* sense voltage scaling factor */
#define GEN_POWER_SCALE (GEN_POWER_FACTOR*GEN_POWER_FACTOR*GEN_POWER_FACTOR)
#else /* TECH_POINT80 */
/* scaling factors */
#define CSCALE                1.0                /* wire capacitance scaling factor */
#define RSCALE                1.0                /* wire resistance scaling factor */
#define LSCALE                1.0                /* length (feature) scaling factor */
#define ASCALE                (LSCALE*LSCALE)        /* area scaling factor */
#define VSCALE                1.0                /* voltage scaling factor */
#define VTSCALE                1.0                /* threshold voltage scaling factor */
#define SSCALE                1.0                /* sense voltage scaling factor */
#define GEN_POWER_SCALE (GEN_POWER_FACTOR*GEN_POWER_FACTOR*GEN_POWER_FACTOR*GEN_POWER_FACTOR)
#endif

/*
 * CMOS 0.8um model parameters
 *   - from Appendix II of Cacti tech report
 */


/* corresponds to 8um of m3 @ 225ff/um */
#define Cwordmetal    (1.8e-15 * (CSCALE * ASCALE))

/* corresponds to 16um of m2 @ 275ff/um */
#define Cbitmetal     (4.4e-15 * (CSCALE * ASCALE))

/* corresponds to 1um of m2 @ 275ff/um */
#define Cmetal        Cbitmetal/16 

#define CM3metal        Cbitmetal/16 
#define CM2metal        Cbitmetal/16 

/* #define Cmetal 1.222e-15 */

/* fF/um2 at 1.5V */
#define Cndiffarea    0.137e-15                /* FIXME: ??? */

/* fF/um2 at 1.5V */
#define Cpdiffarea    0.343e-15                /* FIXME: ??? */

/* fF/um at 1.5V */
#define Cndiffside    0.275e-15                /* in general this does not scale */

/* fF/um at 1.5V */
#define Cpdiffside    0.275e-15                /* in general this does not scale */

/* fF/um at 1.5V */
#define Cndiffovlp    0.138e-15                /* FIXME: by depth??? */

/* fF/um at 1.5V */
#define Cpdiffovlp    0.138e-15                /* FIXME: by depth??? */

/* fF/um assuming 25% Miller effect */
#define Cnoxideovlp   0.263e-15                /* FIXME: by depth??? */

/* fF/um assuming 25% Miller effect */
#define Cpoxideovlp   0.338e-15                /* FIXME: by depth??? */

/* um */
#define Leff          (0.8 * LSCALE)

/* fF/um2 */
#define Cgate         1.95e-15                /* FIXME: ??? */

/* fF/um2 */
#define Cgatepass     1.45e-15                /* FIXME: ??? */

/* note that the value of Cgatepass will be different depending on 
   whether or not the source and drain are at different potentials or
   the same potential.  The two values were averaged */

/* fF/um */
#define Cpolywire        (0.25e-15 * CSCALE * LSCALE)

/* ohms*um of channel width */
#define Rnchannelstatic        (25800 * LSCALE)

/* ohms*um of channel width */
#define Rpchannelstatic        (61200 * LSCALE)

#define Rnchannelon        (9723 * LSCALE)

#define Rpchannelon        (22400 * LSCALE)

/* corresponds to 16um of m2 @ 48mO/sq */
#define Rbitmetal        (0.320 * (RSCALE * ASCALE))

/* corresponds to  8um of m3 @ 24mO/sq */
#define Rwordmetal        (0.080 * (RSCALE * ASCALE))

#define Vdd                (5 * VSCALE)

/* other stuff (from tech report, appendix 1) */

#define Mhz             600e6
#define Period          (1/Mhz)

#define krise                (0.4e-9 * LSCALE)
#define tsensedata        (5.8e-10 * LSCALE)
#define tsensetag        (2.6e-10 * LSCALE)
#define tfalldata        (7e-10 * LSCALE)
#define tfalltag        (7e-10 * LSCALE)
#define Vbitpre                (3.3 * SSCALE)
#define Vt                (1.09 * VTSCALE)
#define Vbitsense        (0.10 * SSCALE)

#define Powerfactor (Mhz)*Vdd*Vdd

#define SensePowerfactor3 (Mhz)*(Vbitsense)*(Vbitsense)
#define SensePowerfactor2 (Mhz)*(Vbitpre-Vbitsense)*(Vbitpre-Vbitsense)
#define SensePowerfactor (Mhz)*(Vdd/2)*(Vdd/2)

#define AF    .5
#define POPCOUNT_AF  (23.9/64.0)

/* Threshold voltages (as a proportion of Vdd)
   If you don't know them, set all values to 0.5 */

#define VSINV         0.456   
#define VTHINV100x60  0.438   /* inverter with p=100,n=60 */
#define VTHNAND60x90  0.561   /* nand with p=60 and three n=90 */
#define VTHNOR12x4x1  0.503   /* nor with p=12, n=4, 1 input */
#define VTHNOR12x4x2  0.452   /* nor with p=12, n=4, 2 inputs */
#define VTHNOR12x4x3  0.417   /* nor with p=12, n=4, 3 inputs */
#define VTHNOR12x4x4  0.390   /* nor with p=12, n=4, 4 inputs */
#define VTHOUTDRINV    0.437
#define VTHOUTDRNOR   0.431
#define VTHOUTDRNAND  0.441
#define VTHOUTDRIVE   0.425
#define VTHCOMPINV    0.437
#define VTHMUXDRV1    0.437
#define VTHMUXDRV2    0.486
#define VTHMUXDRV3    0.437
#define VTHEVALINV    0.267
#define VTHSENSEEXTDRV  0.437

/* transistor widths in um (as described in tech report, appendix 1) */
#define Wdecdrivep        (57.0 * LSCALE)
#define Wdecdriven        (40.0 * LSCALE)
#define Wdec3to8n        (14.4 * LSCALE)
#define Wdec3to8p        (14.4 * LSCALE)
#define WdecNORn        (5.4 * LSCALE)
#define WdecNORp        (30.5 * LSCALE)
#define Wdecinvn        (5.0 * LSCALE)
#define Wdecinvp        (10.0  * LSCALE)

#define Wworddrivemax        (100.0 * LSCALE)
#define Wmemcella        (2.4 * LSCALE)
#define Wmemcellr        (4.0 * LSCALE)
#define Wmemcellw        (2.1 * LSCALE)
#define Wmemcellbscale        2                /* means 2x bigger than Wmemcella */
#define Wbitpreequ        (10.0 * LSCALE)

#define Wbitmuxn        (10.0 * LSCALE)
#define WsenseQ1to4        (4.0 * LSCALE)
#define Wcompinvp1        (10.0 * LSCALE)
#define Wcompinvn1        (6.0 * LSCALE)
#define Wcompinvp2        (20.0 * LSCALE)
#define Wcompinvn2        (12.0 * LSCALE)
#define Wcompinvp3        (40.0 * LSCALE)
#define Wcompinvn3        (24.0 * LSCALE)
#define Wevalinvp        (20.0 * LSCALE)
#define Wevalinvn        (80.0 * LSCALE)

#define Wcompn                (20.0 * LSCALE)
#define Wcompp                (30.0 * LSCALE)
#define Wcomppreequ     (40.0 * LSCALE)
#define Wmuxdrv12n        (30.0 * LSCALE)
#define Wmuxdrv12p        (50.0 * LSCALE)
#define WmuxdrvNANDn    (20.0 * LSCALE)
#define WmuxdrvNANDp    (80.0 * LSCALE)
#define WmuxdrvNORn        (60.0 * LSCALE)
#define WmuxdrvNORp        (80.0 * LSCALE)
#define Wmuxdrv3n        (200.0 * LSCALE)
#define Wmuxdrv3p        (480.0 * LSCALE)
#define Woutdrvseln        (12.0 * LSCALE)
#define Woutdrvselp        (20.0 * LSCALE)
#define Woutdrvnandn        (24.0 * LSCALE)
#define Woutdrvnandp        (10.0 * LSCALE)
#define Woutdrvnorn        (6.0 * LSCALE)
#define Woutdrvnorp        (40.0 * LSCALE)
#define Woutdrivern        (48.0 * LSCALE)
#define Woutdriverp        (80.0 * LSCALE)

#define Wcompcellpd2    (2.4 * LSCALE)
#define Wcompdrivern    (400.0 * LSCALE)
#define Wcompdriverp    (800.0 * LSCALE)
#define Wcomparen2      (40.0 * LSCALE)
#define Wcomparen1      (20.0 * LSCALE)
#define Wmatchpchg      (10.0 * LSCALE)
#define Wmatchinvn      (10.0 * LSCALE)
#define Wmatchinvp      (20.0 * LSCALE)
#define Wmatchnandn     (20.0 * LSCALE)
#define Wmatchnandp     (10.0 * LSCALE)
#define Wmatchnorn     (20.0 * LSCALE)
#define Wmatchnorp     (10.0 * LSCALE)

#define WSelORn         (10.0 * LSCALE)
#define WSelORprequ     (40.0 * LSCALE)
#define WSelPn          (10.0 * LSCALE)
#define WSelPp          (15.0 * LSCALE)
#define WSelEnn         (5.0 * LSCALE)
#define WSelEnp         (10.0 * LSCALE)

#define Wsenseextdrv1p  (40.0*LSCALE)
#define Wsenseextdrv1n  (24.0*LSCALE)
#define Wsenseextdrv2p  (200.0*LSCALE)
#define Wsenseextdrv2n  (120.0*LSCALE)


/* bit width of RAM cell in um */
#define BitWidth        (16.0 * LSCALE)

/* bit height of RAM cell in um */
#define BitHeight        (16.0 * LSCALE)

#define Cout                (0.5e-12 * LSCALE)

/* Sizing of cells and spacings */
#define RatCellHeight    (40.0 * LSCALE)
#define RatCellWidth     (70.0 * LSCALE)
#define RatShiftRegWidth (120.0 * LSCALE)
#define RatNumShift      4
#define BitlineSpacing   (6.0 * LSCALE)
#define WordlineSpacing  (6.0 * LSCALE)

#define RegCellHeight    (16.0 * LSCALE)
#define RegCellWidth     (8.0  * LSCALE)

#define CamCellHeight    (40.0 * LSCALE)
#define CamCellWidth     (25.0 * LSCALE)
#define MatchlineSpacing (6.0 * LSCALE)
#define TaglineSpacing   (6.0 * LSCALE)

/*===================================================================*/

/* ALU POWER NUMBERS for .18um 733Mhz */
/* normalize to cap from W */
#define NORMALIZE_SCALE (1.0/(733.0e6*1.45*1.45))
/* normalize .18um cap to other gen's cap, then xPowerfactor */
#define POWER_SCALE    (GEN_POWER_SCALE * NORMALIZE_SCALE * Powerfactor)
#define I_ADD          ((.37 - .091) * POWER_SCALE)
#define I_ADD32        (((.37 - .091)/2)*POWER_SCALE)
#define I_MULT16       ((.31-.095)*POWER_SCALE)
#define I_SHIFT        ((.21-.089)*POWER_SCALE)
#define I_LOGIC        ((.04-.015)*POWER_SCALE)
#define F_ADD          ((1.307-.452)*POWER_SCALE)
#define F_MULT         ((1.307-.452)*POWER_SCALE)

#define I_ADD_CLOCK    (.091*POWER_SCALE)
#define I_MULT_CLOCK   (.095*POWER_SCALE)
#define I_SHIFT_CLOCK  (.089*POWER_SCALE)
#define I_LOGIC_CLOCK  (.015*POWER_SCALE)
#define F_ADD_CLOCK    (.452*POWER_SCALE)
#define F_MULT_CLOCK   (.452*POWER_SCALE)


// Misc power variables
#define SensePowerfactor (Mhz)*(Vdd/2)*(Vdd/2)
#define Sense2Powerfactor (Mhz)*(2*.3+.1*Vdd)
#define Powerfactor (Mhz)*Vdd*Vdd
#define LowSwingPowerfactor (Mhz)*.2*.2


#define MSCALE (LSCALE * .624 / .2250)

// ISA specific variables... (in bits)
#define opcode_length       8            /* take the avg -> largest is 15 for FP instrs (6 bit op3 + 9 bit opf) */
#define inst_length             32

//modified these in accordance with Opal parameters
#define ruu_decode_width              MAX_DECODE
#define ruu_issue_width                 MAX_DISPATCH
#define ruu_commit_width               MAX_RETIRE
#define RUU_size                          IWINDOW_ROB_SIZE
// the lsq is defined as a hash table of LSQ_HASH_SIZE, so we can't really be sure of its actual size
#define LSQ_size                          IWINDOW_ROB_SIZE/2
#define data_width                        64

//take into account COMPRESSION POWER here
// 1 ALU for computing avoidable and avoided miss, 4 64 bit ALUs for actual compression in parallel
//#define compression_alus      CONFIG_NUM_ALUS[CONFIG_ALU_MAPPING[FU_COMPRESSION]] 
// LUKE - for now, set a pre-defined constant for compression ALUs...
#define compression_alus              4

#define res_ialu                            CONFIG_NUM_ALUS[CONFIG_ALU_MAPPING[FU_INTALU]] + \
                                                  CONFIG_NUM_ALUS[CONFIG_ALU_MAPPING[FU_INTMULT]] + \
                                                  CONFIG_NUM_ALUS[CONFIG_ALU_MAPPING[FU_INTDIV]] 

#define res_fpalu                          CONFIG_NUM_ALUS[CONFIG_ALU_MAPPING[FU_FLOATADD]] + \
                                                  CONFIG_NUM_ALUS[CONFIG_ALU_MAPPING[FU_FLOATCMP]] + \
                                                  CONFIG_NUM_ALUS[CONFIG_ALU_MAPPING[FU_FLOATCVT]] + \
                                                  CONFIG_NUM_ALUS[CONFIG_ALU_MAPPING[FU_FLOATMULT]] + \
                                                  CONFIG_NUM_ALUS[CONFIG_ALU_MAPPING[FU_FLOATDIV]] + \
                                                  CONFIG_NUM_ALUS[CONFIG_ALU_MAPPING[FU_FLOATSQRT]]
#define res_memport                     CONFIG_NUM_ALUS[CONFIG_ALU_MAPPING[FU_RDPORT]] + \
                                                  CONFIG_NUM_ALUS[CONFIG_ALU_MAPPING[FU_WRPORT]] 

// the size of the branch predictor
#define direct_bp_size                  m_pseq->getDirectBPSize();

// the size of the BTB
#define indirect_bp_size                m_pseq->getIndirectBPSize();

//the number of architectural INT registers
#define MD_NUM_IREGS              160*CONFIG_LOGICAL_PER_PHY_PROC
//the number of physical INT registers
#define MD_NUM_PHYS_IREGS     CONFIG_IREG_PHYSICAL
//the number of architectural FP registers
#define MD_NUM_FREGS            64*CONFIG_LOGICAL_PER_PHY_PROC
// the number of physical FP registers
#define MD_NUM_PHYS_FREGS    CONFIG_FPREG_PHYSICAL

//#define MD_NUM_IREGS              80

//Pointers to the L1I and D caches and unified L2 cache...
//  NOTE: Wattch originally separate L2 into I and D, and only worked with L2D cache 
//  IMPORTANT: if running Opal with Ruby, make sure Opal's cache params match Ruby's cache params!!!
#define L1I_NUM_SETS                1 << IL1_SET_BITS 
#define L1D_NUM_SETS               1 << DL1_SET_BITS
#define L2_NUM_SETS                 1 << L2_SET_BITS
#define L1I_ASSOC                      IL1_ASSOC
#define L1D_ASSOC                    DL1_ASSOC
#define L1I_BLOCK_SIZE_BYTES        64
#define L1D_BLOCK_SIZE_BYTES        64
#define L2_BLOCK_SIZE_BYTES          64

#define NUM_L1_BANKS             L1_BANKS

// Instr and Data TLB sizes
// NOTE: Opal does not currently model its own data and instr TLB, so I pulled these off of the UltraSparc III v. 2 manual, p.11-246
#define ITLB_NUM_SETS             128
#define ITLB_ASSOC                   2
#define DTLB_NUM_SETS            512
#define DTLB_ASSOC                  2
// number of bytes for each TLB entry...I estimated from the figures
#define TLB_BLOCK_SIZE_BYTES        4

//number of RAS entries (include both regular and exception entries)
#define ras_size                     (1 << RAS_BITS)+(1 << RAS_EXCEPTION_TABLE_BITS)

/* Used to pass values around the program */
struct parameter_type{
  int tech;
  int iw;
  int winsize;
  int nvreg;
  int npreg;
  int nvreg_width;
  int npreg_width;

  int data_width2;

};

struct  power_result_type{
  double btb;
  double local_predict;
  double global_predict;
  double chooser;
  double ras;
  double rat_driver;
  double rat_decoder;
  double rat_wordline;
  double rat_bitline;
  double rat_senseamp;
  double dcl_compare;
  double dcl_pencode;
  double inst_decoder_power;
  double wakeup_tagdrive;
  double wakeup_tagmatch;
  double wakeup_ormatch;
  double lsq_wakeup_tagdrive;
  double lsq_wakeup_tagmatch;
  double lsq_wakeup_ormatch;
  double selection;
  double regfile_driver;
  double regfile_decoder;
  double regfile_wordline;
  double regfile_bitline;
  double regfile_senseamp;
  double reorder_driver;
  double reorder_decoder;
  double reorder_wordline;
  double reorder_bitline;
  double reorder_senseamp;
  double rs_driver;
  double rs_decoder;
  double rs_wordline;
  double rs_bitline;
  double rs_senseamp;
  double lsq_rs_driver;
  double lsq_rs_decoder;
  double lsq_rs_wordline;
  double lsq_rs_bitline;
  double lsq_rs_senseamp;
  double resultbus;

  double icache_decoder;
  double icache_wordline;
  double icache_bitline;
  double icache_senseamp;
  double icache_tagarray;

  double icache;

  double dcache_decoder;
  double dcache_wordline;
  double dcache_bitline;
  double dcache_senseamp;
  double dcache_tagarray;

  double dtlb;
  double itlb;

  double dcache2_decoder;
  double dcache2_wordline;
  double dcache2_bitline;
  double dcache2_senseamp;
  double dcache2_tagarray;

  double total_power;
  double total_power_nodcache2;
  double ialu_power;
  double falu_power;
  double bpred_power;
  double rename_power;
  double rat_power;
  double dcl_power;
  double window_power;
  double lsq_power;
  double wakeup_power;
  double lsq_wakeup_power;
  double rs_power;
  double rs_power_nobit;
  double lsq_rs_power;
  double lsq_rs_power_nobit;
  double selection_power;
  double regfile_power;
  double regfile_power_nobit;
  double result_power;
  double icache_power;
  double dcache_power;
  double dcache2_power;

  double clock_power;

  //prefetcher power
  double instr_prefetcher_power;
  double data_prefetcher_power;

  //compression power
  double compression_power;

};

/* Used to pass values around the program */

struct  time_parameter_type {
   int cache_size;
   int number_of_sets;
   int associativity;
   int block_size;
};

struct time_result_type {
  double access_time,cycle_time;
  int best_Ndwl,best_Ndbl;
  int best_Nspd;
  int best_Ntwl,best_Ntbl;
  int best_Ntspd;
  double decoder_delay_data,decoder_delay_tag;
  double dec_data_driver,dec_data_3to8,dec_data_inv;
  double dec_tag_driver,dec_tag_3to8,dec_tag_inv;
  double wordline_delay_data,wordline_delay_tag;
  double bitline_delay_data,bitline_delay_tag;
  double sense_amp_delay_data,sense_amp_delay_tag;
  double senseext_driver_delay_data;
  double compare_part_delay;
  double drive_mux_delay;
  double selb_delay;
  double data_output_delay;
  double drive_valid_delay;
  double precharge_delay;
  
}; 

//contains parameters for the cache
struct cache_params_t {
  //number of sets
  int nsets;      
  //block size in bytes
  int bsize;
  //cache assoc
  int assoc;
};

//contains the number of unique banks accessed each cycle
struct bank_num_t{
  int num_l1i_8banks;
  int num_l1d_8banks;
  int num_l1i_4banks;
  int num_l1d_4banks;
  int num_l1i_2banks;
  int num_l1d_2banks;
};


// Class declaration
class power_t{
 public:
   //constructor
  power_t(pseq_t * pseq, int id);

  //destructor
  ~power_t();

  //Called by sequencer every cycle:
  void clear_access_stats();

  void update_power_stats();

  //used by sequencer to print avg power every N cycles
  void output_periodic_power(const uint64 & sim_cycle);

  /* register power stats */
  void power_reg_stats(const tick_t & sim_cycle, const uint64 & total_insn);

  double logtwo(double x);

  //TODO: are these used by anyone else??
  double logfour(double x);
  int pow2(int x);

  double gatecap(double width,double wirelength);
  double gatecappass(double width,double wirelength);
  double draincap(double width,int nchannel,int stack);
  double restowidth(double res,int nchannel);
  double horowitz(double inputramptime,double tf, double vs1,double vs2,int rise);
  double transreson(double width, int nchannel,int stack);
  double transresswitch(double width, int nchannel, int stack);
  double precharge_delay(double worddata);
  double selb_delay_tag_path(double inrisetime, double * outrisetime);
  double dataoutput_delay(int C, int B, int A, int Ndbl, int Nspd,int Ndwl,
                                   double inrisetime, double * outrisetime);
  double mux_driver_delay(int C,int B,int A,int Ndbl,int Nspd,int Ndwl,int Ntbl,int Ntspd,double inputtime,double * outputtime);
  double compare_time(int C,int A,int Ntbl,int Ntspd,double inputtime, double * outputtime);
  double sense_amp_delay(double inrisetime, double * outrisetime);
  double sense_amp_tag_delay(double inrisetime, double * outrisetime);
  double valid_driver_delay(int C, int A,int Ntbl,int Ntspd,double inputtime);
  double bitline_tag_delay(int C,int A,int B,int Ntwl,int Ntbl,int Ntspd,double inrisetime, double * outrisetime);
  double bitline_delay(int C,int A,int B,int Ndwl,int Ndbl,int Nspd,double inrisetime, double * outrisetime);
  double wordline_tag_delay(int C,int A,int Ntspd,int Ntwl,double inrisetime, double * outrisetime);
  double wordline_delay(int B,int A,int Ndwl,int Nspd,double inrisetime, double * outrisetime);
  double decoder_tag_delay(int C, int B, int A, int Ndwl, int Ndbl, int Nspd, int Ntwl, int Ntbl, int Ntspd, double * Tdecdrive, double * Tdecoder1, double * Tdecoder2, double * outrisetime);

  int organizational_parameters_valid(int rows,int cols,int Ndwl,int Ndbl,int Nspd,int Ntwl,int Ntbl,int Ntspd);
  double decoder_delay(int C, int B, int A, int Ndwl, int Ndbl, int Nspd, int Ntwl, int Ntbl, int Ntspd, double * Tdecdrive, double * Tdecoder1, double * Tdecoder2, double * outrisetime);
  int squarify(int rows, int cols);
  double squarify_new(int rows,int cols);
  
  double compute_af(counter_t num_pop_count_cycle, counter_t total_pop_count_cycle,int pop_width);
 

  void calculate_time(time_result_type*result , time_parameter_type* parameters);
  void output_time_components(int A, time_result_type * result);
  void output_data(time_result_type* result, time_parameter_type* parameters);

  int pop_count(uint64 bits);
  int pop_count_slow(uint64 bits);


  /* Various functions for computing the power of different structures */
  double driver_size(double driving_cap, double desiredrisetime); 
  double array_decoder_power(int rows,int cols,double predeclength,int rports,int wports,int cache);
  double simple_array_decoder_power(int rows,int cols,int rports,int wports,int cache);
  double array_wordline_power(int rows,int cols,double wordlinelength,int rports,int wports,int cache);
  double simple_array_wordline_power(int rows,int cols,int rports,int wports,int cache);
  double array_bitline_power(int rows,int cols, double bitlinelength,int rports,int wports,int cache);
  double simple_array_bitline_power(int rows,int cols,int rports,int wports,int cache);
  double senseamp_power(int cols);
  double compare_cap(int compare_bits);
  double dcl_compare_power(int compare_bits);
  double simple_array_power(int rows,int cols,int rports,int wports,int cache);
  double cam_tagdrive(int rows,int cols,int rports,int wports);
  double cam_tagmatch(int rows,int cols,int rports,int wports);
  double cam_array(int rows,int cols,int rports,int wports);
  double selection_power(int win_entries);
  double total_clockpower(double die_length);
  double global_clockpower(double die_length);
  double compute_resultbus_power();

  // print power stats
  void dump_power_stats();
  
  //computes static power estimates
  void calculate_power();

  /* Functions for incrementing the hardware access counters */
  void incrementRenameAccess(){ rename_access++;}
  void incrementBpredAccess(){ bpred_access++;}
  void incrementWindowAccess(){ window_access++;}
  void incrementLSQAccess(){ lsq_access++;}
  void incrementRegfileAccess(){ regfile_access++;}
  void incrementICacheAccess(){ icache_access++;}
  int getNumberICacheAccess(){ return (int) icache_access; }
  void incrementDCacheAccess(){ dcache_access++;}
  int getNumberDCacheAccess(){ return (int) dcache_access; }
  void incrementL2CacheAccess(){ dcache2_access++; }
  // currently count compression as an int ALU access
  void incrementCompressionPower(int num_accesses){ 
    //keep stats on number of compression ALU uses
    compression_access+= num_accesses;
    ialu_access+= num_accesses;
  }
  void incrementPrefetcherAccess(int num_prefetches, int isinstr){
    if(isinstr == 1){
      //this comes from the L1I prefetcher
      instr_prefetcher_access+= num_prefetches;
      icache_access+= num_prefetches;
    }
    else{
      //this comes from the L1D prefetcher
      data_prefetcher_access+= num_prefetches;
      dcache_access+= num_prefetches;
    }
  }
  void notifyUniqueBankNum(bank_num_t bank_num){
    m_bank_num.num_l1i_8banks = bank_num.num_l1i_8banks;
    m_bank_num.num_l1d_8banks = bank_num.num_l1d_8banks;
    m_bank_num.num_l1i_4banks = bank_num.num_l1i_4banks;
    m_bank_num.num_l1d_4banks = bank_num.num_l1d_4banks;
    m_bank_num.num_l1i_2banks = bank_num.num_l1i_2banks;
    m_bank_num.num_l1d_2banks = bank_num.num_l1d_2banks;
  }

  void incrementAluAccess(){ alu_access++; }
  void incrementIAluAccess(){ ialu_access++;}
  void incrementFAluAccess(){ falu_access++;}
  void incrementResultBusAccess(){ resultbus_access++; }
  
  void incrementWinPregAccess(){ window_preg_access++; }
  void incrementWinSelectionAccess(){ window_selection_access++; }
  void incrementWinWakeupAccess(){ window_wakeup_access++; }
  void incrementLSQStoreDataAccess(){ lsq_store_data_access++; }
  void incrementLSQLoadDataAccess(){ lsq_load_data_access++; }
  void incrementLSQPregAccess(){ lsq_preg_access++; }
  void incrementLSQWakeupAccess(){ lsq_wakeup_access++; }
  
  void incrementWinTotalPopCount(uint64 count){ window_total_pop_count_cycle += count; }
  void incrementWinNumPopCount(){ window_num_pop_count_cycle++; }
  void incrementLSQTotalPopCount(uint64 count){ lsq_total_pop_count_cycle += count; }
  void incrementLSQNumPopCount(){ lsq_num_pop_count_cycle++; }
  void incrementRegfileTotalPopCount(uint64 count){ regfile_total_pop_count_cycle += count; }
  void incrementRegfileNumPopCount(){ regfile_num_pop_count_cycle++; }
  void incrementResultBusTotalPopCount(uint64 count){ resultbus_total_pop_count_cycle += count; }
  void incrementResultBusNumPopCount(){ resultbus_num_pop_count_cycle++; }
  

 private:  
  //id used in the filename
  int m_id;

  bank_num_t m_bank_num;

  //output file pointer
  FILE * m_output_file;

  //pointer to the pseq object
  pseq_t  * m_pseq;

  //turns on or off verbose output
  static const bool verbose = false;

  /* static power model results */
  power_result_type * power;

  // The parameters of the BTB
  //  btb_config[0] = number of sets in BTB
  //                [1] = assoc of BTB
  int btb_config[2];

  // The parameters of the 2-level branch predictor
  // twolev_config[0] = level 1 size
  //                    [1] = level 2 size
  //                    [2] = history bits
  //                    [3] = xor (true or false)
  int twolev_config[4];

  // The parameters of the bimodal predictor
  // bimod_config[0] = table size
  int bimod_config[1];

  //The parameters of the combining predictor
  // comb_config[0] = meta table size
  int comb_config[1];

  //The parameters of the L1 I and D caches and L2 unified cache
  //  IMPORTANT: although it is named cache_dl2, we are simulating UNIFIED L2, so we must use its params
  cache_params_t * cache_dl1;
  cache_params_t * cache_il1;
  cache_params_t * cache_dl2;

  //The parameters for the Instr and Data TLBs
  cache_params_t * dtlb;
  cache_params_t * itlb;
  

  /* scale for crossover (vdd->gnd) currents */
  double crossover_scaling;
  /* non-ideal turnoff percentage */
  double turnoff_factor;

  // the log size of the number of architectural INT registers
  int nvreg_width;
  // the log size of the RUU_size, upper bound on number of physical INT registers
  int npreg_width;

  //used for calculating power
  double global_clockcap;

  double rename_power;
  double bpred_power;
  double window_power;
  double lsq_power;
  double regfile_power;
  double icache_power;
  double dcache_power;
  double dcache2_power;
  double alu_power;
  double falu_power;
  double resultbus_power;
  double clock_power;
  double instr_prefetcher_power;
  double data_prefetcher_power;
  double compression_power;
  
  double rename_power_cc1;
  double bpred_power_cc1;
  double window_power_cc1;
  double lsq_power_cc1;
  double regfile_power_cc1;
  double icache_power_cc1;
  double dcache_power_cc1;
  double dcache2_power_cc1;
  double alu_power_cc1;
  double resultbus_power_cc1;
  double clock_power_cc1;
  double instr_prefetcher_power_cc1;
  double data_prefetcher_power_cc1;
  double compression_power_cc1;
  
  double rename_power_cc2;
  double bpred_power_cc2;
  double window_power_cc2;
  double lsq_power_cc2;
  double regfile_power_cc2;
  double icache_power_cc2;
  double dcache_power_cc2;
  double dcache2_power_cc2;
  double alu_power_cc2;
  double resultbus_power_cc2;
  double clock_power_cc2;
  double instr_prefetcher_power_cc2;
  double data_prefetcher_power_cc2;
  double compression_power_cc2;
  
  double rename_power_cc3;
  double bpred_power_cc3;
  double window_power_cc3;
  double lsq_power_cc3;
  double regfile_power_cc3;
  double icache_power_cc3;
  double dcache_power_cc3;
  double dcache2_power_cc3;
  double alu_power_cc3;
  double resultbus_power_cc3;
  double clock_power_cc3;
  double instr_prefetcher_power_cc3;
  double data_prefetcher_power_cc3;
  double compression_power_cc3;
  
  double max_rename_power_cc3;
  double max_bpred_power_cc3;
  double max_window_power_cc3;
  double max_lsq_power_cc3;
  double max_regfile_power_cc3;
  double max_icache_power_cc3;
  double max_dcache_power_cc3;
  double max_dcache2_power_cc3;
  double max_alu_power_cc3;
  double max_resultbus_power_cc3;
  double max_current_clock_power_cc3;
  double max_instr_prefetcher_power_cc3;
  double max_data_prefetcher_power_cc3;
  double max_compression_power_cc3;

  double total_cycle_power;
  double total_cycle_power_cc1;
  double total_cycle_power_cc2;
  double total_cycle_power_cc3;
  
  double last_single_total_cycle_power_cc1;
  double last_single_total_cycle_power_cc2;
  double last_single_total_cycle_power_cc3;
  double current_total_cycle_power_cc1;
  double current_total_cycle_power_cc2;
  double current_total_cycle_power_cc3;

  double last_single_total_cycle_power_no_clock_cc1;
  double last_single_total_cycle_power_no_clock_cc2;
  double last_single_total_cycle_power_no_clock_cc3;
  double current_total_cycle_power_no_clock_cc1;
  double current_total_cycle_power_no_clock_cc2;
  double current_total_cycle_power_no_clock_cc3;

  double current_clock_power_cc1;
  double current_clock_power_cc2;
  double current_clock_power_cc3;

  double last_clock_power_cc1;
  double last_clock_power_cc2;
  double last_clock_power_cc3;
  
  double max_cycle_power;
  double max_clock_power_cc1;
  double max_clock_power_cc2;
  double max_clock_power_cc3;

  double max_cycle_power_cc1;
  double max_cycle_power_cc2;
  double max_cycle_power_cc3;
  double max_cycle_power_no_clock_cc1;
  double max_cycle_power_no_clock_cc2;
  double max_cycle_power_no_clock_cc3;

  counter_t total_rename_access;
  counter_t total_bpred_access;
  counter_t total_window_access;
  counter_t total_lsq_access;
  counter_t total_regfile_access;
  counter_t total_icache_access;
  counter_t total_dcache_access;
  counter_t total_dcache2_access;
  counter_t total_alu_access;
  counter_t total_resultbus_access;
  counter_t total_instr_prefetcher_access;
  counter_t total_data_prefetcher_access;
  counter_t total_compression_access;
  
  counter_t max_rename_access;
  counter_t max_bpred_access;
  counter_t max_window_access;
  counter_t max_lsq_access;
  counter_t max_regfile_access;
  counter_t max_icache_access;
  counter_t max_dcache_access;
  counter_t max_dcache2_access;
  counter_t max_alu_access;
  counter_t max_resultbus_access;
  counter_t max_instr_prefetcher_access;
  counter_t max_data_prefetcher_access;
  counter_t max_compression_access;

  //used for finer granularity for max power stats
  counter_t max_win_preg_access;
  counter_t max_win_selection_access;
  counter_t max_win_wakeup_access;
  counter_t max_lsq_wakeup_access;
  counter_t max_lsq_preg_access;
  counter_t max_ialu_access;
  counter_t max_falu_access;

  //used by sequencer:
  counter_t rename_access;
  counter_t bpred_access;
  counter_t window_access;
  counter_t lsq_access;
  counter_t regfile_access;
  counter_t icache_access;
  counter_t dcache_access;
  counter_t dcache2_access;
  counter_t alu_access;
  counter_t ialu_access;
  counter_t falu_access;
  counter_t resultbus_access;
  counter_t instr_prefetcher_access;
  counter_t data_prefetcher_access;
  counter_t compression_access;
  
  counter_t window_preg_access;
  counter_t window_selection_access;
  counter_t window_wakeup_access;
  counter_t lsq_store_data_access;
  counter_t lsq_load_data_access;
  counter_t lsq_preg_access;
  counter_t lsq_wakeup_access;
  
  counter_t window_total_pop_count_cycle;
  counter_t window_num_pop_count_cycle;
  counter_t lsq_total_pop_count_cycle;
  counter_t lsq_num_pop_count_cycle;
  counter_t regfile_total_pop_count_cycle;
  counter_t regfile_num_pop_count_cycle;
  counter_t resultbus_total_pop_count_cycle;
  counter_t resultbus_num_pop_count_cycle;

};


#endif /* _POWER_H_ */
