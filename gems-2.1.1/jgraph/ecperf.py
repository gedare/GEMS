#!/s/std/bin/python

import glob, hardware_counters, math, mfgraph, os, re, string, sys

def ecperf_get_num_procs(directory):
    filename = directory + "/config.summary"
    if os.access(filename, os.F_OK):
        grep_lines = mfgraph.grep(filename, "Processors")
        line = string.split(grep_lines[0])
        procs = int(line[1])
        return procs
    else:
        return 0

def ecperf_get_tx_rate(directory):
    filename = directory + "/config.summary"
    grep_lines = mfgraph.grep(filename, "Tx")
    line = string.split(grep_lines[0])
    rate = int(line[2])
    return rate

def ecperf_get_stdystate(directory):
    filename = directory + "/ECperf.summary"
    if os.access(filename, os.F_OK):
        grep_lines = mfgraph.grep(filename, "stdyState")
        line = string.split(grep_lines[0])
        stdy_state = int(line[4])
        return stdy_state

    return 0

def ecperf_get_throughput(directory):
    filename = directory + "/ECperf.summary"
    if os.access(filename, os.F_OK):
        grep_lines = mfgraph.grep(filename, ".+Metric.+")
        line = string.split(grep_lines[0])
        throughput = float(line[3])
        return throughput
    else:
        return 0

def ecperf_get_transactions(directory):
    filename = directory + "/Orders.summary"
    if os.access(filename, os.F_OK):
        grep_lines = mfgraph.grep(filename, "Total number of transactions");
        line = string.split(grep_lines[0])
        orders = int(line[5])
    
        filename = directory + "/Mfg.summary"
        if os.access(filename, os.F_OK):
            grep_lines = mfgraph.grep(filename, "Total Number of WorkOrders Processed");
            line = string.split(grep_lines[0])
            work_ords = int(line[6])
            transactions = work_ords + orders
            print "transactions: %d" % transactions
            return transactions

    return 0

# Instructions/Transaction estimated from
# throughput, CPI and processor freq
def get_inst_per_xact(directory, mode):
    procs = ecperf_get_num_procs(directory)
    throughput = ecperf_get_throughput(directory)
    filename = directory + "/appserver_cpustat.summary"
    if os.access(filename, os.F_OK):
        print filename
        print mode
        cpi = float(hardware_counters.cpustat_get(filename, mode, "CPI"))
        inst_per_xact = ( 60 * cpu_freq * procs) / ( cpi * throughput )
        return inst_per_xact
    return 0

############################################
# GC/Memory Stats
############################################

def ecperf_get_gc_time(directory):
    filename = directory + "/gc.summary"
    grep_lines = mfgraph.grep(filename, ".+time.+");
    line = string.split(grep_lines[0])
    gc_time = float(line[3])
    return gc_time

def ecperf_get_avg_mem_use_after(directory):
    filename = directory + "/gc.summary"
    grep_lines = mfgraph.grep(filename, ".+after.+");
    if(grep_lines != []):
        line = string.split(grep_lines[0])
        mem_use = float(line[2])/1024
        return mem_use
    else:
        return 0
    

def ecperf_get_avg_mem_use_before(directory):
    filename = directory + "/gc.summary"
    grep_lines = mfgraph.grep(filename, ".+before.+");
    if(grep_lines != []):
        line = string.split(grep_lines[0])
        mem_use = float(line[2])/1024
        return mem_use
    else:
        return 0

######################################
# Major Graphs
######################################

def get_throughput_data(results_dir):
    data = []
    directories = glob.glob(results_dir + "/*")
    for dir in directories:
        procs = ecperf_get_num_procs(directory = dir)
        if not procs == 0:
            throughput = ecperf_get_throughput(dir)                
            tuple = [procs, throughput]
            data.append(tuple)
    return data

