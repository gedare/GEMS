
import string

condl = [ "a", "n", "u", "g", "ug", "l", "ul", "lg", "ne", "e", "ue", "ge", "uge", "le", "ule", "o" ]

label = 0
for cond in condl:
    print "	fb"+cond+" fbr_%d" % label
    print "	add %o5, 1, %o5"
    print "fbr_%d:" % label
    label = label + 1

for cond in condl:
    print "	fb"+cond+",a fbr_%d" % label
    print "	add %o5, 1, %o5"
    print "fbr_%d:" % label
    label = label + 1

for cond in condl:
    print "	fb"+cond+",pn %%fcc0, fbr_%d" % label
    print "	add %o5, 1, %o5"
    print "fbr_%d:" % label
    label = label + 1

for cond in condl:
    print "	fb"+cond+",pt %%fcc1, fbr_%d" % label
    print "	add %o5, 1, %o5"
    print "fbr_%d:" % label
    label = label + 1

for cond in condl:
    print "	fb"+cond+",a,pn %%fcc2, fbr_%d" % label
    print "	add %o5, 1, %o5"
    print "fbr_%d:" % label
    label = label + 1

for cond in condl:
    print "	fb"+cond+",a,pt %%fcc3, fbr_%d" % label
    print "	add %o5, 1, %o5"
    print "fbr_%d:" % label
    label = label + 1
