#!/s/std/bin/python
import sys, string, os, glob, re, mfgraph

def get_data(benchmark, find_predictor):
    command = 'grep "" %s/*.data | grep -v "#"' % benchmark
    results = os.popen(command, "r")
    predictors = []
    for line in results.readlines():
        line = string.strip(line)
        line = string.translate(line, string.maketrans("/:","  "))
        data = string.split(line)
        (benchmark, predictor, bandwidth, runtime) = data
        if (predictor == find_predictor):
            predictors.append(data[1:])
    return predictors

benchmarks = [
#    "apache_1000",
#    "barnes_16k",
    "oltp_1000",
#    "slash_25",
#    "jbb_50000",
    ]

benchmark_map = {
    "apache_1000" : "Apache",
    "barnes_16k" : "Barnes-Hut",
    "jbb_50000" : "SPECjbb",
    "slash_25" : "Slashcode",
    "oltp_1000" : "OLTP",
    }

predictors = [
    "MOSI_GS.data",
    "MOSI_bcast_opt.data",
    "addr_counter_mask_coarse-large.data",
    "addr_counter_mask_coarse-small.data",
    ]

predictor_map = {
    "MOSI_GS.data" : "Directory",
    "MOSI_bcast_opt.data" : "Broadcast Snooping",
    "addr_counter_mask_coarse-large.data" : "Group (both) - Macroblock - 16k entries",
    "addr_counter_mask_coarse-small.data" : "Group (both) - Macroblock - 1k entries",
    }

jgraph_input = []
cols = 2
row_space = 2.8
col_space = 2.8
num = 0
for bench in benchmarks:
    lines = []
    for predictor in predictors:
        predictor_data = get_data(bench, predictor)[0]
        (predictor, bandwidth, runtime) = predictor_data
        if not (predictor in predictors):
            continue
        line = [predictor_map[predictor], [float(bandwidth), float(runtime)]]
        lines.append(line)

    
    legend_hack = ""
    if (num == 0):
        legend_hack = "yes"

    print legend_hack
    
    jgraph_input.append(mfgraph.line_graph(lines,
                                           title = benchmark_map[bench],
                                           title_fontsize = "12",
                                           title_font = "Times-Roman",
                                           xsize = 1.8,
                                           ysize = 1.8,
                                           xlabel = "control bandwidth (normalized to Broadcast)",
                                           ylabel = "runtime (normalized to Directory)",
                                           label_fontsize = "10",
                                           label_font = "Times-Roman",
                                           legend_fontsize = "10",
                                           legend_font = "Times-Roman",
                                           linetype = ["none"],
                                           marktype = ["circle", "box", "diamond", "triangle", "triangle"],
                                           mrotate = [0, 0, 0, 0, 180],
                                           colors = ["0 0 0"], 
                                           xmin = 0,
                                           x_translate = (num % cols) * col_space,
                                           y_translate = (num / cols) * -row_space,
                                           line_thickness = 1.0,
                                           legend_hack = legend_hack,
                                           legend_x = "150",
#                                           legend_y = "",
                                           ))
    
    num += 1

mfgraph.run_jgraph("\n".join(jgraph_input), "/p/multifacet/papers/mask-prediction/graphs/fullsystem-size")
