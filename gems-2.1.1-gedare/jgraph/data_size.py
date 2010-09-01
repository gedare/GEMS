#!/s/std/bin/python
import sys, string, os, glob, re, mfgraph

def gen_data_size(benchmarks):
    configs = ["1p-MOSI_bcast_opt", "2p-MOSI_bcast_opt", "4p-MOSI_bcast_opt", "8p-MOSI_bcast_opt", "16p-MOSI_bcast_opt"]

    parameters = ["C  GET_INSTR", "C  GETS", "C  GETX"]
    
    stacks = []
    for benchmark in benchmarks:
        bars = []
        for config in configs:
            print "  %s %s" % (benchmark, config)
            filenames = glob.glob(benchmark + "/*-" + config + "-*.stats")
            for filename in filenames:
                numbers = []
                for parameter in parameters:
                    lines = mfgraph.grep(filename, parameter);
                    line = string.split(lines[0])
                    map(string.strip, line)
                    num = string.split(line[2], "%")
                    num = (64L*(long(num[0])))/(1024.0*1024.0)
                    numbers.append(num)

                numbers = mfgraph.stack_bars(numbers)
                number = reduce(lambda x,y:x+y, numbers)
                config_label = string.split(config, "-")[0]
                bars.append([config_label] + [number])
        stacks.append([benchmark] + bars)

#    labels = []
#    for label in parameters:

#        labels.append(string.split(label)[1])

    return [mfgraph.stacked_bar_graph(stacks,
#                                      bar_segment_labels = labels,
                                      title = "Memory touched",
                                      ylabel = "Mbytes",
                                      patterns = ["solid"],
                                      xsize = 8.5,
                                      )]