def get_tuning_data(results_dir):
    map = {}
    data = []

    directories = glob.glob(results_dir + "/*")
    for dir in directories:
        
        filename = dir + "/ECperf.summary"
        if os.access(filename, os.F_OK):
            grep_lines = mfgraph.grep(filename, ".+Metric.+")
            line = string.split(grep_lines[0])
            throughput = float(line[3])

            filename = dir + "/config.summary"
            grep_lines = mfgraph.grep(filename, "Thread Count")
            line = string.split(grep_lines[0])
            threads = int(line[2])

            grep_lines = mfgraph.grep(filename, "Tx Rate")
            line = string.split(grep_lines[0])
            tx_rate = int(line[2])

            procs = ecperf_get_num_procs(directory = dir)

            filename = dir + "/Mfg.summary"
            grep_lines = mfgraph.grep(filename, "ECPerf Requirement for 90")
            line = string.split(grep_lines[0])
            passed_mfg = line[6]

            filename = dir + "/Orders.summary"
            grep_lines = mfgraph.grep(filename, "ECPerf Requirement for 90")
            line = string.split(grep_lines[0])
            passed_ords = line[6]
            
            tuple = [procs,
                     throughput,
                     tx_rate,
                     threads,
                     passed_ords,
                     passed_mfg,
                     os.path.basename(dir)]
                     
            data.append(tuple) 
            

        #         key = threads
        #         if map.has_key(key):
        #             map[key].append(tuple)
        #         else:
        #             map[key] = []
        #             map[key].append(tuple)

    data.sort()

    print "Run Directory: %s" % results_dir
    print "procs score tx_rate threads ords mfg directory"
    for tuple in data:
        print "%d %.1f %d %d %s %s %s" % (tuple[0], tuple[1], tuple[2], tuple[3], tuple[4], tuple[5], tuple[6])
        
    return data


def get_throughput_by_input_data(tx_dir):
    data = []
    directories = glob.glob(tx_dir + "/*")
    for dir in directories:
        tx_rate = ecperf_get_tx_rate(directory = dir)
        throughput = ecperf_get_throughput(dir)      
        tuple = [tx_rate, throughput]
        #print tuple
        data.append(tuple)
    return data


def get_throughput_no_gc_data(results_dir):
    data_no_gc = []
    directories = glob.glob(results_dir + "/*")
    for dir in directories:
        procs = ecperf_get_num_procs(directory = dir)

        xacts = ecperf_get_transactions(dir)
        seconds = ecperf_get_stdystate(dir)
        gc_time = ecperf_get_gc_time(dir)
        if(gc_time < 0):
            print "**** ERROR NEGATIVE GC TIME ****"
        
        seconds_no_gc = seconds - gc_time
        throughput_no_gc = (xacts/seconds_no_gc)*60
        
        data_no_gc.append([procs, throughput_no_gc])
    return data_no_gc

def get_gc_time_data(results_dir):
    data = []
    directories = glob.glob(results_dir + "/*")
    for dir in directories:
        procs = ecperf_get_num_procs(directory = dir)
        gc_time = ecperf_get_gc_time(dir)
        total_time = ecperf_get_stdystate(dir)
        tuple = [procs, 100*gc_time/total_time]
        data.append(tuple)
    return data

def get_gc_time_per_xact_data(results_dir):
    data = []
    directories = glob.glob(results_dir + "/*")
    for dir in directories:
        procs = ecperf_get_num_procs(directory = dir)

        filename = dir + "/gc.summary"
        if os.access(filename, os.F_OK):
            grep_lines = mfgraph.grep(filename, ".+time.+");
            line = string.split(grep_lines[0])
            gc_time = float(line[3])
                
            transactions = ecperf_get_transactions(directory = dir)
            if transactions == 0:
                continue
            
            tuple = [procs, (gc_time/transactions)]

            data.append(tuple)
    return data

