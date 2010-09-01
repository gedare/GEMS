
import string

condl = [ "a", "n", "ne", "e", "g", "le", "ge", "l", "gu", "leu", "cc", "cs", "pos", "neg", "vc", "vs" ]

label = 0
for cond in condl:
    print "	b"+cond+" br_%d" % label
    print "	add %o5, 1, %o5"
    print "br_%d:" % label
    label = label + 1

for cond in condl:
    print "	b"+cond+",a br_%d" % label
    print "	add %o5, 1, %o5"
    print "br_%d:" % label
    label = label + 1

for cond in condl:
    print "	b"+cond+",pn %%icc, br_%d" % label
    print "	add %o5, 1, %o5"
    print "br_%d:" % label
    label = label + 1

for cond in condl:
    print "	b"+cond+",pn %%xcc, br_%d" % label
    print "	add %o5, 1, %o5"
    print "br_%d:" % label
    label = label + 1    

for cond in condl:
    print "	b"+cond+",pt %%icc, br_%d" % label
    print "	add %o5, 1, %o5"
    print "br_%d:" % label
    label = label + 1

for cond in condl:
    print "	b"+cond+",pt %%xcc, br_%d" % label
    print "	add %o5, 1, %o5"
    print "br_%d:" % label
    label = label + 1    

for cond in condl:
    print "	b"+cond+",a,pn %%icc, br_%d" % label
    print "	add %o5, 1, %o5"
    print "br_%d:" % label
    label = label + 1

for cond in condl:
    print "	b"+cond+",a,pn %%xcc, br_%d" % label
    print "	add %o5, 1, %o5"
    print "br_%d:" % label
    label = label + 1    

for cond in condl:
    print "	b"+cond+",a,pt %%icc, br_%d" % label
    print "	add %o5, 1, %o5"
    print "br_%d:" % label
    label = label + 1

for cond in condl:
    print "	b"+cond+",a,pt %%xcc, br_%d" % label
    print "	add %o5, 1, %o5"
    print "br_%d:" % label
    label = label + 1    

