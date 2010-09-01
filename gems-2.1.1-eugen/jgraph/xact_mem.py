#!/s/std/bin/python

import sys, string, os, glob, re, mfgraph

#results_dir = "../results/isca07_final_version"
#results_dir = "/p/multifacet/projects/logtm_eager_lazy/ISCA07_results/old/old-11-7/"
results_dir = "../results/"
#results_dir = "/p/multifacet/projects/shore/HPCA-filters-results/non-smt/"
#results_dir = "/p/multifacet/projects/xact_memory/isca06_submission_results2"

make_excel_files = 0

def get_int_stat(file, stat):
    grep_lines = mfgraph.grep(file, stat)
    if (grep_lines == []):
      return -1        
    line = string.split(grep_lines[0])
    return int(line[1])

def get_float_stat(file, stat):
    grep_lines = mfgraph.grep(file, stat)
    line = string.split(grep_lines[0])
    return float(line[1])

def get_runtime(benchmark):
    data = []
    files = glob.glob(results_dir + "/" + benchmark + "/*.stats")
    for file in files:
        procs  = get_int_stat(file, "g_NUM_NODES")
        cycles = get_int_stat(file, "Ruby_cycles")
        #print "%dp:\t%d" % (procs, cycles)
        data.append((procs, cycles))
    return data

def get_stat(file, str):
    grep_lines = mfgraph.grep(file, str)
    line = (grep_lines[0]).split(':')
    return int(line[1]) 

def get_count_stat(file, stat):
    grep_lines = mfgraph.grep(file, stat)
    line = string.split(grep_lines[0])
    return int(line[6])

def get_average_stat(file, stat):
    grep_lines = mfgraph.grep(file, stat)
    if (grep_lines == []):
      return -1        
    line = string.split(grep_lines[0])
    return float(line[8])

def make_microbench_line(jgraphs, name, runs, bw, protocol_map, nest, label):
    xlabel = "Processors"
    ylabel = "Run Time"

    proc_values = [1,   2,  3,  4,  5,  6,  7,  8,  9, 10,
                   11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
                   21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
                   31]
    #proc_values = [1, 2, 3, 4, 5, 6, 7]
    read_set = None
    data = []
    
    for run in runs:
        #print run
        protocol = protocol_map[run[2]]
        line = [run[0]]
        for procs in proc_values:
            #print procs
            lst = []
            #read_set_str = "%s" % read_set
            glob_str = "%s/%s/%s-%dp-default-%s-%dnx-40minopen-%d-*.stats" % (results_dir, run[1], run[1], procs, protocol, nest, bw)
            files = glob.glob(glob_str)
            if files == []:
                #print "No files match: %s" % glob_str
                #exit
                continue
            for file in files:
                print file
                ruby_cycles = get_float_stat(file, "Ruby_cycles")/1000000.0
                #divide by procs?
                
                print "ruby_cycles: %f" % ruby_cycles
                lst.append(ruby_cycles)
                cycles = mfgraph.mean(lst)
            #print lst
            conf = mfgraph.confidence_interval_95_percent(lst)
            #print "95 conf: %f" % conf
            line.append([procs, cycles, cycles - conf, cycles + conf])
        data.append(line)

    print data
    jgraphs.append(mfgraph.line_graph(data,
                                      title = name,
                                      ylabel = "Execution Time (in millions of cycles)",
                                      xlabel = "Threads",
                                      xsize = 4.5,
                                      ysize = 8.0,
                                      line_thickness = 2.0,
                                      legend_x = "90",
                                      marktype = ["circle", "box", "triangle", "diamond"],
                                      #marksize = [0.4, 0.4, 0.4, 0.5],
                                      ))
    
    graph_out = mfgraph.line_graph(data,
                                   title = "",
                                   ylabel = "Execution Time (in millions of cycles)",
                                   #ymax = 1.50,
                                   ymin = 0,
                                   xsize = 2.5,
                                   ysize = 1.778,
                                   line_thickness = 2.0,
                                   marktype = ["circle", "box", "triangle", "diamond"],
                                   marksize = [0.2, 0.3, 0.3, 0.4],
                                   title_fontsize = "12",
                                   label_fontsize = "8",
                                   legend_fontsize = "9",
                                   legend_x = "70",
                                   colors = ["0 0 0",
                                             ".6 .6 .6",
                                             ".2 .2 .2",
                                             ".8 .8 .8",
                                             ".4 .4 .4"]
                                          )
    mfgraph.make_eps(graph_out, "%s_%s_line" % (name, label), "ttm_figures")
    if make_excel_files:
        mfgraph.make_excel_bar(name=name,
                               data=bars)

def make_simics_microbench_line(jgraphs, name, runs, bw, protocol_map, label):
    xlabel = "Processors"
    ylabel = "Run Time"

    proc_values = [1,   2,  3,  4,  5,  6,  7,  8,  9, 10,
                   11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
                   21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
                   31]
    #proc_values = [1, 2, 3, 4, 5, 6, 7]
    read_set = None
    data = []
    
    for run in runs:
        #print run
        protocol = protocol_map[run[2]]
        line = [run[0]]
        for procs in proc_values:
            #print procs
            lst = []
            #read_set_str = "%s" % read_set
            glob_str = "%s/%s/%s*%dp-default-%s-1nx-%d-*.stats" % (results_dir, run[1], run[1], procs, protocol, bw)
            files = glob.glob(glob_str)
            if files == []:
                print "No files match: %s" % glob_str
                #exit
                continue
            for file in files:
                #print file
                ruby_cycles = get_float_stat(file, "simics_cycles")/1000000.0
                #divide by procs?
                
                print "ruby_cycles: %f" % ruby_cycles
                lst.append(ruby_cycles)
                cycles = mfgraph.mean(lst)
            #print lst
            conf = mfgraph.confidence_interval_95_percent(lst)
            print "95 conf: %f" % conf
            line.append([procs, cycles, cycles - conf, cycles + conf])
        data.append(line)

    #print data
    jgraphs.append(mfgraph.line_graph(data,
                                      title = name,
                                      ylabel = "Execution Time (in millions of cycles)",
                                      xlabel = "Threads",
                                      xsize = 4.5,
                                      ysize = 8.0,
                                      line_thickness = 2.0,
                                      legend_x = "90",
                                      marktype = ["circle", "box", "triangle", "diamond"],
                                      #marksize = [0.4, 0.4, 0.4, 0.5],
                                      ))
    
    graph_out = mfgraph.line_graph(data,
                                   title = "",
                                   ylabel = "Execution Time (in millions of cycles)",
                                   #ymax = 1.50,
                                   ymin = 0,
                                   xsize = 2.5,
                                   ysize = 1.778,
                                   line_thickness = 2.0,
                                   marktype = ["circle", "box", "triangle", "diamond"],
                                   marksize = [0.2, 0.3, 0.3, 0.4],
                                   title_fontsize = "12",
                                   label_fontsize = "8",
                                   legend_fontsize = "9",
                                   legend_x = "70",
                                   colors = ["0 0 0",
                                             ".6 .6 .6",
                                             ".2 .2 .2",
                                             ".8 .8 .8",
                                             ".4 .4 .4"]
                                          )
    mfgraph.make_eps(graph_out, "%s_%s_line" % (name, label), "ttm_figures")
    if make_excel_files:
        mfgraph.make_excel_bar(name=name,
                               data=bars)
    

def make_microbench_speedup_line(jgraphs, name, runs, bw, protocol_map, nest, label):
    xlabel = "Processors"
    ylabel = "Run Time"

    proc_values = [1,   2,  3,  4,  5,  6,  7,  8,  9, 10,
                   11, 12, 13, 14, 15,
                   16, 17, 18, 19, 20,
                   21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
                   31]
    #proc_values = [1, 2, 3, 4, 5, 6, 7]
    read_set = None
    data = []
    
    for run in runs:
        #print run
        protocol = protocol_map[run[2]]
        line = [run[0]]
        base_cycles = 0.0
        for procs in proc_values:
            #print procs
            lst = []
            #read_set_str = "%s" % read_set
            glob_str = "%s/%s/%s-%dp-default-%s-%dnx-40minopen-%d-*.stats" % (results_dir, run[1], run[1], procs, protocol, nest, bw)
            #glob_str = "%s/%s/%s*-%dp-default-%s-%dnx-%d-*.stats" % (results_dir, run[1], run[1], procs, protocol, nesting, bw)
            files = glob.glob(glob_str)
            if files == []:
                print "No files match: %s" % glob_str
                #exit
                continue
            for file in files:
                print file
                ruby_cycles = get_float_stat(file, "Ruby_cycles")/1000000.0
                #divide by procs?
                #print "ruby_cycles: %f" % ruby_cycles
                lst.append(ruby_cycles)
            cycles = mfgraph.mean(lst)
            conf = mfgraph.confidence_interval_95_percent(lst)
            #print "95 conf: %f" % conf
            if procs == 1:
                base_cycles = cycles
                print "base_cycles is: %f" % base_cycles

            line.append([procs, base_cycles/cycles, base_cycles/(cycles - conf), base_cycles/(cycles + conf)])
        data.append(line)

    linear = ["Linear"]
    for p in proc_values:
        linear.append([p, p, p, p])
    data.append(linear)

    #print data
    jgraphs.append(mfgraph.line_graph(data,
                                      title = name,
                                      ylabel = "Speedup",
                                      xlabel = "Threads",
                                      xsize = 4.5,
                                      ysize = 6.0,
                                      line_thickness = 2.0,
                                      legend_x = "90",
                                      marktype = ["circle", "box", "triangle", "diamond"],
                                      #marksize = [0.4, 0.4, 0.4, 0.5],
                                      ))

def make_microbench_speedup_line2(jgraphs, name, runs, bw, protocol, nest, label, minopen):
    xlabel = "Processors"
    ylabel = "Run Time"

    #proc_values = [1,   2,  3,  4,  5,  6,  7,  8,  9, 10,
    #               11, 12, 13, 14, 15,
    #               16, 17, 18, 19, 20,
    #               21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
    #               31]
    proc_values = [1, 3, 7, 15, 24, 31]
    read_set = None
    data = []
    
    for run in runs:
        #print run
        #protocol = protocol_map[run[2]]
        line = [run[0]]
        base_cycles = 0.0
        for procs in proc_values:
            #print procs
            lst = []
            #read_set_str = "%s" % read_set
            glob_str = "%s/%s/%s-%dp-default-%s-%dnx-%dminopen-%d-*.stats" % (results_dir, run[1], run[1], procs, protocol, nest, minopen, bw)
            #glob_str = "%s/%s/%s*-%dp-default-%s-%dnx-%d-*.stats" % (results_dir, run[1], run[1], procs, protocol, nesting, bw)
            files = glob.glob(glob_str)
            if files == []:
                print "No files match: %s" % glob_str
                #exit
                continue
            for file in files:
                print file
                ruby_cycles = get_float_stat(file, "Ruby_cycles")/1000000.0
                #divide by procs?
                #print "ruby_cycles: %f" % ruby_cycles
                lst.append(ruby_cycles)
            cycles = mfgraph.mean(lst)
            conf = mfgraph.confidence_interval_95_percent(lst)
            #print "95 conf: %f" % conf
            if procs == 1:
                base_cycles = cycles
                print "base_cycles is: %f" % base_cycles

            line.append([procs, base_cycles/cycles, base_cycles/(cycles - conf), base_cycles/(cycles + conf)])
        data.append(line)

    linear = ["Linear"]
    for p in proc_values:
        linear.append([p, p, p, p])
    data.append(linear)

    #print data
    jgraphs.append(mfgraph.line_graph(data,
                                      title = name,
                                      ylabel = "Speedup",
                                      xlabel = "Threads",
                                      xsize = 4.5,
                                      ysize = 6.0,
                                      line_thickness = 2.0,
                                      legend_x = "90",
                                      marktype = ["circle", "box", "triangle", "diamond"],
                                      #marksize = [0.4, 0.4, 0.4, 0.5],
                                      ))
    
def make_microbench_combined_speedup_line(jgraphs, name, runs, bw, protocol, label):
    xlabel = "Processors"
    ylabel = "Run Time"

    #proc_values = [1,   2,  3,  4,  5,  6,  7,  8,  9, 10,
    #               11, 12, 13, 14, 15,
    #               16, 17, 18, 19, 20,
    #               21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
    #               31]
    proc_values = [1, 3, 7, 15, 24, 31]
    minopen_values = [40, 35]
    nest_values = [1, 4]
    read_set = None
    data = []
    
    
    for run in runs:
        line = [run[0]]
        base_cycles = 0.0
        for procs in proc_values:
            lst = []
            glob_str = "%s/%s/%s-%dp-default-%s-%snx-%sminopen-%d-*.stats" % (results_dir, run[1], run[1], procs, protocol, run[3], run[2], bw)
            files = glob.glob(glob_str)
            if files == []:
                print "No files match: %s" % glob_str
                continue
            for file in files:
                print file
                ruby_cycles = get_float_stat(file, "Ruby_cycles")/1000000.0
                #divide by procs?
                #print "ruby_cycles: %f" % ruby_cycles
                lst.append(ruby_cycles)
            cycles = mfgraph.mean(lst)
            conf = mfgraph.confidence_interval_95_percent(lst)
            #print "95 conf: %f" % conf
            if procs == 1:
                base_cycles = cycles
                #print "base_cycles is: %f" % base_cycles
            # print "cycles is: ", base_cycles/cycles
            line.append([procs, base_cycles/cycles, base_cycles/(cycles - conf), base_cycles/(cycles + conf)])
        data.append(line)

    #linear = ["Linear"]
    #for p in proc_values:
    #    linear.append([p, p, p, p])
    #data.append(linear)

    #print data
    graph_out = mfgraph.line_graph(data,
                                      ylabel = "Speedup",
                                      xlabel = "Threads",
                                      label_fontsize = "10",
                                      xsize = 2.5,
                                      ysize = 2.3,
                                      line_thickness = 2.0,
                                      legend_x = "75",
                                      legend_y = "60",
                                      legend_fontsize = "10",
                                      marktype = ["circle", "box", "triangle", "diamond"],
                                      #marksize = [0.4, 0.4, 0.4, 0.5],
                                      )
    
    jgraphs.append(graph_out)
    mfgraph.make_eps(graph_out, label, "btree")
    
def make_microbench_bar(jgraphs, name, runs, bw, protocol_map, label):
    xlabel = "Processors"
    ylabel = "Run Time"
    #read_set = 64
    read_set = None

    bars = []
    proc_values = [2, 4, 8, 16, 31]
    #proc_values = [1, 2, 8, 15]
    #proc_values = [1, 2] 
    for run in runs:
        #print run
        protocol = protocol_map[run[2]]
        bar = [run[0]]
        for procs in proc_values:
            #print procs
            lst = []
            #read_set_str = "%s" % read_set
            #glob_str = "%s/%s/%s-%sk-%dp-default-%s-%d-*.stats" % (results_dir, run[1], run[1], read_set_str, procs, protocol, bw)
            glob_str = "%s/%s/%s*-%dp-default-%s-%d-*.stats" % (results_dir, run[1], run[1], procs, protocol, bw)
            files = glob.glob(glob_str)
            if files == []:
                print "No files match: %s" % glob_str
                #exit
                continue
            for file in files:
                #print file
                ruby_cycles = get_float_stat(file, "Ruby_cycles")/1000000.0
                #print "ruby_cycles: %d" % ruby_cycles
                lst.append(ruby_cycles)
                cycles = mfgraph.mean(lst)
            print lst
            conf = mfgraph.confidence_interval_95_percent(lst)

            bar.append(["%d" % procs, [cycles, cycles - conf, cycles + conf]])
        bars.append(bar)

    #print bars
    jgraphs.append(mfgraph.stacked_bar_graph(bars,
                                             bar_segment_labels = ["runtime"],
                                             xsize = 5.0,
                                             ))
    
    graph_out = mfgraph.stacked_bar_graph(bars,
                                          title = "",
                                          bar_segment_labels = ["Execution Time"],
                                          ylabel = "Execution Time (in millions of cycles)",
                                          #ymax = 1.50,
                                          ymin = 0,
                                          xsize = 2.5,
                                          ysize = 1.778,
                                          title_fontsize = "12",
                                          label_fontsize = "8",
                                          bar_name_font_size = "9",
                                          legend_fontsize = "9",
                                          stack_name_font_size = "9",
                                          stack_name_location = 9,
                                          bar_space = 1.2,
                                          colors = ["0 0 0",
                                                    ".6 .6 .6",
                                                    ".2 .2 .2",
                                                    ".8 .8 .8",
                                                    ".4 .4 .4"]
                                          )
    mfgraph.make_eps(graph_out, "%s_%s_bar" % (name, label), "ttm_figures")
    if make_excel_files:
        mfgraph.make_excel_bar(name=name,
                               data=bars)
    

