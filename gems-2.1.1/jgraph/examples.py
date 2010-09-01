#!/s/std/bin/python

# This is an example of how to use the functions from mfgraph to
# generate jgraph images.  Run this script and look in examples.pdf
# for two graphs.  If you want to generate an eps, you can only
# generate one graph at a time.  To do this comment out the line below
# and un-comment the line below it.

graphs = ["graph1", "graph2"]
graphs = ["graph1"]
 
import sys, string, os, glob, re, mfgraph

def generate_line_example(jgraphs, title):
    xlabel = "x label"
    ylabel = "y label"
    lines = [["line1", [10, 10], [20, 20], [30, 30]],
             ["line2", [10, 5], [20, 25], [30, 80]],
             ]
    jgraphs.append(mfgraph.line_graph(lines,
                                      title = title,
                                      xlabel = xlabel,
                                      ylabel = ylabel,
                                      ))


def generate_bar_example(jgraphs):
    bars = [
        ["group1",
         ["bar1", 20, 10, 5],
         ["bar2", 10, 5, 2.5],
         ],
        ["group2",
         ["bar1", 80, 40, 10],
         ["bar2", [100, 90, 110], 50, 10],  # note, this has an error bar
         ["bar3", 30, 25, 5],
         ]
        ]
    
    jgraphs.append(mfgraph.stacked_bar_graph(bars,
                                             bar_segment_labels = ["segment1", "segment2", "segment3"],
                                             xsize = 5.0,
                                             ))

jgraph_input = []
for graph in graphs:
    generate_line_example(jgraph_input, graph)
generate_bar_example(jgraph_input)

mfgraph.run_jgraph("newpage\n".join(jgraph_input), "examples")
