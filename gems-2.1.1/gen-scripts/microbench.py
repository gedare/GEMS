#
# This file has been modified by Kevin Moore and Dan Nussbaum of the
# Scalable Systems Research Group at Sun Microsystems Laboratories
# (http://research.sun.com/scalable/) to support the Adaptive
# Transactional Memory Test Platform (ATMTP).  For information about
# ATMTP, see the GEMS website: http://www.cs.wisc.edu/gems/.
#
# Please send email to atmtp-interest@sun.com with feedback,
# questions, or to request future announcements about ATMTP.
#
# ----------------------------------------------------------------------
#
# File modification date: 2008-02-23
#
# ----------------------------------------------------------------------
#  
# ATMTP is distributed as part of the GEMS software toolset and is
# available for use and modification under the terms of version 2 of
# the GNU General Public License.  The GNU General Public License is
# contained in the file $GEMS/LICENSE.
#  
# Multifacet GEMS is free software; you can redistribute it and/or
# modify it under the terms of version 2 of the GNU General Public
# License as published by the Free Software Foundation.
#
# Multifacet GEMS is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with the Multifacet GEMS; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307,
# USA
#
# ----------------------------------------------------------------------

import time, workloads, sim_commands, string, sys, getpass, os
import mod_xterm_console_commands
from cli import *
import mfacet

def setup_simics():
    ###### set simics parameters
  # enable/disable STCs here
  mfacet.run_sim_command("istc-disable")
  mfacet.run_sim_command("dstc-disable")
  mfacet.run_sim_command("stc-status")
  
  # disable breakpoint before we sync all processors
  mfacet.run_sim_command('magic-break-disable')
  
  # always use 1 for better parallelism
  old_switch_time = conf.sim.cpu_switch_time
  print "Old cpu_switch_time: %d" % conf.sim.cpu_switch_time
  mfacet.run_sim_command("cpu-switch-time")
  conf.sim.cpu_switch_time = 1
  if (SIM_number_processors() == 1):
    delay_time = 1
  else:
    delay_time = old_switch_time
  mfacet.run_sim_command("c %d"%(delay_time)) # sync CPUs after changing switch time
  mfacet.run_sim_command("cpu-switch-time")
  print "New cpu_switch_time: %d" % conf.sim.cpu_switch_time
  
  # enable breakpoint
  mfacet.run_sim_command('magic-break-enable')

  ###### Conserve memory by limiting how much memory each simics image object can use
  #mfacet.limit_all_images(256)

  # enable IFETCH, i-fetch line size is controlled in checkpoint files
  print "Old instruction-fetch-mode: "
  mfacet.run_sim_command("instruction-fetch-mode")
  mfacet.run_sim_command("instruction-fetch-mode instruction-fetch-trace")
  print "New instruction-fetch-mode: "
  mfacet.run_sim_command("instruction-fetch-mode")

