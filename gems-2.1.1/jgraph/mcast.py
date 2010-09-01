#!/s/std/bin/python
from __future__ import nested_scopes
import sys, string, os, glob, re, mfgraph

grey = "0.7 0.7 0.7"
black = "0 0 0"
white = "1 1 1"

mark0 = [black, black]
mark1 = [black, black]
mark2 = [grey, grey]
mark3 = [black, white]

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
    ]

ruby_graphs = []
ruby_graphs.append(["ruby-results",
                    ruby_benchmarks,
                    [("None", mark0)],
                    [("AlwaysBroadcast", mark0)],
                    
                    [("Owner:Implicit:DataBlock:4:2048:4", mark1)],
                    [("BroadcastCounter:Implicit:DataBlock:4:2048:4", mark1)],
                    [("Counter:Implicit:5:DataBlock:4:2048:4", mark1)],
                    [("OwnerGroup:Implicit:DataBlock:4:2048:4", mark1)],
                    
                    ])

opal_graphs = []
opal_graphs.append(["opal-results",
                    opal_benchmarks,
                    [("None", mark0)],
                    [("AlwaysBroadcast", mark0)],
                    
                    [("Owner:Implicit:DataBlock:4:2048:4", mark1)],
                    [("BroadcastCounter:Implicit:DataBlock:4:2048:4", mark1)],
                    [("Counter:Implicit:5:DataBlock:4:2048:4", mark1)],
                    [("OwnerGroup:Implicit:DataBlock:4:2048:4", mark1)],
                    
                    ])

trace_graphs = []
for (label, benchmarks) in [("oltp", ["oltp_1000"]), ("all", trace_benchmarks)]:
    trace_graphs.append(["trace-results-0-%s" % (label),
                         benchmarks,
                         [("None", mark0)],
                         [("AlwaysBroadcast", mark0)],

                         [("Owner:Implicit:DataBlock:4:2048:4", mark1)],
                         [("BroadcastCounter:Implicit:DataBlock:4:2048:4", mark1)],
                         [("Counter:Implicit:5:DataBlock:4:2048:4", mark1)],
                         [("OwnerGroup:Implicit:DataBlock:4:2048:4", mark1)],

                         ])

    trace_graphs.append(["trace-results-1-%s" % (label),
                         benchmarks,
                         [("None", mark0)],
                         [("AlwaysBroadcast", mark0)],

                         [("Owner:Implicit:DataBlock:0:0", mark1),
                          ("Owner:Implicit:PC:0:0", mark3)],

                         [("BroadcastCounter:Implicit:DataBlock:0:0", mark1),
                          ("BroadcastCounter:Implicit:PC:0:0", mark3)],

                         [("Counter:Implicit:5:DataBlock:0:0", mark1),
                          ("Counter:Implicit:5:PC:0:0", mark3)],

                         [("OwnerGroup:Implicit:DataBlock:0:0", mark1),
                          ("OwnerGroup:Implicit:PC:0:0", mark3)],

                         ])

    trace_graphs.append(["trace-results-2-%s" % (label),
                         benchmarks,
                         [("None", mark0)],
                         [("AlwaysBroadcast", mark0)],

                         [("Owner:Implicit:DataBlock:0:0", mark1),
                          ("Owner:Implicit:DataBlock:2:0", mark2),
                          ("Owner:Implicit:DataBlock:4:0", mark3)],

                         [("BroadcastCounter:Implicit:DataBlock:0:0", mark1),
                          ("BroadcastCounter:Implicit:DataBlock:2:0", mark2),
                          ("BroadcastCounter:Implicit:DataBlock:4:0", mark3)],

                         [("Counter:Implicit:5:DataBlock:0:0", mark1),
                          ("Counter:Implicit:5:DataBlock:2:0", mark2),
                          ("Counter:Implicit:5:DataBlock:4:0", mark3)],

                         [("OwnerGroup:Implicit:DataBlock:0:0", mark1),
                          ("OwnerGroup:Implicit:DataBlock:2:0", mark2),
                          ("OwnerGroup:Implicit:DataBlock:4:0", mark3)],

                         ])


    trace_graphs.append(["trace-results-3-%s" % (label),
                         benchmarks,
                         [("None", mark0)],
                         [("AlwaysBroadcast", mark0)],

                         [("Owner:Implicit:DataBlock:4:0", mark1),
                          ("Owner:Implicit:DataBlock:4:8192:4", mark2),
                          ("Owner:Implicit:DataBlock:4:2048:4", mark3)],

                         [("BroadcastCounter:Implicit:DataBlock:4:0", mark1),
                          ("BroadcastCounter:Implicit:DataBlock:4:8192:4", mark2),
                          ("BroadcastCounter:Implicit:DataBlock:4:2048:4", mark3)],

                         [("Counter:Implicit:5:DataBlock:4:0", mark1),
                          ("Counter:Implicit:5:DataBlock:4:8192:4", mark2),
                          ("Counter:Implicit:5:DataBlock:4:2048:4", mark3)],

                         [("OwnerGroup:Implicit:DataBlock:4:0", mark1),
                          ("OwnerGroup:Implicit:DataBlock:4:8192:4", mark2),
                          ("OwnerGroup:Implicit:DataBlock:4:2048:4", mark3)],

                         [("StickySpatial:Both:1:DataBlock:0:0", mark1),
                          ("StickySpatial:Both:1:DataBlock:0:32768:1", mark2),
                          ("StickySpatial:Both:1:DataBlock:0:8192:1", mark3)],

                         ])

