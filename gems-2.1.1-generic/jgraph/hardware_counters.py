import glob, mfgraph,  os, re, string, sys 

######################################
# Accessors
######################################

def get_l2_misses(cpustat_file):
    l2_misses = 0
    grep_lines = mfgraph.grep(cpustat_file, ".+total.+pic0=EC_ref,pic1=EC_hit,sys")
    count = 0
    for g_line in grep_lines:
        line = string.split(g_line)
        l2_misses += (long(line[3]) - long(line[4]))
        count += 1
    if count == 0:
        return -1
    else:
        return l2_misses/count

def get_l2_miss_ratio(cpustat_file):
    ratio = 0.0
    grep_lines = mfgraph.grep(cpustat_file, ".+total.+pic0=EC_ref,pic1=EC_hit,sys")
    for g_line in grep_lines:
        line = string.split(g_line)
        refs = float(line[3])
        hits = float(line[4])
        ratio = (refs - hits)/refs
        #print "ratio: %f" % ratio
    return ratio
    
def get_c2c(cpustat_file):
     grep_lines = mfgraph.grep(cpustat_file, ".+total.+pic0=Instr_cnt,pic1=EC_snoop_cb,sys")
     c2c = 0
     for g_line in grep_lines:
         line = string.split(g_line)
         c2c += long(line[5])
     return c2c

def get_instructions(cpustat_file):
    grep_lines = mfgraph.grep(cpustat_file, ".+total.+pic0=Cycle_cnt,pic1=Instr_cnt")
    count = 0
    instructions = 0
    for g_line in grep_lines:
        #print "line: " + g_line
        line = string.split(g_line)
        instructions += long(line[4])
        count += 1

    if count == 0:
        return -1
    else:
        return instructions/count


###################################
# cpustat Summary Data
###################################

def cpustat_get_CPI(filename, mode):
    # use mode = {USER, KERNEL, COMBINED}
    grep_lines = mfgraph.grep(filename, mode + "_CPI")
    line = string.split(grep_lines[0])
    value = float(line[1])
    return value

def cpustat_get_data_stall_rate(filename, mode):
    grep_lines = mfgraph.grep(filename, mode + "_DATASTR")
    line = string.split(grep_lines[0])
    value = float(line[1])
    return value

def cpustat_get_branch_stall_rate(filename, mode):
    grep_lines = mfgraph.grep(filename, mode + "_BRANCHSTR")
    line = string.split(grep_lines[0])
    value = float(line[1])
    return value

def cpustat_get_inst_stall_rate(filename, mode):
    grep_lines = mfgraph.grep(filename, mode + "_INSTRSTR")
    line = string.split(grep_lines[0])
    value = float(line[1])
    return value

def cpustat_get_data_stall_rate(filename, mode):
    grep_lines = mfgraph.grep(filename, mode + "_DATASTR")
    line = string.split(grep_lines[0])
    value = float(line[1])
    return value

def cpustat_get_ctoc(filename, mode):
    #print "in get_ctoc, filename is %s" % filename
    grep_lines = mfgraph.grep(filename, mode + "_CTOC_RATIO")
    line = string.split(grep_lines[0])
    value = float(line[1])
    return value

def cpustat_get_ctoc_per_inst(filename, mode):
    grep_lines = mfgraph.grep(filename, mode + "_CTOC_PER_INST")
    line = string.split(grep_lines[0])
    value = float(line[1])
    return value

def cpustat_get_storebuf(filename, mode):
    grep_lines = mfgraph.grep(filename, mode + "_storebuf")
    line = string.split(grep_lines[0])
    value = float(line[1])
    return value

def cpustat_get_load_use_RAW(filename, mode):
    grep_lines = mfgraph.grep(filename, mode + "_load_use_RAW")
    line = string.split(grep_lines[0])
    value = float(line[1])
    return value

def cpustat_get_load_use(filename, mode):
    grep_lines = mfgraph.grep(filename, mode + "_load_use")
    line = string.split(grep_lines[1])
    value = float(line[1])
    return value

def cpustat_get_snoop_cb(filename, mode):
    grep_lines = mfgraph.grep(filename, mode + "_snoop_cb")
    line = string.split(grep_lines[0])
    value = float(line[1])
    return value

def cpustat_get_EC_rd_hit(filename, mode):
    grep_lines = mfgraph.grep(filename, mode + "_EC_rd_hit")
    line = string.split(grep_lines[0])
    value = float(line[1])
    return value

def cpustat_get_EC_misses(filename, mode):
    grep_lines = mfgraph.grep(filename, mode + "_EC_misses")
    line = string.split(grep_lines[0])
    value = float(line[1])
    return value

def cpustat_get_IC_misses(filename, mode):
    grep_lines = mfgraph.grep(filename, mode + "_IC_misses")
    line = string.split(grep_lines[0])
    value = float(line[1])
    return value

def cpustat_get_EC_rd_misses(filename, mode):
    grep_lines = mfgraph.grep(filename, mode + "_EC_rd_misses")
    line = string.split(grep_lines[0])
    value = float(line[1])
    return value

###################################
# Generic Get Value Function
###################################


def cpustat_get(filename, mode, parameter):
    grep_lines = mfgraph.grep(filename, mode + "_" + parameter)
    line = string.split(grep_lines[0])
    value = line[1]
    return value
