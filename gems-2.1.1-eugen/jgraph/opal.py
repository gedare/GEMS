#!/s/std/bin/python

import glob, math, mfgraph, os, re, string, sys



##################################
# Graphing Functions
##################################

def make_bars(fields, data):
    if(len(fields) <= 0):
        return

    map = {}
    for field in fields:
        map[field] = {}

    for tuple in data:
        fCnt = 1
        for field in fields:
            #print field
            key = tuple[0]
            #print "key: %d" % key
            if map[field].has_key(key):
                dummy = 0
            else:
                map[field][key] = []

            map[field][key].append(tuple[fCnt])
            fCnt += 1

    fField = fields[0]

    tuple_list = []
    for key in map[fField].keys():
        tuple = [key]
        for field in fields:
            avg_val = mfgraph.average(map[field][key])
            if avg_val < 0: print "Error: entering negative avg val for %s at %dp" % (field, key)
            tuple.append(avg_val)
        #print tuple
        #stacked_tuple = [key] + mfgraph.stack_bars(tuple)
        tuple_list.append(tuple)

    return tuple_list

def make_stacked(tuple_list):
    stacked_list = []
    for tuple in tuple_list:
        stacked_tuple = [tuple[0]] + mfgraph.stack_bars(tuple[1:])
        stacked_list.append(stacked_tuple)
    return stacked_list

def draw_std_line(jgraphs, title, xlabel, ylabel, lines):
    jgraphs.append(mfgraph.line_graph(lines,
                                      title = title,
                                      xlabel = xlabel,
                                      ylabel = ylabel,
                                      xsize = 6,
                                      ysize = 4.5,
                                      ))

def get_linear(data):

    if data == []:
        return []
    
    linear = []
    base = data[1][1]/data[1][0]
    for point in data[1:]:
        l_point = [point[0], point[0]*base] 
        linear.append(l_point)
    return linear

##################################
# Opal Data Collection Functions
##################################

def get_cpi(output_file):
    print output_file
    grep_lines = mfgraph.grep(output_file, ".*Instruction per cycle.*")
    print grep_lines
    line = string.split(grep_lines[0])
    ipc = float(line[4])
    cpi = 1.0/ipc
    return cpi

def get_issue_width(output_file):
    grep_lines = mfgraph.grep(output_file, ".*IWINDOW_WIN_SIZE.*")
    line = string.split(grep_lines[0])
    width = int(line[2])
    return width

def get_reasons_for_fetch_stall(output_file):
    grep_lines = mfgraph.grep(output_file, ".*Fetch i-cache miss.*")
    line = string.split(grep_lines[0])
    value_str = string.split(line[6], "%")
    icache_miss = float(value_str[0])

    grep_lines = mfgraph.grep(output_file, ".*Fetch squash.*")
    line = string.split(grep_lines[0])
    value_str = string.split(line[5], "%")
    squash = float(value_str[0])

    grep_lines = mfgraph.grep(output_file, ".*Fetch I-TLB miss.*")
    line = string.split(grep_lines[0])
    value_str = string.split(line[6], "%")
    itlb_miss = float(value_str[0])

    grep_lines = mfgraph.grep(output_file, ".*Window Full.*")
    line = string.split(grep_lines[0])
    value_str = string.split(line[5], "%")
    window_full = float(value_str[0])

    grep_lines = mfgraph.grep(output_file, ".*Fetch Barrier.*")
    line = string.split(grep_lines[0])
    value_str = string.split(line[5], "%")
    barrier = float(value_str[0])

    width = get_issue_width(output_file)
    return [width, icache_miss, squash, itlb_miss, window_full, barrier]

def get_reasons_for_retire_stall(output_file):
    grep_lines = mfgraph.grep(output_file, ".*Retire Updating.*")
    line = string.split(grep_lines[0])
    value_str = string.split(line[5], "%")
    updating = float(value_str[0])

    grep_lines = mfgraph.grep(output_file, ".*Retire Squash.*")
    line = string.split(grep_lines[0])
    value_str = string.split(line[5], "%")
    squash = float(value_str[0])

    grep_lines = mfgraph.grep(output_file, ".*Retire Limit.*")
    line = string.split(grep_lines[0])
    value_str = string.split(line[5], "%")
    limit = float(value_str[0])

    width = get_issue_width(output_file)
    return [width, updating, squash, limit]

def extract_value(filename, key, position):
    grep_lines = mfgraph.grep(filename, ".*" + key + ".*")
    line = string.split(grep_lines[0])
    return line[position]

def get_branch_prediction_rate_data(branch_type, predictor_type):
    data = []
    files = glob.glob("%s/*.opal" % results_dir)
    for file in files:
        grep_lines = mfgraph.grep(file, ".*Configuration File named.*")
        if not re.match(".*branchpredictors.*", grep_lines[0]):
            #print "Non-branch pred config: " + grep_lines[0]
            continue

        grep_lines = mfgraph.grep(file, "BRANCHPRED_TYPE.*")
        if not re.match(".*" + predictor_type + ".*", grep_lines[0]):
            #print "branch pred doesn't match (%s)" % grep_lines[0]
            continue

        pht_bits = int(extract_value(file, "BRANCHPRED_PHT_BITS", 2))
        grep_lines = mfgraph.grep(file, ".*%s.*" % branch_type)
        
        line = string.split(grep_lines[2]) # use total count
        value_str = string.split(line[7], "%")
        pct_right = float(value_str[0])

        data.append([pht_bits, pct_right])
    return data
        

##################################
# Graph Generation Functions
##################################