def make_read_set(jgraphs, name, runs, bw):
    protocol = "MOESI_xact_hammer_bf"
    xlabel = "Processors"
    ylabel = "Run Time"

    bars = []
    read_set_sizes = [4, 8, 16, 32, 64]
    procs = 7
    for run in runs:
        bar = [run[0]]
        for read_set in read_set_sizes:
            lst = []
            files = glob.glob("%s/%s/%s-%dk-%dp-default-%s-%d-*.stats" % (results_dir, run[1], run[1], read_set, procs, protocol, bw))
            for file in files:
                #print file
                lst.append(get_float_stat(file, "Ruby_cycles"))
            #print lst
            cycles = mfgraph.mean(lst)
            conf = mfgraph.confidence_interval_95_percent(lst)
            #print conf
                
            bar.append(["%dk" % read_set, cycles, cycles - conf, cycles + conf])
        bars.append(bar)

    #print bars
    jgraphs.append(mfgraph.stacked_bar_graph(bars,
                                             bar_segment_labels = ["runtime"],
                                             xsize = 5.0,
                                             ))
    
    graph_out = mfgraph.stacked_bar_graph(bars,
                                          title = "",
                                          bar_segment_labels = ["Execution Time"],
                                          ylabel = "Execution Time (in millions of cycles)",
                                          #ymax = 1.50,
                                          ymin = 0,
                                          xsize = 3.5,
                                          ysize = 1.778,
                                          title_fontsize = "12",
                                          label_fontsize = "8",
                                          bar_name_font_size = "8",
                                          legend_fontsize = "9",
                                          stack_name_font_size = "9",
                                          stack_name_location = 9,
                                          bar_space = 1.2,
                                          colors = ["1 1 1",
                                                    ".8 .8 .8",
                                                    ".6 .6 .6",
                                                    ".4 .4 .4",
                                                    "0 0 0"]
                                          )
    mfgraph.make_eps(graph_out, "%s_read_set" % name, "ttm_figures")
    if make_excel_files:
        mfgraph.make_excel_bar(name="%s_read_set" % name, data=bars)

def make_norm_runtime(jgraphs, pairs, procs):
    xlabel = "Processors"
    ylabel = "Speedup"

    bars = []
    proc_values = [procs]
    for pair in pairs:
        bar = [pair[0]]
        for procs in proc_values:
            #bar = []
            lst = []
            #glob_str = "%s/%s/%s-%dp-1t-default-%s*.stats" % (results_dir, pair[1], pair[1], procs, protocol)
            protocol = "MOESI_directory"
            glob_str = "%s/%s/%s-%dp*-1t-default-%s*.stats" % (results_dir, pair[1], pair[1], procs, protocol)
            files = glob.glob(glob_str)
            if files == []:
                print "%s not found" % glob_str
                return
            for file in files:
                lst.append(get_int_stat(file, "Ruby_cycles"))
            cycles_base = mfgraph.mean(lst)

            lst = []
            #files = glob.glob("%s/%s/%s-%dp-1t-default-%s*.stats" % (results_dir, pair[2], pair[2], procs, protocol))
            protocol = "MOESI_xact_directory_synch"
            files = glob.glob("%s/%s/%s-%dp*-1t-default-%s*.stats" % (results_dir, pair[2], pair[2], procs, protocol))
            for file in files:
                lst.append(get_int_stat(file, "Ruby_cycles"))
            cycles_test = mfgraph.mean(lst)
                        
            norm_exe = float(cycles_base)/float(cycles_test)
            bar.append(["%d" % procs, norm_exe])
            #bar += ["%d" % procs, ["TM", cycles_test], ["L", cycles_base]]
        bars.append(bar)

    jgraphs.append(mfgraph.stacked_bar_graph(bars,
                                             bar_segment_labels = ["runtime"],
                                             xsize = 5.0,
                                             ymax = 5.0,
                                             ))

    graph_out = mfgraph.stacked_bar_graph(bars,
                                          title = "",
                                          bar_segment_labels = ["Execution Time"],
                                          ylabel = "Speedup ",
                                          ymax = 5.0,
                                          ymin = 0,
                                          xsize = 2.5,
                                          ysize = 1.778,
                                          title_fontsize = "12",
                                          label_fontsize = "8",
                                          bar_name_font_size = "5",
                                          legend_fontsize = "9",
                                          stack_name_font_size = "5",
                                          stack_name_location = 9,
                                          bar_space = 1.2,
                                          colors = ["1 0 1",
                                                    ".8 .8 .8",
                                                    ".6 .6 .6",
                                                    ".4 .4 .4",
                                                    "0 0 0"])
    mfgraph.make_eps(graph_out, "splash_norm", "ttm_figures")
    if make_excel_files:
        print bars    
        mfgraph.make_excel_bar("xact_splash_norm", bars) 
    
def make_abs_runtime(jgraphs, pairs, procs):
    xlabel = "Processors"
    ylabel = "Run Time"

    bars = []
    for pair in pairs:
        #print pair[0]
        
        bar = []
        lst = []
	protocol = "MOESI_directory"
        glob_str = "%s/%s/%s-%dp-1t-default-%s*.stats" % (results_dir, pair[1], pair[1], procs, protocol)
        files = glob.glob(glob_str)
        if files == []:
            print "%s not found" % glob_str
            return
        for file in files:
            lst.append(get_int_stat(file, "Ruby_cycles")/1000000.0)
        cycles_base = mfgraph.mean(lst)
        base_conf = mfgraph.confidence_interval_95_percent(lst)
            
        lst = []
	protocol="MOESI_xact_directory_synch"
        glob_str = "%s/%s/%s-%dp-1t-default-%s*.stats" % (results_dir, pair[2], pair[2], procs, protocol)
        #glob_str = "%s/%s/%s-%dp*-default-%s*.stats" % (results_dir, pair[2], pair[2], procs, pair[3])
        files = glob.glob(glob_str)
        if files == []:
            print "%s not found" % glob_str
            return
        for file in files:
            lst.append(get_int_stat(file, "Ruby_cycles")/1000000.0)
        cycles_test = mfgraph.mean(lst)
        test_conf = mfgraph.confidence_interval_95_percent(lst)
            
        norm_exe = float(cycles_test)/float(cycles_base)
        #bar.append(["%d" % procs, norm_exe])
        bar += [pair[0],
                ["TTM", [cycles_test, cycles_test - test_conf, cycles_test + test_conf]],
                ["Locks", [cycles_base, cycles_base - base_conf, cycles_base + base_conf]]
                ]
        bars.append(bar)

    jgraphs.append(mfgraph.stacked_bar_graph(bars,
                                             bar_segment_labels = ["runtime"],
                                             xsize = 5.0,
                                             ))

    graph_out = mfgraph.stacked_bar_graph(bars,
                                          title = "",
                                          bar_segment_labels = ["Execution Time"],
                                          ylabel = "Execution Time (in millions of cycles)",
                                          #ymax = 1.50,
                                          ymin = 0,
                                          xsize = 2.5,
                                          ysize = 1.778,
                                          title_fontsize = "12",
                                          label_fontsize = "8",
                                          bar_name_font_size = "9",
                                          legend_fontsize = "9",
                                          stack_name_font_size = "9",
                                          stack_name_location = 9,
                                          bar_space = 1.2,
                                          colors = ["1 1 1",
                                                    ".8 .8 .8",
                                                    ".6 .6 .6",
                                                    ".4 .4 .4",
                                                    "0 0 0"])
    mfgraph.make_eps(graph_out, "splash_abs", "ttm_figures")
    if make_excel_files:
        mfgraph.make_excel_bar(name="splash_abs",
                               data=bars)
    
def make_distribution(jgraphs, benchmarks, procs, dist, stat):
    protocol = "MOESI_xact_directory"
    lines = []
    max_index = 0
    for (name, benchmark) in benchmarks:
        #glob_str = "%s/%s/%s-%dp-1t-default-%s*.stats" % (results_dir, benchmark, benchmark, procs, protocol)
        glob_str = "%s/%s/%s-%dp*-default-%s*.stats" % (results_dir, benchmark, benchmark, procs, protocol)
        files = glob.glob(glob_str)
        if files == []:
            print "No files match: %s" % glob_str
            exit                
        data = []
        for file in files:
            grep_lines = mfgraph.grep(file, stat[0])
            tokens = string.split(grep_lines[0])
            lst = []
            pipe_count = 0
            count = 0
            count_next = 0
            bin_size_next = 0
            bin_size = 0
            index = 0
            for token in tokens:
                #print token
                if token == '|':
                    pipe_count += 1
                elif token == 'count:':
                    count_next = 1
                elif token == '[binsize:':
                    bin_size_next = 1
                elif bin_size_next == 1:
                    bin_size_next = 0
                    bin_size = int(token)
                    #print "BIN_SIZE = %d" % bin_size
                elif count_next == 1:
                    count_next = 0
                    count = int(token)
                elif token == ']':
                    pipe_count = 0
                elif pipe_count >= 2:
                    lst.append([index*bin_size, float(token)])
                    index += 1
                    if index > max_index:
                        max_index = index
            
            if dist == "CDF":
                data += mfgraph.cdf(count, lst)
            elif dist == "PDF":
                #lst.append([index, 0])
                data += mfgraph.pdf(count, lst)
            elif dist == "WPDF":
                data += mfgraph.w_pdf(count, lst)
            else:
                print "ERROR, distribution %s not supported." % dist
                exit(0)
                
        lines.append(mfgraph.merge_data(name, data))
        
        if dist == "CDF":
            for line in lines[1:]:
                line.append([max_index, 1.0, 1.0, 1.0])
            
    graph_out = mfgraph.line_graph(lines,
                                   title = "",
                                   xlabel = stat[1],
                                   ylabel = dist,
                                   xsize = 4.5
                                   )
    jgraphs.append(graph_out)

    graph_out = mfgraph.line_graph(lines,
                                   title = "",
                                   xlabel = stat[1],
                                   ylabel = dist,
                                   ymax = 1.0,
                                   title_fontsize = "12",
                                   label_fontsize = "8",
                                   bar_name_font_size = "9",
                                   legend_fontsize = "9",
                                   legend_x = "50",
                                   line_thickness = 1.5,
                                   #xlog = 2,
                                   xsize = 2.5,
                                   ysize = 1.778)
    mfgraph.make_eps(graph_out, "%s-%s" % (stat[2], dist), "ttm_figures")
    if make_excel_files:
        mfgraph.make_excel_line(name="%s-%s" % (stat[2], dist),
                                data=lines)


def make_dist_table(benchmarks, procs, stat):
    protocol = "MOESI_xact_directory"

    print
    print "DISTRIBUTION TABLE--%s" % stat[1]

    bins = [1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024]
    block_size = 64
    max_index = 0
    for (name, benchmark) in benchmarks:
        data = {}
        for bin in bins:
            data[bin] = 0;
        
        #glob_str = "%s/%s/%s-%dp-1t-default-%s*.stats" % (results_dir, benchmark, benchmark, procs, protocol)
        glob_str = "%s/%s/%s-%dp*-default-%s*.stats" % (results_dir, benchmark, benchmark, procs, protocol)
        files = glob.glob(glob_str)
        if files == []:
            print "No files match: %s" % glob_str
            exit                
        for file in files:
            grep_lines = mfgraph.grep(file, stat[0])
            tokens = string.split(grep_lines[0])
            lst = []
            pipe_count = 0
            count = 0
            count_next = 0
            bin_size_next = 0
            bin_size = 0
            index = 0
            for token in tokens:
                #print token
                if token == '|':
                    pipe_count += 1
                elif token == 'count:':
                    count_next = 1
                elif token == '[binsize:':
                    bin_size_next = 1
                elif bin_size_next == 1:
                    bin_size_next = 0
                    bin_size = int(token)
                    #print "BIN_SIZE = %d" % bin_size
                elif count_next == 1:
                    count_next = 0
                    count = int(token)
                elif token == ']':
                    pipe_count = 0
                elif pipe_count >= 2:
                    lst.append([index*bin_size, int(token)])
                    index += 1
                    if index > max_index:
                        max_index = index

            for tuple in lst:
                last_bin = 0
                for bin in bins:
                    if tuple[0] >= last_bin and tuple[0] < bin:
                        #print "adding %f" % tuple[1]
                        data[bin] = data[bin] + tuple[1]
                        last_bin = bin
        print "%s for %s: " % (stat[1], name)
        for bin in bins:
            print "%d B\t%d" % ((bin*block_size), data[bin])
            
def make_stall_histogram(jgraphs, benchmarks, protocol):
	stat = "xact_stall_occupancy"
	bins = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32]
	processors = [4, 8, 12, 16, 32]
	bars = []    
	for (name, benchmark) in benchmarks:
		set = [name]	
		for procs in processors:
			#print name
			data = {}
        
			for bin in bins:
				data[bin] = 0

			total_cycles = 0
			trans_cycles = 0
			ruby_cycles = 0
			file_count = 0
			#glob_str = "%s/%s/%s-%dp-1t-default-%s*.stats" % (results_dir, benchmark, benchmark, procs, protocol)
			glob_str = "%s/%s/%s-%dp*-default-%s*.stall" % (results_dir, benchmark, benchmark, procs, protocol)
			files = glob.glob(glob_str)
			if files == []:
				print "No files match: %s" % glob_str
				continue                
			for file in files:
				trans_cycles += get_int_stat(file, "Trans_cycles")
				file_count += 1
				grep_lines = mfgraph.grep(file, stat)
	            #print grep_lines
				bin = 0
				#print grep_lines
				for line in grep_lines:
					#print line
					tokens = string.split(line, ":")
					#print tokens
					cycles = string.atoi(tokens[1])
					total_cycles += cycles
					#print bin
					data[bin] += cycles
					bin += 1
	
			print name
			#print data
			#print "total_cycles = %d, ruby_cycles = %d" % (total_cycles, ruby_cycles)
			print "trans_cycles = %d" % (trans_cycles)
			temp = []
			bar = [procs]
			if file_count > 0 :
				total = 0.0
				if (total_cycles == 0):
					total_cycles = 1	
				for bin in bins:
					#print "%d: %f" % (bin, float(data[bin])/float(total_cycles))
					point = float(data[bin])/float(trans_cycles)
					#point = float(data[bin])
					total += point
					temp.append(total)
				temp.reverse()
				bar += temp
				#print bar.reverse()
				set.append(bar)
		bars.append(set)
	print bars
	jgraphs.append(mfgraph.stacked_bar_graph(bars,
                                             title = "Stall Distribution graph",
											 ylabel = "Normalized Stall times",
                                             bar_segment_labels = ["runtime"],
                                             xsize = 6.5,
                                             #bar_name_rotate = 90.0,
                                             legend = "on",
                                             stack_name_location = 12,
											 colors = ["1 0 0", "0 0 0", "0 .5 0", "1 0 1", "1 1 0", "0.5 0 0"]
                                             ))


def make_xact_histogram(jgraphs, benchmarks, protocol):
	stat = "xact_occupancy"
	bins = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32]
	processors = [4, 8, 12, 16, 32]
	bars = []    
	for (name, benchmark) in benchmarks:
		set = [name]	
		for procs in processors:
			#print name
			data = {}
        
			for bin in bins:
				data[bin] = 0

			total_cycles = 0
			trans_cycles = 0
			ruby_cycles = 0
			file_count = 0
			#glob_str = "%s/%s/%s-%dp-1t-default-%s*.stats" % (results_dir, benchmark, benchmark, procs, protocol)
			glob_str = "%s/%s/%s-%dp*-default-%s*.stall" % (results_dir, benchmark, benchmark, procs, protocol)
			files = glob.glob(glob_str)
			if files == []:
				print "No files match: %s" % glob_str
				continue                
			for file in files:
				trans_cycles += get_int_stat(file, "Total_cycles")
				file_count += 1
				grep_lines = mfgraph.grep(file, stat)
	            #print grep_lines
				bin = 0
				#print grep_lines
				for line in grep_lines:
					if (bin == 0):
						data[bin] = 0
						bin += 1
						continue		
					#print line
					tokens = string.split(line, ":")
					#print tokens
					cycles = string.atoi(tokens[1])
					total_cycles += cycles
					#print bin
					data[bin] += cycles
					bin += 1
	
			print name
			#print data
			#print "total_cycles = %d, ruby_cycles = %d" % (total_cycles, ruby_cycles)
			print "total_cycles = %d" % (trans_cycles)
			temp = []
			bar = [procs]
			if file_count > 0 :
				total = 0.0
				if (total_cycles == 0):
					total_cycles = 1	
				for bin in bins:
					#print "%d: %f" % (bin, float(data[bin])/float(total_cycles))
					point = float(data[bin])/float(trans_cycles)
					#point = float(data[bin])
					total += point
					temp.append(total)
				temp.reverse()
				bar += temp
				#print bar.reverse()
				set.append(bar)
		bars.append(set)
	print bars
	jgraphs.append(mfgraph.stacked_bar_graph(bars,
                                             title = "Xact Occupancy Distribution graph",
											 ylabel = "Normalized Xact times",
                                             bar_segment_labels = ["runtime"],
                                             xsize = 6.5,
                                             #bar_name_rotate = 90.0,
                                             legend = "on",
                                             stack_name_location = 12,
											 colors = ["1 0 0", "0 0 0", "0 .5 0", "1 0 1", "1 1 0", "0.5 0 0"]
                                             ))



