#!/s/std/bin/python
import sys, string, os, glob, re, mfgraph

def gen_misses(benchmarks):
    configs = ["1p-MOSI_bcast_opt", "2p-MOSI_bcast_opt", "4p-MOSI_bcast_opt", "8p-MOSI_bcast_opt", "16p-MOSI_bcast_opt"]

    parameters = ["Request_type_IFETCH", "Request_type_LD", "Request_type_ST", "Request_type_ATOMIC"]

    stacks = []
    for benchmark in benchmarks:
        bars = []
        for config in configs:
            print "  %s %s" % (benchmark, config)
            filenames = glob.glob(benchmark + "/*-" + config + "-*.stats")
            for filename in filenames:
                numbers = []
                lines = mfgraph.grep(filename, "instruction_executed");
                line = string.split(lines[0])
                insn = long(line[1])
                for parameter in parameters:
                    lines = mfgraph.grep(filename, parameter);
                    line = string.split(lines[0])
                    map(string.strip, line)
                    numbers.append(1000.0*(float(line[1])/insn))
                numbers = mfgraph.stack_bars(numbers)
                config_label = string.split(config, "-")[0]
                bars.append([config_label] + numbers)
        stacks.append([benchmark] + bars)

    labels = []
    for label in parameters:
        labels.append(string.split(label, "_")[2])

    return [mfgraph.stacked_bar_graph(stacks,
                                      bar_segment_labels = labels,
                                      title = "Breakdown of misses",
                                      ylabel = "misses per thousand instructions",
                                      patterns = ["solid"],
                                      )]



