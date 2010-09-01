#!/s/std/bin/python
from __future__ import nested_scopes
import sys, string, os, glob, re, mfgraph

trace_benchmarks = [
    "apache2_5000",
    "barnes_64k",
    "ocean_514",
    "oltp_1000",
    "slash_100",
    "jbb_100000",
#    "ocean_66",
#    "barnes_512",
    ]

ruby_benchmarks = [
    "apache2_1000",
    "barnes_64k",
    "ocean_514",
    "oltp_500",
    "slash_50",
    "jbb_10000",
    ]

opal_benchmarks = [
    "apache2_100",
    "jbb_1000",
    "oltp_50",
    "slash_5",
    ]

benchmarks = trace_benchmarks

benchmark_names = {
    "barnes_64k"   :  "Barnes-Hut",
    "barnes_512"   :  "Barnes-Hut (512)",
    "jbb_100000"   :  "SPECjbb", 
    "jbb_10000"    :  "SPECjbb", 
    "jbb_1000"     :  "SPECjbb", 
    "slash_100"    :  "Slashcode", 
    "slash_50"     :  "Slashcode", 
    "slash_5"      :  "Slashcode", 
    "apache2_5000" :   "Apache",
    "apache2_1000" :   "Apache",
    "apache2_100"  :   "Apache",
    "oltp_1000"    :  "OLTP",
    "oltp_500"     :  "OLTP",
    "oltp_50"      :  "OLTP",
    "ocean_514"    :  "Ocean",
    "ocean_66"     :  "Ocean (66)",
    }

type_map = {
    "Directory" : "Directory",
    "Owner" : "Owner",
    "OwnerGroup" : "Owner/Group",
    "OwnerGroupRetry" : "Owner/Group(Retry)",
    "OwnerBroadcast" : "Owner/Broadcast",
    "OwnerGroupMod" : "Owner/Group - Mod",
    "OwnerBroadcastMod" : "Owner/Broadcast - Mod",
    "Counter" : "Group",
    "CounterRetry" : "Group(Retry)",
    "BroadcastCounter" : "Broadcast-if-shared",
    "Pairwise" : "Pairwise",
    "AlwaysBroadcast" : "Broadcast Snooping",
    "AlwaysUnicast" : "Minimal Mask",
    "StickySpatial" : "StickySpatial",
    }

def predictor_name_transform(predictor):
    predictor_parts = string.split(predictor, ":")
        
    # map name
    type_name = predictor_parts.pop(0)
    type_desc = type_map[type_name]

    if len(predictor_parts) == 0:
        return type_desc

    information = predictor_parts.pop(0)
    type_parameter = ""
    if type_name in ["Counter", "CounterRetry", "StickySpatial"]:
        type_parameter = ("(%s)" % predictor_parts.pop(0))
    indexing = predictor_parts.pop(0)
    coarseness = int(predictor_parts.pop(0))
    sets = int(predictor_parts.pop(0))

    entries = "unbounded"
    if sets != 0:
        ways = int(predictor_parts.pop(0))
        entries = "%d" % (sets * ways)
    return "%s%s - %s - %s - blocksize: %d - entries: %s" % (type_desc, type_parameter, information, indexing, pow(2, coarseness), entries)


def get_maskpred_data(benchmark, predictor):
    lst = data_map[benchmark].get(predictor, []);
    total_bw = 0.0
    total_indirections = 0.0
    total_cycles = 0.0
    for (bw, indirections, cycles) in lst:
        total_bw += bw
        total_indirections += indirections
        total_cycles += cycles
    if len(lst) == 0:
        return (-1, -1, -1)
    else:
        return (total_bw / len(lst), total_indirections / len(lst), total_cycles / len(lst))

