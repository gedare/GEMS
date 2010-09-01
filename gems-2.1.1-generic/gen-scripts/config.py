#!/s/std/bin/python
import string, os, sys, workloads, tools, time

def mfdate():
  return time.strftime("%b:%d:%Y:%T", time.localtime(time.time()))

#####################################################################
## Condor Configuration
#####################################################################

gemsroot = os.path.split(os.getcwd())[0]  # default to parent of gen-scripts
results_dir = "results"  # relative to gemsrootx
#results_dir = "/absolute/dir/"

# condor setup
condor_num_runs = 2  # number of runs of each data point
condor_base_priority = 0 # priority of the first run at each point
condor_memory = 500  # minimum memory (in mb) to post in the condor ad

############################################################################
##  Xact System Configuration
##   If you aren't running a transactional protocol, skip this section
############################################################################

# EagerCD EagerVM variants
ee_base_pred = "EagerCD_EagerVM_Base_Pred"
ee_base_nopred = "EagerCD_EagerVM_Base_NoPred"
ee_base_nohwbuf = "EagerCD_EagerVM_Base_NoHWBuf"
ee_magic = "EagerCD_EagerVM_Magic"
ee_timestamp = "EagerCD_EagerVM_Timestamp"
ee_hybrid_pred = "EagerCD_EagerVM_Hybrid_Pred"
ee_hybrid_nopred = "EagerCD_EagerVM_Hybrid_NoPred"

# EagerCD LazyVM variants
el_backoff_pred = "EagerCD_LazyVM_Backoff_Pred"
el_backoff_nopred = "EagerCD_LazyVM_Backoff_NoPred"
el_timestamp_pred = "EagerCD_LazyVM_Timestamp_Pred"
el_timestamp_nopred = "EagerCD_LazyVM_Timestamp_NoPred"
el_cycle = "EagerCD_LazyVM_Cycle"

# LazyVM LazyCD variants
ll_base = "LazyCD_LazyVM_Base"
ll_backoff = "LazyCD_LazyVM_Backoff"

# Virtualization
logtm_se_virtualization = "logtm_se_virtualization"

## Read/Write filter selection
# Physical signature
filter_config_list = []
#filter_config_list.append("Perfect_")

# Bit-Select Bloom Filters: <#Bits>_<#hashes>_<#SkipBits>_Regular or Parallel
#'''
#filter_config_list.append("MultiBitSel_64_1_0_Regular")
#filter_config_list.append("MultiBitSel_64_2_0_Regular")
#filter_config_list.append("MultiBitSel_64_4_0_Regular")
#filter_config_list.append("MultiBitSel_128_1_0_Regular")
#filter_config_list.append("MultiBitSel_128_2_0_Regular")
#filter_config_list.append("MultiBitSel_128_4_0_Regular")
#filter_config_list.append("MultiBitSel_256_1_0_Regular")
#filter_config_list.append("MultiBitSel_256_2_0_Regular")
#filter_config_list.append("MultiBitSel_256_4_0_Regular")
filter_config_list.append("MultiBitSel_512_1_0_Regular")
#filter_config_list.append("MultiBitSel_512_2_0_Regular")
#filter_config_list.append("MultiBitSel_512_4_0_Regular")
#filter_config_list.append("MultiBitSel_1024_1_0_Regular")
#filter_config_list.append("MultiBitSel_1024_2_0_Regular")
#filter_config_list.append("MultiBitSel_1024_4_0_Regular")
filter_config_list.append("MultiBitSel_2048_1_0_Regular")
#filter_config_list.append("MultiBitSel_2048_2_0_Regular")
#filter_config_list.append("MultiBitSel_2048_4_0_Regular")
#'''

#'''
#filter_config_list.append("MultiBitSel_64_2_0_Parallel")
#filter_config_list.append("MultiBitSel_64_4_0_Parallel")
#filter_config_list.append("MultiBitSel_128_2_0_Parallel")
#filter_config_list.append("MultiBitSel_128_4_0_Parallel")
#filter_config_list.append("MultiBitSel_256_2_0_Parallel")
#filter_config_list.append("MultiBitSel_256_4_0_Parallel")
#filter_config_list.append("MultiBitSel_512_2_0_Parallel")
filter_config_list.append("MultiBitSel_512_4_0_Parallel")
#filter_config_list.append("MultiBitSel_1024_2_0_Parallel")
#filter_config_list.append("MultiBitSel_1024_4_0_Parallel")
#filter_config_list.append("MultiBitSel_2048_2_0_Parallel")
filter_config_list.append("MultiBitSel_2048_4_0_Parallel")
#'''

