#!/s/std/bin/python
import sys, string, os, glob, re, mfgraph

## configuration
modules_list = ["MOSI_bcast_opt_1", "MOSI_mcast_aggr_1", "MOSI_GS_1"]
norm_module = "BASH"

#modules_list = ["MOSI_bcast_opt_4", "MOSI_mcast_aggr_4", "MOSI_GS_4"]
#norm_module = "MOSI_mcast_aggr_4"

protocol_name = {
    "MOSI_bcast_opt_1" : "Snooping",
    "MOSI_mcast_aggr_1" : "BASH",
    "MOSI_GS_1" : "Directory",
    "MOSI_bcast_opt_4" : "Snooping",
    "MOSI_mcast_aggr_4" : "BASH",
    "MOSI_GS_4" : "Directory",
    }

threshold_list = []
#threshold_list = [0.55, 0.65, 0.70, 0.75, 0.80, 0.85, 0.95]
default_threshold = 0.75

think_time_list = []
default_think_time = 1

processor_list = []
default_processor = 64

bandwidth_list = []

benchmarks = [
    "microbenchmark",
#    "apache_12500",
    "apache_2500",
    "barnes_64k",
    "oltp_1000",
    "slash_50",
    "jbb_100000",
#    "ocean_514",
              ]

workload_name = {
    "apache_12500" : "Apache",
    "apache_2500" : "Apache",
    "barnes_64k" : "Barnes-Hut",
    "oltp_1000" : "OLTP",
    "jbb_100000" : "SPECjbb",
    "ocean_514" : "Ocean",
    "microbenchmark" : "Microbenchmark",
    "slash_50" : "Slashcode",
    }


statistics = [
    ["Ruby_cycles", "performance", mfgraph.make_relative, 1.0, "2000", ".25"],
    ["Ruby_cycles", "normalized performance", lambda x : mfgraph.normalize_list(x, norm_module), 1.2, "2000", ".25"],
    ["links_utilized_percent", "endpoint link utilization (percent)", lambda x : x, 100.0, "150", "20"],
    ["Broadcast_percent", "broadcast rate", lambda x : x, 1.0, "2000", ".25"],
    ]

## functions

def append_if_not_in(list, item):
    if item not in list:
        list.append(item)

def read_data():
    print "reading data..."
    for benchmark in benchmarks:
        for stat in statistics:
            command = "grep %s %s/*.stats" % (stat[0], benchmark)
            results = os.popen(command, "r")
            for line in results.readlines():
                (filename, stat[0], value) = string.split(line, ":")
                value = float(value)
                components = string.split(filename, "-")
                components[1] = string.split(components[1], "p")[0]
                config_list = [benchmark, stat[0]] + components[1:-2]

                if len(config_list) == 5:
                    config_list.append("%f" % 0.75)  # default threshold
                    config_list.append("%d" % 1)     # default think time 

                if config_list[3] == "MOSI_bcast_opt":
                    config_list[3] = "MOSI_bcast_opt_1"

                if config_list[3] == "MOSI_mcast_aggr":
                    config_list[3] = "MOSI_mcast_aggr_1"

                if config_list[3] == "MOSI_GS":
                    config_list[3] = "MOSI_GS_1"

                if config_list[0] == "microbenchmark":
                    if config_list[2] == "64":

                        # Double add the data
                        config = string.join(config_list, " ")
                        data.setdefault(config, []).append(value)
                
                        append_if_not_in(processor_list, string.atoi(config_list[2]))
                        
                        config_list[2] = "16"
                        if config_list[3] == "MOSI_bcast_opt_1":
                            config_list[3] = "MOSI_bcast_opt_4"

                        if config_list[3] == "MOSI_mcast_aggr_1":
                            config_list[3] = "MOSI_mcast_aggr_4"

                        if config_list[3] == "MOSI_GS_1":
                            config_list[3] = "MOSI_GS_4"

                # Add the data
                config = string.join(config_list, " ")
                data.setdefault(config, []).append(value)
                
                # add the various stats to our configuration options
                (benchmark, statistic, processor, module, bandwidth, threshold, think_time) = config_list
                #(benchmark, statistic, processor, module, bandwidth) = config_list
                append_if_not_in(processor_list, string.atoi(processor))
                append_if_not_in(bandwidth_list, string.atoi(bandwidth))
                append_if_not_in(threshold_list, string.atof(threshold))
                append_if_not_in(think_time_list, string.atoi(think_time))

    processor_list.sort()
    bandwidth_list.sort()
    threshold_list.sort()
    think_time_list.sort()