def get_l2_misses_per_xact_data(results_dir):
    data = []
    directories = glob.glob(results_dir + "/*")
    for dir in directories:
        #print "working in directory %s." % dir
        procs = ecperf_get_num_procs(directory = dir)

        filename = dir + "/appserver_cpustat.out"
        l2_misses = hardware_counters.get_l2_misses(cpustat_file = filename)
        
        transactions = ecperf_get_transactions(directory = dir)
        misses_per_trans = long(l2_misses/transactions)
                
        tuple = [procs, misses_per_trans]
        #print tuple
        data.append(tuple)
    return data

def get_l2_miss_rate_data(results_dir):
    data = []
    directories = glob.glob(results_dir + "/*")
    for dir in directories:
        procs = ecperf_get_num_procs(directory = dir)

        filename = dir + "/appserver_cpustat.out"
        l2_misses = hardware_counters.get_l2_misses(cpustat_file = filename)
        instructions = hardware_counters.get_instructions(cpustat_file = filename)

        misses_per_inst = (1000*l2_misses)/instructions
        tuple = [procs, misses_per_inst]
        #print tuple
        data.append(tuple)
    return data

def get_l2_miss_rate_by_input_data(tx_dir):
    data = []
    directories = glob.glob(tx_dir + "/*")
    for dir in directories:
        tx_rate = ecperf_get_tx_rate(directory = dir)

        filename = dir + "/appserver_cpustat.out"
        #l2_misses = hardware_counters.get_l2_misses(cpustat_file = filename)
        #instructions = hardware_counters.get_instructions(cpustat_file = filename)

        #misses_per_inst = (1000*l2_misses)/instructions
        #tuple = [tx_rate, misses_per_inst]
        tuple = [tx_rate, hardware_counters.get_l2_miss_ratio(cpustat_file = filename)]
        #print tuple
        data.append(tuple)
    return data

def get_inst_per_xact_data(results_dir, mode):
    data = []
    directories = glob.glob(results_dir + "/*")
    for dir in directories:
        #print "working in directory %s." % dir
        procs = ecperf_get_num_procs(directory = dir)

        filename = dir + "/appserver_cpustat.summary"
        if os.access(filename, os.F_OK):
            inst_per_xact = get_inst_per_xact(dir, mode)
            
            #instructions = hardware_counters.get_instructions(cpustat_file = filename)
            #if(instructions == 0):
            #continue
            #transactions = ecperf_get_transactions(directory = dir)
            #if(transactions == 0):
            #continue
            #inst_per_xact = instructions/transactions
            
            tuple = [procs, inst_per_xact]
            data.append(tuple)
    return data

def get_cpi_data(results_dir, mode):
    data = []
    directories = glob.glob(results_dir + "/*")
    for dir in directories:
        procs = ecperf_get_num_procs(directory = dir)
        file = dir + "/appserver_cpustat.summary"
        if os.access(file, os.F_OK):
            cpi = hardware_counters.cpustat_get_CPI(file, mode)
            data.append([procs, cpi])
    return data

def get_c2c_ratio_data(results_dir, mode):
    data = []
    directories = glob.glob(results_dir + "/*")
    for dir in directories:
        procs = ecperf_get_num_procs(directory = dir)
        file = dir + "/appserver_cpustat.summary"
        ctoc = hardware_counters.cpustat_get_ctoc(file, mode) * 100
        data.append([procs, ctoc])
    return data
    
def get_c2c_rate_data(results_dir, mode):
    data = []
    directories = glob.glob(results_dir + "/*")
    for dir in directories:
        procs = ecperf_get_num_procs(directory = dir)
        file = dir + "/appserver_cpustat.summary"
        ctoc = hardware_counters.cpustat_get_ctoc_per_inst(file, mode) * 100
        data.append([procs, ctoc])
    return data

def get_c2c_per_xact_data(results_dir):
    data = []
    directories = glob.glob(results_dir + "/*")
    for dir in directories:
        procs = ecperf_get_num_procs(directory = dir)

        filename = dir + "/appserver_cpustat.out"
        c2c = hardware_counters.get_c2c(cpustat_file = filename)
        transactions = ecperf_get_transactions(directory = dir)
        c2c_per_trans = c2c/transactions
                
        tuple = [procs, c2c_per_trans]
        data.append(tuple)
    return data

