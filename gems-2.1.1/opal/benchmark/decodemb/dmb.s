! Decode micro-benchmark.
!    This assembly language program tests the ability of a simulator
!    to decode and effectively execute many (almost all) of the Sparc V9
!    op codes.
	
! Register layout for a NetBSD/SunOS system call:
! %g1 - syscall number (See /usr/include/sys/syscall.h)
! %o0 - 1st argument
! %o1 - 2nd argument (if there that syscall takes a 2nd arg)
! %o2 - 3rd argument (if there that syscall takes a 3rd arg)
! See "The SPARC Architecture Manual" for ABI info.
!
! Once the program loads the registers as above, it does a "ta"
! (Trap Always) instruction to switch to CPU supervisor mode, and
! branch to the trap vector.  "The SPARC Architecture Manual"
! Chapter 7 (both Version 8 and Version 9 manuals) contains info
! about traps, trap vectors and CPU modes.
!

.seg "text"

.global proc_bcc
.align 4
.proc 4
proc_bcc:
	save %sp, -336, %sp
	
	! branch testing: each time it adds one to %o4
	ba br_0
	add %o4, 1, %o4
br_0:
	bn br_1
	add %o4, 1, %o4
br_1:
	bne br_2
	add %o4, 1, %o4
br_2:
	be br_3
	add %o4, 1, %o4
br_3:
	bg br_4
	add %o4, 1, %o4
br_4:
	ble br_5
	add %o4, 1, %o4
br_5:
	bge br_6
	add %o4, 1, %o4
br_6:
	bl br_7
	add %o4, 1, %o4
br_7:
	bgu br_8
	add %o4, 1, %o4
br_8:
	bleu br_9
	add %o4, 1, %o4
br_9:
	bcc br_10
	add %o4, 1, %o4
br_10:
	bcs br_11
	add %o4, 1, %o4
br_11:
	bpos br_12
	add %o4, 1, %o4
br_12:
	bneg br_13
	add %o4, 1, %o4
br_13:
	bvc br_14
	add %o4, 1, %o4
br_14:
	bvs br_15
	add %o4, 1, %o4
br_15:
	ba,a br_16
	add %o4, 1, %o4
br_16:
	bn,a br_17
	add %o4, 1, %o4
br_17:
	bne,a br_18
	add %o4, 1, %o4
br_18:
	be,a br_19
	add %o4, 1, %o4
br_19:
	bg,a br_20
	add %o4, 1, %o4
br_20:
	ble,a br_21
	add %o4, 1, %o4
br_21:
	bge,a br_22
	add %o4, 1, %o4
br_22:
	bl,a br_23
	add %o4, 1, %o4
br_23:
	bgu,a br_24
	add %o4, 1, %o4
br_24:
	bleu,a br_25
	add %o4, 1, %o4
br_25:
	bcc,a br_26
	add %o4, 1, %o4
br_26:
	bcs,a br_27
	add %o4, 1, %o4
br_27:
	bpos,a br_28
	add %o4, 1, %o4
br_28:
	bneg,a br_29
	add %o4, 1, %o4
br_29:
	bvc,a br_30
	add %o4, 1, %o4
br_30:
	bvs,a br_31
	add %o4, 1, %o4
br_31:
	ba,pn %icc, br_32
	add %o4, 1, %o4
br_32:
	bn,pn %icc, br_33
	add %o4, 1, %o4
br_33:
	bne,pn %icc, br_34
	add %o4, 1, %o4
br_34:
	be,pn %icc, br_35
	add %o4, 1, %o4
br_35:
	bg,pn %icc, br_36
	add %o4, 1, %o4
br_36:
	ble,pn %icc, br_37
	add %o4, 1, %o4
br_37:
	bge,pn %icc, br_38
	add %o4, 1, %o4
br_38:
	bl,pn %icc, br_39
	add %o4, 1, %o4
br_39:
	bgu,pn %icc, br_40
	add %o4, 1, %o4
br_40:
	bleu,pn %icc, br_41
	add %o4, 1, %o4
br_41:
	bcc,pn %icc, br_42
	add %o4, 1, %o4
br_42:
	bcs,pn %icc, br_43
	add %o4, 1, %o4
br_43:
	bpos,pn %icc, br_44
	add %o4, 1, %o4
br_44:
	bneg,pn %icc, br_45
	add %o4, 1, %o4
br_45:
	bvc,pn %icc, br_46
	add %o4, 1, %o4
br_46:
	bvs,pn %icc, br_47
	add %o4, 1, %o4
br_47:
	ba,pn %xcc, br_48
	add %o4, 1, %o4
br_48:
	bn,pn %xcc, br_49
	add %o4, 1, %o4
br_49:
	bne,pn %xcc, br_50
	add %o4, 1, %o4
br_50:
	be,pn %xcc, br_51
	add %o4, 1, %o4
br_51:
	bg,pn %xcc, br_52
	add %o4, 1, %o4
br_52:
	ble,pn %xcc, br_53
	add %o4, 1, %o4
br_53:
	bge,pn %xcc, br_54
	add %o4, 1, %o4
br_54:
	bl,pn %xcc, br_55
	add %o4, 1, %o4
br_55:
	bgu,pn %xcc, br_56
	add %o4, 1, %o4
br_56:
	bleu,pn %xcc, br_57
	add %o4, 1, %o4
br_57:
	bcc,pn %xcc, br_58
	add %o4, 1, %o4
br_58:
	bcs,pn %xcc, br_59
	add %o4, 1, %o4
br_59:
	bpos,pn %xcc, br_60
	add %o4, 1, %o4
br_60:
	bneg,pn %xcc, br_61
	add %o4, 1, %o4
br_61:
	bvc,pn %xcc, br_62
	add %o4, 1, %o4