def start_ruby(debug=0):
  env_dict = workloads.prepare_env_dictionary(simics = 1)
  condor_cluster = int(workloads.get_var(env_dict, "CONDORCLUSTER"))
  condor_process = int(workloads.get_var(env_dict, "CONDORPROCESS"))
  random_seed = workloads.get_var(env_dict, "RANDOM_SEED")
  if random_seed == None:
    import whrandom
    #random_seed = whrandom.randint(0,100000)
    random_seed = (condor_cluster << 16) | condor_process
  else:
    random_seed = int(workloads.get_var(env_dict, "RANDOM_SEED"))

  smt_threads = int(workloads.get_var(env_dict, "SMT_THREADS"))
  processors_per_chip = int(workloads.get_var(env_dict, "PROCS_PER_CHIP"))
  read_write_filter = workloads.get_var(env_dict, "READ_WRITE_FILTER")
  virtual_signature = workloads.get_var(env_dict, "VIRTUAL_READ_WRITE_FILTER")
  summary_signature = workloads.get_var(env_dict, "SUMMARY_READ_WRITE_FILTER")
  enable_tourmaline = int(workloads.get_var(env_dict, "ENABLE_TOURMALINE"))
  xact_lazy         = int(workloads.get_var(env_dict, "XACT_LAZY_VM"))
  xact_eager        = int(workloads.get_var(env_dict, "XACT_EAGER_CD"))
  xact_visualizer   = int(workloads.get_var(env_dict, "XACT_VISUALIZER"))
  xact_commit_token = int(workloads.get_var(env_dict, "XACT_COMMIT_TOKEN_LATENCY"))
  xact_no_backoff   = int(workloads.get_var(env_dict, "XACT_NO_BACKOFF"))
  enable_magic_waiting = int(workloads.get_var(env_dict, "ENABLE_MAGIC_WAITING"))
  xact_enable_virtualization_logtm_se = int(workloads.get_var(env_dict, "XACT_ENABLE_VIRTUALIZATION_LOGTM_SE"))
  enable_watchpoint = int(workloads.get_var(env_dict, "ENABLE_WATCHPOINT"))
  xact_cr              = workloads.get_var(env_dict, "XACT_CONFLICT_RES")
  xact_log_buffer_size = int(workloads.get_var(env_dict, "XACT_LOG_BUFFER_SIZE"))
  xact_store_predictor_entries = int(workloads.get_var(env_dict, "XACT_STORE_PREDICTOR_ENTRIES"))
  xact_store_predictor_history = int(workloads.get_var(env_dict, "XACT_STORE_PREDICTOR_HISTORY"))
  xact_store_predictor_threshold = int(workloads.get_var(env_dict, "XACT_STORE_PREDICTOR_THRESHOLD"))
  xact_first_access_cost = int(workloads.get_var(env_dict, "XACT_FIRST_ACCESS_COST"))
  xact_first_page_access_cost = int(workloads.get_var(env_dict, "XACT_FIRST_PAGE_ACCESS_COST"))
  
  #random_seed = 0
  print "condor_cluster: %d" % condor_cluster
  print "condor_process: %d" % condor_process
  print "random_seed: %d" % random_seed

  ###### set simics parameters
  # enable/disable STCs here
  mfacet.run_sim_command("istc-disable")
  mfacet.run_sim_command("dstc-disable")
  mfacet.run_sim_command("stc-status")
  
  # disable breakpoint before we sync all processors
  mfacet.run_sim_command('magic-break-disable')
  
  # always use 1 for better parallelism
  old_switch_time = conf.sim.cpu_switch_time
  print "Old cpu_switch_time: %d" % conf.sim.cpu_switch_time
  mfacet.run_sim_command("cpu-switch-time")
  conf.sim.cpu_switch_time = 1
  if (SIM_number_processors() == 1):
    delay_time = 1
  else:
    delay_time = old_switch_time
  mfacet.run_sim_command("c %d"%(delay_time)) # sync CPUs after changing switch time
  mfacet.run_sim_command("cpu-switch-time")
  print "New cpu_switch_time: %d" % conf.sim.cpu_switch_time
  
  # enable breakpoint
  mfacet.run_sim_command('magic-break-enable')

  ###### Conserve memory by limiting how much memory each simics image object can use
  #mfacet.limit_all_images(256)

  # enable IFETCH, i-fetch line size is controlled in checkpoint files
  print "Old instruction-fetch-mode: "
  mfacet.run_sim_command("instruction-fetch-mode")
  mfacet.run_sim_command("instruction-fetch-mode instruction-fetch-trace")
  print "New instruction-fetch-mode: "
  mfacet.run_sim_command("instruction-fetch-mode")

  ###### Load Ruby and set parameters
  mfacet.run_sim_command("load-module ruby")
  mfacet.run_sim_command("ruby0.setparam_str REMOVE_SINGLE_CYCLE_DCACHE_FAST_PATH true")
  #mfacet.run_sim_command("ruby0.setparam_str PROFILE_EXCEPTIONS true")
  mfacet.run_sim_command("ruby0.setparam_str PROFILE_XACT false")
  mfacet.run_sim_command("ruby0.setparam_str PROFILE_NONXACT false")
  mfacet.run_sim_command("ruby0.setparam_str XACT_MEMORY true")
  mfacet.run_sim_command("ruby0.setparam g_DEADLOCK_THRESHOLD 20000000")
  mfacet.run_sim_command("ruby0.setparam g_MEMORY_SIZE_BYTES 8589934592")
  mfacet.run_sim_command("ruby0.setparam g_RANDOM_SEED %d" % random_seed)
  mfacet.run_sim_command("ruby0.setparam g_PAGE_SIZE_BYTES %d" % 8192)

  mfacet.run_sim_command("ruby0.setparam_str XACT_DEBUG false")
  mfacet.run_sim_command("ruby0.setparam XACT_DEBUG_LEVEL %d" % 2)
  # Need to set this for CMPs!
  mfacet.run_sim_command("ruby0.setparam g_PROCS_PER_CHIP %d" % processors_per_chip)
  # We don't need to set g_NUM_PROCESSORS explicitly, but need to set # of SMT threads if we want threads per proc> 1
  mfacet.run_sim_command("ruby0.setparam g_NUM_SMT_THREADS %d" % smt_threads)
  # Number of outstanding requests from each sequencer object (default is 16)
  mfacet.run_sim_command("ruby0.setparam g_SEQUENCER_OUTSTANDING_REQUESTS %d" % (16*smt_threads))

  # Comment this out for production/faster runs
  mfacet.run_sim_command("ruby0.setparam_str XACT_ISOLATION_CHECK true")

  # set multipliers here
  mfacet.run_sim_command("ruby0.setparam SIMICS_RUBY_MULTIPLIER %d" % 1)
  mfacet.run_sim_command("ruby0.setparam OPAL_RUBY_MULTIPLIER %d" % 1)
  
  # 32 kB 4-way L1 cache
  mfacet.run_sim_command("ruby0.setparam L1_CACHE_ASSOC %d" % 4)
  mfacet.run_sim_command("ruby0.setparam L1_CACHE_NUM_SETS_BITS %d" % 7)
  mfacet.run_sim_command("ruby0.setparam SEQUENCER_TO_CONTROLLER_LATENCY %d" % 1) # L1 hit time

  # 8 MB 8-way L2 cache
  mfacet.run_sim_command("ruby0.setparam L2_CACHE_ASSOC %d" % 8)
  mfacet.run_sim_command("ruby0.setparam L2_CACHE_NUM_SETS_BITS %d" % 14)
  mfacet.run_sim_command("ruby0.setparam L2_RESPONSE_LATENCY %d" % 20)
  mfacet.run_sim_command("ruby0.setparam L2_TAG_LATENCY %d" % 6)
  mfacet.run_sim_command("ruby0.setparam L2_REQUEST_LATENCY %d" % 15)

  # Main memory
  mfacet.run_sim_command("ruby0.setparam MEMORY_RESPONSE_LATENCY_MINUS_2 %d" % 448)
  
  # interconnection network parameters
  mfacet.run_sim_command("ruby0.setparam NETWORK_LINK_LATENCY %d" % 14)

  # network topology
  # use PT_TO_PT for CMPs
  # CMPs use Dancehall network topology
  mfacet.run_sim_command("ruby0.setparam_str g_NETWORK_TOPOLOGY FILE_SPECIFIED")
  #mfacet.run_sim_command("ruby0.setparam_str g_NETWORK_TOPOLOGY PT_TO_PT")
  mfacet.run_sim_command("ruby0.setparam RECYCLE_LATENCY %d" % 1)
  mfacet.run_sim_command("ruby0.setparam NUMBER_OF_VIRTUAL_NETWORKS %d" % 5)
  #mfacet.run_sim_command("ruby0.setparam_str g_PRINT_TOPOLOGY true")
  if (processors_per_chip == 16):
    mfacet.run_sim_command("ruby0.setparam g_NUM_MEMORIES %d" % 8)
  elif (processors_per_chip == 32):  
    mfacet.run_sim_command("ruby0.setparam g_NUM_MEMORIES %d" % 16)
  else:
    mfacet.run_sim_command("ruby0.setparam g_NUM_MEMORIES %d" % 8)
  mfacet.run_sim_command("ruby0.setparam L2CACHE_TRANSITIONS_PER_RUBY_CYCLE %d" % 1000)
  mfacet.run_sim_command("ruby0.setparam DIRECTORY_TRANSITIONS_PER_RUBY_CYCLE %d" % 1000)
  #mfacet.run_sim_command("ruby0.setparam_str g_NETWORK_TOPOLOGY HIERARCHICAL_SWITCH")

  mfacet.run_sim_command("ruby0.setparam XACT_LOG_BUFFER_SIZE %d" % xact_log_buffer_size)
  ## Read/Write Filter options
  # Physical signatures
  filter_type = read_write_filter.split('_')
  if(filter_type[0] == "Perfect"):
    mfacet.run_sim_command("ruby0.setparam_str PERFECT_FILTER true")
  else:
    mfacet.run_sim_command("ruby0.setparam_str PERFECT_FILTER false")
  # set read/write filter type
  mfacet.run_sim_command("ruby0.setparam_str READ_WRITE_FILTER %s" % read_write_filter)
  # Virtual signatures
  filter_type = virtual_signature.split('_')
  if(filter_type[0] == "Perfect"):
    mfacet.run_sim_command("ruby0.setparam_str PERFECT_VIRTUAL_FILTER true")
  else:
    mfacet.run_sim_command("ruby0.setparam_str PERFECT_VIRTUAL_FILTER false")
  # set read/write filter type
  mfacet.run_sim_command("ruby0.setparam_str VIRTUAL_READ_WRITE_FILTER %s" % virtual_signature)
  # Summary signatures
  filter_type = summary_signature.split('_')
  if(filter_type[0] == "Perfect"):
    mfacet.run_sim_command("ruby0.setparam_str PERFECT_SUMMARY_FILTER true")
  else:
    mfacet.run_sim_command("ruby0.setparam_str PERFECT_SUMMARY_FILTER false")
  # set read/write filter type
  mfacet.run_sim_command("ruby0.setparam_str SUMMARY_READ_WRITE_FILTER %s" % summary_signature)
  
  mfacet.run_sim_command("ruby0.setparam_str XACT_CONFLICT_RES %s" % xact_cr)
  mfacet.run_sim_command("ruby0.setparam XACT_STORE_PREDICTOR_ENTRIES %d" % xact_store_predictor_entries)
  mfacet.run_sim_command("ruby0.setparam XACT_STORE_PREDICTOR_HISTORY %d" % xact_store_predictor_history)
  mfacet.run_sim_command("ruby0.setparam XACT_STORE_PREDICTOR_THRESHOLD %d" % xact_store_predictor_threshold)
  if (xact_visualizer):
    mfacet.run_sim_command("ruby0.setparam_str XACT_VISUALIZER true")
  if (xact_lazy):
    mfacet.run_sim_command("ruby0.setparam_str XACT_LAZY_VM true")
  if (not xact_eager):
    mfacet.run_sim_command("ruby0.setparam_str XACT_EAGER_CD false")
  #enable_tourmaline = 1   
  if (enable_tourmaline):
    mfacet.run_sim_command("ruby0.setparam_str XACT_ENABLE_TOURMALINE true")
  if (xact_commit_token > 0):
    mfacet.run_sim_command("ruby0.setparam XACT_COMMIT_TOKEN_LATENCY %d" % xact_commit_token)
  if (xact_no_backoff):
    mfacet.run_sim_command("ruby0.setparam_str XACT_NO_BACKOFF true")
  if (enable_magic_waiting):
    mfacet.run_sim_command("ruby0.setparam_str ENABLE_MAGIC_WAITING true")
  if (xact_enable_virtualization_logtm_se):
    mfacet.run_sim_command("ruby0.setparam_str XACT_ENABLE_VIRTUALIZATION_LOGTM_SE true")
  if (enable_watchpoint):
    mfacet.run_sim_command("ruby0.setparam_str ENABLE_WATCHPOINT true")
  if (xact_first_access_cost > 0):  
    mfacet.run_sim_command("ruby0.setparam XACT_FIRST_ACCESS_COST %d" % xact_first_access_cost)
  if (xact_first_page_access_cost > 0):  
    mfacet.run_sim_command("ruby0.setparam XACT_FIRST_PAGE_ACCESS_COST %d" % xact_first_page_access_cost)
              
  mfacet.run_sim_command("ruby0.init")
  #debug = 2820483
  if debug > 0:
    #mfacet.run_sim_command('ruby0.debug-verb med')
    mfacet.run_sim_command('ruby0.debug-verb high')
    mfacet.run_sim_command('ruby0.debug-filter lsNqST')
    mfacet.run_sim_command('ruby0.debug-start-time "%d"' % debug)
    mfacet.run_sim_command('ruby0.debug-output-file /scratch/ruby')


