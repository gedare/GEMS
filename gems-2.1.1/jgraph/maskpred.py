#!/s/std/bin/python
import sys, string, os, glob, re, mfgraph

benchmarks = [
    "static-web_warm-16p",
    "barnes-64k-16p",
    "tpcc-10000-16p",
    "slash_100-16p",
    "jbb-16p",
    ]

benchmark_names = {
    "barnes-64k-16p" : "Barnes-Hut",
    "jbb-16p" :        "SPECjbb", 
    "slash_100-16p" :  "Slashcode", 
    "static-web_warm-16p" :  "Apache",
    "tpcc-10000-16p" : "OLTP",
    }

def get_trace_stats(filename):
    file = open(filename, "r")
    lines = file.readlines()

    if len(lines) == 0:
        return []

    if lines[-22] != "Summary\n":
#        print "Error parsing file: %s '%s'", filename, string.strip(lines[-22])
        return []
    
    data = lines[-10:-7]

    # name of the predictor
    name = string.join(string.split(data[0])[0:-1])

    # accuracy percent
    accuracy = string.split(data[1])[2]
    accuracy = string.atof(string.split(accuracy, "%")[0])

    # bandwidth
    bandwidth = string.split(data[2])[4]
    bandwidth = string.split(bandwidth, "%")[0]
    bandwidth = string.split(bandwidth, "(")[1]

    return (name, accuracy, bandwidth)


def normalize_list(lst):
    sum = reduce(lambda x,y:x+y, lst)
    for index in range(len(lst)):
        lst[index] = lst[index]/sum


def touched_by():
    stats = [
        "touched_by_block_address:",
        "touched_by_weighted_block_address:",
        "touched_by_macroblock_address:",
        "touched_by_weighted_macroblock_address:",
        #    "touched_by_pc_address:",
        #    "touched_by_weighted_pc_address:",
        ]
    
    stats_names = {
        "touched_by_block_address:" : "(a) Percent blocks touched by n processors",
        "touched_by_weighted_block_address:": "(b) Percent of misses to blocks touched by n processors",
        "touched_by_macroblock_address:": "(c) Percent of macroblocks touched by n processors",
        "touched_by_weighted_macroblock_address:": "(d) Percent of misses to macroblocks touched by n processors",
        }

    jgraph_input = []

    cols = 1
    row_space = 2.9
    col_space = 3

    num = 0
    for stat in stats:
        bars = []
        for benchmark in benchmarks:
            print stat, benchmark
            group = [benchmark_names[benchmark]]
            filenames = glob.glob("%s/*trace-profiler*.stats" % benchmark)
            for filename in filenames:
                line = mfgraph.grep(filename, stat)[0]
                line = string.replace(line, "]", "")
                line = string.replace(line, "[", "")
                data = string.split(line)[2:]
                data = map(float, data)
                normalize_list(data)
                for index in range(len(data)):
                    if index+1 in [1, 4, 8, 12, 16]:
                        group.append(["%d" % (index+1), data[index]*100.0])
                    else:
                        group.append(["", data[index]*100.0])

            bars.append(group)

        jgraph_input.append(mfgraph.stacked_bar_graph(bars,
                                                      title = stats_names[stat],
                                                      xsize = 6.5,
                                                      ysize = 2,
                                                      xlabel = "",
                                                      ylabel = "",
                                                      stack_space = 3,
                                                      x_translate = (num % cols) * col_space,
                                                      y_translate = (num / cols) * -row_space,
                                                      ))
        num += 1

    mfgraph.run_jgraph("\n".join(jgraph_input), "touched-by")

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
        "^pc_address ",
        ]
    
    stat_name = {
        "^block_address " : "Number of data blocks (64B)",
        "^macroblock_address " : "Number of data macroblocks (1024B)",
        "^pc_address " : "Number of instructions",
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
            filenames = glob.glob("%s/*trace-profiler*.stats" % benchmark)
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
                                               legend_x = "4000",
                                               legend_y = "20",
                                               x_translate = (num % cols) * col_space,
                                               y_translate = (num / cols) * -row_space,
                                               line_thickness = 1.0,
                                               ))


##         jgraph_input.append(mfgraph.stacked_bar_graph(bars,
##                                                       title = stats_names[stat],
##                                                       xsize = 6.5,
##                                                       ysize = 2,
##                                                       xlabel = "",
##                                                       ylabel = "",
##                                                       stack_space = 3,
##                                                       ))
        num += 1

    mfgraph.run_jgraph("\n".join(jgraph_input), "cumulative")