data = {}
def get_data(benchmark, 
             stat = "Ruby_cycles",
             processor = 64,
             module = "MOSI_bcast_opt",
             bandwidth = 1131,
             threshold = 0.75,
             think_time = 1,
             ):
    
    config_str = "%s %s %s %s %d %f %d" % (benchmark, stat, processor, module, bandwidth, threshold, think_time)
    if not data.has_key(config_str):
        print "Config not found: ", config_str
        return []
    else:
        return data[config_str]

def generate_processors(jgraphs, benchmark, stat, ylabel, transformation, ymax):
    ## Generate number of processors vs performance (one graph per bandwidth)
    for bandwidth in bandwidth_list:
        lines = []
        for module in modules_list:
            points = []
            counter = 2
            for processor in processor_list:
                data = get_data(benchmark, processor=processor, module=module, bandwidth=bandwidth, stat=stat)
                if len(data) > 0:
                    value = mfgraph.average(data)
                    stddev = mfgraph.stddev(data)
                    points.append([counter, value, value - stddev, value + stddev])
                counter += 1
                    
            lines.append([module] + points)

        transformation(lines)
        xlabel = "processor count"
        jgraphs.append(mfgraph.line_graph(lines,
                                          title = "%s vs %s - %d bandwidth" % (xlabel, ylabel, bandwidth),
                                          xlabel = xlabel,
                                          ylabel = ylabel,
                                          ymax = ymax,
                                          xmin = 1.9,
                                          ))

def generate_bandwidth(jgraphs, benchmark, stat, ylabel, transformation, ymax):
    ## Generate bandwidth available vs performance (one graph per processor count)
    for processor in processor_list:
        lines = []
        for module in modules_list:
            points = []
            for bandwidth in bandwidth_list:
                data = get_data(benchmark, processor=processor, module=module, bandwidth=bandwidth, stat=stat)
                if len(data) > 0:
                    value = mfgraph.average(data)
                    stddev = mfgraph.stddev(data)
                    points.append([bandwidth, value, value+stddev, value-stddev])
            lines.append([module] + points)

        transformation(lines)
        xlabel = "endpoint bandwidth available (MB/second)"
        jgraphs.append(mfgraph.line_graph(lines,
                                          title = "%s: %d processors" % (benchmark, processor),
                                          ymax = ymax,
                                          xlabel = xlabel,
                                          ylabel = ylabel,
                                          xlog = 10,
                                          xmin = 90.0
                                          ))

def generate_threshold(jgraphs, benchmark, stat, ylabel, transformation, ymax):
    ## Generate threshold variation
    lines = []
    for module in ["MOSI_bcast_opt", "MOSI_GS"]:
        points = []
        for bandwidth in bandwidth_list:
            value = mfgraph.average(get_data(benchmark, module=module, bandwidth=bandwidth, stat=stat))
            if (value != []):
                points.append([bandwidth, value])
                
        lines.append([module] + points)

    for threshold in threshold_list:
        points = []
        for bandwidth in bandwidth_list:
            value = mfgraph.average(get_data(benchmark, module="MOSI_mcast_aggr", bandwidth=bandwidth, threshold=threshold, stat=stat))
            if (value != []):
                points.append([bandwidth, value])

        if threshold == default_threshold:
            lines.append(["MOSI_mcast_aggr"] + points)
        else:
            lines.append(["%f" % threshold] + points)

    transformation(lines)
    xlabel = "endpoint bandwidth available (MB/second)"
    jgraphs.append(mfgraph.line_graph(lines,
                                      title = "multiple thresholds: %s vs %s  - %d processors" % (xlabel, ylabel, default_processor),
                                      ymax = ymax,
                                      xlabel = xlabel,
                                      ylabel = ylabel,
                                      xlog = 10,
                                      xmin = 90.0
                                      ))

def generate_thinktime(jgraphs, benchmark, stat, ylabel, transformation, ymax):
    for bandwidth in bandwidth_list:

        lines = []
        for module in modules_list:
            points = []
            for think_time in think_time_list:
                value = mfgraph.average(get_data(benchmark, module=module, bandwidth=bandwidth, think_time=think_time, stat=stat))
                if (value != []):
                    points.append([think_time, value])
            lines.append([module] + points)

        transformation(lines)
        xlabel = "think time"
        jgraphs.append(mfgraph.line_graph(lines,
                                          title = "%s vs %s - %d bandwidth" % (xlabel, ylabel, bandwidth),
                                          xlabel = xlabel,
                                          ylabel = ylabel,
                                          ymax = ymax,
                                          ))