def start_ruby_small_cache():
  env_dict = workloads.prepare_env_dictionary(simics = 1)
  random_seed = workloads.get_var(env_dict, "RANDOM_SEED")
  #condor_cluster = int(workloads.get_var(env_dict, "CONDORCLUSTER"))
  #condor_process = int(workloads.get_var(env_dict, "CONDORPROCESS"))
  #if random_seed == None:
  #  random_seed = (condor_cluster << 16) | condor_process
  #else:
  #  random_seed = int(random_seed)
  random_seed = 0
  
  ###### set simics parameters

  # enable/disable STCs here
  mfacet.run_sim_command("istc-disable")
  mfacet.run_sim_command("dstc-disable")
  mfacet.run_sim_command("stc-status")
  
  # disable breakpoint before we sync all processors
  mfacet.run_sim_command('magic-break-disable')
  
  # always use 1 for better parallelism
  old_switch_time = conf.sim.cpu_switch_time
  print "Old cpu_switch_time: %d" % conf.sim.cpu_switch_time
  conf.sim.cpu_switch_time = 1
  #if (SIM_number_processors() == 1):
  #  delay_time = 1
  #else:
  #  delay_time = old_switch_time
  #mfacet.run_sim_command("c %d"%(delay_time)) # sync CPUs after changing switch time
  #print "New cpu_switch_time: %d" % conf.sim.cpu_switch_time
  
  # enable breakpoint
  mfacet.run_sim_command('magic-break-enable')

  ###### Conserve memory by limiting how much memory each simics image object can use
  #mfacet.limit_all_images(256)
  
  # enable IFETCH, i-fetch line size is controlled in checkpoint files
  #mfacet.run_sim_command("instruction-profile-mode instruction-cache-access-trace")
  #mfacet.run_sim_command("instruction-profile-mode")

  #import whrandom
  #random_seed = whrandom.randint(0,100000)
  
  ###### Load Ruby and set parameters
  mfacet.run_sim_command("load-module ruby")
  #mfacet.run_sim_command("ruby0.set-seed(%d)" % random_seed)
  #mfacet.run_sim_command("ruby0.fix-bandwidth(%d)" % 64000)
  #mfacet.run_sim_command("ruby0.setparam PROCS_PER_CHIP %d" % 2)
  #mfacet.run_sim_command("ruby0.setparam_str g_NETWORK_TOPOLOGY CMP_DIRECT_SWITCH")
  #mfacet.run_sim_command("ruby0.setparam MEMORY_LATENCY 1")
  #mfacet.run_sim_command("ruby0.setparam NETWORK_LINK_LATENCY 1")
  #mfacet.run_sim_command("ruby0.setparam DIRECTORY_LATENCY 1")
  mfacet.run_sim_command("ruby0.setparam g_DEADLOCK_THRESHOLD 2000000")
  mfacet.run_sim_command("ruby0.setparam g_MEMORY_SIZE_BYTES 8589934592")
  mfacet.run_sim_command("ruby0.setparam_str REMOVE_SINGLE_CYCLE_DCACHE_FAST_PATH true")
  mfacet.run_sim_command("ruby0.setparam g_RANDOM_SEED %d" % random_seed)
  #mfacet.run_sim_command("ruby0.setparam RETRY_LATENCY %d" % 5)

  mfacet.run_sim_command("ruby0.setparam L1_CACHE_ASSOC %d" % 1)
  mfacet.run_sim_command("ruby0.setparam L1_CACHE_NUM_SETS_BITS %d" % 4)
  mfacet.run_sim_command("ruby0.setparam L2_CACHE_ASSOC %d" % 1)
  mfacet.run_sim_command("ruby0.setparam L2_CACHE_NUM_SETS_BITS %d" % 6)
  
  mfacet.run_sim_command("ruby0.init")