br_62:
	bvs,pn %xcc, br_63
	add %o4, 1, %o4
br_63:
	ba,pt %icc, br_64
	add %o4, 1, %o4
br_64:
	bn,pt %icc, br_65
	add %o4, 1, %o4
br_65:
	bne,pt %icc, br_66
	add %o4, 1, %o4
br_66:
	be,pt %icc, br_67
	add %o4, 1, %o4
br_67:
	bg,pt %icc, br_68
	add %o4, 1, %o4
br_68:
	ble,pt %icc, br_69
	add %o4, 1, %o4
br_69:
	bge,pt %icc, br_70
	add %o4, 1, %o4
br_70:
	bl,pt %icc, br_71
	add %o4, 1, %o4
br_71:
	bgu,pt %icc, br_72
	add %o4, 1, %o4
br_72:
	bleu,pt %icc, br_73
	add %o4, 1, %o4
br_73:
	bcc,pt %icc, br_74
	add %o4, 1, %o4
br_74:
	bcs,pt %icc, br_75
	add %o4, 1, %o4
br_75:
	bpos,pt %icc, br_76
	add %o4, 1, %o4
br_76:
	bneg,pt %icc, br_77
	add %o4, 1, %o4
br_77:
	bvc,pt %icc, br_78
	add %o4, 1, %o4
br_78:
	bvs,pt %icc, br_79
	add %o4, 1, %o4
br_79:
	ba,pt %xcc, br_80
	add %o4, 1, %o4
br_80:
	bn,pt %xcc, br_81
	add %o4, 1, %o4
br_81:
	bne,pt %xcc, br_82
	add %o4, 1, %o4
br_82:
	be,pt %xcc, br_83
	add %o4, 1, %o4
br_83:
	bg,pt %xcc, br_84
	add %o4, 1, %o4
br_84:
	ble,pt %xcc, br_85
	add %o4, 1, %o4
br_85:
	bge,pt %xcc, br_86
	add %o4, 1, %o4
br_86:
	bl,pt %xcc, br_87
	add %o4, 1, %o4
br_87:
	bgu,pt %xcc, br_88
	add %o4, 1, %o4
br_88:
	bleu,pt %xcc, br_89
	add %o4, 1, %o4
br_89:
	bcc,pt %xcc, br_90
	add %o4, 1, %o4
br_90:
	bcs,pt %xcc, br_91
	add %o4, 1, %o4
br_91:
	bpos,pt %xcc, br_92
	add %o4, 1, %o4
br_92:
	bneg,pt %xcc, br_93
	add %o4, 1, %o4
br_93:
	bvc,pt %xcc, br_94
	add %o4, 1, %o4
br_94:
	bvs,pt %xcc, br_95
	add %o4, 1, %o4
br_95:
	ba,a,pn %icc, br_96
	add %o4, 1, %o4
br_96:
	bn,a,pn %icc, br_97
	add %o4, 1, %o4
br_97:
	bne,a,pn %icc, br_98
	add %o4, 1, %o4
br_98:
	be,a,pn %icc, br_99
	add %o4, 1, %o4
br_99:
	bg,a,pn %icc, br_100
	add %o4, 1, %o4
br_100:
	ble,a,pn %icc, br_101
	add %o4, 1, %o4
br_101:
	bge,a,pn %icc, br_102
	add %o4, 1, %o4
br_102:
	bl,a,pn %icc, br_103
	add %o4, 1, %o4
br_103:
	bgu,a,pn %icc, br_104
	add %o4, 1, %o4
br_104:
	bleu,a,pn %icc, br_105
	add %o4, 1, %o4
br_105:
	bcc,a,pn %icc, br_106
	add %o4, 1, %o4
br_106:
	bcs,a,pn %icc, br_107
	add %o4, 1, %o4
br_107:
	bpos,a,pn %icc, br_108
	add %o4, 1, %o4
br_108:
	bneg,a,pn %icc, br_109
	add %o4, 1, %o4
br_109:
	bvc,a,pn %icc, br_110
	add %o4, 1, %o4
br_110:
	bvs,a,pn %icc, br_111
	add %o4, 1, %o4
br_111:
	ba,a,pn %xcc, br_112
	add %o4, 1, %o4
br_112:
	bn,a,pn %xcc, br_113
	add %o4, 1, %o4
br_113:
	bne,a,pn %xcc, br_114
	add %o4, 1, %o4
br_114:
	be,a,pn %xcc, br_115
	add %o4, 1, %o4
br_115:
	bg,a,pn %xcc, br_116
	add %o4, 1, %o4
br_116:
	ble,a,pn %xcc, br_117
	add %o4, 1, %o4
br_117:
	bge,a,pn %xcc, br_118
	add %o4, 1, %o4
br_118:
	bl,a,pn %xcc, br_119
	add %o4, 1, %o4
br_119:
	bgu,a,pn %xcc, br_120
	add %o4, 1, %o4
br_120:
	bleu,a,pn %xcc, br_121
	add %o4, 1, %o4
br_121:
	bcc,a,pn %xcc, br_122
	add %o4, 1, %o4
br_122:
	bcs,a,pn %xcc, br_123
	add %o4, 1, %o4
br_123:
	bpos,a,pn %xcc, br_124
	add %o4, 1, %o4
br_124:
	bneg,a,pn %xcc, br_125
	add %o4, 1, %o4
br_125:
	bvc,a,pn %xcc, br_126
	add %o4, 1, %o4
br_126:
	bvs,a,pn %xcc, br_127
	add %o4, 1, %o4
br_127:
	ba,a,pt %icc, br_128
	add %o4, 1, %o4
br_128:
	bn,a,pt %icc, br_129
	add %o4, 1, %o4
br_129:
	bne,a,pt %icc, br_130
	add %o4, 1, %o4
