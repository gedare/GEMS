#!/s/std/bin/python

# This is an example of how to use the functions from mfgraph to
# generate jgraph images.  Run this script and look in examples.pdf
# for two graphs.  If you want to generate an eps, you can only
# generate one graph at a time.  To do this comment out the line below
# and un-comment the line below it.


# Old Results
#results_dir = "/p/multifacet/users/kmoore/ecperf_results/results.wmpi.final"
#results_dir = "/p/multifacet/users/kmoore/ecperf_results/results.pequod.tuning.3-22"
#ecperf_results_dir = "/p/multifacet/projects/ecperf/multifacet/workloads/ecperf/results.loop.9-25"
#ecperf_results_dir = "/p/multifacet/projects/ecperf/multifacet/workloads/ecperf/results.loop.jdk131.10-22"
#jbb_results_dir = "/p/multifacet/users/kmoore/jbb_results/results.loop.jdk131.10-20"
#jbb_results_dir = "/p/multifacet/projects/ecperf/multifacet/workloads/java/SPECjbb2000/results.main"
#jbb_results_dir = "/p/multifacet/users/kmoore/jbb_results/results.6min.10-30"


# jdk 1.3.1 (wl 6.1)
#  ecperf_results_dir = "/u/k/m/kmoore/multifacet/ecperf_results/results.hpca.10-27"
#  jbb_results_dir = "/p/multifacet/users/kmoore/jbb_results/results.hpca.10-28"
#  ecperf_tx_dir = "/u/k/m/kmoore/multifacet/ecperf_results/results.tx_rate.10-23"
#  jbb_warehouse_dir = "/u/k/m/kmoore/multifacet/jbb_results/results.warehouse.10-23"

# jdk 1.4.1 (wl 7.0)
#ecperf_results_dir = "/p/multifacet/projects/ecperf/multifacet/workloads/ecperf/results.wl7_0.11-25"
ecperf_results_dir = "/p/multifacet/projects/java_locks/results.zin.main-runs.6-30"
#ecperf_results_dir = "/p/multifacet/projects/ecperf/multifacet/workloads/ecperf/results.tuning.large.12-10"
#jbb_results_dir = "/p/multifacet/users/kmoore/jbb_results/results.jdk1.4.1.12-2"
jbb_results_dir = "/p/multifacet/users/kmoore/jbb_results/results.jdk1.4.1.12-2"
ecperf_tx_dir = "/p/multifacet/projects/java_locks/jbb_results/results.tx_rate.11-26"
jbb_warehouse_dir = "/p/multifacet/users/kmoore/jbb_results/results.warehouse.jdk1.4.1.12-4"


# Ruby
jbb_ruby_dirs = (["SPECjbb-25", "../condor/results/jbb-jdk131-25w-hpca"],
                 ["SPECjbb-10", "../condor/results/jbb-jdk131-10w-hpca"],
                 ["SPECjbb-1", "../condor/results/jbb-jdk131-1w-hpca"])

ecperf_ruby_dir = "../condor/results/ecperf-mm-2GB-15"
ecperf_ruby_dir = "../condor/results/ecperf_all2"

eps_dir = "figures_jdk1.4.1"
data_dir = "data"
excel_dir = "excel_files_jdk1.4.1"

import sys, string, os, glob, re, mfgraph, ecperf, jbb, hardware_counters

######################################
# Constants 
######################################

main_x_size = 2.75
main_y_size = 1.5

big_x_size = 6.0
big_y_size = 4.0

######################################
# Generate .eps file for each figure
######################################

def make_eps(input_str, base_filename):
    filename = "%s/%s" % (eps_dir, base_filename)
    print "making eps file: %s" % filename
    jgr_file = open("%s.jgr" % filename, "w")
    jgr_file.write(input_str)
    jgr_file.close()

    # generate .eps (ghostview-able)
    (in_file, out_file) = os.popen2("jgraph")
    in_file.write(input_str)
    in_file.close()

    eps_file = open("%s.eps" % filename, "w")
    eps_file.writelines(out_file.readlines())
    eps_file.close()

######################################
# Print the Raw Data to a file
######################################

#  def write_data(data, base_filename):
#      filename = "%s/%s" % (data_dir, base_filename)
#      data_file = open("%s.txt" % filename, "w")

#      i = 0
#      for set in data:
#          j = 0
#          for line in set[i]:
#              data_string = ",".join(line)
#              data_file.write(data_string)
#              j += 1
#          i += 1
#      data_file.close


def get_speedup(data):
    base = data[1][1]/data[1][0] # use the smallest value (assume sorted)
    if(base == 0):
        base = data[2][1]/data[2][0]
    
    real_data = data[1:] # skip label for calculations
    speedup = [data[0]] # keep label in results
    for tuple in real_data:
        #print "tuple " + tuple
        s_tuple = [tuple[0], tuple[1]/base, tuple[2]/base, tuple[3]/base]
        speedup.append(s_tuple)

    return speedup


############################################
# Excel Line Graphs
#
# This function generates a text file
# that can be imported, and (through a
# somewhat ugly macro) easily plotted
# as a line graph.
############################################

def make_excel_line(name, data):
    filename = "%s/%s.txt" % (excel_dir, name)
    print "making excel text file: %s" % filename
    excel_file = open(filename, "w")

    for set in data:
        is_first = 1
        for point in set:
            if is_first == 1:
                excel_file.write(set[0])
                is_first = 0
                continue
            else:
                excel_file.write("\t")
                                
            print point
            x_val = point[0]
            avg = point[1]
            # tuples are expected to be formed
            # with "mfgraph.merge_data" and in the
            # form [x, avg, avg+stdd, avg-stdd]
            if len(point) >= 3:
                std_dev = point[2] - point[1]
                excel_file.write("%f\t%f\t%f\n" % (x_val, avg, std_dev))
            else:
                excel_file.write("%f\t%f\n" % (x_val, avg))

            #excel_file.write("%f\t%f\n" % (x_val, avg))
            #excel_file.write("\t\t%f\n" % (std_dev))

    excel_file.close()

############################################
# Excel Bar Graphs
#
# This function generates a text file
# that can be imported, and (hopefully)
# easily plotted as a stacked bar graph.
############################################

def make_excel_bar(name, fields, data):
    filename = "%s/%s.txt" % (excel_dir, name)
    print "making excel text file: %s" % filename
    excel_file = open(filename, "w")

    fields.reverse()
    excel_file.write("\t")
    for f in fields:
        excel_file.write("\t%s" % f)
    excel_file.write("\n")

    for set in data:
        is_first = 1
       
        for tuple in set:
            if is_first == 1:
                excel_file.write(tuple) # name of the set
                is_first = 0
                continue
            else:
                excel_file.write("\t")

            excel_file.write("%f" % tuple[0])
            values = tuple[1:]
            values.reverse()
            for value in values:
                excel_file.write("\t%f" % (value))

            excel_file.write("\n")

    excel_file.close()
    
######################################
# Hardware Counter Scaling Graphs
######################################

def generate_throughput(jgraphs, title):
    xlabel = "Processors"
    ylabel = "Throughput"

    # Gather ECperf Data
    data = ecperf.get_throughput_data(ecperf_results_dir)
    ecperf_data = mfgraph.merge_data(group_title = "ECperf", data = data)

    # Graph ECperf Data
    base = ecperf_data[1][1]/ecperf_data[1][0]
    #linear = [[1, 1*base], [2, 2*base], [4, 4*base], [6, 6*base], [8, 8*base], [10, 10*base], [12, 12*base], [14, 14*base]]
    linear = [[1, 1*base], [2, 2*base], [3, 3*base], [4, 4*base], [5, 5*base], [6, 6*base], [7, 7*base]]
    linear_data = mfgraph.merge_data(group_title = "Linear", data = linear)
    lines = [ecperf_data, linear_data]
    
    jgraphs.append(mfgraph.line_graph(lines,
                                      title = "ECperf",
                                      xlabel = xlabel,
                                      ylabel = ylabel,
                                      xsize = big_x_size,
                                      ysize = 4.5,
                                      ))

    # Gather SPECjbb Data
    
    data = jbb.get_throughput_data(jbb_results_dir)
    jbb_data = mfgraph.merge_data(group_title = "SPECjbb", data = data)

    
    base = 0
    if (len(jbb_data) > 1):
        #print jbb_data
        base = jbb_data[1][1]/jbb_data[1][0]
        
    linear = [[1, 1*base], [2, 2*base], [4, 4*base], [6, 6*base], [8, 8*base], [10, 10*base], [12, 12*base], [14, 14*base]]
    linear_data = mfgraph.merge_data(group_title = "Linear", data = linear)
    lines = [jbb_data, linear_data]

    jgraphs.append(mfgraph.line_graph(lines,
                                      title = "SPECjbb",
                                      xlabel = xlabel,
                                      ylabel = ylabel,
                                      #title_fontsize = "12",
                                      #label_fontsize = "8",
                                      #legend_fontsize = "9",
                                      xsize = big_x_size,
                                      ysize = big_y_size,
                                      ))

    # Generate Speedup Graph
    ecperf_speedup = get_speedup(ecperf_data)
    jbb_speedup = get_speedup(jbb_data)
    linear_speedup = [[1,1], [2,2], [4,4], [6,6], [8,8], [10,10], [12,12] , [14,14]]
    linear_data = mfgraph.merge_data(group_title = "Linear", data = linear_speedup)


    ylabel = "Speedup"
    lines = [linear_data, ecperf_speedup, jbb_speedup]
    graph_output = mfgraph.line_graph(lines,
                                      title = "",
                                      xlabel = xlabel,
                                      ylabel = ylabel,
                                      #ylabel_location = 8.0,
                                      xlabel_location = 15.0,
                                      title_fontsize = "12",
                                      label_fontsize = "8",
                                      legend_fontsize = "9",
                                      legend_default = "hjc vjb",
                                      xsize = main_x_size, 
                                      ysize = main_y_size,
                                      )
    make_eps(graph_output, "speedup")
    make_excel_line("speedup", lines)
    #write_data(lines, "speedup")
    
    graph_output = mfgraph.line_graph(lines,
                                      title = "Speedup",
                                      xlabel = xlabel,
                                      ylabel = ylabel,
                                      #title_fontsize = "12",
                                      #label_fontsize = "8",
                                      #legend_fontsize = "9",
                                      xsize = big_x_size,
                                      ysize = big_y_size,
                                      )
    jgraphs.append(graph_output)

