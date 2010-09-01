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
#ifndef _SIM_PORT_H
#define _SIM_PORT_H

#define PARM_POWER_STATS	1

/* RF module parameters */
#define PARM_read_port	1
#define PARM_write_port	1
#define PARM_n_regs	64
#define PARM_reg_width	32

#define PARM_ndwl	1
#define PARM_ndbl	1
#define PARM_nspd	1

//Niket

#define PARM_vc_in_arb_model QUEUE_ARBITER
#define PARM_vc_out_arb_model QUEUE_ARBITER
#define PARM_vc_in_arb_ff_model NEG_DFF
#define PARM_vc_out_arb_ff_model NEG_DFF
#define PARM_sw_in_arb_model QUEUE_ARBITER
#define PARM_sw_out_arb_model QUEUE_ARBITER
#define PARM_sw_in_arb_ff_model NEG_DFF
#define PARM_sw_out_arb_ff_model NEG_DFF
#define PARM_VC_per_MC 4

//Niket

//#define PARM_wordline_model	CACHE_RW_WORDLINE
//#define PARM_bitline_model	RW_BITLINE
//#define PARM_mem_model		NORMAL_MEM
//#define PARM_row_dec_model	SIM_NO_MODEL
//#define PARM_row_dec_pre_model	SIM_NO_MODEL
//#define PARM_col_dec_model	SIM_NO_MODEL
//#define PARM_col_dec_pre_model	SIM_NO_MODEL
//#define PARM_mux_model		SIM_NO_MODEL
//#define PARM_outdrv_model	SIM_NO_MODEL

/* these 3 should be changed together */
//#define PARM_data_end		2
//#define PARM_amp_model		GENERIC_AMP
//#define PARM_bitline_pre_model	EQU_BITLINE
//#define PARM_data_end		1
//#define PARM_amp_model		SIM_NO_MODEL
//#define PARM_bitline_pre_model	SINGLE_OTHER


/* router module parameters */
/* general parameters */
#define PARM_in_port 9
#define PARM_cache_in_port	0	/* # of cache input ports */
#define PARM_mc_in_port		0	/* # of memory controller input ports */
#define PARM_io_in_port		0	/* # of I/O device input ports */
#define PARM_out_port 9
#define PARM_cache_out_port	0	/* # of cache output ports */
#define PARM_mc_out_port	0	/* # of memory controller output ports */
#define PARM_io_out_port	0	/* # of I/O device output ports */
#define PARM_flit_width		128	/* flit width in bits */

/* virtual channel parameters */
#define PARM_v_channel		1	/* # of network port virtual channels */
#define PARM_v_class		0	/* # of total virtual classes */
#define PARM_cache_class	0	/* # of cache port virtual classes */
#define PARM_mc_class		0	/* # of memory controller port virtual classes */
#define PARM_io_class		0	/* # of I/O device port virtual classes */
/* ?? */
#define PARM_in_share_buf	0	/* do input virtual channels physically share buffers? */
#define PARM_out_share_buf	0	/* do output virtual channels physically share buffers? */
/* ?? */
#define PARM_in_share_switch	1	/* do input virtual channels share crossbar input ports? */
#define PARM_out_share_switch	1	/* do output virtual channels share crossbar output ports? */

/* crossbar parameters */
#define PARM_crossbar_model	MATRIX_CROSSBAR /* crossbar model type */
#define PARM_crsbar_degree	4		/* crossbar mux degree */
#define PARM_connect_type	TRISTATE_GATE	/* crossbar connector type */
#define PARM_trans_type		NP_GATE		/* crossbar transmission gate type */
#define PARM_crossbar_in_len	0		/* crossbar input line length, if known */
#define PARM_crossbar_out_len	0		/* crossbar output line length, if known */
#define PARM_xb_in_seg		0
#define PARM_xb_out_seg		0
/* HACK HACK HACK */
#define PARM_exp_xb_model	MATRIX_CROSSBAR
#define PARM_exp_in_seg		2
#define PARM_exp_out_seg	2

/* input buffer parameters */
#define PARM_in_buf		1	/* have input buffer? */
#define PARM_in_buf_set 	4
#define PARM_in_buf_rport	1	/* # of read ports */

#define PARM_cache_in_buf	0
#define PARM_cache_in_buf_set	0
#define PARM_cache_in_buf_rport	0

#define PARM_mc_in_buf		0
#define PARM_mc_in_buf_set	0
#define PARM_mc_in_buf_rport	0

#define PARM_io_in_buf		0
#define PARM_io_in_buf_set	0
#define PARM_io_in_buf_rport	0

/* output buffer parameters */
#define PARM_out_buf		0
#define PARM_out_buf_set	4
#define PARM_out_buf_wport	1

/* central buffer parameters */
#define PARM_central_buf	0	/* have central buffer? */
#define PARM_cbuf_set		2560	/* # of rows */
#define PARM_cbuf_rport		2	/* # of read ports */
#define PARM_cbuf_wport		2	/* # of write ports */
#define PARM_cbuf_width		4	/* # of flits in one row */
#define PARM_pipe_depth		4	/* # of banks */

/* array parameters shared by various buffers */
#define PARM_wordline_model	CACHE_RW_WORDLINE
#define PARM_bitline_model	RW_BITLINE
#define PARM_mem_model		NORMAL_MEM
#define PARM_row_dec_model	GENERIC_DEC
#define PARM_row_dec_pre_model	SINGLE_OTHER
#define PARM_col_dec_model	SIM_NO_MODEL
#define PARM_col_dec_pre_model	SIM_NO_MODEL
#define PARM_mux_model		SIM_NO_MODEL
#define PARM_outdrv_model	REG_OUTDRV

/* these 3 should be changed together */
/* use double-ended bitline because the array is too large */
#define PARM_data_end		2
#define PARM_amp_model		GENERIC_AMP
#define PARM_bitline_pre_model	EQU_BITLINE
//#define PARM_data_end		1
//#define PARM_amp_model		SIM_NO_MODEL
//#define PARM_bitline_pre_model	SINGLE_OTHER

/* arbiter parameters */
#define PARM_in_arb_model	MATRIX_ARBITER	/* input side arbiter model type */
#define PARM_in_arb_ff_model	NEG_DFF		/* input side arbiter flip-flop model type */
#define PARM_out_arb_model	MATRIX_ARBITER	/* output side arbiter model type */
#define PARM_out_arb_ff_model	NEG_DFF		/* output side arbiter flip-flop model type */

#endif	/* _SIM_PORT_H */