br_130:
	be,a,pt %icc, br_131
	add %o4, 1, %o4
br_131:
	bg,a,pt %icc, br_132
	add %o4, 1, %o4
br_132:
	ble,a,pt %icc, br_133
	add %o4, 1, %o4
br_133:
	bge,a,pt %icc, br_134
	add %o4, 1, %o4
br_134:
	bl,a,pt %icc, br_135
	add %o4, 1, %o4
br_135:
	bgu,a,pt %icc, br_136
	add %o4, 1, %o4
br_136:
	bleu,a,pt %icc, br_137
	add %o4, 1, %o4
br_137:
	bcc,a,pt %icc, br_138
	add %o4, 1, %o4
br_138:
	bcs,a,pt %icc, br_139
	add %o4, 1, %o4
br_139:
	bpos,a,pt %icc, br_140
	add %o4, 1, %o4
br_140:
	bneg,a,pt %icc, br_141
	add %o4, 1, %o4
br_141:
	bvc,a,pt %icc, br_142
	add %o4, 1, %o4
br_142:
	bvs,a,pt %icc, br_143
	add %o4, 1, %o4
br_143:
	ba,a,pt %xcc, br_144
	add %o4, 1, %o4
br_144:
	bn,a,pt %xcc, br_145
	add %o4, 1, %o4
br_145:
	bne,a,pt %xcc, br_146
	add %o4, 1, %o4
br_146:
	be,a,pt %xcc, br_147
	add %o4, 1, %o4
br_147:
	bg,a,pt %xcc, br_148
	add %o4, 1, %o4
br_148:
	ble,a,pt %xcc, br_149
	add %o4, 1, %o4
br_149:
	bge,a,pt %xcc, br_150
	add %o4, 1, %o4
br_150:
	bl,a,pt %xcc, br_151
	add %o4, 1, %o4
br_151:
	bgu,a,pt %xcc, br_152
	add %o4, 1, %o4
br_152:
	bleu,a,pt %xcc, br_153
	add %o4, 1, %o4
br_153:
	bcc,a,pt %xcc, br_154
	add %o4, 1, %o4
br_154:
	bcs,a,pt %xcc, br_155
	add %o4, 1, %o4
br_155:
	bpos,a,pt %xcc, br_156
	add %o4, 1, %o4
br_156:
	bneg,a,pt %xcc, br_157
	add %o4, 1, %o4
br_157:
	bvc,a,pt %xcc, br_158
	add %o4, 1, %o4
br_158:
	bvs,a,pt %xcc, br_159
	add %o4, 1, %o4
br_159:
	nop
!! Branch on register %i0
	brz,pt %i0, brr_0
	add %o4, 1, %o4
brr_0:
	brlez,pt %i0, brr_1
	add %o4, 1, %o4
brr_1:
	brlz,pt %i0, brr_2
	add %o4, 1, %o4
brr_2:
	brnz,pt %i0, brr_3
	add %o4, 1, %o4
brr_3:
	brgz,pt %i0, brr_4
	add %o4, 1, %o4
brr_4:
	brgez,pt %i0, brr_5
	add %o4, 1, %o4
brr_5:
	brz,pn %i0, brr_6
	add %o4, 1, %o4
brr_6:
	brlez,pn %i0, brr_7
	add %o4, 1, %o4
brr_7:
	brlz,pn %i0, brr_8
	add %o4, 1, %o4
brr_8:
	brnz,pn %i0, brr_9
	add %o4, 1, %o4
brr_9:
	brgz,pn %i0, brr_10
	add %o4, 1, %o4
brr_10:
	brgez,pn %i0, brr_11
	add %o4, 1, %o4
brr_11:
	brz,a,pt %i0, brr_12
	add %o4, 1, %o4
brr_12:
	brlez,a,pt %i0, brr_13
	add %o4, 1, %o4
brr_13:
	brlz,a,pt %i0, brr_14
	add %o4, 1, %o4
brr_14:
	brnz,a,pt %i0, brr_15
	add %o4, 1, %o4
brr_15:
	brgz,a,pt %i0, brr_16
	add %o4, 1, %o4
brr_16:
	brgez,a,pt %i0, brr_17
	add %o4, 1, %o4
brr_17:
	brz,a,pn %i0, brr_18
	add %o4, 1, %o4
brr_18:
	brlez,a,pn %i0, brr_19
	add %o4, 1, %o4
brr_19:
	brlz,a,pn %i0, brr_20
	add %o4, 1, %o4
brr_20:
	brnz,a,pn %i0, brr_21
	add %o4, 1, %o4
brr_21:
	brgz,a,pn %i0, brr_22
	add %o4, 1, %o4
brr_22:
	brgez,a,pn %i0, brr_23
	add %o4, 1, %o4
brr_23:
	nop
	nop
	ret
	restore	
! end proc_bcc

proc_fbcc:
	save %sp, -336, %sp

	! branch testing: each time it adds one to %o4
	fba fbr_0
	add %o4, 1, %o4
fbr_0:
	fbn fbr_1
	add %o4, 1, %o4
fbr_1:
	fbu fbr_2
	add %o4, 1, %o4
fbr_2:
	fbg fbr_3
	add %o4, 1, %o4
fbr_3:
	fbug fbr_4
	add %o4, 1, %o4
fbr_4:
	fbl fbr_5
	add %o4, 1, %o4
fbr_5:
	fbul fbr_6
	add %o4, 1, %o4
fbr_6:
	fblg fbr_7
	add %o4, 1, %o4
fbr_7:
	fbne fbr_8
	add %o4, 1, %o4
fbr_8:
	fbe fbr_9
	add %o4, 1, %o4
fbr_9:
	fbue fbr_10
	add %o4, 1, %o4
