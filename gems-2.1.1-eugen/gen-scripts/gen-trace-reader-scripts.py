#!/s/std/bin/python
import string, os, glob

results_dir = "trace-reader"

def filename_to_workload(filename):
    base = os.path.basename(filename)
    parts = string.split(base, "-")
    return parts[0]

def workload_to_memory(workload):
    workload_to_memory_map = {
        "jbb_100000" : 480,
        "apache_12500" : 480,
        }
    return workload_to_memory_map.get(workload, 450)

def workload_prio(workload):
    workload_prio_map = {
        "barnes_512" : 17,
        "oltp_1000" : 15,
        "ocean_514" : 12,
        "barnes_64" : 11,
        }
    return workload_prio_map.get(workload, 10)

# name:information:index:coarseness:sets:assoc
# ["Random:Both:PC:0:0:0"]
predictor_list = []
predictor_list.append("AlwaysUnicast")
predictor_list.append("AlwaysBroadcast")

##############################################

type_list = []

type_list.append("Owner:Implicit")

type_list.append("Counter:Implicit:5")
type_list.append("OwnerGroup:Implicit")
type_list.append("OwnerGroupMod:Implicit")

type_list.append("BroadcastCounter:Implicit")
type_list.append("OwnerBroadcast:Implicit")
type_list.append("OwnerBroadcastMod:Implicit")

index_list = []
index_list.append("PC:0")
#index_list.append("PC:2")
#index_list.append("PC:4")
#index_list.append("PC:6")
index_list.append("DataBlock:0")
index_list.append("DataBlock:2")
index_list.append("DataBlock:4")
index_list.append("DataBlock:6")

size_list = []
size_list.append("0")
#size_list.append("2048:4")
#size_list.append("8192:4")

for type in type_list:
    for index in index_list:
        for size in size_list:
            predictor_list.append("%s:%s:%s" % (type, index, size))

##############################################

predictor_list.append("StickySpatial:Both:0:DataBlock:0:0")
predictor_list.append("StickySpatial:Both:1:DataBlock:0:0")
predictor_list.append("StickySpatial:Both:2:DataBlock:0:0")

predictor_list.append("StickySpatial:Both:1:DataBlock:0:8192:1")
predictor_list.append("StickySpatial:Both:1:DataBlock:0:32768:1")

##############################################

type_list = []

type_list.append("Owner:Implicit")

type_list.append("Counter:Implicit:5")
type_list.append("OwnerGroup:Implicit")
type_list.append("OwnerGroupMod:Implicit")

type_list.append("BroadcastCounter:Implicit")
type_list.append("OwnerBroadcast:Implicit")
type_list.append("OwnerBroadcastMod:Implicit")

index_list = []
index_list.append("PC:0")
#index_list.append("PC:2")
#index_list.append("PC:4")
#index_list.append("PC:6")
#index_list.append("DataBlock:0")
#index_list.append("DataBlock:2")
index_list.append("DataBlock:4")
#index_list.append("DataBlock:6")

size_list = []
#size_list.append("0")
size_list.append("2048:4")
size_list.append("8192:4")

for type in type_list:
    for index in index_list:
        for size in size_list:
            predictor_list.append("%s:%s:%s" % (type, index, size))

##############################################

trace_list = glob.glob("/scratch/multifacet/traces/*.trace.gz")
workload_list = map(filename_to_workload, trace_list)

common_condor = """
executable = %(cvsroot_condor_bin)s/%(module)s.exec
notification = Never

rank = TARGET.Mips
universe = standard
local_files = /scratch/multifacet/traces/*, /proc/self/*
"""

template_condor = """
arguments = --predictor %(predictor)s -p %(processors)s --trace_input %(filename)s
filename = %(cvsroot_condor_results)s/%(workload)s/%(workload)s-%(processors)sp-%(module)s-%(predictor)s-$(Cluster)-$(Process)
error = $(filename).error
output = $(filename).stats
queue 1
"""

## Calculate the CVS root and other absolute paths
cvsroot_condor = os.path.split(os.getcwd())[0]
cvsroot = os.path.split(cvsroot_condor)[0]
cvsroot_simics = os.path.join(cvsroot, "simics")
cvsroot_condor_bin = os.path.join(cvsroot_condor, "bin")
cvsroot_condor_results = os.path.join(cvsroot_condor, results_dir)
cvsroot_condor_results_scripts = os.path.join(cvsroot_condor_results, "scripts")

if not os.path.exists(cvsroot_condor_bin):
    print "Creating %s" % cvsroot_condor_bin
    os.mkdir(cvsroot_condor_bin)
    os.system("fs setacl %s net:cs read" % cvsroot_condor_bin)

if not os.path.exists(cvsroot_condor_results):
    print "Creating %s" % cvsroot_condor_results
    os.mkdir(cvsroot_condor_results)
    os.system("fs setacl %s net:cs write" % cvsroot_condor_results)

if not os.path.exists(cvsroot_condor_results_scripts):
    print "Creating %s" % cvsroot_condor_results_scripts
    os.mkdir(cvsroot_condor_results_scripts)
    os.system("fs setacl %s net:cs read" % cvsroot_condor_results_scripts)

for workload_name in workload_list:
    directory = "%s/%s" % (cvsroot_condor_results, workload_name)
    if not os.path.exists(directory):
        os.mkdir(directory)

run_list = []
run_list.append(["gs320", ["None"]])
run_list.append(["mcast", predictor_list])

processors = 16
for (module, predictors) in run_list:
    for filename in trace_list:
        workload = filename_to_workload(filename)

        print "  Creating %s-%s" % (workload, module)
        condor_file = open("%s/%s-%s.condor" % (cvsroot_condor_results_scripts, workload, module), "w")
        condor_file.write(common_condor % vars())

        ## Requirements
        requirement_list = []
        memory = workload_to_memory(workload)
        if module == "gs320":
           memory = 1500 
        requirement_list.append('(Memory > %d)' % memory)
        requirement_list.append('(Arch == "INTEL")')
        requirement_list.append('(OpSys == "LINUX")')
        requirement_list.append('(VirtualMachineID == 1)')
        requirement_list.append('(IsC2Cluster == True)')
        requirement_list.append('(IsComputeCluster == True)')
        requirement_list.append('(FileSystemDomain == "cs.wisc.edu" || FileSystemDomain == ".cs.wisc.edu")')
        requirement_list.append('(IsDedicated == True)')
        req = string.join(requirement_list, " && ")

        condor_file.write("requirements = %s\n" % req)
        condor_file.write("\n")

#        condor_file.write("+IsCommittedJob = TRUE\n")

        for predictor in predictors:
            condor_file.write("priority = %d\n" % workload_prio(workload))
            condor_file.write(template_condor % vars())

        condor_file.close()