def generate_macro(scale, benchmarks, stat, ylabel, transformation, ymax):
    cols = 3
    row_space = 2.2
    col_space = 2.3
    jgraph_input = ""
    num = 0
    ## Generate bandwidth available vs performance (one graph per processor count)
    for benchmark in benchmarks:
        processor = 16
        lines = []
        if scale == 4:
            modules = "MOSI_bcast_opt_4", "MOSI_mcast_aggr_4", "MOSI_GS_4", 

        if scale == 1:
            modules = "MOSI_bcast_opt_1", "MOSI_mcast_aggr_1", "MOSI_GS_1", 

        for module in modules:
            points = []
            for bandwidth in bandwidth_list:
                if bandwidth < 600:
                    continue
                if bandwidth > 12800:
                    continue
                data = get_data(benchmark, processor=processor, module=module, bandwidth=bandwidth, stat=stat)
                if len(data) > 0:
                    value = mfgraph.average(data)
                    stddev = mfgraph.stddev(data)
                    if (stddev/value)*100.0 > 1.0 and benchmark != "microbenchmark": # only plot error bars if they are more than 1%
                        points.append([bandwidth, value, value+stddev, value-stddev])
                    else:
                        points.append([bandwidth, value])
                        
            lines.append([protocol_name[module]] + points)
            
        transformation(lines)

        # don't plot marks for the microbenchmark
        benchmark_marktype = ["circle"]
        if benchmark == "microbenchmark":
            benchmark_marktype = ["none"]
            
        xlabel = "endpoint bandwidth available (MB/second)"
        jgraph_input += mfgraph.line_graph(lines,
                                           #title = "%s: %dx%d processors" % (workload_name[benchmark], scale, processor),
                                           title = "%s" % (workload_name[benchmark]),
                                           title_fontsize = "10",
                                           title_font = "Times-Roman",
                                           ymax = ymax,
                                           xlabel = xlabel,
                                           ylabel = ylabel,
                                           label_fontsize = "9",
                                           xsize = 1.8,
                                           ysize = 1.4,
                                           xlog = 10,
                                           xmin = 450.0,
                                           xmax = 12800.0,
                                           legend_x = "2500",
                                           legend_y = ".18",
                                           legend_fontsize = "8",
                                           marktype = benchmark_marktype,
                                           marksize = .03,
                                           x_translate = (num % cols) * col_space,
                                           y_translate = (num / cols) * -row_space,
                                           ylabel_location = 18.0,
                                           )
        
        if stat == "links_utilized_percent":
            jgraph_input += "newcurve clip pts 0.1 75 11000 75 linetype solid linethickness 1 marktype none gray .75\n"
            jgraph_input += "newstring x 10000 y 76 fontsize 8 : 75%\n"
        
        num += 1
        
    mfgraph.run_jgraph(jgraph_input, "bash-macrobenchmarks-%d-%s" % (scale, string.split(ylabel)[0]))

def generate_micro1(stat, ylabel, transformation, ymax, legend_x, legend_y):
    jgraph_input = ""
    ## Generate bandwidth available vs performance (one graph per processor count)
    benchmark = "microbenchmark"
    processor = 64
    lines = []
    for module in modules_list:
        points = []
        for bandwidth in bandwidth_list:
            if bandwidth > 30000:
                continue
            data = get_data(benchmark, processor=processor, module=module, bandwidth=bandwidth, stat=stat)
            if len(data) > 0:
                value = mfgraph.average(data)
                points.append([bandwidth, value])
        lines.append([protocol_name[module]] + points)

    transformation(lines)
    xlabel = "endpoint bandwidth available (MB/second)"
    jgraph_input += mfgraph.line_graph(lines,
                                       ymax = ymax,
                                       xlabel = xlabel,
                                       ylabel = ylabel,
                                       label_fontsize = "9",
                                       xsize = 1.8,
                                       ysize = 1.8,
                                       xlog = 10,
                                       xmin = 90.0,
                                       xmax = 30000.0,
                                       legend_x = legend_x,
                                       legend_y = legend_y,
                                       legend_fontsize = "8",
                                       ylabel_location = 18.0,
                                      )
        
    if stat == "links_utilized_percent":
        jgraph_input += "newcurve clip pts 0.1 75 200000 75 linetype solid linethickness 1 marktype none gray .75\n"
        jgraph_input += "newstring x 20000 y 76 fontsize 8 : 75%\n"
    
    mfgraph.run_jgraph(jgraph_input, "bash-microbench-basic-%d-%s" % (processor, string.split(ylabel)[0]))