# H3 Bloom filters: <#Bits>_<#hashes>_Regular or Parallel
#'''
#filter_config_list.append("H3_64_1_Regular")
#filter_config_list.append("H3_64_2_Regular")
#filter_config_list.append("H3_64_4_Regular")
#filter_config_list.append("H3_128_1_Regular")
#filter_config_list.append("H3_128_2_Regular")
#filter_config_list.append("H3_128_4_Regular")
#filter_config_list.append("H3_256_1_Regular")
#filter_config_list.append("H3_256_2_Regular")
#filter_config_list.append("H3_256_4_Regular")
filter_config_list.append("H3_512_1_Regular")
#filter_config_list.append("H3_512_2_Regular")
#filter_config_list.append("H3_512_4_Regular")
#filter_config_list.append("H3_1024_1_Regular")
#filter_config_list.append("H3_1024_2_Regular")
#filter_config_list.append("H3_1024_4_Regular")
filter_config_list.append("H3_2048_1_Regular")
#filter_config_list.append("H3_2048_2_Regular")
#filter_config_list.append("H3_2048_4_Regular")
#'''
#'''
#filter_config_list.append("H3_64_2_Parallel")
#filter_config_list.append("H3_64_4_Parallel")
#filter_config_list.append("H3_128_2_Parallel")
#filter_config_list.append("H3_128_4_Parallel")
#filter_config_list.append("H3_256_2_Parallel")
#filter_config_list.append("H3_256_4_Parallel")
#filter_config_list.append("H3_512_2_Parallel")
filter_config_list.append("H3_512_4_Parallel")
#filter_config_list.append("H3_1024_2_Parallel")
#filter_config_list.append("H3_1024_4_Parallel")
#filter_config_list.append("H3_2048_2_Parallel")
filter_config_list.append("H3_2048_4_Parallel")
#'''

## Virtual Read/Write Signature
virtual_filter_config_list = []
virtual_filter_config_list.append("Perfect_")
#virtual_filter_config_list.append("MultiBitSel_2048_1_0_Regular")

## Summary Read/Write Signature
summary_filter_config_list = []
summary_filter_config_list.append("Perfect_")
#summary_filter_config_list.append("MultiBitSel_2048_1_0_Regular")

## Lock types, used in microbenchmarks
lock_type_list = []
lock_type_list.append("TM")
#lock_type_list.append("Lock")
#lock_type_list.append("Lock_TATAS")
#lock_type_list.append("Lock_TATAS_EXP")
#lock_type_list.append("Lock_MCS")

# Levels of hardware nesting support
xact_max_depth_list = []
xact_max_depth_list.append(1)
#xact_max_depth_list.append(2)

#####################################################################
##  Protocols
#####################################################################

# Protocol List [protocol name, protocol options]
protocol_list = []

# Non-Transactional Protocols
#protocol_list.append(["MOSI_SMP_bcast", None])
#protocol_list.append(["MOSI_GS", None])
#protocol_list.append(["MOSI_mcast_aggr", "AlwaysBroadcast"])
#protocol_list.append(["MOSI_mcast_aggr", "Owner:Implicit:DataBlock:4:2048:4"])
#protocol_list.append(["MOSI_mcast_aggr", "BroadcastCounter:Implicit:DataBlock:4:2048:4"])
#protocol_list.append(["MOSI_mcast_aggr", "Counter:Implicit:5:DataBlock:4:2048:4"])
#protocol_list.append(["MOSI_mcast_aggr", "OwnerGroup:Implicit:DataBlock:4:2048:4"])
#protocol_list.append(["MOESI_xact_directory_1", None])
#protocol_list.append(["MOESI_xact_directory", None])
#protocol_list.append(["MOESI_directory",None])