benchmark_names = {
    "barnes_64k"   :  "Barnes-Hut",
    "barnes_512"   :  "Barnes-Hut (512)",
    "jbb_100000"   :  "SPECjbb", 
    "jbb_10000"    :  "SPECjbb", 
    "jbb_1000"     :  "SPECjbb", 
    "jbb_10"       :  "SPECjbb", 
    "slash_100"    :  "Slashcode", 
    "slash_50"     :  "Slashcode", 
    "slash_5"      :  "Slashcode", 
    "apache2_5000" :  "Apache",
    "apache2_1000" :  "Apache",
    "apache2_100"  :  "Apache",
    "oltp_1000"    :  "OLTP",
    "oltp_500"     :  "OLTP",
    "oltp_50"      :  "OLTP",
    "ocean_514"    :  "Ocean",
    "ocean_66"     :  "Ocean (66)",
    }

type_map = {
    "None" : "Directory",
    "Owner" : "Owner",
    "OwnerGroup" : "Owner/Group",
    "OwnerBroadcast" : "Owner/Broadcast",
    "OwnerGroupMod" : "Owner/Group - Mod",
    "OwnerBroadcastMod" : "Owner/Broadcast - Mod",
    "Counter" : "Group",
    "BroadcastCounter" : "Broadcast-if-shared",
    "Pairwise" : "Pairwise",
    "AlwaysBroadcast" : "Broadcast Snooping",
    "AlwaysUnicast" : "Minimal Mask",
    "StickySpatial" : "StickySpatial",
    }

shape_map = {
    "None" : "cross",
    "AlwaysBroadcast" : "x",
    "Owner" : "circle",
    "BroadcastCounter" : "box",
    "Counter" : "triangle",
    "OwnerGroup" : "triangle",
    "StickySpatial" : "diamond",
    "OwnerBroadcast" : "diamond",
    }

rotate_map = {
    "OwnerGroup" : 180.0,
    }

def predictor_to_shape(predictor):
    predictor_parts = string.split(predictor, ":")
        
    # map name
    type_name = predictor_parts.pop(0)
    return (shape_map[type_name], rotate_map.get(type_name, 0))

def predictor_name_transform(predictor):
    predictor_parts = string.split(predictor, ":")
        
    # map name
    type_name = predictor_parts.pop(0)
    type_desc = type_map[type_name]

    if len(predictor_parts) == 0:
        return type_desc

    information = predictor_parts.pop(0)
    type_parameter = ""
    if type_name in ["Counter", "StickySpatial"]:
        type_parameter = ("(%s)" % predictor_parts.pop(0))
        if type_name == "Counter":
            type_parameter = "" # Don't display the parameter for counter
    indexing = predictor_parts.pop(0)
    coarseness = int(predictor_parts.pop(0))
    sets = int(predictor_parts.pop(0))

    entries = "unbounded"
    if sets != 0:
        ways = int(predictor_parts.pop(0))
        entries = "%d" % (sets * ways)
