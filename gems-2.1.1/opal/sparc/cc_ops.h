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
#ifndef _CC_OPS_H_
#define _CC_OPS_H_

/* Portions of this code:: */
/******************************************************************************
** $RCSfile: s.cc_ops.h $ -- Part of the FastSim simulation system.
** Interface to the fastsim runtime system.
**
** {COPYRIGHT:1999
** Copyright (c) 1999 by Eric Schnarr (schnarr@cs.wisc.edu).
** ALL RIGHTS RESERVED.
** 
** This software is furnished under a license and may be used and
** copied only in accordance with the terms of such license and the
** inclusion of the above copyright notice. This software or any other
** copies thereof or any derivative works may not be provided or
** otherwise made available to any other persons. Title to and ownership
** of the software is retained by Eric Schnarr.
** 
** This software is provided "as is". The licensor makes no
** warrenties about its correctness or performance.
** 
** DO NOT DISTRIBUTE. Direct all inquiries to:
** 
** Eric Schnarr
** Computer Sciences Department
** University of Wisconsin
** 1210 West Dayton Street
** Madison, WI 53706
** schnarr@cs.wisc.edu
** 608-262-2542
** }
**
** $Id: cc_ops.h 1.1 03/03/27 23:08:16-00:00 cmauer@cottons.cs.wisc.edu $
*/

extern "C" {

  /* operators on signed/unsigned long values that set condition codes */
  ulong_t uadd32_cc(ulong_t, ulong_t, uchar_t*);
  long sadd32_cc(long, long, uchar_t*);
  ulong_t usub32_cc(ulong_t, ulong_t, uchar_t*);
  long ssub32_cc(long, long, uchar_t*);
  ulong_t uand32_cc(ulong_t, ulong_t, uchar_t*);
  ulong_t uor32_cc(ulong_t, ulong_t, uchar_t*);
  ulong_t uxor32_cc(ulong_t, ulong_t, uchar_t*);
  
  /* operators on signed/unsigned long values that set condition codes */
  u_longlong_t uadd64_cc(u_longlong_t, u_longlong_t, uchar_t*);
  longlong_t sadd64_cc(longlong_t, longlong_t, uchar_t*);
  u_longlong_t usub64_cc(u_longlong_t, u_longlong_t, uchar_t*);
  longlong_t ssub64_cc(longlong_t, longlong_t, uchar_t*);
  u_longlong_t uand64_cc(u_longlong_t, u_longlong_t, uchar_t*);
  u_longlong_t uor64_cc(u_longlong_t, u_longlong_t, uchar_t*);
  u_longlong_t uxor64_cc(u_longlong_t, u_longlong_t, uchar_t*);
  
  /* floating point operators that set condition codes */
  float fsub32_cc(float, float, uchar_t*);
  double fsub64_cc(double, double, uchar_t*);
};

#endif  /* _CC_OPS_H_ */