def generate_throughput_by_input(jgraphs, title):
    xlabel = "TX Rate"
    ylabel = "Throughput"

    # Gather ECperf Data
    data = ecperf.get_throughput_by_input_data(ecperf_tx_dir)
    ecperf_data = mfgraph.merge_data(group_title = "ECperf", data = data)

    lines = [ecperf_data]
    
    jgraphs.append(mfgraph.line_graph(lines,
                                      title = "ECperf",
                                      xlabel = xlabel,
                                      ylabel = ylabel,
                                      xsize = big_x_size,
                                      ysize = 4.5,
                                      ))


def generate_throughput_no_gc(jgraphs, title):
    xlabel = "Processors"
    ylabel = "Throughput"

    # Gather ECperf Data
    data = ecperf.get_throughput_data(ecperf_results_dir)
    data_no_gc = ecperf.get_throughput_no_gc_data(ecperf_results_dir)

    ecperf_data = mfgraph.merge_data(group_title = "ECperf", data = data)
    ecperf_data_no_gc = mfgraph.merge_data(group_title = "ECperf no GC", data = data_no_gc)

    # Graph ECperf Data
    base = ecperf_data[1][1]
    linear = [[1, 1*base], [2, 2*base], [4, 4*base], [6, 6*base], [8, 8*base], [10, 10*base], [12, 12*base], [14, 14*base]]

    linear_data = mfgraph.merge_data(group_title = "Linear", data = linear)
    lines = [linear_data, ecperf_data, ecperf_data_no_gc]

    graph_out = mfgraph.line_graph(lines,
                                   title = "ECperf",
                                   xlabel = xlabel,
                                   ylabel = ylabel,
                                   #title_fontsize = "12",
                                   #label_fontsize = "8",
                                   #legend_fontsize = "9",
                                   xsize = big_x_size,
                                   ysize = big_y_size,
                                   )
    jgraphs.append(graph_out)

    # Gather SPECjbb Data
    data = jbb.get_throughput_data(jbb_results_dir)
    data_no_gc = jbb.get_throughput_no_gc_data(jbb_results_dir)
            
    jbb_data = mfgraph.merge_data(group_title = "SPECjbb", data = data)
    jbb_data_no_gc = mfgraph.merge_data(group_title = "SPECjbb no GC", data = data_no_gc)

    base = 0
    if (len(jbb_data) > 1):
        #print jbb_data
        base = jbb_data[1][1]/jbb_data[1][0]
        
    linear = [[1, 1*base], [2, 2*base], [4, 4*base], [6, 6*base], [8, 8*base], [10, 10*base], [12, 12*base], [14, 14*base]]
    linear_data = mfgraph.merge_data(group_title = "Linear", data = linear)
    lines = [linear_data, jbb_data, jbb_data_no_gc]
    graph_out = mfgraph.line_graph(lines,
                                   title = "SPECjbb",
                                   xlabel = xlabel,
                                   ylabel = ylabel,
                                   #title_fontsize = "12",
                                   #label_fontsize = "8",
                                   #legend_fontsize = "9",
                                   xsize = big_x_size,
                                   ysize = big_y_size,
                                   )
    jgraphs.append(graph_out)

    # Generate Speedup Graph
    ylabel = "Speedup"
    ecperf_speedup = get_speedup(ecperf_data)
    ecperf_speedup_no_gc = get_speedup(ecperf_data_no_gc)
    jbb_speedup = get_speedup(jbb_data)
    jbb_speedup_no_gc = get_speedup(jbb_data_no_gc)
    linear_speedup = [[1,1], [2,2], [4,4], [6,6], [8,8], [10,10], [12,12] , [14,14]]
    linear_data = mfgraph.merge_data(group_title = "Linear", data = linear_speedup)
    lines = [linear_data, ecperf_speedup, jbb_speedup, ecperf_speedup_no_gc, jbb_speedup_no_gc]
    graph_out = mfgraph.line_graph(lines,
                                   title = "",
                                   xlabel = xlabel,
                                   ylabel = ylabel,
                                   title_fontsize = "12",
                                   label_fontsize = "8",
                                   legend_fontsize = "9",
                                   legend_default = "hjc vjb",
                                   xlabel_location = 15,
                                   xsize = main_x_size,
                                   ysize = main_y_size - .1,
                                   )
    make_eps(graph_out, "speedup_no_gc")
    make_excel_line("speedup_no_gc", lines) 
    
    graph_out = mfgraph.line_graph(lines,
                                   title = title,
                                   xlabel = xlabel,
                                   ylabel = ylabel,
                                   #title_fontsize = "12",
                                   #label_fontsize = "8",
                                   #legend_fontsize = "9",
                                   xsize = big_x_size,
                                   ysize = big_y_size,
                                   )
    jgraphs.append(graph_out)
    

def generate_gc_time(jgraphs, title):
    xlabel = "Processors"
    ylabel = "Time Spent in Garbage Collection (%)"

    # Gather ECperf data
    data = ecperf.get_gc_time_data(ecperf_results_dir)
    ecperf_data = mfgraph.merge_data(group_title = "ECperf", data = data)

    # Gather SPECjbb data
    data = jbb.get_gc_time_data(jbb_results_dir)
    jbb_data = mfgraph.merge_data(group_title = "SPECjbb", data = data)
    
    lines = [ecperf_data, jbb_data]
    #print lines 
    graph_out = mfgraph.line_graph(lines,
                                   title = title,
                                   xlabel = xlabel,
                                   ylabel = ylabel,
                                   title_fontsize = "12",
                                   label_fontsize = "8",
                                   legend_fontsize = "9",
                                   xsize = big_x_size,
                                   ysize = big_y_size,                               
                                   )
    jgraphs.append(graph_out)
    

def generate_gc_time_per_xact(jgraphs, title):
    xlabel = "Processors"
    ylabel = "GC Time Per Transaction (Normalized)"

    # Gather ECperf data
    data = ecperf.get_gc_time_per_xact_data(ecperf_results_dir)
    ecperf_data = get_speedup(mfgraph.merge_data(group_title = "ECperf", data = data))
    
    # Gather SPECjbb data
    data = jbb.get_gc_time_per_xact_data(jbb_results_dir)
    jbb_data = get_speedup(mfgraph.merge_data(group_title = "SPECjbb", data = data))

    #draw the graph
    lines = [ecperf_data, jbb_data]
    #print lines 
    graph_out = mfgraph.line_graph(lines,
                                   title = title,
                                   xlabel = xlabel,
                                   ylabel = ylabel,
                                   title_fontsize = "12",
                                   label_fontsize = "8",
                                   legend_fontsize = "9",
                                   xsize = big_x_size,
                                   ysize = big_y_size,
                                   )
    jgraphs.append(graph_out)
    

