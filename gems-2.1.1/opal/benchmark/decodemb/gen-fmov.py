
import string

condi = [ "a", "n", "ne", "e", "g", "le", "ge", "l", "gu", "leu", "cc", "cs", "pos", "neg", "vc", "vs" ]
condf = [ "a", "n", "u", "g", "ug", "l", "ul", "lg", "ne", "e", "ue", "ge", "uge", "le", "ule", "o" ]

print "	fzero %f0"

for cond in condi:
    print "	fmovs"+cond+" %icc, %f0, %f2"
    print "	fone %f2"

for cond in condi:
    print "	fmovs"+cond+" %xcc, %f0, %f2"
    print "	fone %f2"

for cond in condi:
    print "	fmovd"+cond+" %icc, %f0, %f2"
    print "	fone %f2"

for cond in condi:
    print "	fmovd"+cond+" %xcc, %f0, %f2"
    print "	fone %f2"


for cond in condf:
    print "	fmovs"+cond+" %fcc0, %f0, %f2"
    print "	fone %f2"

for cond in condf:
    print "	fmovs"+cond+" %fcc1, %f0, %f2"
    print "	fone %f2"

for cond in condf:
    print "	fmovd"+cond+" %fcc0, %f0, %f2"
    print "	fone %f2"

for cond in condf:
    print "	fmovd"+cond+" %fcc1, %f0, %f2"
    print "	fone %f2"
