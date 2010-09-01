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
 * FileName:  ix.C
 * Synopsis:  in-order execution (i.e. actions for each instruction)
 * Author:    cmauer
 * Version:   $Id: ix.C,v 3.2 2003/03/27 23:12:50 cmauer Exp $
 */

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include "hfa.h"
#include "hfacore.h"
#include "flatarf.h"
#include "flow.h"

// need to access condition code functions
#include "ccops.h"
#include "ix.h"

/*------------------------------------------------------------------------*/
/* Static methods                                                         */
/*------------------------------------------------------------------------*/

/*------------------------------------------------------------------------*/
/* Macro declarations                                                     */
/*------------------------------------------------------------------------*/

//***************************************************************************
#define  CONTROL_OP_TYPE flow_inst_t
#define  CONTROL_OP_GET  this

//***************************************************************************
#define  MEMORY_OP_TYPE flow_inst_t
#define  MEMORY_OP_GET  this

//***************************************************************************
#define  STORE_OP_TYPE flow_inst_t
#define  STORE_OP_GET  this

// toggle the floating point status register bit (not implemented currently)
//***************************************************************************
#define  UPDATE_FSR                                  \
    IREGISTER status_reg;                            \
    READ_SELECT_CONTROL( CONTROL_FSR, status_reg );  \
    us_read_fsr( &status_reg );                      \
    WRITE_SELECT_CONTROL( CONTROL_FSR, status_reg );

/*------------------------------------------------------------------------*/
/* Global functions                                                       */
/*------------------------------------------------------------------------*/

#undef  STAT_INC
// no definition for STAT_INC so not statistics will be incremented
#define STAT_INC( A )

