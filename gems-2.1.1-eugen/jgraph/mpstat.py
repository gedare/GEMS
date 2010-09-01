#!/s/std/bin/python
import sys, string, os, glob, re, mfgraph

configs = ["1p-MOSI_bcast_opt", "2p-MOSI_bcast_opt", "4p-MOSI_bcast_opt", "8p-MOSI_bcast_opt", "16p-MOSI_bcast_opt"]

cache = {}
def get_mpstats(filename):
    if cache.has_key(filename):
        return cache[filename]
    sum = [0.0] * 16
    lines = mfgraph.inverse_grep(filename, "CPU");
    counter = 0
    for line in lines:
        columns = string.split(line)
        columns = map(float, columns)
        if len(columns) == len(sum):
            sum = map(lambda x,y: x+y, sum, columns)
            counter += 1
        else:
            print "partial line ignored: ", line

    if counter > 0:
        for index in range(len(sum)):
            sum[index] = sum[index] / float(counter)

    cache[filename] = sum
    return sum


###########################################################3

def gen_mpstats(benchmarks):
    print "parsing..."
    jgraph_input = []

    stacks = [];
    for benchmark in benchmarks:
        bars = []
        for config in configs:
            print "  %s %s" % (benchmark, config)
            filenames = glob.glob(benchmark + "/" + benchmark + "*-" + config + "-*.xterm")
            for filename in filenames:
                stats = get_mpstats(filename)
                stats = stats[12:]
                stats.reverse()
                config_label = string.split(config, "-")[0]
                bar = [config_label] + mfgraph.stack_bars(stats)
                bars.append(bar)
        stacks.append([benchmark] + bars)

    jgraph_input.append(mfgraph.stacked_bar_graph(stacks,
                                                  bar_segment_labels = ["idle", "wait", "system", "user"],
                                                  title = "utilization",
                                                  ylabel = "percent",
                                                  colors = [".8 .8 .8", "0 .5 0", "0 0 1", "1 0 0"],
                                                  patterns = ["stripe 45", "solid", "solid", "solid"]))


    column_label = "CPU minor-faults major-faults cross-calls interrupts interrupts-as-threads context-switches involuntary-context-switches migrations mutex-spins read-write-spins system-calls usr sys  wt idl"

    column_name = string.split(column_label)
    column_name = map(string.strip, column_name)

    for column in range(1, 12):
        stacks = []
        for benchmark in benchmarks:
            bars = []
            for config in configs:
                filenames = glob.glob(benchmark + "/" + benchmark + "*-" + config + "-*.xterm")
                for filename in filenames:
                    stats = get_mpstats(filename)
                    config_label = string.split(config, "-")[0]
                    bars.append([config_label, stats[column]])
            stacks.append([benchmark] + bars)
        jgraph_input.append(mfgraph.stacked_bar_graph(stacks,
                                                      title = column_name[column],
                                                      ylabel = "operations/second (for each processor)",
                                                      colors = ["0 0 1"],
                                                      patterns = ["solid"]))
    return jgraph_input
