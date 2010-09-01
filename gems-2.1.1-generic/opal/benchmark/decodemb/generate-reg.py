
import string

condl = [ "z", "lez", "lz", "nz", "gz", "gez" ]

label = 0
for cond in condl:
    print "	br"+cond+",pt %%i0, brr_%d" % label
    print "	add %o5, 1, %o5"
    print "brr_%d:" % label
    label = label + 1

for cond in condl:
    print "	br"+cond+",pn %%i0, brr_%d" % label
    print "	add %o5, 1, %o5"
    print "brr_%d:" % label
    label = label + 1

for cond in condl:
    print "	br"+cond+",a,pt %%i0, brr_%d" % label
    print "	add %o5, 1, %o5"
    print "brr_%d:" % label
    label = label + 1

for cond in condl:
    print "	br"+cond+",a,pn %%i0, brr_%d" % label
    print "	add %o5, 1, %o5"
    print "brr_%d:" % label
    label = label + 1    
