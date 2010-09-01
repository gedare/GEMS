#!/s/std/bin/env python

import string, sys, os, re

paths = ("protocols",
         "ruby",
         "common",
         "slicc",
         "opal",
         "template",
         "jgraph",
         "condor/gen-scripts")

print "Note: this script should be executed at the root of your cvs tree"

def walk(path):
    if os.path.isfile(path):
        return [path]
    elif os.path.isdir(path):
        filelist = []
        for file in os.listdir(path):
            filelist += walk(os.path.join(path, file))
        return filelist
    else:
        return []

for path in paths:
    for filename in walk(path):
        if re.compile(".*\.(c|C|h|H|l|y|cpp|sm|slicc)$").match(filename) and not re.search("SCCS", filename):
            content = open(filename, "r").read()
            # only modify the file if it has tabs
            if string.count(content, "\t") > 0:
                print filename
                output = os.popen("/s/BitKeeper/bin/bk edit %s" % filename)
                print output
                output = os.popen("/s/BitKeeper/bin/bk edit %s" % filename)
                print output
                content = string.expandtabs(content)
                open(filename, "w").write(content)
