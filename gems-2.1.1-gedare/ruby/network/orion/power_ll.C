/*
    Copyright (C) 1999-2008 by Mark D. Hill and David A. Wood for the
    Wisconsin Multifacet Project.  Contact: gems@cs.wisc.edu
    http://www.cs.wisc.edu/gems/

    --------------------------------------------------------------------

    This file is part of Orion (Princeton's interconnect power model),
    a component of the Multifacet GEMS (General Execution-driven 
    Multiprocessor Simulator) software toolset originally developed at 
    the University of Wisconsin-Madison.

    Garnet was developed by Niket Agarwal at Princeton University. Orion was
    developed by Princeton University.

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

#include <math.h>
#include <assert.h>

#include "parm_technology.h"
#include "SIM_port.h"
#include "power_static.h"
#include "power_ll.h"

/*----------------------------------------------------------------------*/

double SIM_power_gatecap(double width,double wirelength) /* returns gate capacitance in Farads */
//double width;		/* gate width in um (length is Leff) */
//double wirelength;	/* poly wire length going to gate in lambda */
{
  
  double overlapCap;
  double gateCap;
  double l = 0.1525;
  
#if defined(Pdelta_w)
  overlapCap = (width - 2*Pdelta_w) * PCov;
  gateCap  = ((width - 2*Pdelta_w) * (l * LSCALE - 2*Pdelta_l) *
                PCg) + 2.0 * overlapCap;

  return gateCap;
#endif
  return(width*Leff*PARM_Cgate+wirelength*Cpolywire*Leff * SCALE_T);
  /* return(width*Leff*PARM_Cgate); */
  /* return(width*CgateLeff+wirelength*Cpolywire*Leff);*/
}


double SIM_power_gatecappass(double width,double wirelength) /* returns gate capacitance in Farads */
//double width;           /* gate width in um (length is Leff) */
//double wirelength;      /* poly wire length going to gate in lambda */
{
  return(SIM_power_gatecap(width,wirelength));
  /* return(width*Leff*PARM_Cgatepass+wirelength*Cpolywire*Leff); */
}


/*----------------------------------------------------------------------*/

/* Routine for calculating drain capacitances.  The draincap routine
 * folds transistors larger than 10um */
double SIM_power_draincap(double width,int nchannel,int stack)  /* returns drain cap in Farads */
//double width;		/* um */
//int nchannel;		/* whether n or p-channel (boolean) */
//int stack;		/* number of transistors in series that are on */
{
  double Cdiffside,Cdiffarea,Coverlap,cap;

  double overlapCap;
  double swAreaUnderGate;
  double area_peri;
  double diffArea;
  double diffPeri;
  double l = 0.4 * LSCALE;


  diffArea = l * width;
  diffPeri = 2 * l + 2 * width;

#if defined(Pdelta_w)
  if(nchannel == 0) {
    overlapCap = (width - 2 * Pdelta_w) * PCov;
    swAreaUnderGate = (width - 2 * Pdelta_w) * PCjswA;
    area_peri = ((diffArea * PCja)
		 +  (diffPeri * PCjsw));
     
    return(stack*(area_peri + overlapCap + swAreaUnderGate));
  }
  else {
    overlapCap = (width - 2 * Ndelta_w) * NCov;
    swAreaUnderGate = (width - 2 * Ndelta_w) * NCjswA;
    area_peri = ((diffArea * NCja * LSCALE)
		 +  (diffPeri * NCjsw * LSCALE));
   
    return(stack*(area_peri + overlapCap + swAreaUnderGate));
  }
#endif

	Cdiffside = (nchannel) ? PARM_Cndiffside : PARM_Cpdiffside;
	Cdiffarea = (nchannel) ? PARM_Cndiffarea : PARM_Cpdiffarea;
	Coverlap = (nchannel) ? (PARM_Cndiffovlp+PARM_Cnoxideovlp) :
				(PARM_Cpdiffovlp+PARM_Cpoxideovlp);
	/* calculate directly-connected (non-stacked) capacitance */
	/* then add in capacitance due to stacking */
	if (width >= 10) {
	    cap = 3.0*Leff*width/2.0*Cdiffarea + 6.0*Leff*Cdiffside +
		width*Coverlap;
	    cap += (double)(stack-1)*(Leff*width*Cdiffarea +
		4.0*Leff*Cdiffside + 2.0*width*Coverlap);
	} else {
	    cap = 3.0*Leff*width*Cdiffarea + (6.0*Leff+width)*Cdiffside +
		width*Coverlap;
	    cap += (double)(stack-1)*(Leff*width*Cdiffarea +
		2.0*Leff*Cdiffside + 2.0*width*Coverlap);
	}
	return(cap * SCALE_T);
}