def generate_l2_misses_per_trans(jgraphs, title):
    print "Generating l2 misses/trans graph."
    xlabel = "Processors"
    ylabel = "L2 Misses/Transaction"

    # Gather ECperf Data
    data = ecperf.get_l2_misses_per_xact_data(ecperf_results_dir)
    ecperf_data = mfgraph.merge_data(group_title = "ECperf", data = data)

    # Gather SPECjbb Data
    data = jbb.get_l2_misses_per_xact_data(jbb_results_dir)
    jbb_data = mfgraph.merge_data(group_title = "SPECjbb", data = data)
    
    lines = [ecperf_data, jbb_data]
    graph_out = mfgraph.line_graph(lines,
                                   title = title,
                                   xlabel = xlabel,
                                   ylabel = ylabel,
                                   #  title_fontsize = "12",
#                                     label_fontsize = "8",
#                                     legend_fontsize = "9",
                                   xsize = big_x_size,
                                   ysize = big_y_size,
                                   )
    jgraphs.append(graph_out)


    
def generate_l2_miss_rate(jgraphs, title):
    print "Generating L2 miss rate graph."
    xlabel = "Processors"
    ylabel = "L2 Misses/1000 Instructions"

    # Gather ECperf Data
    data = ecperf.get_l2_miss_rate_data(ecperf_results_dir)
    ecperf_data = mfgraph.merge_data(group_title = "ECperf", data = data)

    # Gather SPECjbb Data
    data = jbb.get_l2_miss_rate_data(jbb_results_dir)
    jbb_data = mfgraph.merge_data(group_title = "SPECjbb", data = data)

    lines = [ecperf_data, jbb_data]
    graph_out = mfgraph.line_graph(lines,
                                   title = title,
                                   xlabel = xlabel,
                                   ylabel = ylabel,
                                   #title_fontsize = "12",
                                   #label_fontsize = "8",
                                   #legend_fontsize = "9",
                                   xsize = big_x_size,
                                   ysize = big_y_size,
                                   )
    jgraphs.append(graph_out)

def generate_l2_miss_rate_by_input(jgraphs, title):
    print "Generating L2 miss rate by input graph."
    xlabel = "Scale Factor"
    ylabel = "L2 Misses/1000 Instructions"

    # Gather ECperf Data
    data = ecperf.get_l2_miss_rate_by_input_data(ecperf_tx_dir)
    ecperf_data = mfgraph.merge_data(group_title = "ECperf", data = data)

    # Gather SPECjbb Data
    data = jbb.get_l2_miss_rate_by_input_data(jbb_warehouse_dir)
    jbb_data = mfgraph.merge_data(group_title = "SPECjbb", data = data)

    lines = [ecperf_data, jbb_data]
    graph_out = mfgraph.line_graph(lines,
                                   title = title,
                                   xlabel = xlabel,
                                   ylabel = ylabel,
                                   #title_fontsize = "12",
                                   #label_fontsize = "8",
                                   #legend_fontsize = "9",
                                   xsize = big_x_size,
                                   ysize = big_y_size,
                                   )
    jgraphs.append(graph_out)
    

def generate_inst_per_trans(jgraphs, title):
    print "Generating Inst/Trans graph."
    xlabel = "Processors"
    ylabel = "Instructions/Transaction"

    # Gather ECperf Data
    data = ecperf.get_inst_per_xact_data(ecperf_results_dir)
    ecperf_data = mfgraph.merge_data(group_title = "ECperf", data = data)
    
    # Gather SPECjbb Data
    data = jbb.get_inst_per_xact_data(jbb_results_dir)
    jbb_data = mfgraph.merge_data(group_title = "SPECjbb", data = data)

    lines = [ecperf_data]
    graph_out = mfgraph.line_graph(lines,
                                   title = title,
                                   xlabel = xlabel,
                                   ylabel = ylabel,
                                   #title_fontsize = "12",
                                   #label_fontsize = "8",
                                   #legend_fontsize = "9",
                                   xsize = big_x_size,
                                   ysize = big_y_size,
                                   )
    jgraphs.append(graph_out)

    lines = [jbb_data]
    graph_out = mfgraph.line_graph(lines,
                                   title = title,
                                   xlabel = xlabel,
                                   ylabel = ylabel,
                                   #title_fontsize = "12",
                                   #label_fontsize = "8",
                                   #legend_fontsize = "9",
                                   xsize = big_x_size,
                                   ysize = big_y_size,
                                   )
    jgraphs.append(graph_out)

def generate_cpi(jgraphs, title):
    xlabel = "Processors"
    ylabel = "Cycles Per Instruction (CPI)"

    # ECperf
    data = ecperf.get_cpi_data(ecperf_results_dir, "COMBINED")
    u_data = ecperf.get_cpi_data(ecperf_results_dir, "USER")
    k_data = ecperf.get_cpi_data(ecperf_results_dir, "KERNEL")
    
    ecperf_data = mfgraph.merge_data(group_title = "ECperf", data = data)
    u_ecperf_data = mfgraph.merge_data(group_title = "User", data = u_data)
    k_ecperf_data = mfgraph.merge_data(group_title = "Kernel", data = k_data)

    print "ecperf_data"
    print ecperf_data
    print u_ecperf_data
    print k_ecperf_data

    # Graph ECperf only
    graph_out = mfgraph.line_graph([ecperf_data, u_ecperf_data, k_ecperf_data],
                                   title = title + " (ECperf)",
                                   xlabel = xlabel,
                                   ylabel = ylabel,
                                   #title_fontsize = "12",
                                   #label_fontsize = "8",
                                   #legend_fontsize = "9",
                                   xsize = big_x_size,
                                   ysize = big_y_size,
                                   )
    jgraphs.append(graph_out)

    # SPECjbb
    data = jbb.get_cpi_data(jbb_results_dir, "COMBINED")
    u_data = jbb.get_cpi_data(jbb_results_dir, "USER")
    k_data = jbb.get_cpi_data(jbb_results_dir, "KERNEL")
        
    jbb_data = mfgraph.merge_data(group_title = "SPECjbb", data = data)
    u_jbb_data = mfgraph.merge_data(group_title = "User", data = u_data)
    k_jbb_data = mfgraph.merge_data(group_title = "Kernel", data = k_data)
    
    print "jbb_data"
    print jbb_data
    print u_jbb_data
    print k_jbb_data

    # Graph SPECjbb only
    graph_out = mfgraph.line_graph([jbb_data, u_jbb_data, k_jbb_data],
                                   title = title + " (SPECjbb)",
                                   xlabel = xlabel,
                                   ylabel = ylabel,
                                   #title_fontsize = "12",
                                   #label_fontsize = "8",
                                   #legend_fontsize = "9",
                                   xsize = big_x_size,
                                   ysize = big_y_size,
                                   )
    jgraphs.append(graph_out)
    
    # Main Graph
    lines = [ecperf_data, jbb_data]
    graph_out = mfgraph.line_graph(lines,
                                   title = "",
                                   xlabel = xlabel,
                                   ylabel = ylabel,
                                   title_fontsize = "12",
                                   label_fontsize = "8",
                                   legend_fontsize = "9",
                                   legend_default = "hjc vjb",
                                   xlabel_location = 15,
                                   xsize = main_x_size,
                                   ysize = main_y_size,
                                   )
    make_eps(graph_out, "cpi_scaling")
    make_excel_line("cpi_scaling", lines)
    graph_out = mfgraph.line_graph(lines,
                                   title = "",
                                   xlabel = xlabel,
                                   ylabel = ylabel,
                                   #title_fontsize = "12",
                                   #label_fontsize = "8",
                                   #legend_fontsize = "9",
                                   xlabel_location = 15,
                                   xsize = big_x_size,
                                   ysize = big_y_size,
                                   )
    jgraphs.append(graph_out)

