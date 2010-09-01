#!/s/std/bin/python
import string, os

modules_list = ["MOSI_bcast_opt_1", "MOSI_GS_1", "MOSI_mcast_aggr_1"]
modules_list_4 = ["MOSI_bcast_opt_4", "MOSI_GS_4", "MOSI_mcast_aggr_4"]

threshold_list = [0.0, 0.55, 0.75, 0.95]
default_threshold = 0.75

think_time_list = map(lambda x:x*200, range(11))
default_think_time = 1

processor_list = [4, 8, 16, 32, 64, 128, 256]
default_processor = 64

bandwidth_list = [100, 140, 200, 283, 400, 565, 600, 800, 1000, 1131, 1200, 1400, 1600, 2000, 2600, 2262, 3200, 4500, 6400, 9051, 10000, 12800, 18102, 25600, 36204, 52000]

length = 10000

def replace_parameters(str, pairs):
    temp_str = str
    for pair in pairs:
        temp_str = string.replace(temp_str, pair[0], pair[1])
    return temp_str

common_condor = """
executable = ../bin/MODULE.exec
notification = Never

rank = TARGET.Mips
universe = standard

requirements = (Memory > 50) && (Arch == "INTEL") && (OpSys == "LINUX") && (IsComputeCluster == True) && (FileSystemDomain == "cs.wisc.edu" || FileSystemDomain == ".cs.wisc.edu")

"""

template_condor = """
arguments = -p PROCESSORS -l LENGTH -b BANDWIDTH -t THRESHOLD -k THINKTIME
filename = ../results/WORKLOAD_NAME/WORKLOAD_NAME-PROCESSORSp-MODULE-BANDWIDTH-THRESHOLD-THINKTIME-$(Cluster)-$(Process)
error = $(filename).error
output = $(filename).stats
queue 1
"""

os.system("fs setacl ../bin net:cs read")

if not os.path.exists("../scripts"):
    print "Creating ../scripts"
    os.mkdir("../scripts")
    os.system("fs setacl ../scripts net:cs read")

if not os.path.exists("../results"):
    print "Creating ../results"
    os.mkdir("../results")
    os.system("fs setacl ../results net:cs write")

workload_name = "microbenchmark"
        
if not os.path.exists("../results/"+workload_name):
    os.mkdir("../results/"+workload_name)
    
for module in modules_list:
    print "  Creating "+module+"..."

    ## Set #1:  proc x bandwidth

    condor_file = open("../scripts/"+module+"-proc.condor", "w")
    condor_file.write(replace_parameters(common_condor, [["MODULE", module]]))

    threshold = default_threshold
    think_time = default_think_time
    for processor in processor_list:
        for bandwidth in bandwidth_list:
            replacement_list = (("WORKLOAD_NAME", workload_name),
                                ("PROCESSORS", "%d" % processor),
                                ("MODULE", module),
                                ("BANDWIDTH", "%d" % bandwidth),
                                ("THRESHOLD", "%f" % threshold),
                                ("THINKTIME", "%d" % think_time),
                                ("LENGTH", "%d" % length),                                
                                )

            condor_file.write("priority = -1\n")
            condor_file.write(replace_parameters(template_condor, replacement_list))
            
    condor_file.close()

    ## Set #2:  think time x bandwidth

    condor_file = open("../scripts/"+module+"-thinktime.condor", "w")
    condor_file.write(replace_parameters(common_condor, [["MODULE", module]]))

    threshold = default_threshold
    processor = default_processor
    for think_time in think_time_list:
#        for bandwidth in bandwidth_list:
        for bandwidth in [800, 1000, 1131, 1200, 1400, 1600]:
            replacement_list = (("WORKLOAD_NAME", workload_name),
                                ("PROCESSORS", "%d" % processor),
                                ("MODULE", module),
                                ("BANDWIDTH", "%d" % bandwidth),
                                ("THRESHOLD", "%f" % threshold),
                                ("THINKTIME", "%d" % think_time),
                                ("LENGTH", "%d" % length),                                
                                )
        
            condor_file.write("priority = -3\n")
            condor_file.write(replace_parameters(template_condor, replacement_list))
            
    condor_file.close()

    ## Set #3:  threshold x bandwidth

    condor_file = open("../scripts/"+module+"-threshold.condor", "w")
    condor_file.write(replace_parameters(common_condor, [["MODULE", module]]))

    think_time = default_think_time
    processor = default_processor
    for threshold in threshold_list:
        for bandwidth in bandwidth_list:
            replacement_list = (("WORKLOAD_NAME", workload_name),
                                ("PROCESSORS", "%d" % processor),
                                ("MODULE", module),
                                ("BANDWIDTH", "%d" % bandwidth),
                                ("THRESHOLD", "%f" % threshold),
                                ("THINKTIME", "%d" % think_time),
                                ("LENGTH", "%d" % length),                                
                                )
        
            condor_file.write("priority = -5\n")
            condor_file.write(replace_parameters(template_condor, replacement_list))
            
    condor_file.close()

for module in modules_list_4:
    print "  Creating "+module+"..."
    ## Set #4:  16 proc x bandwidth (with 4x broadcast cost)

    condor_file = open("../scripts/"+module+"-proc.condor", "w")
    condor_file.write(replace_parameters(common_condor, [["MODULE", module]]))

    threshold = default_threshold
    think_time = default_think_time
    processor = 16
    for bandwidth in bandwidth_list:
        replacement_list = (("WORKLOAD_NAME", workload_name),
                            ("PROCESSORS", "%d" % processor),
                            ("MODULE", module),
                            ("BANDWIDTH", "%d" % bandwidth),
                            ("THRESHOLD", "%f" % threshold),
                            ("THINKTIME", "%d" % think_time),
                            ("LENGTH", "%d" % length),                                
                            )

        condor_file.write("priority = -1\n")
        condor_file.write(replace_parameters(template_condor, replacement_list))
            
    condor_file.close()