# Transactional Protocols
protocol_list.append(["MESI_CMP_filter_directory", ee_base_pred])
protocol_list.append(["MESI_CMP_filter_directory", ee_base_nopred])
#protocol_list.append(["MESI_CMP_filter_directory", ee_base_nohwbuf])
#protocol_list.append(["MESI_CMP_filter_directory", ee_base_magic])
#protocol_list.append(["MESI_CMP_filter_directory", ee_base_timestamp])
#protocol_list.append(["MESI_CMP_filter_directory", ee_hybrid_nopred])
protocol_list.append(["MESI_CMP_filter_directory", ee_hybrid_pred])
#protocol_list.append(["MESI_CMP_filter_directory", el_backoff_pred])
#protocol_list.append(["MESI_CMP_filter_directory", el_backoff_nopred])
protocol_list.append(["MESI_CMP_filter_directory", el_timestamp_nopred])
#protocol_list.append(["MESI_CMP_filter_directory", el_timestamp_pred])
#protocol_list.append(["MESI_CMP_filter_directory", el_cycle])
protocol_list.append(["MESI_CMP_filter_directory", ll_base])
protocol_list.append(["MESI_CMP_filter_directory", ll_backoff])
protocol_list.append(["MESI_CMP_filter_directory", logtm_se_virtualization])
#protocol_list.append(["MESI_CMP_filter_directory", logtm_vse_virtualization])

#####################################################################
##  Target System Configuration
#####################################################################

# the total number of contexts in the system is equal to:
#   chips * smt_threads * procs_per_chip
#

chips_list = []
chips_list.append(1)
#chips_list.append(2)

procs_per_chip_list = []
#procs_per_chip_list.append(1)
#procs_per_chip_list.append(2)
#procs_per_chip_list.append(4)
procs_per_chip_list.append(8)
#procs_per_chip_list.append(16)
#procs_per_chip_list.append(32)

smt_threads_list = []
smt_threads_list.append(1)
#smt_threads_list.append(2)
#smt_threads_list.append(4)
#smt_threads_list.append(8)
#smt_threads_list.append(16)

memory_mb_list = []
#memory_mb_list.append(512)
#memory_mb_list.append(1024)
#memory_mb_list.append(2048)
#memory_mb_list.append(4096)
#memory_mb_list.append(8192)
#memory_mb_list.append(16384)
memory_mb_list.append(65536)

bandwidth_list = []
#bandwidth_list.append(400)
#bandwidth_list.append(800)
#bandwidth_list.append(1600)
#bandwidth_list.append(3200)
#bandwidth_list.append(6400)
bandwidth_list.append(10000)

random_seed = None # if you want randomness
#random_seed = 21 # if you do not want randomness

opal_config_file_list = []
opal_config_file_list.append(["default", None]) # use the default config
#opal_config_file_list.append(["iwin-4",   "issue_window/iwin-4.cfg"])

batch_size = 4

####################################################################
##  Microbenchmark Selection
##   Microbenchamrks are typically used in TM experiments.
##   To use full workloads (in non-TM systems) continue to
##   Workload Selection
####################################################################

microbench_dir = "microbenchmarks/transactional"
#microbench_dir = "workloads/splash2/scripts"

## ("<bmark>, <num_xact>, <dump_interval>"
## will run the simics file $GEMS/microbench_dir/<bmark>/<bmark>.simics
microbenchmark_list = []
microbenchmark_list.append(("isolation-test", 1, 1, "None", ""))
microbenchmark_list.append(("logging-test", 1, 1, "None", ""))
microbenchmark_list.append(("partial-rollback", 1, 1, "128array-4depth", "128 4"))
microbenchmark_list.append(("commit-action", 1, 1, "None", ""))
microbenchmark_list.append(("compensation", 1, 1, "None", ""))