#    output = "%s%s - %s - %s - blocksize: %d - entries: %s" % (type_desc, type_parameter, information, indexing, pow(2, coarseness), entries)
    output = "%s%s - %s(%d, %s)" % (type_desc, type_parameter, indexing, pow(2, coarseness), entries)
    return output

def get_data(filename, str, index=1):
    data = mfgraph.grep(filename, str)
    if data:
        line = data[0]
        line = string.replace(line, "]", " ")
        line = string.replace(line, "[", " ")
        return float(string.split(line)[index])
    else:
        return None

def parse_file(filename):
    if string.count(filename, "gs320"):
        return parse_gs320_file(filename)
    else:
        return parse_mcast_file(filename)

def parse_mcast_file(filename):
    # calculate indirections
    total_misses = get_data(filename, "multicast_retries:", 6)
    non_retry = get_data(filename, "multicast_retries:", 13)
    cycles = get_data(filename, "Ruby_cycles:")
    control_msgs = get_data(filename, "  switch_16_messages_out_Control:", 1)

    all_control_msg = get_data(filename, "  switch_16_messages_out_Control")
    all_data_msg = get_data(filename, "  switch_16_messages_out_Data")

    # Check for errors
    if None in (total_misses, non_retry, cycles, control_msgs, all_control_msg, all_data_msg):
        return None

    all_bytes = ((all_control_msg * 8) + (all_data_msg * 72)) / total_misses # FIXME
    retry = total_misses - non_retry
    indirections = 100.0 * (retry/total_misses)
    control_msgs_per_miss = control_msgs/total_misses
    return (control_msgs_per_miss, indirections, cycles, all_bytes)

def parse_gs320_file(filename):
    # to calculate the request bandwidth, we add up all the
    # outgoing control messages and subtract the number of PutXs
    # (which conveniently happens to be the number of virtual
    # network zero data messages sent.  We need to subtract out
    # the number of PutXs, since the GS320 protocol sends a
    # forwarded control message to 'ack' a PutX from a processor.
    control_msgs = get_data(filename, "  switch_16_messages_out_Control", 1) - get_data(filename, "  switch_16_messages_out_Data", 2)
    sharing_misses = get_data(filename, "sharing_misses:")
    total_misses = get_data(filename, "Total_misses:")
    cycles = get_data(filename, "Ruby_cycles:")
    all_control_msg = get_data(filename, "  switch_16_messages_out_Control")
    all_data_msg = get_data(filename, "  switch_16_messages_out_Data")

    # Check for errors
    if sharing_misses == None:
        sharing_misses = 0
        
    if None in (control_msgs, sharing_misses, total_misses, all_control_msg, all_data_msg):
        return None
    
    all_bytes = ((all_control_msg * 8) + (all_data_msg * 72)) / total_misses # FIXME
    control_msgs_per_miss = control_msgs/total_misses
    indirections = 100.0 * (sharing_misses/total_misses)
    return (control_msgs_per_miss, indirections, cycles, all_bytes)

maskpred_data_cache = {}
def get_maskpred_data(benchmark, predictor):

    # See if we have the result in the cache
    global maskpred_data_cache
    key = "%s:%s" % (benchmark, predictor)
    if not maskpred_data_cache.has_key(key):
        # Not in the cache, read the files
        filenames = glob.glob("%s/%s/%s-16p-*-%s-*.stats" % (root, benchmark, benchmark, predictor))
        data = filter(None, map(parse_file, filenames))
        if data:
            inverted_data = map(None, *data)
            output = map(mfgraph.average, inverted_data)
        else:
            output = None

        # insert into the cache
        maskpred_data_cache[key] = output;

    return maskpred_data_cache[key]