fbr_10:
	fbge fbr_11
	add %o4, 1, %o4
fbr_11:
	fbuge fbr_12
	add %o4, 1, %o4
fbr_12:
	fble fbr_13
	add %o4, 1, %o4
fbr_13:
	fbule fbr_14
	add %o4, 1, %o4
fbr_14:
	fbo fbr_15
	add %o4, 1, %o4
fbr_15:
	fba,a fbr_16
	add %o4, 1, %o4
fbr_16:
	fbn,a fbr_17
	add %o4, 1, %o4
fbr_17:
	fbu,a fbr_18
	add %o4, 1, %o4
fbr_18:
	fbg,a fbr_19
	add %o4, 1, %o4
fbr_19:
	fbug,a fbr_20
	add %o4, 1, %o4
fbr_20:
	fbl,a fbr_21
	add %o4, 1, %o4
fbr_21:
	fbul,a fbr_22
	add %o4, 1, %o4
fbr_22:
	fblg,a fbr_23
	add %o4, 1, %o4
fbr_23:
	fbne,a fbr_24
	add %o4, 1, %o4
fbr_24:
	fbe,a fbr_25
	add %o4, 1, %o4
fbr_25:
	fbue,a fbr_26
	add %o4, 1, %o4
fbr_26:
	fbge,a fbr_27
	add %o4, 1, %o4
fbr_27:
	fbuge,a fbr_28
	add %o4, 1, %o4
fbr_28:
	fble,a fbr_29
	add %o4, 1, %o4
fbr_29:
	fbule,a fbr_30
	add %o4, 1, %o4
fbr_30:
	fbo,a fbr_31
	add %o4, 1, %o4
fbr_31:
	fba,pn %fcc0, fbr_32
	add %o4, 1, %o4
fbr_32:
	fbn,pn %fcc0, fbr_33
	add %o4, 1, %o4
fbr_33:
	fbu,pn %fcc0, fbr_34
	add %o4, 1, %o4
fbr_34:
	fbg,pn %fcc0, fbr_35
	add %o4, 1, %o4
fbr_35:
	fbug,pn %fcc0, fbr_36
	add %o4, 1, %o4
fbr_36:
	fbl,pn %fcc0, fbr_37
	add %o4, 1, %o4
fbr_37:
	fbul,pn %fcc0, fbr_38
	add %o4, 1, %o4
fbr_38:
	fblg,pn %fcc0, fbr_39
	add %o4, 1, %o4
fbr_39:
	fbne,pn %fcc0, fbr_40
	add %o4, 1, %o4
fbr_40:
	fbe,pn %fcc0, fbr_41
	add %o4, 1, %o4
fbr_41:
	fbue,pn %fcc0, fbr_42
	add %o4, 1, %o4
fbr_42:
	fbge,pn %fcc0, fbr_43
	add %o4, 1, %o4
fbr_43:
	fbuge,pn %fcc0, fbr_44
	add %o4, 1, %o4
fbr_44:
	fble,pn %fcc0, fbr_45
	add %o4, 1, %o4
fbr_45:
	fbule,pn %fcc0, fbr_46
	add %o4, 1, %o4
fbr_46:
	fbo,pn %fcc0, fbr_47
	add %o4, 1, %o4
fbr_47:
	fba,pt %fcc1, fbr_48
	add %o4, 1, %o4
fbr_48:
	fbn,pt %fcc1, fbr_49
	add %o4, 1, %o4
fbr_49:
	fbu,pt %fcc1, fbr_50
	add %o4, 1, %o4
fbr_50:
	fbg,pt %fcc1, fbr_51
	add %o4, 1, %o4
fbr_51:
	fbug,pt %fcc1, fbr_52
	add %o4, 1, %o4
fbr_52:
	fbl,pt %fcc1, fbr_53
	add %o4, 1, %o4
fbr_53:
	fbul,pt %fcc1, fbr_54
	add %o4, 1, %o4
fbr_54:
	fblg,pt %fcc1, fbr_55
	add %o4, 1, %o4
fbr_55:
	fbne,pt %fcc1, fbr_56
	add %o4, 1, %o4
fbr_56:
	fbe,pt %fcc1, fbr_57
	add %o4, 1, %o4
fbr_57:
	fbue,pt %fcc1, fbr_58
	add %o4, 1, %o4
fbr_58:
	fbge,pt %fcc1, fbr_59
	add %o4, 1, %o4
fbr_59:
	fbuge,pt %fcc1, fbr_60
	add %o4, 1, %o4
fbr_60:
	fble,pt %fcc1, fbr_61
	add %o4, 1, %o4
fbr_61:
	fbule,pt %fcc1, fbr_62
	add %o4, 1, %o4
fbr_62:
	fbo,pt %fcc1, fbr_63
	add %o4, 1, %o4
fbr_63:
!	fba,a,pn %fcc2, fbr_64
!	add %o4, 1, %o4
fbr_64:
	fbn,a,pn %fcc2, fbr_65
	add %o4, 1, %o4
fbr_65:
	fbu,a,pn %fcc2, fbr_66
	add %o4, 1, %o4
fbr_66:
	fbg,a,pn %fcc2, fbr_67
	add %o4, 1, %o4
fbr_67:
	fbug,a,pn %fcc2, fbr_68
	add %o4, 1, %o4
fbr_68:
	fbl,a,pn %fcc2, fbr_69
	add %o4, 1, %o4
fbr_69:
	fbul,a,pn %fcc2, fbr_70
	add %o4, 1, %o4
fbr_70:
	fblg,a,pn %fcc2, fbr_71
	add %o4, 1, %o4
fbr_71:
	fbne,a,pn %fcc2, fbr_72
	add %o4, 1, %o4
