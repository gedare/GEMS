# checkpoint name/path pairs
all_checkpoints = [
    ("/simics/checkpoints-u3/barnes/barnes-512",          "barnes_512",        [1, 2, 4, 8, 16, 32]),
    ("/simics/checkpoints-u3/barnes/barnes-16k",          "barnes_16k",        [1, 2, 4, 8, 16, 32]),
    ("/simics/checkpoints-u3/barnes/barnes-64k",          "barnes_64k",        [1, 2, 4, 8, 16, 32]),
    ("/simics/checkpoints-u3/barnes/barnes-128k",         "barnes_128k",       [1, 2, 4, 8, 16, 32]),
    ("/simics/checkpoints-u3/ocean/ocean-66",             "ocean_66",          [1, 2, 4, 8, 16, 32]),
    ("/simics/checkpoints-u3/ocean/ocean-258",            "ocean_258",         [1, 2, 4, 8, 16, 32]),
    ("/simics/checkpoints-u3/ocean/ocean-514",            "ocean_514",         [1, 2, 4, 8, 16, 32]),
    ("/simics/checkpoints-u3/ocean/ocean-1026",           "ocean_1026",        [1, 2, 4, 8, 16, 32]),
    
    ("/simics/checkpoints-u3/jbb/jbb",                    "jbb",               [1, 2, 4, 8, 16]),

    ("/simics/checkpoints-u3/jbb/jbb_warm",               "jbb",               [1, 2, 4, 8, 16]),
    ("/simics/checkpoints-u3/oltp/oltp_warm",             "oltp",              [1, 2, 4, 8, 16]),
    ("/simics/checkpoints-u3/apache/apache_warm",         "apache",            [1, 2, 4, 8, 16]),
    ("/simics/checkpoints-u3/zeus/zeus_warm",             "zeus",              [1, 2, 4, 8, 16]),
    ]

regress_list = [
    ("/simics/checkpoints-u3/jbb/jbb_warm",               "jbb",             100,  1,  500, None),
    ("/simics/checkpoints-u3/oltp/oltp_warm",             "oltp",              2,  1,  500, None),
    ("/simics/checkpoints-u3/apache/apache_warm",         "apache",            8,  1,  500, None),
    ("/simics/checkpoints-u3/zeus/zeus_warm",             "zeus",              8,  1,  500, None),
    ]

##   checkpoint path     |  workload name | trans | dump_int | memory(MB) | cache warmup file
test_runs = [
    ("jbb/jbb_warm",               "jbb",             100,  1,  500, None),
    ("oltp/oltp_warm",             "oltp",              2,  1,  500, None),
    ("apache/apache_warm",         "apache",            8,  1,  500, None),
    ("zeus/zeus_warm",             "zeus",              8,  1,  500, None),
    ]

short_runs = [
    ("barnes/barnes-512",          "barnes_512",        1,  1,  500, None),
    ("ocean/ocean-66",             "ocean_66",          1,  1,  500, None),
    ("jbb/jbb_warm",               "jbb",            1000,100,  500, None),
    ("ecperf/ecperf",              "ecperf",            0,  1,  500, None),
    ("SPEC2K/gcc",                 "gcc",               0,  1,  500, None),
    ("SPEC2K/equake",              "equake",            0,  1,  500, None), 
    ("oltp/oltp_warm",             "oltp",              5,  1,  500, None),
    ("apache/apache_warm",         "apache",           50, 1,  500, None),
    ("zeus/zeus_warm",             "zeus",             50, 1,  500, None),
    ]

half_runs = [
    ("barnes/barnes-16k",          "barnes_16k",        1,   1, 500, None),
    ("ocean/ocean-514",            "ocean_514",         1,   1, 500, None),
    ("jbb/jbb_warm",               "jbb",          100000,1000,1000, None),
    ("ecperf/ecperf",              "ecperf",            4,   1,1000, None),
    ("oltp/oltp_warm",             "oltp",           1000,  10,1000, None),
    ("apache/apache_warm",         "apache",        10000, 100,1000, None),
    ("zeus/zeus_warm",             "zeus",          10000, 100,1000, None),
    ]

