! Copyright (C) 2007 Sun Microsystems, Inc.  All rights reserved.
! U.S. Government Rights - Commercial software.  Government users are
! subject to the Sun Microsystems, Inc. standard license agreement and
! applicable provisions of the FAR and its supplements.  Use is
! subject to license terms.  This distribution may include materials
! developed by third parties.Sun, Sun Microsystems and the Sun logo
! are trademarks or registered trademarks of Sun Microsystems, Inc. in
! the U.S. and other countries.  All SPARC trademarks are used under
! license and are trademarks or registered trademarks of SPARC
! International, Inc. in the U.S.  and other countries.
!
! ----------------------------------------------------------------------
!
! This file is part of the Adaptive Transactional Memory Test Platform
! (ATMTP) developed and maintained by Kevin Moore and Dan Nussbaum of
! the Scalable Synchronization Research Group at Sun Microsystems
! Laboratories (http://research.sun.com/scalable/).  For information
! about ATMTP, see the GEMS website: http://www.cs.wisc.edu/gems/.
!
! Please send email to atmtp-interest@sun.com with feedback,
! questions, or to request future announcements about ATMTP.
!
! ----------------------------------------------------------------------
!
! ATMTP is distributed as part of the GEMS software toolset and is
! available for use and modification under the terms of version 2 of
! the GNU General Public License.  The GNU General Public License is
! contained in the file $GEMS/LICENSE.
!
! Multifacet GEMS is free software; you can redistribute it and/or
! modify it under the terms of version 2 of the GNU General Public
! License as published by the Free Software Foundation.
!
! Multifacet GEMS is distributed in the hope that it will be useful,
! but WITHOUT ANY WARRANTY; without even the implied warranty of
! MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
! General Public License for more details.
!
! You should have received a copy of the GNU General Public License
! along with the Multifacet GEMS; if not, write to the Free Software
! Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307,
! USA
!
! ----------------------------------------------------------------------

	.inline		sysAtomicSwap,8
	.volatile
	swap		[%o1],%o0
	.nonvolatile
	.end

	.inline		sysCompareAndSwap,16
	.volatile
	cas		[%o2],%o1,%o0
	.nonvolatile
	.end

	.inline		sysCompareAndSwap64,24
	.volatile
	casx		[%o2],%o1,%o0
	.nonvolatile
	.end

	.inline		membarstoreload,0
	.volatile
	membar		#StoreLoad
	.nonvolatile
	.end

	.inline		testAndSetByte,1
	.volatile
	ldstub		[%o0], %o0
	.nonvolatile
	.end

	.inline		sysPopc,1
	.volatile
	popc		%o0, %o0
	.nonvolatile
	.end

	.inline		atomic2WordStore,24
	.volatile
	std		%o0,[%o2]
	.nonvolatile
	.end

	! Note that 64-bit code keeps an offset of 0x7ff in %sp/%fp,
	! so as to make it more efficient to access large structures
	! (such as arrays) that are on the stack.  We undo that offset
	! here.
	!
	.inline		getCurrentCFrame,0
	.volatile
	add		%sp, 0x7ff, %o0
	.nonvolatile
	.end

	! Note that 64-bit code keeps an offset of 0x7ff in %sp/%fp,
	! so as to make it more efficient to access large structures
	! (such as arrays) that are on the stack.  We undo that offset
	! here.
	!
	.inline		getCallerCFrame,0
	.volatile
	add		%fp, 0x7ff, %o0
	.nonvolatile
	.end

	! Warning: don't use in leaf methods - their ret addr is not in %i7.
	.inline		getCallerCFramePC,0
	.volatile
	mov		%i7, %o0
	.nonvolatile
	.end

	.inline		flushInstruction,8
	.volatile
	flush		%o0
	stbar
	.nonvolatile
	.end

	.inline		direct_fastEE,0
	.volatile
	ld		[%fp + 64], %o0
	.nonvolatile
	.end

	.inline		getThreadStruct,0
	.volatile
	mov		%g7, %o0
	.nonvolatile
	.end

! Get Ultra PICs
! o0: address of double-word for data

	.inline		getPIC,8
	.volatile
	rd		%pic, %o1
	stx		%o1, [%o0]
	.nonvolatile
	.end