fbr_72:
	fbe,a,pn %fcc2, fbr_73
	add %o4, 1, %o4
fbr_73:
	fbue,a,pn %fcc2, fbr_74
	add %o4, 1, %o4
fbr_74:
	fbge,a,pn %fcc2, fbr_75
	add %o4, 1, %o4
fbr_75:
	fbuge,a,pn %fcc2, fbr_76
	add %o4, 1, %o4
fbr_76:
	fble,a,pn %fcc2, fbr_77
	add %o4, 1, %o4
fbr_77:
	fbule,a,pn %fcc2, fbr_78
	add %o4, 1, %o4
fbr_78:
	fbo,a,pn %fcc2, fbr_79
	add %o4, 1, %o4
fbr_79:
!	fba,a,pt %fcc3, fbr_80
!	add %o4, 1, %o4
fbr_80:
	fbn,a,pt %fcc3, fbr_81
	add %o4, 1, %o4
fbr_81:
	fbu,a,pt %fcc3, fbr_82
	add %o4, 1, %o4
fbr_82:
	fbg,a,pt %fcc3, fbr_83
	add %o4, 1, %o4
fbr_83:
	fbug,a,pt %fcc3, fbr_84
	add %o4, 1, %o4
fbr_84:
	fbl,a,pt %fcc3, fbr_85
	add %o4, 1, %o4
fbr_85:
	fbul,a,pt %fcc3, fbr_86
	add %o4, 1, %o4
fbr_86:
	fblg,a,pt %fcc3, fbr_87
	add %o4, 1, %o4
fbr_87:
	fbne,a,pt %fcc3, fbr_88
	add %o4, 1, %o4
fbr_88:
	fbe,a,pt %fcc3, fbr_89
	add %o4, 1, %o4
fbr_89:
	fbue,a,pt %fcc3, fbr_90
	add %o4, 1, %o4
fbr_90:
	fbge,a,pt %fcc3, fbr_91
	add %o4, 1, %o4
fbr_91:
	fbuge,a,pt %fcc3, fbr_92
	add %o4, 1, %o4
fbr_92:
	fble,a,pt %fcc3, fbr_93
	add %o4, 1, %o4
fbr_93:
	fbule,a,pt %fcc3, fbr_94
	add %o4, 1, %o4
fbr_94:
	fbo,a,pt %fcc3, fbr_95
	add %o4, 1, %o4
fbr_95:
	nop
	nop
	ret
	restore
! end proc_fbcc