def maskpred_traces():

    ## read in all data
    global data_map
    data_map = {}
    for benchmark in benchmarks:
        print benchmark
        data_map[benchmark] = {} # init map
        
        # gs320 directory protocol
        filenames = glob.glob("*/%s-*gs320*.stats" % benchmark)
        for filename in filenames:
            control_line = mfgraph.grep(filename, "  switch_16_messages_out_Control")[0]
            control_line = string.replace(control_line, "[", " ");
            control_line = string.split(control_line)

            data_line = mfgraph.grep(filename, "  switch_16_messages_out_Data")[0]
            data_line = string.replace(data_line, "[", " ");
            data_line = string.split(data_line)

            # to calculate the request bandwidth, we add up all the
            # outgoing control messages and subtract the number of PutXs
            # (which conveniently happens to be the number of virtual
            # network zero data messages sent.  We need to subtract out
            # the number of PutXs, since the GS320 protocol sends a
            # forwarded control message to 'ack' a PutX from a processor.
            control_msgs = float(control_line[1]) - float(data_line[2])

            sharing_misses = get_data(filename, "sharing_misses:")
            total_misses = get_data(filename, "Total_misses:")
            control_msgs_per_miss = control_msgs/total_misses
            cycles = get_data(filename, "Ruby_cycles:")

            if not data_map[benchmark].has_key("Directory"):
                data_map[benchmark]["Directory"] = []
            indirections = 100.0 * (sharing_misses/total_misses)
            data_map[benchmark]["Directory"].append([control_msgs_per_miss, indirections, cycles])
    
        # mask prediction data
        filenames = glob.glob("*/%s-*mcast*.stats" % benchmark)
        filenames.sort()
        for filename in filenames:
            predictor = string.split(filename, "-")[3]
            
            # calculate indirections
            data_lines = mfgraph.grep(filename, "multicast_retries:")

            if not data_lines:
                continue # missing data

            lst = string.split(string.replace(data_lines[0], "]", ""))
            total_misses = float(lst[6])
            total_misses_alt = get_data(filename, "Total_misses:")
            non_retry = float(lst[13])
            retry = total_misses - non_retry
            indirections = 100.0 * (retry/total_misses)
            cycles = get_data(filename, "Ruby_cycles:")

            # calculate bandwidth
            data_lines = mfgraph.grep(filename, "  switch_16_messages_out_Control:")

            if not data_lines:
                continue # missing data

            lst = string.split(string.replace(data_lines[0], "]", ""))
            control_msgs = float(lst[1])
            control_msgs_per_miss = control_msgs/total_misses

            print "  ", predictor, "->", benchmark, control_msgs_per_miss, indirections, cycles
            if not data_map[benchmark].has_key(predictor):
                data_map[benchmark][predictor] = []
            data_map[benchmark][predictor].append([control_msgs_per_miss, indirections, cycles])

    ## Make the graphs
    all_data = []
    all_parameters = []
    for benchmark in benchmarks:
        print benchmark
        # collect data
        data = []

        # mask prediction data
        for predictor in data_map[benchmark].keys():
            include_list = []
            include_list.append("^Directory")
            include_list.append("^AlwaysBroadcast")
            #include_list.append("^AlwaysUnicast")
            
# Graph1
#              include_list.append("Counter:Implicit:5:DataBlock:0:0")
#              include_list.append("Owner:Implicit:DataBlock:0:0")
#              include_list.append("BroadcastCounter:Implicit:DataBlock:0:0")
#              include_list.append("OwnerGroup:Implicit:DataBlock:0:0")
#              include_list.append("OwnerBroadcast:Implicit:DataBlock:0:0")

# Graph2
#              include_list.append("Counter:Implicit:5:.*:0:0")
#              include_list.append("Owner:Implicit:.*:0:0")
#              include_list.append("BroadcastCounter:Implicit:.*:0:0")
#              include_list.append("OwnerGroup:Implicit:.*:0:0")
#              include_list.append("OwnerBroadcast:Implicit:.*:0:0")

# Graph3

#            include_list.append("Owner:Implicit:DataBlock:.*:0")
#            include_list.append("Counter:Implicit:5:DataBlock:.*:0")
#            include_list.append("OwnerGroup:Implicit:DataBlock:.*:0")
#            include_list.append("OwnerGroupMod:Implicit:DataBlock:.*:0")
#            include_list.append("OwnerBroadcast:Implicit:DataBlock:.*:0")
#            include_list.append("BroadcastCounter:Implicit:DataBlock:.*:0")

