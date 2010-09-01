#!/s/std/bin/python
import sys, string, os, glob, re, mfgraph

def get_request_profile(filename):
    map = {}
    file = open(filename, "r")
    line = string.strip(file.readline())
    while (line != "Request Profile"):
        line = string.strip(file.readline())
    line = string.strip(file.readline())  # skip two lines
    line = string.strip(file.readline())
    line = string.strip(file.readline())  # first line of stats
    while (line != ""):
        line = string.split(line)

        # convert lines
        if line[0] == "GET_INSTR":
            line[0] = "GETS"

        if line[0] == "PUTX":
            line = string.strip(file.readline())
            continue

        if line[2] == "OS" or line[2] == "OSS":
            line[2] = "O"

        if line[2] == "C":
            line[2] = "I"

        key = "%s:%s:%s" % (line[0], line[1], line[2])
        map[key] = int(line[3]) + map.get(key, 0)

        # get next line
        line = string.strip(file.readline())
    return map

def normalize_map(map):
    sum = 0
    for key in map.keys():
        sum += map[key]

    for key in map.keys():
        map[key] = 100.0 * (map[key] / float(sum))
    

stack_names = [
    "GETS:NP",
    "GETS:I",
    "GETX:NP",
    "GETX:I",
    "GETX:S",
    "GETX:O",
    ]

dir_states = [
    "I",
    "S",
    "O",
    "M",
    ]

hop_3 = "3-hop"
hop_2 = "2-hop"
hop_25 = "2-hop"
upg_2 = "2-upg"
upg_3 = "3-upg"

classification_map = {
    "GETS:NP:I" : hop_2,
    "GETS:NP:S" : hop_2,
    "GETS:NP:SS" : hop_2,
    "GETS:NP:O" : hop_3,
    "GETS:NP:M" : hop_3,

    "GETS:I:I" : hop_2,
    "GETS:I:S" : hop_2,
    "GETS:I:SS" : hop_2,
    "GETS:I:O" : hop_3,
    "GETS:I:M" : hop_3,

    "GETX:NP:I" : hop_2,
    "GETX:NP:S" : hop_25,
    "GETX:NP:SS" : hop_25,
    "GETX:NP:O" : hop_3,
    "GETX:NP:M" : hop_3,

    "GETX:I:I" : hop_2,
    "GETX:I:S" : hop_25,
    "GETX:I:SS" : hop_25,
    "GETX:I:O" : hop_3,
    "GETX:I:M" : hop_3,

    "GETX:S:S" : upg_2,
    "GETX:S:SS" : upg_3,
    "GETX:S:O" : upg_3,

    "GETX:O:O" : upg_3,
    }

def get_sharing(filename):
    profile = get_request_profile(filename)

    hops = {}
    for key in profile.keys():
        classification = classification_map.get(key, "other")
        hops[classification] = hops.get(classification, 0.0) + profile[key]

    results = []
    for key in labels:
        results.append(hops.get(key, 0.0))
    return results

labels = [
    "other",
    "2-upg",
    "2-hop",
    "3-upg",
    "3-hop",
    ]

def gen_sharing(benchmarks, normalize = 1):
    configs = ["1p", "2p", "4p", "8p", "16p"]

    stacks = []
    for benchmark in benchmarks:
        bars = []
        for config in configs:
            print "  %s %s" % (benchmark, config)
            filenames = glob.glob(benchmark + "/*-" + config + "-*.stats")
            for filename in filenames:
                lines = mfgraph.grep(filename, "instruction_executed");
                line = string.split(lines[0])
                insn = long(line[1])
                numbers = get_sharing(filename)

                if normalize:
                    sum = reduce(lambda x,y: x+y, numbers)
                    for index in range(len(numbers)):
                        numbers[index] = (numbers[index] / sum) * 100.0
                else:
                    for index in range(len(numbers)):
                        numbers[index] = (numbers[index] / float(insn)) * 1000.0

                numbers = mfgraph.stack_bars(numbers)
                bars.append([config] + numbers)
        stacks.append([benchmark] + bars)

    if normalize:
        y_axis_label = "percent of misses"
    else:
        y_axis_label = "misses per thousand instructions",
        
    return [mfgraph.stacked_bar_graph(stacks,
                                      bar_segment_labels = labels,
                                      title = "Breakdown of misses",
                                      ylabel = y_axis_label,
                                      patterns = ["stripe -45", "stripe -45", "stripe -45", "solid", "solid"],
                                      )]