microbenchmark_list.append(("btree",    100000, 100, "priv-alloc-20pct", "20"))
microbenchmark_list.append(("deque", 1024, 1, "1024ops-32bkoff", "1024 32"))
microbenchmark_list.append(("prioqueue", 8192, 1, "8192ops", ""))
microbenchmark_list.append(("sortedList", 1024, 1, "1024ops-64len", "1024 64"))
microbenchmark_list.append(("cholesky", 2, 1, "14", "tk14.O"))
microbenchmark_list.append(("raytrace", 1, 1, "teapot", "teapot"))
microbenchmark_list.append(("radiosity", 1024, 1, "1024ops", ""))
#microbenchmark_list.append(("db", 1024, 1, "1024ops", ""))
#microbenchmark_list.append(("sphinx3", 1, 1, "None", ""))
microbenchmark_list.append(("barnes", 1, 1, "512bodies", "512"))
microbenchmark_list.append(("mp3d", 1024, 1, "128mol-1024ops", "128"))
#microbenchmark_list.append(("jbb", 200, 1, "None", ""))
microbenchmark_list.append(("delaunay", 1, 1, "gen4.2-m30", "-i inputs/gen4.2 -m 30"))
microbenchmark_list.append(("genome", 1, 1, "g256-s16-n8192", "-g256 -s16 -n8192"))
microbenchmark_list.append(("vacation", 1, 1, "n8-q10-u80-r65536-t4096", "-n8 -q10 -u80 -r65536 -t4096"))
microbenchmark_list.append(("contention", 1024, 1, "config1", "1 1024 64 0 1 1 0.1 0.8 0.1 0.1 0.9 0.9 0.5"))
#microbenchmark_list.append(("contention", 1024, 1, "config2", "16 1024 64 0 1 0 0.1 0.8 0.1 0.5 0.5 0.5 0.5"))
#microbenchmark_list.append(("contention", 1024, 1, "config3", "16 1024 32 0 1 0 0.1 0.8 0.1 0.5 0.5 0.5 0.5"))
#microbenchmark_list.append(("contention", 1024, 1, "config4", "16 1024 64 0 1 0 0.1 0.8 0.1 0.1 0.5 0.1 0.5"))
#microbenchmark_list.append(("contention", 1024, 1, "config5", "16 1024 64 0 1 0 0.1 0.8 0.1 0.9 0.5 0.1 0.5"))
#microbenchmark_list.append(("contention", 1024, 1, "config6", "16 1024 64 0 1 0 0.1 0.8 0.1 0.1 0.5 0.9 0.5"))
#microbenchmark_list.append(("contention", 1024, 1, "config7", "16 1024 64 1 1 1 0.1 0.8 0.1 0.5 0.5 0.5 0.5"))
#microbenchmark_list.append(("contention", 1024, 1, "config8", "16 1024 64 1 1 1 0.1 0.8 0.1 0.5 0.5 0.5 0.1"))
#microbenchmark_list.append(("contention", 1024, 1, "config9", "16 1024 64 1 1 0 0.1 0.8 0.1 0.5 0.5 0.5 0.1"))
#microbenchmark_list.append(("contention", 1024, 1, "config10", "16 1024 64 1 1 0 0.1 0.8 0.1 0.5 0.5 0.1 0.1"))
#microbenchmark_list.append(("contention", 1024, 1, "config11", "16 1024 64 0 1 0 0.1 0.8 0.1 0.9 0.5 0.1 0.1"))
#microbenchmark_list.append(("contention", 1024, 1, "config12", "16 1024 64 0 1 0 0.1 0.8 0.1 0.1 0.5 0.9 0.1"))
#microbenchmark_list.append(("contention", 1024, 1, "config13", "16 8192 32 1 1 1 0.1 0.8 0.1 0.9 0.9 0.9 0.1"))
#microbenchmark_list.append(("contention", 1024, 1, "config14", "128 1024 32 0 1 0 0.1 0.8 0.1 0.5 0.5 0.5 0.5"))
#microbenchmark_list.append(("contention", 8192, 1, "config15", "16 8192 128 0 0 0 0.1 0.8 0.1 0.9 0.5 0.1 0.01"))
# HIGH CONTENTION
#microbenchmark_list.append(("contention", 4096, 1, "config16", "64 8192 4 0 0 0 1.0 0.0 0.0 0.75 0.8 0.8 0.01"))
#microbenchmark_list.append(("contention", 16384, 1, "config17", "64 8192 4 0 0 0 1.0 0.0 0.0 0.75 0.8 0.8 0.10"))
#microbenchmark_list.append(("contention", 32768, 1, "config18", "64 8192 4 0 0 0 1.0 0.0 0.0 0.75 0.8 0.8 0.20"))
#microbenchmark_list.append(("contention", 32768, 1, "config19", "64 8192 4 0 0 0 1.0 0.0 0.0 0.75 0.8 0.8 0.30"))
#microbenchmark_list.append(("contention", 32768, 1, "config20", "64 8192 4 0 0 0 1.0 0.0 0.0 0.75 0.8 0.8 0.40"))
#microbenchmark_list.append(("contention", 81920, 1, "config21", "64 8192 4 0 0 0 1.0 0.0 0.0 0.75 0.8 0.8 0.50"))
# LOW CONTENTION
#microbenchmark_list.append(("contention", 4096, 1, "config22", "64 8192 4 1 1 1 1.0 0.0 0.0 0.75 0.8 0.8 0.01"))
#microbenchmark_list.append(("contention", 16384, 1, "config23", "64 8192 4 1 1 1 1.0 0.0 0.0 0.75 0.8 0.8 0.10"))
#microbenchmark_list.append(("contention", 32768, 1, "config24", "64 8192 4 1 1 1 1.0 0.0 0.0 0.75 0.8 0.8 0.20"))
#microbenchmark_list.append(("contention", 32768, 1, "config25", "64 8192 4 1 1 1 1.0 0.0 0.0 0.75 0.8 0.8 0.30"))
#microbenchmark_list.append(("contention", 32768, 1, "config26", "64 8192 4 1 1 1 1.0 0.0 0.0 0.75 0.8 0.8 0.40"))
#microbenchmark_list.append(("contention", 81920, 1, "config27", "64 8192 4 1 1 1 1.0 0.0 0.0 0.75 0.8 0.8 0.50"))
#microbenchmark_list.append(("contention", 16384, 1, "config28", "64 8192 4 1 1 1 1.0 0.0 0.0 0.75 0.8 0.8 0.01"))
#microbenchmark_list.append(("contention", 16384, 1, "config29", "64 8192 4 1 1 1 1.0 0.0 0.0 0.75 0.8 0.8 0.10"))
#microbenchmark_list.append(("contention", 16384, 1, "config30", "64 8192 4 1 1 1 1.0 0.0 0.0 0.75 0.8 0.8 0.20"))
#microbenchmark_list.append(("contention", 16384, 1, "config31", "64 8192 4 1 1 1 1.0 0.0 0.0 0.75 0.8 0.8 0.30"))
#microbenchmark_list.append(("contention", 16384, 1, "config32", "64 8192 4 1 1 1 1.0 0.0 0.0 0.75 0.8 0.8 0.40"))
#microbenchmark_list.append(("contention", 16384, 1, "config33", "64 8192 4 1 1 1 1.0 0.0 0.0 0.75 0.8 0.8 0.50"))
# SERIALIZE AT END
#microbenchmark_list.append(("contention", 1024, 1, "config34", "1 8192 128 1 0 0 0.80 0.05 0.15 1.0 0.0 1.0 0.25"))
#microbenchmark_list.append(("contention", 1024, 1, "config35", "1 8192 128 1 0 0 0.80 0.05 0.15 1.0 0.0 1.0 0.50"))
#microbenchmark_list.append(("contention", 1024, 1, "config36", "1 8192 128 1 0 0 0.80 0.05 0.15 1.0 0.0 1.0 0.75"))
# SERIALIZE AT BEGINNING
#microbenchmark_list.append(("contention", 1024, 1, "config37", "1 8192 128 0 0 1 0.05 0.15 0.80 0.0 1.0 1.0 0.25"))
#microbenchmark_list.append(("contention", 1024, 1, "config38", "1 8192 128 0 0 1 0.05 0.15 0.80 0.0 1.0 1.0 0.50"))
#microbenchmark_list.append(("contention", 1024, 1, "config39", "1 8192 128 0 0 1 0.05 0.15 0.80 0.0 1.0 1.0 0.75"))
# SERIALIZE MIDWAY
#microbenchmark_list.append(("contention", 1024, 1, "config40", "1 8192 128 1 0 1 0.45 0.10 0.45 1.0 0.25 1.0 0.25"))
#microbenchmark_list.append(("contention", 1024, 1, "config41", "1 8192 128 1 0 1 0.45 0.10 0.45 1.0 0.25 1.0 0.50"))
#microbenchmark_list.append(("contention", 1024, 1, "config42", "1 8192 128 1 0 1 0.45 0.10 0.45 1.0 0.25 1.0 0.75"))
# SERIALIZE AT BEGINNING
#microbenchmark_list.append(("contention", 512, 1, "config43", "1 4 128 0 0 1 0.05 0.15 0.80 0.0 1.0 1.0 0.25"))
#microbenchmark_list.append(("contention", 512, 1, "config44", "1 4 128 0 0 1 0.05 0.15 0.80 0.0 1.0 1.0 0.50"))
#microbenchmark_list.append(("contention", 512, 1, "config45", "1 4 128 0 0 1 0.05 0.15 0.80 0.0 1.0 1.0 0.75"))
# SERIALIZE MIDWAY
#microbenchmark_list.append(("contention", 512, 1, "config46", "1 4 128 1 0 1 0.45 0.10 0.45 1.0 0.25 1.0 0.25"))
#microbenchmark_list.append(("contention", 512, 1, "config47", "1 4 128 1 0 1 0.45 0.10 0.45 1.0 0.25 1.0 0.50"))
#microbenchmark_list.append(("contention", 512, 1, "config48", "1 4 128 1 0 1 0.45 0.10 0.45 1.0 0.25 1.0 0.75"))
# SERIALIZE AT END
#microbenchmark_list.append(("contention", 512, 1, "config49", "1 4 128 1 0 0 0.80 0.05 0.15 1.0 0.0 1.0 0.25"))
#microbenchmark_list.append(("contention", 512, 1, "config50", "1 4 128 1 0 0 0.80 0.05 0.15 1.0 0.0 1.0 0.50"))
#microbenchmark_list.append(("contention", 512, 1, "config51", "1 4 128 1 0 0 0.80 0.05 0.15 1.0 0.0 1.0 0.75"))
# PREFETCHING
#microbenchmark_list.append(("contention", 512, 1, "config52", "1 8192 128 1 0 1 0.45 0.10 0.45 1.0 0.25 1.0 0.25"))
# VIRTUALIZATION TESTING
#microbenchmark_list.append(("conswtch", 8, 1, "None", ""))
#microbenchmark_list.append(("paging", 8, 1, "None", ""))