def get_time_breakdown_data(results_dir):
    tuple_list = []
    directories = glob.glob(results_dir + "/*")
    for dir in directories:
        #print "working in directory %s." % dir
        procs = ecperf_get_num_procs(directory = dir)

        filename = dir + "/appserver_mpstat.summary"
        if os.access(filename, os.F_OK):
            grep_lines = mfgraph.grep(filename, "AVG.+");
            line = string.split(grep_lines[0])
            if(len(line) > 3):
                usr = float(line[12])
                sys = float(line[13])
                wt  = float(line[14])
                idl = float(line[15])
            else:
                usr = 0
                sys = 0
                wt = 0
                idl = 0
                
            gc_time = ecperf_get_gc_time(dir)
            run_time = ecperf_get_stdystate(dir)
            if run_time == 0:
                continue
            
            proc_ratio = float(procs - 1)/procs
            gc_idle = proc_ratio * (gc_time/run_time) * 100

            adj_idl = idl - gc_idle
            if adj_idl < 0:
                adj_idl = 0
                gc_idle = idl
            
            tuple = [procs, usr, sys, wt, adj_idl, gc_idle]
            tuple_list.append(tuple)
    return tuple_list

def get_data_stall_breakdown_data(results_dir,
                                  mode,
                                  l2_lat,
                                  mem_lat,
                                  c2c_lat):
    data = []
    
    directories = glob.glob(results_dir + "/*")
    for dir in directories:
        #print "working in directory %s." % dir
        filename = dir + "/appserver_cpustat.summary"
        procs = ecperf_get_num_procs(dir)

        load_use_cycles = hardware_counters.cpustat_get_load_use(filename, mode)
        lu_raw_cycles = hardware_counters.cpustat_get_load_use_RAW(filename, mode)
        store_buf_cycles = hardware_counters.cpustat_get_storebuf(filename, mode)
        
        data_stall_cycles = load_use_cycles + lu_raw_cycles + store_buf_cycles
        l2_cycles = hardware_counters.cpustat_get_EC_rd_hit(filename, mode) * l2_lat
        c2c_cycles = hardware_counters.cpustat_get_snoop_cb(filename, mode) * c2c_lat
        mem_cycles = (hardware_counters.cpustat_get_EC_misses(filename, mode) - hardware_counters.cpustat_get_snoop_cb(filename, mode)) * mem_lat
        oth_cycles = load_use_cycles - mem_cycles - c2c_cycles - l2_cycles

        store_buf = store_buf_cycles/data_stall_cycles
        raw = lu_raw_cycles/data_stall_cycles
        l2_hit = l2_cycles/data_stall_cycles
        c2c = c2c_cycles/data_stall_cycles
        mem = mem_cycles/data_stall_cycles
        if oth_cycles > 0:
            oth = oth_cycles/data_stall_cycles
        else:
            oth = 0

        load_use_check = mem_cycles + l2_cycles + c2c_cycles
        #  print "Sanity Check:  LU=%d, Check=%d" % (load_use_cycles, load_use_check)
        #          print "L2 Cycles:  %d" % l2_cycles
        #          print "Mem Cycles: %d" % mem_cycles
        #          print "C2C_cycles: %d" % c2c_cycles
        #          print "STBUF Cyc:  %d" % store_buf_cycles
        #          print "RAW:        %d" % lu_raw_cycles
        #          print "Other:      %d" % oth_cycles
        
        tuple = [procs, store_buf, raw, oth, l2_hit, c2c, mem]
        data.append(tuple)
    return data