def generate_cpi(jgraphs, results_dir):
    data = []
    files = glob.glob("%s/*.opal" % results_dir)
    #print files
    for file in files:
        print file
        cpi = get_cpi(file)
        width = get_issue_width(file)
        data.append([width, cpi])
    #data.sort()
    #cpi_data = mfgraph.merge_data(group_title = "CPI", data = data)

    cpi_bars = make_bars(fields=["CPI"], data=data)
    cpi_bars.sort()
    cpi_bars.insert(0, "CPI")
    graph_bars = [cpi_bars]

    #draw_std_line(jgraphs,
    #"CPI vs. Instruction Window Size",
    #"Issue Window Size",
    #"CPI",
    #lines)
    #print graph_bars
    
    jgraphs.append(mfgraph.stacked_bar_graph(graph_bars,
                                          title = "CPI vs Window Size",
                                          bar_segment_labels = ["CPI"],
                                          ylabel = "CPI",
                                          xsize = 4.0,
                                          ysize = 1.778,
                                          ))


def generate_reasons_for_fetch_stall(jgraphs, results_dir):
    data = []
    files = glob.glob("%s/*.opal" % results_dir)
    #print files
    for file in files:
        tuple = get_reasons_for_fetch_stall(file)
        data.append(tuple)
    fetch_data = make_bars(fields=["I-Cache Miss", "Squash", "I-TLB Miss", "Window Full", "Fetch Barrier"],
                               data=data)
    fetch_bars = make_stacked(fetch_data)
    fetch_bars.sort()
    fetch_bars.insert(0, "Window Width")
    graph_bars = [fetch_bars]
    jgraphs.append(mfgraph.stacked_bar_graph(graph_bars,
                                          title = "Reasons for Fetch Stall vs. Window Width",
                                          bar_segment_labels = ["I-Cache Miss", "Squash", "I-TLB Miss", "Window Full", "Fetch Barrier"],
                                          ylabel = "%",
                                          xsize = 4.0,
                                          ysize = 1.778,
                                          ))

def generate_reasons_for_retire_stall(jgraphs, results_dir):
    data = []
    files = glob.glob("%s/*.opal" % results_dir)
    #print files
    for file in files:
        tuple = get_reasons_for_retire_stall(file)
        data.append(tuple)
    retire_data = make_bars(fields=["Updating", "Squash", "Limit"],
                               data=data)
    retire_bars = make_stacked(retire_data)
    retire_bars.sort()
    retire_bars.insert(0, "JBB")
    graph_bars = [retire_bars]
    jgraphs.append(mfgraph.stacked_bar_graph(graph_bars,
                                          title = "Reasons for Retire Stall vs. Window Width",
                                          bar_segment_labels = ["Updating", "Squash", "Limit"],
                                          ylabel = "%",
                                          xsize = 4.0,
                                          ysize = 1.778,
                                          ))

def generate_pcond_branch_prediction_rate(jgraphs, results_dir):
    gshare_data = mfgraph.merge_data(group_title="GSHARE",
                                     data=get_branch_prediction_rate_data("PCOND", "GSHARE"))
    agree_data = mfgraph.merge_data(group_title="AGREE",
                                    data=get_branch_prediction_rate_data("PCOND", "AGREE"))
    yags_data = mfgraph.merge_data(group_title="YAGS",
                                   data=get_branch_prediction_rate_data("PCOND", "YAGS"))
    print gshare_data
    lines = [gshare_data, agree_data, yags_data]

    jgraphs.append(mfgraph.line_graph(lines,
                                      title = "PCOND Branch Prediction Accuracy",
                                      xlabel = "PHT Bits",
                                      ylabel = "% Correct",
                                      xsize = 6,
                                      ysize = 4.5,
                                      ymin = 80,
                                      ymax = 100
                                      ))
def generate_indirect_branch_prediction_rate(jgraphs, results_dir):
    gshare_data = mfgraph.merge_data(group_title="GSHARE",
                                     data=get_branch_prediction_rate_data("INDIRE", "GSHARE"))
    agree_data = mfgraph.merge_data(group_title="AGREE",
                                    data=get_branch_prediction_rate_data("INDIRE", "AGREE"))
    yags_data = mfgraph.merge_data(group_title="YAGS",
                                   data=get_branch_prediction_rate_data("INDIRE", "YAGS"))
    print gshare_data
    lines = [gshare_data, agree_data, yags_data]

    jgraphs.append(mfgraph.line_graph(lines,
                                      title = "INDIRECT Branch Prediction Accuracy",
                                      xlabel = "PHT Bits",
                                      ylabel = "% Correct",
                                      xsize = 6,
                                      ysize = 4.5,
                                      ymin = 80,
                                      ymax = 100
                                      ))
    #draw_std_line(jgraphs, "Branch Prediction Accuracy", "PHT Bits", "% Correct", lines)

    

jgraph_input = []
#results_dir = "/p/multifacet/projects/ecperf/multifacet/workloads/ecperf/"
opal_output_dir = "/p/multifacet/projects/ecperf/multifacet/condor/results/"

if len(sys.argv) == 2:
    run_dir = sys.argv[1]
    
    results_dir = opal_output_dir + run_dir
    print results_dir
    generate_cpi(jgraph_input, results_dir)
    generate_reasons_for_fetch_stall(jgraph_input, results_dir)
    generate_reasons_for_retire_stall(jgraph_input, results_dir)
    generate_pcond_branch_prediction_rate(jgraph_input, results_dir)
    
    print "Graph Inputs Ready...Running Jgraph."
    mfgraph.run_jgraph("newpage\n".join(jgraph_input), "opal-" + run_dir)
else:
    print "usage: opal.py <benchmark>"    