full_runs = [
    ("barnes/barnes-64k",          "barnes_64k",        1,     1,  500, None),
    ("ocean/ocean-1026",           "ocean_1026",        1,     1,  500, None),
    ("jbb/jbb_warm",               "jbb",          200000,  2000, 1000, None),
    ("ecperf/ecperf",              "ecperf",           10,     1, 1000, None),
    ("oltp/oltp_warm",             "oltp",           2000,    20, 1000, None),
    ("apache/apache_warm",         "apache",        20000,   200, 1000, None),
    ("zeus/zeus_warm",             "zeus",          20000,   200, 1000, None),
    ]

# all comm. checkpoint sets use the "_warm" checkpoints
# these checkpoints will also load the warmup cache files (.caches.gz)
warm_runs = [
    ("jbb/jbb_warm",             "jbb",          100000, 1000, 1900, "yes"),
    ("oltp/oltp_warm",           "oltp",           1000,   10, 1900, "yes"),
    ("apache/apache_warm",       "apache",         1000,   10, 1900, "yes"),
    ("zeus/zeus_warm",           "zeus",           1000,   10, 1900, "yes"),
    ]

jbb_nuca_runs = [
  ("jbb-nuca/jbb-HBOLocks",                 "jbb_HBO",        1000,  1,  500,  ""),
  ("jbb-nuca/jbb-TATASLocks",               "jbb_TATAS",      1000,  1,  500,  "")
  ]

##   checkpoint path     |  workload name | trans | dump_int | memory(MB) | cache warmup file | mbench_arg_prefix | mbench_arg_string
transactional_runs = [
  ("deque", "deque", 2000, 1, 500, "", "2000ops-32bkoff", "2000 32"),
  ("btree", "btree", 1000, 1, 500, "", "priv-alloc-20pct", "20"),
  ("prioqueue", "prioqueue", 8192, 1, 500, "", "8192ops", ""),
  ("sortedList", "sortedList", 500, 1, 500, "", "500ops-64len", "500 64"),
  ("isolation-test", "isolation-test", 1, 1, 500, "", "", ""),
  ("logging-test", "logging-test", 1, 1, 500, "", "", ""),
  ("partial-rollback", "partial-rollback", 1, 1, 500, "", "", "2048 4"),
  ]

# Currently these only work for eager VM TM systems
eagervm_transactional_runs = [
  ("compensation", "compensation", 1, 1, 500, "", "", ""),
  ("commit-action", "commit-action", 1, 1, 500, "", "", ""),
]

stamp_runs = [
  ("vacation", "vacation", 1, 1, 500, "", "TM-n8-q10-u80-r65536-t4096", "-n8 -q10 -u80 -r65536 -t4096"),
  ("delaunay", "delaunay", 1, 1, 500, "", "TM-gen4.2-m30", "-i inputs/gen4.2 -m 30"),
  ("genome", "genome", 1, 1, 500, "", "TM-g256-s16-n8192", "-g256 -s16 -n8192"),
  ("kmeans", "kmeans", 1, 1, 500, "", "TM-m20-n20-t0.05", "-m20 -n20 -t0.05 -i inputs/random1000_12"),
  ("bayes", "bayes", 1, 1, 500, "", "TM-v16-r384-n3-p20-s0", "-v16 -r384 -n3 -p20 -s0"),
  ("labyrinth", "labyrinth", 1, 1, 500, "", "TM-x32-y32-z3-n32", "random-x32-y32-z3-n32.txt")
]  