def get_cpi_breakdown_data(results_dir, mode):
    data = []

    directories = glob.glob(results_dir + "/*")
    for dir in directories:
        #print "working in directory %s." % dir
        filename = dir + "/appserver_cpustat.summary"
        procs = ecperf_get_num_procs(dir)

        cpi = hardware_counters.cpustat_get_CPI(filename, mode)
        inst_stall = hardware_counters.cpustat_get_inst_stall_rate(filename, mode)*cpi
        data_stall = hardware_counters.cpustat_get_data_stall_rate(filename, mode)*cpi
        total = inst_stall + data_stall

        if total < cpi:
            oth = cpi - total
            #print "CPI: %f, TOTAL: %f, OTHER %f" % (cpi, total, oth)
        else:
            #print "CPI: %f, TOTAL: %f" % (cpi, total)
            oth = 0

        tuple = [procs, inst_stall, data_stall, oth]
        data.append(tuple)
    return data

def get_mem_use_data(results_dir, when):
    data = []

    directories = glob.glob(results_dir + "/*")
    for dir in directories:
        procs = ecperf_get_num_procs(directory = dir)
        if when == "before":
            mem_use = ecperf_get_avg_mem_use_before(dir)
        elif when == "after":
            mem_use = ecperf_get_avg_mem_use_after(dir)
        else:
            print "Error: when must be 'before' or 'after'"
            return []
        data.append([procs, mem_use])
    return data

def get_gc_time_per_xact_by_input_data(tx_dir):
    data = []
    directories = glob.glob(tx_dir + "/*")
    for dir in directories:
        tx_rate = ecperf_get_tx_rate(directory = dir)

        #filename = dir + "/gc.summary"
        #grep_lines = mfgraph.grep(filename, ".+time.+");
        #line = string.split(grep_lines[0])
        #gc_time = float(line[3])
        gc_time = ecperf_get_gc_time(dir)
                
        transactions = ecperf_get_transactions(directory = dir)

        tuple = [tx_rate, (gc_time/transactions)]
        #print tuple
        data.append(tuple)
    return data


def get_cpustat_data(results_dir, mode, parameter):
    data = []

    directories = glob.glob(results_dir + "/*")
    for dir in directories:
        filename = dir + "/appserver_cpustat.summary"
        if os.access(filename, os.F_OK):
            procs = ecperf_get_num_procs(directory = dir)
            value = float(hardware_counters.cpustat_get(filename, mode, parameter))
            data.append([procs, value])
    return data


# converts a "per-instruction" metric to a "per-transaction"
def get_cpustat_per_xact_data(results_dir, mode, parameter):
    data = []

    directories = glob.glob(results_dir + "/*")
    for dir in directories:
        procs = ecperf_get_num_procs(directory = dir)
        filename = dir + "/appserver_cpustat.summary"
        if os.access(filename, os.F_OK):
            value = float(hardware_counters.cpustat_get(filename, mode, parameter))
            inst_per_xact = get_inst_per_xact(dir, mode)
            adj_value = value * inst_per_xact
        
            data.append([procs, adj_value])
    return data


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

def draw_std_line(jgraphs, title, ylabel, lines):
    xlabel = "Processors"
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

def generate_throughput(jgraphs, results_dir):
    # ECperf Data
    data = get_throughput_data(results_dir)
    ecperf_data = mfgraph.merge_data(group_title = "ECperf", data = data)

    # Graph ECperf Data

    linear = get_linear(data=ecperf_data)
    
    linear_data = mfgraph.merge_data(group_title = "Linear", data = linear)
    lines = [ecperf_data, linear_data]
    
    draw_std_line(jgraphs, "Throughput", "BBops/min", lines)


def generate_gc_time(jgraphs, results_dir):
    xlabel = "Processors"
    ylabel = "Time Spent in Garbage Collection (%)"

    # Gather ECperf data
    data = get_gc_time_data(results_dir)
    ecperf_data = mfgraph.merge_data(group_title = "ECperf", data = data)

    lines = [ecperf_data]

    draw_std_line(jgraphs, "GC Time", "GC Time (s)", lines)
    