def accuracy():
    for benchmark in benchmarks:
        filenames = glob.glob("%s/*trace-reader*.stats" % benchmark)
        for filename in filenames:
            lst = get_trace_stats(filename)
            if len(lst) > 0:
                print filename, lst


def add_list(acc_lst, lst):
    for i in range(len(lst)):
        acc_lst[i] += lst[i]

def instant_sharers():
    jgraph_input = []

    gets_map = {
        "N N" : 1,
        "Y N" : 0,
        }

    getx_map = {
        "N N N N" : 1,
        "N N N Y" : 1,
        "N N Y N" : 1,
        "N N Y Y" : 1,

#        "N Y N N" : "X",
#        "N Y N Y" : "X",
#        "N Y Y N" : "X",
        "N Y Y Y" : 0,

        "Y N N N" : 0,
        "Y N N Y" : 0,
        "Y N Y N" : 0,
        "Y N Y Y" : 0,
        
#        "Y Y N N" : "X",
#        "Y Y N Y" : "X",
#        "Y Y Y N" : "X",
#        "Y Y Y Y" : "X",
        }
    
    cols = 2
    row_space = 2.9
    col_space = 3

    num = 0
    bars = []
    for benchmark in benchmarks:
        getx_dist = [0] * 16
        gets_dist = [0] * 16
        
        print benchmark
        group = [benchmark_names[benchmark]]
        filename = glob.glob("%s/*trace-profiler*.stats" % benchmark)[0]
        gets_mode = 0
        sum = 0
        for line in open(filename).readlines():
            line = string.strip(line)
            line = string.translate(line, string.maketrans("][", "  "))

            # set mode
            str = "Total requests: "
            if line[0:len(str)] == str:
                total_requests = int(string.split(line)[2])

            if line == "GETS message classifications:":
                gets_mode = 1;
            
            if line == "":
                gets_mode = 0;

            if gets_mode == 1:
                #gets
                key = line[0:3]
                if gets_map.has_key(key):
                    parts = string.split(line)
                    sum += int(parts[2])
                    # no histogram
                    data = parts[2:3]

                    # shift if one
                    if gets_map[key] == 1:
                        data = [0] + data

                    data = map(int, data)
                    add_list(gets_dist, data)
            else:
                #getx
                key = line[0:7]
                if getx_map.has_key(key):
                    parts = string.split(line)
                    sum += int(parts[4])
                    if len(parts) > 10:
                        # histogram
                        data = parts[19:]
                    else:
                        # no histogram
                        data = parts[4:5]

                    # shift if one
                    if getx_map[key] == 1:
                        data = [0] + data

                    data = map(int, data)
                    add_list(getx_dist, data)

        for i in range(len(getx_dist)):
            gets_dist[i] = 100.0 * ((gets_dist[i]+getx_dist[i]) / float(sum))
            getx_dist[i] = 100.0 * (getx_dist[i] / float(sum))

        getx_dist = getx_dist[0:3] + [reduce(lambda x,y:x+y, getx_dist[3:7])] + [reduce(lambda x,y:x+y, getx_dist[7:])]
        gets_dist = gets_dist[0:3] + [reduce(lambda x,y:x+y, gets_dist[3:7])] + [reduce(lambda x,y:x+y, gets_dist[7:])]

        print getx_dist
        print gets_dist
        print "indirections:", 100.0-gets_dist[0]

        labels = ["0", "1", "2", "3-7", ">8"]
##         labels = []
##         for i in range(16):
##             if i in [0, 3, 7, 11, 15]:
##                 labels.append("%d" % i)
##             else:
##                 labels.append("")

        bars.append([benchmark_names[benchmark]] + map(None, labels, gets_dist, getx_dist))
    print bars
    jgraph_input.append(mfgraph.stacked_bar_graph(bars,
                                                  bar_segment_labels = ["Get shared", "Get exclusive"],
                                                  xsize = 7,
                                                  ysize = 3,
                                                  ylabel = "Percent of all misses",
                                                  legend_x = "25",
                                                  legend_y = "90",
                        ))
    
    mfgraph.run_jgraph("\n".join(jgraph_input), "instant-sharers")

instant_sharers()
cumulative()
touched_by()