def generate_c2c_ratio(jgraphs, title):
    xlabel = "Processors"
    ylabel = "Cache-to-Cache Ratio (%)"

    # ECperf
    data = ecperf.get_c2c_ratio_data(ecperf_results_dir, "COMBINED")
    u_data = ecperf.get_c2c_ratio_data(ecperf_results_dir, "USER")
    k_data = ecperf.get_c2c_ratio_data(ecperf_results_dir, "KERNEL")
        
    ecperf_data = mfgraph.merge_data(group_title = "ECperf", data = data)
    u_ecperf_data = mfgraph.merge_data(group_title = "User", data = u_data)
    k_ecperf_data = mfgraph.merge_data(group_title = "Kernel", data = k_data)

    # Graph ECperf only
    graph_out = mfgraph.line_graph([ecperf_data, u_ecperf_data, k_ecperf_data],
                                   title = title + " (ECperf)",
                                   xlabel = xlabel,
                                   ylabel = ylabel,
                                   #title_fontsize = "12",
                                   #label_fontsize = "8",
                                   #legend_fontsize = "9",
                                   xsize = big_x_size,
                                   ysize = big_y_size,
                                   ymax = 100
                                   )
    jgraphs.append(graph_out)

    # SPECjbb
    data = jbb.get_c2c_ratio_data(jbb_results_dir, "COMBINED")
    u_data = jbb.get_c2c_ratio_data(jbb_results_dir, "USER")
    k_data = jbb.get_c2c_ratio_data(jbb_results_dir, "KERNEL")
        
    jbb_data = mfgraph.merge_data(group_title = "SPECjbb", data = data)
    u_jbb_data = mfgraph.merge_data(group_title = "User", data = u_data)
    k_jbb_data = mfgraph.merge_data(group_title = "Kernel", data = k_data)

    # Graph SPECjbb only
    graph_out = mfgraph.line_graph([jbb_data, u_jbb_data, k_jbb_data],
                                   title = title + " (SPECjbb)",
                                   xlabel = xlabel,
                                   ylabel = ylabel,
                                   #title_fontsize = "12",
                                   #label_fontsize = "8",
                                   #legend_fontsize = "9",
                                   xsize = big_x_size,
                                   ysize = big_y_size,
                                   )
    jgraphs.append(graph_out)
    
    # Main Graph
    lines = [ecperf_data, jbb_data]
    graph_out = mfgraph.line_graph(lines,
                                   title = "",
                                   xlabel = xlabel,
                                   ylabel = ylabel,
                                   title_fontsize = "12",
                                   label_fontsize = "8",
                                   legend_fontsize = "9",
                                   #legend_default = "hjc vjb",
                                   legend_x = 25,
                                   legend_y = 90,
                                   xlabel_location = 15,
                                   xsize = main_x_size - .1,
                                   ysize = main_y_size,
                                   ymax = 100
                                   )
    make_eps(graph_out, "c2c_ratio")
    make_excel_line("c2c_ratio", lines)
    
    graph_out = mfgraph.line_graph(lines,
                                   title = "",
                                   xlabel = xlabel,
                                   ylabel = ylabel,
                                   legend_x = 65,
                                   legend_y = 80,
                                   title_fontsize = "12",
                                   label_fontsize = "8",
                                   legend_fontsize = "9",
                                   xlabel_location = 15,
                                   xsize = big_x_size,
                                   ysize = big_y_size,
                                   ymax = 100
                                   )
    jgraphs.append(graph_out)
    
   
def generate_c2c_rate(jgraphs, title):
    xlabel = "Processors"
    ylabel = "Cache-to-Cache Misses/1000 Instructions"

    # ECperf
    data = ecperf.get_c2c_rate_data(ecperf_results_dir, "COMBINED")
    u_data = ecperf.get_c2c_rate_data(ecperf_results_dir, "USER")
    k_data = ecperf.get_c2c_rate_data(ecperf_results_dir, "KERNEL")
        
    ecperf_data = mfgraph.merge_data(group_title = "ECperf", data = data)
    u_ecperf_data = mfgraph.merge_data(group_title = "User", data = u_data)
    k_ecperf_data = mfgraph.merge_data(group_title = "Kernel", data = k_data)

    # Graph ECperf only
    graph_out = mfgraph.line_graph([ecperf_data, u_ecperf_data, k_ecperf_data],
                                   title = title + " (ECperf)",
                                   xlabel = xlabel,
                                   ylabel = ylabel,
                                   #title_fontsize = "12",
                                   #label_fontsize = "8",
                                   #legend_fontsize = "9",
                                   xsize = big_x_size,
                                   ysize = big_y_size,
                                   )
    jgraphs.append(graph_out)

    # SPECjbb
    data = jbb.get_c2c_rate_data(jbb_results_dir, "COMBINED")
    u_data = jbb.get_c2c_rate_data(jbb_results_dir, "USER")
    k_data = jbb.get_c2c_rate_data(jbb_results_dir, "KERNEL")
        
    jbb_data = mfgraph.merge_data(group_title = "SPECjbb", data = data)
    u_jbb_data = mfgraph.merge_data(group_title = "User", data = u_data)
    k_jbb_data = mfgraph.merge_data(group_title = "Kernel", data = k_data)

    # Graph SPECjbb only
    graph_out = mfgraph.line_graph([jbb_data, u_jbb_data, k_jbb_data],
                                   title = title + " (SPECjbb)",
                                   xlabel = xlabel,
                                   ylabel = ylabel,
                                   #title_fontsize = "12",
                                   #label_fontsize = "8",
                                   #legend_fontsize = "9",
                                   xsize = big_x_size,
                                   ysize = big_y_size,
                                   )
    jgraphs.append(graph_out)
    
    # Main Graph
    lines = [ecperf_data, jbb_data]
    graph_out = mfgraph.line_graph(lines,
                                   title = "",
                                   xlabel = xlabel,
                                   ylabel = ylabel,
                                   title_fontsize = "12",
                                   label_fontsize = "8",
                                   legend_fontsize = "9",
                                   legend_default = "hjc vjb",
                                   xlabel_location = 15,
                                   xsize = main_x_size,
                                   ysize = main_y_size,
                                   )
    make_eps(graph_out, "c2c_rate")
    make_excel_line("c2c_rate", lines)
    lines = [ecperf_data, u_ecperf_data, k_ecperf_data]


def generate_c2c_per_trans(jgraphs, title):
    xlabel = "Processors"
    ylabel = "Cache-to-Cache Transfers/Transaction"

    # Gather ECperf Data
    data = ecperf.get_c2c_per_xact_data(ecperf_results_dir)
    ecperf_data = mfgraph.merge_data(group_title = "ECperf", data = data)

    lines = [ecperf_data]
    graph_out = mfgraph.line_graph(lines,
                                   title = title,
                                   xlabel = xlabel,
                                   ylabel = ylabel,
                                   #title_fontsize = "12",
                                   #label_fontsize = "8",
                                   #legend_fontsize = "9",
                                   xsize = big_x_size,
                                   ysize = big_y_size,
                                   )
    jgraphs.append(graph_out)
    

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
    
def generate_time_breakdown(jgraphs, title):

    # Gather ECperf data    
    tuple_list = ecperf.get_time_breakdown_data(ecperf_results_dir)

    bar_fields = ["usr", "sys", "wt", "idl", "gc_idle"]
    ecperf_data = make_bars(fields=bar_fields, data=tuple_list)
    ecperf_bars = make_stacked(ecperf_data)
    
    ecperf_bars.sort()
    ecperf_bars.insert(0, "ECperf")
    ecperf_data.sort()
    ecperf_data.insert(0, "ECperf")
    print ecperf_bars

    # Gather SPECjbb data
    tuple_list = jbb.get_time_breakdown_data(jbb_results_dir)

    jbb_data = make_bars(fields=bar_fields, data=tuple_list)
    jbb_bars = make_stacked(jbb_data)

    jbb_bars.sort()
    jbb_bars.insert(0, "SPECjbb")

    jbb_data.sort()
    jbb_data.insert(0, "SPECjbb")
    #print "ECperf"
    #print ecperf_bars
    #print "SPECjbb"
    print jbb_bars

    #draw the graph
    graph_bars = [ecperf_bars, jbb_bars]
    excel_bars = [ecperf_data, jbb_data]
    #print graph_bars
    graph_out = mfgraph.stacked_bar_graph(graph_bars,
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
                                          )
    make_eps(graph_out, "mode_breakdown")
    make_excel_bar(name="mode_breakdown",
                   fields=["User", "System", "I/O", "Idle", "GC Idle"],
                   data=excel_bars)
    
    graph_out = mfgraph.stacked_bar_graph(graph_bars,
                                          title = title,
                                          bar_segment_labels = ["User", "System", "I/O", "Idle", "GC Idle"],
                                          ylabel = "Execution Time (%)",
                                          ymax = 100,
                                          ymin = 0,
                                          xsize = big_x_size,
                                          ysize = 3.0,
                                          #title_fontsize = "12",
                                          #label_fontsize = "8",
                                          #bar_name_font_size = "8",
                                          #legend_fontsize = "9",
                                          #stack_name_font_size = "10",
                                          #stack_name_location = 9,
                                          bar_space = 1.2,
                                          colors = [".9 .9 .9",
                                                    ".3 .3 .4",
                                                    ".1 .1 .8",
                                                    ".8 .1 .1",
                                                    "0 0 0"]
                                          )
    jgraphs.append(graph_out)