def generate_gc_time_per_xact(jgraphs, results_dir):
    ylabel = "GC Time Per Transaction"

    # Gather ECperf data
    data = get_gc_time_per_xact_data(results_dir)
    ecperf_data = mfgraph.merge_data(group_title = "ECperf", data = data)

    #draw the graph
    lines = [ecperf_data]
    draw_std_line(jgraphs, "GC Time/Tranaction", ylabel, lines)

def generate_throughput_no_gc(jgraphs, results_dir):
    ylabel = "Throughput"

    # Gather ECperf Data
    data = get_throughput_data(results_dir)
    data_no_gc = get_throughput_no_gc_data(results_dir)

    ecperf_data = mfgraph.merge_data(group_title = "ECperf", data = data)
    ecperf_data_no_gc = mfgraph.merge_data(group_title = "ECperf no GC", data = data_no_gc)

    # Graph ECperf Data
    linear = get_linear(data=ecperf_data)

    linear_data = mfgraph.merge_data(group_title = "Linear", data = linear)
    lines = [linear_data, ecperf_data, ecperf_data_no_gc]

    draw_std_line(jgraphs, "Effect of GC on Throughput", ylabel, lines)

def generate_time_breakdown(jgraphs, results_dir):

    # Gather ECperf data    
    tuple_list = get_time_breakdown_data(results_dir)

    bar_fields = ["usr", "sys", "wt", "idl", "gc_idle"]
    ecperf_data = make_bars(fields=bar_fields, data=tuple_list)
    ecperf_bars = make_stacked(ecperf_data)
    
    ecperf_bars.sort()
    ecperf_bars.insert(0, "ECperf")
    ecperf_data.sort()
    ecperf_data.insert(0, "ECperf")

    graph_bars = [ecperf_bars]

    #print graph_bars
    jgraphs.append(mfgraph.stacked_bar_graph(graph_bars,
                                          title = "",
                                          bar_segment_labels = ["User", "System", "I/O", "Idle", "GC Idle"],
                                          ylabel = "Execution Time (%)",
                                          ymax = 100,
                                          ymin = 0,
                                          xsize = 4.0,
                                          ysize = 1.778,
                                          title_fontsize = "12",
                                          label_fontsize = "9",
                                          bar_name_font_size = "9",
                                          legend_fontsize = "9",
                                          stack_name_font_size = "10",
                                          stack_name_location = 9,
                                          bar_space = 1.2,
                                          colors = ["1 1 1",
                                                    ".8 .8 .8",
                                                    ".6 .6 .6",
                                                    ".4 .4 .4",
                                                    "0 0 0"]
                                          ))


def generate_tuning(jgraphs, results_dir):
    ylabel = "Throughput in BBops/min"

    data_map = get_tuning_data(results_dir)

    lines = []
    for key in data_map.keys():
        label = "%d threads" % key
        lines.append(mfgraph.merge_data(group_title = label, data = data_map[key]))

    draw_std_line(jgraphs, "Tuning Data", ylabel, lines)

def generate_inst_per_xact(jgraphs, results_dir, title, ylabel):
    user_data     = get_inst_per_xact_data(results_dir, "USER")
    kernel_data   = get_inst_per_xact_data(results_dir, "KERNEL")
    combined_data = get_inst_per_xact_data(results_dir, "COMBINED")

    lines = []
    lines.append(mfgraph.merge_data(group_title = "User", data = user_data))
    lines.append(mfgraph.merge_data(group_title = "Kernel", data = kernel_data))
    lines.append(mfgraph.merge_data(group_title = "Combined", data = combined_data))

    draw_std_line(jgraphs, title, ylabel, lines)


def generate_generic_cpustat(jgraphs, results_dir, parameter, title, ylabel):
    user_data     = get_cpustat_data(results_dir, "USER", parameter)
    kernel_data   = get_cpustat_data(results_dir, "KERNEL", parameter)
    combined_data = get_cpustat_data(results_dir, "COMBINED", parameter)

    lines = []
    lines.append(mfgraph.merge_data(group_title = "User", data = user_data))
    lines.append(mfgraph.merge_data(group_title = "Kernel", data = kernel_data))
    lines.append(mfgraph.merge_data(group_title = "Combined", data = combined_data))

    draw_std_line(jgraphs, title, ylabel, lines)