base_runs = [
  #("ocean-locks/ocean-locks-66",            "ocean-locks_66",         1,   1, 500, None),
  #("final-checkpoints/ocean-locks/ocean-locks-base-66",            "ocean-locks-base-66",         1,   1, 500, None),
  #("final-checkpoints/ocean-locks/ocean-locks-base-130",            "ocean-locks-base-130",         1,   1, 500, None),
  #("final-checkpoints/ocean-locks/ocean-locks-base-258",            "ocean-locks-base-258",         1,   1, 500, None),
  #("ocean-locks/ocean-locks-258",           "ocean-locks_258",        1,   1, 500, None),
  ]

nested_transactional_runs = [
  ("raytrace/raytrace-nested-trans-opt-teapot",      "raytrace-nested-trans-opt-teapot", 1,   1, 500, None),
  ("radiosity/radiosity-nested-trans",              "radiosity-nested-trans",           1,  1,  500 , None),
  ("cholesky/cholesky-nested-trans-14",         "cholesky-nested-trans-14",       1,   1, 500, None),
  ]

mcs_lock_runs = [
  ("raytrace/raytrace-nested-mcs-opt-teapot",      "raytrace-nested-mcs-opt-teapot", 1,   1, 500, None),
  ("radiosity/radiosity-nested-mcs",              "radiosity-nested-mcs",           1,  1,  500 , None),
  ]

###############################################################################
# Environment Variables Manipulations
#############################################
##################################

import string, os

# if you are going to assume the value presents in mfacet.py, you should
# make that "required = 1" here.
# NOTE: a empty env variable "" will be treated like a None, this is because
# python 2.1 does not support unsetenv, (or "del os.environ["key"]"). So we
# lose the ability to pass a "" as parameter before the lab upgrade to 2.2.
g_env_list = (
    # (string) name, (string) default value , required?
    ("INTERACTIVE", None, 0),
    ("OPAL_CONFIG_NAME", None, 0),
    ("OPAL_CONFIG_FILE", None, 0),
    ("CHECKPOINT_AT_END", None, 0),
    ("GENERATE_TRACE", None, 0),
    ("CACHE_DATA_DUMP", None, 0),
    ("PROTOCOL_OPTION", None, 0),
    ("NETWORK_TOPOLOGY", None, 0),
    ("WARMUP_FILE", None, 0),
    ("RANDOM_SEED", None, 0),
    ("WORKLOAD", None, 1),
    ("BANDWIDTH", None, 1),
    ("CHECKPOINT_DIR", None, 1),
    ("CHECKPOINT", None, 1),
    ("BENCHMARK", None, 0),
    ("LOCK_TYPE", None, 0),
    ("READ_SET", None, 0),
    ("MICROBENCH_DIR", "microbenchmarks/transactional", 0),
    ("MAX_DEPTH", "1", 0),
    ("ENABLE_TOURMALINE", "0", 0),
    ("XACT_LAZY_VM", "0", 0),
    ("XACT_EAGER_CD", "1", 0),
    ("XACT_VISUALIZER", "0", 0),
    ("XACT_STORE_PREDICTOR_ENTRIES", "0", 0),
    ("XACT_STORE_PREDICTOR_HISTORY", "0", 0),
    ("XACT_STORE_PREDICTOR_THRESHOLD", "0", 0),
    ("XACT_COMMIT_TOKEN_LATENCY", "0", 0),
    ("XACT_LOG_BUFFER_SIZE", "0", 0),
    ("XACT_NO_BACKOFF", "0", 0),
    ("XACT_FIRST_ACCESS_COST", "0", 0),
    ("XACT_FIRST_PAGE_ACCESS_COST", "0", 0),
    ("XACT_CONFLICT_RES", "BASE", 0),
    ("ENABLE_MAGIC_WAITING", "0", 0),
    ("XACT_ENABLE_VIRTUALIZATION_LOGTM_SE", "0", 0),
    ("PROFILE_EXCEPTIONS", "0", 0),
    ("XACT_DEBUG", "0", 0),
    ("PROFILE_XACT", "0", 0),
    ("PROFILE_NONXACT", "0", 0),
    ("ENABLE_WATCHPOINT", "0", 0),
    ("PROTOCOL", None, 1),
    ("PROCESSORS", None, 1),
    ("CHIPS", None, 1),
    ("PROCS_PER_CHIP", "1", 1),
    ("SMT_THREADS", 1, 1),
    ("NUM_L2_BANKS", "0", 0),
    ("RESULTS_DIR", None, 1),
    ("TRANSACTIONS", None, 1),
    ("DUMP_INTERVAL", "1", 1),
    ("CONDORCLUSTER", "1", 0),
    ("CONDORPROCESS", "1", 0),
    ("MBENCH_ARG_STRING", None, 0),
    ("MBENCH_ARG_PREFIX", None, 0),
    ("READ_WRITE_FILTER", "Perfect_", 0),
    ("VIRTUAL_READ_WRITE_FILTER", "Perfect_", 0),
    ("SUMMARY_READ_WRITE_FILTER", "Perfect_", 0),
    ("BATCH_SIZE", None, 0),
    ("USE_LOCAL_MIRROR", None, 0),
    ("LM_LICENSE_FILE", "/p/multifacet/projects/simics/licenses/license.dat", 1),
    ("SIMICS_EXTRA_LIB", "./modules", 1),
    #("SIMICS_HOST", "x86-linux", 1),
    #("LD_ASSUME_KERNEL", "2.4.1", 1),
    ("MACHTYPE", "i386", 1),
    ("SHELL", "/bin/tcsh", 1),
    ("PATH", "s/std/bin:/usr/afsws/bin:/usr/ccs/bin:/usr/ucb:/bin:/usr/bin:/usr/X11R6/bin:/unsup/condor/bin:.", 1),
)