def start_opal(transactions, dump_interval, opal_outfile):
  env_dict = workloads.prepare_env_dictionary(simics = 1)
  mfacet.run_sim_command("load-module opal")

  # set the SMT thread count
  smt_threads = int(workloads.get_var(env_dict, "SMT_THREADS"))
  mfacet.run_sim_command("opal0.setparam CONFIG_LOGICAL_PER_PHY_PROC %d" % smt_threads)
  # Register file should scale with SMT thread count
  mfacet.run_sim_command("opal0.setparam CONFIG_IREG_PHYSICAL %d" % (160*smt_threads+128))
  mfacet.run_sim_command("opal0.setparam CONFIG_FPREG_PHYSICAL %d" % (64*smt_threads+256))
  mfacet.run_sim_command("opal0.setparam CONFIG_CCREG_PHYSICAL %d" % (5*smt_threads+128))
  mfacet.run_sim_command("opal0.setparam CONFIG_NUM_CONTROL_SETS %d" % (64*smt_threads))
  # do Transactional loads/stores at retirment, not speculatively
  mfacet.run_sim_command("opal0.setparam RETIREMENT_CACHE_ACCESS %d" % 1)

  # ROB, scheduling window sizes
  mfacet.run_sim_command("opal0.setparam IWINDOW_ROB_SIZE %d" % 128)
  mfacet.run_sim_command("opal0.setparam IWINDOW_WIN_SIZE %d" % 64)
  mfacet.run_sim_command("opal0.setparam RESERVED_ROB_ENTRIES %d" % 4)
  
  # Use HW or SW log
  mfacet.run_sim_command("opal0.setparam_str XACT_USE_SWLOG true")

  mfacet.run_sim_command("opal0.setparam XACT_MAX_DEPTH %d" % string.atoi(workloads.get_var(env_dict, "MAX_DEPTH")));

  # Comment this out for production/faster runs
  #mfacet.run_sim_command("opal0.setparam_str XACT_ISOLATION_CHECK true")
  
  mfacet.run_sim_command('opal0.init')
  print "starting opal"
  mfacet.run_sim_command('opal0.sim-start %s' % opal_outfile)
  # now setup and run
  mfacet.setup_run_for_n_transactions(transactions, dump_interval)
  mfacet.run_sim_command("opal0.sim-step 100000000000")
  