def generate_generic_cpustat_per_xact(jgraphs, results_dir, parameter, title, ylabel):
    user_data     = get_cpustat_per_xact_data(results_dir, "USER", parameter)
    kernel_data   = get_cpustat_per_xact_data(results_dir, "KERNEL", parameter)
    combined_data = get_cpustat_per_xact_data(results_dir, "COMBINED", parameter)

    lines = []
    lines.append(mfgraph.merge_data(group_title = "User", data = user_data))
    lines.append(mfgraph.merge_data(group_title = "Kernel", data = kernel_data))
    lines.append(mfgraph.merge_data(group_title = "Combined", data = combined_data))

    draw_std_line(jgraphs, title, ylabel, lines)

cpu_freq = 900 * 1000000

jgraph_input = []
ecperf_dir = "/p/multifacet/projects/ecperf/multifacet/workloads/ecperf/"
#ecperf_dir = "/p/multifacet/projects/java_locks/"
#run_dir = "results.test.pequod.6-6"
#run_dir = "results.pinot.tuning.rawdb.7-8"
run_dir = "results.pinot.tuning.7-14"
run_dir = "results.tuning_for_pass.8-5"
#run_dir = "results.zin.unified.7-10"
results_dir = ecperf_dir + run_dir

do_tuning = 0
if (len(sys.argv) >= 2):
    if (sys.argv[1] == "-t"):
        do_tuning = 1

if (do_tuning == 1):
    get_tuning_data(results_dir)
#generate_tuning(jgraph_input, results_dir)
else:
    generate_throughput(jgraph_input, results_dir)
#    generate_gc_time(jgraph_input, results_dir)
    generate_gc_time_per_xact(jgraph_input, results_dir)
#    generate_throughput_no_gc(jgraph_input, results_dir)
    generate_time_breakdown(jgraph_input, results_dir)
    generate_inst_per_xact(jgraph_input, results_dir, "Instructions/Transaction", "Instructions/BBop")
    generate_generic_cpustat(jgraph_input, results_dir, "CPI", "CPI", "Cycles/Instruction (CPI)")
    
#    generate_generic_cpustat(jgraph_input, results_dir, "DCACHE_MISSRATIO", "L1-D Miss Ratio", "")
    generate_generic_cpustat(jgraph_input, results_dir, "DCACHE_MISSRATE", "L1-D Miss Rate", "Misses/Instruction")
    generate_generic_cpustat_per_xact(jgraph_input, results_dir, "DCACHE_MISSRATE", "L1-D Misses/Transaction", "Misses/Transaction")
#     generate_generic_cpustat(jgraph_input, results_dir, "DCACHE_RDMISSRATIO", "L1-D RD Miss Ratio", "")
#     generate_generic_cpustat(jgraph_input, results_dir, "DCACHE_WRMISSRATIO", "L1-D WR Miss Ratio", "")
#     generate_generic_cpustat(jgraph_input, results_dir, "DATASTR", "Data Stall Rate", "Cycles/Instruction (CPI)")
#     generate_generic_cpustat(jgraph_input, results_dir, "DTLB_MISSRATE", "Data TLB Miss Rate", "Misses/Instruction")
#     generate_generic_cpustat(jgraph_input, results_dir, "DTLB_MISSRATIO", "Data TLB Miss Ratio", "Misses/Instruction")
    
#    generate_generic_cpustat(jgraph_input, results_dir, "ICACHE_MISSRATIO", "L1-I Miss Ratio", "")
    generate_generic_cpustat(jgraph_input, results_dir, "ICACHE_MISSRATE", "L1-I Miss Rate", "Misses/Instruction")