# check invariants of a dictionary w.r.t. g_env_list
def check_requirements(env_dict):
  assert(env_dict)
  # make sure all variables are defined
  for i in g_env_list:
    if i[2] == 0:
      # make sure it at least is in the dictionary
      if (not env_dict.has_key(i[0])):
        print "Error: %s is not in the dictionary"%i[0]
        assert(0)
    elif i[2] == 1:
      # make sure the key is in the dictionary and not None
      if not (env_dict.has_key(i[0]) and env_dict[i[0]] != None):
        print "Error: required key %s missing"%i[0]
        assert(0)
    else:
      assert(0)
  # make sure no extra variables are defined
  assert(len(g_env_list) == len(env_dict))
  return

# returns a directory contains all env vars and their default value
def prepare_env_dictionary(simics):
  env_dict = {}
  for i in g_env_list:
    key = string.upper(i[0])
    assert(not env_dict.has_key(key))
    if(simics == 1):
      # get values from system ENV, both not_set & "" get None
      env_dict[key] = os.environ.get(key, i[1])
      if (env_dict[key] == ""): env_dict[key] = None
    else:
      # set default value
      env_dict[key] = i[1]
  return env_dict

# set some key's value after initilization
# note value is converted to string before inserted into the directory
def set_var(env_dict, key, value):
  assert(env_dict and env_dict.has_key(key))
  if(value == None or str(value) == ""):
    env_dict[key] = None
  else:
    env_dict[key] = str(value)
  return

# get some key's value after initilization
def get_var(env_dict, key):
  assert(env_dict and env_dict.has_key(key))
  assert(env_dict[key] != "")
  return env_dict[key]

# return condor env string
def get_condor_env_string(env_dict):
  li = []
  for k in env_dict.keys():
    if(env_dict[k] != None):
      # only output not None values, since in the condor, nothing else could
      # mess-up the env values
      li.append("%s=%s"%(k, env_dict[k]))
  return string.join(li, ';')