def generate_datastall_breakdown(jgraphs, title, mode):
    ylabel = "Fraction of Data Stall Time"
    
    # constants (in cycles)
    l2_lat = 6
    mem_lat = 67
    c2c_lat = 95
   
    # mode
    #mode = "USER"

    # Gather ECperf data    
    data = ecperf.get_data_stall_breakdown_data(results_dir=ecperf_results_dir,
                                                mode=mode,
                                                l2_lat=l2_lat,
                                                mem_lat=mem_lat,
                                                c2c_lat=c2c_lat)

    ecperf_data = make_bars(fields=["store_buf", "raw", "oth", "l2_hit", "c2c", "mem"],
                            data=data)
    ecperf_bars = make_stacked(ecperf_data)

    ecperf_bars.sort()
    ecperf_bars.insert(0, "ECperf")

    ecperf_data.sort()
    ecperf_data.insert(0, "ECperf")

    # SPECjbb
    # Gather SPECjbb data
    data = jbb.get_data_stall_breakdown_data(results_dir=jbb_results_dir,
                                             mode=mode,
                                             l2_lat=l2_lat,
                                             mem_lat=mem_lat,
                                             c2c_lat=c2c_lat)   
    
    jbb_data = make_bars(fields=["store_buf", "raw", "oth", "l2_hit", "c2c", "mem"],
                         data=data)
    jbb_bars = make_stacked(jbb_data)
    
    jbb_bars.sort()
    jbb_bars.insert(0, "SPECjbb")

    jbb_data.sort()
    jbb_data.insert(0, "SPECjbb")

    #draw the graph
    graph_bars = [ecperf_bars, jbb_bars]
    graph_data = [ecperf_data, jbb_data]
    #graph_bars = [ecperf_bars]
    #print graph_bars
    graph_out = mfgraph.stacked_bar_graph(graph_bars,
                                          title = "",
                                          bar_segment_labels = ["Store Buf", "RAW", "Other", "L2 Hit", "C2C", "Mem"],
                                          #ymax = 20,
                                          ymin = 0,
                                          ylabel = ylabel,
                                          title_fontsize = "12",
                                          label_fontsize = "8",
                                          bar_name_font_size = "8",
                                          legend_fontsize = "9",
                                          stack_name_font_size = "10",
                                          stack_name_location = 9,
                                          xsize = 4.0,
                                          ysize = 1.778,
                                          bar_space = 1.2,
                                          colors = ["1 1 1",
                                                    ".9 .9 .9",
                                                    ".8 .8 .8",
                                                    ".6 .6 .6",
                                                    ".4 .4 .4",
                                                    "0 0 0"]
                                          )
    make_eps(graph_out, "datastall_" + mode)
    make_excel_bar(name="datastall_" + mode,
                    fields=["Store Buf", "RAW", "Other", "L2 Hit", "C2C", "Mem"],
                    data=graph_data)

    graph_out = mfgraph.stacked_bar_graph(graph_bars,
                                          title = title,
                                          bar_segment_labels = ["Store Buf", "RAW", "Other", "L2 Hit", "C2C", "Mem"],
                                          #ymax = 20,
                                          ymin = 0,
                                          ylabel = ylabel,
                                          #title_fontsize = "12",
                                          #label_fontsize = "8",
                                          #bar_name_font_size = "8",
                                          #legend_fontsize = "9",
                                          #stack_name_font_size = "10",
                                          stack_name_location = 9,
                                          xsize = big_x_size,
                                          ysize = 3.0,
                                          bar_space = 1.2,
                                          colors = ["1 1 1",
                                                    ".9 .9 .9",
                                                    ".7 .7 .7",
                                                    ".5 .5 .5",
                                                    ".3 .3 .3",
                                                    "0 0 0"]
                                          )
    jgraphs.append(graph_out)

def generate_cpi_breakdown(jgraphs, title, mode):


    # Gather ECperf data    
    data = ecperf.get_cpi_breakdown_data(ecperf_results_dir,
                                         mode)

    ecperf_data = make_bars(fields=["oth", "inst_stall", "data_stall"],
                            data=data)
    ecperf_bars = make_stacked(ecperf_data)

    ecperf_data.sort()
    ecperf_data.insert(0, "ECperf")
    ecperf_bars.sort()
    ecperf_bars.insert(0, "ECperf")


    # SPECjbb
    # Gather SPECjbb data    
    data = jbb.get_cpi_breakdown_data(jbb_results_dir,
                                      mode)

    jbb_data =  make_bars(fields=["oth", "inst_stall", "data_stall"],
                          data=data)
    jbb_bars = make_stacked(jbb_data)

    jbb_data.sort()
    jbb_data.insert(0, "SPECjbb")
    jbb_bars.sort()
    jbb_bars.insert(0, "SPECjbb")

# end SPECjbb

    #draw the graph
    graph_bars = [ecperf_bars, jbb_bars]
    graph_data = [ecperf_data, jbb_data]

    graph_out = mfgraph.stacked_bar_graph(graph_bars,
                                          title = "",
                                          ylabel = "Cycles Per Instruction (CPI)",
                                          bar_segment_labels = ["Other",
                                                                "Instruction Stall",
                                                                "Data Stall",
                                                                ],
                                          ymax = 3.0,
                                          ymin = 0,
                                          title_fontsize = "12",
                                          label_fontsize = "8",
                                          bar_name_font_size = "8",
                                          legend_fontsize = "9",
                                          stack_name_font_size = "10",
                                          stack_name_location = 9,
                                          xsize = 4.0,
                                          ysize = 1.778,
                                          bar_space = 1.2,
                                          colors = ["1 1 1",
                                                    #".9 .9 .9",
                                                    ".7 .7 .7",
                                                    ".5 .5 .5",
                                                    #".3 .3 .3",
                                                    "0 0 0"]
                                          )
    make_eps(graph_out, "cpi_breakdown_" + mode)
    make_excel_bar(name="cpi_breakdown_" + mode,
                   fields=["Instruction Stall",
                           "Data Stall",
                           "Other",
                           ],
                   data = graph_data)

    graph_out = mfgraph.stacked_bar_graph(graph_bars,
                                          title = "%s (%s)" % (title, mode),
                                          ylabel = "Execution Time (%)",
                                          bar_segment_labels = ["Instruction Stall",
                                                                "Data Stall",
                                                                "Other",
                                                                ],
                                          ymax = 3.0,
                                          ymin = 0,
                                          #title_fontsize = "12",
                                          #label_fontsize = "8",
                                          #bar_name_font_size = "8",
                                          #legend_fontsize = "9",
                                          #stack_name_font_size = "10",
                                          stack_name_location = 9,
                                          xsize = 6.0,
                                          ysize = 3.0,
                                          bar_space = 1.2,
                                          colors = ["1 1 1",
                                                    #".7 .7 .7",
                                                    ".5 .5 .5",
                                                    #".3 .3 .3",
                                                    "0 0 0"]
                                          )
    jgraphs.append(graph_out)

def generate_mem_use(jgraphs, title):
    xlabel = "Processors"
    ylabel = "Live Memory (in MB)"

    # Gather ECperf data
    before_data = ecperf.get_mem_use_data(ecperf_results_dir, "before")
    after_data = ecperf.get_mem_use_data(ecperf_results_dir, "after")
        
    ecperf_after = mfgraph.merge_data(group_title = "ECperf After", data = after_data)
    ecperf_before = mfgraph.merge_data(group_title = "ECperf Before", data = before_data)
        
    # Gather SPECjbb data
    before_data = jbb.get_mem_use_data(jbb_results_dir, "before")
    after_data = jbb.get_mem_use_data(jbb_results_dir, "after")    

    jbb_after = mfgraph.merge_data(group_title = "SPECjbb", data = after_data)
    jbb_before = mfgraph.merge_data(group_title = "SPECjbb Before", data = before_data)
    
    lines = [ecperf_before, ecperf_after, jbb_before, jbb_after]
    #print lines 
    graph_out = mfgraph.line_graph(lines,
                                   title = "",
                                   xlabel = xlabel,
                                   ylabel = ylabel,
                                   title_fontsize = "12",
                                   label_fontsize = "8",
                                   legend_fontsize = "9",
                                   legend_default = "hjc vjb",
                                   xsize = main_x_size - .125, 
                                   ysize = main_y_size,
                                   )
    make_eps(graph_out, "mem_use")
    make_excel_line("mem_use", lines)
    graph_out = mfgraph.line_graph(lines,
                                   title = title,
                                   xlabel = xlabel,
                                   ylabel = ylabel,
                                   title_fontsize = "14",
                                   label_fontsize = "10",
                                   legend_fontsize = "11",
                                   xsize = big_x_size,
                                   ysize = big_y_size,
                                   )
    jgraphs.append(graph_out)


######################################
# Hardware Counter Input Rate Graphs
######################################