proc_cmov:		
	save %sp, -336, %sp
	
	! conditional move testing: move things into i0
	mova %icc, 1, %i0
	set 0, %i0
	movn %icc, 1, %i0
	set 0, %i0
	movne %icc, 1, %i0
	set 0, %i0
	move %icc, 1, %i0
	set 0, %i0
	movg %icc, 1, %i0
	set 0, %i0
	movle %icc, 1, %i0
	set 0, %i0
	movge %icc, 1, %i0
	set 0, %i0
	movl %icc, 1, %i0
	set 0, %i0
	movgu %icc, 1, %i0
	set 0, %i0
	movleu %icc, 1, %i0
	set 0, %i0
	movcc %icc, 1, %i0
	set 0, %i0
	movcs %icc, 1, %i0
	set 0, %i0
	movpos %icc, 1, %i0
	set 0, %i0
	movneg %icc, 1, %i0
	set 0, %i0
	movvc %icc, 1, %i0
	set 0, %i0
	movvs %icc, 1, %i0
	set 0, %i0
	mova %icc, %g0, %i0
	set 1, %i0
	movn %icc, %g0, %i0
	set 1, %i0
	movne %icc, %g0, %i0
	set 1, %i0
	move %icc, %g0, %i0
	set 1, %i0
	movg %icc, %g0, %i0
	set 1, %i0
	movle %icc, %g0, %i0
	set 1, %i0
	movge %icc, %g0, %i0
	set 1, %i0
	movl %icc, %g0, %i0
	set 1, %i0
	movgu %icc, %g0, %i0
	set 1, %i0
	movleu %icc, %g0, %i0
	set 1, %i0
	movcc %icc, %g0, %i0
	set 1, %i0
	movcs %icc, %g0, %i0
	set 1, %i0
	movpos %icc, %g0, %i0
	set 1, %i0
	movneg %icc, %g0, %i0
	set 1, %i0
	movvc %icc, %g0, %i0
	set 1, %i0
	movvs %icc, %g0, %i0
	set 1, %i0
	mova %xcc, 1, %i0
	set 0, %i0
	movn %xcc, 1, %i0
	set 0, %i0
	movne %xcc, 1, %i0
	set 0, %i0
	move %xcc, 1, %i0
	set 0, %i0
	movg %xcc, 1, %i0
	set 0, %i0
	movle %xcc, 1, %i0
	set 0, %i0
	movge %xcc, 1, %i0
	set 0, %i0
	movl %xcc, 1, %i0
	set 0, %i0
	movgu %xcc, 1, %i0
	set 0, %i0
	movleu %xcc, 1, %i0
	set 0, %i0
	movcc %xcc, 1, %i0
	set 0, %i0
	movcs %xcc, 1, %i0
	set 0, %i0
	movpos %xcc, 1, %i0
	set 0, %i0
	movneg %xcc, 1, %i0
	set 0, %i0
	movvc %xcc, 1, %i0
	set 0, %i0
	movvs %xcc, 1, %i0
	set 0, %i0
	mova %xcc, %g0, %i0
	set 1, %i0
	movn %xcc, %g0, %i0
	set 1, %i0
	movne %xcc, %g0, %i0
	set 1, %i0
	move %xcc, %g0, %i0
	set 1, %i0
	movg %xcc, %g0, %i0
	set 1, %i0
	movle %xcc, %g0, %i0
	set 1, %i0
	movge %xcc, %g0, %i0
	set 1, %i0
	movl %xcc, %g0, %i0
	set 1, %i0
	movgu %xcc, %g0, %i0
	set 1, %i0
	movleu %xcc, %g0, %i0
	set 1, %i0
	movcc %xcc, %g0, %i0
	set 1, %i0
	movcs %xcc, %g0, %i0
	set 1, %i0
	movpos %xcc, %g0, %i0
	set 1, %i0
	movneg %xcc, %g0, %i0
	set 1, %i0
	movvc %xcc, %g0, %i0
	set 1, %i0
	movvs %xcc, %g0, %i0
	set 1, %i0
	mova %fcc0, 1, %i0
	set 0, %i0
	movn %fcc0, 1, %i0
	set 0, %i0
	movu %fcc0, 1, %i0
	set 0, %i0
	movg %fcc0, 1, %i0
	set 0, %i0
	movug %fcc0, 1, %i0
	set 0, %i0
	movl %fcc0, 1, %i0
	set 0, %i0
	movul %fcc0, 1, %i0
	set 0, %i0
	movlg %fcc0, 1, %i0
	set 0, %i0
	movne %fcc0, 1, %i0
	set 0, %i0
	move %fcc0, 1, %i0
	set 0, %i0
	movue %fcc0, 1, %i0
	set 0, %i0
	movge %fcc0, 1, %i0
	set 0, %i0
	movuge %fcc0, 1, %i0
	set 0, %i0
	movle %fcc0, 1, %i0
	set 0, %i0
	movule %fcc0, 1, %i0
	set 0, %i0
	movo %fcc0, 1, %i0
	set 0, %i0
	mova %fcc1, %g0, %i0
	set 1, %i0
	movn %fcc1, %g0, %i0
	set 1, %i0
	movu %fcc1, %g0, %i0
	set 1, %i0
	movg %fcc1, %g0, %i0
	set 1, %i0
	movug %fcc1, %g0, %i0
	set 1, %i0
	movl %fcc1, %g0, %i0
	set 1, %i0
	movul %fcc1, %g0, %i0
	set 1, %i0
	movlg %fcc1, %g0, %i0
	set 1, %i0
	movne %fcc1, %g0, %i0
	set 1, %i0
	move %fcc1, %g0, %i0
	set 1, %i0
	movue %fcc1, %g0, %i0
	set 1, %i0
	movge %fcc1, %g0, %i0
	set 1, %i0
	movuge %fcc1, %g0, %i0
	set 1, %i0
	movle %fcc1, %g0, %i0
	set 1, %i0
	movule %fcc1, %g0, %i0
	set 1, %i0
	movo %fcc1, %g0, %i0
	set 1, %i0
	mova %fcc2, 1, %i0
	set 0, %i0
	movn %fcc2, 1, %i0
	set 0, %i0
	movu %fcc2, 1, %i0
	set 0, %i0
	movg %fcc2, 1, %i0
	set 0, %i0
	movug %fcc2, 1, %i0
	set 0, %i0
	movl %fcc2, 1, %i0
	set 0, %i0
	movul %fcc2, 1, %i0
	set 0, %i0
	movlg %fcc2, 1, %i0
	set 0, %i0
	movne %fcc2, 1, %i0
	set 0, %i0
	move %fcc2, 1, %i0
	set 0, %i0
	movue %fcc2, 1, %i0
	set 0, %i0
	movge %fcc2, 1, %i0
	set 0, %i0
	movuge %fcc2, 1, %i0
	set 0, %i0
	movle %fcc2, 1, %i0
	set 0, %i0
	movule %fcc2, 1, %i0
	set 0, %i0
	movo %fcc2, 1, %i0
	set 0, %i0
	mova %fcc3, %g0, %i0
	set 1, %i0
	movn %fcc3, %g0, %i0
	set 1, %i0
	movu %fcc3, %g0, %i0
	set 1, %i0
	movg %fcc3, %g0, %i0
	set 1, %i0
	movug %fcc3, %g0, %i0
	set 1, %i0
	movl %fcc3, %g0, %i0
	set 1, %i0
	movul %fcc3, %g0, %i0
	set 1, %i0
	movlg %fcc3, %g0, %i0
	set 1, %i0
	movne %fcc3, %g0, %i0
	set 1, %i0
	move %fcc3, %g0, %i0
	set 1, %i0
	movue %fcc3, %g0, %i0
	set 1, %i0
	movge %fcc3, %g0, %i0
	set 1, %i0
	movuge %fcc3, %g0, %i0
	set 1, %i0
	movle %fcc3, %g0, %i0
	set 1, %i0
	movule %fcc3, %g0, %i0
	set 1, %i0
	movo %fcc3, %g0, %i0
	set 1, %i0
	ret
	restore
! end proc cmov