# Graph4
#            include_list.append("Owner:Implicit:DataBlock:4:.*")
#            include_list.append("Counter:Implicit:5:DataBlock:4:.*")
#            include_list.append("BroadcastCounter:Implicit:DataBlock:4:.*")
#            include_list.append("OwnerGroup:Implicit:DataBlock:4:.*")
#            include_list.append("OwnerGroupMod:Implicit:DataBlock:4:.*")
#            include_list.append("OwnerBroadcast:Implicit:DataBlock:4:.*")

            include_list.append("^StickySpatial:Both:1:DataBlock:0:.*")
#            include_list.append("^OwnerGroup:.*:DataBlock:0:0")

#            include_list.append("^Owner:Implicit:DataBlock:4:.*")
#            include_list.append("^Counter:.*:5:DataBlock:0:0")
#            include_list.append("^OwnerGroup:.*:DataBlock:0:0")
#            include_list.append("^BroadcastCounter:Implicit:DataBlock:4:.*")
#            include_list.append("^OwnerGroup:Implicit:DataBlock:.*:0")
#            include_list.append("^OwnerGroupMod:Implicit:DataBlock:0:0")
#            include_list.append("^OwnerBroadcast:Implicit:DataBlock:0:0")
#            include_list.append("^OwnerBroadcastMod:Implicit:DataBlock:0:0")

#            include_list.append("^BroadcastCounter:Implicit:1:DataBlock:4:.*")
#            include_list.append("^OwnerGroup:Implicit:DataBlock:[06]:0")
#            include_list.append("^BroadcastCounter:Implicit:1:DataBlock:0:0")
#            include_list.append("^Counter:Implicit:1:DataBlock:[06]:0")
#            include_list.append("^Pairwise:Implicit:DataBlock:4:.*")
#            include_list.append("^StickySpatial:Implicit:2:DataBlock:0:0")

#            include_list.append("^Counter:Implicit:1:DataBlock:.*:0")
#            include_list.append("^BroadcastCounter:Implicit:1:DataBlock:.*:0")
#            include_list.append("^Pairwise:Implicit:DataBlock:.*:0")

#            include_list.append("^Counter:Implicit:1:PC:.*:0")
#            include_list.append("^BroadcastCounter:Implicit:1:PC:.*:0")
#            include_list.append("^Pairwise:Implicit:PC:.*:0")

#            include_list.append("^StickySpatial:.*:0:DataBlock:8")
            
            include = 0 # false
            for pat in include_list:
                if re.compile(pat).search(predictor):
                    include = 1 # true
            if not include:
#                print "  ", predictor, "-> skipped"
                continue
            
            predictor_desc = predictor_name_transform(predictor)

            (control_msgs_per_miss, indirections, cycles) = get_maskpred_data(benchmark, predictor)
            (dir_control_msgs_per_miss, dir_indirections, dir_cycles) = get_maskpred_data(benchmark, "Directory")
#            indirections = 100*indirections/dir_indirections
            
            print "  ", predictor, "->", benchmark, predictor_desc, control_msgs_per_miss, indirections
            data.append([predictor_desc, [control_msgs_per_miss, indirections]])

        # graph the data
        all_data.append(data)
        all_parameters.append({ "title" : benchmark_names[benchmark] })

    # only display the legend on the last graph
    all_parameters[-1]["legend"] = "on"

    output = mfgraph.multi_graph(all_data,
                                 all_parameters,
                                 legend = "off",
                                 xsize = 1.8,
                                 ysize = 1.8,
                                 xlabel = "control messages per miss",
                                 ylabel = "indirections (percent of all misses)",
#                                 linetype = ["dotted"] + (["none"] * 10),
                                 linetype = ["none"] * 10,
                                 colors = ["1 0 0", "0 0 1", "0 .5 0", "0 0 0", "1 0 1"],
                                 fills = ["1 0 0", "0 0 1", "0 .5 0", "0 .5 1", "1 0 1"],
                                 xmin = 0.0,
                                 ymin = 0.0,
                                 cols = 3,
                                 #                    ymax = 100.0,
                                 marktype = (["circle", "box", "diamond", "triangle"] * 10),
#                                 marktype = ["none"] + (["circle", "box", "diamond", "triangle"] * 10),
                                 title_fontsize = "12",
                                 legend_hack = "yes",
                                 )

    mfgraph.run_jgraph(output, "traces")