def generate_micro2(stat, ylabel, transformation, ymax, legend_x, legend_y):

    jgraph_input = ""
    ## Generate threshold variation
    benchmark = "microbenchmark"
    processor = 64

    lines = []
    for module in ["MOSI_bcast_opt_1"]:
        points = []
        for bandwidth in bandwidth_list:
            if bandwidth > 30000:
                continue
            value = mfgraph.average(get_data(benchmark, module=module, bandwidth=bandwidth, stat=stat))
            if (value != 0):
                points.append([bandwidth, value])
                
        lines.append([protocol_name[module]] + points)

    for threshold in [0.55, 0.75, 0.95]:
        points = []
        for bandwidth in bandwidth_list:
            if bandwidth > 30000:
                continue
            value = mfgraph.average(get_data(benchmark, module="MOSI_mcast_aggr_1", bandwidth=bandwidth, threshold=threshold, stat=stat))
            if (value != 0):
                points.append([bandwidth, value])

        lines.append([protocol_name["MOSI_mcast_aggr_1"] + ": %2.0f%%" % (threshold*100)] + points)

    for module in ["MOSI_GS_1"]:
        points = []
        for bandwidth in bandwidth_list:
            if bandwidth > 30000:
                continue
            value = mfgraph.average(get_data(benchmark, module=module, bandwidth=bandwidth, stat=stat))
            if (value != 0):
                points.append([bandwidth, value])
                
        lines.append([protocol_name[module]] + points)

    global norm_module
    old_norm_module = norm_module
    norm_module = protocol_name["MOSI_mcast_aggr_1"] + ": %2.0f%%" % (default_threshold*100)
    transformation(lines)
    norm_module = old_norm_module

    xlabel = "endpoint bandwidth available (MB/second)"
    jgraph_input += mfgraph.line_graph(lines,
                                       title_fontsize = "12",
                                       title_font = "Times-Roman",
                                       ymax = ymax,
                                       xlabel = xlabel,
                                       ylabel = ylabel,
                                       label_fontsize = "9",
                                       xsize = 1.8,
                                       ysize = 1.8,
                                       xlog = 10,
                                       xmin = 90.0,
                                       xmax = 30000.0,
                                       legend_x = legend_x,
                                       legend_y = legend_y,
                                       legend_fontsize = "8",
                                       ylabel_location = 18.0,
                                       )

    mfgraph.run_jgraph(jgraph_input, "bash-microbench-threshold-%d-%s" % (processor, string.split(ylabel)[0]))


def generate_micro3(stat, ylabel, transformation, ymax, legend_x, legend_y):
    benchmark = "microbenchmark"
    ## Generate bandwidth available vs performance (one graph per processor count)
    for bandwidth in bandwidth_list:
        jgraph_input = ""
        lines = []
        for module in modules_list:
            points = []
            for processor in processor_list:
                data = get_data(benchmark, processor=processor, module=module, bandwidth=bandwidth, stat=stat)
                if len(data) > 0:
                    value = mfgraph.average(data)
                    points.append([processor, value])
                        
            lines.append([protocol_name[module]] + points)
            
        transformation(lines)
        xlabel = "processors"
        if ylabel == "performance":
            ylabel = "performance per processor"
        jgraph_input += mfgraph.line_graph(lines,
                                           title_fontsize = "12",
                                           title_font = "Times-Roman",
                                           ymax = ymax,
                                           xlabel = xlabel,
                                           ylabel = ylabel,
                                           label_fontsize = "9",
                                           xsize = 1.8,
                                           ysize = 1.8,
                                           xlog = 10,
                                           xmin = 3.0,
                                           legend_x = 4,
                                           legend_y = legend_y,
                                           legend_fontsize = "8",
                                           marktype = ["circle"],
                                           marksize = .03,
                                           hash_marks = map(str, processor_list),
                                           )
        
        if stat == "links_utilized_percent":
            jgraph_input += "newcurve clip pts 0.1 75 512 75 linetype solid linethickness 1 marktype none gray .75\n"
            jgraph_input += "newstring x 512 y 76 fontsize 8 : 75%\n"
        
        mfgraph.run_jgraph(jgraph_input, "bash-microbenchmark-processors-%d-%s" % (bandwidth, string.split(ylabel)[0]))