/*----------------------------------------------------------------------*/

/* The following routines estimate the effective resistance of an
   on transistor as described in the tech report.  The first routine
   gives the "switching" resistance, and the second gives the 
   "full-on" resistance */
double SIM_power_transresswitch(double width,int nchannel,int stack)  /* returns resistance in ohms */
//double width;		/* um */
//int nchannel;		/* whether n or p-channel (boolean) */
//int stack;		/* number of transistors in series */
{
	double restrans;
        restrans = (nchannel) ? (Rnchannelstatic):
                                (Rpchannelstatic);
	/* calculate resistance of stack - assume all but switching trans
	   have 0.8X the resistance since they are on throughout switching */
	return((1.0+((stack-1.0)*0.8))*restrans/width);	
}


/*----------------------------------------------------------------------*/

double SIM_power_transreson(double width,int nchannel,int stack)  /* returns resistance in ohms */
//double width;           /* um */
//int nchannel;           /* whether n or p-channel (boolean) */
//int stack;              /* number of transistors in series */
{
        double restrans;
        restrans = (nchannel) ? Rnchannelon : Rpchannelon;

      /* calculate resistance of stack.  Unlike transres, we don't
           multiply the stacked transistors by 0.8 */
        return(stack*restrans/width);
}


/*----------------------------------------------------------------------*/

/* This routine operates in reverse: given a resistance, it finds
 * the transistor width that would have this R.  It is used in the
 * data wordline to estimate the wordline driver size. */
double SIM_power_restowidth(double res,int nchannel)  /* returns width in um */
//double res;            /* resistance in ohms */
//int nchannel;          /* whether N-channel or P-channel */
{
   double restrans;

        restrans = (nchannel) ? Rnchannelon : Rpchannelon;

   return(restrans/res);
}


/*----------------------------------------------------------------------*/

double SIM_power_horowitz(double inputramptime,double tf,double vs1,double vs2,int rise)
//double inputramptime,    /* input rise time */
  //     tf,               /* time constant of gate */
    //   vs1,vs2;          /* threshold voltages */
//int rise;                /* whether INPUT rise or fall (boolean) */
{
    double a,b,td;

    a = inputramptime/tf;
    if (rise==RISE) {
       b = 0.5;
       td = tf*sqrt(fabs( log(vs1)*log(vs1)+2*a*b*(1.0-vs1))) +
            tf*(log(vs1)-log(vs2));
    } else {
       b = 0.4;
       td = tf*sqrt(fabs( log(1.0-vs1)*log(1.0-vs1)+2*a*b*(vs1))) +
            tf*(log(1.0-vs1)-log(1.0-vs2));
    }

    return(td);
}




double SIM_power_driver_size(double driving_cap, double desiredrisetime)
{
  double nsize, psize;
  double Rpdrive; 

  Rpdrive = desiredrisetime/(driving_cap*log(PARM_VSINV)*-1.0);
  psize = SIM_power_restowidth(Rpdrive,PCH);
  nsize = SIM_power_restowidth(Rpdrive,NCH);
  if (psize > Wworddrivemax) {
    psize = Wworddrivemax;
  }
  if (psize < 4.0 * LSCALE)
    psize = 4.0 * LSCALE;

  return (psize);
}