########################################### section 2 data #####################################

def normalize_list(lst):
    sum = reduce(lambda x,y:x+y, lst)
    for index in range(len(lst)):
        lst[index] = lst[index]/sum

def touched_by():
    stats = [
        "touched_by_block_address:",
        "touched_by_weighted_block_address:",
#        "touched_by_macroblock_address:",
#        "touched_by_weighted_macroblock_address:",
#        "touched_by_supermacroblock_address:",
#        "touched_by_weighted_supermacroblock_address:",
#        "last_n_block_touched_by",
#        "last_n_macroblock_touched_by",
        ]
    
    yaxis_names = [
        "Percent of all blocks",
        "Percent of all misses",
        "Percent of all macroblocks",
        "Percent of all misses",
        "Percent of all macroblocks",
        "Percent of all misses",
#        "Percent",
#        "Percent",
        ]

    stats_names = {
        "touched_by_block_address:" : "(a) Percent of data blocks (64B) touched by n processors",
        "touched_by_weighted_block_address:": "(b) Percent of misses to data blocks (64B) touched by n processors",
#        "touched_by_macroblock_address:": "(c) Percent of data macroblocks (1024B) touched by n processors",
#        "touched_by_weighted_macroblock_address:": "(d) Percent of misses to data macroblocks (1024B) touched by n processors",
#        "touched_by_supermacroblock_address:": "(e) Percent of 4kB macroblocks touched by n processors",
#        "touched_by_weighted_supermacroblock_address:": "(f) Percent of misses to 4kB macroblocks touched by n processors",
#        "last_n_block_touched_by" : "(e) Percent of misses touched by n processors in the last 64 misses to the block",
#        "last_n_macroblock_touched_by" : "(f) Percent of misses touched by n processors in the last 64 misses to the macroblock",
        }

    jgraph_input = []

    cols = 1
    row_space = 2.2
    col_space = 3

    num = 0
    for stat in stats:
        bars = []
        for benchmark in benchmarks:
            print stat, benchmark
            group = [benchmark_names[benchmark]]
            filenames = glob.glob("*/%s-*gs320*.stats" % benchmark)
            print benchmark, filenames
            for filename in filenames:
                line = mfgraph.grep(filename, stat)[0]
                line = string.replace(line, "]", "")
                line = string.replace(line, "[", "")
                data = string.split(line)[2:]
#                data = string.split(line)[14:]
                data = map(float, data)
                print data

#                  new_data = []
#                  sum = 0.0
#                  for item in data:
#                      new_data.append(item + sum)
#                      sum += item
#                  data = new_data
                print data
                
#                  for index in range(len(data)):
#                      data[index] = data[index]/sum

                print data
                normalize_list(data)
                for index in range(len(data)):
                    if index+1 in [1, 4, 8, 12, 16]:
                        group.append(["%d" % (index+1), data[index]*100.0])
                    else:
                        group.append(["", data[index]*100.0])

            bars.append(group)

        jgraph_input.append(mfgraph.stacked_bar_graph(bars,
                                                      title = stats_names[stat],
                                                      title_fontsize = "12",
                                                      title_font = "Times-Roman",
                                                      title_y = -25.0,
                                                      xsize = 6.5,
                                                      ysize = 1.5,
                                                      xlabel = "",
                                                      ymax = 100.01,
                                                      ylabel = yaxis_names[num],
                                                      colors = [".5 .5 .5"],
                                                      patterns = ["solid"],
                                                      stack_name_location = 12.0,
                                                      stack_space = 3,
                                                      x_translate = (num % cols) * col_space,
                                                      y_translate = (num / cols) * -row_space,
                                                      ))
        num += 1

    mfgraph.run_jgraph("\n".join(jgraph_input), "touched-by")