######################################################################################################
## Workload Selection
##
##  Standard (non-transactional) workloads can be specified in one of two ways,
##  depending on whether or not you want to use the automated checkpoint generation scripts
##
##  You should NOT have the same workload name in both the autogen and custom lists, as the condor file
##  for the autogen runs will be overwritten.  If you really do want to run both autogen and custom
##  checkpoints for the same workload, be sure to give them different names
######################################################################################################

# autogen_workload_list
#   For each workload in this list, a checkpoint will be created for each processor/memory combination
#   (specified by processor_list, smt_list, procs_per_chip, and memory_mb_list above)
#   Note: workloads will only be created once: if a checkpoint already exists for a given configuration, it will be reused
#   See the README in the checkpoints directory for more information on this process
#
#   workload name | # of transactions | dump interval | cache warmup file
autogen_workload_list = []
#autogen_workload_list.append("jbb",    10000, 1000, "yes"))
#autogen_workload_list.append("oltp",   1000,   10,   "yes"))  # oltp is not yet supported by the automatic checkpoint generator
#autogen_workload_list.append("apache", 1000,   10,   "yes"))
#autogen_workload_list.append("zeus",   1000,   10,   "yes"))

# if you use these workloads, the values in memory_list are ignored
#
#  These checkpoints are expected to be named as (<checkpoint prefix>-%dp.check) % num_processors
#  Correspondingly, if a cache warmup file is to be used, it should be named (<checkpoint prefix>-%dp.caches.gz) % num_processors
#  and should be placed in the same directory as (checkpoint location)
#
#  workload name | checkpoint location (absolute) | checkpoint prefix | # transactions | dump interval | cache warmup file
custom_workload_list = []
#custom_workload_list.append("apache", "%s/simics/checkpoints-u3/apache/"%gemsroot, "apache_warm", 1000, 10, "yes")
#custom_workload_list.append("jbb", "%s/simics/checkpoints-u3/jbb/"%gemsroot, "jbb_warm", 100000, 10, "yes")
#custom_workload_list.append("oltp", "%s/simics/checkpoints-u3/oltp/"%gemsroot, "oltp_warm", 1000, 10, "yes")
#custom_workload_list.append("zeus", "%s/simics/checkpoints-u3/zeus/"%gemsroot, "zeus_warm", 1000, 10, "yes")

########################
##  Workload options
########################

checkpoint_at_end = 'no'
generate_trace = 'no'

### END CONFIGURATION



