#!/s/std/bin/python
import sys, string, os, glob, re, mfgraph

def gen_cache_state(benchmarks):
    configs = ["1p", "2p", "4p", "8p", "16p"]
    parameters = ["GETS NP", "GETS I", "GETX NP", "GETX I", "GETX S", "GETX O"]
    
    stacks = []
    for benchmark in benchmarks:
        bars = []
        for config in configs:
            filenames = glob.glob(benchmark + "/*-" + config + "-*.stats")
            print "  %s %s" % (benchmark, config), filenames
            numbers = []
            for parameter in parameters:
                sum = 0
                for filename in filenames:
#                    print benchmark, config, parameter, filename
                    lines = mfgraph.grep(filename, "instruction_executed");
                    line = string.split(lines[0])
                    insn = long(line[1])
                    
                    lines = mfgraph.grep(filename, parameter);
                    for line in lines:
                        fields = string.split(line)
#                        print fields
                        sum += int(fields[3])
                numbers.append(float(sum)/float(insn))

            numbers = mfgraph.stack_bars(numbers)
            bars.append([config] + numbers)
        stacks.append([benchmark] + bars)

    labels = []
    for label in parameters:
        labels.append(label)

    return [mfgraph.stacked_bar_graph(stacks,
                                      bar_segment_labels = labels,
                                      title = "Cache Misses by state",
                                      ylabel = "Number",
                                      )]