def run_long_xact_test():
  environ["PROCESSORS"] = "4"
  run_count_test("average_trans", 100)
  #mfacet.run_sim_command("read-configuration /scratch/kmoore/long_xact_test.check")
  # load ruby and opal
  #start_ruby()
  #setup_run_for_n_transactions(100000, 100)
  #mfacet.run_sim_command("c")


def run_long_lock_test():
  environ["PROCESSORS"] = "4"
  run_count_test("average_locks", 100)
  #mfacet.run_sim_command("read-configuration /scratch/kmoore/long_xact_test.check")
  # load ruby and opal
  #start_ruby()
  #setup_run_for_n_transactions(100000, 100)
  #mfacet.run_sim_command("c")

def run_xact_test():
  environ["PROCESSORS"] = "4"
  run_count_test("count_trans", 1000)

def run_lock_test():
  environ["PROCESSORS"] = "4"
  run_count_test("count_locks", 1000)

def run_abort_test():
  environ["PROCESSORS"] = "1"
  run_count_test("abort_test", 10)
  start_ruby()
  mfacet.setup_run_for_n_transactions(100000, 100)
  mfacet.run_sim_command("c")


def run_abort_test3():
  os.environ["PROCESSORS"] = "2"
  run_count_test("abort_test3", 50)
  start_ruby(debug=0)
  mfacet.setup_run_for_n_transactions(10, 1)
  mfacet.run_sim_command("c")