proc_fmov:
	save %sp, -336, %sp
	fzero %f0
	fmovsa %icc, %f0, %f2
	fone %f2
	fmovsn %icc, %f0, %f2
	fone %f2
	fmovsne %icc, %f0, %f2
	fone %f2
	fmovse %icc, %f0, %f2
	fone %f2
	fmovsg %icc, %f0, %f2
	fone %f2
	fmovsle %icc, %f0, %f2
	fone %f2
	fmovsge %icc, %f0, %f2
	fone %f2
	fmovsl %icc, %f0, %f2
	fone %f2
	fmovsgu %icc, %f0, %f2
	fone %f2
	fmovsleu %icc, %f0, %f2
	fone %f2
	fmovscc %icc, %f0, %f2
	fone %f2
	fmovscs %icc, %f0, %f2
	fone %f2
	fmovspos %icc, %f0, %f2
	fone %f2
	fmovsneg %icc, %f0, %f2
	fone %f2
	fmovsvc %icc, %f0, %f2
	fone %f2
	fmovsvs %icc, %f0, %f2
	fone %f2
	fmovsa %xcc, %f0, %f2
	fone %f2
	fmovsn %xcc, %f0, %f2
	fone %f2
	fmovsne %xcc, %f0, %f2
	fone %f2
	fmovse %xcc, %f0, %f2
	fone %f2
	fmovsg %xcc, %f0, %f2
	fone %f2
	fmovsle %xcc, %f0, %f2
	fone %f2
	fmovsge %xcc, %f0, %f2
	fone %f2
	fmovsl %xcc, %f0, %f2
	fone %f2
	fmovsgu %xcc, %f0, %f2
	fone %f2
	fmovsleu %xcc, %f0, %f2
	fone %f2
	fmovscc %xcc, %f0, %f2
	fone %f2
	fmovscs %xcc, %f0, %f2
	fone %f2
	fmovspos %xcc, %f0, %f2
	fone %f2
	fmovsneg %xcc, %f0, %f2
	fone %f2
	fmovsvc %xcc, %f0, %f2
	fone %f2
	fmovsvs %xcc, %f0, %f2
	fone %f2
	fmovda %icc, %f0, %f2
	fone %f2
	fmovdn %icc, %f0, %f2
	fone %f2
	fmovdne %icc, %f0, %f2
	fone %f2
	fmovde %icc, %f0, %f2
	fone %f2
	fmovdg %icc, %f0, %f2
	fone %f2
	fmovdle %icc, %f0, %f2
	fone %f2
	fmovdge %icc, %f0, %f2
	fone %f2
	fmovdl %icc, %f0, %f2
	fone %f2
	fmovdgu %icc, %f0, %f2
	fone %f2
	fmovdleu %icc, %f0, %f2
	fone %f2
	fmovdcc %icc, %f0, %f2
	fone %f2
	fmovdcs %icc, %f0, %f2
	fone %f2
	fmovdpos %icc, %f0, %f2
	fone %f2
	fmovdneg %icc, %f0, %f2
	fone %f2
	fmovdvc %icc, %f0, %f2
	fone %f2
	fmovdvs %icc, %f0, %f2
	fone %f2
	fmovda %xcc, %f0, %f2
	fone %f2
	fmovdn %xcc, %f0, %f2
	fone %f2
	fmovdne %xcc, %f0, %f2
	fone %f2
	fmovde %xcc, %f0, %f2
	fone %f2
	fmovdg %xcc, %f0, %f2
	fone %f2
	fmovdle %xcc, %f0, %f2
	fone %f2
	fmovdge %xcc, %f0, %f2
	fone %f2
	fmovdl %xcc, %f0, %f2
	fone %f2
	fmovdgu %xcc, %f0, %f2
	fone %f2
	fmovdleu %xcc, %f0, %f2
	fone %f2
	fmovdcc %xcc, %f0, %f2
	fone %f2
	fmovdcs %xcc, %f0, %f2
	fone %f2
	fmovdpos %xcc, %f0, %f2
	fone %f2
	fmovdneg %xcc, %f0, %f2
	fone %f2
	fmovdvc %xcc, %f0, %f2
	fone %f2
	fmovdvs %xcc, %f0, %f2
	fone %f2
	fmovsa %fcc0, %f0, %f2
	fone %f2
	fmovsn %fcc0, %f0, %f2
	fone %f2
	fmovsu %fcc0, %f0, %f2
	fone %f2
	fmovsg %fcc0, %f0, %f2
	fone %f2
	fmovsug %fcc0, %f0, %f2
	fone %f2
	fmovsl %fcc0, %f0, %f2
	fone %f2
	fmovsul %fcc0, %f0, %f2
	fone %f2
	fmovslg %fcc0, %f0, %f2
	fone %f2
	fmovsne %fcc0, %f0, %f2
	fone %f2
	fmovse %fcc0, %f0, %f2
	fone %f2
	fmovsue %fcc0, %f0, %f2
	fone %f2
	fmovsge %fcc0, %f0, %f2
	fone %f2
	fmovsuge %fcc0, %f0, %f2
	fone %f2
	fmovsle %fcc0, %f0, %f2
	fone %f2
	fmovsule %fcc0, %f0, %f2
	fone %f2
	fmovso %fcc0, %f0, %f2
	fone %f2
	fmovsa %fcc1, %f0, %f2
	fone %f2
	fmovsn %fcc1, %f0, %f2
	fone %f2
	fmovsu %fcc1, %f0, %f2
	fone %f2
	fmovsg %fcc1, %f0, %f2
	fone %f2
	fmovsug %fcc1, %f0, %f2
	fone %f2
	fmovsl %fcc1, %f0, %f2
	fone %f2
	fmovsul %fcc1, %f0, %f2
	fone %f2
	fmovslg %fcc1, %f0, %f2
	fone %f2
	fmovsne %fcc1, %f0, %f2
	fone %f2
	fmovse %fcc1, %f0, %f2
	fone %f2
	fmovsue %fcc1, %f0, %f2
	fone %f2
	fmovsge %fcc1, %f0, %f2
	fone %f2
	fmovsuge %fcc1, %f0, %f2
	fone %f2
	fmovsle %fcc1, %f0, %f2
	fone %f2
	fmovsule %fcc1, %f0, %f2
	fone %f2
	fmovso %fcc1, %f0, %f2
	fone %f2
	fmovda %fcc0, %f0, %f2
	fone %f2
	fmovdn %fcc0, %f0, %f2
	fone %f2
	fmovdu %fcc0, %f0, %f2
	fone %f2
	fmovdg %fcc0, %f0, %f2
	fone %f2
	fmovdug %fcc0, %f0, %f2
	fone %f2
	fmovdl %fcc0, %f0, %f2
	fone %f2
	fmovdul %fcc0, %f0, %f2
	fone %f2
	fmovdlg %fcc0, %f0, %f2
	fone %f2
	fmovdne %fcc0, %f0, %f2
	fone %f2
	fmovde %fcc0, %f0, %f2
	fone %f2
	fmovdue %fcc0, %f0, %f2
	fone %f2
	fmovdge %fcc0, %f0, %f2
	fone %f2
	fmovduge %fcc0, %f0, %f2
	fone %f2
	fmovdle %fcc0, %f0, %f2
	fone %f2
	fmovdule %fcc0, %f0, %f2
	fone %f2
	fmovdo %fcc0, %f0, %f2
	fone %f2
	fmovda %fcc1, %f0, %f2
	fone %f2
	fmovdn %fcc1, %f0, %f2
	fone %f2
	fmovdu %fcc1, %f0, %f2
	fone %f2
	fmovdg %fcc1, %f0, %f2
	fone %f2
	fmovdug %fcc1, %f0, %f2
	fone %f2
	fmovdl %fcc1, %f0, %f2
	fone %f2
	fmovdul %fcc1, %f0, %f2
	fone %f2
	fmovdlg %fcc1, %f0, %f2
	fone %f2
	fmovdne %fcc1, %f0, %f2
	fone %f2
	fmovde %fcc1, %f0, %f2
	fone %f2
	fmovdue %fcc1, %f0, %f2
	fone %f2
	fmovdge %fcc1, %f0, %f2
	fone %f2
	fmovduge %fcc1, %f0, %f2
	fone %f2
	fmovdle %fcc1, %f0, %f2
	fone %f2
	fmovdule %fcc1, %f0, %f2
	fone %f2
	fmovdo %fcc1, %f0, %f2
	fone %f2
	ret
	restore