//***************************************************************************
void flow_inst_t::ix_build_jump_table( void )
{
  for (uint32 i = 0; i < i_maxcount; i++) {
    m_jump_table[i] = &flow_inst_t::dx_ill;
  }
  m_jump_table[i_add] = &flow_inst_t::dx_add;
  m_jump_table[i_addcc] = &flow_inst_t::dx_addcc;
  m_jump_table[i_addc] = &flow_inst_t::dx_addc;
  m_jump_table[i_addccc] = &flow_inst_t::dx_addccc;
  m_jump_table[i_and] = &flow_inst_t::dx_and;
  m_jump_table[i_andcc] = &flow_inst_t::dx_andcc;
  m_jump_table[i_andn] = &flow_inst_t::dx_andn;
  m_jump_table[i_andncc] = &flow_inst_t::dx_andncc;
  m_jump_table[i_fba] = &flow_inst_t::dx_fba;
  m_jump_table[i_ba] = &flow_inst_t::dx_ba;
  m_jump_table[i_fbpa] = &flow_inst_t::dx_fbpa;
  m_jump_table[i_bpa] = &flow_inst_t::dx_bpa;
  m_jump_table[i_fbn] = &flow_inst_t::dx_fbn;
  m_jump_table[i_fbpn] = &flow_inst_t::dx_fbpn;
  m_jump_table[i_bn] = &flow_inst_t::dx_bn;
  m_jump_table[i_bpn] = &flow_inst_t::dx_bpn;
  m_jump_table[i_bpne] = &flow_inst_t::dx_bpne;
  m_jump_table[i_bpe] = &flow_inst_t::dx_bpe;
  m_jump_table[i_bpg] = &flow_inst_t::dx_bpg;
  m_jump_table[i_bple] = &flow_inst_t::dx_bple;
  m_jump_table[i_bpge] = &flow_inst_t::dx_bpge;
  m_jump_table[i_bpl] = &flow_inst_t::dx_bpl;
  m_jump_table[i_bpgu] = &flow_inst_t::dx_bpgu;
  m_jump_table[i_bpleu] = &flow_inst_t::dx_bpleu;
  m_jump_table[i_bpcc] = &flow_inst_t::dx_bpcc;
  m_jump_table[i_bpcs] = &flow_inst_t::dx_bpcs;
  m_jump_table[i_bppos] = &flow_inst_t::dx_bppos;
  m_jump_table[i_bpneg] = &flow_inst_t::dx_bpneg;
  m_jump_table[i_bpvc] = &flow_inst_t::dx_bpvc;
  m_jump_table[i_bpvs] = &flow_inst_t::dx_bpvs;
  m_jump_table[i_call] = &flow_inst_t::dx_call;
  m_jump_table[i_casa] = &flow_inst_t::dx_casa;
  m_jump_table[i_casxa] = &flow_inst_t::dx_casxa;
  m_jump_table[i_done] = &flow_inst_t::dx_doneretry;
  m_jump_table[i_retry] = &flow_inst_t::dx_doneretry;
  m_jump_table[i_jmpl] = &flow_inst_t::dx_jmpl;
  m_jump_table[i_fabss] = &flow_inst_t::dx_fabss;
  m_jump_table[i_fabsd] = &flow_inst_t::dx_fabsd;
  m_jump_table[i_fabsq] = &flow_inst_t::dx_fabsq;
  m_jump_table[i_fadds] = &flow_inst_t::dx_fadds;
  m_jump_table[i_faddd] = &flow_inst_t::dx_faddd;
  m_jump_table[i_faddq] = &flow_inst_t::dx_faddq;
  m_jump_table[i_fsubs] = &flow_inst_t::dx_fsubs;
  m_jump_table[i_fsubd] = &flow_inst_t::dx_fsubd;
  m_jump_table[i_fsubq] = &flow_inst_t::dx_fsubq;
  m_jump_table[i_fcmps] = &flow_inst_t::dx_fcmps;
  m_jump_table[i_fcmpd] = &flow_inst_t::dx_fcmpd;
  m_jump_table[i_fcmpq] = &flow_inst_t::dx_fcmpq;
  m_jump_table[i_fcmpes] = &flow_inst_t::dx_fcmpes;
  m_jump_table[i_fcmped] = &flow_inst_t::dx_fcmped;
  m_jump_table[i_fcmpeq] = &flow_inst_t::dx_fcmpeq;
  m_jump_table[i_fmovs] = &flow_inst_t::dx_fmovs;
  m_jump_table[i_fmovd] = &flow_inst_t::dx_fmovd;
  m_jump_table[i_fmovq] = &flow_inst_t::dx_fmovq;
  m_jump_table[i_fnegs] = &flow_inst_t::dx_fnegs;
  m_jump_table[i_fnegd] = &flow_inst_t::dx_fnegd;
  m_jump_table[i_fnegq] = &flow_inst_t::dx_fnegq;
  m_jump_table[i_fmuls] = &flow_inst_t::dx_fmuls;
  m_jump_table[i_fmuld] = &flow_inst_t::dx_fmuld;
  m_jump_table[i_fmulq] = &flow_inst_t::dx_fmulq;
  m_jump_table[i_fdivs] = &flow_inst_t::dx_fdivs;
  m_jump_table[i_fdivd] = &flow_inst_t::dx_fdivd;
  m_jump_table[i_fdivq] = &flow_inst_t::dx_fdivq;
  m_jump_table[i_fsmuld] = &flow_inst_t::dx_fsmuld;
  m_jump_table[i_fdmulq] = &flow_inst_t::dx_fdmulq;
  m_jump_table[i_fsqrts] = &flow_inst_t::dx_fsqrts;
  m_jump_table[i_fsqrtd] = &flow_inst_t::dx_fsqrtd;
  m_jump_table[i_fsqrtq] = &flow_inst_t::dx_fsqrtq;
  m_jump_table[i_retrn] = &flow_inst_t::dx_retrn;
  m_jump_table[i_brz] = &flow_inst_t::dx_brz;
  m_jump_table[i_brlez] = &flow_inst_t::dx_brlez;
  m_jump_table[i_brlz] = &flow_inst_t::dx_brlz;
  m_jump_table[i_brnz] = &flow_inst_t::dx_brnz;
  m_jump_table[i_brgz] = &flow_inst_t::dx_brgz;
  m_jump_table[i_brgez] = &flow_inst_t::dx_brgez;
  m_jump_table[i_fbu] = &flow_inst_t::dx_fbu;
  m_jump_table[i_fbg] = &flow_inst_t::dx_fbg;
  m_jump_table[i_fbug] = &flow_inst_t::dx_fbug;
  m_jump_table[i_fbl] = &flow_inst_t::dx_fbl;
  m_jump_table[i_fbul] = &flow_inst_t::dx_fbul;
  m_jump_table[i_fblg] = &flow_inst_t::dx_fblg;
  m_jump_table[i_fbne] = &flow_inst_t::dx_fbne;
  m_jump_table[i_fbe] = &flow_inst_t::dx_fbe;
  m_jump_table[i_fbue] = &flow_inst_t::dx_fbue;
  m_jump_table[i_fbge] = &flow_inst_t::dx_fbge;
  m_jump_table[i_fbuge] = &flow_inst_t::dx_fbuge;
  m_jump_table[i_fble] = &flow_inst_t::dx_fble;
  m_jump_table[i_fbule] = &flow_inst_t::dx_fbule;
  m_jump_table[i_fbo] = &flow_inst_t::dx_fbo;
  m_jump_table[i_fbpu] = &flow_inst_t::dx_fbpu;
  m_jump_table[i_fbpg] = &flow_inst_t::dx_fbpg;
  m_jump_table[i_fbpug] = &flow_inst_t::dx_fbpug;
  m_jump_table[i_fbpl] = &flow_inst_t::dx_fbpl;
  m_jump_table[i_fbpul] = &flow_inst_t::dx_fbpul;
  m_jump_table[i_fbplg] = &flow_inst_t::dx_fbplg;
  m_jump_table[i_fbpne] = &flow_inst_t::dx_fbpne;
  m_jump_table[i_fbpe] = &flow_inst_t::dx_fbpe;
  m_jump_table[i_fbpue] = &flow_inst_t::dx_fbpue;
  m_jump_table[i_fbpge] = &flow_inst_t::dx_fbpge;
  m_jump_table[i_fbpuge] = &flow_inst_t::dx_fbpuge;
  m_jump_table[i_fbple] = &flow_inst_t::dx_fbple;
  m_jump_table[i_fbpule] = &flow_inst_t::dx_fbpule;
  m_jump_table[i_fbpo] = &flow_inst_t::dx_fbpo;
  m_jump_table[i_bne] = &flow_inst_t::dx_bne;
  m_jump_table[i_be] = &flow_inst_t::dx_be;
  m_jump_table[i_bg] = &flow_inst_t::dx_bg;
  m_jump_table[i_ble] = &flow_inst_t::dx_ble;
  m_jump_table[i_bge] = &flow_inst_t::dx_bge;
  m_jump_table[i_bl] = &flow_inst_t::dx_bl;
  m_jump_table[i_bgu] = &flow_inst_t::dx_bgu;
  m_jump_table[i_bleu] = &flow_inst_t::dx_bleu;
  m_jump_table[i_bcc] = &flow_inst_t::dx_bcc;
  m_jump_table[i_bcs] = &flow_inst_t::dx_bcs;
  m_jump_table[i_bpos] = &flow_inst_t::dx_bpos;
  m_jump_table[i_bneg] = &flow_inst_t::dx_bneg;
  m_jump_table[i_bvc] = &flow_inst_t::dx_bvc;
  m_jump_table[i_bvs] = &flow_inst_t::dx_bvs;
  m_jump_table[i_fstox] = &flow_inst_t::dx_fstox;
  m_jump_table[i_fdtox] = &flow_inst_t::dx_fdtox;
  m_jump_table[i_fqtox] = &flow_inst_t::dx_fqtox;
  m_jump_table[i_fstoi] = &flow_inst_t::dx_fstoi;
  m_jump_table[i_fdtoi] = &flow_inst_t::dx_fdtoi;
  m_jump_table[i_fqtoi] = &flow_inst_t::dx_fqtoi;
  m_jump_table[i_fstod] = &flow_inst_t::dx_fstod;
  m_jump_table[i_fstoq] = &flow_inst_t::dx_fstoq;
  m_jump_table[i_fdtos] = &flow_inst_t::dx_fdtos;
  m_jump_table[i_fdtoq] = &flow_inst_t::dx_fdtoq;
  m_jump_table[i_fqtos] = &flow_inst_t::dx_fqtos;
  m_jump_table[i_fqtod] = &flow_inst_t::dx_fqtod;
  m_jump_table[i_fxtos] = &flow_inst_t::dx_fxtos;
  m_jump_table[i_fxtod] = &flow_inst_t::dx_fxtod;
  m_jump_table[i_fxtoq] = &flow_inst_t::dx_fxtoq;
  m_jump_table[i_fitos] = &flow_inst_t::dx_fitos;
  m_jump_table[i_fitod] = &flow_inst_t::dx_fitod;
  m_jump_table[i_fitoq] = &flow_inst_t::dx_fitoq;
  m_jump_table[i_fmovfsn] = &flow_inst_t::dx_fmovfsn;
  m_jump_table[i_fmovfsne] = &flow_inst_t::dx_fmovfsne;
  m_jump_table[i_fmovfslg] = &flow_inst_t::dx_fmovfslg;
  m_jump_table[i_fmovfsul] = &flow_inst_t::dx_fmovfsul;
  m_jump_table[i_fmovfsl] = &flow_inst_t::dx_fmovfsl;
  m_jump_table[i_fmovfsug] = &flow_inst_t::dx_fmovfsug;
  m_jump_table[i_fmovfsg] = &flow_inst_t::dx_fmovfsg;
  m_jump_table[i_fmovfsu] = &flow_inst_t::dx_fmovfsu;
  m_jump_table[i_fmovfsa] = &flow_inst_t::dx_fmovfsa;
  m_jump_table[i_fmovfse] = &flow_inst_t::dx_fmovfse;
  m_jump_table[i_fmovfsue] = &flow_inst_t::dx_fmovfsue;
  m_jump_table[i_fmovfsge] = &flow_inst_t::dx_fmovfsge;
  m_jump_table[i_fmovfsuge] = &flow_inst_t::dx_fmovfsuge;
  m_jump_table[i_fmovfsle] = &flow_inst_t::dx_fmovfsle;
  m_jump_table[i_fmovfsule] = &flow_inst_t::dx_fmovfsule;
  m_jump_table[i_fmovfso] = &flow_inst_t::dx_fmovfso;
  m_jump_table[i_fmovfdn] = &flow_inst_t::dx_fmovfdn;
  m_jump_table[i_fmovfdne] = &flow_inst_t::dx_fmovfdne;
  m_jump_table[i_fmovfdlg] = &flow_inst_t::dx_fmovfdlg;
  m_jump_table[i_fmovfdul] = &flow_inst_t::dx_fmovfdul;
  m_jump_table[i_fmovfdl] = &flow_inst_t::dx_fmovfdl;
  m_jump_table[i_fmovfdug] = &flow_inst_t::dx_fmovfdug;
  m_jump_table[i_fmovfdg] = &flow_inst_t::dx_fmovfdg;
  m_jump_table[i_fmovfdu] = &flow_inst_t::dx_fmovfdu;
  m_jump_table[i_fmovfda] = &flow_inst_t::dx_fmovfda;
  m_jump_table[i_fmovfde] = &flow_inst_t::dx_fmovfde;
  m_jump_table[i_fmovfdue] = &flow_inst_t::dx_fmovfdue;
  m_jump_table[i_fmovfdge] = &flow_inst_t::dx_fmovfdge;
  m_jump_table[i_fmovfduge] = &flow_inst_t::dx_fmovfduge;
  m_jump_table[i_fmovfdle] = &flow_inst_t::dx_fmovfdle;
  m_jump_table[i_fmovfdule] = &flow_inst_t::dx_fmovfdule;
  m_jump_table[i_fmovfdo] = &flow_inst_t::dx_fmovfdo;
  m_jump_table[i_fmovfqn] = &flow_inst_t::dx_fmovfqn;
  m_jump_table[i_fmovfqne] = &flow_inst_t::dx_fmovfqne;
  m_jump_table[i_fmovfqlg] = &flow_inst_t::dx_fmovfqlg;
  m_jump_table[i_fmovfqul] = &flow_inst_t::dx_fmovfqul;
  m_jump_table[i_fmovfql] = &flow_inst_t::dx_fmovfql;
  m_jump_table[i_fmovfqug] = &flow_inst_t::dx_fmovfqug;
  m_jump_table[i_fmovfqg] = &flow_inst_t::dx_fmovfqg;
  m_jump_table[i_fmovfqu] = &flow_inst_t::dx_fmovfqu;
  m_jump_table[i_fmovfqa] = &flow_inst_t::dx_fmovfqa;
  m_jump_table[i_fmovfqe] = &flow_inst_t::dx_fmovfqe;
  m_jump_table[i_fmovfque] = &flow_inst_t::dx_fmovfque;
  m_jump_table[i_fmovfqge] = &flow_inst_t::dx_fmovfqge;
  m_jump_table[i_fmovfquge] = &flow_inst_t::dx_fmovfquge;
  m_jump_table[i_fmovfqle] = &flow_inst_t::dx_fmovfqle;
  m_jump_table[i_fmovfqule] = &flow_inst_t::dx_fmovfqule;
  m_jump_table[i_fmovfqo] = &flow_inst_t::dx_fmovfqo;
  m_jump_table[i_fmovsn] = &flow_inst_t::dx_fmovsn;
  m_jump_table[i_fmovse] = &flow_inst_t::dx_fmovse;
  m_jump_table[i_fmovsle] = &flow_inst_t::dx_fmovsle;
  m_jump_table[i_fmovsl] = &flow_inst_t::dx_fmovsl;
  m_jump_table[i_fmovsleu] = &flow_inst_t::dx_fmovsleu;
  m_jump_table[i_fmovscs] = &flow_inst_t::dx_fmovscs;
  m_jump_table[i_fmovsneg] = &flow_inst_t::dx_fmovsneg;
  m_jump_table[i_fmovsvs] = &flow_inst_t::dx_fmovsvs;
  m_jump_table[i_fmovsa] = &flow_inst_t::dx_fmovsa;
  m_jump_table[i_fmovsne] = &flow_inst_t::dx_fmovsne;
  m_jump_table[i_fmovsg] = &flow_inst_t::dx_fmovsg;
  m_jump_table[i_fmovsge] = &flow_inst_t::dx_fmovsge;
  m_jump_table[i_fmovsgu] = &flow_inst_t::dx_fmovsgu;
  m_jump_table[i_fmovscc] = &flow_inst_t::dx_fmovscc;
  m_jump_table[i_fmovspos] = &flow_inst_t::dx_fmovspos;
  m_jump_table[i_fmovsvc] = &flow_inst_t::dx_fmovsvc;
  m_jump_table[i_fmovdn] = &flow_inst_t::dx_fmovdn;
  m_jump_table[i_fmovde] = &flow_inst_t::dx_fmovde;
  m_jump_table[i_fmovdle] = &flow_inst_t::dx_fmovdle;
  m_jump_table[i_fmovdl] = &flow_inst_t::dx_fmovdl;
  m_jump_table[i_fmovdleu] = &flow_inst_t::dx_fmovdleu;
  m_jump_table[i_fmovdcs] = &flow_inst_t::dx_fmovdcs;
  m_jump_table[i_fmovdneg] = &flow_inst_t::dx_fmovdneg;
  m_jump_table[i_fmovdvs] = &flow_inst_t::dx_fmovdvs;
  m_jump_table[i_fmovda] = &flow_inst_t::dx_fmovda;
  m_jump_table[i_fmovdne] = &flow_inst_t::dx_fmovdne;
  m_jump_table[i_fmovdg] = &flow_inst_t::dx_fmovdg;
  m_jump_table[i_fmovdge] = &flow_inst_t::dx_fmovdge;
  m_jump_table[i_fmovdgu] = &flow_inst_t::dx_fmovdgu;
  m_jump_table[i_fmovdcc] = &flow_inst_t::dx_fmovdcc;
  m_jump_table[i_fmovdpos] = &flow_inst_t::dx_fmovdpos;
  m_jump_table[i_fmovdvc] = &flow_inst_t::dx_fmovdvc;
  m_jump_table[i_fmovqn] = &flow_inst_t::dx_fmovqn;
  m_jump_table[i_fmovqe] = &flow_inst_t::dx_fmovqe;
  m_jump_table[i_fmovqle] = &flow_inst_t::dx_fmovqle;
  m_jump_table[i_fmovql] = &flow_inst_t::dx_fmovql;
  m_jump_table[i_fmovqleu] = &flow_inst_t::dx_fmovqleu;
  m_jump_table[i_fmovqcs] = &flow_inst_t::dx_fmovqcs;
  m_jump_table[i_fmovqneg] = &flow_inst_t::dx_fmovqneg;
  m_jump_table[i_fmovqvs] = &flow_inst_t::dx_fmovqvs;
  m_jump_table[i_fmovqa] = &flow_inst_t::dx_fmovqa;
  m_jump_table[i_fmovqne] = &flow_inst_t::dx_fmovqne;
  m_jump_table[i_fmovqg] = &flow_inst_t::dx_fmovqg;
  m_jump_table[i_fmovqge] = &flow_inst_t::dx_fmovqge;
  m_jump_table[i_fmovqgu] = &flow_inst_t::dx_fmovqgu;
  m_jump_table[i_fmovqcc] = &flow_inst_t::dx_fmovqcc;
  m_jump_table[i_fmovqpos] = &flow_inst_t::dx_fmovqpos;
  m_jump_table[i_fmovqvc] = &flow_inst_t::dx_fmovqvc;
  m_jump_table[i_fmovrsz] = &flow_inst_t::dx_fmovrsz;
  m_jump_table[i_fmovrslez] = &flow_inst_t::dx_fmovrslez;
  m_jump_table[i_fmovrslz] = &flow_inst_t::dx_fmovrslz;
  m_jump_table[i_fmovrsnz] = &flow_inst_t::dx_fmovrsnz;
  m_jump_table[i_fmovrsgz] = &flow_inst_t::dx_fmovrsgz;
  m_jump_table[i_fmovrsgez] = &flow_inst_t::dx_fmovrsgez;
  m_jump_table[i_fmovrdz] = &flow_inst_t::dx_fmovrdz;
  m_jump_table[i_fmovrdlez] = &flow_inst_t::dx_fmovrdlez;
  m_jump_table[i_fmovrdlz] = &flow_inst_t::dx_fmovrdlz;
  m_jump_table[i_fmovrdnz] = &flow_inst_t::dx_fmovrdnz;
  m_jump_table[i_fmovrdgz] = &flow_inst_t::dx_fmovrdgz;
  m_jump_table[i_fmovrdgez] = &flow_inst_t::dx_fmovrdgez;
  m_jump_table[i_fmovrqz] = &flow_inst_t::dx_fmovrqz;
  m_jump_table[i_fmovrqlez] = &flow_inst_t::dx_fmovrqlez;
  m_jump_table[i_fmovrqlz] = &flow_inst_t::dx_fmovrqlz;
  m_jump_table[i_fmovrqnz] = &flow_inst_t::dx_fmovrqnz;
  m_jump_table[i_fmovrqgz] = &flow_inst_t::dx_fmovrqgz;
  m_jump_table[i_fmovrqgez] = &flow_inst_t::dx_fmovrqgez;
  m_jump_table[i_mova] = &flow_inst_t::dx_mova;
  m_jump_table[i_movfa] = &flow_inst_t::dx_movfa;
  m_jump_table[i_movn] = &flow_inst_t::dx_movn;
  m_jump_table[i_movfn] = &flow_inst_t::dx_movfn;
  m_jump_table[i_movne] = &flow_inst_t::dx_movne;
  m_jump_table[i_move] = &flow_inst_t::dx_move;
  m_jump_table[i_movg] = &flow_inst_t::dx_movg;
  m_jump_table[i_movle] = &flow_inst_t::dx_movle;
  m_jump_table[i_movge] = &flow_inst_t::dx_movge;
  m_jump_table[i_movl] = &flow_inst_t::dx_movl;
  m_jump_table[i_movgu] = &flow_inst_t::dx_movgu;
  m_jump_table[i_movleu] = &flow_inst_t::dx_movleu;
  m_jump_table[i_movcc] = &flow_inst_t::dx_movcc;
  m_jump_table[i_movcs] = &flow_inst_t::dx_movcs;
  m_jump_table[i_movpos] = &flow_inst_t::dx_movpos;
  m_jump_table[i_movneg] = &flow_inst_t::dx_movneg;
  m_jump_table[i_movvc] = &flow_inst_t::dx_movvc;
  m_jump_table[i_movvs] = &flow_inst_t::dx_movvs;
  m_jump_table[i_movfu] = &flow_inst_t::dx_movfu;
  m_jump_table[i_movfg] = &flow_inst_t::dx_movfg;
  m_jump_table[i_movfug] = &flow_inst_t::dx_movfug;
  m_jump_table[i_movfl] = &flow_inst_t::dx_movfl;
  m_jump_table[i_movful] = &flow_inst_t::dx_movful;
  m_jump_table[i_movflg] = &flow_inst_t::dx_movflg;
  m_jump_table[i_movfne] = &flow_inst_t::dx_movfne;
  m_jump_table[i_movfe] = &flow_inst_t::dx_movfe;
  m_jump_table[i_movfue] = &flow_inst_t::dx_movfue;
  m_jump_table[i_movfge] = &flow_inst_t::dx_movfge;
  m_jump_table[i_movfuge] = &flow_inst_t::dx_movfuge;
  m_jump_table[i_movfle] = &flow_inst_t::dx_movfle;
  m_jump_table[i_movfule] = &flow_inst_t::dx_movfule;
  m_jump_table[i_movfo] = &flow_inst_t::dx_movfo;
  m_jump_table[i_movrz] = &flow_inst_t::dx_movrz;
  m_jump_table[i_movrlez] = &flow_inst_t::dx_movrlez;
  m_jump_table[i_movrlz] = &flow_inst_t::dx_movrlz;
  m_jump_table[i_movrnz] = &flow_inst_t::dx_movrnz;
  m_jump_table[i_movrgz] = &flow_inst_t::dx_movrgz;
  m_jump_table[i_movrgez] = &flow_inst_t::dx_movrgez;
  m_jump_table[i_ta] = &flow_inst_t::dx_ta;
  m_jump_table[i_tn] = &flow_inst_t::dx_tn;
  m_jump_table[i_tne] = &flow_inst_t::dx_tne;
  m_jump_table[i_te] = &flow_inst_t::dx_te;
  m_jump_table[i_tg] = &flow_inst_t::dx_tg;
  m_jump_table[i_tle] = &flow_inst_t::dx_tle;
  m_jump_table[i_tge] = &flow_inst_t::dx_tge;
  m_jump_table[i_tl] = &flow_inst_t::dx_tl;
  m_jump_table[i_tgu] = &flow_inst_t::dx_tgu;
  m_jump_table[i_tleu] = &flow_inst_t::dx_tleu;
  m_jump_table[i_tcc] = &flow_inst_t::dx_tcc;
  m_jump_table[i_tcs] = &flow_inst_t::dx_tcs;
  m_jump_table[i_tpos] = &flow_inst_t::dx_tpos;
  m_jump_table[i_tneg] = &flow_inst_t::dx_tneg;
  m_jump_table[i_tvc] = &flow_inst_t::dx_tvc;
  m_jump_table[i_tvs] = &flow_inst_t::dx_tvs;
  m_jump_table[i_sub] = &flow_inst_t::dx_sub;
  m_jump_table[i_subcc] = &flow_inst_t::dx_subcc;
  m_jump_table[i_subc] = &flow_inst_t::dx_subc;
  m_jump_table[i_subccc] = &flow_inst_t::dx_subccc;
  m_jump_table[i_or] = &flow_inst_t::dx_or;
  m_jump_table[i_orcc] = &flow_inst_t::dx_orcc;
  m_jump_table[i_orn] = &flow_inst_t::dx_orn;
  m_jump_table[i_orncc] = &flow_inst_t::dx_orncc;
  m_jump_table[i_xor] = &flow_inst_t::dx_xor;
  m_jump_table[i_xorcc] = &flow_inst_t::dx_xorcc;
  m_jump_table[i_xnor] = &flow_inst_t::dx_xnor;
  m_jump_table[i_xnorcc] = &flow_inst_t::dx_xnorcc;
  m_jump_table[i_mulx] = &flow_inst_t::dx_mulx;
  m_jump_table[i_sdivx] = &flow_inst_t::dx_sdivx;
  m_jump_table[i_udivx] = &flow_inst_t::dx_udivx;
  m_jump_table[i_sll] = &flow_inst_t::dx_sll;
  m_jump_table[i_srl] = &flow_inst_t::dx_srl;
  m_jump_table[i_sra] = &flow_inst_t::dx_sra;
  m_jump_table[i_sllx] = &flow_inst_t::dx_sllx;
  m_jump_table[i_srlx] = &flow_inst_t::dx_srlx;
  m_jump_table[i_srax] = &flow_inst_t::dx_srax;
  m_jump_table[i_taddcc] = &flow_inst_t::dx_taddcc;
  m_jump_table[i_taddcctv] = &flow_inst_t::dx_taddcctv;
  m_jump_table[i_tsubcc] = &flow_inst_t::dx_tsubcc;
  m_jump_table[i_tsubcctv] = &flow_inst_t::dx_tsubcctv;
  m_jump_table[i_udiv] = &flow_inst_t::dx_udiv;
  m_jump_table[i_sdiv] = &flow_inst_t::dx_sdiv;
  m_jump_table[i_umul] = &flow_inst_t::dx_umul;
  m_jump_table[i_smul] = &flow_inst_t::dx_smul;
  m_jump_table[i_udivcc] = &flow_inst_t::dx_udivcc;
  m_jump_table[i_sdivcc] = &flow_inst_t::dx_sdivcc;
  m_jump_table[i_umulcc] = &flow_inst_t::dx_umulcc;
  m_jump_table[i_smulcc] = &flow_inst_t::dx_smulcc;
  m_jump_table[i_mulscc] = &flow_inst_t::dx_mulscc;
  m_jump_table[i_popc] = &flow_inst_t::dx_popc;
  m_jump_table[i_flush] = &flow_inst_t::dx_flush;
  m_jump_table[i_flushw] = &flow_inst_t::dx_flushw;
  m_jump_table[i_rd] = &flow_inst_t::dx_rd;
  m_jump_table[i_rdcc] = &flow_inst_t::dx_rdcc;
  m_jump_table[i_wr] = &flow_inst_t::dx_wr;
  m_jump_table[i_wrcc] = &flow_inst_t::dx_wrcc;
  m_jump_table[i_save] = &flow_inst_t::dx_save;
  m_jump_table[i_restore] = &flow_inst_t::dx_restore;
  m_jump_table[i_saved] = &flow_inst_t::dx_saved;
  m_jump_table[i_restored] = &flow_inst_t::dx_restored;
  m_jump_table[i_sethi] = &flow_inst_t::dx_sethi;
  m_jump_table[i_ldf] = &flow_inst_t::dx_ldf;
  m_jump_table[i_lddf] = &flow_inst_t::dx_lddf;
  m_jump_table[i_ldqf] = &flow_inst_t::dx_ldqf;
  m_jump_table[i_ldfa] = &flow_inst_t::dx_ldfa;
  m_jump_table[i_lddfa] = &flow_inst_t::dx_lddfa;
  m_jump_table[i_ldblk] = &flow_inst_t::dx_ldblk;
  m_jump_table[i_ldqfa] = &flow_inst_t::dx_ldqfa;
  m_jump_table[i_ldsb] = &flow_inst_t::dx_ldsb;
  m_jump_table[i_ldsh] = &flow_inst_t::dx_ldsh;
  m_jump_table[i_ldsw] = &flow_inst_t::dx_ldsw;
  m_jump_table[i_ldub] = &flow_inst_t::dx_ldub;
  m_jump_table[i_lduh] = &flow_inst_t::dx_lduh;
  m_jump_table[i_lduw] = &flow_inst_t::dx_lduw;
  m_jump_table[i_ldx] = &flow_inst_t::dx_ldx;
  m_jump_table[i_ldd] = &flow_inst_t::dx_ldd;
  m_jump_table[i_ldsba] = &flow_inst_t::dx_ldsba;
  m_jump_table[i_ldsha] = &flow_inst_t::dx_ldsha;
  m_jump_table[i_ldswa] = &flow_inst_t::dx_ldswa;
  m_jump_table[i_lduba] = &flow_inst_t::dx_lduba;
  m_jump_table[i_lduha] = &flow_inst_t::dx_lduha;
  m_jump_table[i_lduwa] = &flow_inst_t::dx_lduwa;
  m_jump_table[i_ldxa] = &flow_inst_t::dx_ldxa;
  m_jump_table[i_ldda] = &flow_inst_t::dx_ldda;
  m_jump_table[i_ldqa] = &flow_inst_t::dx_ldqa;
  m_jump_table[i_stf] = &flow_inst_t::dx_stf;
  m_jump_table[i_stdf] = &flow_inst_t::dx_stdf;
  m_jump_table[i_stqf] = &flow_inst_t::dx_stqf;
  m_jump_table[i_stb] = &flow_inst_t::dx_stb;
  m_jump_table[i_sth] = &flow_inst_t::dx_sth;
  m_jump_table[i_stw] = &flow_inst_t::dx_stw;
  m_jump_table[i_stx] = &flow_inst_t::dx_stx;
  m_jump_table[i_std] = &flow_inst_t::dx_std;
  m_jump_table[i_stfa] = &flow_inst_t::dx_stfa;
  m_jump_table[i_stdfa] = &flow_inst_t::dx_stdfa;
  m_jump_table[i_stblk] = &flow_inst_t::dx_stblk;
  m_jump_table[i_stqfa] = &flow_inst_t::dx_stqfa;
  m_jump_table[i_stba] = &flow_inst_t::dx_stba;
  m_jump_table[i_stha] = &flow_inst_t::dx_stha;
  m_jump_table[i_stwa] = &flow_inst_t::dx_stwa;
  m_jump_table[i_stxa] = &flow_inst_t::dx_stxa;
  m_jump_table[i_stda] = &flow_inst_t::dx_stda;
  m_jump_table[i_ldstub] = &flow_inst_t::dx_ldstub;
  m_jump_table[i_ldstuba] = &flow_inst_t::dx_ldstuba;
  m_jump_table[i_prefetch] = &flow_inst_t::dx_prefetch;
  m_jump_table[i_prefetcha] = &flow_inst_t::dx_prefetcha;
  m_jump_table[i_swap] = &flow_inst_t::dx_swap;
  m_jump_table[i_swapa] = &flow_inst_t::dx_swapa;
  m_jump_table[i_ldfsr] = &flow_inst_t::dx_ldfsr;
  m_jump_table[i_ldxfsr] = &flow_inst_t::dx_ldxfsr;
  m_jump_table[i_stfsr] = &flow_inst_t::dx_stfsr;
  m_jump_table[i_stxfsr] = &flow_inst_t::dx_stxfsr;
  m_jump_table[i__trap] = &flow_inst_t::dx__trap;
  m_jump_table[i_impdep1] = &flow_inst_t::dx_impdep1;
  m_jump_table[i_impdep2] = &flow_inst_t::dx_impdep2;
  m_jump_table[i_membar] = &flow_inst_t::dx_membar;
  m_jump_table[i_stbar] = &flow_inst_t::dx_stbar;
  m_jump_table[i_cmp] = &flow_inst_t::dx_cmp;
  m_jump_table[i_jmp] = &flow_inst_t::dx_jmp;
  m_jump_table[i_mov] = &flow_inst_t::dx_mov;
  m_jump_table[i_nop] = &flow_inst_t::dx_nop;
  m_jump_table[i_not] = &flow_inst_t::dx_not;
  m_jump_table[i_rdpr] = &flow_inst_t::dx_rdpr;
  m_jump_table[i_wrpr] = &flow_inst_t::dx_wrpr;
  m_jump_table[i_faligndata] = &flow_inst_t::dx_faligndata;
  m_jump_table[i_alignaddr] = &flow_inst_t::dx_alignaddr;
  m_jump_table[i_alignaddrl] = &flow_inst_t::dx_alignaddrl;
  m_jump_table[i_fzero] = &flow_inst_t::dx_fzero;
  m_jump_table[i_fzeros] = &flow_inst_t::dx_fzeros;
  m_jump_table[i_fsrc1] = &flow_inst_t::dx_fsrc1;
  m_jump_table[i_fcmple16] = &flow_inst_t::dx_fcmple16;
  m_jump_table[i_fcmpne16] = &flow_inst_t::dx_fcmpne16;
  m_jump_table[i_fcmple32] = &flow_inst_t::dx_fcmple32;
  m_jump_table[i_fcmpne32] = &flow_inst_t::dx_fcmpne32;
  m_jump_table[i_fcmpgt16] = &flow_inst_t::dx_fcmpgt16;
  m_jump_table[i_fcmpeq16] = &flow_inst_t::dx_fcmpeq16;
  m_jump_table[i_fcmpgt32] = &flow_inst_t::dx_fcmpgt32;
  m_jump_table[i_fcmpeq32] = &flow_inst_t::dx_fcmpeq32;
  m_jump_table[i_bshuffle] = &flow_inst_t::dx_bshuffle;
  m_jump_table[i_bmask] = &flow_inst_t::dx_bmask;
  m_jump_table[i_ill] = &flow_inst_t::dx_ill;
}

#include "ix.i"
