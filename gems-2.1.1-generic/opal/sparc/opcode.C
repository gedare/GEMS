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
 * FileName:  opcode.C
 * Synopsis:  Decodes each opcode into a string
 * Author:    cmauer
 * Version:   $Id$
 */

/*------------------------------------------------------------------------*/
/* Includes                                                               */
/*------------------------------------------------------------------------*/

#include "hfa.h"
#include "opcode.h"

/*------------------------------------------------------------------------*/
/* Global variables                                                       */
/*------------------------------------------------------------------------*/

/// A table indicating verbose output for each instruction
bool g_dynamic_debug_table[i_maxcount];

/*------------------------------------------------------------------------*/
/* Global functions                                                       */
/*------------------------------------------------------------------------*/

//***************************************************************************
const char *decode_opcode( uint16 op )
{
  return ( decode_opcode( (enum i_opcode) op ) );
}

//***************************************************************************
const char *decode_opcode( enum i_opcode op )
{
  switch (op) {

  case i_add:
    return("add");
    break;
  case i_addcc:
    return("addcc");
    break;
  case i_addc:
    return("addc");
    break;
  case i_addccc:
    return("addccc");
    break;
  case i_and:
    return("and");
    break;
  case i_andcc:
    return("andcc");
    break;
  case i_andn:
    return("andn");
    break;
  case i_andncc:
    return("andncc");
    break;
  case i_fba:
    return("fba");
    break;
  case i_ba:
    return("ba");
    break;
  case i_fbpa:
    return("fbpa");
    break;
  case i_bpa:
    return("bpa");
    break;
  case i_fbn:
    return("fbn");
    break;
  case i_fbpn:
    return("fbpn");
    break;
  case i_bn:
    return("bn");
    break;
  case i_bpn:
    return("bpn");
    break;
  case i_bpne:
    return("bpne");
    break;
  case i_bpe:
    return("bpe");
    break;
  case i_bpg:
    return("bpg");
    break;
  case i_bple:
    return("bple");
    break;
  case i_bpge:
    return("bpge");
    break;
  case i_bpl:
    return("bpl");
    break;
  case i_bpgu:
    return("bpgu");
    break;
  case i_bpleu:
    return("bpleu");
    break;
  case i_bpcc:
    return("bpcc");
    break;
  case i_bpcs:
    return("bpcs");
    break;
  case i_bppos:
    return("bppos");
    break;
  case i_bpneg:
    return("bpneg");
    break;
  case i_bpvc:
    return("bpvc");
    break;
  case i_bpvs:
    return("bpvs");
    break;
  case i_call:
    return("call");
    break;
  case i_casa:
    return("casa");
    break;
  case i_casxa:
    return("casxa");
    break;
  case i_done:
    return("done");
    break;
  case i_jmpl:
    return("jmpl");
    break;
  case i_fabss:
    return("fabss");
    break;
  case i_fabsd:
    return("fabsd");
    break;
  case i_fabsq:
    return("fabsq");
    break;
  case i_fadds:
    return("fadds");
    break;
  case i_faddd:
    return("faddd");
    break;
  case i_faddq:
    return("faddq");
    break;
  case i_fsubs:
    return("fsubs");
    break;
  case i_fsubd:
    return("fsubd");
    break;
  case i_fsubq:
    return("fsubq");
    break;
  case i_fcmps:
    return("fcmps");
    break;
  case i_fcmpd:
    return("fcmpd");
    break;
  case i_fcmpq:
    return("fcmpq");
    break;
  case i_fcmpes:
    return("fcmpes");
    break;
  case i_fcmped:
    return("fcmped");
    break;
  case i_fcmpeq:
    return("fcmpeq");
    break;
  case i_fmovs:
    return("fmovs");
    break;
  case i_fmovd:
    return("fmovd");
    break;
  case i_fmovq:
    return("fmovq");
    break;
  case i_fnegs:
    return("fnegs");
    break;
  case i_fnegd:
    return("fnegd");
    break;
  case i_fnegq:
    return("fnegq");
    break;
  case i_fmuls:
    return("fmuls");
    break;
  case i_fmuld:
    return("fmuld");
    break;
  case i_fmulq:
    return("fmulq");
    break;
  case i_fdivs:
    return("fdivs");
    break;
  case i_fdivd:
    return("fdivd");
    break;
  case i_fdivq:
    return("fdivq");
    break;
  case i_fsmuld:
    return("fsmuld");
    break;
  case i_fdmulq:
    return("fdmulq");
    break;
  case i_fsqrts:
    return("fsqrts");
    break;
  case i_fsqrtd:
    return("fsqrtd");
    break;
  case i_fsqrtq:
    return("fsqrtq");
    break;
  case i_retrn:
    return("retrn");
    break;
  case i_brz:
    return("brz");
    break;
  case i_brlez:
    return("brlez");
    break;
  case i_brlz:
    return("brlz");
    break;
  case i_brnz:
    return("brnz");
    break;
  case i_brgz:
    return("brgz");
    break;
  case i_brgez:
    return("brgez");
    break;
  case i_fbu:
    return("fbu");
    break;
  case i_fbg:
    return("fbg");
    break;
  case i_fbug:
    return("fbug");
    break;
  case i_fbl:
    return("fbl");
    break;
  case i_fbul:
    return("fbul");
    break;
  case i_fblg:
    return("fblg");
    break;
  case i_fbne:
    return("fbne");
    break;
  case i_fbe:
    return("fbe");
    break;
  case i_fbue:
    return("fbue");
    break;
  case i_fbge:
    return("fbge");
    break;
  case i_fbuge:
    return("fbuge");
    break;
  case i_fble:
    return("fble");
    break;
  case i_fbule:
    return("fbule");
    break;
  case i_fbo:
    return("fbo");
    break;
  case i_fbpu:
    return("fbpu");
    break;
  case i_fbpg:
    return("fbpg");
    break;
  case i_fbpug:
    return("fbpug");
    break;
  case i_fbpl:
    return("fbpl");
    break;
  case i_fbpul:
    return("fbpul");
    break;
  case i_fbplg:
    return("fbplg");
    break;
  case i_fbpne:
    return("fbpne");
    break;
  case i_fbpe:
    return("fbpe");
    break;
  case i_fbpue:
    return("fbpue");
    break;
  case i_fbpge:
    return("fbpge");
    break;
  case i_fbpuge:
    return("fbpuge");
    break;
  case i_fbple:
    return("fbple");
    break;
  case i_fbpule:
    return("fbpule");
    break;
  case i_fbpo:
    return("fbpo");
    break;
  case i_bne:
    return("bne");
    break;
  case i_be:
    return("be");
    break;
  case i_bg:
    return("bg");
    break;
  case i_ble:
    return("ble");
    break;
  case i_bge:
    return("bge");
    break;
  case i_bl:
    return("bl");
    break;
  case i_bgu:
    return("bgu");
    break;
  case i_bleu:
    return("bleu");
    break;
  case i_bcc:
    return("bcc");
    break;
  case i_bcs:
    return("bcs");
    break;
  case i_bpos:
    return("bpos");
    break;
  case i_bneg:
    return("bneg");
    break;
  case i_bvc:
    return("bvc");
    break;
  case i_bvs:
    return("bvs");
    break;
  case i_fstox:
    return("fstox");
    break;
  case i_fdtox:
    return("fdtox");
    break;
  case i_fqtox:
    return("fqtox");
    break;
  case i_fstoi:
    return("fstoi");
    break;
  case i_fdtoi:
    return("fdtoi");
    break;
  case i_fqtoi:
    return("fqtoi");
    break;
  case i_fstod:
    return("fstod");
    break;
  case i_fstoq:
    return("fstoq");
    break;
  case i_fdtos:
    return("fdtos");
    break;
  case i_fdtoq:
    return("fdtoq");
    break;
  case i_fqtos:
    return("fqtos");
    break;
  case i_fqtod:
    return("fqtod");
    break;
  case i_fxtos:
    return("fxtos");
    break;
  case i_fxtod:
    return("fxtod");
    break;
  case i_fxtoq:
    return("fxtoq");
    break;
  case i_fitos:
    return("fitos");
    break;
  case i_fitod:
    return("fitod");
    break;
  case i_fitoq:
    return("fitoq");
    break;
  case i_fmovfsn:
    return("fmovfsn");
    break;
  case i_fmovfsne:
    return("fmovfsne");
    break;
  case i_fmovfslg:
    return("fmovfslg");
    break;
  case i_fmovfsul:
    return("fmovfsul");
    break;
  case i_fmovfsl:
    return("fmovfsl");
    break;
  case i_fmovfsug:
    return("fmovfsug");
    break;
  case i_fmovfsg:
    return("fmovfsg");
    break;
  case i_fmovfsu:
    return("fmovfsu");
    break;
  case i_fmovfsa:
    return("fmovfsa");
    break;
  case i_fmovfse:
    return("fmovfse");
    break;
  case i_fmovfsue:
    return("fmovfsue");
    break;
  case i_fmovfsge:
    return("fmovfsge");
    break;
  case i_fmovfsuge:
    return("fmovfsuge");
    break;
  case i_fmovfsle:
    return("fmovfsle");
    break;
  case i_fmovfsule:
    return("fmovfsule");
    break;
  case i_fmovfso:
    return("fmovfso");
    break;
  case i_fmovfdn:
    return("fmovfdn");
    break;
  case i_fmovfdne:
    return("fmovfdne");
    break;
  case i_fmovfdlg:
    return("fmovfdlg");
    break;
  case i_fmovfdul:
    return("fmovfdul");
    break;
  case i_fmovfdl:
    return("fmovfdl");
    break;
  case i_fmovfdug:
    return("fmovfdug");
    break;
  case i_fmovfdg:
    return("fmovfdg");
    break;
  case i_fmovfdu:
    return("fmovfdu");
    break;
  case i_fmovfda:
    return("fmovfda");
    break;
  case i_fmovfde:
    return("fmovfde");
    break;
  case i_fmovfdue:
    return("fmovfdue");
    break;
  case i_fmovfdge:
    return("fmovfdge");
    break;
  case i_fmovfduge:
    return("fmovfduge");
    break;
  case i_fmovfdle:
    return("fmovfdle");
    break;
  case i_fmovfdule:
    return("fmovfdule");
    break;
  case i_fmovfdo:
    return("fmovfdo");
    break;
  case i_fmovfqn:
    return("fmovfqn");
    break;
  case i_fmovfqne:
    return("fmovfqne");
    break;
  case i_fmovfqlg:
    return("fmovfqlg");
    break;
  case i_fmovfqul:
    return("fmovfqul");
    break;
  case i_fmovfql:
    return("fmovfql");
    break;
  case i_fmovfqug:
    return("fmovfqug");
    break;
  case i_fmovfqg:
    return("fmovfqg");
    break;
  case i_fmovfqu:
    return("fmovfqu");
    break;
  case i_fmovfqa:
    return("fmovfqa");
    break;
  case i_fmovfqe:
    return("fmovfqe");
    break;
  case i_fmovfque:
    return("fmovfque");
    break;
  case i_fmovfqge:
    return("fmovfqge");
    break;
  case i_fmovfquge:
    return("fmovfquge");
    break;
  case i_fmovfqle:
    return("fmovfqle");
    break;
  case i_fmovfqule:
    return("fmovfqule");
    break;
  case i_fmovfqo:
    return("fmovfqo");
    break;
  case i_fmovsn:
    return("fmovsn");
    break;
  case i_fmovse:
    return("fmovse");
    break;
  case i_fmovsle:
    return("fmovsle");
    break;
  case i_fmovsl:
    return("fmovsl");
    break;
  case i_fmovsleu:
    return("fmovsleu");
    break;
  case i_fmovscs:
    return("fmovscs");
    break;
  case i_fmovsneg:
    return("fmovsneg");
    break;
  case i_fmovsvs:
    return("fmovsvs");
    break;
  case i_fmovsa:
    return("fmovsa");
    break;
  case i_fmovsne:
    return("fmovsne");
    break;
  case i_fmovsg:
    return("fmovsg");
    break;
  case i_fmovsge:
    return("fmovsge");
    break;
  case i_fmovsgu:
    return("fmovsgu");
    break;
  case i_fmovscc:
    return("fmovscc");
    break;
  case i_fmovspos:
    return("fmovspos");
    break;
  case i_fmovsvc:
    return("fmovsvc");
    break;
  case i_fmovdn:
    return("fmovdn");
    break;
  case i_fmovde:
    return("fmovde");
    break;
  case i_fmovdle:
    return("fmovdle");
    break;
  case i_fmovdl:
    return("fmovdl");
    break;
  case i_fmovdleu:
    return("fmovdleu");
    break;
  case i_fmovdcs:
    return("fmovdcs");
    break;
  case i_fmovdneg:
    return("fmovdneg");
    break;
  case i_fmovdvs:
    return("fmovdvs");
    break;
  case i_fmovda:
    return("fmovda");
    break;
  case i_fmovdne:
    return("fmovdne");
    break;
  case i_fmovdg:
    return("fmovdg");
    break;
  case i_fmovdge:
    return("fmovdge");
    break;
  case i_fmovdgu:
    return("fmovdgu");
    break;
  case i_fmovdcc:
    return("fmovdcc");
    break;
  case i_fmovdpos:
    return("fmovdpos");
    break;
  case i_fmovdvc:
    return("fmovdvc");
    break;
  case i_fmovqn:
    return("fmovqn");
    break;
  case i_fmovqe:
    return("fmovqe");
    break;
  case i_fmovqle:
    return("fmovqle");
    break;
  case i_fmovql:
    return("fmovql");
    break;
  case i_fmovqleu:
    return("fmovqleu");
    break;
  case i_fmovqcs:
    return("fmovqcs");
    break;
  case i_fmovqneg:
    return("fmovqneg");
    break;
  case i_fmovqvs:
    return("fmovqvs");
    break;
  case i_fmovqa:
    return("fmovqa");
    break;
  case i_fmovqne:
    return("fmovqne");
    break;
  case i_fmovqg:
    return("fmovqg");
    break;
  case i_fmovqge:
    return("fmovqge");
    break;
  case i_fmovqgu:
    return("fmovqgu");
    break;
  case i_fmovqcc:
    return("fmovqcc");
    break;
  case i_fmovqpos:
    return("fmovqpos");
    break;
  case i_fmovqvc:
    return("fmovqvc");
    break;
  case i_fmovrsz:
    return("fmovrsz");
    break;
  case i_fmovrslez:
    return("fmovrslez");
    break;
  case i_fmovrslz:
    return("fmovrslz");
    break;
  case i_fmovrsnz:
    return("fmovrsnz");
    break;
  case i_fmovrsgz:
    return("fmovrsgz");
    break;
  case i_fmovrsgez:
    return("fmovrsgez");
    break;
  case i_fmovrdz:
    return("fmovrdz");
    break;
  case i_fmovrdlez:
    return("fmovrdlez");
    break;
  case i_fmovrdlz:
    return("fmovrdlz");
    break;
  case i_fmovrdnz:
    return("fmovrdnz");
    break;
  case i_fmovrdgz:
    return("fmovrdgz");
    break;
  case i_fmovrdgez:
    return("fmovrdgez");
    break;
  case i_fmovrqz:
    return("fmovrqz");
    break;
  case i_fmovrqlez:
    return("fmovrqlez");
    break;
  case i_fmovrqlz:
    return("fmovrqlz");
    break;
  case i_fmovrqnz:
    return("fmovrqnz");
    break;
  case i_fmovrqgz:
    return("fmovrqgz");
    break;
  case i_fmovrqgez:
    return("fmovrqgez");
    break;
  case i_mova:
    return("mova");
    break;
  case i_movfa:
    return("movfa");
    break;
  case i_movn:
    return("movn");
    break;
  case i_movfn:
    return("movfn");
    break;
  case i_movne:
    return("movne");
    break;
  case i_move:
    return("move");
    break;
  case i_movg:
    return("movg");
    break;
  case i_movle:
    return("movle");
    break;
  case i_movge:
    return("movge");
    break;
  case i_movl:
    return("movl");
    break;
  case i_movgu:
    return("movgu");
    break;
  case i_movleu:
    return("movleu");
    break;
  case i_movcc:
    return("movcc");
    break;
  case i_movcs:
    return("movcs");
    break;
  case i_movpos:
    return("movpos");
    break;
  case i_movneg:
    return("movneg");
    break;
  case i_movvc:
    return("movvc");
    break;
  case i_movvs:
    return("movvs");
    break;
  case i_movfu:
    return("movfu");
    break;
  case i_movfg:
    return("movfg");
    break;
  case i_movfug:
    return("movfug");
    break;
  case i_movfl:
    return("movfl");
    break;
  case i_movful:
    return("movful");
    break;
  case i_movflg:
    return("movflg");
    break;
  case i_movfne:
    return("movfne");
    break;
  case i_movfe:
    return("movfe");
    break;
  case i_movfue:
    return("movfue");
    break;
  case i_movfge:
    return("movfge");
    break;
  case i_movfuge:
    return("movfuge");
    break;
  case i_movfle:
    return("movfle");
    break;
  case i_movfule:
    return("movfule");
    break;
  case i_movfo:
    return("movfo");
    break;
  case i_movrz:
    return("movrz");
    break;
  case i_movrlez:
    return("movrlez");
    break;
  case i_movrlz:
    return("movrlz");
    break;
  case i_movrnz:
    return("movrnz");
    break;
  case i_movrgz:
    return("movrgz");
    break;
  case i_movrgez:
    return("movrgez");
    break;
  case i_ta:
    return("ta");
    break;
  case i_tn:
    return("tn");
    break;
  case i_tne:
    return("tne");
    break;
  case i_te:
    return("te");
    break;
  case i_tg:
    return("tg");
    break;
  case i_tle:
    return("tle");
    break;
  case i_tge:
    return("tge");
    break;
  case i_tl:
    return("tl");
    break;
  case i_tgu:
    return("tgu");
    break;
  case i_tleu:
    return("tleu");
    break;
  case i_tcc:
    return("tcc");
    break;
  case i_tcs:
    return("tcs");
    break;
  case i_tpos:
    return("tpos");
    break;
  case i_tneg:
    return("tneg");
    break;
  case i_tvc:
    return("tvc");
    break;
  case i_tvs:
    return("tvs");
    break;
  case i_sub:
    return("sub");
    break;
  case i_subcc:
    return("subcc");
    break;
  case i_subc:
    return("subc");
    break;
  case i_subccc:
    return("subccc");
    break;
  case i_or:
    return("or");
    break;
  case i_orcc:
    return("orcc");
    break;
  case i_orn:
    return("orn");
    break;
  case i_orncc:
    return("orncc");
    break;
  case i_xor:
    return("xor");
    break;
  case i_xorcc:
    return("xorcc");
    break;
  case i_xnor:
    return("xnor");
    break;
  case i_xnorcc:
    return("xnorcc");
    break;
  case i_mulx:
    return("mulx");
    break;
  case i_sdivx:
    return("sdivx");
    break;
  case i_udivx:
    return("udivx");
    break;
  case i_sll:
    return("sll");
    break;
  case i_srl:
    return("srl");
    break;
  case i_sra:
    return("sra");
    break;
  case i_sllx:
    return("sllx");
    break;
  case i_srlx:
    return("srlx");
    break;
  case i_srax:
    return("srax");
    break;
  case i_taddcc:
    return("taddcc");
    break;
  case i_taddcctv:
    return("taddcctv");
    break;
  case i_tsubcc:
    return("tsubcc");
    break;
  case i_tsubcctv:
    return("tsubcctv");
    break;
  case i_udiv:
    return("udiv");
    break;
  case i_sdiv:
    return("sdiv");
    break;
  case i_umul:
    return("umul");
    break;
  case i_smul:
    return("smul");
    break;
  case i_udivcc:
    return("udivcc");
    break;
  case i_sdivcc:
    return("sdivcc");
    break;
  case i_umulcc:
    return("umulcc");
    break;
  case i_smulcc:
    return("smulcc");
    break;
  case i_mulscc:
    return("mulscc");
    break;
  case i_popc:
    return("popc");
    break;
  case i_flush:
    return("flush");
    break;
  case i_flushw:
    return("flushw");
    break;
  case i_rd:
    return("rd");
    break;
  case i_rdcc:
    return("rdcc");
    break;
  case i_wr:
    return("wr");
    break;
  case i_wrcc:
    return("wrcc");
    break;
  case i_save:
    return("save");
    break;
  case i_restore:
    return("restore");
    break;
  case i_saved:
    return("saved");
    break;
  case i_restored:
    return("restored");
    break;
  case i_sethi:
    return("sethi");
    break;
  case i_ldf:
    return("ldf");
    return("ld"); // synonym to ldf
    break;
  case i_lddf:
    return("lddf");
    return("ldd"); // synonym to lddf
    break;
  case i_ldqf:
    return("ldqf");
    return("ldq"); // synonym to ldqf
    break;
  case i_ldfa:
    return("ldfa");
    return("lda"); // synonym to lda
    break;
  case i_lddfa:
    return("lddfa");
    return("ldda"); // synonym to ldda
    break;
  case i_ldblk:
    return("ldblk");
    break;
  case i_ldqfa:
    return("ldqfa");
    return("ldqa"); // synonym to ldqfa
    break;
  case i_ldsb:
    return("ldsb");
    break;
  case i_ldsh:
    return("ldsh");
    break;
  case i_ldsw:
    return("ldsw");
    break;
  case i_ldub:
    return("ldub");
    break;
  case i_lduh:
    return("lduh");
    break;
  case i_lduw:
    return("lduw"); // synonym to lduw
    return("ld"); // synonym to lduw
    break;
  case i_ldx:
    return("ldx");
    break;
  case i_ldd:
    return("ldd");
    break;
  case i_ldsba:
    return("ldsba");
    break;
  case i_ldsha:
    return("ldsha");
    break;
  case i_ldswa:
    return("ldswa");
    break;
  case i_lduba:
    return("lduba");
    break;
  case i_lduha:
    return("lduha");
    break;
  case i_lduwa:
    return("lduwa"); // synonym to lduwa
    return("lda"); // synonym to lduwa
    break;
  case i_ldxa:
    return("ldxa");
    break;
  case i_ldda:
    return("ldda");
    break;
  case i_ldqa:
    return("ldqa");
    break;
  case i_stf:
    return("st");
    break;
  case i_stdf:
    return("std");
    break;
  case i_stqf:
    return("stq");
    break;
  case i_stb:
    return("stb");
    break;
  case i_sth:
    return("sth");
    break;
  case i_stw:
    return("st"); // synonym to stw
    break;
  case i_stx:
    return("stx");
    break;
  case i_std:
    return("std");
    break;
  case i_stfa:
    return("sta"); // cuts down from stfa
    break;
  case i_stdfa:
    return("stdfa");
    break;
  case i_stblk:
    return("stblk");
    break;
  case i_stqfa:
    return("stqa");
    break;
  case i_stba:
    return("stba");
    break;
  case i_stha:
    return("stha");
    break;
  case i_stwa:
    return("sta"); // synonym to sta
    break;
  case i_stxa:
    return("stxa");
    break;
  case i_stda:
    return("stda");
    break;
  case i_ldstub:
    return("ldstub");
    break;
  case i_ldstuba:
    return("ldstuba");
    break;
  case i_prefetch:
    return("prefetch");
    break;
  case i_prefetcha:
    return("prefetcha");
    break;
  case i_swap:
    return("swap");
    break;
  case i_swapa:
    return("swapa");
    break;
  case i_ldfsr:
    return("ld");
    break;
  case i_ldxfsr:
    return("ldx");
    break;
  case i_stfsr:
    return("st");
    break;
  case i_stxfsr:
    return("stx");
    break;
  case i__trap:
    return("_trap");
    break;
  case i_impdep1:
    return("impdep1");
    break;
  case i_impdep2:
    return("impdep2");
    break;
  case i_membar:
    return("membar");
    break;
  case i_stbar:
    return("stbar");
    break;
  case i_cmp:
    return("cmp");
    break;
  case i_jmp:
    return("jmp");
    break;
  case i_mov:
    return("mov");
    break;
  case i_nop:
    return("nop");
    break;
  case i_not:
    return("not");
    break;
  case i_rdpr:
    return("rdpr");
    break;
  case i_wrpr:
    return("wrpr");
    break;
  case i_faligndata:
    return("faligndata");
    break;
  case i_alignaddr:
    return("alignaddr");
    break;
  case i_alignaddrl:
    return("alignaddrl");
    break;
  case i_fzero:
    return("fzero");
    break;
  case i_fzeros:
    return("fzeros");
    break;
  case i_fsrc1:
    return("fsrc1");
    break;
  case i_retry:
    return("retry");
    break;
  case i_fcmple16:
    return("fcmple16");
    break;
  case i_fcmpne16:
    return("fcmpne16");
    break;
  case i_fcmple32:
    return("fcmple32");
    break;
  case i_fcmpne32:
    return("fcmpne32");
    break;
  case i_fcmpgt16:
    return("fcmpgt16");
    break;
  case i_fcmpeq16:
    return("fcmpeq16");
    break;
  case i_fcmpgt32:
    return("fcmpgt32");
    break;
  case i_fcmpeq32:
    return("fcmpeq32");
    break;
  case i_bshuffle:
    return("bshuffle");
    break;
  case i_bmask:
    return("bmask");
    break;
  case i_mop:
    return("mop");
    break;
  case i_ill:
    return("ill");
    break;
  default:
    return("**Unknown**");
    break;
  }
}