def make_aborts_graph(jgraphs, benchmarks, protocol):

	bars = []
	bar = []
	processors = [2, 4, 8, 12, 16, 32]
	for (name, benchmark) in benchmarks:
		set = [name]
		for procs in processors:		
			glob_str = "%s/%s/%s-%dp-1t-default-%s*.stats" % (results_dir, benchmark, benchmark, procs, protocol)
			files = glob.glob(glob_str)
			lst = []
			count = []
			if files == []:
				print "%s not found" % glob_str
				continue
			for file in files:
				lst.append(get_int_stat(file, "xact_aborts"))
				count.append(get_int_stat(file, "^Begin_XACT"))	
			mean_aborts = mfgraph.mean(lst)
			mean_count = mfgraph.mean(count)
			if files == []:
				mean_aborts = 0
				mean_count = 1
			norm_abort = mean_aborts / mean_count
			set.append([procs,norm_abort])
		bars.append(set)

			
        #test_conf = mfgraph.confidence_interval_95_percent(lst)
            
        #norm_exe = float(cycles_test)/float(cycles_base)
        #bar.append(["%d" % procs, norm_exe])
        #bar += [pair[0],
        #        ["TTM", [cycles_test, cycles_test - test_conf, cycles_test + test_conf]],
        #        ["Locks", [cycles_base, cycles_base - base_conf, cycles_base + base_conf]]
        #        ]
	print bars
	jgraphs.append(mfgraph.stacked_bar_graph(bars,
                                             title = "Transactional Aborts",
											 ylabel = "Aborts/Transaction",
                                             bar_segment_labels = ["runtime"],
                                             xsize = 5.0,
                                             ))

	graph_out = mfgraph.stacked_bar_graph(bars,
                                          title = "Transactional Aborts",
                                          bar_segment_labels = ["Execution Time"],
                                          ylabel = "Aborts/Transaction",
                                          #ymax = 1.50,
                                          ymin = 0,
                                          xsize = 2.5,
                                          ysize = 1.778,
                                          title_fontsize = "12",
                                          label_fontsize = "8",
                                          bar_name_font_size = "9",
                                          legend_fontsize = "9",
                                          stack_name_font_size = "9",
                                          stack_name_location = 9,
                                          bar_space = 1.2,
                                          colors = ["1 0 1",
                                                    ".8 .8 .8",
                                                    ".6 .6 .6",
                                                    ".4 .4 .4",
                                                    "0 0 0"])
    
def make_writebacks_graph(jgraphs, benchmarks, protocol):

	bars = []
	bar = []
	processors = [2, 4, 8, 12, 16, 32]
	for (name, benchmark) in benchmarks:
		set = [name]
		for procs in processors:		
			glob_str = "%s/%s/%s-%dp-1t-default-%s*.stats" % (results_dir, benchmark, benchmark, procs, protocol)
			files = glob.glob(glob_str)
			lst = []
			count = []
			if files == []:
				print "%s not found" % glob_str
				continue
			for file in files:
				lst.append(get_int_stat(file, "xact_extra_wbs"))
				count.append(get_int_stat(file, "^Begin_XACT"))	
			mean_aborts = mfgraph.mean(lst)
			mean_count = mfgraph.mean(count)
			if files == []:
				mean_aborts = 0
				mean_count = 1
			norm_abort = mean_aborts 
			set.append([procs,norm_abort])
		bars.append(set)

			
        #test_conf = mfgraph.confidence_interval_95_percent(lst)
            
        #norm_exe = float(cycles_test)/float(cycles_base)
        #bar.append(["%d" % procs, norm_exe])
        #bar += [pair[0],
        #        ["TTM", [cycles_test, cycles_test - test_conf, cycles_test + test_conf]],
        #        ["Locks", [cycles_base, cycles_base - base_conf, cycles_base + base_conf]]
        #        ]
	print bars
	jgraphs.append(mfgraph.stacked_bar_graph(bars,
                                             bar_segment_labels = ["runtime"],
                                             xsize = 5.0,
                                             ))

	graph_out = mfgraph.stacked_bar_graph(bars,
                                          title = "Extra Writebacks",
                                          bar_segment_labels = ["Execution Time"],
                                          ylabel = "",
                                          #ymax = 1.50,
                                          ymin = 0,
                                          xsize = 2.5,
                                          ysize = 1.778,
                                          title_fontsize = "12",
                                          label_fontsize = "8",
                                          bar_name_font_size = "9",
                                          legend_fontsize = "9",
                                          stack_name_font_size = "9",
                                          stack_name_location = 9,
                                          bar_space = 1.2,
                                          colors = ["1 0 1",
                                                    ".8 .8 .8",
                                                    ".6 .6 .6",
                                                    ".4 .4 .4",
                                                    "0 0 0"])

def make_scalability_lines(jgraphs, benchmarks, protocol):
    xlabel = "Processors"
    ylabel = "Run Time"

    proc_values = [1, 2, 4, 8, 12, 16, 32]				   
    data = []
    
    for (name,benchmark) in benchmarks:
        for procs in proc_values:
            #print procs
            lst = []
            #read_set_str = "%s" % read_set
            glob_str = "%s/%s/%s*-%dp-default-%s-%d-*.stats" % (results_dir, benchmark, benchmark, procs, protocol)
            files = glob.glob(glob_str)
            if files == []:
                print "No files match: %s" % glob_str
                #exit
                continue
            for file in files:
                #print file
                ruby_cycles = get_float_stat(file, "Ruby_cycles")/1000000.0
                #divide by procs?
                
                #print "ruby_cycles: %d" % ruby_cycles
                lst.append(ruby_cycles)
                cycles = mfgraph.mean(lst)
            #print lst
            conf = mfgraph.confidence_interval_95_percent(lst)

            line.append([procs, cycles, cycles - conf, cycles + conf])
        data.append(line)

    #print data
    jgraphs.append(mfgraph.line_graph(data,
                                      title = name,
                                      ylabel = "Execution Time (in millions of cycles)",
                                      xlabel = "Threads",
                                      xsize = 4.5,
                                      ysize = 8.0,
                                      line_thickness = 2.0,
                                      legend_x = "90",
                                      marktype = ["circle", "box", "triangle", "diamond"],
                                      #marksize = [0.4, 0.4, 0.4, 0.5],
                                      ))
    

def make_tlb_line(jgraphs):
    xlabel = "Benchmark Footprint(in KB)"
    ylabel = "Data Read (in KB) / msec"

    array_values = [4, 8, 16, 32, 64, 128, 256, 384, 448, 480, 496, 504, 508, 510, 512]
    data = []
    runs = ["tlb-TM", "tlb-abort-TM"] 
    
    for run in runs:
        #print run
        if (run == "tlb-TM"):
          line = ["Escape"]
        else:
          line = ["Abort"]          
        base_cycles = 0.0
        for array_value in array_values:
            lst = []
            glob_str = "%s/%s/tlb-%d.stats" % (results_dir, run, array_value)
            files = glob.glob(glob_str)
            if files == []:
                print "No files match: %s" % glob_str
                #continue
                cycles = 0.0
            for file in files:
                print file
                ruby_cycles = get_float_stat(file, "Ruby_cycles")
                lst.append(ruby_cycles)
            cycles = mfgraph.mean(lst)
            if (cycles == 0.0):
              tput = 0.0
            else:        
              tput = array_value * 32 * 1000000000 / (cycles * 1024 * 1024)
            #print cycles, tput
            line.append([array_value * 8, tput, tput, tput])
        data.append(line)

    graph_out = mfgraph.line_graph(data,
                                      ylabel = "Data Read (in MB) / sec",
                                      xlabel = "Benchmark Footprint (in KB)",
                                      label_fontsize = "9",
                                      xsize = 2.5,
                                      ysize = 2.3,
                                      line_thickness = 2.0,
                                      legend_x = "75",
                                      legend_y = "60",
                                      legend_fontsize = "10",
                                      marktype = ["diamond", "box", "triangle", "diamond"],
                                      #marksize = [0.4, 0.4, 0.4, 0.5],
                                      )
    
    jgraphs.append(graph_out)
    mfgraph.make_eps(graph_out, "tlb-access", ".")
    
def make_filters_line(jgraphs,benchmarks):
    array_values = [64, 128, 256, 512, 1024, 2048]
    data = []
    runs = ["Perfect", "NonCounting_0", "NonCounting_4", "Bulk"] 
    
    for benchmark in benchmarks:
      data = []      
      min_tput = 1000000
      cycles_base = 1
      for run in runs:
        line = []
        if (run == "NonCounting_0"):
          filter = "NonCounting"      
          line.append("BS")        
        elif (run == "NonCounting_1"):
          line.append("BS_1")        
          filter = "NonCounting"      
        elif (run == "NonCounting_2"):
          line.append("BS_2")        
          filter = "NonCounting"      
        elif (run == "NonCounting_4"):
          line.append("CBS")        
          filter = "NonCounting"      
        elif (run == "Bulk"):
          line.append("DBS")        
          filter = "Bulk"      
        elif (run == "Perfect"):
          line.append("P")        
          filter = "Perfect"      
        for array_value in array_values:
            print benchmark, run, array_value
            lst = []
            lst_read_false_positives = []
            lst_write_false_positives = []
            lst_false_positives = []
            if (run == "NonCounting_0"):
              glob_str = "%s/%s-TM-%s/*%s_%d_0*.stats" % (results_dir, benchmark[1], benchmark[2], filter, array_value)
            elif (run == "NonCounting_4"):
              glob_str = "%s/%s-TM-%s/*%s_%d_4*.stats" % (results_dir, benchmark[1], benchmark[2], filter, array_value)
            elif (run == "Bulk"):  
              glob_str = "%s/%s-TM-%s/*%s_%d*.stats" % (results_dir, benchmark[1], benchmark[2], filter, array_value)
            elif (run == "Perfect"):  
              glob_str = "%s/%s-TM-%s/*%s*.stats" % (results_dir, benchmark[1], benchmark[2], filter)
            files = glob.glob(glob_str)
            if files == []:
                print "No files match: %s" % glob_str
                continue
            for file in files:
                #print file
                ruby_cycles = get_float_stat(file, "Ruby_cycles")
                lst.append(ruby_cycles)
                if (run != "Perfect"): 
                  read_match = get_stat(file, "Total read set matches")
                  read_fp = get_stat(file, "Total read set false positives")
                  write_match = get_stat(file, "Total write set matches")
                  write_fp = get_stat(file, "Total write set false positives")
                  lst_read_false_positives.append((float)(read_fp)/(read_fp+read_match))
                  lst_write_false_positives.append((float)(write_fp)/(write_fp+write_match))
                  lst_false_positives.append((float)(read_fp+write_fp)/(read_fp+write_fp+read_match+write_match))
            if (run == "Perfect"):
              cycles_base = mfgraph.mean(lst)  
              print cycles_base              
            cycles = float(mfgraph.mean(lst))
            print cycles, cycles_base
            print "read_fp: ", mfgraph.mean(lst_read_false_positives), "write_fp: ", mfgraph.mean(lst_write_false_positives), "fp: ", mfgraph.mean(lst_false_positives)
            conf = mfgraph.confidence_interval_95_percent(lst)
            tput = 0
            tput_low = 0
            tput_high = 0
            #if (benchmark == "db-TM-128ops"): 
            #  tput = 128 * 1000000000 / cycles
            #  tput_low = 128 * 1000000000 / (cycles + conf)
            #  tput_high = 128 * 1000000000 / (cycles - conf)
            #elif (benchmark == "raytrace-TM-teapot"): 
            #  tput = 1 * 1000000000 / cycles
            #  tput_low = 1 * 1000000000 / (cycles + conf)
            #  tput_high = 1 * 1000000000 / (cycles - conf)
            #if (tput < min_tput):
            #  min_tput = tput        
            line.append([array_value, float(cycles_base) / cycles, float(cycles_base) / (cycles-conf), float(cycles_base) / (cycles+conf)])
        data.append(line)
      
      graph_title = benchmark[0]
      graph_out = mfgraph.line_graph(data,
                                      title = graph_title,  
                                      title_fontsize = "14",
                                      ylabel = "Speedup",
                                      xlabel = "Signature Size(in bits)",
                                      label_fontsize = "9",
                                      ymin = 0.5,
                                      xlog = 2,
                                      xsize = 3.0,
                                      ysize = 3.0,
                                      line_thickness = 2.0,
                                      legend_x = "75",
                                      legend_y = "60",
                                      legend_fontsize = "10",
                                      marktype = ["diamond", "box", "triangle", "diamond"],
                                      #marksize = [0.4, 0.4, 0.4, 0.5],
                                      )
    
      jgraphs.append(graph_out)
      mfgraph.make_eps(graph_out, "32bit_filter-size-%s" % benchmark[0], "filters")

def make_filters_speedup_graph(jgraphs, pairs):

    bars = []
    proc_value = 16
    smt_threads = 1
    synch_types = ['Lock_MCS', 'TM']
    filters = ['Perfect', 'NonCounting_2048_0', 'NonCounting_2048_4', 'Bulk_2048', 'NonCounting_64_0']
    bar_seg_labels = ['LOCK', 'Perfect', 'BS', 'CBS', 'DBS','BS_64']
    for pair in pairs:
        bar = [pair[0]]
        cycles_base = 0
        for synch in synch_types:
          for filter in filters:
            #bar = []
            lst = []
            protocol = "EagerCD_EagerVM_Base"
            if (synch == 'Lock_MCS' and filter != 'Perfect'):
              continue        
            glob_str = "%s/%s-%s-%s/%s-*%dp*-%dt-default-%s-*-%s*.stats" % (results_dir, pair[1], synch, pair[2], pair[1], proc_value, smt_threads, protocol, filter)
            files = glob.glob(glob_str)
            if files == []:
                print "%s not found" % glob_str
                #continue
            else:
              for file in files:
                lst.append(get_int_stat(file, "Ruby_cycles"))
              if (synch == 'Lock_MCS'):    
                cycles_base = mfgraph.mean(lst)
              elif (synch == 'TM' and filter == 'Perfect' and cycles_base == 0):    
                cycles_base = mfgraph.mean(lst)

              cycles_test = mfgraph.mean(lst)
              cycles_conf = mfgraph.confidence_interval_95_percent(lst)
           
            if (cycles_test == 0):
              cycles_test = 10000000000
              cycles_conf = 0        
                        
            norm_exe = float(cycles_base)/float(cycles_test)
            print "norm_exe = %f" % norm_exe
            
            bar.append(["", [norm_exe, float(cycles_base)/(cycles_test + cycles_conf), float(cycles_base)/(cycles_test - cycles_conf)]])
        bars.append(bar)

    jgraphs.append(mfgraph.stacked_bar_graph(bars,
                                             bar_segment_labels = bar_seg_labels,
                                             xsize = 8.5,
                                             ))

    # LUKE - submission orig values
    x_size = 8.5
    y_size = 1.778
    labelfontsize = 8
    barnamesize = 5
    stackfontsize = 5

    # for the SMT graph
    smt_xsize = 15
    smt_ysize = 7
    smt_labelfontsize = "30"
    smt_barnamesize = "30"
    smt_stacknamefontsize = "30"
    
    graph_out = mfgraph.stacked_bar_graph(bars,
                                          title = "",
                                          bar_segment_labels = bar_seg_labels,
                                          ylabel = "Speedup ",
                                          ymin = 0,
                                          #xsize = 15,
                                          #ysize = 7,
                                          title_fontsize = "9",
                                          #label_fontsize = "30",
                                          #bar_name_font_size = "14",
                                          legend_fontsize = "9",
                                          stack_name_font_size = "9",
                                          stack_name_location = 3,
                                          bar_space = 1.1,
                                          colors = ["1 0 1",
                                                    ".8 .8 .8",
                                                    ".6 .6 .6",
                                                    ".4 .4 .4",
                                                    "0 0 0"])
    mfgraph.make_eps(graph_out, "filters_norm_SMT", "filters")

