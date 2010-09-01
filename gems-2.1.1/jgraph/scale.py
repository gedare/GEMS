#!/s/std/bin/python
import sys, string, os, glob, re, mfgraph

def gen_scale(benchmarks):
    configs = ["1p", "2p", "4p", "8p", "16p"]

    base_config = "1p"

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
            med = mfgraph.median(numbers)
            assoc_data[config] = med;
        mfgraph.normalize(assoc_data, base_config)

        bars = []
        stack_data = [benchmark]
        for config in configs:
            bars.append([config, assoc_data[config]])
        stacks.append([benchmark] + bars)
    print "done."

    return [mfgraph.stacked_bar_graph(stacks,
                                      title = "Scalability",
                                      ylabel = "normalized runtime",
                                      colors = ["0 0 1"],
                                      patterns = ["solid"])]