def generate_mem_use_by_input(jgraphs, title):
    xlabel = "Scale Factor"
    ylabel = "Live Memory (in MB)"

    # Gather ECperf data
    after_data = []
    before_data = []
    directories = glob.glob(ecperf_tx_dir + "/*")
    for dir in directories:
        tx_rate = ecperf.ecperf_get_tx_rate(dir)
        mem_use_after = ecperf.ecperf_get_avg_mem_use_after(dir)
        mem_use_before = ecperf.ecperf_get_avg_mem_use_before(dir)
        after_data.append([tx_rate, mem_use_after])
        before_data.append([tx_rate, mem_use_before])
        
    ecperf_after = mfgraph.merge_data(group_title = "ECperf", data = after_data)
    ecperf_before = mfgraph.merge_data(group_title = "ECperf Before", data = before_data)

    lines = [ecperf_before, ecperf_after]
    graph_out = mfgraph.line_graph(lines,
                                   title = title + " (ECperf)",
                                   xlabel = "Transaction Rate",
                                   ylabel = ylabel,
                                   title_fontsize = "12",
                                   label_fontsize = "8",
                                   legend_fontsize = "9",
                                   xsize = big_x_size,
                                   ysize = big_y_size,
                                   xmin = 0
                                   )
    jgraphs.append(graph_out)
    
    # SPECjbb
    after_data = []
    before_data = []
    files = glob.glob(jbb_warehouse_dir + ".*/*.results")
    for file in files:
        #print file
        sequence = jbb.jbb_get_sequence(file)
        warehouses = jbb.jbb_get_warehouses(file)
        w_dir = "%s.%d" % (jbb_warehouse_dir, warehouses)
        mem_use_after = jbb.jbb_get_avg_mem_after(
            sequence = sequence,
            directory = w_dir
            )
        mem_use_before = jbb.jbb_get_avg_mem_before(
            sequence = sequence,
            directory = w_dir
            )
        after_data.append([warehouses, mem_use_after])
        before_data.append([warehouses, mem_use_before])
        #print [warehouses, mem_use_before]

    after_data.sort()
    jbb_after = mfgraph.merge_data(group_title = "SPECjbb", data = after_data)
    jbb_before = mfgraph.merge_data(group_title = "SPECjbb Before", data = before_data)
    
    lines = [jbb_before, jbb_after]
    #print lines 
    graph_out = mfgraph.line_graph(lines,
                                   title = title + " (SPECjbb)",
                                   xlabel = "Number of Warehouses",
                                   ylabel = ylabel,
                                   title_fontsize = "12",
                                   label_fontsize = "8",
                                   legend_fontsize = "9",
                                   xsize = big_x_size,
                                   ysize = big_y_size,
                                   xmin = 0,
                                   )
    jgraphs.append(graph_out)

    lines = [ecperf_after, jbb_after]
    #print lines 
    graph_out = mfgraph.line_graph(lines,
                                   title = title,
                                   xlabel = xlabel,
                                   ylabel = ylabel,
                                   title_fontsize = "12",
                                   label_fontsize = "8",
                                   legend_fontsize = "9",
                                   xsize = big_x_size,
                                   ysize = big_y_size,
                                   xmin = 0,
                                   )
    jgraphs.append(graph_out)
    
    lines = [ecperf_after, jbb_after]
    graph_out = mfgraph.line_graph(lines,
                                   title = "",
                                   xlabel = xlabel,
                                   ylabel = ylabel,
                                   title_fontsize = "12",
                                   label_fontsize = "8",
                                   legend_fontsize = "9",
                                   legend_default = "hjc vjb",
                                   xsize = main_x_size - .125, 
                                   ysize = main_y_size,
                                   xmin = 0,
                                   )
    make_eps(graph_out, "mem_use_input")
    make_excel_line("mem_use_input", lines)

def generate_gc_time_per_xact_by_input(jgraphs, title):
    print "starting generate_gc_time_per_xact_by_input..."
    xlabel = "Input Rate (ECperf: Driver tx_rate, SPECjbb: #Warehouses)"
    ylabel = "GC Time Per Transaction (Normalized)"

    # Gather ECperf data
    data = ecperf.get_gc_time_per_xact_by_input_data(ecperf_tx_dir)
    ecperf_data = get_speedup(mfgraph.merge_data(group_title = "ECperf", data = data))    
    
    # Gather SPECjbb data
    data = jbb.get_gc_time_per_xact_by_input_data(jbb_warehouse_dir)
    print "GC_TIME_per_xact Data: "
    print data
    jbb_data = get_speedup(mfgraph.merge_data(group_title = "SPECjbb", data = data))

    #draw the graph
    lines = [ecperf_data, jbb_data]
    #print lines 
    graph_out = mfgraph.line_graph(lines,
                                   title = title,
                                   xlabel = xlabel,
                                   ylabel = ylabel,
                                   ymax = 40,
                                   #y_precision = 2.0,
                                   title_fontsize = "12",
                                   label_fontsize = "8",
                                   legend_fontsize = "9",
                                   xsize = big_x_size,
                                   ysize = big_y_size,
                                   )
    jgraphs.append(graph_out)

    
    

#####################################
# Ruby Results
#####################################

def ruby_get_cache_size(file):
    grep_lines = mfgraph.grep(file, ".+cache_size_Kbytes.+")
    line = string.split(grep_lines[0])
    cache_size = int(line[1])
    return cache_size

def ruby_get_cache_assoc(file):
    grep_lines = mfgraph.grep(file, ".+cache_associativity.+")
    line = string.split(grep_lines[0])
    cache_assoc = int(line[1])
    return cache_assoc

def ruby_get_l1d_misses(file):
    grep_lines = mfgraph.grep(file, ".+L1D_cache_total_misses.+")
    line = string.split(grep_lines[0])
    misses = float(line[1])
    return misses

def ruby_get_l1i_misses(file):
    grep_lines = mfgraph.grep(file, ".+L1I_cache_total_misses.+")
    line = string.split(grep_lines[0])
    misses = float(line[1])
    return misses

def ruby_get_instructions(file):
    grep_lines = mfgraph.grep(file, "instruction_executed.+")
    line = string.split(grep_lines[0])
    inst = float(line[1])
    return inst

def ruby_get_miss_rate(result_dir, group_title, associativity, access_type):
    data = []
    files = glob.glob("%s/*%dw**.stats" % (result_dir, associativity))
    for file in files:
        size = ruby_get_cache_size(file)
        inst = ruby_get_instructions(file)
        if(access_type == "data"):
            misses = ruby_get_l1d_misses(file)
        else:
            misses = ruby_get_l1i_misses(file)
        
        missrate = 1000.0*(misses/inst)
        if missrate < .10:
            missrate = .10

        data.append([size, missrate])
        #print [os.path.basename(file), size, missrate]
    output = mfgraph.merge_data(group_title = group_title, data = data)
    #for tuple in output:
         #print tuple

    return output

    
def generate_ruby_miss_rate(jgraphs, title, access_type):
    xlabel = "Cache Size (in KB)"
    ylabel = "Misses/1000 Instructions"

    if access_type == "data":
        eps_file = "data_missrate"
        ymin = .5
    else:
        eps_file = "inst_missrate"
        ymin = .1

    #ecperf_1w = ruby_get_miss_rate(ecperf_ruby_dir, "ECperf 1-Way", 1, access_type)
    ecperf_4w = ruby_get_miss_rate(ecperf_ruby_dir, "ECperf", 4, access_type)
    #ecperf_lines = [ecperf_1w, ecperf_4w]
    ecperf_lines = [ecperf_4w]

    jbb_lines = []
    colors = ["1 0 0", "0 0 1"]
    for entry in jbb_ruby_dirs:
        #jbb_1w    = ruby_get_miss_rate(entry[1], entry[0] + "-1-Way", 1, access_type)
        jbb_4w    = ruby_get_miss_rate(entry[1], entry[0], 4, access_type)
        #jbb_lines.append(jbb_1w)
        jbb_lines.append(jbb_4w)
        colors.append("1 0 0")
        colors.append("0 0 1")
        

    lines = ecperf_lines + jbb_lines
    #print lines 
    graph_out = mfgraph.line_graph(lines,
                                   title = "",
                                   xlabel = xlabel,
                                   ylabel = ylabel,
                                   xlog = 4,
                                   ylog = 10,
                                   ymin = ymin,
                                   title_fontsize = "12",
                                   label_fontsize = "8",
                                   legend_fontsize = "9",
                                   #legend_default = "hjc vjb",
                                   legend_x = 65,
                                   legend_y = 70,
                                   xlabel_location = 15,
                                   y_precision = 1,
                                   xsize = main_x_size - .25,
                                   ysize = main_y_size,
                                   colors = colors
                                   )
    make_eps(graph_out, eps_file)
    make_excel_line(eps_file, lines)
    #print lines

    # ECperf
    graph_out = mfgraph.line_graph(ecperf_lines,
                                   title = title + "(ECperf)",
                                   xlabel = xlabel,
                                   ylabel = ylabel,
                                   xlog = 4,
                                   ylog = 10,
                                   ymin = ymin,
                                   #y_precision = 2,
                                   #x_precision = 2,
                                   #title_fontsize = "12",
                                   #label_fontsize = "8",
                                   #legend_fontsize = "9",
                                   xsize = big_x_size,
                                   ysize = big_y_size,
                                   y_precision = 1,
                                   colors = colors
                                   )
    jgraphs.append(graph_out)   

    # SPECjbb
    graph_out = mfgraph.line_graph(jbb_lines,
                                   title = title + "(SPECjbb)",
                                   xlabel = xlabel,
                                   ylabel = ylabel,
                                   xlog = 2,
                                   ylog = 10,
                                   ymin = ymin,
                                   #title_fontsize = "12",
                                   #label_fontsize = "8",
                                   #legend_fontsize = "9",
                                   y_precision = 1,
                                   xsize = big_x_size,
                                   ysize = big_y_size,
                                   colors = colors
                                   )
    jgraphs.append(graph_out)

    graph_out = mfgraph.line_graph(lines,
                                   title = title,
                                   xlabel = xlabel,
                                   ylabel = ylabel,
                                   xlog = 2,
                                   ylog = 10,
                                   ymin = ymin,
                                   #title_fontsize = "12",
                                   #label_fontsize = "8",
                                   #legend_fontsize = "9",
                                   y_precision = 2,
                                   xsize = big_x_size,
                                   ysize = big_y_size,
                                   colors = colors
                                   )
    jgraphs.append(graph_out)
    
    