def make_filters_speedup_graph2(jgraphs, pairs):

    bars = []
    abort_bars = []
    trans_bars = []
    nontrans_bars = []
    stall_trans_bars = []
    stall_nontrans_bars = []
    escape_trans_bars = []
    escape_nontrans_bars = []
      
    proc_value = 16
    smt_threads = 1
    synch_types = ['Lock', 'TM']
    filters = ['Perfect', 'NonCounting_2048_0', 'NonCounting_2048_4', 'Bulk_2048', 'NonCounting_64_0']
    for pair in pairs:
        bar = [pair[0]]
        abort_bar = [pair[0]]
        trans_bar = [pair[0]]
        nontrans_bar = [pair[0]]
        stall_trans_bar = [pair[0]]
        stall_nontrans_bar = [pair[0]]
        escape_trans_bar = [pair[0]]
        escape_nontrans_bar = [pair[0]]
        
        cycles_base = 0
        for synch in synch_types:
          for filter in filters:
            #bar = []
            lst = []
            lst2 = []
            lst_aborts = []
            lst2_aborts = []
            trans_cycles = []
            nontrans_cycles = []
            stall_trans_cycles = []
            stall_nontrans_cycles = []
            escape_trans_cycles = []
            escape_nontrans_cycles = []
            protocol = "MESI_CMP_filter_directory_reduce"
            if (synch == 'Lock' and filter != 'Perfect'):
              continue        
            glob_str = "%s/%s-%s-%s/%s-*%dp*-%dt-default-%s-*-%s*.stats" % (results_dir, pair[1], synch, pair[2], pair[1], proc_value, smt_threads, protocol, filter)
            files = glob.glob(glob_str)
            if files == []:
                print "%s not found" % glob_str
                continue
            for file in files:
                lst.append(get_int_stat(file, "Ruby_cycles"))
                lst_aborts.append(get_int_stat(file, "xact_aborts"))
                # get cycle breakdown
                trans_cycles.append(get_int_stat(file, "xact_system_trans_cycles"))
                nontrans_cycles.append(get_int_stat(file, "xact_system_nontrans_cycles"))
                stall_trans_cycles.append(get_int_stat(file, "xact_system_stall_trans_cycles"))
                stall_nontrans_cycles.append(get_int_stat(file, "xact_system_stall_nontrans_cycles"))
                escape_trans_cycles.append(get_int_stat(file, "xact_system_escape_trans_cycles"))
                escape_nontrans_cycles.append(get_int_stat(file, "xact_system_escape_nontrans_cycles"))

            # normalize to perfect
            if (filter == 'Perfect'):    
                cycles_base = mfgraph.mean(lst)
                trans_cycles_base = mfgraph.mean(trans_cycles)
                nontrans_cycles_base = mfgraph.mean(nontrans_cycles)
                stall_trans_cycles_base = mfgraph.mean(stall_trans_cycles)
                stall_nontrans_cycles_base = mfgraph.mean(stall_nontrans_cycles)
                escape_trans_cycles_base = mfgraph.mean(escape_trans_cycles)
                escape_nontrans_cycles_base = mfgraph.mean(escape_nontrans_cycles)
                
                trans_cycles_base_conf = mfgraph.confidence_interval_95_percent(trans_cycles)
                nontrans_cycles_base_conf = mfgraph.confidence_interval_95_percent(nontrans_cycles)
                stall_trans_cycles_base_conf = mfgraph.confidence_interval_95_percent(stall_trans_cycles)
                stall_nontrans_cycles_base_conf = mfgraph.confidence_interval_95_percent(stall_nontrans_cycles)
                escape_trans_cycles_base_conf = mfgraph.confidence_interval_95_percent(escape_trans_cycles)
                escape_nontrans_cycles_base_conf = mfgraph.confidence_interval_95_percent(escape_nontrans_cycles)

            trans_cycles_dir = mfgraph.mean(trans_cycles)
            nontrans_cycles_dir = mfgraph.mean(nontrans_cycles)
            stall_trans_cycles_dir = mfgraph.mean(stall_trans_cycles)
            stall_nontrans_cycles_dir = mfgraph.mean(stall_nontrans_cycles)
            escape_trans_cycles_dir = mfgraph.mean(escape_trans_cycles)
            escape_nontrans_cycles_dir = mfgraph.mean(escape_nontrans_cycles)
            
            trans_cycles_dir_conf = mfgraph.confidence_interval_95_percent(trans_cycles)
            nontrans_cycles_dir_conf = mfgraph.confidence_interval_95_percent(nontrans_cycles)
            stall_trans_cycles_dir_conf = mfgraph.confidence_interval_95_percent(stall_trans_cycles)
            stall_nontrans_cycles_dir_conf = mfgraph.confidence_interval_95_percent(stall_nontrans_cycles)
            escape_trans_cycles_dir_conf = mfgraph.confidence_interval_95_percent(escape_trans_cycles)
            escape_nontrans_cycles_dir_conf = mfgraph.confidence_interval_95_percent(escape_nontrans_cycles)



            # do the same for broadcast protocol
            trans_cycles = []
            nontrans_cycles = []
            stall_trans_cycles = []
            stall_nontrans_cycles = []
            escape_trans_cycles = []
            escape_nontrans_cycles = []
            protocol = "MESI_CMP_filter_bcast"
            glob_str = "%s/%s-%s-%s/%s-*%dp*-%dt-default-%s-*-%s*.stats" % (results_dir, pair[1], synch, pair[2], pair[1], proc_value, smt_threads, protocol, filter)
            files = glob.glob(glob_str)
            if files == []:
                print "%s not found" % glob_str
                continue
            for file in files:
                lst2.append(get_int_stat(file, "Ruby_cycles"))
                lst2_aborts.append(get_int_stat(file, "xact_aborts"))
                # get cycle breakdown
                trans_cycles.append(get_int_stat(file, "xact_system_trans_cycles"))
                nontrans_cycles.append(get_int_stat(file, "xact_system_nontrans_cycles"))
                stall_trans_cycles.append(get_int_stat(file, "xact_system_stall_trans_cycles"))
                stall_nontrans_cycles.append(get_int_stat(file, "xact_system_stall_nontrans_cycles"))
                escape_trans_cycles.append(get_int_stat(file, "xact_system_escape_trans_cycles"))
                escape_nontrans_cycles.append(get_int_stat(file, "xact_system_escape_nontrans_cycles"))


            cycles_test = mfgraph.mean(lst)
            cycles_test2 = mfgraph.mean(lst2)
            aborts_test = mfgraph.mean(lst_aborts)
            aborts_test2 = mfgraph.mean(lst2_aborts)
            trans_cycles_bcast = mfgraph.mean(trans_cycles)
            nontrans_cycles_bcast = mfgraph.mean(nontrans_cycles)
            stall_trans_cycles_bcast = mfgraph.mean(stall_trans_cycles)
            stall_nontrans_cycles_bcast = mfgraph.mean(stall_nontrans_cycles)
            escape_trans_cycles_bcast = mfgraph.mean(escape_trans_cycles)
            escape_nontrans_cycles_bcast = mfgraph.mean(escape_nontrans_cycles)
            
            cycles_conf = mfgraph.confidence_interval_95_percent(lst)
            cycles_conf2 = mfgraph.confidence_interval_95_percent(lst2)
            aborts_conf = mfgraph.confidence_interval_95_percent(lst_aborts)
            aborts_conf2 = mfgraph.confidence_interval_95_percent(lst2_aborts)
            trans_cycles_conf = mfgraph.confidence_interval_95_percent(trans_cycles)
            nontrans_cycles_conf = mfgraph.confidence_interval_95_percent(nontrans_cycles)
            stall_trans_cycles_conf = mfgraph.confidence_interval_95_percent(stall_trans_cycles)
            stall_nontrans_cycles_conf = mfgraph.confidence_interval_95_percent(stall_nontrans_cycles)
            escape_trans_cycles_conf = mfgraph.confidence_interval_95_percent(escape_trans_cycles)
            escape_nontrans_cycles_conf = mfgraph.confidence_interval_95_percent(escape_nontrans_cycles)

            if (cycles_test == 0):
              cycles_test = 1
              cycles_conf = 0        

            if(filter == 'Perfect'):
                base_aborts = aborts_test

            norm_exe = float(cycles_base)/float(cycles_test)
            norm_exe2 = float(cycles_base)/float(cycles_test2)

            # prevents division by zero
            if(synch == 'Lock'):
                base_aborts = 1
                trans_cycles_base = 1
                nontrans_cycles_base = 1
                stall_trans_cycles_base = 1
                stall_nontrans_cycles_base = 1
                escape_trans_cycles_base = 1
                escape_nontrans_cycles_base = 1

                
            norm_aborts = float(aborts_test)/float(base_aborts)
            norm_aborts2 = float(aborts_test2)/float(base_aborts)

            # for directory
            norm_trans_cycles_dir = float(trans_cycles_dir)/float(trans_cycles_base)
            norm_nontrans_cycles_dir = float(nontrans_cycles_dir)/float(nontrans_cycles_base)
            norm_stall_trans_cycles_dir = float(stall_trans_cycles_dir)/float(stall_trans_cycles_base)
            norm_stall_nontrans_cycles_dir = float(stall_nontrans_cycles_dir)/float(stall_nontrans_cycles_base)
            norm_escape_trans_cycles_dir = float(escape_trans_cycles_dir)/float(escape_trans_cycles_base)
            norm_escape_nontrans_cycles_dir = float(escape_nontrans_cycles_dir)/float(escape_nontrans_cycles_base)
            # for bcast
            norm_trans_cycles = float(trans_cycles_bcast)/float(trans_cycles_base)
            norm_nontrans_cycles = float(nontrans_cycles_bcast)/float(nontrans_cycles_base)
            norm_stall_trans_cycles = float(stall_trans_cycles_bcast)/float(stall_trans_cycles_base)
            norm_stall_nontrans_cycles = float(stall_nontrans_cycles_bcast)/float(stall_nontrans_cycles_base)
            norm_escape_trans_cycles = float(escape_trans_cycles_bcast)/float(escape_trans_cycles_base)
            norm_escape_nontrans_cycles = float(escape_nontrans_cycles_bcast)/float(escape_nontrans_cycles_base)
            #print "norm_aborts %d" % aborts_test
            #print "norm_aborts2 %d" % aborts_test2

            if(synch == 'Lock'):
                label = "Lock"
            elif(filter == 'Perfect'):
                label = "P"
                bcast_label = "P_b"
            elif(filter == 'NonCounting_2048_0'):
                label = "BS"
                bcast_label = "BS_b"
            elif(filter == 'NonCounting_2048_4'):
                label = "CBS"
                bcast_label = "CBS_b"
            elif(filter == 'NonCounting_64_0'):
                label = "BS_64"
                bcast_label = "BS_64_b"
            elif(filter == 'Bulk_2048'):
                label = "DBS"
                bcast_label = "DBS_b"


            if (synch == 'Lock'):
              #bar.append(["Lock", [norm_exe, float(cycles_base)/(cycles_test + cycles_conf), float(cycles_base)/(cycles_test - cycles_conf)]])
              print "Lock"
            else:
                bar.append([label, [norm_exe, float(cycles_base)/(cycles_test + cycles_conf), float(cycles_base)/(cycles_test - cycles_conf)]])
                bar.append([bcast_label, [norm_exe2, float(cycles_base)/(cycles_test2 + cycles_conf2), float(cycles_base)/(cycles_test2 - cycles_conf2)]])
                abort_bar.append([label, [norm_aborts, float(aborts_test+aborts_conf)/(base_aborts), float(aborts_test-aborts_conf)/(base_aborts)]])
                abort_bar.append([bcast_label, [norm_aborts2, float(aborts_test2+aborts_conf2)/(base_aborts), float(aborts_test2-aborts_conf2)/(base_aborts)]])

                trans_bar.append([label, [norm_trans_cycles_dir, float(trans_cycles_dir+trans_cycles_dir_conf)/(trans_cycles_base), float(trans_cycles_dir-trans_cycles_dir_conf)/(trans_cycles_base)]])
                trans_bar.append([bcast_label, [norm_trans_cycles, float(trans_cycles_bcast+trans_cycles_conf)/(trans_cycles_base), float(trans_cycles_bcast-trans_cycles_conf)/(trans_cycles_base)]])
                
                nontrans_bar.append([label, [norm_nontrans_cycles_dir, float(nontrans_cycles_dir+nontrans_cycles_dir_conf)/(nontrans_cycles_base), float(nontrans_cycles_dir-nontrans_cycles_dir_conf)/(nontrans_cycles_base)]])
                nontrans_bar.append([bcast_label, [norm_nontrans_cycles, float(nontrans_cycles_bcast+nontrans_cycles_conf)/(nontrans_cycles_base), float(nontrans_cycles_bcast-nontrans_cycles_conf)/(nontrans_cycles_base)]])
                
                stall_trans_bar.append([label, [norm_stall_trans_cycles_dir, float(stall_trans_cycles_dir+stall_trans_cycles_dir_conf)/(stall_trans_cycles_base), float(stall_trans_cycles_dir-stall_trans_cycles_dir_conf)/(stall_trans_cycles_base)]])
                stall_trans_bar.append([bcast_label, [norm_stall_trans_cycles, float(stall_trans_cycles_bcast+stall_trans_cycles_conf)/(stall_trans_cycles_base), float(stall_trans_cycles_bcast-stall_trans_cycles_conf)/(stall_trans_cycles_base)]])

                stall_nontrans_bar.append([label, [norm_stall_nontrans_cycles_dir, float(stall_nontrans_cycles_dir+stall_nontrans_cycles_dir_conf)/(stall_nontrans_cycles_base), float(stall_nontrans_cycles_dir-stall_nontrans_cycles_dir_conf)/(stall_nontrans_cycles_base)]])
                stall_nontrans_bar.append([bcast_label, [norm_stall_nontrans_cycles, float(stall_nontrans_cycles_bcast+stall_nontrans_cycles_conf)/(stall_nontrans_cycles_base), float(stall_nontrans_cycles_bcast-stall_nontrans_cycles_conf)/(stall_nontrans_cycles_base)]])


                escape_trans_bar.append([label, [norm_escape_trans_cycles_dir, float(escape_trans_cycles_dir+escape_trans_cycles_dir_conf)/(escape_trans_cycles_base), float(escape_trans_cycles_dir-escape_trans_cycles_dir_conf)/(escape_trans_cycles_base)]])
                escape_trans_bar.append([bcast_label, [norm_escape_trans_cycles, float(escape_trans_cycles_bcast+escape_trans_cycles_conf)/(escape_trans_cycles_base), float(escape_trans_cycles_bcast-escape_trans_cycles_conf)/(escape_trans_cycles_base)]])

                escape_nontrans_bar.append([label, [norm_escape_nontrans_cycles_dir, float(escape_nontrans_cycles_dir+escape_nontrans_cycles_dir_conf)/(escape_nontrans_cycles_base), float(escape_nontrans_cycles_dir-escape_nontrans_cycles_dir_conf)/(escape_nontrans_cycles_base)]])
                escape_nontrans_bar.append([bcast_label, [norm_escape_nontrans_cycles, float(escape_nontrans_cycles_bcast+escape_nontrans_cycles_conf)/(escape_nontrans_cycles_base), float(escape_nontrans_cycles_bcast-escape_nontrans_cycles_conf)/(escape_nontrans_cycles_base)]])


        bars.append(bar)
        abort_bars.append(abort_bar)
        trans_bars.append(trans_bar)
        nontrans_bars.append(nontrans_bar)
        stall_trans_bars.append(stall_trans_bar)
        stall_nontrans_bars.append(stall_nontrans_bar)
        escape_trans_bars.append(escape_trans_bar)
        escape_nontrans_bars.append(escape_nontrans_bar)
        

    jgraphs.append(mfgraph.stacked_bar_graph(bars,
                                             bar_segment_labels = ["runtime"],
                                             xsize = 8.5,
                                             ))

    # LUKE - orig values
    x_size = 8.5
    y_size = 1.778
    labelfontsize = 8
    barnamesize = 5
    stackfontsize = 5
    graph_out = mfgraph.stacked_bar_graph(bars,
                                          title = "",
                                          bar_segment_labels = ["Execution Time"],
                                          ylabel = "Speedup ",
                                          ymin = 0,
                                          ymax = 1.2,
                                          xsize = x_size,
                                          ysize = y_size,
                                          title_fontsize = "12",
                                          label_fontsize = "8",
                                          bar_name_font_size = "5",
                                          legend_fontsize = "5",
                                          stack_name_font_size = "5",
                                          stack_name_location = 9,
                                          bar_space = 1.2,
                                          colors = ["1 0 1",
                                                    ".8 .8 .8",
                                                    ".6 .6 .6",
                                                    ".4 .4 .4",
                                                    "0 0 0"])
    graph_out2 = mfgraph.stacked_bar_graph(abort_bars,
                                          title = "",
                                          bar_segment_labels = ["Execution Time"],
                                          ylabel = "Aborts ",
                                          ymin = 0,
                                          ymax = 5,
                                          xsize = x_size,
                                          ysize = y_size,
                                          title_fontsize = "12",
                                          label_fontsize = "8",
                                          bar_name_font_size = "5",
                                          legend_fontsize = "5",
                                          stack_name_font_size = "5",
                                          stack_name_location = 9,
                                          bar_space = 1.2,
                                          colors = ["1 0 1",
                                                    ".8 .8 .8",
                                                    ".6 .6 .6",
                                                    ".4 .4 .4",
                                                    "0 0 0"])
    mfgraph.make_eps(graph_out, "filters_norm_SMT", "filters")

    graph_out3 = mfgraph.stacked_bar_graph(trans_bars,
                                           title = "",
                                          bar_segment_labels = ["Execution Time"],
                                          ylabel = "Trans Cycles ",
                                          ymin = 0,
                                          ymax = 1.3,
                                          xsize = x_size,
                                          ysize = y_size,
                                          title_fontsize = "12",
                                          label_fontsize = "8",
                                          bar_name_font_size = "5",
                                          legend_fontsize = "5",
                                          stack_name_font_size = "5",
                                          stack_name_location = 9,
                                          bar_space = 1.2,
                                          colors = ["1 0 1",
                                                    ".8 .8 .8",
                                                    ".6 .6 .6",
                                                    ".4 .4 .4",
                                                    "0 0 0"])

    graph_out4 = mfgraph.stacked_bar_graph(nontrans_bars,
                                           title = "",
                                          bar_segment_labels = ["Execution Time"],
                                          ylabel = "NonTrans Cycles ",
                                          ymin = 0,
                                          ymax = 1.3,
                                          xsize = x_size,
                                          ysize = y_size,
                                          title_fontsize = "12",
                                          label_fontsize = "8",
                                          bar_name_font_size = "5",
                                          legend_fontsize = "5",
                                          stack_name_font_size = "5",
                                          stack_name_location = 9,
                                          bar_space = 1.2,
                                          colors = ["1 0 1",
                                                    ".8 .8 .8",
                                                    ".6 .6 .6",
                                                    ".4 .4 .4",
                                                    "0 0 0"])

    graph_out5 = mfgraph.stacked_bar_graph(stall_trans_bars,
                                           title = "",
                                          bar_segment_labels = ["Execution Time"],
                                          ylabel = "Stall Trans Cycles ",
                                          ymin = 0,
                                          ymax = 10,
                                          xsize = x_size,
                                          ysize = y_size,
                                          title_fontsize = "12",
                                          label_fontsize = "8",
                                          bar_name_font_size = "5",
                                          legend_fontsize = "5",
                                          stack_name_font_size = "5",
                                          stack_name_location = 9,
                                          bar_space = 1.2,
                                          colors = ["1 0 1",
                                                    ".8 .8 .8",
                                                    ".6 .6 .6",
                                                    ".4 .4 .4",
                                                    "0 0 0"])

    graph_out6 = mfgraph.stacked_bar_graph(stall_nontrans_bars,
                                           title = "",
                                          bar_segment_labels = ["Execution Time"],
                                          ylabel = "Stall NonTrans Cycles ",
                                          ymin = 0,
                                          ymax = 10,
                                          xsize = x_size,
                                          ysize = y_size,
                                          title_fontsize = "12",
                                          label_fontsize = "8",
                                          bar_name_font_size = "5",
                                          legend_fontsize = "5",
                                          stack_name_font_size = "5",
                                          stack_name_location = 9,
                                          bar_space = 1.2,
                                          colors = ["1 0 1",
                                                    ".8 .8 .8",
                                                    ".6 .6 .6",
                                                    ".4 .4 .4",
                                                    "0 0 0"])

    graph_out7 = mfgraph.stacked_bar_graph(escape_trans_bars,
                                           title = "",
                                          bar_segment_labels = ["Execution Time"],
                                          ylabel = "Escape Trans Cycles ",
                                          ymin = 0,
                                          ymax = 3,
                                          xsize = x_size,
                                          ysize = y_size,
                                          title_fontsize = "12",
                                          label_fontsize = "8",
                                          bar_name_font_size = "5",
                                          legend_fontsize = "5",
                                          stack_name_font_size = "5",
                                          stack_name_location = 9,
                                          bar_space = 1.2,
                                          colors = ["1 0 1",
                                                    ".8 .8 .8",
                                                    ".6 .6 .6",
                                                    ".4 .4 .4",
                                                    "0 0 0"])


    graph_out8 = mfgraph.stacked_bar_graph(escape_nontrans_bars,
                                           title = "",
                                          bar_segment_labels = ["Execution Time"],
                                          ylabel = "Escape NonTrans Cycles ",
                                          ymin = 0,
                                          ymax = 1.5,
                                          xsize = x_size,
                                          ysize = y_size,
                                          title_fontsize = "12",
                                          label_fontsize = "8",
                                          bar_name_font_size = "5",
                                          legend_fontsize = "5",
                                          stack_name_font_size = "5",
                                          stack_name_location = 9,
                                          bar_space = 1.2,
                                          colors = ["1 0 1",
                                                    ".8 .8 .8",
                                                    ".6 .6 .6",
                                                    ".4 .4 .4",
                                                    "0 0 0"])




    mfgraph.make_eps(graph_out, "filters_norm_speedup", "filters")
    mfgraph.make_eps(graph_out2, "filters_norm_aborts", "filters")
    mfgraph.make_eps(graph_out3, "filters_norm_trans", "filters")
    mfgraph.make_eps(graph_out4, "filters_norm_nontrans", "filters")
    mfgraph.make_eps(graph_out5, "filters_norm_stall_trans", "filters")
    mfgraph.make_eps(graph_out6, "filters_norm_stall_nontrans", "filters")
    mfgraph.make_eps(graph_out7, "filters_norm_escape_trans", "filters")
    mfgraph.make_eps(graph_out8, "filters_norm_escape_nontrans", "filters")