def make_graph(graph):
    name = graph[0]
    print name
    name = "graphs/" + name
    benchmarks = graph[1]
    data_points = graph[2:]

    ## calculate the line configurations
    marktype_lst = []
    mrotate_lst = []
    color_lst = []
    fill_lst = []
    linetype_lst = []

    for series in data_points:
        ## the lines connecting the points        
        marktype_lst.append("none")
        mrotate_lst.append(0)
        color_lst.append(grey)
        fill_lst.append(grey)
        linetype_lst.append("solid")

        ## the data points
        for (predictor, mark) in series:
            if predictor in ("None", "AlwaysBroadcast"):
                marktype_lst.append("none")
                mrotate_lst.append(0)
                color_lst.append(black)
                fill_lst.append(black)
                linetype_lst.append("dotted")

            (marktype, mrotate) = predictor_to_shape(predictor)
            marktype_lst.append(marktype)
            mrotate_lst.append(mrotate)

            (color, fill) = mark
            color_lst.append(color)
            fill_lst.append(fill)
            linetype_lst.append("none")
        
    ## Make the graphs
    all_data = []
    all_parameters = []

    for benchmark in benchmarks:
        print " ", benchmark
        # collect data
        data = []

        for series in data_points:
            segments = []
            points = []
            for data_point in series:
                predictor = data_point[0]
                predictor_desc = predictor_name_transform(predictor)

                # get the data
                #lst = (-1, -1, -1, -1)
                pred_data = get_maskpred_data(benchmark, predictor)
                dir_data = get_maskpred_data(benchmark, "None")
                bcast_data = get_maskpred_data(benchmark, "AlwaysBroadcast")

                if None in (pred_data, dir_data, bcast_data):
                    x_value = -1
                    y_value = -1
                else:
                    (control_msgs_per_miss, indirections, cycles, total_bandwidth) = pred_data
                    (dir_control_msgs_per_miss, dir_indirections, dir_cycles, dir_total_bandwidth) = dir_data
                    (bcast_control_msgs_per_miss, bcast_indirections, bcast_cycles, bcast_total_bandwidth) = bcast_data

                    normalized_cycles = 100*cycles/dir_cycles  # correct one
                    normalized_bandwidth = 100*total_bandwidth/bcast_total_bandwidth # correct one

                    if not runtime:
                        x_value = control_msgs_per_miss
                        y_value = indirections

                    else:
                        x_value = normalized_bandwidth
                        y_value = normalized_cycles

                print "   ", predictor, "->", benchmark, predictor_desc, x_value, y_value
                if predictor == "None":
                    points.append(["", [x_value, y_value], [x_value, 0]])
                if predictor == "AlwaysBroadcast":
                    points.append(["", [x_value, y_value], [0, y_value]])
                points.append([predictor_desc, [x_value, y_value]])
                segments.append([x_value, y_value])

            data.append([""] + segments)
            for line in points:
                data.append(line)
            
        # graph the data
        all_data.append(data)
        all_parameters.append({ "title" : benchmark_names[benchmark] })

    # only display the legend on the last graph
##     all_parameters[-1]["legend"] = "on"
##     if len(benchmarks) == 6:
##         all_parameters[-1]["legend_x"] = -80
##         all_parameters[-1]["legend_y"] = -70
##         all_parameters[-1]["legend_default"] = "hjl vjt"
        
    if not runtime:
        xlabel = "request messages per miss"
        ylabel = "indirections (percent of all misses)"
    else:
        xlabel = "normalized traffic per miss"
        ylabel = "normalized runtime"
        
    output = mfgraph.multi_graph(all_data,
                                 all_parameters,
                                 legend = "off",
                                 xsize = 1.8,
                                 ysize = 1.8,
                                 xlabel = xlabel,
                                 ylabel = ylabel,
                                 linetype = linetype_lst,
                                 colors = color_lst,
                                 fills = fill_lst,
                                 xmin = 0.0,
                                 ymin = 0.0,
                                 cols = 3,
                                 #ymax = 100.0,
                                 marktype = marktype_lst,
                                 mrotate = mrotate_lst,
                                 title_fontsize = "12",
                                 legend_hack = "yes",
                                 )

    mfgraph.run_jgraph(output, name)

##############################################

runs = []
runs.append([trace_graphs, 0, "trace-reader"])
runs.append([ruby_graphs, 1, "ruby_results"])
runs.append([opal_graphs, 1, "opal_results"])

for (all_graphs, config, root_local) in runs:
    maskpred_data_cache = {}
    runtime = config
    root = root_local
    for graph in all_graphs:
        make_graph(graph)
