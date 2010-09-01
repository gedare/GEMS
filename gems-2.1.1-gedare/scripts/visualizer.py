#!/s/std/bin/python
import sys, string
import os, glob, re

#xact_reset = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0] # 0 - NONXACT
                                                              # 1 - XACT

xact_reset = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
              0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
              ]
num_processors = 32
                                                              
for arg in sys.argv:
  in_file_name = arg
in_file = open(in_file_name, 'r')
out_file = open('temp', 'w')

for line in in_file.readlines()[::-1]:
  line = line.rstrip()
  fields = line.split(' ')
  outline = ''

  for i in range(num_processors):
    if (fields[i] == '%'):
      xact_reset[i] = 0
      outline += '  '
    elif ((fields[i] == '$') and (xact_reset[i] == 1)):
      outline += '. '
    elif (fields[i] == 'X'):
      xact_reset[i] = 1
      outline += fields[i] + ' '
    else:  
      outline += fields[i] + ' '
  outline += '   ' + fields[len(fields) - 1] + '\n'
  out_file.write(outline)

out_file.close()
out_file = open('temp', 'r')
for line in out_file.readlines()[::-1]:
  print line,      
                         