#     generate_generic_cpustat(jgraph_input, results_dir, "INSTRSTR", "Instruction Stall Rate", "Cycles/Instruction (CPI)")
#     generate_generic_cpustat(jgraph_input, results_dir, "ITLB_MISSRATE", "Inst TLB Miss Rate", "Misses/Instruction")

#    generate_generic_cpustat(jgraph_input, results_dir, "ECACHE_MISSRATIO", "L2 Miss Ratio", "")
    generate_generic_cpustat(jgraph_input, results_dir, "ECACHE_MISSRATE", "L2 Miss Rate", "Misses/Instruction")
    generate_generic_cpustat_per_xact(jgraph_input, results_dir, "ECACHE_MISSRATE", "L2 Misses/Transaction", "Misses/Transaction")
    generate_generic_cpustat(jgraph_input, results_dir, "ECACHE_AVGMISSPENALTY", "L2 Miss Penalty", "Cycles")
    generate_generic_cpustat(jgraph_input, results_dir, "ECACHE_CTOC_PER_INST", "CTOC Rate", "Misses/Instruction")
    generate_generic_cpustat_per_xact(jgraph_input, results_dir, "ECACHE_CTOC_PER_INST", "CTOC/Transaction", "Misses/Transaction")
    
#     generate_generic_cpustat(jgraph_input, results_dir, "ADDRESSBUS_UTIL", "Bus Utilization", "")
#     generate_generic_cpustat(jgraph_input, results_dir, "BRANCH_RATE", "Branch Rate", "Branches/Instruction")
#     generate_generic_cpustat(jgraph_input, results_dir, "BRANCH_MISSRATE", "Branch Missrate", "")
#     generate_generic_cpustat(jgraph_input, results_dir, "BRANCH_STR", "Branch Stall Rate", "Cycles/Instruction")
#     generate_generic_cpustat(jgraph_input, results_dir, "BRANCH_TAKENRATE", "Branch Taken Rate", "")

#     generate_generic_cpustat(jgraph_input, results_dir, "STOREQUEUE_STR", "Store Queue Stall Rate", "Cycles/Instruction")
#     generate_generic_cpustat(jgraph_input, results_dir, "IU_RAWSTALLRATE", "IU Raw Stall Rate", "Cycles/Instruction")
#     generate_generic_cpustat(jgraph_input, results_dir, "FP_RAWSTALLRATE", "FP Raw Stall Rate", "Cycles/Instruction")

#     generate_generic_cpustat(jgraph_input, results_dir, "MCU_READS0", "MCU Reads S0", "")
#     generate_generic_cpustat(jgraph_input, results_dir, "MCU_READS1", "MCU Reads S1", "")
#     generate_generic_cpustat(jgraph_input, results_dir, "MCU_READS2", "MCU Reads S2", "")
#     generate_generic_cpustat(jgraph_input, results_dir, "MCU_READS3", "MCU Reads S3", "")

#     generate_generic_cpustat(jgraph_input, results_dir, "MCU_WRITES0", "MCU Writes S0", "")
#     generate_generic_cpustat(jgraph_input, results_dir, "MCU_WRITES1", "MCU Writes S1", "")
#     generate_generic_cpustat(jgraph_input, results_dir, "MCU_WRITES2", "MCU Writes S2", "")
#     #generate_generic_cpustat(jgraph_input, results_dir, "MCU_WRITES3", "MCU Writes S3", "")

#     generate_generic_cpustat(jgraph_input, results_dir, "MCU_BANK0STALL", "MCU Bank-0 Stall", "")
#     generate_generic_cpustat(jgraph_input, results_dir, "MCU_BANK1STALL", "MCU Bank-1 Stall", "")
#     generate_generic_cpustat(jgraph_input, results_dir, "MCU_BANK2STALL", "MCU Bank-2 Stall", "")
#     generate_generic_cpustat(jgraph_input, results_dir, "MCU_BANK3STALL", "MCU Bank-3 Stall", "")
    
    print "Graph Inputs Ready...Running Jgraph."
    mfgraph.run_jgraph("newpage\n".join(jgraph_input), "ecperf")