def make_contention_perf_bars(jgraphs, benchmark_set, protocols, processors):

    bars = []
    benchmark_set_name = benchmark_set[0];
    benchmarks = benchmark_set[1];
    bar_seg_labels = [];
    
    if (benchmark_set_name == "SPLASH" and protocols[0] == "BEST"):
      bar_seg_labels.append("LOCK")
              
    for (pname, protocol) in protocols[1]:
      bar_seg_labels.append(pname);        

    for (name, benchmark, arg) in benchmarks:
        if (benchmark_set_name == "SPLASH" and protocols[0] == "BEST" and (name == "BTree" or name == "LFUCache")):
          continue            
        norm_cycles = 0   
        bar = [name]
        all_protocols = protocols[1]
        if (benchmark_set_name == "SPLASH" and protocols[0] == "BEST"):
          all_protocols = [("LOCK", "MESI_CMP_filter_directory")] + protocols[1]         
        #print name, all_protocols  
        for (pname, protocol) in all_protocols:		
            glob_str = "%s/%s-TM-%s/%s-TM-%s*-%dp-1t-default-%s-1nx*.stats" % (results_dir, benchmark, arg, benchmark, arg, processors, protocol)
            if (pname == "LOCK" and benchmark_set_name == "SPLASH"):
              glob_str = "%s/%s-Lock_MCS-%s/%s-Lock_MCS-%s-%dp-1t-default-%s-1nx*.stats" % (results_dir, benchmark, arg, benchmark, arg, processors, protocol)
            if (pname == "LOCK" and benchmark_set_name == "SERIAL"):
              glob_str = "%s/%s-Lock-%s/%s-Lock-%s-%dp-1t-default-%s-1nx*.stats" % (results_dir, benchmark, arg, benchmark, arg, processors, protocol)
            if (pname == "LOCK" and name == "BerkeleyDB"):
              glob_str = "%s/%s-Lock-%s/%s-Lock-%s-%dp-1t-default-%s-1nx*.stats" % (results_dir, benchmark, arg, benchmark, arg, processors, protocol)
            files = glob.glob(glob_str)
            lst = []
            mean_cycles = 0
            mean_conf = 0
            if files == []:
                print "%s not found" % glob_str
                mean_cycles = 10000000000
                mean_conf   = 0
            else: 
              for file in files:
                lst.append(get_int_stat(file, "Ruby_cycles"))
              mean_cycles = mfgraph.mean(lst)
              mean_conf   = mfgraph.confidence_interval_95_percent(lst)
            if (pname == "LOCK" and mean_cycles != 10000000000):
              norm_cycles = mean_cycles
            if ((pname == "EE") and (norm_cycles == 0)):
              norm_cycles = mean_cycles
            if ((pname == "EE_H") and (norm_cycles == 0)):
              norm_cycles = mean_cycles
            bar.append(["", [float(norm_cycles) / float(mean_cycles), float(norm_cycles) / (float(mean_cycles) + float(mean_conf)), float(norm_cycles) / (float(mean_cycles) - float(mean_conf))]])
        bars.append(bar)
    #print bars

    graph_out = mfgraph.stacked_bar_graph(bars,
                                          #title = "%s %dp %s" % (benchmark_set_name, processors, protocols[0]),
                                          #title = "%s %s" % (benchmark_set_name, protocols[0]) ,
                                          title = "",
                                          bar_segment_labels = bar_seg_labels,
                                          ylabel = "Speedup",
                                          #ymax = 1.50,
                                          ymin = 0,
                                          #xsize = 6.0,
                                          ysize = 2.778,
                                          title_fontsize = "12",
                                          label_fontsize = "12",
                                          bar_name_font_size = "7",
                                          legend = "on",
                                          legend_fontsize = "7",
                                          stack_name_font_size = "9",
                                          stack_name_location = 3,
                                          bar_space = 1.1,
                                          colors = ["0 0 0",
                                                    ".8 .8 .8",
                                                    ".9 .9 .9",
                                                    ".6 .6 .6",
                                                    "0 0 0"])
    mfgraph.make_eps(graph_out, "contention-%s-%dp-%s" % (benchmark_set_name, processors, protocols[0]), "contention")
    jgraphs.append(mfgraph.stacked_bar_graph(bars,
                                             title = "%s Contention %dp %s" % (benchmark_set_name, processors, protocols[0]),
											 ylabel = "Speedup",
                                             bar_segment_labels = bar_seg_labels,
                                             xsize = 5.0,
                                             bar_name_font_size = "5",
                                             stack_name_font_size = "9",
                                             ))

def spit_contention_perf_stats(jgraphs, benchmark_set, protocols, processors):

    bars = []
    benchmark_set_name = benchmark_set[0];
    benchmarks = benchmark_set[1];

    #motif_map_1_EE = {"Barnes":597410, "Cholesky":820965, "Mp3d":0, "Radiosity":1485012, "Raytrace":3603321, "BTree":1826109, "LFUCache":2409561}
    #motif_map_1_EE_H = {"Barnes":1878747, "Cholesky":415178, "Mp3d":7299175, "Radiosity":561152, "Raytrace":2160821, "BTree":69837, "LFUCache":598848}
    #motif_map_1_EL_T = {"Barnes":717325, "Cholesky":310164, "Mp3d":6494463, "Radiosity":1081659, "Raytrace":0, "BTree":32778, "LFUCache":8490987}
    #motif_map_4_LL = {"Barnes":2487785, "Cholesky":16793582, "Mp3d":151329956, "Radiosity":2215835, "Raytrace":425995115, "BTree":5378, "LFUCache":37712}
    #motif_map_4_LL_B = {"Barnes":2732545, "Cholesky":19870590, "Mp3d":93418402, "Radiosity":1021744, "Raytrace":1888602, "BTree":7058, "LFUCache":24846}

    motif_map_1_EE = {}
    motif_map_1_EE_H = {}
    motif_map_1_EL_T = {}
    motif_map_4_LL = {}
    motif_map_4_LL_B = {}
    
    for (name, benchmark, arg) in benchmarks:
        bar = [name]
        all_protocols = protocols[1]
        for (pname, protocol) in all_protocols:		
            glob_str = "%s/%s-TM-%s/%s-TM-%s*-%dp-1t-default-%s-1nx*.stats" % (results_dir, benchmark, arg, benchmark, arg, processors, protocol)
            files = glob.glob(glob_str)
            lst = []
            lst_good_stalls = []
            lst_bad_stalls = []
            lst_load_misses = []
            lst_store_misses = []
            lst_average_xact_time = []
            lst_xact_count = []
            lst_good_xact_time = []
            lst_backoff_time = []
            lst_aborting_time = []
            lst_load_set_size = []
            lst_store_set_size = []
            lst_aborts = []
            lst_stall_on_aborts = []
            lst_wasted_backoff_time = []
            lst_system_xact_time = []
            lst_total_stalls = []
            lst_total_xact_time = []
            lst_other_wasted = []
            if files == []:
				print "%s not found" % glob_str
				continue
            for file in files:
                lst.append(get_int_stat(file, "Ruby_cycles"))
                good_stalls = get_int_stat(file, "xact_system_good_stall")
                if (good_stalls != 0):
                  lst_good_stalls.append(good_stalls)
                bad_stalls = get_int_stat(file, "xact_system_bad_stall")  
                if (bad_stalls != 0):
                  lst_bad_stalls.append(bad_stalls)
                miss_load = get_average_stat(file, "xact_miss_load")   
                if (miss_load > 0):
                  lst_load_misses.append(miss_load)
                miss_store = get_average_stat(file, "xact_miss_store")   
                if (miss_store > 0):
                  lst_store_misses.append(miss_store)
                xact_time = get_average_stat(file, "xact_time_dist")
                if (xact_time > 0.0):
                  lst_average_xact_time.append(xact_time)
                lst_xact_count.append(get_count_stat(file, "xact_time_dist"))
                good_trans = get_int_stat(file, "xact_system_good_trans")
                if (good_trans > 0.0):
                  lst_good_xact_time.append(good_trans)
                backoff_time = get_int_stat(file, "xact_system_backoff")
                if (backoff_time > 0.0):
                  lst_backoff_time.append(backoff_time)
                aborting_time = get_int_stat(file, "xact_system_aborting")
                if (aborting_time >= 0.0):
                  lst_aborting_time.append(aborting_time)
                system_xact_time = get_int_stat(file, "xact_system_xact_time")
                if (system_xact_time >= 0.0):
                  lst_system_xact_time.append(system_xact_time)
                wasted_backoff_time = get_int_stat(file, "xact_system_wasted_backoff")
                if (wasted_backoff_time >= 0.0):
                  lst_wasted_backoff_time.append(wasted_backoff_time)
                other_wasted_time = get_int_stat(file, "xact_system_other_wasted")
                if (other_wasted_time >= 0.0):
                  lst_other_wasted.append(other_wasted_time)
                stall_on_aborts_time = get_int_stat(file, "xact_system_stallOnAbort")
                if (stall_on_aborts_time >= 0.0):
                  lst_stall_on_aborts.append(stall_on_aborts_time)
                lst_load_set_size.append(get_average_stat(file, "xact_read_set_size_dist"))
                lst_store_set_size.append(get_average_stat(file, "xact_write_set_size_dist"))
                lst_aborts.append(get_int_stat(file, "xact_aborts:")) 
                total_xact_time = get_int_stat(file, "xact_system_trans_cycles")
                total_stall_time = get_int_stat(file, "xact_system_stall_trans_cycles")
                if (total_xact_time >= 0.0):
                  lst_total_xact_time.append(total_xact_time)
                if (total_stall_time >= 0.0):
                  lst_total_stalls.append(total_stall_time)                   
            mean_cycles = mfgraph.mean(lst)
            mean_conf   = mfgraph.confidence_interval_95_percent(lst)
            mean_good_xact_time = mfgraph.mean(lst_good_xact_time)
            mean_good_stalls = mfgraph.mean(lst_good_stalls)
            mean_bad_stalls = mfgraph.mean(lst_bad_stalls)
            if (mean_bad_stalls == 0):
              mean_bad_stalls = 1        
            mean_xact_count = mfgraph.mean(lst_xact_count)
            mean_xact_time = mfgraph.mean(lst_average_xact_time)
            mean_system_xact_time = mfgraph.mean(lst_system_xact_time)
            if (mean_system_xact_time == 0):
              mean_system_xact_time = 10000000000        
            mean_xact_backoff_time = mfgraph.mean(lst_backoff_time)
            if (mean_xact_backoff_time == 0):
              mean_xact_backoff_time = 1000000000          
            mean_stall_on_aborts_time = mfgraph.mean(lst_stall_on_aborts)  
            mean_other_wasted_time = mfgraph.mean(lst_other_wasted)
            mean_total_stall_time = mfgraph.mean(lst_total_stalls)
            mean_total_xact_time = mfgraph.mean(lst_total_xact_time)
            mean_total_aborting_time = mfgraph.mean(lst_aborting_time)
            mean_total_backoff_time = mfgraph.mean(lst_backoff_time)
            mean_total_cycles = mean_cycles * (processors - 1)
            if (pname == "EL" or pname == "LL_B"):
              mean_total_backoff_time = mean_total_aborting_time        
              mean_total_aborting_time = 0
            mean_total_xact_activity = mean_total_xact_time + mean_total_stall_time + mean_total_aborting_time + mean_total_backoff_time
            #print name, pname
            #print " Concurrency Factor:         ", '%.4f' % (float(mean_good_xact_time) / mean_system_xact_time) 
            #print " Transaction Execution Time: ", '%.4f' % ((mean_xact_time * mean_xact_count)/ (mean_cycles * (processors - 1)))
            #print "    Work:        ", '%.4f' % (float(mean_good_xact_time) / (mean_xact_count * mean_xact_time))
            #print "    Good stalls: ", '%.4f' % (float(mean_good_stalls) / (mean_xact_count * mean_xact_time)), ' %.4f' % (float(mean_good_stalls) / mean_good_xact_time)
            #print "    Bad stalls:  ", '%.4f' % (float(mean_bad_stalls) / (mean_xact_count * mean_xact_time)),  ' %.4f' % (float(mean_bad_stalls) / mean_good_xact_time) 
            #print " Stall%:                     ", '%.4f' % (float(mean_bad_stalls + mean_good_stalls) / (mean_xact_count * mean_xact_time))
            #print " Good Stall % :              ", '%.4f' % (float(mean_good_stalls) / (float(mean_good_stalls) + float(mean_bad_stalls))) 
            #print " Stall On Abort % :          ", '%.4f' % (float(mean_stall_on_aborts_time) / (float(mean_good_stalls) + float(mean_bad_stalls))) 
            #print " Wasted backoff %:           ", '%.4f' % (float(mfgraph.mean(lst_wasted_backoff_time)) / mean_xact_backoff_time)
            #print " Aborting / Xact Work:       ", '%.4f' % (float(mfgraph.mean(lst_aborting_time)) / (mean_good_xact_time))
            #print " Backoff / Xact Work:        ", '%.4f' % (float(mfgraph.mean(lst_backoff_time))/(mean_good_xact_time))
            #print " Abort %:     ", '%.4f' % (float(mfgraph.mean(lst_aborts)) / mean_xact_count)
            #print " Read Set:    ", '%.4f' % mfgraph.mean(lst_load_set_size), " Write Set:    ", '%.4f' % mfgraph.mean(lst_store_set_size)
            #print " Load Misses: ", '%.4f' % mfgraph.mean(lst_load_misses),   " Store Misses: ", '%.4f' % mfgraph.mean(lst_store_misses)  
            #print " Transactional Activity:     ", '%.1f' % (100 * (mean_total_xact_activity / (mean_cycles * (processors - 1))))
            #print " Useful Work:                ", '%.1f' % (100 * (mean_good_xact_time / mean_total_xact_activity))
            #print " Total Stalls:               ", '%.1f' % (100 * (mean_total_stall_time / mean_total_xact_activity))
            #print " Wasted Work:                ", '%.1f' % (100 * ((mean_total_xact_time - mean_good_xact_time ) / mean_total_xact_activity))
            #print " Total Backoff:              ", '%.1f' % (100 * (mean_total_backoff_time / mean_total_xact_activity))     
            #print " Total Aborting:             ", '%.1f' % (100 * (mean_total_aborting_time / mean_total_xact_activity))     
            #print name, pname, "CF: ", '%.2f' % (float(mean_good_xact_time) / mean_system_xact_time), " TA: ", '%.1f' % (100 * (mean_total_xact_activity / (mean_cycles * (processors - 1))))," $: ", '%.1f' % (100 * (mean_good_xact_time / mean_total_xact_activity))," |: ", '%.1f' % (100 * (mean_total_stall_time / mean_total_xact_activity)), " .X: ", '%.1f' % (100 * ((mean_total_xact_time - mean_good_xact_time + mean_total_aborting_time) / mean_total_xact_activity)), " O: ", '%.1f' % (100 * (mean_total_backoff_time / mean_total_xact_activity))     
            print name, pname, "CF: ", '%.2f' % (float(mean_good_xact_time) / mean_system_xact_time), " TA: ", '%.1f' % (100 * (mean_total_xact_activity / (mean_cycles * (processors - 1))))," $: ", '%.1f' % (100 * (mean_good_xact_time / mean_total_cycles))," |: ", '%.1f' % (100 * (mean_total_stall_time / mean_total_cycles)), " .X: ", '%.1f' % (100 * ((mean_total_xact_time - mean_good_xact_time + mean_total_aborting_time) / mean_total_cycles)), " O: ", '%.1f' % (100 * (mean_total_backoff_time / mean_total_cycles))     
            print name, pname, "TOTAL XACT ACTIVITY: ", mean_total_xact_activity, " TOTAL CYCLES: ", mean_total_cycles 
            #print " Other Wasted:             ", '%.1f' % (100 * (mean_other_wasted_time / mean_total_xact_activity))     
            if pname == "EE":
              if motif_map_1_EE.has_key(name):
                print " Motif1:      ", '%.4f' % (float(motif_map_1_EE[name]) / mean_good_xact_time) 
            if pname == "EE_H":
              if motif_map_1_EE_H.has_key(name):
                print " Motif1:      ", '%.4f' % (float(motif_map_1_EE_H[name]) / mean_good_xact_time) 
            if pname == "EL_T":
              if motif_map_1_EL_T.has_key(name):
                print " Motif1:      ", '%.4f' % (float(motif_map_1_EL_T[name]) / mean_good_xact_time) 
            if pname == "LL":
              if motif_map_4_LL.has_key(name):
                print " Motif4:      ", '%.4f' % (float(motif_map_4_LL[name]) / mean_good_xact_time) 
            if pname == "LL_B":
              if motif_map_4_LL_B.has_key(name):
                print " Motif4:      ", '%.4f' % (float(motif_map_4_LL_B[name]) / mean_good_xact_time) 
            print ""