def generate_micro4(stat, ylabel, transformation, ymax, legend_x, legend_y):
    benchmark = "microbenchmark"
    processor = 64
    ## Generate bandwidth available vs performance (one graph per processor count)
    for bandwidth in bandwidth_list:
        jgraph_input = ""
        lines = []
        for module in modules_list:
            points = []
            for think_time in think_time_list:
                if think_time > 1000:
                    continue
                data = get_data(benchmark, think_time=think_time, processor=processor, module=module, bandwidth=bandwidth, stat=stat)
                if len(data) > 0:
                    value = mfgraph.average(data)
                    if (stat == "Ruby_cycles"):
                        points.append([think_time, ((value/10000.0)-think_time)/2.0])
                    else:
                        points.append([think_time, value])
                        
            lines.append([protocol_name[module]] + points)
            
        transformation(lines)
        xlabel = "think time (cycles)"
        jgraph_input += mfgraph.line_graph(lines,
                                           title_fontsize = "12",
                                           title_font = "Times-Roman",
                                           ymax = ymax,
                                           xlabel = xlabel,
                                           ylabel = ylabel,
                                           label_fontsize = "9",
                                           xsize = 1.8,
                                           ysize = 1.8,
                                           xmin = 0.0,
                                           legend_x = legend_x,
                                           legend_y = legend_y,
                                           legend_fontsize = "8",
#                                           marktype = ["circle"],
#                                           marksize = .03,
                                           )
        
        if stat == "links_utilized_percent":
            jgraph_input += "newcurve clip pts 0.1 75 1000 75 linetype solid linethickness 1 marktype none gray .75\n"
            jgraph_input += "newstring x 950 y 76 fontsize 8 : 75%\n"
        
        mfgraph.run_jgraph(jgraph_input, "bash-microbenchmark-thinktime-%d-%s" % (bandwidth, string.split(ylabel)[0]))








def generate_macro_bar():
    processor = 16
    bandwidth = 1600
    stat = "Ruby_cycles"

    stacks = []
    for benchmark in benchmarks[1:]:
        bars = []
        modules = "MOSI_mcast_aggr_4", "MOSI_bcast_opt_4", "MOSI_GS_4",
        norm = mfgraph.average(get_data(benchmark, processor=processor, module="MOSI_mcast_aggr_4", bandwidth=bandwidth, stat=stat))
        for module in modules:

            data = get_data(benchmark, processor=processor, module=module, bandwidth=bandwidth, stat=stat)
            value = mfgraph.average(data)
            stddev = mfgraph.stddev(data)
            if (stddev/value)*100.0 > 1.0: # only plot error bars if they are more than 1%
                bars.append([protocol_name[module], [norm/value, norm/(value+stddev), norm/(value-stddev)]])
            else:
                bars.append([protocol_name[module], [norm/value, norm/(value+stddev), norm/(value-stddev)]])
#                bars.append([protocol_name[module], norm/value])
        stacks.append([workload_name[benchmark]] + bars)
    jgraph_input = mfgraph.stacked_bar_graph(stacks,
                                             colors = [".85 .85 .85", ".5 .5 .5", ".45 .45 .45"],
                                             patterns = ["solid", "stripe -45"],
                                             ymax = 1.1,
                                             xsize = 2.7,
                                             ysize = 2.3,
                                             label_fontsize = "9",
                                             hash_label_fontsize = "9",
                                             stack_name_font_size = "9",
                                             bar_name_font_size = "9",
                                             bar_name_rotate = 90.0,
                                             stack_name_location = 28.0,
                                             ylabel = "normalized performance",
                                             yhash = 0.2,
                                             ymhash = 1,
                                             )
    mfgraph.run_jgraph(jgraph_input, "bash-macrobenchmark-bars")








read_data()

#jgraph_input = []
#for stat in statistics:
#    for bench in benchmarks:
#        jgraph_input.append("newgraph xaxis min 0 max 1 nodraw yaxis min 0 max 1 nodraw title : %s\n" % stat[1])
        #    generate_threshold(jgraph_input, *stat)
#        generate_bandwidth(jgraph_input, bench, *stat)
        #    generate_processors(jgraph_input, *stat)
        #    generate_thinktime(jgraph_input, *stat)

#mfgraph.run_jgraph("newpage\n".join(jgraph_input), "bash")

for stat in statistics:
    generate_macro(1, benchmarks, *stat[:-2])
    generate_macro(4, benchmarks, *stat[:-2])
    generate_micro1(*stat);
    generate_micro2(*stat)
    generate_micro3(*stat)

generate_micro4("Ruby_cycles", "average miss latency (cycles)", lambda x : x, 400.0, "250", "60")
generate_micro4("links_utilized_percent", "endpoint link utilization (percent)", lambda x : x, 100.0, "150", "50")
generate_macro_bar()
    

    