def generate_ruby_inst_miss_rate(jgraphs, title):
    xlabel = "Cache Size (in KB)"
    ylabel = "Misses/1000 Instructions"
    
    #ecperf_1w = get_inst_miss_rate(ecperf_ruby_dir, "ECperf 1-Way", 1)
    ecperf_4w = get_inst_miss_rate(ecperf_ruby_dir, "ECperf", 4)
    #lines = [ecperf_1w, ecperf_4w]
    lines = []
    for entry in jbb_ruby_dirs:
        #jbb_1w    = get_inst_miss_rate(entry[1], entry[0] + "-1-Way", 1)
        jbb_4w    = get_inst_miss_rate(entry[1], entry[0], 4)
        #lines.append(jbb_1w)
        lines.append(jbb_4w)
    

    #print lines 
    graph_out = mfgraph.line_graph(lines,
                                   title = "",
                                   xlabel = xlabel,
                                   ylabel = ylabel,
                                   xlog = 2,
                                   ylog = 10,
                                   ymin = 0.1,
                                   y_precision = 1,
                                   title_fontsize = "12",
                                   label_fontsize = "8",
                                   legend_fontsize = "8",
                                   legend_default = "hjc vjb",
                                   xlabel_location = 15,
                                   xsize = main_x_size,
                                   ysize = main_y_size,
                                   )
    make_eps(graph_out, "inst_missrate")
    make_excel_line("inst_missrate", lines)
    graph_out = mfgraph.line_graph(lines,
                                   title = title,
                                   xlabel = xlabel,
                                   ylabel = ylabel,
                                   xlog = 2,
                                   ylog = 10,
                                   ymin = 0.1,
                                   #title_fontsize = "12",
                                   #label_fontsize = "8",
                                   #legend_fontsize = "8",
                                   y_precision = 1,
                                   xsize = big_x_size,
                                   ysize = big_y_size,
                                   )
    jgraphs.append(graph_out)

def parse_sumo_data(filename, group_name, c_max, type):
    file = open(filename,'r')
    data = [group_name]

    # 0 - looking for header
    # 1 - header found
    # 2 - valid data
    # 3 - past useful data
    #c_max = 128

    # get num cache lines touched
    num_lines = -1
    if type == "percent":
        for line in tail(filename):
            tokens = string.split(line)
            if len(tokens) == 3:
                num_lines = int(tokens[0])
                percent_misses = float(tokens[2])
                #if percent_misses != 100.0:
                #print line
    print "%s, total lines = %d" % (group_name, num_lines)

    last_val = -1.0
    line_count = 0
    state = 0
    while state != 3:
        line = file.readline()
        if state == 0:
            # Stop if we read 1000 lines without finding
            # the right header
            if line_count >= 1000:
                return []
            line_count += 1
            if re.match(".+misses broken down per cacheline", line):
                state += 1
        elif state == 1:
            print "found header, skipping: %s " % line
            state += 1
        elif state == 2:
            tokens = string.split(line)
            if len(tokens) == 3:
                cacheline = float(tokens[0])
                if type != "percent":
                    cacheline += 1
                percent_lines = 100.0*cacheline/num_lines
                percent_misses = float(tokens[2])
                if percent_misses != last_val:
                    if type == "percent":
                        tuple = [percent_lines, percent_misses]
                    else:
                        #if cacheline == 0:
                        #cacheline = 1
                        
                        #tuple = [cacheline * 64/1000, percent_misses]
                        tuple = [cacheline, percent_misses]

                    data.append(tuple)
                    last_val = percent_misses

                if type == "percent":
                    if percent_lines > .1:
                        if percent_lines < .1001:
                            print tuple
                            
                if percent_misses > 85:
                    if percent_misses < 85.001:
                        print tuple
                
                #if percent_misses == 100.0:
                if percent_misses >= 100.0:
                    state += 1
                    print tuple
                    if type == "percent":
                        tuple = [100.0, percent_misses]
                    else:
                        tuple = [c_max, percent_misses]
                    data.append(tuple)
                elif cacheline >= c_max:
                    state += 1
                    if type == "percent":
                        tuple = [percent_lines, percent_misses]
                    else:
                        #tuple = [cacheline * 64/1000, percent_misses]
                        tuple = [cacheline, percent_misses]
                    print tuple
                    data.append(tuple)
        elif state == 3:
            return data
    
    return data

def tail(filename):
    # generate .eps (ghostview-able)
    (in_file, out_file) = os.popen2("tail %s" % filename)
    print "running tail on file %s" + filename
    lines = out_file.readlines()
    out_file.close()
    return lines
    
def generate_sumo_histogram(jgraphs, title, c_max):
    xlabel = "Cache Lines Touched (%)"
    ylabel = "Cache-to-Cache Transfers (%)"
    ecperf_file = "/p/multifacet/users/kmoore/sumo_data/ecperf_new/1x8-1M-4w.sumodata_30"
    jbb_file = "/p/multifacet/users/kmoore/sumo_data/jbb/1x8-1M-4w.sumodata"

    ecperf_data = parse_sumo_data(ecperf_file, "ECperf", c_max, "percent")
    print "ECperf data: %d" % len(ecperf_data)
    #print ecperf_data
    jbb_data = parse_sumo_data(jbb_file, "SPECjbb-25", c_max, "percent")
    print "JBB data: %d" % len(jbb_data)
    #print jbb_data

    lines = [ecperf_data, jbb_data]
    graph_out = mfgraph.line_graph(lines,
                                   title = title,
                                   xlabel = xlabel,
                                   ylabel = ylabel,
                                   ymin = 0.00,
                                   ymax = 100,
                                   #title_fontsize = "12",
                                   #label_fontsize = "8",
                                   #legend_fontsize = "8",
                                   xsize = big_x_size,
                                   ysize = big_y_size,
                                   )    
    jgraphs.append(graph_out)
    graph_out = mfgraph.line_graph(lines,
                                   title = "",
                                   xlabel = xlabel,
                                   ylabel = ylabel,
                                   ymin = 0.00,
                                   ymax = 100,
                                   title_fontsize = "12",
                                   label_fontsize = "8",
                                   legend_fontsize = "8",
                                   legend_default = "hjc vjb",
                                   xsize = main_x_size -.1,
                                   ysize = main_y_size,
                                   )
    make_eps(graph_out, "histogram")
    make_excel_line("histogram", lines)
    print "finishing histogram"

def generate_sumo_histogram_zoomed(jgraphs, title, c_max):
    xlabel = "Memory Touched by Cache Line (64-Byte Lines)"
    ylabel = "Cache-to-Cache Transfers (%)"
    ecperf_file = "/p/multifacet/users/kmoore/sumo_data/ecperf_new/1x8-1M-4w.sumodata_30"
    jbb_file = "/p/multifacet/users/kmoore/sumo_data/jbb/1x8-1M-4w.sumodata"

    ecperf_data = parse_sumo_data(ecperf_file, "ECperf", c_max, "absolute")
    jbb_data = parse_sumo_data(jbb_file, "SPECjbb-25", c_max, "absolute")

    lines = [ecperf_data, jbb_data]
    graph_out = mfgraph.line_graph(lines,
                                   title = title,
                                   xlabel = xlabel,
                                   ylabel = ylabel,
                                   xmin = 1,
                                   xlog = 4,
                                   ymin = 0.00,
                                   ymax = 100,
                                   #xlog = 10,
                                   #title_fontsize = "12",
                                   #label_fontsize = "8",
                                   #legend_fontsize = "8",
                                   xsize = big_x_size,
                                   ysize = big_y_size,
                                   )    
    jgraphs.append(graph_out)

    graph_out = mfgraph.line_graph(lines,
                                   title = "",
                                   xlabel = xlabel,
                                   ylabel = ylabel,
                                   ymin = 0.00,
                                   ymax = 100,
                                   xmin = 1,
                                   xlog = 16,
                                   title_fontsize = "12",
                                   label_fontsize = "8",
                                   legend_fontsize = "8",
                                   legend_default = "hjc vjb",
                                   xsize = main_x_size - .2,
                                   ysize = main_y_size,
                                   )
    make_eps(graph_out, "histogram_zoomed")
    make_excel_line("histogram_zoomed", lines)
    print "finishing hist zoomed"

