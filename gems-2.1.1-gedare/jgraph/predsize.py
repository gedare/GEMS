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
#    "static-web_warm-16p",
#    "barnes-64k-16p",
    "tpcc-10000-16p",
#    "slash_100-16p",
#    "jbb-16p",
]

benchmark_map = {
    "static-web_warm-16p" : "Apache",
    "barnes-64k-16p" : "Barnes-Hut",
    "tpcc-10000-16p" : "OLTP",
    "slash_100-16p" : "Slashcode",
    "jbb-16p" : "SPECjbb",
}

predictors_old = [
#    'Address-based_Broadcast_Counter-16k.data',
#    'Address-based_Broadcast_Counter-1k.data',
#    'Address-based_Broadcast_Counter-infinite.data',
#    'Address-based_Broadcast_Counter_Coarse-16k.data',
#    'Address-based_Broadcast_Counter_Coarse-1k.data',
#    'Address-based_Broadcast_Counter_Coarse-infinite.data',
    'Address-based_Counter-16k.data',
    'Address-based_Counter-1k.data',
    'Address-based_Counter-infinite.data',
    'Address-based_Counter_Coarse-16k.data',
    'Address-based_Counter_Coarse-1k.data',
    'Address-based_Counter_Coarse-infinite.data',
    'Address-based_Counter_W__Mask_Update-16k.data',
    'Address-based_Counter_W__Mask_Update-1k.data',
    'Address-based_Counter_W__Mask_Update-infinite.data',
    'Address-based_Counter_W__Mask_Update_Coarse-16k.data',
    'Address-based_Counter_W__Mask_Update_Coarse-1k.data',
    'Address-based_Counter_W__Mask_Update_Coarse-infinite.data',
#    'Broadcast_If_Shared-16k.data',
#    'Broadcast_If_Shared-1k.data',
##    'Broadcast_If_Shared-infinite.data',
#    'Broadcast_If_Shared_Coarse-16k.data',
#    'Broadcast_If_Shared_Coarse-1k.data',
##    'Broadcast_If_Shared_Coarse-infinite.data',
#    'Broadcast_Snooping-16k.data',
#    'Broadcast_Snooping-1k.data',
    'Broadcast_Snooping-infinite.data',
#    'Broadcast_Unless_Private-16k.data',
#    'Broadcast_Unless_Private-1k.data',
#    'Broadcast_Unless_Private-infinite.data',
#    'Broadcast_Unless_Private_Coarse-16k.data',
#    'Broadcast_Unless_Private_Coarse-1k.data',
#    'Broadcast_Unless_Private_Coarse-infinite.data',
#    'Directory-16k.data',
#    'Directory-1k.data',
    'Directory-infinite.data',
#    'PC-based_Broadcast_Counter-16k.data',
#    'PC-based_Broadcast_Counter-1k.data',
#    'PC-based_Broadcast_Counter-infinite.data',
#    'PC_Counter-16k.data',
#    'PC_Counter-1k.data',
#    'PC_Counter-infinite.data',
#    'Pairwise-16k.data',
#    'Pairwise-1k.data',
#    'Pairwise-infinite.data',
#    'Pairwise_Coarse-16k.data',
#    'Pairwise_Coarse-1k.data',
#    'Pairwise_Coarse-infinite.data',
#    'PerfectMask-16k.data',
#    'PerfectMask-1k.data',
    'PerfectMask-infinite.data',
#    'Sticky-Spatial_2K-16k.data',
#    'Sticky-Spatial_2K-1k.data',
#    'Sticky-Spatial_2K-infinite.data',
#    'Sticky-Spatial_2K_Coarse-16k.data',
#    'Sticky-Spatial_2K_Coarse-1k.data',
#    'Sticky-Spatial_2K_Coarse-infinite.data',
#    'Sticky-Spatial_Infinite-infinite.data',
#    'Sticky-Spatial_Infinite_Coarse-infinite.data',
#    'Unicast-16k.data',
#    'Unicast-1k.data',
#    'Unicast-infinite.data'
    ]

predictors = [
    'Directory-infinite.data',
    'Broadcast_Snooping-infinite.data',
    'PerfectMask-infinite.data',

#    'Address-based_Counter_W__Mask_Update-1k.data',
#    'Address-based_Counter_W__Mask_Update-16k.data',
#    'Address-based_Counter_W__Mask_Update-infinite.data',
    'Address-based_Counter_W__Mask_Update_Coarse-1k.data',
    'Address-based_Counter_W__Mask_Update_Coarse-16k.data',
    'Address-based_Counter_W__Mask_Update_Coarse-infinite.data',

    ]

predictor_map = {
    'Directory-infinite.data' : "Directory", 
    'Broadcast_Snooping-infinite.data' : "Broadcast Snooping",
    'Pairwise-infinite.data' : "Pairwise (implicit) - Block",
    'Pairwise_Coarse-infinite.data' : "Pairwise (implicit) - Macroblock",
    'PerfectMask-infinite.data' : "Perfect",
    "Address-based_Broadcast_Counter-infinite.data" : "AtHome (explicit) - Block",
    "Address-based_Broadcast_Counter_Coarse-infinite.data" : "AtHome (explicit) - Macroblock",


    "Address-based_Counter-infinite.data" : "Group (implicit) - Block - infinite",
    "Address-based_Counter_Coarse-infinite.data" : "Group (implicit) - Macroblock - infinite",
    "Address-based_Counter_W__Mask_Update-infinite.data" : "Group (both) - Block - infinite",
    "Address-based_Counter_W__Mask_Update_Coarse-infinite.data" : "Group (both) - Macroblock - infinite",

    "Address-based_Counter-1k.data" : "Group (implicit) - Block - 1k",
    "Address-based_Counter_Coarse-1k.data" : "Group (implicit) - Macroblock - 1k",
    "Address-based_Counter_W__Mask_Update-1k.data" : "Group (both) - Block - 1k",
    "Address-based_Counter_W__Mask_Update_Coarse-1k.data" : "Group (both) - Macroblock - 1k",

    "Address-based_Counter-16k.data" : "Group (implicit) - Block - 16k",
    "Address-based_Counter_Coarse-16k.data" : "Group (implicit) - Macroblock - 16k",
    "Address-based_Counter_W__Mask_Update-16k.data" : "Group (both) - Block - 16k",
    "Address-based_Counter_W__Mask_Update_Coarse-16k.data" : "Group (both) - Macroblock - 16k",

    "PC-based_Broadcast_Counter-infinite.data" : "AtHome (explicit) - PC",
    "PC_Counter-infinite.data" : "Group (explicit) - PC ",

#    'Broadcast_If_Shared-infinite.data',
#    'Broadcast_If_Shared_Coarse-infinite.data',

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
#        line = [predictor, [float(bandwidth), float(runtime)]]
        lines.append(line)
    
    legend_hack = ""
    if (num+1 == len(benchmarks)):
        legend_hack = "yes"

    print legend_hack
    
    jgraph_input.append(mfgraph.line_graph(lines,
                                           title = benchmark_map[bench],
                                           title_fontsize = "12",
                                           title_font = "Times-Roman",
                                           xsize = 1.8,
                                           ysize = 1.8,
                                           xlabel = "control bandwidth (normalized to Broadcast)",
                                           ylabel = "indirections (normalized to Directory)",
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

mfgraph.run_jgraph("\n".join(jgraph_input), "/p/multifacet/papers/mask-prediction/graphs/predsize")