def make_execution_time_breakdown(jgraphs, benchmark_set, protocols, processors, predictor_config):
    bars = []
    cycle_bars = []
    benchmark_set_name = benchmark_set[0];
    benchmarks = benchmark_set[1];
    
    for (name, benchmark, arg) in benchmarks:
        bar = [name]
        cycle_bar = [name]
        all_protocols = protocols[1]
        tuple_list = []
        for (pname, protocol) in all_protocols:
            glob_str="%s/%s-TM-%s/%s-TM-%s*-%dp-1t-default-%s*.stats" % (results_dir, benchmark, arg, benchmark, arg, processors, protocol)
            #    glob_str = "%s/%s-TM-%s/%s/%s-TM-%s*-%dp-1t-default-%s*.stats" % (results_dir, benchmark, arg, predictor_config, benchmark, arg, processors, protocol)
            files = glob.glob(glob_str)
            lst_cycles = []
            lst_non_trans_cycles = []
            lst_good_trans_cycles = []
            lst_commiting_cycles  = []
            lst_aborting_cycles   = []
            lst_backoff_cycles    = []
            lst_barrier_cycles    = []
            lst_stall_cycles      = []
            
            lst_abort_on_commit_cycles = []
            lst_abort_on_abort_cycles  = []
            lst_commited_stall_on_trans_commit = []
            lst_commited_stall_on_trans_abort  = []
            lst_commited_stall_on_stall_commit = []
            lst_commited_stall_on_stall_abort  = []
            lst_commited_stall_on_commiting    = []
            lst_commited_stall_on_aborting     = []
            lst_aborted_stall_on_trans_commit = []
            lst_aborted_stall_on_trans_abort  = []
            lst_aborted_stall_on_stall_commit = []
            lst_aborted_stall_on_stall_abort  = []
            lst_aborted_stall_on_commiting    = []
            lst_aborted_stall_on_aborting     = []

            lst_aborts_per_conflict           = []

            if files == []:
				print "%s not found" % glob_str
				continue
            for file in files:
                lst_cycles.append(get_int_stat(file, "Ruby_cycles"))

                lst_non_trans_cycles.append(get_int_stat(file, " XACT_BREAKDOWN_NON_TRANS_CYCLES"))
                lst_good_trans_cycles.append(get_int_stat(file, " XACT_BREAKDOWN_GOOD_TRANS_CYCLES"))
                lst_commiting_cycles.append(get_int_stat(file, " XACT_BREAKDOWN_COMMITING_CYCLES"))
                lst_aborting_cycles.append(get_int_stat(file, " XACT_BREAKDOWN_ABORTING_CYCLES"))
                lst_backoff_cycles.append(get_int_stat(file, " XACT_BREAKDOWN_BACKOFF_CYCLES"))
                lst_barrier_cycles.append(get_int_stat(file, " XACT_BREAKDOWN_BARRIER_CYCLES"))
                lst_stall_cycles.append(get_int_stat(file," XACT_BREAKDOWN_STALL_CYCLES"))
                lst_abort_on_commit_cycles.append(get_int_stat(file, " XACT_BREAKDOWN_ABORT_ON_COMMIT_CYCLES"))
                lst_abort_on_abort_cycles.append(get_int_stat(file, " XACT_BREAKDOWN_ABORT_ON_ABORT_CYCLES"))

                lst_commited_stall_on_trans_commit.append(get_int_stat(file, " C:\$:C"))
                lst_commited_stall_on_trans_abort.append(get_int_stat(file, " C:\$:X"))
                lst_commited_stall_on_stall_commit.append(get_int_stat(file, " C:\|:C"))
                lst_commited_stall_on_stall_abort.append(get_int_stat(file, " C:\|:X"))
                lst_commited_stall_on_commiting.append(get_int_stat(file, " C:C:C"))
                lst_commited_stall_on_aborting.append(get_int_stat(file, " C:X:X"))
                lst_aborted_stall_on_trans_commit.append(get_int_stat(file, " X:\$:C"))
                lst_aborted_stall_on_trans_abort.append(get_int_stat(file, " X:\$:X"))
                lst_aborted_stall_on_stall_commit.append(get_int_stat(file, " X:\|:C"))
                lst_aborted_stall_on_stall_abort.append(get_int_stat(file, " X:\|:X"))
                lst_aborted_stall_on_commiting.append(get_int_stat(file, " X:C:C"))
                lst_aborted_stall_on_aborting.append(get_int_stat(file, " X:X:X"))
                lst_aborts_per_conflict.append(get_average_stat(file, "xact_aborts_per_conflict"))
                    
            mean_cycles = mfgraph.mean(lst_cycles)
            cycle_bar.append([pname, mean_cycles/1000000])
            #mean_conf   = mfgraph.confidence_interval_95_percent(lst)
            mean_total_cycles = (processors - 1) * mean_cycles
            mean_non_trans_cycles  = mfgraph.mean(lst_non_trans_cycles)
            mean_good_trans_cycles = mfgraph.mean(lst_good_trans_cycles)
            mean_commiting_cycles  = mfgraph.mean(lst_commiting_cycles)
            mean_aborting_cycles   = mfgraph.mean(lst_aborting_cycles)
            mean_backoff_cycles    = mfgraph.mean(lst_backoff_cycles)
            mean_barrier_cycles    = mfgraph.mean(lst_barrier_cycles)
            mean_stall_trans_cycles = mfgraph.mean(lst_stall_cycles)
            mean_abort_on_commit_cycles     = mfgraph.mean(lst_abort_on_commit_cycles)
            mean_abort_on_abort_cycles     = mfgraph.mean(lst_abort_on_abort_cycles)
            
            mean_commited_stall_on_trans_commit_cycles = mfgraph.mean(lst_commited_stall_on_trans_commit)
            mean_commited_stall_on_trans_abort_cycles = mfgraph.mean(lst_commited_stall_on_trans_abort)
            mean_commited_stall_on_stall_commit_cycles = mfgraph.mean(lst_commited_stall_on_stall_commit)
            mean_commited_stall_on_stall_abort_cycles = mfgraph.mean(lst_commited_stall_on_stall_abort)
            mean_commited_stall_on_commiting_cycles = mfgraph.mean(lst_commited_stall_on_commiting)
            mean_commited_stall_on_aborting_cycles = mfgraph.mean(lst_commited_stall_on_aborting)
            mean_aborted_stall_on_trans_commit_cycles = mfgraph.mean(lst_aborted_stall_on_trans_commit)
            mean_aborted_stall_on_trans_abort_cycles = mfgraph.mean(lst_aborted_stall_on_trans_abort)
            mean_aborted_stall_on_stall_commit_cycles = mfgraph.mean(lst_aborted_stall_on_stall_commit)
            mean_aborted_stall_on_stall_abort_cycles = mfgraph.mean(lst_aborted_stall_on_stall_abort)
            mean_aborted_stall_on_commiting_cycles = mfgraph.mean(lst_aborted_stall_on_commiting)
            mean_aborted_stall_on_aborting_cycles = mfgraph.mean(lst_aborted_stall_on_aborting)

            mean_abort_cycles = mean_abort_on_abort_cycles + mean_abort_on_commit_cycles
            mean_commited_stall_cycles = mean_commited_stall_on_aborting_cycles + mean_commited_stall_on_commiting_cycles + mean_commited_stall_on_stall_abort_cycles + mean_commited_stall_on_stall_commit_cycles + mean_commited_stall_on_trans_commit_cycles + mean_commited_stall_on_trans_abort_cycles
            mean_aborted_stall_cycles = mean_aborted_stall_on_aborting_cycles + mean_aborted_stall_on_commiting_cycles + mean_aborted_stall_on_stall_abort_cycles + mean_aborted_stall_on_stall_commit_cycles + mean_aborted_stall_on_trans_commit_cycles + mean_aborted_stall_on_trans_abort_cycles
            mean_stall_cycles = mean_commited_stall_cycles + mean_aborted_stall_cycles
            #mean_non_trans_cycles = mean_total_cycles - (mean_good_trans_cycles + mean_aborting_cycles + mean_commiting_cycles + mean_backoff_cycles + mean_barrier_cycles + mean_abort_cycles + mean_stall_cycles)
            
            #mean_total_cycles = mean_non_trans_cycles + (mean_good_trans_cycles + mean_aborting_cycles + mean_commiting_cycles + mean_backoff_cycles + mean_barrier_cycles + mean_abort_cycles + mean_stall_cycles)
            mean_total_cycles = mean_non_trans_cycles + (mean_good_trans_cycles + mean_aborting_cycles + mean_commiting_cycles + mean_backoff_cycles + mean_barrier_cycles + mean_abort_cycles + mean_stall_trans_cycles)

            print name, pname
            print "TOTAL_CYCLES: ", mean_total_cycles
            print "RUBY_CYCLES: ", mean_cycles
            print "AVG_CYCLES_PER_THREAD: ", mean_total_cycles/(processors-1)

            
            if (mean_non_trans_cycles < 0):
              print " WARNING: NON_TRANS_CYCLES NEGATIVE "
              print " TOTAL_CYCLES:     ", mean_total_cycles
              print " TRANS_CYCLES:     ", mean_good_trans_cycles
              print " COMMITING_CYCLES: ", mean_commiting_cycles
              print " ABORTING_CYCLES:  ", mean_aborting_cycles
              print " BACKOFF_CYCLES:   ", mean_backoff_cycles
              print " BARRIER_CYCLES:   ", mean_barrier_cycles
              print " WASTED_CYCLES:    ", mean_abort_cycles
              print " STALL_CYCLES:     ", mean_stall_cycles
                    
            #print "total:    %d" % mean_total_cycles
            #print "Non-xact:    ", '%.2f' % (float(mean_non_trans_cycles * 100) / mean_total_cycles)
            #print "Barrier:     ", '%.2f' % (float(mean_barrier_cycles * 100) / mean_total_cycles)
            #print "Xact:        %.2f" % (float(mean_good_trans_cycles * 100) / mean_total_cycles)
            #print "Stall:       %.2f" % (float(mean_stall_cycles  * 100) / mean_total_cycles)
            #print "Commiting:   %.2f" % (float(mean_commiting_cycles * 100) / mean_total_cycles)
            #print "Abort:       %.2f" % (float(mean_abort_cycles  * 100) / mean_total_cycles)
            #print "Aborting:    %.2f" % (float(mean_aborting_cycles * 100) / mean_total_cycles)
            #print "Backoff:     %.2f" % (float(mean_backoff_cycles * 100) / mean_total_cycles)
            #print

            print "Mean_aborts_per_conflict: ", '%.2f' % (mfgraph.mean(lst_aborts_per_conflict))

            #print "Stall Breakdown "
            if (mean_commited_stall_on_trans_commit_cycles > 0):
              print "(stall,xact) -> (commit,commit) %.2f" % (float(mean_commited_stall_on_trans_commit_cycles * 100)/mean_stall_cycles) 
            if (mean_commited_stall_on_trans_abort_cycles > 0):
              print "(stall,xact) ->(commit,abort) %.2f" % (float(mean_commited_stall_on_trans_abort_cycles * 100)/mean_stall_cycles) 
            if (mean_commited_stall_on_stall_commit_cycles > 0):
              print "(stall,stall) -> (commit,commit) %.2f" % (float(mean_commited_stall_on_stall_commit_cycles * 100)/mean_stall_cycles) 
            if (mean_commited_stall_on_stall_abort_cycles > 0):
              print "(stall,stall) -> (commit,abort) %.2f" % (float(mean_commited_stall_on_stall_abort_cycles * 100)/mean_stall_cycles) 
            if (mean_commited_stall_on_commiting_cycles > 0):
              print "(stall,commit) -> (commit,commit) %.2f" % (float(mean_commited_stall_on_commiting_cycles * 100)/mean_stall_cycles) 
            if (mean_commited_stall_on_aborting_cycles > 0):
              print "(stall,abort) -> (commit,abort) %.2f" % (float(mean_commited_stall_on_aborting_cycles * 100)/mean_stall_cycles) 
            if (mean_aborted_stall_on_trans_commit_cycles > 0):
              print "(stall,xact) -> (abort,commit) %.2f" % (float(mean_aborted_stall_on_trans_commit_cycles * 100)/mean_stall_cycles) 
            if (mean_aborted_stall_on_trans_abort_cycles > 0):
              print "(stall,xact) -> (abort,abort) %.2f" % (float(mean_aborted_stall_on_trans_abort_cycles * 100)/mean_stall_cycles) 
            if (mean_aborted_stall_on_stall_commit_cycles > 0):
              print "(stall,stall) -> (abort,commit) %.2f" % (float(mean_aborted_stall_on_stall_commit_cycles * 100)/mean_stall_cycles) 
            if (mean_aborted_stall_on_stall_abort_cycles > 0):
              print "(stall,stall) -> (abort,abort) %.2f" % (float(mean_aborted_stall_on_stall_abort_cycles * 100)/mean_stall_cycles) 
            if (mean_aborted_stall_on_commiting_cycles > 0):
              print "(stall,stall) -> (abort,commit) %.2f" % (float(mean_aborted_stall_on_commiting_cycles * 100)/mean_stall_cycles) 
            if (mean_aborted_stall_on_aborting_cycles > 0):
              print "(stall,abort) -> (abort,abort) %.2f" % (float(mean_aborted_stall_on_aborting_cycles * 100)/mean_stall_cycles) 
            #print "Abort Breakdown"
            #if (mean_abort_on_commit_cycles > 0):
            #  print "(abort,commit) %.2f" % (float(mean_abort_on_commit_cycles * 100)/mean_abort_cycles) 
            #if (mean_abort_on_abort_cycles > 0):
            #  print "(abort,abort) %.2f" % (float(mean_abort_on_abort_cycles * 100)/mean_abort_cycles)
            print
               
            #tuple = [pname,  mean_backoff, mean_good_xact_time, mean_good_stalls, mean_bad_stalls]
            tuple = [pname, float(mean_total_cycles/1000000)]
            #tuple += mfgraph.stack_bars([float(mean_non_trans_cycles)/1000000, float(mean_good_trans_cycles)/1000000, float(mean_barrier_cycles)/1000000, float(mean_backoff_cycles)/1000000, float(mean_stall_cycles)/1000000, float(mean_commiting_cycles)/1000000, float(mean_abort_cycles)/1000000, float(mean_aborting_cycles)/1000000])
            #tuple += mfgraph.stack_bars([float(mean_aborting_cycles)/1000000, float(mean_abort_cycles)/1000000, float(mean_commiting_cycles)/1000000, float(mean_stall_cycles)/1000000, float(mean_backoff_cycles)/1000000, float(mean_barrier_cycles)/1000000, float(mean_good_trans_cycles)/1000000, float(mean_non_trans_cycles)/1000000])
            
            #tuple += mfgraph.stack_bars([float(mean_abort_cycles)/1000000, float(mean_commiting_cycles)/1000000, float(mean_stall_cycles)/1000000, float(mean_backoff_cycles)/1000000, float(mean_barrier_cycles)/1000000, float(mean_good_trans_cycles)/1000000, float(mean_non_trans_cycles)/1000000])
            tuple += mfgraph.stack_bars([float(mean_abort_cycles)/1000000, float(mean_commiting_cycles)/1000000, float(mean_stall_trans_cycles)/1000000, float(mean_backoff_cycles)/1000000, float(mean_barrier_cycles)/1000000, float(mean_good_trans_cycles)/1000000, float(mean_non_trans_cycles)/1000000])
            
            tuple_list.append(tuple)
        tuple_list.insert(0, benchmark)    
        bars.append(tuple_list)
        cycle_bars.append(cycle_bar)
                    
        #bars.sort()
        print bars

        #draw the graph
        graph_bars = bars

        graph_out = mfgraph.stacked_bar_graph(graph_bars,
                                          title = predictor_config,
                                          #bar_segment_labels = ["NonTrans", "Trans", "Barrier", "Backoff", "Stall", "Commiting", "Aborted", "Aborting"],
                                          bar_segment_labels = ["Aborting", "Aborted", "Commiting", "Stall", "Backoff", "Barrier", "Trans", "NonTrans"],
                                          ylabel = "Execution Time (in millions of cycles)",
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
                                          colors = ["1 1 1", "1 0 0", ".6 .6 .6", ".4 .4 .4", "0 1 0", ".8 .8 .8", "0 0 1", "0 .5 0"]
                                          )
        mfgraph.make_eps(graph_out, "exe_breakdown", "contention")
    
        graph_out = mfgraph.stacked_bar_graph(graph_bars,
                                          title = predictor_config,
                                          bar_segment_labels = ["Aborting", "Aborted", "Commiting", "Stall", "Backoff", "Barrier", "Trans", "NonTrans"],
                                          ylabel = "Execution Time (in millions of cycles)",
                                          ymin = 0,
                                          xsize = 5.5,
                                          #title_fontsize = "12",
                                          #label_fontsize = "8",
                                          #bar_name_font_size = "8",
                                          #legend_fontsize = "9",
                                          #stack_name_font_size = "10",
                                          #stack_name_location = 9,
                                          bar_space = 1.2,
                                          colors = ["1 1 1", "1 0 0", ".6 .6 .6", ".4 .4 .4", "0 1 0", ".8 .8 .8", "0 0 1", "0 .5 0"]
                                          )
        jgraphs.append(graph_out)


        graph_bars = cycle_bars
        print cycle_bars
        graph_out = mfgraph.stacked_bar_graph(graph_bars,
                                          title = predictor_config,
                                          bar_segment_labels = ["Execution Time"],
                                          ylabel = "Execution Time (millions of cycles)",
                                          ymin = 0,
                                          xsize = 5.5,
                                          #title_fontsize = "12",
                                          #label_fontsize = "8",
                                          #bar_name_font_size = "8",
                                          #legend_fontsize = "9",
                                          #stack_name_font_size = "10",
                                          #stack_name_location = 9,
                                          bar_space = 1.2
                                          )        
        jgraphs.append(graph_out)
        bars = []
        cycle_bars = []


