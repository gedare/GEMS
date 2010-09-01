
import string

pregs = [ "%tpc", "%tnpc", "%tstate", "%tt", "%tick", "%tba", "%pstate", "%tl", "%pil", "%cwp", "%cansave", "%canrestore", "%cleanwin", "%otherwin", "%wstate"]

for reg in pregs:
    print "	mov 0, %i0"
    print "	rdpr "+reg+", %i1"
    print "	wrpr %i0, %i1, "+reg
    print "	wrpr %i1,   0, "+reg
