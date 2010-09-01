#!/s/std/bin/python
import sys, string, os, glob, re, mfgraph

def gen_protocol(benchmarks):
#    configs = ["8p-perfect", "8p-MOSI_bcast_opt", "8p-MOSI_GS"]
    configs = ["8p-MOSI_bcast_opt", "8p-MOSI_GS"]

    base_config = "8p-MOSI_bcast_opt"

    parameter = "Ruby_cycles"

    stacks = [];
    print "parsing..."
    for benchmark in benchmarks:
        assoc_data = {};
        for config in configs:
            sys.stderr.write("  %s %s\n" % (benchmark, config))
            numbers = []
            filenames = glob.glob(benchmark + "/*-" + config + "-*.stats")
            for filename in filenames:
                lines = mfgraph.grep(filename, parameter);
                line = lines[0]
                numbers.append(float(string.split(line)[1]))
            print numbers
            med = mfgraph.median(numbers)
            stddev = mfgraph.stddev(numbers)
            min_val = min(numbers)
            max_val = max(numbers)
            assoc_data[config] = [med, min_val, max_val];
        mfgraph.normalize(assoc_data, base_config)

        bars = []
        stack_data = [benchmark]
        for config in configs:
            bars.append([config, assoc_data[config]])
        stacks.append([benchmark] + bars)
    print "done."

    print stacks

    return [mfgraph.stacked_bar_graph(stacks,
                                      title = "Snooping vs Directory",
                                      ylabel = "normalized runtime",
                                      colors = [".5 .5 1"],
                                      patterns = ["solid"])]
