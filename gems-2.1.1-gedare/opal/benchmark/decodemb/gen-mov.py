
import string

condi = [ "a", "n", "ne", "e", "g", "le", "ge", "l", "gu", "leu", "cc", "cs", "pos", "neg", "vc", "vs" ]
condf = [ "a", "n", "u", "g", "ug", "l", "ul", "lg", "ne", "e", "ue", "ge", "uge", "le", "ule", "o" ]

for cond in condi:
    print "	mov"+cond+" %icc, 1, %i0"
    print "	set 0, %i0"

for cond in condi:
    print "	mov"+cond+" %icc, %g0, %i0"
    print "	set 1, %i0"

for cond in condi:
    print "	mov"+cond+" %xcc, 1, %i0"
    print "	set 0, %i0"

for cond in condi:
    print "	mov"+cond+" %xcc, %g0, %i0"
    print "	set 1, %i0"

for cond in condf:
    print "	mov"+cond+" %fcc0, 1, %i0"
    print "	set 0, %i0"

for cond in condf:
    print "	mov"+cond+" %fcc1, %g0, %i0"
    print "	set 1, %i0"

for cond in condf:
    print "	mov"+cond+" %fcc2, 1, %i0"
    print "	set 0, %i0"

for cond in condf:
    print "	mov"+cond+" %fcc3, %g0, %i0"
    print "	set 1, %i0"