! end proc fmov

proc_priv:
	save %sp, -336, %sp

	mov 0, %i0
	rdpr %tpc, %i1
	wrpr %i0, %i1, %tpc
	wrpr %i1,   0, %tpc
	mov 0, %i0
	rdpr %tnpc, %i1
	wrpr %i0, %i1, %tnpc
	wrpr %i1,   0, %tnpc
	mov 0, %i0
	rdpr %tstate, %i1
	wrpr %i0, %i1, %tstate
	wrpr %i1,   0, %tstate
	mov 0, %i0
	rdpr %tt, %i1
	wrpr %i0, %i1, %tt
	wrpr %i1,   0, %tt
	mov 0, %i0
	rdpr %tick, %i1
	wrpr %i0, %i1, %tick
	wrpr %i1,   0, %tick
	mov 0, %i0
	rdpr %tba, %i1
	wrpr %i0, %i1, %tba
	wrpr %i1,   0, %tba
	mov 0, %i0
	rdpr %pstate, %i1
	wrpr %i0, %i1, %pstate
	wrpr %i1,   0, %pstate
	mov 0, %i0
	rdpr %tl, %i1
	wrpr %i0, %i1, %tl
	wrpr %i1,   0, %tl
	mov 0, %i0
	rdpr %pil, %i1
	wrpr %i0, %i1, %pil
	wrpr %i1,   0, %pil
	mov 0, %i0
	rdpr %cwp, %i1
	wrpr %i0, %i1, %cwp
	wrpr %i1,   0, %cwp
	mov 0, %i0
	rdpr %cansave, %i1
	wrpr %i0, %i1, %cansave
	wrpr %i1,   0, %cansave
	mov 0, %i0
	rdpr %canrestore, %i1
	wrpr %i0, %i1, %canrestore
	wrpr %i1,   0, %canrestore
	mov 0, %i0
	rdpr %cleanwin, %i1
	wrpr %i0, %i1, %cleanwin
	wrpr %i1,   0, %cleanwin
	mov 0, %i0
	rdpr %otherwin, %i1
	wrpr %i0, %i1, %otherwin
	wrpr %i1,   0, %otherwin
	mov 0, %i0
	rdpr %wstate, %i1
	wrpr %i0, %i1, %wstate
	wrpr %i1,   0, %wstate
	mov 0, %i0

	ret
	restore
! end proc priv
	
proc_new:
	save %sp, -336, %sp
	ret
	restore
! end proc new	

.global main
.align 4
.proc 4
main:
!	save %sp, -336, %sp

	add %i0, %i1, %o0
	add %i0, 39, %o1
	addc %i0, %i1, %o0
	addc %i0, 39, %o1
	addccc %i0, %i1, %o0
	addccc %i0, 39, %o1
	addcc %i0, %i1, %o0
	addcc %i0, 39, %o1
	alignaddr %i0, %i1, %o3
	alignaddrl %i0, %i1, %o3
	and %i0, %i1, %o0
	andn %i0, %i1, %o0
	andncc %i0, %i1, %o0
	andncc %i0, 39, %o1
	andn %i0, 39, %o1
	andcc %i0, %i1, %o0
	andcc %i0, 39, %o1
	and %i0, 39, %o1
	array16 %i0, %g0, %o0
	array32 %i0, %g0, %o0
	array8 %i0, %g0, %o0

	call proc_bcc
	! delay slot
	nop
	call proc_fbcc
	nop
	call proc_cmov
	nop
	call proc_fmov
	nop
	
	mov 0, %o0
	set string, %o1
	! length of string == 14
	mov 14, %o2    
        ! 4 is write sys call: see /usr/include/sys/syscall.h
	mov 4, %g1
	ta 8

	! exit(0) 1 is exit syscall
	mov 0, %o0
	mov 1, %g1
	ta 8

	ret
	restore

.seg "data"
.align 4
.proc 4
string:
.ascii "Hello, World!\n"