# return shell setenv string
def get_shell_setenv_string(env_dict):
  li = []
  for k in env_dict.keys():
    # make sure we overwrite the old env value
    if(env_dict[k] != None):
      # km - ugly hack
      if(k == "MBENCH_ARG_STRING"):
        li.append("export %s='%s'"%(k, env_dict[k]))
      else:
        li.append("export %s=%s"%(k, env_dict[k]))
    else:
      li.append("export %s=%s"%(k, ""))
  return string.join(li, '\n')

# put all variables to system environment
def update_system_env(env_dict):
  check_requirements(env_dict)
  for k in env_dict.keys():
    # make sure we overwrite the old env value
    if(env_dict[k] != None):
      os.environ[k] = env_dict[k]
    else:
      if os.environ.has_key(k):
        os.environ[k] = ""
        del os.environ[k]
  return

# print all variables to stdout
def print_all_variables(env_dict):
  #check_requirements(env_dict)
  for k in env_dict.keys():
    print "%30s    "%k,
    print env_dict[k]
  return

# get output filename prefix from environment variables
def get_output_file_name_prefix(env_dict, condor):
  workload_name = get_var(env_dict, "WORKLOAD")
  chips = int(get_var(env_dict, "CHIPS"))
  procs_per_chip = int(get_var(env_dict, "PROCS_PER_CHIP"))
  smt_threads =  int(get_var(env_dict, "SMT_THREADS"))
  opal_config_name = get_var(env_dict, "OPAL_CONFIG_NAME")
  protocol = get_var(env_dict, "PROTOCOL")
  protocol_options = get_var(env_dict, "PROTOCOL_OPTION")
  bandwidth = int(get_var(env_dict, "BANDWIDTH"))
  if(condor == 1):
    return "%s/%s-%dc-%dp-%dt-%s-%s-%s-%d-$(Cluster)-$(Process)" % (workload_name, workload_name, chips, procs_per_chip, smt_threads, protocol, protocol_options, opal_config_name, bandwidth)
  else:
    condor_cluster = int(get_var(env_dict, "CONDORCLUSTER"))
    condor_process = int(get_var(env_dict, "CONDORPROCESS"))
    return "%s/%s-%dc-%dp-%dt-%s-%s-%s-%d-%d-%d" % (workload_name, workload_name, chips, procs_per_chip, smt_threads, protocol, protocol_options, opal_config_name, bandwidth, condor_cluster, condor_process)

def get_script_file_name(env_dict):
  workload_name = get_var(env_dict, "WORKLOAD")
  chips = int(get_var(env_dict, "CHIPS"))
  procs_per_chip = int(get_var(env_dict, "PROCS_PER_CHIP"))
  smt_threads =  int(get_var(env_dict, "SMT_THREADS"))
  opal_config_name = get_var(env_dict, "OPAL_CONFIG_NAME")
  protocol = get_var(env_dict, "PROTOCOL")
  protocol_options = get_var(env_dict, "PROTOCOL_OPTION")
  bandwidth = int(get_var(env_dict, "BANDWIDTH"))
  
  script_filename = "%s_%dc_%dp_%dt_%s_%s_%s_%s.sh" % (
    workload_name,
    chips,
    procs_per_chip,
    smt_threads,
    protocol,
    protocol_option,
    opal_config_name,
    bandwidth)
  return script_filename