def gen_sharing_histo():
    bars = []
    for benchmark in benchmarks:
        filenames = glob.glob("*/%s-gs320*.stats" % benchmark)
        if len(filenames) != 1:
            continue
        filename = filenames[0]

        gets_dist = get_histo(filename, "gets_sharing_histogram:")
        getx_dist = get_histo(filename, "getx_sharing_histogram:")
        total_misses = get_data(filename, "Total_misses:")
        gets_dist = map(lambda x : 100.0 * (float(x) / total_misses), gets_dist)
        gets_dist += [0] * 14  # fill in the end with zeros
        getx_dist = map(lambda x : 100.0 * (float(x) / total_misses), getx_dist)

        getx_dist = getx_dist[0:3] + [reduce(lambda x,y:x+y, getx_dist[3:7])] + [reduce(lambda x,y:x+y, getx_dist[7:])]
        gets_dist = gets_dist[0:3] + [reduce(lambda x,y:x+y, gets_dist[3:7])] + [reduce(lambda x,y:x+y, gets_dist[7:])]

#        getx_dist = mfgraph.stack_bars(getx_dist)
#        gets_dist = mfgraph.stack_bars(gets_dist)
        
        labels = ["0", "1", "2", "3-7", "8+"]
        bars.append([benchmark_names[benchmark]] + map(lambda l, gets, getx : (l, gets+getx, getx), labels, gets_dist, getx_dist))
    jgraph_input = mfgraph.stacked_bar_graph(bars,
                                             bar_segment_labels = ["Get shared", "Get exclusive"],
                                             xsize = 7,
                                             ysize = 3,
                                             ylabel = "Percent of all misses",
                                             colors = ["1 0 0", "0 0 1"],
                                             patterns = ["solid", "stripe -45"],
                                             bar_name_font_size = "10",
                                             stack_name_location = 10.0,
                                             legend_x = "31",
                                             legend_y = "87",
                                             )
    mfgraph.run_jgraph(jgraph_input, "sharers")

def get_data(filename, str):
    data = mfgraph.grep(filename, str)
    return float(string.split(data[0])[1])

def get_histo(filename, str):
    line = mfgraph.grep(filename, str)[0]
    line = string.replace(line, "]", " ")
    line = string.split(line)
    return line[13:]

def gen_table():
    print "%s, %s, %s, %s, %s, %s, %s, %s, %s" % ("Workload", "Touched (64B)", "Touched (1024B)", "Touched (4KB)", "Unique miss PCs", "L2 per thousand inst", "Supervisor", "Indirections", "Total misss (in millions)")
    for benchmark in benchmarks:
        filenames = glob.glob("*/%s-gs320*.stats" % benchmark)
        if len(filenames) != 1:
            continue
        filename = filenames[0]

        # calculate bytes touched (64B)
        block_bytes = get_data(filename, "Total_entries_block_address:") * 64 # 64B blocks
        block_mbytes = block_bytes / (1024*1024)

        # calculate bytes touched (1024B)
        macro_block_bytes = get_data(filename, "Total_entries_macroblock_address:") * 1024 # 1024B blocks
        macro_block_mbytes = macro_block_bytes / (1024*1024)

        # calculate bytes touched (1024B)
        supermacro_block_bytes = get_data(filename, "Total_entries_supermacroblock_address:") * 4 * 1024 # 4096B blocks
        supermacro_block_mbytes = supermacro_block_bytes / (1024*1024)

        # unique miss PCs
        num_pc_entries = get_data(filename, "Total_entries_pc_address:")

        # indirections
        total_misses = get_data(filename, "Total_misses:")
        sharing_misses = get_data(filename, "sharing_misses:")
        indirections = (sharing_misses/total_misses) * 100.0

        filenames = glob.glob("../traces/*/%s-16p-MOSI_bcast*.stats" % benchmark)
        if len(filenames) != 1:
            continue
        filename = filenames[0]

        # misses per 1000 instructions
        misses_per_thousand = get_data(filename, "misses_per_thousand_instructions:")

        # supervisor misses
        supervisor_misses = get_data(filename, "supervisor_misses:")
        supervisor_misses_percent = (supervisor_misses/total_misses) * 100.0

        print "%15s %5d MB  %5d MB  %5d MB %7d  %4.1f  %4.f%% %4.0f%% %20.0f" % \
              (benchmark_names[benchmark], int(block_mbytes), int(macro_block_mbytes), int(supermacro_block_mbytes), int(num_pc_entries), misses_per_thousand, supervisor_misses_percent, indirections, total_misses/1000000.0)


