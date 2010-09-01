#!/s/std/bin/python
import sys, string, os, glob, re, mfgraph

import misses, mpstat, scale, data_size, cache_state, sharing, protocol

benchmarks = [
    "static_web_12500",
    "dynamic_web_100",
    "java_oltp_100000",
    "oltp_1000a",
    "oltp_1000b",
    "oltp_1000c",
    "oltp_1000d",
    "barnes_128k",
    "ocean_514",
    ]

jgraph_input = []
#jgraph_input += protocol.gen_protocol(benchmarks)
jgraph_input += scale.gen_scale(benchmarks)
jgraph_input += data_size.gen_data_size(benchmarks)
jgraph_input += misses.gen_misses(benchmarks)
jgraph_input += sharing.gen_sharing(benchmarks, normalize=0)
jgraph_input += sharing.gen_sharing(benchmarks, normalize=1)
jgraph_input += mpstat.gen_mpstats(benchmarks)

mfgraph.run_jgraph("newpage\n".join(jgraph_input), "data")
