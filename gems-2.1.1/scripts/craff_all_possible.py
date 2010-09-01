#!/s/std/bin/python

import os, sys, re
sys.path.append("%s/../condor/gen-scripts" % os.path.dirname(os.path.abspath(sys.argv[0])))
import tools

simics_path = "%s/../simics/"%os.path.dirname(os.path.abspath(sys.argv[0]))
#craff_path = "%s/x86-linux/bin/craff"%simics_path
craff_path = "%s/v9-sol8-64/bin/craff"%simics_path

for i in sys.argv[1:]:
  output = tools.run_command("%s -n %s" % (craff_path, i), echo = 0, throw_exception = 0)
  for line in output.split("\n"):
    if re.search("Compression: 0", line):
      print "Craffing %s ..." % i,
      sys.stdout.flush()
      tools.run_command("%s -o craff.out %s" % (craff_path, i), echo=0)
      tools.run_command("mv craff.out %s" % i, echo=0)
      print "done."