def get_microbench_output_file_name_prefix(env_dict, condor):
  workload_name = get_var(env_dict, "WORKLOAD")
  chips = int(get_var(env_dict, "CHIPS"))
  processors = int(get_var(env_dict, "PROCESSORS"))
  procs_per_chip = int(get_var(env_dict, "PROCS_PER_CHIP"))
  smt_threads = int(get_var(env_dict, "SMT_THREADS"))
  filter_config = get_var(env_dict, "READ_WRITE_FILTER")
  virtual_filter_config = get_var(env_dict, "VIRTUAL_READ_WRITE_FILTER")
  summary_filter_config = get_var(env_dict, "SUMMARY_READ_WRITE_FILTER")
  opal_config_name = get_var(env_dict, "OPAL_CONFIG_NAME")
  protocol_option = get_var(env_dict, "PROTOCOL_OPTION")
  bandwidth = int(get_var(env_dict, "BANDWIDTH"))
  arg_prefix = get_var(env_dict, "MBENCH_ARG_PREFIX")
  if(condor == 1):
    return "%s-%s/%s-%s-%dc-%dp-%dt-%s-%s-%s-%s-%s-%d-$(Cluster)-$(Process)" % (workload_name, arg_prefix, workload_name, arg_prefix, chips, procs_per_chip, smt_threads,opal_config_name, protocol_option, filter_config, virtual_filter_config, summary_filter_config, bandwidth)
  else:
    condor_cluster = int(get_var(env_dict, "CONDORCLUSTER"))
    condor_process = int(get_var(env_dict, "CONDORPROCESS"))
    return "%s-%s/%s-%s-%dp-%dt-%s-%s-%s-%s-%s-%d-%d-%d" % (workload_name, arg_prefix, workload_name, arg_prefix, processors, smt_threads, opal_config_name, protocol_option, filter_config, virtual_filter_config, summary_filter_config, bandwidth, condor_cluster, condor_process)

def get_microbench_script_file_name(env_dict):
  workload_name = get_var(env_dict, "WORKLOAD")
  arg_prefix = get_var(env_dict, "MBENCH_ARG_PREFIX")
  chips = int(get_var(env_dict, "CHIPS"))
  procs_per_chip = int(get_var(env_dict, "PROCS_PER_CHIP"))
  smt_threads = int(get_var(env_dict, "SMT_THREADS"))
  filter_config = get_var(env_dict, "READ_WRITE_FILTER")
  virtual_filter_config = get_var(env_dict, "VIRTUAL_READ_WRITE_FILTER")
  summary_filter_config = get_var(env_dict, "SUMMARY_READ_WRITE_FILTER")
  opal_config_name = get_var(env_dict, "OPAL_CONFIG_NAME")
  protocol_option = get_var(env_dict, "PROTOCOL_OPTION")
  bandwidth = int(get_var(env_dict, "BANDWIDTH"))
  return "%s-%s-%dc-%dp-%dt-%s-%s-%s-%s-%s-%d.sh" % (workload_name, arg_prefix, chips, procs_per_chip, smt_threads, opal_config_name, protocol_option, filter_config, virtual_filter_config, summary_filter_config, bandwidth)


# test this module
def test():
  env_dict = prepare_env_dictionary(simics = 0)
  set_var(env_dict, "WORKLOAD", 100)
  set_var(env_dict, "BANDWIDTH", 100)
  set_var(env_dict, "CHECKPOINT", 100)
  set_var(env_dict, "PROTOCOL", 100)
  set_var(env_dict, "CHIPS", 1)
  set_var(env_dict, "PROCS_PER_CHP", 100)
  set_var(env_dict, "SMT_THREADS", 1)
  set_var(env_dict, "RESULTS_DIR", 100)
  set_var(env_dict, "TRANSACTIONS", 100)
  set_var(env_dict, "DUMP_INTERVAL", 100)
  set_var(env_dict, "CONDORCLUSTER", 100)
  set_var(env_dict, "CONDORPROCESS", 100)
  print get_var(env_dict, "DUMP_INTERVAL")
  print get_condor_env_string(env_dict)
  print get_shell_setenv_string(env_dict)
  set_var(env_dict, "DUMP_INTERVAL", 222)
  set_var(env_dict, "INTERACTIVE", "")
  update_system_env(env_dict)
  env_dict = prepare_env_dictionary(simics = 1)
  print_all_variables(env_dict)
  print get_var(env_dict, "DUMP_INTERVAL")
  print get_condor_env_string(env_dict)
  print get_shell_setenv_string(env_dict)

# for debug
#test()