all_benchmarks = ("raytrace-trans_1",
                  "raytrace-locks_1",
                  #"radiosity-trans_1",
                  #"radiosity-locks_1",
                  "ocean-trans_66_1",
                  "ocean-locks_66_1",
                  #"barnes-trans_512_1",
                  #"barnes-locks_512_1",
                  "water-nsquared-trans-216_1",
                  "water-nsquared-locks-216_1",
                  )

directory_protocols = {}
directory_protocols["lock"] = "MOESI_directory";
directory_protocols["trans"] = "MOESI_xact_directory";

nested_directory_protocols = {}
nested_directory_protocols["flat"] = "MOESI_xact_directory";
nested_directory_protocols["closed"] = "MOESI_xact_directory_closed";
nested_directory_protocols["open"] = "MOESI_xact_directory_open";

abort_directory_protocols = {}
abort_directory_protocols["flat"] = "MOESI_xact_directory_abort";
abort_directory_protocols["closed"] = "MOESI_xact_directory_abort_closed";
abort_directory_protocols["open"] = "MOESI_xact_directory_abort_open";

directory_protocols_btree = {}
directory_protocols_btree["lock"] = "MOESI_xact_directory";
directory_protocols_btree["trans"] = "MOESI_xact_directory";

directory_protocols_avg = {}
directory_protocols_avg["lock"] = "MOESI_directory";
directory_protocols_avg["trans"] = "MOESI_xact_directory_bf_less_aborts";

bcast_protocols = {}
bcast_protocols["lock"] = "MOESI_hammer";
bcast_protocols["trans"] = "MOESI_xact_hammer_bf";

bcast_protocols_count = {}
bcast_protocols_count["lock"] = "MOESI_hammer";
bcast_protocols_count["trans"] = "MOESI_xact_hammer_bf_delay_abort";

bcast_protocols_btree = {}
bcast_protocols_btree["lock"] = "MOESI_xact_hammer_bf";
bcast_protocols_btree["trans"] = "MOESI_xact_hammer_bf";

bcast_protocols_avg = {}
bcast_protocols_avg["lock"] = "MOESI_hammer";
bcast_protocols_avg["trans"] = "MOESI_xact_hammer_bf_delay_abort";

nested_xact_protocols = [
    "MOESI_xact_directory",
    "MOESI_xact_directory_imm",
    "MOESI_xact_directory_abort"
    ]

contention_xact_protocols = [
    "BASE", [
    ("EE", "MESI_CMP_filter_directory"),
    #("EE_M", "MESI_CMP_eagerCD_eagerVM_magic"),
    #("EE_T", "MESI_CMP_eager_timestamp"),
    #("EE_H", "MESI_CMP_eagerCD_hybrid"),
    ("EL", "MESI_CMP_eagerCD_lazyVM_backoff"),
    ("LL", "MESI_CMP_lazy"),
    #("LL_B", "MESI_CMP_lazy_backoff"),
    ]
    ]

contention_xact_alt_protocols = [
    "ALT", [     
    #("EE", "EagerCD_EagerVM_Base_NoHWBuf"),
    ("EE", "EagerCD_EagerVM_Base_NoPred"),
    ("EE_P", "EagerCD_EagerVM_Base_Pred"),
    ("EE_H", "EagerCD_EagerVM_Hybrid_NoPred"),
    ("EE_HP", "EagerCD_EagerVM_Hybrid_Pred"),
    ("EL", "EagerCD_LazyVM_Base_NoPred"),
    ("EL_P", "EagerCD_LazyVM_Base_Pred"),
    ("EL_B", "EagerCD_LazyVM_Backoff_NoPred"),
    ("EL_BP", "EagerCD_LazyVM_Backoff_Pred"),
    ("EL_T", "EagerCD_LazyVM_Timestamp_NoPred"),
    ("EL_TP", "EagerCD_LazyVM_Timestamp_Pred"),
    ("EL", "EagerCD_LazyVM_Backoff"),
    ("LL", "LazyCD_LazyVM_Base"),
    ("LL_B", "LazyCD_LazyVM_Backoff"),
    
    #("EL_H", "MESI_CMP_eagerCD_lazyVM_cycle"),
    ]
    ]    

contention_xact_best_protocols = [
    "BEST", [     
    #("EE", "MESI_CMP_filter_directory"),
    ("EE_H", "MESI_CMP_eagerCD_hybrid"),
    ("EL_T", "MESI_CMP_eagerCD_lazyVM"),
    ("LL_B", "MESI_CMP_lazy_backoff"),
    ]
    ]    

contention_xact_all_protocols = [    
    "ALL", [
    ("EE", "MESI_CMP_filter_directory"),
    ("EL", "MESI_CMP_eagerCD_lazyVM_backoff"),
    ("LL", "MESI_CMP_lazy"),
    #("EE_M", "MESI_CMP_eagerCD_eagerVM_magic"),
    #("EE_T", "MESI_CMP_eager_timestamp"),
    #("EE_H", "MESI_CMP_eagerCD_hybrid"),
    #("EL_T", "MESI_CMP_eagerCD_lazyVM"),
    #("EL_H", "MESI_CMP_eagerCD_lazyVM_cycle"),
    #("LL_B", "MESI_CMP_lazy_backoff")
    ]
    ]    

contention_xact_enh_protocols = [    
    "ENH", [
    ("EE_H", "MESI_CMP_eagerCD_hybrid"),
    ("EL_T", "MESI_CMP_eagerCD_lazyVM"),
    ("LL_B", "MESI_CMP_lazy_backoff"),
    ]
    ]    

count_runs = [
    ("TATAS","shared-counter-Lock_TATAS_20000", "lock"),
    ("MCS", "shared-counter-Lock_MCS_20000", "lock"),
    ("TTM", "shared-counter-TM_20000", "trans")
    ]

btree_runs = [
    ("TATAS Locks","btree-Lock_TATAS_10000", "lock"),
    #("MCS Locks", "btree-Lock_MCS_2000"),
    ("TTM", "btree-TM_10000", "trans")
    ]

btree_large_runs = [
    ("TATAS Locks","btree-large-Lock_TATAS_10000", "lock"),
    #("MCS Locks","btree-large-Lock_MCS_10000", "lock"),
    ("TTM", "btree-large-TM_10000", "trans")
    ]

avg_runs = [
    ("TATAS","average-Lock_TATAS_50", "lock"),
    #("MCS Locks", "average-Lock_MCS_50", "lock"),
    ("TTM", "average-TM_50", "trans")
    ]

avg_rs_runs = [
    ("TATAS Locks","average-Lock_TATAS_50"),
    ("MCS Locks", "average-Lock_MCS_50"),
    ("ULTM", "average-TM_50")
    ]

splash_perf_runs = [
    #("OCEAN 66",       "ocean-locks_66_1", "ocean-trans_66_1"),
    ("OCEAN",       "ocean-locks-base-258_1", "ocean-trans-base-258_1"),
    ("RADIOSITY",     "radiosity-locks-base_1", "radiosity-trans-base_1"),
    #("RAD",     "radiosity-locks-base_1", "radiosity-trans-base_1"),
    ("CHOLESKY",     "cholesky-locks-base-14_1", "cholesky-trans-base-14_1"),
    ("RT-OPT",       "raytrace-locks-opt-teapot_1", "raytrace-trans-opt-teapot_1"),
    ("RT-BASE",       "raytrace-locks-base-teapot_1", "raytrace-trans-base-teapot_1"),
    #("BARNES 512",     "barnes-locks_512_1", "barnes-trans_512_1"),
    ("BARNES",     "barnes-locks-base-512_1", "barnes-trans-base-512_1"),
    #("WATER N-SQ 216", "water-nsquared-locks-216_1", "water-nsquared-trans-216_1"),
    ("WATER", "water-nsquared-locks-base-512_1", "water-nsquared-trans-base-512_1"),
    #("VOLREND", "volrend-locks-base_1", "volrend-trans-base_1"),
    ]


splash_dist_runs = [
    ("BARNES",     "barnes-trans-base-8192_1"),
    #("OCEAN",      "ocean-trans_66_1"),
    ("OCEAN",      "ocean-trans-base-258_1"),
    ("RT-BASE",   "raytrace-trans-base-teapot_1"),
    ("RT-OPT",   "raytrace-trans-opt-teapot_1"),
    ("WATER", "water-nsquared-trans-base-512_1"),
    #("RADIOSITY", "radiosity-trans"),
    #("RAD",  "radiosity-trans-base_1"),
    ("CHOL", "cholesky-trans-base-29_1"),
    ]

filter_perf_runs = [
    #("BerkeleyDB", "db", "128ops"),
    #("Sphinx", "sphinx3", "None"),
    ("Barnes", "barnes", "512bodies"),
    ("Cholesky", "cholesky", "14"),
    ("Mp3d", "mp3d", "128mol-1024ops"),
    ("Radiosity", "radiosity", "1024ops"),
    ("Raytrace", "raytrace", "teapot"),
    ]

filter_perf_runs_2 = [
    #("BerkeleyDB", "db", "128ops_32bit"),
    #("Sphinx", "sphinx3", "None_32bit"),
    #("mp3d", "mp3d", "128mol-1024ops_32bit"),
    #("Raytrace", "raytrace", "teapot_32bit"),
    ("Radiosity", "radiosity", "64ops_32bit")
    ]

filter_perf_runs_3 = [
    #("BerkeleyDB", "db", "128ops"),
    #("Sphinx", "sphinx3", "None"),
    ("mp3d", "mp3d", "128mol-1024ops"),
    ("Radiosity", "radiosity", "256ops"),
    ("Raytrace", "raytrace", "teapot")
	]

contention_perf_runs = [
    "SET1", (
    ("CONF1", "contention", "config1"),    
    ("CONF2", "contention", "config2"),    
    ("CONF3", "contention", "config3"),    
    ("CONF4", "contention", "config4"),    
    ("CONF5", "contention", "config5")    
    )    
    ]

contention_perf_runs_2 = [
    "SET2", (
    ("CONF6", "contention", "config6"),
    ("CONF7", "contention", "config7"),    
    ("CONF8", "contention", "config8"),    
    ("CONF9", "contention", "config9"),    
    ("CONF10", "contention", "config10")    
    )    
    ]

contention_perf_runs_3 = [
    "SET3", (
    ("CONF11", "contention", "config11"),    
    ("CONF12", "contention", "config12"),    
    ("CONF13", "contention", "config13"),    
    ("CONF14", "contention", "config14")    
    )    
    ]

contention_perf_runs_4 = [
    "HIGH", (
    ("0.01", "contention", "config16"),    
    ("0.1", "contention", "config17"),    
    ("0.2", "contention", "config18"),    
    #("0.3", "contention", "config19"),    
    #("0.4", "contention", "config20"),    
    ("0.5", "contention", "config21")    
    )    
    ]

contention_perf_runs_5 = [
    "LOW", (
    ("0.01", "contention", "config22"),    
    ("0.1", "contention", "config23"),    
    ("0.2", "contention", "config24"),    
    #("0.3", "contention", "config25"),    
    #("0.4", "contention", "config26"),    
    ("0.5", "contention", "config27"),
    )    
    ]

