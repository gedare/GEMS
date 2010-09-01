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
#ifndef _DX_H_
#define _DX_H_

/* -- Defines */
#define DX_RETURNT void
#define DX_PLIST void

/* -- Global Functions */
DX_RETURNT dx_add( DX_PLIST );
DX_RETURNT dx_addcc( DX_PLIST );
DX_RETURNT dx_addc( DX_PLIST );
DX_RETURNT dx_addccc( DX_PLIST );
DX_RETURNT dx_and( DX_PLIST );
DX_RETURNT dx_andcc( DX_PLIST );
DX_RETURNT dx_andn( DX_PLIST );
DX_RETURNT dx_andncc( DX_PLIST );
DX_RETURNT dx_fba( DX_PLIST );
DX_RETURNT dx_ba( DX_PLIST );
DX_RETURNT dx_fbpa( DX_PLIST );
DX_RETURNT dx_bpa( DX_PLIST );
DX_RETURNT dx_fbn( DX_PLIST );
DX_RETURNT dx_fbpn( DX_PLIST );
DX_RETURNT dx_bn( DX_PLIST );
DX_RETURNT dx_bpn( DX_PLIST );
DX_RETURNT dx_bpne( DX_PLIST );
DX_RETURNT dx_bpe( DX_PLIST );
DX_RETURNT dx_bpg( DX_PLIST );
DX_RETURNT dx_bple( DX_PLIST );
DX_RETURNT dx_bpge( DX_PLIST );
DX_RETURNT dx_bpl( DX_PLIST );
DX_RETURNT dx_bpgu( DX_PLIST );
DX_RETURNT dx_bpleu( DX_PLIST );
DX_RETURNT dx_bpcc( DX_PLIST );
DX_RETURNT dx_bpcs( DX_PLIST );
DX_RETURNT dx_bppos( DX_PLIST );
DX_RETURNT dx_bpneg( DX_PLIST );
DX_RETURNT dx_bpvc( DX_PLIST );
DX_RETURNT dx_bpvs( DX_PLIST );
DX_RETURNT dx_call( DX_PLIST );
DX_RETURNT dx_casa( DX_PLIST );
DX_RETURNT dx_casxa( DX_PLIST );
DX_RETURNT dx_doneretry( DX_PLIST );
DX_RETURNT dx_jmpl( DX_PLIST );
DX_RETURNT dx_fabss( DX_PLIST );
DX_RETURNT dx_fabsd( DX_PLIST );
DX_RETURNT dx_fabsq( DX_PLIST );
DX_RETURNT dx_fadds( DX_PLIST );
DX_RETURNT dx_faddd( DX_PLIST );
DX_RETURNT dx_faddq( DX_PLIST );
DX_RETURNT dx_fsubs( DX_PLIST );
DX_RETURNT dx_fsubd( DX_PLIST );
DX_RETURNT dx_fsubq( DX_PLIST );
DX_RETURNT dx_fcmps( DX_PLIST );
DX_RETURNT dx_fcmpd( DX_PLIST );
DX_RETURNT dx_fcmpq( DX_PLIST );
DX_RETURNT dx_fcmpes( DX_PLIST );
DX_RETURNT dx_fcmped( DX_PLIST );
DX_RETURNT dx_fcmpeq( DX_PLIST );
DX_RETURNT dx_fmovs( DX_PLIST );
DX_RETURNT dx_fmovd( DX_PLIST );
DX_RETURNT dx_fmovq( DX_PLIST );
DX_RETURNT dx_fnegs( DX_PLIST );
DX_RETURNT dx_fnegd( DX_PLIST );
DX_RETURNT dx_fnegq( DX_PLIST );
DX_RETURNT dx_fmuls( DX_PLIST );
DX_RETURNT dx_fmuld( DX_PLIST );
DX_RETURNT dx_fmulq( DX_PLIST );
DX_RETURNT dx_fdivs( DX_PLIST );
DX_RETURNT dx_fdivd( DX_PLIST );
DX_RETURNT dx_fdivq( DX_PLIST );
DX_RETURNT dx_fsmuld( DX_PLIST );
DX_RETURNT dx_fdmulq( DX_PLIST );
DX_RETURNT dx_fsqrts( DX_PLIST );
DX_RETURNT dx_fsqrtd( DX_PLIST );
DX_RETURNT dx_fsqrtq( DX_PLIST );
DX_RETURNT dx_retrn( DX_PLIST );
DX_RETURNT dx_brz( DX_PLIST );
DX_RETURNT dx_brlez( DX_PLIST );
DX_RETURNT dx_brlz( DX_PLIST );
DX_RETURNT dx_brnz( DX_PLIST );
DX_RETURNT dx_brgz( DX_PLIST );
DX_RETURNT dx_brgez( DX_PLIST );
DX_RETURNT dx_fbu( DX_PLIST );
DX_RETURNT dx_fbg( DX_PLIST );
DX_RETURNT dx_fbug( DX_PLIST );
DX_RETURNT dx_fbl( DX_PLIST );
DX_RETURNT dx_fbul( DX_PLIST );
DX_RETURNT dx_fblg( DX_PLIST );
DX_RETURNT dx_fbne( DX_PLIST );
DX_RETURNT dx_fbe( DX_PLIST );
DX_RETURNT dx_fbue( DX_PLIST );
DX_RETURNT dx_fbge( DX_PLIST );
DX_RETURNT dx_fbuge( DX_PLIST );
DX_RETURNT dx_fble( DX_PLIST );
DX_RETURNT dx_fbule( DX_PLIST );
DX_RETURNT dx_fbo( DX_PLIST );
DX_RETURNT dx_fbpu( DX_PLIST );
DX_RETURNT dx_fbpg( DX_PLIST );
DX_RETURNT dx_fbpug( DX_PLIST );
DX_RETURNT dx_fbpl( DX_PLIST );
DX_RETURNT dx_fbpul( DX_PLIST );
DX_RETURNT dx_fbplg( DX_PLIST );
DX_RETURNT dx_fbpne( DX_PLIST );
DX_RETURNT dx_fbpe( DX_PLIST );
DX_RETURNT dx_fbpue( DX_PLIST );
DX_RETURNT dx_fbpge( DX_PLIST );
DX_RETURNT dx_fbpuge( DX_PLIST );
DX_RETURNT dx_fbple( DX_PLIST );
DX_RETURNT dx_fbpule( DX_PLIST );
DX_RETURNT dx_fbpo( DX_PLIST );
DX_RETURNT dx_bne( DX_PLIST );
DX_RETURNT dx_be( DX_PLIST );
DX_RETURNT dx_bg( DX_PLIST );
DX_RETURNT dx_ble( DX_PLIST );
DX_RETURNT dx_bge( DX_PLIST );
DX_RETURNT dx_bl( DX_PLIST );
DX_RETURNT dx_bgu( DX_PLIST );
DX_RETURNT dx_bleu( DX_PLIST );
DX_RETURNT dx_bcc( DX_PLIST );
DX_RETURNT dx_bcs( DX_PLIST );
DX_RETURNT dx_bpos( DX_PLIST );
DX_RETURNT dx_bneg( DX_PLIST );
DX_RETURNT dx_bvc( DX_PLIST );
DX_RETURNT dx_bvs( DX_PLIST );
DX_RETURNT dx_fstox( DX_PLIST );
DX_RETURNT dx_fdtox( DX_PLIST );
DX_RETURNT dx_fqtox( DX_PLIST );
DX_RETURNT dx_fstoi( DX_PLIST );
DX_RETURNT dx_fdtoi( DX_PLIST );
DX_RETURNT dx_fqtoi( DX_PLIST );
DX_RETURNT dx_fstod( DX_PLIST );
DX_RETURNT dx_fstoq( DX_PLIST );
DX_RETURNT dx_fdtos( DX_PLIST );
DX_RETURNT dx_fdtoq( DX_PLIST );
DX_RETURNT dx_fqtos( DX_PLIST );
DX_RETURNT dx_fqtod( DX_PLIST );
DX_RETURNT dx_fxtos( DX_PLIST );
DX_RETURNT dx_fxtod( DX_PLIST );
DX_RETURNT dx_fxtoq( DX_PLIST );
DX_RETURNT dx_fitos( DX_PLIST );
DX_RETURNT dx_fitod( DX_PLIST );
DX_RETURNT dx_fitoq( DX_PLIST );
DX_RETURNT dx_fmovfsn( DX_PLIST );
DX_RETURNT dx_fmovfsne( DX_PLIST );
DX_RETURNT dx_fmovfslg( DX_PLIST );
DX_RETURNT dx_fmovfsul( DX_PLIST );
DX_RETURNT dx_fmovfsl( DX_PLIST );
DX_RETURNT dx_fmovfsug( DX_PLIST );
DX_RETURNT dx_fmovfsg( DX_PLIST );
DX_RETURNT dx_fmovfsu( DX_PLIST );
DX_RETURNT dx_fmovfsa( DX_PLIST );
DX_RETURNT dx_fmovfse( DX_PLIST );
DX_RETURNT dx_fmovfsue( DX_PLIST );
DX_RETURNT dx_fmovfsge( DX_PLIST );
DX_RETURNT dx_fmovfsuge( DX_PLIST );
DX_RETURNT dx_fmovfsle( DX_PLIST );
DX_RETURNT dx_fmovfsule( DX_PLIST );
DX_RETURNT dx_fmovfso( DX_PLIST );
DX_RETURNT dx_fmovfdn( DX_PLIST );
DX_RETURNT dx_fmovfdne( DX_PLIST );
DX_RETURNT dx_fmovfdlg( DX_PLIST );
DX_RETURNT dx_fmovfdul( DX_PLIST );
DX_RETURNT dx_fmovfdl( DX_PLIST );
DX_RETURNT dx_fmovfdug( DX_PLIST );
DX_RETURNT dx_fmovfdg( DX_PLIST );
DX_RETURNT dx_fmovfdu( DX_PLIST );
DX_RETURNT dx_fmovfda( DX_PLIST );
DX_RETURNT dx_fmovfde( DX_PLIST );
DX_RETURNT dx_fmovfdue( DX_PLIST );
DX_RETURNT dx_fmovfdge( DX_PLIST );
DX_RETURNT dx_fmovfduge( DX_PLIST );
DX_RETURNT dx_fmovfdle( DX_PLIST );
DX_RETURNT dx_fmovfdule( DX_PLIST );
DX_RETURNT dx_fmovfdo( DX_PLIST );
DX_RETURNT dx_fmovfqn( DX_PLIST );
DX_RETURNT dx_fmovfqne( DX_PLIST );
DX_RETURNT dx_fmovfqlg( DX_PLIST );
DX_RETURNT dx_fmovfqul( DX_PLIST );
DX_RETURNT dx_fmovfql( DX_PLIST );
DX_RETURNT dx_fmovfqug( DX_PLIST );
DX_RETURNT dx_fmovfqg( DX_PLIST );
DX_RETURNT dx_fmovfqu( DX_PLIST );
DX_RETURNT dx_fmovfqa( DX_PLIST );
DX_RETURNT dx_fmovfqe( DX_PLIST );
DX_RETURNT dx_fmovfque( DX_PLIST );
DX_RETURNT dx_fmovfqge( DX_PLIST );
DX_RETURNT dx_fmovfquge( DX_PLIST );
DX_RETURNT dx_fmovfqle( DX_PLIST );
DX_RETURNT dx_fmovfqule( DX_PLIST );
DX_RETURNT dx_fmovfqo( DX_PLIST );
DX_RETURNT dx_fmovsn( DX_PLIST );
DX_RETURNT dx_fmovse( DX_PLIST );
DX_RETURNT dx_fmovsle( DX_PLIST );
DX_RETURNT dx_fmovsl( DX_PLIST );
DX_RETURNT dx_fmovsleu( DX_PLIST );
DX_RETURNT dx_fmovscs( DX_PLIST );
DX_RETURNT dx_fmovsneg( DX_PLIST );
DX_RETURNT dx_fmovsvs( DX_PLIST );
DX_RETURNT dx_fmovsa( DX_PLIST );
DX_RETURNT dx_fmovsne( DX_PLIST );
DX_RETURNT dx_fmovsg( DX_PLIST );
DX_RETURNT dx_fmovsge( DX_PLIST );
DX_RETURNT dx_fmovsgu( DX_PLIST );
DX_RETURNT dx_fmovscc( DX_PLIST );
DX_RETURNT dx_fmovspos( DX_PLIST );
DX_RETURNT dx_fmovsvc( DX_PLIST );
DX_RETURNT dx_fmovdn( DX_PLIST );
DX_RETURNT dx_fmovde( DX_PLIST );
DX_RETURNT dx_fmovdle( DX_PLIST );
DX_RETURNT dx_fmovdl( DX_PLIST );
DX_RETURNT dx_fmovdleu( DX_PLIST );
DX_RETURNT dx_fmovdcs( DX_PLIST );
DX_RETURNT dx_fmovdneg( DX_PLIST );
DX_RETURNT dx_fmovdvs( DX_PLIST );
DX_RETURNT dx_fmovda( DX_PLIST );
DX_RETURNT dx_fmovdne( DX_PLIST );
DX_RETURNT dx_fmovdg( DX_PLIST );
DX_RETURNT dx_fmovdge( DX_PLIST );
DX_RETURNT dx_fmovdgu( DX_PLIST );
DX_RETURNT dx_fmovdcc( DX_PLIST );
DX_RETURNT dx_fmovdpos( DX_PLIST );
DX_RETURNT dx_fmovdvc( DX_PLIST );
DX_RETURNT dx_fmovqn( DX_PLIST );
DX_RETURNT dx_fmovqe( DX_PLIST );
DX_RETURNT dx_fmovqle( DX_PLIST );
DX_RETURNT dx_fmovql( DX_PLIST );
DX_RETURNT dx_fmovqleu( DX_PLIST );
DX_RETURNT dx_fmovqcs( DX_PLIST );
DX_RETURNT dx_fmovqneg( DX_PLIST );
DX_RETURNT dx_fmovqvs( DX_PLIST );
DX_RETURNT dx_fmovqa( DX_PLIST );
DX_RETURNT dx_fmovqne( DX_PLIST );
DX_RETURNT dx_fmovqg( DX_PLIST );
DX_RETURNT dx_fmovqge( DX_PLIST );
DX_RETURNT dx_fmovqgu( DX_PLIST );
DX_RETURNT dx_fmovqcc( DX_PLIST );
DX_RETURNT dx_fmovqpos( DX_PLIST );
DX_RETURNT dx_fmovqvc( DX_PLIST );
DX_RETURNT dx_fmovrsz( DX_PLIST );
DX_RETURNT dx_fmovrslez( DX_PLIST );
DX_RETURNT dx_fmovrslz( DX_PLIST );
DX_RETURNT dx_fmovrsnz( DX_PLIST );
DX_RETURNT dx_fmovrsgz( DX_PLIST );
DX_RETURNT dx_fmovrsgez( DX_PLIST );
DX_RETURNT dx_fmovrdz( DX_PLIST );
DX_RETURNT dx_fmovrdlez( DX_PLIST );
DX_RETURNT dx_fmovrdlz( DX_PLIST );
DX_RETURNT dx_fmovrdnz( DX_PLIST );
DX_RETURNT dx_fmovrdgz( DX_PLIST );
DX_RETURNT dx_fmovrdgez( DX_PLIST );
DX_RETURNT dx_fmovrqz( DX_PLIST );
DX_RETURNT dx_fmovrqlez( DX_PLIST );
DX_RETURNT dx_fmovrqlz( DX_PLIST );
DX_RETURNT dx_fmovrqnz( DX_PLIST );
DX_RETURNT dx_fmovrqgz( DX_PLIST );
DX_RETURNT dx_fmovrqgez( DX_PLIST );
DX_RETURNT dx_mova( DX_PLIST );
DX_RETURNT dx_movfa( DX_PLIST );
DX_RETURNT dx_movn( DX_PLIST );
DX_RETURNT dx_movfn( DX_PLIST );
DX_RETURNT dx_movne( DX_PLIST );
DX_RETURNT dx_move( DX_PLIST );
DX_RETURNT dx_movg( DX_PLIST );
DX_RETURNT dx_movle( DX_PLIST );
DX_RETURNT dx_movge( DX_PLIST );
DX_RETURNT dx_movl( DX_PLIST );
DX_RETURNT dx_movgu( DX_PLIST );
DX_RETURNT dx_movleu( DX_PLIST );
DX_RETURNT dx_movcc( DX_PLIST );
DX_RETURNT dx_movcs( DX_PLIST );
DX_RETURNT dx_movpos( DX_PLIST );
DX_RETURNT dx_movneg( DX_PLIST );
DX_RETURNT dx_movvc( DX_PLIST );
DX_RETURNT dx_movvs( DX_PLIST );
DX_RETURNT dx_movfu( DX_PLIST );
DX_RETURNT dx_movfg( DX_PLIST );
DX_RETURNT dx_movfug( DX_PLIST );
DX_RETURNT dx_movfl( DX_PLIST );
DX_RETURNT dx_movful( DX_PLIST );
DX_RETURNT dx_movflg( DX_PLIST );
DX_RETURNT dx_movfne( DX_PLIST );
DX_RETURNT dx_movfe( DX_PLIST );
DX_RETURNT dx_movfue( DX_PLIST );
DX_RETURNT dx_movfge( DX_PLIST );
DX_RETURNT dx_movfuge( DX_PLIST );
DX_RETURNT dx_movfle( DX_PLIST );
DX_RETURNT dx_movfule( DX_PLIST );
DX_RETURNT dx_movfo( DX_PLIST );
DX_RETURNT dx_movrz( DX_PLIST );
DX_RETURNT dx_movrlez( DX_PLIST );
DX_RETURNT dx_movrlz( DX_PLIST );
DX_RETURNT dx_movrnz( DX_PLIST );
DX_RETURNT dx_movrgz( DX_PLIST );
DX_RETURNT dx_movrgez( DX_PLIST );
DX_RETURNT dx_ta( DX_PLIST );
DX_RETURNT dx_tn( DX_PLIST );
DX_RETURNT dx_tne( DX_PLIST );
DX_RETURNT dx_te( DX_PLIST );
DX_RETURNT dx_tg( DX_PLIST );
DX_RETURNT dx_tle( DX_PLIST );
DX_RETURNT dx_tge( DX_PLIST );
DX_RETURNT dx_tl( DX_PLIST );
DX_RETURNT dx_tgu( DX_PLIST );
DX_RETURNT dx_tleu( DX_PLIST );
DX_RETURNT dx_tcc( DX_PLIST );
DX_RETURNT dx_tcs( DX_PLIST );
DX_RETURNT dx_tpos( DX_PLIST );
DX_RETURNT dx_tneg( DX_PLIST );
DX_RETURNT dx_tvc( DX_PLIST );
DX_RETURNT dx_tvs( DX_PLIST );
DX_RETURNT dx_sub( DX_PLIST );
DX_RETURNT dx_subcc( DX_PLIST );
DX_RETURNT dx_subc( DX_PLIST );
DX_RETURNT dx_subccc( DX_PLIST );
DX_RETURNT dx_or( DX_PLIST );
DX_RETURNT dx_orcc( DX_PLIST );
DX_RETURNT dx_orn( DX_PLIST );
DX_RETURNT dx_orncc( DX_PLIST );
DX_RETURNT dx_xor( DX_PLIST );
DX_RETURNT dx_xorcc( DX_PLIST );
DX_RETURNT dx_xnor( DX_PLIST );
DX_RETURNT dx_xnorcc( DX_PLIST );
DX_RETURNT dx_mulx( DX_PLIST );
DX_RETURNT dx_sdivx( DX_PLIST );
DX_RETURNT dx_udivx( DX_PLIST );
DX_RETURNT dx_sll( DX_PLIST );
DX_RETURNT dx_srl( DX_PLIST );
DX_RETURNT dx_sra( DX_PLIST );
DX_RETURNT dx_sllx( DX_PLIST );
DX_RETURNT dx_srlx( DX_PLIST );
DX_RETURNT dx_srax( DX_PLIST );
DX_RETURNT dx_taddcc( DX_PLIST );
DX_RETURNT dx_taddcctv( DX_PLIST );
DX_RETURNT dx_tsubcc( DX_PLIST );
DX_RETURNT dx_tsubcctv( DX_PLIST );
DX_RETURNT dx_udiv( DX_PLIST );
DX_RETURNT dx_sdiv( DX_PLIST );
DX_RETURNT dx_umul( DX_PLIST );
DX_RETURNT dx_smul( DX_PLIST );
DX_RETURNT dx_udivcc( DX_PLIST );
DX_RETURNT dx_sdivcc( DX_PLIST );
DX_RETURNT dx_umulcc( DX_PLIST );
DX_RETURNT dx_smulcc( DX_PLIST );
DX_RETURNT dx_mulscc( DX_PLIST );
DX_RETURNT dx_popc( DX_PLIST );
DX_RETURNT dx_flush( DX_PLIST );
DX_RETURNT dx_flushw( DX_PLIST );
DX_RETURNT dx_rd( DX_PLIST );
DX_RETURNT dx_rdcc( DX_PLIST );
DX_RETURNT dx_wr( DX_PLIST );
DX_RETURNT dx_wrcc( DX_PLIST );
DX_RETURNT dx_save( DX_PLIST );
DX_RETURNT dx_restore( DX_PLIST );
DX_RETURNT dx_saved( DX_PLIST );
DX_RETURNT dx_restored( DX_PLIST );
DX_RETURNT dx_sethi( DX_PLIST );
DX_RETURNT dx_magic( DX_PLIST );
DX_RETURNT dx_ldf( DX_PLIST );
DX_RETURNT dx_lddf( DX_PLIST );
DX_RETURNT dx_ldqf( DX_PLIST );
DX_RETURNT dx_ldfa( DX_PLIST );
DX_RETURNT dx_lddfa( DX_PLIST );
DX_RETURNT dx_ldblk( DX_PLIST );
DX_RETURNT dx_ldqfa( DX_PLIST );
DX_RETURNT dx_ldsb( DX_PLIST );
DX_RETURNT dx_ldsh( DX_PLIST );
DX_RETURNT dx_ldsw( DX_PLIST );
DX_RETURNT dx_ldub( DX_PLIST );
DX_RETURNT dx_lduh( DX_PLIST );
DX_RETURNT dx_lduw( DX_PLIST );
DX_RETURNT dx_ldx( DX_PLIST );
DX_RETURNT dx_ldd( DX_PLIST );
DX_RETURNT dx_ldsba( DX_PLIST );
DX_RETURNT dx_ldsha( DX_PLIST );
DX_RETURNT dx_ldswa( DX_PLIST );
DX_RETURNT dx_lduba( DX_PLIST );
DX_RETURNT dx_lduha( DX_PLIST );
DX_RETURNT dx_lduwa( DX_PLIST );
DX_RETURNT dx_ldxa( DX_PLIST );
DX_RETURNT dx_ldda( DX_PLIST );
DX_RETURNT dx_ldqa( DX_PLIST );
DX_RETURNT dx_stf( DX_PLIST );
DX_RETURNT dx_stdf( DX_PLIST );
DX_RETURNT dx_stqf( DX_PLIST );
DX_RETURNT dx_stb( DX_PLIST );
DX_RETURNT dx_sth( DX_PLIST );
DX_RETURNT dx_stw( DX_PLIST );
DX_RETURNT dx_stx( DX_PLIST );
DX_RETURNT dx_std( DX_PLIST );
DX_RETURNT dx_stfa( DX_PLIST );
DX_RETURNT dx_stdfa( DX_PLIST );
DX_RETURNT dx_stblk( DX_PLIST );
DX_RETURNT dx_stqfa( DX_PLIST );
DX_RETURNT dx_stba( DX_PLIST );
DX_RETURNT dx_stha( DX_PLIST );
DX_RETURNT dx_stwa( DX_PLIST );
DX_RETURNT dx_stxa( DX_PLIST );
DX_RETURNT dx_stda( DX_PLIST );
DX_RETURNT dx_ldstub( DX_PLIST );
DX_RETURNT dx_ldstuba( DX_PLIST );
DX_RETURNT dx_prefetch( DX_PLIST );
DX_RETURNT dx_prefetcha( DX_PLIST );
DX_RETURNT dx_swap( DX_PLIST );
DX_RETURNT dx_swapa( DX_PLIST );
DX_RETURNT dx_ldfsr( DX_PLIST );
DX_RETURNT dx_ldxfsr( DX_PLIST );
DX_RETURNT dx_stfsr( DX_PLIST );
DX_RETURNT dx_stxfsr( DX_PLIST );
DX_RETURNT dx__trap( DX_PLIST );
DX_RETURNT dx_impdep1( DX_PLIST );
DX_RETURNT dx_impdep2( DX_PLIST );
DX_RETURNT dx_membar( DX_PLIST );
DX_RETURNT dx_stbar( DX_PLIST );
DX_RETURNT dx_cmp( DX_PLIST );
DX_RETURNT dx_jmp( DX_PLIST );
DX_RETURNT dx_tst( DX_PLIST );
DX_RETURNT dx_mov( DX_PLIST );
DX_RETURNT dx_nop( DX_PLIST );
DX_RETURNT dx_not( DX_PLIST );
DX_RETURNT dx_rdpr( DX_PLIST );
DX_RETURNT dx_wrpr( DX_PLIST );
DX_RETURNT dx_faligndata( DX_PLIST );
DX_RETURNT dx_alignaddr( DX_PLIST );
DX_RETURNT dx_alignaddrl( DX_PLIST );
DX_RETURNT dx_fzero( DX_PLIST );
DX_RETURNT dx_fzeros( DX_PLIST );
DX_RETURNT dx_fsrc1( DX_PLIST );
DX_RETURNT dx_idealwr( DX_PLIST );
DX_RETURNT dx_fcmple16( DX_PLIST );
DX_RETURNT dx_fcmpne16( DX_PLIST );
DX_RETURNT dx_fcmple32( DX_PLIST );
DX_RETURNT dx_fcmpne32( DX_PLIST );
DX_RETURNT dx_fcmpgt16( DX_PLIST );
DX_RETURNT dx_fcmpeq16( DX_PLIST );
DX_RETURNT dx_fcmpgt32( DX_PLIST );
DX_RETURNT dx_fcmpeq32( DX_PLIST );
DX_RETURNT dx_bshuffle( DX_PLIST );
DX_RETURNT dx_bmask( DX_PLIST );
DX_RETURNT dx_ill( DX_PLIST );
#endif  /* _DX_H_ */
