#!/s/std/bin/env python

import string, sys, os, re

if len(sys.argv) != 2:
    sys.stderr.write("format: remove-tabs-from-single-file.py <filename>")
    sys.exit(1)

filename = sys.argv[1]

if re.compile(".*\.(c|C|h|H|l|y|cpp|sm|slicc)$").match(filename):
    content = open(filename, "r").read()
    # only modify the file if it has tabs
    if string.count(content, "\t") > 0:
        print filename
        output = os.popen("/s/BitKeeper/bin/bk edit %s" % filename)
        print output
        content = open(filename, "r").read()
        content = string.expandtabs(content)
        print "here"
        open(filename, "w").write(content)