contention_perf_runs_6 = [
    "SERIALIZED", (
    ("0.25B", "contention", "config43"),    
    ("0.75B", "contention", "config45"),    
    ("0.25M", "contention", "config46"),    
    ("0.75M", "contention", "config48"),    
    ("0.25E", "contention", "config49"),    
    ("0.75E", "contention", "config51"),    
    #("PRE", "contention", "config52"),
    )    
    ]

contention_perf_runs_7 = [
    "SERIAL", (
    ("0.25B", "contention", "config43"),    
    ("0.75B", "contention", "config45"),    
    ("0.25M", "contention", "config46"),    
    ("0.75M", "contention", "config48"),    
    ("0.25E", "contention", "config49"),    
    ("0.75E", "contention", "config51"),    
    )    
    ]

contention_splash_runs = [
    "SPLASH", (
    ("Barnes", "barnes", "512bodies"),
    ("Cholesky", "cholesky", "14"),
    ("Mp3d", "mp3d", "128mol-1024ops"),
    ("Radiosity", "radiosity", "1024ops"),
    ("Raytrace", "raytrace", "teapot"),
    )
	]

contention_bench_runs = [
    "BENCHMARKS", (
    ("BTree", "btree", "priv-alloc-20pct"),
    ("LFUCache", "prioqueue", "8192ops"),
    ("BerkeleyDB", "db", "1024ops"),
    #("Sphinx", "sphinx3", "None")
    )
    ]

contention_mb_runs = [
    "SEL", (
    ("BerkeleyDB", "db", "128ops"),
    ("Raytrace", "raytrace", "teapot"),
    ("Sphinx", "sphinx3", "None"),
    )
	]

contention_all_runs = [    
    "ALL", (
    ("Barnes", "barnes", "512bodies"),
    ("Cholesky", "cholesky", "14"),
    ("Mp3d", "mp3d", "128mol-1024ops"),
    ("Radiosity", "radiosity", "1024ops"),
    ("Raytrace", "raytrace", "teapot"),
    #("BerkeleyDB", "db", "128ops"),
    #("Sphinx", "sphinx3", "None"),
    #("BTree", "btree", "priv-alloc-20pct"),
    ("LFUCache", "prioqueue", "8192ops"),
    #("LFUCache_TP", "prioqueue-tp", "8192ops"),
    #("HIGH 0.01", "contention", "config16"),    
    #("HIGH 0.1", "contention", "config17"),    
    #("HIGH 0.2", "contention", "config18"),    
    #("HIGH 0.5", "contention", "config21"),    
    #("LOW 0.01", "contention", "config22"),    
    #("LOW 0.1", "contention", "config23"),    
    #("LOW 0.2", "contention", "config24"),    
    #("LOW 0.5", "contention", "config27"),    
    #("SERIAL 0.25B", "contention", "config43"),    
    #("SERIAL 0.75B", "contention", "config45"),    
    #("SERIAL 0.25M", "contention", "config46"),    
    #("SERIAL 0.75M", "contention", "config48"),    
    #("SERIAL 0.25E", "contention", "config49"),    
    #("SERIAL 0.75E", "contention", "config51"),    
    #("PREFETCH", "contention", "config52"),    
    )
    ]

btree_dist_runs = [("BTree 64 B",    "btree-TM_10000"),
                   ("BTree 8 kB",    "btree-large-TM_10000")]

btree_mix = [
    ("BTree-100","btree-TM-15pct-100freelist", "flat"),
    ("BTree-1","btree-TM-15pct-1freelist", "flat"),
    ]

btree_single_freelist = [
    ("BTree-1","btree-TM-15pct-1freelist", "flat"),
    ]

btree_5_pct = [
    ("BTree-flat","btree-5pct-2000000_10000", "flat"),
    ("BTree-closed","btree-5pct-2000000_10000", "closed"),
    ("BTree-open","btree-5pct-2000000_10000", "open"),
    ]

btree_10_pct = [
    ("BTree-flat","btree-10pct-2000000_5000", "flat"),
    ("BTree-closed","btree-10pct-2000000_5000", "closed"),
    ("BTree-open","btree-10pct-2000000_5000", "open"),
    ]

btree_multi_nested_runs = [
    ("BTree-multi-flat","btree-multi-4-5pct-2000000_1000", "flat"),
    ("BTree-multi-closed","btree-multi-4-5pct-2000000_1000", "closed"),
    ("BTree-multi-open","btree-multi-4-5pct-2000000_1000", "open"),
    ]

btree_nested_overall_runs = [
    # <label> < directory > < minopen > < depth >
    #("Open-1","btree-TM-15pct-1freelist", "35", "4"),
    ("Closed-100","btree-TM-15pct-100freelist", "40", "4"),
    ("Flat-100","btree-TM-15pct-100freelist", "40", "1"),
    ("Closed-1","btree-TM-15pct-1freelist", "40", "4"),
    ("Flat-1","btree-TM-15pct-1freelist", "40", "1"),
    ]

btree_nested_closed_open_runs = [
    ("Open-1","btree-TM-15pct-1freelist", "35", "4"),
    ("Closed-100","btree-TM-15pct-100freelist", "40", "4"),
    ("Closed-1","btree-TM-15pct-1freelist", "40", "4")
    ]


log_size      = ("xact_log_size_dist", "Transaction Log Size (in 64-Byte lines)", "write_set")
read_set_size = ("xact_read_set_size_dist", "Transaction Read Set Size (in 64-Byte lines)", "read_set")

jgraph_input = []


# # microbenchmark performance
# make_microbench_line(jgraph_input, "counter", count_runs, 10000, directory_protocols,  "dir")
# make_microbench_line(jgraph_input, "counter", count_runs, 10000, bcast_protocols_count, "bcast")
# #make_microbench_bar(jgraph_input, "shared-counter", count_runs, 10000, directory_protocols, "dir")
# #make_microbench_bar(jgraph_input, "shared-counter", count_runs, 10000, bcast_protocols, "bcast")
# #make_microbench_bar(jgraph_input, "shared-counter", count_runs, 10000, "MOESI_xact_directory_greedy")
# #make_microbench_bar(jgraph_input, "btree", btree_runs, 10000, "MOESI_xact_directory_bf")
# #make_microbench_bar(jgraph_input, "btree", btree_runs, 10000, bcast_protocols_btree, "bcast")
# # make_microbench_line(jgraph_input, "btree", btree_runs, None, 10000, directory_protocols_btree, "dir")
# #make_microbench_line(jgraph_input, "btree", btree_runs, 10000, bcast_protocols_btree, "bcast")

#make_microbench_line(jgraph_input, "LogTM Base", btree_mix, 10000, nested_directory_protocols, 8, "dir")
#make_microbench_line(jgraph_input, "LogTM Abort", btree_mix, 10000, abort_directory_protocols, 8, "dir")
#make_simics_microbench_line(jgraph_input, "btree", btree_mix, 10000, nested_directory_protocols, "dir")
#make_microbench_speedup_line(jgraph_input, "btree 1", btree_mix, 10000, nested_directory_protocols, 1, "dir")
#make_microbench_speedup_line(jgraph_input, "BTree LogTM Base, Closed", btree_mix, 10000, nested_directory_protocols, 8, "dir")
#make_microbench_speedup_line(jgraph_input, "BTree LogTM Base, Flat", btree_mix, 10000, nested_directory_protocols, 1, "dir")
#make_microbench_speedup_line(jgraph_input, "BTree LogTM Abort", btree_mix, 10000, abort_directory_protocols, 8, "dir")
#make_microbench_line(jgraph_input, "btree 5%", btree_5_pct, 10000, nested_directory_protocols, "dir")
#make_microbench_bar(jgraph_input, "btree 5%", btree_5_pct, 10000, nested_directory_protocols, "dir")
#make_microbench_line(jgraph_input, "btree", btree_10_pct, 10000, nested_directory_protocols, "dir")
#make_microbench_bar(jgraph_input, "btree", btree_10_pct, 10000, nested_directory_protocols, "dir")
#make_microbench_line(jgraph_input, "btree-multi", btree_multi_nested_runs, 10000, nested_directory_protocols, "dir")

# make_microbench_line(jgraph_input, "btree_lg", btree_large_runs, 10000, directory_protocols_btree, "dir")
# # #make_microbench_line(jgraph_input, "btree_lg", btree_large_runs, 10000, bcast_protocols_btree, "bcast")

#make_microbench_line(jgraph_input, "average", avg_runs, 10000, bcast_protocols_avg, "bcast")
#make_microbench_line(jgraph_input, "average", avg_runs, 10000, directory_protocols_avg, "dir")
#make_read_set(jgraph_input, "average", avg_rs_runs, 10000)

# # microbenchmark read set/write set
# make_distribution(jgraph_input, btree_dist_runs, 7, "PDF", log_size)
# make_distribution(jgraph_input, btree_dist_runs, 7, "CDF", log_size)
# make_distribution(jgraph_input, btree_dist_runs, 7, "PDF", read_set_size)
# make_distribution(jgraph_input, btree_dist_runs, 7, "CDF", read_set_size)
#make_xact_histogram(jgraph_input, splash_dist_runs, "MOESI_xact_directory_synch")
#make_stall_histogram(jgraph_input, splash_dist_runs, "MOESI_xact_directory_synch")
#make_aborts_graph(jgraph_input, splash_dist_runs, "MOESI_xact_directory_synch")
#make_writebacks_graph(jgraph_input, splash_dist_runs, "MOESI_xact_directory_synch")

# splash performance
#make_norm_runtime(jgraph_input, splash_perf_runs, 32)
#make_abs_runtime(jgraph_input, splash_perf_runs, 32)

# splash read set/write set
#make_distribution(jgraph_input, splash_dist_runs, 16, "PDF", log_size)
#make_distribution(jgraph_input, splash_dist_runs, 16, "CDF", log_size)
#make_distribution(jgraph_input, splash_dist_runs, 16, "WPDF", log_size)

#make_distribution(jgraph_input, splash_dist_runs, 16, "PDF", read_set_size)
#make_distribution(jgraph_input, splash_dist_runs, 16, "CDF", read_set_size)
#make_distribution(jgraph_input, splash_dist_runs, 16, "WPDF", read_set_size)

## Flat
#make_microbench_speedup_line2(jgraph_input, "BTree LogTM Base, Flat", btree_mix, 10000, nested_xact_protocols[0], 1, "dir", 40)
#make_microbench_speedup_line2(jgraph_input, "BTree LogTM Reader-Abort, Flat", btree_mix, 10000, nested_xact_protocols[1], 1, "dir", 40)
#make_microbench_speedup_line2(jgraph_input, "BTree LogTM Abort, Flat", btree_mix, 10000, nested_xact_protocols[2], 1, "dir", 40)
## Closed
#make_microbench_speedup_line2(jgraph_input, "BTree LogTM Base, Closed", btree_mix, 10000, nested_xact_protocols[0], 4, "dir", 40)
#make_microbench_speedup_line2(jgraph_input, "BTree LogTM Reader-Abort, Closed", btree_mix, 10000, nested_xact_protocols[1], 4, "dir", 40)
#make_microbench_speedup_line2(jgraph_input, "BTree LogTM Abort, Closed", btree_mix, 10000, nested_xact_protocols[2], 4, "dir", 40)
## Open
#make_microbench_speedup_line2(jgraph_input, "BTree LogTM Base, Open", btree_single_freelist, 10000, nested_xact_protocols[0], 4, "dir", 35)
#make_microbench_speedup_line2(jgraph_input, "BTree LogTM Reader-Abort, Open", btree_single_freelist, 10000, nested_xact_protocols[1], 4, "dir", 35)
#make_microbench_speedup_line2(jgraph_input, "BTree LogTM Abort, Open", btree_single_freelist, 10000, nested_xact_protocols[2], 4, "dir", 35)

## Combined base
#make_microbench_combined_speedup_line(jgraph_input, "BTree LogTM Base, Overall", btree_nested_overall_runs, 10000, nested_xact_protocols[0], "btree_overall")
#make_microbench_combined_speedup_line(jgraph_input, "BTree LogTM Reader-abort, Overall", btree_nested_overall_runs, 10000, nested_xact_protocols[1], 4, "dir")
#make_microbench_combined_speedup_line(jgraph_input, "BTree LogTM Abort, Overall", btree_nested_overall_runs, 10000, nested_xact_protocols[2], 4, "dir")

## Combined with closed 1, 100 vs open 1
#make_microbench_combined_speedup_line(jgraph_input, "BTree LogTM Base, Overall", btree_nested_closed_open_runs, 10000, nested_xact_protocols[0], "btree_closed_open")
#make_tlb_line(jgraph_input)

#make_filters_speedup_graph(jgraph_input, filter_perf_runs)
#make_filters_line(jgraph_input, filter_perf_runs_2)
#make_filters_line(jgraph_input, filter_perf_runs_2)
#make_filters_speedup_graph2(jgraph_input, filter_perf_runs_3)

#spit_contention_perf_stats(jgraph_input, contention_all_runs, contention_xact_all_protocols, 16)
#spit_contention_perf_stats(jgraph_input, contention_all_runs, contention_xact_all_protocols, 32)
#spit_contention_perf_stats(jgraph_input, contention_all_runs, contention_xact_enh_protocols, 32)
#make_contention_perf_bars(jgraph_input, contention_splash_runs, contention_xact_protocols, 16)
#make_contention_perf_bars(jgraph_input, contention_splash_runs, contention_xact_alt_protocols, 16)
#make_contention_perf_bars(jgraph_input, contention_splash_runs, contention_xact_protocols, 32)
#make_contention_perf_bars(jgraph_input, contention_splash_runs, contention_xact_alt_protocols, 32)
#make_contention_perf_bars(jgraph_input, contention_all_runs, contention_xact_alt_protocols, 32)
#make_contention_perf_bars(jgraph_input, contention_splash_runs, contention_xact_best_protocols, 32)
#make_contention_perf_bars(jgraph_input, contention_mb_runs, contention_xact_all_protocols, 16)
#make_contention_perf_bars(jgraph_input, contention_mb_runs, contention_xact_protocols, 32)
#make_contention_perf_bars(jgraph_input, contention_perf_runs_4, contention_xact_protocols, 16)
#make_contention_perf_bars(jgraph_input, contention_perf_runs_4, contention_xact_alt_protocols, 16)
#make_contention_perf_bars(jgraph_input, contention_perf_runs_4, contention_xact_protocols, 32)
#make_contention_perf_bars(jgraph_input, contention_perf_runs_4, contention_xact_alt_protocols, 32)
#make_contention_perf_bars(jgraph_input, contention_perf_runs_5, contention_xact_protocols, 16)
#make_contention_perf_bars(jgraph_input, contention_perf_runs_5, contention_xact_alt_protocols, 16)
#make_contention_perf_bars(jgraph_input, contention_perf_runs_5, contention_xact_protocols, 32)
#make_contention_perf_bars(jgraph_input, contention_perf_runs_5, contention_xact_alt_protocols, 32)
#make_contention_perf_bars(jgraph_input, contention_perf_runs_6, contention_xact_protocols, 16)
#make_contention_perf_bars(jgraph_input, contention_perf_runs_6, contention_xact_alt_protocols, 16)
#make_contention_perf_bars(jgraph_input, contention_perf_runs_7, contention_xact_protocols, 16)
#make_contention_perf_bars(jgraph_input, contention_perf_runs_7, contention_xact_alt_protocols, 16)
#make_contention_perf_bars(jgraph_input, contention_perf_runs_6, contention_xact_protocols, 32)
#make_contention_perf_bars(jgraph_input, contention_perf_runs_6, contention_xact_alt_protocols, 32)
#make_contention_perf_bars(jgraph_input, contention_perf_runs_7, contention_xact_protocols, 32)
#make_contention_perf_bars(jgraph_input, contention_perf_runs_7, contention_xact_alt_protocols, 32)
#make_contention_perf_bars(jgraph_input, contention_perf_runs_3, contention_xact_protocols)
#make_contention_perf_bars(jgraph_input, contention_bench_runs, contention_xact_all_protocols, 32)
#spit_contention_perf_stats(jgraph_input, contention_bench_runs, contention_xact_all_protocols, 32)
#make_execution_time_breakdown(jgraph_input, contention_bench_runs, contention_xact_all_protocols, 32)
#make_execution_time_breakdown(jgraph_input, contention_splash_runs, contention_xact_all_protocols, 32)

predictor_config = "256hist_256pred_4thresh"
#predictor_config = "1024hist_1024pred_4thresh"
#predictor_config = "1024hist_1024pred_20thresh"
make_execution_time_breakdown(jgraph_input, contention_splash_runs, contention_xact_alt_protocols, 32, predictor_config)
make_execution_time_breakdown(jgraph_input, contention_bench_runs, contention_xact_alt_protocols, 32, predictor_config)

mfgraph.run_jgraph("newpage\n".join(jgraph_input), "contention-"+predictor_config)

#make_dist_table(splash_dist_runs, 16, read_set_size)
#make_dist_table(splash_dist_runs, 16, log_size)
