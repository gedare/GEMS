#!/s/std/bin/python
import sys, string
import os, glob, re

my_pattern = re.compile('PC=(0x[0-9|a-f]*)')
pc_val = ''
symbol_table = []
file_in = file("../../scripts/symbol-table.inf", "r") 
for line in file_in:
  line = line.rstrip()
  fields = line.split(' ')
  symbol_table.append((fields[0], fields[1], fields[2]))

for value in sys.stdin:
  val = value.rstrip()
  pc_m = my_pattern.search(val)
  if (pc_m):
    pc_val = pc_m.group(1)
    int_val = int(pc_val, 0x)
  else:
    print val      
    continue     

  matching_str = None
  for (base, limit, name) in symbol_table:
    base_val = int(base, 0x)
    limit_val = int(limit, 0x)
    if (int_val >= base_val and int_val < (base_val+limit_val)):
      #print pc_val, base, limit, name
      matching_str = name + '+' + hex(int_val - base_val)
    if (int_val < base_val):
      break;          
  if (matching_str):
    new_line = my_pattern.sub("PC="+matching_str, val)         
    print new_line   
  else:
    print val            
              
              
                    
  
	