def run_abort_test4():
  os.environ["PROCESSORS"] = "2"
  run_count_test("abort_test4", 50)
  start_ruby(debug=0)
  mfacet.setup_run_for_n_transactions(10, 1)
  mfacet.run_sim_command("c")

def run_malloc_test():
  os.environ["PROCESSORS"] = "4"
  os.environ["SIMICS_EXTRA_LIB"] = "./modules"
  run_count_test("malloc_test", 50)
  start_ruby(debug=0)
  mfacet.setup_run_for_n_transactions(10000, 1)
  mfacet.run_sim_command("c")

def run_count_test(exe_name, iterations):
  os.environ["WORKLOAD"] = exe_name
  os.environ["MODULE"] = "MOESI_xact_hammer"
  os.environ["BANDWIDTH"] = "10000"
  os.environ["CHECKPOINT"] = "golden"
  os.environ["RESULTS_DIR"] = "."
  os.environ["TRANSACTIONS"] = "0"

  if exe_name == "count_locks":
    print "USING LOCK-BASED SYNCHRONIZATION"
  else:
    print "USING TRANSACTIONAL MEMORY"
  
  ###### Read simulation parameters
  env_dict = workloads.prepare_env_dictionary(simics = 1)
  processors = int(workloads.get_var(env_dict, "PROCESSORS"))
  #workloads.print_all_variables(env_dict)
  
  checkpoint = "golden"
  ruby = 1 # true

  opal = None
  
  ###### print out local host name to help with troubleshooting
  print "Local host name:", string.strip(os.popen("hostname").readline())
  
  ###### init simics with a checkpoint
  if checkpoint[0] == '/':
    mfacet.run_sim_command('read-configuration "%s"' % (checkpoint))
  else:
    mfacet.run_sim_command('read-configuration "../../checkpoints-u3/%s-%dp.check"' % (checkpoint, processors));

  #mfacet.run_sim_command("c 100000")
  mfacet.run_sim_command('magic-break-enable')
  
  lines = ["bash\n",
           "mount /host\n",
           "cp /host/s/gcc-3.3/sun4x_58/lib/libstdc++.so.5 .\n",
           "cp /host/s/gcc-3.3/sun4x_58/lib/libgcc_s.so.1 .\n",
           #"cp /host/p/multifacet/projects/xact_memory/multifacet/microbenchmarks/transactional/dining_philosophers/dphil .\n",
           #"cp /host/p/multifacet/projects/xact_memory/multifacet/microbenchmarks/transactional/linked-list/list_bench .\n",
           #"cp /host/p/multifacet/projects/xact_memory/multifacet/microbenchmarks/transactional/shared-counter/count .\n",
           "cp /host/p/multifacet/projects/java_locks/nuca_locks/src/libparmacs_locks.so .\n",
           "cp /host/p/multifacet/projects/java_locks/nuca_locks/nuca_bench/%s .\n" % exe_name,
           "cp /host/p/multifacet/projects/java_locks/nuca_locks/nuca_bench/WILDFIRE.on .\n",
           "export LD_LIBRARY_PATH=.\n",
           "source WILDFIRE.on\n",
           ]

  proc_no = [0, 1, 4, 5, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19]
  for j in range(processors):
    if(j != 0):
      print "j=%d, num_p=%d" % (j, processors)
      lines.append("psrset -c\n")
      print "psrset -c\n"
      lines.append("psrset -a %d %d\n" % (j, proc_no[j]))
      print "psrset -a %d %d\n" % (j, proc_no[j])

  #for i in lines:
  #insert_cmd(SIM_get_object("con0"), i)
  #  break_cmd(SIM_get_object("con0"), "bash-2.05#")
  #  mfacet.run_sim_command("c")
  #  unbreak_cmd(SIM_get_object("con0"), "bash-2.05#")
  lines.append("./%s %d %d\n" % (exe_name, processors, iterations))
  mfacet.console_commands(lines, "bash-2.05#")

  #insert_cmd(SIM_get_object("con0"), "ls -l\n")
  #insert_cmd(SIM_get_object("con0"), "ls -l\n")
  #insert_cmd(SIM_get_object("con0"), "./dphil 10\n")
  #insert_cmd(SIM_get_object("con0"), "./list_bench %d\n" % processors)
  #insert_cmd(SIM_get_object("con0"), "./%s %d %d; magic_call break\n" % (exe_name, processors, iterations))
  #insert_cmd(SIM_get_object("con0"), "./%s %d %d\n" % (exe_name, processors, iterations))
  mfacet.run_sim_command("c") # run up to magic_break
  mfacet.run_sim_command("c 1") # run up to magic_break