def generate_shared_caches(jgraphs, title):
    xlabel = "CPUs Per Shared Cache"
    ylabel = "Misses/1000 Instructions"
    #1x8   (private) 
    #read 0.002056
    #write 0.000876
    
    #2x4
    #read  0.001955
    #write 0.000848
    
    #4x2
    #read  0.001831
    #write 0.000758
    
    #8x1
    #read  0.001636
    #write 0.000604
    #data = [["1x8", 0.002056],
    #        ["2x4", 0.001955],
    #        ["4x2", 0.001831],
    #        ["8x1", 0.001636]]

    # new 30 BBop ECperf 11/1
    #  1x8
    #  Instr  0.001135
    #  Read   0.006412
    #  Write  0.005337
    
    #  2x4    
    #  Instr  0.001163
    #  Read   0.005753
    #  Write  0.004866
    
    #  4x2
    #  Instr  0.001201
    #  Read   0.004581
    #  Write  0.003993
    
    #  8x1
    #  Instr  0.001252
    #  Read   0.002673
    #  Write  0.002455

    #             instruction  read      write
    data = [[1, 0.001135 + 0.006412 + 0.005337],
            [2, 0.001163 + 0.005753 + 0.004866],
            [4, 0.001201 + 0.004581 + 0.003993],
            [8, 0.001252 + 0.002673 + 0.002455]]



    calc_data = []
    for tuple in data:
        new_tuple = [tuple[0], 1000.0*tuple[1]]
        calc_data.append(new_tuple)

    ecperf_data = mfgraph.merge_data("ECperf", calc_data)

    #  Misses per instruction SPECjbb
    #  1x8
    #  Instr   0.000007
    #  Read    0.010068
    #  Write   0.002351
    
    #  2x4    
    #  Instr   0.000005
    #  Read    0.012154
    #  Write   0.002594
    
    #  4x2
    #  Instr   0.000004
    #  Read    0.014876
    #  Write   0.003041
    
    #  8x1
    #  Instr   0.000003
    #  Read    0.017428
    #  Write   0.003731

    #             inst    read   write       inst
    data = [[1, 0.000007 + 0.010068 + 0.002351],
            [2, 0.000005 + 0.012154 + 0.002594],
            [4, 0.000004 + 0.014876 + 0.003041],
            [8, 0.000003 + 0.017428 + 0.003731]]

    calc_data = []
    for tuple in data:
        new_tuple = [tuple[0], 1000.0*tuple[1]]
        calc_data.append(new_tuple)
    jbb_data = mfgraph.merge_data("SPECjbb-25", calc_data)
    
    lines = [ecperf_data, jbb_data]
    graph_out = mfgraph.stacked_bar_graph(lines,
                                          title = title,
                                          xlabel = xlabel,
                                          ylabel = ylabel,
                                          ymin = 0.00,
                                          #ymax = 100,
                                          xlabel_location = 15,
                                          stack_name_location = 9,
                                          #title_fontsize = "12",
                                          #label_fontsize = "8",
                                          #legend_fontsize = "8",
                                          xsize = big_x_size,
                                          ysize = big_y_size,
                                          )    
    jgraphs.append(graph_out)   
    graph_out = mfgraph.stacked_bar_graph(lines,
                                          title = "",
                                          xlabel = "",
                                          ylabel = ylabel,
                                          ymin = 0.00,
                                          #ymax = 100,
                                          xlabel_location = 15,
                                          stack_name_location = 9,
                                          #title_fontsize = "12",
                                          #label_fontsize = "8",
                                          #legend_fontsize = "8",
                                          xsize = 2.75,
                                          ysize = 1.778,
                                          title_fontsize = "12",
                                          label_fontsize = "9",
                                          bar_name_font_size = "9",
                                          legend_fontsize = "9",
                                          stack_name_font_size = "10",
                                          bar_space = 1.2,
                                          )   
    make_eps(graph_out, "shared")
    make_excel_line(name="shared", data=lines)
    # above doesn't work.  FIXME


def generate_gc_theory(jgraphs, title):
    filename = "/p/multifacet/projects/ecperf/multifacet/jgraph/GC_Theory_Data.txt"

    data = []
    data.append(["p1"])
    data.append(["p2"])
    data.append(["p3"])
    data.append(["p4"])
    data.append(["p5"])
    data.append(["p6"])
    data.append(["p7"])
    data.append(["p8"])
    
    state = 0
    file = open(filename,'r')
    for line in file.readlines():
        tokens = string.split(line)
        #print tokens

        if state == 0:
            #print line
            state += 1
        elif state == 1:
            #print line
            state += 1
        elif state == 2:
            if line == "":
                state += 1
            elif tokens == []:
                state += 1
            else:
                time = float(tokens[0])
                tokens = tokens[1:]
                i = 0
                j = 0
                for token in tokens:
                    if j % 5 == 3:
                        if i <= 7:
                            #print [i, j, time, float(token)]
                            data[i].append([time, float(token)])
                            i += 1
                    j += 1
                    
    xlabel = "Time (in s)"
    ylabel = "Cache-to-Cache Tansfers/s (Normalized)"
    graph_out = mfgraph.line_graph(data,
                                   title = title,
                                   xlabel = xlabel,
                                   ylabel = ylabel,
                                   #title_fontsize = "12",
                                   #label_fontsize = "8",
                                   #legend_fontsize = "9",
                                   xsize = big_x_size,
                                   ysize = big_y_size,
                                   )
    jgraphs.append(graph_out)

    graph_out = mfgraph.line_graph(data,
                                   title = "",
                                   xlabel = xlabel,
                                   ylabel = ylabel,
                                   title_fontsize = "12",
                                   label_fontsize = "8",
                                   legend_fontsize = "9",
                                   #legend_default = "hjc vjb",
                                   xsize = main_x_size -.25,
                                   ysize = main_y_size,
                                   )
    make_eps(graph_out, "gc_theory")
    make_excel_line(name="gc_theory", data=data)
    
# Generate graph inputs
jgraph_input = []
generate_throughput(jgraph_input, "Throughput")
#generate_throughput_by_input(jgraph_input, "Throughput vs TX_Rate")
#generate_throughput_no_gc(jgraph_input, "Effect of Garbage Collection on Throughput Scaling")
generate_time_breakdown(jgraph_input, "Time Breakdown")
#generate_cpi(jgraph_input, "CPI")
#generate_cpi_breakdown(jgraph_input, "CPI Breakdown", "USER")
#generate_cpi_breakdown(jgraph_input, "CPI Breakdown", "KERNEL")
#generate_cpi_breakdown(jgraph_input, "CPI Breakdown", "COMBINED")
#generate_l2_misses_per_trans(jgraph_input, "L2 Misses/Transaction")
#generate_inst_per_trans(jgraph_input, "Instructions/Transaction")
#generate_c2c_ratio(jgraph_input, "Cache-to-Cache Transfer Ratio")
#generate_c2c_rate(jgraph_input, "Cache-to-Cache Transfer Rate")
#generate_c2c_per_trans(jgraph_input, "Cache-to-Cache Transfers/Transaction")
#generate_l2_miss_rate(jgraph_input, "L2 Miss Rate")
# generate_l2_miss_rate_by_input(jgraph_input, "L2 Miss Rate vs Scale Factor")
# generate_gc_time(jgraph_input, "Garbage Collection Time")
generate_gc_time_per_xact(jgraph_input, "GC Time/Transaction")
# generate_mem_use(jgraph_input, "Average Heap Use Before and After Garbage Collection")
# generate_mem_use_by_input(jgraph_input, "Average Heap Use vs Input Rate")
#generate_gc_time_per_xact_by_input(jgraph_input, "GC Time/Transaction vs Input Rate")
#generate_ruby_miss_rate(jgraph_input, "Data Miss Rate", "data")
#generate_ruby_miss_rate(jgraph_input, "Instruction Miss Rate", "inst")
#generate_datastall_breakdown(jgraph_input, "Data Stall Breakdown (USER)", "USER")
#generate_datastall_breakdown(jgraph_input, "Data Stall Breakdown (KERNEL)", "KERNEL")
#generate_datastall_breakdown(jgraph_input, "Data Stall Breakdown (COMBINED)", "COMBINED")
#generate_sumo_histogram_zoomed(jgraph_input, "Histogram Zoomed", 1024*256)
#generate_sumo_histogram(jgraph_input, "Histogram", 4*1024*1024)
#generate_shared_caches(jgraph_input, "Shared Cache Loads")
#generate_gc_theory(jgraph_input, "GC Theory")
    
# Generate 1 document with all graphs
print "Graph Inputs Ready...Running Jgraph."
mfgraph.run_jgraph("newpage\n".join(jgraph_input), "hpca_scaling")