################# figure 4 ########################
        
def make_cumulative(lst, total):
    sum = 0;
    for i in range(len(lst)):
        sum = sum + lst[i]
        lst[i] = float(sum)/float(total)

def get_cumulative(filename, column, total, stat):
    # get all_misses histrogram
    command = 'egrep "^%s" %s' % (stat, filename)
    results = os.popen(command, "r")
    lines = results.readlines()
    lst = []
    for line in lines:
        line = string.split(line)
        lst.append(line[column])
    lst = map(float, lst)
    lst.sort()
    lst.reverse()
    make_cumulative(lst, (total/100.0))
    return lst

def cumulative():
    stats = [
        "^block_address ",
        "^macroblock_address ",
#        "^supermacroblock_address ",
        "^pc_address ",
        ]
    
    stat_name = {
        "^block_address " : "Number of data blocks (64B)",
        "^macroblock_address " : "Number of data macroblocks (1024B)",
        "^supermacroblock_address " : "Number of data macroblocks (4096B)",
        "^pc_address " : "Number of static instructions",
        }

    jgraph_input = []

    cols = 3
    row_space = 2.9
    col_space = 3

    num = 0
    for stat in stats:
        graph_lines = []
        for benchmark in benchmarks:
            print stat, benchmark
            group = [benchmark_names[benchmark]]
            filenames = glob.glob("*/%s-gs320*.stats" % benchmark)
            for filename in filenames:
                print filename

                command = 'egrep "^Total_data_misses_block_address" %s' % (filename)
                results = os.popen(command, "r")
                line = results.readlines()[0]
                total_misses = float(string.split(line)[1])
                
                command = 'egrep "^sharing_misses:" %s' % (filename)
                results = os.popen(command, "r")
                line = results.readlines()[0]
                total_sharing_misses = float(string.split(line)[1])

                sharing_misses = get_cumulative(filename, 16, total_sharing_misses, stat)
                line = [benchmark_names[benchmark]]
#                points = range(0, 100) + range(100, len(sharing_misses), 10)
                points = range(1, len(sharing_misses), 100)
                for i in points:
                    line.append([i+1, sharing_misses[i]])
                graph_lines.append(line)

        jgraph_input.append(mfgraph.line_graph(graph_lines,
                                               ymax = 100.0,
#                                               xlog = 10,
                                               xmax = 10000,
                                               title = "",
                                               title_fontsize = "12",
                                               title_font = "Times-Roman",
                                               xsize = 2.0,
                                               ysize = 2.5,
                                               xlabel = stat_name[stat],
                                               ylabel = "Percent of all sharing misses (cumulative)",
                                               label_fontsize = "10",
                                               label_font = "Times-Roman",
                                               legend_fontsize = "10",
                                               legend_font = "Times-Roman",
                                               legend_x = "50",
                                               legend_y = "20",
                                               x_translate = (num % cols) * col_space,
                                               y_translate = (num / cols) * -row_space,
                                               line_thickness = 1.0,
                                               ))

        num += 1

    mfgraph.run_jgraph("\n".join(jgraph_input), "cumulative")

#########################################

#gen_sharing_histo() # single bar graph with GetS/GetX
touched_by() # the 4 bar graph full page image
#cumulative() # the three line graphs
#askpred_traces() # the mask prediction trace results
#gen_table() # table data
